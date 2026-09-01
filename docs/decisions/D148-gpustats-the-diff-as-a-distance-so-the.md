# D148 - `gpustats`: the diff as a distance, so the approximation is a number, not a yes/no


Status: derived - `obscene-tool gpustats device reference` reports per-kernel ULP statistics,
ranked worst first. Proven on the llvmpipe golden against its reference, and it surfaced a real
finding on the first run.

`gpudiff` answers "does this lane match" - right for the exact operations and the regression
gate. The transcendentals need the other question: they are *allowed* to be approximate, so what
matters is by how much. `gpustats` reduces a corpus-against-reference diff to a ULP distance per
kernel - lanes, exact, diverged, max and mean ULP, and the worst lane as an example - ranked so
the worst approximation is on top. On llvmpipe today it names that rasteriser's error; on a
Deck's corpus it is RDNA2's; against an emulator's corpus it is the shader gap, measured.

The first run found the shape of the problem: the well-behaved SFU ops sit at 1-6 ULP (rsq 1,
exp2 2, exp and cosh 6), and the catastrophic divergence is large-argument trig - sin/tan/cos of
~1e20 return a value on the wrong side of the range reduction entirely. That is exactly the kind
of thing the tool exists to rank.

Two choices the first output forced, each a small piece of numerical honesty:

- **Signed zeros are zero ULP apart.** The ordering maps +0 and -0 to the same point, because
  they compare equal as numbers and the sign of a zero is a bit fact gpudiff already catches, not
  an approximation error. The naive `0x80000000 - bits` transform put them 2^32 apart and made
  min/max/pow/sinh look catastrophically wrong on their signed-zero lanes; the mirror-magnitude
  transform fixes it.
- **Integer-output kernels are omitted, by name.** ULP is a distance between floats; `cvti`,
  `bitcount`, `bfe`, the packs and the rest output an integer, a bitfield or a packed word, and
  reading those bits as a float gives a number nobody computed. They are gpudiff's domain
  (exact-or-bug), so `gpustats` skips them and says so. The list is by name because the record
  carries no type tag - a comment marks that a new integer-output kernel must be added to it.

Not a gate: it is an analysis tool, like `gpuref`, so it prints and exits 0. Its correctness is
in the unit tests (the ULP of adjacent floats, of signed zeros, of a NaN against a number), which
the tool suite runs; it needs no `verify.sh` step because it produces no committed artifact that
could drift.

