# D076 - Condition variables and barriers: the operations that cannot block were available all along


Status: decided, closing the non-blocking half of BACKLOG §6d.

That item deferred the whole subsystem because testing it meaningfully needs a second
thread that waits, and a waiter needs a timeout or a broken implementation hangs the run.
The reasoning is right about the *interesting* cases and was applied to everything.

Two operations here cannot block:

- **a barrier of one** is satisfied by the calling thread, so `Wait` returns immediately;
- **signalling a condition variable nobody is waiting on** has no effect and returns.

No second thread, no timeout, nothing faked. The deferral was correct in substance and too
broad in scope, which is worth noticing as a pattern: "this cannot be done safely" is
usually true of a subset.

**Signatures read, not inferred.** `scePthreadBarrierInit` takes a handle, attributes, a
count *and* a name - from the OpenOrbis toolchain headers, with the condition-variable half
independently confirmed by an emulator's own declaration. Inferring the arity from the
POSIX shape would have been the mistake D008 exists to prevent, and the extra `name`
argument is exactly where it would have gone wrong.

**What is still not established is that a waiter is ever woken**, which is the half a title
depends on. Stated in the section rather than left implicit: a green subsystem that has
never been waited on is the false confidence this project exists to avoid.

### The first run separated two things presence could not

Under shadPS4: condition variables pass, barriers fail with "creation reported success and
handed back nothing".

Correct on both counts. shadPS4 implements condition variables and has no barrier
implementation at all, so `scePthreadBarrierInit` resolves to a generic stub returning
success with a null handle - a caller trusting that return gets a null dereference later,
somewhere else.

The census reports both as present. Three checks told them apart.

