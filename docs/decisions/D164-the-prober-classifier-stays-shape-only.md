# D164 - The prober's classifier keeps emulator knowledge out

**Status:** decided
**Date:** 2026-09-26

`910-bulk` classifies returns as `zero`, `rejected` (facility `0x8002`), `error-shaped` or `value`,
and every record carries the full 64-bit return. No table of vendor error facilities is added to the
probe.

**Why:** the facility scheme was learned from an emulator, and classifying by it would assert on
hardware something measured elsewhere. Widening `rejected` would change a documented meaning. The
full value in each record lets any reader decode facilities.

**Rejected:** a facility table in the probe.
