# 2026-09-08 - The title stops crashing at boot, and a fault guard keeps it running

The native title (pkg and eboot) had crashed on every launch since `e251612`. This session
found why, fixed it, captured the orbistoun-premise results the title could reach, and - on
the user's direction - added a fault guard so a crashing check no longer ends the run.
Documented in `docs/decisions/D324` and `D325`; `D323` is corrected by `D324`.

## The boot crash was `sys_call_init` on a title's entry (D324)

The fault was `rip = 0x2` before a single record, `rdi = 1`, `rsi` at the string `"getpid"`.
Symbolised (patching the mkmodule'd `e_type` back to `ET_DYN`, and relinking in the
Makefile's own object order so the addresses matched), it landed in `obscene_start` right
after `call sys_call_init` - code `e251612` added for the payload/injector path:

```c
const payload_args_t *pargs_init = obs_get_payload_args();
if (pargs_init != NULL) { sys_call_init(pargs_init); }
```

A title is entered with `rdi` pointing at the loader's own handoff struct, which is real,
mapped, aligned memory - so `obs_capture_payload_args`'s shape check passed and it was
accepted as a `payload_args`. Its first word, read as `sys_dynlib_dlsym`, was `0x2`;
`sys_call_init` called `dlsym(1, "getpid", …)` through it. `24036b6` (the Sep-4 good build)
never called `sys_call_init`, which is exactly why it ran.

The fix is in `kernelprobe.c`: accept the args as a payload only when `sys_dynlib_dlsym` is
callable, and drop `obs_get_payload_args`'s raw-pointer fallback that re-exposed the same
garbage. It restores that file's own stated invariant - "issues no primitive against a
struct that is not a payload_args" - at the source, so every payload-gated path is fixed at
once. **Every earlier theory this session was wrong** (unresolved-import `JUMP_SLOT` split,
reloc-table scale, `libScePosix` phantom) - a reminder that `rip = 0x2` is not always a bad
import; here it was a good pointer read from the wrong struct.

## What the title measured before it hit a check that crashed

With the boot fix the title ran the suite. On the ps4-BC pkg leg:

- **`016-syncbounds` (premise b): settled.** A bad handle returns **`0x80020003` (ESRCH)**
  for both `sceKernelPollSema` and `sceKernelPollEventFlag` - the two families agree,
  refuting orbistoun's `0x7fff0003` placeholder. The semaphore count is "how many to
  acquire" (`0x80020016` EINVAL for zero, `0x80020010` EAGAIN when too few); event-flag
  wait-mode `0x01` is AND, `0x02` is OR, `0x00` is EINVAL.
- **`019-posixerr` (premise a): not answerable here.** Both checks skip -
  `libScePosix` is absent in the ps4_game sandbox (all of `017-posix` skips too). Premise a
  needs the payload or a gen-5 native leg.
- **`018-relational`: a finding.** `sceKernelCreateSema` writes past the end of the `int`
  out-parameter it is given - the handle does not fit an `int`.

## The fault guard (D325), on the user's direction that a probe not crash a probe

Every `check->run()` is now wrapped in `OBS_FAULT_ARM`: a handler for SIGSEGV/SIGBUS/SIGILL/
SIGFPE (primitives resolved by name from libkernel/libSceLibcInternal) `siglongjmp`s back to
a per-thread landing pad, and the check is recorded as a new `crash` status while the suite
runs on. The futex worker (`032-syncaddr`) arms its own pad, since it faults on a thread the
main pad cannot reach. Confirmed on hardware: it caught the deliberate self-test crash at
`000-boot` **and** the futex crash at `032-syncaddr`, and the run continued.

Two things the first hardware run surfaced, both fixed:

- **A single service-thread fault was disarming the guard.** The handler's no-pad path
  restored `SIG_DFL`, which is process-global - so a platform-spawned thread (libScePad,
  video) faulting once left the guard off for every thread after, and a later main-thread
  crash went uncaught. It now `scePthreadExit`s just that thread instead.
- **Five checks read execute-only memory.** `media`, `audiodec` and `inputext` (all from
  `e251612`) dumped a resolved function's 256-byte prologue, guarding on
  `obs_address_is_callable` - but library text is execute-only on the console: callable, not
  readable. `obs_linkmap_readable` (made public for this) now gates each dump, so it runs on
  a loader that maps text readable and skips it on hardware.

## The report contract changed, deliberately

`OBS_CRASH` is a fifth status where D004 fixed four, chosen by the report's owner over
folding a crash into `fail`. `docs/OUTPUT.md` gains a `crash` row and a `guard` record; the
`tally`/`sectiontally` lines gain a trailing `crash` count (appended, so an old parser reads
the first four - Principle 3); the Rust `Status` gains `Crash`, ordered below `Skip` so any
check that starts crashing is the most severe regression. `make host FAULT_SELFTEST=1` is the
harness's own proof, and it is green.

## Surprises worth keeping

- Two concurrent readers of `klogsrv` get **zero** records each - it serves one at a time.
  A dual `report` + `hw logs` capture looked like "the title never launched" when it was
  running fine; the screen (drawn by the harness directly) was the honest witness. Capture
  with one reader.
- The console gets wedged ("resource temporarily unavailable", `0x80940031`) after a dozen
  rapid install/launch/crash cycles - settle between launches.
