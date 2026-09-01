# D199 - Five scripts were still driving a build environment that had been deleted that morning


multipass was dropped on 2026-08-26 and `CLAUDE.md` says so in bold. `sweep-emulators.sh` was
ported the same day; `bulk-sweep.sh`, `sweep.sh`, `repeat.sh`, `run-emulator.sh` and
`run-kyty.sh` were not, and every one of them still called `multipass` in live code.

Found while trying to do backlog item 6, which is described there as "pure machine time, no
judgement - the best thing to leave running unattended". It could not run at all. The item was
not blocked on machine time; it was blocked on a script calling a program that is not
installed.

**Nothing failed loudly enough to notice.** A missing `multipass` produces a message about a
missing command, not about a decommissioned build environment, and none of these five run
inside `verify.sh` - so the gate stayed green over five broken scripts for a day. That is the
house failure again: a mechanism that reads as working while being wrong (D158, D163, D194).

### One shim, not forty rewrites

`scripts/wsl.sh` translates the four multipass shapes these scripts already use:

```text
vm exec NAME [--working-directory DIR] -- bash -lc "CMD"
vm transfer NAME:SRC DEST
```

Rewriting every call site would have been forty chances to change one by accident; translating
four shapes in one place is one chance. The shapes also carry meaning worth keeping -
`--working-directory` says "in the repository", `transfer` says "get this file out" - and an
unrecognised verb is **refused rather than approximated**, because a shim that quietly does
something nearly right is worse than one that stops.

### The bug the test caught, which is the reason to test a shim at all

`VM_MODULE="${VM_MODULE:-$HOME/obs/obscene.module.elf}"` expands `$HOME` in **Git Bash**, not
in WSL. It becomes `/c/Users/<name>/obs/...` - a Windows home with no module in it - and the
failure then reads as a missing build rather than a mangled path. Escaped to `\$HOME` so the
inner shell expands it, which is what `sweep-emulators.sh` already did and why comparing
against the working example was worth the minute.

All four shapes verified end to end: exec with and without a working directory, a 12,977,480
byte transfer, and an unsupported verb refused.

### The `|| true` at every transfer site is kept deliberately

multipass copied files correctly and *then* failed setting POSIX permissions on NTFS, so every
call site ignores the exit code and tests for the file instead. WSL does not have that fault -
but "test for what you actually want rather than trusting an exit code" does not stop being
right when the tool improves, and today produced three separate bugs from trusting a pipeline's
status. The tests stay.

Status: **derived** - every script checked, every shape exercised.

