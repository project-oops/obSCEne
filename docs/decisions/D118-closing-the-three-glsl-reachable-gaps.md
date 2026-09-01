# D118 - Closing the three GLSL-reachable gaps the census named, and finding a denormal difference by doing it


Status: derived - three kernels added (`frexpmant`, `frexpexp`, `packhalf`), reference support
for the frexp pair, census coverage moved from 8 to 11, all gated and green.

D117's census named exactly three operations reachable from GLSL but not yet probed:
`frexp_mant`, `frexp_exp` (GLSL `frexp`) and `cvt_pkrtz` (GLSL `packHalf2x16`). This is the
census paying off immediately - a gap list is only worth having if it is acted on, so the
three kernels were added and the census now reports them covered rather than pending. It is the
tightest loop the GPU work has: the census names a gap, a kernel closes it, the reference
judges it, the diff reports what is left.

Two of the three the reference can judge exactly. frexp splits a float into a mantissa in
[0.5, 1) and an integer exponent, both exact - no rounding - so a `frexpf` computed by
power-of-two scaling (exact in floating point) is a bit-for-bit baseline. The exponent is an
`int`, so the kernel reinterprets it into the report's float slot with `intBitsToFloat` and the
reference produces the same bits; the record is bits, not a value. `packhalf` the reference
cannot judge - it packs two f32 to two f16, and an exact baseline needs the same f16 rounding
`half` needs and this build does not carry - so it is skipped like `half` and its device record
stands alone for the eventual hardware-vs-emulator diff. The GLSL spec leaves packHalf2x16's
rounding to the implementation, so the mode it reveals (RDNA's round-toward-zero, cvt_pkrtz) is
itself a finding.

The finding that came free: frexp agrees with the reference bit for bit on every normal input
and diverges on exactly one - a subnormal (`0x000116c2`). llvmpipe reports exponent -126 with
an un-normalised mantissa; IEEE frexp, which the reference computes, normalises the subnormal
and reports -132. That is a real denormal-handling difference, not a reference bug - the same
shape as `cvti` diverging out of range - and it is precisely the kind of behaviour the probe
exists to surface. The reference is left IEEE-correct so the divergence keeps flagging it,
rather than taught the device's quirk and made silent. The kernel and reference comments say so,
so it reads as an observation and not a defect.

