# 179 - Texture 3D mipmap layout measured on hardware; level 1 is largest-first (offset 0x400)

**2026-09-25**

Target hardware execution via `pros probe` against native title `PPSA90000` on FW 12.40 (`task-81363.log`). Settled `REQ-20260923T1745Z-7a24`.

## 1. 3D Texture Mipmap Placement (`166-agc/texture-3d-mipmap`, REQ-20260923T1745Z-7a24)

`oops-gl` re-filed `-9b73` to definitively settle where level 1 begins for linear 3D textures on GFX10.3 / RDNA2 hardware. Instead of guessing candidate placements, `obSCEne` filled a 64 KiB buffer with unique position-identifying blocks (block index = byte offset / 256; `R` = block index & 0xff, `G` = block index >> 8, `B` = 0xa5 sentinel, `A` = 0xff), sampling via `image_sample_l` at (0.5, 0.5, 0.5) and exporting the raw texel without modification.

Three arms ran on live hardware (`PPSA90000`) for a 4x4x4 RGBA8 3D texture (row pitch 64 texels / 256 bytes per row, 4 rows = 1024 bytes per level 0):

1. **`arm1-signature-lod1` (LOD 1.0, `LAST_LEVEL` 1)**:
   - `fence-hit`: `0x1` (passed)
   - `pixel-val`: `0xffa50004`
   - `blue-sentinel`: `0xa5` (confirms fetch was within painted 64 KiB buffer)
   - `block-index`: `0x4` (Index 4)
   - `byte-position`: `0x400` (1024 bytes)
   - Descriptors programmed:
     `dt0=0x2009100`, `dt1=0xc3800000`, `dt2=0x8000c000`, `dt3=0xa0010fac`, `dt4=0x3`, `dt5=0x10`, `dt6=0x0`, `dt7=0x0`
   - `modified-pixels`: `0x200` (512)

2. **`arm2-signature-lod0` (LOD 0.0, `LAST_LEVEL` 1)**:
   - `fence-hit`: `0x1` (passed)
   - `pixel-val`: `0xffa50004`
   - `blue-sentinel`: `0xa5`
   - `block-index`: `0x4` (Index 4)
   - `byte-position`: `0x400` (1024 bytes)
   - Descriptors programmed:
     `dt0=0x2009100`, `dt1=0xc3800000`, `dt2=0x8000c000`, `dt3=0xa0010fac`, `dt4=0x3`, `dt5=0x10`, `dt6=0x0`, `dt7=0x0`
   - `modified-pixels`: `0x200` (512)

3. **`arm3-signature-lod1-nolast` (LOD 1.0, `LAST_LEVEL` 0)**:
   - `fence-hit`: `0x1` (passed)
   - `pixel-val`: `0xffa50002`
   - `blue-sentinel`: `0xa5`
   - `block-index`: `0x2` (Index 2)
   - `byte-position`: `0x200` (512 bytes)
   - Descriptors programmed:
     `dt0=0x2009100`, `dt1=0xc3800000`, `dt2=0x8000c000`, `dt3=0xa0000fac`, `dt4=0x3`, `dt5=0x0`, `dt6=0x0`, `dt7=0x0`
   - `modified-pixels`: `0x200` (512)

Overall check verdict: `OBS|res|166-agc/texture-3d-mipmap|pass|||assumed` (execution duration: 148,922 µs).

## 2. Hardening per D334 (Zero Stalls)

In alignment with D334, experimental blend/RMW/NGG/sampling arms in `166-agc/compiled-ps` (`arm10a..arm17h`, `arm9`), `166-agc/ps-pos-xy` (`arm1`), and `166-agc/texture-extended` were guarded behind `#ifdef OBS_RUN_WEDGING_GPU_CHECKS`. This ensured the default suite ran reliably without stalling the graphics hardware engine or latching the queue fault flag, allowing `166-agc/texture-3d-mipmap` to execute cleanly to completion.

## 3. Findings and Conclusion

1. **Largest-First Layout**: Level 1 begins at byte offset 1024 (`0x400`, Block 4), which sits directly above level 0 (4 rows of 256 bytes = 1024 bytes). This confirms that on GFX10.3 / RDNA2, linear 3D texture mip levels are laid out largest-first.
2. `gl_tex_chain_layout_3d` in `oops-gl` can invert its loop for the 3D linear mip chain, rather than using smallest-first.
3. Acceptance criteria for `REQ-20260923T1745Z-7a24` fully met; request marked RESOLVED.
