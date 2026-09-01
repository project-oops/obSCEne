# The name, and what building shadPS4 from source actually found


**`SCE` is marked wherever the project renders its own name** (D147): magenta on screen via
`OBS_COLOUR_MARK`, magenta in `obscene-tool pretty`, bold in the README heading. No new
mechanism was needed - `obs_display_text` already returned the next x so callers could chain
colour runs, with a header comment saying so, written for exactly this and unused until now.
The terminal run ends with `39` (default foreground) rather than `RESET`, because the banner
is inside a bold run and `RESET` would have silently unbolded the rest of the line. Left out
of prose: the name appears some 160 times in the docs and marking every one is noise, not
identity.

Two things the wordmark cleaned up on the way past: it is a function returning where it
finished, so the `8 * 6 * 7` that two call sites carried to place the HUD beside the title is
gone; and the first version of it was pasted *inside* a function body, which the build caught
immediately.

**shadPS4 from source refuses to run obSCEne** (D149). This was meant to be a five-minute
confirmation and is the most consequential finding of the session.

The self-built `be21649` asserts in the loader - `libc.prx cannot be loaded, but the guest
attempted to use it` - and never reaches the suite. The downloaded binary, handed the same
module, prints `expect crashes` and carries on: 65,665 log lines against 33,044. **Every
shadPS4 figure in `COMPATIBILITY.md` came from a binary whose behaviour the current source no
longer has**, which is precisely what D094 predicted would stay invisible while the source
was only read and the binary only run.

The cause is ours. A corpus-free build loads on the same emulator with no complaint at all -
284 records, no assertion, not one missing-`.prx` line. `mkmodule` emits `DT_NEEDED` per
imported library, correct when the imports were the curated surface and wrong once the corpus
made that hundreds of libraries: the module now declares a load-time dependency on
`libSceFios2`, which obSCEne never calls and only asks about.

A census asks whether something is present. That is not a dependency, and a loader that takes
the claim at face value is entitled to demand the file. Not repaired in this session because
the repair trades against D100 - fpPS4 keys its implementation table on `DT_NEEDED` - and
that is a change to what two emulators report, not a one-line fix.

Numbering, again: this file's decision numbers were never max-plus-one. Two appends collided
with existing entries (D146, D148 both already taken, out of order in the middle of the file)
and `doccheck.py` caught both within the hour of being written. Next free number is now
worth computing rather than assuming.

---

Hardware day made turnkey (D150). `scripts/gpu-analyze.sh <corpus>` runs gpuref + gpudiff +
gpustats in one command - the same analysis for the golden today and a Deck/PS5 corpus later,
since only the corpus's source changes. `docs/DECK.md` is the runbook: `make deck` (pure-Vulkan
host build, no sceGnm - the Deck needs none), run/serve, capture, analyse, diff vs an emulator.
Chose this over the Gnm/shadPS4 path after reconsidering: that path is large and low-value here
(the Deck needs no Gnm; Gnm would only give shadPS4's gaps, not orbistoun's, and shadPS4's source
isn't even present - only its binary). This is the honest completion of the ahead-of-hardware GPU
work. doccheck/tool/analyze/golden all green.

**State of the GPU workstream:** 50 kernels; reference oracle, ISA census (docs/GPU_SURFACE.md),
golden gate, controlled-ISA relaxed variants, protocol escape hatch, and the gpudiff/gpuref/
gpustats trio wrapped by gpu-analyze.sh. Everything ahead-of-hardware is built and proven on
llvmpipe; the remaining progress needs the Deck (machinery ready) or the orbistoun thread
(off-limits while forked).

**D149 resolved: the `DT_NEEDED` declarations stay.** The first conclusion was reached
emulator-first and was wrong - it treated a module declaring the platform libraries it works
with as an over-claim, because the least tolerant loader in the toolkit could not satisfy it.
On a console those modules exist. What changed is shadPS4's tolerance for running without
firmware, and that is a gap in a loader's deployment rather than a defect in the module.

These emulators are scaffolding for getting obSCEne running before hardware, not the standard
it has to satisfy; trimming the module to suit them would shape the oracle to fit the thing it
is meant to outrank, and would cut what obSCEne can reach on the target that matters.
`COMPATIBILITY.md` now carries a section saying so, with the two logs side by side.

Still open, and on our side rather than theirs: no single console carries every library across
23 firmware versions, so a few hundred hard dependencies may not suit hardware either - same
failure, different cause, unmeasured. The likely answer is that a census should not be a
load-time declaration at all. `sceKernelLoadStartModule` and `sceKernelDlsym` are already
declared and `110-modules` already exists, and asking the platform at runtime returns an error
code rather than an address - which is the difference between evidence and a number that D140
showed a stub-everything loader can fake wholesale.

---

Gnm command-building API probe (D152). Reconsidered the Gnm path after (correctly) being pushed
on it: I was wrong that it wasn't worth it and wrong that shadPS4's source was absent (it's at
<emulators>\src\shadPS4). Gnm is the real console GPU API. Built the cheap axis: a 165-gnm
section that calls the two command-builders whose arity shadPS4 and GPCS4 confirm
(DispatchInitDefaultHardwareState 2 args, DispatchDirect 6 args) into a guard-banded buffer and
dumps the PM4 with obs_report_buffer (D008-safe). SetCsShader left out - the two sources disagree
(3 vs 4 args), exactly what D008 forbids calling. Promoted the two to platform.h + imports.c,
regenerated corpus.h (drops them from the census, no collision), registered the section. Builds
host + module; host skips both (no libSceGnmDriver), correctly. counts updated (+1 section, +2
checks).

### Notes
- The execution axis (a real compute result via Gnm) needs GCN shader microcode - a separate
  toolchain (GLSL/OpenCL -> LLVM AMDGPU -> GCN) - scoped as the follow-on, not built.
- Live shadPS4 capture not done: shadPS4 routes guest stdout to its log, so records need
  SERVE+drive (a Windows tool binary + VM-to-host networking this setup lacks), and shadPS4's PM4
  is already source-readable so it would prove plumbing, not find anything. The finding-grade PM4
  is a real PS4's, which awaits one. Probe is built, D008-confirmed, host-verified, ready.

---

Scoped the Gnm execution axis (D153, docs/GNM.md). The follow-on to D152's "Both, API first."
Tested the one uncertain premise: can this toolchain make a PS4 GCN shader? Yes -
`clang -target amdgcn-amd-amdhsa -mcpu=gfx600 -nogpulib` on a trivial compute kernel produces an
ELF with microcode (.text) + the COMPUTE_PGM_RSRC1/2 descriptor (.rodata) = the cs_regs
SetCsShaderWithModifier wants. Clean-room LLVM, no vendor compiler. Arities cross-confirm
(SetCsShaderWithModifier 4 args - usable where the 3-arg SetCsShader is not - SubmitCommandBuffers,
AllocateDirectMemory). So the GCN shader is NOT the blocker; the input struct layouts (SRD/V#,
user-data SGPR map, memory-type constants) are - inputs the GPU reads, where obs_report_buffer
can't help and a wrong layout is a silent wrong result (D008). Doc names the pipeline, the
confirmed-vs-uncertain, and the estimate. PS4-specific (PS5 = sceAgc). Not built; scoped so the
next attempt starts from "confirm the layouts." doccheck ok.

