# Platform HUD


Replaced the static tagline with a live HUD of eight platform facts (D139), each value or
`unknown`, colour-coded known/present-unwired/absent. Real reads for GEN/MEM/VRAM/NET;
IP/FW present-probed pending confirmed signatures; TEMP/DISK honest placeholders. New
sysinfo.c/.h; screen.c draws it and gains obs_screen_hud() for the serving build.

Conceded a wrong call: I had argued against the sensor fields because emulators fake them.
The operator was right - the emulator NOT implementing them is exactly what obSCEne exists to
surface, and `unknown` on screen is the forcing function. The only real constraint (D008 -
confirmed signature to call) is preserved by present-probing rather than blind-calling.

### Surprise
- The tree was being edited concurrently by other work mid-session: platform.h gained
  PS5PCEM sceVideoOut*2 functions, colliding with the mined corpus.h census (duplicate
  declaration) - it broke the build, then fixed itself when corpus.h was regenerated
  externally between two greps. gen-corpus.py has also drifted from committed corpus.h
  (numbered macro tags), so a clean regen is not a no-op. Flagged, not adopted.

---

The GPU ISA surface census (D117). `scripts/gen-gpu-surface.py` generates
`docs/GPU_SURFACE.md` - the AMD hardware's special math operations (SFU transcendentals, the
IEEE division sequence, float decomposition, fmed3, the packing conversions) and how obSCEne
reaches each, the GPU analogue of the CPU surface census. Establishes coverage as a checked
claim: of 41 intrinsic ops, 33 exist on RDNA2, 8 covered today, 3 GLSL-reachable and not yet
probed (`frexp_mant`/`frexp_exp` via GLSL frexp, `cvt_pkrtz` via packHalf2x16 - the next
kernels to add), 22 hand-SPIR-V only (controlled-ISA workstream), 8 CDNA-only. Source of truth
is the in-repo classification table; the .td is a cross-check that gates in verify.sh. It
earned its keep at once: caught `int_amdgcn_ldexp` as stale (LLVM 18 moved it to the generic
`llvm.ldexp`), which was corrected to base ISA.

Tool + GPU gates green: 115 tool tests, `gen-gpu-surface.py --check`, `gen_shaders.py --check`,
GPU=1 build, all three targets, harness self-check, protocol, doccheck all pass.

**Concurrent-edit note (the same surprise, from this side).** Running the full `verify.sh`
regenerated the stale `corpus.h` (which was build-broken by the sceVideoOut*2 collision above),
which unblocked the build but rippled into the shared gates the video-out/corpus workstream
owns: `counts.py` has since converged to green, but `compat.py --check` now reports the
compatibility table drifted, because emulator reports regenerated externally today (ps5pcem,
fpps4) reflect the changed census. Neither is GPU work; both are that workstream's to
reconcile (`compat.py --write`), so I have left them. DECISIONS.md/WORKLOG.md are being
appended by both sessions at once - edits here are surgical (match-exact) to avoid clobbering.

