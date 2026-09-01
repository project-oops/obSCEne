# D163 - Correction: the blind prober walks the corpus, not the curated census


**Correction: the blind prober walks the corpus, not the curated census, and two entries
above stated its coverage wrong by two orders of magnitude.**

D161 and D162 as first written said a sweep had reached "index 351 of 371" and called it
"95% of the census". Both numbers were fixed on the spot; the reasoning around them was not
affected, but the conclusion a reader would draw from them was completely wrong.

`910-bulk` expands `OBS_CORPUS_CALLABLE_LIBRARIES` **as well as** `OBS_SURFACE_LIBRARIES`
(D114). The curated census in `surface.h` is 371 symbols; the callable corpus is 34,958 mined
symbols less the `Object` and `TLS` entries, which are data and calling one jumps into a
variable. The list is both, and it is **32,466** long.

So index 351 is not 95% of anything. It is **1.1%**.

### How the mistake was available

Two counts, both correct, both called "the census" in different parts of this repository -
one printed by `verify.sh` as `surface.h current (371 symbols)`, the other never printed at
all. The prober reports a bare index and nothing beside it, so there was no denominator on
screen to disagree with the one already in mind.

The fix is that `910-bulk` reports its own size, and this entry is the reason. A number with
no denominator invites the reader to supply one, and the reader will supply the one they last
saw.

It is emitted as a `measure` record with quantity `list-length`, summed from the group table
at run time rather than written as a constant, so it cannot disagree with what the loop
actually walks.

### The record caught the second wrong number within a minute of existing

Writing this entry, the denominator was recomputed by hand - by summing the `N callable`
comments in `corpus.h` - and came to **32,095**. That went into this entry, the worklog, the
sweep script and a message to the other project.

The first run of the new record printed `list-length 0x7ed2`, which is **32,466**. The
difference is 371: the hand count had summed the corpus and forgotten that the same loop also
walks the curated census, which is the *exact* conflation this entry is about, committed a
second time while documenting the first.

That is the argument for the record in its strongest form. A denominator derived by hand from
the sources is a denominator that can be wrong in the same way twice. One taken from the
table the loop iterates cannot.

### What the corrected figures actually say

Encouraging, in fact, and the shape is the interesting part. The dense region is at the
front: 31 rounds to cross the first 351 indices, then **one round covering 367 to 914**.
Faults cluster in `libkernel` and the pthread family, which is exactly where a loader
implements most and therefore has most to get wrong; the long tail of the corpus is largely
callable without incident because most of it is not implemented at all and returns
immediately.

That means rounds-per-index is not a constant and a round budget cannot be estimated from an
average. It also means the early sweeps that looked slow were doing the hard part.

