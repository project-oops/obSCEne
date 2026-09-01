# 2026-08-24 - the relational section grew, and two of it had never run


Item 5 on the backlog was "more relations". Reading the section first found something worth
more than the additions: **`018-relational/descriptors-distinct` and
`close-is-not-idempotent` had never executed a single instruction**, on any target, host
build included, since the day they were written.

Both declared `OBS_CAP_FILE`. Capabilities are granted in running order from nothing, and
the check that grants `OBS_CAP_FILE` is in `040-file` - twenty-two sections after this one.
They did not fail. They reported

```text
skip: a prerequisite capability was not established
```

which is what a platform genuinely without files reports. The message was true and it was
about the wrong thing, and no amount of reading the report could tell those apart. Four
loaders' worth of reports carried it. (D158)

`obscene-tool caps` gates it now: registry order, accumulate `provides`, report any
`requires` nothing earlier can satisfy. Run against the tree with the requirement put back,
it names the grant site rather than just the offence:

```text
018-relational/descriptors-distinct wants OBS_CAP_FILE - granted later, by 040-file/open-rejects-missing
```

### Seven new relations, and what they are aimed at

The existing ten all ask about **one** object. An implementation backing every event flag
with a single global word passes all of them - the count counts, the bits set, the handles
are distinct because handles are allocated separately from the state they name. Three of the
new checks ask a *second* object whether it can see the first one's state, which is the only
way that shape is visible, and it is the shape a subsystem takes when the API is needed
before the implementation is.

Two more need a second thread, because mutual exclusion and thread identity cannot be
measured from one. `030-thread/self` and `thread-identity-stable` are both satisfied by a
function returning the same constant to everyone - which makes every lock on the platform
silently wrong, since they are all keyed on that value. Neither joins: `scePthreadJoin`
blocks and `030-thread` runs *after* this section, so a join here on a platform whose
threads do not finish would take the checks that diagnose it. The child sets a flag last and
the parent spins a bounded number of iterations. (D159)

The last two: two direct-memory allocations held at once must not name the same memory, and
a file's position must advance by the number of bytes `read` said it returned, with a
re-read of the same offset giving the same bytes.

### Making them evidence

Four of the seven passed on the host immediately; three skipped for want of stubs, which by
rule 5 means they were not evidence. So the host build grew real implementations: event-flag
set/clear/poll over the table it already had, file operations forwarded to the real ones
with `/app0/eboot.bin` redirected to `/proc/self/exe`, and a direct-memory allocator with a
free list over a half-gigabyte arena of offsets that are never mapped.

| host build | before | after |
|---|---|---|
| pass | 101 | 112 |
| skip | 38 | 29 |

Ten checks moved off `skip`, including the two that had never run. All seventeen relational
checks now pass against a known-good implementation. `020-memory/map` went `skip → fail` -
honest, and better: it was only skipping because the allocation ahead of it failed, and the
host genuinely does not implement mapping.

### The gates caught me twice on the way

`obscene-tool guards` found two of the new checks calling a symbol they had not tested the
address of - `sceKernelOpen` and `sceKernelCreateSema`, both D058 to the letter. And the
first host arena was sixteen megabytes, which made `020-memory/direct-size` report
`partial: implausibly small` against its 256 MiB threshold and left that check's pass branch
unexercised on the one platform where the answer is known. Raised to half a gigabyte, which
is not a plausible lie: nothing is ever mapped, so the span costs an integer, and the
allocator really does hand out offsets across it and really does refuse past the end.

