# Checks chosen from an emulator's own release notes


`015-sync` was written against what an emulator said it had just fixed, as a test of the
method rather than of the emulator (D045). It found two things on the first run.

**A cleared event-flag bit still polls as set.** In the release that rewrote event flags.
Set a bit, poll it (present, correct), clear it, poll again - still present.

**Thread churn crashes the emulator about one run in four**, inside its own `ExitThread`
assertion. Forty create-and-join cycles. Two would never have seen it, and two is what a
normal thread check does.

Both functions were already censused - reported present, never called - so the suite had
said nothing about either while both were broken. That is the whole argument for
invocations over existence tests, and it took one section to demonstrate.

### The methodology lesson, learned the hard way

The churn crash appeared once, then five clean runs in a row on a build that differed
only by some progress records. It reproduced 1-in-4 on the earlier build. Adding
instrumentation moved the race.

A single crash is not a finding and a single clean run is not a pass; both would have
been reported as fact on the same afternoon. Anything intermittent now gets run
repeatedly and reported with its rate (D046).

### Where the coverage stands

    81 checks, 16 sections, 530 records
    expectations: 51 assumed, 30 spec, 0 hardware

The `assumed` majority is the honest picture and the reason a hardware run matters: it
does not check the suite, it upgrades it.

### POSIX primitives, and a clean result worth having

Mutexes, read/write locks and counting semaphores converted from census entries to real
invocations, all `spec` provenance because POSIX settles what they do.

All three pass. That is worth recording rather than glossing over: the event-flag failure
next to them means more *because* its neighbours are clean. A suite where everything
fails says nothing about any particular thing.

Every one is a `try`, never a blocking call. A lock whose implementation is broken does
not refuse, it hangs, and a probe that hangs loses everything behind it. The pair "a
fresh lock can be taken" and "a held lock cannot be taken again" pins the semantics
without ever waiting on anything - so does "two readers yes, a writer no", and so does
"two tokens in, two tokens out, and not a third".

    17 sections, 88 checks, 523 records, well-formed
    expectations: 53 assumed, 1 documented, 34 spec, 0 hardware
    59 pass  3 partial  20 fail  6 skip

### A third loader, and three distinct failures

craziiEmu (C#, shares a lineage with SharpEmu) builds from source and is a console
application, so unlike Kyty its output can be captured. One run:

    [LOADER] EntryPoint: 0x4177D0, ImportStubs: 0
    [LOADER] Setup 0/0 import stubs
    [LOADER] Calling guest entry...
    [LOADER] Execution stalled with no import progress for 20s (imports=0)

It loads the module, maps it, finds the entry, and resolves **nothing**. Its constants
carry eleven vendor tags - every one describing a *table* - and none of the four that say
who a module is and what it imports from. It keys its lookup off `NeededLibraries`, taken
from standard `DT_NEEDED`, which a vendor-format module does not have. So there is nothing
for it to match against.

Stated carefully: that is what this build does with this module. It is consistent with the
constants it defines, and a real module declares its imports the same way ours does.

**Three loaders, three different walls**, which is the argument for testing against more
than one:

| | where it stops |
|---|---|
| shadPS4 | runs the whole suite; its output is readable |
| Kyty | runs and binds real functions; its output is not reachable from a parent process |
| craziiEmu | loads, resolves nothing, stalls at the entry |

Each one found something the others could not. shadPS4 gave the behaviour findings; Kyty's
`elf.h` gave the two swapped tag values that had been wrong for weeks; craziiEmu confirmed
that correction independently and then produced a gap of its own.

### The libc conversion begins

Twelve censused names made callable and five new checks written against them: `strncat`
bounds, `strpbrk` ordering with case-insensitive compare, integer conversion, character
classes, and wide strings. All `spec` - ISO C settles every answer.

**Validated on the host first, which is the point of having one.** All five pass against
real glibc, so they are correct rather than merely self-consistent. A check that fails
everywhere proves nothing; one that passes a known-good implementation and fails a
platform has said something.

All five then fail on the emulator.

**What that most likely means, stated plainly.** These are almost certainly stubs
returning zero rather than implementations that are subtly wrong - `wcslen` returned 0,
`atol` returned 0. The census control already established that this emulator resolves
unknown symbols to stubs, so the two readings are hard to separate from outside, and the
report does not currently distinguish them.

That is a real limitation and worth naming rather than dressing up: "returns zero because
it is not implemented" and "returns zero because it is wrong" produce identical records.
Telling them apart needs either the enumerator reaching export tables, or hardware.

    17 sections, 93 checks, well-formed
    expectations: 53 assumed, 1 documented, 39 spec, 0 hardware
    59 pass  3 partial  25 fail  6 skip

### Stub, or wrong - no longer a guess

`007-responsive` (D047) asks whether a function reads its arguments at all, by calling it
twice with inputs whose answers must differ and comparing the two answers to each other.
No expected value is involved, so the verdict cannot be argued with.

Validated on the host first: all twelve probes report `responds` against real glibc.

Against the emulator, two respond and ten are silent - every silent one returning zero.
So `strlen` and `strcmp` are implemented and the rest of that dozen are stubs.

**That reframes the whole libc picture.** The twenty-five failures in the previous run
were being read as twenty-five bugs. They are mostly absence: a handful of incorrect
implementations sitting among a large number of functions that were never written. Those
need entirely different work, and until now the report could not tell anyone which was
which.

It is the same lesson as D046 in a different costume - a number that looks like a finding
is not one until something has separated the ways it could have been produced.

### The map, and two independent mechanisms agreeing

The responsiveness table now covers 27 functions across two verdicts - string, memory and
conversion in one, the maths library in the other, kept apart because "wrote the string
functions, not the maths ones" is a different state of affairs from having done neither.

**The host caught a bad probe before it reached anything else.** `fmod(7,4)` and
`fmod(11,4)` are both 3, so the first version's inputs did not differ and it reported a
working `fmod` as a stub. A responsiveness probe invents stubs if its inputs are not
checked against a real implementation - which is the whole reason the host build exists,
and it paid for itself again.

**Against the emulator: 8 of 27 implemented, 19 stubs.**

    responds  strlen strcmp strncmp strchr memcmp | sin cos pow
    silent    everything else, all returning zero

**The two mechanisms agree exactly**, which is the strongest evidence either of them is
right. Compare the maths section with no knowledge of the other:

| function | responsive | behavioural |
|---|---|---|
| `pow` | responds | pass |
| `sin`/`cos` | responds | pass |
| `sqrt` | silent | fail |
| `fabs` | silent | fail |
| `floor`/`ceil` | silent | fail |
| `fmod` | silent | fail |

Every behavioural pass is an implemented function and every failure is a stub. Two
independent tests, written for different reasons, producing the same partition. That is
not a coincidence available to a suite that is measuring nothing.

**And it settles the earlier question.** The twenty-five libc and maths failures were read
as twenty-five bugs, then guessed at as "probably stubs". They are stubs, all but a
handful, and the report now says so rather than leaving it to be inferred.

One detail worth keeping: several silent functions return `0x3c205d65726f435b`, which is
ASCII `[Core] <`. That is the emulator's own log text arriving in a value field - the
report and the emulator's logging share a stream, and a long record can be interleaved
with one. Worth knowing before trusting any single value captured this way.

### Every check, on screen

Six pages of eighteen, cycling every three seconds from the summary and back (D048). Any
page reaches the screen unattended, so any of them can be photographed - no controller,
which is one less thing that has to work on a platform being tested because things do not
work on it.

`PAGE 1 OF 6` and `CHECKS 37 TO 54 OF 95` are drawn on every page, so a photograph says
whether it is the whole story.

**And the churn crash surfaced again**, on the run before these captures: a thread named
`obscene-churn` faulted *after* it had been joined and the run had moved on two sections.
That is a use-after-free signature in the emulator's thread teardown rather than a failure
of the check. Six subsequent runs were clean at 554 records, so the rate is roughly
one in seven - measured, not asserted (D046).

