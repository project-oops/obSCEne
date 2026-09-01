# D034 - A finished module has no section headers


Status: decided, on evidence.

The linked file carries `.rela.dyn` and `.rela.plt` as sections, and the vendor segment
carries copies described by `DT_SCE_RELA` and `DT_SCE_JMPREL`. Two descriptions of the
same relocations, in two places, reachable two ways - and a loader is free to find them
either way.

Two separate emulators were observed reading relocations from somewhere neither tag
points at, logging entries whose "type" was four bytes of ASCII out of a string table.
That is not harmless noise: a loader that mis-reads a relocation does not fail to apply
it, it *applies* it - writing a bad value at a bad address inside the loaded image.

`mkmodule` now zeroes the section header table. Nothing is deleted, only the index;
the bytes stay where they are, described by the program headers, which is all a loader
reads. The garbage relocations stopped immediately.

**It cost something.** `obscene-tool imports` read `.dynsym` and could no longer see a
module at all. It now reads the vendor tables the way a loader does, which is what it
should have done from the start - a module is the shape it is, and a tool that can only
inspect the intermediate form cannot inspect a real one either.

