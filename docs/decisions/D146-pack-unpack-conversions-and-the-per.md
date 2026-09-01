# D146 - Pack/unpack conversions, and the per-kernel input scheme they needed


Status: derived - five kernels (`packunorm`, `packsnorm`, `unpackunorm`, `unpacksnorm`,
`unpackhalf`), operand-set selection by kernel name in both sweep paths, references and census
entries for all five, golden re-blessed. 50 kernels; all match the reference bit-for-bit.

D124 deferred these because they need operands in a value range the edge cross-product does not
supply: packUnorm clamps to [0,1], packSnorm to [-1,1], and feeding them the bit edges (the two
infinities, a NaN) would probe the clamp's undefined corner and nothing else. So this first
builds the input scheme, then the kernels.

The input scheme is a name predicate, not a schema change. The unary path already chose the
bit-pattern vector for bit kernels by name (D124); this adds the same idea to the multi-operand
path - `obs_gpu_operands(name)` returns the operand set, a [0,1]-with-clamp-tails set for
packUnorm, a [-1,1] one for packSnorm, the bit edges for everything else. The unpack kernels are
unary and take a packed word, so they simply join the bit-pattern list. One predicate per path,
no new field on the generated kernel list - the same restraint the bit-kernel selection kept.

Two things worth recording:

- **unpackHalf has a reference; packHalf does not.** f16 -> f32 loses nothing - every half is a
  representable float - so `unpackhalf` is decoded exactly and the device must match it, whereas
  packing *to* f16 needs a rounding this build does not judge (why `half`/`packhalf` are skipped).
  The asymmetry is real, not an oversight, and the decoder is a bit-exact `f16_to_f32`.
- **The packing rounds where the device rounds.** packUnorm/packSnorm scale to an integer and
  round; the reference does it in f32 (mirroring the device's intermediate) with ties-to-even,
  and the operand sets were chosen to avoid half-integer ties, so the match is exact. A tie left
  in deliberately would be a rounding-mode probe - a divergence there would be an observation,
  the same footing as the transcendentals - but that is a later choice, not smuggled in here.

The census gained real coverage: `cvt_pknorm_u16`/`cvt_pknorm_i16` were `isa` (unreachable) and
are now covered by packUnorm/packSnorm - the packing intrinsics were reachable from GLSL all
along, they just needed the kernel. The golden re-bless was again inspected first: 133 new lanes,
all five new kernels, no existing kernel moved (the multi path's default operand set is unchanged
for everything else).

