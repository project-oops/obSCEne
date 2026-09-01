# D117 - A GPU ISA surface census, so "probe every GPU op" has a statement of what every GPU op is


Status: derived - `scripts/gen-gpu-surface.py` generates `docs/GPU_SURFACE.md`, gated by
`--check` in `verify.sh`, and validated against LLVM's `IntrinsicsAMDGPU.td` on the build VM.

obSCEne has a CPU surface census - thousands of NIDs, so the report can say what the platform
offers and what the probe covers. The GPU had 32 hand-picked kernels and no statement of what
they covered *of*. This is that statement: the AMD hardware's special math operations - the
transcendental SFU, the IEEE division sequence, float decomposition, the median op and the
packing conversions - each marked with how obSCEne reaches it, or that it does not. It is the
map the hardware day needs, because "run every GPU op and diff it" presupposes a list of every
GPU op, and without one, coverage is whatever occurred to whoever wrote the kernels.

What it establishes: of 41 intrinsic operations, 33 exist on the RDNA2 target, and obSCEne
covers 8 of them today (the transcendentals). Three more are reachable from GLSL and not yet
probed - `frexp_mant`, `frexp_exp` (GLSL `frexp`) and `cvt_pkrtz` (GLSL `packHalf2x16`) - which
are the next kernels to add. Twenty-two are reachable only through hand-written SPIR-V (the
division primitives, the norm/int packs, the cube-face ops), which is the controlled-ISA
workstream. Eight are CDNA-only and absent on the Deck, marked so rather than counted as a gap
the target could ever close.

Three choices that matter:

- **The in-repo table is the source of truth, the toolchain is the cross-check.** The census
  generates from the classification table in the script, so the document reproduces on any
  machine with no LLVM installed - the same reproducibility the other generated files have.
  `IntrinsicsAMDGPU.td`, when the VM has it, only validates: every name we name must still
  exist in it, and a new scalar-math intrinsic it gains that we have not classified is flagged.
  This is `counts.py` cross-checking the census against `platform.h`, applied to the GPU.
- **Reachability is stated, not assumed.** A GLSL kernel cannot name `v_rcp_f32`; the driver
  chooses the lowering. So each op says how obSCEne can actually exercise it - covered, GLSL,
  hand-SPIR-V, or not-on-this-part - rather than pretending the intrinsic list is a to-do list
  of things a compute shader could call.
- **Standard IEEE ops are kept out of the intrinsic surface.** `floor`, `add`, `fma`, `min`,
  the roundings - LLVM models these as generic float operations, not AMDGPU intrinsics, so
  they carry no `int_amdgcn_` name. obSCEne covers them and the census lists them under base
  ISA, separately, so the intrinsic surface stays exactly the AMD-specific operations.

The check earned its place immediately: the first generation named `int_amdgcn_ldexp`, which
LLVM 18 has replaced with the generic `llvm.ldexp` - the drift validation caught the stale
classification against the toolchain, and `ldexp` moved to base ISA where a generically-modelled
op belongs.

