# D154 - Three threading contract probes, added at the sibling project's request


**Three threading contract probes, added at the sibling project's request. The recursion
policy is the one that matters, and the obvious way to probe it deadlocks.**

The sibling's threading layer defaults mutexes to non-recursive "because it is POSIX's" and
has no way to check. Guessing wrong there does not produce a wrong answer, it produces a
whole-process hang: a guest re-taking a lock it already holds proceeds under one policy and
stops forever under the other. Its semaphore work took two titles from 45 calls to 222, so
the layer is load-bearing while resting on inferences nobody has checked.

### The proposed probe would hang, and precisely when it matters

Settype recursive, then lock twice, then see whether the second call returns - that hangs
**exactly when the answer is "not recursive"**, which is the case being tested for, and takes
the rest of the suite with it. `scePthreadMutexLock` is not declared in this program for that
reason and stays undeclared for this.

`scePthreadMutexTrylock` asks the same question, always returns, and answers in more detail:
success means the second acquisition was allowed, and the failure code separates "already
held" from "deadlock refused" where the platform distinguishes them.

### No type constant is invented, and that turned out to be the feature

POSIX names three mutex types and fixes none of their values, so hardcoding one would be the
invention D008 forbids and would make the check measure a guess. The probe sweeps candidates
0-3 and records what happened to each, with a quantity name per type - a report saying "type
1 allowed a second acquisition, types 0, 2 and 3 refused with 0x10" states which value is
recursive without ever naming a constant.

The first version emitted the same quantity string four times, which would have said a
recursive type exists while being unable to say which. A table of per-type names fixes it;
the runtime has no string formatting and should not grow any for this.

### `Gettype` exists, which upgrades the question

The sibling asked whether the `Settype` calls write anything, on the reasoning that a `Get`
counterpart would otherwise read whatever the stack held. `scePthreadMutexattrGettype` is in
the mined corpus, so the question becomes a **round trip**: set a value, read it back,
compare. That is a positive check in the sense of principle 7 - it proves the attribute
object carries state, where a return code proves only argument validation.

### Validated against a known-good implementation first

Step five of the checklist, and it earned its place. On the host build the checks report
default type 0, all four candidates accepted and round-tripping, and type 1 alone allowing a
second acquisition with the others returning 0x10. glibc's `PTHREAD_MUTEX_RECURSIVE` is 1,
`PTHREAD_MUTEX_NORMAL` is 0 and Linux's `EBUSY` is 16 - the check found all three without
being told any of them.

That required fixing a host stub: `scePthreadMutexInit` took an attribute argument and
discarded it, passing NULL to `pthread_mutex_init`. Harmless for every check that existed
before, and it would have made this one report "not recursive" on a machine where it
demonstrably is - a check that looks right while measuring nothing.

### What is deliberately not probed

"Unlock a mutex held by another thread" needs a second thread to be holding it at the moment
of the call. That is an interleaving, and a probe whose answer depends on scheduling produces
confident noise. The unheld case is deterministic, asks most of the same question, and is
what `015-sync/mutex-unlock-unheld` does - reporting `partial` when the platform allows it,
because an implementation that permits it puts two holders in one critical section as soon as
a guest relies on it.

Arity remains unanswerable by probe. `scePthreadCreate` is declared here at 5 with the name
last and has been called that way in three places for some time, which agrees with the
sibling's inference - but on this ABI extra arguments sit in registers the callee ignores and
the caller cleans up, so no probe distinguishes 4 from 5 by crashing. That is the same
property that makes the blind prober safe, and here it is the thing in the way.

### Already answered without a probe

Two of the six questions the sibling raised are settled in `platform.h` today, and both are
worth sending back before any more work is done on them:

- `sceKernelCreateSema` is declared `(int *out, const char *name, uint32_t attr, int init,
  int max, const void *opt)`. **Argument 1 is a name string.** A layer treating it as a count
  has every subsequent count shifted by one position, which would present exactly as "the
  counts are wrong and nothing notices".
- The semaphore handle is an `int` written through `out`, where a mutex handle is a `void *`.
  That difference is already encoded here and is the answer to whether the two are the same
  shape: they are not.

Both are `OBS_FROM_SPEC`, meaning public interface documentation rather than hardware, so a
probe would still upgrade them. Neither is a guess.

