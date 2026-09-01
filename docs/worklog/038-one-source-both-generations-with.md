# One source, both generations, with pictures


obSCEne now draws its complete report on a previous-generation loader and a
current-generation one, from the same source and the same binary logic.

| | records | ran to end | screenshot |
|---|---|---|---|
| shadPS4 (previous) | 36,491 | yes | full report, 499/499, `REPORT COMPLETE` |
| PS5PCEM (current) | 36,377 | yes | full report, 499/499, `GEN 5 (CURRENT)` |
| fpPS4 (previous) | 25 | no | drawing, slowly |
| Kyty | 0 | no | black window, alive at 60fps |
| ChonkyStation4 | - | - | **requires firmware** |
| SharpEMU / craziiEmu | 0 | no | resolve nothing |

### What it took, and what it cost

The display had been asking for the previous generation's video-out entry points
unconditionally. The two generations expose *different* ones and neither has the other's,
so a current-generation loader reported "the display symbols are not present" while
carrying a complete set under other names (D110, D111).

Fixing that broke fpPS4 twice, and both breakages had the same shape: **a change made to
satisfy one loader, applied unconditionally.**

- Preferring the newer pair picked a **logging stub**. fpPS4 installs one for every import
  it cannot resolve, so the symbol had an address and did nothing (D140). Presence is not
  behaviour - which this program measures in two whole sections and then assumed away in a
  third.
- Initialising the user service before asking for a user hung inside the initialise. Asking
  first and initialising only on refusal leaves working platforms untouched.

### Surprises worth keeping

- **The census made a loader unusable.** 35,339 census records is most of the runtime on a
  slow loader; fpPS4 went from finishing to reaching check 11 in fifteen minutes. `CORPUS=0`
  makes the mined census a build option, which is a property of the loader rather than of
  the probe.
- **`grep` silently discarded every fpPS4 report.** Its log carries NUL bytes, so grep
  called the stream binary and emitted nothing - and a sweep over that concluded there was
  no dangling call and stopped. Every fpPS4 result recorded before today was thinned by it.
- **The sweep could not tell a hang from a timeout.** Both leave a `try` with no `res`, so
  on a slow loader it excluded a healthy check every round and called it a crash. The runner
  now reports which happened, because only the runner knows.
- **ChonkyStation4 is a low-level design.** It links the guest against real firmware modules
  and panics on the first missing one. Correct for what it is, and unusable here.

---

Closing the census-named GLSL gaps (D118). D117's census named three GLSL-reachable ops not
yet probed; this adds them: kernels `frexpmant`, `frexpexp`, `packhalf`, moving census coverage
8 -> 11. frexp gets an exact reference (power-of-two scaling, bit-for-bit); packhalf is skipped
by the reference like `half` (needs f16 rounding) and stands alone for the hardware diff. The
tightest loop the GPU work has - census names a gap, a kernel closes it, the reference judges
it.

### Surprise
- frexp agrees with the reference on every normal input and diverges on exactly one subnormal
  (`0x000116c2`): llvmpipe reports exponent -126 un-normalised, IEEE frexp (the reference)
  normalises to -132. A real denormal-handling difference, not a reference bug - same shape as
  `cvti` out of range. Left the reference IEEE-correct so the divergence keeps flagging it;
  documented in the kernel and reference comments as an observation.

GPU gates green: 116 tool tests, clippy, `gen_shaders.py --check` (comments do not change the
embedded SPIR-V), `gen-gpu-surface.py --check`, GPU=1 build. Still standing off the parallel
video-out/corpus workstream's `compat.py`/`corpus.h` reconciliation, which is theirs.

---

Integer/bit breadth (D119). A second GPU surface the float census did not map: kernels
`bitcount`, `findmsb`, `findlsb`, `bitreverse` (v_bcnt / v_ffbh / v_ffbl / v_bfrev), read the
input lane as a bit pattern and return an int. Exact references; a new "Integer / bit base ISA"
census table beside the float one. 39 kernels now. Every lane agrees with the reference bit for
bit - an integer op that diverged would be a device bug, and now a check would say so.
`bitcount(0xbf800000)=8` (the -1.0f pattern), `bitcount(0x3f800000)=7` (1.0f), sane. Gates
green: 117 tool tests, clippy, both census/shader checks, GPU=1 build.

---

Controlled-ISA (D120). Relaxed-precision variants `rcprelaxed`, `sqrtrelaxed`, `divrelaxed`
pin the fast SFU path against the correctly-rounded one. The lever is `mediump` ->
RelaxedPrecision decoration (confirmed 5 in rcprelaxed vs 0 in full rcp), which RDNA lowers to
the bare fast op while full precision gets the IEEE sequence - the lever a real shader pulls,
not a hand-forced opcode. Each relaxed kernel shares its full form's correctly-rounded
reference, so on hardware the relaxed one diverges (the fast-path error) and the full one
matches. Census updated: `fdiv_fast` now covered:divrelaxed; the division primitives
(div_scale/fixup/fmas) reclassified `isa` -> a new `sequence` reachability (reached only as the
decomposition of full-precision division, not isolable). 42 kernels.

### Note
- On llvmpipe the relaxed variants match the reference and their full forms bit-for-bit (rcp
  and rcprelaxed of 1/3 both 0x3eaaaaab): llvmpipe ignores RelaxedPrecision, so this proves the
  plumbing and defers the finding to the part that honours the decoration - exactly the
  ahead-of-hardware shape. `spirv-as`/`spirv-dis` are present for cases GLSL cannot express; the
  fast/correct split is not one of them, so no hand-written SPIR-V was needed.

Gates green: 117 tool tests, clippy, `gen_shaders.py --check`, `gen-gpu-surface.py --check`,
GPU=1 build.

