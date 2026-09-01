# D073 - Five more relations, and the one with the worst failure mode


Status: decided, extending D068.

`018-relational` is ten checks now. The additions:

- **two opens of one path must give two descriptors**, and closing one must leave the
  other live. This is the worst failure mode in the section: an implementation returning
  the same descriptor twice passes every check in `040-file` - each open succeeds, each
  read works - and then a caller that closes one has silently closed the other's. What
  breaks is elsewhere, later, in code that did nothing wrong.
- **closing twice must not succeed twice**, which is how a double-close hides.
- **a clock must never go backwards**, sampled across real work so a coarse clock is not
  failed for being coarse.
- **two live mutexes must not share a handle** - the same relation as the event flags, on
  a different subsystem, because handle tables are per-type and one being right says
  nothing about another.
- **a held default mutex must refuse a second lock from the same thread.** Written in the
  try form throughout, so a recursive mutex is reported rather than deadlocking the run.
  Marked `assumed`, not `spec`: POSIX leaves the default type implementation-defined, so
  what this really establishes is that the lock tracks state at all.

**The mutex relations needed real mutexes in the host build** to be validated, as the event
flags and semaphores did. Constants would have left them unexercised, and a check that has
never passed a working implementation is not evidence.

Under shadPS4: eight pass, two skip. The two that skip are the file relations, because the
check that establishes `OBS_CAP_FILE` is one of the two excluded for ending the process -
so excluding a crashing check also withholds the capability it provides. That is correct
behaviour and worth knowing: the exclusion list has reach beyond the checks named in it.

