# GPU kernels, bulk-authored


The GPU sweep went from one kernel to seventeen (D131). `scripts/gen_shaders.py` now emits
an `OBS_GPU_KERNELS` X-macro alongside the embedded SPIR-V, and `160-gpu/kernels` expands it
to dispatch every shader over a shared edge-value vector - so a new kernel is a `.comp` file
and a regenerate, nothing more. First batch: the unary transcendentals, rounding family, and
f16/i32 round-trips. 17 kernels x 16 lanes = 272 observations a run, all carrying the device
name. verify: ok, with two new conditional GPU stages (shader-drift check and a GPU=1
compile) so the capability is in the gate rather than left to rot.

### Surprises
- The sweep on llvmpipe already reads like an oracle: `round(0.5) -> 0` (ties to even),
  `rcp(-0) -> -inf`, `sqrt(-1)/log2(-1) -> NaN`. These are the exact divergences the corpus
  exists to pin - and they will read differently on RDNA2, which is the point.

