# D151 - The blind prober does not belong on the host build, and the harness did not scale to the corpus. Both were found by running it


First full-scale run: 121 rounds, 8,356 of ~30,000 targets reached, **328 answers**, and a
**354 MB** report. It stopped at the round cap, not at the end.

### One process per faulting function is the wrong economy here

`bulk-sweep.sh` was written when the surface was 383 symbols, and its stated arithmetic -
"rounds needed is the number of functions that *fault*, not the number of functions" - was
true then. At corpus scale on the host those are the same number, and the run shows it
plainly: 120 of 121 rounds ended in a segmentation fault, marching alphabetically through
`posix_spawnattr_setpgroup`, `setschedparam`, `setschedpolicy`, `setsigdefault`, `setsigmask`,
`posix_spawnp`, `putc`, `putc_unlocked`.

None of those is a finding. **On the host every censused libc name resolves to real glibc**,
so calling one with a null first argument dereferences null, exactly as it should. The
prober's question - *is there an implementation behind this symbol, or a stub?* - has a known
answer here, and the crashes are the correct behaviour of a working library.

The question is only meaningful where a symbol might be a stub: an emulator, or hardware. On
a loader that stub-resolves what it cannot find, the same calls return zero and return
*fast*, so rounds stay proportional to real faults and the sweep advances. The host run
should be a short mechanism check, not a corpus sweep.

### Every round re-emitted the census

The accumulated report is the rounds concatenated, so anything a round prints is printed once
per round. 4.24 million of the report's 4.36 million records were **121 identical copies of a
35,045-symbol census**, wrapped around 328 answers.

Rounds now build with the census section excluded, through a new `EXTRA_EXCLUDE` on
`sweep-build.sh` rather than the sweep's own exclusion list - a section left out for output
volume must not be left behind in that file as though a crash had been proved there. Per-round
output fell from about 2.9 MB to 59 KB.

**Still coarse, and left that way for now.** A prober round still runs the whole suite; only
the largest section is gone. The clean form is a whitelist - run `910-bulk` and nothing else -
which the exclusion mechanism cannot express, and which is a bigger change than this
measurement justifies. Named here so the next person sees a bound rather than a finished job.

### What the run did establish

The mechanism works unattended: 121 rounds, resume-past-the-fault correct every time, no
manual intervention, and the announcement named the exact function in every case. That was
the thing in doubt, and it is no longer.

