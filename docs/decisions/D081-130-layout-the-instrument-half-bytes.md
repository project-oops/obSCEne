# D081 - `130-layout`: the instrument half. Bytes recorded, nothing interpreted


Status: decided, and it is what `docs/HARDWARE-PROBE.md` asked for.

That document argues obSCEne inverts on a console: "did this behave correctly?" is
uninteresting because the answer is yes, and what cannot be got any other way is *what the
platform actually wrote*. Its first request is a hexdump of a filled buffer, with the
fields deliberately unnamed.

The program could not do it. There was one byte-dumping path, hardcoded into `110-modules`
for a single structure. `OBS|bytes` is now general, and `130-layout` uses it.

**It routes around D008 without weakening it.** D008 forbids calling a function whose
structure layout is unknown, because a wrong layout produces a call that succeeds and does
the wrong thing. That is a rule about *expectations* - about naming fields and reading them
at offsets nobody confirmed. Dumping needs no layout: it needs the arity, which is read
from a published header, and a buffer larger than anything the call could write. So the
surface BACKLOG §2 records as blocked is now partly reachable, and the rule that blocked it
is untouched.

**The buffer is oversized and guarded.** 256 bytes with a 64-byte pattern after it, checked
on return. A call writing past its buffer is the one fault this design could cause and not
notice, so it is reported rather than trusted.

**Verdicts here are weak on purpose.** A pass means "it returned and wrote something". The
value is in the `bytes` records, and inflating the verdict would put a green line in a
tally for what is really a measurement.

### Validated where the answer is known, deliberately

A dump is the one kind of record whose correctness cannot be judged from the data - the
whole point is that nobody knows what the bytes should be. A wrong extent, a swapped
nibble, a chunk boundary off by one would all produce a plausible hexdump of the wrong
thing, and on hardware that would be permanent and undetectable.

So the host writes a pattern chosen to catch exactly those: a recognisable ASCII run, a
byte that is not its own nibble-swap, an interior zero the extent must see past, and a run
crossing the sixteen-byte record boundary. All four came out right.

### The first emulator run read a firmware version out of an opaque structure

`sceKernelGetSystemSwVersion` wrote 40 bytes. At offset 8, ASCII `13.520.001`; at offset
36, `0x13520001` little-endian. Most of that layout is legible from one dump, and no field
was named to get it.

`sceKernelDirectMemoryQuery` wrote 17 bytes where the emulator side had spent four
experiments establishing 24. That number is shadPS4's, not hardware's - which is exactly
why the record carries bytes and not a conclusion.

