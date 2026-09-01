# D112 - GPU device selection prefers real silicon, and the device type is recorded as gradable provenance


Status: derived - the selection compiles and the `gpudev` record now carries the type.

The Vulkan backend used to take the first enumerated device. A Steam Deck with the Mesa
software driver also present enumerates both its APU and llvmpipe, and first-is-fine would
sometimes pick llvmpipe - reporting CPU results for a run meant to measure gfx1033, the exact
provenance failure the GPU work exists to prevent. Selection now takes the first non-CPU
device and falls back to llvmpipe only when that is all there is (the build VM).

`gpudev` gains a third field, the device type: `integrated`, `discrete`, `cpu`, `virtual` or
`other`. That makes provenance machine-gradable - a consumer rejects a `cpu` result for a
hardware claim without recognising device-name strings. On llvmpipe it reads
`gpudev|vulkan|llvmpipe (...)|cpu`; the Deck's APU will read `integrated`. New field appended,
so the contract holds.

The `deck` target itself (host-shaped, `GPU=1`, served via `--serve`) already existed; this
is what makes it reach the right device. The RDNA2 run still awaits the hardware - proven on
llvmpipe, a device swap away from silicon.

