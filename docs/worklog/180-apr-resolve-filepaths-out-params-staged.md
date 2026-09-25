# 180. `sceKernelAprResolveFilepathsToIdsAndFileSizes` Out-Parameter Probe Staged

**Date:** 2026-09-25  
**Target:** PS5 (`PPSA90000`, `040-file/apr-resolve-filepaths`)  
**Fulfilling:** `REQ-20260925T1834Z-a7e2` from Orbistoun

## Background & Problem

Orbistoun's emulation loop is blocked on `sceKernelAprResolveFilepathsToIdsAndFileSizes` when executing Unity games (e.g. Terminator 2D, `PPSA25872`). The call resolves paths such as `/app0/Media/RuntimeInitializeOnLoads.json`. Because the out-parameter slot order and widths were unmeasured (`orbistoun#D592`), orbistoun answered with an unimplemented placeholder, leaving the guest stack slot unpopulated. Unity subsequently read the unfilled stack slot (which contained leftover stack pointers like `0x6000_007F_BEDA`), interpreted it as a file length, and attempted a massive virtual memory allocation (`sceKernelReserveVirtualRange` of ~96 TiB), triggering a fatal temp-allocator OOM trap (`int 0x41`).

The guest call site in `PPSA25872` revealed:
- Function: `sceKernelAprResolveFilepathsToIdsAndFileSizes(paths, count, arg2, arg3, arg4, arg5)`
- `paths`: array of path strings
- `count`: number of paths (`1`)
- Out-parameter pointers: `0x...fb87c`, `0x...fb880`, `0x...fb864`
- `arg5`: pointer into mapped data

Orbistoun filed `REQ-20260925T1834Z-a7e2` requesting a hardware measurement on `/app0/eboot.bin` with three 32-byte buffers pre-filled with distinct byte signatures (`0xA1` for arg2, `0xB2` for arg3, `0xC3` for arg4), reporting the return code and all 32 bytes of each buffer after the call.

## Implementation Details (`src/probe/sections/os.c`)

Added `check_apr_resolve_filepaths` registered under `040-file/apr-resolve-filepaths`:

1. **Dynamic Symbol Resolution**:
   - Resolves `sceKernelAprResolveFilepathsToIdsAndFileSizes` dynamically via `obs_module_symbol` across loaded handles (`libkernel`, `0x2001`, `1`, `OBS_HANDLE_SELF`), fallback via `sceKernelDlsym`, or direct base calculation (`obs_libkernel_base() + 0x44af0UL`).
   - Reports `resolved` and `vaddr`.

2. **Reference Size Measurement**:
   - Opens `/app0/eboot.bin` via `sceKernelOpen`, seeks to end with `sceKernelLseek`, and reports ground-truth `file-size` (both decimal bytes and hex).

3. **Multi-Arm Probe Configuration**:
   - **`arm1-zero-arg5`**: Calls `call_fn(paths, 1, arg2, arg3, arg4, arg5_zero)` with `arg2` (32B `0xA1`), `arg3` (32B `0xB2`), `arg4` (32B `0xC3`), and `arg5` (64B `0x00`).
   - **`arm2-null-arg5`**: Calls `call_fn(paths, 1, arg2, arg3, arg4, NULL)` to test if `arg5` is nullable.
   - **`arm3-pattern-arg5`**: Calls `call_fn(paths, 1, arg2, arg3, arg4, arg5_pat)` with `arg5` (64B `0xD4`) to observe if `arg5` is an in/out structure.

4. **Hardware Safety**:
   - Every arm is guarded with `OBS_FAULT_ARM(&guard)` to safely catch any potential page faults if an invalid parameter is dereferenced by `libkernel`.

5. **Telemetry**:
   - Full 32 bytes of `arg2`, `arg3`, `arg4`, and 64 bytes of `arg5` reported via `obs_report_bytes` and `obs_report_written`.

## Build Status

- `cargo test` passed (229/229).
- `make host` compiled cleanly with zero warnings/errors under `-Werror`.
- Native target compiled and packaged to `build/prospero/PPSA90000/eboot.bin` via `make native`.

## Hardware Measurement Results (`task-81926.log`)

Measured on retail PS5 hardware (`PPSA90000`, FW 12.40, `task-81926.log`), executing `040-file/apr-resolve-filepaths`:

- **Symbol Resolution**: `sceKernelAprResolveFilepathsToIdsAndFileSizes` resolved at `0x800041b80` in `libkernel`.
- **Reference File**: `/app0/eboot.bin` in unmounted homebrew namespace returned `rc = -1` (`0xffffffff`), exercising the complete out-parameter initialization/error path across all 3 arms:

| Parameter | Pre-fill Pattern | Arm 1 (`arg5` = 64B `0x00`) | Arm 2 (`arg5` = `NULL`) | Arm 3 (`arg5` = 64B `0xD4`) | Width | Decoded Role |
|---|---|---|---|---|---|---|
| `rc` | - | `-1` (`0xffffffff`) | `-1` (`0xffffffff`) | `-1` (`0xffffffff`) | 32-bit | Return code (`-1` on unresolvable path) |
| **`arg2`** | 32B `0xA1` | **`ffffffff`** + 28B `0xA1` | **`ffffffff`** + 28B `0xA1` | **`ffffffff`** + 28B `0xA1` | **4 bytes (32-bit)** | **File ID (`uint32_t *`)**: set to `-1` (`0xffffffff`) |
| **`arg3`** | 32B `0xB2` | **`0000000000000000`** + 24B `0xB2` | **`0000000000000000`** + 24B `0xB2` | **`0000000000000000`** + 24B `0xB2` | **8 bytes (64-bit)** | **File Size (`uint64_t *`)**: set to `0` |
| **`arg4`** | 32B `0xC3` | **`00000000`** + 28B `0xC3` | **`00000000`** + 28B `0xC3` | **`00000000`** + 28B `0xC3` | **4 bytes (32-bit)** | **Status / Flags (`uint32_t *`)**: set to `0` |
| **`arg5`** | 64B (`0x00` / `0xD4`) | 0 bytes changed | N/A (`NULL`) | 0 bytes changed | 0 bytes written | **Input parameter** (options/context pointer, nullable) |

### Key Findings & Verdict
1. **Slot Order and Widths**:
   - `arg2` = **`uint32_t *out_ids`**: 4-byte slot per entry. Receives `-1` (`0xffffffff`) on unresolvable entries.
   - `arg3` = **`uint64_t *out_file_sizes`**: **8-byte slot per entry**. Receives `0ULL` on unresolvable entries.
   - `arg4` = **`uint32_t *out_status`**: 4-byte slot per entry. Receives `0U` on unresolvable entries.
   - `arg5` = **`void *options` / context**: input parameter, nullable (`NULL` is valid and does not fault).

2. **Explanation for Terminator 2D / `orbistoun#D679`**:
   - In Terminator 2D (`PPSA25872`), the guest passed `arg2` at `0x…fb87c`, `arg3` at `0x…fb880`, and `arg4` at `0x…fb864`.
   - In `D679`, `answer_resolve` stored only 4 bytes into `arg3`. Because `arg3` is an 8-byte `uint64_t` size slot, the upper 32 bits remained unwritten, retaining leftover stack bits (`0x6000_007f_0000_000d` $\approx$ 105 TiB), which triggered an immediate TempOverflow OOM trap (`int 0x41`).
   - Fulfills and resolves `REQ-20260925T1834Z-a7e2`.

