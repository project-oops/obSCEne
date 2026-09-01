# 2026-09-01 (Orphaned Process Diagnostics: P_PPWAIT and Init Reparenting Recovery) (D303)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 36-50):
1. **Root Cause of Post-Crash Attach Failure**:
   - In Run #15, the target was stopped and mid-injection when the process control error occurred.
   - When the injector exited without completing `PT_DETACH`, FreeBSD reparented PID 297 to `init` (`target parent pid=1`).
   - The target's `p_flag` retained bit `0x1000` (`P_PPWAIT`: Parent is waiting for child to exec/exit), leaving it in a zombie/suspended state.
   - Subsequent `PT_ATTACH` requests returned `errno=37` (`EALREADY`) because the process was trapped in `P_PPWAIT` under `init`.
2. **Remediation**:
   - **Clear `P_PPWAIT` on Recovery (`src/injector/procctl.c`)**: Updated the recovery clear mask to `0x00061900` (`P_PPWAIT 0x1000`, `P_TRACED 0x800`, `P_STOPPED_TRACE 0x40000`, `P_STOPPED_SIG 0x20000`, `P_SUGID 0x100`).
   - **Process Refresh**: For an orphaned target from a previous aborted run, restarting the game on the PS5 home screen gives it a clean state with parent `SceShellCore`, allowing `PT_ATTACH` to succeed cleanly (as demonstrated in Run #15).

Verified: `make payload injector HARDWARE=1` (`9,410,856` bytes) compiles 100% clean with zero warnings and zero errors.

