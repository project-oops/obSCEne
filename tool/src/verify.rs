//! Checking a report against its own format contract.
//!
//! This is the harness testing itself. The C side has no unit-test framework and cannot
//! reasonably have one - it builds freestanding - so the invariants that would normally
//! be unit tests are asserted here against a real captured report.
//!
//! # It says nothing about whether the checks passed
//!
//! A report that is entirely red is perfectly well-formed, and on a host build that is
//! the expected outcome. The question here is "can I believe this file", not "is the
//! platform any good". Conflating the two would make the tool useless in exactly the
//! situation it exists for.

use std::collections::BTreeSet;

use crate::report::Report;

/// Something wrong with a report.
pub type Problem = String;

/// Every invariant the format promises.
#[must_use]
pub fn check(report: &Report) -> Vec<Problem> {
    let mut problems = Vec::new();

    if report.meta.is_none() {
        problems.push("no meta record".to_owned());
    }
    if report.build.is_none() {
        // Without it a diff cannot tell "the probe changed" from "the platform
        // changed", which are very different answers to "did that help?".
        problems.push("no build record, so this report cannot be attributed".to_owned());
    }
    if !report.ended {
        problems.push("the stream has no end record, so it was truncated".to_owned());
    }
    if let Some(id) = &report.unresolved_attempt {
        problems.push(format!("{id} was announced and never resolved"));
    }

    if let Some((_, sections, checks)) = report.meta {
        if sections as usize != report.sections.len() {
            problems.push(format!(
                "meta declares {sections} sections, {} were reported",
                report.sections.len()
            ));
        }
        if checks as usize != report.results.len() {
            problems.push(format!(
                "meta declares {checks} checks, {} were reported",
                report.results.len()
            ));
        }
    }

    duplicate_ids(report, &mut problems);
    duplicate_symbols(report, &mut problems);
    tallies(report, &mut problems);
    ordering(report, &mut problems);

    problems
}

/// Identifiers are the key when diffing, so a duplicate makes a comparison silently
/// ambiguous rather than loudly wrong.
fn duplicate_ids(report: &Report, problems: &mut Vec<Problem>) {
    let mut seen = BTreeSet::new();
    for result in &report.results {
        if !seen.insert(result.id.as_str()) {
            problems.push(format!("duplicate check identifier {}", result.id));
        }
    }
}

/// The same reasoning one level down: a symbol censused twice cannot be diffed.
fn duplicate_symbols(report: &Report, problems: &mut Vec<Problem>) {
    let mut seen = BTreeSet::new();
    for symbol in &report.symbols {
        if !seen.insert((symbol.library.as_str(), symbol.name.as_str())) {
            problems.push(format!(
                "censused twice: {}:{}",
                symbol.library, symbol.name
            ));
        }
    }
}

/// What the report declares must match what it recorded.
fn tallies(report: &Report, problems: &mut Vec<Problem>) {
    let counted = report.counted();
    match report.tally {
        None => problems.push("no final tally".to_owned()),
        Some(declared) => {
            if declared != counted {
                problems.push(format!(
                    "final tally {declared:?} does not match the records {counted:?}"
                ));
            }
            if declared.total() as usize != report.results.len() {
                problems.push("the tally does not account for every check".to_owned());
            }
        }
    }
}

/// Section identifiers carry a numeric prefix precisely so base-to-high-level order is
/// verifiable rather than merely intended.
fn ordering(report: &Report, problems: &mut Vec<Problem>) {
    let ids: Vec<&str> = report.sections.iter().map(|s| s.id.as_str()).collect();
    let mut sorted = ids.clone();
    sorted.sort_unstable();
    if ids != sorted {
        problems.push(format!("sections are out of order: {ids:?}"));
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
    use super::check;
    use crate::report::Report;

    fn sound() -> String {
        "\
OBS|meta|1|1|2
OBS|build|abc123
OBS|section|000-boot|Boot|purpose
OBS|res|000-boot/a|pass||
OBS|res|000-boot/b|fail||
OBS|tally|1|0|1|0
OBS|end
"
        .to_owned()
    }

    #[test]
    fn a_sound_report_has_no_problems() {
        assert!(check(&Report::parse(&sound())).is_empty());
    }

    #[test]
    fn an_all_red_report_is_still_well_formed() {
        // The distinction the whole module rests on: this asks whether the file can be
        // believed, not whether the platform is any good.
        let text = "\
OBS|meta|1|1|1
OBS|build|abc
OBS|section|000-boot|Boot|purpose
OBS|res|000-boot/a|fail||everything is broken
OBS|tally|0|0|1|0
OBS|end
";
        assert!(check(&Report::parse(text)).is_empty());
    }

    #[test]
    fn a_duplicated_identifier_is_caught() {
        let text = sound().replace("000-boot/b", "000-boot/a");
        let problems = check(&Report::parse(&text));
        assert!(
            problems
                .iter()
                .any(|p| p.contains("duplicate check identifier"))
        );
    }

    #[test]
    fn a_missing_build_record_is_caught() {
        let text = sound().replace("OBS|build|abc123\n", "");
        let problems = check(&Report::parse(&text));
        assert!(problems.iter().any(|p| p.contains("cannot be attributed")));
    }

    #[test]
    fn a_truncated_stream_is_caught() {
        let text = sound().replace("OBS|end\n", "");
        let problems = check(&Report::parse(&text));
        assert!(problems.iter().any(|p| p.contains("truncated")));
    }

    #[test]
    fn an_announcement_without_a_result_is_caught() {
        let text = sound().replace(
            "OBS|res|000-boot/b|fail||\n",
            "OBS|try|000-boot/b|libkernel|sceKernelWrite\n",
        );
        let problems = check(&Report::parse(&text));
        assert!(problems.iter().any(|p| p.contains("never resolved")));
    }

    #[test]
    fn a_tally_that_does_not_add_up_is_caught() {
        let text = sound().replace("OBS|tally|1|0|1|0", "OBS|tally|9|0|0|0");
        let problems = check(&Report::parse(&text));
        assert!(
            problems
                .iter()
                .any(|p| p.contains("does not match the records"))
        );
    }

    #[test]
    fn sections_out_of_order_are_caught() {
        // Ordering is the whole value of the report, so it is enforced rather than
        // assumed.
        let text = "\
OBS|meta|1|2|0
OBS|build|abc
OBS|section|100-input|Input|purpose
OBS|section|010-kernel|Kernel|purpose
OBS|tally|0|0|0|0
OBS|end
";
        let problems = check(&Report::parse(text));
        assert!(problems.iter().any(|p| p.contains("out of order")));
    }

    #[test]
    fn a_symbol_censused_twice_is_caught() {
        let text = "\
OBS|meta|1|0|0
OBS|build|abc
OBS|sym|libkernel|sceKernelPread|absent|shared
OBS|sym|libkernel|sceKernelPread|present|shared
OBS|tally|0|0|0|0
OBS|end
";
        let problems = check(&Report::parse(text));
        assert!(problems.iter().any(|p| p.contains("censused twice")));
    }
}
