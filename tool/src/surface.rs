//! The curated census: `include/obscene/surface.h`, generated from `data/surface.txt`.
//!
//! # What the census is, and why it is safe to hold
//!
//! Every name here is declared `const char`, so the type system forbids calling it. A census
//! asks "is this here?", which costs only a name; a behavioural check asks "does this work?",
//! which costs a confident signature. Two different instruments, and this is the cheap one.
//!
//! # The definition is data, the generator is code
//!
//! `data/surface.txt` holds the groups, their prose notes and their symbol lists. That is a
//! judgement about which names belong to which library, with several pages of prose attached,
//! and retyping it into another language is how a transcription error gets into a census.
//! This module assembles; the data file decides.
//!
//! Blocks open with `@<kind>` and close with `@end`. The terminator is explicit because an
//! earlier version used a blank line, and the reader then could not tell a separator from a
//! blank line the prose actually wanted - which silently split the libraries macro across two
//! lines.
//!
//! # Three refusals, each with a scar behind it
//!
//! - **A duplicate name**, because a censused symbol declared twice is a build failure at
//!   best and two entries for one symbol at worst.
//! - **A name also declared in `platform.h`**, refused rather than filtered. Silently
//!   dropping it would let this succeed while the list still claims the symbol is censused,
//!   and the next person to read it would believe that.
//! - **A name not shaped like a symbol**, because it is almost always the tail of one.
//!   `sceKernelGetModuleInfo` was once removed from a list without matching on word
//!   boundaries and left `FromAddr` behind, which censused a symbol that cannot exist while
//!   the real one went uncounted.
//!
//! # This generator had stopped working, and nothing noticed
//!
//! It was never gated. Ten names were promoted to `platform.h` over time - seven networking
//! calls and three mutex-attribute ones - and each was removed from the header and added to
//! the refusal list while staying in the generator's own group list. So the generator refused
//! its own definition and could not reproduce the committed header. It is gated now.

use std::collections::BTreeSet;
use std::fmt::Write as _;
use std::path::Path;

/// One group: a macro of names, and the library they come from.
pub struct Group {
    /// The X-macro this group defines.
    pub macro_name: String,
    /// The group's identifier tag in the library list.
    pub tag: String,
    /// The library that provides them.
    pub library: String,
    /// Which generations have it.
    pub availability: String,
    /// The prose note emitted above the macro.
    pub note: String,
    /// The names, in the order the data file lists them.
    pub symbols: Vec<String>,
}

/// Everything `data/surface.txt` defines.
#[derive(Default)]
pub struct Definition {
    /// Fixed prose, by key: `preamble`, `fence`, `libraries-note`.
    pub prose: std::collections::BTreeMap<String, String>,
    /// Names declared in `platform.h`, which must not also be censused.
    pub called_elsewhere: BTreeSet<String>,
    /// The groups, in order.
    pub groups: Vec<Group>,
}

/// Which block the reader is inside.
enum Block {
    Nothing,
    /// Verbatim prose, under a key.
    Prose(String, Vec<String>),
    /// Names declared elsewhere.
    CalledElsewhere,
    /// Verbatim prose belonging to the group most recently opened.
    Note(Vec<String>),
    /// Names belonging to the group most recently opened.
    Symbols,
}

/// Fold a finished block into the definition.
fn close(out: &mut Definition, block: Block) {
    match block {
        Block::Prose(key, lines) => {
            out.prose.insert(key, lines.join("\n"));
        }
        Block::Note(lines) => {
            if let Some(group) = out.groups.last_mut() {
                group.note = lines.join("\n");
            }
        }
        Block::Nothing | Block::CalledElsewhere | Block::Symbols => {}
    }
}

/// Open the block a `@` line introduces.
fn open(out: &mut Definition, rest: &str) -> Block {
    let mut fields = rest.split_whitespace();
    match fields.next() {
        Some("prose") => Block::Prose(fields.next().unwrap_or_default().to_owned(), Vec::new()),
        Some("called-elsewhere") => Block::CalledElsewhere,
        Some("note") => Block::Note(Vec::new()),
        Some("symbols") => Block::Symbols,
        Some("group") => {
            let parts: Vec<&str> = fields.collect();
            if let (Some(macro_name), Some(tag), Some(library), Some(availability)) =
                (parts.first(), parts.get(1), parts.get(2), parts.get(3))
            {
                out.groups.push(Group {
                    macro_name: (*macro_name).to_owned(),
                    tag: (*tag).to_owned(),
                    library: (*library).to_owned(),
                    availability: (*availability).to_owned(),
                    note: String::new(),
                    symbols: Vec::new(),
                });
            }
            Block::Nothing
        }
        // `@end` and anything unrecognised close whatever was open and start nothing.
        _ => Block::Nothing,
    }
}

/// Read the definition.
pub fn read(root: &Path) -> std::io::Result<Definition> {
    let text = std::fs::read_to_string(root.join("data").join("surface.txt"))?;
    let mut out = Definition::default();
    let mut block = Block::Nothing;

    for line in text.lines() {
        if let Some(rest) = line.strip_prefix('@') {
            let previous = std::mem::replace(&mut block, Block::Nothing);
            close(&mut out, previous);
            block = open(&mut out, rest);
            continue;
        }
        match &mut block {
            Block::Prose(_, lines) | Block::Note(lines) => lines.push(line.to_owned()),
            Block::CalledElsewhere => {
                let name = line.trim();
                if !name.is_empty() && !name.starts_with('#') {
                    out.called_elsewhere.insert(name.to_owned());
                }
            }
            Block::Symbols => {
                if let Some(group) = out.groups.last_mut() {
                    let name = line.trim();
                    if !name.is_empty() && !name.starts_with('#') {
                        group.symbols.push(name.to_owned());
                    }
                }
            }
            Block::Nothing => {}
        }
    }
    close(&mut out, block);
    Ok(out)
}

/// Vendor names are `sce` and a capital, C library names are lowercase, ABI plumbing starts
/// with two underscores. Nothing else is a symbol.
#[must_use]
pub fn is_symbol_shaped(name: &str) -> bool {
    if name.starts_with("__") {
        return true;
    }
    if let Some(rest) = name.strip_prefix("sce")
        && rest.chars().next().is_some_and(|c| c.is_ascii_uppercase())
    {
        return true;
    }
    name.chars().next().is_some_and(char::is_lowercase)
}

/// Check every group's names, returning the problems found.
#[must_use]
pub fn validate(definition: &Definition) -> Vec<String> {
    let mut problems = Vec::new();
    let mut seen: BTreeSet<&str> = BTreeSet::new();
    for group in &definition.groups {
        for symbol in &group.symbols {
            if !seen.insert(symbol) {
                problems.push(format!("duplicate symbol: {symbol}"));
            } else if definition.called_elsewhere.contains(symbol) {
                problems.push(format!(
                    "{symbol} is declared in platform.h and cannot also be censused; \
                     remove it from data/surface.txt"
                ));
            } else if !is_symbol_shaped(symbol) {
                problems.push(format!(
                    "{symbol} is not shaped like a symbol - it is probably the tail of one, \
                     left by a removal that did not match on word boundaries"
                ));
            }
        }
    }
    problems
}

/// The header.
///
/// The prose blocks carry their own trailing blank lines, so nothing is inserted between
/// them and what follows.
#[must_use]
pub fn render(definition: &Definition) -> String {
    let prose = |key: &str| definition.prose.get(key).cloned().unwrap_or_default();
    let mut out = String::new();
    out.push_str(&prose("preamble"));
    out.push_str("#ifndef OBSCENE_SURFACE_H\n#define OBSCENE_SURFACE_H\n\n");
    out.push_str(&prose("fence"));

    for group in &definition.groups {
        if !group.note.is_empty() {
            out.push_str(&group.note);
            // One newline, not two: the notes end at their closing marker, and the header
            // has never carried a blank line between a note and the macro it introduces.
            out.push('\n');
        }
        let _ = write!(out, "#define {}(X)", group.macro_name);
        for symbol in &group.symbols {
            let _ = write!(out, " \\\n    X({symbol})");
        }
        out.push_str("\n\n");
    }

    out.push_str(&prose("libraries-note"));
    for group in &definition.groups {
        let _ = write!(
            out,
            " \\\n    L({}, \"{}\", {}, {})",
            group.tag, group.library, group.availability, group.macro_name
        );
    }
    out.push_str("\n\n/* clang-format on */\n\n#endif /* OBSCENE_SURFACE_H */\n");
    out
}

/// Write or check the header. Returns true when all is well.
pub fn run(root: &Path, check: bool) -> std::io::Result<bool> {
    let definition = read(root)?;
    let problems = validate(&definition);
    if !problems.is_empty() {
        println!("the surface definition is not usable:");
        for line in &problems {
            println!("  {line}");
        }
        return Ok(false);
    }
    let fresh = render(&definition);
    let path = root.join("include").join("obscene").join("surface.h");
    let total: usize = definition.groups.iter().map(|g| g.symbols.len()).sum();

    if check {
        let current = std::fs::read_to_string(&path).unwrap_or_default();
        if current == fresh {
            println!("surface.h current ({total} symbols)");
            return Ok(true);
        }
        println!("surface.h is out of date - regenerate it");
        return Ok(false);
    }
    std::fs::write(&path, fresh)?;
    println!(
        "wrote surface.h: {} groups, {total} symbols",
        definition.groups.len()
    );
    Ok(true)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn group(symbols: &[&str]) -> Group {
        Group {
            macro_name: "MAC".to_owned(),
            tag: "tag".to_owned(),
            library: "libX".to_owned(),
            availability: "OBS_SHARED".to_owned(),
            note: String::new(),
            symbols: symbols.iter().map(|s| (*s).to_owned()).collect(),
        }
    }

    /// The scar this rule carries: `sceKernelGetModuleInfo` was removed from a list without
    /// matching on word boundaries and left `FromAddr` behind, which censused a symbol that
    /// cannot exist.
    #[test]
    fn a_fragment_is_not_shaped_like_a_symbol() {
        assert!(!is_symbol_shaped("FromAddr"));
        assert!(!is_symbol_shaped("Info"));
        assert!(is_symbol_shaped("sceKernelGetModuleInfo"));
        assert!(is_symbol_shaped("__stack_chk_fail"));
        assert!(is_symbol_shaped("posix_memalign"));
        // `sce` without a capital after it is an ordinary lowercase name, still a symbol.
        assert!(is_symbol_shaped("scepticism"));
    }

    #[test]
    fn a_duplicate_is_refused() {
        let mut definition = Definition::default();
        definition.groups.push(group(&["sceOne", "sceOne"]));
        assert!(
            validate(&definition)
                .iter()
                .any(|p| p.contains("duplicate symbol"))
        );
    }

    /// Refused, not filtered: silently dropping it would let this succeed while the list
    /// still claims the symbol is censused. Ten real names were in exactly this state.
    #[test]
    fn a_name_declared_in_platform_h_is_refused() {
        let mut definition = Definition::default();
        definition.called_elsewhere.insert("sceNetInit".to_owned());
        definition.groups.push(group(&["sceNetInit"]));
        assert!(
            validate(&definition)
                .iter()
                .any(|p| p.contains("cannot also be censused"))
        );
    }

    #[test]
    fn blocks_round_trip_through_the_reader() {
        let dir = std::env::temp_dir().join("obscene-surface-read");
        std::fs::create_dir_all(dir.join("data")).expect("temp dir");
        std::fs::write(
            dir.join("data").join("surface.txt"),
            "@prose preamble\n/* head */\n\n@end\n\n@called-elsewhere\nsceNetInit\n@end\n\n\
             @group MAC tag libX OBS_SHARED\n@note\n/* a note */\n@end\n@symbols\nsceOne\nsceTwo\n@end\n",
        )
        .expect("write");
        let definition = read(&dir).expect("read");
        assert_eq!(
            definition.prose.get("preamble").map(String::as_str),
            Some("/* head */\n"),
            "a blank line the prose wanted must survive"
        );
        assert!(definition.called_elsewhere.contains("sceNetInit"));
        assert_eq!(definition.groups.len(), 1);
        let first = definition.groups.first().expect("one group");
        assert_eq!(first.library, "libX");
        assert_eq!(first.note, "/* a note */");
        assert_eq!(
            first.symbols,
            vec!["sceOne".to_owned(), "sceTwo".to_owned()]
        );
        std::fs::remove_dir_all(&dir).ok();
    }
}
