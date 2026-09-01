# Example runs

Real reports, kept so a change can be diffed against something that actually happened
rather than against a description of it.

## `full-sweep.txt`

The complete run: 17 sections, 88 checks, 523 records, a terminator, and a final tally of
59 pass / 3 partial / 20 fail / 6 skip. `obscene-tool verify` calls it well-formed.

Two findings in it are worth looking at directly, because they are what the suite is for:

- `015-sync/event-flag-round-trip` fails - a bit that is set and then cleared still polls
  as set, in the release that rewrote event flags.
- `015-sync/thread-churn` passes here, and crashed the emulator roughly one run in four
  on a slightly earlier build. An intermittent crash is only a finding once it has been
  counted (D046).

Read the `expectations:` line before drawing conclusions from any red. Nothing in this
report has been confirmed against real hardware, so a failure marked `[assumed]` may be
this suite's belief rather than the platform's bug.

Two checks are excluded, which is what made it complete. Each of them ends the emulator
process rather than returning, and a call that does that takes every check behind it with
it - the first run reached 110 records. They are reported as skips with the reason, so
they stay visible and a diff still sees them stop being run:

```bash
make module EXCLUDE="040-file/open-rejects-null 080-video/flip-rate-rejects-bad-handle"
```

Read `900-surface/control` before believing any census count in it. It fails, correctly:
this emulator resolves every unrecognised import to a stub, so the presence test cannot
tell a real function from a placeholder.

## `emulator-run.txt`

The **first** run against an emulator, kept because it is where the crash was found
rather than excluded. 110 records, stopping a third of the way in.

It is **not a clean report**, and that is why it is worth keeping:

- It stops inside `040-file/open-rejects-null` - a `try` with no matching `res`. Passing
  a null path to `sceKernelOpen` throws inside that emulator and takes the process down.
  The report names the call because every check announces itself before making it.
- A handful of value fields contain bytes that are obviously not values, e.g.
  `0x3c205d65726f435b` - that is the ASCII `[Core] <`. The emulator writes its own log
  to the same stream, and a record can be interleaved with one. Worth knowing before
  trusting a value in a report captured this way.

Compare a later run against it with:

```bash
obscene-tool diff docs/examples/emulator-run.txt build/current.txt
```

which reports what got **worse**, not what is failing - under an early emulator almost
everything fails, and a tool that called that a regression would be ignored within a
day.
