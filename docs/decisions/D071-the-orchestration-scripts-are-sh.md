# D071 - The orchestration scripts are `sh`. PowerShell was solving one environment variable


Status: decided, correcting D050.

D050 argued the split: `sh` for building, PowerShell for running, "because the emulators
are Windows applications". That reasoning does not hold - what language drives a process
has nothing to do with what platform the process targets, and Git Bash launches a Windows
executable perfectly well.

**The real cause was one line.** Git Bash rewrites anything path-shaped before a Windows
program sees it, so `--working-directory /home/ubuntu/obscene` reached multipass as
`C:/Program Files/Git/home/ubuntu/obscene`. CLAUDE.md said "use PowerShell for multipass
invocations", which worked and hid the cause behind a language choice. The fix is
`MSYS_NO_PATHCONV=1`.

**And PowerShell brought a hazard of its own.** It turns a native command's stderr into a
terminating error, so multipass warning about a file it had just copied correctly killed
scripts - intermittently, which made it read as a new fault every time. That needed a
shared helper module to work around (`vm.ps1`), and the whole of it is deleted by not
using PowerShell.

One hazard survives the move and is worth naming: multipass copies to an NTFS target
correctly and then exits non-zero setting POSIX permissions. The scripts tolerate it and
assert that the file exists, which is what they actually want to know.

Four scripts converted, `vm.ps1` deleted, `scripts/` is one language again. The bash sweep
reproduces the same three rounds and the same 680 records.

