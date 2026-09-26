# D180 - Each loading mechanism is its own artifact

**Status:** decided
**Date:** 2026-09-26

The probe ships one artifact per loading mechanism: a plain ELF for a homebrew loader, a vendor ELF
for emulators, an eboot for the system loader, a package for the installer, and a native title
directory. Each has its own CI job.

**Why:** how a program was loaded is part of what there is to measure. Only the system loader
answers questions about the system loader; a homebrew loader or an emulator measures itself. One
combined job would hide which mechanism broke.

**Rejected:** treating loading as packaging with one shipped shape.
