# Kyty, patched to see how far it would go


The question was whether obSCEne is at fault for calling too many unsupported functions,
whether it could be defensive, and whether patching Kyty locally was reasonable. Answered by
doing all three, and the premise was wrong in a way worth stating first: **it was never "too
many". It was one, before a single check ran.** obSCEne died at `puts` - the second channel of
its own report writer, reached because `sceKernelWrite(fd=1)` returns `EPERM` on Kyty.

Reading the mechanism paid for itself. The obvious edit is Kyty's `RelocateHandler`, which is
what the error names - and it would have produced a crash somewhere unrelated, because that
function is a PLT0 resolver entered by `jmp` with two words already pushed, so letting it
return leaves `rsp` two words out. The right place was the generated trampoline one level
down, which the guest enters by an ordinary `call`. Three bytes: `xor eax,eax; ret`.

| | records |
|---|---|
| stock | **1**, dies at `puts` |
| trampoline returns 0 | **14**, dies at `080-video/open` |
| + video-out user id relaxed | **102**, dies at `015-sync/mutexattr-round-trip` |

Stopped there, and the number that decides it is **1,970 `EXIT_NOT_IMPLEMENTED` sites and 207
`EXIT` calls**. Kyty aborts on anything it does not handle; a probe that sweeps input spaces
meets those constantly. Patching past all of them would leave every unhandled case continuing
with default state - the stub-everything result `900-surface/control` exists to mark `(void)`.
So it is a design mismatch and neither side is wrong. (D176)

### Surprise: the patched build is a bug-finder, not a measurement platform

Three findings in about fifteen minutes, and one of them is about the **platform** rather than
about Kyty:

- `UserServiceGetInitialUser` returns `1`; `VideoOutOpen` refuses anything but `255` or `0`.
  One build, two functions, mutually exclusive - so the documented sequence cannot succeed.
- `PthreadMutexattrSettype` calls `EXIT` on an unrecognised type where POSIX says `EINVAL`.
- **The mutex type constants are one-based and are not the POSIX values.** Kyty maps
  `{1,2,3,4}` and rejects `0`, which is POSIX `PTHREAD_MUTEX_NORMAL`.
  `015-sync/mutexattr-round-trip` had been sweeping `0..3` - a quarter of its range on a value
  one implementation refuses, and never trying one it accepts. Widened to `0..4`. (D177)

The host build was left alone deliberately: its stub passes straight through to real glibc,
so "4 accepted, type 4 refused" is a true statement about POSIX rather than a rehearsal of the
same guess. That is the D166 trap avoided rather than re-entered.

### Surprise: shadPS4 needs no build-time exclusions, and this was nearly misread twice

D175 recorded "zero exclusions". Testing it against a fresh module produced 142 records and a
hang, which looked like a regression from the morning's screen changes - the 01:32 module
still completed in 6s, which seemed to confirm it. It was a flake. Rerunning gave a different
failure, and then the actual mechanism became visible:

| run | died at | records |
|---|---|---|
| 1 | `015-sync/condvar-wakes-a-waiter` (flake) | 142 |
| 2 | `040-file/open-rejects-null` | 316 |
| 3 | `080-video/flip-rate-rejects-bad-handle` | 349 |
| 4 | - **completed in 5s** | 36,578 |

That is D172's runtime resume working end to end on a real loader: one binary, no build-time
exclusions, each run skipping what the last one died inside. "Zero exclusions" is right; the
missing word was *build-time*.

Which exposed a table that lied by omission - `Excluded to keep the loader alive: 0` reads as
*ran everything* when four checks were skipped. Added a `Skipped after dying on a previous run`
row, counted separately because the provenance differs: a build-time exclusion is this
project's judgement, a runtime one is a dangling `try` in a report, which is evidence.

### Retested with the corrected module, unchanged

craziiEmu and ChonkyStation4: still 0 records, consistent with their documented reasons
(craziiEmu resolves zero imports; ChonkyStation4 wants firmware). The `e_type`/`DT_INIT` fixes
did not reach them. Two minutes well spent to close the question.

### And a fifth instance of the recurring failure, this time mine

`obscene-tool compat` exits non-zero and prints **nothing** when given neither `--write` nor
`--check`. The run looked like it had regenerated the table; it had not, and the stale table
was then read out as the current result. Two lessons, both now in `BOOT.md`: the tool names
the missing flag, and the output had been piped through `tail`, which discarded the exit code
along with the silence. A diagnostic destroyed while looking at it is worse than one never
produced, because the absence reads as a clean result.

### State

Patches stored in `patches/` with the reasoning, so they can be reproduced or refused; Kyty's
tree left pristine. `reports/kyty.txt` holds the **stock** result (1 record) and
`reports/kyty-patched.txt` the other - a row labelled `kyty` describing a build only this
machine has would be the same lie as an invented constant, at a larger scale.

fpPS4's 44 build-time exclusions are being rediscovered at run time by the same convergence
shadPS4 just demonstrated. Machine time only, roughly four minutes a run.

