# D094 - Emulators are built from the source that is read

**Status:** decided
**Date:** 2026-09-26

If a loader's behaviour is explained by reading its source, the binary run is built from that
source. A finding about a downloaded binary is not explained from a different commit.

**Why:** a binary and a clone at different revisions answer the same question twice with nothing
checking that they agree, and conclusions drawn that way were wrong.

**Rejected:** running release binaries and reading an unrelated checkout.
