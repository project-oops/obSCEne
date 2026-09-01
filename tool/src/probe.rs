//! Building minimal modules that ask a loader a question.
//!
//! The vendor dynamic tag values are not in any documentation this project could find,
//! and the obvious source is another project's parser - which this repository does not
//! read. The alternative to reading them is not guessing: a loader reports the tags it
//! does not recognise and **names** some of those it does, which makes it answerable.
//!
//! # One tag per probe
//!
//! The first attempt put every candidate in one table. A recognised tag makes the loader
//! act on its value, the third one it acted on faulted, and every later verdict was lost -
//! leaving a result that looked like fifty-seven recognised tags and was actually a
//! crash. A probe that stops early and a probe that finds nothing produce identical
//! silence.
//!
//! So: one candidate per module, with a control after it. If the control is reported,
//! the run reached the end and the candidate's silence means recognition rather than a
//! crash.
//!
//! # Values are chosen to be harmless, or to be informative
//!
//! Zero keeps a recognised tag from sending the loader after a table that is not there.
//! A caller chasing a *named* failure can supply a value instead and watch whether the
//! message changes - which identifies a tag rather than merely proving it is known.

use crate::elf::{ELFOSABI_FREEBSD, ET_SCE_DYNAMIC, PT_DYNAMIC, PT_LOAD};

/// The vendor process-parameter segment. A loader reads it before transferring control.
const PT_SCE_PROC_PARAM: u32 = 0x6100_0001;

/// Bytes reserved for the structure. Only the leading size field is asserted; the rest
/// is zero rather than guessed.
const PROC_PARAM_SIZE: usize = 0x50;

/// Standard tags a loader is known to reject, used as controls.
///
/// If these are reported, the run reached the end of the table. If they are not, it
/// stopped early and every verdict after the candidate is worthless.
const CONTROL_TAGS: [u64; 3] = [0x05, 0x06, 0x07];

const EHDR_SIZE: usize = 64;
const PHDR_SIZE: usize = 56;
const PHDR_COUNT: usize = 2;

// The same three as u16, so the header is written without a cast. Casting a usize into
// a header field is how the wrong value gets there quietly.
const EHDR_SIZE_U16: u16 = 64;
const PHDR_SIZE_U16: u16 = 56;
const PHDR_COUNT_U16: u16 = 2;
const EM_X86_64: u16 = 0x3E;
const PAGE: u64 = 0x4000;

/// Builds a module that imports nothing and returns immediately.
///
/// The control for every other question about loading. A module that fails to run has
/// many possible reasons - a rejected header, an unmappable segment, an unresolvable
/// import, a bad entry - and they are indistinguishable from "it did not work". This
/// removes all but one: there is nothing to resolve, so if it runs, everything except
/// import resolution is known good.
///
/// # `ret` versus spin, and why both exist
///
/// With `ret` the body returns immediately - which is what obSCEne's own entry does, so
/// it also checks a loader tolerates it. But a crash afterwards is ambiguous: returning
/// from an entry with nothing meaningful on the stack pops a garbage address and faults,
/// which looks identical to never having run at all.
///
/// `spin` emits an infinite loop instead. If the emulator hangs, the guest's own
/// instructions are executing - there is no other way to reach that state. That turns
/// "probably ran" into a fact.
#[must_use]
pub fn minimal(spin: bool, proc_param: bool) -> Vec<u8> {
    const RET: u8 = 0xC3;
    // jmp -2: a two-byte branch to itself.
    const SPIN: [u8; 2] = [0xEB, 0xFE];

    let body: &[u8] = if spin { &SPIN } else { &[RET] };
    // Kept as u16 as well as usize, so the header field is written without a cast.
    // Casting into a header field is how a wrong value gets there quietly.
    let (phdr_count, phdr_count_u16): (usize, u16) = if proc_param { (2, 2) } else { (1, 1) };
    let headers = EHDR_SIZE.saturating_add(PHDR_SIZE.saturating_mul(phdr_count));
    let code_offset = headers.next_multiple_of(16);
    let param_offset = code_offset.saturating_add(body.len()).next_multiple_of(8);
    let total = if proc_param {
        param_offset.saturating_add(PROC_PARAM_SIZE)
    } else {
        code_offset.saturating_add(body.len())
    };

    let mut out = vec![0_u8; EHDR_SIZE];
    write_bytes(&mut out, 0, b"\x7fELF");
    set(&mut out, 4, 2);
    set(&mut out, 5, 1);
    set(&mut out, 6, 1);
    set(&mut out, 7, ELFOSABI_FREEBSD);
    write_u16(&mut out, 0x10, ET_SCE_DYNAMIC);
    write_u16(&mut out, 0x12, EM_X86_64);
    write_u32(&mut out, 0x14, 1);
    write_u64(&mut out, 0x18, code_offset as u64);
    write_u64(&mut out, 0x20, EHDR_SIZE as u64);
    write_u16(&mut out, 0x34, EHDR_SIZE_U16);
    write_u16(&mut out, 0x36, PHDR_SIZE_U16);
    write_u16(&mut out, 0x38, phdr_count_u16);

    // One loadable, readable and executable segment. No dynamic segment at all: a
    // module with nothing to link needs none, and its absence is the point.
    out.extend(program_header(PT_LOAD, 0x5, 0, total as u64, PAGE));
    if proc_param {
        out.extend(program_header(
            PT_SCE_PROC_PARAM,
            0x4,
            param_offset as u64,
            PROC_PARAM_SIZE as u64,
            8,
        ));
    }
    out.resize(code_offset, 0);
    out.extend_from_slice(body);

    if proc_param {
        out.resize(param_offset, 0);
        // Only the size is asserted. Everything after it is zero, which keeps this an
        // experiment about whether the segment must *exist* or must *contain* something.
        out.extend_from_slice(&(PROC_PARAM_SIZE as u64).to_le_bytes());
        out.resize(param_offset.saturating_add(PROC_PARAM_SIZE), 0);
    }
    out
}

/// Builds a probe module carrying one candidate tag.
#[must_use]
pub fn build(tag: u64, value: u64) -> Vec<u8> {
    let mut entries = Vec::new();
    push_entry(&mut entries, tag, value);
    for control in CONTROL_TAGS {
        push_entry(&mut entries, control, 0);
    }
    push_entry(&mut entries, 0, 0);

    let headers = EHDR_SIZE.saturating_add(PHDR_SIZE.saturating_mul(PHDR_COUNT));
    let dyn_offset = headers.next_multiple_of(16);
    let total = dyn_offset.saturating_add(entries.len());

    let mut out = vec![0_u8; EHDR_SIZE];
    write_bytes(&mut out, 0, b"\x7fELF");
    set(&mut out, 4, 2); // 64-bit
    set(&mut out, 5, 1); // little-endian
    set(&mut out, 6, 1); // version
    set(&mut out, 7, ELFOSABI_FREEBSD);
    write_u16(&mut out, 0x10, ET_SCE_DYNAMIC);
    write_u16(&mut out, 0x12, EM_X86_64);
    write_u32(&mut out, 0x14, 1);
    // Entry points into the mapped page. Nothing runs it: the loader gives up long
    // before transferring control, which is the point of the probe.
    write_u64(&mut out, 0x18, dyn_offset as u64);
    write_u64(&mut out, 0x20, EHDR_SIZE as u64);
    // 0x34 e_ehsize, 0x36 e_phentsize, 0x38 e_phnum - in that order, and writing them
    // one slot along makes e_phnum the header size, which sends a parser looking for
    // fifty-six program headers in a small file.
    write_u16(&mut out, 0x34, EHDR_SIZE_U16);
    write_u16(&mut out, 0x36, PHDR_SIZE_U16);
    write_u16(&mut out, 0x38, PHDR_COUNT_U16);

    out.extend(program_header(PT_LOAD, 0x5, 0, total as u64, PAGE));
    out.extend(program_header(
        PT_DYNAMIC,
        0x6,
        dyn_offset as u64,
        entries.len() as u64,
        8,
    ));
    out.resize(dyn_offset, 0);
    out.extend_from_slice(&entries);
    out
}

fn program_header(p_type: u32, flags: u32, offset: u64, size: u64, align: u64) -> Vec<u8> {
    let mut out = Vec::with_capacity(PHDR_SIZE);
    out.extend_from_slice(&p_type.to_le_bytes());
    out.extend_from_slice(&flags.to_le_bytes());
    out.extend_from_slice(&offset.to_le_bytes());
    out.extend_from_slice(&offset.to_le_bytes()); // vaddr, identity-mapped
    out.extend_from_slice(&offset.to_le_bytes()); // paddr
    out.extend_from_slice(&size.to_le_bytes());
    out.extend_from_slice(&size.to_le_bytes());
    out.extend_from_slice(&align.to_le_bytes());
    out
}

fn push_entry(out: &mut Vec<u8>, tag: u64, value: u64) {
    out.extend_from_slice(&tag.to_le_bytes());
    out.extend_from_slice(&value.to_le_bytes());
}

fn set(out: &mut [u8], at: usize, value: u8) {
    if let Some(slot) = out.get_mut(at) {
        *slot = value;
    }
}

fn write_bytes(out: &mut [u8], at: usize, value: &[u8]) {
    if let Some(slot) = out.get_mut(at..at.saturating_add(value.len())) {
        slot.copy_from_slice(value);
    }
}

fn write_u16(out: &mut [u8], at: usize, value: u16) {
    write_bytes(out, at, &value.to_le_bytes());
}

fn write_u32(out: &mut [u8], at: usize, value: u32) {
    write_bytes(out, at, &value.to_le_bytes());
}

fn write_u64(out: &mut [u8], at: usize, value: u64) {
    write_bytes(out, at, &value.to_le_bytes());
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
    use super::{CONTROL_TAGS, build};
    use crate::elf::{ET_SCE_DYNAMIC, Elf, PT_DYNAMIC};

    #[test]
    fn a_probe_is_a_module_a_loader_will_accept() {
        // If the probe is rejected on its header, it answers nothing about tags.
        let bytes = build(0x6100_0025, 0);
        let elf = Elf::parse(&bytes).expect("parse");
        assert_eq!(elf.e_type, ET_SCE_DYNAMIC);
        assert!(elf.segment(PT_DYNAMIC).is_some());
    }

    #[test]
    fn the_candidate_comes_first_and_the_controls_follow() {
        // Order matters: a control reported after the candidate proves the run reached
        // the end, which is what separates "recognised" from "crashed".
        let bytes = build(0x6100_0025, 0);
        let elf = Elf::parse(&bytes).expect("parse");
        let entries = elf.dynamic().expect("dynamic");
        assert_eq!(entries[0].tag, 0x6100_0025);
        for (index, control) in CONTROL_TAGS.iter().enumerate() {
            assert_eq!(entries[index + 1].tag, *control);
        }
    }

    #[test]
    fn a_value_can_be_supplied_for_a_differential_probe() {
        let bytes = build(0x6100_0009, 0xDEAD);
        let elf = Elf::parse(&bytes).expect("parse");
        assert_eq!(elf.dynamic().expect("dynamic")[0].value, 0xDEAD);
    }

    #[test]
    fn the_table_terminates() {
        // Without a terminator the loader reads whatever follows as more entries.
        let bytes = build(0x6100_0025, 0);
        let elf = Elf::parse(&bytes).expect("parse");
        assert_eq!(
            elf.dynamic().expect("dynamic").len(),
            1 + CONTROL_TAGS.len()
        );
    }
}
