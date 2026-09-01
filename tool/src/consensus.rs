//! Agreement between independent implementations, used as a substitute oracle.
//!
//! # The problem this addresses
//!
//! Nothing in this suite carries `hardware` provenance, and until a console is available
//! nothing will. That caps what a report can claim: when obSCEne and an emulator disagree,
//! the report cannot say which is wrong. A `fail [assumed]` is a verdict the recipient can
//! decline without argument, and declining it is often reasonable.
//!
//! There is an oracle available that is not a console: **several independent
//! implementations agreeing.** Four emulators, one message - "you are the only one of four
//! that fails this" - is actionable in a way "you failed check X" is not, and it needs no
//! authority beyond the reports themselves.
//!
//! # What it must not pretend
//!
//! **These implementations are not independent of each other.** They read each other's
//! source; one's NID table is a superset of another's database. This project already made
//! the mistake of counting one source twice while writing up how well corroborated
//! something was (D064).
//!
//! So the output names *which* implementations agreed, always, and never reduces agreement
//! to a bare count. A reader who knows two of the four share a lineage can discount
//! accordingly; a number would have hidden that.
//!
//! # Reading the result
//!
//! Only disagreements are printed. Unanimity is the uninteresting case and there is a lot
//! of it, so a tool that printed everything would bury the finding.
//!
//! An **outlier** - one implementation differing from a unanimous rest - is the strong
//! signal. A **split** is weaker and often more interesting: it means the platform's
//! behaviour is genuinely unsettled between implementations, and that is a question for
//! hardware rather than a bug report.

use std::collections::{BTreeMap, BTreeSet};

use crate::report::{Report, Status};

/// One check, and what each implementation said about it.
#[derive(Debug, Clone)]
pub struct Disagreement {
    pub check: String,
    /// Status to the implementations that reported it, in name order.
    pub by_status: BTreeMap<String, Vec<String>>,
}

impl Disagreement {
    /// The lone dissenter, when exactly one implementation differs from a unanimous rest.
    ///
    /// The actionable shape: it names an implementation and a check, and the claim it
    /// supports - "everyone else does this differently" - does not require anyone to
    /// accept this project's expectation.
    #[must_use]
    pub fn outlier(&self) -> Option<(&str, &str)> {
        if self.by_status.len() != 2 {
            return None;
        }
        let mut iter = self.by_status.iter();
        let (first_status, first) = iter.next()?;
        let (second_status, second) = iter.next()?;
        if first.len() == 1 && second.len() > 1 {
            return Some((first.first()?.as_str(), first_status.as_str()));
        }
        if second.len() == 1 && first.len() > 1 {
            return Some((second.first()?.as_str(), second_status.as_str()));
        }
        None
    }
}

/// What a run of `consensus` found.
#[derive(Debug, Default)]
pub struct Consensus {
    /// Implementation names, in the order given.
    pub implementations: Vec<String>,
    /// Checks every implementation agreed on.
    pub unanimous: usize,
    /// Checks they did not agree on, worst-shaped first.
    pub disagreements: Vec<Disagreement>,
    /// Checks absent from at least one report.
    ///
    /// Not a disagreement: a check missing because that run died earlier says something
    /// about the run, not about the check, and folding the two together would invent
    /// findings.
    pub partial_coverage: Vec<String>,
    /// Checks too few implementations actually attempted to compare.
    ///
    /// A `skip` is not an opinion. See `compare`.
    pub not_comparable: usize,
}

/// Compares reports from several implementations.
///
/// `named` pairs a name with its report. Names are the caller's to choose and appear
/// verbatim in the output, because which implementation said what is the whole point.
#[must_use]
pub fn compare(named: &[(String, Report)]) -> Consensus {
    let mut out = Consensus {
        implementations: named.iter().map(|(name, _)| name.clone()).collect(),
        ..Consensus::default()
    };

    // Every check any report mentions, so one report having more checks than another is
    // visible as partial coverage rather than silently dropping the extras.
    let mut every_check: BTreeSet<&str> = BTreeSet::new();
    for (_, report) in named {
        for result in &report.results {
            every_check.insert(result.id.as_str());
        }
    }

    for check in every_check {
        let mut by_status: BTreeMap<String, Vec<String>> = BTreeMap::new();
        let mut missing = false;
        for (name, report) in named {
            match report.results.iter().find(|r| r.id == check) {
                // **A skip is not an opinion.** It means the implementation did not
                // attempt the check - the symbol was absent, a prerequisite did not hold,
                // or it was excluded at build time - so it has said nothing about the
                // behaviour and cannot disagree about it.
                //
                // Counting skips as verdicts made the first run of this tool report 80
                // disagreements out of 126, almost all of them "this platform has the
                // function and that one does not". True, useless, and enough noise to
                // bury the handful of real behavioural differences.
                Some(result) if result.status == Status::Skip => {}
                Some(result) => by_status
                    .entry(result.status.name().to_owned())
                    .or_default()
                    .push(name.clone()),
                None => missing = true,
            }
        }
        if missing {
            out.partial_coverage.push(check.to_owned());
            continue;
        }
        // One opinion is not a consensus and no opinions is not a disagreement.
        let opinions: usize = by_status.values().map(Vec::len).sum();
        if opinions < 2 {
            out.not_comparable = out.not_comparable.saturating_add(1);
            continue;
        }
        if by_status.len() <= 1 {
            out.unanimous = out.unanimous.saturating_add(1);
        } else {
            out.disagreements.push(Disagreement {
                check: check.to_owned(),
                by_status,
            });
        }
    }

    // Outliers first: they are the ones a reader can act on without adjudicating.
    out.disagreements
        .sort_by_key(|d| (d.outlier().is_none(), d.check.clone()));
    out
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    reason = "test fixtures build known-size inputs; a panic here is the failure signal"
)]
mod tests {
    use super::compare;
    use crate::report::Report;

    fn report(lines: &[&str]) -> Report {
        Report::parse(&lines.join("\n"))
    }

    #[test]
    fn unanimity_is_not_reported() {
        let a = report(&["OBS|res|010-kernel/write|pass|||spec"]);
        let b = report(&["OBS|res|010-kernel/write|pass|||spec"]);
        let out = compare(&[("a".to_owned(), a), ("b".to_owned(), b)]);
        assert_eq!(out.unanimous, 1);
        assert!(out.disagreements.is_empty());
    }

    #[test]
    fn a_lone_dissenter_is_named() {
        // The actionable shape, and the reason this tool exists: it supports "you are the
        // only one" without anyone having to accept our expectation.
        let out = compare(&[
            ("one".to_owned(), report(&["OBS|res|a/b|pass|||spec"])),
            ("two".to_owned(), report(&["OBS|res|a/b|pass|||spec"])),
            ("odd".to_owned(), report(&["OBS|res|a/b|fail||why|spec"])),
        ]);
        assert_eq!(out.disagreements.len(), 1);
        let (who, what) = out.disagreements[0].outlier().expect("an outlier");
        assert_eq!(who, "odd");
        assert_eq!(what, "fail");
    }

    #[test]
    fn an_even_split_has_no_outlier() {
        // Two against two is not one implementation being wrong, and saying it was would
        // be this tool inventing a majority.
        let out = compare(&[
            ("a".to_owned(), report(&["OBS|res|x/y|pass|||spec"])),
            ("b".to_owned(), report(&["OBS|res|x/y|pass|||spec"])),
            ("c".to_owned(), report(&["OBS|res|x/y|fail||no|spec"])),
            ("d".to_owned(), report(&["OBS|res|x/y|fail||no|spec"])),
        ]);
        assert_eq!(out.disagreements.len(), 1);
        assert!(out.disagreements[0].outlier().is_none());
    }

    #[test]
    fn a_check_one_report_lacks_is_coverage_not_disagreement() {
        // A run that died early is missing checks. Counting that as disagreement would
        // manufacture a finding out of a crash that is already reported elsewhere.
        let out = compare(&[
            ("full".to_owned(), report(&["OBS|res|a/b|pass|||spec"])),
            ("short".to_owned(), report(&["OBS|res|c/d|pass|||spec"])),
        ]);
        assert!(out.disagreements.is_empty());
        assert_eq!(out.partial_coverage.len(), 2);
    }

    #[test]
    fn a_skip_is_not_an_opinion() {
        // The rule that makes this tool usable rather than noise. An implementation that
        // does not have the function has said nothing about how it should behave.
        let out = compare(&[
            ("has_it".to_owned(), report(&["OBS|res|a/b|pass|||spec"])),
            (
                "lacks_it".to_owned(),
                report(&["OBS|res|a/b|skip||absent|spec"]),
            ),
        ]);
        assert!(out.disagreements.is_empty());
        assert_eq!(out.not_comparable, 1);
    }

    #[test]
    fn one_opinion_among_skips_is_not_a_consensus() {
        let out = compare(&[
            ("a".to_owned(), report(&["OBS|res|a/b|fail||x|spec"])),
            ("b".to_owned(), report(&["OBS|res|a/b|skip||absent|spec"])),
            ("c".to_owned(), report(&["OBS|res|a/b|skip||absent|spec"])),
        ]);
        assert_eq!(out.not_comparable, 1);
        assert_eq!(out.unanimous, 0);
    }

    #[test]
    fn implementations_are_named_in_the_result() {
        // Never reduced to a count: two of these may share a lineage, and a reader can
        // only discount that if the names survive. See D064.
        let out = compare(&[
            ("shadps4".to_owned(), report(&["OBS|res|a/b|pass|||spec"])),
            ("kyty".to_owned(), report(&["OBS|res|a/b|fail||x|spec"])),
        ]);
        assert_eq!(out.implementations, vec!["shadps4", "kyty"]);
        assert_eq!(out.disagreements[0].by_status["pass"], vec!["shadps4"]);
    }
}
