# D184 - Hardware is registered by address; its capabilities are measured on every use

**Status:** decided
**Date:** 2026-09-26

`obscene-tool hw` stores a console's name and address only. `hw check` connects to every service
each time and says what each port is for and how long it took. `hw send` refuses a vendor-format
module before sending it. A payload's loader socket is read when present but is not treated as the
report channel.

**Why:** which services run depends on what was loaded since the last power cycle, so stored
capabilities go stale silently. A vendor module sent to a homebrew loader takes the loader down. A
title launched from the home screen has no loader socket.

**Rejected:** caching capabilities. Building on the loader socket as the report channel.
