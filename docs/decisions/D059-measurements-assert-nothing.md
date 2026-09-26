# D059 - Measurements assert nothing

**Status:** decided
**Date:** 2026-09-26

`OBS|measure`, `OBS|bytes` and `OBS|err` records carry a quantity, a value and a unit, and no
verdict. A check may add a loose assertion marked `assumed` beside the measured value.

**Why:** a duration or a returned code is a fact that needs no expectation and does not go stale
when a guess is corrected. With the number beside the verdict, correcting an assumption from
hardware is reading a figure, not re-deriving the expectation.

**Rejected:** only judged records - every uncertain behaviour needs an invented threshold.
