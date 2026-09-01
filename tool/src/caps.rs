//! Can every check's prerequisite capability actually exist by the time it runs?
//!
//! # The bug this exists to catch
//!
//! A check declares what it needs (`requires`) and what a passing run of it establishes
//! (`provides`), and `harness.c` grants them **in running order** starting from nothing. So a
//! requirement is satisfiable only if some earlier check grants it. Nothing checked that.
//!
//! Two checks in `018-relational` required `OBS_CAP_FILE`, which is granted in `040-file` -
//! twenty-two sections later. Both were unreachable on every target, host included, from the
//! day they were written. They did not fail; they reported
//!
//! ```text
//! skip: a prerequisite capability was not established
//! ```
//!
//! which is exactly what a genuinely fileless platform would report. That is why it survived
//! every reading of every report: the message was true, and it was about the wrong thing.
//!
//! # Impossibility, not flakiness
//!
//! This gate cannot know whether a check passes, and the harness only grants a capability on
//! a pass. So it assumes the best case - every check reached grants what it claims - and
//! reports only requirements that fail *even then*. A capability that goes ungranted because
//! its check failed on some platform is a result about that platform. A capability that
//! cannot be granted in time on any platform is a build-order mistake, and that is the whole
//! set this reports.

use crate::sections;
use std::collections::BTreeSet;
use std::path::Path;

/// One unsatisfiable requirement.
pub struct Impossible {
    /// The check that cannot run.
    pub id: String,
    /// The capability it asks for.
    pub wanted: String,
    /// Where that capability is granted, if anywhere.
    pub granted_by: Option<String>,
}

/// The capability tokens in one field, which may be an or-of-several.
fn tokens(field: &str) -> Vec<String> {
    field
        .split('|')
        .map(str::trim)
        .filter(|t| t.starts_with("OBS_CAP") && *t != "OBS_CAP_NONE")
        .map(str::to_owned)
        .collect()
}

/// Section identifiers in the order `registry.c` runs them.
///
/// Read from the registry rather than sorted by their numeric prefix. The prefixes happen to
/// ascend today, and a gate that assumed they always would is a gate that stops working the
/// first time someone reorders the list without renumbering - silently, and in the direction
/// of passing.
pub fn section_order(root: &Path) -> std::io::Result<Vec<String>> {
    let registry = std::fs::read_to_string(root.join("src").join("registry.c"))?;
    let mut variables = Vec::new();
    for line in registry.lines() {
        let trimmed = line.trim();
        let Some(rest) = trimmed.strip_prefix("&obs_section_") else {
            continue;
        };
        let name: String = rest.chars().take_while(|c| c.is_alphanumeric()).collect();
        if !name.is_empty() {
            variables.push(name);
        }
    }

    // Each section file ends with `const obs_section obs_section_<var> = {` followed by the
    // identifier string on the next line. Mapping through the variable rather than guessing
    // the identifier from the file name: `os.c` alone defines five sections.
    let mut names = std::collections::BTreeMap::new();
    let dir = root.join("src").join("sections");
    let mut paths: Vec<_> = std::fs::read_dir(&dir)?
        .filter_map(Result::ok)
        .map(|e| e.path())
        .filter(|p| p.extension().is_some_and(|e| e == "c"))
        .collect();
    paths.sort();
    for path in paths {
        let text = std::fs::read_to_string(&path)?;
        for chunk in text.split("const obs_section obs_section_").skip(1) {
            let variable: String = chunk.chars().take_while(|c| c.is_alphanumeric()).collect();
            let Some((_, after)) = chunk.split_once('"') else {
                continue;
            };
            let Some((id, _)) = after.split_once('"') else {
                continue;
            };
            names.insert(variable, id.to_owned());
        }
    }

    Ok(variables
        .into_iter()
        .filter_map(|v| names.get(&v).cloned())
        .collect())
}

/// Every check, in the order the harness runs it.
///
/// Source order within a section file *is* the running order - the table is an array and the
/// harness walks it - so the rows must not be sorted here. `sections::rows` sorts, which is
/// right for counting and wrong for this.
fn running_order(root: &Path) -> std::io::Result<Vec<sections::Row>> {
    let dir = root.join("src").join("sections");
    let mut paths: Vec<_> = std::fs::read_dir(&dir)?
        .filter_map(Result::ok)
        .map(|e| e.path())
        .filter(|p| p.extension().is_some_and(|e| e == "c"))
        .collect();
    paths.sort();

    let mut all = Vec::new();
    for path in paths {
        all.extend(sections::rows_in(&std::fs::read_to_string(&path)?));
    }

    let mut ordered = Vec::new();
    for section in section_order(root)? {
        let prefix = format!("{section}/");
        ordered.extend(all.iter().filter(|r| r.id.starts_with(&prefix)).cloned());
    }
    Ok(ordered)
}

/// Requirements no earlier check can satisfy.
pub fn unsatisfiable(rows: &[sections::Row]) -> Vec<Impossible> {
    // Where each capability first becomes available, by position.
    let mut granted: std::collections::BTreeMap<String, String> = std::collections::BTreeMap::new();
    let mut available: BTreeSet<String> = BTreeSet::new();
    let mut out = Vec::new();

    for row in rows {
        for wanted in tokens(&row.requires) {
            if !available.contains(&wanted) {
                out.push(Impossible {
                    id: row.id.clone(),
                    granted_by: granted.get(&wanted).cloned(),
                    wanted,
                });
            }
        }
        for given in tokens(&row.provides) {
            available.insert(given.clone());
            granted.entry(given).or_insert_with(|| row.id.clone());
        }
    }

    // A second pass, only to name where each capability is granted. The first pass reports a
    // requirement before its grant, so at that moment `granted` does not know about it yet -
    // and "nothing grants this at all" and "something grants it too late" are different
    // diagnoses that a reader needs told apart.
    for miss in &mut out {
        if miss.granted_by.is_none() {
            miss.granted_by = rows
                .iter()
                .find(|r| tokens(&r.provides).contains(&miss.wanted))
                .map(|r| r.id.clone());
        }
    }
    out
}

/// Run the gate.
pub fn run(root: &Path) -> std::io::Result<bool> {
    let rows = running_order(root)?;
    let misses = unsatisfiable(&rows);
    if misses.is_empty() {
        println!(
            "caps: {} checks, every prerequisite is granted before it is needed",
            rows.len()
        );
        return Ok(true);
    }
    println!(
        "{} check(s) require a capability that cannot exist yet:",
        misses.len()
    );
    for miss in &misses {
        match &miss.granted_by {
            Some(by) => println!(
                "  {} wants {} - granted later, by {by}",
                miss.id, miss.wanted
            ),
            None => println!("  {} wants {} - nothing grants it", miss.id, miss.wanted),
        }
    }
    println!("\nA check ordered ahead of its prerequisite never runs anywhere. It reports");
    println!("\"a prerequisite capability was not established\", which is what a platform");
    println!("genuinely lacking the capability reports, so the report looks reasonable.");
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn row(id: &str, requires: &str, provides: &str) -> sections::Row {
        sections::Row {
            id: id.to_owned(),
            library: "libkernel".to_owned(),
            symbol: "sym".to_owned(),
            runner: "check_x".to_owned(),
            provenance: "spec".to_owned(),
            requires: requires.to_owned(),
            provides: provides.to_owned(),
        }
    }

    #[test]
    fn a_grant_before_the_need_is_fine() {
        let rows = [
            row("040-file/open", "OBS_CAP_NONE", "OBS_CAP_FILE"),
            row("040-file/read", "OBS_CAP_FILE", "OBS_CAP_NONE"),
        ];
        assert!(unsatisfiable(&rows).is_empty());
    }

    /// The actual bug: `018-relational` required a capability `040-file` grants.
    #[test]
    fn a_grant_after_the_need_is_caught() {
        let rows = [
            row("018-relational/descriptors", "OBS_CAP_FILE", "OBS_CAP_NONE"),
            row("040-file/open", "OBS_CAP_NONE", "OBS_CAP_FILE"),
        ];
        let misses = unsatisfiable(&rows);
        assert_eq!(misses.len(), 1);
        let miss = misses.first().expect("one");
        assert_eq!(miss.id, "018-relational/descriptors");
        assert_eq!(miss.granted_by.as_deref(), Some("040-file/open"));
    }

    /// Told apart from the above, because the fix is different: one is a reorder, the other
    /// is a capability nobody establishes.
    #[test]
    fn a_capability_nothing_grants_is_named_as_such() {
        let rows = [row("080-video/open", "OBS_CAP_MEMORY", "OBS_CAP_NONE")];
        let misses = unsatisfiable(&rows);
        assert_eq!(misses.len(), 1);
        assert_eq!(misses.first().and_then(|m| m.granted_by.clone()), None);
    }

    /// `OBS_CAP_NONE` is the absence of a requirement, not a requirement named "none".
    #[test]
    fn none_is_not_a_capability() {
        assert!(tokens("OBS_CAP_NONE").is_empty());
        assert_eq!(tokens("OBS_CAP_FILE | OBS_CAP_MEMORY").len(), 2);
    }
}
