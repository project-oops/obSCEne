# 178 - Blend quad lattice census measured on hardware; SEPARATE_ALPHA_BLEND eliminated

**2026-09-23**

Target hardware sweep via `pros probe` against native title `PPSA90000` on FW 12.40 (`C:\tmp\obscene-sweep.log`). Settled `REQ-20260923T2015Z-5b8e`.

## 1. Blended Draw Quad Lattice Census (`166-agc/compiled-ps`, REQ-20260923T2015Z-5b8e)

`oops-gl` and `gl1-probe` identified that blended draws on PS5 RDNA2 hardware produce a 2-pixel alternating pattern / lattice, where even/even coordinates are correct while other coordinates produce incorrect values. In Update 4, `gl1-probe` highlighted `arm16d-blend-census-noseparate` as the key discriminator: testing whether `SEPARATE_ALPHA_BLEND` (bit 29 in `CB_BLEND0_CONTROL`) was the root cause.

Four census arms ran on live hardware (`PPSA90000`), evaluating a full 64x64 viewport (4,096 pixels) rendered into `CB_COLOR0_INFO = 0x88a8` with `BLEND_BYPASS = 0`, uniform shader export `(0.25, 0.50, 0.75, 0.0)` over clear `0xffff0000`:

1. **`arm16a-blend-census-one-one` (`CB_BLEND0_CONTROL = 0x61010101`, SEPARATE=1)**:
   - `fence-hit`: `0x1`, `fence-val`: `0xbeefcafe`
   - `region-total`: 4096
   - `row32-bits-lo` / `row32-bits-hi`: `0x55555555` / `0x55555555`
   - `b-even-even`: 1024 (100% of even/even)
   - `b-other-parity`: 1024 (33.3% of other parities)
   - `b-lane0`: 1024
   - `b-other-lanes`: 1024
   - `g-correct`: 2048 (50%)

2. **`arm16d-blend-census-noseparate` (`CB_BLEND0_CONTROL = 0x41010101`, SEPARATE=0)**:
   - `fence-hit`: `0x1`, `fence-val`: `0xbeefcafe`
   - `region-total`: 4096
   - `row32-bits-lo` / `row32-bits-hi`: `0x55555555` / `0x55555555`
   - `b-even-even`: 1024 (100%)
   - `b-other-parity`: 1024 (33.3%)
   - `b-lane0`: 1024
   - `b-other-lanes`: 1024
   - `g-correct`: 2048 (50%)
   - **Result**: `arm16a` and `arm16d` produce the **exact same lattice** down to the bitmask. `SEPARATE_ALPHA_BLEND` is NOT the cause of the lattice.

3. **`arm16b-blend-census-one-zero` (`CB_BLEND0_CONTROL = 0x61010001`, `GL_ONE, GL_ZERO`)**:
   - `fence-hit`: `0x1`, `fence-val`: `0xbeefcafe`
   - `region-total`: 4096
   - `row32-bits-lo` / `row32-bits-hi`: `0xaaaaaaaa` / `0xaaaaaaaa`
   - `b-even-even`: 0 (0%)
   - `b-other-parity`: 2016 (65.6%)
   - `b-lane0`: 0
   - `b-other-lanes`: 2016
   - `g-correct`: 2048 (50%)
   - **Result**: Inverts column parity from `0x55555555` to `0xaaaaaaaa`.

4. **`arm16c-unblended-census` (`CB_BLEND0_CONTROL = 0x00000000`, unblended control)**:
   - `fence-hit`: `0x1`, `fence-val`: `0xbeefcafe`
   - `region-total`: 4096
   - `row32-bits-lo` / `row32-bits-hi`: `0x00000000` / `0x00000000`
   - `g-correct`: 4096 / 4096 (100% clean)
   - `center-pixel`: `0x004080bf` (LE uint32)
   - **Result**: Entire surface is completely clean and uniform across all 4,096 pixels. Geometry, NGG rasterizer, viewport, memory mapping, and export pipelines are 100% solid; the phenomenon is purely within the blend hardware block.

## 2. Hardening Fence Waits for System App Transitions

During the initial sweep, `166-agc/texture-extended`'s `arm4-depth-fail` hit a timeout when SceShellUI (`/app0/eboot.bin [system]`, category `shell_ui`) launched concurrently, causing dropped frames and a ~2-second pause. `check_agc_texture_extended_sub` has been hardened by extending the wait iteration bound from 25,000 (2.5s) to 100,000 (10s) with early-exit at 80,000 iterations, ensuring background system events do not trip false fence failures.

## Data delivered along the way

- `REQ-20260923T2015Z-5b8e`: Arm 16 resolved; Arm 17 measured on hardware and diagnosed in `C:\tmp\Obscene\worklog.md` with full quad telemetry and Mesa architectural analysis.
- `REQ-20260923T1810Z-7d42`: RESOLVED in `C:\tmp\OopsSdk\worklog.md` (`MB_CUR_MAX`, `aligned_alloc`, `<sys/types.h>`).

## 3. Arm 17 Census & Root Cause Settlement (Update 10)

Hardware sweep `20260923-210700` ran Arm 17 across 8 register variations:
- Clear: `0x11223344` (`[B=0x44, G=0x33, R=0x22, A=0x11]`).
- PS export: `(R=0.37647f, G=0.25098f, B=0.12549f, A=0.50196f)` $\to$ `[B=0x20, G=0x40, R=0x60, A=0x80]`.
- Expected `GL_ONE, GL_ONE`: `0x91827364` (`[B=0x64, G=0x73, R=0x82, A=0x91]`).

**Hardware Results across all 8 arms**:
- `lane00` (x=32, y=32): `0x91827364` (100% byte-exact)
- `lane01` (x=32, y=33): `0x91827364` (100% byte-exact)
- `lane10` (x=33, y=32): `0xff6491xx` (corrupted)
- `lane11` (x=33, y=33): `0xff6491xx` (corrupted)
- Ruled out: `DISABLE_DUAL_QUAD`, `RESOURCE_TYPE = 2D`, `SIMPLE_FLOAT`, `SX_MRT0_BLEND_OPT`, and linear `SW_MODE = 0`.

**Architectural Root Cause**:
Mesa `ac_choose_spi_color_formats` (`ac_shader_util.c:672-693`) requires `SPI_SHADER_FP16_ABGR` (4) for `COLOR_8_8_8_8` targets when RB+ is enabled (`info->rbplus_allowed = true` on GFX10.3+). Both `obscene` and `oops-sdk` currently export uncompressed 32-bit floats (`SPI_SHADER_32_ABGR` = 9). The hardware CB blender / RB+ unpacker expects packed 16-bit half-floats for 8-bit blend operations, scattering 16-bit words across 16-byte memory burst transactions.

