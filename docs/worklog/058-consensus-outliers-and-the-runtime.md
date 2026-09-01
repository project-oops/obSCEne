# Consensus outliers, and the runtime census


**Reading the 24 outliers rather than only generating them found a defect in the table that
generated them** (D156). Ten were `host alone says fail`, which is a PC being a PC. Twelve
were `900-surface/*` with PS5PCEM standing alone, which is the stub-versus-honest split
restated once per library. Two mattered, and one was that `900-surface/control` **fails on
shadPS4 and fpPS4** - the check whose own message reads "every count in this section is
meaningless" - while the compatibility table printed their census counts beside PS5PCEM's
with nothing to separate them. Marked `(void)` now, not hidden: the 35,337 is itself the
evidence of stub-everything behaviour, and the marker says only that it measures a loader
rather than a platform.

**The runtime census exists** (D157), and one run settled the argument D149 could only make:

| instrument, same shadPS4 run | reported |
|---|---|
| address census | 373 of 373 symbols present |
| `sceKernelLoadStartModule` | `0x80020002` for all five paths |

`0x8002_0002` is `0x8002_0000 \| errno`, so ENOENT - **no such module**, five times,
specifically. The loader says it has nothing while the census on the same run says it has
everything, and only one of those came from the platform being asked.

Getting there needed two exclusions to reach the section at all, both long-known shadPS4
crashes - which is the exclusion walk working exactly as `WORKFLOW.md` describes it.

