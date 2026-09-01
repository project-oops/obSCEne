# Threading contract probes, at the sibling project's request


Three checks added to `015-sync`, and `scePthreadMutexattr{Init,Destroy,Settype,Gettype}`
promoted out of the census into real declarations by the five-step route (D154). Builds clean
at `GEN=5`, which is the target.

**The requested probe would have deadlocked.** Set the recursive attribute, lock twice, see
whether the second returns - that hangs exactly when the answer is "not recursive", which is
the case being tested for. `scePthreadMutexLock` is undeclared in this program for that
reason and stays undeclared. `Trylock` asks the same question, always returns, and separates
"already held" from "deadlock refused" on top.

**No type constant invented, which turned out to be the feature.** POSIX names three mutex
types and fixes none of their values, so the probe sweeps 0-3 and records what happened to
each. The report names which value is recursive without this project ever asserting one.

The first version emitted the same quantity string on every iteration - it could have said a
recursive type exists while being unable to say which. Per-type quantity names fixed it.

**`scePthreadMutexattrGettype` is in the mined corpus**, so "does Settype store anything"
becomes a round trip rather than a byte dump: set, read back, compare. Positive evidence in
the sense of principle 7, where a return code is only argument validation.

**Step five caught a real one.** The host stub for `scePthreadMutexInit` took an attribute
argument and threw it away, passing NULL to `pthread_mutex_init`. Harmless for every check
that existed before, and it would have made the recursion probe report "not recursive" on a
machine where it demonstrably is not - a check that looks correct while measuring nothing.
Fixed, and the host run then reproduced glibc exactly: default type 0, all four candidates
accepted and round-tripping, type 1 alone allowing a second acquisition and the rest
returning 0x10. `PTHREAD_MUTEX_RECURSIVE` is 1, `PTHREAD_MUTEX_NORMAL` is 0, Linux `EBUSY`
is 16 - found without being told any of them.

Two of the six questions needed no probe at all and are already answered in `platform.h`:
`sceKernelCreateSema`'s **argument 1 is a name string**, not a count, and the semaphore
handle is an `int` where a mutex handle is a `void *`. Sent back, because a layer treating
arg1 as a count has every count shifted by one and would see exactly the symptom described.

