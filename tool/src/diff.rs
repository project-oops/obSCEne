//! Comparing two reports.
//!
//! The reason the report format is machine-readable at all. Every awkward choice on the
//! C side - stable identifiers, one record per line, values as raw numbers rather than
//! prose - was made so this comparison is mechanical rather than a reading exercise.
//!
//! # A regression is a check that got *worse*
//!
//! Not a check that is failing. Under an early emulator almost everything fails, and a
//! tool that reported that as a permanent regression would be ignored within a day. So
//! a run where everything fails and nothing changed is a clean exit - correct, and it
//! looks wrong until you think about when this is used.
//!
//! Losing a check counts as getting worse: `Skip` ranks below `Fail`, and a check that
//! vanishes from the report entirely is treated the same way. Otherwise deleting an
//! inconvenient check would read as progress.

use crate::report::{CheckResult, Report, Status, Tally};

/// What changed between two runs.
#[derive(Debug, Default)]
pub struct Comparison {
    /// Checks that got better, with their old and new status.
    pub improved: Vec<Transition>,
    /// Checks that got worse.
    pub regressed: Vec<Transition>,
    /// Checks present before and absent after. Lost coverage, so a regression.
    pub vanished: Vec<String>,
    /// Checks whose verdict held but whose observed value moved.
    pub changed_values: Vec<ValueChange>,
    /// Checks whose verdict and value both held.
    pub unchanged: usize,
    /// Symbols that became resolvable.
    pub now_present: Vec<(String, String)>,
    /// Symbols that stopped resolving.
    pub now_absent: Vec<(String, String)>,
    /// The two builds, when they differ.
    pub build_changed: Option<(String, String)>,
    /// Set when the two reports came from different target shapes.
    ///
    /// Not a difference to report - a reason the comparison is meaningless.
    pub target_mismatch: Option<(String, String)>,
    /// Tally movement, when both reports carry one.
    pub tally_delta: Option<(Tally, Tally)>,
}

/// One check's change of verdict.
#[derive(Debug, Clone)]
pub struct Transition {
    /// Which check.
    pub id: String,
    /// What it was.
    pub before: Status,
    /// What it became.
    pub after: Status,
}

/// A check whose verdict held but whose observed value moved.
///
/// Not a regression, and often the most informative line in a diff: the same verdict
/// with a different number means something underneath changed.
#[derive(Debug, Clone)]
pub struct ValueChange {
    /// Which check.
    pub id: String,
    /// What it observed before.
    pub before: String,
    /// What it observes now.
    pub after: String,
}

impl Comparison {
    /// Whether anything got worse.
    #[must_use]
    pub fn has_regression(&self) -> bool {
        !self.regressed.is_empty() || !self.vanished.is_empty() || !self.now_absent.is_empty()
    }

    /// How many things got worse, counting lost checks.
    #[must_use]
    pub fn regression_count(&self) -> usize {
        // `now_absent` belongs here because `has_regression` counts it, and the two
        // disagreeing produced a report that printed "0 regressed" and then exited 1.
        // The exit code is a documented contract (docs/OUTPUT.md), so a summary line
        // that contradicts it is worse than either alone: it tells a reader the tool is
        // broken at the exact moment it is right.
        self.regressed
            .len()
            .saturating_add(self.vanished.len())
            .saturating_add(self.now_absent.len())
    }
}

/// Compares two reports.
#[must_use]
pub fn compare(before: &Report, after: &Report) -> Comparison {
    let mut out = Comparison::default();

    if before.build != after.build {
        // Worth saying loudly: when the probe itself changed, a difference in results
        // is not necessarily a difference in the platform.
        out.build_changed = Some((
            before.build.clone().unwrap_or_else(|| "unknown".to_owned()),
            after.build.clone().unwrap_or_else(|| "unknown".to_owned()),
        ));
    }

    // Two targets run identical checks through different loaders. A module run and a
    // payload run therefore differ because of the loader, not because anything
    // changed, and every line of the comparison would be noise presented as signal.
    if before.target != after.target {
        out.target_mismatch = Some((
            before
                .target
                .clone()
                .unwrap_or_else(|| "unknown".to_owned()),
            after.target.clone().unwrap_or_else(|| "unknown".to_owned()),
        ));
        return out;
    }

    let old = before.by_id();
    for result in &after.results {
        let Some(previous) = old.get(result.id.as_str()) else {
            // A check that did not exist before is new coverage, not a regression.
            out.improved.push(Transition {
                id: result.id.clone(),
                before: Status::Skip,
                after: result.status,
            });
            continue;
        };
        classify(previous, result, &mut out);
    }

    let new = after.by_id();
    out.vanished = before
        .results
        .iter()
        .map(|result| result.id.clone())
        .filter(|id| !new.contains_key(id.as_str()))
        .collect();

    compare_symbols(before, after, &mut out);

    if let (Some(a), Some(b)) = (before.tally, after.tally) {
        out.tally_delta = Some((a, b));
    }

    out
}

/// Places one check into improved, regressed, or unchanged.
fn classify(previous: &CheckResult, current: &CheckResult, out: &mut Comparison) {
    if previous.status == current.status {
        if previous.value == current.value {
            out.unchanged = out.unchanged.saturating_add(1);
        } else {
            out.changed_values.push(ValueChange {
                id: current.id.clone(),
                before: previous.value.clone(),
                after: current.value.clone(),
            });
        }
        return;
    }
    let transition = Transition {
        id: current.id.clone(),
        before: previous.status,
        after: current.status,
    };
    // Status ordering does the work: Skip < Fail < Partial < Pass.
    if current.status > previous.status {
        out.improved.push(transition);
    } else {
        out.regressed.push(transition);
    }
}

/// Symbols that changed resolvability.
fn compare_symbols(before: &Report, after: &Report, out: &mut Comparison) {
    let old = before.symbols_by_key();
    for symbol in &after.symbols {
        let key = (symbol.library.as_str(), symbol.name.as_str());
        let Some(previous) = old.get(&key) else {
            continue;
        };
        if previous.present == symbol.present {
            continue;
        }
        let entry = (symbol.library.clone(), symbol.name.clone());
        if symbol.present {
            out.now_present.push(entry);
        } else {
            out.now_absent.push(entry);
        }
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
    use std::fmt::Write as _;

    use super::compare;
    use crate::report::Report;

    fn report(build: &str, results: &[(&str, &str)]) -> Report {
        let mut text = format!("OBS|meta|1|1|{}\nOBS|build|{build}\n", results.len());
        for (id, status) in results {
            let _ = writeln!(text, "OBS|res|{id}|{status}||");
        }
        text.push_str("OBS|tally|0|0|0|0\nOBS|end\n");
        Report::parse(&text)
    }

    #[test]
    fn a_fail_becoming_a_pass_is_an_improvement() {
        let out = compare(
            &report("a", &[("x", "fail")]),
            &report("a", &[("x", "pass")]),
        );
        assert_eq!(out.improved.len(), 1);
        assert!(!out.has_regression());
    }

    #[test]
    fn a_pass_becoming_a_fail_is_a_regression() {
        let out = compare(
            &report("a", &[("x", "pass")]),
            &report("a", &[("x", "fail")]),
        );
        assert_eq!(out.regressed.len(), 1);
        assert!(out.has_regression());
    }

    #[test]
    fn everything_failing_unchanged_is_not_a_regression() {
        // The case the tool exists for. Under an early emulator almost everything
        // fails, and reporting that as a permanent regression would make it useless.
        let before = report("a", &[("x", "fail"), ("y", "fail")]);
        let after = report("a", &[("x", "fail"), ("y", "fail")]);
        let out = compare(&before, &after);
        assert!(!out.has_regression());
        assert_eq!(out.unchanged, 2);
    }

    #[test]
    fn a_check_that_stops_running_counts_as_getting_worse() {
        // Skip ranks below Fail: a check that did not run tells you less than one that
        // ran and failed.
        let out = compare(
            &report("a", &[("x", "fail")]),
            &report("a", &[("x", "skip")]),
        );
        assert_eq!(out.regressed.len(), 1);
    }

    #[test]
    fn a_vanished_check_is_a_regression_not_a_tidy_up() {
        // Otherwise deleting an inconvenient check would read as progress.
        let out = compare(
            &report("a", &[("x", "pass"), ("y", "pass")]),
            &report("a", &[("x", "pass")]),
        );
        assert_eq!(out.vanished, vec!["y".to_owned()]);
        assert!(out.has_regression());
    }

    #[test]
    fn a_new_check_is_coverage_rather_than_a_regression() {
        let out = compare(
            &report("a", &[("x", "pass")]),
            &report("a", &[("x", "pass"), ("y", "fail")]),
        );
        assert_eq!(out.improved.len(), 1);
        assert!(!out.has_regression());
    }

    #[test]
    fn a_changed_build_is_surfaced() {
        // When the probe changed, a difference in results is not necessarily a
        // difference in the platform.
        let out = compare(
            &report("old", &[("x", "pass")]),
            &report("new", &[("x", "pass")]),
        );
        assert!(out.build_changed.is_some());
    }

    #[test]
    fn the_same_verdict_with_a_different_value_is_reported_but_not_a_regression() {
        let before = Report::parse("OBS|build|a\nOBS|res|x|pass|0x1|\nOBS|end\n");
        let after = Report::parse("OBS|build|a\nOBS|res|x|pass|0x2|\nOBS|end\n");
        let out = compare(&before, &after);
        assert_eq!(out.changed_values.len(), 1);
        assert!(!out.has_regression());
    }

    #[test]
    fn a_symbol_that_stops_resolving_is_a_regression() {
        let before = Report::parse("OBS|build|a\nOBS|sym|libkernel|f|present|shared\nOBS|end\n");
        let after = Report::parse("OBS|build|a\nOBS|sym|libkernel|f|absent|shared\nOBS|end\n");
        let out = compare(&before, &after);
        assert_eq!(out.now_absent.len(), 1);
        assert!(out.has_regression());
    }
}
