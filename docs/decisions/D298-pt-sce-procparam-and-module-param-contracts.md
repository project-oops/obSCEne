# D298 - PT_SCE_PROCPARAM and PT_SCE_MODULE_PARAM Contracts on PS5 Native

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Investigating startup crashes on hardware: diagnosing SIGSEGV at address 0x28, PRX module discovery in `/app0/sce_module/`, and measuring `PT_SCE_MODULE_PARAM` in retail PRXs.

---

## Context

During hardware bringup of the native current-generation title on PS5 FW 12.40, two critical loader failure modes were observed and analyzed:

1. **Page fault on process start (`SIGSEGV` at `0x0000000000000028`)**:
   Hypothesizing that native PS5 processes link `libSceLibcInternal.sprx` and might leave `.libc_param = 0` (as observed in static ELF analysis of certain vendor dumps), `.libc_param` was set to NULL. Process execution immediately died before the entry point:
   ```text
   # A user thread receives a fatal signal
   # signal: 11 (SIGSEGV)
   # reason: page fault (user write data, page not present)
   # fault address: 0000000000000028
   # /.../common/lib/libkernel.sprx
   # /.../common/lib/libSceLibcInternal.sprx
   ```
   `libkernel.sprx` initialization unconditionally writes to `*(libc_param + 0x28)`. When `.libc_param == NULL`, the store attempts to write to physical address `0x28`, triggering an immediate MMU page fault.

2. **Module discovery failure (`PRX_SCE_MODULE_LOAD_ERROR` `0xa0020102`)**:
   Once `.libc_param` was restored to a valid buffer, `libkernel.sprx`, `libSceLibcInternal.sprx`, `libSceSysmodule.sprx`, and `libSceAmpr.sprx` loaded cleanly. Startup then failed with:
   ```text
   # === Lack of a .prx file in /app0/sce_module is detected!!! ===
   # Copy the file (e.g. libc.prx) from target/sce_module.
   # Refer to PRX_SCE_MODULE_LOAD_ERROR in Kernel Reference.
   mDBG: Sending signal(pid: ..., tid: ..., signo: 0xa0020102)
   ```
   Despite `libc.prx` existing on `/app0/sce_module/`, `libSceSysmodule` rejected it.

## Decision

1. **`libc_param` Is Non-Optional in `PT_SCE_PROCPARAM` (`0x61000001`)**:
   - `obs_process_param.libc_param` MUST point to a valid writable structure (`&obs_libc_param`).
   - The memory at offset `+0x28` is initialized to zero and populated dynamically by `libkernel`.
   - Never set `.libc_param = 0` in any build targeting hardware execution.

2. **PRX Modules Require `PT_SCE_MODULE_PARAM` (`0x61000002`)**:
   - The PS5 native loader expects bundled PRX modules in `/app0/sce_module/` to declare `PT_SCE_MODULE_PARAM` (`p_type == 0x61000002`):
     ```text
     PT_SCE_MODULE_PARAM: size 0x20
     mod_param struct:
       size: 0x20
       magic: 0x3c13f4bf
       version: 0x3
       sdk_version: 0x08050001 (PS4 SDK 8.050.001)
       sdk_version_second: 0x02000009 (PPR SDK 2.000.009)
       flags: 0x1
     ```
   - When `libSceSysmodule` checks PRX files in `/app0/sce_module/`, it requires this segment to validate module parameters and SDK targeting. Modules lacking `PT_SCE_MODULE_PARAM` trigger `PRX_SCE_MODULE_LOAD_ERROR` (`0xa0020102`).

3. **Synthetic Module Compliance**:
   - Populating `struct sce_module_param` inside `.data.sce_module_param` directly in `src/probe/sce_module.c` satisfies the loader's requirements, allowing synthetic modules to load cleanly from source.

## Consequences

* Discovered the precise structural requirements for PS5 native PRX modules.
* Preserves `libc_param` stability across both PS4 compatibility and PS5 native targets.

