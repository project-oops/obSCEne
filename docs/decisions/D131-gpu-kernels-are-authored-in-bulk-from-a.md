# D131 - GPU kernels are authored in bulk from a generated list, and the GPU capability is gated


Status: derived - 17 kernels dispatch and the sweep records their results; the gate builds
GPU=1 and checks the embedded shaders for drift.

The GPU section now sweeps a whole set of compute kernels rather than one. Each is a small
`.comp` in `src/shaders`; `scripts/gen_shaders.py` embeds every one as SPIR-V and emits an
`OBS_GPU_KERNELS(X)` X-macro, which `160-gpu/kernels` expands to dispatch them all over one
shared input vector and emit `gpu` observation records. **Adding a kernel is dropping a file
and regenerating** - no C edit - the same "the list has one home" idiom the census uses.

The first batch is the unary transcendentals and conversions where hardware diverges from
exact emulation: reciprocal, inverse-sqrt, sqrt, sin, cos, exp2, log2, the rounding family,
abs, sign, an f16 round-trip and an i32 round-trip. Multi-operand ops (fma, pow, division,
min/max with NaN) need more than one input per lane and wait on a buffer-layout extension.

### The input vector is chosen for edges, not tidiness

Signed zero (reciprocal's two infinities), negatives (the domain edge of sqrt and log), an
unrepresentable fraction (1/3), pi (sine's range reduction), and near-extreme magnitudes -
all finite, so the `double` self-check stays exactly predictable. On llvmpipe the sweep
already shows the behaviour worth catching: `rcp(-0) = -inf`, `sqrt(-1) = NaN`,
`round(0.5) = 0` (ties to even). On RDNA2 the approximations will land elsewhere, and the
`gpudev` record on every line is what keeps the software result from being read as silicon.

### Gated, because a capability outside the gate rots

`verify.sh` gains two conditional stages, guarded like the compatibility table so a machine
without the GPU toolchain still passes the rest: a shader-drift check
(`gen_shaders.py --check`, when glslangValidator is present) and a `GPU=1` compile (when the
Vulkan headers are). Compile, not run - it proves the backend and section still build under
`-Werror -Wconversion` without depending on a working rasteriser. The dispatch is exercised
by hand against llvmpipe and, in time, on the Deck.

