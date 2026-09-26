# D202 - A mined name a section calls is declared in that section

**Status:** decided
**Date:** 2026-09-26

A name from the mined corpus that a section calls is declared in that section's source file (with
its host stub prototype beside the stub), not in `platform.h`, and listed in `src/probe/imports.c`.

**Why:** the mined corpus has no exclusion list, so the name stays in the generated `corpus.h`, and
declaring it in `platform.h` conflicts in the one translation unit that sees both headers. The
census asks whether the name is there; the section asks what it does.

**Rejected:** declaring it in `platform.h` - does not build. Hand-editing the generated corpus - the
next regeneration undoes it.
