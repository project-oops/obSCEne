//! Re-deriving the vendor dynamic tag assignment from a module.
//!
//! The fourteen tag values in [`crate::dynlib::tag`] are not documented anywhere this
//! project may read. They were worked out from a reference module by arithmetic: the
//! tables in the vendor dynamic segment are laid end to end, so an offset plus the size
//! that follows it lands exactly on the next offset. Which tag is which follows from
//! which sums come out.
//!
//! That reasoning was written down, and prose rots. This runs it.
//!
//! # It needs no reference module
//!
//! The reference is gone - it was a toolchain we consulted and deleted. What replaced it
//! is that `mkmodule` lays our own tables out in the same order, so the same sums hold in
//! our own output. `obscene-tool derive` on a module we just built re-checks the whole
//! chain, which means the derivation is exercised on every build rather than resting on a
//! file nobody has any more.
//!
//! Point it at a genuine vendor module and it does the original derivation instead.
//!
//! # What a failure means
//!
//! A relation that does not hold has two possible causes, and this cannot tell them
//! apart: the constants are wrong, or the module is not laid out the way we think. Both
//! are worth stopping for, which is why either exits non-zero.

use selfish_elf::dynamic::{self as tag, Table, Tags};

use crate::elf::{Elf, PT_DYNAMIC, PT_LOAD, PT_SCE_DYNLIBDATA};

/// One arithmetic claim about the layout.
#[derive(Debug, Clone)]
pub struct Relation {
    /// What the relation asserts, in the form a reader can check by eye.
    pub claim: String,
    /// The two sides, evaluated.
    pub left: u64,
    /// The right-hand side.
    pub right: u64,
    /// Which tag the relation identifies, if it holds.
    ///
    /// Not the same as what is on the right of the sum, though the first version of
    /// this used one field for both: `STRTAB + STRSZ == SYMTAB` lands on SYMTAB and
    /// identifies STRSZ. Conflating them printed relations that read as tautologies.
    pub identifies: &'static str,
    /// Whether it held.
    pub holds: bool,
    /// Set when the relation could not be checked because a tag is not declared.
    ///
    /// Not the same as failing. A module that declares no relocations has no JMPREL to
    /// land on, and treating that as a broken layout would fail every module that
    /// happens not to need one - while treating it as a pass would let a genuinely
    /// missing tag through. It is reported as neither.
    pub skipped: bool,
}

/// What a run found.
#[derive(Debug, Default)]
pub struct Derivation {
    /// The relations, in layout order.
    pub relations: Vec<Relation>,
    /// Tags present in the module that we have no name for.
    pub unassigned: Vec<u64>,
    /// Roles two tags share a value with, and so cannot be told apart by arithmetic.
    pub ambiguous: Vec<&'static str>,
    /// Set when the module carries no vendor segment to reason about.
    pub not_a_module: bool,
}

impl Derivation {
    /// Whether every relation that could be checked held.
    ///
    /// A run where everything was skipped is not consistent: that is a module with no
    /// layout to check, and reporting it as reproduced would be a lie of omission.
    #[must_use]
    pub fn is_consistent(&self) -> bool {
        !self.not_a_module
            && self.relations.iter().any(|relation| !relation.skipped)
            && self
                .relations
                .iter()
                .all(|relation| relation.holds || relation.skipped)
    }
}

/// How much of the vendor segment's tail the dynamic table occupies, or zero when it is
/// elsewhere.
///
/// A console executable's `PT_DYNAMIC` carries no address and lies inside the vendor segment,
/// ending on the same byte (selfish D076). A module built for a loader that maps it keeps the
/// table in the image instead, and then this is zero and the last relation lands on the end of
/// the segment as it always did.
///
/// Measured from the headers rather than assumed, so a module laid out either way is checked
/// against how it actually is.
fn dynamic_tail(elf: &Elf, segment: &crate::elf::ProgramHeader) -> u64 {
    elf.program_headers
        .iter()
        .find(|header| header.p_type == PT_DYNAMIC)
        .filter(|header| {
            let segment_end = segment.offset.saturating_add(segment.filesz);
            header.offset >= segment.offset
                && header.offset.saturating_add(header.filesz) == segment_end
        })
        .map_or(0, |header| header.filesz)
}

/// The three relations that are not arithmetic.
///
/// Two entry sizes and a relocation form. They are not offsets, so the chain of sums says
/// nothing about them - they are pinned by their values instead. Split out of `run_with`
/// because that function is a sequence of independent assertions and the line limit is a
/// reasonable place to be told so.
fn fixed_values(out: &mut Derivation, t: &Tags, value: &impl Fn(u64) -> Option<u64>) {
    push(
        out,
        "PLTREL names the relocation form",
        value(t.pltrel),
        Some(selfish_elf::dynlib::version::RELA_FORM),
        "DT_SCE_PLTREL",
    );
    push(
        out,
        "SYMENT is one symbol",
        value(t.syment),
        Some(selfish_elf::section::SYMBOL_SIZE as u64),
        "DT_SCE_SYMENT",
    );
    push(
        out,
        "RELAENT is one relocation",
        value(t.relaent),
        Some(selfish_elf::reloc::RELA_SIZE as u64),
        "DT_SCE_RELAENT",
    );
}

/// Runs the derivation against a parsed module, reading the convention from the module.
///
/// `Tags::detect` decides, because a reader is handed a file rather than a build flag. A
/// caller that is *writing* a module knows which convention it chose and says so with
/// `run_with`; a caller inspecting someone else's module does not, and guessing wrong there
/// reports "nothing to derive from" about a perfectly good module. (D193)
#[must_use]
pub fn run(elf: &Elf) -> Derivation {
    // Legacy when a module declares neither convention. A reader handed something that is
    // not a vendor module at all has nothing to derive from either way, and the legacy
    // tables are what this project writes.
    let entries = elf.dynamic().map(|entries| {
        entries
            .iter()
            .map(|entry| (entry.tag, entry.value))
            .collect::<Vec<_>>()
    });
    let table = entries
        .ok()
        .and_then(|entries| Tags::detect(&entries))
        .unwrap_or(Table::Legacy);
    run_with(elf, table)
}

/// Re-derives the tag assignment, checked against the convention the module claims.
///
/// The relations are arithmetic on offsets and sizes and they hold under either convention -
/// what changes is *which numbers carry them*. So the tag set is a parameter, and a module
/// written one way is checked against that way rather than against whichever was hardcoded
/// here. Passing the wrong one produces a wall of failed relations, which is the correct
/// outcome: it means the module is not laid out the way the caller thinks. (D193)
#[must_use]
pub fn run_with(elf: &Elf, table: Table) -> Derivation {
    let t = Tags::of(table);
    let mut out = Derivation::default();

    let Ok(entries) = elf.dynamic() else {
        out.not_a_module = true;
        return out;
    };

    // Where the tables live, which is a different segment under each convention.
    //
    // Previous-generation: a `PT_SCE_DYNLIBDATA` that is never mapped, and the tag values are
    // offsets into it. Current-generation: an ordinary `PT_LOAD`, and the tag values are
    // addresses - so the segment is found *through* the string table's address rather than by
    // type, because nothing marks it as special. That is not a weakness of the check: a retail
    // module carries no marker either, and looking it up the way a loader would is the point.
    // (D193)
    let segment = match table {
        Table::Legacy => elf.segment(PT_SCE_DYNLIBDATA),
        Table::Current => entries
            .iter()
            .find(|entry| entry.tag == t.strtab)
            .and_then(|entry| {
                elf.program_headers.iter().find(|header| {
                    header.p_type == PT_LOAD
                        && entry.value >= header.vaddr
                        && entry.value < header.vaddr.saturating_add(header.memsz)
                })
            }),
    };
    let Some(segment) = segment else {
        out.not_a_module = true;
        return out;
    };

    // What a table tag's value is measured from.
    //
    // Zero under the previous-generation convention, because the values are offsets into a
    // segment that is never mapped. The segment's own address under the current one, because
    // there the values are addresses.
    //
    // Branched rather than inferred from the address being zero. That shortcut passed against
    // real modules - a `PT_SCE_DYNLIBDATA` does carry `vaddr=0` - and failed against the
    // fixtures here, which place a previous-generation segment at 400 precisely because the
    // address is not supposed to matter. The tests were right and the shortcut was a
    // coincidence dressed as economy. (D193)
    let base = match table {
        Table::Legacy => 0,
        Table::Current => segment.vaddr,
    };

    // How much of the segment's tail the dynamic table occupies, or zero when it sits
    // elsewhere. Measured from the headers rather than assumed, so a module built either way
    // is checked against how it is actually laid out. See the last link below.
    let dynamic_tail = dynamic_tail(elf, segment);

    let value = |wanted: u64| -> Option<u64> {
        entries
            .iter()
            .find(|entry| entry.tag == wanted)
            .map(|entry| entry.value)
    };

    // The string table is the first *table* in the segment, but not the first thing in it:
    // a fingerprint region sits ahead of it, and `DT_SCE_FINGERPRINT` carries that region's
    // offset - which is the start of the segment. Everything else is an offset measured from
    // the same base, which is what makes the sums below meaningful.
    //
    // This asserted that STRTAB itself was the start of the segment, which held only while
    // nothing reserved the head. A real executable puts its string table at `0x18`. (D217)
    push(
        &mut out,
        "FINGERPRINT is the start of the segment",
        value(tag::vendor::FINGERPRINT),
        Some(base),
        "DT_SCE_FINGERPRINT",
    );
    push(
        &mut out,
        "STRTAB follows the fingerprint",
        value(t.strtab),
        Some(base.saturating_add(selfish_elf::dynlib::FINGERPRINT_SIZE)),
        "DT_SCE_STRTAB",
    );

    chain_relations(&mut out, &t, &value, base, segment.filesz, dynamic_tail);

    fixed_values(&mut out, &t, &value);

    // Both carry 0x18 and neither appears in a sum, so arithmetic cannot separate them.
    // Said out loud rather than left implied: a derivation that quietly claims more than
    // it establishes is worse than one that stops short.
    out.ambiguous
        .push("DT_SCE_SYMENT and DT_SCE_RELAENT both hold 0x18 and neither appears in a sum");

    // The one that was missed, and it cost real time.
    //
    // Every link below is an offset plus a size, and addition is commutative - so a
    // sum lands on the right answer whichever of the pair is the offset. The
    // derivation reported these as established for weeks while JMPREL and PLTRELSZ
    // were the wrong way round, because it literally cannot tell.
    //
    // The symptom was a loader reading relocations from the start of the string table
    // and reporting "types" that were four bytes of ASCII, which reads as a table
    // pointer problem rather than as two tag numbers being swapped.
    out.ambiguous.push(
        "a sum cannot say which of a pair is the offset and which the size:          JMPREL/PLTRELSZ, RELA/RELASZ, STRTAB/STRSZ, SYMTAB/SYMTABSZ and HASH/HASHSZ          are each fixed only by their values being plausible, not by the arithmetic",
    );

    out.unassigned = entries
        .iter()
        .map(|entry| entry.tag)
        .filter(|entry_tag| {
            *entry_tag >= u64::from(PT_SCE_DYNLIBDATA) && tag::tag_name(*entry_tag).is_none()
        })
        .collect();

    out
}

/// One step of the chain: a table, its size, and where the next one starts.
struct Link {
    /// Tag holding the offset this step starts from.
    base: u64,
    /// Tag holding the size of the table there.
    size: u64,
    /// Where the next table starts, if that tag is present.
    lands_on: Option<u64>,
    /// What to call the right-hand side when printing.
    lands_on_name: &'static str,
    /// Which tag this step pins down. Not the same as `lands_on_name`.
    identifies: &'static str,
    /// Whether the next table may begin after a few bytes of alignment padding.
    padded: bool,
}

/// Records one step of the chain.
///
/// `padded` allows the next table to start on an alignment boundary rather than exactly
/// where the previous one ended. Only tables whose sizes are not a multiple of the
/// alignment need it; the rest get the exact check, which is much stronger evidence.
fn sum(out: &mut Derivation, base: Option<u64>, size: Option<u64>, link: &Link) {
    let named = |tag| {
        tag::tag_name(tag)
            .unwrap_or("?")
            .trim_start_matches("DT_SCE_")
    };
    let base_name = named(link.base);
    let size_name = named(link.size);
    let claim = format!("{base_name} + {size_name} == {}", link.lands_on_name);

    // A table that is not declared cannot be checked; a table that *is* declared and
    // is missing its size is broken. Those are different, and collapsing them would
    // either fail every module with no relocations or let a genuinely missing tag
    // through.
    //
    // `base` absent means the table itself was never declared. `lands_on` absent means
    // the next table was not, so there is nothing for this one to end at. Either way
    // there is no claim to test. Anything else missing is a hole in a declaration that
    // was made, and that is a failure.
    if base.is_none() || link.lands_on.is_none() {
        out.relations.push(Relation {
            claim: format!("{claim} (not declared)"),
            left: 0,
            right: 0,
            identifies: link.identifies,
            holds: false,
            skipped: true,
        });
        return;
    }
    let (Some(base), Some(size), Some(expected)) = (base, size, link.lands_on) else {
        out.relations.push(Relation {
            claim: format!("{claim} (declared, but a tag is missing)"),
            left: 0,
            right: 0,
            identifies: link.identifies,
            holds: false,
            skipped: false,
        });
        return;
    };
    let left = base.saturating_add(size);
    let holds = if link.padded {
        // Landing at or just short of the next table is consistent with alignment.
        // Overshooting it is not, and would mean the tables overlap.
        left <= expected && expected.saturating_sub(left) < 16
    } else {
        left == expected
    };
    out.relations.push(Relation {
        claim,
        left,
        right: expected,
        identifies: link.identifies,
        holds,
        skipped: false,
    });
}

/// Records a relation of the form `value == expected`.
fn push(
    out: &mut Derivation,
    claim: &str,
    value: Option<u64>,
    expected: Option<u64>,
    identifies: &'static str,
) {
    let (Some(value), Some(expected)) = (value, expected) else {
        out.relations.push(Relation {
            claim: format!("{claim} (not declared)"),
            left: 0,
            right: 0,
            identifies,
            holds: false,
            skipped: true,
        });
        return;
    };
    out.relations.push(Relation {
        claim: claim.to_owned(),
        left: value,
        right: expected,
        identifies,
        holds: value == expected,
        skipped: false,
    });
}

/// The chain of sums that assigned the tags, checked against one module.
///
/// Split out of `run_with` because it is one argument made of eight assertions, and a function
/// that long stops being readable as an argument at all.
fn chain_relations(
    out: &mut Derivation,
    t: &Tags,
    value: &impl Fn(u64) -> Option<u64>,
    base: u64,
    segment_filesz: u64,
    dynamic_tail: u64,
) {
    // Each table is followed by the next, so an offset plus its size lands on the
    // offset that follows. These are the sums that assigned the tags: no other pairing
    // of the fourteen makes them all come out.
    //
    // A table rather than a run of calls, because the chain is the argument. Written as
    // separate calls it needed eight positional arguments apiece, and the two that name
    // things - what the sum lands on, and what it thereby identifies - got swapped.
    for link in [
        Link {
            base: t.strtab,
            size: t.strsz,
            lands_on: value(t.symtab),
            lands_on_name: "SYMTAB",
            identifies: "DT_SCE_STRSZ",
            padded: true,
        },
        Link {
            base: t.symtab,
            size: t.symtabsz,
            lands_on: value(t.jmprel),
            lands_on_name: "JMPREL",
            identifies: "DT_SCE_SYMTABSZ",
            padded: false,
        },
        Link {
            base: t.jmprel,
            size: t.pltrelsz,
            lands_on: value(t.rela),
            lands_on_name: "RELA",
            identifies: "DT_SCE_RELA",
            padded: false,
        },
        Link {
            base: t.rela,
            size: t.relasz,
            lands_on: value(t.hash),
            lands_on_name: "HASH",
            identifies: "DT_SCE_HASH",
            padded: false,
        },
        // The last table runs up to the dynamic table, which sits at the tail of the same
        // segment. That is the only length here coming from outside the dynamic table, so it
        // checks the whole chain against something the loader independently knows.
        //
        // This read "the end of the segment", and was right only while the dynamic table was
        // still left behind in a `PT_LOAD`. A real executable settles it: its hash ends at
        // `0x3320` and its vendor segment at `0x3760`, and the `0x440` between them is exactly
        // its `PT_DYNAMIC` - which carries no address and lies inside the vendor segment. So
        // the chain ends where the dynamic table begins, and the segment's own end is one
        // dynamic table further on. (D217)
        Link {
            base: t.hash,
            size: t.hashsz,
            lands_on: Some(
                base.saturating_add(segment_filesz)
                    .saturating_sub(dynamic_tail),
            ),
            lands_on_name: if dynamic_tail == 0 {
                "end of segment"
            } else {
                "the dynamic table at the segment's tail"
            },
            identifies: "DT_SCE_HASHSZ",
            padded: true,
        },
    ] {
        sum(out, value(link.base), value(link.size), &link);
    }
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    clippy::arithmetic_side_effects,
    clippy::cast_possible_truncation,
    reason = "test fixtures build known-size buffers; a panic here is the failure \
              signal, which is the opposite of what these lints guard in the tool"
)]
mod tests {
    use super::run;
    use crate::elf::{ELFOSABI_FREEBSD, Elf, PT_DYNAMIC, PT_SCE_DYNLIBDATA};
    use selfish_elf::dynamic::vendor as tag;

    /// Move one tag's value, found **by tag** rather than by position.
    ///
    /// The tests reached into `consistent()`'s vector by index, with the tag named only in a
    /// comment. Adding the fingerprint entry at the front shifted every index by one, so a
    /// test that meant to overrun STRSZ silently started adjusting STRTAB - and one of them
    /// went on passing, for a reason it was not testing.
    fn adjust(entries: &mut [(u64, u64)], wanted: u64, by: i64) {
        let entry = entries
            .iter_mut()
            .find(|(tag, _)| *tag == wanted)
            .expect("the fixture declares this tag");
        entry.1 = entry.1.wrapping_add_signed(by);
    }

    /// A module whose tables sit end to end, as a vendor one does.
    ///
    /// Sizes are chosen so the chain closes exactly on `dynlib_size`. Changing one
    /// without changing the next breaks a relation, which is what the tests below rely
    /// on to tell a working derivation from one that says yes to anything.
    fn consistent() -> (Vec<(u64, u64)>, u64) {
        let (strsz, symtabsz, pltrelsz, relasz, hashsz) = (0x100, 0x240, 0x60, 0x180, 0x80);
        // The string table does not start the segment; the fingerprint region does, and the
        // tables begin after it. A real executable puts its string table at 0x18. (D217)
        let strtab = selfish_elf::dynlib::FINGERPRINT_SIZE;
        let symtab = strtab + strsz;
        let jmprel = symtab + symtabsz;
        let rela = jmprel + pltrelsz;
        let hash = rela + relasz;
        (
            vec![
                (tag::FINGERPRINT, 0),
                (tag::STRTAB, strtab),
                (tag::STRSZ, strsz),
                (tag::SYMTAB, symtab),
                (tag::SYMTABSZ, symtabsz),
                (tag::JMPREL, jmprel),
                (tag::PLTRELSZ, pltrelsz),
                (tag::RELA, rela),
                (tag::RELASZ, relasz),
                (tag::HASH, hash),
                (tag::HASHSZ, hashsz),
                (tag::PLTREL, selfish_elf::dynlib::version::RELA_FORM),
                (tag::SYMENT, selfish_elf::section::SYMBOL_SIZE as u64),
                (tag::RELAENT, selfish_elf::reloc::RELA_SIZE as u64),
            ],
            hash + hashsz,
        )
    }

    fn module(entries: &[(u64, u64)], dynlib_size: u64) -> Vec<u8> {
        const EHDR: usize = 64;
        const PHDR: usize = 56;

        let mut table = Vec::new();
        for (tag, value) in entries {
            table.extend_from_slice(&tag.to_le_bytes());
            table.extend_from_slice(&value.to_le_bytes());
        }
        table.extend_from_slice(&0_u64.to_le_bytes());
        table.extend_from_slice(&0_u64.to_le_bytes());

        let dyn_offset = (EHDR + PHDR * 2) as u64;

        let mut out = vec![0_u8; EHDR];
        out[0..4].copy_from_slice(b"\x7fELF");
        out[4] = 2;
        out[5] = 1;
        out[6] = 1;
        out[7] = ELFOSABI_FREEBSD;
        out[0x10..0x12].copy_from_slice(&0xFE10_u16.to_le_bytes());
        out[0x12..0x14].copy_from_slice(&0x3E_u16.to_le_bytes());
        out[0x20..0x28].copy_from_slice(&(EHDR as u64).to_le_bytes());
        out[0x34..0x36].copy_from_slice(&(EHDR as u16).to_le_bytes());
        out[0x36..0x38].copy_from_slice(&(PHDR as u16).to_le_bytes());
        out[0x38..0x3A].copy_from_slice(&2_u16.to_le_bytes());

        for (p_type, offset, size) in [
            (PT_DYNAMIC, dyn_offset, table.len() as u64),
            (
                PT_SCE_DYNLIBDATA,
                dyn_offset + table.len() as u64,
                dynlib_size,
            ),
        ] {
            out.extend_from_slice(&p_type.to_le_bytes());
            out.extend_from_slice(&4_u32.to_le_bytes());
            for field in [offset, offset, offset] {
                out.extend_from_slice(&field.to_le_bytes());
            }
            out.extend_from_slice(&size.to_le_bytes());
            out.extend_from_slice(&size.to_le_bytes());
            out.extend_from_slice(&8_u64.to_le_bytes());
        }
        out.extend_from_slice(&table);
        out
    }

    #[test]
    fn a_module_laid_out_the_way_we_think_derives_cleanly() {
        let (entries, size) = consistent();
        let bytes = module(&entries, size);
        let out = run(&Elf::parse(&bytes).expect("parse"));
        assert!(out.is_consistent(), "relations: {:?}", out.relations);
    }

    #[test]
    fn a_table_that_does_not_meet_the_next_one_fails() {
        // The whole point. A checker that accepts this accepts anything, and would have
        // reported the assignment as reproduced no matter what the constants said.
        let (mut entries, size) = consistent();
        adjust(&mut entries, tag::STRSZ, 0x40); // now overrunning SYMTAB
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert!(!out.is_consistent());
        assert!(
            out.relations
                .iter()
                .any(|relation| !relation.holds && relation.identifies == "DT_SCE_STRSZ")
        );
    }

    #[test]
    fn a_chain_that_does_not_reach_the_end_of_the_segment_fails() {
        // The one length that comes from outside the dynamic table. Without this check
        // a self-consistent chain of wrong values would pass.
        let (entries, size) = consistent();
        let out = run(&Elf::parse(&module(&entries, size + 0x200)).expect("parse"));
        assert!(!out.is_consistent());
    }

    #[test]
    fn alignment_padding_between_tables_is_accepted() {
        // Real string tables are not multiples of the symbol alignment, so the next
        // table starts a few bytes later. Rejecting that would fail on every real module.
        let (mut entries, size) = consistent();
        adjust(&mut entries, tag::STRSZ, -3); // three bytes of padding before SYMTAB
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert!(out.is_consistent(), "relations: {:?}", out.relations);
    }

    #[test]
    fn a_missing_tag_fails_rather_than_being_skipped() {
        let (mut entries, size) = consistent();
        entries.retain(|(tag, _)| *tag != tag::RELASZ);
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert!(!out.is_consistent());
    }

    #[test]
    fn a_module_with_no_vendor_segment_derives_nothing() {
        // Distinguished from a failed derivation: there is no claim to check, which is
        // a different answer from "the claim is false".
        let (entries, _) = consistent();
        let mut bytes = module(&entries, 0x400);
        // Turn the vendor segment into something else.
        let phdr = 64 + 56;
        bytes[phdr..phdr + 4].copy_from_slice(&PT_DYNAMIC.to_le_bytes());
        let out = run(&Elf::parse(&bytes).expect("parse"));
        assert!(out.not_a_module);
        assert!(!out.is_consistent());
    }

    #[test]
    fn an_unnamed_vendor_tag_is_reported_rather_than_ignored() {
        // A tag we have no assignment for is an open question, and silence about it
        // reads as an answer.
        let (mut entries, size) = consistent();
        entries.push((0x6100_0009, 0));
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert_eq!(out.unassigned, vec![0x6100_0009]);
    }

    #[test]
    fn a_table_that_is_not_declared_at_all_is_skipped_rather_than_failed() {
        // Hidden visibility can leave a module with no procedure-linkage relocations,
        // and an empty table is not declared - so the link that lands on it has
        // nothing to check. Failing there would fail every such module.
        let (mut entries, size) = consistent();
        entries.retain(|(tag, _)| ![tag::JMPREL, tag::PLTRELSZ].contains(tag));
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert!(
            out.relations.iter().any(|relation| relation.skipped),
            "the JMPREL links should be skipped"
        );
        assert!(
            out.is_consistent(),
            "a module with no PLT relocations is still laid out correctly: {:?}",
            out.relations
        );
    }

    #[test]
    fn a_declared_table_missing_its_size_still_fails() {
        // The other half of the same distinction, and the reason it is not just
        // "absent means skip": RELA is declared here, so its size should be too.
        let (mut entries, size) = consistent();
        entries.retain(|(tag, _)| *tag != tag::RELASZ);
        let out = run(&Elf::parse(&module(&entries, size)).expect("parse"));
        assert!(!out.is_consistent());
    }
}
