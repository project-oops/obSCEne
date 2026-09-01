# 2026-09-01 (hardware injector capture #9 analysis: EPERM eradicated, P_TRACED 0x800 clearing for EALREADY 37) (D296)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 360-381):
1. **Capability Elevation and Security Bypass Verified**:
   - Telemetry confirmed capability bitmasks are successfully populated in kernel memory:
     - `target elevated caps[0]=0xffffffffffffffff`
     - `target elevated caps[1]=0xffffffffffffffff`
     - `target elevated attr[3]=0x80`
   - **`EPERM` (error 1) is completely gone.** All Sony MAC security and FreeBSD `p_candebug()` credential checks passed.
2. **Root Cause Analysis of `PT_ATTACH errno=37` (EALREADY)**:
   - `PT_ATTACH` returned `errno=37` (`EALREADY`).
   - Line 377 revealed `target p_flag = 0x10060800`.
   - In FreeBSD `sys/proc.h`, bit `0x00000800` is **`P_TRACED`** (`Debugged process being traced`).
   - In FreeBSD kernel `sys_ptrace(PT_ATTACH)`:
     ```c
     if (p->p_flag & P_TRACED) return (EALREADY);
     ```
   - The retail game already had `P_TRACED` (`0x800`) set in its `p_flag` (maintained by system supervisor/crash handling). FreeBSD `sys_ptrace` strictly rejects attaching to a process that is flagged as already traced.
3. **Remediation**:
   - **Clear `P_TRACED` and Trace Flags (`src/injector/krw.c`)**: In `krw_elevate_process(target_pid)`, mask `0x00060900` (`P_TRACED 0x800`, `P_STOPPED_TRACE 0x40000`, `P_STOPPED_SIG 0x20000`, `P_SUGID 0x100`) is now cleared from `target_kproc->p_flag`.
   - **`EALREADY` Automatic Recovery (`src/injector/procctl.c`)**: `procctl_attach()` now detects `errno == 37`, dynamically strips `P_TRACED` and retry-attaches.

Verified: `make payload injector HARDWARE=1` (`9,410,720` bytes) compiles 100% clean with zero warnings and zero errors.

