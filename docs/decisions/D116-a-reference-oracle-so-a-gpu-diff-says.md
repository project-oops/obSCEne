# D116 - A reference oracle, so a GPU diff says *which instructions are approximate* rather than merely that two runs differ


Status: derived - `obscene-tool gpuref` computes each kernel's near-exact result and is proven
end to end against an llvmpipe corpus.

D137 gave us `gpudiff`, but a diff on its own compares run against run. The question that
actually matters on hardware day is narrower: *where does the device stop computing the exact
answer?* An emulator can compute an exact `1.0/x`; RDNA2 cannot, and the lanes where the
device departs from exact are exactly the ones an emulator must stop computing exactly. So the
oracle recomputes every kernel on the CPU in `f64` through the host `libm`, rounds to `f32`,
and emits it as a corpus in the same `gpu`/`gpuop` shape. `gpudiff device-corpus reference`
then reads, per operand, where the device is approximate - a real finding today against
llvmpipe, and the RDNA2 approximation map the instant a Deck corpus exists.

What "reference" does and does not claim:

- **Exact where the instruction is exact.** Doubling, min/max, division, the roundings and
  fma are computed directly and the device must match them bit for bit; a divergence there is
  a device bug, not an approximation. Proven: `absf`, `double`, `floor`, `ceil`, `trunc`,
  `round`, `roundeven`, `fract`, `divf` all agree with llvmpipe to the bit.
- **A strong baseline where it is transcendental.** `libm`-through-`f64` is not a
  correct-rounding proof, but it is far tighter than any single-precision device, so the
  transcendental set (`sin`, `cos`, `tan`, the inverses and hyperbolics, `exp`/`ln`/`exp2`/
  `log2`, `rcp`, `rsq`, `powf`) diverging *is* the measurement, not a defect.
- **Never mistaken for silicon.** The emitted `gpudev` line says `reference|host libm via
  f64|reference`, so a reference corpus can never be read as a device result.
- **Skips what it cannot stand behind.** `half` (an f16 round-trip needing a correct f16
  rounding this build does not carry) has no reference and is dropped, surfacing as a
  device-only lane rather than a fabricated baseline. The skip count is reported, never
  silent.

### The false divergence it exposed, and the fix

Running device-against-reference immediately caught a defect in `gpudiff` that
device-against-device had hidden: obSCEne prints minimal-width hex (`0x0`), the oracle prints
`0x00000000`, and the string compare called identical bits a divergence - 123 of them.
The fix is to canonicalise every hex field to `0x%08x` at parse time, so a lane keys and
compares on its bits, not on how a producer chose to print them. Crucially this leaves a
*real* bit difference intact: `0xffc00000` and `0x7fc00000` are two NaNs with different sign
bits and stay distinct, because they are distinct. That is what keeps the normalisation from
hiding the genuinely interesting residue - llvmpipe does NaN-suppressing min/max and passes
some out-of-range `int(x)` conversions through unchanged, both of which the diff now reports as
honest device-behaviour observations rather than drowning them in width noise.

Built ahead of hardware on the same reasoning as D137: it is verifiable now, it turns every
future diff from "these differ" into "the device is approximate here and exact there", and it
is precisely the machinery the hardware day diffs against. Six unit tests lock the oracle's
GLSL semantics (fract of a negative, sign of zero, min's select form, a transcendental
rounding from f64) and the canonicalisation invariant.

