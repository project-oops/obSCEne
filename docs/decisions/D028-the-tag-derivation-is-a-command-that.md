# D028 - The tag derivation is a command that runs on every build, not a paragraph


Status: decided.

The fourteen vendor tag values were worked out by arithmetic on a reference module's
layout: the tables sit end to end, so an offset plus its size lands on the next offset,
and only one assignment of the fourteen makes every sum come out. That reasoning was
written down in `MODULE-FORMAT.md`, and prose rots - D023 stated a byte order as fact
that a later test disproved.

`obscene-tool derive` runs it. `mkmodule` calls it on what it has just written, so a
constant that drifts from the layout fails the build rather than producing a module
that loads and resolves nothing.

**It needs no reference module**, which matters because the reference is gone - it was
a toolchain we consulted and deleted (see `ACKNOWLEDGEMENTS.md`). `mkmodule` lays our
own tables out in the same order, so the same five sums close on our own output. The
last one closes on the segment's own size, which is the only length that comes from
outside the dynamic table - so a self-consistent chain of wrong values still fails.

What it does **not** establish: `DT_SCE_SYMENT` and `DT_SCE_RELAENT` both hold `0x18`
and neither appears in a sum. Arithmetic cannot separate them. The command says so
rather than leaving the claim implied.

