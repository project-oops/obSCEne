# D090 - The compatibility table is generated and ranks nothing

**Status:** decided
**Date:** 2026-09-26

`docs/COMPATIBILITY.md` is rendered from loader reports by `obscene-tool compat` and gated for
drift. It keeps per-section tallies, marks a census whose control failed as void, and does not
rank loaders. Hardware results are not a column; the document points to `docs/HARDWARE.md`.

**Why:** what each loader did changes with every suite or emulator update, so a hand-written table
is stale at once. A pass count is not a quality score. The console is the reference the loaders
are measured toward, not one of them.

**Rejected:** a hand-maintained table - drifts. A hardware column - couples a gated generated table
to a hand-captured run.
