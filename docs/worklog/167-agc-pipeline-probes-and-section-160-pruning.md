# 2026-09-14 - AGC pipeline probes, section 160 pruning, and BigApp budget resolution

## AGC Hardware Pipeline Probes & `*GetSize` Invariant Checks (D331)

Section `166-agc` was extended to cover the complete command buffer pipeline state and
draw submission sequence:
- Depth late-Z (probe 166), Stencil state (probe 167), and Color blending (probe 168).
- Indexed draw via `DRAW_INDEX_2` with a 16-bit index buffer (probe 169).
- Textured draw via descriptor tables (probe 170).
- Primitive cull face control (probe 171).
- Color target masking via `CB_TARGET_MASK` (probe 172).

To settle conflicting figures across independent reimplementations without adopting unverified
assumptions, `*GetSize` sibling builders (`sceAgcCbNopGetSize`, `sceAgcDcbDmaDataGetSize`,
`sceAgcDcbSetIndexCountGetSize`, `sceAgcDcbSetUcRegisterDirectGetSize`) were instrumented to record
the byte allocation contract directly against what the emitter writes. Decided in `D331`.

## Pruning Section 160 (GPU Compute)

Section `160-gpu` (which attempted external Vulkan/GNM compute kernel execution) was pruned from
obSCEne. External GPU memory reads from unprivileged contexts are blocked by kernel hardening
(as documented in Section `170-gpu-capture`), and direct hardware execution is now handled
canonically by Section `166-agc` builders and `oops-sdk`.

The commercial shader tracing and dumping functionality was moved out of obSCEne into `oops-apps/src/tracer`,
implemented as an in-process injection service with x86_64 detour hooks rather than an external reader.

## Hardware Native Sweep & BigApp Budget Conflict Resolution

Hardware sweeps on 2026-09-14 verified native Prospero execution after Section 160 pruning.

### Root cause of `checkExistingApp: 0x80940010`
The initial launch attempt failed in `SceShellCore` with error `0x80940010` (`Resource temporarily unavailable`).
Inspection of `ps` revealed that a previously launched graphics test (`GLCB00001` / `gl-cube`, PID 1164)
remained running in the background, occupying the platform's single foreground BigApp execution budget.
`obscene-tool hw close-app GLCB00001` cleanly reaped the process, and `scripts/sweep.sh` was updated to
automatically ensure both `GLHW00001` and `GLCB00001` are closed prior to deploying each leg.

### Fast Sweep Results (`20260914-095045-eboot`, CORPUS=0)
- **Tally**: 240 pass, 23 warn, 29 fail, 80 skip, 0 fatal, 4 error.
- **Records**: 3,249 OBS records, terminating on `OBS|end|none` with 0 unhandled crashes.
- Section `165-gnm` skipped 2/2 as expected for the native Prospero target.
- Section `166-agc` passed 56 checks (9 warnings, 1 failure on variable-length arguments, 1 skip).
- Section `170-gpu-capture` cleanly skipped 4/4 ("external GPU/shader memory read blocked by kernel hardening; superseded by 166-agc").

### Deploy-Only Full Census Sweep (`20260914-100833-eboot`)
- **Tally**: 240 pass, 23 warn, 380 fail, 82 skip, 0 fatal, 4 error.
- **Records**: 12,262 OBS records, terminating on `OBS|end|none` with 0 unhandled crashes.
- Section `900-surface` verified all 369 censused libraries across userland.

## Operator Guides & Documentation Added
Added comprehensive operator documentation under `docs/USER_GUIDE.md` and `docs/features/`:
- `docs/USER_GUIDE.md`: Operator guide for running probes, interpreting telemetry, and generating reports.
- `docs/features/`: Dedicated guides for the three execution environments (`payload.md`, `eboot.md`, `pkg.md`).
