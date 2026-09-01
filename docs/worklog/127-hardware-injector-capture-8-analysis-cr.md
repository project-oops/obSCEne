# 2026-09-01 (hardware injector capture #8 analysis: cr_sceCaps offset correction 0x60 & readback verification) (D295)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 358-377):
1. **Verification of Target Process and Prison**:
   - `selected foreground game pid=213` confirmed foreground application resolution.
   - `target p_flag=0x10060800` confirmed successful clearing of `P_SUGID` (`0x4000`).
   - `my_ucred cr_prison=0xffffffffc68629f0` and `target_ucred cr_prison=0xffffffffc68629f0` confirmed that the host daemon and the foreground game reside within the identical jail prison container.
2. **Root Cause Analysis of `PT_ATTACH errno=1` (EPERM)**:
   - In `src/injector/krw.c`, `KERNEL_OFFSET_UCRED_CR_SCECAPS` was mistakenly configured as `0x70`.
   - On the PlayStation 5 kernel, `cr_sceCaps` is located at offset `0x60` within `struct ucred` (16 bytes: `0x60` and `0x68`).
   - Because of this offset disparity, `krw_set_ucred_caps()` was writing capability bitmasks into `ucred + 0x70` instead of `0x60`. Consequently, `cr_sceCaps` remained unpopulated with all-1s on both the injector process and the target process.
   - Sony's MAC security module (`mac_proc_check_debug()`) checks caller capability flags in `cr_sceCaps` before permitting `PT_ATTACH`. Lacking capabilities, the attachment request was denied with `EPERM` (`errno=1`).
3. **Remediation**:
   - Fixed `KERNEL_OFFSET_UCRED_CR_SCECAPS` to `0x60` in `src/injector/krw.c`.
   - Added readback logging of `caps[0]`, `caps[1]`, and `attr[3]` for both the calling process and the target process to verify capability bitmasks in kernel memory prior to attachment.

Verified: `make payload injector HARDWARE=1` (`9,410,720` bytes) compiles 100% clean with zero warnings and zero errors.

