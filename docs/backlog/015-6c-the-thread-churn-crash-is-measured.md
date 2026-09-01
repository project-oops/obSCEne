# 6c. The thread-churn crash is measured but not characterised


`015-sync/thread-churn` crashes one emulator roughly one run in seven, inside its own
thread teardown, sometimes on a thread that was joined two sections earlier. What is not
known is the *threshold*: forty cycles trigger it and two do not, and nobody has bisected
between. "Fails above N live threads" is a bug report; "fails sometimes" is a complaint.

`OBS_THREAD_CHURN` is now overridable - `make module CHURN=8` - so bisecting it is a
handful of builds.

**A first attempt established nothing, and how it failed is the useful part.** Four runs
each at 4, 16 and 40 cycles produced no crashes at all: twelve runs, zero events. At a
rate near one in seven that is an unremarkable outcome and says nothing about a threshold
- four runs at a one-in-seven rate miss it more often than not.

`scripts/repeat.sh` exists so this cannot happen again: it reports the denominator as
loudly as the numerator, names where each short run died, and prints an explicit note when
fewer than twenty runs found nothing (D079).

### It still reproduces

Twenty runs at the default forty cycles: **nineteen complete, one died in
`015-sync/thread-churn`.** So the fault is alive on shadPS4 0.18.0 and a bisect is not
characterising something already fixed - which was the real risk, and the reason to
measure before bisecting.

One in twenty against the previously reported one in seven is not a disagreement; at these
sample sizes the two are indistinguishable, and neither is a rate worth quoting to a
decimal.

### The threshold is the expensive question, so falsify it instead

Distinguishing "crashes above N" from "crashes at any N" by measuring both rates needs
hundreds of runs at one event per twenty. **Testing whether the threshold exists at all is
much cheaper:** run at the lowest churn the knob allows. A crash there kills the threshold
hypothesis outright - the fault is thread teardown being flaky, not thread count - and
twenty runs is enough to see it.

A clean twenty at the low value is weaker: it is consistent with a threshold and also with
having missed a one-in-twenty fault, which the script says out loud.

### Result: not falsified, and not established either

| cycles | runs | died in thread-churn |
|---|---|---|
| 40 (default) | 20 | 1 |
| 2 | 20 | 0 |

The threshold hypothesis survives, and that is nearly all that can be said. **0 of 20
against 1 of 20 is not a distinguishable difference** - at a one-in-twenty rate, twenty
clean runs is the expected outcome whether the fault is absent at two cycles or merely
rare there.

So the direction is not contradicted and nothing is confirmed. Separating the two needs
roughly an order of magnitude more runs per point, which is hours of wall clock for a
finding whose value is a sharper sentence in one bug report.

**Recorded rather than pursued.** The failure mode this item exists to warn about is
claiming more than the runs support, and "consistent with a threshold" is the honest
ceiling on 40 runs.

