# 16. The futex timeout, and what the two threading probes still owe

`031-stackattr` and `032-syncaddr` were authored on 2026-09-07 (D322) against the sibling
emulator's two open premises about threading. Both are validated on the host build and neither
has been on hardware. This is what they deliberately do not reach.

## The wait's third register

`sceKernelSyncOnAddressWait` is declared with two arguments - an address and a value - which is
what three retail titles pass, every observed call leaving the third register zero. FreeBSD's
`_umtx_op(2)`, the citable analogue, has a timeout slot there.

**It is not probed, and it cannot be probed the way the width was.** The width question has two
safe outcomes: a 64-bit comparison returns, a 32-bit one blocks on a worker the main thread
releases anyway. A timeout has no such symmetry - the measurement *is* how long the call waits,
so a wrong guess at the unit either returns instantly and establishes nothing or outlives the
run. Declaring the parameter to find out would be declaring an arity nothing establishes, which
principle 2 forbids.

What would settle it: a session that can bring a known unit to it - a wait on a matching word
with a bounded third argument on the worker thread, timed against `sceKernelGetProcessTimeCounter`,
with the existing wake as the recovery path if it never returns. It is a small addition to
`032-syncaddr` once the unit is a hypothesis rather than a blank.

Until then the sibling refuses a non-zero third register rather than model it, which is the
honest state on both sides.

## What a hardware run turns into an answer

Neither section has run on a console. Every check reads `assumed` or `derived` until one does,
and these are the records to read out of it:

| record | what it settles |
|---|---|
| `031-stackattr/address-is-the-base` placement | **1** the address is the stack's lowest byte, **2** its top. The sibling assumes the first and a collector's scan bound depends on it |
| `031-stackattr/self-describes` address and size | the real span of the main thread's stack, against which its `sceKernelIsStack` bounds can be read |
| `031-stackattr/fresh-attr-names-no-stack` | whether a set nothing configured reports a stack at all, or leaves the out-parameter untouched |
| `032-syncaddr/compare-width` | **64** or **32**. The sibling models 64 from an export layout, never measured |
| `032-syncaddr/wait-returns-on-mismatch` code | what a wait on an already-differing word answers - the path a caller takes on every wake that arrived first |
| `032-syncaddr/wake-releases-a-waiter` code | what a wake answers with one waiter blocked, and whether it is a status or a count |
| `032-syncaddr/wake-with-no-waiter` code | the same with nobody waiting. A count would come back zero here and one above, which tells the two apart |

## The sized variants, still censused

`libkernel_sync_on_address2` exports `sceKernelSyncOnAddressWait8`, `16`, `32` and `64`, and they
remain in the census for presence only. Nothing calls them: no title anybody has traced imports
one, and the plain pair is what the Unity shim uses. If `compare-width` comes back **32**, the
`64` spelling becomes the interesting one and this is where to start.

## What the sections needed that did not exist

`scePthreadAttrInit` and its family had **no host implementation**, so
`010-kernel/thread-attributes` had been skipping on every host build since it was written - a
check that had never passed a known-good implementation, which CLAUDE.md is explicit is not
evidence. Stubs were written for the attribute family and the futex pair, and that check now
runs and passes. Worth knowing that the host build's skips are not all inert: some of them are
checks nobody has validated.
