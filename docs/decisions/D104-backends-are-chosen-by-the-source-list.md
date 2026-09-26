# D104 - Platform backends are chosen by the source list

**Status:** decided
**Date:** 2026-09-26

Where the host and the target need different platform calls (the file sink, the socket, the GPU),
the shared logic lives in one file and each backend in its own (`sink_host.c`, `sink_target.c`),
and the Makefile compiles exactly one.

**Why:** an untaken `#if` branch is never parsed and rots. A source list says in one place which
backend a target gets, and lets the host stub stay honest for the checks while the backend under
it does real work.

**Rejected:** preprocessor branches inside one file.
