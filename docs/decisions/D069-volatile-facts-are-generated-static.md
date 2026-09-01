# D069 - Volatile facts are generated; static facts are written. The distinction is the whole fix


Status: decided, on the observation that most of a review's findings were drift.

A count in prose has no mechanism to stay right. The README said 79 checks when there were
106, 326 censused symbols when there were 312, and `screen.c` said "fifteen rows, which
fits with room to spare" while drawing twenty-one rows off the bottom of the framebuffer.
None of it was carelessness - every number was correct when written.

The fix is not proofreading. It is deciding, per fact, whether it moves:

**Things that change every working day** - check counts, census size, the provenance split
- belong to the code. `scripts/counts.py` renders them into marked regions and `--check`
fails when they have drifted. Two rendering sites, one source of truth, and no way to
forget.

**Things that are settled** - the hash suffix, the dynamic tag values, why `-fno-plt` was
removed - stay hand-written, and should. Generating those would be machinery for nothing.

The same split resolves the duplication question underneath it: the numbers appeared in
both `README.md` and `BACKLOG.md`, which is two places to update and two places to drift.
They now render from one computation.

