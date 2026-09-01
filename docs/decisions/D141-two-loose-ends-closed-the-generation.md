# D141 - Two loose ends closed: the generation had two inferences, and the generated censuses had no gate


Status: derived - one was a live inconsistency, the other a drift waiting to happen.

### The header and the section could disagree

D110 recorded that `obs_detected_generation()` had no callers. What it missed is *why* that
was worse than dead code: `sysinfo.c`, which draws the `GEN 5 (CURRENT)` in every report
header, ran **its own inference from different marker symbols**.

| | previous marker | current marker |
|---|---|---|
| `005-generation` | `sceGnmSubmitCommandBuffers`, `sceGnmDrawIndex` | `sceAgcCreateShader` |
| `sysinfo.c` | `sceGnmSubmitDone` | `sceAgcAcbAcquireMem` |

Two answers to the one fact the report states in its header, from two sets of symbols,
either of which a platform could resolve without the other - while the accessor built to
prevent that sat unused.

Now there is one inference. `generation.c` owns all four markers, `obs_detected_generation()`
computes on demand and caches, and `sysinfo.c` asks it. On-demand rather than
section-order-dependent because the header is drawn before any section runs, so a value
cached only by `005-generation` would be unknown for the first part of every run - and the
fix for that is not a second copy of the reasoning somewhere earlier.

It also gained an answer it could not previously give: `4+5 (both)`, which is what a loader
that stubs everything actually reports.

### The censuses could go stale silently

`corpus.h` and `nids.h` are generated from `data/` and committed. Nothing checked that the
committed headers still matched the data - the exact drift `counts.py` gates one level up,
and ungated for the whole life of the corpus.

A stale census is worse than a stale README. It reports an **absence** for a symbol the
corpus no longer claims, and an absence in this program's report means "the platform does
not have this" - so the failure mode is a fabricated platform gap rather than a wrong
number in prose.

Both generators now take `--check`, and `verify.sh` runs them. The check was proved able to
say no before being believed when it says yes, which is this project's rule after shipping
two gates that could never fail.

