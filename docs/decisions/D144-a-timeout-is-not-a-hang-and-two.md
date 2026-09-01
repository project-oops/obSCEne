# D144 - A timeout is not a hang, and two timeouts are. The sweep now measures the difference instead of asking an operator to assert it


`sweep.sh` refused to exclude anything on a timeout, and was right to: a run killed by the
clock leaves the same trace as a hang - a `try` with no `res` - so excluding on it blames
whichever check happened to be running when time ran out. Against a slow loader that is a
healthy check removed every round and recorded as a crash.

The cost was that the sweep could not finish against a loader that genuinely hangs, which is
the case it exists for. It stopped and said "raise `--timeout`", and raising it does not
help when nothing is running.

**The distinction needs two runs, not a judgement.** Within one round the two are
indistinguishable. Across two they are trivial: a check that was merely unfinished gets
further when given more time; a hang stops in exactly the same place however long it is left,
because it is not doing anything. So a timeout now doubles the budget and retries the same
build, and only a stopping point *identical under twice the time* is called a hang and
excluded. If it moved, the budget was short, and the doubled value is kept for the rest of
the sweep - a loader slow enough to need it once will need it again.

The alternative was a flag meaning "the budget is generous enough to trust". That puts a
judgement where a measurement will do, and it is wrong exactly when it is most confident.

This is fpPS4's diagnosis promoted into the loop: 300 seconds and 5400 seconds both ended at
`007-responsive/libc` on the same record, so eighteen times the budget bought nothing and
`strspn` - the next entry in the table after `strcmp` - does not return.

**Retries count against `--max-rounds`,** because a retry is a run. Otherwise the bound
covers only rounds that exclude something, and a loader that keeps needing more time keeps
doubling it outside any budget that was set.

### Record count is not progress

Recorded because it produced a confidently wrong conclusion, written into `WORKLOG.md` as
fact, from evidence that was already on screen.

Runs stopped at 24, 25 and 33 records. The 33 came from the round with *no* exclusions and
the 24 from a round with two, so the shorter exclusion list looked like it got further -
which reads as variance rather than a crash walk, and produced "fpPS4 is slow, not hung".

It is the opposite. Excluding a check replaces its several `responsive` records with a single
`skip`, so **the run that progressed furthest through the suite emitted the fewest records.**
The counts moved opposite to the progress they were being read as, and `wc -l` on the report
is not a measure of how far a run got whenever the exclusion list differs between runs.

The HUD says what the count cannot - `SECTION 4 OF 26  CHECKS 11 OF 148` - and it was on
screen throughout. `checks n of m` is the measure; a record total is only comparable between
two runs built from the same exclusion list.

