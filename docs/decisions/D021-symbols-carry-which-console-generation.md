# D021 - Symbols carry which console generation they belong to

**assumed** · 2026-08-19

Every census group declares `OBS_SHARED`, `OBS_PREVIOUS`, `OBS_CURRENT` or
`OBS_AVAILABILITY_UNKNOWN`, and the `sym` record carries it.

Without it "absent" means two incompatible things: the function does not exist on this
generation, which is correct and not a gap; or it exists and the platform has not
implemented it, which is work. Counted together, the coverage figure means nothing -
the same mistake the census control guards against, one level up.

The census verdict changes accordingly. A library belonging to the *other* generation
scores `skip` rather than `fail` when wholly absent, because scoring it red buries the
real gaps under absences that are correct.

**What it exposed immediately:** of 329 censused symbols, 312 are shared, 17 belong to
the previous generation only, and **none are specific to the current one**. That is the
honest state of this program as a current-generation probe, and it was invisible until
the field existed.

`OBS_AVAILABILITY_UNKNOWN` is offered deliberately. Assuming `shared` for something
uncertain silently reclassifies a real gap as an expected absence, which is the exact
failure this decision exists to prevent.

