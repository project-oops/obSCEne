# D193 - obSCEne writes a dynamic table no retail current-generation module uses, and six real dumps say so


Building prosper produced `self_dump`, a module reader that runs standalone. Pointed at
obSCEne it parses the file correctly - raw-ELF fallback engaged, `type=0xfe10`
**ET_SCE_DYNEXEC (main exe)** named independently, all six program headers, `DYNAMIC` mapped,
1,428 tags read, 35,519 symbols found. Then:

```text
[NEEDED MODULES] (0)
[IMPORT LIBRARIES] (0)
[SYMBOLS] total=35519  imports=0  exports=0
```

It finds the symbols and **none of the imports**. Pointed at the six retail titles in the
sibling project's library, the same tool finds 27 to 62 import libraries each.

### Two conventions, and this module is on the wrong one

| module | `DT_STRTAB` standard | `DT_SCE_STRTAB` vendor | `NEEDED_MODULE` 0x45 | import libs found |
|---|---|---|---|---|
| six retail titles | **1** | 0 | 26-59 | **27-62** |
| obSCEne GEN=5 | 0 | **1** | 0 | **0** |
| obSCEne GEN=4 | 0 | **1** | 0 | **0** |

A retail current-generation module uses the **standard ELF tags** for the standard tables -
`DT_STRTAB` (0x05), `DT_SYMTAB` (0x06), `DT_SYMENT` (0x0b) - and spends vendor tags only on
vendor concepts, in a high range: `DT_SCE_SYMTABSZ` 0x3f, `DT_SCE_ORIG_FILENAME` 0x41,
`DT_SCE_NEEDED_MODULE` 0x45, `DT_SCE_IMPORT_LIB` 0x49.

obSCEne spends vendor tags on **everything** - `DT_SCE_STRTAB` 0x35, `DT_SCE_SYMTAB` 0x39 -
and names its needed modules with 0x0f and its import libraries with 0x15. Of nineteen vendor
tags it writes, retail modules use **five**.

### Why five loaders never noticed

They all accept it. shadPS4, fpPS4 and Kyty are previous-generation emulators where this *is*
the convention; PS5PCEM and orbistoun accept it too. So every existing measurement stands - and
none of them is evidence about the console, because **not one of them is the console**.

This is the failure this project is built to avoid, found one layer below where it was looking:
a thing that works everywhere it has been tried, and may not work on the only platform that
matters. `obscene-tool derive` re-derives the tag assignment from a finished module and passes,
which proves the module is *self-consistent* - it says nothing about whether the convention is
right.

### What it does not establish

**That the previous-generation convention is invalid on hardware.** A console loader may accept
both; the previous generation's software has to keep working somehow. What is established is
narrower and still serious: **no retail current-generation module looks like ours**, so we are
outside the set the platform is known to load, and we would find out on hardware.

### Fixed, in two halves, and the second half was the one that mattered

**The numbering was the easy half.** `Tags::of(table)` holds both assignments and `derive`
checks either. Done first, and on its own it made things *worse*: prosper began reporting
352 import libraries where it had reported none, which looked like the fix landing and was
not. The counts come from tag records prosper could now see; the strings they pointed at
still resolved against the wrong segment. A plausible number from a wrong address - the same
shape as every constant D008 forbids inventing.

**The layout was the real half.** The two go together: a vendor tag holds an *offset* because
there is a `PT_SCE_DYNLIBDATA` to be an offset into, and a standard tag holds a *virtual
address* because the table is in the mapped image. So `build()` now computes a `table_base` -
0 under legacy, the end of the last `PT_LOAD` rounded to the 0x4000 page under current - and
emits the header as a real `PT_LOAD` rather than a vendor segment. `derive` finds the segment
through the string table's address instead of by segment type.

| | GEN=4 → legacy | GEN=5 → current |
|---|---|---|
| prosper's verdict | not a PS5 module | **PS5 layout** |
| prosper's imports | 0 | **35,518** |

### Neither convention is the better one, so `TABLE` follows `GEN`

The sweep splits on it. `current` breaks shadPS4 and fpPS4 outright - `unsupported dynamic tag
0x02`, `0x03`, `0x07`, `0x61000043`, `0x61000047` - where under `legacy` shadPS4 runs 36,575
records. `legacy` is invisible to prosper.

So this is not a quality dial and `--table` is not a flag to pick by preference. The Makefile
derives it: `TABLE ?= $(if $(filter 5,$(GEN)),current,legacy)`. Asking for a generation now
gets you that generation's module shape, which is what `GEN` always claimed to mean.

Status: **assumed** - six retail dumps agree and a fourth reader now accepts our module; no
console has been asked. Still ranked first on `docs/HARDWARE-PROBE.md`, because agreeing with
an open-source reader is not the same as loading on hardware.

### Writers are told the convention; readers detect it

The first version of this shipped the switch to `mkmodule` and stopped, and `verify.sh` failed
on two gates that had nothing to do with writing:

```text
=== derive (module)
not a vendor-format module: no PT_SCE_DYNLIBDATA segment ... nothing here to derive from.
--- target object ---
imports      0   no imports: this module asks the platform for nothing
```

Both about the module the same run had just built and checked nine relations against. `derive`
and `imports` are handed a *file*, not a build flag, so a default was the wrong shape for them
entirely - and the default silently made every current-generation module unreadable to this
project's own tools. `imports 0` is the same false zero as the sweep's Kyty row: it reads as
"this module asks for nothing", which is a damning finding, rather than "this reader cannot see
the table".

So `dynlib::detect` reads the convention off the module and both callers route through it.
`mkmodule` still takes `--table`, because a writer genuinely does know.

**It detects on the vendor tags, not on `DT_STRTAB`,** and that is load-bearing: an ordinary ELF
payload has a `DT_STRTAB` too, so keying on it would have made `derive` accept a payload and
quietly broken the gate that exists to stop exactly that. The high-range vendor tags are the
part a module can only have on purpose.

