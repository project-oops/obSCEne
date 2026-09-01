# D201 - The migration found four things wrong, and three of them were wrong here


Worth recording, because "move the code and see what breaks" turned out to be a better review
than reading it:

- **A version exception was silently dropped.** `selfish`'s first dynamic-table writer
  hardcoded module version 1.1 for every library, losing `libSceVideoOut` at generation 4 -
  the case D186 exists for, where declaring 1.1 binds a previous-generation module to the
  current generation's display library and draws a black window. It is now
  `selfish/data/library-versions.tsv`, with a row and a test.
- **`selfish` detected the tag convention from the string table alone.** `5` is also plain
  `DT_STRTAB`, so every ordinary shared object read as a current-convention vendor module.
  *This repository's* `detect` was already right, keying on the vendor-range identity tags,
  and that is what `selfish` now does.
- **`selfish` could not find the tables under the current convention at all**, where they live
  in a `PT_LOAD` rather than a `PT_SCE_DYNLIBDATA`. `vendor_imports` here handled it;
  `selfish::Elf::tables` now does, for both conventions, rebasing offsets so a reader does not
  need to know which it was handed.
- **`module.rs` carried a stale comment**, arguing at length for `0xFE18` above code writing
  `0xFE10`. It predates the D175 correction. It left with the file; `selfish` sidesteps it by
  making the object type a parameter, since an executable and a library are both legitimate
  outputs and only a builder knows which it is making.

The direction of the fixes is the point. Two of the four were `selfish` being wrong and this
project being right, which is what a shared library gets from having a second reader - and is
the same argument the two vendor tag ranges made in the other direction.

