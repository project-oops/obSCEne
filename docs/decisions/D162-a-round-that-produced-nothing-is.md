# D162 - A round that produced nothing is retried once


**A round that produced nothing is retried once, because "the run failed" and "the module
did not start" are different things and only one of them is about the module.**

`bulk-sweep.sh` treated an empty round as terminal:

```text
the run produced no bulk records and no unfinished check; it did not start
```

Two causes were enumerated when that was written - the module crashed inside the section
(there is a dangling announcement, handled above it) and the module was blocked by a check
ahead of the section (there is an unfinished check, handled beside it). Anything else was
assumed to mean the module never ran.

**There is a third cause and it is the common one.** A run can simply fail: the loader
crashes before it opens a window, the module transfer glitches, the emulator loses its
device. Nothing was measured, and nothing is wrong with the module.

### What it cost, measured

A sweep against shadPS4 ran 31 rounds and reached index 351 of 32,466, with 321
answered calls, an hour of wall clock. It then hit an empty round and exited.

Re-running that exact index by hand, unchanged, immediately produced **916 records**. The
round was a bad run and nothing else. One of them discarded thirty rounds of work, and
reported it in words that name the module rather than the run - so the natural next step was
to go looking for a fault in a build that did not have one.

### Retried once, which is the discipline the sibling script already has

`sweep.sh` does exactly this for a suspected hang (D144): observe, retry with a changed
budget, and only conclude when the second observation agrees with the first. **A single
observation of nothing is not evidence of nothing.**

Only a second empty round at the *same index* concludes. The flag resets whenever any round
produces records at all, so an unrelated later failure gets its own retry rather than
inheriting a used one, and the summary now reports how many empty runs were retried - a
sweep that needed six of them is describing an unstable loader, which is a finding about the
loader and should not be silent.

### Why not retry indefinitely

Because the terminal message is a real outcome that must stay reachable. A module that
genuinely cannot start would otherwise loop until `--max-rounds`, and report
`INCOMPLETE: stopped after N rounds` - which says the surface was not covered but not that
nothing ever ran, and those need telling apart.

