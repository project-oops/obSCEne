# D001 - A host build beside the target build

**Status:** decided
**Date:** 2026-09-26

One source tree builds both the target shapes and `make host`, a native binary with every
platform function stubbed or backed by the host C library. The host build is required and kept
building.

**Why:** without it the first place the harness runs is inside an unfinished emulator, where a
harness bug and a platform bug look the same. On the host the expected outcome is known, so the
harness is falsifiable on its own, and a check that has never passed a known-good implementation
is not evidence.

**Rejected:** target builds only - leaves the harness untestable except against the thing it
measures.
