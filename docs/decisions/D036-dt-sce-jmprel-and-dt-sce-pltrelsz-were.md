# D036 - `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were swapped, and the derivation could not have caught it


Status: corrected, from an open-source loader's own header.

The two were the wrong way round:

|  | was | is |
|---|---|---|
| `DT_SCE_JMPREL` | `0x6100002D` | `0x61000029` |
| `DT_SCE_PLTRELSZ` | `0x61000029` | `0x6100002D` |

Every other one of the nineteen matched exactly, which is a strong check on the rest of
the table.

**Why `obscene-tool derive` reported this as established.** The relation it tests is

    JMPREL + PLTRELSZ == RELA

and addition is commutative. An offset plus a size sums to the same place whichever of
the pair is which, so the arithmetic that fixed the other tags is structurally incapable
of separating these two. D028 said this about `SYMENT` and `RELAENT` - both hold `0x18`
and neither appears in a sum - and missed that the same hole applies to every
offset/size pair. `derive` now says so.

**What it cost.** A module declaring a linkage table told both loaders that its size was
its address, so both read relocations from near the start of the vendor segment - which
is the string table - and reported "relocation types" that were four bytes of ASCII out
of a symbol name. That reads as a table-pointer problem, and a day went into looking for
one.

It also produced a false conclusion. `-fno-plt` leaves `.rela.plt` empty, and an empty
table is not declared at all (D034's sibling rule), so the two bad tags were simply
absent from that build. One emulator therefore ran the module with the flag and failed
without it; the other did the reverse. That was recorded as two loaders wanting
incompatible things. They did not: one bug was hidden by a build flag, and the flag was
holding up the wrong conclusion.

With the tags corrected the ordinary PLT build runs in both, and `-fno-plt` is gone.

**Provenance.** Read from `elf.h` in an open-source emulator, which principle 6 names as
a sanctioned source. Recorded in `ACKNOWLEDGEMENTS.md`. Nothing was copied: the
correction is two integers and the reason they were wrong.

**The method lesson.** Deriving the table from a reference module was right and produced
nineteen correct values from arithmetic that could not be argued with. It also produced
two wrong ones with exactly the same confidence, and no amount of further black-box
probing would have found them, because the property that distinguishes them is not
visible from outside. Reading a loader took ten minutes.

