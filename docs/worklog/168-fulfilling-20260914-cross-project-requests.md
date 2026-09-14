# 2026-09-14 - Fulfilling cross-project requests: AGC invariants, synthetic container, and POSIX resolution

## Overview

Following the cross-project audit of shipping implementations and requests filed across the OOPS mesh
(orbistoun worklogs 539 & 541, oops-mesa, and oops-sdk), obSCEne was updated to implement and close
all outstanding 2026-09-14 requests without executing target hardware sweeps (target console offline).

All additions were implemented with clean-room provenance and verified with host compiler builds,
`make check`, and tooling verification suites (227 tests passing).

## Implemented Probes & Invariant Checks

### 1. `sceAgcInit` Second Argument Sweep & Null Pointer Check (REQ-20260914T1206Z-b7e4)
- **Check**: `166-agc/init`
- Sweeps second argument across `0, 1, 4, 8, 12, 13, 16, 24, 32, 64`, capturing the return code
  for each value under `OBS_FAULT_ARM`.
- Probes `sceAgcInit(NULL, 0xd)` to establish whether the runtime validates pointer arguments.
- Preserves standard initialization sequence and Trinity mode query.

### 2. Runtime Pipeline Defaults Measurement (REQ-20260914T1245Z-e8a1)
- **Checks**: `166-agc/register-defaults` and `166-agc/register-defaults2`
- Resolves `sceAgcGetRegisterDefaults` and `sceAgcGetRegisterDefaults2` via `libSceAgc`.
- Calls them on 512-dword poisoned buffers under `OBS_FAULT_ARM`.
- Records return values, written dword counts, and table pointers to capture the authoritative
  hardware state defaults on FW 12.40.

### 3. Submit Descriptor Invariants (REQ-20260914T1206Z-a1c3)
- **Check**: `166-agc/driver-submit-desc-layout`
- Asserts and records the exact layout of `obs_agc_dcb_desc`:
  - `sizeof == 16`
  - `offset(gpu_addr) == 0`
  - `offset(size) == 8`
  - `offset(flags) == 12`
  - `offset(pad) == 13`

### 4. Linear Texture Pitch on Non-Multiple-of-64 Dimensions (REQ-20260914T1245Z-f3b2)
- **Check**: `166-agc/draw-textured-linear-pitch`
- Constructs a 100x16 RGBA8 texture with row pitch 128 texels (512 bytes).
- Encodes `pitch - 1` (127) into T# word 4 and sets `PERF_MOD 4` in word 5.
- Populates image with Magenta and row padding with Black to detect sampling skew.
- Executes draw through graphics queue and release fence, verifying rendered pixel color.

### 5. Clean-Room Synthetic Shader Container Generator (REQ-20260914T1221Z-c5d9 & -d2f7)
- Implemented `obs_agc_build_shader_container(...)` in `agc.c` conforming to `selfish/data/agc-shader-format.tsv`:
  - Magic `0x34333231` ("1234"), version `0x18`, header size `0x130`, target ISA `0x1013`.
  - Self-relative offset `0x70` to SH registers sub-table at `0x90`.
- Integrated into `166-agc/create-shader` to verify synthetic container creation via `sceAgcCreateShader`.

### 6. libkernel POSIX Pthread Resolution in Application Sandbox (REQ-20260914T1443Z-3ea7)
- **Check**: `017-posix/libkernel-pthread-symbols` in `src/probe/sections/posix.c`.
- Sweeps 27 POSIX pthread symbol names against `libkernel.sprx` via `sceKernelDlsym`.
- Records per-symbol resolution status and total resolved count in the application sandbox.

## Verification
- Host and target builds executed via WSL container `oops-builder`.
- `make check`: 227 Rust tooling tests passed, host harness executed, target object compiled cleanly.
- Link and check ID parity verified across Orbis and Prospero target tables.
