# The report format

One record per line. Fields are pipe-separated, order is fixed, and every line begins
`OBS|` so records survive being interleaved with an emulator's own logging.

**This is an interface.** Parsers exist, and the intended reader is an agent diffing
one run against the next. Field order and meaning do not change without bumping the
version in the `meta` record; new fields may only be appended to the end of a line.

## Records

| Record | Fields |
|---|---|
| `meta` | format version, section count, check count |
| `build` | build identifier, target (`module`, `payload` or `host`) |
| `context` | measured run environment `<delivery>/<generation>` (e.g. `payload/ps4-bc`, `payload/ps5-native`), then a basis; the environment a run measured in, orthogonal to a check's `OBS_FROM_*` provenance |
| `sink` | path the report was also written to, or `none` |
| `net` | command-socket state (`listening`/`unavailable`), port |
| `sysinfo` | field (`memory`, `vram`, `generation`, `gpu`, `ip`, `firmware`, `temp`, `storage`, `listening`), state (`known`/`unconfirmed`/`absent`), value (or `unknown`) |
| `display` | state, detail, code - the code is the platform's own answer where a call refused, `0x0` where none did |
| `section` | id, title, purpose |
| `try` | check id, library, symbol |
| `res` | check id, status, value, detail, provenance |

The provenance field takes one of five values, ordered by how much a reader should trust
the expectation behind the verdict:

| | |
|---|---|
| `assumed` | this project's own reasoning. Sensible, unconfirmed, and could be wrong in any direction |
| `implementations` | two or more independent implementations were read directly and they agree. Stronger than a guess; **weaker than a document**, because implementations are not independent witnesses - these projects read each other's source, and two sharing an ancestor agree about their ancestor. Named in the check's own comment |
| `derived` | the kernel derives from a documented system and that system's specification settles this specific case. Wrong only if the vendor changed a behaviour while keeping the name |
| `spec` | ISO C or POSIX names the function and settles it |
| `documented` | vendor interface documentation describes this behaviour specifically |
| `hardware` | observed on the hardware. The only kind an emulator can be held to without argument |

**A `fail` is only as strong as its provenance.** An emulator author reading `fail
[assumed]` is entitled to disagree; `fail [spec]` is a different conversation.
| `sym` | library, symbol, `present` or `absent`, availability |
| `responsive` | library, symbol, verdict, observed value |
| `import` | library, symbol, `linked`/`unlinked`, `resolvable`/`unresolvable` - one of this program's own imports on two axes. See below |
| `call` | library, symbol, index, outcome, returned value |
| `gpudev` | backend, device, type - the GPU a following run of `gpu` records came from (`type` is `integrated`/`discrete`/`cpu`/…, so a `cpu` result is gradable as non-hardware) |
| `gpu` | kernel, lane, input bits, output bits - one lane of a unary compute dispatch, observed not judged |
| `gpuop` | kernel, lane, output bits, then one field per input - a multi-operand dispatch (fma, pow, min, max, division) |
| `measure` | check id, symbol, quantity, value, unit |
| `progress` | check id, how far it got |
| `module` | module name, handle |
| `moduleword` | offset, value |
| `sectiontally` | section id, pass, partial, fail, skip |
| `frontier` | capabilities established, checks blocked, deepest wholly-green section |
| `tally` | pass, partial, fail, skip |
| `bytes` | check id, symbol, label, offset, hex - one line of a buffer dump. Three labels are counts rather than data and carry an empty hex field: `extent` (last byte written, or with `written` the last byte **changed**), `changed` (how many bytes differ), `untouched` (a run inside the extent the call left alone - a field boundary a hexdump cannot show) |
| `size` | library, symbol, argument index, size, `accepted`/`rejected`, returned code - one rung of a size ladder. The boundary between the two **is** the structure size, drawn by the platform rather than by this project |
| `err` | library, symbol, argument description, returned value |
| `region` | index, first, second, `advanced`/`stalled` - one memory region |
| `resolve` | library, symbol, `present`/`absent`, address - the name oracle |
| `resume` | checks skipped, `ok`/`full`, then zero or more ids being watched |
| `end` | output channel |

**It drifted again, and the same way.** `bytes`, `err`, `region`, `resolve` and `resume` were
all being emitted and none was listed; `size` was added at the same time as this note. Five
kinds is not a lapse in bookkeeping, it is the documentation describing a different program
from the one that runs - and the reason it survived is that the openness rule below makes an
undocumented kind *harmless to a parser*, so nothing ever failed to reveal it. Openness is a
courtesy to readers, not permission to stop writing rows.

**This table drifted, and it is a contract, so that mattered.**
 Four kinds had gained a
field the table did not list and five were missing entirely - the `res` *example* below
was updated when provenance was added and the row above it was not. A parser written
against the documentation would have been wrong about half the stream.

Anything reading this should key on the second field and ignore a record kind it does not
know, exactly as it must for any kind added later.

**The same openness applies to the *values* within a field, not only to record kinds.**
The enumerated sets a report field draws from - a `sysinfo` `state`, a `res` `status` or
`provenance`, a `call` `outcome` - are **open**. A reader keys on the values it knows and
treats an unrecognised one as unrecognised, degrading rather than failing; and this program
may append a value to such a set without bumping the format version, exactly as it may add
a record kind. The reasoning is the same one the whole format rests on: the program exists
to surface findings, and a finding that needs a new value should not be gated behind a
version handshake. What still bumps the version is changing the *meaning or order* of an
existing field, never an addition.

**The protocol grammar is the deliberate exception, and it is closed.** The verbs, the
refusal reasons, and the capability tokens in `docs/PROTOCOL.md` are fixed lists that the
specification and `obscene-tool protocol` enforce against each other. A consumer must be able
to reason about every command it can send and every refusal it can receive, so adding one
of those is a specification change, not a silent append. The split is the point: the report
is an open stream of observations, the protocol is a closed grammar of exchanges.

```
OBS|meta|1|14|77
OBS|build|a1b2c3d
OBS|context|payload/ps4-bc|elfldr payload; libSceGnm mapped, libSceAgc absent
OBS|section|020-memory|Direct memory|A full reserve, map, use, unmap and release cycle.
OBS|try|020-memory/allocate|libkernel|sceKernelAllocateDirectMemory
OBS|res|020-memory/allocate|pass|0x8804000000||assumed
OBS|sectiontally|020-memory|1|0|2|4
OBS|tally|16|4|7|8
OBS|end
```

## Diffing two reports

The reason this format is machine-readable at all.

```bash
obscene-tool diff before.txt after.txt
```

| Exit | Meaning |
|---|---|
| 0 | No regressions - things improved, or nothing changed |
| 1 | At least one regression |
| 2 | The reports cannot be compared |

A **regression** is a check that got *worse*, not a check that is failing. A run where
everything fails and nothing changed exits 0, which is correct: nothing regressed. Ask
`obscene-tool verify` whether a report is sound, and the tally whether the platform is any good.

Statuses are ordered `skip < fail < partial < pass`. `skip` sits below `fail`
deliberately - a check that stopped running tells you *less* than one that ran and
failed, so losing coverage counts as a regression even though nothing went red. A
check that disappears from the report entirely counts the same way.

The `build` record is what lets a diff distinguish "the probe changed" from "the
platform changed" - very different answers to "did that help?". It is stamped in at
compile time (`make BUILD_ID=...`), because a freestanding guest has no clock it can
trust and asking the platform for one would make the stamp a measurement of the thing
being measured.

## `measure` records are observations, not verdicts

Every other record carries a judgement. A `measure` record carries a number and asserts
nothing about it.

```
OBS|measure|120-measure/identify-clocks|sceKernelUsleep|requested|0x4e20|us
OBS|measure|120-measure/identify-clocks|sceKernelGetProcessTime|delta-across-sleep|0x2a|ticks
OBS|measure|120-measure/identify-clocks|sceKernelReadTsc|delta-across-sleep|0x1332f57|ticks
```

`quantity` names what was measured, `unit` says what it is in. Several belong to one
check, and the check emits its own verdict separately as usual.

**Why they exist.** Some questions have no published answer. Nobody documents how long a
one-millisecond sleep takes, so a threshold would be this program inventing a
specification - and the check would then be testing the threshold rather than the
platform. Recording the figure needs no expectation and does not go stale when a guess
turns out wrong.

**How to read them.** One run says little. The same figures from several emulators and
from the hardware say which are faithful, which is why each record carries its units and why
the requested value is emitted beside the observed one. They are built to be diffed
across platforms rather than read down a page.

**What they are worth to the checks around them.** Most of this suite is `assumed`.
Correcting an assumed expectation today means re-deriving what it should have said;
correcting it from a measurement means reading a number the hardware produced and editing a
constant. The figures are the calibration, and the assertions beside them are loose on
purpose until that calibration exists.

**Adding one does not bump the report version.** A parser keys on the second field and
should ignore a record kind it does not know, exactly as it must for any kind added
later. What would bump the version is changing the fields of a record that already
exists.

## `sysinfo` records are the HUD, in the stream

The drawn report shows a status line - memory, VRAM, generation, the socket's address -
and until it existed as records that readout lived only on the screen. A driver reading
the report over the socket could see `net|listening` come up and not a byte of the rest.
Each `sysinfo` record mirrors one field of that line.

```
OBS|sysinfo|listening|known|0.0.0.0:9803
OBS|sysinfo|memory|known|441M
OBS|sysinfo|vram|known|4608M
OBS|sysinfo|ip|unconfirmed|unknown
OBS|sysinfo|firmware|unconfirmed|unknown
OBS|sysinfo|generation|known|both
OBS|sysinfo|gpu|known|gnm
OBS|sysinfo|temp|absent|unknown
OBS|sysinfo|storage|known|198304M
```

**The `state` field is the point.** All three of `known`, `unconfirmed` and `absent` can
carry the value `unknown`, and they mean different things: `absent` is a query this
platform does not have, `unconfirmed` is one it has that this program cannot yet call
through a confirmed signature, and `known` is a value read through one. Collapsing them
to a bare `unknown` would throw away the distinction between an emulator gap and our own
unfinished wiring - which is exactly the distinction that makes the readout worth diffing.

**`listening` is the server's state, not the machine's**, and a consumer rendering machine
identity may reasonably filter it. It sits in this record kind because it is a fact about the
target at this moment, but a bind address printed beside VRAM and generation under a heading
that says "what this target is" reads oddly, and correctly so - it followed from the framing
rather than from a mistake. Raised by a consumer doing exactly that. Filtering it is
supported; a separate record kind was considered and judged more churn than the problem.

**Observations, never provenance.** A probe cannot certify its own machine (D108): inside
an emulator every one of these answers as the emulator chooses. So they are recorded and
compared across platforms, and they never enter the corpus as `measured` machine facts.

`generation` follows the same rule the section does, and its values carry the finding
rather than collapsing it. A generation number (`4 (gnm)` / `5 (agc)`) only when
exactly *one* graphics driver resolves; `both` when both do - a positive observation, the
fingerprint of a stub-everything loader as much as of real back-compat, and a *different*
fact from an absence even though neither names the hardware; `unknown` only when *neither*
resolves. So `generation|known|both` and `generation|absent|unknown` are distinct records,
because "both are here" and "nothing is here" are distinct findings. What `both` is not is a
claim the machine *is* both generations - presence is not implementation, and naming a
hardware from it was the mistake this field was corrected to stop making.

**Where they appear.** With the report header (after `sink`), and again from a serving
build the moment it is listening, so a driver that connects before asking for a full run
can still read the machine's own account of itself.

## `sym` records are presence, not behaviour

The census section emits one `sym` record per known symbol. It is a different claim
from a `res` record - that the symbol *resolved*, not that it works - and there are an
order of magnitude more of them, which is why they carry their own record type rather
than inflating the check count.

```
OBS|sym|libkernel|scePthreadMutexLock|present
OBS|sym|libSceNet|sceNetSocket|absent
```

The census itself never calls anything. The names are declared as data rather than as
functions precisely so that the type system forbids it.

**One section steps around that deliberately**, and it is documented ten lines below: the
`call` record is emitted by `910-bulk`, which casts a censused address and calls it. The
cast keeps the exception inside one expression rather than redeclaring the name, so every
other translation unit still cannot call these by accident - which is the whole value of
the convention. It is compiled in only under `OBS_BULK`.

This paragraph read "nothing in the census is ever called" while the record type
contradicting it was defined on the same page, which is the kind of drift a format contract
can least afford.

**Read `900-surface/control` before trusting any census number.** It probes one symbol
that must resolve and one that must not. On a platform implementing none of the
surface, "everything absent" and "the presence test is broken" produce identical
output, and the control is what separates them.

## `import` records say why a symbol was null, which `sym` cannot

A check that does not run reports `skip` with `the loader did not resolve this symbol for
this build`. That is all a check can honestly say (D235), and it is not a diagnosis. A null
import has two causes needing opposite repairs:

- **the platform does not have the symbol** - nothing to fix, the skip is the finding
- **the platform has it and this module's import did not bind** - a defect in what the
  module declares, fixable, and every check behind it comes back

An `import` record carries both axes so the two can be told apart:

```text
OBS|import|libScePad|scePadOpen|unlinked|resolvable
```

| | `resolvable` | `unresolvable` |
|---|---|---|
| **`linked`** | ordinary; the call works | the run-time resolver is weaker than the loader - a fact about the resolver only |
| **`unlinked`** | **the repairable case.** The symbol is there under this name in this library and the import still did not bind | the platform does not offer it under that name and library, or `imports.c` names the wrong library |

`linked` is whether the loader bound the import slot. `resolvable` is whether the same name
in the same library comes back from a run-time lookup. **Neither answers the other** -
measured on hardware, a library was mapped into the process with an address range and a
fingerprint while every import from it stayed null.

### Why these are not `sym` records

They are the same question and they cannot share a record, because they cannot share a
declaration. A censused name is declared as data so the type system forbids calling it
(D008); this program's own imports are declared as functions in `platform.h`. A name cannot
be in both places - so the symbols whose status matters most were exactly the ones the
census could not see. `import` records cover that gap and only that gap: the set walked is
the symbols named by checks, because an unbound import nothing calls costs nothing.

## `call` records come in pairs, and a lone one is the finding

Emitted by `910-bulk`, which calls every censused symbol with nothing in its arguments to
separate a resolved address from an implementation behind it.

Two per symbol, sharing an index: the first has outcome `attempt` and a zero value, before
the call; the second has the classification and the answer, after it.

```
OBS|call|libSceLibcInternal|div|0x9f|attempt|0x0
OBS|call|libkernel|sceKernelClose|0x2a|rejected|0x80020009
```

**An `attempt` with no partner is the result, not a truncation.** It names the function
that ended the process and carries the index the sweep resumes from - the first host run
produced exactly one, `div` at index 159, because `div(0, 0)` raises SIGFPE.

Library and symbol are repeated on both records rather than carried only on the attempt,
because the pair is separated by the thing that may not return.

The outcome values, and the rule behind each - stated so a reader can disagree with the
label while keeping the raw value beside it:

| | |
|---|---|
| `attempt` | announcement only; the call has not happened yet |
| `rejected` | the answer matches the vendor error scheme. **Something validated the arguments and said no,** which a generic stub cannot do |
| `zero` | returned zero. A success on null arguments and a do-nothing stub are indistinguishable here, and this does not pretend otherwise |
| `error-shaped` | negative or high-bit-set, but not the vendor scheme - another facility, or an errno returned directly |
| `value` | anything else |

### `rejected` is narrower than it sounds, and deliberately

`rejected` tests one constant: `(returned & 0xFFFF0000) == 0x80020000`. That is the facility
seen first, in one emulator (D088), and a sweep of 31,754 answered calls has since shown it is
**one facility of at least 33**. The scheme is `0x8` + a 16-bit facility + a facility-local
code, and the facility tracks a subsystem *family* - `0x8055` is returned by eleven `libSceNp*`
libraries, `0x80b8` by seven dialog libraries.

So most genuine argument rejections land in `error-shaped`, next to a plain `-1`. **That is a
weaker summary, not a lost measurement:** every record carries the full 64-bit return, so a
reader can decode the facility and the bucket costs nothing to ignore.

It is not widened, for two reasons. Changing what `rejected` means would make historical
reports and new ones disagree while both say `rejected`, and the ids are the diffing key. And
a facility table learned from an emulator, baked into the probe, would have the instrument
assert on hardware something no hardware has confirmed - which is the failure
`900-surface/presence-is-not-behaviour` exists to name. See D164.

Only meaningful for functions returning an integer. One returning a float leaves its answer
in a vector register and this reads the integer one; that is recorded as whatever was there
rather than corrected, because correcting it needs the signature the section exists to
avoid needing.

## `try` comes before the call

A `try` record is written and flushed **before** the platform function is called. A
`res` follows it once the call returns.

A `try` with no matching `res` is therefore the signature of a call that did not
return - under an emulator, usually a hard crash. This is the intended way to locate
one, and `obscene-tool pretty` reports it explicitly rather than treating the stream as
merely truncated.

A **skipped** check never emits a `try`. It was not attempted, and forging an
announcement would make a skip indistinguishable from a crash.

## Status values

| Status | Meaning |
|---|---|
| `pass` | The call succeeded and every postcondition held |
| `partial` | It returned, but something was off - a success code with a nonsensical value, or a documented "not supported" answered gracefully |
| `fail` | It returned an error where success was expected |
| `skip` | A prerequisite did not hold, so nothing was attempted and nothing was learned |

`partial` exists because an implementation returning zero for everything would
otherwise look perfect. `skip` exists because without it one broken allocator turns
every later check red and buries the one real fault.

## Fields

**Check id** - `<section-id>/<slug>`, unique across the whole program. This is the
key when diffing two runs, so it must never be renamed casually. `obscene-tool verify`
fails a report containing duplicates, because a duplicate makes a diff silently
ambiguous rather than loudly wrong.

**Value** - hexadecimal with a `0x` prefix, or empty. The observed return code or
result. This is what makes a run diffable: a code that changes between builds is the
finding, and prose describing it is not.

Values from functions returning a 64-bit signed type appear sign-extended
(`0xffffffff80020016`). That is the actual returned value, and it is reported rather
than masked.

**Detail** - free text, or empty. Never contains `|` or a newline; both are replaced
with a space at the point of writing. Substitution rather than escaping, so no parser
needs a matching unescape step.

## A report and a corpus are different artifacts

Two things wear the `OBS`-ish prefix, and conflating them is the mistake a consumer makes
first. They answer different questions and only one carries a machine origin.

**A report** (`OBS|...`) is what the probe emits on its own - to stdout, the file sink, or
the drawn screen - with no driver and no session. `docs/examples/emulator-run.txt` is one.
Its only origin record is `build`, which names the **binary kind** (module, payload, host),
**not the machine**. A report has no machine provenance and cannot be graded by machine, and
that is correct rather than missing: there is no session to carry it. A consumer that grades
a bare report should get "0 gradeable", because nothing in it says where it ran.

**A corpus** (`OBSCORPUS|...`) is what the driver (`obscene-tool drive`) produces by ingesting
a session. It carries the machine origin on every line, denormalised, so a line read alone is
still interpretable. This is the artifact to grade.

## Corpus records

Emitted by the driver, one artifact per session. Two record kinds:

```
OBSCORPUS|call|<session>|<seq>|<verb>|<outcome>|<value>|<detail>|<observed-by>|<origin>
OBSCORPUS|record|<session>|<seq>|<the OBS record verbatim>|<origin>
```

| field | meaning |
|---|---|
| `session` | the probe's session identifier; a change in it means the probe restarted |
| `seq` | the command's sequence number, in issue order |
| `verb` | the command - `hello`, `call`, `read`, `report`, `bye` … |
| `outcome` | `returned`, `refused`, `ok`, or a non-answer: `died`, `timeout`, `lost`, `not-sent` |
| `value` | the returned value, **empty for every non-answer** - a death never carries `0x0` |
| `detail` | free text, or the refusal reason / missing capability |
| `observed-by` | `probe` for what the system reported, `driver` for what was inferred from its silence |
| `origin` | the machine provenance, denormalised - see below |

A `record` line wraps an `OBS|sym`/`bytes`/`module` record the probe emitted mid-command,
verbatim, with the origin appended so it too stands alone.

**The origin field** is a comma-joined list of `key=value`, sorted by key:

```
OBSCORPUS|call|t0x13bd05e|2|report|returned|0x28||probe|firmware=13.520.001,probe=dev,target=prospero
```

Keys are open; `target`, `gpu`, `driver`, `firmware`, `probe` are the expected ones.

### The origin is stamped by the operator, not claimed by the probe

This is the rule the whole thing turns on, and it is not optional. **A probe cannot certify
its own machine.** Running inside an emulator, `sceKernelGetSystemSwVersion` returns the
*emulator's* chosen version; a probe that stamped that as `firmware=` would be dressing an
emulator's answer as the hardware measurement - the exact `measured`-versus-`assumed` confusion
that grading exists to prevent.

So the machine identity that matters for grading - above all "is this real hardware" - is
**asserted by the operator through the driver** (`--part target=prospero --part
firmware=…`), never self-reported by the probe. What the probe *observes* about itself (its
generation, the raw version bytes) travels as ordinary records marked `observed-by=probe`,
which is a different and weaker claim than an operator-asserted origin. A consumer grading
`hardware -> measured` only when the hardware was the target is right to, and this is where
that fact comes from.

### Absent provenance is absent, never defaulted

A `res` record from before the provenance field existed carries no grade, and none should be
invented for it - an assumed grade on an ungraded record is the same fabrication as a value
on a call that died. The probe never backfills provenance onto old records for the same
reason.

## Verifying a report

```bash
obscene-tool verify build/host-report.txt
```

Checks the invariants: counts agree with records, every announcement resolves, check
ids are unique, tallies match what was recorded, sections are in ascending order, and
the stream ends properly.

It says nothing about whether the checks *passed*. A report that is entirely red is
perfectly well-formed, and on a host build that is the expected outcome.
