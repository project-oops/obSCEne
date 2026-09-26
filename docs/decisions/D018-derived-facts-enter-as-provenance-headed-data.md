# D018 - Derived facts enter the repository as provenance-headed data

**Status:** decided
**Date:** 2026-09-26

Names, identifiers and other facts derived from outside material enter the repository only as
text under `data/`, with a header naming exactly what they came from. The inputs (emulator
checkouts, extracted firmware trees) stay outside the repository and no build reads them. A
generator reads the data file, never the inputs.

**Why:** the header makes the derivation checkable and re-runnable by anyone holding the same
inputs. Keeping inputs out of the build keeps code from being shaped by reading a binary, which
is the thing the provenance rule is about.

**Rejected:** reading inputs at build time - makes the build depend on material that cannot be
published. Refusing all derived facts - leaves the census unable to name most of the surface.
