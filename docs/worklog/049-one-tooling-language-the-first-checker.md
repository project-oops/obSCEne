# One tooling language: the first checker moved to Rust


Five languages, and only one of them removable. C is the product and freestanding by
mandate; GLSL is forced by the GPU probes; sh is glue and a documented choice over
PowerShell; Rust is already paid for at 8,194 lines, 17 subcommands and 133 tests. Python's
4,358 lines are the only ones whose job another language in this repo already does.

Python is not a build dependency - the Makefile never invokes it - so `README.md`'s "the
only build dependency is clang" is honest. It is a *verification* dependency, which is worse:
the gates were the only part of this tree written without tests or types.

**`scripts/guards.py` is now `obscene-tool guards`, and the port paid for itself before it
was finished.** Run side by side against the same tree the two disagreed: 128 checks against
135. Neither was right.

- The Python required the symbol column to match `"(\w+)"`, so it silently skipped **eight
  real rows** - every check whose symbol is descriptive rather than an identifier:
  `"(self-check)"`, `"mapped memory"`, `"(census control)"`, `"(compute dispatch)"`,
  `"(every censused symbol)"`. Among them `910-bulk/probe`, the blind prober, which calls
  arbitrary censused symbols and is the check a guard rule least wants to miss.
- The first Rust version over-matched in the other direction, counting
  `obs_report_progress("910-bulk/probe", ...)` as a table row because it looks like one.
- Both assumed every runner is named `check_*`. The blind prober's is `run_bulk`. The runner
  is a *positional* field; a naming convention is a habit, not a contract.

Anchoring on `OBS_CAP` - the token that actually makes a row a row - gives **136 of 136**,
matching an independent count. Nine tests now pin each of those failures, including that a
symbol named in a comment is not a call, and that division is not the start of one.

Neither Python fault could ever have appeared as a failure. Both appeared as a smaller
number nobody was comparing against anything, which is this project's most-repeated defect
and the whole argument for the move.

**And `scripts/lint.sh` had the `.` bug.** `. "$HOME/.cargo/env" 2>/dev/null || true` -
a POSIX special built-in that fails exits the shell before `||` is consulted, so on a machine
without that file the script exited with no output and status 1, indistinguishable from a
tree full of lints. `verify.sh` was fixed for exactly this months ago; this file was never
run on the host, so it kept the bug. Found by running it on the host.

Sixteen Python scripts remain. The order is checkers, then analysis, then generators.

