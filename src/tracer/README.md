# tracer/ - the recording core and its decoder

The isolated, buildable heart of the call/response tracer described in
[`../../docs/TRACER.md`](../../docs/TRACER.md). This subtree touches nothing in the probe: it is new
code with its own Makefile, so it builds and tests on its own long before any on-hardware
injection exists.

It delivers the two things the tracer needs before anything else:

1. **The recording core** (`trace_encode.h`) - what runs *inside* a traced process. Freestanding
   and allocation-free, it samples per-nid so a hot function cannot drown the device, and appends
   fixed 64-byte records to a caller-provided buffer.
2. **Parseable results** (`trace_decode.c`) - off-hardware, it turns a raw trace into the same
   `OBS|` records the corpus already reads (`../../docs/OUTPUT.md`), so a decoded trace and a probe
   report land in one comparable corpus.

## Files

| file | role | runs where |
|---|---|---|
| `trace_format.h` | the wire format - fixed 64-byte records, 32-byte header, versioned | both ends |
| `trace_encode.h` | append buffer + per-nid sampler + record emitters | on-hardware (freestanding) |
| `trace_decode.c` | raw stream → `OBS|` lines | host |
| `trace_selftest.c` | drives a synthetic run, asserts the format and the safety policies | host |
| `Makefile` | `make check` builds both and runs the proof | host |

## Build and prove it

```bash
make -C tracer check
```

This compiles the encoder and decoder under the probe's strict flags (`-Wall -Wextra -Werror
-Wconversion` …), runs a synthetic 1000-call run through the encoder, and checks that:

- a hot function is **capped** at `OBS_TRACE_CAP` detailed records while its **count still
  travels** (a `COUNT` record carries the true 1000) - the mechanism that keeps the payload from
  overwhelming real hardware;
- an oversized out-parameter is recorded as **(length, hash)**, and its bytes appear nowhere in
  the stream - the provenance rule made mechanical;
- a small out-parameter and all call arguments **round-trip exactly** through a written file;
- the on-disk decoder turns the same stream into `OBS|` records with no error.

## The record kinds

| decoded line | from | carries |
|---|---|---|
| `OBS\|call\|?\|<nid>\|<tid>\|<seq>\|<args…>` | a call happening | arity + argument values |
| `OBS\|ret\|<nid>\|<seq>\|<value>` | a call returning | return value |
| `OBS\|outbuf\|<nid>\|<addr>\|<len>\|<hex or hash=…>` | out-parameter bytes | small inline, large hashed |
| `OBS\|name\|<nid>\|<name>` | a name the payload resolved | optional nid→name hint |
| `OBS\|count\|<nid>\|<total>` | drain | true call total for a capped nid |

The library column on a `call` is `?`: a trace carries NID hashes, and resolving them to
`library` / `symbol` is a join against the corpus done off-hardware. The decoder never invents a
name it was not given.

## What this is not, yet

The **injection and hooking** - getting this core running inside a retail game and pointing its
emitters at the game's real calls - is the on-hardware half, and it is not here. It is staged in
`../../docs/TRACER.md` §6 and gated on the five-minute go/no-go test (can a Payload-Manager payload
`ptrace`-attach a running game). Everything in this directory is the part that needs no hardware
and no injection, built and proven first so that when the go/no-go passes there is a known-good
pipeline to point at a real title.

## Provenance

The format and encoder were written from the design in `../../docs/TRACER.md`, which was informed by
*reading* open-source scene tools to understand the technique - not by copying them. No code here
is derived from any vendor material. The `(length, hash)` redaction is enforced in the encoder so
a trace is publishable by construction: it never carries a copyrighted title's buffer contents.
