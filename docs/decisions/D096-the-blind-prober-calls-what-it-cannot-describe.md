# D096 - The blind prober calls what it cannot describe, and never on hardware

**Status:** decided
**Date:** 2026-09-26

`910-bulk` (built with `BULK=1`) calls every callable censused symbol with zero in all six argument
registers and records the answer. `HARDWARE=1` with `BULK=1` is a build error, and CI checks that
the published payload carries no prober.

**Why:** on this ABI the caller cleans up and arguments travel in registers, so a wrong arity
cannot corrupt the stack, and nothing is asserted. A returned vendor error code shows an
implementation behind an address, which the census cannot. On a console the list includes
shutdown functions, and most entries are unnamed identifiers no blocklist can screen.

**Rejected:** a blocklist for hardware - cannot cover unnamed identifiers. A convention that `BULK`
stays empty - fails when a command line is reused.
