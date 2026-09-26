# Module format

What a module needs beyond an ordinary ELF for a loader to run it, and the evidence for each
requirement. The format itself (tag numbers, segment types, table layout) is owned by selfish:
`crates/selfish-elf/src/dynamic.rs` there holds the dynamic tags for both conventions.
`obscene-tool mkmodule` builds a module; `obscene-tool inspect` prints one;
`obscene-tool derive` re-derives the tag assignment from a finished module and fails if it
does not reproduce.

## Header and segment requirements

Each row is a hard rejection by a real loader, with the expected value named in the message.

| Requirement | Loader message when absent |
|---|---|
| `EI_OSABI` = `0x09` (FreeBSD); lld sets it, GNU ld does not | `IsElfFile: e_ident[EI_OSABI] expected 0x09 is (0x0)` |
| `e_type` ∈ {`0xFE00`, `0xFE10`, `0xFE18`} | `IsElfFile: e_type expected 0xFE10 OR 0xFE18 OR 0xfe00 is (0x3)` |
| Loadable segments page-aligned: `-z separate-loadable-segments`, lld only | `SplitRegion: Unreachable code!` |
| Vendor dynamic table | `LoadDynamicInfo: unsupported dynamic tag` for each standard tag, then `Symbol table not found!` |
| Library and module declaration tags | `Assertion Failed! Unable to find library and module` |

**`0xFE10` is the executable type and `0xFE18` is the shared-library type.** Kyty states the
pair: `ET_DYNEXEC = 0xfe10 // Executable file`, `ET_DYNAMIC = 0xfe18 // Shared`. A loader
that respects the distinction runs a `0xFE18` module's initialisers and never enters it.
obSCEne's main module is `0xFE10`. [BOOT.md](BOOT.md) covers the load sequence.

Every module also carries a `PT_SCE_PROC_PARAM` segment (`0x61000001`), declared by the linker
script.

## Dynamic table conventions

Two conventions exist, and they differ in layout as well as in tag numbers.

| | `orbis` (previous generation) | `prospero` (current generation) |
|---|---|---|
| string/symbol/hash/reloc tables live in | a `PT_SCE_DYNLIBDATA` segment | ordinary `LOAD` segments |
| a table tag's value is | an offset into that segment | a virtual address |
| tags for the standard tables | `DT_SCE_STRTAB`, `DT_SCE_SYMTAB`, … | `DT_STRTAB`, `DT_SYMTAB`, … |
| vendor tags | low range, for everything | high range, vendor concepts only |
| module and library tags | `0x6100000D`-`0x61000019` | `0x61000043`-`0x61000049` |

The two halves go together. `DT_SCE_STRTAB` holds an offset because there is a segment to be
an offset into; `DT_STRTAB` holds an address because the table is in the image. Renumbering
tags without moving the tables produces a module that is neither convention.

No retail current-generation module uses `PT_SCE_DYNLIBDATA`.

### Retail current-generation tags

Measured against retail current-generation dumps with prosper's `self_dump` (see
[EMULATORS.md](EMULATORS.md)). Standard tags carry the standard tables:

```text
DT_NULL(0)  DT_NEEDED(1)  DT_PLTRELSZ(2)  DT_PLTGOT(3)  DT_HASH(4)  DT_STRTAB(5)
DT_SYMTAB(6)  DT_RELA(7)  DT_RELASZ(8)  DT_RELAENT(9)  DT_STRSZ(0xa)  DT_SYMENT(0xb)
DT_PLTREL(0x14)  DT_JMPREL(0x17)
```

Vendor tags carry vendor concepts only:

| tag | name |
|---|---|
| `0x61000017` | `DT_SCE_EXPORT_LIB_ATTR` |
| `0x61000019` | `DT_SCE_IMPORT_LIB_ATTR` |
| `0x6100003d` | `DT_SCE_HASHSZ` |
| `0x6100003f` | `DT_SCE_SYMTABSZ` |
| `0x61000041` | `DT_SCE_ORIG_FILENAME` |
| `0x61000043` | `DT_SCE_MODULE_INFO` |
| `0x61000045` | `DT_SCE_NEEDED_MODULE` |
| `0x61000047` | `DT_SCE_MODULE_ATTR` |
| `0x61000049` | import-library records |

The conventions share the attribute tags (`0x17`, `0x19`) and `HASHSZ`/`SYMTABSZ`, and differ
on every base record.

### `0x61000049` is the import-library record

prosper's tag namer calls `0x61000049` `DT_SCE_EXPORT_LIB` while its parser files it under
import libraries. A main executable imports many libraries and exports at most itself, so the
counts decide it:

| module | `NEEDED_MODULE` 0x45 | tag 0x49 | imports reported | exports reported |
|---|---|---|---|---|
| PPSA02664 | 36 | **38** | **38** | 1 |
| PPSA03416 | 36 | **38** | **38** | 1 |
| PPSA04263 | 52 | **55** | **55** | 0 |
| PPSA21564 | 59 | **62** | **62** | 0 |
| PPSA25872 | 36 | **38** | **38** | 0 |
| PPSA28061 | 26 | **27** | **27** | 0 |

The `0x49` count equals the import count in every module, so `0x61000049` carries the
import-library records and the namer is wrong. The current-generation export-library tag is
not attested by any module examined; nothing is assumed about it. (D008)

### Previous-generation tag values

`obscene-tool probe` builds a module carrying candidate tags, and the loader names the ones it
recognises. It measures what a particular loader supports. The recognised values, swept one at
a time:

```
low group   0x61000009  0x6100000d  0x6100000f  0x61000011  0x61000013
            0x61000015  0x61000019
main block  0x61000025  0x61000027  0x61000029  0x6100002b  0x6100002d
            0x6100002f  0x61000031  0x61000033  0x61000035  0x61000037
            0x61000039  0x6100003b  0x6100003d  0x6100003f
plus        0x61000007
```

Not recognised: `0x61000000`-`0x61000006`, `0x61000008`, and everything else in the range.
The six documented values (`STRTAB` `0x35`, `STRSZ` `0x37`, `SYMTAB` `0x39`, `SYMENT` `0x3B`,
`HASHSZ` `0x3D`, `SYMTABSZ` `0x3F`) all fall inside the recognised set, which validates the
sweep. The loader names `0x61000007` as `DT_SCE_FINGERPRINT`.

Tag values are never inferred from the numbering pattern. A guessed tag produces a module
that loads and resolves nothing, and the failure looks like a broken loader rather than a bad
constant. (D008) Another project's source is not read to fill them (D018); published
documentation and written descriptions are.

### Table selection

`TABLE` follows the target generation in the Makefile rather than being chosen by hand:
`GEN=4` builds `orbis` and `GEN=5` builds `prospero`. Each convention is required by one
family of loaders:

- `prospero` is rejected by the previous-generation emulators: shadPS4 and fpPS4 report
  `unsupported dynamic tag 0x02` / `0x03` / `0x07` for the standard tags and `0x61000043` /
  `0x61000047` for the vendor ones.
- `orbis` is invisible to current-generation readers: prosper finds none of its imports.

Every emulator in the toolkit accepts `orbis`, including the current-generation ones, so an
emulator accepting it is not evidence about the hardware.

Under `prospero`, `mkmodule` computes a `table_base` at the end of the last `PT_LOAD`, rounded
up to the `0x4000` page. Every table address is written relative to it, the tables are
emitted as a real `PT_LOAD` at that address with `0x4000` alignment, and `derive` finds the
segment through the string table's address rather than by segment type. Under `orbis`,
`table_base` is 0.

A table tag resolved as a virtual address while the vendor segment and the first `LOAD` both
sit at `vaddr=0` lands on the ELF header. Symbol counts still come out right, because they are
computed from the declared size, so a plausible count is no evidence that the tables were read
from the right bytes.

## Symbol names

Names in the symbol table are NID-encoded, `<11 characters>#<library>#<module>`, not plain
`sceKernelWrite`. The library and module identifiers index the module's own import-library and
needed-module tables.

    NID = first 8 bytes of SHA-1(name || suffix), little-endian

then eleven characters of a 64-symbol alphabet with two padding bits. `obscene-tool nid`
computes it; the hash lives in selfish's `selfish-nid` crate.

The suffix, byte order, alphabet and bit packing are each plausible when wrong and each
produce ordinary-looking output that resolves to nothing. One published pair pins all four:
`sceKernelLoadStartModule` → `wzvqT4UqKX8`, checked by `obscene-tool selftest`.

## Diagnosing a module

- `obscene-tool inspect` prints each tag with its value and shape: offsets land inside
  `PT_SCE_DYNLIBDATA`, sizes do not, entry sizes are `0x18`, and `PLTREL` holds 7 (`RELA`).
- A module with no dynamic segment is not a valid control: every real module has one, and
  removing it produces a different, earlier failure than the one under study. A control is a
  valid instance of the thing being tested.
- A crash after a `ret` looks exactly like never having run. A body that spins distinguishes
  them: a hang means guest code executed.
