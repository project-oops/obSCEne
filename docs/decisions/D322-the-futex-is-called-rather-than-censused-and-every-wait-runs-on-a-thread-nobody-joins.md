# D322 - the futex is called rather than censused, and every wait runs on a thread nobody joins

**assumed** - 2026-09-07

Two sections were added for the sibling emulator's two open premises about threading:
`031-stackattr`, for what a thread attribute set says about a running thread's stack, and
`032-syncaddr`, for the platform's futex. Both promote names out of the census into real
declarations, and the second has to call something that blocks. This records the four choices
that carry risk, because in each of them the obvious approach is wrong.

## 1. The futex is called, and a wait cannot be written as a `try` form

The no-block rule is absolute in this program and has been paid for twice: a probe that hangs
loses every check behind it. `016-syncbounds` honoured it by measuring `sceKernelWaitSema`
through the non-blocking `sceKernelPollSema` (D321).

**There is no `Poll` counterpart to a futex wait.** Blocking *is* the function, and the pair
that surrounds it is the whole measurement - a wait nothing releases has not been observed
working. So the rule cannot be satisfied by choosing a different symbol, and the choice is
between not measuring the busiest pair on the platform and measuring it another way.

The other way already exists here. `015-sync/condvar-wakes-a-waiter` puts its one unbounded
call on a thread nobody joins, advances a volatile counter through known states, and lets the
main thread observe with sleeps - *"if it never returns, this thread stays here and the run
does not"*. `032-syncaddr` uses exactly that arrangement for all three of its waits: the main
thread only ever sleeps, reads the counter, and calls the **wake**, which cannot block.

So the rule is honoured in substance - nothing that can block is on the path the suite depends
on - while the letter of "written as the `try` form or not at all" does not apply to a function
with no `try` form. That is the reviewable call, and it is why this entry is `assumed`.

## 2. The comparison width is measured by a case where blocking *is* an answer

Whether the wait compares 32 bits or 64 is the sharpest open question: the sibling models it as
64-bit from an export layout in which the wait shares an entry point with the `Wait64` spelling,
and has no way to check.

The discriminating case is a word whose high half differs from the value passed and whose low
half matches. A 64-bit comparison returns at once; a 32-bit one blocks. Ordinarily "it blocks"
would be an unusable outcome - but on the worker thread it is simply the other reading, and the
main thread releases it with the wake it has already proved works. **Both answers are
recoverable**, which is what makes the measurement safe rather than a gamble.

The check reports `64` or `32` and grades neither. It was validated both ways: against a
64-bit host stub it reports 64, and against a deliberately 32-bit one it reports 32 and records
the worker's release.

## 3. No timeout is declared, so none is probed

`_umtx_op` has a timeout slot and the wrapper may carry one. The sibling refuses a non-zero
third register rather than guess at its unit, and every guest call anybody has observed leaves
that register zero.

Declaring a third parameter to probe it would be declaring an arity nothing establishes, which
principle 2 forbids - and unlike the width, a timeout cannot be measured by a case where both
outcomes are safe: the measurement *is* how long it waits, so getting the unit wrong is either a
wait that returns instantly and says nothing or one that outlives the run. It is left out and
recorded in `docs/backlog/024` for a session that can bring a known unit to it.

## 4. The stack address is placed against the frame that asked, not against a constant

`scePthreadAttrGetstackaddr` hands back an address, and whether that is the lowest byte of the
stack or its top differs by the whole size of the stack. The sibling answers FreeBSD's
convention and records it as an assumption; three retail titles compute a garbage collector's
scan bound from it, and got that bound from a placeholder for months (its D575).

The check takes the address of one of its own locals - inside the asking thread's stack by
construction - and asks which side of the reported address it falls on. Inside
`[address, address + size)` is a base; inside `[address - size, address)` is a top; neither is
graded a failure, because a region that does not contain the frame asking about it is wrong
under both conventions.

No constant is invented and no layout is assumed; the arithmetic is the whole method.
`sceKernelIsStack` reports bounds of its own and is consulted as a second witness where it
resolves, but nothing is asserted on it - platform.h is explicit that those two out-pointers are
only *almost certainly* the region's bounds.

## What the host build could and could not settle

All seven checks pass against host stubs written for the occasion, and each was then made to
fail on purpose, which is the part that makes them evidence:

| break | what the check did |
|---|---|
| the wake does nothing | `wake-releases-a-waiter` failed naming the state the worker stopped in, `compare-width` skipped rather than reporting a width it could not observe, **and the run completed** |
| the comparison reads 32 bits | `compare-width` reported 32 and recorded the worker's release |
| the address is the stack's top | `address-is-the-base` reported a top |
| the region contains no frame | `address-is-the-base` failed, naming the address |

A stub can implement only one convention, so the host proves the mechanism and the arithmetic,
never the platform's answer. Which convention each stub picks is stated in its comment rather
than left for a reader to infer.

Adding two sections also overran `OBS_SCREEN_MAX`, and the compile-time assertion D259 put in
`registry.c` caught it. It was raised from 48 to 56 - what two columns hold, by `screen.c`'s own
note, rather than arbitrary headroom, so the next session past it has to write the third column
that note describes.
