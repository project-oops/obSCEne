# The obSCEne command protocol, version 1

A line protocol for asking a running system what it does, over a socket.

This document is the contract. The C in `src/` and the driver in `tool/` implement it, and
where they disagree with this file, they are wrong.

## Purpose

A session issues commands, the system answers, and what comes back is what the system did.
The session is the interface. The product is the corpus - the committed records in
`records/`.

## Scope

A shell operates the machine from outside a guest process; this protocol interrogates the ABI
from inside one. Answering "what does `sceKernelAllocateDirectMemory` return for these
arguments, in a process loaded and relocated the way a title is" means being a loaded guest,
calling the function and reporting what came back. Anything a shell does - browsing the
filesystem, listing processes, launching payloads - is out of scope. (D183)

It is not a debugger. It has no breakpoints, no stepping, and no symbol table beyond what the
platform itself resolves.

The socket layer differs per target; everything above it is shared and specified here.

## The generic rule

**This protocol describes the platform. It knows nothing about any program that consumes
it.** An emulator may answer these commands, and so may a probe on real hardware. None is
named here and none changes the grammar.

- Every command corresponds to something the platform can be asked. A command that makes no
  sense addressed to real hardware is coupling.
- No shared code in either direction. This document and the captured exchanges in
  `docs/examples/protocol/` are the entire contract.
- Unknown commands are refused, never guessed. A responder that interprets a command it does
  not know produces a record that looks like evidence and is not.
- Transport is separable from protocol. Everything below is defined over a byte stream.

## Transport

**The probe listens. The driver connects.** The hardware has no DNS, no configuration file,
and no way to be told where a host is; it has an address the operator reads off a screen.

- TCP, one connection at a time. A second connection while one is open is refused with
  `busy` rather than queued, because two drivers interleaving commands would make the
  ordering record meaningless.
- Port 9803 by default, overridable.
- The stream is UTF-8 text, newline-terminated: `\n`, never `\r\n`.
- Lines are at most 4096 bytes including the terminator. Anything longer is a protocol error;
  commands that carry payloads which could exceed it are chunked.

### Text encoding

Replies reuse the report format (`docs/OUTPUT.md`): line-oriented ASCII with `|` separators.
A session transcript and a report are the same kind of artefact, and `obscene-tool verify`
reads both. A session can be driven by hand with `nc`. Binary payloads are hex, two characters
per byte; `blob` is the only command that carries bulk data.

## Grammar

### Requests

```
CMD|<seq>|<verb>|<argument>...
```

- `CMD` - fixed prefix, distinguishing a request from a record on a shared transcript.
- `<seq>` - decimal, strictly increasing within a session, starting at 1. It matches a reply
  to its request and records the order commands were issued in. It is required.
- `<verb>` - lower-case ASCII.
- Arguments are `|`-separated. A literal `|` or newline cannot appear in an argument. Numbers
  are hexadecimal with an `0x` prefix unless stated otherwise.

### Replies

Replies are `OBS|` records, the shape `docs/OUTPUT.md` defines. Three are specific to this
protocol:

| Record | Fields |
|---|---|
| `ack` | seq, verb - emitted before the command is carried out |
| `done` | seq, outcome, value, detail |
| `refused` | seq, reason |

Every command produces exactly one `ack` and then exactly one `done` or `refused`. A command
may also produce other records - `sym`, `bytes`, `module` - between the two; they are defined
in `docs/OUTPUT.md` and mean the same thing here.

**Bad sequence rule.** A request whose sequence number is malformed or does not increase is
refused without an acknowledgement. An `ack` is keyed by sequence, so acknowledging a
repeated one would produce two acknowledgements with the same key. The refusal carries the
offending sequence as sent, and the reason is `bad-argument`.

## Acknowledge first

`ack` is written to the socket and flushed before the command runs, the same principle the
probe applies to its own report (`CLAUDE.md`, principle 1).

Arbitrary calls with arbitrary arguments fault as a matter of course, and a probe cannot
report its own death. An `ack` with no `done` after it, followed by the connection closing,
means that command did not return. The driver records it as such. It does not invent a value
and does not record a null.

> **A command that did not answer is never recorded as having answered.**
>
> `died` is not `returned 0`. `timeout` is not `died`.

### Non-answers

| outcome | established by | means |
|---|---|---|
| `died` | driver saw `ack`, then the connection closed with no `done` | the command ended the process |
| `timeout` | driver saw `ack`, waited past its budget, connection still open | the command has not returned yet; the probe may be alive, blocked, or looping |
| `lost` | driver saw `ack`, connection closed, and the probe never came back | ambiguous - recorded as ambiguous |

`timeout` is not resolved into `died`. A blocked call and a dead process look the same from
one end of a socket, and the record says which was observed.

`died` and `timeout` records are written by the driver, and the corpus marks them
`observed-by=driver`. Everything else is `observed-by=probe`.

## Restart and reconnect

A faulting command ends the probe. Whatever restarts it is outside this protocol. The
restart is visible:

- Every session opens with `hello`, carrying a fresh `session` identifier.
- The driver holds the identifier. A new one where it expected the old one means the probe
  restarted, and everything since the last `hello` is a different process.
- The probe does not resume. There is no state worth resuming.

## Session lifecycle

### `hello` - capability negotiation

The first exchange, before any other command is accepted.

```
CMD|1|hello|1
OBS|ack|1|hello
OBS|hello|1|<session>|<capabilities>
OBS|part|<session>|<key>|<value>
...
OBS|done|1|ok||
```

The driver sends the highest protocol version it speaks. The probe replies with the version
it will use, at most that, and refuses the session if it has no version in common. A fourth
field carries the session secret (see below).

Capabilities are a `,`-separated list of tokens. The driver does not send a command whose
capability was not announced; one sent anyway is refused.

| token | the probe can |
|---|---|
| `call` | invoke a function by address |
| `resolve` | look a symbol up by name at run time |
| `read` | read guest memory |
| `write` | write guest memory - off unless deliberately enabled |
| `blob` | receive a code blob and execute it |
| `reset` | return to a known state without restarting |
| `report` | run the compiled-in check suite and stream its records |
| `gpu` | dispatch a compiled-in compute kernel and read back its result bits |

One grammar covers targets with different abilities. A stand-in target with no system
libraries announces no `resolve`, and a driver discovers that rather than assuming it.

### `part` - what produced the answers

Emitted during `hello`, one record per key. A number measured on one part and read as
authoritative for another is a wrong answer with no visible sign, so these are required.

Keys are open; these are expected where the target can answer them:

| key | example |
|---|---|
| `target` | `deck`, `hardware`, `host` |
| `gpu` | `AMD Custom GPU 0405 (gfx1033)` |
| `driver` | `radv 25.1.0` |
| `mesa` | `25.1.0` |
| `firmware` | `13.520.001` |
| `os` | `SteamOS 3.6.20` |
| `probe` | obSCEne build identifier |

Every corpus record carries these, denormalised. The wire binds them to a session identifier;
the corpus writer expands them, because a record that must be joined against something else
will eventually be read without it.

### `bye`

Closes the session cleanly. The probe replies `done` and closes. A session that ends without
`bye` is recorded as having ended without `bye`.

## Commands

### `resolve` - a name to an address

```
CMD|<seq>|resolve|<library>|<symbol>
```

Replies with a `sym` record and a `done` carrying the address, or `done` with outcome
`absent`.

Identifiers are one-way hashes, so recovering a name means guessing candidates, and finds
only names something already imports. A platform that resolves by name answers one yes-or-no
question and reaches functions nothing imports. Whether the platform can do this is a
capability.

### `call` - invoke something

```
CMD|<seq>|call|<address>|<arg0>|<arg1>|...
```

Up to six integer arguments, in the order the calling convention passes them. The address
comes from `resolve` or from anywhere else the driver got it.

```
OBS|done|<seq>|returned|<value>|
```

`<value>` is the integer return register and nothing else. A function returning a float
leaves its answer in a vector register and this reads the integer one. Not a struct, not an
error message.

The command does not report what the call did. An allocation, a mutated buffer, a started
thread or changed global state appears nowhere in the record. Establishing an effect means a
subsequent `read` or `call`, issued and recorded as its own command.

### `read` - guest memory

```
CMD|<seq>|read|<address>|<length>
```

Replies with `bytes` records - the same record a report uses - followed by `done`. Length is
bounded per command; longer regions are read across several commands and the driver
reassembles them.

Reading an unmapped address faults, which is the `died` path. A probe may validate an address
first if the platform lets it, and then answers `refused|unmapped` instead. Both behaviours
are legitimate and the record distinguishes them: "this address is not readable" and "asking
about this address killed the process" are different facts.

### `write` - guest memory

```
CMD|<seq>|write|<address>|<hex>
```

Behind the `write` capability, off unless deliberately enabled. A read or a call costs a
crash at worst; an arbitrary write costs a crash and whatever state was being built.
Read-only is the default posture.

### `blob` and `run` - the escape hatch

```
CMD|<seq>|blob|<id>|<offset>|<hex>
CMD|<seq>|run|<id>|<arg0>|...
```

`blob` uploads position-independent machine code in bounded chunks; `run` calls it. Blobs
are identified so several can be resident, and chunking keeps a line inside the length bound.

The escape hatch keeps the vocabulary small: specificity goes in the payload, not in a new
command per question.

### `reset` - a known state

```
CMD|<seq>|reset
```

State leaks between commands. An allocation made by one changes what the next observes, and a
memory map read after ten calls is not the map a title sees at startup. A target supports one
of three answers:

1. **A fresh process per command**, where something can restart the probe cheaply.
2. **`reset`**, returning what can be returned - freeing what was allocated, closing what was
   opened. Announced as a capability, and it never claims more than it does.
3. **Neither**, in which case ordering is part of the input. Every corpus record carries its
   `seq` and the count of commands since the last `reset`, so a result that depended on what
   came before is interpretable.

A probe that cannot reset answers `refused|unsupported`, never a `reset` that returns `ok`
and does nothing.

### `report` - the compiled-in suite

```
CMD|<seq>|report
```

Runs the checks built into the probe and streams their records, then `done`. This is the
report (`docs/OUTPUT.md`) delivered over the socket rather than over whatever output channel
the target has.

### `gpu` - dispatch a compiled-in kernel

```
CMD|<seq>|gpu|<kernel>|<operand0>|<operand1>|...
```

Runs one of the probe's built-in compute kernels over the supplied operands and streams the
results. The reply is a `gpudev` record (the device and its type), then one `gpu` or `gpuop`
record per lane, then `done` whose value is the lane count.

It runs only named, compiled-in shaders, never arbitrary code. The driver chooses which kernel
and what inputs. Kernel names are the ones a `report` emits.

Operands are 32-bit words (float bit patterns) and follow the kernel's arity: a unary kernel
takes N operands and answers N lanes; an arity-*k* kernel takes a multiple of *k*, each group
a tuple. A wrong operand count or an unknown kernel is `refused|bad-argument`. A dispatch that
fails without crashing answers `done|returned|0` - zero lanes, no records - distinct from a
death. The capability is announced only by a build with a working GPU backend.

### Unknown verbs

```
OBS|refused|<seq>|unknown-verb
```

A responder never guesses, approximates, or answers a command it does not implement.

Refusal reasons: `unknown-verb`, `unsupported`, `bad-argument`, `busy`, `not-negotiated`,
`unmapped`, `unauthorised`.

## Ordering and concurrency

One command in flight at a time. The driver sends a command and waits for `done` or
`refused` before sending the next. With two commands outstanding and a process that has
vanished, nothing says which one ended it.

## Versioning

The version is negotiated in `hello`; this document describes version 1.

Within a version, new verbs, new capability tokens, and new fields appended to the end of a
record may be added. Nothing else changes: field order and meaning are fixed, and a verb is
never repurposed. A consumer ignores records and fields it does not recognise.

Anything that would break a consumer built against this file is a new version.

## Security posture

A socket that executes arbitrary code, gated by a session secret. The default build announces
`call`, `read` and `report`, and `call` invokes an arbitrary address with six integer
arguments.

- A session secret, generated fresh at every startup, stops another device on the network
  driving the probe.
<!-- obscene:claim file=src/probe/net_posix.c contains=INADDR_ANY -->
- The probe binds every interface, because on hardware the driver is on another machine.
  That is why the secret is required there.
<!-- obscene:claim file=src/probe/net.c contains=OBS_NET_ESCAPE -->
- `write` and `blob` are off unless a build enables them.
- It is a laboratory instrument. It does not belong on a network anyone else is on, or
  running when nobody is using it.

### The session secret

<!-- obscene:claim file=src/probe/net.c contains=obs_net_secret_generate -->
<!-- obscene:claim file=src/probe/net.c absent=OBS_NET_SECRET_BUILTIN -->
**Generated per startup, never compiled in.** A secret built into the module would be shared
by everybody who has the module. This one is made when the probe starts listening, lasts for
that run, and is replaced by a restart.

It is displayed, because the hardware has no other channel. The HUD draws it beside the port
as `KEY`. The host build prints it on stderr, and it is emitted as a `sysinfo` record either
way.

`hello` carries it as a fourth field, after the version:

```
CMD|1|hello|1|d98130d191ccee2143658132dd1d2499
OBS|ack|1|hello
OBS|hello|1|c0x1|call,read,report
```

A wrong or missing secret is `refused|unauthorised`, before any capability is disclosed. The
session is then never negotiated, so every other verb is refused `not-negotiated`.

```
CMD|1|hello|1|deadbeefdeadbeefdeadbeefdeadbeef
OBS|ack|1|hello
OBS|refused|1|unauthorised
CMD|2|report
OBS|ack|2|report
OBS|refused|2|not-negotiated
```

A probe that cannot generate a secret serves unauthenticated and says so, on stderr and as an
absent `secret` field.

**Limits.** The socket is cleartext, so anyone who can observe the link reads the secret out
of the `hello`. On hardware the entropy is timing jitter - the low bits of differences between
successive timestamp reads, mixed - because no CSPRNG is available: tens of bits, not 128. The
host build reads `/dev/urandom`. Both are proportionate to another device on the network
connecting; neither is proportionate to a wiretap.

**Constant-time comparison.** Every byte is compared and the differences accumulated, so the
reply time does not reveal how many leading characters matched.

## Captured exchanges

`docs/examples/protocol/` holds real transcripts, one per scenario: negotiation, a call that
returns, a call that dies, a timeout, a refusal, memory read, blob and run, reset. They are
part of the contract: a consumer can be built and tested against them with no hardware
attached. `obscene-tool protocol` parses every one against the grammar above and fails if a
line does not conform.
