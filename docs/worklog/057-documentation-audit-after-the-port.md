# Documentation audit after the port


`TOOLING.md` documented **one subcommand out of thirty-two**, and its "What is still a
script" section said text handling stayed in shell - true when written, false since this
morning. Rewritten with three new sections (the gates, the generators, analysis) and a
corrected split: scripts are orchestration only, and everything else is Rust for a reason
worth stating rather than implying - the checkers were the only part of the tree without
tests or types, and they were the code deciding whether everything else was correct.

`WORKFLOW.md` still listed a Python generator in its by-hand table and described a sweep that
predates hang detection. Both fixed, and the sweep section now carries the fpPS4 walk - 33
records to a complete 742 over 44 exclusions - plus the two flags that matter at that scale,
`--resume` and `--corpus 0`.

`CLAUDE.md` pointed at `scripts/` for things that are now subcommands, and **`data/` was not
described anywhere at all** despite now holding the source of truth for five generated
artefacts. Both entries added.

Also removed `data/obscene-report.txt`: a 47 KB report sitting in the tracked data directory,
referenced by nothing. Reports live in `reports/`, which is ignored.

Nothing else in the live documents mentions Python, and `doccheck` covers the rest - every
`make` rule, script path, subcommand and decision number in prose resolves, and every
document is listed in the index. The counts, the compatibility table, the decision index and
all five generated artefacts are gated against their sources.

