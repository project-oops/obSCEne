# D049 - `crack`: a hit is proof and a miss is nothing

**Status:** decided
**Date:** 2026-09-26

`obscene-tool crack` recovers names by hashing candidates against identifiers. A match is
reported as certain; a non-match is reported only as "not in the candidate list", with the number
of candidates tried. `--known` measures how many established pairs the candidate generator
reproduces before its misses are weighed.

**Why:** the hash is one way, so a miss says nothing about whether a name exists. A generator that
cannot regenerate known names is not ready to be believed about unknown ones.

**Rejected:** reporting misses as absences - turns "we did not guess it" into "it is not there".
