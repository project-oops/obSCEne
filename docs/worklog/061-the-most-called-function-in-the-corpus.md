# 2026-08-24 - the most-called function in the corpus, and we were asking it the wrong question


orbistoun's ranked list put `sceKernelDirectMemoryQuery` first: 87.6 million calls in one
title, ~99.9% of every call it makes, and the guest is the only judge of what belongs in the
destination buffer. Three questions attached, and answering them turned up something about
our own instrument.

**`130-layout/direct-memory-query` queries with `flags = 0`. Every observed call from a real
title passes `1`.** So the dump this project has been collecting for a hardware day answers a
different call from the one that matters. Nobody noticed because the argument was never the
subject - the check was about the *buffer*, and the buffer was faithfully dumped.

`130-layout/direct-memory-query-flags` sweeps 0, 1, 2 and 4 and reports the offset of the
first byte that differs from the baseline. The difference *is* the answer, so it needs no
oracle and no layout, and it is a measurement of whichever implementation runs it - an
emulator today, a console later. (D160)

### Neither query check had ever been validated

Both skipped on the host for want of a stub, so this would have shipped an instrument nobody
had calibrated - to answer a question the other side cannot answer at all. The host stub now
writes a start and a length, a byte at offset 16 that varies with the flag, and refuses
`flags = 4`. Every branch drove:

```
flags-0  code 0x0
flags-1  code 0x0    first-differing-byte 0x10
flags-2  code 0x0    first-differing-byte 0x10
flags-4  code 0xffffffffdeadbeee
```

The varying byte sits past both eight-byte fields deliberately. A differencing pass that
stopped after sixteen bytes would report "the flag changes nothing" - a conclusion rather
than a silence, and the mistake most worth catching here.

Found on the way: **both** query checks guarded `sceKernelGetDirectMemorySize` and neither
calls it. D058 is about not jumping to a null weak symbol; a guard on an uncalled symbol is
the inverse error and it is not harmless - it makes the check skip on a platform that has the
function, reporting "the symbol is not present" about a symbol that is present.

### The prober needed a budget, not more patience

Seventeen rounds reached index 92 of 32,466. A round ends at the first function that does not
return, so rounds needed is *number of functions that fault*, and against shadPS4 that is
about one in five - a full pass is near seventy rounds, not thirty.

`bulk-sweep.sh --resume` now continues an accumulation instead of restarting it, reading the
index out of the report rather than from the operator. Two things it got wrong first: the
index has to be taken **numerically**, because sorting hex text puts `0x9` after `0x10` and
would resume behind ground already covered; and `strtonum` is a gawk extension the build VM's
awk does not have. (D161)

**And a caution that goes in the header, not a footnote.** `scePthreadExit` appears in the
fault list every time, at index 81 - correctly, because it does not return by design. A
function blocking on a null argument does not return either. Neither does one that crashes.
The announcement says only that no answer came back; which of the three it was needs the
function's own contract. The list is input to a judgement, not the judgement.

### Sent to orbistoun

Their three unnameable hashes: **0 of 3**, against 166,956 candidates, with the generator
measured at **198 of 200** known pairs first so the miss could be weighed. Also absent from
all 1,130,757 unnamed identifiers mined from firmware. The reason is structural and closes
the question rather than deferring it: our firmware corpus is **1.05 through 9.00**, entirely
previous-generation, and they are chasing current-generation modules. No amount of searching
harder changes that; a current-generation module dump would.

Declined their proposed `measured-elsewhere` grade. The origin field already carries
*which* elsewhere as operator-asserted data, and an enum member would keep the "elsewhere"
while discarding the identifying half. Also corrected a belief worth correcting: they think
we share a four-value vocabulary and we do not - ours is the five-rung ladder. Two gradings
that were never identical cannot drift apart; they can only be mistaken for each other, which
is worse, because it is invisible.

