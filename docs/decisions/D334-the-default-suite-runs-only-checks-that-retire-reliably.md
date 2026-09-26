# D334 - The default suite runs only checks that retire reliably

**Status:** decided
**Date:** 2026-09-26

A GPU submit-and-fence check that can stall the queue is compiled in but gated behind
`-DOBS_RUN_WEDGING_GPU_CHECKS`, off by default; each names the run that found it. Every active GPU
submit check both guards on and latches the queue-fault flag, so one unknown wedger stops all
further submits rather than cascading into a system freeze.

**Why:** the suite is meant to be trusted and run to completion without leaving the console worse
than it found it. A check whose retirement is a coin-flip, or a known non-retiring one, does not
belong in the run people cite; a suite that fails intermittently teaches distrust. The latch is the
backstop that turns a system-kill into an isolated fault the console survives.

**Rejected:** shipping stalling checks in the default run. Scattered one-off skips instead of one
switch.
