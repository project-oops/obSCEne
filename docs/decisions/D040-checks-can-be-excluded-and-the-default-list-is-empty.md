# D040 - Checks can be excluded, and the default list is empty

**Status:** decided
**Date:** 2026-09-26

`EXCLUDE="<id> ..."` skips named checks at build time; an entry with no `/` names a whole section.
An excluded check reports `skip` with the reason. The default list is empty, and section entries
are written by hand, never inferred by a sweep.

**Why:** a call that ends the process loses every check behind it, so a run needs a way past it.
The empty default makes the first run find the crash. Reporting the skip keeps the exclusion
visible, and `skip` ranking below `fail` makes excluding a failure read as a regression. A section
entry asserts that every check in it ends the process, which only a completed walk can support.

**Rejected:** omitting excluded checks from the report - hides them. Collapsing to a section
automatically after N exclusions - claims crashes for checks nothing has run.
