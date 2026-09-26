# D316 - Porthole drives a virtual controller device

**Status:** decided
**Date:** 2026-09-26

Porthole feeds controller state through `libScePad`'s virtual-device calls, self-resolved via the
live kernel dispatch-table walk, registering a virtual pad per slot on first use. State is treated
as continuous: a record whose sequence number is at or below the last seen for its slot is
discarded, and slots are tracked independently.

**Why:** `scePadReadState` consumes controller state; the virtual-device path is the mechanism the
OS itself uses to provide it. Because the state is continuous, dropping stale records keeps network
jitter from replaying old positions.

**Rejected:** driving `scePadReadState` - it is a consumer, not a provider. Treating the stream as
discrete events - replays stale state on jitter.
