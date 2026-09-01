# 2026-09-01 (Stack Frame Return Alignment & Direct SYS_klog Output) (D310)


Analyzed hardware test log (Run #23):
1. **Diagnosis**:
   - `obscene_start()` successfully completed execution in the retail process.
   - However, the injector's previous hijack calculation (`(bak_reg.r_rsp - 8) & ~0xF; new_rsp -= 8;`) subtracted 16 bytes instead of 8 bytes.
   - When `obscene_start` executed `ret`, it returned to the host game code, but the host game stack was offset by 8 bytes, causing the host function's subsequent `ret` to pop the saved RBP (`0x7eeff7570`) into RIP and jump to the NX stack (`fault address: 0x7eeff7570`).
   - In addition, retail titles close standard file descriptor 1 (`stdout`), so `sceKernelWrite(1, ...)` does not reach the kernel log.
2. **Remediation**:
   - **Exact Stack Hijack Alignment (`src/injector/injector.c`)**: Pinned `jmp_reg.r_rsp = bak_reg.r_rsp - 8`. On entry, `obscene_start` sets up its frame; on `ret`, `RSP` returns to `bak_reg.r_rsp` with 100% precision.
   - **Kernel Log Syscall Channel (`src/probe/runtime.c`, `src/probe/start.c`)**: Updated `obs_debug_out_write()` and `obs_boot_note()` to invoke `SYS_klog` (`601`) via `libkernel`'s generic syscall stub (`base + 0x6a0`), ensuring records and boot notes appear directly in the PS5 kernel log without requiring an open file descriptor.

Verified: `make payload injector HARDWARE=1` (`9,411,144` bytes) compiles 100% clean with zero warnings and zero errors.

