# D113 - The unary kernel set is widened to the rest of the transcendentals, and the input vector to the edges they strain


Status: derived - 32 kernels dispatch; the widened self-check passes over 24 lanes.

Ten more unary kernels - exp, ln, tan, asin, acos, atan, sinh, cosh, tanh, roundEven -
joining the reciprocal/sqrt/trig/rounding batch. Pure content: each is a `.comp` from the
same template, swept automatically, no code change. 32 kernels total now.

The shared input vector grew from 16 to 24, adding the values the transcendentals actually
strain: a subnormal and the smallest normal (flush-to-zero), values near the float extremes
(exp and sinh overflow, and `double` itself overflowing to infinity, which C and the device
agree on), a large angle that forces sine's range reduction, and values just off 1.0 where
inverse-trig's domain ends. Still finite throughout, so the `double` self-check stays exact -
NaN belongs only to the multi-operand edge set, where nothing asserts equality.

The sweep already reads the divergences: `asin(100000) -> NaN` (out of domain),
`exp(100000) -> +inf` (overflow). On llvmpipe these are correctly-rounded software results;
on the Deck the approximations will sit elsewhere, which is the corpus's whole purpose.

