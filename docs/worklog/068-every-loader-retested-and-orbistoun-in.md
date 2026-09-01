# 2026-08-25 - every loader retested, and orbistoun in the table for the first time


Six targets, each with **its own build directory and exclusion list** - the `sweep.sh --build`
change, and it paid for itself immediately.

| | records | reached the end | exclusions |
|---|---|---|---|
| host | 36,559 | yes | - |
| shadPS4 | 36,578 | yes | **2** |
| fpPS4 | 36,535 | yes | 44 |
| PS5PCEM | 36,434 | yes | 0 |
| Kyty | 0 | no | - |
| orbistoun | 0 | no | - |

**shadPS4 needs two exclusions and had been running with four**, and at one point with fpPS4's
forty-four. Two contamination routes, not one: across loaders, because the list lived in a
shared directory; and across *purposes*, because `bulk-sweep.sh` writes "this check blocked the
prober" into the same file `sweep.sh` fills with "this check crashes", and afterwards nothing
could tell them apart. Both were reported as crashes on whichever loader ran next.

### orbistoun, first contact

It parses the container, maps two segments, and halts before the thunk table: the bare-ELF
path locates data for none of the six program headers. The module *does* carry a `PT_DYNAMIC`
at `0x67ec10`, inside a LOAD it mapped, so it is the "address could not be located" half of
its message. Its own inspector says `mapped segments []` while the loader path placed two.

Recorded as an observation, not a defect - the same treatment PS5PCEM's host pointers got -
and put to its authors on the bridge, including how they want the row characterised before
this project writes "does not load" about somebody else's work.

The entry in `EMULATORS.md` says the uncomfortable part out loud: orbistoun is the **least
independent** loader in the toolkit. Two projects written in concert agreeing about the
platform is worth less than shadPS4 and PS5PCEM agreeing, and `obscene-tool consensus` already
prints the general form of that warning in its own output.

### Two stale claims corrected

**`EMULATORS.md` said fpPS4 "produces no guest output".** It produces 521 results and runs to
the end. What changed was not fpPS4 - it needs 44 checks excluded to get there, and the walk
that finds them is `sweep.sh`. The loader was reporting all along, behind a check that hangs
at `strlen` in the third section.

**Kyty was retested as a report target, which D080 says it cannot be** - twice, by me, having
not read the decision log. `sceKernelWrite` is a filesystem call there and descriptor 1 returns
`EPERM`; the `puts` fallback is registered under `LibcInternal` while this module imports it
from `libSceLibcInternal`, so it never resolves. Two independent design choices, closed as not
worth a per-loader accommodation. What Kyty is uniquely good for is the inverse: **252 named
missing functions** out of 34,213 imports, including twelve `sceAgc*` entry points, which a
stub-everything loader cannot produce at all.

