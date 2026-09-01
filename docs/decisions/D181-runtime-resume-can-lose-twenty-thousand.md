# D181 - Runtime resume can lose twenty thousand measurements and still report success


D172 skips a check whose `try` had no `res` on the previous run. That trace means "did not
return" - and a run killed by a timeout part-way through a check leaves exactly the same
trace as one that hung. The mechanism cannot tell them apart, and neither can the report.

Measured, on shadPS4, same binary, minutes apart:

| | records | `sym` records | verdict |
|---|---|---|---|
| poisoned state | 16,453 | **15,215** | `complete`, 515 of 515 checks |
| cleared and reconverged | 36,402 | **35,161** | `complete`, 515 of 515 checks |

**Both say complete. Both ran every check.** The difference is four `900-surface/corpus_*`
census libraries - `libSceLibcInternal` and `libSceNKWebKit` among them - that a timed-out run
had marked as blockers. Each carries thousands of symbols, so twenty thousand measurements
vanished behind a green verdict.

### Why this is worse than it sounds

The `res` count is unchanged, because a skipped check still emits a `res`. So every summary
this project prints - the tally, the frontier, the compatibility row - is identical between
the two runs. Nothing in the report distinguishes them except the raw record count, which
nobody reads.

That is the failure this repository keeps meeting, in the mechanism built to *avoid* one of
its instances. A skip that is a **result** ("the platform could not get there") and a skip
that is a **guess** ("a previous run died here, possibly because I killed it") are printed
identically, and only one of them is evidence.

### What follows

- **A timeout is not a hang, and the runner already knows which happened.** `run-emulator.sh`
  computes `timed_out` precisely so the two can be told apart (its own comment says so). That
  fact never reaches the module, so the module cannot decline to learn from a run that was cut
  short. It should: a report from a run the runner killed is not evidence about any check.
- **`sym` count belongs in the compatibility table.** It is the only number that moved, and it
  moved by more than half.
- The fpPS4 convergence loop already checks each discovered blocker against the 44 known ones
  and flags anything outside. That guard was written for this hazard and is the reason it was
  caught here; the sweep has no equivalent and that is how it bit.

### Measured again, cleanly, and it is worse than the first reading suggested

Eight consecutive shadPS4 runs of one binary, nothing else changed:

```
36,341  complete
36,342  complete
13,307  CRASHED inside 900-surface/videoout
20,342  complete     <- and every run after it
20,342  complete
20,342  complete
```

**One crash costs sixteen thousand measurements permanently.** The crash put two corpus groups
on the skip list - `libSceGnmDriver` and `libSceNKWebKit` - and every run afterwards reported:

| | records | `sym` | tally | verdict |
|---|---|---|---|---|
| poisoned | 20,342 | 19,111 | 454/6/37/18 | **complete** |
| state cleared | 36,575 | 35,330 | 456/7/37/15 | **complete** |

Deleting the resume file restores it exactly, which confirms the mechanism and the cost.

### Made visible, since it cannot yet be prevented

`obscene-tool compat` now counts skipped `900-surface/` groups and marks the census cell
`(N groups skipped)`. Counted, not inferred from the total: a first attempt compared each
loader's census size against the largest in the table and fired on a **one**-symbol difference
between the host and the emulators, which is noise, and said nothing about the cause.

Verified by forging a report with two skipped groups and confirming the marker appears against
a clean run that does not carry it.

This makes the loss legible. It does not stop it happening, and the run that produced 20,342
still says `complete`.

Status: **assumed** - the diagnosis is measured, the fix is not yet built.

