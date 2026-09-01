# 2026-08-19 - First contact with a real emulator


**Done.** Downloaded shadPS4 0.18.0 and iterated the module against it until it loaded.
Four gates, each found by running it and reading the complaint rather than by reading a
specification.

| Gate | Complaint | Fix |
|---|---|---|
| 1 | `EI_OSABI expected 0x09 is (0x0)` | lld sets it correctly; GNU ld does not |
| 2 | `e_type expected 0xFE10 OR 0xFE18 OR 0xfe00 is (0x3)` | rewrite to `0xFE18` post-link |
| 3 | `SplitRegion: Unreachable code!` | segments were not page-aligned |
| 4 | `Symbol table not found!` | standard dynamic tags are ignored |

**Where it now gets to.** All segments load at 16 KiB alignment, and the loader finds
the entry point: `program entry addr: 0x0000000080018bd0`. It then reads the dynamic
table, reports every standard tag as unsupported - `DT_STRTAB`, `DT_SYMTAB`, `DT_RELA`,
`DT_JMPREL`, `DT_HASH` and the rest - and gives up with "Symbol table not found".

Also new this session: `005-generation` (which console, inferred from which exclusive
symbols resolve), availability on every censused symbol, `tools/mkmodule.py`, and
`tools/nid.py`.

**Surprises.**

- **Only one of four gates needed guessing.** Each rejection named the field, the
  expected value and the observed one. Forty minutes of running an emulator produced
  more certainty than any amount of reading about the format - and gate 3, the
  alignment one, is not documented anywhere as a requirement.

- **GNU ld was silently the wrong linker.** `-z separate-loadable-segments` produced
  `warning: ... ignored` inside an otherwise successful build, so the alignment never
  happened and nothing failed. `-fuse-ld=lld` fixed both that and the OSABI byte -
  which means gate 1 was self-inflicted.

- **lld cannot write to the Windows mount.** It writes by rename, which the share
  refuses: `failed to write output: Operation not permitted`. Same root cause as
  clang-format (D012) and as the missing execute bit. The mount is not a filesystem.

- **Page alignment is not what `-z max-page-size` gives you.** That makes segments
  *congruent* modulo the page size, which is all the ELF specification requires and is
  not the same as starting on a page boundary. A loader mapping whole pages needs the
  stronger property, and the difference only shows up as a crash inside the loader.

- **The vendor dynamic table is the whole remaining gap, and it is now precisely
  scoped.** Not "the format is different somehow" but a specific list of tags the
  loader ignores and their `DT_SCE_*` equivalents, in a `PT_SCE_DYNLIBDATA` segment.

**Not done.** The module does not execute: no symbol table the loader recognises means
no imports resolve. NID encoding is built but unused, pending a suffix.


