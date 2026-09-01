//! The per-loader results table, built from reports rather than by hand.
//!
//! `docs/EMULATORS.md` says what each loader *is*. This says what each loader *did* - the
//! question anybody arrives with, and the one whose answer changes every time the suite
//! grows or an emulator is updated. A hand-written table of results would be stale within a
//! day (D069).
//!
//! # What it deliberately does not do
//!
//! **It does not rank loaders.** A pass count is not a quality score: a loader that resolves
//! everything to a stub scores well on presence and badly on behaviour, and one that refuses
//! to load at all scores nothing while being the most honest of the group.
//!
//! **It does not merge sections.** "Fails everything in one section" and "fails one check in
//! each of eight" are different platforms, and a single total hides which.

use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

const BEGIN: &str = "<!-- obscene:compat -->";
const END: &str = "<!-- /obscene:compat -->";

/// Everything the table needs from one report.
#[derive(Debug, Default, Clone)]
pub struct Summary {
    /// How many `OBS|` records the report holds.
    pub records: usize,
    /// Whether an `end` record arrived.
    pub ended: bool,
    /// Pass / partial / fail / skip.
    pub tally: Option<[u64; 4]>,
    /// Capabilities established / checks blocked / deepest wholly-green section.
    pub frontier: Option<[u64; 3]>,
    /// Per-section tallies.
    pub sections: BTreeMap<String, [u64; 4]>,
    /// Checks this project removed from the build so the loader could reach the rest.
    ///
    /// Counted separately from the `skip` column, and the reason is that the two are not
    /// comparable. A check skipped because a prerequisite failed is a *result*: the platform
    /// could not get there. A check excluded at build time never ran because it takes the
    /// process down, so its verdict is unknown - and it is unknown on exactly the checks that
    /// loader is worst at, which is the direction that flatters.
    ///
    /// fpPS4 reports 447 pass and 5 fail, and 44 of its 55 skips are these. shadPS4 reports
    /// 39 fail with 2. Reading the two `fail` counts side by side without this row invites
    /// precisely the wrong conclusion.
    pub excluded: usize,
    /// Checks skipped because a *previous run of the same build* died inside them.
    ///
    /// The same fact as `excluded`, discovered at run time instead of baked in - so a loader
    /// needing no build-time exclusions is not thereby running every check, and a lone `0` on
    /// the row above says exactly the wrong thing. shadPS4 converges over four runs of one
    /// binary: 142 records, then 316, then 349, then the whole suite with four skipped here.
    ///
    /// Reported separately from `excluded` because the provenance differs. A build-time
    /// exclusion is this project's judgement; a runtime one is a dangling `try` in a report,
    /// which is evidence.
    pub resumed: usize,
    /// Census groups a previous run's death removed from this one.
    ///
    /// The cause of a silently short census, counted rather than inferred. A first attempt
    /// compared each loader's census total against the largest in the table, which fired on a
    /// one-symbol difference between the host and the emulators and said nothing about why.
    /// This counts the thing that actually happens: a run dies inside a corpus group, the
    /// resume mechanism skips that group for good, and every run afterwards reports a complete
    /// suite with an unchanged tally and thousands of measurements missing.
    ///
    /// Measured on shadPS4: one crash, then `complete` at 19,111 symbols where a clean state
    /// gives 35,330. Two groups - `libSceGnmDriver` and `libSceNKWebKit`. (D181)
    pub census_skipped: usize,
    /// Censused symbols reported present.
    pub present: usize,
    /// Censused symbols reported absent.
    pub absent: usize,
    /// Probed functions that answered.
    pub responding: usize,
    /// Probed functions that returned the same thing to everything.
    pub silent: usize,
    /// What carried the report off the target.
    pub channel: Option<String>,
    /// The check that announced and never answered.
    pub unresolved_attempt: Option<String>,
    /// Whether `900-surface/control` passed.
    ///
    /// The census probes one symbol that must resolve and one that must not. A loader that
    /// stub-resolves everything answers "present" to the one that cannot exist, and the
    /// control fails - at which point **every census count from that run is meaningless**,
    /// in the control's own words. The table has to say so, or it invites a reader to
    /// compare a number against one that was actually measured.
    pub census_trustworthy: bool,
}

/// Parse a numeric field, which the report writes in hex or decimal.
fn number(text: &str) -> Option<u64> {
    let text = text.trim();
    text.strip_prefix("0x").map_or_else(
        || text.parse::<u64>().ok(),
        |hex| u64::from_str_radix(hex, 16).ok(),
    )
}

/// Read one report.
pub fn parse(path: &Path) -> std::io::Result<Summary> {
    let text = std::fs::read_to_string(path)?;
    // Trusted until the control says otherwise. A report with no control record at all - a
    // run that died before the census - has not been contradicted, and marking it void would
    // be a claim the run never made.
    let mut out = Summary {
        census_trustworthy: true,
        ..Summary::default()
    };
    // Insertion order matters: the *first* unanswered attempt is the one that died, and a
    // set would lose which that was.
    let mut tried: Vec<String> = Vec::new();

    for line in text.lines() {
        // The report is written unbuffered and a loader may interleave NUL bytes into its
        // log; those are stripped by the runner, but a stray one should not end parsing.
        let line = line.trim_end_matches('\0');
        if !line.starts_with("OBS|") {
            continue;
        }
        out.records = out.records.saturating_add(1);
        let f: Vec<&str> = line.split('|').collect();
        let Some(kind) = f.get(1) else { continue };
        match *kind {
            "tally" => {
                if let (Some(a), Some(b), Some(c), Some(d)) = (
                    f.get(2).and_then(|s| number(s)),
                    f.get(3).and_then(|s| number(s)),
                    f.get(4).and_then(|s| number(s)),
                    f.get(5).and_then(|s| number(s)),
                ) {
                    out.tally = Some([a, b, c, d]);
                }
            }
            "frontier" => {
                if let (Some(a), Some(b), Some(c)) = (
                    f.get(2).and_then(|s| number(s)),
                    f.get(3).and_then(|s| number(s)),
                    f.get(4).and_then(|s| number(s)),
                ) {
                    out.frontier = Some([a, b, c]);
                }
            }
            "sectiontally" => {
                if let (Some(id), Some(a), Some(b), Some(c), Some(d)) = (
                    f.get(2),
                    f.get(3).and_then(|s| number(s)),
                    f.get(4).and_then(|s| number(s)),
                    f.get(5).and_then(|s| number(s)),
                    f.get(6).and_then(|s| number(s)),
                ) {
                    out.sections.insert((*id).to_owned(), [a, b, c, d]);
                }
            }
            "sym" => {
                if let Some(state) = f.get(4) {
                    if *state == "present" {
                        out.present = out.present.saturating_add(1);
                    } else {
                        out.absent = out.absent.saturating_add(1);
                    }
                }
            }
            "responsive" => match f.get(4) {
                Some(&"silent") => out.silent = out.silent.saturating_add(1),
                Some(&"responds") => out.responding = out.responding.saturating_add(1),
                _ => {}
            },
            "res" if f.get(2) == Some(&"900-surface/control") => {
                out.census_trustworthy = f.get(3) == Some(&"pass");
            }
            "end" => {
                out.ended = true;
                out.channel = f.get(2).map(|s| (*s).to_owned());
            }
            "try" => {
                if let Some(id) = f.get(2) {
                    tried.push((*id).to_owned());
                }
            }
            "res" => {
                if let Some(id) = f.get(2) {
                    tried.retain(|t| t != id);
                }
                // Checks this project removed from the build so the loader could reach the
                // rest. The harness words it exactly, in `harness.c`, and matching that text
                // is cheaper than a second record that would be a second thing to keep true.
                // Field 5, not 4: `res` is `id|status|value|detail|provenance`, and the
                // reason text is the *detail*. Indexed off by one first time, which the
                // table reported as a flat zero for every loader - a wrong number that
                // looks like a clean result, which is the shape this project keeps meeting.
                if f.get(3).is_some_and(|s| *s == "skip")
                    && f.get(5)
                        .is_some_and(|s| s.contains("excluded at build time"))
                {
                    out.excluded = out.excluded.saturating_add(1);
                }
                // The runtime half, worded in `sink.c`. Matched on the same principle as
                // above: the text the harness already writes, rather than a second record.
                if f.get(3).is_some_and(|s| *s == "skip")
                    && f.get(5)
                        .is_some_and(|s| s.contains("did not return on the previous run"))
                {
                    out.resumed = out.resumed.saturating_add(1);
                    if f.get(2).is_some_and(|s| s.starts_with("900-surface/")) {
                        out.census_skipped = out.census_skipped.saturating_add(1);
                    }
                }
            }
            _ => {}
        }
    }
    // The announce-before-attempting signature: a `try` with no result names the call that
    // did not return, and it belongs in a compatibility table more than any tally does.
    out.unresolved_attempt = tried.first().cloned();
    Ok(out)
}

fn tally_cell(t: Option<[u64; 4]>) -> String {
    t.map_or_else(
        || "-".to_owned(),
        |t| {
            t.iter()
                .map(ToString::to_string)
                .collect::<Vec<_>>()
                .join("/")
        },
    )
}

/// One frontier column, by position: capabilities, blocked, deepest green.
fn frontier_cell(frontier: Option<[u64; 3]>, at: usize, emphasise: bool) -> String {
    frontier.and_then(|f| f.get(at).copied()).map_or_else(
        || "-".to_owned(),
        |v| {
            if emphasise {
                format!("**{v}**")
            } else {
                v.to_string()
            }
        },
    )
}

/// The header and the per-loader summary rows.
///
/// Split from [`render`] because the two are different jobs and together they tripped the
/// function-length lint - which is the lint doing something useful rather than being
/// appeased: the summary rows and the disagreement table are read separately too.
fn summary_rows(named: &[(String, Summary)]) -> Vec<String> {
    let header: Vec<&str> = named.iter().map(|(n, _)| n.as_str()).collect();
    let mut rows = vec![
        format!("| | {} |", header.join(" | ")),
        format!("|---|{}", "---|".repeat(named.len())),
    ];
    let mut row = |label: &str, cells: Vec<String>| {
        rows.push(format!("| {label} | {} |", cells.join(" | ")));
    };
    let each = |f: &dyn Fn(&Summary) -> String| -> Vec<String> {
        named.iter().map(|(_, r)| f(r)).collect()
    };

    row("Records", each(&|r| r.records.to_string()));
    row(
        "Ran to the end",
        each(&|r| if r.ended { "yes" } else { "**no**" }.to_owned()),
    );
    row(
        "Output channel",
        each(&|r| r.channel.clone().unwrap_or_else(|| "-".to_owned())),
    );
    row(
        "Pass / partial / fail / skip",
        each(&|r| tally_cell(r.tally)),
    );
    row(
        "Excluded to keep the loader alive",
        each(&|r| {
            if r.excluded == 0 {
                "0".to_owned()
            } else {
                // Bolded because it qualifies the row above it, and a reader comparing two
                // `fail` counts needs to see this before drawing the obvious conclusion.
                format!("**{}**", r.excluded)
            }
        }),
    );
    row(
        "Skipped after dying on a previous run",
        each(&|r| {
            if r.resumed == 0 {
                "0".to_owned()
            } else {
                // Bolded for the same reason as the row above: it qualifies the tally.
                format!("**{}**", r.resumed)
            }
        }),
    );
    row(
        "Capabilities established",
        each(&|r| frontier_cell(r.frontier, 0, false)),
    );
    row(
        "Checks blocked behind a missing one",
        each(&|r| frontier_cell(r.frontier, 1, true)),
    );
    row(
        "Deepest wholly-green section",
        each(&|r| frontier_cell(r.frontier, 2, false)),
    );
    row(
        "Census present / absent",
        each(&|r| {
            let cell = if r.census_trustworthy || (r.present == 0 && r.absent == 0) {
                format!("{} / {}", r.present, r.absent)
            } else {
                // Not hidden, qualified. The numbers are what the run produced; the marker
                // is what obSCEne's own control says they are worth.
                format!("{} / {} **(void)**", r.present, r.absent)
            };
            // A run that measured materially less of the census than the best one did, while
            // reporting a complete suite, is the failure mode of D181 rendered as a cell.
            //
            // shadPS4 crashed once inside the census, which put two corpus libraries on its
            // skip list; every run afterwards reported `complete`, with a healthy tally and an
            // unchanged check count, and **sixteen thousand fewer measurements**. Nothing in
            // the summary moved except this total, which nobody compares unless something
            // tells them to. This is that something.
            //
            // Named, not inferred from the size. Every loader probes the same census, so a
            // short total does mean missing measurement - but the total is also the wrong
            // thing to compare, because it differs by one between the host and the emulators
            // for reasons that have nothing to do with this. The groups skipped are countable
            // exactly, and they are the whole cause.
            if r.census_skipped != 0 {
                format!("{cell} **({} groups skipped)**", r.census_skipped)
            } else {
                cell
            }
        }),
    );
    row(
        "Probed functions responding / silent",
        each(&|r| format!("{} / {}", r.responding, r.silent)),
    );
    row(
        "Died in",
        each(&|r| {
            r.unresolved_attempt
                .as_ref()
                .map_or_else(|| "-".to_owned(), |a| format!("`{a}`"))
        }),
    );
    rows
}

/// The table, as markdown.
#[must_use]
pub fn render(named: &[(String, Summary)]) -> String {
    let header: Vec<&str> = named.iter().map(|(n, _)| n.as_str()).collect();
    let mut rows = summary_rows(named);

    // Sections any report disagrees about. Agreement is the uninteresting case and there is
    // a lot of it - the same reasoning `consensus` uses.
    //
    // Only among loaders that actually reported the section. A loader that produced no
    // report has said nothing, and counting its absence as disagreement made every section
    // look disputed. Absence is not an opinion (D072).
    let every: BTreeSet<&String> = named.iter().flat_map(|(_, r)| r.sections.keys()).collect();
    let mut differing = Vec::new();
    for section in every {
        let seen: BTreeSet<[u64; 4]> = named
            .iter()
            .filter_map(|(_, r)| r.sections.get(section).copied())
            .collect();
        if seen.len() > 1 {
            differing.push(section.clone());
        }
    }
    if !differing.is_empty() {
        rows.push(String::new());
        rows.push("Sections the loaders do not agree on, pass/partial/fail/skip:".to_owned());
        rows.push(String::new());
        rows.push(format!("| Section | {} |", header.join(" | ")));
        rows.push(format!("|---|{}", "---|".repeat(named.len())));
        for section in differing {
            let cells: Vec<String> = named
                .iter()
                .map(|(_, r)| tally_cell(r.sections.get(&section).copied()))
                .collect();
            rows.push(format!("| `{section}` | {} |", cells.join(" | ")));
        }
    }
    rows.join("\n")
}

/// Write or check the marked region. Returns true when it is current.
pub fn run(
    into: &Path,
    named: &[(String, Summary)],
    write: bool,
    check: bool,
) -> std::io::Result<bool> {
    let table = render(named);
    let text = std::fs::read_to_string(into)?;
    let (Some(before), Some((_, after))) = (text.split(BEGIN).next(), text.split_once(END)) else {
        println!("no {BEGIN} region in {}", into.display());
        return Ok(false);
    };
    if !text.contains(BEGIN) {
        println!("no {BEGIN} region in {}", into.display());
        return Ok(false);
    }
    let updated = format!("{before}{BEGIN}\n{table}\n{END}{after}");
    if updated == text {
        if check {
            println!("compatibility table current");
        }
        return Ok(true);
    }
    if write {
        std::fs::write(into, updated)?;
        println!("written: {}", into.display());
        return Ok(true);
    }
    if check {
        println!("the compatibility table has drifted; regenerate it");
    } else {
        // Neither flag: this run was asked to do nothing, and it used to say so by returning
        // a bare non-zero and printing not one word. That is the failure this project keeps
        // meeting from the other side - a silent result read as a clean one - and it cost a
        // stale table being reported as current. Naming the missing flag costs a line.
        println!(
            "the compatibility table has drifted, and neither --write nor --check was given; \
             pass --write to rewrite {} or --check to gate on it",
            into.display()
        );
    }
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn numbers_parse_in_both_notations() {
        assert_eq!(number("0x10"), Some(16));
        assert_eq!(number("16"), Some(16));
        assert_eq!(number("nonsense"), None);
    }

    /// A `try` with no `res` names the call that did not return. The *first* one is the
    /// answer; a set would lose the order and report an arbitrary member.
    #[test]
    fn the_first_unanswered_attempt_is_the_one_that_died() {
        let dir = std::env::temp_dir().join("obscene-compat-died");
        std::fs::create_dir_all(&dir).expect("temp dir");
        let path = dir.join("r.txt");
        std::fs::write(
            &path,
            "OBS|try|a/one|lib|s\nOBS|res|a/one|pass||\nOBS|try|b/two|lib|s\nOBS|try|c/three|lib|s\n",
        )
        .expect("write");
        let s = parse(&path).expect("parse");
        assert_eq!(s.unresolved_attempt.as_deref(), Some("b/two"));
        std::fs::remove_dir_all(&dir).ok();
    }

    /// Absence is not an opinion: a loader that never reported a section must not make it
    /// look disputed.
    #[test]
    fn a_section_only_one_loader_ran_is_not_a_disagreement() {
        let mut a = Summary::default();
        a.sections.insert("000-boot".to_owned(), [4, 0, 0, 0]);
        a.sections.insert("010-kernel".to_owned(), [3, 0, 0, 0]);
        let mut b = Summary::default();
        b.sections.insert("000-boot".to_owned(), [4, 0, 0, 0]);
        let table = render(&[("a".to_owned(), a), ("b".to_owned(), b)]);
        assert!(
            !table.contains("do not agree"),
            "one loader not running a section is silence, not disagreement:\n{table}"
        );
    }

    #[test]
    fn a_real_disagreement_is_listed() {
        let mut a = Summary::default();
        a.sections.insert("035-libc".to_owned(), [30, 0, 0, 0]);
        let mut b = Summary::default();
        b.sections.insert("035-libc".to_owned(), [6, 2, 0, 22]);
        let table = render(&[("a".to_owned(), a), ("b".to_owned(), b)]);
        assert!(table.contains("do not agree"));
        assert!(table.contains("`035-libc`"));
        assert!(table.contains("6/2/0/22"));
    }
}
