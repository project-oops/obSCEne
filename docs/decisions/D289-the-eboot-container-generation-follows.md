# D289 - the eboot container generation follows EBOOT_GEN


`make eboot` stamped the SELF container with a hardcoded `--generation 4` while the `mkmodule` step
directly above it took `--generation $(EBOOT_GEN)`. So `EBOOT_GEN=5` produced a gen-5 *module* inside
a gen-4 *container* - a split that made a current-generation eboot impossible to build even though
selfish has supported `mkself --generation 5` all along.

`EBOOT_GEN`'s own comment describes it as "what the file says it is", which is precisely the container
magic that `mkself` writes. So `mkself` now takes `$(EBOOT_GEN)` like `mkmodule` does. The default is
unchanged (`EBOOT_GEN ?= 4`, the proven fake-signed container that installs and launches), and
`EBOOT_GEN=5` now stamps the current-generation container (`54 14 F5 EE`) - whose structure selfish
still calls a hypothesis until a console accepts a file built from it. `./bin/obscene native
EBOOT_GEN=5` is the intended way to build the gen-5 experiment; gen-4 stays the default.

**No new selfish command was needed** - the request framed PS5 output as a selfish addition, but
`--generation 5` already exists there. The gap was only this hardcode.

**Untested, and honestly so.** The change is correct and default-preserving, but it could not be
exercised this session: the eboot build is currently broken *upstream of this line*, at `mkmodule`,
which fails to assign a library to three libc symbols (`_Getpctype`, `_Getptolower`, `_Getptoupper`)
- at both gen-4 and gen-5. That is another session's in-flight probe work (the same tree that had the
`runtime.c` module-enumeration breakage earlier), and closing it is the five-step import process in
`imports.c`, not this file. The eboot built cleanly earlier this session, before those edits landed,
so this is a regression in the probe, not in this change (which sits after the failing step and is
never reached). Once the tree builds again, `EBOOT_GEN=5` should produce a gen-5 container; if gen-5
then surfaces its own import gaps, those are closed the same way.

Whether a gen-5 eboot is even *needed* is a separate open question: the native title's PS5 badge comes
from `AppInstallTitleDir` registration, not the eboot's generation, so the gen-4 native title already
built (proven container) may launch fine. Test that first; reach for gen-5 only if the native loader
rejects the gen-4 container.
