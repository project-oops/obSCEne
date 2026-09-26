# D303 - A scalar out-parameter is poisoned, not zeroed

**Status:** decided
**Date:** 2026-09-26

A check that reports an out-parameter initialises it to `0xC7C7C7C7` (the project's poison byte)
rather than zero, so "untouched" is a visible answer and a real zero is a measurement.

**Why:** a field initialised to zero and reported as zero cannot be told from one the platform
never wrote; the probe would be reading back its own initialiser. This is `obs_report_written`'s
argument for buffers, applied to scalars. The poison is a fact about the probe, not about the
platform, so nothing is invented.

**Rejected:** zero-initialising - untouched and written-zero become indistinguishable.
