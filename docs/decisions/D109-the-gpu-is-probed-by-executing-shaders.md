# D109 - The GPU is probed by executing shaders and reading the result bits, not by calling an API


**The GPU is probed by executing shaders and reading the result bits, not by calling an
API. The oracle is the device, and the device name is its provenance.**

Status: derived - a compute dispatch runs end to end and the pipeline self-check passes.

obSCEne now has a `160-gpu` section that dispatches a compute shader over known inputs and
reads the results back (`obs_gpu_run`). This answers the one question the behavioural
sections cannot: not what a *function* returns, but what an *instruction* computes - which
is what an emulator's shader translation must be checked against and cannot answer about
itself.

### Two layers, and this is the one that matters

"GPU calls" splits in two. The Sony GPU *API* (`sceGnm*`/`sceAgc*`) is ordinary functions,
probed the CPU-NID way, and is console-only. GPU *execution* - run a kernel, read the bits -
is not a function call at all, and it is where shader gaps are closed. This section is the
execution layer. On a Steam Deck's gfx1033 (and a console) the read-back bits are RDNA2
ground truth; the API layer is a later, smaller, console-only addition.

### The backend split, same as net and sink

`gpu_vulkan.c` (host and Deck native) talks standard public Vulkan compute - no vendor
anything. `gpu_gnm.c` (console module) refuses until the Gnm/Agc submission format is
confirmed, exactly as `net_target.c` began. Chosen by the build's source list, never a
`#if`. The section compiles in every build and skips when the backend is absent, so the
capability never silently disappears and the check count does not move with the flag.

### Provenance is the whole point

A result from `llvmpipe` (Mesa's software Vulkan, in the build VM) and one from real RDNA2
are the *same code* on a different device - and must never be confused. Every run emits a
`gpudev` record naming the backend and device, and the check provenance stays `derived`
rather than `hardware` because whether a result is silicon is a run-time fact about the
device, not a compile-time property of the check. The pipeline self-check ran on llvmpipe
and reported `gpudev|vulkan|llvmpipe (LLVM 20.1.2)`; the Deck will report its gfx1033, and
only then is it hardware.

### Proven, and what it showed

The `double` kernel (exact `x*2`) passed - the instrument is sound. The `rcp` kernel emitted
`1/3 -> 0x3eaaaaab` on llvmpipe (correctly-rounded). Real RDNA2's `v_rcp` refinement may
land a ULP away, and that difference - recorded, device-labelled, diffed by the consumer -
is the reason the oracle exists. Shaders are embedded as SPIR-V by `scripts/gen_shaders.py`
into a committed header, so the build needs no shader compiler; only regeneration does.

Two Werror snags worth remembering: Vulkan under `-Wconversion` needs deliberate casts on
every size, and a generated banner containing `src/shaders/*` tripped `-Wcomment` on the
`/*` inside a block comment.

