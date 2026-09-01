# 2026-09-01 - repo cleanup sweep, phase 1 (structure + litter)


Litter: closed the .gitignore gaps that let it accumulate at the root (*.pkg was never covered
though *.elf was; obscene-report-*.txt slipped past the exact /obscene-report.txt; +vulkan cache,
+/stdout.txt), removed two tracked junk files (stdout.txt, t.tmp.sh), and deleted the 25 now-ignored
root artifacts (regenerable pkgs/elfs/reports/pngs).

Structure - projects brought under src/ (they were top-level): tracer/ -> src/tracer/, porthole/ ->
src/porthole/, and the three loose root experiment sources (boot.c, hwreport.c, hwreport_body.c) ->
src/experiments/. Updated the only external refs: scripts/run-boot.sh and run-report.sh default
sources, and src/porthole/README's now-deeper relative link to prosperous. Both isolated subtrees
still build from their new homes (make -C src/tracer check, make -C src/porthole check|skeleton green).
src/ now holds: sections/ (probe), tracer/, porthole/, injector/, common/, experiments/, shaders/.

Deliberately NOT done: moving the probe shell itself (src/*.c + src/sections -> src/probe/) for full
symmetry. That is the one move that rewrites the whole Makefile/census/gate/tool path set at once AND
collides with the live injector workstream (shared src/common/, src/injector/), and it cannot be
verified while make host is broken by that same WIP. It wants a focused pass on a green tree with the
injector WIP committed, not a blind move now. Phase 1 touched nothing the injector session is editing.

