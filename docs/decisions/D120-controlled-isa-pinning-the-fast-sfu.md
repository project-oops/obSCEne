# D120 - Controlled-ISA: pinning the fast SFU path against the correctly-rounded one, through the precision the SPIR-V asks for


Status: derived - three relaxed-precision variant kernels (`rcprelaxed`, `sqrtrelaxed`,
`divrelaxed`), sharing the correctly-rounded reference with their full forms; the machinery is
verified as plumbing on llvmpipe, and the finding it produces waits on RDNA hardware.

The census marks the reciprocal, sqrt and division as covered, but that hides a distinction the
shader gap turns on. A full-precision `1.0/x` is not one instruction on RDNA - it is the
correctly-rounded IEEE division sequence (v_div_scale, v_rcp, v_div_fmas, v_div_fixup). The
*bare* v_rcp, ~1 ULP off, is a different result, and a game's shaders reach it by asking for
relaxed precision. Whether an emulator must reproduce the fast path bit-for-bit or can compute
the correct value depends on which the title used - so obSCEne has to probe both, not just one.

The lever is the precision decoration, not hand-written SPIR-V. A `mediump` reciprocal compiles
to a `RelaxedPrecision`-decorated division (confirmed: five such decorations in `rcprelaxed`
against zero in full `rcp`), which RDNA lowers to the bare fast op; the full-precision kernel
gets the correct sequence. This is exactly the lever a real shader pulls, so it is more faithful
than forcing an opcode by hand and less fragile - the driver honours the hint rather than being
tricked past an optimiser. `spirv-as` is available for the cases GLSL genuinely cannot express,
but the fast/correct split is not one of them.

Two honest boundaries recorded in the census as it changed:

- **The division primitives are a `sequence`, not `isa`.** `div_scale`, `div_fixup` and
  `div_fmas` have no SPIR-V opcode and no GLSL - they exist only as the driver's decomposition
  of a full-precision division. They cannot be isolated, but they are not unreached: full
  `divf`/`rcp` exercise the whole sequence, and its end-to-end correctness against the reference
  is the check. A new reachability, `sequence`, says this rather than pretending the primitives
  are either covered or unreachable. `fdiv_fast`, by contrast, *is* the fast path and is now
  covered by `divrelaxed`.
- **The reference is shared, deliberately.** `rcprelaxed` references the same correctly-rounded
  1/x as `rcp`. That is the point: on hardware the relaxed kernel diverges from it (the fast-path
  error) while the full kernel matches, so the diff against the reference reads directly as
  "this side is approximate, that side is exact". On llvmpipe, which ignores the hint, both match
  and agree bit-for-bit (rcp and rcprelaxed of 1/3 are both 0x3eaaaaab) - the plumbing proven,
  the finding deferred to the part that honours the decoration.

