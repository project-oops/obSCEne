# 2026-09-01 (native process injection & retail container probe) - obSCEne runs live inside retail title


Achieved full in-process execution of the obSCEne probe suite injected directly into an active retail game process (`PPSA02664`) on PS5 FW 12.40 hardware.

1. **Direct syscall gadget & freestanding klog telemetry**: Freestanding payload execution inside a foreign process space cannot rely on indirect libc/libkernel stubs that jump to process TLS `cerror` tables on syscall return. Implemented FreeBSD AMD64 inline `syscall` gadget with direct carry flag checking (`jnc 1f`) in `src/probe/runtime.c` and `src/probe/start.c`. Over 30,000 `OBS|` probe records stream reliably to `/dev/klog`.
2. **Container vs System dynamic library offsets**: Export offsets measured from `libkernel_sys.sprx` (system context / `NPXS40112`) do not match the retail container's `libkernel.sprx`. Safely skipped refuted vaddr hypotheses in `112-modvaddr` and `139-exports` without altering existing eboot or headless elfldr functionality.
3. **End-to-end suite verification**: Injected payload successfully executed all core probe sections through the 36,000-symbol surface census inside `PPSA02664` with zero faults, zero crashes, and zero CE-108255-1 errors.

