# D128 - 30,610 names mined from seven emulators. The hash reproduces 30,137 of them, and every exception is the source's naming, not the hash


Status: derived - by comparison against every identifier the toolkit publishes.

`scripts/mine-nids.py` reads seven emulators in six formats and produces
`data/mined-names.txt`: every symbol name any loader in the toolkit knows, with the
identifiers it claims.

| source | rows |
|---|---|
| fpPS4 `ps4libdoc.pas` | 78,372 |
| shadPS4 `LIB_FUNCTION` | 5,174 |
| PS5PCEM Zig tables | 1,496 |
| SharpEMU | 1,190 |
| craziiEmu | 945 |
| GPCS4 | 899 |
| ChonkyStation4 | 507 |
| ps4libdoc `known_names` | 22,382 |

**30,610 unique names, 22,944 corroborated by two or more independent sources.** obSCEne
knows 564 and 530 of those appear here, so the surface would grow **54-fold**.

### The hash, checked against all of it

30,137 match. 473 do not, and none is a hash failure:

- **223 are placeholders.** `Func_00F4D778F1C88CB3` and `_import_060337B772EF70D9` are
  fpPS4's way of writing "the function whose identifier is this" - the hex is in the name.
  Not names at all, and a free source of *unnamed* identifiers.
- **The rest are implementation names, not symbol names.** `SysmoduleGetModuleInfoForUnwind`
  hashes to `0x0563ea3d...` against a claimed `0xe1f539ca...`; restore the prefix and
  `sceSysmoduleGetModuleInfoForUnwind` hashes to `0xe1f539caf3a4546e` exactly. shadPS4
  names its C functions for the reader, and one C function can serve several symbols -
  `sceHmdDistortionGet2dVrCommand` is registered under two identifiers in two libraries,
  only one of which is that name's hash.

So the corroboration is now seven-way and spans both generations.

### Two things that must happen before any of this is ingested

**A name that does not hash to a claimed identifier must not enter the surface.** It would
make obSCEne import a symbol that cannot exist - harmless, since an unresolved import is
reported absent, but 250 rows of noise in a census whose value is that its absences mean
something. The miner needs to check each row against the hash and mark it.

**25,320 of the 30,610 have no library attribution**, because only three of the seven
sources publish it. That matters for obSCEne's own answer and not at all for supplying
identifiers to a name search - the two goals want different things from the same corpus.

### The number that reframes the exercise

`ps4libdoc` carries **1,130,742 identifiers observed in real modules with no known name**,
against the 30,610 names collected here. **2.64%.**

Mining every emulator in existence still leaves 97% of the observed surface nameless. The
named set is the rounding error; the unnamed identifiers are the interesting half, and
obSCEne can import those directly because the import is the identifier and the name only
ever existed to compute it.

