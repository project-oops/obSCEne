# D314 - The decision gate checks the split log, and finding nothing fails

**Status:** decided
**Date:** 2026-09-26

`obscene-tool decisions` does not build the index - `tools/split-decisions.sh --index obscene`
does. It checks that the directory and the generated index describe the same set: every file has
exactly one row, every row links to a file, and no number is claimed twice. An empty decisions
directory is a failure, not a pass.

**Why:** two generators for one file is how a file ends up with two values. A duplicate number is a
silent failure - every citation still finds an entry, just not reliably the right one. A gate with
nothing to check has failed to run, not passed.

**Rejected:** the subcommand generating the index - a second generator. Passing on an empty
directory.
