# D172 - Exclusions are learned at run time from the previous report

**Status:** decided
**Date:** 2026-09-26

At startup the probe reads its previous report before truncating it. A check whose `try` had no
`res` is watched and retried; if it fails to return again on the next consecutive run, it is
skipped with the reason "did not return on the previous run of this build". The skip set
accumulates through the report, carried on `OBS|resume|<skipped>|<ok|full>|<watched>...`. A report
from a different build or check count is ignored. The set is bounded and says when it is full.

**Why:** one binary then runs on every loader, and "the same binary behaves differently" stays a
measurement. The report is already durable before each risky call. Two consecutive sightings
separate a hang from a run that was killed mid-check.

**Rejected:** a build-time list per loader - makes each loader's report a different program.
Skipping on one sighting - one intermittent crash permanently removed thousands of measurements.
