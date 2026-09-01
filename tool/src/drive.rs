//! The driver: the other end of `docs/PROTOCOL.md`.
//!
//! # What a driver is actually for
//!
//! Not "sending commands" - a shell could do that. It exists because **half the protocol's
//! meaning is established by this end, and cannot be established by the other one.**
//!
//! A probe cannot report its own death. When a command faults, the process is gone before
//! it can say so, and all that is left is an acknowledgement with nothing after it. Turning
//! that into a record is the driver's job, and it is the single most important thing in
//! this file:
//!
//! > A command that did not answer is never recorded as having answered.
//!
//! `died`, `timeout` and `lost` are three distinct outcomes and none of them is
//! `returned 0`. A corpus that blurs them is worse than no corpus - the fiction is
//! indistinguishable from evidence, and it is the fiction that gets trusted later.
//!
//! # The transport is separable, and that is what makes this testable
//!
//! Everything here works over anything that yields lines. A TCP connection is the obvious
//! source; a captured transcript from `docs/examples/protocol/` is another, and it needs no
//! hardware at all.
//!
//! That is not a testing convenience bolted on afterwards - it is why the specification
//! shipped with transcripts. A driver that can only be exercised against a live console is
//! a driver that gets written once the console arrives, which is the wrong order.

use std::collections::BTreeMap;
use std::fmt::Write as _;

/// What a command did, from the driver's point of view.
///
/// The three non-answers are separate variants rather than an `Option`, because the whole
/// design rests on them being distinguishable. Collapsing them into "no value" would put
/// the decision back where it cannot be made correctly.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Outcome {
    /// The probe answered. The payload is exactly what it said, uninterpreted.
    Answered {
        outcome: String,
        value: String,
        detail: String,
    },
    /// Refused, with a reason from the specification's closed list.
    Refused { reason: String },
    /// Acknowledged, then the connection closed with no result. **The command ended the
    /// process.** Established here, never by the probe.
    Died,
    /// Acknowledged, no result within the budget, and the connection is still open. The
    /// probe may be alive, blocked, or looping - deliberately *not* resolved into `Died`,
    /// because a blocked call and a dead process look identical from this end and the
    /// honest record says which was observed.
    TimedOut,
    /// Acknowledged, connection closed, and it is not established whether the probe came
    /// back. Ambiguous, and recorded as ambiguous.
    Lost,
    /// Never put on the wire, because the probe did not announce the capability it needs.
    ///
    /// A separate outcome rather than a refusal, and the distinction is the point. A
    /// refusal is something the *platform* said, and in a corpus it reads as one - a
    /// refused `resolve` looks like "this platform cannot resolve by name". This says the
    /// question was never asked, which is a fact about the driver and not about the
    /// platform, and it must not be mistaken for the other.
    NotSent { capability: String },
}

impl Outcome {
    /// The word written into the corpus.
    pub fn word(&self) -> &str {
        match self {
            Outcome::Answered { outcome, .. } => outcome,
            Outcome::Refused { .. } => "refused",
            Outcome::Died => "died",
            Outcome::TimedOut => "timeout",
            Outcome::Lost => "lost",
            Outcome::NotSent { .. } => "not-sent",
        }
    }

    /// Which end established this.
    ///
    /// In the corpus so a reader can separate a fact the system reported from one inferred
    /// from its silence. They are not equally strong and the record should not pretend they
    /// are.
    pub fn observed_by(&self) -> &'static str {
        match self {
            Outcome::Answered { .. } | Outcome::Refused { .. } => "probe",
            Outcome::Died | Outcome::TimedOut | Outcome::Lost | Outcome::NotSent { .. } => "driver",
        }
    }
}

/// One command and what became of it.
#[derive(Debug, Clone)]
pub struct Exchange {
    pub seq: u64,
    pub verb: String,
    pub outcome: Outcome,
    /// Records the probe emitted between the acknowledgement and the result - `sym`,
    /// `bytes`, `module`. Kept verbatim: they are defined by `docs/OUTPUT.md` and mean the
    /// same here as in a report, so re-interpreting them would be inventing a second
    /// meaning for a record that already has one.
    pub records: Vec<String>,
}

/// A session's outcome, and everything needed to interpret it.
#[derive(Debug, Clone, Default)]
pub struct Session {
    /// The probe's identifier for this session. **A different one where the driver expected
    /// the old one means the probe restarted**, and everything before it belongs to a
    /// different process.
    pub id: String,
    pub protocol_version: String,
    pub capabilities: Vec<String>,
    /// What produced the answers: device, driver, firmware, build.
    pub part: BTreeMap<String, String>,
    pub exchanges: Vec<Exchange>,
}

impl Session {
    /// Whether a capability was announced.
    ///
    /// Test-only: the live driver checks capabilities inline in `run_session`, against the
    /// list as it arrives, because it has to decide before sending the *next* command
    /// rather than after the session is parsed. This is the same question asked of a
    /// finished session, which is what the tests want and nothing on the hot path does.
    #[cfg(test)]
    pub fn capable_of(&self, capability: &str) -> bool {
        self.capabilities.iter().any(|c| c == capability)
    }
}

/// Which capability gates a verb.
///
/// Not one-to-one, and that is the reason this is a function rather than a comparison:
/// `blob` and `run` are two halves of one ability, so a driver that assumed
/// verb-equals-capability would demand a `run` token that the specification never defines
/// and refuse to send a command the probe would have answered.
///
/// An unrecognised verb maps to itself, which makes it fail the capability test and never
/// reach the wire. That is the right default - the specification requires a responder to
/// refuse what it does not know, and there is nothing to learn from making it do so.
fn verb_capability(verb: &str) -> &str {
    match verb {
        "run" => "blob",
        other => other,
    }
}

/// Parses a session from lines that have already been exchanged.
///
/// Takes the whole stream rather than driving it, so the same code reads a live connection
/// and a captured transcript. `connection_closed` says whether the stream ended because the
/// far end went away - which is what separates `Died` from `Lost`, and cannot be recovered
/// from the bytes alone.
pub fn parse_session(lines: &[String], connection_closed: bool) -> Session {
    let mut session = Session::default();
    // seq -> (verb, records so far). An entry still here at the end was acknowledged and
    // never answered, which is the case this whole module exists for.
    let mut pending: Vec<(u64, String, Vec<String>)> = Vec::new();

    for line in lines {
        let line = line.trim_end_matches(['\r', '\n']);
        if line.is_empty() || line.starts_with('#') || line.starts_with("CMD|") {
            continue;
        }
        let fields: Vec<&str> = line.split('|').collect();
        if fields.first() != Some(&"OBS") {
            continue;
        }
        // Every field read through `.get()` rather than by index. The record shapes vary in
        // length - a `done` has more fields than an `ack` - so an index that is safe for one
        // record kind panics on another. `.get(...).unwrap_or("")` turns a missing field
        // into an empty one, which the matching below already treats as "not that record",
        // and keeps a malformed line from taking the parser down. (This is production code;
        // the test module opts into indexing separately, as the other modules' tests do.)
        let field = |n: usize| -> &str { fields.get(n).copied().unwrap_or("") };
        let kind = field(1);
        match kind {
            "ack" => {
                if let Ok(seq) = field(2).parse::<u64>() {
                    pending.push((seq, field(3).to_string(), Vec::new()));
                }
            }
            "hello" => {
                session.protocol_version = field(2).to_string();
                session.id = field(3).to_string();
                session.capabilities = field(4)
                    .split(',')
                    .filter(|t| !t.is_empty())
                    .map(str::to_string)
                    .collect();
            }
            "part" => {
                session
                    .part
                    .insert(field(3).to_string(), field(4).to_string());
            }
            "done" | "refused" => {
                let Ok(seq) = field(2).parse::<u64>() else {
                    continue;
                };
                let Some(at) = pending.iter().position(|(s, _, _)| *s == seq) else {
                    // A refusal with no acknowledgement is legitimate for exactly one
                    // reason: a malformed or repeated sequence, which cannot be
                    // acknowledged because an acknowledgement is keyed by sequence.
                    if kind == "refused" {
                        session.exchanges.push(Exchange {
                            seq,
                            verb: String::new(),
                            outcome: Outcome::Refused {
                                reason: field(3).to_string(),
                            },
                            records: Vec::new(),
                        });
                    }
                    continue;
                };
                let (seq, verb, records) = pending.remove(at);
                let outcome = if kind == "refused" {
                    Outcome::Refused {
                        reason: field(3).to_string(),
                    }
                } else {
                    // A non-answer written into the stream is mapped back to its own
                    // variant, not treated as an answer whose value happens to be "died".
                    //
                    // A captured transcript records a death as an explicit `done|died` line
                    // - that is the canonical form, and the protocol checker validates it -
                    // while the live path synthesises `Died` from a dangling `ack`. Both must
                    // land on the same outcome, or a replayed session would attribute the
                    // death to the probe (observed-by) when the live session attributes it to
                    // the driver. Re-deriving here keeps replay and live identical, which is
                    // the property that makes replay worth having.
                    match field(3) {
                        "died" => Outcome::Died,
                        "timeout" => Outcome::TimedOut,
                        "lost" => Outcome::Lost,
                        other => Outcome::Answered {
                            outcome: other.to_string(),
                            value: field(4).to_string(),
                            detail: field(5).to_string(),
                        },
                    }
                };
                session.exchanges.push(Exchange {
                    seq,
                    verb,
                    outcome,
                    records,
                });
            }
            _ => {
                // Anything else belongs to whichever command is outstanding.
                if let Some(last) = pending.last_mut() {
                    last.2.push(line.to_string());
                }
            }
        }
    }

    // Whatever is still outstanding did not answer. This is where the driver does the one
    // thing the probe cannot, and the distinction between the two verdicts is real: a
    // stream that ended because the far end went away is evidence the process died; a
    // stream that merely ran out is not evidence of anything, and says so.
    for (seq, verb, records) in pending {
        session.exchanges.push(Exchange {
            seq,
            verb,
            outcome: if connection_closed {
                Outcome::Died
            } else {
                Outcome::Lost
            },
            records,
        });
    }
    session.exchanges.sort_by_key(|e| e.seq);
    session
}

/// Renders a session as corpus records.
///
/// # Why the provenance is repeated on every line
///
/// The wire binds `part` to a session identifier, which is efficient and correct for a
/// stream somebody is watching. A corpus is not watched, it is queried - months later, a
/// line at a time, by someone who has never seen the session it came from.
///
/// A record that has to be joined against something else to be interpreted will eventually
/// be read without it. **A figure measured on one part and read as authoritative for
/// another is a wrong answer with nothing in the record to reveal it** - which is precisely
/// the failure this project's sibling spent months inside. So the corpus denormalises, and
/// the redundancy is the point rather than a cost.
pub fn to_corpus(session: &Session) -> String {
    let mut out = String::new();
    let provenance: String = session
        .part
        .iter()
        .map(|(k, v)| format!("{k}={v}"))
        .collect::<Vec<_>>()
        .join(",");

    for exchange in &session.exchanges {
        for record in &exchange.records {
            let _ = writeln!(
                out,
                "OBSCORPUS|record|{}|{}|{}|{}",
                session.id, exchange.seq, record, provenance
            );
        }
        let (value, detail) = match &exchange.outcome {
            Outcome::Answered { value, detail, .. } => (value.as_str(), detail.as_str()),
            Outcome::Refused { reason } => ("", reason.as_str()),
            Outcome::NotSent { capability } => ("", capability.as_str()),
            // Deliberately empty. Writing `0x0` here is the exact fiction the format
            // exists to prevent, and it would be indistinguishable from a call that
            // genuinely returned zero.
            _ => ("", ""),
        };
        let _ = writeln!(
            out,
            "OBSCORPUS|call|{}|{}|{}|{}|{}|{}|{}|{}",
            session.id,
            exchange.seq,
            exchange.verb,
            exchange.outcome.word(),
            value,
            detail,
            exchange.outcome.observed_by(),
            provenance
        );
    }
    out
}

/// Drives a live probe, one command at a time.
///
/// # One in flight, and it is not an implementation shortcut
///
/// The driver sends a command and waits for its result before sending the next. Overlapping
/// them would make a death unattributable: with two commands outstanding and a process that
/// has just vanished, nothing says which one killed it - and that attribution *is* the
/// finding.
///
/// # The budget, and what running out of it means
///
/// A read timeout is the only way this end can observe a command that has not come back. It
/// produces `TimedOut` and **never** `Died`: the connection is still open, so the probe may
/// be alive, blocked, or looping, and this end cannot tell which. Recording the distinction
/// costs nothing; erasing it would put a guess into the corpus wearing the clothes of an
/// observation.
pub fn run_session(
    address: &str,
    commands: &[String],
    budget: std::time::Duration,
) -> std::io::Result<Session> {
    use std::io::{BufRead, BufReader, Write};

    let stream = std::net::TcpStream::connect(address)?;
    stream.set_read_timeout(Some(budget))?;
    let mut writer = stream.try_clone()?;
    let mut reader = BufReader::new(stream);

    let mut transcript: Vec<String> = Vec::new();
    let mut closed = false;
    let mut timed_out_at: Option<u64> = None;
    // Learned from the greeting, and empty until one arrives - which is why the check below
    // is skipped while it is empty rather than treating "nothing announced" as "nothing
    // permitted". Before a greeting there is no list to consult, not an empty one.
    let mut session_capabilities: Vec<String> = Vec::new();
    let mut not_sent: Vec<(u64, String)> = Vec::new();

    // The driver owns sequence numbers, and the caller supplies only the verb and its
    // arguments.
    //
    // The first version wrote each command verbatim while numbering them here, so the
    // sequence this end believed it had sent and the one actually on the wire agreed only
    // if the caller happened to number them identically. That is not a cosmetic mismatch:
    // `timed_out_at` is matched against sequences parsed back from the wire, so a timeout
    // would have been attached to the wrong command - or to none, quietly degrading to
    // `Lost`. A wrong record rather than a crash, which is the expensive kind.
    //
    // Owning them here also makes the strictly-increasing rule this end's responsibility,
    // which is where it belongs: a caller cannot get it wrong if it never states it.
    for (index, command) in commands.iter().enumerate() {
        // `try_from` rather than `as`. The lint that objects to a bare cast here is
        // pedantic and correct: `as` is silent when it truncates, and this project has
        // already had one build stopped by exactly that lint on exactly that reasoning.
        // A command list long enough to overflow is not a real case, which is why
        // saturating is a safe answer rather than a lie.
        // Saturating, and split from the cast, because a bare `index + 1` is arithmetic
        // the lint flags for silent wrap. Neither overflow is reachable with a real command
        // list; saturating is a safe answer to an impossible input rather than a pretence
        // that it cannot happen.
        let seq = u64::try_from(index).unwrap_or(u64::MAX).saturating_add(1);

        // Capabilities are checked here, not only announced.
        //
        // The specification says a driver must not send a command whose capability was
        // not announced, and until now this end read the list and never consulted it -
        // which the compiler noticed before anyone else did, as an unused method.
        //
        // Enforcing it matters because the failure it prevents is a *record*, not an
        // error. A probe that receives an un-negotiated verb refuses it, and a refusal
        // sits in the corpus looking like a fact about the platform: `resolve` refused
        // reads as "this platform cannot resolve by name". It cannot be told apart
        // afterwards from a driver that simply should not have asked. Declining to send
        // keeps the question out of the corpus entirely, which is the honest outcome for
        // a question that was never askable.
        //
        // `hello` and `bye` are session control and are always available - and the check
        // only applies once a greeting has established what the far end can do, since
        // before that there is nothing to consult.
        let verb = command.split('|').next().unwrap_or("");
        if !session_capabilities.is_empty()
            && !matches!(verb, "hello" | "bye")
            && !session_capabilities
                .iter()
                .any(|c| c == verb_capability(verb))
        {
            not_sent.push((seq, verb.to_string()));
            continue;
        }

        let line = format!("CMD|{seq}|{command}");
        writeln!(writer, "{line}")?;
        writer.flush()?;
        transcript.push(line);

        // Read until this command is terminated, the stream ends, or the budget runs out.
        loop {
            let mut line = String::new();
            match reader.read_line(&mut line) {
                Ok(0) => {
                    closed = true;
                    break;
                }
                Ok(_) => {
                    let trimmed = line.trim_end_matches(['\r', '\n']).to_string();
                    // Captured as it arrives rather than after the session, because the
                    // very next command has to be checked against it.
                    if let Some(tokens) = trimmed
                        .strip_prefix("OBS|hello|")
                        .and_then(|rest| rest.split('|').nth(2))
                    {
                        session_capabilities = tokens
                            .split(',')
                            .filter(|t| !t.is_empty())
                            .map(str::to_string)
                            .collect();
                    }
                    let terminal =
                        trimmed.starts_with("OBS|done|") || trimmed.starts_with("OBS|refused|");
                    transcript.push(trimmed);
                    if terminal {
                        break;
                    }
                }
                Err(e)
                    if e.kind() == std::io::ErrorKind::WouldBlock
                        || e.kind() == std::io::ErrorKind::TimedOut =>
                {
                    timed_out_at = Some(seq);
                    break;
                }
                Err(e) => return Err(e),
            }
        }
        if closed || timed_out_at.is_some() {
            break;
        }
    }

    let mut session = parse_session(&transcript, closed);
    // The timeout is applied after parsing, because only this end knows the budget elapsed.
    // Parsing would otherwise have called it `Lost` - true but weaker, and this end has
    // better evidence: the connection was still open when the clock ran out.
    if let Some(seq) = timed_out_at {
        for exchange in &mut session.exchanges {
            if exchange.seq == seq && matches!(exchange.outcome, Outcome::Lost) {
                exchange.outcome = Outcome::TimedOut;
            }
        }
    }

    // Commands that never reached the wire still go in the corpus.
    //
    // Leaving them out would be tidier and would lose the thing worth knowing: that a
    // question was asked of the driver and not of the platform. A corpus with a gap where
    // a command should be reads as though it was never wanted; one that says `not-sent`
    // says the target could not be asked, which is a fact about the target.
    for (seq, verb) in not_sent {
        let capability = verb_capability(&verb).to_string();
        session.exchanges.push(Exchange {
            seq,
            verb,
            outcome: Outcome::NotSent { capability },
            records: Vec::new(),
        });
    }
    session.exchanges.sort_by_key(|e| e.seq);
    Ok(session)
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    reason = "test fixtures build known-size inputs; a panic here is the failure signal"
)]
mod tests {
    use super::*;

    fn lines(text: &str) -> Vec<String> {
        text.lines().map(str::to_string).collect()
    }

    /// The case the whole module exists for.
    #[test]
    fn an_acknowledgement_with_no_result_is_died_not_zero() {
        let session = parse_session(&lines("OBS|ack|1|call\n"), true);
        assert_eq!(session.exchanges.len(), 1);
        assert_eq!(session.exchanges[0].outcome, Outcome::Died);

        let corpus = to_corpus(&session);
        // The value field must be empty. A zero here is the fiction.
        assert!(corpus.contains("|died|||driver|"), "corpus was: {corpus}");
        assert!(
            !corpus.contains("0x0"),
            "a death must never carry a value: {corpus}"
        );
    }

    /// A stream that merely ran out is not evidence the probe died.
    #[test]
    fn an_unfinished_stream_is_lost_rather_than_died() {
        let session = parse_session(&lines("OBS|ack|1|call\n"), false);
        assert_eq!(session.exchanges[0].outcome, Outcome::Lost);
    }

    /// Replay and live must agree on who established a death.
    ///
    /// A captured transcript writes a death as an explicit `done|died` line; the live path
    /// synthesises `Died` from a dangling ack. Both must produce `Died`, attributed to the
    /// driver - otherwise a replayed corpus blames the probe for a silence the driver
    /// observed, which is the exact confusion this module exists to prevent.
    #[test]
    fn a_death_written_into_a_transcript_is_attributed_to_the_driver() {
        let session = parse_session(
            &lines(
                "OBS|ack|1|call\n\
                 OBS|done|1|died||connection closed after ack with no result\n",
            ),
            true,
        );
        assert_eq!(session.exchanges[0].outcome, Outcome::Died);
        assert_eq!(session.exchanges[0].outcome.observed_by(), "driver");
        // And the live synthesis from a dangling ack lands in the same place.
        let live = parse_session(&lines("OBS|ack|1|call\n"), true);
        assert_eq!(live.exchanges[0].outcome, Outcome::Died);
    }

    #[test]
    fn a_returned_value_is_carried_verbatim() {
        let session = parse_session(
            &lines("OBS|ack|1|call\nOBS|done|1|returned|0x8002000e|\n"),
            true,
        );
        match &session.exchanges[0].outcome {
            Outcome::Answered { outcome, value, .. } => {
                assert_eq!(outcome, "returned");
                assert_eq!(value, "0x8002000e");
            }
            other => panic!("expected an answer, got {other:?}"),
        }
    }

    #[test]
    fn capabilities_and_provenance_are_captured() {
        let session = parse_session(
            &lines(
                "OBS|ack|1|hello\n\
                 OBS|hello|1|c0x1|call,read,report\n\
                 OBS|part|c0x1|gpu|AMD Custom GPU 0405 (gfx1033)\n\
                 OBS|done|1|ok||\n",
            ),
            true,
        );
        assert_eq!(session.id, "c0x1");
        assert!(session.capable_of("read"));
        assert!(!session.capable_of("write"));
        assert_eq!(
            session.part.get("gpu").map(String::as_str),
            Some("AMD Custom GPU 0405 (gfx1033)")
        );
        // Every corpus line carries the part, because a line read alone must still be
        // interpretable.
        let corpus = to_corpus(&session);
        for line in corpus.lines() {
            assert!(line.contains("gfx1033"), "line lacks provenance: {line}");
        }
    }

    /// A command the target cannot be asked is recorded, and is not a refusal.
    ///
    /// The two look alike and mean opposite things: a refusal is the platform's answer,
    /// and this is the absence of a question. Only the second may be attributed to the
    /// driver.
    #[test]
    fn a_command_never_sent_is_distinct_from_one_refused() {
        let not_sent = Outcome::NotSent {
            capability: "resolve".to_string(),
        };
        let refused = Outcome::Refused {
            reason: "not-negotiated".to_string(),
        };
        assert_eq!(not_sent.word(), "not-sent");
        assert_eq!(refused.word(), "refused");
        assert_eq!(not_sent.observed_by(), "driver");
        assert_eq!(refused.observed_by(), "probe");
    }

    /// `run` is gated by `blob`, not by a capability of its own.
    #[test]
    fn a_verb_maps_to_the_capability_that_gates_it() {
        assert_eq!(verb_capability("run"), "blob");
        assert_eq!(verb_capability("read"), "read");
        // Unrecognised verbs map to themselves, so they fail the check and never reach
        // the wire.
        assert_eq!(verb_capability("disassemble"), "disassemble");
    }

    /// A refusal with no acknowledgement is legitimate for exactly one reason.
    #[test]
    fn a_refusal_without_an_acknowledgement_is_kept() {
        let session = parse_session(&lines("OBS|refused|1|bad-argument\n"), true);
        assert_eq!(session.exchanges.len(), 1);
        assert_eq!(
            session.exchanges[0].outcome,
            Outcome::Refused {
                reason: "bad-argument".to_string()
            }
        );
    }

    #[test]
    fn records_between_the_pair_belong_to_the_command() {
        let session = parse_session(
            &lines(
                "OBS|ack|2|read\n\
                 OBS|bytes|read/0x1000|(memory)|contents|0|deadbeef\n\
                 OBS|done|2|returned|0x4|\n",
            ),
            true,
        );
        assert_eq!(session.exchanges[0].records.len(), 1);
        assert!(session.exchanges[0].records[0].contains("deadbeef"));
    }
}
