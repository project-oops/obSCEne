# D053 - The census had been carrying a fragment of a symbol name, and now it cannot


Status: bug, found and fixed.

`X(FromAddr)` was in the generated census. It is the tail of
`sceKernelGetModuleInfoFromAddr`: when `sceKernelGetModuleInfo` was promoted to a real
check, it was removed from the census list by a match that did not respect word
boundaries, and it took the front off a longer name that happened to contain it.

The damage is quiet in both directions. `FromAddr` is not a symbol, so it was censused as
absent forever - a false negative, which the census header correctly says is harmless.
The real cost is the other half: `sceKernelGetModuleInfoFromAddr` *is* a symbol, and it
stopped being counted at all while the list still appeared to contain it.

**Nothing would have caught it.** The build succeeds, the census runs, the report is
well-formed, and the only signal is a name that does not look like a name - visible only
to somebody reading the generated header for another reason, which is how it was found.

`gen-surface.py` now refuses any name not shaped like a symbol: `sce` and a capital, or
two leading underscores, or a lowercase first letter. `FromAddr` matches none of the
three. The rule is crude and that is the point - it costs nothing and closes the only
route this failure has.

Worth noting what the guard is really for. The removal that caused this was done by hand
in a moment when the attention was on the promoted symbol, not on the list it was leaving.
That will happen again, and the fix is not to be more careful.

