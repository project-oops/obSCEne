# D302 - A hardware capture is named for the launch shape, and hardware is the authority

**Status:** decided
**Date:** 2026-09-26

A capture is named `obscene-probe-<target>-<mode>-<privilege>-<category>.log`, one file per cell,
overwritten in place; the directory is the matrix that `obscene-tool matrix` reads. Category is an
axis because it governs memory and display independently of privilege (D296). Where a `hardware`
cell is present it settles a disagreement; where none is, `matrix` reports the disagreement as
unsettled. A gap is a shape one target produced that another lacks, not an empty cross-product
cell.

**Why:** how a run was launched is the axis that changes the answers, and nothing in a
scope-or-finding filename predicts it. A console now answers, so hardware is the authority rather
than one vote (D072). Dated accumulation rebuilds version history by hand.

**Rejected:** naming by scope or finding. A privilege-only name - files two different-category runs
in one cell. A hardware column in the compat table (D090).
