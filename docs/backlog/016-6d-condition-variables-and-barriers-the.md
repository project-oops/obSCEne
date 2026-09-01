# 6d. Condition variables and barriers - the non-blocking half is checked


Was deferred because testing either meaningfully needs a second thread that waits, and a
waiter needs a timeout or a broken implementation hangs the run.

**That was true of the interesting cases and got applied to the whole subsystem.** Two
operations here cannot block at all:

- a barrier initialised with a count of **one** is satisfied by the calling thread, so
  `Wait` returns immediately;
- `Signal` and `Broadcast` on a condition variable nobody is waiting on are defined to
  have no effect and return.

Three checks, no second thread, no timeout, nothing faked. Signatures read from the
OpenOrbis toolchain headers rather than inferred from the POSIX shape - `BarrierInit`
takes a count *and* a name, and guessing that arity is what D008 forbids.

### And the wakeup path, which needed no timeout after all

The deferral assumed a waiter must be given a deadline or a broken implementation hangs
the run. True of a thread that waits - and **the run is not that thread**.

`015-sync/condvar-wakes-a-waiter` spawns a waiter, sleeps a fixed interval, signals,
sleeps again, and reads a flag. Every step has a bound this program chose. A wakeup that
never arrives strands the waiter, and the run carries on and reports it. The stuck thread
is deliberately never joined - joining is the hang the design avoids - and `exit` takes it
down with the process.

The flag advances through named states rather than being a boolean, so "waited and was
never woken" and "died before it got as far as waiting" stay distinguishable. They have
different causes and a boolean would merge them.

Validated on the host with real threads, where a real signal wakes a real waiter, so a
pass means the mechanism was exercised rather than merely not crashed. Passes under
shadPS4 too.

**What is still not established:** anything about timed waits, which need a `timespec`
(§2), and anything about barriers with more than one participant.

### It found something immediately

Under shadPS4, `condvar-lifecycle` passes and `barrier-of-one-releases` fails with
"creation reported success and handed back nothing".

Exactly right: shadPS4 implements condition variables and has no barrier implementation at
all, so `scePthreadBarrierInit` resolves to a generic stub that returns success and leaves
the handle null. A caller trusting that return gets a null dereference later, elsewhere.

**The census reports both as present.** Three checks separated an implemented subsystem
from an unimplemented one that presence could not distinguish.

