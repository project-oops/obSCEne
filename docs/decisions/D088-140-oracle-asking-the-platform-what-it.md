# D088 - `140-oracle`: asking the platform what it knows, and recording what it returns


Status: decided, from `docs/HARDWARE-PROBE.md` items 3 and 4.

Two checks, neither of which tests anything. Both record answers.

### Resolution by name, "the one that changes the most"

An identifier is a truncated hash and hashing is one way, so recovering a name means
guessing - generate candidates, hash them, hope for a match. It works, it costs on the
order of a quarter of a million candidates per confirmed name, and it can only reach names
something happens to import.

If the platform resolves symbols **by name**, that collapses to one question per candidate
and reaches functions no title imports at all.

**The control pair is the whole design.** A name that must resolve and a name that cannot
exist. A platform answering yes to both is not an oracle, and its yes for every other name
means nothing - which is the more dangerous failure, because it looks like success. The
same reasoning as the census control, for the same reason: an instrument that cannot be
shown to work is not evidence.

Under both the host and shadPS4: nothing resolves, including a symbol known to be present.
Reported as a skip rather than a failure - a platform is not obliged to have a by-name
interface, and saying so honestly is the useful answer. **The question is now asked, and a
console will answer it.**

### Real error codes

Every negative check in this suite asserts that a bad argument is refused and throws the
returned code away. Correct on an emulator, where the code is invented. On hardware the
code *is* the finding: an implementation returning a value no caller recognises makes
guests retry forever.

So `OBS|err` records what came back, with no expectation at all. shadPS4 immediately
produced a legible scheme:

| call | returned | |
|---|---|---|
| `sceKernelClose(-1)` | `0x80020009` | 9, `EBADF` |
| `sceKernelOpen(missing)` | `0x80020002` | 2, `ENOENT` |
| `sceKernelDeleteEventFlag(NULL)` | `0x80020003` | 3, `ESRCH` |

`0x8002_0000 | errno`. That is one run of a section that asserts nothing, and it is the
table another project can use to replace invented placeholder codes with values a guest
recognises.

**Neither check can fail.** The oracle skips when the platform has no by-name interface;
the error table passes whenever it recorded anything. Verdicts here would be noise - the
records are the output, and a green line in a tally for a measurement would be one more
thing to add up and one more reason to stop reading.

