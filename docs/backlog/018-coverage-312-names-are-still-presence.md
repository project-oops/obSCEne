# 8. Coverage: 312 names are still presence-only


The largest single item, and the least interesting per unit of effort. A hundred and one
checks invoke something; three hundred and twelve censused names only prove a symbol
resolves.

**And under one loader that claim is weaker than it looks.** shadPS4 resolves every import
through a generic stub, so all 312 report present while roughly forty per cent behave.
Presence measures the loader's stubbing policy as much as the platform (D061).
An existence test cannot see a behaviour bug, and behaviour bugs are what emulators have -
that is settled (D045), so the remainder is grind rather than argument.

The conversion is mechanical now and the shape is fixed:

1. Move the name out of the census in `surface.h` into a real declaration in `platform.h`
   (the census declares names as `const char` so the type system forbids calling them,
   which means a name cannot be in both).
2. Add it to `src/probe/imports.c` so `mkmodule` can place it in a library.
3. Add it to `data/surface.txt`'s `@called-elsewhere` block, or regenerating puts it back
   and the build breaks on the duplicate declaration.
4. Write the check with `OBS_FROM_SPEC` if ISO C or POSIX settles it.
5. **Run it on the host first.** Twice now the host build has caught a check that was
   wrong rather than the platform - a `fmod` probe whose two inputs gave the same answer,
   and a set of checks ordered before the capability they required. A check that has not
   passed a known-good implementation is not evidence of anything.

Worth doing opportunistically rather than exhaustively. Nineteen of twenty-seven functions
probed so far are stubs, so most new checks against the current emulators will report
"still a stub" - real coverage, no new information. A hardware run will reorder which of
these are worth having.

