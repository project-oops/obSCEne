# D150 - The hardware day made turnkey: one analysis command and a runbook, so a Deck corpus needs no new thinking


Status: derived - `scripts/gpu-analyze.sh <corpus>` runs the three tools in sequence, and
`docs/DECK.md` is the capture-and-diff runbook. Proven end to end on the golden.

The three analysis tools were built to be read together but were three commands and an
intermediate file. `gpu-analyze.sh` makes them one: reference for the corpus's inputs, the
exact-match diff, then the ULP ranking. The point is that the analysis does not change between
llvmpipe and a Deck - only what produced the corpus does - so the command that reads the golden
today is the command that reads a Deck's corpus the day one exists, unchanged.

`docs/DECK.md` records the rest of that day: `make deck` (the Vulkan host build, renamed - the
Deck needs no console GPU API, which is why it is the near-term target), running or serving it,
reducing the report to a corpus, and diffing a hardware corpus against an emulator's to name the
shader-recompiler gaps by operand.

Why this rather than the Gnm/shadPS4 path, which was the other candidate: reconsidered, that path
is large and of limited value for the goal. The Deck runs pure Vulkan and needs no `sceGnm` at
all; Gnm would only produce a *shadPS4* corpus - that emulator's gaps, not the ones this project
exists to close, which live in the Deck's silicon (reachable already) and in orbistoun (the
clean-room thread this fork must not touch). shadPS4's source is not even present here to confirm
signatures against, only its binary. So the leverage was in making the reachable path turnkey,
not in building a large bridge to a bystander emulator. This is the honest completion of the
ahead-of-hardware GPU work: the execution probe (50 kernels), the census, the reference, the
golden gate, controlled-ISA, the protocol escape hatch, and now a single command from any corpus
to its approximation map.

