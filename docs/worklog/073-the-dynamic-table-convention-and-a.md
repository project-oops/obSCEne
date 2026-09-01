# 2026-08-26 - the dynamic-table convention, and a deadlock wearing a timeout's clothes


### `--table current` is finished, and `TABLE` now follows `GEN`

D193's second half landed. The tag renumbering alone had made things worse: prosper began
reporting 352 import libraries where it had reported none, which read as the fix landing and
was not - the counts came from tag records it could now see, while the strings behind them
still resolved against the wrong segment. A plausible number from a wrong address.

The layout was the real half. `build()` computes a `table_base` - 0 under legacy, the last
`PT_LOAD` rounded to the 0x4000 page under current - and emits the table header as a real
`PT_LOAD`. prosper now reads **35,518 imports** and calls it a PS5 layout.

Neither convention is better, which is why the Makefile picks it rather than a person:
`current` breaks shadPS4 and fpPS4 outright (`unsupported dynamic tag 0x02`), `legacy` is
invisible to prosper. `TABLE ?= $(if $(filter 5,$(GEN)),current,legacy)`.

### The surprise: `grep -q` cannot read a Windows program

A Kyty window sat on screen for ten minutes against a 100-second budget. The wait loop was
correct - wall clock, re-tested every iteration - and `alive`'s `tasklist` call was slow but
not *that* slow, so the throttle I added fixed nothing. The loop was not running slowly, it
was not running.

`grep -q` closes the pipe on its first match. `tasklist.exe` has no SIGPIPE, so it blocks
forever writing into a dead reader, and the loop stops mid-iteration and never re-tests its
own budget. Three sweep runs at `TIMEOUT=100` had taken 278s, 352s and 216s; that is what
those numbers were. (D194)

The same failure had hung `apt` for thirty-nine minutes that morning with *me* as the reader
that went away, via a `| tail -3`. I did not recognise it the second time, in the other role.

**It was never only a slowness bug.** With the loop dead, the end-of-report detector never
ran either, so loaders that had finished were recorded as never finishing. Fixed:

| | before | after |
|---|---|---|
| shad | 36,575 records, 278s, `ended no` | 36,574, **13s**, complete |
| cem | 36,432, 352s, `ended no` | 36,432, **16s**, complete |

### A second false zero, from the same run

The sweep printed `0 records` for a Kyty run whose own screenshot read **SUITE COMPLETE,
515 of 515**. Records were extracted from `$log` alone, and Kyty's guest output never goes
there - it goes to the file it holds open, unreadable until the process exits. 36,524 records
were in that file while the table said none.

Zero is the worst value to be wrong about: it reads as a dead loader rather than a missed
file, so the row stated the opposite of what happened while looking like a measurement.
Extraction now reads the watched file too.

