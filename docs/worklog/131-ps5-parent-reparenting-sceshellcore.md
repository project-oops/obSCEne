# 2026-09-01 (PS5 Parent Reparenting & SceShellCore Inspection Diagnostics) (D299)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 1-15 and 350-376):
1. **Kernel Patch Status Verified**:
   - `kpatch addr=0xffffffffc56c3088`
   - `kpatch read byte[1]=0x3`
   - `kpatch already set`
   - Verified that the kernel's ptrace AppContext gate variable is already set to `3`.
2. **Analysis of PT_ATTACH Failure (errno 37)**:
   - On PS5, standalone processes calling `PT_ATTACH` on retail games face parentage/supervisor gating: retail games are children spawned by `SceShellCore` (`p_pptr`).
   - In FreeBSD, attaching to a child already supervised or having active ptrace/proc event listeners returns `EALREADY` (37).
   - In `ps5debug-NG`, userland debug payloads avoid this by attaching to `SceShellCore` and executing from inside `SceShellCore` where `p_pptr` and shell rights permit game debugging.
3. **Remediation**:
   - **Target Process Parent Telemetry (`src/injector/procctl.c`)**: Added logging of `p_pptr` (`+0xE0`), parent PID (`p_pptr + 0xBC`), `p_oppid` (`+0x1FC`), and `p_ptevents` (`+0x24C`).
   - **Parent Reparenting Fallback (`src/injector/procctl.c`)**: If `PT_ATTACH` returns 37, the injector tests reparenting the target to itself (`p_pptr = my_kproc`), which satisfies FreeBSD child debugging rules, with automatic rollback if unsuccessful.
   - **SceShellCore Telemetry & Lookup (`src/injector/krw.c`, `src/common/krw.h`)**: Implemented `krw_find_proc_by_name()` and logging of `SceShellCore`'s PID, `kproc`, `ucred`, `cr_sceAuthID`, and `cr_prison`.

Verified: `make payload injector HARDWARE=1` (`9,410,824` bytes) compiles 100% clean with zero warnings and zero errors.

