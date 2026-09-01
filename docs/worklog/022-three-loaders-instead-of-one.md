# Three loaders instead of one


Fourteen emulators had been cloned and exactly one was ever run. Everything the project
believed about its behaviour under an emulator came from shadPS4, and several conclusions
were phrased as facts about the platform when they were facts about that loader (D061).

### Kyty: the drawn report was never necessary

Its console output genuinely is invisible to a parent process. `--printf-direction File
--printf-output-file` removes the problem entirely, and the whole obstacle was a setting
nobody had looked for. The drawn report stays - it will matter on hardware - but it was
built to route around a command-line flag.

Also: the checked-out source describes a Lua configuration interface and the binary on
disk is 0.2.2, which takes arguments. Read the clone to understand the loader; ask the
binary how to drive it.

### The census means different things under different loaders

shadPS4 resolves every import through a generic stub, so all 313 census symbols report
present while roughly forty per cent behave. Kyty patches each unresolved import
individually and says which - 238 functions it does not have, including **all 63 of
`libScePosix`**.

Neither is wrong. Presence under shadPS4 measures its stubbing policy; absence under Kyty
measures what it implements. Census results had been read as statements about the
platform, and this is the correction.

`scripts/unresolved.py` turns that log into the useful direction - which functions are
missing, by name, grouped by library. A lookup rather than a crack: this program knows the
name of every symbol it imports and computed the NIDs itself, so hashing our own list gives
an exact table. 237 of 238 named.

### craziiEmu: the module had been declaring the wrong console generation

Built it from source; dotnet was already present. It reported `Generation: Gen4 (PS4)` for
a probe aimed at the current generation, because `e_ident[EI_ABIVERSION]` was zero - which
every linker leaves it at, none of them knowing about either console.

Setting it to 2 flips craziiEmu to `Gen5 (PS5)` and takes its relocation-descriptor count
from zero to one. Corroborated before it was believed: Kyty's `Elf64::IsNextGen()` is
exactly that byte against 2.

### And then it broke shadPS4 completely

**A correction to what was reported an hour earlier.** The change was described as safe for
shadPS4 because that emulator "reads this field only to name it in a log line and does not
gate on it". It rejects the module outright:

```
IsElfFile: e_ident[EI_ABIVERSION] expected 0x00 is (0x2)
```

666 records to zero. The claim came from grepping for the field, finding a function that
formats it for a log, and stopping - the actual check is forty lines away in the same file.
Finding *an* answer is not finding *the* answer, and this is the second time today a
too-early stop produced a confident wrong statement.

The conflict itself is real and nobody is at fault: shadPS4 is a previous-generation
emulator, and refusing a module marked for the current generation is what the field is for.
So the generation became a build option - `GEN=5` by default, `GEN=4` for shadPS4 - rather
than a constant either loader has to be worked around (D062). A `GEN=4` build says something
true about what it was built as; the previous state said something false to every
current-generation loader.

shadPS4 restored: 667 records, complete, 65 pass / 6 partial / 38 fail / 7 skip.

### Smaller things

Clippy flagged the dispatcher as too long once `mkmodule` gained a flag, so its arguments
moved into their own struct rather than the lint being suppressed. 88 tool tests.

The Makefile edit went in with a literal `\n` instead of a line continuation - the same
escaping mistake that has caught this session repeatedly. Fixed with an editor rather than
a string-escaping script, which is the actual lesson.

---

