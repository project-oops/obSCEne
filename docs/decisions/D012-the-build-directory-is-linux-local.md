# D012 - The build directory is overridable and Linux-local

**Status:** decided
**Date:** 2026-09-26

`BUILD` and `CARGO_TARGET_DIR` point at a Linux-local path (`$HOME/...`), never a Windows mount.
Every host-run step changes into `$(BUILD)` first, so files a run writes land there too.

**Why:** a Windows mount carries no execute bit, so a binary built into the tree cannot run, and
`symbols.txt` comes from running the host binary. Writing a report across the mount also costs
most of a host run's time.

**Rejected:** building in the repository tree - unrunnable output and slow writes.
