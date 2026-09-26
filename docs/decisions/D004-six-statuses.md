# D004 - Six statuses

**Status:** decided
**Date:** 2026-09-26

A check result is one of `pass`, `partial`, `fail`, `skip`, `crash` or `pending`. New statuses are
appended to the tally lines as trailing fields, so an older parser still reads the first four.

- `partial` - the call works in part; without it an implementation returning zero for
  everything would look perfect.
- `skip` - nothing was learned; a failed prerequisite does not cascade into forty failures.
- `crash` - the call faulted and the fault guard recovered the run (D325).
- `pending` - the check applies but its input (a peripheral, a button press) was absent (D328).

**Why:** each status names a different piece of work for the reader. Folding any of them into
`fail` hides it in the counts and in a diff.

**Rejected:** red, amber and green only - a missing prerequisite would bury the one real failure
under the checks that depend on it.
