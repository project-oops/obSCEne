# Multi-operand GPU kernels


The sweep grew from unary to any arity (D134). fma, pow, min, max and division now dispatch
over the cross-product of an edge operand set (+/-0, 1, 3, +/-inf, NaN) - 49 pairs each for
binary, 343 triples for fma - emitting a new `gpuop` record that carries the output and each
input without disturbing the unary `gpu` format. 22 kernels, 539 multi-operand observations a
run. Arity is a directive in the shader, so a new multi-operand kernel is still a .comp file
plus a regenerate. verify: ok.

