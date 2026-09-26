# D017 - The C runtime is a behavioural section

**Status:** decided
**Date:** 2026-09-26

`035-libc` and `037-math` check the platform's C library for behaviour: `calloc` zeroes, `qsort`
sorts and calls back into guest code, `snprintf` reports the length it would have written.
`memcpy`, `memset` and `memmove` are not checked, because the probe defines its own.

**Why:** a title imports more of the C library than of any vendor subsystem, and ISO C settles
every expectation, so these are positive checks with certain signatures. Checking the three
functions the probe defines would measure the probe.

**Rejected:** treating the guest as freestanding because the probe is - leaves the largest
imported surface unchecked.
