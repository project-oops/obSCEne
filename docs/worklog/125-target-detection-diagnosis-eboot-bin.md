# 2026-09-01 (target detection diagnosis: eboot.bin POSIX root uid 0 & empty comm filtering) (D293)


Diagnosed why the running retail game title was skipped in hardware capture Run #8:
1. **Root Cause Analysis (`reports/hardware/injector-klog.txt`)**:
   - The diagnostic census logged running processes with `comm == "eboot.bin"` (`pid=213`, `pid=212`, `pid=208`, `pid=191`, `pid=178`, etc.).
   - However, every single `eboot.bin` was logged as:
     `skip system proc: eboot.bin pid=... uid=0`
   - In D291, `if (is_system_daemon(comm) || uid <= 1)` was introduced under the false assumption that retail games run with non-root POSIX UIDs (`uid > 1`). On FreeBSD/Prospero, sandboxed container jails execute applications under container root `cr_uid == 0`. Filtering out `uid <= 1` caused the scanner to discard every retail game process on the system!
   - Furthermore, unnamed daemons (like PID 204 in previous runs) had `comm == ""` (empty), which `is_system_daemon()` previously treated as non-system.
2. **Remediation (`src/injector/target.c`)**:
   - **Removed `uid <= 1` Filter**: Restored full evaluation of all userland processes regardless of POSIX `cr_uid`.
   - **Filtered Empty / Unnamed Daemons**: In `is_system_daemon(comm)`, empty or NULL `comm` strings (`comm[0] == '\0'`) are now treated as system daemons and skipped immediately. Retail games are strictly required to have an executable name (`eboot.bin`).
   - **Prioritize Newest `eboot.bin`**: In `allproc` traversal (ordered newest-first from list head), candidate `eboot.bin` processes are scanned for `PPSA...` / `CUSA...` Title IDs; if un-named in `struct proc`, the newest launched `eboot.bin` PID is selected.
   - **Prison Pointer Telemetry**: Added logging of `cr_prison` (`ucred + 0x30`) for all candidate processes to verify jail structures.

Verified: `make payload injector HARDWARE=1` (`9,394,336` bytes) compiles 100% clean with zero warnings and zero errors.

