# D121 - Console generation is observed, never asserted from presence


**Console generation is observed, never asserted from presence: `both` and `neither` resolve
to `unknown`, and only a single driver may name a console.**

Status: implemented - the HUD (`src/sysinfo.c`) renders `GEN unknown` when both or neither
graphics driver resolves; `005-generation` already returned a partial for `both`, and its
wording now names the cause. Verified on shadPS4 (`GEN UNKNOWN`); `make check` green (117).

The HUD's `GEN` field read the presence of a current-generation graphics symbol as proof of
the current console. On shadPS4 that printed `5 (current)` on a PS4 emulator, next to its own
PS4-scale `MEM`/`VRAM`. It was the one field that trusted a pointer instead of observing
behaviour - principle 2, broken in the program's own header.

No address-only test can rescue it. `obs_address_is_callable` is `addr != NULL && addr >=
OBS_LOWEST_CALLABLE`: a crash-guard (D058) answering "safe to jump here", not "does this work".
A loader that stub-resolves every unresolved import answers "yes" to any symbol - shadPS4
stub-resolved 64,183 of this program's imports in one run, 1,058 of them NIDs it could not even
name, 236 in `libSceAgc` - so presence on such a loader is worthless by construction. The
contrast is in its own log: the PS4 video-out it implements runs through `Lib.VideoOut`
(`driver.cpp`) with real parameters; every `Agc`/`Gnm`/`…2` symbol is a `Core.Linker`
`Stub resolved` warning.

So presence can name a generation only when **exactly one** driver resolves. `both` is the
fingerprint of a stub-everything loader as much as of back-compat, and `neither` is the normal
early-emulator state - both are `unknown`. `both` renders unconfirmed (something graphics-shaped
did resolve), `neither` absent.

Detection (`obs_detected_generation`, D110) is unchanged; it still returns
current/previous/both/unknown. This decision governs how the two consumers report `both`: the
header as `unknown`, the section as a partial that cannot attribute absences. Neither claims a
console it cannot prove.

