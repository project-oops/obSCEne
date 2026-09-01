# GPU compute probe - Vulkan backend, one kernel end to end


The GPU workstream's first milestone: obSCEne dispatches a compute shader and reads the
result bits back (D109). Proven on llvmpipe in the build VM - plumbing correct - and the
identical code runs on a Deck's RDNA2 for real.

Landed: `gpu.h` interface; `gpu_vulkan.c` (host/Deck) and `gpu_gnm.c` (refusing console
stub), source-list split; `src/shaders/*.comp` + `scripts/gen_shaders.py` embedding SPIR-V
into a committed header; `160-gpu` section with a `pipeline` self-check (exact `x*2`,
asserted) and a `rcp` observation (emits input/output bits per lane); `gpudev`/`gpu` records
with device provenance; `GPU=1` build flag. All four build combinations compile clean,
verify: ok.

### Surprises
- A standalone Vulkan spike ran first and confirmed the VM can dispatch compute on llvmpipe
  before any integration - the whole GPU direction rested on that being possible, and it is.
- counts.py counts check rows as text, so the first gpu.c with two `#if`/`#else` check
  tables would have counted three checks. Restructured to one table with the bodies varying
  under the flag - stable count, cleaner diffs.
- Vulkan is verbose under the project's `-Wconversion -Wsign-conversion -Werror`, and a
  `/*` inside a generated comment (`src/shaders/*`) failed `-Wcomment`. Both are the strict
  gate doing its job.

