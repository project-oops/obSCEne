# D046 - An intermittent failure is reported with its rate

**Status:** decided
**Date:** 2026-09-26

Anything that looks intermittent is run repeatedly with `scripts/repeat.sh`, which reports the
denominator beside the count, warns on a small sample with no events, and names where each short
run died. A check that loops emits progress records.

**Why:** one crash is not a finding and one clean run is not a pass. A fault near one run in seven
is missed by four runs more often than found, and "died in the same call three times in twenty" is
a bug report where "crashed three times" is not.

**Rejected:** judging from single runs - wrong in both directions on the same afternoon.
