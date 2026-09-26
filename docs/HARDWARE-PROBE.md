# obSCEne on real hardware

On an emulator obSCEne asks whether a target behaves correctly and answers pass, partial, fail
or skip. On real hardware that question is settled, and the instrument runs the other way: it
measures what a correct system does and records it. A `res|pass` record is worth nothing from
hardware; the bytes a function wrote are worth an afternoon. The measured answers are in
[`HARDWARE.md`](HARDWARE.md); this page is the reference for what hardware measurement adds and
how each measurement is shaped.

## Measurement records

Hardware output is a measurement, not a verdict, so it needs record types that carry values
rather than judgements. [`docs/OUTPUT.md`](OUTPUT.md) is the contract; the measurement shapes are:

| Record | Fields | For |
|---|---|---|
| `dump` | check id, label, address, length, hex | structure layouts, buffers |
| `region` | index, start, end, type, flags | one line per memory region |
| `err` | library, symbol, argument description, returned value | the error-code table |
| `resolve` | library, symbol, `present` / `absent`, address | the name oracle |
| `size` | library, symbol, argument index, accepted, rejected | binary-searched structure sizes |

Two disciplines hold across all of them:

- **Hex, not interpretation.** `start=0, end=0x200000000` has already decided the layout;
  `00 00 ...` has not, and is re-read later when someone knows better.
- **A record per fact, not per narrative.** Both sides diff these, and prose does not diff.

## What hardware measures that a census cannot

Each area maps to a probe section and to the records it emits.

### Structure layouts

Call a function and hexdump the buffer it filled, without parsing or naming the fields.
`obs_report_buffer` does this at four sites; `130-layout` reports the bytes of a call even when
the call refuses. `130-layout/query-size-ladder` varies the size argument to find where a call
starts refusing, which yields exact structure sizes and produces `derived` rather than `assumed`
provenance. (D081)

### The memory map

Walk the direct-memory enumeration to completion and print every region - start, end, type,
flags. `150-memory-map/walk` captures the map at process start and `/after-allocation` captures
it after a few allocations, so a change of shape is visible and not only the initial state.

### Error codes

Call with deliberately wrong arguments - null pointers, zero lengths, offsets past the end,
undersized structures - and record what comes back. `obs_report_error_code` does this at six
sites. Distinguishing kinds of wrong argument (null pointer, bad size, bad handle, bad flags,
misaligned pointer) turns an error-code list into a decision table an implementer acts on: a code
tied to a bad handle specifically tells an emulator which stub returns what, so a guest that
recognises the failure stops retrying.

### Resolution by name

If the system resolves a symbol by name at runtime, obSCEne becomes a name oracle: one question
per name, yes or no, for functions no title happens to import. `140-oracle/resolve-by-name`
resolves against a candidate list and reports which resolve, and it runs the moment something
resolves by name. A working `sceKernelDlsym` turns `data/mined-names.txt` and
`data/unnamed-nids.txt` into a lookup instead of a search, and if resolution returns an address,
two names at one address are aliases and the address ordering exposes the export layout.

### A command buffer

`165-gnm` calls the command builders and dumps their bytes, so the PM4 command buffer is
recorded. Submitting it and dumping the pages its shader addresses reference is the hardware
dispatch handled by `166-agc` and `oops-sdk`; see [`GNM.md`](GNM.md).

### Enumerated argument spaces

The size-ladder mechanism applies to any small argument. Sweeping the `flags` argument to
`sceKernelDirectMemoryQuery`, the protection bits on `sceKernelMapDirectMemory`, and the memory
types on `sceKernelAllocateDirectMemory` derives the valid set rather than inventing it, so an
emulator rejects what the hardware rejects. (D008)

### Differential dumps

Calling one function twice with different inputs and diffing the two buffers says which field
carries which input. `sceKernelDirectMemoryQuery(offset=0)` against `(offset=0x10000000)`: the
bytes that differ are where the start address lives, the bytes that stay constant are reserved.
`obs_report_written` already runs the poison-versus-after comparison this needs.

### The platform manifest

`sceKernelGetModuleList` deals only in handles; `sceKernelGetModuleInfo` fills a structure whose
layout the size ladder establishes. Together they give the module inventory of a real machine -
names, handles, base addresses, segment sizes - which is metadata about the environment.

## The provenance boundary

Hardware output is our binary running our commands and observing our own experiment. In
orbistoun's terms that is `observed`, the strongest category, and unlike a baseline taken from
another emulator it cannot be wrong in the way that one can.

Metadata about the environment is in bounds: module names, handles, sizes, addresses, which
symbols resolve, what a call returns. The contents of vendor binaries are not. Enumerating that a
module is loaded at some address with some size is an observation about the machine; dumping or
disassembling its text is reading a vendor binary, which principle 6 forbids. What a correct
system does travels freely - sizes, codes, layouts, behaviour. How another implementation is
written does not, because arriving by way of obSCEne would not make it ours.

`HARDWARE=1` excludes `BULK`. The blind prober calls unnamed NIDs to see what happens, which is
reasonable against an emulator and not against hardware someone owns.
