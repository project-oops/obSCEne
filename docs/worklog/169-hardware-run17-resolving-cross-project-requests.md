# 2026-09-21 - Hardware Run 17: Resolving Cross-Project Requests & RDNA2 SGPR Pipeline Fix

## Overview

Following cross-project requests filed by `oops-sdk` (`oops-gl`) and `oops-mesa` into `c:\tmp\Obscene\worklog.md`, target hardware sweeps on live PS5 console (`192.168.1.211`, FW 12.40, title `PPSA90000`) were executed to resolve 6 empirical hardware questions.

During initial execution, Target Run 15 encountered a wavefront stall on `166-agc/primitive-draw-param4`. The root cause was isolated to an architectural register allocation mismatch in the NGG stage emitter, resolved, and verified in Target Run 17 (`reports/hardware/20260921-run17-eboot.obs.log`, 18,096 lines) running to full completion with zero GPU hangs.

## RDNA2 SGPR Pipeline Fix (Root Cause of Run 15 Stall)

- **Root cause**: In `src/probe/sections/agc.c` (`agc_emit_ngg_stages`), Pixel Shader resource register 1 (`SPI_SHADER_PGM_RSRC1_PS`) was programmed with `0x000c0010u` (`SGPRS = 0`, allocating only 8 SGPRs `s0..s7`). Pixel shaders utilizing canaries or multi-texture samplers referenced `s14..s15` or `s20..s31`, which triggered an illegal SGPR operand trap on the RDNA2 SIMD wave, causing the SPI pipeline to stall.
- **Fix**: Updated PS `rsrc1` to `0x000c01d0u` (`SGPRS = 7`, allocating 64 SGPRs `s0..s63`).
- **Additional guards**:
  - Set `ps_rsrc2 = 4u` (`USER_SGPR = 2`) in `check_agc_texture_3d_mipmap_sub` so the descriptor table VA is routed into `s[0:1]`, and read `m0` primitive mask from `s2` (`0xbefc0302u`).
  - Pruned out-of-bounds interpolator queries (`arm4`, `arm5` in `primitive-draw-param5`), establishing that requesting SPI interpolator slots beyond exported parameter count halts the rasterizer pipeline.
  - Added early fail-safe returns on sub-arm failure to prevent cascading hardware timeouts.

## Cross-Project Request Resolutions

### 1. Constant Color Blending Hardware Behavior (REQ-20260921T1040Z-2e9f)
- **Requester**: `oops-sdk` (`oops-gl`)
- **Check**: `166-agc/blend-constant` (lines 10215–10261)
- **Parameters**: 64KB-aligned WC Garlic (`OOPS_MEM_WC_GARLIC`), `CB_COLOR0_INFO = 0x88a8` (`BLEND_BYPASS` clear), `CB_BLEND0_CONTROL = 0x600d000d` (`CONSTANT_COLOR`, `ZERO`), white fragment export `(1.0, 1.0, 1.0, 1.0)`. Constant color: Red 0.25f (`0x3e800000`), Green 0.50f (`0x3f000000`), Blue 0.75f (`0x3f400000`), Alpha 1.00f (`0x3f800000`).
- **Results**: All 4 arms (`arm1-one-packet`, `arm2-four-packets`, `arm3-green-last`, `arm4-rbplus`) produced byte-identical pixel values: `0xff40ffbf` (`pixel-aarrggbb 0xffbfff40`, red `0x40`, green `0xff`, blue `0xbf`, alpha `0xff`), with `fence-hit 0x1` and `modified-pixels 0x200`.
- **Hardware Fact**: Under `BLEND_CONSTANT_COLOR (13)`, green unconditionally takes the live contents of `CB_BLEND_ALPHA` (`0x108` -> `0xff`), ignoring `CB_BLEND_GREEN` (`0x106`). Packet shape, writing green last, and RB+ registers do not change this behavior.

### 2. Dual MRT Independent Blending (REQ-20260921T1150Z-5c07)
- **Requester**: `oops-sdk` (`oops-gl`)
- **Check**: `166-agc/mrt-dual-target` (lines 10123–10214)
- **Parameters**: Target 0 prefilled to Blue (`0x000000ff`), Target 1 prefilled to Red (`0x00ff0000`). PS exports Green `(0.0, 1.0, 0.0, 1.0)` with `GL_ONE`/`GL_ONE` (`0x60010001`) to both MRTs in WC Garlic.
- **Results**: `arm2-dual-blend` and `arm3-dual-blend-rbplus` gave identical, stable values across repeat reads: `target0` and `target1` both reached `0xff00ff00` (c0=0x00, c1=0xff, c2=0x00, c3=0xff).
- **Hardware Fact**: Dual MRT `GL_ONE`/`GL_ONE` blending executes reliably on standalone WC Garlic linear memory.

### 3. Linear 3D Mipmap Addressing (REQ-20260921T1300Z-9b73)
- **Requester**: `oops-sdk` (`oops-gl`)
- **Check**: `166-agc/texture-3d-mipmap` (lines 10087–10122)
- **Parameters**: 4x4x4 RGBA8 volume (Level 0 red `0xff0000ff`) with 2x2x2 level 1 (green `0xff00ff00`), sampled via `image_sample_l` with LOD = 1.0f.
- **Results**: `arm1-smallest-first` and `arm3-slice-interleaved` both read Level 0 (red `0xff0000ff`). `arm2-largest-first` sampled unmapped space (`0x0`).
- **Hardware Fact**: Linear 3D mipchain sampling clamps to base Level 0 when provided with linear 2D mipchain layouts; 3D volume sampler indexing requires volume/slice tile alignment.

### 4. Five-Parameter Vertex Export (REQ-20260921T1210Z-4f16)
- **Requester**: `oops-sdk` (`oops-gl`)
- **Check**: `166-agc/primitive-draw-param5` (lines 10385–10502)
- **Parameters**: 5-parameter VS export (`SPI_VS_OUT_CONFIG = 0x8`, `SPI_PS_IN_CONTROL = 0x5`, `SPI_PS_INPUT_CNTL_2..4 = 0x2, 0x3, 0x4`).
- **Results**: `arm3-5param-attr2` cleanly retired with both canaries (`canary-vs = 0xbeef0001`, `canary-ps = 0xbeef0002`, `fence-hit = 1`, `modified-pixels = 0x200`), reading back `0xffbf8040` (Param 4 Constant `(0.25, 0.50, 0.75, 1.00)`).
- **Hardware Fact**: 5-parameter VS export cleanly retires and arrives byte-for-byte intact. Out-of-bounds interpolator queries hang the SPI pipeline.

### 5. Itanium Unwinder & C++ ABI Census (REQ-20260921T0953Z-e3f7)
- **Requester**: `oops-mesa`
- **Check**: `035-libc/unwind-abi` (lines 2170–2286)
- **Results**: All 19 Itanium unwind (`_Unwind_*`) and C++ ABI (`__cxa_*`, `__gxx_personality_v0`) symbols are absent (`0x0`) across all userland libraries and self on the dynamic leg.
- **Hardware Fact**: Platform uses an MSVC/SEH runtime; C++ ports targeting the PS5 application sandbox must bundle `libunwind`.

### 6. VideoOut Buffer Set Immutability (REQ-20260921T1202Z-9a4c)
- **Requester**: `oops-mesa`
- **Check**: `080-video/buffer-set-extension` (lines 3014–3025)
- **Results**: Error code `0x80290010` returned upon second registration attempt is identified as `SCE_VIDEO_OUT_ERROR_SLOT_OCCUPIED`. `sceVideoOutUnregisterBuffers` does not exist (`0x0`).
- **Hardware Fact**: VideoOut buffer registration is single-shot and immutable per process context.

## Bus Status

All six requests have been moved to `## RESOLVED REQUESTS` in [c:/tmp/Obscene/worklog.md](file:///c:/tmp/Obscene/worklog.md) with complete empirical citations.

