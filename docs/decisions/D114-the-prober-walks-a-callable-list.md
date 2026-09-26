# D114 - The census walks every symbol; the prober walks a callable list

**Status:** decided
**Date:** 2026-09-26

The generated corpus carries a `fn` or `data` column per symbol and emits a callable twin of every
group. The census walks the full list; `910-bulk` walks only the callable one. Where sources
disagree, `data` wins.

**Why:** calling a data symbol jumps into a variable. Misclassifying a function as data costs one
skipped probe; the reverse costs the run.

**Rejected:** a kind field on every census row - widens a macro shape shared with the hand-written
census for one consumer.
