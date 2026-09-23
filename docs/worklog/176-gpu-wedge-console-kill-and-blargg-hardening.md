# 176 - a GPU wedge killed the console; hardening the suite to blargg-reliable

**2026-09-23**

A full hardware run wedged the GFX pipe and took the whole console down (`0xa0d0c00e`,
SceShellCore frozen, reboot + re-exploit needed). Root cause and fix are D334. This is the log.

## What happened

`166-agc/mrt-dual-target/arm3-dual-blend-rbplus` submitted a draw that never retired (fence
miss). The next check, `blend-constant`, submitted three more into the stalled queue. Neither
participated in the `s_agc_queue_faulted` protocol, so nothing stopped the pile-on, and the
kernel's async suspend-point timeout fired. Sweep `reports/hardware/20260923-blend-eboot.obs.log`.

## What was done

- **Completed the stall-flag protocol.** Hoisted `s_agc_queue_faulted` + an `OBS_AGC_QUEUE_GUARD()`
  macro to the top of `agc.c`. Added guard+latch to every active GPU submit+fence check that
  lacked it (`mrt_dual_target_sub`, `blend_constant_sub`, `compiled_ps_sub`, `driver_submit_fence`,
  `compute_dispatch`, `graphics_submit`, `primitive_draw_sub`, `primitive_draw_target_sub`). Audit
  now shows zero gaps among active submitting checks. This is the backstop: a single unknown wedger
  can no longer cascade - the first stall stops all further submits.
- **Gated the known stallers out of the default suite** behind `-DOBS_RUN_WEDGING_GPU_CHECKS`
  (default off): `mrt-dual-target`, `blend-constant`, `primitive-draw-param5`, `compiled-ps`
  `arm10a/10b/11`, `gpu-wait-reg-mem-sync/arm3b`. Each carries a comment naming the run that found
  it. Softened the hard `return` on the compiled-ps front-face arms so one flaky arm no longer
  abandons the reliable arms below it.

## What the runs showed (all captured, then archived under reports/hardware/)

- `20260923-hardened-eboot.obs.log`: first hardened run **completed the full suite** (825 checks,
  reached 900-surface) with the wedgers gated. Two live stalls were **caught by the protocol** (7
  downstream checks skipped, no cascade) - the console **survived** where before it died. Proof the
  backstop works.
- A subsequent run hit a live `0xa0d0c00e` from an intermittent early stall (`compiled-ps/arm10a`):
  the protocol still held the *system* (console stayed up), but obscene's *process* faulted after
  emitting the full report, because the flag cannot un-wedge a GPU. That is what moved arm10a/10b/11
  and gpu-wait/arm3b into the opt-in gate.
- **Run 5 (the verification run, operator-approved).** No crash (`0xa0d0c00e = 0`) - the console was
  never at risk. arm3b was correctly isolated by its gate, and with it out of the way `zpass-counters`
  ran for the first time in a while and *stalled*: its `ZPASS_DONE` submit was accepted but the EOP
  fence never retired (`fence-hit 0`, `fence-val` stayed at the `0x11111111` canary). The stall latched
  `s_agc_queue_faulted`, so `display-target-memory` behind it guard-skipped - one masked check, no
  cascade, console up. Sweep the run in `scratchpad/probe-run5.log`.
- **What that showed and what was done.** `zpass-counters` is one more of the same stall class, and
  `display-target-memory` has now been masked in *every* run - never once observed to retire, and
  structurally the same raw render-target-draw-plus-fence path. Both were gated behind
  `-DOBS_RUN_WEDGING_GPU_CHECKS` (D334 updated to list them). The default suite now carries no
  submit+fence check that has ever been seen to stall or that has never been seen to retire.
- Host build clean under `-Werror -Wconversion`; Orbis native title builds (eboot 5,953,328 bytes).
  `make check` still reports the 22 pre-existing `libSceLibcInternal` "censused twice" problems from
  another session's libc/unwind half-move - untouched by this agc-only change. A verification run on
  the fully-gated build is pending operator go-ahead (`probe`/`launch` needs per-run approval).

## Data delivered along the way

- `9c31` RESOLVED on the bus (blend is positional-by-channel-pair: `arm13 = 0x40c040c0`).
- `9b41` RESOLVED (`sceSystemServiceReceiveEvent` no-event return `0x80a10004`, 0 bytes, arity 1).
- `5e8c`, `d1c4` stay OPEN (behavioural / manual-controller parts, not probe-settleable).

## Note for next time

The console is single-slot and shared; `probe`/`launch` clobbers whoever else is on it and needs a
per-run go-ahead. `restore` is free. (Cost a clobbered run on 2026-09-23.)

**`pros probe`'s internal restore dedups and will silently launch a STALE build.** The first approved
verification run of the gated build came back *identical to run 5* (`zpass-counters` fail,
`display-target-memory` skip) - because the deploy leg printed `0 files, 0 B -> ...` /
`9 items already there, unchanged - not re-sent` and the console ran the previous build. The eboot
had genuinely changed (gates confirmed in `obj/eboot/.../agc.o` and a new md5), but the dedup
compared against its last-restore manifest, not the new bytes. Fix: `pros restore --all -y <FROM>
/data/homebrew/<ID>` before probing (it force-sent `9 files, 6.1 MiB`), then launch. Verify the
staged eboot's md5 against the build's before trusting any run. (Also: verify greps by running a
script *file*, never `wsl.exe -- bash -lc '...'` - the arg mangling gave three false "ABSENT"s here,
including on a gate string that was plainly in the source.)
