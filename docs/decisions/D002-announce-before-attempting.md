# D002 - Announce before attempting

**decided** · 2026-08-19

Every check writes its identity, library and symbol **before** calling into the
platform, and the write is unbuffered.

Under an emulator the normal outcome of an unimplemented function is a hard crash
that takes the process down. When that happens the stream stops, and the last line
names the exact call responsible. A report ending on a `try` record is not a
truncated report - it is a one-frame stack trace that needs no debugger, no symbols
and no cooperation from the thing that crashed.

This is the most important property of the program. Everything else is arranged to
keep it true: no buffering, no batching, and a skipped check deliberately emits no
announcement, because a `try` with no result must mean exactly one thing.

