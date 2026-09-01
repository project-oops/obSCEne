# Consensus run, and a stale ranking


**Four loaders now reach the end of the suite, so the N-way oracle was run** - it had been
ranked second on the backlog and blocked on "needs a second reporting loader" since before
there were four. `reports/consensus.txt`: 63 checks agreed, 73 disagreed, 24 outliers where
one implementation stands alone. By loader: host 10, PS5PCEM 6, shadPS4 5, fpPS4 3.

The best of them cross-validates today's other work. `018-relational/mutex-not-recursive` is
a PS5PCEM-alone failure, and the recursion probe added this morning independently found
PS5PCEM's type 2 recursive where the host's is type 1. Two unrelated checks reaching the same
conclusion is the strongest evidence this project has produced without hardware.

Worth noting the tool says so itself in its header: these implementations read each other's
source, so agreement is evidence rather than four witnesses.

**The ranking was stale in a way that would have wasted a session.** Three of its eight items
were blocked on things that had already happened, and two were finished: N-way consensus
(ranked 2), current-generation graphics (ranked 4, and PS5PCEM runs `GEN=5` to completion),
and condvars/barriers (ranked 5, four checks exist). Rewritten, with the completed items
struck explicitly rather than deleted - a list that quietly drops things reads as though they
were never planned.

New top two are the two threads left dangling by this session's findings: the runtime module
census (D149) and running the blind prober somewhere its question means something (D151).

Also recorded D155, which had been agreed in conversation and never written down: **one
source, as many binaries as the targets need.** Single-binary was never the requirement, and
`EI_ABIVERSION` makes it impossible anyway - a loader reads that byte before the guest runs,
so it cannot be a runtime decision. The rule is runtime detection wherever the platform will
answer at runtime, a build target only where it physically cannot.

