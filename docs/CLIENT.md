# Writing a client for the obSCEne protocol

`docs/PROTOCOL.md` is the specification: what the wire carries and what each reply means.
This is the other half - what a client author has to do about it, in the order they will hit
it.

**Self-contained on purpose.** Everything needed to build a client is here, so this file can
be copied out and used as data by a consumer that does not have obSCEne checked out. The
contract is this document, `docs/PROTOCOL.md`, `docs/OUTPUT.md`, and the captured transcripts
in `docs/examples/protocol/`, all of which are copyable.

**Read the "implemented today vs reserved" section before writing any reader.** Some of the
protocol is spec-only right now, and building a consumer for a record that never arrives is
the mistake this section exists to prevent.

## The shape, in one paragraph

obSCEne is the server. It runs on the target (the hardware, or an emulator standing in for one),
binds a TCP port, and waits. The client connects to that port and issues
commands, one at a time, and reads back what the target actually did. This is deliberate - a
hardware has no shell, no DNS, and no config file; it has an IP you read off a screen, and
something on the network connects *to* it.

## Connecting

- **TCP, port 9803** by default. obSCEne binds `INADDR_ANY` (all interfaces), so it is
  reachable from any machine that can route to the target and past its firewall.
- **The address to connect to depends on where obSCEne is running:**
  - **In shadPS4 (or any emulator whose net layer maps to host sockets):** the guest listen
    opens a port on the *host* machine. From the same machine, connect to `127.0.0.1:9803`.
    From another machine, `‹emulator-host-ip›:9803`, subject to that host's firewall.
  - **On real hardware:** connect to the *hardware's* IP, `‹hardware-ip›:9803`. The operator
    reads that IP from the device; obSCEne does not and cannot self-report it reliably (see
    "Machine origin" below). Your GUI should have the operator type the address in.
- **One connection at a time.** The server serves a single session to completion, then
  accepts the next. A second client that connects during a live session waits in the listen
  backlog until the first disconnects; it is not refused with an error (the spec reserves a
  `busy` refusal, but it is **not implemented today** - treat a stall on connect as "someone
  else is connected", not as a protocol error).

obSCEne only listens if it was built to. The operator builds and runs a serving module
(`make module GEN=… SERVE=1`, with the crash-exclusion list); your client just connects. If
nothing is listening, obSCEne is either not a serving build or has not reached the listen yet
(it serves *after* running its self-report).

## The wire

- UTF-8 text, **newline-terminated** (`\n`, never `\r\n`; a stray `\r` is tolerated on input).
- Lines are at most **4096 bytes** including the terminator. Anything longer is a protocol
  error.
- **Requests** (client → server): `CMD|<seq>|<verb>|<arg>|<arg>…`
- **Replies** (server → client): `OBS|<kind>|…` - the same record shapes `docs/OUTPUT.md`
  defines, so the same parser reads a session and a report.

### Sequence numbers are the client's job

`<seq>` is a decimal integer, **strictly increasing within a session, starting at 1**. The
client owns it and the server never supplies it. It is how a reply is matched to its request
and how the corpus records issue order. A repeated or non-increasing sequence is refused as
`bad-argument` **without an acknowledgement** - the one exception to acknowledge-first, because
an `ack` is keyed by sequence and a repeated key could not be matched. (`docs/PROTOCOL.md`
has the full rule.)

## The handshake

Every session opens with `hello`, before any other command is accepted:

```
→ CMD|1|hello|1|<secret>
← OBS|ack|1|hello
← OBS|hello|1|<session-id>|<capabilities>
← OBS|part|<session-id>|probe|<build-id>
← OBS|part|<session-id>|binary|<module|host>
← OBS|part|<session-id>|transport|<scenet|posix>
← OBS|done|1|ok||
```

**The fourth field is the session secret**, and it is required whenever the probe generated
one. It is a random value made fresh at every startup, never compiled in, and the probe
displays it: on the hardware the HUD draws it beside the listening port (labelled `KEY`), and the
host build prints it on stderr. There is no way to obtain it remotely - that is the point.

A wrong or missing secret is `refused|unauthorised`, **before any capability is announced**, so
an unauthenticated peer learns nothing about the build. Nothing else is accepted afterwards
either: the session is never negotiated, so every later command is `refused|not-negotiated`.

A probe that could not generate a secret serves without one and says so. Then `hello` with no
fourth field is accepted, which is what a driver written before this existed sends.

The socket is cleartext, so this defends against another machine on the network connecting -
not against anyone able to observe the link, who reads the secret out of this exchange. See
`docs/PROTOCOL.md`.

- The `1` argument is the highest protocol version the client speaks. The server replies with
  the version it will use (at most that).
- **`<session-id>`** identifies this run of the probe. A prefix says how much to trust it:
  `t…` is clock-derived (changes across restarts - restart detection works), `c…` is a
  counter (only distinguishes sessions within one process). **A session id you did not expect,
  where you expected the previous one, means the probe restarted** - everything before it
  belongs to a different process. A faulting command ends the probe, so this happens.
- **`<capabilities>`** is a comma-separated list of what this build can do. **Send no command
  whose capability was not announced.** Today a serving build announces `call,read,report`.
- **`part` records** carry what the probe can *observe* about itself. `binary` is the build
  kind, not the machine. The machine identity is not here - see "Machine origin".

Close cleanly with `CMD|<seq>|bye` → `OBS|done|<seq>|ok||`.

## Implemented today vs reserved

This is the part to get right before building readers.

| verb | status | what it does |
|---|---|---|
| `hello` | **implemented** | negotiate version and capabilities |
| `report` | **implemented** | run the compiled-in suite (see caveat below) |
| `call` | **implemented** | invoke an address with args, read the return |
| `read` | **implemented** | read guest memory |
| `bye` | **implemented** | close the session |
| `resolve` | reserved | name → address; **refused today** |
| `write` | reserved | write guest memory; **refused today** |
| `blob` / `run` | reserved | upload code and run it; **refused today** |
| `reset` | reserved | return to a known state; **refused today** |

A serving build now announces `call,read,report`. A reserved verb is refused with
`OBS|refused|<seq>|unknown-verb` (or `not-negotiated` if its capability was not announced).
`write`/`blob`/`reset` stay off deliberately - a read or a call costs a crash at worst, a
write costs a crash *and* whatever state was being built. A client must still handle the
reserved verbs being refused, and must not send them expecting an answer.

## `call` - invoke an address

```
→ CMD|3|call|<addr>|<arg0>|…|<arg5>          (address and up to six integer args, all hex)
← OBS|ack|3|call
← OBS|done|3|returned|<value-in-hex>|
```

- Up to six integer arguments; omitted ones are zero. The value is the integer return
  register, uninterpreted - meaningful for functions returning an integer, whatever was in
  that register otherwise (a float lands in a vector register this does not read).
- A **malformed or missing** address, or a bad argument, is `refused|bad-argument`.
- A **valid but fatal** address (0, an unmapped page, a bad callee) is **not** refused - it
  is called, and it faults. That is the normal outcome of poking an arbitrary address: the
  `ack` is already on the wire, so the death shows up as a lone `ack` with no `done`, and you
  record it `died` (never `returned 0`). Null is called, not rejected.
- Getting an address to call: today you supply it (from a report, a prior `read`, or known
  layout). `resolve` (name → address) is still reserved, so name lookup is not available yet.

## `read` - dump guest memory

```
→ CMD|2|read|<addr>|<len>                    (both hex)
← OBS|bytes|read/0x<addr>|(memory)|contents|<offset>|<hex>
← OBS|bytes|read/0x<addr>|(memory)|contents|<offset>|<hex>          (16 bytes per record)
← OBS|done|2|returned|<len-in-hex>|
```

- The `bytes` records are the same shape a report uses (`docs/OUTPUT.md`), so the same parser
  reads them. The id is `read/0x<addr>` - a single `0x`.
- An **unmapped address faults** - this build does **not** pre-validate and reply
  `refused|unmapped`. It reads, and if the page is bad the process dies, which is the `died`
  path. So expect `died` for a bad read address on obSCEne, not `unmapped`. (`docs/PROTOCOL.md`
  permits a probe to pre-validate where the platform allows; this one does not.)

Both were proven live over a real socket: `read` returned a process's ELF-header bytes,
`call` returned a real function's value, and `call 0x0` produced the lone-`ack` death.

## `report` - run the suite, streamed over the socket

```
→ CMD|2|report
← OBS|ack|2|report
← OBS|meta|1|26|499               (the full report streams here, between ack and done)
← OBS|sink|obscene-report.txt
← OBS|sysinfo|memory|known|441M   (the sysinfo header - see the next section)
← …section / try / res / sym / sectiontally records…
← OBS|tally|…
← OBS|end|…
← OBS|done|2|returned|<fail-count-in-hex>|
```

`report` runs the compiled-in suite and **streams every record down the socket as it runs**,
between the `ack` and the `done` - the same `meta` / `sink` / `sysinfo` / `section` / `try` /
`res` / `sym` / `tally` shapes `docs/OUTPUT.md` defines, so one parser reads a live session and
a report file alike. The stream is *additive*: the probe still writes the identical records to
its own stdout and file sink (`obscene-report.txt` on the target), so a run is never lost to a
dropped connection. The `done` value is the fail count, as a summary after the stream, not
instead of it.

Two things to build for:

- **It is a large stream.** A full run is tens of thousands of records - the census `sym`
  records dominate. Read and parse incrementally; do not buffer the whole run before parsing.
- **It can die mid-stream.** The suite pokes the platform, so a faulting check ends the probe
  partway: you receive the records up to that point, then the connection closes with no `done`.
  That is the `died` path below, and the partial stream is still evidence - the last `try`
  without its `res` names where it fell over.

(An older obSCEne did not stream at all - `report` returned only `ack` + `done` and the records
reached the file alone. If you are reading a capture that stops at `done` with nothing between,
that is a pre-streaming one.)

## `sysinfo` records - the target's account of itself

The report stream carries a short block of `sysinfo` records near the top (after `sink`): the
same status readout obSCEne draws on screen - memory, VRAM, generation, its own address - one
record per field. A serving build also emits this block the moment it starts listening, so it
is in the probe's stdout and file even before a client asks for a run.

```
OBS|sysinfo|listening|known|0.0.0.0:9803
OBS|sysinfo|memory|known|441M
OBS|sysinfo|vram|known|4608M
OBS|sysinfo|ip|unconfirmed|unknown
OBS|sysinfo|firmware|unconfirmed|unknown
OBS|sysinfo|generation|known|both
OBS|sysinfo|temp|absent|unknown
OBS|sysinfo|storage|absent|unknown
```

Fields are `field | state | value`. **Read the `state`, not just the value.** All three states
can show `unknown`, and they mean different things:

| state | value | means |
|---|---|---|
| `known` | the real reading (`441M`) | read through a confirmed signature |
| `unconfirmed` | `unknown` | the query resolves, but obSCEne has no confirmed signature to call it through yet - obSCEne's TODO, not a platform gap |
| `absent` | `unknown` | no such query here - the platform gap |

A GUI that collapses all three to one blank throws away the only distinction worth showing: an
emulator's missing feature versus obSCEne's unfinished wiring versus a genuine value.

**The state set is open, so tolerate an unrecognised one** - treat it as unrecognised and
degrade, exactly as you already do for a record kind you do not know. This is the contract, not
just prudence: obSCEne may append a value to a report enum (a `state`, a `res` `status` or
`provenance`, a `call` `outcome`) without bumping the format version, because the report is an
open stream of observations (`docs/OUTPUT.md`). The **protocol grammar is the closed exception**
- verbs, refusal reasons and capability tokens are fixed lists the specification enforces, so a
new one there is a spec change you will see announced, never a silent arrival. Your `Unrecognised`
fallback is the right build for the report side; keep it, and rely on the closed grammar for the
protocol side.

**These are observations, never machine identity - the same rule as "Machine origin" above
(D108).** Inside an emulator every one of these answers as the emulator chooses: `memory|known|
441M` is shadPS4's number, not the hardware's. Display them as the target's *self-report*; never
promote them to graded provenance. In particular, `generation`'s value carries the finding and
does not collapse it: `4 (gnm)` / `5 (agc)` only when exactly *one* graphics driver
resolves; `both` when both do (a positive observation - the fingerprint of a stub-everything
loader as much as of real back-compat, and a different fact from an absence); `unknown` only
when *neither* resolves. So `known|both` and `absent|unknown` are distinct - "both here" is not
"nothing here". `both` is not a claim the machine *is* both generations; presence is not
implementation. Treat `sysinfo|generation` as a display hint; the graded target generation stays
operator-asserted, like the rest of the machine identity.

## Outcomes, and the rule that matters most

A command ends in exactly one terminal reply:

- `OBS|done|<seq>|<outcome>|<value>|<detail>` - `outcome` is `ok`, `returned`, or one of the
  non-answers below.
- `OBS|refused|<seq>|<reason>` - `reason` ∈ {`unknown-verb`, `unsupported`, `bad-argument`,
  `busy`, `not-negotiated`, `unmapped`, `unauthorised`}.

**Arbitrary calls fault constantly - that is normal, not exceptional - and the protocol is
built so a death is legible from the client's end.** The `ack` is written and flushed *before*
the command runs. So:

> An `ack` with no matching `done`, followed by the connection closing, means the command
> ended the process. **Record it as `died`, never as `returned 0`.**

The three non-answers, and how the client establishes each - **the client writes these, the
probe cannot, because it is gone:**

| outcome | client observes | means |
|---|---|---|
| `died` | `ack`, then the connection closed with no `done` | the command ended the process |
| `timeout` | `ack`, budget elapsed, connection still open | not returned *yet*; do not resolve into `died` |
| `lost` | `ack`, connection closed, and it is unclear if the probe returned | ambiguous, recorded as ambiguous |

A value field is **empty** for every non-answer. Writing `0x0` there is the exact fiction the
format exists to prevent - a call that died is indistinguishable from one that returned zero
if you do that, and it is the fiction that gets trusted later.

## Machine origin - the part a grading client depends on

obSCEne's records carry no trustworthy machine identity, **by design**, and this is the point
your GUI has to handle.

**A probe cannot certify its own machine.** Running inside shadPS4, obSCEne's
`sceKernelGetSystemSwVersion` returns *shadPS4's* chosen version. If obSCEne stamped that as
`firmware=`, it would be an emulator's answer wearing the hardware's badge - the exact
`measured`-vs-`assumed` confusion your grading exists to catch.

So the machine identity - `target`, `gpu`, `firmware`, above all "is this real hardware" - is
**asserted by the operator**, not self-reported by the probe. Your GUI must collect it from
the person running the session (a form: what device, what firmware, real hardware or which
emulator) and stamp it onto the records it keeps. obSCEne's own reference driver does this with
`--part key=value`; your client should do the equivalent.

Grade accordingly: `hardware → measured` **only when the operator asserted the target was real
hardware.** The same result observed in shadPS4 is `assumed`, and the operator's assertion is
the only thing that tells you which.

## The corpus your client should write

A *session* is transient; the *corpus* is the product. obSCEne's reference driver writes each
session as `OBSCORPUS|` records (documented in `docs/OUTPUT.md`); build the same, or your own
shape, but preserve these properties:

- **The machine origin (operator-asserted) on every record**, denormalised - a record read
  alone, months later, must still say where it came from. Joining against a separate header
  that has drifted away is the failure mode.
- **`observed-by`**: `probe` for what the system reported, the client for a `died`/`timeout`/
  `lost` it inferred from silence. These are not equally strong and the record should say
  which.
- **Non-answers carry no value.** Never a `0x0` for a death.
- **Absent provenance stays absent.** A record with no grade is ungraded; never invent one.

## Building and testing with no hardware

Your client can be built and tested end to end with nothing attached:

- **Against captured transcripts.** `docs/examples/protocol/*.txt` are real exchanges,
  including the awkward ones - a death (`03-died.txt`), a timeout (`04-timeout.txt`),
  refusals (`05-refused.txt`), a bad sequence (`10-bad-sequence.txt`). Copy them in as data
  and drive your parser off them. obSCEne's reference driver has a `--replay <file>` mode for
  exactly this; build the equivalent so your CI never opens a socket.
- **Against a live emulator, when you want the real thing.** The operator runs a serving
  obSCEne in shadPS4 (proven working) and your client connects to `127.0.0.1:9803`. That
  exercises the same wire path the hardware will use.

**Your CI must never require a socket or a plugged-in hardware** - replay transcripts there.
The live path is for the operator, not the build.

## A real session, start to finish

From obSCEne serving inside shadPS4, with the `report` stream abbreviated to its shape (the
real run is tens of thousands of records). The `part` machine fields would be operator-supplied
in a graded run:

```
→ CMD|1|hello|1|d98130d191ccee2143658132dd1d2499
← OBS|ack|1|hello
← OBS|hello|1|t0x13bd05e|call,read,report
← OBS|part|t0x13bd05e|probe|dev
← OBS|part|t0x13bd05e|binary|module
← OBS|part|t0x13bd05e|transport|scenet
← OBS|done|1|ok||
→ CMD|2|report
← OBS|ack|2|report
← OBS|meta|1|26|499                   (the run streams between ack and done…)
← OBS|sink|obscene-report.txt
← OBS|sysinfo|memory|known|441M       (…the sysinfo header, then the checks…)
← OBS|sysinfo|vram|known|4608M
← OBS|sysinfo|generation|known|both
← …section / try / res / sym / tally records…
← OBS|end|1
← OBS|done|2|returned|0x28|           (0x28 = 40 checks failed; the detail is above and in the file)
→ CMD|3|bye
← OBS|ack|3|bye
← OBS|done|3|ok||
```

## Checklist for a client and its interface

1. TCP client, operator-supplied `host:port` (default 9803), one connection at a time.
2. Line reader/writer: `\n`-terminated, 4096-byte cap, `CMD|` out and `OBS|` in.
3. Client-owned sequence numbers, strictly increasing from 1.
4. `hello` first; parse `<session-id>` and capabilities; refuse to send an un-announced verb.
5. Detect a changed session id as a probe restart.
6. Implement the ack-before-done model: a lone `ack` + closed socket → `died`; a budget →
   `timeout`; ambiguous → `lost`; never a value on any of them.
7. A GUI form for operator-asserted machine identity, stamped onto every kept record.
8. Ingest the report stream over the socket - records arrive between `report`'s `ack` and
   `done`; parse incrementally (a run is tens of thousands of records) and tolerate the stream
   dying partway as a `died`. The same records also land in the probe's file sink. Ignore
   record kinds you do not know.
9. Read the `sysinfo` block for the target's self-report, and key on its `state` field
   (`known`/`unconfirmed`/`absent`), not just the value. Show it as self-reported, never as
   graded machine identity - `generation` included.
10. Replay-from-transcript mode for CI; never open a socket in a test.
11. Build `call` and `read` consumers now - they are live. `call` → `returned`/`bad-argument`/
    `died`; `read` → `bytes` records then `returned`, or `died` on a bad address. Do not build
    `write`/`blob`/`reset`/`resolve` yet - handle them being refused, and wire them up when
    obSCEne announces them.
