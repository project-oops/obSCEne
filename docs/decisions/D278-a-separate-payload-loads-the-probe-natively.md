# D278 - A separate payload loads the probe into a native process

**Status:** decided
**Date:** 2026-09-26

`obscene-injector` is its own freestanding ELF that loads `obscene.elf` into a native-category
process, so the probe measures native behaviour rather than the compatibility sandbox. The probe
never links it and it never links the probe's checks; a shared freestanding layer (`src/common/`)
holds the string and memory helpers both use.

**Why:** running inside a native process is what reaches the current generation's graphics stack,
and keeping the two payloads separate keeps the probe free of the process-control machinery.

**Rejected:** building the loading into the probe - couples a measurement tool to a process-control
payload.
