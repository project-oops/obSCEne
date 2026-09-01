# Making a module a real loader will run

What the hardware loader requires beyond an ordinary ELF, and how much of it is
established. Written down because most of it was learned by running a loader and
reading its complaints, and that evidence is worth more than the conclusions drawn
from it.

## Established by running a real loader

Each of these was a hard rejection, with the expected value named in the message.

| Requirement | Evidence | Status |
|---|---|---|
| `EI_OSABI` = `0x09` (FreeBSD) | `IsElfFile: e_ident[EI_OSABI] expected 0x09 is (0x0)` | done - lld sets it; GNU ld does not |
| `e_type` ∈ {`0xFE00`, `0xFE10`, `0xFE18`} | `IsElfFile: e_type expected 0xFE10 OR 0xFE18 OR 0xfe00 is (0x3)` | done - rewritten to `0xFE10`, the **executable** type (D175) |
| Loadable segments page-aligned | `SplitRegion: Unreachable code!` | done - `-z separate-loadable-segments`, lld only |
| Vendor dynamic table | `LoadDynamicInfo: unsupported dynamic tag` ×14, then `Symbol table not found!` | **done** - `PT_SCE_DYNLIBDATA` with 5 tags |
| Library and module declarations | `Assertion Failed! Unable to find library and module` | **next** |

**`0xFE10` is the executable type and `0xFE18` is the shared-library type.** This paragraph
said the opposite for months - "`0xFE18` is the dynamic-executable type" - and `mkmodule`
wrote `0xFE18` to match, because the constants behind it were named backwards. Kyty states
the pair plainly: `ET_DYNEXEC = 0xfe10 // Executable file`, `ET_DYNAMIC = 0xfe18 // Shared`.

A loader that respects the distinction runs a `0xFE18` module's initialisers and then looks
elsewhere for a process to start, so the module loads, relocates, and is never entered. Three
of the four loaders here do not distinguish, which is why it survived so long. See D175 and
`docs/BOOT.md`.

## The dynamic table

A real loader ignores every standard tag. Observed, in order:

```
0x02 PLTRELSZ   0x03 PLTGOT    0x04 HASH      0x05 STRTAB    0x06 SYMTAB
0x07 RELA       0x08 RELASZ    0x09 RELAENT   0x0a STRSZ     0x0b SYMENT
0x14 PLTREL     0x17 JMPREL    0x6ffffef5 GNU_HASH          0x6ffffff9 RELACOUNT
```

It wants the vendor equivalents instead, carried in a `PT_SCE_DYNLIBDATA` segment
(`p_type` `0x61000000`) rather than in the ordinary dynamic segment.

### Tag values confirmed from documentation

| Tag | Value | Meaning |
|---|---|---|
| `DT_SCE_STRTAB` | `0x61000035` | offset of the string table |
| `DT_SCE_STRSZ` | `0x61000037` | string table size |
| `DT_SCE_SYMTAB` | `0x61000039` | offset of the symbol table |
| `DT_SCE_SYMENT` | `0x6100003B` | symbol entry size |
| `DT_SCE_HASHSZ` | `0x6100003D` | hash table size |
| `DT_SCE_SYMTABSZ` | `0x6100003F` | symbol table size |

### Confirmed by probing a loader

`obscene-tool probe` builds a module carrying candidate tags; the loader names the
ones it recognises. Re-runnable, and it says what a *particular* loader supports rather
than what the format defines - which is the more useful question when the goal is
running on that loader.

**21 vendor tag values are recognised**, swept one at a time:

```
low group   0x61000009  0x6100000d  0x6100000f  0x61000011  0x61000013
            0x61000015  0x61000019
main block  0x61000025  0x61000027  0x61000029  0x6100002b  0x6100002d
            0x6100002f  0x61000031  0x61000033  0x61000035  0x61000037
            0x61000039  0x6100003b  0x6100003d  0x6100003f
plus        0x61000007
```

Not recognised: `0x61000000`-`0x61000006`, `0x61000008`, and everything else in the
range.

**The sweep validates itself.** All six documented values sit inside the recognised
set, which they would not if the probe were measuring something other than what it
claims.

| Tag | Value | How |
|---|---|---|
| `DT_SCE_FINGERPRINT` | `0x61000007` | named by the loader, between two it rejected |
| `DT_SCE_STRTAB` | `0x61000035` | documented, confirmed recognised |
| `DT_SCE_STRSZ` | `0x61000037` | documented, confirmed recognised |
| `DT_SCE_SYMTAB` | `0x61000039` | documented, confirmed recognised |
| `DT_SCE_SYMENT` | `0x6100003B` | documented, confirmed recognised |
| `DT_SCE_HASHSZ` | `0x6100003D` | documented, confirmed recognised |
| `DT_SCE_SYMTABSZ` | `0x6100003F` | documented, confirmed recognised |

### Two independent loaders agree, which narrows it sharply

The same module was put in front of a second, unrelated emulator - a current-generation
one, so a different codebase and a different target. It behaves the same way:

| | shadPS4 (previous gen) | KytyPS5 (current gen) |
|---|---|---|
| Accepts the file | yes | yes |
| Maps every segment | yes, at `0x80000000` | yes, at `0x900000000` |
| Reports the entry point | `0x80000080` | `0x9000000b0` |
| **Executes it** | **no** | **no** |
| How it ends | fault at a fixed address | silent exit, no error |

Two loaders written by different people, for different hardware, both stopping at
exactly the transfer of control. That is not a quirk of one emulator - the module is
missing something both require.

**The strongest remaining hypothesis is the entry convention.** A module for the hardware exports
`module_start` and a loader calls *that*, rather than jumping to the ELF entry point.
Both emulators printing an entry address and then declining to use it is exactly what
that would look like. Testable by exporting the symbol and watching whether either one
calls it.

Recorded because the alternative was to keep iterating against one emulator and treat
its behaviour as the specification. A second implementation costs one download and turns
"this emulator does not like our module" into "our module is wrong".

### Ruled out

Kept because a dead hypothesis is worth as much as a live one, and re-testing these
would cost the same as testing them did.

| Hypothesis | How it died |
|---|---|
| Import resolution / dynlibdata | A module with **no imports and no dynamic segment** fails identically |
| `PT_SCE_PROC_PARAM` missing | Added it; same fault, same address |
| Wrong `e_type` | Tried all three accepted values (`0xFE00`, `0xFE10`, `0xFE18`); all fail the same way |
| Our `ret` ran and faulted on return | A spinning body exits in three seconds instead of hanging |

The pattern in those is worth naming: each was a plausible single missing *thing*, and
the format is turning out not to have one. `create-eboot` is documented as performing
"the necessary patches and relinking to create an Orbis ELF" - **patches**, plural,
before any signing. We have been reproducing that stage by inference, one field at a
time, and inference has now been wrong four times in a row.

The remedy is a known-good binary in this format to diff against. The reference payload
obtained earlier is the wrong shape - it is a plain ELF for a homebrew loader, which is
why every emulator rejects it - so it answers nothing about this.

### The blocker is upstream of import resolution

A **control module** - no imports, no dynamic segment at all, one executable page -
settles this. It loads, maps, and the loader reports its entry point:

```
LoadModuleToMemory: program entry addr ......: 0x0000000080000080
SignalHandler: Unhandled Exception code 0xc0000005 at 0x700000f3546b
```

The fault address is **identical** whether the body is `ret` or an infinite loop, and a
spinning module exits in three seconds rather than hanging. **Guest code is therefore
not executing at all** - the fault is at a fixed location in the emulator, reached
before control ever transfers.

**That conclusion was wrong, and the control was the reason.** A module with *no dynamic
segment at all* is not a stripped-down version of a real one - it is very likely an
invalid input for this format, since every real module has one. So its failure says
nothing about what a valid module is missing.

The evidence for that: the two failures are **not the same**.

```
full module     module.cpp:535 Assertion Failed! Unable to find library and module
minimal module  Unhandled Exception 0xc0000005 - no assertion, no message
```

The full module reaches symbol processing and complains about something specific. The
minimal one dies earlier and less informatively. **The full module is the one that is
closer**, and treating the minimal one as a control inverted that - it looked like a
simpler case failing worse, which reads as "the problem is more basic", when it is
actually "this input is malformed in a different way".

A control has to be a *valid* instance of the thing being tested. Removing the dynamic
segment did not simplify the module, it broke it. Real modules carry a `PT_SCE_PROC_PARAM` segment
(`0x61000001`) that neither of ours had.

**Tested, and it is not the cause.** A minimal module with the segment and one without
it crash at the same address:

```
WITH    proc_param : Unhandled Exception 0xc0000005 at 0x700000f3546b
WITHOUT proc_param : Unhandled Exception 0xc0000005 at 0x700000f3546b
```

The segment is now emitted anyway - a real module has one, and the linker script that
declares it replaced a pile of post-processing regardless - but it answered nothing.
The fault is elsewhere, at a fixed address in the emulator, reached after the module is
mapped and its entry reported.

**Worth recording how this was nearly missed.** Before the control existed, the full
module's crash was read as "our `ret` executed and popped a garbage address" - plausible,
consistent with the evidence, and wrong. Only replacing the body with a loop, where
*hanging* is the unambiguous signal of execution, distinguished the two. A crash after a
`ret` looks exactly like never having run.

### The library and module tables are a later gate

With a vendor segment carrying `STRTAB`, `STRSZ`, `SYMTAB`, `SYMTABSZ` and `SYMENT`,
and 401 symbols re-encoded to NID form, the loader gets considerably further: it accepts
the module, maps every segment, reads the entry point, **finds the symbol table**, and
then fails with

```
module.cpp:535 lambda: Assertion Failed!
Unable to find library and module
```

An encoded symbol name is `<nid>#<library>#<module>`. Those identifiers index tables the
module is supposed to declare - the wiki names `DT_SCE_IMPORT_LIB` and
`DT_SCE_MODULE_INFO` without values - and with no such table, library 0 refers to
nothing.

That makes the next experiment concrete rather than speculative: the seven unassigned
tags in the low group (`0x61000009` `0d` `0f` `11` `13` `15` `19`) sit apart from the
table block at `0x25`-`0x3f`, which is consistent with them being the declaration tags.
Setting each in turn and watching whether this specific message changes is a
**differential probe** - the same method that assigned `DT_SCE_FINGERPRINT`, aimed at a
named failure instead of at silence.

### Eight values recognised but unassigned

`0x61000025` `0x27` `0x29` `0x2b` `0x2d` `0x2f` `0x31` `0x33` - sitting immediately
below the six documented ones.

The wiki names exactly eight tags without values (`RELA`, `RELASZ`, `RELAENT`,
`JMPREL`, `PLTREL`, `PLTRELSZ`, `PLTGOT`, `HASH`), and eight slots is a suggestive
coincidence. **It is not evidence, and the mapping is not assumed.** The documented six
are not in the wiki's listed order, so numeric order does not follow name order, and
there is no basis to pair them up.

Raising the loader's log filter to trace adds nothing: it names `FINGERPRINT` and logs
nothing for the others.

**What would assign them:** a real module. `obscene-tool inspect` reads one and prints
each tag with its value and shape - offsets land inside `PT_SCE_DYNLIBDATA`, sizes do
not, entry sizes are exactly `0x18`, and `DT_SCE_PLTREL` holds 7. Pairing an offset with
the size after it reconstructs the layout, anchored by the six known tags.

### Not yet established

The relocation tags - the vendor equivalents of `RELA`, `RELASZ`, `RELAENT`,
`JMPREL`, `PLTRELSZ`, `PLTREL`, `PLTGOT` - and `DT_SCE_HASH`.

The confirmed values are odd and step by two, which suggests a contiguous block below
`0x61000035`. **That pattern is not a source.** Guessing a tag value produces a module
that loads and resolves nothing, and the failure would look like a broken loader rather
than a bad constant, so they stay unfilled until documented (D008).

The obvious place to read them is another project's source, which this repository does
not do (D018). Published documentation, a specification, or a written description are
all fine.

## Symbol names

The table above is only half of it: the names in that symbol table must be
NID-encoded - `<11 characters>#<library>#<module>` - not plain `sceKernelWrite`.

    NID = first 8 bytes of SHA-1(name || suffix), little-endian

then eleven characters of a 64-symbol alphabet with two padding bits. `obscene-tool nid`
does it, using the constant in `data/hash-suffix.toml`.

**The byte order is little-endian and that is verified, not assumed.** It is pinned by
a published pair - `sceKernelLoadStartModule` → `wzvqT4UqKX8` - checked by
`obscene-tool selftest`. Four things here (suffix, byte order, alphabet, bit packing) are each
plausible when wrong and each produce ordinary-looking output that resolves to nothing;
one real pair pins all four. See D024 and ACKNOWLEDGEMENTS.md.

## Real hardware needs one more layer

Everything here concerns being *loadable*. A stock hardware additionally requires the
module to be signed, which in practice means a fake-signed `fself`/`eboot.bin` wrapper
and a jailbroken system. None of that exists here.

So the honest position is that this work is **necessary but not sufficient** for real
hardware, and currently verified against exactly one emulator - which cannot
distinguish a genuine format requirement from that emulator's own strictness.

## Two dynamic-table conventions, and this module writes the older one

Measured 2026-08-26 against **six retail current-generation dumps**, using prosper's `self_dump`
(see `docs/EMULATORS.md`). It reads this module's 1,428 tags and 35,519 symbols correctly and
then reports `[IMPORT LIBRARIES] (0)`; against the retail titles the same tool finds 27 to 62
each. (D193)

### What a retail current-generation module writes

**Standard ELF tags for the standard tables** - nothing vendor-specific about a string table:

```text
DT_NULL(0)  DT_NEEDED(1)  DT_PLTRELSZ(2)  DT_PLTGOT(3)  DT_HASH(4)  DT_STRTAB(5)
DT_SYMTAB(6)  DT_RELA(7)  DT_RELASZ(8)  DT_RELAENT(9)  DT_STRSZ(0xa)  DT_SYMENT(0xb)
DT_PLTREL(0x14)  DT_JMPREL(0x17)
```

**Vendor tags only for vendor concepts**, and in a high range:

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
| `0x61000049` | `DT_SCE_EXPORT_LIB` / import-library records |

### What this module writes instead

Vendor tags for **everything**, in the low range: `DT_SCE_STRTAB` `0x61000035`, `DT_SCE_SYMTAB`
`0x61000039`, `DT_SCE_NEEDED_MODULE` `0x6100000f`, `DT_SCE_IMPORT_LIB` `0x61000015`, and no
standard `DT_STRTAB` or `DT_SYMTAB` at all.

Of the nineteen vendor tags written, retail modules use **five**: `0x11`, `0x17`, `0x19`,
`0x3d`, `0x3f`. The two conventions agree on the *attribute* tags (`0x17`, `0x19`) and differ on
every base record.

### Why nothing caught it

Every loader in the toolkit accepts the older form - three of them are previous-generation
emulators where it *is* the convention, and the two current-generation ones take it as well. So
the measurements stand and **none of them is evidence about the hardware**, because none of them
is the hardware. `obscene-tool derive` re-derives the tag assignment and passes, which proves
this module is self-consistent and says nothing about whether the convention is right.

### `0x61000049` is the import record, and the counts prove it

prosper's tag *namer* calls it `DT_SCE_EXPORT_LIB` while its *parser* files it under import
libraries, so one of the two is wrong in that project. Counting settles it, because a main
executable imports many libraries and exports at most itself:

| module | `NEEDED_MODULE` 0x45 | tag 0x49 | imports reported | exports reported |
|---|---|---|---|---|
| PPSA02664 | 36 | **38** | **38** | 1 |
| PPSA03416 | 36 | **38** | **38** | 1 |
| PPSA04263 | 52 | **55** | **55** | 0 |
| PPSA21564 | 59 | **62** | **62** | 0 |
| PPSA25872 | 36 | **38** | **38** | 0 |
| PPSA28061 | 26 | **27** | **27** | 0 |

The 0x49 count equals the import count in all six, exactly, while exports never exceed one.
**0x61000049 carries the import-library records**; the namer is wrong.

`DT_SCE_EXPORT_LIB` is therefore **unidentified**. Retail main executables export nothing or one
library, so the tag may simply be absent from every dump here rather than hiding under another
number. Nothing is assumed about it - this module declares its own export library and where
that record belongs in the newer convention is not established. (D008)

### What is not established

That the older convention fails on hardware. A hardware loader may accept both - the previous
generation's software has to keep working somehow. What is established is narrower and still
serious: **no retail current-generation module looks like this one**, so this module sits
outside the set the platform is known to load, and hardware is where that gets found out.

## The conventions differ in layout, not only in tag numbers

Measured 2026-08-26. The tag numbering is the visible half of a deeper difference, and the
second half is what `--table current` originally got wrong. Both halves are implemented now.
(D193)

**No retail current-generation module uses `PT_SCE_DYNLIBDATA`.** Checked across three dumps:
zero occurrences each.

| | legacy | current |
|---|---|---|
| string/symbol/hash/reloc tables live in | a `PT_SCE_DYNLIBDATA` segment | ordinary `LOAD` segments |
| what a table tag's value *means* | an **offset into that segment** | a **virtual address** |
| tags for the standard tables | `DT_SCE_STRTAB`, `DT_SCE_SYMTAB`, … | `DT_STRTAB`, `DT_SYMTAB`, … |
| vendor tags | low range, for everything | high range, vendor concepts only |

The two halves go together, and that is the whole lesson. `DT_SCE_STRTAB` holds an offset
*because* there is a segment to be an offset into; `DT_STRTAB` holds an address *because* the
table is in the image. Renumbering one without moving the other produces a module that is
neither convention, which is exactly what the first attempt produced.

### The failure that made the layout half visible

prosper resolves a table tag as a virtual address. The half-converted module declared its
vendor segment at `vaddr=0` and its first `LOAD` at `vaddr=0` too, so `strtab va=0x0` matched
the `LOAD` first and landed at file offset 0 - the ELF header. The symbol *count* still came
out right, because it is computed from the declared size, so the dump reported 35,519 symbols
and 352 libraries **while reading the wrong bytes for every one of them**.

A plausible number from a wrong address is the worst shape a bug can take, and it is the same
shape as D008: the count looked like progress, so it read as progress.

### What `--table current` does now

`build()` computes a `table_base` - 0 under `legacy`, and under `current` the end of the last
`PT_LOAD` rounded up to the 0x4000 page. Every table address is written relative to it, the
header is emitted as a real `PT_LOAD` with that vaddr and 0x4000 alignment, and `derive` finds
the segment *through the string table's address* rather than by segment type.

| | GEN=4 → `legacy` | GEN=5 → `current` |
|---|---|---|
| prosper's verdict | not a PS5 module | **PS5 layout** |
| prosper's import count | 0 | **35,518** |
| `derive` round trip | 9 relations reproduced | 9 relations reproduced |

### Neither convention is the better one

The sweep splits cleanly on it, and that is the reason `TABLE` follows `GEN` in the Makefile
rather than being a flag anyone chooses:

- **`current` breaks the previous-generation emulators outright** - shadPS4 and fpPS4 reject
  the standard tags with `unsupported dynamic tag 0x02` / `0x03` / `0x07`, and the vendor ones
  with `0x61000043` / `0x61000047`. Under `legacy` shadPS4 runs 36,575 records.
- **`legacy` is invisible to the current-generation readers.** prosper finds none of its
  imports; the whole module is outside what it will look at.

So the switch is not a quality dial. It selects which platform's module you are building, the
Makefile derives it from the generation you asked for, and passing it by hand is how you get a
module no loader in the toolkit accepts.
