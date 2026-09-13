# D331 - The `*GetSize` family is measured against its own builder, not against a borrowed number

**decided** - 2026-09-12

`libSceAgc` exposes a `*GetSize` sibling for most command builders. The guest calls it,
reserves that many bytes, and the builder then writes into the reservation - so the two are
one ABI contract, and a wrong size is not cosmetic: it is either a builder writing past
what the guest set aside, or a guest reserving room nobody uses.

## Why reading the implementations does not settle it

Two independent current-generation reimplementations were consulted and they **disagree**
on several of these counts. One of them documents why: its payloads are its own encoding
rather than the hardware packet, so its numbers carry a build snapshot here and a
predication slot there. Its own guidance is not to shrink a builder to match a published
table, because that is how a working title gets broken to satisfy a number.

So there is no consensus to take, and taking either side would mean writing an expectation
this project cannot defend. Worse, it would be the failure principle 3 already names one
level down: an instrument fitted to the answer it was built to confirm. A check that
asserts a borrowed figure and passes has established nothing except that the figure was
copied correctly.

## What is knowable without either number

The **relationship**. A reservation that does not cover the write is a fault whoever turns
out to be right about the size. That is this project's own reasoning, it is falsifiable,
and it needs no prior figure.

So `166-agc/*-getsize` records both numbers and judges only the invariant:

| record | what it is |
|---|---|
| `getsize` | what the library says it will consume, in bytes |
| `getsize-high32` | the top half of the return, because the return *width* is not established either. Non-zero means this is not a 32-bit byte count and every dword figure derived from it is wrong |
| `getsize-dwords` | the byte count divided by four, emitted only when it divides |
| `bytes-advanced`, `bytes-written` | what the builder then did |

The only **fail** is `getsize < bytes-advanced`.

`sceAgcCbNopGetSize` is swept over five argument values rather than read once, because a
single reading cannot tell a constant from a function of its argument - and which it is
decides whether a translator may cache the answer. The sweep records the curve and asserts
no shape for it.

## Why these checks stay `assumed` while their output is the point

The invariant is this project's reasoning, so `OBS_FROM_ASSUMED` describes it accurately.
The **measurements** are what matter: run on hardware they are the first figures in this
space taken from the real library rather than from an emulator measuring its own emitter,
and they settle a live disagreement between two implementations by authority rather than by
vote.

This is the shape the collection should use for every borrowed answer. A value read
somewhere else is a *hypothesis*, and the honest place to put it is a check that can refute
it - not an expectation that launders it into a fact.

## The four chosen, and why only four

The pilot is the intersection of two conditions already satisfied in this tree: the builder
is prototyped and callable, and the `*GetSize` identifier is attested in
`data/mined-names.txt` and recorded `present` by the census.

| builder | sibling | identifier |
|---|---|---|
| `sceAgcCbNop` | `sceAgcCbNopGetSize` | `0xb7b3e567d9ede4b7` |
| `sceAgcDcbDmaData` | `sceAgcDcbDmaDataGetSize` | `0xd9c709cfd2d023ec` |
| `sceAgcDcbSetIndexCount` | `sceAgcDcbSetIndexCountGetSize` | `0x9a58f3b860d9450e` |
| `sceAgcDcbSetUcRegisterDirect` | `sceAgcDcbSetUcRegisterDirectGetSize` | `0x68fd4a8bd1b7fbee` |

Identifiers were derived from the names with `obscene-tool nid` and then checked against the
mined corpus, which agrees on all four from five independent sources. Nothing here rests on
another project's hash.

`sceAgcDcbSetUcRegisterDirect` was prototyped in `platform.h` and missing from the curated
census. That was an omission rather than a judgement - it is attested by five sources and
recorded present - and this change closes it.

## Declared with six register arguments, deliberately

The arity of these functions is not established. System V AMD64 makes a call with six
register arguments safe for any arity up to six, which is the idiom the builders beside them
already use. A narrower declaration would assert an argument count nobody here has
measured.
