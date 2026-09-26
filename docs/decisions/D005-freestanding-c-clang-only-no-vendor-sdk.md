# D005 - Freestanding C, clang only, no vendor SDK

**Status:** decided
**Date:** 2026-09-26

The probe is freestanding C (`-ffreestanding -nostdlib`) built with clang, with no libc and no
vendor headers. Imports are ordinary undefined symbols the loader resolves. The compiler's own
`memcpy`, `memset` and `memmove` calls are satisfied by local definitions.

**Why:** provenance stays clean, the import list is an exact statement of what the program asks
the platform for, and the toolchain is one anybody has.

**Rejected:** a vendor or community SDK - its headers and stub lists carry provenance this
project cannot adopt. Linking a libc - the probe would measure its own library instead of the
platform's.
