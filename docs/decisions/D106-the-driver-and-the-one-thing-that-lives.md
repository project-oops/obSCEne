# D106 - The driver, and the one thing that lives only at this end


Status: derived - proven end to end by replaying the captured transcripts, no hardware.

`tool/src/drive.rs` is the other end of `docs/PROTOCOL.md`. It exists because **half the
protocol's meaning is established by the driver and cannot be established by the probe**: a
probe that faults is gone before it can report it, leaving an acknowledgement with nothing
after it. Turning that silence into a record - `died`, never `returned 0` - is this side's
job.

`died`, `timeout` and `lost` are separate outcomes and none collapses into a value. The
corpus marks each record with `observed_by`: `probe` for what the system reported, `driver`
for what was inferred from its silence. A reader can therefore weight a fact against an
inference, which a single undifferentiated record cannot support.

### Replay is the contract, not a convenience

The driver reads a live socket and a captured transcript through the same parser. That is
why the specification shipped with transcripts: `obscene-tool drive --replay
docs/examples/protocol/03-died.txt` exercises the whole path with nothing attached, and it
is how the driver was built and checked months before any console exists.

### Two bugs the build and the replay caught, both silent-wrong rather than loud

1. **Sequence numbers were the caller's, not the driver's.** The first version wrote each
   command verbatim while numbering its own bookkeeping separately, so a timeout could be
   attributed to the wrong command - a wrong record, not a crash. The driver now owns the
   `CMD|<seq>|` framing; the caller passes only the verb and arguments.

2. **A replayed death was blamed on the probe.** A captured transcript records a death as an
   explicit `done|died` line, and the parser classified every `done` as an answer - so
   replay attributed the death to the probe while the live path attributed it to the driver.
   The two must agree or replay is worthless. Non-answer words in a `done` line now map back
   to their own outcome, and a test asserts replay and live land identically.

### The capability check the compiler found

`capable_of` was dead code, which meant the driver read the announced capabilities and never
consulted them - while the specification says a driver must not send a command whose
capability was not announced. The failure that would have caused is a *record*: an
un-negotiated verb is refused, and a refusal in the corpus reads as a fact about the
platform (`resolve` refused looks like "cannot resolve by name") when it is a fact about the
driver. The driver now declines to send, recording `not-sent` with `observed_by=driver` - a
distinct outcome from a refusal, because the absence of a question and the platform's answer
must never be confused.

