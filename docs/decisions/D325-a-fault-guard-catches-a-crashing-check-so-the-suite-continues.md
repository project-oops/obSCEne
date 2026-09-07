# D325 - a fault guard catches a crashing check, so the suite continues

**assumed** - 2026-09-07

`obs_address_is_callable` stops a jump to a bad address, but it cannot stop a fault *inside*
a resolved function. The platform's futex (`sceKernelSyncOnAddressWait`, `032-syncaddr`)
faulted inside libkernel with valid-looking arguments, and took the whole run down at
section 32 of 50 - every section behind it lost. The requirement is that a probe not crash
when a probe crashes: a faulting check should be caught, recorded, and the run continue.
This is the freestanding equivalent of exception handling, and this records how it is built
and what was chosen.

## The mechanism

`fault.c` installs a handler for the fault signals - SIGSEGV, SIGBUS, SIGILL, SIGFPE - and
the harness arms a landing pad around every `check->run()` with `OBS_FAULT_ARM`. On a fault
the handler `siglongjmp`s back to the pad, which reads as a `crash` result; the suite runs on.

Three things make it work where a naive version would not:

- **The primitives are the platform's own, resolved by name.** The target links no libc, so
  `sigsetjmp`, `siglongjmp` and `_sigaction` are resolved through `obs_module_open`/
  `obs_module_symbol` from libkernel - all exported by libraries a title already loads
  (`data/hardware/libkernel-vaddrs.txt`). The host uses libc directly, which is what lets
  the mechanism be proven on an ordinary machine. Where the primitives cannot be resolved,
  the guard is simply absent and a fault ends the run exactly as before - no new failure
  mode is introduced on a platform that cannot support it.

  > **Superseded in part by D326.** This resolution path held for the package loader but not
  > the native eboot, whose `sceKernelDlsym` resolves only symbols the process already imports
  > (and `setjmp`/`longjmp` are libSceLibcInternal's, not libkernel's). The guard now carries
  > its own freestanding `setjmp`/`longjmp` and takes `_sigaction`/`_sigprocmask`/`scePthreadExit`
  > as imports, preferring the bound address over dlsym; dlsym is only a fallback. That is what
  > lets the eboot leg be guarded too.

- **`OBS_FAULT_ARM` is a macro, not a function.** `sigsetjmp` captures the frame it is
  called in; wrapped in a function it would capture a frame that has already returned, and
  the `siglongjmp` would land in freed stack. So the arm happens at the check's own call
  site.

- **The landing pad is keyed by thread.** The futex runs its blocking wait on a worker
  (D322, so a *hang* cannot take the run down), and that is the thread that faults. A single
  main-thread pad could not catch it - `siglongjmp` across stacks is corruption. So the
  registry maps each thread to its own pad, the worker arms its own, and the handler lands
  on whichever thread raised the signal. `032-syncaddr`'s worker now catches the fault,
  records it, and the main thread reports the crash.

## What was chosen, and by whom

Two decisions were the report interface's owner's to make, and were put to them:

1. **A new `OBS_CRASH` status**, not a `fail` with a crash reason. A crash is the strongest
   finding a probe can make; folding it into `fail` would hide it in the counts and in a
   diff. This is a fifth status where D004 fixed four, and it changes the contract:
   `docs/OUTPUT.md` gains a `crash` row, the `tally`/`sectiontally` lines gain a trailing
   `crash` count (appended, so a pre-guard parser still reads the first four - Principle 3),
   and the Rust `Status` gains a `Crash` variant ordered below `Skip` so any check that
   *starts* crashing is the most severe regression.

2. **Faults only, not hangs.** A signal handler cannot catch a call that never returns and
   raises no signal; that needs a per-check watchdog, a larger change with its own failure
   modes (a call stuck inside a syscall cannot be reliably unwound). Hangs stay handled the
   way they were - written as a `try` form, or run on a thread nobody joins.

## Principle 1 gains its one exception

"A `try` with no matching `res` means the call did not return" was absolute. It now has
exactly one exception, and it is explicit: a fault the guard catches turns the `try` into a
`crash` `res`. A dangling `try` still means "did not return **and was not caught**" - the
loader took the process down before the guard could land - and a `crash` `res` means
"faulted, and the run went on". The two remain distinguishable, which is the reason to
record the crash rather than leave the record absent. `docs/OUTPUT.md` states this.

## What the host settles, and what only hardware can

`make host FAULT_SELFTEST=1` adds one check that dereferences a bad pointer. It is caught:
the run records `000-boot/fault-guard-self-test crash 0xb (SIGSEGV)` and completes to its
`end` record with 6027 records after the crash and a `tally` of `...|1` crash. That proves
the mechanism - handler, thread-keyed pad, `siglongjmp`, the `crash` status and its counts -
against a real signal.

What it cannot prove is the *target* path: the host uses libc, the target resolves libkernel
by name and relies on FreeBSD `siglongjmp` restoring the signal mask so a second fault is
caught after the first. That is why this is `assumed` until a hardware run shows
`032-syncaddr` reported as a crash with the sections behind it still running - the run that
will also, at last, complete the three-way sweep.
