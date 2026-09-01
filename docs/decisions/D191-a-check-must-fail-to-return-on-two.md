# D191 - A check must fail to return on two *consecutive* runs before it is skipped


D181 measured the cost of trusting one sighting: eight consecutive shadPS4 runs, one crash
inside `900-surface/videoout` on the third, and every run after it reporting **`complete`**
with 20,342 records where a clean state gives 36,575. Two corpus groups had been skipped
permanently on the strength of a single intermittent fault, and nothing in the tally, the check
count or the verdict moved - only the raw record count, which nobody compares.

A dangling `try` means "did not return", and that is produced by two different things: a check
that hangs, and a check that was still running when something killed the process. The report
cannot tell them apart. **So stop asking it to.**

### The rule

A first dangling `try` puts the check on a *watch list* and it is **retried**. A second
dangling `try` on the very next run skips it. A `res` for a watched check drops it from the
list, which is what makes the rule *consecutive* rather than *twice ever* - an intermittent
fault has to recur immediately to count.

### Where the watch list lives

On the `resume` record, appended after its two existing fields:

```text
OBS|resume|<skipped>|<ok|full>|<watched id>...
```

The report **is** the state file - there is no second one, by design - and a check being
retried emits an ordinary result rather than a skip, so nothing else in the stream would carry
it forward. Appending is what the format permits without a version bump, and a reader that does
not know about these stops at the two fields it does. (Principle 3.)

### Measured, both directions

Positive, fpPS4 from a cleared state:

| run | died in | `resume` record |
|---|---|---|
| 1 | `007-responsive/libc` | `0\|ok` |
| 2 | `007-responsive/libc` | `0\|ok\|007-responsive/libc` - watched, retried |
| 3 | `007-responsive/math` | `1\|ok` - libc skipped on the second sighting |
| 4 | `007-responsive/math` | `1\|ok\|007-responsive/math` |

Negative, on the host build, seeding a watch entry for a check that answers normally:

```text
seeded:  OBS|resume|0|ok|010-kernel/process-time
run 1:   OBS|resume|0|ok|010-kernel/process-time   (answered; still watched)
run 2:   OBS|resume|0|ok                           (dropped; 0 skipped)
```

The guard has been watched both accepting and rejecting, which is the standard this project
sets for one.

### What it costs

One extra run per blocker. fpPS4 has 44, so its convergence roughly doubles - paid in machine
time, unattended. Against that: a report that says `complete` while missing half its
measurements, which is the failure this program exists to make impossible.

### What it does not fix

A check that hangs *reliably* is still skipped, and should be - that is the mechanism working.
And a run killed twice in a row at the same place still poisons, which is the residual case. A
runner that told the module "I killed you" would close it, but the runner and the module do not
share a filesystem view, so that stays open.

Status: **decided** - measured in both directions.


