# Numbering, junk names, and what fpPS4 is actually doing


**The decision log had lost the ability to identify a decision** (D142). D102-D114 each
headed two or three unrelated entries, from two numbering streams that had merged without
either noticing. Renumbered the collided entries from D125 up, keeping whichever entry had
the most citations so the fewest references broke - three in source, nineteen in docs, all
repointed by reading what each citing sentence claims. `doccheck.py` now gates uniqueness,
and catches `D<n>` citations naming no entry. Its first version passed while checking only
the last eighty per cent of the file: the early entries carry a title after the number and
the pattern insisted on end-of-line. Both styles now match.

**Four junk rows in the corpus, and a filter that could never have caught them.**
`NOT_A_SYMBOL` gained a whitespace clause to drop SharpEMU rows whose name column had
captured a neighbouring log format string (`sceBgftNotifyExtUsbEjected 0x%08x -`). It
changed nothing, because all four call sites used `.match`, which anchors at position zero -
so the clause could only fire on a name *beginning* with whitespace. Switched to `.search`;
166,960 names became 166,956. The filter had looked correct in review and in the diff.

**The non-platform corpus is not what it looked like.** 7,574 names sit outside
`libSce*`/`libkernel`/`libc`, and the largest groups are Mono and .NET - `mscorlib`,
`libmonosgen`, `libicu`, `I18N*` - plus 176 libraries holding exactly ten symbols each, all
of the form `mono_aot_<Assembly>jit_code_end`. Ten-apiece uniformity across 176 libraries
reads as a parse artefact, and the first conclusion was that these are a title's bundled
runtime rather than console exports. **The firmware disproves it**:
`Sce.PlayStation.Core.dll.sprx.json` declares its library name as `ScePlayStationCore` and
exports those symbols, so the attribution is the vendor's own and the names are real. They
are also all `Object`, so D141's callable-only split already keeps the prober off them.
Recorded because the wrong conclusion was reached first and the data settled it.

**fpPS4 hangs on `strspn`, and record count is not progress.** The stopping point is
`007-responsive/libc`, immediately after `strcmp` - `strspn` is the next entry in the table
and never returns. A 300-second run and a 5400-second run both produced **33 records and
stopped in the same place**, which is what settles hang against slow: eighteen times the
budget bought nothing.

Recorded because the wrong conclusion was reached first and written down here as fact. The
reasoning that produced it: runs stopped at 24, 25 and 33 records, and the 33 came from the
round with *no* exclusions while 24 came from a round with two - so the shorter list looked
like it got further, which reads as variance rather than a crash walk. **It is neither.**
Excluding a check replaces its several `responsive` records with a single `skip`, so the run
that progressed furthest through the suite emitted the fewest records. The counts moved
opposite to the progress they were being read as.

The HUD says what the count cannot: `SECTION 4 OF 26  CHECKS 11 OF 148`. That is the measure
- `checks n of m`, not `wc -l` - and it was on screen and in hand the whole time the wrong
conclusion was being formed from the record totals.

What this needs is the exclusion walk after all, and it is now `sweep.sh`'s guard that stands
in the way: it refuses to exclude on a timeout, correctly, because a timeout cannot be told
from a hang *within one round*. Two rounds can - a stopping point identical under a doubled
budget is a hang - and that is the shape of the fix, rather than an operator asserting the
budget was generous enough.

`sweep.sh` now forwards `CORPUS` to the build the way it already forwarded `GEN`. A sweep is
a loop of builds and runs, and the thirty thousand corpus targets are dead weight in every
round of a hunt whose quarry is in the hand-written checks.

**The corpus now records what it read** (D143). `data/mined-names.txt` and
`data/unnamed-nids.txt` carry a `mined-from:` line of `<source>@<commit>` for all eight
emulator checkouts and a `firmware:` line of the 23 versions, and `mine-nids.py --check`
compares them against disk without re-mining. `verify.sh` runs it beside the generated-census
check, which gated headers-against-data and left data-against-emulators unwatched. Noted in
both the script and the decision: the build VM has no emulator checkouts mounted, so the gate
skips there every time and only bites on the host. Real cover, in one place only.

**The sweep can now finish against a loader that hangs** (D144). A timeout doubles the
budget and retries the same build; only a stopping point identical under twice the time is
called a hang and excluded, and one that moved means the budget was short - the doubled value
is then kept for the rest of the sweep. Retries count against `--max-rounds`, since a retry
is a run. This is the fpPS4 diagnosis promoted from something an operator had to remember
into something the loop does.

Also recorded there, because it produced a wrong conclusion from evidence already on screen:
**a record total is only comparable between runs built from the same exclusion list.**
Excluding a check swaps several records for one `skip`, so the run that got furthest emitted
the fewest records. `checks n of m` on the HUD is the measure.

`sweep.sh --resume` keeps the exclusion list a previous sweep built instead of truncating it.
Each exclusion costs two runs, so a sweep stopped at `--max-rounds` mid-hunt has bought
findings that are expensive to buy twice - seven of them and ninety minutes, against fpPS4.
Off by default: the first pass must start empty or a stale exclusion hides a check that no
longer crashes, and the report goes on reporting `skip` for something that would now pass.

The seven hangs found so far are worth reading as a group - `007-responsive/libc` (`strspn`),
`007-responsive/math`, `015-sync/machine-kind`, and then `017-posix/page-size`,
`signal-sets`, `short-sleep` and `rwlock` back to back. Four consecutive POSIX checks that
never return is a characterisation of the loader, not four separate accidents.

Considered and rejected: a watchdog inside obSCEne, so a hanging call could be survived and
no exclusion walk would be needed. It would have to be built on a thread and a timer - which
are among the things under test - so it would depend on the primitives it exists to measure,
and would fail exactly where it is needed. The `try` form plus exclusion stays the design.

**Section-level exclusion** (D145). `EXCLUDE="035-libc"` now matches every check in the
section; an entry containing `/` still means one check, and the two mix freely. Added because
the fpPS4 sweep spent forty runs discovering, one check at a time, that the loader returns
from nothing in `035-libc` - a fact one entry states, and states better than twenty skips
that invite the reader to look for twenty causes.

Not applied to fpPS4's list yet, and that is the point of the decision: the sweep proved
twenty of the section's thirty checks hang and hit its round budget before reaching the other
ten, so a section entry would report "known to end the process" for checks nothing has run.
`017-posix` shows the same caution earning its keep - five of its checks hang and the sweep
walked past the rest, so collapsing it would have buried working checks. Resumed the sweep to
settle the remaining ten by measurement rather than by inference.

**fpPS4 completes.** Round 35 of the sweep ended `COMPLETE` at **742 records**, 44 exclusions
in. The walk that started at 33 records and looked like a slow loader was a chain of
forty-four hangs, each confirmed by the doubled-budget retry.

The finished walk settles D145's open question, and settles it both ways:

| section | skip | pass | partial |
|---|---|---|---|
| `035-libc` | 22 | 6 | 2 |
| `037-math` | 13 | 0 | 0 |

**Eight of the thirty `035-libc` checks work on fpPS4.** Collapsing that section to one entry
- which the twenty consecutive exclusions made look obviously right, and which a "collapse
after N in a row" heuristic in `sweep.sh` would have done automatically - would have reported
eight working checks as known to end the process. `037-math` is the case that genuinely is a
section: all thirteen, no survivors.

So both halves of the decision are now measured rather than argued. The caution was worth
having, and the section form is worth having; what would have been wrong is letting the sweep
apply it on a pattern instead of on a completed walk.

