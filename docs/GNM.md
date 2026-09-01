# The platform's GPU API: libSceGnmDriver, both axes

The GPU section (160-gpu) probes what the device *computes*, through public Vulkan - the path a
Steam Deck takes, needing no vendor library. This is the hardware's own GPU API, `sceGnm`, which
a PS4 title uses to drive the hardware directly. It is the GPU counterpart of the CPU NID
probes: "does this call exist, is it reachable, what does it do."

There are two axes, and they cost very differently.

## Axis 1 - the command builders (built: `165-gnm`)

Some `sceGnm` calls do not touch the GPU at all: they build **PM4 command packets** into a
caller's buffer and return. `sceGnmDispatchInitDefaultHardwareState` and `sceGnmDispatchDirect`
are two of these. Calling them is as safe as any buffer-filling call (the 130-layout class), and
what they write is the PM4 encoding a command-processor emulator must parse.

`src/probe/sections/gnm.c` calls both into an oversized, guard-banded buffer and dumps the PM4 with
`obs_report_buffer` - the D008-safe route, which needs the arity and a buffer larger than the
call can fill, not the struct layout. Confirmed by two independent open reimplementations that
agree exactly (shadPS4 and GPCS4): `DispatchInitDefaultHardwareState` takes two arguments,
`DispatchDirect` six. `sceGnmSetCsShader` is deliberately left uncalled - those two sources
disagree on whether it takes three arguments or four - and D008 forbids calling a function whose
arity is uncertain.

It runs wherever `libSceGnmDriver` is present (shadPS4, or a PS4); on the host build the harness
skips it as not-present. The PM4 it records in an emulator is that emulator's encoding, already
readable in its source; the encoding that would be a *finding* is a real PS4's.

## Axis 2 - a compute result through Gnm (scoped, not built)

Getting the GPU to actually *run* a kernel and produce a result is the hardware-native
counterpart of the Vulkan dispatch - and the larger piece. The pipeline:

1. **Produce a GCN shader.** `sceGnmSetCsShaderWithModifier` points `COMPUTE_PGM_LO` at GCN
   machine code, not SPIR-V. This is the piece that looked like the blocker and is not: LLVM's
   AMDGPU backend, already in the build VM, compiles a compute kernel to a PS4-class GCN object -
   `clang -target amdgcn-amd-amdhsa -mcpu=gfx600 -nogpulib` produces an ELF with the microcode in
   `.text` and the kernel descriptor (COMPUTE_PGM_RSRC1/RSRC2) in `.rodata`. Clean-room: LLVM,
   no vendor compiler. The `cs_regs` array `SetCsShaderWithModifier` wants is
   `{PGM_LO, PGM_HI=0, RSRC1, RSRC2, NUM_THREAD_X, Y, Z}` - PGM_LO from where the microcode is
   mapped, RSRC1/2 from the descriptor, the thread dims chosen.
2. **Allocate GPU-visible memory.** `sceKernelAllocateDirectMemory` + `sceKernelMapDirectMemory`
   for the shader, the data buffer and the command buffer. Signatures cross-confirm (shadPS4,
   GPCS4).
3. **Build the command buffer.** `DispatchInitDefaultHardwareState` -> `SetCsShaderWithModifier`
   (four args, cross-confirmed) -> bind the data buffer -> `DispatchDirect`.
4. **Submit and read back.** `sceGnmSubmitCommandBuffers` (cross-confirmed), wait for completion
   (an end-of-pipe write to a label, polled, or `sceGnmComputeWaitOnAddress`), then read the data
   buffer - a corpus in the same shape the Vulkan path produces, feeding the same `gpuref` /
   `gpudiff` / `gpustats`.

### Where the real risk is - and it is not the shader

The function *arities* confirm. What does not, from two agreeing sources, is the **input struct
layouts** step 3 needs: the shader resource descriptor (the SRD/V# that tells the shader where
the data buffer is and how big), the user-data SGPR mapping (which SGPRs the descriptor is
loaded into, per RSRC2), and the direct-memory type constants. These are not output buffers, so
the `obs_report_buffer` route that keeps axis 1 D008-safe does not apply - they are *inputs* the
GPU reads, and a wrong layout produces a dispatch that reads garbage or writes nowhere: a silent
wrong result, exactly the failure D008 exists to forbid. Confirming them means reading the SRD
and user-data handling in the emulator shader-translators (shadPS4, GPCS4) closely enough that
two sources agree, the same bar the arities met.

### Estimate

Axis 2 is a workstream, not a change: GCN production and extraction (feasible, ~small), the
memory and submission plumbing (medium, signatures known), and the SRD/user-data layout
confirmation (the hard part, D008-gated). It is PS4-specific - the PS5 uses `sceAgc` and RDNA2
ISA, a different API and a different backend target - so it serves a PS4 or shadPS4, not the
Deck (pure Vulkan, no Gnm) or the PS5 directly. Worth building when a PS4-class result corpus is
the goal; until then axis 1 stands, and the Vulkan/Deck path remains the near-term route to real
silicon.
