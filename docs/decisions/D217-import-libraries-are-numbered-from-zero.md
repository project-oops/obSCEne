# D217 - Import libraries are numbered from zero, needed modules from one


Status: **decided**, 2026-08-29.

`Imports::parse` gave both `index + 1`. Real material shows they are different sequences: a
launching executable declares ten import libraries at ids 0 through 9 and nine needed modules at
ids 1 through 9, and its symbol suffixes agree. Module zero is the executable itself so nothing
else may claim it; library zero is free, because an executable exports no library to put there
(selfish D074).

Numbering libraries from one left the import table with no entry at the front. Two things follow
from where that id is written down, and they are why this is fixed here rather than in `selfish`:
the id is baked into every symbol name at build time - `4wSze92BhLI#A#B` carries it - so
renumbering later would leave the symbol table and the tag table stating different things, which
is the exact failure mode this project keeps finding. The manifest decides the id, so the
manifest is where it gets decided correctly.

Two derivation relations moved with it, and the checker caught both rather than being told:

- **"STRTAB is the start of the segment"** is now "FINGERPRINT is the start of the segment"
  plus "STRTAB follows the fingerprint". A real executable puts its string table at `0x18`.
- **"HASH + HASHSZ == end of segment"** is now "== the dynamic table at the segment's tail",
  measured from the headers rather than assumed, so a module laid out either way is checked
  against how it actually is. Real: hash ends `0x3320`, segment ends `0x3760`, and the `0x440`
  between them is the dynamic table.

Both had been true only while something else was in the wrong place. A relation that holds for
the wrong reason is worth more than one that fails, and worth less than one that says why.

