# D199 - Scripts reach WSL through one shim

**Status:** decided
**Date:** 2026-09-26

Scripts that run a step inside WSL go through `scripts/wsl.sh`, which translates a small set of
command shapes and refuses anything else. Paths meant for the inner shell are escaped so WSL, not
Git Bash, expands them, and calls set `MSYS_NO_PATHCONV=1`. Call sites test for the file they need
rather than trusting an exit code.

**Why:** one translation point is one chance to get it wrong instead of dozens. An unknown verb
that is approximated does something nearly right silently.

**Rejected:** rewriting every call site directly.
