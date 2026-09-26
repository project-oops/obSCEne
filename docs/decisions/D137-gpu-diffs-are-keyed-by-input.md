# D137 - GPU diffs are keyed by input, not lane

**Status:** decided
**Date:** 2026-09-26

`obscene-tool gpudiff` keys each lane by `(kernel, inputs)`, prints both corpora's `gpudev` first
and warns when they are the same device, and reports a lane present on only one side as a
divergence.

**Why:** the operands are a lane's identity and what a person fixing an emulator needs. Diffing a
device against itself measures nondeterminism, not a gap. A one-sided lane means the runs did not
cover the same ground.

**Rejected:** keying by lane index - breaks on any change to lane order.
