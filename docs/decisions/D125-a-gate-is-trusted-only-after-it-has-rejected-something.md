# D125 - A gate is trusted only after it has rejected something

**Status:** decided
**Date:** 2026-09-26

Every gate is shown to fail on a deliberate defect before it is believed, and the defect case is a
test where possible (the protocol checker's mutation self-test runs before the checker). No filter
sits between a command and its exit status in a gate script. A gate that finds nothing to check
fails rather than passes.

**Why:** gates here have reported success four ways regardless of outcome, printed `clean` when the
linter had not run, and checked a file shape the repository no longer had. A gate that cannot fail
is reported as evidence while being none.

**Rejected:** trusting a green gate that has never been seen red.
