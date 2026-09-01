# 2026-09-01 - cleanup sweep finished: projects under src/, encoder wired, docs consistent (D281)


Finished the D280 encoder section (imports.c + surface.txt @called-elsewhere - make check green with
them) and completed the structural sweep: all source projects now live under src/ (probe/, injector/,
porthole/, tracer/, experiments/, common/, shaders/), with the probe being the former loose src/*.c +
src/sections/ moved to src/probe/. Makefile source lists repointed; object machinery unchanged (the
src/%.c patterns absorb the nested dir). Host builds clean.

The move made verify.sh's doccheck claims stale - a gate failure, since every `obscene:claim
file=src/...` pointed at a moved file. Fixed all of them to src/probe/, plus the current-doc prose
(CLAUDE.md, GNM.md, HARDWARE-PROBE.md, BACKLOG.md, INJECTOR.md) and the run-boot/run-report script
defaults. Left historical DECISIONS/WORKLOG prose as the record. Litter (gitignore gaps, tracked junk,
root artifacts) cleared earlier in the sweep.

Note: the very last `make check` after the probe move was interrupted; another thread was fixing an
unrelated build error concurrently. host build is confirmed green post-move; a full make check should
be re-run once that thread's fix lands to confirm the gates on the final tree.

