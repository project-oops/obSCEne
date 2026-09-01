# The orchestration got written down


The analysis tooling was in good shape and the *running* of it was not written down
anywhere. Build a module, fetch it out of the VM, start an emulator, pull the report out
of a stream it shares with the emulator's own logging, notice which call did not return,
exclude it, go again - done by hand twenty-odd times and living entirely in the head of
whoever was doing it.

That is the exact failure `docs/DECISIONS.md` exists to prevent, applied to procedure
instead of reasoning, and it went unnoticed for as long as it did because the part that
was written down was working well.

Three scripts (D050):

- `run-emulator.ps1` - one run, module to report.
- `sweep.ps1` - rounds until nothing kills the process.
- `harvest-nids.ps1` - grows the NID table from logs.

**Measured, not asserted.** `run-emulator.ps1` gave `Records: 554, Ended: True,
Crashes: 0`. `sweep.ps1` went 180 → 209 → 554 COMPLETE in three rounds, finding and
stepping over `040-file/open-rejects-null` and `080-video/flip-rate-rejects-bad-handle`
without anyone watching. `harvest-nids.ps1` read 96 logs for 389 pairs, then re-ran for
389 pairs and 0 new - idempotent, and the corpus keeps its provenance header.

**Surprises, both environmental and both expensive:**

`build/` was not writable from the host. The VM mounts this repository and had created
that directory as root, leaving `Everyone:(RX)` - so the host could not write into its
own build directory. The error says only "Access to the path is denied", with nothing
pointing at ownership. Reports now go to `reports/`.

Windows PowerShell turns a native command's stderr into an `ErrorRecord`. `multipass
transfer` warns about permissions it cannot set on a Windows target, copies the file
correctly, and under `ErrorActionPreference = Stop` that warning ends the script. The fix
is to relax the preference around the call and assert by `Test-Path`, which is the thing
actually being claimed.

Also: `Select-String -SimpleMatch` takes a *literal* pattern, so regex-escaping it first
searches for the backslash. Zero matches, no error. Worth making that mistake only once.

**`docs/TOOLING.md` was stale in a way that says something.** Its script section claimed
`scripts/` held three, listed four, and the directory held sixteen. Rewritten and grouped
by where the work happens rather than by file extension, which is the only reason there
are two kinds of script here at all.

`verify.sh`: 86 tool tests, lints clean, all three targets, derivation agrees, payload
refused as it should.

---

