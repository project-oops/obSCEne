# Writing down what was only in someone's head


A pass over the durable records rather than the code, on the principle the repository
already states: the decision records are the memory and the conversation is not.

**`docs/BACKLOG.md` brought up to date.** Two entries claimed problems that had been
solved - including one asserting no emulator implements a write path, which was wrong
rather than stale (the path was there; every import was typed `STT_NOTYPE` so no loader
could match it). Added: the unbuilt NID cracker, the unrendered responsiveness records,
the uncharacterised churn threshold, condition variables and why they were deferred, the
289 names still presence-only, and the absence of version control.

**`CLAUDE.md` gained the procedure for adding a check.** Five steps, three of which are
places the build fails in a confusing way if skipped - a name cannot be in both the
census and `platform.h`, `mkmodule` will not guess a library, and regenerating the census
undoes step one. The fifth is "run it on the host first", which has now caught two wrong
checks: a probe whose two "must differ" inputs did not, and a set of checks ordered before
the capability they required.

Also recorded there: that anything which can block is written as the `try` form or not at
all, and that a looping check reports progress. Both were learned by losing a run.

### Two small things finished

`pretty` now renders the responsiveness map, naming the stubs rather than counting them -
"nineteen stubs" is a statistic, "sqrt, fabs, floor, ceil are stubs" is a list someone can
act on. It prints above the totals, because it changes how every failure below it reads.

`OBS_THREAD_CHURN` is overridable so the crash threshold can be bisected. The first
attempt found nothing and that is recorded as a null result with the reason: twelve runs
at a one-in-seven rate is not enough to see anything, and the next person should budget
twenty per point.

