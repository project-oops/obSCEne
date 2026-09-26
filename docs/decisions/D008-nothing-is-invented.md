# D008 - Nothing is invented; uncertain signatures are omitted

**Status:** decided
**Date:** 2026-09-26

Where an arity, a constant or a structure layout is uncertain, the function is not called with an
expectation attached. Adding a check means confirming its signature from a nameable source. The
rule governs expectations: a call that asserts nothing and only records what came back (a byte
dump, the blind prober, a protocol `call`) is permitted where the ABI makes it safe.

**Why:** a wrong arity corrupts the stack and fails far from the cause. A wrong constant makes the
call succeed and do something else, silently. Either costs more to diagnose than the declaration
saves, and the report is only worth something if it can be trusted.

**Rejected:** plausible guesses marked as such - a guess that runs produces a confident record
nobody can distinguish from a measurement.
