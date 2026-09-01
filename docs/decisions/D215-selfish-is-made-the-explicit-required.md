# D215 - selfish is made the explicit, required build path - in the Makefile, in CI, and through a wired `make pkg`


The dependency on selfish was real but implicit: it worked, but a reader could not tell from the
Makefile header that a checkout without selfish beside it builds nothing a console loads, CI
could not pass because it never checked the siblings out, and `make pkg` was a bare `exit 1`.
Three changes make the intent legible and the pipeline ready.

### The Makefile says what selfish is for

The header stated "the only build dependency is clang", which is true of compiling C and false
of producing a format. It now separates the two: clang compiles, and **selfish produces every
vendor format** - module and eboot through `selfish-container`/`selfish-elf`, the linker script
from `$(SELFISH)/link/module.ld`, payload resolution through `dynamic_symbols` + `Nid`, and the
package through `selfish-pfs` + `selfish pack`. It names selfish as the single source of truth
for the formats, so a wrong magic cannot be introduced here in isolation.

### CI checks out the siblings

`tool/Cargo.toml`'s `../../selfish` and `../../prosperous` path dependencies meant every job
that built the tooling could never pass on a bare checkout - documented for months as "cannot
pass as written". Now each such job checks out obscene, selfish and prosperous side by side and
sets `working-directory: obscene`; `${{ github.repository_owner }}` resolves the siblings under
whatever account owns the repo, which is what "the names are not settled" was waiting for. Two
pre-existing bugs fell out in passing: the eboot and pkg jobs uploaded a `dist/` they never
staged into, so both artifacts would have been empty.

### `make pkg` is wired end to end, and stops honestly

`make pkg` -> `scripts/build-pkg.sh` builds the eboot, stages the app tree (`eboot.bin` +
`sce_sys/param.json`, the four fields measured on hardware per D180), and calls selfish for the
rest. It runs to the one step selfish does not yet expose as a command - a filesystem-image
builder over `selfish-pfs::build` (the crate function exists; no `selfish image` subcommand) -
and stops there naming it, rather than pretending nothing exists. `selfish pack` already takes
the finished image, so the day that command lands, this target completes with no change here.
CI's pkg job stays `continue-on-error` until then, so the gap is visible without a red pipeline.

This is the "build with selfish, the real way" the operator asked for: the toolchain is selfish
for every format, CI produces the full artifact set (elf / module / eboot, and pkg the moment
selfish can), and the one remaining piece is a selfish-pkg command the sibling thread is
building.

Status: **assumed** - Makefile and scripts run; CI is structurally correct but unexecuted (no
remote yet), which is the same state every workflow here is in.

