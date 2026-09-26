# D002 - Announce before attempting

**Status:** decided
**Date:** 2026-09-26

Every check writes its identity, library and symbol as a `try` record, unbuffered, before it
calls the platform. A skipped check emits no `try`. A `try` with no matching `res` means the call
did not return and was not caught by the fault guard (D325).

**Why:** the usual outcome of an unimplemented function under an emulator is a crash that takes
the process down. With the announcement first, the last line of the stream names the call
responsible, with no debugger and no cooperation from the thing that crashed.

**Rejected:** buffered or batched output - loses exactly the record that names the crash.
Announcing checks that will not run - makes a dangling `try` ambiguous.
