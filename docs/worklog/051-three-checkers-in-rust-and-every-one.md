# Three checkers in Rust, and every one was miscounting


`guards`, `counts` and `doccheck` are now `obscene-tool` subcommands. 17 Python scripts down
to 14, 149 tests up from 119, and each port found the same class of bug: a hand-rolled
pattern producing a **smaller number**, which no gate can detect because a gate compares
against nothing.

| tool | what it missed | effect |
|---|---|---|
| `guards.py` | symbol column had to be an identifier | 8 rows never examined, incl. `910-bulk/probe` |
| `guards.py` | runner assumed to be `check_*` | the blind prober's is `run_bulk` |
| `counts.py` | check id matched as `[0-9]{3}-[a-z]+/` | `150-memory-map`'s two checks uncounted - README said 134, truth is **136** |
| `counts.py` | `X\((\w+)\)` over the census | a library ending in `X` emits a literal `X(X)`, so the **macro parameter** was counted as a symbol - 39,549 against a true **39,548** |

Three tools had each grown their own idea of what a check-table row looks like, so the row
parser is now `tool/src/sections.rs` and is shared. The anchor is `OBS_CAP` - the token that
actually makes a row a row - rather than a guess at the shape. `guards` and `counts` both
read it; nine tests pin the specific rows each old pattern dropped.

The same shape one level out: splitting on `L(` also splits inside any identifier *ending*
in `L`, and `OBS_NIDS_0000_LIBKERNEL(` does, which reported 382 libraries against 372. A
marker now only counts when the character before it is not part of an identifier.

`doccheck` was ported without a behavioural change and proved by differential rejection
rather than by agreement: both versions were fed a document naming a missing rule, a missing
script, a missing subcommand and a missing decision, and produced identical output on all
four. Agreement on a clean tree would have proved nothing.

One thing tightened on the way: `PROPOSED` existed so a backlog-proposed subcommand would
not fail the gate, and its comment claimed a proposal that ships "fails loudly when the name
is added for real". It did not - a name in both lists was accepted silently, and `consensus`
had been in both for some time. The Rust reports it, which is what the comment always said.

Next: `compat.py`, `protocol.py`, `protocol-selftest.py`, then the analysis scripts, then the
generators.

