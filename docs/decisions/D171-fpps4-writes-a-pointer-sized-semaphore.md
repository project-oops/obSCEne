# D171 - fpPS4 writes a pointer-sized semaphore handle through an `int *`, and the check that found it described the wrong thing


`018-relational/semaphore-state-is-per-object` creates two semaphores into two adjacent `int`s
and signals the first. On fpPS4 it failed with *"a fresh semaphore refused a signal"* and
`0x80020016` - EINVAL - which sends a reader at the signal.

The signal was fine. fpPS4's `sem_enter` returns EINVAL for a **null handle**, and the handle
was null because creating the *second* semaphore had overwritten the first.

| implementation | out-parameter |
|---|---|
| shadPS4 | `OrbisKernelSema = Common::SlotId`, wrapping a `u32` |
| PS5PCEM | `output: ?*u32` |
| fpPS4 | `PSceKernelSema = ^SceKernelSema`, a pointer to a struct |

Two say four bytes, one says eight. `platform.h` declares `int *out`, which follows the two -
so this is an **overrun**, not a disagreement about arity. Whether the two adjacent `int`s sat
in the order that made it visible was the compiler's choice, which is why a single-semaphore
check on the same platform passes.

### The finding is real and the diagnosis was accidental

A corrupted handle fails at whatever touches it next, and that is never where the fault is.
The check reported the symptom in the vocabulary of the wrong subsystem, and it took reading
three implementations' source to get back to the cause.

So the question is now asked directly. `018-relational/handle-fits-its-out-parameter` puts a
guard word immediately after the handle, in one struct so adjacency is the compiler's contract
rather than a hope about stack layout, and reports what landed there:

```text
measure  guard-after-handle  0x0
fail     the call wrote past the end of the int it was given
```

`0x0` because the upper half of a heap pointer below 4 GiB is zero. That is `130-layout`'s
oversized-buffer-and-guard trick applied to a four-byte destination.

### Judged narrowly, because the wide judgement is not ours to make

A platform whose handle is genuinely pointer-sized is not committing an error by writing eight
bytes - it is disagreeing with this program's declaration. The record says which happened. What
is never acceptable is the silent half: a caller who allocated four bytes lost whatever sat in
the next four and nothing told them.

### The relation check skips rather than fails there

It checks its guards before asking the relation, and where they are disturbed it **skips and
names the check that measures it**. On such a platform the question cannot be asked - a handle
wider than its slot is unusable through that slot - and failing the relation would blame
per-object state for a width problem.

The report now explains itself: one check names the cause, the other points at it.

### This bears directly on something we told the sibling project

They found the mirror of this on their own side - a `u64` handle written through a guest
`int` out-parameter, corrupting the neighbour - and narrowed it to `i32` **on the strength of
our signature**. That was the right call and this is the evidence for it: two of three
implementations agree with the narrow reading. Worth sending, because a third implementation
doing the wide thing is exactly the counterexample that would make somebody re-open a settled
decision, and it should arrive with the count attached.

