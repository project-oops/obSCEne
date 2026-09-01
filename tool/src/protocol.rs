//! The captured protocol exchanges, checked against the specification.
//!
//! `docs/PROTOCOL.md` is written first and the transcripts under `docs/examples/protocol/`
//! are the evidence that an implementation matches it. This checks the transcripts.
//!
//! # Two rules do most of the work
//!
//! **Acknowledge first.** A request is acknowledged before it is attempted, so an `ack` with
//! no terminal reply after it names the command that killed the responder - the same
//! announce-before-attempting property the report itself is arranged around.
//!
//! **The grammar is closed.** Verbs, refusal reasons, outcomes and capability tokens are
//! fixed lists. Report enum values are open and may gain members without a version bump; the
//! protocol's are not, and that asymmetry is deliberate - a consumer negotiating a session
//! needs to know that an unfamiliar verb is a fault rather than a newer peer.

use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

/// Verbs, and the capability each requires. `hello` and `bye` need none.
///
/// `run` requires `blob`: it executes something previously uploaded, so the capability that
/// gates the upload gates the execution too.
const VERB_CAPABILITY: [(&str, Option<&str>); 11] = [
    ("hello", None),
    ("bye", None),
    ("resolve", Some("resolve")),
    ("call", Some("call")),
    ("read", Some("read")),
    ("write", Some("write")),
    ("blob", Some("blob")),
    ("run", Some("blob")),
    ("reset", Some("reset")),
    ("report", Some("report")),
    ("gpu", Some("gpu")),
];

const REASONS: [&str; 6] = [
    "unknown-verb",
    "unsupported",
    "bad-argument",
    "busy",
    "not-negotiated",
    "unmapped",
];

const CAPABILITIES: [&str; 8] = [
    "call", "resolve", "read", "write", "blob", "reset", "report", "gpu",
];

const OUTCOMES: [&str; 6] = ["ok", "returned", "died", "timeout", "lost", "absent"];

/// The wire's line limit, in bytes including the newline.
const MAX_LINE: usize = 4096;

fn verbs() -> BTreeSet<&'static str> {
    VERB_CAPABILITY.iter().map(|(v, _)| *v).collect()
}

fn capability_for(verb: &str) -> Option<&'static str> {
    VERB_CAPABILITY
        .iter()
        .find(|(v, _)| *v == verb)
        .and_then(|(_, c)| *c)
}

/// State carried across the lines of one transcript.
#[derive(Default)]
struct Session {
    problems: Vec<String>,
    /// Sequences seen, in order, so a repeat or a regression can be spotted.
    seq_seen: Vec<u64>,
    /// Acknowledged and not yet answered.
    pending: BTreeMap<u64, String>,
    /// Sequences carrying a verb outside the grammar.
    unknown_verbs: BTreeSet<u64>,
    /// Sequences the responder must refuse as `bad-argument`.
    rejected_seqs: BTreeSet<u64>,
    awaiting_rejection: bool,
    /// Capabilities from the current `hello`; `None` before one.
    announced: Option<BTreeSet<String>>,
    /// Verb by sequence, for matching replies back to requests.
    verbs_sent: BTreeMap<u64, String>,
}

impl Session {
    fn fail(&mut self, file: &str, number: usize, message: &str) {
        self.problems.push(format!("{file}:{number}: {message}"));
    }
}

fn parse_seq(text: &str) -> Option<u64> {
    if text.chars().all(|c| c.is_ascii_digit()) && !text.is_empty() {
        text.parse().ok()
    } else {
        None
    }
}

/// A request line.
fn on_request(s: &mut Session, file: &str, number: usize, fields: &[&str]) {
    let (Some(raw_seq), Some(verb)) = (fields.get(1), fields.get(2)) else {
        s.fail(
            file,
            number,
            "a request needs at least a sequence and a verb",
        );
        return;
    };
    // A malformed or repeated sequence is not a fault in the transcript. The specification
    // permits it and requires it to be refused as `bad-argument` **without** an
    // acknowledgement - the one place acknowledge-first yields, and it yields to the rule
    // that makes acknowledgement useful, since an `ack` is keyed by sequence and a repeated
    // key cannot be matched to a request.
    let Some(seq) = parse_seq(raw_seq) else {
        // The responder reports these against sequence 0: it has no number to quote back.
        s.rejected_seqs.insert(0);
        s.awaiting_rejection = true;
        return;
    };
    if s.seq_seen.last().is_some_and(|last| seq <= *last) {
        s.rejected_seqs.insert(seq);
        s.awaiting_rejection = true;
        return;
    }
    s.seq_seen.push(seq);
    // An unknown verb is not failed here. Sending one and having it refused is a scenario
    // the specification requires a transcript for, so the complaint is deferred until the
    // reply is seen - a verb outside the grammar is a fault only if something answered it.
    if !verbs().contains(verb) {
        s.unknown_verbs.insert(seq);
    }
    s.verbs_sent.insert(seq, (*verb).to_owned());
    s.pending.insert(seq, (*verb).to_owned());
}

fn on_ack(s: &mut Session, file: &str, number: usize, fields: &[&str]) {
    let (Some(raw), Some(verb)) = (fields.get(2), fields.get(3)) else {
        s.fail(file, number, "an ack needs a sequence and a verb");
        return;
    };
    let seq = parse_seq(raw);
    match seq.and_then(|q| s.verbs_sent.get(&q).cloned()) {
        None => {
            let shown = seq.map_or(-1, |q| i64::try_from(q).unwrap_or(-1));
            s.fail(
                file,
                number,
                &format!("ack for sequence {shown}, which was never sent"),
            );
        }
        Some(sent) if sent != *verb => {
            s.fail(
                file,
                number,
                &format!("ack names {verb:?}, request was {sent:?}"),
            );
        }
        Some(_) => {}
    }
}

fn on_terminal(s: &mut Session, file: &str, number: usize, kind: &str, fields: &[&str]) {
    let (Some(raw), Some(word)) = (fields.get(2), fields.get(3)) else {
        s.fail(
            file,
            number,
            &format!("a {kind} needs a sequence and an outcome"),
        );
        return;
    };
    let seq = parse_seq(raw).unwrap_or(u64::MAX);

    // The deferred complaint from a malformed or repeated sequence.
    if s.awaiting_rejection && s.rejected_seqs.contains(&seq) {
        s.awaiting_rejection = false;
        if kind != "refused" || *word != "bad-argument" {
            s.fail(
                file,
                number,
                &format!(
                    "a malformed or repeated sequence must be refused as bad-argument, \
                     not {kind} {word:?}"
                ),
            );
        }
        return;
    }
    if !s.verbs_sent.contains_key(&seq) {
        s.fail(
            file,
            number,
            &format!("{kind} for sequence {seq}, which was never sent"),
        );
        return;
    }
    let Some(verb) = s.pending.remove(&seq) else {
        s.fail(
            file,
            number,
            &format!("sequence {seq} already had a terminal reply"),
        );
        return;
    };

    if kind == "refused" {
        if !REASONS.contains(word) {
            s.fail(
                file,
                number,
                &format!("refusal reason {word:?} is not in the specification"),
            );
        }
        if s.unknown_verbs.contains(&seq) && *word != "unknown-verb" {
            s.fail(
                file,
                number,
                &format!(
                    "{verb:?} is outside the grammar and was refused as {word:?}; \
                     it must be unknown-verb"
                ),
            );
        }
        return;
    }

    if s.unknown_verbs.contains(&seq) {
        s.fail(
            file,
            number,
            &format!("{verb:?} is not in the specification and was answered rather than refused"),
        );
    }
    if !OUTCOMES.contains(word) {
        s.fail(
            file,
            number,
            &format!("outcome {word:?} is not in the specification"),
        );
    }
    if let (Some(announced), Some(needed)) = (s.announced.clone(), capability_for(&verb))
        && !announced.contains(needed)
    {
        s.fail(
            file,
            number,
            &format!(
                "{verb:?} needs the {needed:?} capability, which was not announced; \
                 it must be refused"
            ),
        );
    }
}

fn on_hello(s: &mut Session, file: &str, number: usize, fields: &[&str]) {
    let Some(tokens) = fields.get(4) else {
        s.fail(
            file,
            number,
            "a hello reply needs a version, a session and capabilities",
        );
        return;
    };
    let announced: BTreeSet<String> = tokens
        .split(',')
        .filter(|t| !t.is_empty())
        .map(str::to_owned)
        .collect();
    let unknown: Vec<&String> = announced
        .iter()
        .filter(|t| !CAPABILITIES.contains(&t.as_str()))
        .collect();
    if !unknown.is_empty() {
        s.fail(
            file,
            number,
            &format!("capability {unknown:?} is not in the specification"),
        );
    }
    s.announced = Some(announced);
}

/// The hex run must be a whole number of bytes, and actually hex.
///
/// Added because a consumer's decoder caught a defect this checker had passed: a 65-digit
/// run in a read of 0x20, one nibble too many to be bytes at all. A lenient parser would
/// have dropped or invented a nibble - both invisible, both wrong.
fn on_bytes(s: &mut Session, file: &str, number: usize, fields: &[&str]) {
    let hexrun = fields.last().copied().unwrap_or_default();
    if hexrun.len() % 2 != 0 {
        s.fail(
            file,
            number,
            &format!(
                "a bytes record carries {} hex digits, which is not a whole number of bytes",
                hexrun.len()
            ),
        );
    } else if !hexrun.is_empty() && !hexrun.chars().all(|c| c.is_ascii_hexdigit()) {
        s.fail(
            file,
            number,
            "a bytes record carries a non-hex digit in its data",
        );
    }
}

/// Check one transcript. Each problem names the file and line.
pub fn check_transcript(path: &Path) -> std::io::Result<Vec<String>> {
    let file = path
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or_default()
        .to_owned();
    let text = std::fs::read_to_string(path)?;
    let mut s = Session::default();

    for (index, line) in text.split('\n').enumerate() {
        let number = index.saturating_add(1);
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        if line.len().saturating_add(1) > MAX_LINE {
            let bytes = line.len().saturating_add(1);
            s.fail(
                &file,
                number,
                &format!("line is {bytes} bytes, over the {MAX_LINE} limit"),
            );
        }
        let fields: Vec<&str> = line.split('|').collect();
        if line.starts_with("CMD|") {
            on_request(&mut s, &file, number, &fields);
            continue;
        }
        if !line.starts_with("OBS|") {
            s.fail(
                &file,
                number,
                "line is neither a request, a reply, nor an annotation",
            );
            continue;
        }
        match fields.get(1).copied().unwrap_or_default() {
            "ack" => on_ack(&mut s, &file, number, &fields),
            kind @ ("done" | "refused") => on_terminal(&mut s, &file, number, kind, &fields),
            "hello" => on_hello(&mut s, &file, number, &fields),
            "bytes" => on_bytes(&mut s, &file, number, &fields),
            _ => {}
        }
    }

    // Every acknowledged command must have a terminal reply. No exception.
    //
    // The first version allowed a dangling `ack` when the file mentioned `died` or `timeout`
    // anywhere, on the reasoning that a dead probe cannot reply. That was wrong twice over.
    // It is too loose - one transcript refers to another in a *comment*, which satisfied a
    // substring search and let a truncated transcript through. And it is wrong in principle:
    // the specification makes recording a non-answer the driver's obligation, so a died or
    // timed-out command still ends in a terminal record written by the other end.
    let dangling: Vec<(u64, String)> = s.pending.iter().map(|(k, v)| (*k, v.clone())).collect();
    for (seq, verb) in dangling {
        s.fail(
            &file,
            0,
            &format!(
                "{verb:?} at sequence {seq} was acknowledged and never answered; a command \
                 that did not return still needs a terminal record, written by the driver \
                 as died, timeout or lost"
            ),
        );
    }
    Ok(s.problems)
}

/// Every verb and reason this checker knows must appear in the specification.
///
/// Catches the drift that matters most: a term the checker enforces and the document never
/// mentions, which makes the checker the real contract - exactly the inversion writing the
/// specification first was meant to avoid.
pub fn check_spec_lists(spec: &Path) -> std::io::Result<Vec<String>> {
    if !spec.is_file() {
        return Ok(vec![format!("{} is missing", spec.display())]);
    }
    let text = std::fs::read_to_string(spec)?;
    let mut problems = Vec::new();
    for verb in verbs() {
        if !text.contains(&format!("`{verb}`")) {
            problems.push(format!("PROTOCOL.md never mentions the verb `{verb}`"));
        }
    }
    for reason in REASONS {
        if !text.contains(reason) {
            problems.push(format!(
                "PROTOCOL.md never mentions the refusal reason `{reason}`"
            ));
        }
    }
    for capability in CAPABILITIES {
        if !text.contains(&format!("`{capability}`")) {
            problems.push(format!(
                "PROTOCOL.md never mentions the capability `{capability}`"
            ));
        }
    }
    problems.sort();
    Ok(problems)
}

/// Check every transcript in a directory, plus the specification's own term lists.
pub fn run(examples: &Path, spec: &Path) -> std::io::Result<(Vec<String>, usize)> {
    let mut problems = check_spec_lists(spec)?;
    let mut paths: Vec<_> = std::fs::read_dir(examples)?
        .filter_map(Result::ok)
        .map(|e| e.path())
        .filter(|p| p.extension().is_some_and(|e| e.eq_ignore_ascii_case("txt")))
        .collect();
    paths.sort();
    let count = paths.len();
    for path in paths {
        problems.extend(check_transcript(&path)?);
    }
    Ok((problems, count))
}

/// One deliberate corruption of a captured transcript, and what it should be caught as.
pub struct Mutation {
    /// What the corruption represents.
    pub what: &'static str,
    /// Which transcript to corrupt.
    pub file: &'static str,
    /// The text to replace.
    pub from: &'static str,
    /// What to replace it with. Empty deletes the line's content.
    pub to: &'static str,
}

/// Corruptions the checker must reject.
///
/// A gate is not trusted until it has been shown to say no (D125). This project has shipped
/// gates that could never fail, and the protocol is a contract two implementations are built
/// against - not the place to find out about another.
pub const MUTATIONS: [Mutation; 13] = [
    Mutation {
        what: "an unknown verb is answered rather than refused",
        file: "05-refused.txt",
        from: "OBS|refused|2|unknown-verb",
        to: "OBS|done|2|returned|0x0|",
    },
    Mutation {
        what: "an ack names a different verb from its request",
        file: "02-resolve-call.txt",
        from: "OBS|ack|2|resolve",
        to: "OBS|ack|2|call",
    },
    Mutation {
        what: "a sequence number goes backwards",
        file: "02-resolve-call.txt",
        from: "CMD|4|call|0x800124a0",
        to: "CMD|2|call|0x800124a0",
    },
    Mutation {
        what: "one request gets two terminal replies",
        file: "06-read.txt",
        from: "OBS|done|2|returned|0x20|",
        to: "OBS|done|2|returned|0x20|
OBS|done|2|returned|0x20|",
    },
    Mutation {
        what: "an outcome outside the specification",
        file: "08-reset.txt",
        from: "OBS|done|3|returned|0x0|",
        to: "OBS|done|3|maybe|0x0|",
    },
    Mutation {
        what: "a refusal reason outside the specification",
        file: "05-refused.txt",
        from: "OBS|refused|4|unsupported",
        to: "OBS|refused|4|because-i-said-so",
    },
    Mutation {
        what: "a verb answered whose capability was never announced",
        file: "06-read.txt",
        from: "call,resolve,read,report",
        to: "call,report",
    },
    Mutation {
        what: "a command acknowledged and never answered, with nothing saying it died",
        file: "06-read.txt",
        from: "OBS|done|2|returned|0x20|",
        to: "",
    },
    Mutation {
        what: "a capability token outside the specification",
        file: "01-hello.txt",
        from: "call,read,blob,reset,report,gpu",
        to: "call,read,teleport",
    },
    Mutation {
        what: "a line that is neither request, reply, nor annotation",
        file: "01-hello.txt",
        from: "OBS|done|1|ok||",
        to: "everything went fine",
    },
    Mutation {
        what: "a repeated sequence answered instead of refused",
        file: "10-bad-sequence.txt",
        from: "OBS|refused|1|bad-argument",
        to: "OBS|done|1|ok||",
    },
    Mutation {
        what: "a malformed sequence refused for the wrong reason",
        file: "10-bad-sequence.txt",
        from: "OBS|refused|0|bad-argument",
        to: "OBS|refused|0|unknown-verb",
    },
    // A bytes run one nibble too long to be whole bytes - the exact defect a consumer's
    // decoder caught that this checker had passed. If this stops being caught, the contract
    // can carry a fixture no reader can decode.
    Mutation {
        what: "a bytes record with an odd number of hex digits",
        file: "06-read.txt",
        from: "contents|0|35000000610000000000000000000000",
        to: "contents|0|350000006100000000000000000000000",
    },
];

/// Prove the checker rejects each corruption, having first proved it accepts the originals.
///
/// The control is not decorative: if the checker rejected the originals then every "caught"
/// below would be meaningless, because it would reject anything.
pub fn selftest(examples: &Path, spec: &Path) -> std::io::Result<(usize, Vec<String>)> {
    let (control, _) = run(examples, spec)?;
    if !control.is_empty() {
        return Ok((
            0,
            vec![format!(
                "the unmodified transcripts do not pass, so nothing below would mean                  anything: {}",
                control.join("; ")
            )],
        ));
    }

    let mut missed = Vec::new();
    let mut caught = 0usize;
    for mutation in &MUTATIONS {
        let path = examples.join(mutation.file);
        let original = std::fs::read_to_string(&path)?;
        if !original.contains(mutation.from) {
            missed.push(format!(
                "{}: the text to corrupt is not in {} - the fixture changed and this                  mutation now tests nothing",
                mutation.what, mutation.file
            ));
            continue;
        }
        let corrupted = original.replacen(mutation.from, mutation.to, 1);
        std::fs::write(&path, &corrupted)?;
        let outcome = check_transcript(&path);
        std::fs::write(&path, &original)?;
        if outcome?.is_empty() {
            missed.push(format!("{}: not caught", mutation.what));
        } else {
            caught = caught.saturating_add(1);
        }
    }
    Ok((caught, missed))
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Each call gets its own file. A shared path raced under the parallel test runner and
    /// produced a `NotFound` from another test's cleanup - a fault in the test, not the
    /// checker, and one worth not rediscovering.
    fn check_named(tag: &str, body: &str) -> Vec<String> {
        let dir = std::env::temp_dir().join("obscene-protocol-tests");
        std::fs::create_dir_all(&dir).expect("temp dir");
        let path = dir.join(format!("{tag}.txt"));
        std::fs::write(&path, body).expect("write");
        let out = check_transcript(&path).expect("check");
        std::fs::remove_file(&path).ok();
        out
    }

    /// A `hello` exchange as the real transcripts spell it: acknowledged, answered with the
    /// capability line, and terminated with a `done`. Fixtures that skip the terminal record
    /// trip the dangling-ack rule and test the wrong thing.
    const HELLO: &str = "CMD|1|hello|1
OBS|ack|1|hello
OBS|hello|1|s|call,resolve,read
OBS|done|1|ok||
";

    #[test]
    fn a_complete_exchange_passes() {
        let body = format!(
            "{HELLO}CMD|2|resolve|sceKernelOpen
OBS|ack|2|resolve
OBS|done|2|ok|0x1000
"
        );
        assert!(check_named("complete", &body).is_empty());
    }

    /// The property the whole protocol is arranged around.
    #[test]
    fn an_acknowledged_command_that_never_answers_is_a_fault() {
        let body = format!(
            "{HELLO}CMD|2|call|0x1000
OBS|ack|2|call
"
        );
        let problems = check_named("dangling", &body);
        assert_eq!(problems.len(), 1, "got {problems:?}");
        assert!(problems.iter().any(|p| p.contains("never answered")));
    }

    /// A transcript that merely *mentions* another one must not satisfy the dangling-ack
    /// rule. The first version searched the whole file for the word and let a truncation
    /// through.
    #[test]
    fn mentioning_died_in_a_comment_does_not_excuse_a_dangling_ack() {
        let body = format!(
            "# see 03-died.txt for the timeout case
{HELLO}CMD|2|call|0x1
OBS|ack|2|call
"
        );
        let problems = check_named("comment", &body);
        assert!(problems.iter().any(|p| p.contains("never answered")));
    }

    #[test]
    fn an_unknown_verb_must_be_refused_as_unknown_verb() {
        let refused = format!(
            "{HELLO}CMD|2|frobnicate
OBS|refused|2|unknown-verb
"
        );
        assert!(check_named("unknown-ok", &refused).is_empty());

        let answered = format!(
            "{HELLO}CMD|2|frobnicate
OBS|ack|2|frobnicate
OBS|done|2|ok||
"
        );
        assert!(
            check_named("unknown-answered", &answered)
                .iter()
                .any(|p| p.contains("answered rather than refused"))
        );

        let wrong = format!(
            "{HELLO}CMD|2|frobnicate
OBS|refused|2|unsupported
"
        );
        assert!(
            check_named("unknown-wrong", &wrong)
                .iter()
                .any(|p| p.contains("must be unknown-verb"))
        );
    }

    /// A repeated sequence cannot be acknowledged, because an `ack` is keyed by sequence.
    #[test]
    fn a_repeated_sequence_is_refused_without_an_ack() {
        let ok = format!(
            "{HELLO}CMD|1|call|0x1
OBS|refused|1|bad-argument
"
        );
        assert!(
            check_named("repeat-ok", &ok).is_empty(),
            "{:?}",
            check_named("repeat-ok2", &ok)
        );

        let answered = format!(
            "{HELLO}CMD|1|call|0x1
OBS|done|1|ok||
"
        );
        assert!(
            check_named("repeat-answered", &answered)
                .iter()
                .any(|p| p.contains("must be refused as bad-argument"))
        );
    }

    #[test]
    fn a_verb_needing_an_unannounced_capability_must_be_refused() {
        let body = "CMD|1|hello|1
OBS|ack|1|hello
OBS|hello|1|s|resolve
OBS|done|1|ok||
CMD|2|call|0x1000
OBS|ack|2|call
OBS|done|2|returned|0x0|
";
        let problems = check_named("capability", body);
        assert!(
            problems.iter().any(|p| p.contains("capability")),
            "got {problems:?}"
        );
    }

    /// The defect a consumer's decoder found and this checker had passed.
    #[test]
    fn an_odd_number_of_hex_digits_is_not_bytes() {
        let body = format!(
            "{HELLO}CMD|2|read|0x1000|0x1
OBS|ack|2|read
OBS|bytes|2|0x1000|abc
OBS|done|2|ok||
"
        );
        assert!(
            check_named("odd-hex", &body)
                .iter()
                .any(|p| p.contains("whole number of bytes"))
        );
    }

    #[test]
    fn a_non_hex_digit_in_a_bytes_record_is_a_fault() {
        let body = format!(
            "{HELLO}CMD|2|read|0x1000|0x1
OBS|ack|2|read
OBS|bytes|2|0x1000|zz
OBS|done|2|ok||
"
        );
        assert!(
            check_named("non-hex", &body)
                .iter()
                .any(|p| p.contains("non-hex"))
        );
    }
}
