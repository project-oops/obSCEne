# D328 - A pending status for a check that is waiting for its input

**Status:** decided
**Date:** 2026-09-26

A peripheral or lifecycle check that can run but was not given its input - no controller attached,
no button pressed - reports `pending` (D004), not `skip` or `fail`. It never blocks: it samples its
window, finds nothing, and reports that it is still waiting. A `peripherals` record says once per
run what was attached, so a pending reads as "nothing plugged in" rather than a defect.

**Why:** `skip` says the check does not apply and does not count against coverage the same way; a
check an input away from an answer is a different thing, worth seeing apart and worth re-running
with the peripheral attached.

**Rejected:** reporting a missing peripheral as skip or fail - conflates "does not apply" with
"needs input".
