# Writing a client for the obSCEne protocol

`docs/PROTOCOL.md` is the specification: what the wire carries and what each reply means.
This document is what a client author does about it, in the order they meet it.

It is self-contained so it can be copied out and used by a consumer that does not have
obSCEne checked out. The contract is this document, `docs/PROTOCOL.md`, `docs/OUTPUT.md`, and
the captured transcripts in `docs/examples/protocol/`.

## Shape

obSCEne is the server. It runs on the target (the hardware, or an emulator standing in for
one), binds a TCP port, and waits. The client connects, issues commands one at a time, and
reads back what the target did. The hardware has no shell, no DNS and no config file; it has
an address the operator reads off a screen, and the client connects to it.

## Connecting

- TCP, port 9803 by default. obSCEne binds `INADDR_ANY`, so it is reachable from any machine
  that can route to the target and past its firewall.
- The address depends on where obSCEne runs:
  - **In an emulator whose net layer maps to host sockets** (shadPS4, for one), the guest
    listen opens a port on the host. From the same machine, connect to `127.0.0.1:9803`;
    from another, `<emulator-host-ip>:9803`, subject to that host's firewall.
  - **On hardware**, connect to `<console-ip>:9803`. The operator reads the address from the
    device and types it into the client; obSCEne does not self-report it reliably (see
    "Machine origin").
- One connection at a time. The server serves a single session to completion, then accepts
  the next. A client that connects during a live session waits in the listen backlog until
  the first disconnects; treat a stall on connect as "someone else is connected".

obSCEne listens only in a serving build. The operator builds and runs one
(`make module GEN=<gen> SERVE=1`, with the crash-exclusion list). It serves after running its
self-report, so nothing is listening until that finishes.

## The wire

- UTF-8 text, newline-terminated (`\n`, never `\r\n`; a stray `\r` is tolerated on input).
- Lines are at most 4096 bytes including the terminator. Anything longer is a protocol error.
- Requests (client to server): `CMD|<seq>|<verb>|<arg>|<arg>...`
- Replies (server to client): `OBS|<kind>|...` - the record shapes `docs/OUTPUT.md` defines,
  so one parser reads a session and a report.

### Sequence numbers

`<seq>` is a decimal integer, strictly increasing within a session, starting at 1. The
client owns it; the server never supplies it. It matches a reply to its request and records
issue order. A repeated or non-increasing sequence is refused as `bad-argument` without an
acknowledgement (`docs/PROTOCOL.md`, bad sequence rule).

## Handshake

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

- `1` is the highest protocol version the client speaks. The server replies with the version
  it will use, at most that.
- The fourth field is the **session secret**, required whenever the probe generated one. It
  is random, made fresh at every startup, and displayed: on hardware the HUD draws it beside
  the listening port as `KEY`; the host build prints it on stderr. It cannot be obtained
  remotely.
- A wrong or missing secret is `refused|unauthorised`, before any capability is announced.
  The session is then never negotiated, so every later command is `refused|not-negotiated`.
- A probe that could not generate a secret serves without one and says so. `hello` with no
  fourth field is then accepted.
- The socket is cleartext: the secret defends against another machine on the network
  connecting, not against anyone observing the link (`docs/PROTOCOL.md`).
- `<session-id>` identifies this run of the probe. Its prefix says how much to trust it:
  `t...` is clock-derived and changes across restarts; `c...` is a counter and distinguishes
  sessions only within one process. A session id other than the expected one means the probe
  restarted, and everything before it belongs to a different process.
- `<capabilities>` is a comma-separated list of what this build can do. Send no command whose
  capability was not announced.
- `part` records carry what the probe can observe about itself. `binary` is the build kind,
  not the machine. Machine identity is not here - see "Machine origin".

Close cleanly with `CMD|<seq>|bye`, answered `OBS|done|<seq>|ok||`.

## Capabilities and verbs

| capability | verbs |
|---|---|
| `call` | `call` |
| `read` | `read` |
| `report` | `report` |
| `blob` | `blob`, `run` |
| `reset` | `reset` |
| `resolve` | `resolve` |
| `write` | `write` |
| `gpu` | `gpu` |

`hello` and `bye` need no capability. A default serving build announces `call,read,report`;
a build with `OBS_NET_ESCAPE` adds `blob,reset`. `write` is off in every build: a read or a
call costs a crash at worst, a write costs a crash and whatever state was being built.

A verb whose capability was not announced is refused `not-negotiated` when the build knows
the verb, or `unknown-verb` when it does not. A client handles both and never sends an
un-announced verb expecting an answer.

## `call` - invoke an address

```
→ CMD|3|call|<addr>|<arg0>|...|<arg5>          (address and up to six integer args, all hex)
← OBS|ack|3|call
← OBS|done|3|returned|<value-in-hex>|
```

- Up to six integer arguments; omitted ones are zero. The value is the integer return
  register, uninterpreted: meaningful for a function returning an integer, whatever was in
  that register otherwise (a float lands in a vector register this does not read).
- A malformed or missing address, or a bad argument, is `refused|bad-argument`.
- A valid but fatal address (0, an unmapped page, a bad callee) is not refused. It is called
  and it faults. The `ack` is already on the wire, so the death shows as a lone `ack` with no
  `done`, recorded `died`, never `returned 0`.
- The client supplies the address: from a report, a prior `read`, known layout, or `resolve`
  where announced.

## `read` - dump guest memory

```
→ CMD|2|read|<addr>|<len>                    (both hex)
← OBS|bytes|read/0x<addr>|(memory)|contents|<offset>|<hex>
← OBS|bytes|read/0x<addr>|(memory)|contents|<offset>|<hex>          (16 bytes per record)
← OBS|done|2|returned|<len-in-hex>|
```

- The `bytes` records are the report's shape (`docs/OUTPUT.md`). The id is `read/0x<addr>`,
  with a single `0x`.
- obSCEne does not pre-validate the address. An unmapped address faults and the process dies,
  which is the `died` path; expect `died`, not `refused|unmapped`, for a bad read address.

## `report` - the suite, streamed

```
→ CMD|2|report
← OBS|ack|2|report
← OBS|meta|1|26|499               (the full report streams here, between ack and done)
← OBS|sink|obscene-report.txt
← OBS|sysinfo|memory|known|441M   (the sysinfo header - see the next section)
← ...section / try / res / sym / sectiontally records...
← OBS|tally|...
← OBS|end|...
← OBS|done|2|returned|<fail-count-in-hex>|
```

`report` runs the compiled-in suite and streams every record down the socket as it runs,
between the `ack` and the `done`, in the shapes `docs/OUTPUT.md` defines. The stream is
additive: the probe also writes the same records to its stdout and file sink
(`obscene-report.txt` on the target), so a dropped connection loses nothing. The `done` value
is the fail count, a summary after the stream.

- The stream is large; census `sym` records dominate. Parse incrementally rather than
  buffering the whole run.
- A faulting check ends the probe partway. The client receives the records up to that point,
  then the connection closes with no `done`: the `died` path. The partial stream is evidence -
  the last `try` without its `res` names where it stopped.

## `sysinfo` records

The report stream carries a block of `sysinfo` records after `sink`: the status readout
obSCEne draws on screen, one record per field. A serving build also emits the block when it
starts listening, so it reaches stdout and the file before any client asks for a run.

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

Fields are `field|state|value`. Key on the `state`, not only the value; all three states can
show `unknown`:

| state | value | means |
|---|---|---|
| `known` | the reading (`441M`) | read through a confirmed signature |
| `unconfirmed` | `unknown` | the query resolves, but obSCEne has no confirmed signature to call it through |
| `absent` | `unknown` | no such query on this platform |

A display that collapses the three loses the distinction between an emulator's missing
feature, obSCEne's own unwired query, and a real value.

The state set is open: treat an unrecognised state as unrecognised and degrade. obSCEne may
append a value to any report enum (a `state`, a `res` `status` or `provenance`, a `call`
`outcome`) without bumping the format version (`docs/OUTPUT.md`). The protocol grammar is
closed - verbs, refusal reasons and capability tokens are fixed lists, and a new one is a
specification change.

These are observations, never machine identity (D108). Inside an emulator each answers as the
emulator chooses: `memory|known|441M` is the emulator's number. Display them as the target's
self-report and never promote them to graded provenance. `generation` is `4 (gnm)` or
`5 (agc)` when exactly one graphics driver resolves, `both` when both do, and `unknown` when
neither does. `known|both` and `absent|unknown` are different findings. `both` is not a claim
that the machine is both generations; presence is not implementation. Treat
`sysinfo|generation` as a display hint; the graded target generation is operator-asserted.

## Outcomes

A command ends in exactly one terminal reply:

- `OBS|done|<seq>|<outcome>|<value>|<detail>` - `outcome` is `ok`, `returned`, or a
  non-answer below.
- `OBS|refused|<seq>|<reason>` - `reason` is one of `unknown-verb`, `unsupported`,
  `bad-argument`, `busy`, `not-negotiated`, `unmapped`, `unauthorised`.

The `ack` is written and flushed before the command runs, so a death is legible from the
client's end:

> An `ack` with no matching `done`, followed by the connection closing, means the command
> ended the process. **Record it as `died`, never as `returned 0`.**

The client writes the non-answers; the probe cannot, because it is gone.

| outcome | client observes | means |
|---|---|---|
| `died` | `ack`, then the connection closed with no `done` | the command ended the process |
| `timeout` | `ack`, budget elapsed, connection still open | not returned yet; not resolved into `died` |
| `lost` | `ack`, connection closed, and it is unclear whether the probe returned | ambiguous, recorded as ambiguous |

The value field is empty for every non-answer. A `0x0` there makes a call that died
indistinguishable from one that returned zero.

## Machine origin

obSCEne's records carry no trustworthy machine identity, by design. A probe cannot certify
its own machine: inside an emulator, `sceKernelGetSystemSwVersion` returns the emulator's
chosen version, and stamping that as `firmware=` would present an emulator's answer as a
hardware measurement.

The machine identity - `target`, `gpu`, `firmware`, and whether this is real hardware - is
asserted by the operator. The client collects it from the person running the session (what
device, what firmware, real hardware or which emulator) and stamps it onto the records it
keeps. obSCEne's reference driver does this with `--part key=value`.

Grade `hardware -> measured` only when the operator asserted the target was real hardware. The
same result observed in an emulator is `assumed`.

## The corpus

A session is transient; the corpus is the product. obSCEne's reference driver writes each
session as `OBSCORPUS|` records (`docs/OUTPUT.md`). A client's own corpus preserves these
properties:

- The operator-asserted machine origin on every record, denormalised, so a record read alone
  still says where it came from.
- `observed-by`: `probe` for what the system reported, the client for a `died`, `timeout` or
  `lost` it inferred from silence.
- Non-answers carry no value.
- Absent provenance stays absent. A record with no grade is ungraded.

## Testing without hardware

- **Captured transcripts.** `docs/examples/protocol/*.txt` are real exchanges, including a
  death (`03-died.txt`), a timeout (`04-timeout.txt`), refusals (`05-refused.txt`) and a bad
  sequence (`10-bad-sequence.txt`). Drive the parser from them. The reference driver's
  `--replay <file>` mode does this; CI replays transcripts and never opens a socket.
- **A live emulator.** The operator runs a serving obSCEne in an emulator and the client
  connects to `127.0.0.1:9803`, the same wire path the hardware uses.

## A session

From obSCEne serving inside shadPS4, with the `report` stream abbreviated. In a graded run
the machine fields are operator-supplied:

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
← OBS|meta|1|26|499                   (the run streams between ack and done...)
← OBS|sink|obscene-report.txt
← OBS|sysinfo|memory|known|441M       (...the sysinfo header, then the checks...)
← OBS|sysinfo|vram|known|4608M
← OBS|sysinfo|generation|known|both
← ...section / try / res / sym / tally records...
← OBS|end|1
← OBS|done|2|returned|0x28|           (0x28 = 40 checks failed; the detail is above and in the file)
→ CMD|3|bye
← OBS|ack|3|bye
← OBS|done|3|ok||
```

## Client checklist

1. TCP client, operator-supplied `host:port` (default 9803), one connection at a time.
2. Line reader and writer: `\n`-terminated, 4096-byte cap, `CMD|` out and `OBS|` in.
3. Client-owned sequence numbers, strictly increasing from 1.
4. `hello` first, with the secret; parse `<session-id>` and capabilities; never send an
   un-announced verb.
5. A changed session id is a probe restart.
6. Ack-before-done: a lone `ack` and a closed socket is `died`; an elapsed budget is
   `timeout`; ambiguity is `lost`; none carries a value.
7. An operator form for machine identity, stamped onto every kept record.
8. The report stream between `report`'s `ack` and `done`, parsed incrementally, with a
   mid-stream death handled as `died`. Unknown record kinds are ignored.
9. The `sysinfo` block keyed on `state`, shown as self-report, never as graded machine
   identity.
10. Replay-from-transcript mode for CI.
11. `call`: `returned`, `bad-argument` or `died`. `read`: `bytes` records then `returned`, or
    `died` on a bad address. Every other verb only when its capability is announced, with
    refusal handled.
