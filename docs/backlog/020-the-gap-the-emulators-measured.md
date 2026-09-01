# 10. The gap the emulators measured


`obscene-tool gap` reads what each emulator registers and subtracts what obSCEne
names. The numbers as of this writing:

| | |
|---|---|
| obSCEne names | 407 symbols across 14 libraries |
| the emulators collectively name | 5,757 across 159 |
| missing from obSCEne | **5,420** |
| of those, with a library shadPS4 supplies | 4,459 - addable to the census mechanically |
| of those, with a published signature and two or more implementations | 592 - addable as behavioural checks |
| libraries with no obSCEne coverage at all | 145, totalling 2,940 symbols |

**This unblocks the census expansion that D055 could not do.** `ps4libdoc` has 42,010
names and no library association, and an import with no library resolves to nothing.
shadPS4's registration macro carries name and library together, so the 4,459 are
placeable today.

### `libScePosix` - done

Five behavioural checks in `017-posix` and the remaining 51 symbols censused (D057).
What follows is the reasoning, kept because it is the argument for what to do next.



Sixty-four symbols, none of them named here, and obSCEne does not touch the library at
all. That is the single sharpest gap in the list, for three reasons.

**The expectations are settled.** POSIX fixes what these do, so every check written
against them carries `OBS_FROM_SPEC` rather than `OBS_FROM_ASSUMED`. Fifty-three checks
in this suite are still assumed and none is `hardware`; this moves that ratio in the only
direction available without the hardware.

**They are a second spelling of functions already checked.** obSCEne calls
`scePthreadRwlockTryrdlock`; `libScePosix` exports `posix_pthread_rwlock_tryrdlock`.
Checking both and comparing is a class of finding nothing else here produces: two entry
points onto what should be one implementation, and a divergence between them is an
emulator bug that no single-path test can see. The same holds for
`posix_clock_gettime` against the kernel's own time calls.

**They fill in what the try-form rule excluded.** The rwlock family here includes
`timedrdlock` and `timedwrlock`, which take a timeout - so they can be called without
risking the hang that made `Lock` and `Wait` unwritable (CLAUDE.md, "anything that can
block"). Condvars and barriers were deferred for exactly that reason and this is the
route back to them.

### After that, by size

The largest uncovered subsystems are `libSceFont` (229), `libSceSsl` (219),
`libSceShellCoreUtil` (217), `libSceHmd` (178) and `libSceHttp` (115). These are census
work rather than check work: their behaviour is not settled by any public document, so an
expectation about them would be `assumed`, and presence is the honest claim.

`reports/gap-analysis.txt` and `reports/gap-checkable.txt` carry the full lists.

This is also the route back to §6d: the timed rwlock calls take a timeout, so they can be
called without risking the hang that put condition variables and barriers there.

### What the numbers are not

The counts for craziiEmu, SharpEMU and rpcsx come from matching quoted vendor names
rather than a registration macro, because those projects have no single shape to match.
That over-counts - a name in a comment or a log line reads the same as an implementation
- so those three contribute to the ranking and never place a symbol in a library. The
rpcsx figure in particular (17) is far too low to be its real surface and should be read
as "this reader does not work on rpcsx" rather than as a fact about rpcsx.

These are also PS4 sources against a PS5 target. Shared surface is large and not total; a
name that does not exist on the target censuses as absent, which is a false negative and
harmless (see `surface.h`).

