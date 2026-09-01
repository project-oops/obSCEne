//! Rendering a report for a person.
//!
//! Colour and grouping are presentation, and presentation does not belong in a
//! freestanding binary that has to survive running inside a half-finished emulator. The
//! probe emits one machine-readable format; this turns it into the red, amber and green
//! view.
//!
//! # The stream ending inside a call is the interesting case
//!
//! The probe announces each check before making it, so a stream that stops between an
//! announcement and its result means that call did not return. That is reported
//! explicitly rather than shown as a report that merely happens to be short - it is the
//! whole reason the announcement exists.

use std::fmt::Write as _;

use crate::report::{Report, Status, Tally};

/// ANSI colour, or nothing when writing somewhere that is not a terminal.
struct Palette {
    enabled: bool,
}

impl Palette {
    const RESET: &'static str = "\x1b[0m";
    const BOLD: &'static str = "\x1b[1m";
    const DIM: &'static str = "\x1b[2m";
    const MARK: &'static str = "\x1b[35m";
    /// Default foreground: ends a colour run without ending a bold one.
    const DEFAULT_FG: &'static str = "\x1b[39m";

    fn colour(status: Status) -> &'static str {
        match status {
            // Amber rather than plain yellow: on a light terminal, yellow on white is
            // close to unreadable, and this is meant to be read at a glance.
            Status::Partial => "\x1b[33m",
            Status::Pass => "\x1b[32m",
            Status::Fail => "\x1b[31m",
            Status::Skip => "\x1b[90m",
        }
    }

    fn paint(&self, status: Status, text: &str) -> String {
        if self.enabled {
            format!("{}{text}{}", Self::colour(status), Self::RESET)
        } else {
            text.to_owned()
        }
    }

    fn emphasise(&self, prefix: &'static str, text: &str) -> String {
        if self.enabled {
            format!("{prefix}{text}{}", Self::RESET)
        } else {
            text.to_owned()
        }
    }

    /// The name, with the platform's three letters marked.
    ///
    /// Magenta rather than one of the status colours, and deliberately not a status: it
    /// grades nothing, it just says which part of the name is the vendor's. The screen
    /// draws the same split in `OBS_COLOUR_MARK`.
    ///
    /// Ends the run with `39` - default foreground - rather than `RESET`. The banner is
    /// already inside a bold run, and `RESET` would clear the bold along with the colour,
    /// so the rest of the line would quietly come out unemphasised.
    fn wordmark(&self) -> String {
        if self.enabled {
            format!("ob{}SCE{}ne", Self::MARK, Self::DEFAULT_FG)
        } else {
            "obSCEne".to_owned()
        }
    }
}

/// The four-character marker for a status.
fn marker(status: Status) -> &'static str {
    match status {
        Status::Pass => "OK  ",
        Status::Partial => "WARN",
        Status::Fail => "FAIL",
        Status::Skip => "--  ",
    }
}

/// Renders a whole report.
///
/// `colour` should be false when writing anywhere but a terminal - piping this into a
/// file or an agent should produce plain text, not escape sequences someone then has to
/// strip.
#[must_use]
pub fn render(report: &Report, colour: bool) -> String {
    let palette = Palette { enabled: colour };
    let mut out = String::new();

    if let Some((version, sections, checks)) = report.meta {
        let build = report.build.as_deref().unwrap_or("unknown");
        out.push_str(&palette.emphasise(
            Palette::BOLD,
            &format!(
                "{} report  format {version}  build {build}  \
                 {sections} sections  {checks} checks",
                palette.wordmark()
            ),
        ));
        out.push('\n');
    }

    // Results are grouped under the section that was running when they were recorded.
    // Walking the records in order preserves that without the report having to repeat
    // the section on every line.
    let mut index = 0_usize;
    for section in &report.sections {
        render_section(&mut out, report, section, &palette, &mut index);
    }

    render_responsive(&mut out, report, &palette);
    render_provenance(&mut out, report, &palette);
    render_totals(&mut out, report, &palette);
    out
}

/// One section: its heading, its results, and what they add up to.
fn render_section(
    out: &mut String,
    report: &Report,
    section: &crate::report::Section,
    palette: &Palette,
    index: &mut usize,
) {
    out.push('\n');
    out.push_str(&palette.emphasise(Palette::BOLD, &format!("{}  {}", section.id, section.title)));
    out.push('\n');
    out.push_str(&palette.emphasise(Palette::DIM, &format!("  {}", section.purpose)));
    out.push('\n');

    // Results carry their section as an identifier prefix, so walking forward while the
    // prefix matches groups them without the report repeating the section per line.
    let prefix = format!("{}/", section.id);
    let mut tally = Tally::default();
    while let Some(result) = report.results.get(*index) {
        if !result.id.starts_with(&prefix) {
            break;
        }
        tally.add(result.status);
        let mut line = format!(
            "  {}  {}",
            palette.paint(result.status, marker(result.status)),
            result.id
        );
        if !result.value.is_empty() {
            let _ = write!(line, "  {}", result.value);
        }
        if !result.detail.is_empty() {
            let _ = write!(line, "  {}", result.detail);
        }
        // Shown on failures only, and only when the expectation is not settled by a
        // standard. A FAIL against ISO C is the platform's problem and needs no
        // annotation; a FAIL against something this suite merely believes is a claim
        // the reader should be able to weigh, and silence would present the two as
        // equally solid.
        if matches!(result.status, Status::Fail | Status::Partial)
            && !result.from.is_empty()
            && result.from != "spec"
            && result.from != "hardware"
        {
            let _ = write!(
                line,
                "{}",
                palette.emphasise(Palette::DIM, &format!("  [{}]", result.from))
            );
        }
        out.push_str(&line);
        out.push('\n');
        *index = index.saturating_add(1);
    }

    out.push_str(&palette.emphasise(
        Palette::DIM,
        &format!(
            "    {} pass, {} partial, {} fail, {} skip",
            tally.pass, tally.partial, tally.fail, tally.skip
        ),
    ));
    out.push('\n');
}

/// The final tally, and the crash notice if the stream stopped inside a call.
/// How much of the suite rests on what.
///
/// The headline number a reader needs before trusting any of the rest: a suite that is
/// mostly assumptions is a suite whose red is negotiable. It also makes the value of a
/// hardware run visible, because that is the run that moves counts out of `assumed`.
/// Which functions are implemented and which are stubs.
///
/// Placed before the totals, because it changes how every failure below reads: a
/// function that answers the same thing to everything is unimplemented, not incorrect,
/// and the two need opposite work from whoever is fixing the platform.
///
/// Names the silent ones rather than counting them. "Nineteen stubs" is a statistic;
/// "sqrt, fabs, floor, ceil are stubs" is a list somebody can act on.
fn render_responsive(out: &mut String, report: &Report, palette: &Palette) {
    if report.responsive.is_empty() {
        return;
    }
    let responds: Vec<&str> = report
        .responsive
        .iter()
        .filter(|r| r.verdict == "responds")
        .map(|r| r.symbol.as_str())
        .collect();
    let silent: Vec<&str> = report
        .responsive
        .iter()
        .filter(|r| r.verdict == "silent")
        .map(|r| r.symbol.as_str())
        .collect();
    let absent: Vec<&str> = report
        .responsive
        .iter()
        .filter(|r| r.verdict == "absent")
        .map(|r| r.symbol.as_str())
        .collect();

    out.push('\n');
    out.push_str(&palette.emphasise(
        Palette::BOLD,
        &format!(
            "implemented {} of {}",
            responds.len(),
            report.responsive.len()
        ),
    ));
    out.push('\n');
    if !responds.is_empty() {
        let _ = writeln!(out, "  responds  {}", responds.join(" "));
    }
    if !silent.is_empty() {
        out.push_str(&palette.emphasise(
            Palette::DIM,
            &format!(
                "  stubbed   {}
",
                silent.join(" ")
            ),
        ));
        out.push_str(&palette.emphasise(
            Palette::DIM,
            "  a failure below against one of these is absence, not incorrectness
",
        ));
    }
    if !absent.is_empty() {
        out.push_str(&palette.emphasise(
            Palette::DIM,
            &format!(
                "  absent    {}
",
                absent.join(" ")
            ),
        ));
    }
}

fn render_provenance(out: &mut String, report: &Report, palette: &Palette) {
    let mut counts: std::collections::BTreeMap<&str, usize> = std::collections::BTreeMap::new();
    for result in &report.results {
        if !result.from.is_empty() {
            let slot: &mut usize = counts.entry(result.from.as_str()).or_default();
            *slot = slot.saturating_add(1);
        }
    }
    if counts.is_empty() {
        return;
    }
    let summary = counts
        .iter()
        .map(|(name, n)| format!("{n} {name}"))
        .collect::<Vec<_>>()
        .join(", ");
    out.push('\n');
    out.push_str(&palette.emphasise(Palette::DIM, &format!("expectations: {summary}")));
    out.push('\n');
    if counts.get("hardware").copied().unwrap_or(0) == 0 {
        out.push_str(&palette.emphasise(
            Palette::DIM,
            "  nothing here has been confirmed against real hardware yet",
        ));
        out.push('\n');
    }
}

fn render_totals(out: &mut String, report: &Report, palette: &Palette) {
    if let Some(tally) = report.tally {
        out.push('\n');
        let _ = writeln!(
            out,
            "{}  {}  {}  {}",
            palette.paint(Status::Pass, &format!("{} pass", tally.pass)),
            palette.paint(Status::Partial, &format!("{} partial", tally.partial)),
            palette.paint(Status::Fail, &format!("{} fail", tally.fail)),
            palette.paint(Status::Skip, &format!("{} skip", tally.skip)),
        );
    }

    // The whole reason the probe announces before attempting: a stream that stops here
    // means the call did not return, and that is worth saying rather than showing a
    // report that merely happens to be short.
    if let Some(id) = &report.unresolved_attempt {
        out.push('\n');
        out.push_str(&palette.paint(Status::Fail, &format!("STREAM ENDED INSIDE A CALL: {id}")));
        out.push('\n');
        out.push_str(
            "  The report stopped between the attempt and its result, so this call \
             most likely took the process down.\n",
        );
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
    use super::render;
    use crate::report::Report;

    const SAMPLE: &str = "\
OBS|meta|1|1|2
OBS|build|abc123
OBS|section|010-kernel|Kernel core|Clocks and process identity
OBS|res|010-kernel/a|pass|0x2f8a1|
OBS|res|010-kernel/b|fail||the counter frequency is zero
OBS|tally|1|0|1|0
OBS|end
";

    #[test]
    fn results_appear_under_their_section() {
        let text = render(&Report::parse(SAMPLE), false);
        let section_at = text.find("010-kernel  Kernel core").expect("section");
        let result_at = text.find("010-kernel/a").expect("result");
        assert!(section_at < result_at, "a result must follow its section");
    }

    #[test]
    fn plain_output_carries_no_escape_sequences() {
        // Piping into a file or an agent must produce text, not escapes to strip.
        let text = render(&Report::parse(SAMPLE), false);
        assert!(!text.contains('\x1b'), "colour leaked into plain output");
    }

    #[test]
    fn colour_output_does_carry_them() {
        let text = render(&Report::parse(SAMPLE), true);
        assert!(text.contains('\x1b'));
    }

    #[test]
    fn a_detail_is_shown_next_to_its_check() {
        let text = render(&Report::parse(SAMPLE), false);
        assert!(text.contains("the counter frequency is zero"));
    }

    #[test]
    fn a_stream_ending_inside_a_call_is_called_out() {
        // The whole point of announcing before attempting: a truncated report should
        // name the call that killed it rather than look merely short.
        let text = "OBS|meta|1|0|1\nOBS|build|a\nOBS|try|020-memory/map|libkernel|sceKernelMapDirectMemory\n";
        let rendered = render(&Report::parse(text), false);
        assert!(rendered.contains("STREAM ENDED INSIDE A CALL"));
        assert!(rendered.contains("020-memory/map"));
    }

    #[test]
    fn a_complete_report_does_not_claim_a_crash() {
        let text = render(&Report::parse(SAMPLE), false);
        assert!(!text.contains("STREAM ENDED"));
    }
}
