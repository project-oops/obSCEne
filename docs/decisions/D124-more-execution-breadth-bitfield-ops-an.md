# D124 - More execution breadth: bitfield ops, an FTZ probe, and a bit-pattern input vector for the kernels that work on bits


Status: derived - three kernels added (`bfe`, `bfi`, `ftz`), a `obs_gpu_bits` input vector the
bit kernels now use, references and census entries for all three, golden re-blessed. 45 kernels;
every gate green.

The reference and golden made the existing kernels trustworthy; this widens what is covered.
Three additions, each chosen to fit the sweep the section already has rather than force a new
input scheme:

- **Bitfield extract and insert, with a fixed field.** `bfe`/`bfi` bake in the [8, 16) field so
  extract stays unary and insert stays arity-2 - the sweep drives them unchanged, and the offset
  and width being immediates is also how RDNA encodes v_bfe_u32/v_bfi_b32. A varying field would
  have meant per-lane operands the edge cross-product cannot supply without becoming undefined
  (offset+width past 32), so the fixed field is the honest fit, not a shortcut.
- **A flush-to-zero probe.** `ftz` is `x * 0.5` - halving pushes the smallest normal into the
  denormal range, so a device that flushes returns 0 where IEEE returns the denormal. It is the
  arithmetic complement of `double` (x*2, which pushes the other way), the reference is the exact
  IEEE result, and on llvmpipe it matches bit-for-bit (no flush, consistent with the frexp
  subnormal finding) - the divergence waits on a device that flushes, exactly the ahead-of-hardware
  shape.
- **A bit-pattern input vector.** A bit operation has nothing to say about a float value, and the
  float input vector clusters every operand's bits where floats put them - never an all-ones
  word, a lone high bit, or an alternating pattern. `obs_gpu_bits` supplies those, and
  `sweep_kernel` feeds it to the bit kernels (bitcount, findmsb, findlsb, bitreverse, bfe) by
  name while everything else keeps the float edges. No schema change - the selection is one
  predicate over the kernel name. It immediately paid off: bitcount now sees 0xffffffff (32) and
  0xaaaaaaaa (16), patterns the float vector never produced.

The golden re-bless was the discipline working: the diff was inspected before blessing and every
one of its 233 changes was accounted for - the three new kernels and the four bit kernels' input
switch, and no float kernel moved - so re-capturing was blessing a change that was understood,
not adopting whatever the device said. A bug surfaced in the golden script while doing it: it
skipped the rebuild when the GPU binary merely existed, so it blessed a stale binary after a
kernel changed; fixed to always run the (incremental) build.

Deferred, and named so it is not mistaken for done: the pack/unpack conversions (packUnorm,
unpackHalf and kin) want operands in a value range the edge cross-product does not supply, so
they need a per-kernel input scheme - the next step in this direction, not part of this one.

