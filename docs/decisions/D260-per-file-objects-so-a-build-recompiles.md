# D260 - Per-file objects, so a build recompiles only what changed


*status: measured*

Every C target compiled all of its sources in a single `clang <every .c> -o target` command.
So every build recompiled everything - `surface.c` and its thirty-five-thousand-symbol census
included - `make` could not skip an unchanged file, `-j` had nothing to parallelise inside one
compile, and a compiler cache could not cache a multi-file compile-and-link at all.

Each target now compiles each source to its own `.o` and links the objects. Measured:

| | before (monolithic) | after |
|---|---|---|
| edit one file, rebuild `module` | full recompile (~30-60 s) | **1.5 s** |
| clean `module` rebuild, cache warm | ~30-60 s | **2.6 s** |
| script rebuild, nothing changed | tens of seconds | **7 s** |

The object-built eboot was run on hardware and behaves identically - same report, same tally,
GEN unknown, clean display - so the link from objects is equivalent to the monolithic
compile-and-link.

**The correctness hazard, and how it is handled.** make decides what to rebuild from file
times, not from compile flags. This project flips `GEN`, `DISPLAY_MEM`, `EXCLUDE` and more
between builds, and reusing an object built with the old value would be a silently wrong binary
- which costs a hardware cycle to notice, the worst kind of wrong here. Each object depends on a
per-target `.flags` sentinel that is rewritten, and so made newer than every object, exactly
when the flag string changes; a flag change therefore forces a full recompile. Verified: touch
one file → one object rebuilds; change `DISPLAY_MEM` → all eighty-eight rebuild.

The flags are written to the sentinel with make's `$(file ...)` function rather than through the
shell, because they carry embedded quotes - `-DOBSCENE_BUILD_ID='"dev"'` - that no shell quoting
survives. `$(file)` evaluates when make expands the recipe, before any `mkdir` in that recipe
would run, so the object directories are made by an order-only prerequisite instead.

Link-only flags (`-nostdlib`, the `-Wl,` group) are kept off the compile line and `-D`/`-I`/
warning flags off the link line: each is an unused argument in the other phase, and `-Werror`
turns an unused-argument warning into a failure.

`CC` stays `:=`, not `?=`: `?=` loses to make's built-in `cc` default, which would quietly build
with gcc. A command-line `make CC="sccache clang"` overrides `:=` and is how the cache wraps in.

The `payload` target is left monolithic - it is off the package path and rarely built, so the
restructure is scoped to `host`, `module` and `eboot`.

