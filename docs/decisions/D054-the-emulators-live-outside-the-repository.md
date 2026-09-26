# D054 - The emulators live outside the repository, read for facts

**Status:** decided
**Date:** 2026-09-26

Emulator binaries and source checkouts live in `<emulators>`, outside the repository, and the run
scripts default there. They are read for facts - what a format is, what a symbol is called - and
none of their code enters this repository.

**Why:** hundreds of megabytes of third-party material has no place in the probe's history, and
several of the projects are GPL. The probe calls the platform's own interface and needs no
emulator code.

**Rejected:** vendoring the checkouts - bloats history and imports licences. A transient
scratch location - loses the toolkit.
