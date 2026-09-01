# D160 - The flag sweep: measurable without a console, and the dump answered the wrong call


**The flag sweep: the second argument to `sceKernelDirectMemoryQuery` is measurable without
a console, and the existing dump was answering the wrong call.**

`sceKernelDirectMemoryQuery` is the most-called function in the corpus mined from real
titles - 87.6 million calls in one of them, about 99.9% of every call it makes. The guest
walks the memory map, refuses what it is shown, and walks it again. So the contents of its
destination buffer are the single most valuable thing a hardware run could bring back.

`130-layout/direct-memory-query` has dumped that buffer for some time. It queries with
`flags = 0`. **Every observed call from a real title passes `1`.** The dump this project has
been collecting therefore answers a different call from the one that matters, and nobody had
noticed because the argument was never the subject.

### Why a sweep answers it, and needs no oracle

Query one offset repeatedly with `flags` 0, 1, 2 and 4, dump all four buffers, and report
the offset of the first byte that differs from the baseline. **The difference is the answer.**
Nothing is compared against an expectation and no field is named, so this stays inside
`130-layout`'s design - bytes, not verdicts - and inside D008, because adding a value to an
`int` argument invents no structure and no arity.

It is a measurement of whichever implementation runs it. An emulator today says what that
emulator does, which is worth having on its own terms; the same check on a console later says
what the platform does. Neither is a guess.

Four values because a flag argument that means anything is usually a bit set, and two extra
single bits separate "bit field" from "enumerated selector" without guessing what any bit
means. Widening is a decision to make after these four have said something.

### Zero differences is a result

A sweep where no flag changes the buffer says the flag does not affect what this offset
reports, on this implementation. That is exactly as much of an answer as a difference, so it
passes with a count rather than failing. Every value refused is `partial` - a value the
platform rejects is a value that means something.

The guard check stops the sweep on the first overrun rather than continuing. Calling a
function three more times after it has written past its buffer is not a measurement; it is
repeating the damage.

### The superfluous guard, found on the way

Both query checks carried `OBS_REQUIRE(&sceKernelGetDirectMemorySize)` and **neither calls
it** - the symbol appeared in nothing but the guard.

D058 is about not jumping to a null weak symbol. A guard on a symbol the check never touches
is the inverse mistake and it is not harmless: it makes the check skip on a platform that has
`sceKernelDirectMemoryQuery` and happens not to export the other, and the skip says "the
symbol is not present" about a symbol that is present. Removed from both.

### Validated, which neither query check had been

Both skipped on the host build for want of a stub, so `130-layout/direct-memory-query` had
never run against a known-good implementation either. The host now writes a start and a
length in the first sixteen bytes, a byte at offset 16 that varies with the flag, and refuses
`flags = 4`.

That shape is **not** a model of what a console returns - this program does not know that,
and finding it out is the whole purpose of the section. It is chosen to drive every branch:
baseline capture, difference detection, the first-differing-byte report, and the refusal
count. The varying byte sits past both fields deliberately, so a differencing pass that
stopped after sixteen bytes would report "the flag changes nothing", which is a conclusion
rather than a silence.

