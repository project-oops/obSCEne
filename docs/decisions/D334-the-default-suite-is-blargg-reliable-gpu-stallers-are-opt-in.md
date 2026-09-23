# D334 - the default suite is blargg-reliable; GPU checks that stall are opt-in

**assumed** - 2026-09-23

obSCEne is meant to be the blargg of the PS5: a suite you *trust*, that runs to completion
deterministically and never leaves the console worse than it found it. On 2026-09-23 it was not
that. A full run submitted a raw GPU draw (`166-agc/mrt-dual-target/arm3-dual-blend-rbplus`) that
wedged the GFX pipe; the next check piled more submissions onto the stalled queue; the kernel's
async suspend-point timeout (`0xa0d0c00e`) fired and froze SceShellCore. The console needed a
reboot and re-exploit. A conformance suite that can do that is not conformance - it is a hazard.

## The principle

**The default run contains only checks that retire reliably.** A GPU submit+fence check that can
stall the queue - whether it wedges the console, or "only" stalls and recovers but latches the
safety flag and costs every check after it - does not belong in the run people trust and cite. It
is a *research* probe: correct to have, run deliberately, in isolation, by someone working that
GPU path. It is compiled in but gated behind `-DOBS_RUN_WEDGING_GPU_CHECKS`, off by default.

This is the same call the codebase already made ad hoc - `arm2-3param-packed`,
`primitive-draw-depth/stencil/blend/indexed/cull-face/color-mask`, `draw-textured-linear-pitch`
were each `obs_skip("... isolated ...")` after they were found to hang. D334 makes it a rule and a
single switch instead of a scatter of one-offs.

## The two layers

1. **The stall flag is now a complete protocol (backstop).** `s_agc_queue_faulted` is hoisted to
   the top of `agc.c` with a guard macro; **every** active GPU submit+fence check now both *guards*
   on it (skips if the pipe already stalled) and *latches* it (sets it when its own submit is
   accepted but never retires). Before, the two checks that killed the console did neither, so the
   first stall cascaded. This layer keeps a *single unknown* wedger from taking the system down: the
   first stall stops all further submits. It cannot un-wedge a GPU, so it turns a system-kill into
   an isolated process fault the console survives - proven on 2026-09-23 (a live `0xa0d0c00e` with
   the console staying up).

2. **The known stallers are gated out of the default (the rule above).** Behind
   `OBS_RUN_WEDGING_GPU_CHECKS`: `mrt-dual-target`, `blend-constant` (RB+ blend, wedged the
   console); `primitive-draw-param5` (5-param export, stalls the GE queue); `compiled-ps`
   `arm10a/10b/11` (front-face / discard-depth NGG geometry, intermittently stall - and the hard
   `return` on their failure used to abandon the reliable arms below them, now softened to record);
   `gpu-wait-reg-mem-sync` `arm3b` (submits a real GPU->GPU `WAIT_REG_MEM` that halts the ME by
   design - the very thing `-4b8e` measured - so its fence never retires); `zpass-counters` (run 5,
   2026-09-23: the `ZPASS_DONE` query submit was accepted but its EOP fence never retired, latching
   the flag and costing the check below it); `display-target-memory` (the check right behind
   zpass-counters, masked in every run so far and never once observed to retire - the same raw
   render-target draw + fence path, gated as unproven rather than shipped in the trusted default).
   Each names the run that found it and how to re-enable it.

## Why gate rather than fix

Some of these will become reliable once the register/geometry work behind them lands (the
`NGG geometry register sync` the isolated checks already wait on); `gpu-wait/arm3b` is settled
negative and will never retire (that *is* the finding). Either way the default run should not
carry a check whose retirement is a coin-flip or a known no. A blargg test that fails intermittently
is worse than no test, because it teaches you to distrust the suite.
