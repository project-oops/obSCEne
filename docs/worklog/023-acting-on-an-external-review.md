# Acting on an external review


Two reviews arrived from outside this thread. Between them they found one thing that
undermined the program's central claim, one that explained why nothing else had been
caught, and a strategic argument that changed the backlog's ranking.

### The gate could not fail

`verify.sh` always exited 0 - four separate ways, each looking reasonable alone (D065).
`lint.sh` printed `clean` when clippy failed to run at all.

That is the root cause of most of the rest. **Every "verify.sh green" recorded in this log
before now meant only that the script reached the end.** The real failures caught today
were caught by reading the output, which is the opposite of what a gate is for.

Both rewritten so no filter sits between a command and its status. `cargo fmt --check`
added, because CI ran it and nothing local did, so the tree had drifted out of format -
three diffs, all in the file edited most today.

### The rule from this morning, violated thirty-eight times

D058 was written today: a check that calls a symbol other than its own must test that
address first. It was added to CLAUDE.md, applied to the one check that prompted it, and
never checked against the existing tree. Thirty-eight checks violated it (D066).

The crash risk is the smaller half. `015-sync/event-flag-round-trip` announced
`sceKernelPollEventFlag` and called `sceKernelCreateEventFlag` as its **first statement**;
had Create been the null one, the report's last line would have named a function never
reached. Announce-before-attempting producing a confident lie.

`OBS_REQUIRE(&sym, ...)` states the requirement in one line, and `scripts/guards.py`
derives it independently and fails the build. In `verify.sh` and in CI.

Second time in one session that a written-down rule went unenforced - the `multipass`
stderr hazard did the same thing four hours earlier. The lesson is not "be more careful".

### The display had the same defect, in the worst place

`obs_display_open` guards six symbols and called `sceKernelGetDirectMemorySize` inline as
an argument to a seventh. It is the **first** platform interaction in the program, ahead
of the boot section, so a null there ended the run with `OBS|build` as the last record: no
try, no section, nothing naming the display.

Guarded, and it now emits an `opening` record before attempting, so an `opening` with no
outcome after it names the display as the cause.

### The screen was silently losing its totals

Row spacing was a constant 46 pixels, chosen when there were fifteen sections. At
twenty-one the totals row lands at y=1184 on a 1080-high framebuffer, and
`obs_display_rect` clips rather than faults - so the totals and footer were drawn into
nothing and the last section ran off the bottom. Confirmed in the committed screenshot.

The worst way for this to fail: the screen exists for when the stream cannot be read, so
one that silently omits the totals shows a confident, incomplete answer. Spacing is
computed from the sections present now.

### Frontier instead of a sum

"65 pass, 6 partial, 38 fail, 7 skip" has no denominator and throws away the dependency
structure the sections are deliberately ordered by (D067). `OBS|frontier` reports
capabilities established, **checks that never ran because one was missing**, and the
deepest wholly-green section - measured over the capability graph rather than section
order, which is a reading order.

On the host: `frontier|6|3|19`.

### 018-relational - the move worth making

The best point in either review: `007-responsive` compares results to each other rather
than to an expected value, which is oracle-free testing and the escape from having no
hardware - and it was aimed at 54 libc and maths symbols, exactly where ISO C already
gives a free oracle (D068).

Five checks now aim it where nothing supplies one: distinct handles, handle reuse after
delete, memory released and reallocated, a semaphore that counts, a stable thread
identity. None needs a struct layout or a documented error code, so they route around
BACKLOG §2 and §5 rather than waiting on them.

**Validated on the host, which meant writing real event flags and semaphores into
`host_stubs.c` rather than constants.** A check that has never passed a working
implementation is not evidence. Four pass, one skips honestly.

### And the ranking was wrong

§10 - 4,459 mechanically addable census names - was first in the backlog. It is the
largest number available and close to the least valuable: an emulator author already has
that list, perfectly and free, from their own stub log; 4,800 presence records against 106
checks makes the report 98% inventory; and presence is not even a clean measurement, since
shadPS4 stubs everything.

Moved to last. Relational checks and N-way consensus are first now.

### Also corrected

- `make target` does not exist and never did. README, the Makefile's own header, a
  decision record and CI all called it - one of three reasons CI could not pass.
- `diff` printed "0 regressed" and exited 1: `has_regression` counted `now_absent` and
  `regression_count` did not.
- `docs/OUTPUT.md`'s record table was wrong for four kinds and missing five. It is a
  declared contract, so that mattered.
- The README's status line said "loads into a real emulator without executing", two suite
  generations stale - it misled one reviewer into three findings about a solved problem.
- CLAUDE.md's "no floating point, no variadics" read as though it covered the checks,
  making `037-math` and `035-libc/snprintf` look like violations. It means the runtime.
- A 6 MB `display.png` and three emulator litter directories at the repository root, none
  ignored.
- One claim in the README was an overclaim: "nothing in it is emulator-specific" is false
  three ways (`EXCLUDE`, `GEN=4/5`, `puts` before `write`). Named rather than dropped.

### What the review got wrong, for the record

The check count was given as 116; it is 106. And three findings - `module_start`, entry
stack alignment, the `e_type` doc - rested on the stale README status line rather than on
the code. Which is the documentation-drift finding proving itself.

---

