# 2026-09-01 (hardware injector capture #7 analysis: thread ucred confirmed, system daemon false positive diagnosed & direct syscall errno) (D291)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` and `reports/hardware/injector-klog.txt.klog`):
1. **Thread Credential Scanning Confirmed Working Live**:
   - The thread scanner successfully traversed `my_kproc->p_threads` and dynamically located `td_ucred` at offset `0x140` in `struct thread`.
   - Successfully matched `my_ucred = 0xffffca1334f8a800` and updated `td->td_ucred` to `target_ucred = 0xffffca133c20b800`.
   - Target credential UIDs elevated (`target orig cr_uid=1` -> `0`).
2. **False Positive Target Resolution Diagnosed**:
   - `SceShellCore` VM Stats (lines 44-82 of `.klog`) lists every active process on the system: No retail game was active on the console; the console was sitting idle at the Login/Welcome screen.
   - PID 204 was a system service daemon (`cr_uid=1`), not a retail game. `get_proc_title_id()` scanned 2048 bytes of `struct proc` and matched a cached title ID string (`"PPSA04263"`) in that daemon's metadata/dashboard tile memory.
   - Because `get_proc_title_id()` executed before verifying `comm` or `is_system_daemon()`, PID 204 was mistakenly selected. `sys_ptrace(PT_ATTACH)` fails when attempted against system service daemons.
3. **Remediation**:
   - **System Daemon & UID Filtering**: Updated `target_find_foreground_app()` in `src/injector/target.c` to read `comm` and `cr_uid` first. All system daemons (`is_system_daemon(comm)`) and root/daemon processes (`uid <= 1`) are skipped. Title ID scanning is strictly restricted to userland processes (`uid > 1`).
   - **Explicit Idle Guard**: If no userland retail game process is active, `target_find_foreground_app()` returns -1 and logs `ERROR: no active retail game found in userland`, cleanly exiting without touching any system processes.
   - **Direct Syscall Errno Capture**: Implemented `sys_enable_direct()` in `src/common/syscall.c` and `src/common/syscall.h`, activated after `krw_elevate_current_process()` unlocks the syscall table. Bypasses `libkernel`'s error stub and directly captures the kernel's `%rax` errno when CF=1, guaranteeing non-zero errno visibility.

Verified: `make payload injector HARDWARE=1` (`9,394,336` bytes) compiles 100% clean with zero warnings and zero errors.

