# D079 - `scripts/repeat.sh`: an intermittent fault needs a denominator


Status: decided, before attempting BACKLOG §6c.

The churn crash was to be characterised by bisecting a threshold. The first attempt ran
four builds at each of three values, found nothing, and nearly got written up as evidence
that the knob does nothing. At a rate near one in seven, four runs miss the fault more
often than they find it - twelve runs with zero events is not a result.

So the measurement gets a script rather than a procedure, and the script **reports the
denominator as loudly as the numerator** and refuses to be quiet about a small sample: a
run of fewer than twenty with no crash prints a note saying so.

It also names where each short run died, from the `try` with no `res`, because "crashed
three times in twenty" and "died in the same call three times in twenty" are different
findings and only the second is a bug report.

**And the first question is whether the fault still exists.** Recent sweeps complete
cleanly with `thread-churn` passing at forty cycles, so bisecting a threshold could be
characterising something already fixed. Establishing that a phenomenon still reproduces
before spending an hour on its shape is cheaper than the alternative, and it is the step
the earlier attempt skipped.

