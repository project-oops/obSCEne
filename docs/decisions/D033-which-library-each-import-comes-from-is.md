# D033 - Which library each import comes from is published by the host build


Status: decided.

A module encodes an import as a NID plus a library id, and separately declares what
that id means. An id with nothing declared against it resolves to nothing - the first
version encoded every symbol as `<nid>#A#A`, four hundred imports all claiming a
library that was never declared, and the loader gave up with "Unable to find library
and module".

A linked ELF cannot supply the association: a `.dynsym` entry records that a symbol is
undefined, not who is expected to define it. It exists in the census tables and the
import table and nowhere else, so `obscene-host --symbols` prints it and the module
build consumes it. That makes the host build a prerequisite of the module build, which
it already was in principle (D001).

**There is no default.** `mkmodule` fails, naming every symbol it cannot place, rather
than falling back to id zero. Defaulting turns a missing line into a module that loads,
resolves nothing, and says nothing about why.

