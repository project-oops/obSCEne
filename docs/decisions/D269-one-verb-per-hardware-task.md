# D269 - One verb per hardware task, on `bin/obscene`

**Status:** decided
**Date:** 2026-09-26

Every hardware task is a verb on `bin/obscene` (`deploy`, `report`, `recover`, `payload`, `native`,
and the rest); the scripts under `scripts/` are its implementation and are not run directly. The
report is captured off the system log by an `obscene-tool` subcommand, not pulled from disk.

**Why:** a task with no single entry point grows a new script each session, and the scripts drift.
A packaged run's report file is sealed `0600` inside the title's sandbox and is torn down when the
title exits, so the system log is the one channel that leaves the sandbox as the run proceeds.

**Rejected:** invoking scripts by hand. Pulling the report file over FTP - the file is not
readable by another user and does not outlive the process.
