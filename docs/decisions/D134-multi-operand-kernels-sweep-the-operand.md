# D134 - Multi-operand kernels sweep the operand cross-product, in a new record that leaves the unary one untouched


Status: derived - fma, pow, min, max and division dispatch and record 539 lanes.

The GPU sweep now handles kernels with more than one input. A kernel declares `obs-arity: N`
in its `.comp`; `gen_shaders.py` reads it and emits `X(name, arity)`; `sweep_kernel` lays out
the lane buffer as `[in0..in_{k-1}, out]` and dispatches over the cross-product of an edge
operand set. Adding a multi-operand kernel is still just a `.comp` file and a regenerate.

### The operand set is bits, and it is chosen for the edges

`+0, -0, 1, 3, +inf, -inf, NaN` - as raw bit patterns, because the operands that make these
ops diverge cannot be written as finite decimals. Binary kernels sweep all 49 pairs, ternary
all 343 triples. That is deliberately narrow and edge-aimed rather than broad: min/max
propagation of NaN, division's `0/0` and `x/0`, and fma's fused-vs-two-step rounding all live
at these points. Widening to a denser space is a later volume step.

### A new record, because the old one's meaning is fixed

`gpuop|kernel|lane|output|in0|in1|...` - the single output first, the inputs trailing. Not a
change to `gpu`: that record's input field is a single value and a consumer already parses
it, so widening it would change a field's meaning, which the contract forbids. A separate
record carries any arity and leaves the unary format exactly as it was. The layout was
confirmed on llvmpipe: `divf 0/0 -> NaN`, `1/0 -> +inf`, `1/3 -> 0x3eaaaaab`.

