# D283 - flexible-configured: the probe orbistoun's allocator work was blocked on


Orbistoun's flexible-memory work (its D442) found that the only title using flexible memory imports
`sceKernelConfiguredFlexibleMemorySize` - the *configured* total, distinct from the *available* figure this
probe already measures (`020-memory/flexible-available` = `0x1b40_0000`) - and that orbistoun cannot answer
it honestly because obSCEne had never measured a configured value. Added `020-memory/flexible-configured`
to close that: it reads `sceKernelConfiguredFlexibleMemorySize` and reports the value, the same shape as
its available sibling, so a hardware run supplies the ceiling an emulator seeds a flexible budget from.

Signature and provenance: `int(size_t *out)`, mirroring the available sibling exactly. The name is mined
across eight independent sources (`data/mined-names.txt`: PS5PCEM, SharpEMU, aerolib, fpPS4, ps4libdoc,
shadPS4, ...) and the OpenOrbis toolchain headers give the signature - so declaring it is confirming a
signature two-plus sources agree on, not inventing one (principle 2). Provenance `OBS_FROM_ASSUMED`.

**This one was in the mined census, which the dlsym probe (D282) was not - so it cost the full admission
dance, plus one wrinkle.** Declaring it callable in `platform.h` moves it into the generator's `taken` set
(`tool/src/census.rs`: any `OBS_WEAK name(...)` is excluded), exactly as `sceKernelAvailableFlexibleMemorySize`
is - which is why Available appears in no `X()` list while every other kernel name does. So `corpus.h` must
lose it. Normally `obscene-tool census` regenerates `corpus.h`; that tool is blocked by the same `selfish`
rescaffold as above, so `corpus.h` was **hand-edited to exactly what a regen produces**: its two `X()`
entries removed (the all-symbols list `OBS_CORPUS_0346_LIBKERNEL` and the callable list
`OBS_CORPUS_CALLABLE_0346_LIBKERNEL`), and the group's count comment taken from `786 symbols, 783 callable`
to `785`/`782`. The edit is deterministic from `census.rs`'s `taken()` logic and `mined-names.txt` is left
untouched (the name stays a mined fact; the generator excludes it at emit time), so re-running
`obscene-tool census` once `selfish` is back is a verified no-op. Flagged here because a hand-edited
generated file is exactly the half-moved-census state the project warns about - the difference is that this
edit matches the generator rather than diverging from it, and the regen will confirm it.

Verified: `make host` builds clean (-Werror), the check id compiles into the binary, and the run emits
`020-memory/flexible-configured` skipping on the host stub - identical to its two flexible siblings, which
have no host definition either. `make check` and the census/counts gates stay blocked by `selfish`; the
count-comment arithmetic is the one thing a regen should re-confirm.
