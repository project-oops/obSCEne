# D072 - Consensus is a substitute oracle within one generation

**Status:** decided
**Date:** 2026-09-26

`obscene-tool consensus` compares reports from several implementations. A skip is no opinion; a
check fewer than two implementations attempted is not compared. It names implementations and never
counts them, reports `OUTLIER` (one against a unanimous rest) separately from `SPLIT`, and is run
only over reports of the same generation. Where a hardware report exists, `matrix` treats it as
the authority (D302).

**Why:** agreement is actionable where no console answers. Counting skips as opinions, mixing
generations, or counting implementations that read each other each manufactures disagreement or
agreement that is not there. Two implementations give a diff; three give a verdict.

**Rejected:** majority vote over every report - the first run named one loader the dissenter
hundreds of times because it ran a different build.
