# D032 - The output channel is chosen by what moves a byte

**Status:** decided
**Date:** 2026-09-26

`obs_write` tries `sceKernelWrite`, `puts`, `write` and `putchar` in that order and keeps the
first that accepts a byte. The channel used is appended to the `end` record. `puts` is used only
for whole records that already end in a newline. An eboot does not call raw import channels; it
writes through pointers resolved by name before the first record. The system log and the file
sink are written separately and unconditionally, because on a title no descriptor may be
listening and the sandboxed report file is torn down when the title exits.

**Why:** a probe that reports through one function turns that function into a check whose failure
takes the whole report with it. Loaders differ: one discards `write`, one refuses descriptor 1,
one implements only `puts`. Which channel worked is itself a finding.

**Rejected:** a single channel - silent on the loaders that lack it. An emulator-specific
reporting hook - measures that emulator's opinion of itself.
