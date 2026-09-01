# 2026-09-01 (hardware injector capture #8 analysis: EPERM resolution, multi-thread ucred sync & P_SUGID clearing) (D294)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt`):
1. **Foreground Retail Game Identified**:
   - The scanner correctly selected the foreground running game `eboot.bin` at PID 213.
   - Process credentials: `target orig cr_uid=0`, `target_ucred=0xffffca1379ad0a00`, `prison=0xffffffffc68629f0`.
   - Zero hardcoded title references: scanner operates completely generically on PlayStation executable names and title ID patterns.
2. **Root Cause Analysis of `PT_ATTACH errno=1` (EPERM)**:
   - **Single-Thread Credential Synchronization**: `krw_swap_ucred()` previously only patched `p_threads.tqh_first` (the head of `my_kproc->p_threads`). `NPXS40112` has multiple running threads. If the thread executing `sys_ptrace` was thread 2+, its `td_ucred` was still pointing to `my_ucred` inside the VSH prison container, causing `p_candebug()` / `prison_check()` to fail and return `EPERM`.
   - **Target `P_SUGID` Flag**: Target process had `p_flag = 0x10064800`. Bit `0x4000` is `P_SUGID` (`Had set id privileges, since last exec`). In FreeBSD, `sys_ptrace(PT_ATTACH)` explicitly rejects tracing `P_SUGID` processes unless the tracer holds `PRIV_DEBUG_DIFFCRED`.
3. **Remediation**:
   - **Multi-Thread Credential Synchronization (`src/injector/krw.c`)**: `krw_swap_ucred()` now traverses the complete thread list (`td_plist.tqe_next`) of `my_kproc`, synchronizing `td_ucred` on all active threads.
   - **Clear `P_SUGID` (`src/injector/krw.c`)**: In `krw_elevate_process(target_pid)`, clears bit `0x4000` (`p_flag &= ~0x4000`) on `target_kproc->p_flag` so the kernel does not enforce setuid/setgid debug restrictions.

Verified: `make payload injector HARDWARE=1` (`9,410,720` bytes) compiles 100% clean with zero warnings and zero errors.

