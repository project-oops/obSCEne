//! Does the table parser see every check that actually ran?
//!
//! # The hole this closes
//!
//! `sections.rs` exists because three separate tools hand-rolled a pattern for "a row in a
//! check table" and all three were wrong in different ways. Its own module documentation ends
//! on the part nobody had an answer for:
//!
//! > Every one of those failed by producing a *smaller number*, which no gate can detect,
//! > because a gate compares against nothing.
//!
//! That was true while the only readings came from the source. **A report is a second
//! reading**, and it is the authoritative one: the harness walks the tables at run time and
//! emits a `res` record per check, so a report says what actually ran rather than what a
//! regular expression believes is there.
//!
//! Comparing the two gives the parser something to be wrong against.
//!
//! # It was not hypothetical for long
//!
//! A comment placed *inside* a table row's braces - between the capability fields and the
//! runner - makes the row invisible to the parser, because the field before `OBS_FROM_*` is
//! then a block comment rather than an identifier. The check still compiles and still runs.
//! `guards` silently stopped checking it, `caps` stopped ordering it, and `counts` reported
//! one fewer. Nothing failed. It was found by hand-counting while writing something else.
//! (D168)

use crate::sections;
use std::collections::BTreeSet;
use std::path::Path;

/// Check identifiers the harness reported running.
///
/// Read from `res` records rather than `try`: a skipped check emits no `try` (the
/// announce-before-attempting rule) but does emit a result, so `try` would under-report by
/// exactly the set of checks that were skipped - reintroducing the same failure in the
/// measuring instrument.
pub fn ran(report: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in report.lines() {
        let mut fields = line.split('|');
        if fields.next() != Some("OBS") || fields.next() != Some("res") {
            continue;
        }
        if let Some(id) = fields.next().filter(|id| !id.is_empty()) {
            out.insert(id.to_owned());
        }
    }
    out
}

/// Compare the parser's reading of the source against a report.
pub fn run(root: &Path, report_path: &Path) -> std::io::Result<bool> {
    let report = std::fs::read_to_string(report_path)?;
    let reported = ran(&report);
    if reported.is_empty() {
        println!(
            "no result records in {} - not a report?",
            report_path.display()
        );
        return Ok(false);
    }

    let parsed: BTreeSet<String> = sections::rows(&sections::find_dir(root, "sections"))?
        .into_iter()
        .map(|row| row.id)
        .collect();

    // Sections whose tables expand a macro, and therefore have rows no text parser can see.
    //
    // `900-surface` writes two rows by hand and builds 368 more with
    // `OBS_SURFACE_LIBRARIES(OBS_GROUP_ROW)` over the census. Those exist only after the
    // preprocessor runs. That is a property of the design and not a fault to report on every
    // invocation - a gate that cries wolf 368 times is a gate somebody switches off.
    //
    // Detected from the *source*, by looking for a table-macro expansion, rather than by
    // naming the section. A hardcoded exemption rots quietly; this one stops applying the
    // moment the macro does.
    //
    // The first attempt tried "a section with no literal rows is generated" and was wrong for
    // exactly the case in hand: `900-surface` has both kinds, so every generated row was
    // reported as missed. The discrimination has to be per-file, not per-row.
    let mut generated_sections: BTreeSet<String> = BTreeSet::new();
    let dir = sections::find_dir(root, "sections");
    for entry in std::fs::read_dir(&dir)?.filter_map(Result::ok) {
        let path = entry.path();
        if path.extension().is_none_or(|e| e != "c") {
            continue;
        }
        let text = std::fs::read_to_string(&path)?;
        // An expansion used as a table row, as opposed to the `#define` that introduces it.
        // A `#define` line does not begin with `OBS_` once trimmed, so testing for the
        // expansion is enough to tell a use from the definition that introduces it.
        let expands = text
            .lines()
            .any(|l| l.trim_start().starts_with("OBS_") && l.contains("_LIBRARIES(OBS_"));
        if !expands {
            continue;
        }
        for row in sections::rows_in(&text) {
            if let Some((section, _)) = row.id.split_once('/') {
                generated_sections.insert(section.to_owned());
            }
        }
    }

    let mut generated = 0usize;
    let mut missed: Vec<&String> = Vec::new();
    for id in reported.difference(&parsed) {
        let section = id.split_once('/').map_or(id.as_str(), |(s, _)| s);
        if generated_sections.contains(section) {
            generated = generated.saturating_add(1);
        } else {
            missed.push(id);
        }
    }
    if generated != 0 {
        println!("{generated} check(s) come from a table macro and are not source rows");
    }
    // Seen by the parser but never run. Not necessarily wrong - a report from a build with
    // sections excluded, or from another version - so it is reported without failing.
    let unseen: Vec<&String> = parsed.difference(&reported).collect();

    if !unseen.is_empty() {
        println!(
            "{} check(s) in the tables did not run in this report (excluded build, or an \
             older report):",
            unseen.len()
        );
        for id in unseen.iter().take(10) {
            println!("  {id}");
        }
    }

    if missed.is_empty() {
        println!(
            "rows: the parser sees all {} checks this report ran",
            reported.len()
        );
        return Ok(true);
    }

    println!(
        "\n{} check(s) RAN but the table parser cannot see them:",
        missed.len()
    );
    for id in &missed {
        println!("  {id}");
    }
    println!("\nEvery gate built on the check tables - guards, caps, counts - is silently");
    println!("skipping these. The usual cause is a comment inside the row's braces: the");
    println!("parser requires the field before OBS_FROM_* to be an identifier. Move it above");
    println!("the row. See D168.");
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn results_name_the_checks_that_ran() {
        let report = "OBS|res|015-sync/mutex|pass|||spec\n\
                      OBS|res|020-memory/map|fail|because|0x1|assumed\n\
                      OBS|try|030-thread/self|libkernel|scePthreadSelf\n";
        let ids = ran(report);
        assert_eq!(ids.len(), 2);
        assert!(ids.contains("015-sync/mutex"));
        assert!(ids.contains("020-memory/map"));
    }

    /// A skipped check emits no `try` and does emit a `res`. Reading `try` would under-report
    /// by exactly the skipped set - the same failure this gate exists to catch, committed by
    /// the gate.
    #[test]
    fn a_skipped_check_still_counts() {
        let report = "OBS|res|040-file/open|skip|the symbol is not present||assumed\n";
        assert!(ran(report).contains("040-file/open"));
    }

    #[test]
    fn other_records_are_not_checks() {
        let report = "OBS|section|015-sync|Synchronisation|prose\n\
                      OBS|call|libkernel|sceKernelClose|0x2a|rejected|0x80020009\n\
                      OBS|measure|130-layout/x|sym|quantity|0x1|unit\n";
        assert!(ran(report).is_empty());
    }
}
