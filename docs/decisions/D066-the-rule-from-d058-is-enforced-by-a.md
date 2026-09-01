# D066 - The rule from D058 is enforced by a script now, because writing it down did nothing


Status: bug, thirty-eight instances.

D058 was written after a null `scePthreadRwlockInit` took the host build down mid-check:
a check that calls a symbol other than its own must test that address first. It was added
to CLAUDE.md, applied to the one check that prompted it, and **thirty-eight existing
checks violated it** - including `035-libc/malloc-free`, which guards `malloc` and calls
`free` unguarded.

The crash risk is the smaller half. A check announces its table-row symbol before running;
`015-sync/event-flag-round-trip` announced `sceKernelPollEventFlag` and called
`sceKernelCreateEventFlag` as its first statement. Had Create been null, the last line of
the report would have named a function that was never reached - announce-before-attempting
producing a confident lie, which is the failure the whole program is arranged to prevent.

`OBS_REQUIRE(&sym, ...)` marks the requirement in one line, and `scripts/guards.py`
derives the check's declared symbol from the table, the symbols its body calls from the
source, and fails when the difference is not guarded. It runs in `verify.sh` and in CI.

**The lesson is not "be more careful".** It is that a rule with no enforcement is a
comment. This is the second time in one session: the `multipass` stderr hazard was written
into D050 and then broke `sweep.ps1`, in a script that had not been given the fix.

