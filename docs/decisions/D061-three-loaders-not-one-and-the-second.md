# D061 - Three loaders, not one, and the second and third each found something the first could not


Status: decided, after a fair criticism.

Fourteen emulators had been cloned and exactly one was ever run. Everything the project
knew about how it behaves under an emulator came from shadPS4, and several conclusions
were phrased as facts about the platform when they were facts about that loader.

### Kyty: its output was never uncapturable

obSCEne learned to draw its report to the screen because Kyty's console output is not
visible to a parent process. That is true, and the workaround was never necessary: the
binary takes `--printf-direction File --printf-output-file`, and the whole obstacle was a
setting nobody had looked for.

The drawn report is a good feature and it will matter on hardware. It was still built to
route around something a command-line flag removes.

**Also: the checked-out source is not the binary.** Kyty's default branch describes a Lua
configuration interface; the build on disk is 0.2.2 and takes arguments. The clone is for
reading the loader, not for learning to drive it - ask the binary, which prints its usage.

### The census means different things under different loaders

shadPS4 resolves every import through a generic stub, so all 313 census symbols report
present while roughly forty per cent behave. Kyty patches each unresolved import
individually and says which, so its log names 238 functions it does not have.

**Neither is wrong and neither is complete.** Presence under shadPS4 measures its stubbing
policy; absence under Kyty measures what it implements. A census result is a statement
about the loader as much as about the platform, and this only became visible on running a
second one.

`scripts/unresolved.py` turns Kyty's log into that list by name - a lookup, not a crack,
because this program knows the name of every symbol it imports and computed the NIDs
itself. 237 of 238 named.

### craziiEmu: obSCEne had been declaring itself the wrong console generation

`e_ident[EI_ABIVERSION]` was zero, which every linker leaves it at because no linker knows
about this console. craziiEmu classified the module as previous-generation; setting the
byte to 2 makes it report Gen5, and its descriptor count went from zero to one.

**Corroborated before being believed.** Kyty's `Elf64::IsNextGen()` is exactly this byte
against 2, and it rejects any value that is neither 0 nor 2. Two implementations that did
not copy each other reading one byte identically is the standard that settled the dynamic
tag values (D038). shadPS4 reads the field only to name it in a log line and does not gate
on it, so the change is safe for the loader that already worked.

A probe for the current console generation had been telling every loader that checks that
it was built for the previous one.

