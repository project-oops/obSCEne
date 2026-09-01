# Design

Why obSCEne is shaped the way it is: what it announces before it acts, why it is written from
the failure side, how the sections are ordered, and what it refuses to invent.

These were the middle of the README, which made a reader wanting the build instructions scroll
past a design essay to reach them. The reasoning is worth keeping and worth keeping *here*.

See also [OUTPUT.md](OUTPUT.md) for the report format itself, and [WORKFLOW.md](WORKFLOW.md)
for the loop this feeds.

## Announce before attempting

The single most important property. Every check prints its identity **before** making
the call:

```
OBS|try|020-memory/map|libkernel|sceKernelMapDirectMemory
OBS|res|020-memory/map|pass|0x8804000000|
```

Under an emulator the normal outcome of an unimplemented function is a hard crash
that takes the process with it. When that happens the stream simply stops - and the
last line names the exact call that did it. A report that ends on a `try` is not a
truncated report, it is a stack trace with one frame and no debugger required.

Everything else in the program is arranged around keeping that property true.

## Checked from the failure side

Much of the suite passes deliberately invalid arguments and expects an error:

```c
int rc = sceKernelClose(-1);   /* must fail */
```

This needs no struct layouts - guessing at one corrupts the stack, and the crash
lands nowhere near the mistake - and it still proves the function exists, is
reachable, validates its arguments, and returns something plausible. That is exactly
the set of things an implementation returning a constant gets wrong.

**The honest limitation:** an implementation that fails *everything* passes every
negative check. Negative checks prove argument validation; only the positive ones
prove the function does its job.

That is why `035-libc` and `037-math` matter out of proportion to their size. The C runtime is the one
library whose whole interface is ISO C, so every signature is certain and every check
can ask whether it *works*: `calloc` must return zeroed memory, `realloc` must preserve
contents, `qsort` must actually sort, `snprintf` must report the length it would have
written. A stub returning success fails all four. The maths section is the same idea
with no tolerances anywhere - every value checked against is exactly representable, so
`floor(-1.5)` must be `-2.0` and nothing else. An epsilon is where a wrong answer
hides.

Shifting the balance from checks this project reasoned out to checks a public document
settles is the main direction of travel - the current split is in the status block above,
and the reasoning is in [docs/BACKLOG.md](BACKLOG.md).

`018-relational` takes the same idea somewhere no document reaches. It compares results
**to each other** rather than to an expected value: two live event flags must not share a
handle, a counting semaphore must refuse a third claim against two signals, memory
released must be allocatable again. Those hold whatever the platform's actual numbers
are, so they need no authority to check - which is what makes them usable on the vendor
surface, where no authority exists.

## Reading a report

The binary emits one machine-readable format. Colour and grouping are presentation
and live in a script, so the guest stays small:

```bash
make pretty                          # colour, grouped by section
./build/obscene-host | obscene-tool pretty
```

The format contract is [docs/OUTPUT.md](OUTPUT.md). Treat it as an interface -
parsers exist.

## The loop this exists for

```bash
make host && ./build/obscene-host > baseline.txt
# ...change the emulator...
make diff BASELINE=baseline.txt
```

```
  improved   037-math/sqrt: skip -> pass
  value      900-surface/libc: 0x39 -> 0x4d
7 improved, 0 regressed, 70 unchanged
tally: pass +7, skip -7
```

Exit 0 means no regressions, 1 means something got worse. **A regression is a check
that got *worse*, not a check that is failing** - under an early emulator almost
everything fails, and the only useful question is whether today beats yesterday.

`skip` ranks *below* `fail`: a check that stopped running tells you less than one that
ran and failed, so losing coverage counts against you. A check vanishing from the
report entirely counts the same way - otherwise deleting an awkward check would read
as progress.

## Sections, base to high level

Order is the whole value of the report: a failure at the top is read before a failure
at the bottom, because the bottom depends on the top.

| | Section | Establishes |
|---|---|---|
| 000 | boot | The report itself can be trusted |
| 005 | generation | Which generation this is, inferred from exclusive symbols |
| 010 | kernel | Process identity and the clocks |
| 020 | memory | Reserve, map, use, unmap, release |
| 030 | thread | A thread that actually runs its body |
| 035 | libc | The C runtime, checked for behaviour rather than presence |
| 037 | math | Floating point, where sign and rounding direction break |
| 040 | file | Descriptor handling |
| 050 | time | Sleeping with the clock still monotonic |
| 060 | module | Dynamic linking |
| 070 | user | The identity subsystems are opened against |
| 080 | video | Acquiring the display output |
| 090 | audio | Bringing up audio |
| 100 | input | Acquiring a controller |
| 900 | surface | A census of the whole known surface - presence only |

A check whose prerequisites were not met is **skipped, not failed**. One broken
allocator would otherwise turn everything below it red and bury the one real fault.
The same applies to a check whose symbol the loader could not resolve - every platform
declaration is weak, so an absent function is skipped rather than jumped to.

## Presence and behaviour are different questions

The behavioural sections ask whether a function *works*, and each one costs a confident
signature. The census asks only whether it *exists*, which costs a name - so it scales
to the whole platform, and it is where the honest coverage number comes from.

**The census never calls anything, and one section deliberately does.** Every censused
name is declared as data rather than as a function, so the type system forbids the call
outright - calling a function whose signature you do not know is the mistake this project
refuses to make, and forty thousand names would otherwise be forty thousand chances to make
it. `910-bulk` steps around that on purpose, by casting an address rather than redeclaring
the name, so the exception stays inside one expression and every other translation unit
still cannot call these by accident. It is compiled in only under `OBS_BULK` and it is the
one part of the program expected to end the process.

**Presence is a statement about the loader as much as the platform.** shadPS4 resolves
every import through a generic stub and reports **35,337 of 35,337 present** for libraries
it does not implement; PS5PCEM resolves what it implements and names the other 31,601
absent. The higher number is the less honest one. Read the census beside `007-responsive`,
never alone - and see `910-bulk`, which exists because a resolved address and an
implementation are not the same claim.

A wrong name in the census is a false negative - visible, harmless, correctable. A
wrong *arity* in a behavioural check corrupts the stack and crashes somewhere
unrelated. That asymmetry is why the census can cast a much wider net.

**`900-surface/control` runs first and validates the test itself**, probing one symbol
that must resolve and one that must not. On a platform implementing none of the
surface, "everything absent" and "the presence test is broken" look identical - the
control is what tells them apart.

## Nothing here is invented

Every declaration in [include/obscene/platform.h](../include/obscene/platform.h) is a
signature this project is confident about. Where an arity or a struct layout is
uncertain, the function is **left out** rather than guessed at.

The symbol and library strings are ABI identifiers - the import hash is computed from
the symbol name, so renaming them for tidiness would stop this testing anything. They
stay exactly as the platform spells them. Prose elsewhere avoids vendor branding.
