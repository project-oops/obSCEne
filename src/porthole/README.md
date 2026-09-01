# porthole/ - the Porthole payload

The target-side half of **Porthole**, the DIY remote-play stand-in: our own video stream out and
our own controller state in, over our own payload, so watching and playing a jailbroken target
never speaks the vendor's remote-play protocol. The design is
[`../../../prosperous/docs/VIDEO.md`](../../../prosperous/docs/VIDEO.md) "Part three: Porthole"; the host
client half is `pros-video` in prosperous. This subtree is the payload, isolated with its own
Makefile the same way `../tracer/` is, so it builds and tests before any of the hard parts exist.

## What is here, and what is deliberately not

**Scaffold.** The bytes on the wire are decided and real; the three things the payload actually
does are stubs.

| file | role |
|---|---|
| `porthole.h` | the wire contract - two ports, the 24-byte `PPAD` input record, status codes |
| `porthole.c` | the payload flow, with the encoder/capture/inject work stubbed and gated |
| `porthole_selftest.c` | host test that the record layout and its decoder agree |
| `Makefile` | `check` (host wire-contract test) and `skeleton` (freestanding compile of the payload) |

Two sockets, two directions, no negotiation:

```
9805  --->  encoded video out (raw Annex-B H.264, start-code delimited, no container of ours)
9806  <---  controller state in (a fixed 24-byte PPAD record, absolute state, up to 60/s)
```

The input record's shape is fixed here; the meaning of its button bits and stick range is the
target's own pad structure, **Ghostpad's**, confirmed against real hardware - credited in
obSCEne's `ACKNOWLEDGEMENTS.md` when the real code lands.

## The one question everything rests on

**Can an unsigned payload reach the hardware encoder?** `porthole_encoder_open()` returns
`PORTHOLE_NO_ENCODER` until that is answered - and it is not answered by guessing here. It is
answered by the **obSCEne probes** (`src/probe/sections/record.c` and the encoder-reachability section):
call it, record what came back, grade it by what it ran on. If the door opens, Porthole is the few
hundred lines VIDEO.md promises. If it does not, Porthole is not built and the answer is the raw
frame-grabber (VIDEO.md part two).

## Build

```bash
make -C porthole check      # host: the wire contract holds
make -C porthole skeleton   # target: the payload compiles freestanding (object only, not linked)
```
