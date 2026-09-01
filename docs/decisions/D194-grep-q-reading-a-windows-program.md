# D194 - `grep -q` reading a Windows program deadlocks the harness, and it hid as a timeout bug for hours


The symptom the user saw: a Kyty window sitting on screen doing nothing for ten minutes, against
a stated budget of 100 seconds. The measured version: sweep runs of 278s, 352s and 216s with
`TIMEOUT=100`, then a 60-second run still going at five minutes.

Every wrong diagnosis on the way there was a plausible one. The wait loop recomputes `elapsed`
from the wall clock and tests it at the top, so the loop was correct. `alive` calls `tasklist`,
which costs about two seconds warm and eighty-five cold, so I throttled it to once every ten
seconds - a real improvement that fixed nothing, because the loop was not running slowly, it
was **not running at all**.

`bash -x` with a timestamped `PS4` settled it in one run:

```text
+[1787753398] alive fc_script
+[1787753399] tasklist
+[1787753400] grep -qi '^fc_script\.exe'
```

and then 148 lines of trace and no line 149, ever.

### The mechanism

`grep -q` exits the moment it matches and closes its end of the pipe. A POSIX writer gets
SIGPIPE and dies, which is why this idiom is safe everywhere it is normally written.
`tasklist.exe` is a Windows program: no SIGPIPE, so it **blocks forever** writing the rest of
its output into a reader that is gone. The loop stops mid-iteration and never re-tests its own
budget, so the timeout is not late - it is unreachable.

The same failure hung `apt` for thirty-nine minutes earlier the same day, from the other end:
that time *I* was the reader that went away, via a `| tail -3`. Twice in one day, in opposite
roles, and I did not recognise the second one.

### Fix

Four call sites across `run-emulator.sh` and `sweep-emulators.sh` collapse into one helper that
never abandons a pipe:

```sh
out=$(MSYS_NO_PATHCONV=1 tasklist /FI "IMAGENAME eq $1.exe" /NH 2>/dev/null | tr 'A-Z' 'a-z')
```

`$(...)` reads to EOF, so the writer always finishes. `/FI` makes the output a line or two
rather than two hundred, which also removes most of the cost the throttle was added for.
`MSYS_NO_PATHCONV` because Git Bash otherwise rewrites `/FI` into a Windows path and the filter
silently stops filtering.

Verified: Kyty now stops at 60s against a 60s budget.

### The general rule

**Do not pipe a Windows program into anything that can exit early** - `grep -q`, `head`,
`tail`, `read`. Capture with `$(...)` and match on the string. The failure mode is not a slow
command or a wrong answer, it is a harness that stops measuring while continuing to look busy,
which is the same class of silent measurement loss as D181.

Status: **derived** - mechanism traced, fix measured against the budget it was missing.

