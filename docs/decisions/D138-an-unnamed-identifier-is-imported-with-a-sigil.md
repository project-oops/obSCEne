# D138 - An unnamed identifier is imported with a `$` sigil

**Status:** decided
**Date:** 2026-09-26

A symbol known only by its identifier is declared with a generated C name and an assembler label
`$<identifier>`; the module writer passes the identifier through instead of hashing it. The census
prints the sigil in its `sym` record. An identifier with no attributed library is not emitted.

**Why:** the import is the identifier; the name only ever existed to compute it. `$` is legal in an
ELF symbol and illegal in C, so it cannot collide with a real name, and printing it keeps an unnamed
symbol from passing for a named one. An identifier resolves only from the library that exports it.

**Rejected:** inventing names. Printing the bare identifier - reads as a name.
