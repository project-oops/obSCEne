# D326 - the guard carries its own setjmp and imports its signal primitives, so a native title is guarded

**assumed** - 2026-09-08

The fault guard (D325) worked on the package - it caught the futex crash and the suite ran on.
On the native eboot it did not: the guard record read `OBS|guard|off|no setjmp/longjmp export`,
so the one late uncaught crash - `166-agc/dcb-reset-queue` (`sceAgcDcbResetQueue`, SIGBUS), in
the *last* section of the run - took every record behind `OBS|end` with it. The eboot reached
1441 records and stopped one `try` short of finishing. This records why the guard could not arm
there and what makes it arm now.

## Why the guard could not arm on a native title

D325 resolved every guard primitive - `sigsetjmp`, `siglongjmp`, `_sigaction`,
`scePthreadSelf` - by name through `obs_module_symbol`, i.e. the loader's `sceKernelDlsym`.
That works on the package's loader. It does **not** on a native eboot, and the reason is
specific: **a native title's `sceKernelDlsym` resolves only the symbols the process already
imports.** The eboot's own output proves it - `sceKernelDebugOutText` and `sceKernelWrite` are
in `imports.c`, the loader binds them, and 1441 records reach klog through them - while
`sceKernelDlsym` returns `0x80020003` (ESRCH) for anything the eboot did not import
(`060-module/dlsym-resolves-known-symbol` fails, and every `OBS|module|...|0x0` shows the base
never resolved). The guard primitives were dlsym-only, so on the eboot they resolved to nothing:
the run's own bitmap said `sa=0 spm=0 pex=0`, and `setjmp`/`longjmp` (which live in
libSceLibcInternal, not libkernel as D325 stated) were absent too.

So the guard needs its primitives the way every *working* call on the eboot has them: bound as
imports, or carried in our own code - not fished out of a dlsym that cannot see them.

## The fix, in two parts

**setjmp/longjmp are our own.** `fault.c` carries `obs_local_setjmp`/`obs_local_longjmp`, a
freestanding x86-64 SysV save/restore (rbx, rbp, r12-r15, rsp, the return address - the whole of
what a non-local jump needs) in a module-level `__asm__` block. The guard no longer depends on
libc exporting setjmp at all. Two properties matter beyond "it links": it is our own *defined*
symbol, so the call is a link-time PC-relative branch and carries none of the GLOB_DAT/JUMP_SLOT
split that leaves a native title's *imports* at the `0x2` sentinel (D323); and it does not save
the signal mask, which is fine because the handler already unblocks the fault signal with
`sigprocmask` before jumping, exactly as it did for the resolved plain-`setjmp` fallback. This is
our own code implementing a published ABI, the same footing as any `runtime.c` helper
(Principle 8), not a borrowed declaration.

**The signal primitives are imports.** `_sigaction`, `_sigprocmask` and `scePthreadExit` are
declared in `platform.h` and listed in `imports.c`, so the native loader binds them like
`sceAgc*` - they are real libkernel exports (`data/hardware/libkernel-vaddrs.txt`: `_sigaction`
0xd100, `_sigprocmask` 0xcf70; `scePthreadExit` in `ps5-full.txt`). `obs_fault_init` now prefers
the bound import address (`obs_fault_pick`, callable-checked) and keeps dlsym and the POSIX
spellings as fallbacks. The callable check is what keeps this safe where D323 warns: it rejects a
weak-unbound `0` and the `0x2` unresolved sentinel and falls through to dlsym, and by the time
`obs_fault_init` runs (the first thing in `obs_run_all`) the loader has bound the imports every
check then calls, so the address is real.

The guard record now carries the resolution bitmap - `sa=`/`spm=`/`pex=` and which setjmp path
won - so a run that is unguarded says *which* primitive was missing rather than only that it was.

## What the hardware showed

`OBS|guard|on|installed (local setjmp) sa=1 spm=1 pex=1`; the crash that used to truncate the run
became `OBS|res|166-agc/dcb-reset-queue|crash|0xa|the call faulted (SIGBUS) and the run was
recovered`; `OBS|tally|162|10|33|58|1` and `OBS|end` followed. 1805 records, the eboot leg
completing the same way the package leg does. The freestanding longjmp-out-of-handler, which the
package had never exercised (it resolved libc's `sigsetjmp`), works on real hardware.

## Supersedes part of D325

D325's account of target resolution - "`sigsetjmp`, `siglongjmp` and `_sigaction` ... resolved
through `obs_module_open`/`obs_module_symbol` from libkernel" - held only for the package loader.
A native title's dlsym cannot see un-imported symbols, and setjmp/longjmp are not libkernel's.
The guard's primitives are now imported or carried, not dlsym-resolved; dlsym remains only a
fallback. The scope of D325 (faults not hangs, one pad per armed thread) is unchanged.

## Also here: the sweep no longer hangs when a leg finishes fast

A guarded eboot now stops on `OBS|end` in under a minute instead of dying at the window. That hit
a latent bug in `scripts/sweep.sh`: `poll_and_stop` killed the klog reader and the runner
mid-stream, but they were `( cmd | tr )` subshells - `kill -9` on the subshell orphaned the tool
and the `tr`, and the orphaned `tr` held the tee pipe open so the sweep hung before writing the
`.obs.log`, while an orphaned `hw logs` would keep klogsrv's single reader slot and starve the
next leg. Each leg now runs the reader and runner as bare tool processes writing to files, so
`$!` is the tool's own PID and `kill -9` reaps it - no pipe on either, nothing to orphan.
