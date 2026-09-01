# D281 - Projects live under src/, and the encoder section finished its wiring


**decided** - 2026-09-01 (user-directed cleanup sweep)

Two things the D280 work left unfinished, done here.

**The encoder section (106-encoder) is now fully wired.** D280 added the section but skipped two of
the five "before adding a check" steps, so `mkmodule`/`verify.sh` would have refused: the three
`libSceVencCore` imports are now in `src/probe/imports.c`, and the names are in `surface.txt`'s
`@called-elsewhere` block (they were in no census group, so nothing to remove). `make check` is
green with them.

**Structure: every source project is under `src/`.** They were scattered at the top level. Now
`src/` holds only project directories - `probe/` (the conformance probe: what used to be the loose
`src/*.c` plus `src/sections/`), `injector/`, `porthole/`, `tracer/`, `experiments/` (the loose
`boot.c`/`hwreport.c` bring-up sources), and the shared `common/` (freestд, used by both probe and
injector) and `shaders/`. The Makefile's source lists move to `src/probe/…`; its object machinery
(`$(patsubst src/%.c,…)`, the `src/%.c` pattern rules) needed no change because the `%` already
absorbs a nested directory - `obj/host/probe/sections/…` is created the same way `obj/host/sections/…`
was. `src/common/` stays put precisely because it is shared, so it is not the probe's to own.

**The move made the doccheck claims stale, which is a gate failure, not cosmetics.** `verify.sh`'s
`doccheck` enforces `<!-- obscene:claim file=src/… -->` directives, and every one pointed at a moved
file (`src/crt.c`, `src/harness.c`, `src/sections/sync.c`, `src/net.c`, `src/net_posix.c`). All were
repointed to `src/probe/…`, along with the current-reference prose in `CLAUDE.md`, `GNM.md`,
`HARDWARE-PROBE.md`, `BACKLOG.md`, `INJECTOR.md` and the two hardware scripts' default source paths.
Historical DECISIONS/WORKLOG prose describing the old layout is left as the record it is.

Litter, in the same sweep: `.gitignore` gaps closed (`*.pkg`, `obscene-report-*.txt`, the vulkan
cache, `/stdout.txt`), two tracked junk files removed, and the regenerable root artifacts deleted.

