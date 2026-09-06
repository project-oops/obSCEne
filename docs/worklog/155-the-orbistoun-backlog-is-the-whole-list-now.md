# 2026-09-03 - the orbistoun backlog is the whole list now

```
backlog 022    4 entries  ->  504 functions, 717 questions
```

Written yesterday as four questions orbistoun could not answer. orbistoun's ledger holds **717
open questions across 504 functions**, so four was a sample, and a console day against it would
have answered four things.

## Joined, not curated

`orbistoun-cli questions` ranks every unverified claim by how often a guest calls the function.
Joined against `platform.h` (callable) and the `OBS_SURFACE_*` lists (censused), that sorts into
three tiers by **what blocks the measurement**:

| tier | blocked on | count | recorded calls |
|---|---|---|---|
| callable | nothing - write the check | 137 | 1,256,209 |
| censused | a signature | 63 | 27,511 |
| absent | a name | 303 | 221,203 |

Tier 1 is an afternoon of checks against functions guests call a million times. D320 has the
reasoning, including why the axis is what-blocks-it rather than which-library.

## The join checked itself, and the first one was wrong

A name cannot be both callable and censused - `surface.h` declares the census as `const char`
precisely so the type system forbids calling it. **Overlap must be zero**, and the first
extraction reported 67: a fact about a loose regex, not about the headers. Tightened to the
`OBS_WEAK` declarations and the `X(...)` census lists, overlap went to zero, and three entries
from each tier were then checked against the headers by hand.

Worth the two minutes. Reporting a tier-1 count of 204 would have promised checks that cannot
be written.

## What landed in which tier, and one that surprised

`_Getpctype` is **absent** - not censused, not declared - and the current run calls it 415
times. `sceKernelDebugOutText` is absent too, at 220,383 recorded calls. The two loudest
functions a guest touches were not on this project's radar at all.

`scePthreadSetprio` and `scePthreadSetaffinity` are tier 2, and they are the only two stub calls
left in PPSA02664's whole run - 2,075 of 2,077 calls reach a real implementation. A lawful
reference unblocks them, not a console run.

## State

Identity scan clean. Nothing committed; the day holds D302, D303, D314, D315, D320, worklogs
153-155, the `matrix` subcommand, the rewritten `decisions` gate, the `doccheck` fix, backlog
022, and nine normalised headings.
