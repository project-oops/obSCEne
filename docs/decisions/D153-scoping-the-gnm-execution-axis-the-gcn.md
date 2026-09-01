# D153 - Scoping the Gnm execution axis: the GCN shader is not the blocker; the input struct layouts are


Status: derived - `docs/GNM.md` scopes the compute-result-via-Gnm path, and the one uncertain
premise (can this toolchain even make a PS4 GCN shader) is now tested and answered yes.

The execution axis - getting the console GPU to run a kernel and produce a result through Gnm,
the console-native counterpart of the Vulkan dispatch - was deferred as "needs GCN shaders,
which is a big sub-project." Scoping it moved the blocker. The GCN shader is *not* the hard part:
LLVM's AMDGPU backend, already in the build VM, compiles a compute kernel to a PS4-class GCN
object - `clang -target amdgcn-amd-amdhsa -mcpu=gfx600 -nogpulib` produces the microcode in
`.text` and the COMPUTE_PGM_RSRC1/RSRC2 descriptor in `.rodata`, which is exactly the `cs_regs`
`sceGnmSetCsShaderWithModifier` wants. Clean-room, no vendor compiler. And the function arities
the path needs cross-confirm across shadPS4 and GPCS4: `SetCsShaderWithModifier` (four args, so
it is usable where the three-arg `SetCsShader` is not), `SubmitCommandBuffers`, and
`AllocateDirectMemory`.

The real blocker is elsewhere, and naming it is the point of the scope: the **input struct
layouts** the dispatch reads - the shader resource descriptor (the V# telling the shader where
the data buffer is), the user-data SGPR mapping, and the direct-memory type constants. These are
not output buffers, so the `obs_report_buffer` route that keeps the API axis (D152) D008-safe
does not apply: a wrong layout here is a dispatch that reads garbage or writes nowhere, a silent
wrong result, which is exactly what D008 forbids guessing at. Confirming them means reading the
emulator shader-translators closely enough that two sources agree - the same bar the arities met,
applied to layouts instead.

So the honest estimate in `docs/GNM.md`: GCN production small, memory/submission plumbing medium,
the SRD/user-data layout the hard D008-gated part. And it is PS4-specific - the PS5 is `sceAgc`
and RDNA2 ISA - so it is worth building when a PS4-class result corpus is the goal, not before.
Recording it so the next attempt starts from "confirm the layouts" rather than rediscovering that
the shader was never the problem.

