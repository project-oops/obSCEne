# The gpu protocol verb


The interactive oracle (D115). `CMD|seq|gpu|<kernel>|<operand>...` dispatches a compiled-in
kernel over the socket and streams its result records - so a driver interrogates any of the
32 kernels at any inputs without a rebuild, which is the whole point of an interactive mode.
Safe to expose broadly because it runs only named, built-in shaders, never arbitrary code.
Announced only when a GPU backend is up; a graceful failure returns zero lanes rather than
faking a death. Proven over a live loopback socket (rcp, fmaf, an unknown-kernel refusal),
documented, captured as example 11, and in the checker. verify: ok.

---

The reference oracle (D116). `obscene-tool gpuref <corpus>` recomputes every kernel in `f64`
through the host `libm`, rounds to `f32`, and emits it as a corpus in the same shape, so
`gpudiff device reference` says *where the device is approximate* instead of merely that two
runs differ. Proven end to end against a real llvmpipe corpus: the nine exact ops
(`absf`/`double`/`floor`/`ceil`/`trunc`/`round`/`roundeven`/`fract`/`divf`) agree to the bit,
and the whole transcendental set diverges - the approximation map, today. `half` is skipped
(no f16 rounding this build stands behind) and reported as a device-only lane, not faked.

Surprise, and the reason to run device-against-reference rather than only device-against-device:
it caught a false-divergence bug in `gpudiff` that matched formats had hidden. obSCEne prints
`0x0`, the oracle prints `0x00000000`, and the string compare called 123 identical-bit lanes
divergent. Fixed by canonicalising every hex field to `0x%08x` at parse time - which leaves a
*real* bit difference intact (two NaNs with different sign bits stay distinct), so the honest
residue survives: llvmpipe does NaN-suppressing min/max and passes some out-of-range `int(x)`
through unchanged, both now reported as device-behaviour observations. Six new unit tests
(115 total) lock the GLSL semantics and the canonicalisation invariant.

Tool gate green: `cargo fmt`, `cargo clippy --all-targets -- -D warnings`, `cargo test` all
clean. The GPU C also builds and runs clean (`make host GPU=1` captured the llvmpipe corpus).

**Full `verify.sh` is RED, but not on anything here.** It fails in the video-out/display
workstream (D110), which is mid-edit: `src/display.c:334` calls `obs_address_is_callable`, a
D058-style address guard around `sceVideoOutSetBufferAttribute2` that is not yet declared where
`display.c` can see it. Regenerating `corpus.h` (stale - it predated the promotion of the
`sceVideoOut*2` symbols into `platform.h`, so it collided) cleared the first error and exposed
that second one underneath. Both belong to that workstream; nothing in the GPU reference-oracle
change touches `display.c`, `obs_address_is_callable`, or the video-out symbols. Left untouched
pending direction, since this fork is scoped to the GPU work.

