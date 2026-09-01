# D174 - The screen said nothing about what it was doing, and the first fix for that made the probe worse


Three faults, found by watching PS5PCEM sit on one page and asking why.

### The page cycling stopped, and the reason was in our own reports

`obs_screen_present` paced its detail pages with a single `sceKernelUsleep(3000000)` - three
seconds. On PS5PCEM the screen froze on whatever page was up.

The evidence was already recorded here: `050-time/usleep` sleeps **2 ms** and passes on
PS5PCEM, while `120-measure/sleep-fidelity`, which asks for longer, is excluded there as
*known to end the process*. We were asking for 1500× the duration known to work.

Pages now wait by repeating the 2 ms sleep. Same total time where either works; keeps
advancing where only the short one does. **This is presence-versus-behaviour inside our own
display loop** - the guard above it tests that `sceKernelUsleep` *exists*, which says nothing
about whether it returns.

### "Is it stuck or finished?" was unanswerable from the screen

`REPORT COMPLETE` appeared on page 0 only, so a detail page said nothing about whether the
suite had ended. Someone looking at page seven of fifteen could not distinguish a completed
run cycling its results from a run stopped dead - and when a page sits unchanged, the honest
reading is the second.

Every page now carries `SUITE COMPLETE - IDLE, CYCLING RESULTS`, and the header carries
`RUNNING <id>` while a check is in flight. These pages are only ever drawn after the suite
ends, so the word is a constant in the code - which is exactly why it is worth printing. The
reader does not know what the code knows.

### And the in-flight indicator, done wrong first

The first version redrew the screen inside `obs_screen_attempt`, so the name was on screen
*during* the call. `obs_screen_redraw` ends in `obs_display_flip`, so that added a full present
per check - 515 of them - and **shadPS4 began dying in `120-measure`, three checks it had never
crashed on before**.

A probe that perturbs what it measures is worse than one that reports less. The id is recorded
without redrawing; the screen is redrawn at every section boundary anyway, so a hang is
narrowed to its section rather than its exact check. That is the trade, and it is the right way
round.

### Waiting for the end record instead of the budget

`run-emulator.sh` only stopped on its timeout, because a module that draws its report never
exits - it cycles pages for as long as it is alive. So every successful run cost its whole
budget: shadPS4 finishes the suite in about a minute and was being given two hundred seconds,
multiplied by every sweep round.

The report says when it is done. The screenshot path had watched for `OBS|end` for a long time
for exactly this reason; it simply was not applied to the run.

**And it had never worked there either.** Both used `grep -q "^OBS|end"`, anchored - but
records arrive *embedded in a loader's own log lines*, which is why the extraction at the
bottom of the same script uses an unanchored `grep -o 'OBS|.*'`. The anchored form matched
nothing, so the screenshot always landed late and the wait always ran full term.

Unanchored, a shadPS4 run went from **3m32s to 51s**, finishing on the end record after five
seconds. A run that ends on the record is reported as *not* timed out however long it took to
get there - reporting otherwise would make `sweep.sh` exclude a check that finished.

