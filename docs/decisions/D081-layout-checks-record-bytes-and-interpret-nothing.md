# D081 - Layout checks record bytes and interpret nothing

**Status:** decided
**Date:** 2026-09-26

`130-layout` calls a structure-filling function into an oversized buffer filled with a poison
pattern and guarded past its end, and dumps the bytes as `OBS|bytes`. A byte counts as written when
it changed from the poison. Fields are named by position, never by assumed meaning. The verdict is
weak on purpose; the dump is the result.

**Why:** dumping needs the arity, not the layout, so it reaches the struct-taking surface without
breaking D008. Poison separates a field written as zero from one never touched. A guard catches
the one fault the method could cause.

**Rejected:** zeroed buffers - hide written zeros. Named fields - a reader who later learns the
layout cannot correct a field already called `end`.
