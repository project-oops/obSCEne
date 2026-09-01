//! Parsing the report stream.
//!
//! One parser, three consumers - `verify`, `diff` and `pretty`. Previously each script
//! picked the fields apart itself, which meant three places to update when the format
//! grew and three chances for one of them to disagree about what a field means.
//!
//! The format is documented in `docs/OUTPUT.md` and is a contract: fixed field order,
//! one record per line, growth only at the end of a line. That last rule is why parsing
//! reads fields positionally and tolerates extra ones - a report from a newer probe
//! must still be readable by an older tool rather than rejected.
//!
//! # Lines that are not records pass through
//!
//! An emulator interleaves its own logging with the guest's output. Anything not
//! starting with the record prefix is kept verbatim so `pretty` can print it in place,
//! and ignored by everything else.

use std::fmt;

/// Prefix every record line carries.
pub const PREFIX: &str = "OBS";
/// Field separator. A detail string never contains one.
pub const SEPARATOR: char = '|';

/// What a check concluded.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Status {
    /// A prerequisite did not hold, so nothing was attempted.
    ///
    /// **Ordered below `Fail` deliberately.** A check that stopped running tells you
    /// less than one that ran and failed, so losing coverage is a regression even
    /// though nothing went red.
    Skip,
    /// Returned an error where success was expected.
    Fail,
    /// Returned, but something was off.
    Partial,
    /// Succeeded and every postcondition held.
    Pass,
}

impl Status {
    /// Parses the wire form.
    #[must_use]
    pub fn parse(text: &str) -> Option<Self> {
        match text {
            "pass" => Some(Self::Pass),
            "partial" => Some(Self::Partial),
            "fail" => Some(Self::Fail),
            "skip" => Some(Self::Skip),
            _ => None,
        }
    }

    /// The wire form.
    #[must_use]
    pub fn name(self) -> &'static str {
        match self {
            Self::Pass => "pass",
            Self::Partial => "partial",
            Self::Fail => "fail",
            Self::Skip => "skip",
        }
    }
}

impl fmt::Display for Status {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

/// One check's outcome.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CheckResult {
    /// Stable identifier, and the key when diffing two runs.
    pub id: String,
    /// What it concluded.
    pub status: Status,
    /// Observed value, as written. Empty when the check reported none.
    pub value: String,
    /// Free text, or empty.
    pub detail: String,
    /// Where the expectation came from: `spec`, `documented`, `hardware`, `assumed`.
    ///
    /// Empty on a report that predates the field, which is different from `assumed` and
    /// must stay so: one says nobody recorded it, the other says somebody did and the
    /// answer was "we believe this". A FAIL against a standard is the platform's
    /// problem; a FAIL against an assumption might be the suite's.
    pub from: String,
}

/// One function, and whether it reads its arguments at all.
///
/// Separate from a censused symbol because they answer different questions: a symbol can
/// be present and do nothing. See `007-responsive` for how the two are told apart.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Responsive {
    /// The library it belongs to.
    pub library: String,
    /// The function.
    pub symbol: String,
    /// `responds`, `silent`, or `absent`.
    pub verdict: String,
    /// What it answered. For a silent function this is the constant it always returns,
    /// which is worth seeing - always-zero and always-the-same-wrong-number are
    /// different kinds of unfinished.
    pub observed: String,
}

/// One censused symbol.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Symbol {
    /// The library it belongs to.
    pub library: String,
    /// The symbol name.
    pub name: String,
    /// Whether the loader resolved it.
    pub present: bool,
    /// Which console generation it belongs to, as written.
    pub availability: String,
}

/// A section header.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Section {
    /// Ordering identifier, e.g. `020-memory`.
    pub id: String,
    /// Human-readable name.
    pub title: String,
    /// What the section establishes.
    pub purpose: String,
}

/// A parsed report.
#[derive(Debug, Default)]
pub struct Report {
    /// Format version, section count, check count, as declared.
    pub meta: Option<(u32, u32, u32)>,
    /// Identifier of the probe build that produced this.
    pub build: Option<String>,
    /// Which shape the binary was: `module`, `payload`, or `host`.
    ///
    /// Two targets run the same checks through different loaders, so comparing across
    /// them measures the loader rather than the change. `diff` refuses it for that
    /// reason.
    pub target: Option<String>,
    /// Sections, in the order they ran.
    pub sections: Vec<Section>,
    /// Results, in order.
    pub results: Vec<CheckResult>,
    /// Censused symbols, in order.
    pub symbols: Vec<Symbol>,
    /// Which functions read their arguments, when the report says.
    pub responsive: Vec<Responsive>,
    /// Per-section tallies, paired with the section identifier.
    pub section_tallies: Vec<(String, Tally)>,
    /// The final tally, as declared.
    pub tally: Option<Tally>,
    /// Which output channel the run used, when the report says.
    ///
    /// A run that fell back from the ordinary write is telling you something about the
    /// platform before any check has run. `None` means the report predates this field,
    /// which is different from a run that had no channel at all.
    pub channel: Option<String>,
    /// Whether the stream terminated properly.
    pub ended: bool,
    /// A check announced with no result following.
    ///
    /// The signature of a call that did not return. This is what
    /// announce-before-attempting exists to make visible, so it is a first-class field
    /// rather than something each consumer re-derives.
    pub unresolved_attempt: Option<String>,
    /// Lines that were not records, kept in order with their line numbers.
    pub passthrough: Vec<(usize, String)>,
}

/// Counts of each outcome.
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub struct Tally {
    /// Checks that passed.
    pub pass: u32,
    /// Checks that partly worked.
    pub partial: u32,
    /// Checks that failed.
    pub fail: u32,
    /// Checks that were skipped.
    pub skip: u32,
}

impl Tally {
    /// Adds one outcome.
    pub fn add(&mut self, status: Status) {
        match status {
            Status::Pass => self.pass = self.pass.saturating_add(1),
            Status::Partial => self.partial = self.partial.saturating_add(1),
            Status::Fail => self.fail = self.fail.saturating_add(1),
            Status::Skip => self.skip = self.skip.saturating_add(1),
        }
    }

    /// Every outcome counted.
    #[must_use]
    pub fn total(self) -> u32 {
        self.pass
            .saturating_add(self.partial)
            .saturating_add(self.fail)
            .saturating_add(self.skip)
    }
}

impl Report {
    /// Parses a whole report.
    ///
    /// Never fails: a malformed report is still parsed as far as it goes, because
    /// deciding *whether* it is well-formed is `verify`'s job and it needs the partial
    /// result to say what is wrong with it.
    #[must_use]
    pub fn parse(text: &str) -> Self {
        let mut report = Self::default();
        let mut announced: Option<String> = None;
        let mut current_section: Option<String> = None;
        let mut running = Tally::default();

        for (number, line) in text.lines().enumerate() {
            let Some(fields) = record_fields(line) else {
                report
                    .passthrough
                    .push((number.saturating_add(1), line.to_owned()));
                continue;
            };
            let Some((kind, rest)) = fields.split_first() else {
                continue;
            };

            match *kind {
                "meta" => {
                    report.meta = Some((
                        number_at(rest, 0).unwrap_or(0),
                        number_at(rest, 1).unwrap_or(0),
                        number_at(rest, 2).unwrap_or(0),
                    ));
                }
                "build" => {
                    report.build = field(rest, 0).map(str::to_owned);
                    // Appended after the build id, so a report from before this field
                    // existed still parses - growth at the end of a line is what the
                    // format contract allows.
                    report.target = field(rest, 1).map(str::to_owned);
                }
                "section" => {
                    if let Some(id) = current_section.take() {
                        report.section_tallies.push((id, running));
                    }
                    running = Tally::default();
                    let id = field(rest, 0).unwrap_or_default().to_owned();
                    current_section = Some(id.clone());
                    report.sections.push(Section {
                        id,
                        title: field(rest, 1).unwrap_or_default().to_owned(),
                        purpose: field(rest, 2).unwrap_or_default().to_owned(),
                    });
                }
                "try" => announced = field(rest, 0).map(str::to_owned),
                "res" => {
                    announced = None;
                    let status = field(rest, 1).and_then(Status::parse);
                    if let (Some(id), Some(status)) = (field(rest, 0), status) {
                        running.add(status);
                        report.results.push(CheckResult {
                            id: id.to_owned(),
                            status,
                            value: field(rest, 2).unwrap_or_default().to_owned(),
                            detail: field(rest, 3).unwrap_or_default().to_owned(),
                            from: field(rest, 4).unwrap_or_default().to_owned(),
                        });
                    }
                }
                "responsive" => {
                    if let (Some(library), Some(symbol), Some(verdict)) =
                        (field(rest, 0), field(rest, 1), field(rest, 2))
                    {
                        report.responsive.push(Responsive {
                            library: library.to_owned(),
                            symbol: symbol.to_owned(),
                            verdict: verdict.to_owned(),
                            observed: field(rest, 3).unwrap_or_default().to_owned(),
                        });
                    }
                }
                "sym" => {
                    if let (Some(library), Some(name), Some(presence)) =
                        (field(rest, 0), field(rest, 1), field(rest, 2))
                    {
                        report.symbols.push(Symbol {
                            library: library.to_owned(),
                            name: name.to_owned(),
                            present: presence == "present",
                            availability: field(rest, 3).unwrap_or_default().to_owned(),
                        });
                    }
                }
                "sectiontally" => {
                    // The declared figures, kept as declared. `verify` compares them
                    // against what was recorded, so parsing must not quietly correct
                    // them.
                    if let Some(id) = field(rest, 0) {
                        current_section = None;
                        report
                            .section_tallies
                            .push((id.to_owned(), tally_from(rest, 1)));
                        running = Tally::default();
                    }
                }
                "tally" => report.tally = Some(tally_from(rest, 0)),
                "end" => {
                    report.ended = true;
                    // Appended after the fact, so an older report has no field here.
                    // Absent means "this predates the channel being reported", not
                    // "no channel" - those are different and only one is a problem.
                    report.channel = field(rest, 0).map(str::to_owned);
                }
                _ => {}
            }
        }

        report.unresolved_attempt = announced;
        report
    }

    /// Results keyed by identifier, for comparison.
    #[must_use]
    pub fn by_id(&self) -> std::collections::BTreeMap<&str, &CheckResult> {
        self.results
            .iter()
            .map(|result| (result.id.as_str(), result))
            .collect()
    }

    /// Symbols keyed by library and name.
    #[must_use]
    pub fn symbols_by_key(&self) -> std::collections::BTreeMap<(&str, &str), &Symbol> {
        self.symbols
            .iter()
            .map(|symbol| ((symbol.library.as_str(), symbol.name.as_str()), symbol))
            .collect()
    }

    /// What the records actually add up to, as against what the report declares.
    #[must_use]
    pub fn counted(&self) -> Tally {
        let mut tally = Tally::default();
        for result in &self.results {
            tally.add(result.status);
        }
        tally
    }
}

/// Splits a record line into its fields, or `None` if it is not a record.
fn record_fields(line: &str) -> Option<Vec<&str>> {
    let rest = line.strip_prefix(PREFIX)?.strip_prefix(SEPARATOR)?;
    Some(rest.split(SEPARATOR).collect())
}

fn field<'a>(fields: &[&'a str], index: usize) -> Option<&'a str> {
    fields.get(index).copied()
}

fn number_at(fields: &[&str], index: usize) -> Option<u32> {
    field(fields, index)?.trim().parse().ok()
}

fn tally_from(fields: &[&str], from: usize) -> Tally {
    Tally {
        pass: number_at(fields, from).unwrap_or(0),
        partial: number_at(fields, from.saturating_add(1)).unwrap_or(0),
        fail: number_at(fields, from.saturating_add(2)).unwrap_or(0),
        skip: number_at(fields, from.saturating_add(3)).unwrap_or(0),
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
    use super::{Report, Status};

    const SAMPLE: &str = "\
OBS|meta|1|2|3
OBS|build|abc123
OBS|section|000-boot|Boot|Establishes the report can be trusted
OBS|try|000-boot/a|libkernel|sceKernelWrite
OBS|res|000-boot/a|pass|0x10|
OBS|res|000-boot/b|skip||a prerequisite was not established
OBS|sym|libkernel|sceKernelPread|absent|shared
OBS|sectiontally|000-boot|1|0|0|1
OBS|tally|1|0|0|1
OBS|end
";

    #[test]
    fn a_well_formed_report_parses_every_record_kind() {
        let report = Report::parse(SAMPLE);
        assert_eq!(report.meta, Some((1, 2, 3)));
        assert_eq!(report.build.as_deref(), Some("abc123"));
        assert_eq!(report.sections.len(), 1);
        assert_eq!(report.results.len(), 2);
        assert_eq!(report.symbols.len(), 1);
        assert!(report.ended);
        assert!(report.unresolved_attempt.is_none());
    }

    #[test]
    fn skip_ranks_below_fail() {
        // Not a stylistic choice: a check that stopped running tells you less than one
        // that ran and failed, so losing coverage has to count as getting worse.
        assert!(Status::Skip < Status::Fail);
        assert!(Status::Fail < Status::Partial);
        assert!(Status::Partial < Status::Pass);
    }

    #[test]
    fn an_announcement_with_no_result_is_surfaced() {
        // The signature of a call that did not return, which is the whole reason the
        // probe announces before attempting.
        let text = "OBS|meta|1|1|1\nOBS|try|020-memory/map|libkernel|sceKernelMapDirectMemory\n";
        let report = Report::parse(text);
        assert_eq!(report.unresolved_attempt.as_deref(), Some("020-memory/map"));
        assert!(!report.ended);
    }

    #[test]
    fn non_record_lines_are_kept_rather_than_dropped() {
        // An emulator interleaves its own logging, and `pretty` prints it in place.
        let text = "[Core.Linker] loading\nOBS|meta|1|0|0\n[Render] frame\n";
        let report = Report::parse(text);
        assert_eq!(report.passthrough.len(), 2);
        assert_eq!(report.passthrough[0].1, "[Core.Linker] loading");
    }

    #[test]
    fn extra_trailing_fields_are_tolerated() {
        // The format allows growth at the end of a line, so an older tool must read a
        // newer probe's output rather than refuse it.
        let text = "OBS|res|x/y|pass|0x1|detail|something-new-later\n";
        let report = Report::parse(text);
        assert_eq!(report.results.len(), 1);
        assert_eq!(report.results[0].status, Status::Pass);
        assert_eq!(report.results[0].detail, "detail");
    }

    #[test]
    fn declared_and_counted_tallies_are_kept_apart() {
        // Parsing must not quietly correct a wrong figure: comparing the two is how
        // `verify` catches a report that does not add up.
        let text = "OBS|res|a|pass||\nOBS|res|b|pass||\nOBS|tally|99|0|0|0\n";
        let report = Report::parse(text);
        assert_eq!(report.tally.expect("declared").pass, 99);
        assert_eq!(report.counted().pass, 2);
    }

    #[test]
    fn a_malformed_status_is_dropped_rather_than_guessed() {
        let report = Report::parse("OBS|res|a|nonsense||\n");
        assert!(report.results.is_empty());
    }
}
