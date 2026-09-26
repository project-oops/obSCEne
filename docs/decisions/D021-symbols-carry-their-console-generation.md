# D021 - Symbols carry their console generation

**Status:** decided
**Date:** 2026-09-26

Every census group declares `OBS_SHARED`, `OBS_PREVIOUS`, `OBS_CURRENT` or
`OBS_AVAILABILITY_UNKNOWN`, and the `sym` record carries it. A library belonging to the other
generation scores `skip`, not `fail`, when wholly absent.

**Why:** "absent" otherwise means two incompatible things - correctly missing on this generation,
or missing work - and summing them makes the coverage figure meaningless. `unknown` exists so an
uncertain symbol is not silently reclassified as an expected absence.

**Rejected:** one undifferentiated census - buries real gaps under correct absences.
