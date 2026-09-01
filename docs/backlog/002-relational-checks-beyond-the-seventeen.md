# 11. Relational checks, beyond the seventeen that exist


`018-relational` compares results to each other rather than to an expected value, which means
it needs no authority to check. **Seventeen checks**, and the technique has been taken a good
deal further than this entry once proposed.

### The list this entry used to call "cheap and unexploited" is now four-fifths built

| proposed here | built as |
|---|---|
| two opens of the same path must yield distinct descriptors, and closing one must not close the other | `descriptors-distinct` |
| an operation and its inverse: allocate, release, allocate the same size again | `direct-memory-round-trip` |
| monotonicity: a counter read twice never goes backwards | `clock-never-goes-backwards` |
| idempotence: closing an already-closed descriptor must not succeed twice | `close-is-not-idempotent` |
| ordering: a FIFO queue must return what went in, in that order | **not built** - needs an equeue, which takes a struct |

Seven more went in on 2026-08-24 that this entry had not thought of, and they are aimed at a
failure the original five cannot see. **Every one of the first ten asks about a single
object.** An implementation backing every event flag with one global word passes all of them:
the count counts, the bits set, and the handles are distinct because handles are allocated
separately from the state they name. It is only visible by asking a *second* object whether
it can see the first one's state - `event-flag-state-is-per-object`,
`semaphore-state-is-per-object`, `mutex-state-is-per-object`.

Two more need a second thread, because mutual exclusion and thread identity cannot be
measured from one: `mutex-excludes-another-thread` and `thread-identities-differ`.
`030-thread/self` and `thread-identity-stable` are both satisfied by a function returning the
same constant to everyone, which makes every lock on the platform silently wrong, since they
are all keyed on that value. Neither joins - `scePthreadJoin` blocks and `030-thread` runs
*after* this section, so a join here on a platform whose threads do not finish would take the
checks that diagnose it (D159).

And two on resources: `allocations-do-not-overlap` and `file-position-tracks-reads`.

### "None has found a bug yet" - that claim was false, and the reason matters

This entry used to say *"All pass under shadPS4, which is the most complete loader
available - so this is the expected result."*

Two of them were not passing. They were **skipping**, and had never executed a single
instruction on any target since the day they were written: `descriptors-distinct` and
`close-is-not-idempotent` both required `OBS_CAP_FILE`, which is granted twenty-two sections
later in `040-file` (D158). They reported `skip: a prerequisite capability was not
established` - which is what a platform genuinely without files reports - and this document
read that as a pass.

So the technique had never been given the test this entry claimed it had passed. Gated now by
`obscene-tool caps`, and all seventeen pass against the host build's known-good
implementation, which required teaching the host stubs real event flags, real file operations
and a real direct-memory allocator - because a check that has only ever skipped is not
evidence either.

### What is left, and why it is genuinely blocked

The relations available without a struct layout are largely spent. The ones worth having next
need structures this program will not guess at (D008):

- **ordering**: an equeue must return events in the order they were triggered - needs
  `SceKernelEvent`
- **isolation between mappings**: writing through one mapping must not change another - needs
  `sceKernelMapDirectMemory` to work, which no loader here implements
- **a directory listing must contain what was written into it** - needs `dirent`

That puts the remainder in the same place as §2, which is the honest position: this section
was valuable precisely because it *routed around* the layout blocker, and it has now reached
the far side of what routing around can do.

**`007-responsive` was the same idea aimed at the wrong target.** It covers 54 libc and maths
symbols - exactly where ISO C already supplies a free oracle and the technique is needed
least. The vendor surface is where nothing supplies one.
