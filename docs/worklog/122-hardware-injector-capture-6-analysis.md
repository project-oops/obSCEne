# 2026-09-01 (hardware injector capture #6 analysis: errno diagnostics, thread ucred synchronization & root uid elevation) (D290)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` and `reports/hardware/injector-klog.txt.klog`):
1. **Target Identification & System Stability Validated**:
   - Title ID scanner successfully crawled `struct proc` in kernel memory, found candidate retail title `PPSA04263` at PID 204, and distinguished it from background candidates.
   - Embedded payload blob verified present and valid (`size: 9316792`).
   - Clean failure handling verified: On `PT_ATTACH` failure, `injector_exit(-6)` cleanly invoked `SYS_exit`, reporting `exit_value=fffffffa` with zero panics, zero coredumps, and zero fatal signals.
2. **Root Cause Analysis of `PT_ATTACH ret=-1`**:
   - **Per-Thread Cached Credentials (`td->td_ucred`)**: In FreeBSD, system calls execute in the context of the calling thread (`curthread`). While D289 updated `my_kproc->p_ucred`, the thread structure's cached credential pointer (`td->td_ucred`) was left pointing to the original `my_ucred` (inside `NPXS40112`'s VSH prison). When `sys_ptrace` invoked `p_candebug(td, target_p)`, `prison_check(td->td_ucred, target_p->p_ucred)` evaluated the two disjoint prisons and rejected attachment across the jail boundary (`ESRCH`).
   - **Unprivileged Process Tracing (`PRIV_DEBUG_UNPRIV`)**: Even when tracer and tracee share prison boundaries, FreeBSD's `cr_candebug()` enforces `priv_check_cred(cred, PRIV_DEBUG_UNPRIV)`. If the credential's user ID is not root (`cr_uid != 0`), unprivileged process debugging in a container jail is denied with `EPERM` (error 1).
   - **Missing `errno` Telemetry**: `sys_call` wraps the syscall trampoline. When a syscall fails, `libkernel` sets thread-local `errno` via `__error()` and returns `-1`. The exact POSIX error code was unlogged.
3. **Remediation**:
   - **Dynamic Thread Credential Scanning**: In `krw_swap_ucred()`, traverses `my_kproc->p_threads` (`my_kproc + 0x10`) to find the calling thread `td`, scans for any member matching `my_ucred` to dynamically identify `td_ucred`, and synchronizes it to `target_ucred`.
   - **Root UID Elevation**: In `krw_elevate_current_process()` and `krw_elevate_process()`, saves original `cr_uid`, `cr_ruid`, and `cr_svuid` and zeroes them out (`cr_uid = 0`), granting root privileges so `priv_check_cred(PRIV_DEBUG_UNPRIV)` succeeds without `EPERM`.
   - **`sys_get_errno()` Diagnostic Logging**: Resolved `libkernel`'s `__error()` in `src/common/syscall.c` and updated `procctl_attach()` to report `PT_ATTACH errno=` and `wait4 errno=`.
   - **Safe Restoration**: Both process and thread credentials, along with original UIDs, are cleanly restored on detach or early exit.

Verified: `make payload injector HARDWARE=1` (`9,394,216` bytes) compiles 100% clean with zero warnings and zero errors.

