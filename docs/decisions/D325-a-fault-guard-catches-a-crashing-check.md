# D325 - A fault guard catches a crashing check so the suite continues

**Status:** decided
**Date:** 2026-09-26

The harness installs a handler for SIGSEGV, SIGBUS, SIGILL and SIGFPE and arms a landing pad around
every check with the `OBS_FAULT_ARM` macro. A caught fault becomes a `crash` result (D004) and the
run continues; a dangling `try` still means the process went down before the guard could land. The
pad is keyed per thread, so a fault on a worker thread is caught. The guard carries its own
freestanding `setjmp`/`longjmp` and takes its signal primitives as imports, preferring the bound
address over `dlsym` (D326). Hangs are not caught - a call that never returns raises no signal.

**Why:** `obs_address_is_callable` cannot stop a fault inside a resolved function, and one such
fault took a whole run down. `sigsetjmp` must capture the check's own frame, so the arm is a macro.
A native title's `dlsym` sees only imported symbols, so the primitives cannot be dlsym-only.

**Rejected:** letting a fault end the run. A single main-thread pad - `longjmp` across stacks
corrupts. dlsym-resolved primitives - absent on a native eboot.
