# 2026-09-01 (PS5 FW 12.40 ptrace kernel patch implementation: 0xD83088 AppContext gate) (D298)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 360-380):
1. **Target Decoupled and Clean**:
   - `selected foreground game pid=212`
   - `target p_flag=0x10004000` (`P_INMEM | P_EXEC`, zero trace flags)
   - `target p_flag2=0x0`
   - `my_ucred` and `target_ucred` decoupled with identical prison `0xffffffffc68629f0` and root UID.
   - `PT_ATTACH` still returned `errno=37` (`EALREADY` / `SCE_KERNEL_ERROR_EALREADY`).
2. **Breakthrough: Sony AppContext ptrace Gate in Kernel Data Memory**:
   - In standard FreeBSD, `sys_ptrace(PT_ATTACH)` does not return `37`.
   - On PlayStation 5, the kernel enforces an AppContext ptrace gate in kernel `.data` memory that specifically restricts `PT_ATTACH` on retail game applications unless the kernel ptrace flag is set or the request originates from `SceShellCore`. Lacking this patch, the kernel immediately rejects attachment to games with `SCE_KERNEL_ERROR_EALREADY` (37).
   - In `ps5debug-NG` on FW 12.40 (`0x12400000`), the kernel patch address is located at:
     ```c
     patch_addr = kbase + 0xD83088ULL;
     scratch[1] |= 3;
     ```
   - If `scratch[1] & 3 != 3`, `PT_ATTACH on games will not work`.
3. **Remediation**:
   - Implemented `krw_apply_ptrace_kernel_patch()` in `src/injector/krw.c` covering FW 3.xx through 13.xx, targeting `s_kdata_base + 0xD83088ULL` on FW 12.40.
   - Function reads the 16-byte gate descriptor, ORs byte `[1]` with `3`, writes it back via kernel write primitive, and verifies the write.
   - Integrated into `krw_elevate_current_process()` so ptrace attachment capability is permanently unlocked for the session before process resolution.

Verified: `make payload injector HARDWARE=1` (`9,410,776` bytes) compiles 100% clean with zero warnings and zero errors.

