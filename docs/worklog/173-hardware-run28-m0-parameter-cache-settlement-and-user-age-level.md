# 2026-09-22 - Hardware Run 28: Parameter Cache Addressing (`m0` Preamble) Settlement, Verified Texel Sampling, and `sceUserServiceGetAgeLevel` Resolution

## Overview

Following cross-project requests `REQ-20260921T2015Z-7f38` (`oops-sdk` / `oops-gl`), `REQ-20260922T0215Z-4431` (`gap-analysis`), and `REQ-20260922T0215Z-1fa2` (`gap-analysis`) recorded in `c:\tmp\Obscene\worklog.md`, target hardware sweeps on live PS5 console (`192.168.1.211`, FW 12.40, title `PPSA90000`) were executed in Target Run 28 (`reports/hardware/20260922-run28-eboot.obs.log`) via fixtures `166-agc/compiled-ps` and `070-user/age-level` to settle:
1. Whether parameter cache addressing via `v_interp_p1_f32` / `v_interp_p2_f32` strictly requires the `s_mov_b32 m0, s0` preamble to evaluate exported parameter slots without reading residual parameter state (`arm8a` vs `arm8b`).
2. Whether sampling a 2D texture at known coordinates returns the exact pre-staged texel bytes (`arm9-sample-known-texel`).
3. The return code and age level written by `libSceUserService::sceUserServiceGetAgeLevel` for the signed-in user and for an invalid user ID (`-1`).
4. The feasibility of tracing commercial title workload element allocations inside obSCEne (`REQ-20260922T0215Z-1fa2`).

---

## Hardware Findings & Telemetry

### 1. Parameter Cache Addressing via `m0` (`166-agc/compiled-ps` `arm8a` vs `arm8b`)
- **Log citation**: `reports/hardware/20260922-run28-eboot.obs.log:5950-5984`
- **Configuration**:
  - The vertex shader exports constant `(0.25, 0.50, 0.75, 1.00)` (`0xffbf8040`) to both `param0` and `param3` across all 3 triangle vertices.
  - `arm8a-param0-vs-param3-nom0` (11 words, `0xb`): reads `attr0.x` and `attr3.x` into `v4` (R) and `v5` (G), without setting `m0`.
  - `arm8b-param0-vs-param3-m0` (12 words, `0xc`): identical shader with `s_mov_b32 m0, s0` (`0xbefc0300`) prepended (`ps_rsrc2 = 0u`).
- **Telemetry Results**:
  - `arm8a-param0-vs-param3-nom0`:
    - `fence-hit`: `0x1` (RETIRED)
    - `center-pixel` / `pixel-val`: **`0xff009740`**
    - `r-param0-x`: **`0x40`** (constant `0.25 * 255 = 64 = 0x40`)
    - `g-param3-x`: **`0x97`** (residual/uninitialised parameter cache read)
    - `r-equals-g`: **`0x0`** (false)
    - `shader-words`: **`0xb`** (11 words)
  - `arm8b-param0-vs-param3-m0`:
    - `fence-hit`: `0x1` (RETIRED)
    - `center-pixel` / `pixel-val`: **`0xff004040`**
    - `r-param0-x`: **`0x40`**
    - `g-param3-x`: **`0x40`**
    - `r-equals-g`: **`0x1`** (**MATCHED IDENTICAL!**)
    - `shader-words`: **`0xc`** (12 words: visibly exactly +1 word for preamble)
- **Settlement & Architectural Conclusion**:
  This empirical A/B test conclusively settles the cause of the discrepancy observed in earlier runs. `v_interp_p1_f32` and `v_interp_p2_f32` address parameter cache data via `m0`. Without initializing `m0` (`s_mov_b32 m0, s0`), `v_interp` reads stale wave parameter data, reproducing the deterministic residual `0x97` seen in Run 23 and Run 26. Prepending `s_mov_b32 m0, s0` produces `r == g == 0x40`, proving that both `param0` and `param3` evaluate identically. The earlier hypothesis attributing the `0xff9feb97` value to barycentric centroid interpolation is fully retired.

---

### 2. Verified Known Texel Sampling (`166-agc/compiled-ps` `arm9-sample-known-texel`)
- **Log citation**: `reports/hardware/20260922-run28-eboot.obs.log:5985-6014`
- **Configuration**:
  - 2x2 linear texture staged with 4 distinguishable colors: Texel 0 (0, 0) = Red (`0xff0000ff`).
  - Triangle vertices configure fixed coordinates `(u=0.25, v=0.25)` to land squarely on Texel 0.
  - PS shader carries `s_mov_b32 m0, s2` (primitive mask in `s2` following 2-dword descriptor table base in `s[0:1]`) and samples the texture using `image_sample`.
- **Telemetry Results**:
  - `fence-hit`: `0x1`
  - `center-pixel` / `pixel-val`: **`0xff0000ff`**
  - `texel-index`: `0x0`
  - `texel-expected`: `0xff0000ff`
  - `texel-bytes`: `ff0000ff`
  - `shader-words`: `0x17` (23 words)
- **Finding**:
  The compiled pixel shader sampled the exact requested texel (Texel 0, Red: `0xff0000ff`), verifying that descriptor loading, coordinate interpolation with `m0` initialized from `s2`, whole-quad mode, and linear texture addressing operate with full precision.

---

### 3. User Service Age Level Resolution (`070-user/age-level`)
- **Log citation**: `reports/hardware/20260922-run28-eboot.obs.log:3526-3532`
- **Telemetry Results**:
  - `initial-user` (`user_id = 0x1ea2f4d9`):
    - `return_code`: **`0x0`** (`SCE_OK`)
    - `age_level`: **`0x15`** (decimal 21)
  - `invalid-user` (`user_id = -1`):
    - `return_code`: **`0x80960009`** (`SCE_USER_SERVICE_ERROR_NO_SUCH_USER_ID`)
    - `age_level`: **`0xfffffc19`** (initial sentinel -999, output pointer unmodified)
- **Settlement**:
  `libSceUserService::sceUserServiceGetAgeLevel` succeeds with `0` for the logged-in user and outputs `21` (adult age level). For an invalid user ID, it returns `0x80960009` and does not alter the output pointer.

---

### 4. Workload Element Allocation Resolution (`REQ-20260922T0215Z-1fa2`)
- **Outcome**: `not-possible` (in obSCEne)
- **Resolution Statement**:
  obSCEne is an active synthetic test suite executing standalone as `PPSA90000`. It probes platform libraries by generating synthetic workloads and cannot execute commercial game executables (PPSA02664 / PPSA03416) or run dynamic instruction tracing over external title workloads. Tracing the Unity workload-array buffer allocation at `+0x18` across commercial titles requires the passive tracer payload from `oops-apps`. Per the terms of the request, this leg is declared not feasible in obSCEne and is referred to orbistoun / oops-apps.

