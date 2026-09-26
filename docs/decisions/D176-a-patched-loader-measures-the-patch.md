# D176 - A patched loader's report never occupies that loader's row

**Status:** assumed
**Date:** 2026-09-26

A loader may be patched locally to let the probe run. The patch is kept in `patches/`, applied by
nothing, and the report from the patched build is stored and labelled separately; it never
occupies the loader's row in `docs/COMPATIBILITY.md`.

**Why:** a patched build is useful for finding bugs, but its report describes a build only one
machine has. Presenting it as the loader's result is an invented fact at a larger scale.

**Rejected:** refusing to patch - loses real findings. Reporting patched results under the stock
name.
