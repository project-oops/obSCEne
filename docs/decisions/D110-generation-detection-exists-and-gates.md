# D110 - Generation detection exists and gates nothing. `obs_detected_generation()` has no callers


Status: correction to an assumption held about this program, including by its own author.

`005-generation` infers which console it is running on, and the census annotates every
group with `OBS_PREVIOUS`, `OBS_CURRENT` or `OBS_SHARED`. It is reasonable to read that as
"obSCEne runs previous-generation code on the previous generation and current on current".
It does not.

- `obs_detected_generation()` is declared in `harness.h`, defined in `generation.c`, and
  **called from nowhere**.
- `availability` reaches exactly one place - `census()` - where it decides whether an
  absence scores as a skip or a partial. **Scoring, not gating.**

So the detected generation is reported and then ignored.

### How far it actually matters

Narrowly, which is the only good news here. obSCEne calls **no** `sceGnm*` or `sceAgc*`
function at all - the graphics libraries are census-only, so nothing there can be called
against the wrong generation.

The whole of the exposure is video-out, and two functions of it:

| called by src/display.c | generation |
|---|---|
| `sceVideoOutRegisterBuffers` | previous only |
| `sceVideoOutSetBufferAttribute` | previous only |
| `sceVideoOutOpen`, `Close`, `SubmitFlip`, `SetFlipRate`, `GetResolutionStatus` | both |

Which is exactly D127: a module built `GEN=5` declares itself current-generation and then
asks for the previous generation's video-out entry points. PS5PCEM reports
`display|absent` for that reason and no other.

### Why the fix is not "consult the generation"

The obvious repair - ask `obs_detected_generation()` and branch - is the wrong one. It puts
a second source of truth beside the one that already works: **whether the symbol resolves.**
A weak import that comes back null has answered the question directly, without inferring
anything from other symbols, and it stays right on a platform that supports both.

So the fix is to import both variants weakly and prefer the `2` form when it is present -
which is what a title targeting both generations would do, and needs no detector at all.
That leaves `obs_detected_generation()` still unused, and the honest options are to give it
a consumer or to delete it. An accessor nothing calls is a claim the program does not
make.

