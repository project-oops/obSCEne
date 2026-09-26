# D071 - The orchestration scripts are `sh`

**Status:** decided
**Date:** 2026-09-26

Everything in `scripts/` is POSIX shell run from Git Bash or WSL. A script that hands a Linux
path or a `/`-flag to a Windows program sets `MSYS_NO_PATHCONV=1`. A Windows program's output is
captured with `$(...)` and never piped into a reader that can exit early.

**Why:** the language that drives a process has nothing to do with the platform the process
targets. PowerShell turned a native command's stderr into a terminating error. Git Bash rewrites
path-shaped arguments, and a Windows program has no SIGPIPE, so an early-exit reader blocks it.

**Rejected:** PowerShell for running and `sh` for building - two languages solving one environment
variable.
