# D063 - The NID implementation agrees with an independent one on 78,372 pairs out of 78,372


Status: evidence.

fpPS4 embeds `ps4libdoc.pas`: 78,372 NID-to-name pairs, as raw 64-bit values beside their
names. It is a Pascal project by a different author with no connection to this one.

Every pair agrees. Not most - all of them, on the first comparison, with no byte-order or
alphabet adjustment needed once the published test vector settled the interpretation.

Worth setting against where this started. The suffix came from community forum posts,
and the only confirmation available was a single published pair
(`sceKernelLoadStartModule` -> `wzvqT4UqKX8`). Then 388 of 389 against ps4libdoc's name
list, the one difference explained by a PS5 name absent from a PS4 database. Now 78,372
of 78,372, including that PS5 name.

**It is referenced, never copied.** It is somebody else's data file in a GPL project, and
the rule already in force for the emulator sources applies unchanged: read them where they
are, take facts, vendor nothing. `scripts/unresolved.py` consults the path when it exists
and works without it, with less reach, and says which names came from where.

**It was not needed for our own module, and that is the point.** For a module this program
built, its own manifest names every import exactly - 238 of 238 - because the NIDs were
computed here. The borrowed table is for logs from modules we did not build, where the
names cannot be recomputed.

### A control symbol looked like a gap

One import stayed unnamed against both tables: `obs_census_control_absent`, the
deliberately non-existent name the census uses to prove it can detect absence. A loader
reporting it unresolved is the control working correctly.

The reader had been scanning the two headers and not `src/imports.c`, which is the
authoritative manifest - `mkmodule` refuses to build a module whose undefined symbols are
not all in it. The control is declared in a section file, so no header scan could have
found it. Reading the manifest is both the fix and the thing that cannot go stale.

