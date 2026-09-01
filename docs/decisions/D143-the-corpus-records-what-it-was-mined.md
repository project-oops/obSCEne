# D143 - The corpus records what it was mined from, so the half of census drift nothing was watching becomes visible


`gen-corpus.py --check` gates the generated headers against `data/`. Nothing gated `data/`
against the emulators it was mined out of, and that is the drift with consequences: an
emulator gains four hundred names in a release, the corpus does not, and obSCEne reports
`absent` for a surface it never asked about. Every symptom points at the platform. The
census reads as complete the entire time, because it is complete - with respect to a
question asked months ago.

**Recorded, not recomputed.** Re-mining takes minutes and needs both the emulator checkouts
and the 23-version firmware tree, so it cannot be a gate. Both data files now carry a
`mined-from:` line of `<source>@<commit>` and a `firmware:` line of versions, and `--check`
compares those against what is on disk.

That is a weaker claim than "the names are current" and it is the claim that can actually be
made: it says *this corpus has not been shown the current commit*, never *these names
changed*. A new commit that touched no export table is a false positive, and re-mining
clears it in a few minutes. The opposite error - a corpus quietly older than its sources -
has no such recovery, because nothing ever announces it.

**Absent sources are not drift.** A machine without the checkouts says so and passes. It has
no evidence either way, and a gate that fails on missing evidence is one people learn to
skip - the same reasoning that gates the compatibility table on its reports existing.

The build VM is one of those machines: the emulator toolkit lives on the Windows host and is
not mounted, so the usual `multipass exec ... verify.sh` skips this every time. The check
bites on the host, where mining actually happens. Worth stating plainly rather than leaving
someone to assume the gate has them covered.

Shown to reject before being believed: a doctored commit in the recorded line, which it
named in both directions - on disk but not in the corpus, and the reverse.

