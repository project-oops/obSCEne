# D200 - The file formats live in selfish; measurements stay here

**Status:** decided
**Date:** 2026-09-26

The import hash, ELF identity, vendor dynamic table, container, package, SDK table and linker
scripts are selfish's, and every vendor-format artifact is produced through it. This repository
keeps its own measurements and conventions: the name-to-library manifest, the `$` sigil, the mined
corpus and the census.

**Why:** a format shared by several projects needs one home, or a wrong constant can be introduced
in one of them alone. A measurement belongs with whatever measured it.

**Rejected:** a local copy of the format code.
