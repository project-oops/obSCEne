# 2026-09-01 (Fatal Signal in sceKernelDebugOutText & Safe Write Remediation) (D309)


Analyzed hardware test log (Run #22):
1. **Diagnosis**:
   - `obscene_start` was reached at `0x21d517880`.
   - `obs_boot_note` called `sceKernelDebugOutText(0, text)` at `0x80002b020`.
   - In retail game processes (`PPSA02664`), `sceKernelDebugOutText` accesses an uninitialized debug logging context pointer (`0x0 + 0x13a`), causing a SIGSEGV / page fault at `0x000000000000013a` (`rip=0x80002b037`, error CE-108255-1).
2. **Remediation**:
   - **Safe Boot Breadcrumbs (`src/probe/start.c`)**: Updated `obs_boot_note()` to write directly via `obs_libkernel_base() + 0x16e00` (`sceKernelWrite`) / `SYS_write`, which is a direct syscall stub that never accesses uninitialized retail debug structures.
   - **Loader Symbol Resolution (`src/injector/loader.c`)**: Removed `sceKernelDebugOutText` from dynamic symbol resolution so the weak symbol remains safely null.

Verified: `make payload injector HARDWARE=1` compiles 100% clean with zero warnings and zero errors.

