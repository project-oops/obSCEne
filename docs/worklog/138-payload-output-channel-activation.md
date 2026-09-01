# 2026-09-01 (Payload Output Channel Activation: libkernel Base & Symbol Resolution) (D306)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` and `.klog` lines 105-131):
1. **Diagnosis of Successful Detach & Silent Execution**:
   - In Run #18, `PT_DETACH ret=0`, `detach return=0`! The retail game remained alive and did NOT crash or close.
   - However, no `obscene: ...` lines appeared in the kernel log or socket output.
   - Analysis of `src/probe/start.c` and `runtime.c`:
     - `sceKernelDebugOutText` and `sceKernelWrite` in `obscene-payload.elf` are weak unresolved symbols.
     - In raw payload mode, `obscene` relies on `payload_args[0]` (`sys_dynlib_dlsym`) containing `getpid`'s address to bootstrap `libkernel_base` (`getpid - 0x5b0`) and `sceKernelWrite` (`base + 0x16e00`).
     - The injector was staging a zeroed `target_args` struct where `sys_dynlib_dlsym` was `0`, so `obs_bootstrap_payload_output` refused to initialize output channels.
     - In addition, `loader.c` only handled `R_X86_64_RELATIVE` relocations, leaving the GOT and PLT slots for `sceKernelDebugOutText` and `sceKernelWrite` at `0`.
2. **Remediation**:
   - **Target `libkernel` Base Resolution (`src/injector/krw.c`)**: Implemented `krw_find_target_libkernel_base()`, which inspects the target process's kernel module list at `target_kproc + 0x3E8` (matching `sel == 0x2001 || sel == 1`) with fallback to `0x800000000`.
   - **Staging `getpid` in `payload_args` (`src/injector/injector.c`)**: Initialized `target_args.sys_dynlib_dlsym = target_libkernel_base + 0x5b0`.
   - **Dynamic Symbol Resolution in Loader (`src/injector/loader.c`)**: Updated `loader_load_into_proc()` to parse `.dynsym` / `.dynstr` during relocation processing, resolving GOT/PLT entries for `sceKernelDebugOutText` (`0x2b020`), `sceKernelWrite` (`0x16e00`), `getpid` (`0x5b0`), `sceKernelOpen` (`0x16d60`), `sceKernelClose` (`0x16dc0`), and `sceKernelUsleep` (`0x16f00`).
   - **Kernel Log Output Fallback (`src/probe/start.c`)**: Added fallback in `obs_boot_note()` using `obs_libkernel_base() + 0x2b020` when `&sceKernelDebugOutText` is not directly linked.

Verified: `make payload injector HARDWARE=1` (`9,410,912` bytes) compiles 100% clean with zero warnings and zero errors.

