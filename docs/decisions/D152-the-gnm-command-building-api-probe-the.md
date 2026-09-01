# D152 - The Gnm command-building API probe: the console GPU's own calls, the other GPU axis - and a correction to having dismissed it


Status: derived - a `165-gnm` section calls the two cross-confirmed libSceGnmDriver command
builders and records the PM4 they encode. Builds host and module, skips correctly on the host;
the confirmed subset was reached the D107 way.

D150 argued the Gnm path was not worth building. That was wrong twice, and this records the
correction. Gnm is the real console GPU API - it drives the actual hardware, so on a PS4 it
probes real silicon, not an emulator; and the shadPS4 source that confirms its signatures *is*
present (`<emulators>\src\shadPS4`, checked the sibling binary folder before). What is true
is that Gnm is not one thing but two axes of very different cost, and this builds the cheap one.

- **The API axis, built here.** The command *builders* -
  `sceGnmDispatchInitDefaultHardwareState` and `sceGnmDispatchDirect` - write PM4 command packets
  into a caller's buffer and return, touching no GPU. Calling them is the same class as the
  130-layout buffer-fillers, and the PM4 they write is the encoding a command-processor emulator
  must parse. So the section calls them into an oversized guard-banded buffer and dumps what they
  wrote with `obs_report_buffer` - the D008-safe route (dumping bytes needs the arity, not the
  struct layout). It is the GPU counterpart of the CPU NID probes, which is what "probe every GPU
  call" always meant.
- **The execution axis, not built.** Reaching a compute *result* through Gnm needs
  `sceGnmSetCsShader` to point at **GCN shader microcode** - a different artifact than obSCEne's
  SPIR-V, produced by a separate toolchain (GLSL/OpenCL -> LLVM AMDGPU -> GCN + descriptor
  extraction) - plus command buffers, direct-memory and submission. That is a workstream, scoped
  as the follow-on, not smuggled in here.

D008 did visible work confirming the arities. shadPS4 and GPCS4 agree exactly on
`DispatchInitDefaultHardwareState` (2 args) and `DispatchDirect` (6 args), so those are declared
and called. They **disagree** on `sceGnmSetCsShader` - three arguments in one, four (a trailing
modifier) in the other - so it is left in the census, uncalled: a wrong arity corrupts the stack
and surfaces far from the cause. OpenOrbis's headers stub all of them as `void f()`, so they are
no help here; the confirmation is two independent reimplementations agreeing, the same standard
that let sceNet be built (D107), and the provenance is `assumed` because no vendor document
backs it.

What is validated, and what waits: it compiles both ways, and on the host (no libSceGnmDriver)
the harness skips both checks as not-present, which is correct. A live capture wants a loader
with sceGnm - shadPS4 - but shadPS4 routes guest stdout to its own log, so records come only over
the SERVE+drive path, which needs a Windows tool binary and VM-to-host networking this setup does
not have; and shadPS4's PM4 is already readable in its source, so that capture would confirm
plumbing, not find anything. The PM4 that would be a *finding* is a real PS4's, which awaits one
(the Deck is Vulkan and needs no Gnm; the PS5 uses sceAgc). So the probe is built, confirmed and
host-verified, ready for the environment that has the API.

