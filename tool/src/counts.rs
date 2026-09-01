//! Keep the numbers in the documentation equal to the numbers in the code.
//!
//! # The problem this exists for
//!
//! Prose that quotes a count goes stale the moment the count changes, and nothing notices.
//! The README said 79 checks when there were 106, and 326 censused symbols when there were
//! 312. None of that was carelessness - each number was right when written. A number in
//! prose has no mechanism to stay right, and the more places it appears the faster it rots.
//!
//! **Volatile facts are generated; static facts are written.** A count of checks changes
//! every working day and belongs to the code, so prose renders it rather than repeating it.
//! The hash suffix, the dynamic tag values, why `-fno-plt` was removed - those are settled,
//! and writing them down is exactly right. This only owns things that move.
//!
//! # Ported from Python, and it was undercounting
//!
//! `scripts/counts.py` matched a check id as `[0-9]{3}-[a-z]+/…`, which has no room for a
//! hyphen in the section name. `150-memory-map` has two checks and neither was ever counted,
//! so the README claimed **134 checks when there were 136** - right up until this port put
//! the two numbers side by side.
//!
//! That is the third tool found miscounting the same tables in the same way, which is why
//! the row parser now lives in [`crate::sections`] and is shared rather than re-derived. All
//! three failed by producing a *smaller* number, and a gate that compares against nothing
//! cannot notice that.

use crate::sections;
use std::collections::BTreeMap;
use std::path::Path;

const BEGIN: &str = "<!-- obscene:counts -->";
const END: &str = "<!-- /obscene:counts -->";

/// Everything that moves, derived from the tree.
#[derive(Debug, Default, Clone)]
pub struct Counts {
    /// Behavioural checks across all sections.
    pub checks: usize,
    /// Sections in the running order.
    pub sections: usize,
    /// Censused symbol names, across all three censuses.
    pub census: usize,
    /// Distinct libraries those symbols come from.
    pub libraries: usize,
    /// Imports the manifest places.
    pub manifest: usize,
    /// Checks by provenance token.
    pub provenance: BTreeMap<String, usize>,
}

impl Counts {
    /// How many checks carry one provenance.
    #[must_use]
    pub fn of(&self, kind: &str) -> usize {
        self.provenance.get(kind).copied().unwrap_or(0)
    }
}

/// Split on a macro marker that stands on its own.
///
/// `text.split("L(")` also splits inside any identifier *ending* in `L` - and
/// `OBS_NIDS_0000_LIBKERNEL(X)` ends in one, so every nameless-census group was being read
/// as a library and the count came out 382 against a true 372. The marker only counts when
/// the character before it is not part of an identifier.
fn split_marker<'a>(text: &'a str, marker: &str) -> Vec<&'a str> {
    let mut out = Vec::new();
    let mut previous_tail_is_identifier = false;
    for (index, chunk) in text.split(marker).enumerate() {
        if index > 0 && !previous_tail_is_identifier {
            out.push(chunk);
        }
        previous_tail_is_identifier = chunk
            .chars()
            .last()
            .is_some_and(|c| c.is_alphanumeric() || c == '_');
    }
    out
}

/// Names inside `X(...)` macro invocations, which is how every census lists its symbols.
fn census_names(text: &str, out: &mut std::collections::BTreeSet<String>) {
    for chunk in split_marker(text, "X(") {
        let Some((inside, _)) = chunk.split_once(')') else {
            continue;
        };
        // `surface.h` and `corpus.h` carry one argument; `nids.h` carries a generated C name
        // and the real identifier as a quoted string. The identifier is the name there.
        if let Some((_, quoted)) = inside.split_once('"') {
            if let Some((identifier, _)) = quoted.split_once('"')
                && identifier.starts_with('$')
            {
                out.insert(identifier.to_owned());
            }
            continue;
        }
        let name = inside.trim();
        if !name.is_empty()
            && name != "name"
            && name.chars().all(|c| c.is_alphanumeric() || c == '_')
        {
            out.insert(name.to_owned());
        }
    }
}

/// Library names from `L(tag, "name", ...)` group lists.
///
/// The *name*, not the tag: `surface.h` splits one library into two groups for readability,
/// and counting tags reported 32 libraries for 16. A module importing from 32 and one
/// importing from 16 are different things to a loader.
fn library_names(text: &str, out: &mut std::collections::BTreeSet<String>) {
    for chunk in split_marker(text, "L(") {
        let Some((_, rest)) = chunk.split_once(',') else {
            continue;
        };
        let rest = rest.trim_start();
        let Some(rest) = rest.strip_prefix('"') else {
            continue;
        };
        let Some((name, _)) = rest.split_once('"') else {
            continue;
        };
        if !name.is_empty() {
            out.insert(name.to_owned());
        }
    }
}

/// Read the tree and count what moves.
pub fn gather(root: &Path) -> std::io::Result<Counts> {
    let rows = sections::rows(&sections::find_dir(root, "sections"))?;
    let mut provenance: BTreeMap<String, usize> = BTreeMap::new();
    for row in &rows {
        let tally = provenance.entry(row.provenance.clone()).or_insert(0usize);
        *tally = tally.saturating_add(1);
    }

    let include = root.join("include").join("obscene");
    let mut census = std::collections::BTreeSet::new();
    let mut libraries = std::collections::BTreeSet::new();
    // Three censuses, and this count has been behind each one in turn. Keeping the list in
    // one place is the thing that stops that.
    for name in ["surface.h", "corpus.h", "nids.h"] {
        let text = std::fs::read_to_string(include.join(name))?;
        census_names(&text, &mut census);
        library_names(&text, &mut libraries);
    }

    let registry = std::fs::read_to_string(sections::find_file(root, "registry.c"))?;
    let sections_count = registry.matches("&obs_section_").count();

    let imports = std::fs::read_to_string(sections::find_file(root, "imports.c"))?;
    let manifest = crate::guards::platform_symbols(&imports).len();

    Ok(Counts {
        checks: rows.len(),
        sections: sections_count,
        census: census.len(),
        libraries: libraries.len(),
        manifest,
        provenance,
    })
}

/// What goes between the markers, per file. Each reads naturally where it sits.
fn render(name: &str, c: &Counts) -> Option<String> {
    match name {
        "README.md" => Some(format!(
            "**{} checks across {} sections**, {} censused symbols across {} libraries.\n\
             \n\
             Of those checks, {} rest on a public specification, {} on the specification of \
             the system this kernel derives from, {} on independent implementations that \
             agree, and {} on this project's own reasoning. **{} {} been confirmed on real \
             hardware**, which is the number that limits what any of this can claim.",
            c.checks,
            c.sections,
            c.census,
            c.libraries,
            c.of("spec"),
            c.of("derived"),
            c.of("implementations"),
            c.of("assumed"),
            c.of("hardware"),
            // The count is generated and the verb was not, so the README front page read
            // "**1 have been confirmed on real hardware**" for as long as it sat at one.
            if c.of("hardware") == 1 { "has" } else { "have" },
        )),
        "BACKLOG.md" | "001-where-it-stands.md" => Some(format!(
            "| | |\n\
             |---|---|\n\
             | Behavioural checks | **{}** - {} `spec`, {} `derived`, {} `implementations`, \
             {} `assumed`, {} `documented`, **{} `hardware`** |\n\
             | Census symbols | **{}** across {} libraries |\n\
             | Sections | {} |\n\
             | Imports placed by the manifest | {} |",
            c.checks,
            c.of("spec"),
            c.of("derived"),
            c.of("implementations"),
            c.of("assumed"),
            c.of("documented"),
            c.of("hardware"),
            c.census,
            c.libraries,
            c.sections,
            c.manifest,
        )),
        _ => None,
    }
}

/// Whether one file's marked region is current, updating it when asked.
enum Outcome {
    Current,
    Stale,
    NoRegion,
}

fn apply(path: &Path, c: &Counts, write: bool) -> std::io::Result<Outcome> {
    let name = path
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or_default()
        .to_owned();
    let text = std::fs::read_to_string(path)?;
    let (Some(before), Some(rendered)) = (text.split(BEGIN).next(), render(&name, c)) else {
        return Ok(Outcome::NoRegion);
    };
    if !text.contains(BEGIN) || !text.contains(END) {
        return Ok(Outcome::NoRegion);
    }
    let Some((_, after)) = text.split_once(END) else {
        return Ok(Outcome::NoRegion);
    };
    let updated = format!("{before}{BEGIN}\n{rendered}\n{END}{after}");
    if updated == text {
        return Ok(Outcome::Current);
    }
    if write {
        std::fs::write(path, updated)?;
        return Ok(Outcome::Current);
    }
    Ok(Outcome::Stale)
}

/// Update or check the marked regions. Returns true when everything is current.
pub fn run(root: &Path, write: bool, check: bool) -> std::io::Result<bool> {
    let c = gather(root)?;
    let backlog_doc = if root.join("docs").join("backlog").join("001-where-it-stands.md").exists() {
        root.join("docs").join("backlog").join("001-where-it-stands.md")
    } else {
        root.join("docs").join("BACKLOG.md")
    };
    let targets = [root.join("README.md"), backlog_doc];

    let mut stale = Vec::new();
    for path in &targets {
        let name = path
            .file_name()
            .and_then(|n| n.to_str())
            .unwrap_or_default()
            .to_owned();
        match apply(path, &c, write)? {
            Outcome::Current => {}
            Outcome::Stale => stale.push(format!("{name}: out of date")),
            Outcome::NoRegion => stale.push(format!("{name}: no {BEGIN} region")),
        }
    }

    if check {
        if stale.is_empty() {
            println!("counts current");
            return Ok(true);
        }
        println!("documentation counts have drifted:");
        for line in &stale {
            println!("  {line}");
        }
        println!("\nrun: obscene-tool counts --write");
        return Ok(false);
    }

    println!("checks       {}", c.checks);
    println!("sections     {}", c.sections);
    println!("census       {}", c.census);
    println!("libraries    {}", c.libraries);
    println!("manifest     {}", c.manifest);
    for (kind, count) in &c.provenance {
        println!("{kind:12} {count}");
    }
    if write {
        println!("\nwritten");
    }
    Ok(true)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn census_reads_both_macro_shapes() {
        let mut out = std::collections::BTreeSet::new();
        census_names("X(sceKernelOpen) \\\n X(sceKernelClose)", &mut out);
        census_names("X(obs_nid_0000001, \"$Xh3kd9sLpQw\")", &mut out);
        assert!(out.contains("sceKernelOpen"));
        assert!(out.contains("sceKernelClose"));
        assert!(
            out.contains("$Xh3kd9sLpQw"),
            "the nameless census carries two arguments and needs the quoted one"
        );
        assert_eq!(out.len(), 3);
    }

    /// Counting group tags reported 32 libraries for 16, because one library is split into
    /// two groups for readability. The name is the thing a loader sees.
    #[test]
    fn libraries_are_counted_by_name_not_by_group_tag() {
        let mut out = std::collections::BTreeSet::new();
        library_names(
            "L(libkernel_a, \"libkernel\", OBS_SHARED, X) L(libkernel_b, \"libkernel\", OBS_SHARED, Y)",
            &mut out,
        );
        assert_eq!(out.len(), 1, "two groups, one library");
    }

    /// A library whose name ends in `X` produces a literal `X(X)` in its macro definition,
    /// and the Python counted that parameter as a censused symbol. `LIBSCEDSEEHX`,
    /// `LIBSCEPOSIX` and `LIBSCEUSBSTORAGEAUX` all do it, so the README claimed 39,549
    /// symbols against a true 39,548.
    #[test]
    fn a_macro_parameter_is_not_a_censused_symbol() {
        let mut out = std::collections::BTreeSet::new();
        census_names(
            "#define OBS_CORPUS_0083_LIBSCEDSEEHX(X) \
    X(sceRealSymbol)",
            &mut out,
        );
        assert!(out.contains("sceRealSymbol"));
        assert!(!out.contains("X"), "the macro parameter is not a symbol");
        assert_eq!(out.len(), 1);
    }

    /// The same shape one level out: a marker only counts when it stands on its own.
    #[test]
    fn a_marker_inside_an_identifier_does_not_split() {
        assert_eq!(split_marker("OBS_LIBKERNEL(X)", "L(").len(), 0);
        assert_eq!(
            split_marker("    L(tag, \"libkernel\", A, B)", "L(").len(),
            1
        );
    }

    #[test]
    fn provenance_is_tallied_per_check() {
        let text = "\
{\"040-file/a\", \"lib\", \"s\", OBS_CAP_NONE, OBS_CAP_NONE, p, check_a, OBS_FROM_SPEC},
{\"150-memory-map/b\", \"lib\", \"s\", OBS_CAP_NONE, OBS_CAP_NONE, p, check_b, OBS_FROM_ASSUMED},
{\"150-memory-map/c\", \"lib\", \"s\", OBS_CAP_NONE, OBS_CAP_NONE, p, check_c, OBS_FROM_SPEC},";
        let rows = sections::rows_in(text);
        assert_eq!(rows.len(), 3, "hyphenated sections must count");
        let mut tally: BTreeMap<String, usize> = BTreeMap::new();
        for row in &rows {
            *tally.entry(row.provenance.clone()).or_default() += 1;
        }
        assert_eq!(tally.get("spec"), Some(&2));
        assert_eq!(tally.get("assumed"), Some(&1));
    }
}
