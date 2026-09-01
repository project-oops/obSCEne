# Two swapped integers


Reading an open-source loader's header found in ten minutes what black-box probing
could not have found at all: `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were the wrong way
round (D036). Every other one of the nineteen matched exactly.

The derivation could not have caught it. `JMPREL + PLTRELSZ == RELA` is commutative, so
an offset and a size sum to the same place whichever is which - the arithmetic that
fixed the rest of the table is structurally blind to this pair. `derive` now says so,
for every offset/size pair rather than just the two entry sizes.

**It had also produced a false conclusion.** `-fno-plt` leaves the linkage table empty,
and an empty table is not declared - so the two bad tags were simply absent from that
build. One emulator ran the module with the flag and failed without it; the other did
the reverse. That went into the record as two loaders wanting incompatible things.
They did not. One bug was hidden by a build flag, and the flag was holding up a
conclusion that was wrong.

**After the correction, with the ordinary PLT build:**

| | before | after |
|---|---|---|
| imports resolved | 806 | 928 |
| platform calls | 77 | 131 |
| guest faults | 0 | 0 |
| second emulator | refused at relocation | **executes the guest** |

The second emulator now reaches `--- Execute: Main ---`, having loaded, relocated and
resolved. `-fno-plt` is gone.

### What the loader source also gave

- Its `write(1, ...)` returns the byte count and discards the bytes. A channel that
  reports success and prints nothing is the one failure the selection cannot detect, so
  `puts` - which that emulator implements properly - is now tried first of the two
  (D037).
- It resolves an unresolved weak import to a valid allocated page rather than to null,
  which no guest can tell from a real function. The census control catches the
  resulting inflation, which is the argument for having had it.
- Its `sceKernelWrite(1, ...)` writes to the host's stdout, so a report from that
  emulator will arrive on the process's own output rather than its guest-printf file.

### On method

Deriving the tag table from a reference module was right, and produced nineteen values
from arithmetic that cannot be argued with. It also produced two wrong ones with exactly
the same confidence. Both halves belong in the record: derive what can be checked, and
read the thing that already knows when the check cannot distinguish two answers.

### It reports

An emulator now runs obSCEne end to end and the report comes out on its console
channel. 110 records, fifteen sections, real verdicts.

The last thing in the way was the symbol type. The linker leaves an undefined reference
as `STT_NOTYPE`; a console loader matches on the NID *and* the type, so a `NOTYPE`
import matches nothing and binds to a stub returning zero - including the write the
report goes through. Everything ran, everything got a plausible zero back, and nothing
was ever said. Rewriting imports to `STT_FUNC` (D039) fixed it.

**The earlier conclusion was wrong.** "Four output channels, all stubbed, no emulator
implements a write path" - no. The write path was there. We were asking for it in a way
the loader could not match, and its stub answered instead.

**It was nearly missed twice.** Typing the imports made the emulator log fewer
resolutions and end in a fault, which read as a regression, and it was reverted on that
basis. Fewer lines because it stopped resolving and started *running*. The fault is a
real defect the probe found and named.

### What the first real run says

    035-libc     5 pass, 1 partial, 12 fail, 2 skip
    037-math     2 pass, 0 partial,  5 fail, 0 skip
    040-file     2 pass

`sqrt(4)` is wrong, `fabs` is wrong, rounding a positive value is wrong, twelve string
and conversion checks fail. Those are findings about an emulator, produced by a probe,
which is the entire point of this repository.

And the run ends the way it was designed to:

    STREAM ENDED INSIDE A CALL: 040-file/open-rejects-null
    The report stopped between the attempt and its result, so this call most likely
    took the process down.

A `try` with no matching `res`, naming the exact call. Passing a null path to
`sceKernelOpen` throws inside the emulator. Announce-before-attempting earned its
keep - that check was the one thing in fifteen sections that could not report for
itself, and it is the one thing the report names.

### Versions, and where Kyty still stops

Identity values declared version zero; libraries are registered as version one and
modules as 1.1, packed differently - a library uses sixteen bits, a module splits them
into a major and a minor (D038). Corrected. No observed behaviour change in either
emulator, which is worth saying rather than implying otherwise.

**Correction: it binds properly.** The one-import control was extended to check the
*return value* rather than only the address - a stub and a real function are both
non-null, and only what comes back tells them apart. Under Kyty the write returns the
full byte count, so `sceKernelWrite` is bound to the real implementation and the bytes
were written.

They are not reachable. Kyty is a windowed application whose C `stdout` is not the
handle a parent process redirects, and the same is true of `stderr` - both writes
succeed and the output goes somewhere a script cannot read. Its own logger reaches a
redirected stdout; the guest's writes do not.

So obSCEne runs correctly on both emulators. One of them can be read.


### A third loader, and an independent check on the tag table

craziiEmu (and SharpEmu, which shares its lineage) is a C# PS5 emulator with a small,
readable loader. Its constants were compared against ours:

    DtSceJmpRel     = 0x61000029
    DtScePltRelSize = 0x6100002D

which is the corrected assignment from D036, arrived at independently. Two loaders now
agree that our original derivation had those two the wrong way round.

It also names `SceRelro = 0x61000010`, a segment type this project does not emit and has
not needed. Worth knowing it exists.

Its resolution is much simpler than the other two: it matches the whole encoded name
against per-module stub tables, with no symbol type and no version involved. That is a
useful third data point - the type matching that D039 turned on is a property of one
loader, not of the format, and a module that satisfies the strictest of them satisfies
the others.

