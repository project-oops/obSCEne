# 2026-09-01 (hardware injector capture #10 analysis: credential decoupling & p_flag2 P2_NOTRACE/P2_PTRACEREQ clearing) (D297)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 367-385):
1. **P_TRACED Clearing Verified**:
   - `cleared target p_flag bits, new p_flag=0x10000000`
   - `target p_flag=0x10000000` (`P_INMEM`) confirmed `P_TRACED` was completely stripped from `p_flag`.
2. **Root Cause Analysis of Persistent `errno=37` (EALREADY)**:
   - **Credential Identity Collision**: `krw_swap_ucred()` was previously setting `my_kproc->p_ucred` and `td->td_ucred` to `target_ucred`. When `sys_ptrace(PT_ATTACH)` executed, the calling thread and the target process possessed the exact same `struct ucred` pointer in kernel memory. The kernel/ACMGR interprets this as the target attempting to attach to itself or an already-active session, returning `EALREADY` (37).
   - **`p_flag2` Flags (`struct proc + 0xB4`)**: Directly following `p_flag` at offset `0xB4` is `p_flag2`. On FreeBSD/Prospero:
     - `P2_NOTRACE` (`0x00000002`): Explicitly forbids `ptrace(2)` attachment and coredumps.
     - `P2_NOTRACE_EXEC` (`0x00000004`): Keeps no-trace across execve.
     - `P2_PTRACEREQ` (`0x00004000`): Active ptrace request lock in progress.
3. **Remediation**:
   - **Decouple Tracer and Target Credentials (`src/injector/krw.c`)**: `krw_swap_ucred()` no longer overwrites `my_kproc->p_ucred` or `td_ucred` with `target_ucred`. The injector retains its own elevated credentials (`SYSTEM_AUTHID`, all caps at `0x60`, root UID) while synchronizing `cr_prison` if container IDs differ.
   - **Clear `p_flag2` Security Bits (`src/injector/krw.c`, `src/injector/procctl.c`)**: Mask `0x00004006` (`P2_NOTRACE | P2_NOTRACE_EXEC | P2_PTRACEREQ`) is now cleared on `kproc + 0xB4` for both the injector process and the target process, with telemetry and retry recovery.

Verified: `make payload injector HARDWARE=1` (`9,410,720` bytes) compiles 100% clean with zero warnings and zero errors.

