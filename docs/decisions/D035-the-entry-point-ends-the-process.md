# D035 - The entry point ends the process

**Status:** decided
**Date:** 2026-09-26

After the suite, `obscene_start` calls `exit`, and spins if there is none. It does not return.

**Why:** the module is an executable, and its entry is where the process starts; returning pops
whatever the loader left on the stack and faults right after the last check, which reads as that
check crashing. Exiting and spinning are distinguishable from each other and from a crash.

**Rejected:** returning to the loader so the probe can be rerun in one session - produces a fault
indistinguishable from a real one.
