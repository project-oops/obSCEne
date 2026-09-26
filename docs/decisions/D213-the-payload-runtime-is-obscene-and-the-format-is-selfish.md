# D213 - The payload runtime is obSCEne's; its format primitives are selfish's

**Status:** decided
**Date:** 2026-09-26

The start-up code a plain-ELF payload runs, and the build step that generates its per-firmware
resolution table, live in this repository. Reading dynamic symbol tables and computing identifiers
are selfish primitives this repository calls.

**Why:** selfish's charter excludes anything that executes on the console, and the per-firmware
addresses are measurements. Reading `.dynsym` is format knowledge and belongs with the formats.

**Rejected:** putting the start-up code in selfish. Reimplementing symbol-table reading here.
