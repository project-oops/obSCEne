# D200 - The file formats moved to `selfish`, and this repository now depends on them


Four modules and four data files left this tree. `tool/src/` lost 2,801 lines:

| gone | now |
|---|---|
| `nid.rs` (289) | `selfish-nid` |
| `dynlib.rs` (1,499) | `selfish-elf::dynlib`, both directions |
| `module.rs` (258) | `selfish-elf::identity` |
| `mkself.rs` (755) | `selfish-container` |
| `data/hash-suffix.toml`, `self-format.tsv`, `pkg-format.tsv` | `selfish/data/` |
| `link/module.ld` | `selfish/link/module.ld` |

`data/` now holds only this project's own measurement products - the mined corpus, the surface
census, the GPU surface table. That is the split the sibling repository exists to make: a
format is shared, a measurement stays with whatever measured it.

**What did not move: the manifest.** Which library resolves which name is this project's, and
`selfish::dynlib::build` takes it as a closure. So is `imports::PREENCODED`, the `$` sigil
marking a symbol whose name *is* the identifier - a convention of this project's symbol tables
rather than a fact about the format.

### It was checked by this repository's own verifier

`mkmodule` re-derives the tag assignment from the bytes it just wrote, and that derivation
knows nothing about how they were produced:

```
p.elf: EI_OSABI 0x0000 -> 0x0009
p.elf: e_type 0x0003 -> 0xfe10
p.elf: vendor segment at 0x297e0, 408 bytes, 23 tags, 1 symbols from 1 libraries
p.elf: layout reproduces the tag derivation (9 relations)
```

`selftest` still matches the published pair through the moved suffix, and 204 tests pass - ten
fewer than before, which is the ten that went with the modules.

