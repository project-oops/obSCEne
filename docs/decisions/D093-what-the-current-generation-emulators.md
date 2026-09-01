# D093 - What the current-generation emulators implement that obSCEne does not touch


Status: analysed, and one gap closed.

`scripts/gap-analysis.py` surveys every emulator, and its answer is dominated by the
previous generation - seven of nine target it. `scripts/ps5-gap.py` asks the narrower
question that matters for a probe whose goal is to flex a *console*: what do the
current-generation emulators implement, and what does obSCEne do about each?

| | |
|---|---|
| implemented by current-generation emulators | **1,177** |
| obSCEne calls | 88 |
| obSCEne censuses but never calls | 248 |
| **obSCEne does not know at all** | **841** |

**The gaps are whole subsystems, not scattered functions**, which is the useful shape:
`libKernel` 192, `libSceJson` 50, `libSceAudioPropagation` 39, `libSceSaveData` 36,
`libSceRtc` 34, `libSceAgc` 31 beyond the 87 already censused.

The 192 in `libKernel` matter most for a kernel-focused probe, and they are subsystems:
**asynchronous I/O** (`sceKernelAio*`), **asset path resolution** (`sceKernelApr*`),
**AMPR events**, **virtual range naming**, **flexible memory**, time conversion.

**Most are not actionable and the reason is D008.** The OpenOrbis headers carry no
signatures for the AIO or APR families, and `sceKernelConvertLocaltimeToUtc` is declared
there with no arguments and a void return - which is the toolchain saying it does not know
either. A gap that cannot be called safely stays a gap, and the census is where it belongs.

### Flexible memory was actionable, and is now covered

Full signatures, implemented by current-generation emulators, and **an entire allocation
path obSCEne had no coverage of**. Direct memory - which `020-memory` already exercises -
is physical memory reserved by offset and mapped explicitly. Flexible memory is the other
kind: the system finds the pages and the caller asks for a size rather than a location.

A title uses both. An emulator implementing one and not the other passed every memory check
in this suite.

Two checks: how much the system will lend, and a map / write / read-back / release round
trip. The read-back is what makes it positive rather than negative - an implementation
returning a plausible address it has not actually mapped fails it and passes anything that
only reads the return code.

Under shadPS4 both pass: 448 MB available, mapped at `0x2007ec000`, and the write held.

