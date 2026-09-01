# 0. The vendor dynamic table - done


A real loader ignores every standard ELF dynamic tag and wants the `DT_SCE_*`
equivalents in a `PT_SCE_DYNLIBDATA` segment. That is what stood between this
repository and ever executing a single guest instruction.

**Closed.** The module loads, resolves 807 imports by NID, runs every section and ends
without a fault. All fourteen table tags plus the seven identity tags are emitted, the
derivation that produced them runs on every build (D028), and the whole chain is
recorded in `docs/MODULE-FORMAT.md` and D027-D035.

What replaced it, in rough order of how much they cost to find:

- The string table must be declared before anything carrying a name offset (D030).
- Three loadable segments, not four (D029).
- Hidden visibility and symbolic binding, or the module calls address zero (D031).
- No section headers, or a loader finds the relocations twice (D034).
- The entry point ends the process; it does not return (D035).

