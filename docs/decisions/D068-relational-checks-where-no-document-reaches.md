# D068 - Relational checks where no document reaches

**Status:** decided
**Date:** 2026-09-26

`018-relational` checks properties between calls rather than values: two live objects do not
share a handle, a released allocation can be allocated again, a counting semaphore refuses a
claim past its count, a clock never goes backwards, two opens of one path give two descriptors.

**Why:** the vendor surface has no oracle, and a relation needs no structure layout and no
documented error code. It checks behaviour where value checks cannot.

**Rejected:** waiting for layouts or hardware before checking the vendor surface.
