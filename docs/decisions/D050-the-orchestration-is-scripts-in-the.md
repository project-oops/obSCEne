# D050 - The orchestration is scripts in the repository, not steps somebody remembers


Status: decided, late.

Everything in `scripts/` was build-side. Running a module in an emulator, pulling the
report out of a stream it shares with the emulator's logging, looping until nothing kills
the process, harvesting NID pairs from logs - all of it was done by hand, twenty-odd
times, and none of it was written down. The analysis tooling was strong and the
orchestration existed only in whoever had been doing it.

That is the same failure this project keeps designing against: `docs/DECISIONS.md` exists
because the conversation that produced a decision does not survive, and a remembered
procedure is no better.

Three scripts now: `run-emulator.ps1`, `sweep.ps1`, `harvest-nids.ps1`.

**They are PowerShell, and that is not a preference.** The module is built inside a VM
because a Windows mount cannot carry the execute bit (D012), and the emulators are Windows
applications. `sh` for building, PowerShell for running, and the split is where the work
is rather than a style choice.

Two environment hazards are recorded in `docs/WORKFLOW.md` because both cost time and
neither is guessable: `build/` can end up owned by the VM and unwritable from the host,
and Windows PowerShell turns a native command's stderr into a terminating error, so a
`multipass transfer` that succeeds with a warning ends a script.

