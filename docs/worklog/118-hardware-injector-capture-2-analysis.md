# 2026-09-01 (hardware injector capture #2 analysis & jaildir sandbox resolution) (D286)


Hardware test `./bin/obscene inject` produced full `/dev/klog` telemetry:
1. **Syscall fix validated**: `injector_start` logged `<118>[injector]` five times to `/dev/klog`. `krw_init` and `krw_elevate_current_process()` succeeded cleanly with zero `PPRBUG-22859` traps.
2. **Crash in `target_init` resolved**:
   - The crash at `rip: ffffca133dc09f80` was traced to `target_init` attempting to execute `args->sys_dynlib_dlsym` to find `sceKernelGetAppInfo`. Freestanding payloads cannot call dynamic linker stubs directly this way.
   - Removed `target_init` and `args->sys_dynlib_dlsym` calling entirely from `src/injector/target.c` and `src/injector/target.h`.
3. **Absence of Foreground Game & Jaildir Title Detection**:
   - The klog's full process list (`SceShellCore` VM Stats) revealed that NO retail game was active on the console during the run (all running processes were `NPXS` system apps or background daemons).
   - In `src/injector/target.c`, eliminated fallback selection. Now checks `krw_get_proc_jaildir(pid)`: retail titles run inside a sandbox jail (`jaildir != 0`), whereas root/system processes have `jaildir == 0`.
   - If no valid game process (`PPSA...`, `CUSA...`, or jailed `eboot.bin`) is detected, `target_resolve` returns `-1`, logs `"ERROR: no running retail game found in userland"`, and exits safely without touching any system processes.

Verified: `make check BUILD=$HOME/obs`, `make payload injector HARDWARE=1 BUILD=$HOME/obs` (`9,393,512` bytes), and `bash scripts/verify.sh` pass 100% clean.

