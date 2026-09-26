# D220 - The modules a title bundles are built here, never copied

**Status:** decided
**Date:** 2026-09-26

`/app0/sce_module/` holds stub modules built from `src/probe/sce_module.c` that load and do
nothing. They carry the file names a real package bundles (`SCE_MODULES`), because the loader looks
for those names. `sce-module-guard` f