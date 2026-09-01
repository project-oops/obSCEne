# Measuring instead of only judging


The suggestion was: rather than asserting how long a sleep should take, record how long it
actually took, and compare that across emulators and eventually a console. That is a better
idea than what was here, and it corrects a limitation in the earlier reasoning (D059).

The objection at the time - timing a sleep means picking a threshold, and picking one is
inventing a specification - was aimed at the wrong thing. It argues against *asserting* on
a measurement, not against *recording* one.

`120-measure` now emits `OBS|measure` records: a quantity, a value, a unit, and no verdict.
Assertions stay beside them, deliberately loose and marked `assumed`, because a stated guess
is a better position than no expectation at all - and because the number beside it turns
correcting that guess into reading a figure and editing a constant.

### The clock experiment works

Three time sources are readable without a struct, and the platform does not say what any of
them counts. Rather than assume: sleep a known interval, read them all, and see which
advanced by roughly the sleep (D060).

**Validated where the answer is known.** The host stubs were changed from constants to real
clocks - `CLOCK_PROCESS_CPUTIME_ID` and `CLOCK_MONOTONIC` - so the experiment runs somewhere
the correct classification exists. Across 20ms it reported 42µs of process time and 20.13ms
of monotonic, which is right. A section whose logic has never run is not something to hand
an emulator, and constants in the stubs would have meant exactly that.

### Two findings on the first emulator run

**`sceKernelGetProcessTime` is a wall clock under shadPS4.** All three sources advanced by
the full 20ms; on the host, process time moved 42µs. The name and the toolchain
documentation both say CPU time. Recorded as a divergence from the host reference rather
than as a proven bug - what the console does is precisely what has not been measured, and
this is the first check written to answer that rather than assume it.

`sceKernelReadTsc` and `sceKernelGetProcessTimeCounter` also differ by 36 ticks, so they are
the same underlying clock there.

**`sceKernelUsleep` returns early.**

| requested | host | shadPS4 |
|---|---|---|
| 1,000µs | 1,087 (over) | 1,372 (over) |
| 5,000µs | 5,120 (over) | 4,783 - **217 short** |
| 20,000µs | 20,120 (over) | 19,288 - **712 short** |

Same direction across two runs (the earlier one was 243µs short at 20ms), against a host
that overshoots every time. A sleep is a lower bound; code that sleeps before reading a
register expects the interval to have passed.

**The first version of the check could not see it.** Its floor was half the request, so
19,288 against 20,000 passed comfortably. The shortfall is now its own recorded quantity and
a `partial` verdict. That is the value of measuring showing up immediately: the number was
in the report before the check knew to care about it.

### The documented hazard bit again, somewhere it had not been fixed

`sweep.ps1` died on a `multipass exec` stderr warning - the exact failure recorded in D050
for `multipass transfer`, guarded in `run-emulator.ps1` and not here. It is intermittent,
which is what made it read as a new fault rather than a known one: this script had run
cleanly a dozen times.

Now behind an `Invoke-Vm` helper that relaxes the preference and checks the exit code, which
is the thing that actually reports failure. Writing a hazard down is not the same as fixing
every place it applies, and that is worth remembering the next time one gets documented.

### State

666 records, complete. 65 pass, 6 partial, 38 fail, 7 skip.

Provenance across 105 checks: 57 `spec`, 47 `assumed`, 1 `documented`, 0 `hardware`.

Thirty-six checks target the FreeBSD-derived `sceKernel*` and `scePthread*` surface, and a
good share of the assumed ones have a FreeBSD man page that settles them. That is what
`OBS_FROM_DOCUMENTED` is for and it is currently used once. Reading is the only cost, and it
narrows what a console actually has to answer.

---

