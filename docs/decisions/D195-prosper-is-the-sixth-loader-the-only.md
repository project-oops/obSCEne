# D195 - prosper is the sixth loader, the only headless one, and the first to have ground truth to check us against


`boot_trace` takes a game directory and runs it with no window at all. That matters beyond
convenience: every other loader in the sweep opens a window on the user's desktop, and a run
long enough to be minimised is a run whose screenshot fails - while the screenshot *is* the
report for any loader whose text channel does not work. prosper has neither problem, so it can
be run as often as wanted. `scripts/prosper-run.sh`.

obSCEne boots under it and the resume mechanism converges as designed, each blocker costing two
rounds under D191's two-consecutive-failures rule:

```text
round 1  118 records  last=015-sync/semaphore
round 2  165          last=018-relational/semaphore-counts
round 3  165          last=018-relational/semaphore-counts
round 4  180          last=018-relational/semaphore-state-is-per-object
round 5  180          last=018-relational/semaphore-state-is-per-object
round 6  181          last=018-relational/handle-fits-its-out-parameter
```

### The result that could not be got anywhere else

`hle_registry_dump` prints prosper's own registry: 1,201 `real` implementations, 6
`placeholder`, and anything absent is unimplemented and logged as such at run time. **The
loader states its own answer.**

`007-responsive` guesses that answer from behaviour alone - give a function two inputs whose
results must differ, and call it a stub if both come back the same. Plausible, and until now
never checked against anything that knew. Checked now, over every function the section tests:

| obSCEne's verdict | prosper's registry | count | agreed |
|---|---|---|---|
| `responds` | real | 45 | 45 |
| `silent` | absent | 9 | 9 |

**54 of 54, no disagreements in either direction.** No false stub, no missed one. That is the
first independent evidence the heuristic measures what it claims to, and it is the kind of
positive result principle 7 asks for: it proves the section works, where a negative check would
only have proved it rejects bad input.

### The SIGSEGV is prosper's, and the announcement said so first

Round 6 died with a fault report naming a host frame:

```text
RUN ENDED: kind=2  SIGSEGV at addr=0x1af22510  rip=... (mapped/host)
  backtrace: eboot +0x442695 / eboot +0x46252d / mapped/host +0x1
```

`rip` is in host code with obSCEne frames only as callers, so obSCEne called into prosper's HLE
and that faulted dereferencing its first argument. The five semaphore entries are all `real` in
the registry, so this is a real implementation crashing, not a missing one. The check that
provoked it holds a guard word beside its handle and never dereferences a handle at all.

obSCEne's contribution was the dangling `try` with no `res`, which named the call before it was
made. That is announce-before-attempting working on a loader it had never met, first run.

### What it does not establish

Nothing about hardware. prosper is a fourth reader agreeing with us about module layout and a
sixth loader running our code; it is not a console. The responsive result is evidence about
**the heuristic**, measured against one implementation's idea of its own completeness.

Status: **derived** - 54/54 measured against a registry the loader publishes about itself.

