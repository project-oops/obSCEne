# Every checker is Rust now


Six ported, 17 scripts down to 11, 119 tests up to 161. `guards`, `counts`, `doccheck`,
`compat`, `protocol` and `protocol --selftest` are `obscene-tool` subcommands, and the
`verify.sh` stages and CI step point at them.

`compat` and `protocol` ported without behavioural change, and both were proved by
*differential rejection* rather than by agreement. `compat` renders a table compared
byte-for-byte against the committed document, so both versions producing "current" means
both produced identical markdown. `protocol` was run against its own thirteen mutations -
an unknown verb answered, a sequence going backwards, a capability never announced, a bytes
run one nibble too long - and both caught all thirteen. Agreement on clean input proves
nothing; that is exactly how the Python guard passed for months while skipping eight rows.

Three things worth keeping from the port itself:

- **I guessed three of the protocol's fixed lists rather than reading them**, and invented a
  `part` verb, a `refused` reason and a `too-long` reason while dropping `blob`, `run` and
  `gpu`. The Python sitting beside it is what made that visible within a minute. Ported
  lists are transcribed now, not remembered.
- **The first protocol tests raced.** They shared one temp path under the parallel runner
  and one test deleted another's file mid-check. A fault in the test rather than the
  checker, but a flaky gate is a gate people learn to re-run rather than read.
- **The first test fixtures did not look like the real transcripts** - no `ack`, no terminal
  `done` - so they tripped the dangling-ack rule and tested the wrong thing. Same mistake as
  the guard fixture earlier in the day: a fixture that does not resemble the thing under
  test proves nothing about it.

Remaining Python is four analysis scripts and seven generators. Five of the generators are
gated in `verify.sh`, so those are next.

