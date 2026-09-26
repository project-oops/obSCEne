# D097 - Blame for a loader failure needs a control build

**Status:** decided
**Date:** 2026-09-26

Before a loader failure is attributed to the loader or to obSCEne, the same loader is given a
module built by a standard toolchain. Control runs and obSCEne fails: obSCEne is at fault. Both
fail: the loader is. Control fails and obSCEne runs: the loader's limits are the finding.

**Why:** arguments from source and README claims went both ways and were wrong both ways. A
control settles it in one run.

**Rejected:** reasoning from source and documentation alone.
