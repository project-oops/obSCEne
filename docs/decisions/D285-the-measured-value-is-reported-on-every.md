# D285 - the measured value is reported on every row, not only on a difference


`048-selfaudit/confirm-table` printed the value it measured only when that value **disagreed** with
selfish's table. A matching row reported the word `matches` and a fixed sentence, with no number.

The reasoning was signal-to-noise: on a match the number equals the table the reader already has,
so printing it repeats a known fact. The effect was the opposite of what a cautious reading would
want. The number was withheld in exactly the case where it is **already public** - the nine fixed
rows live in `selfish/data/self-format.tsv`, whose own header describes it as "the SELF container,
as three independent open-source projects read or write it", with every row cited to OpenOrbis,
shadPS4 or fpPS4 - and printed in the case where it is genuinely new. Nothing is protected by
omitting a constant this repository already commits and publishes a citation for.

It also made the report worse at its job. `version|matches` cannot be diffed against the next
firmware, cannot be read by anyone without the table open beside it, and cannot answer "what did it
actually say" without a rebuild. A conformance probe whose output omits the measurement is
reporting its own opinion instead of its evidence.

So every row now carries the measured value, and a differing row carries the table's value beside
it - `0x24 (table: 0x22)` - because the gap is the point of the line and one half of it is not
actionable alone.

**This does not move the provenance line, and it is worth being exact about where that line is.**
The claim "no bytes leave the console" was always too strong: the report streams over a socket and
lands in `reports/hardware/`. What the design actually refuses is *bulk copying of the container* -
which is what the removed `selfdump` did (D086) - not the reporting of individual measured
constants. A format constant observed and reported is a datum about an interface, the ordinary
output of clean-room reverse engineering; a copied header is a piece of someone's binary. Keys and
signatures are neither read nor reachable: `key_type` is a selector naming which key class the
container declares, not key material, and the probe reads bytes 0x00-0x40 only.

Verified: compiles clean in the host and target shapes under the project's full flag set
(`-Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes`). Not yet run end to end -
`make host` still fails in `src/probe/runtime.c` on undeclared `sceKernelGetModuleList` /
`sceKernelGetModuleInfo`, another session's in-flight work that this change does not touch. The
next hardware run is where the nine values appear in the log for the first time.
