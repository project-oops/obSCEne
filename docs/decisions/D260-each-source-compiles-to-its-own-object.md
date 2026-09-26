# D260 - Each source compiles to its own object, keyed by a flags sentinel

**Status:** decided
**Date:** 2026-09-26

`host`, `module` and `eboot` compile each source to its own object and link the objects. Each
object depends on a per-target sentinel that is rewritten whenever the build's flag string
(`GEN`, `EXCLUDE`, `DISPLAY_MEM` and the rest) changes, forcing a full recompile on any flag
change.

**Why:** a monolithic compile recompiles everything on any edit and defeats a compiler cache. make
decides freshness from file times, not flags, so reusing an object built with a different flag
value would be a silently wrong binary - which costs a hardware cycle to notice.

**Rejected:** one compile-and-link per target - slow, and no incremental build. File targets
without a flags sentinel - reuses objects across incompatible flag values.
