# D327 - The payload zeroes its own .bss

**Status:** decided
**Date:** 2026-09-26

The plain-ELF payload (`-DOBS_ZERO_BSS`, set on the `payload` target) walks its own program
headers at entry and zeroes the `p_memsz`-beyond-`p_filesz` tail of each `PT_LOAD` before any
zero-initialised static is read. The eboot and module leave this to their loader.

**Why:** elfldr maps segments but does not zero the .bss tail, so every zero-init static came up as
page garbage and the program assumed zero everywhere. The system loader zeroes it for the eboot, so
this is the plain-ELF path's problem alone.

**Rejected:** assuming the loader zeroed .bss. Reapplying relocations - relocation was never the
fault.
