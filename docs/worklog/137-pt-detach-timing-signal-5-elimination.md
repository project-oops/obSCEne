# 2026-09-01 (PT_DETACH Timing & Signal 5 Elimination: Post-Detach Credential Drop) (D305)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` and `.klog` lines 105-116):
1. **Diagnosis of Game Exit on Signal 5**:
   - `registers set, resuming target...`
   - `PT_DETACH ret=-1, PT_DETACH errno=32`
   - `# process pid=329, payload.elf calls exit() exit_value=0.`
   - `<6>pid 327 (eboot.bin), uid 1: exited on signal 5`
   - Telemetry showed `PT_DETACH` failed with `errno=32` (`EPIPE` / error).
   - In FreeBSD, when a debugger process terminates without cleanly detaching from a traced target, the kernel delivers `SIGTRAP` (signal 5) to the traced child, causing the retail game to close and return to the home screen.
2. **Root Cause**:
   - `krw_restore_current_process()` was executed *before* `procctl_detach(target_pid, 0)`.
   - Dropping the injector's elevated capabilities (`authid=0x4800000000010003` and full caps) turned the injector back into an unprivileged WebKit process *before* detaching, causing `proc_can_ptrace()` / `p_candebug()` to reject the `PT_DETACH` call.
   - In addition, `PT_DETACH` requires `addr = (void *)1` per FreeBSD `ptrace(2)` specification rather than `NULL`.
3. **Remediation**:
   - **Post-Detach Credential Restoration (`src/injector/injector.c`)**: `procctl_detach()` is now called while the injector remains fully elevated with root/system credentials. `krw_restore_current_process()` is called only after `PT_DETACH` completes.
   - **PT_DETACH Argument Conformance (`src/injector/procctl.c`)**: Updated `procctl_detach()` to pass `(void *)1` per FreeBSD specification.
   - **Stack Alignment (`src/injector/injector.c`)**: Aligned hijacked `RSP` so `(RSP % 16) == 8` on function entry, satisfying the SysV AMD64 ABI and preventing vector instruction `#GP` alignment faults.

Verified: `make payload injector HARDWARE=1` (`9,410,856` bytes) compiles 100% clean with zero warnings and zero errors.

