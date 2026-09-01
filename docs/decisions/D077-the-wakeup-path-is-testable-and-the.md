# D077 - The wakeup path is testable, and the timeout was never the thing that was needed


Status: decided, on the observation that telemetry beats a deadline.

Every version of this problem assumed a waiter needs a timeout or a broken implementation
hangs the run. That is true of a thread that waits. **The run is not that thread.**

`015-sync/condvar-wakes-a-waiter` keeps the main thread out of every blocking primitive.
It spawns a waiter, sleeps a fixed interval, signals, sleeps again, and reads a flag -
every step bounded by a number this program chose. A wakeup that never arrives strands the
waiter while the run continues and reports exactly that.

**The stuck thread is never joined**, deliberately. Joining it would import the hang the
whole design exists to avoid, so it is left where it is and `exit` takes it down with the
process. Two handles leak on a platform where the wakeup is already broken, which is the
cheaper of the two mistakes available.

**The flag is a state, not a boolean.** `WAITING` and never `WOKEN` is a wakeup that never
arrived; never reaching `WAITING` means the waiter died before it got there. Different
faults, different causes, and a boolean would have merged them into one uninformative
failure.

`volatile` rather than atomic, because this is freestanding and `stdatomic.h` is not
guaranteed. The sleeps between writes and reads involve the kernel, which is enough in
practice on this architecture. Recorded because it is a real limitation, not a rigorous
one.

### It required making the host build real

The probe could not run against a stubbed `scePthreadCreate`, so the host now uses real
pthreads - creation, join, condition variables, mutexes and barriers. That is the same
argument as everywhere else in this file: a check that has never passed a working
implementation is not evidence, and this is the check whose *design* is the interesting
part, so shipping it unexercised would have been the worst case of it.

Three checks that had been failing on the host only because they met a stub now pass:
`015-sync/thread-churn`, `030-thread/create` and `030-thread/join`. Their failures were
never about the platform.

