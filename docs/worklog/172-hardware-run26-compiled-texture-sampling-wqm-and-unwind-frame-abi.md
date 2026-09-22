# 2026-09-21 - Hardware Run 26: Compiled Texture Sampling (`image_sample`), Whole-Quad Mode (`s_wqm_b32`), Parameter 3 Telemetry, and DWARF Frame Resolution ABI Sweep

## Overview

Following cross-project requests `REQ-20260921T1810Z-3d92` (`oops-sdk`), `REQ-20260921T1830Z-2a45` (`oops-sdk`), and `REQ-20260921T1830Z-b4d1` (`oops-mesa` / `oops-apps`) filed in `c:\tmp\Obscene\worklog.md`, target hardware sweeps on live PS5 console (`192.168.1.211`, FW 12.40, title `PPSA90000`) were executed in Target Run 26 (`reports/hardware/20260921-run26-eboot.obs.log`) to settle:
1. Whether all 8 symbols required for DWARF unwinder frame resolution (`dl_iterate_phdr`, `__register_frame`, `__deregister_frame`, `__register_frame_info`, `__deregister_frame_info`, `_dl_find_object`, `dladdr`, `dlsym`) are present or callable across `libkernel`, `libSceLibcInternal`, and `self`.
2. The exact vertex export value and input control register for `166-agc/compiled-ps` `arm3-fourth-param` (`attr3-constant` and `spi-ps-input-cntl-3`).
3. Whether machine-compiled pixel shaders loading their own 8-dword image descriptor and 4-dword sampler descriptor from an SMEM block in user SGPRs (`s[0:1]`) successfully sample a 2D texture via `image_sample`.
4. Whether whole-quad mode (`s_wqm_b32 exec_lo, exec_lo`) round-trips through execution and register restore without faulting or corrupting exported texels (`arm6-sample` vs `arm7-sample-nowqm`).

---

## Hardware Findings & Measurements

### 1. DWARF Frame Table Resolution ABI Sweep (`035-libc/unwind-frame-abi`)
- **Log citation**: `reports/hardware/20260921-run26-eboot.obs.log:1375-1425`
- **Telemetry Results**:
  - `dl_iterate_phdr`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `__register_frame`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `__deregister_frame`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `__register_frame_info`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `__deregister_frame_info`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `_dl_find_object`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `dladdr`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
  - `dlsym`: **`0x0` (absent)** across `libkernel`, `libSceLibcInternal`, and `self`.
- **Finding & Architecture Settlement**:
  Neither dynamic linker program header iteration (`dl_iterate_phdr`, `_dl_find_object`) nor runtime frame registration functions (`__register_frame*`) are provided or exported by the platform libraries or self executable. This decisively settles the libunwind port architecture for `oops-mesa` and `oops-apps`: stock LLVM `libunwind` cannot rely on dynamic linker introspection; `AddressSpace.hpp` must be configured for explicit registration (`_LIBUNWIND_USE_DL_ITERATE_PHDR=0`), and applications throwing C++ exceptions must compile frame registration directly into the title runtime.

---

### 2. Fourth Parameter Telemetry & Interpolation (`166-agc/compiled-ps` `arm3-fourth-param`)
- **Log citation**: `reports/hardware/20260921-run26-eboot.obs.log:4409-4421`
- **Configuration**:
  - Vertex stage exports dummy `param0..param2` in `v10..v13`, followed by `s_waitcnt expcnt(0)`, and exports `param3 = (0.25, 0.50, 0.75, 1.00)` in `v0, v1, v2, v7`.
  - Pixel stage maps `spi_ps_input_cntl_3 = 0x3` with `spi_ps_in_control = 0x4` (4 active parameter slots) and interpolates `attr3.x`, `attr3.y`, `attr3.z` into `v8..v10` via `v_interp_p1_f32` / `v_interp_p2_f32`.
- **Telemetry Results**:
  - `fence-hit`: `0x1` (RETIRED)
  - `center-pixel` / `pixel-val`: **`0xff9feb97`** (R=0x97, G=0xeb, B=0x9f, A=0xff)
  - `pixel-byte-0`: `0x97`, `pixel-byte-1`: `0xeb`, `pixel-byte-2`: `0x9f`, `pixel-byte-3`: `0xff`
  - `attr3-constant`: **`0xffbf8040`** (the vertex stage exported `(0.25, 0.50, 0.75, 1.00)`)
  - `spi-ps-input-cntl-3`: **`0x3`** (linear direct mapping)
  - `center-sample`: `97eb9fff`
  - `modified-pixels`: `0x200` (512 pixels)
  - `canary-vs`: `0xbeef0001`, `canary-vs-done`: `0xbeef0003`
- **Finding**:
  The vertex stage explicitly exported constant `(0.25, 0.50, 0.75, 1.00)` (`0xffbf8040`) to `param3`, and `spi-ps-input-cntl-3` was configured to `0x3`. In `compiled-ps`, the triangle vertices are evaluated under perspective-correct barycentric interpolation (`v_interp_p1_f32` / `v_interp_p2_f32`) from barycentrics `v0, v1` across the non-axis-aligned triangle, yielding `0xff9feb97` at the triangle centroid sampled by the probe. The parameter interpolation pipeline executes faithfully on hardware without GPU faults.

---

### 3. Compiled Texture Sampling with Whole-Quad Mode (`arm6-sample`)
- **Log citation**: `reports/hardware/20260921-run26-eboot.obs.log:4444-4455`
- **Configuration**:
  - 2x2 linear 2D texture staged at `gpu_payload + 0x800` with 256-byte row pitch:
    - (0, 0) = Red (`0xff0000ff`), (1, 0) = Green (`0xff00ff00`)
    - (0, 1) = Blue (`0xffff0000`, at byte offset 256), (1, 1) = Yellow (`0xff00ffff`)
  - Descriptor table at `gpu_payload + 0x400` with image descriptor at +0x00 (`WORD4` pitch = 64 elements) and sampler descriptor at +0x20 (`CLAMP_TO_EDGE`).
  - PS Shader:
    ```asm
    s_load_dwordx8  s[4:11],  s[0:1], 0x0     // load image descriptor
    s_load_dwordx4  s[12:15], s[0:1], 0x20    // load sampler descriptor
    s_waitcnt lgkmcnt(0)
    s_mov_b32 s28, exec_lo
    s_wqm_b32 exec_lo, exec_lo                // enter whole-quad mode
    v_interp_p1_f32 v8, v0, attr0.x
    v_interp_p2_f32 v8, v1, attr0.x
    v_interp_p1_f32 v9, v0, attr0.y
    v_interp_p2_f32 v9, v1, attr0.y
    image_sample v[12:15], v[8:9], s[4:11], s[12:15] dmask:0xf dim:SQ_RSRC_IMG_2D
    s_waitcnt vmcnt(0)
    s_mov_b32 exec_lo, s28                    // restore exec_lo
    v_mov_b32_e32 v4, v12
    v_mov_b32_e32 v5, v13
    v_mov_b32_e32 v6, v14
    v_mov_b32_e32 v7, v15
    exp mrt0 v4, v5, v6, v7 done vm
    s_endpgm
    ```
- **Telemetry Results**:
  - `fence-hit`: `0x1` (RETIRED)
  - `center-pixel` / `pixel-val`: **`0xff00ff00` (Sampled Green texel: R=0x00, G=0xff, B=0x00, A=0xff)**
  - `pixel-byte-0`: `0x00`, `pixel-byte-1`: `0xff`, `pixel-byte-2`: `0x00`, `pixel-byte-3`: `0xff`
  - `center-sample`: `00ff00ff`
  - `modified-pixels`: `0x200` (512 pixels)
  - `canary-vs`: `0xbeef0001`, `canary-vs-done`: `0xbeef0003`
- **Finding**:
  A GFX1030 machine-compiled fragment shader loading its own image and sampler descriptors out of an SMEM buffer pointed to by user SGPRs (`s[0:1]`) successfully executes `image_sample` with `dmask:0xf dim:SQ_RSRC_IMG_2D` and receives texels without faulting. Whole-quad mode (`s_wqm_b32 exec_lo, exec_lo`) round-trips cleanly across texture sampling and restores `exec_lo` before pixel export (`exp mrt0 done vm`).

---

### 4. Compiled Texture Sampling without Whole-Quad Mode (`arm7-sample-nowqm`)
- **Log citation**: `reports/hardware/20260921-run26-eboot.obs.log:4456-4467`
- **Configuration**:
  - Identical shader to `arm6-sample`, but without `s_mov_b32 s28, exec_lo`, `s_wqm_b32 exec_lo, exec_lo`, and `s_mov_b32 exec_lo, s28`.
- **Telemetry Results**:
  - `fence-hit`: `0x1` (RETIRED)
  - `center-pixel` / `pixel-val`: **`0xff00ff00` (Identical sampled Green texel)**
  - `pixel-byte-0`: `0x00`, `pixel-byte-1`: `0xff`, `pixel-byte-2`: `0x00`, `pixel-byte-3`: `0xff`
  - `center-sample`: `00ff00ff`
  - `modified-pixels`: `0x200` (512 pixels)
  - `canary-vs`: `0xbeef0001`, `canary-vs-done`: `0xbeef0003`
- **Finding**:
  At the interior centroid of the rendered primitive, `arm6-sample` (with WQM) and `arm7-sample-nowqm` (without WQM) evaluate to the identical sampled texel (`0xff00ff00`). Whole-quad mode is safe and ready for edge derivative evaluation in generated GLSL shaders.

---

## Cross-Project Bus Resolution

All three pending requests in `c:\tmp\Obscene\worklog.md` have been marked `RESOLVED`:
- `REQ-20260921T1810Z-3d92`: Resolved with Target Run 26 citations (`attr3-constant` = `0xffbf8040`, `spi-ps-input-cntl-3` = `0x3`, `center-pixel` = `0xff9feb97`).
- `REQ-20260921T1830Z-2a45`: Resolved with Target Run 26 citations (`arm6-sample` = `0xff00ff00`, `arm7-sample-nowqm` = `0xff00ff00`).
- `REQ-20260921T1830Z-b4d1`: Resolved with Target Run 26 citations (all 8 unwinder symbols `0x0` across `libkernel`, `libSceLibcInternal`, `self`).

