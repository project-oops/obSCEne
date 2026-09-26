# D235 - An unresolved symbol has three explanations, and the report names which

**Status:** decided
**Date:** 2026-09-26

A check whose symbol is null does not report "not present on this platform". It distinguishes:
the symbol is genuinely absent; the symbol exists in its library but this build did not link it;
or the loader resolves nothing by name and cannot be asked. `061-imports` reports each import on
two axes - whether the loader bound it, and whether a run-time lookup finds the same name - so a
this-project defect is told from a platform gap. A run-level `resolution` record says when the
enumeration could not look at all. Deciding which explanation holds is left to the census, which
loads modules where that is its job, not to the skip path, which does nothing.

**Why:** a title is given fewer libraries than it asks for, so a link gap read as a platform gap
becomes a false finding about the console. `obs_address_is_callable` guards a call and cannot say
why an address is null.

**Rejected:** reporting a null symbol as a platform absence. Resolving a library inside the inert
skip path - gives the one path that should do nothing a side effect.
