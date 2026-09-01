//! What the emulators implement that obSCEne does not reach.
//!
//! # Why this exists
//!
//! obSCEne's surface was assembled from public documentation and grew by hand. That makes it
//! a list of what somebody thought to write down, which is not the same as a list of what the
//! platform has - and the difference is invisible from inside the project.
//!
//! The emulators are the correction. Each registers the functions it implements, by name, and
//! none was working from our list. What they collectively name and we do not is a measurement
//! of the blind spot rather than a guess at it.
//!
//! # Two scopes, because the answer differs
//!
//! **Every emulator** is dominated by the previous generation - most of the toolkit targets
//! it - so the union is mostly a previous-generation surface. Useful for growing the census.
//!
//! **Current-generation only** asks a different question: what does a console offer that
//! obSCEne never touches? Somebody decided each of those functions was worth implementing,
//! usually because something real called it.
//!
//! # Three states, and the middle one is the interesting one
//!
//! - **called** - obSCEne has a real declaration and exercises it.
//! - **censused** - obSCEne knows the name and never calls it. A candidate for promotion.
//! - **unknown** - obSCEne has never heard of it. The blind spot.

use crate::mining;
use std::collections::{BTreeMap, BTreeSet};
use std::fmt::Write as _;
use std::path::Path;

/// Which emulators to count.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Scope {
    /// Every emulator in the toolkit.
    All,
    /// Only those targeting the current generation.
    CurrentGeneration,
}

impl Scope {
    /// Emulators in scope. The current-generation set is the three that target it.
    fn emulators(self) -> &'static [&'static str] {
        match self {
            Self::All => &[
                "shadPS4",
                "fpPS4",
                "PS5PCEM",
                "SharpEMU",
                "craziiEmu",
                "GPCS4",
                "ChonkyStation4",
            ],
            Self::CurrentGeneration => &["PS5PCEM", "SharpEMU", "craziiEmu"],
        }
    }
}

/// What obSCEne calls, and what it merely counts.
///
/// Read from the headers rather than the data file: `platform.h` is where a real declaration
/// lives, and a name being *declared* is what separates "called" from "censused".
fn ours(root: &Path) -> std::io::Result<(BTreeSet<String>, BTreeSet<String>)> {
    let include = root.join("include").join("obscene");
    let platform = std::fs::read_to_string(include.join("platform.h"))?;
    let mut called = BTreeSet::new();
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
            called.insert(name);
        }
    }

    let surface = std::fs::read_to_string(include.join("surface.h"))?;
    let mut censused = BTreeSet::new();
    for chunk in surface.split("X(").skip(1) {
        if let Some((inner, _)) = chunk.split_once(')') {
            let name = inner.trim();
            if name != "name"
                && !name.is_empty()
                && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
            {
                censused.insert(name.to_owned());
            }
        }
    }
    Ok((called, censused))
}

/// The library a symbol most likely belongs to, from its prefix.
///
/// A guess, and labelled as one. It groups the report so a reader sees subsystems rather than
/// an undifferentiated list; nothing downstream treats it as fact.
fn library_from_prefix(name: &str) -> &'static str {
    const PREFIXES: [(&str, &str); 10] = [
        ("sceAgcDriver", "libSceAgcDriver"),
        ("sceAgc", "libSceAgc"),
        ("sceKernel", "libkernel"),
        ("sceVideoOut", "libSceVideoOut"),
        ("sceAudioOut", "libSceAudioOut"),
        ("sceNet", "libSceNet"),
        ("sceNp", "libSceNp"),
        ("sceSaveData", "libSceSaveData"),
        ("scePad", "libScePad"),
        ("sceSysmodule", "libSceSysmodule"),
    ];
    for (prefix, library) in PREFIXES {
        if name.starts_with(prefix) {
            return library;
        }
    }
    "(prefix unknown)"
}

/// Names the `OpenOrbis` toolchain headers declare, with the header that declares each.
///
/// A published signature is what separates "add a census entry" from "add a behavioural
/// check": the census costs a name, a check costs a confident arity and struct layout, and
/// D008 forbids inventing either. A gap with a signature behind it is the cheap kind.
fn toolchain_declarations(emulators_dir: &Path) -> BTreeMap<String, String> {
    let mut out = BTreeMap::new();
    let root = emulators_dir
        .join("OpenOrbis-PS4-Toolchain")
        .join("include")
        .join("orbis");
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
                .is_some_and(|e| e.eq_ignore_ascii_case("h"))
            {
                continue;
            }
            let Ok(text) = std::fs::read_to_string(&path) else {
                continue;
            };
            let header = path
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or_default()
                .to_owned();
            for line in text.lines() {
                // A declaration is a vendor name immediately followed by an opening
                // parenthesis. Anything else on the line is a return type or storage class.
                let Some((head, _)) = line.split_once('(') else {
                    continue;
                };
                let tail: String = head
                    .trim_end()
                    .chars()
                    .rev()
                    .take_while(|c| c.is_ascii_alphanumeric() || *c == '_')
                    .collect();
                let name: String = tail.chars().rev().collect();
                let vendor = name.strip_prefix("sce").is_some_and(|rest| {
                    rest.chars().next().is_some_and(|c| c.is_ascii_uppercase())
                });
                if vendor {
                    out.entry(name).or_insert_with(|| header.clone());
                }
            }
        }
    }
    out
}

/// One symbol an emulator implements, and where obSCEne stands on it.
struct Row {
    name: String,
    library: String,
    emulators: BTreeSet<String>,
}

/// The gaps worth turning into behavioural checks rather than census entries.
///
/// One emulator is one opinion; two that never read each other's list is evidence. A
/// published signature is the other half - without it the gap can only become a census
/// entry, because D008 forbids inventing an arity.
fn checkable_gaps(root: &Path, emulators_dir: &Path, unknown: &[&Row]) -> std::io::Result<()> {
    // The gaps worth turning into behavioural checks rather than census entries: a published
    // signature exists, and at least two independent emulators implement it. One emulator is
    // one opinion; two that never read each other's list is evidence.
    let declarations = toolchain_declarations(emulators_dir);
    let mut checkable: Vec<(usize, &Row, &String)> = unknown
        .iter()
        .filter_map(|row| {
            let header = declarations.get(&row.name)?;
            (row.emulators.len() >= 2).then_some((row.emulators.len(), *row, header))
        })
        .collect();
    checkable.sort_by(|a, b| b.0.cmp(&a.0).then_with(|| a.1.name.cmp(&b.1.name)));
    println!(
        "
toolchain declares                  : {}",
        declarations.len()
    );
    println!(
        "checkable gaps                      : {}  (published signature, two or more emulators)",
        checkable.len()
    );

    let checkable_out = root.join("reports").join("gap-checkable.txt");
    std::fs::create_dir_all(checkable_out.parent().unwrap_or(Path::new(".")))?;
    let mut text = [
        "# Gaps that could become behavioural checks, not merely census entries: a",
        "# signature exists in the OpenOrbis toolchain headers and at least two",
        "# independent emulators implement it.",
        "# Columns: emulator-count, symbol, library, declaring header.",
        "#",
        "",
    ]
    .join(
        "
",
    );
    for (count, row, header) in &checkable {
        let _ = writeln!(text, "{count} {} {} {header}", row.name, row.library);
    }
    std::fs::write(&checkable_out, text)?;
    Ok(())
}

/// Run the analysis and write the report.
pub fn run(root: &Path, emulators_dir: &Path, scope: Scope) -> std::io::Result<bool> {
    // Mined from the emulator tables, which is the same reading `obscene-tool mine` does -
    // shared so the two can never disagree about what an emulator implements.
    let (found, _) = mining::mine(emulators_dir, Path::new("(none)"))?;
    let wanted: BTreeSet<&str> = scope.emulators().iter().copied().collect();

    let (called, censused) = ours(root)?;
    let mut rows: BTreeMap<String, Row> = BTreeMap::new();
    for (name, entry) in &found {
        let mine: BTreeSet<String> = entry
            .sources
            .iter()
            .filter(|s| wanted.contains(s.as_str()))
            .cloned()
            .collect();
        if mine.is_empty() {
            continue;
        }
        let library = entry
            .libraries
            .iter()
            .next()
            .cloned()
            .unwrap_or_else(|| library_from_prefix(name).to_owned());
        rows.insert(
            name.clone(),
            Row {
                name: name.clone(),
                library,
                emulators: mine,
            },
        );
    }
    if rows.is_empty() {
        println!("no exports found under {}", emulators_dir.display());
        return Ok(false);
    }

    let mut exercised = Vec::new();
    let mut counted = Vec::new();
    let mut unknown = Vec::new();
    for row in rows.values() {
        if called.contains(&row.name) {
            exercised.push(row);
        } else if censused.contains(&row.name) {
            counted.push(row);
        } else {
            unknown.push(row);
        }
    }

    let label = match scope {
        Scope::All => "emulators implement",
        Scope::CurrentGeneration => "current-generation emulators implement",
    };
    println!("{label:<38}: {}", rows.len());
    println!(
        "  obSCEne calls                       : {}",
        exercised.len()
    );
    println!("  obSCEne censuses but never calls    : {}", counted.len());
    println!("  obSCEne does not know at all        : {}", unknown.len());

    by_library(&unknown, "not in obSCEne's surface at all");
    by_library(&counted, "censused but never called");

    checkable_gaps(root, emulators_dir, &unknown)?;

    let name = match scope {
        Scope::All => "gap-analysis.txt",
        Scope::CurrentGeneration => "ps5-gap.txt",
    };
    let out = root.join("reports").join(name);
    std::fs::create_dir_all(out.parent().unwrap_or(Path::new(".")))?;
    let mut text = String::from(
        "# What the emulators implement, against obSCEne's surface.\n\
         # Columns: state, library, symbol, emulators implementing it.\n\
         # state: called | censused | unknown\n#\n",
    );
    for (state, group) in [
        ("called", &exercised),
        ("censused", &counted),
        ("unknown", &unknown),
    ] {
        for row in group {
            let who = row.emulators.iter().cloned().collect::<Vec<_>>().join(",");
            let _ = writeln!(text, "{state} {} {} {who}", row.library, row.name);
        }
    }
    std::fs::write(&out, text)?;
    println!("\nwritten: {}", out.display());
    Ok(true)
}

/// The interesting groups, largest first, truncated so the shape is readable.
fn by_library(rows: &[&Row], title: &str) {
    if rows.is_empty() {
        return;
    }
    let mut groups: BTreeMap<&str, Vec<&str>> = BTreeMap::new();
    for row in rows {
        groups
            .entry(row.library.as_str())
            .or_default()
            .push(row.name.as_str());
    }
    let mut ordered: Vec<(&&str, &Vec<&str>)> = groups.iter().collect();
    ordered.sort_by_key(|(_, names)| std::cmp::Reverse(names.len()));

    println!("\n--- {title}, by library ---");
    for (library, names) in ordered {
        println!("{:4}  {library}", names.len());
        for name in names.iter().take(6) {
            println!("        {name}");
        }
        if let Some(extra) = names.len().checked_sub(6)
            && extra > 0
        {
            println!("        ... and {extra} more");
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A guess, and only ever used to group the report.
    #[test]
    fn the_library_guess_prefers_the_longer_prefix() {
        assert_eq!(library_from_prefix("sceAgcDriverSubmit"), "libSceAgcDriver");
        assert_eq!(library_from_prefix("sceAgcCreateShader"), "libSceAgc");
        assert_eq!(library_from_prefix("sceKernelOpen"), "libkernel");
        assert_eq!(library_from_prefix("somethingElse"), "(prefix unknown)");
    }

    #[test]
    fn the_current_generation_scope_is_narrower() {
        assert!(Scope::CurrentGeneration.emulators().len() < Scope::All.emulators().len());
        assert!(Scope::CurrentGeneration.emulators().contains(&"PS5PCEM"));
        assert!(!Scope::CurrentGeneration.emulators().contains(&"fpPS4"));
    }
}
