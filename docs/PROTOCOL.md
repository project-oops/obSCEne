# The obSCEne command protocol, version 1

A line protocol for asking a running system what it does, over a socket.

**This document is the contract.** The C in `src/` and the driver in `tool/` are
implementations of it, and where they disagree with this file, they are wrong. Written
before either, deliberately: a specification derived from an implementation is not a
specification, it is a description, and every correction to the code becomes a silent
break for anyone who built against it.

## What this is for

obSCEne answers questions by running on a real system. Today it answers a fixed list
compiled into an ELF, so a new question costs a rebuild and a redeploy. Establishing one
fact about one function has cost an hour that way.

This protocol turns that into a round trip. A session issues commands, the system answers,
and what comes back is what the system actually did.

The session is the *interface*. **The product is the corpus** - the committed records in
`records/`. A session that answers ten questions and leaves nothing on disk has produced
nothing.

## What this is not

It is not a shell, and it is deliberately not built on one - but the reason first written
here was wrong, and it is worth replacing rather than quietly deleting.

The original argument was that the real target has no shell to attach to. **It does.**
`shsrv` puts a telnet-like shell on port 2323 of a jailbroken hardware, and it was sitting in
the same payload repository as the loader this project already uses. A justification that
rests on a missing capability stops being a justification the moment somebody ships it.

The real distinction survives, and it is sharper. **A shell operates the machine from
outside a guest process; this interrogates the ABI from inside one.** `shsrv` can list a
directory, start a process, read a file. It cannot answer "what does
`sceKernelAllocateDirectMemory` return for these arguments, in a process that has been
loaded and relocated the way a title is" - because answering that means being a loaded
guest, calling the function, and reporting what came back. A shell can only answer it by
compiling and running a program, and that program is this one.

So the two do not compete, and the boundary tells this project what **not** to build.
Anything a shell already does - browsing the filesystem, listing processes, launching
payloads - is out of scope here and stays out. What is in scope is the part no shell can
reach: the platform's own behaviour, observed from inside a guest, in the platform's
vocabulary. (D183)

The socket layer differs per target; everything above it is shared and is specified here.

It is also not a debugger. It has no breakpoints, no stepping, and no symbol table beyond
what the platform itself will resolve. It asks the platform questions in the platform's own
vocabulary.

## The generic rule

**This protocol describes the platform. It knows nothing about any program that consumes
it.**

An emulator may answer these commands; so may a probe on real hardware; so may a third
implementation nobody has written. None of them is named here and none of them gets to
change the grammar.

Concretely, and these are testable:

- No command exists because a consumer needs it. Every command corresponds to something the
  platform can be asked. **A command that makes no sense addressed to real hardware is
  coupling**, and it will be visible in this file as exactly that.
- No shared code in either direction, ever. This document and the captured exchanges in
  `docs/examples/protocol/` are the entire contract.
- Unknown commands are **refused**, never guessed. A responder that interprets a command it
  does not know produces a record that looks like evidence and is not.
- Transport is separable from protocol. Everything below is defined over a byte stream.

## Transport

**The probe listens. The driver connects.**

Not the other way round, and this is not arbitrary: the hardware has no DNS, no configuration
file, and no way to be told where to find a host. It has an address of its own, and the
person at the keyboard can read it off a screen. So the probe binds and waits, and the
driver is given the address.

- **TCP**, one connection at a time. A second connection while one is open is refused with
  `busy` rather than queued - two drivers interleaving commands would make the ordering
  record meaningless, and ordering is part of the evidence.
- **Port 9803 by default**, overridable. Nothing else claims it.
- **The stream is UTF-8 text, newline-terminated.** `\n`, never `\r\n`.
- Lines are at most **4096 bytes** including the terminator. Anything longer is a protocol
  error; payloads that could exceed it are chunked by the commands that carry them.

### Why text, and why this text

The report format (`docs/OUTPUT.md`) is already line-oriented ASCII with `|` separators, and
replies here reuse it exactly. That buys three things:

- **Every existing parser works on a session transcript.** A recorded session and a report
  are the same kind of artefact and `obscene-tool verify` reads both.
- A session can be driven by hand with `nc` when the driver itself is the suspect.
- There is no binary framing to get subtly wrong on a target with no debugger attached.

The cost is that binary payloads are hex, at two characters per byte. That cost is paid by
`blob`, which is the only command that carries bulk data, and it is worth it.

## Grammar

### Requests

```
CMD|<seq>|<verb>|<argument>...
```

- `CMD` - fixed prefix, distinguishing a request from a record on a shared transcript.
- `<seq>` - decimal, strictly increasing within a session, starting at 1. **Not optional
  and not decorative**: it is how a reply is matched to its request, and how the corpus
  records the order commands were issued in.
- `<verb>` - lower-case ASCII.
- Arguments are `|`-separated. A literal `|` or newline cannot appear in an argument.
  Numbers are hexadecimal with an `0x` prefix unless stated otherwise.

### Replies

Replies are `OBS|` records, the same shape `docs/OUTPUT.md` defines. Three are specific to
this protocol:

| Record | Fields |
|---|---|
| `ack` | seq, verb - **emitted before the command is carried out** |
| `done` | seq, outcome, value, detail |
| `refused` | seq, reason |

Every command produces exactly one `ack` and then exactly one `done` or `refused`. A command
may also produce other records - `sym`, `bytes`, `module` - between the two, and those are
defined in `docs/OUTPUT.md` and mean the same thing here as they do in a report.

**One exception, and only one: a request whose sequence number is malformed or does not
increase is refused without an acknowledgement.** An `ack` is keyed by sequence, so
acknowledging a repeated one would produce two acknowledgements bearing the same key and a
reply that cannot be matched to a request. The refusal carries the offending sequence as
sent, so the driver can see which line was rejected, and the reason is `bad-argument`.

This is the one place the acknowledge-first rule yields, and it yields to the rule that
makes acknowledgement useful in the first place.

## `ack` is the whole design, not a nicety

`ack` is written to the socket and flushed **before the command runs**. That single
ordering is what makes the corpus trustworthy, and it is the same principle the probe
already applies to its own report (`CLAUDE.md`, principle 1).

**Arbitrary calls with arbitrary arguments fault constantly. That is the normal case, not
the exceptional one.** A probe cannot report its own death: the process is gone. So the
protocol is arranged so that death is legible to the *other* end.

An `ack` with no `done` after it, and a connection that then closes, means exactly one
thing: **that command did not return.** The driver records it as such. It does not invent a
value, and it does not record a null.

This is the single most important rule in the document:

> **A command that did not answer is never recorded as having answered.**
>
> `died` is not `returned 0`. `timeout` is not `died`. A corpus that blurs these is worse
> than no corpus, because the fiction is indistinguishable from evidence and it is the
> fiction that will be trusted.

### The three non-answers, and how each is established

| outcome | established by | means |
|---|---|---|
| `died` | driver saw `ack`, then the connection closed with no `done` | the command ended the process |
| `timeout` | driver saw `ack`, waited past its budget, connection still open | the command has not returned **yet**; the probe may be alive, blocked, or looping |
| `lost` | driver saw `ack`, connection closed, and the probe never came back | ambiguous - recorded as ambiguous |

`timeout` is deliberately *not* resolved into `died`. A blocked call and a dead process look
the same from one end of a socket, and the honest record says which was observed rather
than which was guessed.

`died` and `timeout` records are written **by the driver**, and the corpus marks them
`observed-by=driver`. Everything else is `observed-by=probe`. A reader can therefore tell a
fact the system reported from a fact inferred from its silence.

## Restart and reconnect

A faulting command ends the probe. Whatever restarts it is outside this protocol - a
supervising process on a general-purpose target, a person on the hardware. What the protocol
guarantees is that the restart is **visible**:

- Every session opens with `hello`, carrying a fresh `session` identifier.
- The driver holds the identifier. A new one where it expected the old one means the probe
  restarted, and everything since the last `hello` is a different process.
- The probe does **not** attempt to resume. There is no state worth resuming and pretending
  otherwise would hide exactly the discontinuity that matters.

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
it will use, which is at most that, and refuses the session if it has no version in common.

**Capabilities are a `,`-separated list of tokens.** The driver must not send a command
whose capability was not announced. A probe that does not implement `write` says so, and a
driver that sends it anyway gets `refused`.

| token | the probe can |
|---|---|
| `call` | invoke a function by address |
| `resolve` | look a symbol up by name at run time |
| `read` | read guest memory |
| `write` | **write** guest memory - off unless deliberately enabled |
| `blob` | receive a code blob and execute it |
| `reset` | return to a known state without restarting |
| `report` | run the compiled-in check suite and stream its records |
| `gpu` | dispatch a compiled-in compute kernel and read back its result bits |

Capability negotiation exists so that one grammar covers targets with very different
abilities. **A stand-in target that has no system libraries announces no `resolve`**, and a
driver discovers that rather than assuming it.

### `part` - what produced the answers

Emitted during `hello`, one record per key. **This is not optional metadata.**

A number measured on a stand-in part and read later as authoritative for a different part is
a wrong answer with no way to see that it is wrong. That has already cost this project's
sibling months of work against the wrong device generation, with nothing in the record
saying so.

Keys are open, and these are expected where the target can answer them:

| key | example |
|---|---|
| `target` | `deck`, `hardware`, `host` |
| `gpu` | `AMD Custom GPU 0405 (gfx1033)` |
| `driver` | `radv 25.1.0` |
| `mesa` | `25.1.0` |
| `firmware` | `13.520.001` |
| `os` | `SteamOS 3.6.20` |
| `probe` | obSCEne build identifier |

**Every corpus record carries these, denormalised.** The wire binds them to a session
identifier to avoid repeating them on every line; the corpus writer expands them, because a
record that has to be joined against something else to be interpreted will eventually be
read without it.

### `bye`

Closes the session cleanly. The probe replies `done` and closes. A session that ends without
`bye` is recorded as having ended without `bye`, which is a fact about the run.

## Commands

### `resolve` - a name to an address

```
CMD|<seq>|resolve|<library>|<symbol>
```

Replies with a `sym` record and a `done` carrying the address, or `done` with outcome
`absent`.

This is the command that changes the most, because identifiers are one-way. Recovering a
name from a hash means guessing candidate names and hashing them, and it can only ever find
names something already imports. **A platform that resolves by name turns that into one
question with a yes-or-no answer, and reaches functions nothing imports at all.**

Whether the platform can do this is not assumed: it is a capability, and a target without it
says so.

### `call` - invoke something

```
CMD|<seq>|call|<address>|<arg0>|<arg1>|...
```

Up to six integer arguments, in the order the calling convention passes them. The address
comes from `resolve` or from anywhere else the driver got it.

The reply is:

```
OBS|done|<seq>|returned|<value>|
```

**`<value>` is the integer return register, and nothing else.** Not a float - a function
returning one leaves its answer in a vector register and this reads the integer one. Not a
struct. Not an error message. The record says `returned` because that is precisely what it
observed.

**What this command does not tell you** is what the call *did*. A function that allocates,
mutates a buffer, starts a thread or changes global state reports none of that here, and the
record format has no field that could imply it did. Establishing an effect means a
subsequent `read` or `call`, issued as its own command, recorded as its own input. That
sequencing is the evidence; a single result is not.

### `read` - guest memory

```
CMD|<seq>|read|<address>|<length>
```

Replies with `bytes` records - the same record a report uses - followed by `done`. Length is
bounded per command; longer regions are read across several commands and the driver
reassembles them.

Reading an unmapped address faults, and that is the `died` path, not an error reply. A probe
may validate an address first if the platform lets it, and if it does, it says so by
answering `refused|unmapped` instead. **Both behaviours are legitimate and the record
distinguishes them**, because "this address is not readable" and "asking about this address
killed the process" are different facts.

### `write` - guest memory

```
CMD|<seq>|write|<address>|<hex>
```

Behind the `write` capability, which is off unless deliberately enabled.

`read` and `call` cost a crash at worst. **An arbitrary write costs a crash and whatever
state was being built**, and on a target that took effort to reach that is the expensive
kind of mistake. Read-only is the default posture.

### `blob` and `run` - the escape hatch

Two primitives cover most questions. The rest need code.

```
CMD|<seq>|blob|<id>|<offset>|<hex>
CMD|<seq>|run|<id>|<arg0>|...
```

`blob` uploads position-independent machine code in bounded chunks; `run` calls it. The blob
is identified so several can be resident, and chunking is what keeps a line inside the
length bound.

This exists so the vocabulary can stay small. Adding a command for every question would
grow the grammar without limit and couple it to whatever asked; an escape hatch keeps the
protocol generic and puts the specificity in the payload, where it belongs.

### `reset` - a known state

```
CMD|<seq>|reset
```

**State leaks between probes and this is a real problem, not a tidiness concern.** An
allocation made by one command changes what the next one observes. A memory map read after
ten calls is not the map a title would see at startup.

Three honest answers, and a target picks whichever it can support:

1. **A fresh process per command.** Cleanest, and only possible where something can restart
   the probe cheaply.
2. **`reset`**, returning what can be returned - freeing what was allocated, closing what was
   opened. Announced as a capability, and it never claims more than it does.
3. **Neither**, in which case ordering is recorded as part of the input. Every corpus record
   carries its `seq` and the count of commands since the last `reset` for exactly this
   reason: a result that depended on what came before is still interpretable if what came
   before is written down.

A probe that cannot reset must not pretend it can. `refused|unsupported` is the correct
answer and it is more useful than a `reset` that returns `ok` and does nothing.

### `report` - the compiled-in suite

```
CMD|<seq>|report
```

Runs the checks built into the probe and streams their records, then `done`. This is the
existing report (`docs/OUTPUT.md`) delivered over the socket rather than over whatever
output channel the target happens to have - which on some targets is none.

### `gpu` - dispatch a compiled-in kernel

```
CMD|<seq>|gpu|<kernel>|<operand0>|<operand1>|...
```

Runs one of the probe's built-in compute kernels over the supplied operands and streams the
results. The reply is a `gpudev` record (the device and its type), then one `gpu` or `gpuop`
record per lane, then `done` whose value is the lane count.

**This runs only named, compiled-in shaders** - never arbitrary code, unlike `call` and
`blob`. That is what makes it safe to expose broadly: the driver chooses *which* known kernel
and *what* inputs, which is the interactive loop - a new question answered without rebuilding
and redeploying an ELF. The kernel names are the ones a `report` emits.

Operands are 32-bit words (float bit patterns) and follow the kernel's arity: a unary kernel
takes N operands and answers N lanes; an arity-*k* kernel takes a multiple of *k*, each group
a tuple. A wrong operand count, or an unknown kernel, is `refused|bad-argument`. A dispatch
that fails without crashing answers `done|returned|0` - zero lanes, no records - which is
distinct from a death (the `ack`-with-no-`done` path). This capability is announced only when
the build has a working GPU backend.

### Unknown verbs

```
OBS|refused|<seq>|unknown-verb
```

Always. A responder never guesses, never approximates, and never answers a command it does
not implement. The reasons are `unknown-verb`, `unsupported`, `bad-argument`, `busy`,
`not-negotiated`, `unmapped`, `unauthorised`.

## Ordering and concurrency

One command in flight at a time. The driver sends a command and waits for `done` or
`refused` before sending the next.

This is a real constraint rather than an implementation shortcut. Overlapping commands would
make `died` unattributable - with two commands outstanding and a process that has just
vanished, nothing says which one killed it, and that attribution is the finding.

## Versioning

The version is negotiated in `hello` and this document describes version 1.

Within a version: **new verbs may be added, new capability tokens may be added, and new
fields may be appended to the end of a record.** Nothing else. Field order does not change,
a field's meaning does not change, and a verb is never repurposed. A consumer ignores
records and fields it does not recognise, exactly as it must for a report.

Anything that would break a consumer built against this file is a new version.

## Security posture

**A socket that executes arbitrary code, gated by a session secret.** Said plainly because it
is worth being clear-eyed about: the default build announces `call`, `read`, `report` and
`gpu`, and `call` invokes an arbitrary address with six integer arguments.

- **A session secret, generated fresh at every startup.** Described below. It stops another
  device on the network driving the probe, which is the threat it exists for.
<!-- obscene:claim file=src/probe/net_posix.c contains=INADDR_ANY -->
- **It binds every interface**, because the hardware module must: the driver is on another
  machine and there is no loopback worth binding to. That is why the secret is not optional
  on that target.
<!-- obscene:claim file=src/probe/net.c contains=OBS_NET_ESCAPE -->
- `write` and `blob` are off unless a build enables them.
- It is a laboratory instrument. It does not belong on a network anyone else is on, and it
  does not belong running when nobody is using it.

### The session secret

<!-- obscene:claim file=src/probe/net.c contains=obs_net_secret_generate -->
<!-- obscene:claim file=src/probe/net.c absent=OBS_NET_SECRET_BUILTIN -->
**Generated per startup, never compiled in.** A secret built into the module would be shared
by everybody who has that module, which is the opposite of a secret. This one is made when the
probe starts listening, lasts for that run, and is replaced by a restart.

**It is displayed**, because the hardware has no other channel. The HUD already draws the port so
a driver can be pointed at it - "read the address off the screen", which this protocol assumes
- and the secret is drawn beside it as `KEY`. The host build prints it on stderr, and it is
emitted as a `sysinfo` record either way.

`hello` carries it as a fourth field, appended after the version:

```
CMD|1|hello|1|d98130d191ccee2143658132dd1d2499
OBS|ack|1|hello
OBS|hello|1|c0x1|call,read,report
```

A wrong or missing secret is `refused|unauthorised`, **before any capability is disclosed** -
an unauthenticated peer does not get to learn what this build can do. The existing rule does
the rest: `greeted` is never set, so every other verb is refused `not-negotiated`.

```
CMD|1|hello|1|deadbeefdeadbeefdeadbeefdeadbeef
OBS|ack|1|hello
OBS|refused|1|unauthorised
CMD|2|report
OBS|ack|2|report
OBS|refused|2|not-negotiated
```

A probe that could not generate one serves unauthenticated and says so, on stderr and as an
absent `secret` field. Refusing to run would remove a working instrument to enforce a control
the platform cannot support.

**What it is not.** The socket is cleartext, so anyone who can observe the link reads the
secret out of the `hello` and never has to guess it. And on the hardware the entropy behind it is
timing jitter - the low bits of differences between successive timestamp reads, mixed - rather
than a CSPRNG, because there is not one available: tens of bits, not 128. The host build reads
`/dev/urandom` and does better. Both are proportionate to "something else on the network
connected to it"; neither is proportionate to a wiretap.

**The comparison is constant-time.** An ordinary one returns at the first differing byte, so
an adversary who can time the reply recovers the secret a character at a time - a few hundred
attempts rather than an intractable number. Every byte is compared and the differences
accumulated.

## The captured exchanges are part of the contract

`docs/examples/protocol/` holds real transcripts, one per scenario: negotiation, a call that
returns, a call that dies, a timeout, a refusal, memory read, blob and run, reset.

They exist so that **a consumer can be built and tested with no hardware attached.** A
specification in prose is agreed to and then interpreted differently by each side; a
transcript is checkable. `obscene-tool protocol` parses every one of them against the
grammar above and fails if a line does not conform, so the examples cannot drift away from
this document without something saying so.
