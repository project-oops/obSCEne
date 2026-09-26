# D139 - The HUD shows a value or `unknown`, and is screen-only

**Status:** decided
**Date:** 2026-09-26

The status line shows each system fact (network, firmware, generation, memory, disk and others) as
a value read through a confirmed signature, as present-but-unwired, or as `unknown`. The fields are
never written into the graded report as machine provenance. A build with no UI (`OBS_NO_UI`, the
payload) gathers none of them.

**Why:** an `unknown` field is a finding about the platform; a fabricated one is the failure the
project exists to expose. The probe cannot certify its own machine (D108). Gathering status in a
headless payload touches subsystems it has not set up.

**Rejected:** placeholder numbers. Guessed structure reads for fields without a confirmed
signature.
