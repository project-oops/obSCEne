# Example runs

Captured reports to diff a change against. `protocol/` holds example command-protocol
exchanges.

## `full-sweep.txt`

A complete run against an emulator, ending in a terminator and a final tally.
`obscene-tool verify` accepts it as well formed. Nothing in it is confirmed against hardware,
so read the `expectations:` line before treating a failure marked `[assumed]` as the
platform's defect.

Two checks are excluded because each ends the emulator process instead of returning. They
report as skips with the reason, so a diff still sees them:

```bash
make module EXCLUDE="040-file/open-rejects-null 080-video/flip-rate-rejects-bad-handle"
```

`900-surface/control` fails in this report: the emulator resolves every unrecognised import to
a stub, so the presence census cannot tell a real function from a placeholder, and its counts
are void.

`015-sync/thread-churn` crashes this emulator intermittently. An intermittent crash is a
finding only once it has been counted (D046).

## `emulator-run.txt`

A run that stops partway, kept as an example of an incomplete report:

- It ends inside `040-file/open-rejects-null`: a `try` with no matching `res`. A null path to
  `sceKernelOpen` ends that emulator's process, and the `try` line names the call.
- Some value fields hold text rather than values, for example `0x3c205d65726f435b`, the ASCII
  `[Core] <`. The emulator writes its own log to the same stream, and a record can interleave
  with it.

Compare a later run against it with:

```bash
obscene-tool diff docs/examples/emulator-run.txt build/current.txt
```

`diff` reports what got worse, not what is failing.
