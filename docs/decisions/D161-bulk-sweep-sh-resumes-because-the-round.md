# D161 - `bulk-sweep.sh` resumes, because the round budget is the binding constraint and not the size of the surface


A round ends at the first function that does not return, so rounds needed is "number of
functions that fault". Against shadPS4 that is roughly one in five: seventeen rounds reached
index 92 of 32,466 - the prober walks the corpus and the curated
census, so a full pass is a much longer proposition than the surface count suggests.

Without resuming, a sweep that exhausts `--max-rounds` leaves two bad options - throw the
accumulation away and pay for it again, or concatenate report fragments by hand. The second
is precisely the "reading one round as though it were the answer" mistake the header of that
script exists to prevent someone making, so the operation is supported rather than
improvised.

`--start` alone is not enough, because the report file is truncated on every invocation. Both
have to move together, and the index is read out of the report rather than typed by the
operator: a number copied from scrolled-back terminal output is a number that can be wrong.

### Two things this got wrong first

The index had to be taken **numerically**. The round loop's own version of this query uses
`tail -1` safely, because a single round has exactly one unanswered call. An accumulated
report has one per round, and sorting hex text puts `0x9` after `0x10` - which would resume
*behind* ground already covered and silently re-walk it. Converted one value at a time,
because `strtonum` is a gawk extension and the build VM's awk is not gawk.

A run that ended *blocked ahead of the section* contributes no new announcement, so resuming
lands on the previous round's index and re-walks it. That is correct rather than merely
tolerable: the exclusion it added lives in the VM's list and persists, so the retry is the
one the round loop would have made anyway.

### "Did not return" is not "faulted"

The fault list needs reading with one caution, now in the script's header.

`scePthreadExit` appears in it every time, at index 81 under shadPS4 - and it is **correct**.
That function does not return by design. A function that blocks on a null argument does not
return either. Neither does one that crashes. The announcement says only that no answer came
back, and deciding which of the three it was needs the function's own contract.

This does not weaken the mechanism - an announcement that named only crashes would need to
know in advance which calls crash, which is the thing being measured. It means the list is
input to a judgement, not the judgement.

