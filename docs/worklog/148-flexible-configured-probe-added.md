# 2026-09-01 - flexible-configured probe added (unblocks orbistoun's allocator work)


Added `020-memory/flexible-configured` reading `sceKernelConfiguredFlexibleMemorySize` - the configured
flexible-memory total, the value orbistoun's D442 found it needs and cannot invent. Mirrors the existing
`flexible-available` check (same `int(size_t*)` shape); signature from the OpenOrbis headers, name mined
across eight sources, provenance `OBS_FROM_ASSUMED`. Declared in `platform.h`, added to `imports.c`,
check + table row in `sections/memory.c`.

Wrinkle worth recording: unlike the D282 dlsym probe, this name was in the mined census, so declaring it
callable moves it into the generator's `taken` set and it must leave `corpus.h`. `obscene-tool census`
(which regenerates `corpus.h`) is blocked by the `selfish` rescaffold, so `corpus.h` was hand-edited to
exactly what a regen yields - two `X()` entries dropped (all-symbols + callable lists of
`OBS_CORPUS_0346_LIBKERNEL`) and the count comment `786/783` -> `785/782`; `mined-names.txt` left untouched.
Deterministic from `census.rs`'s `taken()`, so a regen once `selfish` is back is a no-op (D283). `make host`
builds clean and the check skips on the host stub like its two flexible siblings (no host definition); the
census/counts gates remain blocked by `selfish` and should re-confirm the count comment on regen.

