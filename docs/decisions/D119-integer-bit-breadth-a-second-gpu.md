# D119 - Integer/bit breadth: a second GPU surface the float census did not map, and the kernels for its first four operations


Status: derived - four kernels added (`bitcount`, `findmsb`, `findlsb`, `bitreverse`), exact
references, a new census table, all gated and green. Every lane agrees with the reference bit
for bit.

The intrinsic census (D117) maps the float special operations - the SFU, division and
conversion surface. It does not map the integer and bit operations, which are a real and
separate part of what a shader executes: population count, find-first-bit, bit reversal, and
the bitfield ops. This is the first four of that surface, the ones with an exact reference and
an immediate GLSL builtin: `bitCount` (v_bcnt), `findMSB`/`findLSB` (v_ffbh/v_ffbl) and
`bitfieldReverse` (v_bfrev).

Three choices that matter:

- **The input lane is read as bits, not as a float.** A bit operation has nothing to say about
  a float *value*; it operates on the 32-bit pattern. The kernels take `floatBitsToUint(x)`, so
  the section's existing float input vector doubles as a spread of bit patterns (0, the sign
  bit, 1.0's pattern, the infinities and NaNs), and the reference reads the operand back with
  `to_bits()`. A dedicated bit-pattern input vector can come later; the float edges already
  exercise a useful range.
- **The result is an int reinterpreted, and -1 is the all-ones word.** findMSB/findLSB return
  -1 for a zero word; the kernel writes `intBitsToFloat` and the reference emits the same bits,
  so the "no bit set" answer is `0xffffffff` on both sides rather than a special case.
- **A third census table, not a widening of the float one.** These are generic LLVM operations
  (ctpop, ctlz, cttz, bitreverse) that lower to dedicated RDNA integer instructions, so they
  are named by the instruction and kept in their own "Integer / bit base ISA" table - the float
  intrinsic surface stays exactly the AMD-specific float operations, and the integer surface
  grows beside it as kernels are added.

Exact throughout: all four agree with the reference bit for bit on every lane, which is the
point - an integer op that diverged would be a device bug, not an approximation, and now there
is a check that would say so.

