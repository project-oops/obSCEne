# 2026-09-01 (Target Identification & Attachment Succeeded: PPSA02664 Injected, Hijacking Fix) (D302)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 1-116):
1. **Historic Milestone - Full Retail Game Injection Succeeded**:
   - `MATCHED RETAIL GAME BY TITLE ID: PPSA02664`
   - `path: /app0/eboot.bin, pid=297`
   - `PT_ATTACH ret=0, wait4 stop signal=17` -> **ptrace attach 100% success!**
   - Remote `mmap` allocated `0x6a0000` bytes at `0x21e540000` via remote syscall 477.
   - All 4 ELF segments mapped and protected with remote `mprotect` (sysno 74).
   - Entire 9.3 MB payload mapped into the retail game memory!
   - Remote `mmap` allocated payload args at `0x210034000`!
2. **Analysis of Thread Hijacking Glitch**:
   - At line 235 of `src/injector/injector.c`, a redundant second call to `krw_elevate_process(target_pid)` was executed immediately before hijacking the thread.
   - `krw_elevate_process()` indiscriminately cleared `P_TRACED` (`0x800`) and `P_STOPPED_TRACE` (`0x40000`) from the target's `p_flag`.
   - Stripping `P_TRACED` during an active ptrace session broke the debugger attachment, causing `procctl_setlong` (errno 57) and `procctl_setregs` (errno 1 `EPERM`) to fail.
3. **Remediation**:
   - **Remove Redundant Elevation (`src/injector/injector.c`)**: Removed `krw_elevate_process(target_pid)` at line 235 (credentials are already elevated at step 4 prior to attach).
   - **Preserve Active Ptrace State (`src/injector/krw.c`)**: Guarded `P_TRACED` and stopped flags in `krw_elevate_process()`, ensuring that if `p_flag & 0x800` is active, the ptrace attachment flags are strictly preserved.

Verified: `make payload injector HARDWARE=1` (`9,410,856` bytes) compiles 100% clean with zero warnings and zero errors.

