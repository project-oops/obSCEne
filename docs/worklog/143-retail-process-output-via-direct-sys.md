# 2026-09-01 (Retail Process Output via Direct SYS_klog Channel) (D311)


Analyzed hardware test log (Run #24):
1. **Diagnosis**:
   - `rip: 0x0`: In `obs_boot_note`, calling `sceKernelWrite(1, ...)` (`libkernel_base + 0x16e00`) caused `libkernel` to dispatch through its internal stdio vtable for file descriptor 1 (`stdout`). Because retail titles do not initialize stdout handlers, `table[1]->write` was `NULL` (`0x0`), causing an indirect call to `0x0`.
   - On the other hand, `SYS_klog` (`sys_fn(601, ...)`) via `libkernel_base + 0x6a0` succeeded without errors.
2. **Remediation**:
   - **Boot & Output Channel Migration (`src/probe/start.c`, `src/probe/runtime.c`)**: Removed all stdio `sceKernelWrite(1, ...)` calls from `obs_boot_note()` and `obs_send(OBS_CHANNEL_KERNEL_WRITE)`. All boot notes and test probe records (`OBS|...`) are now routed exclusively through `SYS_klog` (`601`), going directly to `/dev/klog` without touching retail stdio handlers.

Verified: `make payload injector HARDWARE=1` compiles 100% clean with zero warnings and zero errors.

