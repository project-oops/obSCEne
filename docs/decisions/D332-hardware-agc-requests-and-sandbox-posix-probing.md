# D332 - Hardware AGC Requests, Clean-Room Container Generation, and Sandbox POSIX Probing

**decided** - 2026-09-14

Cross-project requests filed on 2026-09-14 from orbistoun (worklogs 539 & 541) and oops-mesa
established specific open questions regarding current-generation graphics API invariants, runtime
defaults, shader packaging, and application sandbox POSIX symbol exports. This decision documents
their implementation and safety constraints in obSCEne.

## 1. `sceAgcInit` Second Argument Sweep and Null Validation (REQ-20260914T1206Z-b7e4)

Audit of shipping implementations (orbistoun worklog 541) revealed a contradiction in assumptions:
previous obSCEne probes passed 13 ("version 13"), whereas other implementations pass 8
(`sizeof(state)`), both returning 0 on their respective firmwares.

To resolve what the second parameter represents without guessing:
- `166-agc/init` now sweeps values `0, 1, 4, 8, 12, 13, 16, 24, 32, 64` into `sceAgcInit`,
  recording the return code for each value under `OBS_FAULT_ARM`.
- It executes a NULL state buffer call under fault protection to confirm whether the function
  validates its pointer argument or faults.

## 2. Default Pipeline Registers Measurement (REQ-20260914T1245Z-e8a1)

Prior art audits confirmed that `libSceAgc` exports `sceAgcGetRegisterDefaults` (and in some
firmwares `sceAgcGetRegisterDefaults2`), which serves as the runtime's authoritative source of
pipeline state defaults (including MRT0 target configuration and context defaults).

Checks `166-agc/register-defaults` and `166-agc/register-defaults2` resolve these symbols,
invoke them on poisoned destination buffers under `OBS_FAULT_ARM`, and record:
- Return value / status.
- Written dword count.
- Pointer to internal constant state table if returned.

## 3. Submit Descriptor Layout Invariant (REQ-20260914T1206Z-a1c3)

The driver submit descriptor `obs_agc_dcb_desc` must match the hardware command processor's
expectation across compilers and optimizations. `166-agc/driver-submit-desc-layout` explicitly
verifies and records the structural layout invariants:
- `sizeof(obs_agc_dcb_desc) == 16`
- `offsetof(gpu_addr) == 0`
- `offsetof(size) == 8`
- `offsetof(flags) == 12`
- `offsetof(pad) == 13`

## 4. Linear Texture Pitch on Non-Multiple-of-64 Dimensions (REQ-20260914T1245Z-f3b2)

A contradiction existed between previous live measurements on FW 12.40 (which observed no effect
from the linear pitch field on a 16x16 texture) and multiple public implementations targeting FW 6.02
and FW 12.02 (which encode `pitch - 1` into T# word 4 for linear 2D textures when pitch exceeds width).

Because 16 is a multiple of 64 texels / 256 bytes, pitch padding did not come into play in the earlier
test. Check `166-agc/draw-textured-linear-pitch` renders a 100x16 RGBA8 texture with a row pitch of
128 texels (512 bytes), setting word 4 to `pitch - 1` (127) and word 5 to `PERF_MOD 4`. Padding texels
`[100..127]` are filled with Black (`0xff000000`) while image texels `[0..99]` are filled with
Magenta (`0xffff00ff`). Any pitch calculation discrepancy produces an unmistakable color shift in the
sampled polygon.

## 5. Clean-Room Synthetic Shader Container Generator (REQ-20260914T1221Z-c5d9 & -d2f7)

To allow obSCEne and sibling projects to synthesize compliant shader containers without borrowing
opaque binary blobs from external build trees, a clean-room C implementation was constructed:
`obs_agc_build_shader_container(...)`.
Its layout derives strictly from open-source specifications (`selfish/data/agc-shader-format.tsv`):
magic `0x34333231`, version `0x18`, header size `0x130`, target ISA `0x1013`, and self-relative
offsets to SH register pairs. Check `166-agc/create-shader` synthesizes a container and verifies it
directly against `sceAgcCreateShader`.

## 6. libkernel POSIX Pthread Symbols in Application Sandbox (REQ-20260914T1443Z-3ea7)

Section `017-posix/libkernel-pthread-symbols` sweeps 27 standard POSIX pthread functions against
`libkernel.sprx` via `sceKernelDlsym` inside the application sandbox, recording the exact resolution
frontier available to unprivileged titles.

