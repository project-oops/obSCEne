# D046 - An intermittent failure is only a finding once it has been counted


Status: decided, learned immediately.

The thread-churn crash appeared on the first run and then not at all - five clean runs
in a row on a build that differed only by some progress records, which shifted the
timing enough to hide it. On the earlier build it reproduced **one run in four**.

A single observation of a crash is not a finding, and a single clean run is not a pass.
Reporting either as fact would have been wrong in opposite directions on the same
afternoon.

So: anything that looks intermittent gets run repeatedly and reported with its rate, and
a check that loops emits progress records so a crash inside it can be located rather
than merely attributed. That the progress records themselves changed the outcome is
worth remembering - measuring a race can move it.

