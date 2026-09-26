# D192 - Corroboration is counted per independent claim

**Status:** decided
**Date:** 2026-09-26

Agreement between implementations counts only between independent sources, judged per claim. A
new loader's code is diffed against the loaders already in the toolkit before its results count as
independent. A claim annotated with a published contract or a live capture counts; a bare constant
counts no more than this project's own; a claim derived from another project here does not count.

**Why:** these projects read each other. A copy of a loader and a claim citing another loader are
the same reading arriving twice, and summing them manufactures confidence.

**Rejected:** counting per project.
