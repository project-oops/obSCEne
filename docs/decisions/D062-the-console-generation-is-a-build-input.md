# D062 - The console generation is a build input; everything else is detected

**Status:** decided
**Date:** 2026-09-26

`TARGET` (`orbis`, `neo`, `prospero`, `trinity`; default `prospero`) selects the generation a
build declares, including `EI_ABIVERSION` and the dynamic-table convention. `GEN=4|5` is accepted
as an alias. Anything a platform answers at run time is detected at run time, from which symbols
and libraries resolve. One source tree produces as many binaries as the targets need.

**Why:** a loader reads the generation byte before any guest code runs, and loaders of the two
generations refuse each other's value, so it cannot be decided by the guest. Everything after load
can be.

**Rejected:** one constant for every loader - one generation's loaders refuse it. A separate
source per target - findings would stop being comparable.
