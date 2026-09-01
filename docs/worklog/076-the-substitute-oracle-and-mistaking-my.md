# 2026-08-26 - the substitute oracle, and mistaking my own comparison for a finding


Four loaders complete the full suite as of today, so `consensus` had four complete reports for
the first time. It reported 142 agreed, 360 disagreed, and named PS5PCEM as the lone dissenter
**328 times out of 358**.

That is a damning-looking result about one project and it was an artefact of my comparison. The
sweep builds a module per loader generation: PS5PCEM ran GEN=5 and the other three ran GEN=4.
The reports say so in their own `sysinfo` line. Within a generation, same tool, same day:

| cohort | agreed | disagreed |
|---|---|---|
| mixed generations, 4 loaders | 142 | 360 |
| **same generation, 3 loaders** | **469** | **32** |

The outliers rebalance too - Kyty 15, shadPS4 8, fpPS4 5, where the mixed run had put 92% of
them on one loader. Almost every disagreement was the comparison, and it pointed hard and
specifically at a project that had done nothing wrong.

The real finding is what that leaves: the working oracle is **three previous-generation
emulators**, and for the generation this project targets there is no cohort at all. Agreement
is currently evidence about the previous platform. (D197)

### The report corroborates itself

`007-responsive` and `035-libc` reach the same functions by different routes. Over five string
functions and three loaders they agree on every case - including fpPS4's `strstr`, which
responds and only partially passes. Implemented and wrong, which is the distinction the two
sections exist to draw and needs different work from an absence.

Kyty is silent and failing on all five: it implements essentially no libc string functions,
which is what its fifteen outliers are. The patch did not cause that, it made it visible.

