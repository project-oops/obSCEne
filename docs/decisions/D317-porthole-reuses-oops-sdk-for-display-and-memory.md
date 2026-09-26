# D317 - Porthole reuses oops-sdk for display and memory

**Status:** decided
**Date:** 2026-09-26

Porthole links `liboops` and uses `oops-sdk` for display management, direct-memory allocation and
tiling rather than writing its own. Encoded video goes out over one socket in Annex-B H.264 and
controller state comes in over another.

**Why:** the display and memory foundation already exists in oops-sdk, tested on the same hardware;
reimplementing allocators or register writes in Porthole would duplicate it.

**Rejected:** a separate allocator and display path in Porthole.
