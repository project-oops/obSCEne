# D121 - Console generation is observed, never asserted from presence

**Status:** decided
**Date:** 2026-09-26

`005-generation` and the HUD name a console generation only when exactly one generation's graphics
driver resolves. Both or neither is `unknown`. One inference in `generation.c` serves both
consumers. A build for the previous generation running on a current console reports its mode,
`ps4_mode`, rather than a generation it cannot see (D263).

**Why:** a loader that stub-resolves every import answers "present" for both drivers, so presence
proves nothing, and two inferences from different marker symbols can disagree in one report.

**Rejected:** naming a generation from any resolved marker - prints the wrong console on a
stub-everything loader.
