//! The conformance matrix: one capture per launch shape, and what differs between them.
//!
//! # Why this is not `diff` and not `consensus`
//!
//! [`crate::diff`] compares two runs of the *same* shape and answers "did it get worse". It
//! deliberately refuses a cross-target comparison, because two targets run the same checks
//! through different loaders and diffing them measures the loader rather than the change.
//!
//! [`crate::consensus`] compares N implementations and finds the outlier by majority. It was
//! written when **nothing in this suite carried hardware provenance and nothing was going to**,
//! and majority-agreement was the best substitute oracle available.
//!
//! Both premises have moved. A console now answers, so:
//!
//! - the cross-target comparison is the *point* rather than a confound - as long as each side
//!   is labelled with the shape it ran in, which is what this module's naming convention is
//!   for;
//! - and where hardware is present it is the **authority**, not one vote among several. An
//!   emulator disagreeing with three other emulators is interesting; an emulator disagreeing
//!   with the console is simply wrong.
//!
//! # The filename is the metadata
//!
//! A capture is named for the cell it fills, and nothing else identifies it:
//!
//! ```text
//! obscene-probe-<target>-<mode>-<privilege>-<category>.log
//! obscene-probe-hardware-prospero-root-bigapp.log
//! obscene-probe-orbistoun-prospero-app-bigapp.log
//! ```
//!
//! **Latest run per cell, overwritten in place.** The history of a cell is the file's history,
//! which is what a version control system is for; accumulating dated captures rebuilds `git
//! log` badly and by hand.
//!
//! # Why category is an axis and not a detail
//!
//! Privilege tier and application category are **independent**, and the one that governs
//! direct memory and display is *category* (D301). The same `root` privilege granted full Big
//! App resources under category 0 and **zero bytes of direct memory** under category 65536,
//! with `sceVideoOutOpen` refusing `0x80290001`.
//!
//! So a name carrying only the privilege would file those two runs in one cell, and every
//! memory and display measurement would read as a disagreement with itself.

use std::collections::{BTreeMap, BTreeSet};

use crate::report::{Report, Status};

/// What ran the probe.
///
/// Open rather than closed: a new emulator is a new column, and refusing to file its capture
/// until this list is edited would mean the capture goes somewhere unnamed instead.
const TARGETS: &[&str] = &["hardware", "orbistoun", "shadps4", "fpps4", "host"];

/// How the artefact was launched.
///
/// `prospero` is a native title, `orbis` a backwards-compatible one, `elf` a raw payload with no
/// title around it at all.
const MODES: &[&str] = &["prospero", "orbis", "elf"];

/// The authority id in the SELF header (D301).
const PRIVILEGES: &[&str] = &["app", "sysmodule", "system", "root"];

/// `applicationCategoryType` in `param.json` (D301).
///
/// `none` is for the shapes that carry no `param.json` - a raw ELF has no category, and
/// recording one would be inventing a fact about a file that does not have the field.
const CATEGORIES: &[&str] = &["bigapp", "systemapp", "miniapp", "none"];

/// The prefix every capture carries, so a stray file in the directory is obvious.
const PREFIX: &str = "obscene-probe-";

/// The shape one capture ran in.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct Cell {
    /// What ran it.
    pub target: String,
    /// How the artefact was launched.
    pub mode: String,
    /// The authority id it ran under.
    pub privilege: String,
    /// The application category it declared.
    pub category: String,
}

impl Cell {
    /// The filename this cell is stored under.
    #[must_use]
    pub fn file_name(&self) -> String {
        format!(
            "{PREFIX}{}-{}-{}-{}.log",
            self.target, self.mode, self.privilege, self.category
        )
    }

    /// How it reads in a report column.
    #[must_use]
    pub fn label(&self) -> String {
        format!(
            "{}/{}/{}/{}",
            self.target, self.mode, self.privilege, self.category
        )
    }

    /// Whether this cell is the authority the others are measured against.
    #[must_use]
    pub fn is_authority(&self) -> bool {
        self.target == "hardware"
    }
}

/// Reads a cell out of a filename, or says exactly what is wrong with it.
///
/// **Strict, and a rejection is reported rather than the file skipped.** A capture whose name
/// does not parse is one nobody is comparing, and a matrix that quietly omitted it would
/// report agreement it had not checked - the failure this whole suite exists to avoid.
///
/// # Errors
///
/// When the prefix, extension, field count or any field's vocabulary does not match.
pub fn parse_name(file: &str) -> Result<Cell, String> {
    let stem = file
        .strip_suffix(".log")
        .ok_or_else(|| format!("{file}: not a `.log`"))?;
    let rest = stem
        .strip_prefix(PREFIX)
        .ok_or_else(|| format!("{file}: does not begin `{PREFIX}`"))?;
    let fields: Vec<&str> = rest.split('-').collect();
    let [target, mode, privilege, category] = fields.as_slice() else {
        return Err(format!(
            "{file}: has {} fields after the prefix, expected 4 (target-mode-privilege-category)",
            fields.len()
        ));
    };
    let mode = match *mode {
        "ps5" => "prospero",
        "ps4" => "orbis",
        other => other,
    };
    let checked = |value: &str, allowed: &[&str], axis: &str| -> Result<String, String> {
        if allowed.contains(&value) {
            Ok(value.to_owned())
        } else {
            Err(format!(
                "{file}: `{value}` is not a {axis} - expected one of {}",
                allowed.join(", ")
            ))
        }
    };
    Ok(Cell {
        target: checked(target, TARGETS, "target")?,
        mode: checked(mode, MODES, "mode")?,
        privilege: checked(privilege, PRIVILEGES, "privilege")?,
        category: checked(category, CATEGORIES, "category")?,
    })
}

/// One check, and what each cell answered for it.
#[derive(Debug, Clone)]
pub struct Row {
    /// The check's stable identifier.
    pub check: String,
    /// What each cell said, by cell label.
    pub answers: BTreeMap<String, (Status, String)>,
    /// What the authority said, when a hardware cell took this check.
    pub authority: Option<(Status, String)>,
}

impl Row {
    /// Cells whose verdict differs from the authority's.
    ///
    /// **A skip is not an opinion**, so a cell that skipped the check is not diverging - it
    /// did not answer. Counting a skip as a difference would fill the report with coverage
    /// gaps dressed as defects.
    #[must_use]
    pub fn diverging(&self) -> Vec<&str> {
        let Some((expected, _)) = &self.authority else {
            return Vec::new();
        };
        self.answers
            .iter()
            .filter(|(_, (status, _))| *status != Status::Skip)
            .filter(|(_, (status, _))| status != expected)
            .map(|(label, _)| label.as_str())
            .collect()
    }

    /// Whether the cells disagree among themselves, with no authority to settle it.
    #[must_use]
    pub fn unsettled(&self) -> bool {
        if self.authority.is_some() {
            return false;
        }
        let verdicts: BTreeSet<Status> = self
            .answers
            .values()
            .map(|(status, _)| *status)
            .filter(|status| *status != Status::Skip)
            .collect();
        verdicts.len() > 1
    }
}

/// Every cell that was read, and every check across them.
#[derive(Debug, Default)]
pub struct Matrix {
    /// The cells found, in name order.
    pub cells: Vec<Cell>,
    /// One row per check any cell took.
    pub rows: Vec<Row>,
    /// Files in the directory that are not named as captures, and why each was rejected.
    ///
    /// **Carried in the result rather than logged.** A caller deciding whether the matrix is
    /// trustworthy needs to know what was left out of it.
    pub unnamed: Vec<String>,
}

impl Matrix {
    /// Whether any cell is the authority.
    #[must_use]
    pub fn has_authority(&self) -> bool {
        self.cells.iter().any(Cell::is_authority)
    }

    /// Rows where something disagrees with the console.
    #[must_use]
    pub fn divergences(&self) -> Vec<&Row> {
        self.rows
            .iter()
            .filter(|r| !r.diverging().is_empty())
            .collect()
    }

    /// Rows where the cells disagree and nothing can settle it.
    #[must_use]
    pub fn unsettled(&self) -> Vec<&Row> {
        self.rows.iter().filter(|r| r.unsettled()).collect()
    }

    /// Captures that would complete the matrix, and are not there.
    ///
    /// # Why this is derived rather than a fixed list
    ///
    /// The full cross-product is five targets by three modes by four privileges by four
    /// categories - two hundred and forty cells, almost all of them meaningless. A raw ELF
    /// has no application category; nothing runs a mini app as root.
    ///
    /// So the shapes worth having are **the ones somebody has already produced**: if hardware
    /// was captured as `prospero/root/bigapp`, then an emulator that never ran that shape is a real
    /// gap, and one nobody has run anywhere is not. That makes the list grow from evidence
    /// rather than from a guess about what the platform permits.
    ///
    /// Reported because a matrix that showed only the cells it has looks complete when it is
    /// two runs out of twelve.
    #[must_use]
    pub fn missing(&self) -> Vec<Cell> {
        let shapes: BTreeSet<(&str, &str, &str)> = self
            .cells
            .iter()
            .map(|c| (c.mode.as_str(), c.privilege.as_str(), c.category.as_str()))
            .collect();
        let targets: BTreeSet<&str> = self.cells.iter().map(|c| c.target.as_str()).collect();
        let present: BTreeSet<&Cell> = self.cells.iter().collect();
        let mut gaps = Vec::new();
        for target in &targets {
            for (mode, privilege, category) in &shapes {
                let wanted = Cell {
                    target: (*target).to_owned(),
                    mode: (*mode).to_owned(),
                    privilege: (*privilege).to_owned(),
                    category: (*category).to_owned(),
                };
                if !present.contains(&wanted) {
                    gaps.push(wanted);
                }
            }
        }
        gaps
    }
}

/// Builds the matrix from named captures.
#[must_use]
pub fn compare(captures: &[(Cell, Report)]) -> Matrix {
    let mut rows: BTreeMap<String, Row> = BTreeMap::new();
    for (cell, report) in captures {
        for result in &report.results {
            let row = rows.entry(result.id.clone()).or_insert_with(|| Row {
                check: result.id.clone(),
                answers: BTreeMap::new(),
                authority: None,
            });
            row.answers
                .insert(cell.label(), (result.status, result.value.clone()));
            if cell.is_authority() && result.status != Status::Skip {
                row.authority = Some((result.status, result.value.clone()));
            }
        }
    }
    let mut cells: Vec<Cell> = captures.iter().map(|(cell, _)| cell.clone()).collect();
    cells.sort();
    Matrix {
        cells,
        rows: rows.into_values().collect(),
        unnamed: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::{Row, parse_name};
    use crate::report::Status;
    use std::collections::BTreeMap;

    /// A well-formed name reads back as the cell it describes, and round-trips.
    #[test]
    fn a_name_carries_the_whole_shape() {
        let cell = parse_name("obscene-probe-hardware-prospero-root-bigapp.log").expect("parses");
        assert_eq!(cell.target, "hardware");
        assert_eq!(cell.mode, "prospero");
        assert_eq!(cell.privilege, "root");
        assert_eq!(cell.category, "bigapp");
        assert_eq!(
            cell.file_name(),
            "obscene-probe-hardware-prospero-root-bigapp.log"
        );
    }

    /// **A name missing the category is refused rather than defaulted.**
    ///
    /// Defaulting it would file a Big App run and a system-app run in one cell, and D301
    /// measured those two answering differently about direct memory and display - so the
    /// merged cell would disagree with itself and the report would blame the emulator.
    #[test]
    fn a_name_without_a_category_is_refused() {
        let error = parse_name("obscene-probe-hardware-prospero-root.log").expect_err("refused");
        assert!(error.contains("expected 4"), "{error}");
    }

    /// A field outside its vocabulary names the axis it failed, so the fix is obvious.
    #[test]
    fn an_unknown_field_says_which_axis_it_failed() {
        let error = parse_name("obscene-probe-hardware-prospero-superuser-bigapp.log")
            .expect_err("refused");
        assert!(error.contains("privilege"), "{error}");
        assert!(error.contains("root"), "the allowed set is listed: {error}");
    }

    /// Anything not carrying the prefix is not a capture.
    #[test]
    fn a_stray_file_is_refused() {
        assert!(parse_name("notes.log").is_err());
        assert!(parse_name("obscene-probe-hardware-prospero-root-bigapp.txt").is_err());
    }

    fn row(authority: Option<Status>, answers: &[(&str, Status)]) -> Row {
        Row {
            check: "010-x/y".to_owned(),
            answers: answers
                .iter()
                .map(|(l, s)| ((*l).to_owned(), (*s, String::new())))
                .collect::<BTreeMap<_, _>>(),
            authority: authority.map(|s| (s, String::new())),
        }
    }

    /// A cell answering differently from the console is named.
    #[test]
    fn a_cell_that_disagrees_with_hardware_is_named() {
        let r = row(
            Some(Status::Pass),
            &[
                ("hardware/prospero/app/bigapp", Status::Pass),
                ("orbistoun/prospero/app/bigapp", Status::Fail),
            ],
        );
        assert_eq!(r.diverging(), ["orbistoun/prospero/app/bigapp"]);
    }

    /// **A skip is not a disagreement.**
    ///
    /// A cell that did not take the check has no opinion about it, and reporting one would
    /// turn every coverage gap into a defect.
    #[test]
    fn a_skip_is_not_a_divergence() {
        let r = row(
            Some(Status::Pass),
            &[
                ("hardware/prospero/app/bigapp", Status::Pass),
                ("orbistoun/prospero/app/bigapp", Status::Skip),
            ],
        );
        assert!(r.diverging().is_empty());
    }

    /// Without a console nothing is settled, and the report must not pretend otherwise.
    #[test]
    fn disagreement_with_no_authority_is_unsettled_rather_than_divergent() {
        let r = row(
            None,
            &[
                ("shadps4/prospero/app/bigapp", Status::Pass),
                ("orbistoun/prospero/app/bigapp", Status::Fail),
            ],
        );
        assert!(r.diverging().is_empty(), "nothing to diverge from");
        assert!(r.unsettled(), "but the disagreement is still reported");
    }

    /// Agreement with no authority is not unsettled - there is nothing to settle.
    #[test]
    fn agreement_without_an_authority_is_not_unsettled() {
        let r = row(
            None,
            &[
                ("shadps4/prospero/app/bigapp", Status::Fail),
                ("orbistoun/prospero/app/bigapp", Status::Fail),
            ],
        );
        assert!(!r.unsettled());
    }

    /// **A shape one target has and another lacks is reported as a gap.**
    ///
    /// A matrix showing only what it holds looks complete when it is half empty, and the
    /// missing cell is usually the interesting one - it is the comparison nobody has made.
    #[test]
    fn a_shape_one_target_lacks_is_named_with_the_file_to_produce() {
        let built = super::Matrix {
            cells: vec![
                parse_name("obscene-probe-hardware-prospero-root-bigapp.log").expect("parses"),
                parse_name("obscene-probe-hardware-prospero-app-bigapp.log").expect("parses"),
                parse_name("obscene-probe-orbistoun-prospero-app-bigapp.log").expect("parses"),
            ],
            rows: Vec::new(),
            unnamed: Vec::new(),
        };
        let gaps = built.missing();
        assert_eq!(gaps.len(), 1, "one cell is absent: {gaps:?}");
        let only = gaps.first().expect("just asserted there is one");
        assert_eq!(
            only.file_name(),
            "obscene-probe-orbistoun-prospero-root-bigapp.log"
        );
    }

    /// A shape nobody has run anywhere is not a gap.
    ///
    /// Otherwise the list would be the cross-product, and a report nobody can act on.
    #[test]
    fn a_shape_nobody_has_run_is_not_a_gap() {
        let built = super::Matrix {
            cells: vec![
                parse_name("obscene-probe-hardware-prospero-app-bigapp.log").expect("parses"),
                parse_name("obscene-probe-orbistoun-prospero-app-bigapp.log").expect("parses"),
            ],
            rows: Vec::new(),
            unnamed: Vec::new(),
        };
        assert!(built.missing().is_empty(), "{:?}", built.missing());
    }

    /// The authority is hardware and only hardware.
    #[test]
    fn only_hardware_is_the_authority() {
        let hw = parse_name("obscene-probe-hardware-elf-root-none.log").expect("parses");
        let em = parse_name("obscene-probe-orbistoun-elf-root-none.log").expect("parses");
        assert!(hw.is_authority());
        assert!(!em.is_authority());
    }

    /// Legacy mode names in filenames are normalized to canonical codenames.
    #[test]
    fn older_ps5_and_ps4_mode_names_are_normalized() {
        let legacy_ps5 = parse_name("obscene-probe-hardware-ps5-root-bigapp.log").expect("parses");
        assert_eq!(legacy_ps5.mode, "prospero");
        let legacy_ps4 = parse_name("obscene-probe-hardware-ps4-app-bigapp.log").expect("parses");
        assert_eq!(legacy_ps4.mode, "orbis");
    }
}
