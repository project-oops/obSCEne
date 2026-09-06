# D320 - the orbistoun backlog is generated and tiered by what blocks it

**decided** - 2026-09-03

`docs/backlog/022` began as four hand-picked questions orbistoun could not answer. orbistoun's
own ledger has **717 open questions across 504 functions** - so four was not a backlog, it was a
sample, and a console day spent against it would have answered four things.

Rewritten to carry the whole list, because the point of writing a gap down is that one sweep
closes it.

## Not curated - joined

The list is `orbistoun-cli questions` (every unverified claim in its knowledge base, ranked by
how often a guest actually calls the function) joined against what this repository can call.
Naming the command matters more than the snapshot: a hand-typed list is stale the day after it
is written, and nobody can tell which entries went stale.

## The three tiers, and why that is the right axis

The axis is **what blocks the measurement**, not what the function does:

| tier | blocked on | count | recorded calls |
|---|---|---|---|
| callable | nothing - write the check | 137 | 1,256,209 |
| censused | a signature | 63 | 27,511 |
| absent | a name | 303 | 221,203 |

Grouping by library or by subsystem would have read better and planned worse. These three want
different work from different people: tier 1 is an afternoon of checks, tier 2 needs a lawful
reference confirmed against the vendor form, tier 3 needs names added to `data/surface.txt`
where a wrong one is a harmless false negative (D014) and costs nothing.

**The join validates itself.** A name cannot be both callable and censused - `platform.h`
declares functions, `surface.h` declares the same identifiers as `const char` so the type system
forbids calling them - so an overlap of zero is the check that the extraction is right. The
first attempt reported an overlap of 67, which was a fact about the regex and not about the
headers.

## Two things to settle before the sweep, not after

The return width (orbistoun's decision 398) and the out-parameter poisoning (D303) are
properties of *how every check reports*, not of any one check. Retiring them first means the whole of tier 1
is measured once rather than measured and then re-measured. Both are named at the bottom of the
backlog for that reason.
