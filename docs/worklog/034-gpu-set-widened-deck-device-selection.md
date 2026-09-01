# GPU set widened, Deck device selection


Closed the three GPU follow-ups in order. Multi-operand (D134): fma/pow/min/max/division over
an edge cross-product, new `gpuop` record. Deck target (D112): device selection prefers real
silicon over llvmpipe, and `gpudev` carries the device type so a `cpu` result is gradable as
non-hardware. Widened set (D113): ten more transcendentals and eight more edge inputs,
including subnormals and range-reduction stressors.

32 kernels now, ~1000 GPU observations a run, all device-labelled. verify: ok throughout,
with the shader-drift and GPU=1 compile gates carrying it. The whole set is proven on
llvmpipe (software) and is a device swap from RDNA2 truth on a Deck.

