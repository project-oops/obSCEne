# D298 - A native title carries a process parameter block and compliant bundled modules

**Status:** decided
**Date:** 2026-09-26

A hardware build carries a `PT_SCE_PROCPARAM` segment whose `libc_param` points at a writable
structure (never null), because `libkernel` writes into it before the entry point. Bundled modules
in `/app0/sce_module` declare a `PT_SCE_MODULE_PARAM` segment with the matching SDK versions;
without it the loader refuses them. `048-selfaudit` reads and reports these on console.

**Why:** a null `libc_param` faults at address `0x28` before any instruction runs, and a bundled
module without the parameter segment fails module discovery. These were measured as hard
requirements, not options.

**Rejected:** a null or absent parameter block - startup faults.
