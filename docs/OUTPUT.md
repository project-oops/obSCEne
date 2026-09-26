# The report format

One record per line. Fields are pipe-separated, order is fixed, and every line begins
`OBS|` so records survive being interleaved with an emulator's own logging.

**This is an interface.** Parsers read it, and the intended reader is an agent diffing one
run against the next. Field order and meaning do not change without bumping the version in
the `meta` record; new fields are only appended to the end of a line.

## Records

| Record | Fields |
|---|---|
| `meta` | format version, section count, check count |
| `build` | build identifier, target (`module`, `payload` or `host`) |
| `context` | measured run environment `<delivery>/<generation>` (e.g. `payload/ps4-bc`, `payload/ps5-native`), then a basis; the environment a run measured in, orthogonal to a check's `OBS_FROM_*` provenance |
| `sink` | path the report was also written to, or `none` |
| `guard` | fault guard `on`/`off`, and a short account of what init resolved - so a run that could catch a crashing check is told from one that could not |
| `peripherals` | four fields - pad, keyboard, mouse, audio - each the fixed category word when it opened at run start or `-` when it did not (not a device/product name), so a peripheral probe's `pending` reads against what was attached |
| `resolution` | whether module enumeration and dlsym work here (`works`/`unavailable`) and a short reason - so a `module\|...\|0x0` and an unresolvable symbol read as "not seen" rather than "absent" in a leg that could not enumerate (payload mode) |
| `net` | command-socket state (`listening`/`unavailable`/`unauthenticated`), port |
| `sysinfo` | field (`memory`, `vram`, `generation`, `gpu`, `ip`, `firmware`, `temp`, `storage`, `listening`), state (`known`/`unconfirmed`/`absent`), value (or `unknown`) |
| `display` | state, detail, code - the code is the platform's own answer where a call refused, `0x0` where none did |
| `section` | id, title, purpose |
| `try` | check id, library, symbol |
| `res` | check id, status, value, detail, provenance |
| `sym` | library, symbol, `present` or `absent`, availability |
| `responsive` | library, symbol, verdict, observed value |
| `import` | library, symbol, `linked`/`unlinked`, `resolvable`/`unresolvable` - one of this program's own imports on two axes. See below |
| `call` | library, symbol, index, outcome, returned value |
| `gpudev` | backend, device, type - the GPU a following run of `gpu` records came from (`type` is `integrated`/`discrete`/`cpu`/..., so a `cpu` result is gradable as non-hardware) |
| `gpu` | kernel, lane, input bits, output bits - one lane of a unary compute dispatch, observed not judged |
| `gpuop` | kernel, lane, output bits, then one field per input - a multi-operand dispatch (fma, pow, min, max, division) |
| `measure` | check id, symbol, quantity, value, unit |
| `progress` | check id, how far it got |
| `module` | module name, handle |
| `moduleword` | offset, value |
| `modtier` | library, status, privilege tier, detail - the verified permission tier of a reachable module (D296), see `docs/PLATFORM_LIBRARIES.md` |
| `sectiontally` | section id, pass, partial, fail, skip, crash, pending, optional duration (us) |
| `frontier` | capabilities established, checks blocked, deepest wholly-green section |
| `tally` | pass, partial, fail, skip, crash, pending |
| `time` | kind (`start`, `section`, `check`, `total`), target/id, duration (us) |
| `bytes` | check id, symbol, label, offset, hex - one line of a buffer dump. Three labels are counts rather than data and carry an empty hex field: `extent` (last byte written, or with `written` the last byte changed), `changed` (how many bytes differ), `untouched` (a run inside the extent the call left alone - a field boundary a hexdump cannot show) |
| `size` | library, symbol, argument index, size, `accepted`/`rejected`, returned code - one rung of a size ladder. The boundary between the two is the structure size, drawn by the platform rather than by this project |
| `err` | library, symbol, argument description, returned value |
| `region` | index, first, second, `advanced`/`stalled` - one memory region |
| `resolve` | library, symbol, `present`/`absent`, address - the name oracle |
| `resume` | checks skipped, `ok`/`full`, then zero or more ids being watched |
| `end` | output channel, optional total duration (us) |

```
OBS|meta|1|14|77
OBS|build|a1b2c3d|payload
OBS|context|payload/ps4-bc|elfldr payload; libSceGnm mapped, libSceAgc absent
OBS|section|020-memory|Direct memory|A full reserve, map, use, unmap and release cycle.
OBS|try|020-memory/allocate|libkernel|sceKernelAllocateDirectMemory
OBS|res|020-memory/allocate|pass|0x8804000000||assumed
OBS|sectiontally|020-memory|1|0|2|4|0|0
OBS|tally|16|4|7|8|0|0
OBS|end|<channel>
```

`sectiontally` and `tally` carry six counts - pass, partial, fail, skip, crash, pending - and
`sectiontally` takes an optional trailing duration in microseconds. `end` carries the
output-channel name and an optional trailing total duration in microseconds.

## Openness

A reader keys on the second field and ignores a record kind it does not know, as it must for
any kind added later.

The enumerated sets a report field draws from - a `sysinfo` `state`, a `res` `status` or
`provenance`, a `call` `outcome` - are open in the same way. A reader keys on the values it
knows and treats an unrecognised one as unrecognised, degrading rather than failing. The
program may append a value to such a set, or add a record kind, without bumping the format
version. Changing the meaning or order of an existing field bumps it.

**The protocol grammar is closed.** The verbs, the refusal reasons and the capability tokens
in `docs/PROTOCOL.md` are fixed lists that the specification and `obscene-tool protocol`
enforce against each other. Adding one is a specification change, not a silent append. The
report is an open stream of observations; the protocol is a closed grammar of exchanges.

## Provenance

The `res` provenance field takes one of these values, ordered by how much a reader should
trust the expectation behind the verdict:

| | |
|---|---|
| `assumed` | this project's own reasoning. Sensible, unconfirmed, and could be wrong in any direction |
| `implementations` | two or more independent implementations were read directly and they agree. Stronger than a guess; weaker than a document, because implementations read each other's source and two sharing an ancestor agree about their ancestor. Named in the check's own comment |
| `derived` | the kernel derives from a documented system and that system's specification settles this specific case. Wrong only if the vendor changed a behaviour while keeping the name |
| `spec` | ISO C or POSIX names the function and settles it |
| `documented` | vendor interface documentation describes this behaviour specifically |
| `hardware` | observed on the hardware. The only kind an emulator can be held to without argument |

A `fail` is only as strong as its provenance. An emulator author reading `fail [assumed]` is
entitled to disagree; `fail [spec]` is a different conversation.

## Diffing two reports

```bash
obscene-tool diff before.txt after.txt
```

| Exit | Meaning |
|---|---|
| 0 | No regressions - things improved, or nothing changed |
| 1 | At least one regression |
| 2 | The reports cannot be compared |

A **regression** is a check that got worse, not a check that is failing. A run where
everything fails and nothing changed exits 0. `obscene-tool verify` says whether a report is
sound; the tally says whether the platform is any good.

Statuses are ordered `crash < pending < skip < fail < partial < pass`.

- `skip` sits below `fail`: a check that stopped running says less than one that ran and
  failed, so lost coverage is a regression. A check that disappears from the report counts
  the same way.
- `crash` sits below all of them: a check that starts faulting, from any status, is the most
  serious regression a run can show.
- `pending` sits between `crash` and `skip`. It resolved nothing, but it can run - it is one
  input away from an answer (a controller plugged in, a button pressed) - so it is shown apart
  from a skip and does not count against coverage the way a skip does.

The `build` record lets a diff distinguish "the probe changed" from "the platform changed".
It is stamped at compile time (`make BUILD_ID=...`), because a freestanding guest has no
trustworthy clock, and asking the platform for one would make the stamp a measurement of the
thing being measured.

## `measure` records

Every other record carries a judgement. A `measure` record carries a number and asserts
nothing about it.

```
OBS|measure|120-measure/identify-clocks|sceKernelUsleep|requested|0x4e20|us
OBS|measure|120-measure/identify-clocks|sceKernelGetProcessTime|delta-across-sleep|0x2a|ticks
OBS|measure|120-measure/identify-clocks|sceKernelReadTsc|delta-across-sleep|0x1332f57|ticks
```

`quantity` names what was measured, `unit` what it is in. Several belong to one check, and
the check emits its own verdict separately.

They cover questions with no published answer, such as how long a one-millisecond sleep
takes. A threshold there would be this program inventing a specification; a recorded figure
needs no expectation. The requested value is emitted beside the observed one, and the figures
are meant to be diffed across emulators and hardware rather than read down a page. They are
the calibration for the `assumed` assertions around them, which stay loose until it exists.

Adding a `measure` record does not bump the report version.

## `sysinfo` records

Each `sysinfo` record mirrors one field of the status line the drawn report shows - memory,
VRAM, generation, the socket's address.

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

**State.** All three of `known`, `unconfirmed` and `absent` can carry the value `unknown`.
`absent` is a query this platform does not have; `unconfirmed` is one it has that this program
has no confirmed signature to call through; `known` is a value read through one. The state
separates an emulator gap from this program's own unwired query.

**`listening`** is the server's state, not the machine's. A consumer rendering machine
identity may filter it.

**Observations, never provenance.** A probe cannot certify its own machine (D108): inside an
emulator every one of these answers as the emulator chooses. They are recorded and compared
across platforms, and never enter the corpus as `measured` machine facts.

**`generation`** is a generation number (`4 (gnm)` / `5 (agc)`) when exactly one graphics
driver resolves, `both` when both do, and `unknown` when neither does. `generation|known|both`
and `generation|absent|unknown` are distinct findings. `both` is a positive observation - the
fingerprint of a stub-everything loader as much as of real back-compat - and not a claim that
the machine is both generations; presence is not implementation.

**Placement.** With the report header (after `sink`), and again from a serving build when it
starts listening, so a driver that connects before asking for a full run can read the target's
account of itself.

## `sym` records

The census section emits one `sym` record per known symbol. It claims that the symbol
resolved, not that it works, and there are an order of magnitude more of them than checks,
so they have their own record type rather than inflating the check count.

```
OBS|sym|libkernel|scePthreadMutexLock|present|<availability>
OBS|sym|libSceNet|sceNetSocket|absent|<availability>
```

The trailing `availability` field is always present; it is abbreviated here.

The census never calls anything. The names are declared as data rather than as functions so
the type system forbids it. The one exception is `910-bulk`, compiled in only under
`OBS_BULK`, which casts a censused address and calls it (see `call` records). The cast keeps
the exception inside one expression, so every other translation unit still cannot call these
names.

Read `900-surface/control` before trusting any census number. It probes one symbol that must
resolve and one that must not. On a platform implementing none of the surface, "everything
absent" and "the presence test is broken" produce identical output; the control separates
them.

## `import` records

A check that does not run reports `skip` with `the loader did not resolve this symbol for
this build` (D235). A null import has two causes needing opposite repairs:

- the platform does not have the symbol - nothing to fix; the skip is the finding
- the platform has it and this module's import did not bind - a defect in what the module
  declares, and every check behind it comes back when repaired

An `import` record carries both axes:

```text
OBS|import|libScePad|scePadOpen|unlinked|resolvable
```

| | `resolvable` | `unresolvable` |
|---|---|---|
| **`linked`** | ordinary; the call works | the run-time resolver is weaker than the loader - a fact about the resolver only |
| **`unlinked`** | the repairable case. The symbol is there under this name in this library and the import still did not bind | the platform does not offer it under that name and library, or `imports.c` names the wrong library |

`linked` is whether the loader bound the import slot. `resolvable` is whether the same name in
the same library comes back from a run-time lookup. Neither answers the other: on hardware a
library can be mapped into the process with an address range and a fingerprint while every
import from it stays null.

These are not `sym` records because they cannot share a declaration. A censused name is
declared as data (D008); this program's own imports are declared as functions in
`platform.h`, and a name cannot be in both. The set walked is the symbols named by checks.

## `call` records

Emitted by `910-bulk`, which calls every censused symbol with nothing in its arguments to
separate a resolved address from an implementation behind it.

Two per symbol, sharing an index: the first has outcome `attempt` and a zero value, before
the call; the second has the classification and the answer, after it.

```
OBS|call|libSceLibcInternal|div|0x9f|attempt|0x0
OBS|call|libkernel|sceKernelClose|0x2a|rejected|0x80020009
```

An `attempt` with no partner is the result, not a truncation. It names the function that
ended the process and carries the index the sweep resumes from - `div(0, 0)` raising SIGFPE
on the host is one.

Library and symbol appear on both records, because the pair is separated by the thing that
may not return.

| | |
|---|---|
| `attempt` | announcement only; the call has not happened yet |
| `rejected` | the answer matches the vendor error scheme. Something validated the arguments and said no, which a generic stub cannot do |
| `zero` | returned zero. A success on null arguments and a do-nothing stub are indistinguishable here |
| `error-shaped` | negative or high-bit-set, but not the vendor scheme - another facility, or an errno returned directly |
| `value` | anything else |

### `rejected`

`rejected` tests one constant: `(returned & 0xFFFF0000) == 0x80020000`. The vendor
scheme is `0x8` + a 16-bit facility + a facility-local code, and the facility tracks a
subsystem family - `0x8055` for the `libSceNp*` libraries, `0x80b8` for the dialog libraries.
`0x8002` is one facility among many, so most genuine argument rejections land in
`error-shaped`, next to a plain `-1`. Every record carries the full 64-bit return, so a
reader can decode the facility itself.

The test is not widened. Changing what `rejected` means would make older and newer reports
disagree while both say `rejected`, and the ids are the diffing key. A facility table learned
from an emulator and baked into the probe would assert on hardware something no hardware has
confirmed - the failure `900-surface/presence-is-not-behaviour` names. See D164.

The classification is meaningful only for functions returning an integer. One returning a
float leaves its answer in a vector register; the integer register is recorded as found,
because correcting it needs the signature the section exists to avoid needing.

## `try` and `res`

A `try` record is written and flushed before the platform function is called. A `res`
follows once the call returns. A `try` with no matching `res` is a call that did not return -
under an emulator, usually a hard crash. `obscene-tool pretty` reports it explicitly rather
than treating the stream as truncated.

**Fault guard.** When a call faults and the guard recovers the run, the `try` is followed by
a `res` reading `crash`. A dangling `try` means "did not return and was not caught"; a
`crash` `res` means "faulted, and the run went on".

A skipped check never emits a `try`. It was not attempted, and an announcement would make a
skip indistinguishable from a crash.

## Status values

| Status | Meaning |
|---|---|
| `pass` | The call succeeded and every postcondition held |
| `partial` | It returned, but something was off - a success code with a nonsensical value, or a documented "not supported" answered gracefully |
| `fail` | It returned an error where success was expected |
| `skip` | A prerequisite did not hold, so nothing was attempted and nothing was learned |
| `crash` | The call faulted (SIGSEGV and its kin) and the fault guard recovered the run. The value field carries the signal number. The strongest finding a probe can make; ordered below every other status for regressions |
| `pending` | The check can run but was not given the input it needs - a peripheral attached, a button pressed, a stick deflected. Never blocking and never fatal: it samples its window, finds nothing, and reports that it is still waiting. A re-run with the input provided produces the real result. Distinct from `skip`, which says the check does not apply here (D328) |

`partial` exists because an implementation returning zero for everything would otherwise
look perfect. `skip` exists because without it one broken allocator turns every later check
red and buries the one real fault.

## Fields

**Check id** - `<section-id>/<slug>`, unique across the whole program. It is the key when
diffing two runs and is never renamed casually. `obscene-tool verify` fails a report
containing duplicates, because a duplicate makes a diff silently ambiguous.

**Value** - hexadecimal with a `0x` prefix, or empty. The observed return code or result. A
code that changes between builds is the finding.

Values from functions returning a 64-bit signed type appear sign-extended
(`0xffffffff80020016`). That is the returned value, reported rather than masked.

**Detail** - free text, or empty. Never contains `|` or a newline; both are replaced with a
space at the point of writing. Substitution rather than escaping, so no parser needs an
unescape step.

## Reports and corpora

**A report** (`OBS|...`) is what the probe emits on its own - to stdout, the file sink, or
the drawn screen - with no driver and no session. `docs/examples/emulator-run.txt` is one. Its
only origin record is `build`, which names the binary kind (module, payload, host), not the
machine. A report has no machine provenance; a consumer grading a bare report gets
"0 gradeable".

**A corpus** (`OBSCORPUS|...`) is what the driver (`obscene-tool drive`) produces by ingesting
a session. It carries the machine origin on every line, denormalised, so a line read alone is
interpretable. This is the artifact to grade.

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
| `verb` | the command - `hello`, `call`, `read`, `report`, `bye` ... |
| `outcome` | `returned`, `refused`, `ok`, or a non-answer: `died`, `timeout`, `lost`, `not-sent` |
| `value` | the returned value, empty for every non-answer - a death never carries `0x0` |
| `detail` | free text, or the refusal reason / missing capability |
| `observed-by` | `probe` for what the system reported, `driver` for what was inferred from its silence |
| `origin` | the machine provenance, denormalised - see below |

A `record` line wraps an `OBS|sym`/`bytes`/`module` record the probe emitted mid-command,
verbatim, with the origin appended so it too stands alone.

The **origin field** is a comma-joined list of `key=value`, sorted by key:

```
OBSCORPUS|call|t0x13bd05e|2|report|returned|0x28||probe|firmware=13.520.001,probe=dev,target=prospero
```

Keys are open; `target`, `gpu`, `driver`, `firmware`, `probe` are the expected ones.

### Operator-stamped origin

A probe cannot certify its own machine. Inside an emulator, `sceKernelGetSystemSwVersion`
returns the emulator's chosen version; a probe that stamped that as `firmware=` would present
an emulator's answer as a hardware measurement.

The machine identity used for grading - above all whether this is real hardware - is asserted
by the operator through the driver (`--part target=prospero --part firmware=...`), never
self-reported by the probe. What the probe observes about itself (its generation, the raw
version bytes) travels as ordinary records marked `observed-by=probe`, a weaker claim than an
operator-asserted origin. A consumer grades `hardware -> measured` only when the hardware was
the target.

### Absent provenance

A `res` record without a provenance field carries no grade, and none is invented for it. An
assumed grade on an ungraded record is the same fabrication as a value on a call that died.

## Verifying a report

```bash
obscene-tool verify build/host-report.txt
```

Checks the invariants: counts agree with records, every announcement resolves, check ids are
unique, tallies match what was recorded, sections are in ascending order, and the stream
ends properly.

It says nothing about whether the checks passed. A report that is entirely red is
well-formed, and on a host build that is the expected outcome.
