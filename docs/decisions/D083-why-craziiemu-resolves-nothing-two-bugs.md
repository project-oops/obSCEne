# D083 - Why craziiEmu resolves nothing: two bugs, one fixed, and the earlier guess was wrong


Status: diagnosed.

BACKLOG §0c recorded craziiEmu as resolving nothing and guessed at the reason - "eleven
vendor tags, every one describing a table, and none of the four that say who a module is".
That was wrong. **Its tag constants match this project's exactly, all eight of them.**

Running obSCEne against it found two independent faults:

**A zero offset read as an absent table.** `HasImportMetadata` required
`StrTabOffset != 0`. Those offsets are relative to the start of `PT_SCE_DYNLIBDATA`, so the
first table in that segment legitimately begins at zero - and this project's string table
does. Every table parsed correctly and the module was then declared to have no import
metadata. Sizes say whether a table is present; offsets do not. Fixed in the local clone,
one line, and the module now reports `HasImportMetadata: True`.

**Table offsets resolved against the wrong base.** `TryLoadTableBytes` computes
`location + imageBase`, but a `DT_SCE_*` offset is relative to `PT_SCE_DYNLIBDATA`. So it
reads the string table out of the ELF header, finds no valid symbol names, and ends with
1691 relocation descriptors and zero recovered identifiers.

craziiEmu never locates that segment: there is no reference to program header type
`0x61000000` anywhere in its loader. That is not a correction to its architecture but an
addition to it - roughly forty lines across several call sites - so it is diagnosed and
left rather than done unilaterally.

**Worth noting what produced this.** Neither fault is visible from reading the source; both
came from running a module through it and reading what it printed. The first was found by a
guessed offset being valid, the second by a table parsing successfully and yielding
nothing - and a project that had never had a module reach that far would have no reason to
look.

