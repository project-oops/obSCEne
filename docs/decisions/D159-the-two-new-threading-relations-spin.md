# D159 - The two new threading relations spin for their child rather than joining it


`scePthreadJoin` blocks. `018-relational` runs at 018 and `030-thread` - where join is
established as working - runs after it. A join here on a platform whose threads do not finish
hangs, and takes every check behind it, including the ones in `030-thread` that would have
diagnosed the problem. The section's own rule is that anything which can block is written as
the `try` form or not at all, and there is no `tryjoin`.

So the child sets a flag **last** and the parent spins a bounded number of iterations waiting
for it. Three things follow from that ordering and each matters:

- The child touches nothing shared after setting the flag, so a parent that has seen it set
  may safely tear down the mutex the child was using.
- Giving up is its own outcome - `partial`, "did not finish within the budget" - rather than
  being folded into a failure. A slow platform and a broken one are different findings.
- When the contending child does *not* finish, `mutex-excludes-another-thread` deliberately
  leaks the mutex rather than destroying it. The child may still be inside `trylock` on it,
  and tearing it down underneath would turn an inconclusive result into a crash somewhere
  unrelated. One leaked mutex in a process that is about to end is the cheaper outcome.

The threads are never joined, so they are left behind. That is a real cost and it is the
lesser one: an unjoined thread that has finished its body costs a stack, and a hung join
costs the rest of the report.

