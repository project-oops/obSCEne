//! Turn a loader's unresolved-import log into a list of names it does not implement.
//!
//! # Why this is a lookup and not a cracking problem
//!
//! A loader reports what it could not resolve as an identifier, because an identifier is all
//! a module gives it. Recovering a name from one in general means guessing - that is what
//! `obscene-tool crack` is for.
//!
//! Not here. **This program knows the name of every symbol it imports**: it computed those
//! identifiers itself at build time. Hashing our own import list produces an exact
//! identifier-to-name table for everything we ask for, and a loader's unresolved list
//! translates by lookup.
//!
//! The result is the useful direction - not "which hashes failed" but "which functions this
//! emulator does not have", by name, grouped by library.
//!
//! # Why Kyty and not shadPS4
//!
//! shadPS4 resolves everything to a generic stub, so nothing appears unresolved even when
//! nothing is implemented. Kyty patches each unresolved import individually and says so,
//! which makes its log the better source for absence.

use crate::suffix;
use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

/// Every symbol this program imports.
///
/// Three sources, and all three are needed. The census and the platform header carry most of
/// it; `src/imports.c` is the authoritative manifest, because `mkmodule` refuses to build a
/// module whose undefined symbols are not all in it.
///
/// Reading only the first two left exactly one import unnamed, and it turned out to be the
/// deliberately non-existent name the census uses to prove it can detect absence. It is
/// declared in a section file rather than a header, so a header scan could never have found
/// it - and a loader reporting it unresolved is the control working.
pub fn our_names(root: &Path) -> std::io::Result<BTreeSet<String>> {
    let include = root.join("include").join("obscene");
    let mut names = BTreeSet::new();

    let surface = std::fs::read_to_string(include.join("surface.h"))?;
    for chunk in surface.split("X(").skip(1) {
        if let Some((inner, _)) = chunk.split_once(')') {
            let name = inner.trim();
            if !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
                names.insert(name.to_owned());
            }
        }
    }

    let platform = std::fs::read_to_string(include.join("platform.h"))?;
    for chunk in platform.split("OBS_WEAK").skip(1) {
        let Some((head, _)) = chunk.split_once('(') else {
            continue;
        };
        let tail: String = head
            .trim_end()
            .chars()
            .rev()
            .take_while(|c| c.is_ascii_alphanumeric() || *c == '_')
            .collect();
        let name: String = tail.chars().rev().collect();
        if !name.is_empty() {
            names.insert(name);
        }
    }

    let imports = std::fs::read_to_string(root.join("src").join("imports.c"))?;
    names.extend(crate::guards::platform_symbols(&imports));

    names.remove("name");
    Ok(names)
}

/// Identifier-to-name pairs from fpPS4's table.
///
/// For a log from a module this program did not build, where the names are not ours and
/// cannot be recomputed from our own import list.
fn borrowed_names(emulators: &Path) -> BTreeMap<String, String> {
    let mut out = BTreeMap::new();
    let root = emulators.join("fpPS4");
    let mut stack = vec![root];
    while let Some(dir) = stack.pop() {
        let Ok(entries) = std::fs::read_dir(&dir) else {
            continue;
        };
        for entry in entries.filter_map(Result::ok) {
            let path = entry.path();
            if path.is_dir() {
                stack.push(path);
                continue;
            }
            if !path
                .extension()
                .is_some_and(|e| e.eq_ignore_ascii_case("pas"))
            {
                continue;
            }
            let Ok(raw) = std::fs::read(&path) else {
                continue;
            };
            let text = String::from_utf8_lossy(&raw);
            for chunk in text.split("(i:$").skip(1) {
                let Some((hex, rest)) = chunk.split_once(';') else {
                    continue;
                };
                if hex.len() != 16 || !hex.chars().all(|c| c.is_ascii_hexdigit()) {
                    continue;
                }
                let Some(rest) = rest.trim_start().strip_prefix("n:'") else {
                    continue;
                };
                let Some((name, _)) = rest.split_once('\'') else {
                    continue;
                };
                if let Ok(value) = u64::from_str_radix(hex, 16) {
                    out.insert(
                        selfish_nid::Nid::from_value(value).encode(),
                        name.to_owned(),
                    );
                }
            }
        }
    }
    out
}

/// The identifier and library from one Kyty stub line.
///
/// `... patched to stub [n] [addr] <- addr, <NID>[Library][Module][Func], ...`
fn stub_line(line: &str) -> Option<(String, String)> {
    let (_, rest) = line.split_once("patched to stub")?;
    // The *first* comma-separated field that looks like an identifier followed by its
    // library, not the last. These lines end with a trailing `, ok`, so taking the last
    // field found the status word and reported the log as having no stubs at all.
    for candidate in rest.split(", ").skip(1) {
        let encoded: String = candidate
            .chars()
            .take_while(|c| c.is_ascii_alphanumeric() || *c == '+' || *c == '-')
            .collect();
        if encoded.len() != 11 {
            continue;
        }
        let Some(tail) = candidate.get(encoded.len()..) else {
            continue;
        };
        let Some(library) = tail.strip_prefix('[').and_then(|t| t.split_once(']')) else {
            continue;
        };
        return Some((encoded, library.0.to_owned()));
    }
    None
}

/// Translate one or more logs.
pub fn run(root: &Path, emulators: &Path, logs: &[std::path::PathBuf]) -> std::io::Result<bool> {
    let salt = suffix::read(None).unwrap_or_default();
    let mut table = BTreeMap::new();
    for name in our_names(root)? {
        table.insert(selfish_nid::Nid::with_suffix(&name, &salt).encode(), name);
    }

    let mut unresolved: BTreeMap<String, String> = BTreeMap::new();
    for path in logs {
        let Ok(raw) = std::fs::read(path) else {
            println!("missing: {}", path.display());
            continue;
        };
        for line in String::from_utf8_lossy(&raw).lines() {
            if let Some((encoded, library)) = stub_line(line) {
                unresolved.insert(encoded, library);
            }
        }
    }
    if unresolved.is_empty() {
        println!("no unresolved imports found - is this a Kyty log?");
        return Ok(false);
    }

    let borrowed = borrowed_names(emulators);
    let mut named: BTreeMap<String, Vec<String>> = BTreeMap::new();
    let mut anonymous: BTreeMap<String, usize> = BTreeMap::new();
    for (encoded, library) in &unresolved {
        if let Some(name) = table.get(encoded) {
            named.entry(library.clone()).or_default().push(name.clone());
        } else if let Some(name) = borrowed.get(encoded) {
            // Ours by construction - the loader was given this module - so reaching this
            // branch means our own reader missed something. The name is worth having anyway,
            // and marked, because "we could not account for this" is a different fact from
            // "this function is missing".
            named
                .entry(library.clone())
                .or_default()
                .push(format!("{name}  (named externally)"));
        } else {
            // Counted rather than dropped: a silent drop would read as better coverage.
            let count = anonymous.entry(library.clone()).or_insert(0);
            *count = count.saturating_add(1);
        }
    }

    let total: usize = named.values().map(Vec::len).sum();
    println!("# {total} of {} unresolved imports named", unresolved.len());
    for (library, names) in &named {
        println!("\n## {library} ({})", names.len());
        let mut sorted = names.clone();
        sorted.sort();
        for name in sorted {
            println!("  {name}");
        }
    }
    for (library, count) in &anonymous {
        println!("\n## {library}: {count} NIDs this reader could not name");
    }
    Ok(true)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_stub_line_yields_the_identifier_and_library() {
        let line = "[Core] patched to stub [12] [0x1000] <- 0x2000, 9vA2aW+CHuA[libSceNet][libSceNet][Func], ok";
        assert_eq!(
            stub_line(line),
            Some(("9vA2aW+CHuA".to_owned(), "libSceNet".to_owned()))
        );
    }

    /// These lines end with a trailing status word, so the *first* matching field is the
    /// one that carries the identifier.
    #[test]
    fn a_trailing_field_does_not_hide_the_identifier() {
        let line = "patched to stub [1] [0x1] <- 0x2, 9vA2aW+CHuA[libSceNet][mod][Func], ok";
        assert_eq!(
            stub_line(line).map(|(a, _)| a),
            Some("9vA2aW+CHuA".to_owned())
        );
    }

    #[test]
    fn an_ordinary_log_line_is_not_a_stub() {
        assert!(stub_line("[Core] loaded module libkernel.prx").is_none());
        assert!(stub_line("patched to stub but no identifier here").is_none());
    }
}
