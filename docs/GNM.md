# The platform GPU API: libSceGnmDriver

`sceGnm` is the hardware's own GPU API, which a PS4 title uses to drive the hardware directly. It
is the GPU counterpart of the CPU NID probes: does this call exist, is it reachable, what does it
do. There are two axes, and they cost very differently.

## Axis 1: the command builders

Some `sceGnm` calls do not touch the GPU. They build PM4 command packets into a caller's buffer
and return: `sceGnmDispatchInitDefaultHardwareState` and `sceGnmDispatchDirect` are two.
Calling them is as safe as any buffer-filling call (the `130-layout` class), and what they write
is the PM4 encoding a command-processor emulator parses.

`src/probe/sections/gnm.c` calls both into an oversized, guard-banded buffer and dumps the PM4
with `obs_report_buffer` - the D008-safe route, which needs the arity and a buffer larger than
the call can fill, not the struct layout. Two independent open reimplementations agree on the
arities (shadPS4 and GPCS4): `DispatchInitDefaultHardwareState` takes two arguments,
`DispatchDirect` six. `sceGnmSetCsShader` stays uncalled, because those two sources disagree on
whether it takes three arguments or four and D008 forbids calling a function whose arity is
uncertain.

The section runs wherever `libSceGnmDriver` is present (shadPS4, or a PS4); on the host build the
harness skips it as not-present. The PM4 recorded in an emulator is that emulator's own encoding,
already readable in its source; the encoding that is a finding is a real PS4's.

## Axis 2: a compute result through Gnm

Getting the GPU to run a kernel and produce a result is the larger piece. The pipeline:

1. **Produce a GCN shader.** `sceGnmSetCsShaderWithModifier` points `COMPUTE_PGM_LO` at GCN
   machine code, not SPIR-V. LLVM's AMDGPU backend compiles a compute kernel to a PS4-class GCN
   object: `clang -target amdgcn-amd-amdhsa -mcpu=gfx600 -nogpulib` produces an ELF with the
   microcode in `.text` and the kernel descriptor (COMPUTE_PGM_RSRC1/RSRC2) in `.rodata`.
   Clean-room: LLVM, no vendor compiler. The `cs_regs` array `SetCsShaderWithModifier` wants is
   `{PGM_LO, PGM_HI=0, RSRC1, RSRC2, NUM_THREAD_X, Y, Z}` - PGM_LO from where the microcode is
   mapped, RSRC1/2 from the descriptor, the thread dims chosen.
2. **Allocate GPU-visible memory.** `sceKernelAllocateDirectMemory` and
   `sceKernelMapDirectMemory` for the shader, the data buffer and the command buffer. Signatures
   cross-confirm (shadPS4, GPCS4).
3. **Build the command buffer.** `DispatchInitDefaultHardwareState`, then
   `SetCsShaderWithModifier`, then bind the data buffer, then `DispatchDirect`.
4. **Submit and read back.** `sceGnmSubmitCommandBuffers`, wait for completion (an end-of-pipe
   write to a label, polled, or `sceGnmComputeWaitOnAddress`), then read the data buffer - a
   corpus in the same shape the Vulkan path produces, feeding the same `gpuref`, `gpudiff` and
   `gpustats`.

### The input struct layouts

The function arities cross-confirm. The input struct layouts step 3 needs do not, from two
agreeing sources: the shader resource descriptor (the SRD/V# that tells the shader where the data
buffer is and how big), the user-data SGPR mapping (which SGPRs the descriptor is loaded into,
per RSRC2), and the direct-memory type constants. These are inputs the GPU reads, not output
buffers, so the `obs_report_buffer` route that keeps axis 1 D008-safe does not apply: a wrong
layout produces a dispatch that reads garbage or writes nowhere, a silent wrong result of exactly
the kind D008 forbids. Confirming them means reading the SRD and user-data handling in the
emulator shader-translators (shadPS4, GPCS4) closely enough that two sources agree, the same bar
the arities meet.

Axis 2 is PS4-specific: the PS5 uses `sceAgc` and the RDNA2 ISA, a different API and backend
target, so axis 2 serves a PS4 or shadPS4 rather than a Steam Deck (pure Vulkan, no Gnm) or the
PS5 directly.
