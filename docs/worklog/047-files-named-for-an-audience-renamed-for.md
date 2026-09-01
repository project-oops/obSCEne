# Files named for an audience, renamed for their content


Two handover documents were named for who they were written *to*, which is what made them
age: a handover is addressed to a moment, and reads as stale the day after it is read.
Neither one's content was perishable, though - only the framing.

- `HANDOVER-ORBISTOUN.md` → **`docs/LOADING.md`**, "Loading an obSCEne module: what a loader
  has to handle". Every observation in it came from several loaders and applies to any of
  them; nothing in it was ever specific to the one that prompted writing it down.
- `HANDOVER-ORBISTOUN-NET.md` → **`docs/CLIENT.md`**, "Writing a client for the obSCEne
  protocol". `PROTOCOL.md` is the specification - what the wire carries; this is the other
  half - what a client author has to do about it. Consumer-specific references removed:
  the file now names no project but this one.

Kept as separate files rather than folded into `PROTOCOL.md` and `MODULE-FORMAT.md`. Spec
and guidance are different documents with different readers, and merging 365 lines of client
guidance into a 441-line specification produces something nobody reads end to end.

**`docs/README.md` listed five documents out of seventeen**, which is the more damaging
version of the same problem. An index reads as complete, so a reader who does not find a
subject in it concludes the project has nothing on the subject - two thirds of the
documentation was invisible to anyone starting where the index sent them.

Rewritten in full, and **gated**: `doccheck.py` now fails if a document exists without a
pointer, or if a pointer names a file that does not exist. The second direction is the one
that catches a rename leaving a dead link in the first document anybody opens - exactly what
today's two renames would have done. Shown to reject in both directions before being
believed.

