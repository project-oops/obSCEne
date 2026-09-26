# Design

What obSCEne announces before it acts, how it checks from the failure side, how the sections
are ordered, and what it refuses to invent.

[OUTPUT.md](OUTPUT.md) is the report format; [WORKFLOW.md](WORKFLOW.md) is the loop this
feeds.

## Announce before attempting

Every check prints its identity **before** making the call:

```
OBS|try|020-memory/map|libkernel|sceKernelMapDirectMemory
OBS|res|020-memory/map|pass|0x8804000000|
```

Under an emulator, an unimplemented function usually crashes the process. The stream then
stops, and the last line names the exact call that did it. A report that ends on a `try` is a
stack trace with one frame. Everything else in the program is arranged to keep this property.

## Checks from the failure side

Much of the suite passes deliberately invalid arguments and expects an error:

```c
int rc = sceKernelClose(-1);   /* must fail */
```

This needs no struct layouts, and a guessed layout corrupts the stack far from the mistake. It
still shows the function exists, is reachable, validates its arguments and returns something
plausible - the things an implementation returning a constant gets wrong.

An implementation that fails everything passes every negative check. Negative checks prove
argument validation; positive checks prove the function does its job.

`035-libc` and `037-math` are positive checks over an interface ISO C defines completely, so
every signature is certain and every check asks whether the function works: `calloc` returns
zeroed memory, `realloc` preserves contents, `qsort` sorts, `snprintf` reports the length it
would have written. The maths section uses no tolerances: every expected value is exactly
representable, so `floor(-1.5)` must be `-2.0`.

`018-relational` compares results **to each other** rather than to an expected value: two live
event flags do not share a handle, a counting semaphore refuses a third claim against two
signals, released memory is allocatable again. These hold whatever the platform's actual
numbers are, so they need no authority, which makes them usable on the vendor surface.

## Reading a report

The binary emits one machine-readable format. Colour and grouping are presentation and live in
the tool, so the guest stays small:

```bash
make pretty                          # colour, grouped by section
./build/obscene-host | obscene-tool pretty
```

[OUTPUT.md](OUTPUT.md) is the format contract, and parsers depend on it.

## Comparing runs

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

Exit 0 means no regressions; 1 means something got worse. A **regression** is a check that got
worse, not a check that is failing. `skip` ranks below `fail`, because a check that stopped
running says less than one that ran and failed. A check that vanishes from the report counts
the same way.

## Section order

Sections run from the base of the platform to high level, so a failure at the top is read
before the failures below that depend on it. `src/probe/registry.c` is the authoritative,
ordered list. The landmarks:

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

A check whose prerequisites were not met is **skipped, not failed**, so one broken allocator
does not turn everything below it red. A check whose symbol the loader could not resolve is
also skipped: every platform declaration is weak, so an absent function is skipped rather than
jumped to.

## Display and user

obSCEne opens its display against a user obtained from the user service. When no initial user
can be determined it does not open a display and does not guess a user id; the text report is
still complete.

## Presence and behaviour

The behavioural sections ask whether a function works, and each costs a confident signature.
The census asks only whether it exists, which costs a name, so it scales to the whole platform.

**The census never calls anything.** Every censused name is declared as data rather than as a
function, so the type system forbids the call. `910-bulk` is the one exception: it casts an
address rather than redeclaring the name, so the exception stays inside one expression and no
other translation unit can call these names. It is compiled in only under `OBS_BULK` and is
the one part of the program expected to end the process.

**Presence measures the loader as much as the platform.** A loader that resolves every import
through a generic stub reports the whole census present, including libraries it does not
implement; one that resolves only what it implements reports the rest absent. Read the census
beside `007-responsive`, never alone. `910-bulk` exists because a resolved address and an
implementation are different claims.

A wrong name in the census is a visible, harmless false negative. A wrong arity in a
behavioural check corrupts the stack and crashes somewhere unrelated. That asymmetry is why the
census casts a wider net.

**`900-surface/control` runs first and validates the census**, probing one symbol that must
resolve and one that must not. On a platform implementing none of the surface, "everything
absent" and "the presence test is broken" look identical without it.

## Nothing is invented

Every declaration in [include/obscene/platform.h](../include/obscene/platform.h) is a signature
this project is confident about. Where an arity or a struct layout is uncertain, the function is
left out.

The symbol and library strings are ABI identifiers. The import hash is computed from the symbol
name, so they stay exactly as the platform spells them. Prose elsewhere avoids vendor branding.
