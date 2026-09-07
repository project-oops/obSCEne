# 2026-09-08 - The eboot leg completes, and the sweep stops hanging

The fault guard (D325) caught crashes on the package but not the native eboot: its record read
`OBS|guard|off|no setjmp/longjmp export`, and one late uncaught crash truncated the whole native
run before `OBS|end`. This session found why the guard could not arm on a native title, made it
arm, and fixed a sweep-teardown bug the working guard then exposed. Documented in `D326`; `D325`
is corrected in part.

## Why the guard was off on the eboot

The three-way sweep completes on the package (1927 records, `OBS|end`, one caught crash) but the
eboot reached 1441 records and stopped one `try` short, at `166-agc/dcb-reset-queue`
(`sceAgcDcbResetQueue`, SIGBUS) in the last section - the guard never armed to catch it.

Adding a resolution bitmap to the guard record turned the guess into a fact. The first eboot run
said `no setjmp/longjmp export`; with a freestanding setjmp added it said
`no sigaction export sa=0 spm=0 pex=0`. So *nothing* the guard needed resolved. The reason is
that **a native title's `sceKernelDlsym` resolves only symbols the process already imports.** The
eboot's own output proves it: `sceKernelDebugOutText`/`sceKernelWrite` are imports, the loader
binds them, 1441 records reach klog - while dlsym returns ESRCH for anything un-imported. The
guard primitives were dlsym-only, so on the eboot they were nothing.

## The fix

Two parts (D326):

- **setjmp/longjmp are ours.** `fault.c` carries `obs_local_setjmp`/`obs_local_longjmp`, a
  freestanding x86-64 SysV save/restore in a module-level `__asm__` block. The guard no longer
  needs libc to export setjmp; being our own defined symbol, the call is also a link-time
  PC-relative branch, free of the GLOB_DAT/JUMP_SLOT split D323 warns about.
- **The signal primitives are imports.** `_sigaction`, `_sigprocmask`, `scePthreadExit` go in
  `platform.h` + `imports.c` (all real libkernel exports per `data/hardware/libkernel-vaddrs.txt`
  / `ps5-full.txt`); `obs_fault_init` prefers the bound import address (`obs_fault_pick`,
  callable-checked) over dlsym, which stays a fallback.

Hardware, on the eboot: `OBS|guard|on|installed (local setjmp) sa=1 spm=1 pex=1`, the
`dcb-reset-queue` SIGBUS caught as `crash` and recovered, `OBS|tally|162|10|33|58|1`, `OBS|end`,
1805 records. The eboot leg now completes the way the package leg does. `make check` stays green.

## The working guard exposed a sweep hang

A guarded eboot stops on `OBS|end` in under a minute instead of running to the window. That hit a
latent bug in `scripts/sweep.sh`: `poll_and_stop` killed the klog reader and runner mid-stream,
but they were `( cmd | tr )` subshells - `kill -9` orphaned the tool and the `tr`, the orphaned
`tr` held the tee pipe open (the sweep hung before writing `.obs.log`), and an orphaned `hw logs`
would keep klogsrv's single reader slot and starve the next leg. Each leg now runs the reader and
runner as bare tool processes writing to files, so `$!` is the tool's own PID and `kill -9` reaps
it. Earlier sweeps never hit this because their eboot leg *timed out*, so the reader exited on its
own.

## Surprise worth keeping

The guard record's own claim was self-contradictory before the bitmap: `on|not initialised` on
the payload, where `s_available` (BSS, zero then set) read correctly but `s_detail` (initialised
`.data`) read its initial value - the same data-relocation corruption that garbles the payload's
`OBS|sink` string and SIGBUSes it at the first check. The eboot's guard is fixed; the payload's
early crash is a separate data-relocation problem in the elfldr plain-ELF shape, taken up next.
