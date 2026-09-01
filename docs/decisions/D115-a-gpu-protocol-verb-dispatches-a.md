# D115 - A `gpu` protocol verb dispatches a compiled-in kernel over the socket - the interactive oracle, with no rebuild


Status: derived - driven end to end over a live socket; result bits confirmed.

`CMD|seq|gpu|<kernel>|<operand>...` runs one of the probe's embedded kernels over the
supplied operands and streams back the same `gpu`/`gpuop` records a report emits, `gpudev`
first. That turns the loop from rebuild-and-redeploy into ask: point a driver at a Deck and
interrogate any of the 32 kernels at any inputs, live.

### Why it is safe to expose where `call` and `blob` are guarded

It runs **only named, compiled-in shaders** - the driver picks which known kernel and what
inputs, never arbitrary code. So unlike `call` (any address) and `blob` (uploaded code), it
executes nothing the build did not already contain, and it is announced whenever the GPU
backend is up rather than held behind a deliberate switch.

### The details that keep it honest

- **Announced only when a backend is actually present** - a GPU build on the console's
  refusing stub advertises no `gpu`, the capability rule the list exists to keep.
- **A graceful dispatch failure is not a death.** A Vulkan error with the probe still alive
  answers `done|returned|0` - zero lanes, no records - which a driver tells from success by
  the lane count, and from a real crash by the fact that a `done` arrived at all.
- **The kernel registry has one home.** `obs_gpu_arity`/`obs_gpu_dispatch_named` expand the
  same generated `OBS_GPU_KERNELS` list the section sweeps, so a new kernel is reachable over
  the socket the moment it is added - no second table.

Confirmed on llvmpipe: `gpu rcp 1 2 3` returns `1, 0.5, 0x3eaaaaab`; `gpu fmaf 2 3 1` returns
`7.0`; an unknown kernel is refused. Documented in `docs/PROTOCOL.md`, captured in
`docs/examples/protocol/11-gpu.txt`, and the checker knows the verb.

