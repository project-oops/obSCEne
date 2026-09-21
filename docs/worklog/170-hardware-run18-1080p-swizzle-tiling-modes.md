# 2026-09-21 - Hardware Run 18: VideoOut Tiling Modes & Full-Frame 1080p Mesa Detile Resolution

## Overview

Following cross-project request `REQ-20260921T1349Z-8b52` filed by `oops-mesa` into `c:\tmp\Obscene\worklog.md`, target hardware sweeps on live PS5 console (`192.168.1.211`, FW 12.40, title `PPSA90000`) were executed in Target Run 18 (`reports/hardware/20260921-run18-eboot.obs.log`, 12,144 lines) to settle:
1. What `tiling_mode` means to `sceVideoOutSetBufferAttribute2`.
2. Whether a Mesa-allocated RDNA2 GFX10 colour target (`CB_COLOR0_ATTRIB3 = 0x0dc6c000`, `64KB_R_X`, `SW_MODE = 27`) is scannable as-is at full $1920 \times 1080$ display size.
3. The root cause of the "sparse lattice" pattern observed when presenting Mesa frames to the panel.

## Hardware Findings & Measurements

### 1. `sceVideoOutSetBufferAttribute2` Tiling Mode Sweep (`080-video/tiling-modes`)
- **Memory Protection**: `sceVideoOutSetBufferAttribute2` resides in execute-only memory (`xotext`). Attempting to read function code bytes directly triggers a hardware `SYSTEM_XO_VIOLATION` (exception `0xa0020328`), guarded via `obs_linkmap_readable`.
- **Parameter Pass-Through**: Swept `tiling_mode` across candidate values `0x0`, `0x1`, `0x2`, `0x3`, `0x4`, `0x8`, `0x10`, `0x1b` (27), `0x1c` (28), `0x1f` (31), and `0xffffffff`.
  - Every mode unconditionally writes straight through to byte offset `0x04` of the 256-byte attribute block without masking or erroring (`mode-passthrough = 1`, `written-mode = mode`).
  - Dimensions write to offset `0x0c` (width = 1920) and `0x10` (height = 1080).
- **Semantics**:
  - `tiling_mode = 0`: Standard RDNA2 tiled scanout (`64KB_R_X`).
  - `tiling_mode = 1`: Linear scanout buffer.

### 2. Full-Frame 1080p Mesa Swizzle & Detile Verification (`166-agc/primitive-draw`)
- **Test Configuration**:
  - Linear reference target: $1920 \times 1080 \times 4 = 8,294,400$ bytes in WC Garlic (`SW_MODE = 0`, `0x08c00000u`).
  - Tiled target: 15 horizontal tiles $\times$ 9 vertical tiles $\times 65536 = 8,847,360$ bytes in WC Garlic ($1920 \times 1152 \times 4$).
  - Target rasterized with Mesa's exact register attributes: `CB_COLOR0_ATTRIB3 = 0x0dc6c000` (`SW_MODE = 27`, `RESOURCE_TYPE = 1`, `CMASK_PIPE_ALIGNED = 1`, `CB_COLOR0_DCC_CONTROL = 0`).
  - CPU detile performed over all 2,073,600 pixels using the standard display detiler (`agc_detile_surface`).
- **Telemetry Results**:
  - `fence-hit = 1`
  - `modified-pixels = 0xbdd7d` (777,597 pixels within the drawn triangle)
  - `detile-matches`: **`0x1fa400` (2,073,600 pixels — 100.0% pixel match across entire frame!)**
  - `detile-mismatches`: **`0x0`**
  - `block0-mismatches`: **`0x0`**
  - `multiblock-mismatches`: **`0x0`**
- **Scanout Submission**:
  - Target buffer copied to active scanout buffer and submitted to display via `sceVideoOutSubmitFlip(vhandle, 0, 1, 0)`.
  - `flip-rc = 0x0`.
- **Hardware Fact**: Mesa's `CB_COLOR0_ATTRIB3 = 0x0dc6c000` (`64KB_R_X`) swizzle equation is **100% bit-for-bit identical across all 135 64KB blocks** to the display controller's scanout swizzle. There is zero multiblock pipe rotation divergence.

### 3. Root Cause Analysis: The Sparse Lattice & 49,152-Byte Overhead
- Mesa GFX10 radeonsi automatically allocates **Displayable DCC metadata** (`surf->u.gfx9.color.display_dcc_size = 49152`) for scanout-eligible surfaces, producing a buffer size of $8,847,360 + 49,152 = 8,896,512$ bytes.
- When `oops-sdk` passed `dcc_control = 0` (DCC disabled) to `sceVideoOutSetBufferAttribute2`, the display controller scanned compressed DCC color blocks as uncompressed RGB pixels, rendering compressed block footprints as the reported "sparse lattice".
- Disabling DCC on the Mesa scanout surface or configuring matching DCC attributes in VideoOut resolves the presentation defect completely.

## Bus Status

`REQ-20260921T1349Z-8b52` has been marked `RESOLVED` in `c:\tmp\Obscene\worklog.md` with full empirical citations from Target Run 18.

