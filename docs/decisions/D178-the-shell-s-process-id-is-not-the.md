# D178 - The shell's process id is not the loader, and the runner had already learnt that once


`run-emulator.sh` launched a loader in the background, waited on `kill -0 "$emulator_pid"`,
and killed the same id when the budget ran out. For shadPS4 and PS5PCEM that works. For fpPS4
it never has, and the failure is silent in every direction.

Measured, from a clean baseline of zero fpPS4 processes:

```
$ fpPS4.exe <module> & pid=$!
$ sleep 8
$ kill -0 $pid   -> fails: the shell says it is gone
$ tasklist       -> two fpPS4.exe processes running
```

fpPS4 re-execs. The process the shell is holding exits immediately, and the loader that
matters is one the shell never knew about. So:

- the wait loop falls through in about a second, whatever `TIMEOUT` says
- `timed_out` comes out **0** on a run that never finished - the opposite of the truth, and
  `timedout` exists precisely to keep that confusion out of the record
- the final `kill` is addressed to a process id that no longer exists
- **the loader keeps running**, holding its window and writing to the report file

The next run then starts on top of it. Two windows, two runs interleaving records into one
file, and a resume state derived from the blend. From outside it looks like a hang: the
abandoned window sits at whatever section it reached and never moves - reported here as
"stuck on 9/27", which was `SECTION 9 OF 27` on an orphan nothing owned.

### The part worth keeping

**This exact trap was already documented in this file, and fixed in the other place it
occurs.** From the screenshot path, thirty lines above:

> `$!` under Git Bash is a *bash* process id, and the Windows tooling that owns window
> handles knows nothing about it - so passing it looked correct and found no window every
> time. The executable's basename is what Windows calls the process.

The lesson was learnt, written down, applied once, and not carried the twenty lines to the
run loop. A note next to the fix is not a fix.

### What replaced it

`emulator_running` asks **both** views and believes either: `kill -0` for loaders whose pid
is real, and `tasklist` by executable basename for loaders whose is not. `emulator_stop` kills
through both. Neither alone is right for every loader in the kit, and picking one was the
original mistake.

A pre-launch guard refuses to start on top of an existing instance, because a leftover is not
a harmless duplicate - it shares the report file, so the extraction merges two attempts and
the resume state is computed from the mixture.

### The convergence loop checks its own timeout

A separate hazard the same session nearly walked into. Runtime resume marks a check as a
blocker when its `try` has no `res`, and a run killed mid-call leaves exactly that trace - so
a timeout short enough to interrupt a healthy check would exclude it permanently, on a lie.

The 44 checks fpPS4 needs excluded are already known, having been found the slow way by
rebuilding. So each round now compares the blocker it just discovered against that list and
flags anything outside it. It costs one `grep` per run and it is the only thing standing
between an unattended loop and a quietly wrong exclusion list.

Status: **decided** - the fix is measured, and the negative test is above.

