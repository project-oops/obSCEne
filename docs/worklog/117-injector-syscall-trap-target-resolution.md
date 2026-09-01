# 2026-09-01 (injector syscall trap & target resolution fix) - direct syscall elimination & PPSA title prioritization (D285)


Diagnosed hardware log failure `PPRBUG-22859: the process pid=213 directly issued a syscall 601 at 0x2000080c7`:
1. **Syscall Trampoline Fix**:
   - `src/common/syscall.c`: Fixed `sys_call_init` logic which previously skipped `s_ptr_syscall` assignment when `sceKernelDlsym` resolution returned null. `s_ptr_syscall` now unconditionally initializes from `args->sys_dynlib_dlsym` (guaranteed to be inside `libkernel.sprx`).
   - Removed inline `syscall` instruction fallback in `sys_call`. Any call without trampoline safely returns `-1` instead of issuing raw userland `syscall`.
   - Verified via disassembly: `sys_call` contains zero `syscall` instructions and exclusively dispatches via `callq *%r14` into `libkernel`.
2. **Elevation Order & Process Syscall Unrestriction**:
   - `src/injector/injector.c`: Reordered initialization so `krw_init(args)` and `krw_elevate_current_process()` execute BEFORE any logging (`klog_write`).
   - `src/injector/krw.c` & `src/common/krw.h`: Added `krw_elevate_process(pid_t pid)` to unlock the syscall address range (`[kproc + 0x3e8 -> kaddr + 0xf0] = 0`, `[kaddr + 0xf8] = ~0ULL`) for both the injector and target process.
3. **Target Title Prioritization (Prevent SceSpZeroConf Hijack)**:
   - `src/injector/target.c` & `src/injector/target.h`: Added `target_init(args)` resolving `sceKernelGetAppInfo`.
   - In `target_find_foreground_app()`, inspects `app_info.title_id`. Explicitly prioritizes native retail PS5 games (`PPSA...`) first, PS4 games (`CUSA...`) second, and rejects system VSH daemons (`NPXS...`, `NPXX...`, `.elf`, `Sce...`). This prevents accidentally selecting `NPXS40112` (`SceSpZeroConf`) when a retail title (`PPSA...`) is active.
4. **Census Deduplication**:
   - Removed newly promoted `sceKernelVirtualQuery` from `include/obscene/surface.h` and `sceKernelAllocateMainDirectMemory` from `include/obscene/corpus.h` to prevent `redefinition as different kind of symbol` conflict with `platform.h`.

Verified: `make check BUILD=$HOME/obs`, `make payload injector HARDWARE=1 BUILD=$HOME/obs` (`9,393,592` bytes), and `bash scripts/verify.sh` pass 100% clean.

