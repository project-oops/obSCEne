# D131 - GPU kernels are files, and the kernel list is generated

**Status:** decided
**Date:** 2026-09-26

Each GPU kernel is a `.comp` file under `src/shaders/` declaring its arity. `obscene-tool shaders`
embeds them as SPIR-V in a committed header and emits one X-macro list that the section sweep and
the protocol's `gpu` verb both expand. Multi-operand kernels report through `gpuop` records, leaving
the unary `gpu` record unchanged.

**Why:** adding a kernel is adding a file, with no second table to keep in step. The build needs no
shader compiler; only regeneration does. A new record keeps an existing field's meaning fixed.

**Rejected:** hand-written dispatch tables - drift from the kernel set. Widening the `gpu` record -
changes a field's meaning under existing parsers.
