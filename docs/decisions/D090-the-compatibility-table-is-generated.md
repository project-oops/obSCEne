# D090 - The compatibility table is generated from reports, because a hand-written one would be stale by tomorrow


Status: decided.

`docs/EMULATORS.md` said what each loader *is* and nothing about what each loader *did* -
which is the question anybody arrives with, and the one whose answer changes every time the
suite grows or an emulator is updated.

`scripts/compat.py` renders it from the reports themselves into a marked region, and
`--check` runs in `verify.sh`. Same contract as `counts.py`, for the same reason (D069).

**It does not rank loaders and says so.** A pass count is not a quality score: a loader
resolving everything to a stub scores well on presence and badly on behaviour, and one that
refuses to load scores nothing while being the most honest of the three. Per-section
tallies are kept rather than merged, because "fails everything in one section" and "fails
one check in each of eight" are different platforms.

**Kyty is included despite producing no records**, because "produced nothing and did not
reach the end" is a fact about a loader and belongs in a compatibility table rather than
only in prose.

### And it repeated a mistake I had already fixed once

The first version listed `000-boot` among the sections the loaders disagree on - with both
reporting loaders showing exactly `4/0/0/0`. Kyty had no entry for it, and its *absence*
was being counted as a differing opinion.

That is precisely the skip-handling bug found in `obscene-tool consensus` four hours
earlier (D072), in a different file, written by the same hand, after writing the decision
record explaining it. **Absence is not an opinion** appears to need enforcing rather than
remembering, and both places now say so in a comment pointing at the other.

