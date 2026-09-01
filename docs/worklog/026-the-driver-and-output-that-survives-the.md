# The driver, and output that survives the run - completed


`tool/src/drive.rs` and the `drive` subcommand close order item 1: obSCEne can now be driven
over the protocol and its answers committed as corpus records, and the same driver reads a
captured transcript so the whole path runs with no hardware.

Verified: `verify: ok` (104 tool tests, clippy clean under `-D warnings`, all three C
targets, protocol gates), and `drive --replay` over the captured transcripts produces
correct corpus records - a death attributed to the driver with an empty value, a real return
carried verbatim, provenance denormalised onto every line.

### Surprises

**Auto mode's safety classifier blocked every Bash command for most of a session.** It is
separate from the permission system, fires on accumulated conversation content, and persists
for the conversation - so the security-tooling vocabulary here (arbitrary calls, memory
read/write, a remote command socket) tripped it. The fix was default mode plus an allowlist
in `.claude/settings.local.json`; the classifier is an auto-mode gate and does not run when
a human approves. Recorded here because it cost real time and the cause was not obvious.

**Every clippy warning is an error here** (`lint.sh` runs `-D warnings`), and `drive.rs`
arrived with 20 - guarded indexing, one arithmetic op, a collapsible `if`, an over-long
dispatcher. Production indexing was rewritten through a `.get()` field accessor; the test
module opts into indexing with the same `#[allow(reason=...)]` the other modules' tests use;
the dispatcher arm became a `DriveArgs` struct, matching `MkmoduleArgs`.

