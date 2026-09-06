//! The decision log: one file per entry, and an index that has to agree with them.
//!
//! This gate used to build the index. It no longer does - `tools/split-decisions.sh --index
//! obscene`, shared with the sibling repositories, generates `docs/DECISIONS.md` from
//! `docs/decisions/`, and two generators for one file is how a file ends up with two values.
//!
//! What is left is the half a generator cannot do for itself: **checking that the directory
//! and the index still describe the same set.** A file with no row is invisible to anyone
//! reading the log. A row with no file is a dead link. A number claimed twice is D184 all
//! over again - two sessions appended, one reused a number the other had taken, and the tool
//! rendered it without a word.
//!
//! # Finding nothing is a failure, not a pass
//!
//! The previous version looked for `## D` headings in a file that stopped containing them
//! when the log was split. It reported "no entries found" and refused, which was right - but
//! it is worth stating as a rule rather than leaving to luck, because the same shape has now
//! appeared three times in one week: a probe reading back its own initialiser (D303), a
//! filter whose negative was a fact about the filter, and this.
//!
//! **A gate that finds nothing to check has not passed.** It has failed to run.

use std::collections::BTreeMap;
use std::path::Path;

/// One decision file: its number, and the file name a row must link to.
struct Entry {
    number: u32,
    file: String,
}

/// Every `D<NNN>-*.md` under `docs/decisions/`, ordered by number.
///
/// Ordered because a duplicate is found by looking at neighbours, and because every message
/// this gate prints is read by someone looking a number up.
fn entries(dir: &Path) -> std::io::Result<Vec<Entry>> {
    let mut out = Vec::new();
    for entry in std::fs::read_dir(dir)? {
        let name = entry?.file_name().to_string_lossy().into_owned();
        let Some(number) = number_of(&name) else {
            continue;
        };
        out.push(Entry { number, file: name });
    }
    out.sort_by(|a, b| a.number.cmp(&b.number).then_with(|| a.file.cmp(&b.file)));
    Ok(out)
}

/// The number a decision file name claims, or `None` if the name is not one.
///
/// Deliberately strict about the separator: `D303-...` and `D303.md` are entries, `D303x.md`
/// is not. A loose match here would let an unrelated file claim a number and report a
/// collision that does not exist.
fn number_of(name: &str) -> Option<u32> {
    let rest = name.strip_prefix('D')?;
    let digits: String = rest.chars().take_while(char::is_ascii_digit).collect();
    if digits.is_empty() {
        return None;
    }
    let after = rest.get(digits.len()..).unwrap_or_default();
    if !after.starts_with('-') && after != ".md" {
        return None;
    }
    digits.parse().ok()
}

/// Every decision file the index links to, in the order the rows appear.
///
/// Matches the whole link opener, not the bare directory name. Looking for `decisions/`
/// alone finds it in the front matter that *describes* the layout and in any prose that
/// mentions the directory, and then runs to whatever `)` comes next - which produced a
/// "link" spanning six rows on the first run against the real file. A name may not contain
/// a newline or a bracket, which is what stops a missing `)` swallowing the rest of the
/// document.
fn linked(text: &str) -> Vec<String> {
    const OPEN: &str = "](decisions/";
    let mut out = Vec::new();
    let mut rest = text;
    while let Some(at) = rest.find(OPEN) {
        let tail = rest
            .get(at.saturating_add(OPEN.len())..)
            .unwrap_or_default();
        let end = tail.find(')').unwrap_or(tail.len());
        let name = tail.get(..end).unwrap_or_default();
        if Path::new(name)
            .extension()
            .is_some_and(|e| e.eq_ignore_ascii_case("md"))
            && !name.contains(['\n', '(', '[', ']'])
        {
            out.push(name.to_owned());
        }
        rest = tail;
    }
    out
}

/// Files whose row carries no title at all - `[](decisions/D302-….md)`.
///
/// The splitter reads a title from `# D<NNN> - Title` and nothing else. Nine entries were
/// written as `# D302: Title`, so nine rows rendered as a bare link with no text, and an index
/// of unlabelled links is the failure the index exists to prevent. Cheap to check, and it had
/// gone unnoticed through several regenerations.
fn untitled(text: &str) -> Vec<String> {
    const OPEN: &str = "[](decisions/";
    let mut out = Vec::new();
    let mut rest = text;
    while let Some(at) = rest.find(OPEN) {
        let tail = rest
            .get(at.saturating_add(OPEN.len())..)
            .unwrap_or_default();
        let end = tail.find(')').unwrap_or(tail.len());
        let name = tail.get(..end).unwrap_or_default();
        if Path::new(name)
            .extension()
            .is_some_and(|e| e.eq_ignore_ascii_case("md"))
            && !name.contains(['\n', '(', '[', ']'])
        {
            out.push(name.to_owned());
        }
        rest = tail;
    }
    out
}

/// Check the directory and the index against each other. Returns true when they agree.
pub fn run(root: &Path) -> std::io::Result<bool> {
    let dir = root.join("docs").join("decisions");
    let index = root.join("docs").join("DECISIONS.md");

    let found = entries(&dir)?;
    if found.is_empty() {
        println!("no decision files in {}", dir.display());
        println!("this gate found nothing to check, which is a failure and not a pass");
        return Ok(false);
    }

    let mut ok = true;

    // Two entries may not share a number, and until D184 nothing said so. Two sessions
    // worked in this repository at once and both appended; one reused a number the other
    // had already taken, and the tool that exists to catch drift rendered it silently.
    let mut by_number: BTreeMap<u32, Vec<&str>> = BTreeMap::new();
    for entry in &found {
        by_number
            .entry(entry.number)
            .or_default()
            .push(entry.file.as_str());
    }
    for (number, files) in by_number.iter().filter(|(_, f)| f.len() > 1) {
        println!(
            "D{number:03} is claimed by {} files: {}",
            files.len(),
            files.join(", ")
        );
        ok = false;
    }

    let text = std::fs::read_to_string(&index)?;
    let rows = linked(&text);

    for entry in &found {
        match rows.iter().filter(|r| *r == &entry.file).count() {
            1 => {}
            0 => {
                println!("{}: no row in docs/DECISIONS.md", entry.file);
                ok = false;
            }
            n => {
                println!(
                    "{}: {n} rows in docs/DECISIONS.md, expected one",
                    entry.file
                );
                ok = false;
            }
        }
    }

    for row in &rows {
        if !dir.join(row).exists() {
            println!("docs/DECISIONS.md links to decisions/{row}, which does not exist");
            ok = false;
        }
    }

    for row in untitled(&text) {
        println!("decisions/{row}: the row has no title - the heading must read `# D<NNN> - …`");
        ok = false;
    }

    if ok {
        println!("decision log consistent ({} entries)", found.len());
        return Ok(true);
    }
    println!("regenerate with: tools/split-decisions.sh --index obscene");
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A file name claims a number only in the two shapes the splitter produces.
    #[test]
    fn a_number_is_claimed_only_by_the_shapes_the_splitter_writes() {
        assert_eq!(number_of("D303-a-scalar-out-parameter.md"), Some(303));
        assert_eq!(number_of("D001.md"), Some(1));
        assert_eq!(number_of("D303x-something.md"), None);
        assert_eq!(number_of("README.md"), None);
        assert_eq!(number_of("Dxyz-nope.md"), None);
    }

    /// The index links by path, and only a `.md` under `decisions/` is a row.
    #[test]
    fn rows_are_read_from_the_links_not_from_the_prose() {
        let text = "\
| 🟢 | D001 | [Two builds](decisions/D001-two-builds.md) | decided | 2026-01-01 |
front matter mentioning decisions/ and a stray ) bracket
| 🟡 | D002 | [Another](decisions/D002-another.md) | assumed | 2026-01-02 |
";
        assert_eq!(
            linked(text),
            vec![
                "D001-two-builds.md".to_owned(),
                "D002-another.md".to_owned()
            ]
        );
    }

    /// An index of unlabelled links looks navigable and is not, which is the failure the
    /// index exists to prevent. Eight rows were in that state before this was written.
    #[test]
    fn a_row_with_no_title_is_found() {
        let text = "\
| 🟢 | D001 | [Two builds](decisions/D001-two-builds.md) | decided | 2026-01-01 |
| 🟢 | D302 | [](decisions/D302-the-matrix.md) | decided | 2026-01-02 |
";
        assert_eq!(untitled(text), vec!["D302-the-matrix.md".to_owned()]);
        assert!(
            untitled("| 🟢 | D001 | [Two builds](decisions/D001-two-builds.md) |").is_empty(),
            "a titled row must not be reported"
        );
    }

    /// The guard nobody had watched reject anything. An empty directory must not read as
    /// agreement - that is the failure this rewrite exists to name.
    #[test]
    fn an_empty_directory_is_refused() {
        let dir = std::env::temp_dir().join("obscene-decisions-empty");
        let docs = dir.join("docs").join("decisions");
        std::fs::create_dir_all(&docs).expect("temp dirs");
        std::fs::write(dir.join("docs").join("DECISIONS.md"), "# Decision log\n").expect("index");
        assert!(!run(&dir).expect("run"), "nothing to check is not a pass");
        std::fs::remove_dir_all(&dir).ok();
    }

    /// A file with no row is invisible to a reader of the log, so it fails.
    #[test]
    fn a_file_without_a_row_is_refused() {
        let dir = std::env::temp_dir().join("obscene-decisions-orphan");
        let docs = dir.join("docs").join("decisions");
        std::fs::create_dir_all(&docs).expect("temp dirs");
        std::fs::write(docs.join("D001-listed.md"), "# D001\n").expect("entry");
        std::fs::write(docs.join("D002-orphan.md"), "# D002\n").expect("entry");
        std::fs::write(
            dir.join("docs").join("DECISIONS.md"),
            "| 🟢 | D001 | [Listed](decisions/D001-listed.md) | decided | 2026-01-01 |\n",
        )
        .expect("index");
        assert!(!run(&dir).expect("run"), "an unlisted entry must fail");
        std::fs::remove_dir_all(&dir).ok();
    }

    /// And the positive case, so the negatives above are known to be about the thing tested.
    #[test]
    fn a_directory_matching_its_index_passes() {
        let dir = std::env::temp_dir().join("obscene-decisions-agree");
        let docs = dir.join("docs").join("decisions");
        std::fs::create_dir_all(&docs).expect("temp dirs");
        std::fs::write(docs.join("D001-listed.md"), "# D001\n").expect("entry");
        std::fs::write(
            dir.join("docs").join("DECISIONS.md"),
            "| 🟢 | D001 | [Listed](decisions/D001-listed.md) | decided | 2026-01-01 |\n",
        )
        .expect("index");
        assert!(run(&dir).expect("run"), "agreement must pass");
        std::fs::remove_dir_all(&dir).ok();
    }
}
