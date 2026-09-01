# D138 - A symbol can be imported with no name at all. The identifier is the import; the name only ever existed to compute it


Status: derived - built, and the identifier verified present in the module verbatim.

Firmware export tables carry 1,130,742 identifiers whose names nobody outside the vendor
holds, and the hash is one-way. They are nonetheless importable, because an import *is* an
identifier - computing one you already have is not required.

What stood in the way was C. `Xh3kd9sLpQw` might be a legal identifier by luck;
`4wSze92BhLI` is not, and neither is anything containing `+` or `-`. Each is now declared
with a generated C name and an assembler label carrying the real symbol:

```c
extern OBS_WEAK const char obs_nid_000001 __asm__("$Xh3kd9sLpQw");
```

`mkmodule` recognises the `$` and passes the rest through instead of hashing it - hashing
would produce the identifier of the *string* `$Xh3kd9sLpQw`, which names nothing and would
resolve nowhere. `$` is legal in an ELF symbol name and illegal in a C one, so no real name
can collide with the marker.

Verified in the built module: `3DIRdq4I-s8#...` appears in the vendor segment exactly as
the firmware table spells it. 4,219 identifiers across 113 libraries; the module now
carries **39,737 symbols from 372 libraries**.

### The sigil stays visible in the report

A census line reads:

```
OBS|sym|libSceFont|$Xh3kd9sLpQw|present|shared
```

That is the honest rendering. This project knows something is exported there and cannot say
what it is called. Inventing a name would be worse and printing the bare identifier would
let it pass for one - and the marker is exactly the "unknown, flag it" treatment asked for
rather than a silent blend into the named census.

### Why the library is load-bearing here and nowhere else

A name can be hashed and asked of any library; if the guess is wrong the identifier is
still right. An identifier cannot be recomputed, so it is only ever resolvable from the
library that exports it. Entries without an attributed library are not emitted at all -
which is affordable only because the firmware module descriptions supply one for **every**
unnamed identifier (D105).

