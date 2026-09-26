# D070 - Documentation is checked against the tree

**Status:** decided
**Date:** 2026-09-26

`obscene-tool doccheck` resolves every `make`, `scripts/`, `obscene-tool` and `src/` reference
inside code spans, every decision number cited, and the listing in `docs/README.md`. A `make`
rule resolves against the root Makefile and the Makefile beside the document. Prose is not
matched.

**Why:** a named command that does not exist fails only when somebody types it. Matching bare
prose produces false failures, and a checker that cries wolf gets switched off.

**Rejected:** matching all prose - noisy. Requiring `make -C` everywhere - makes documents worse
to satisfy the gate.
