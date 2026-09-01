# D272 - A GPU field for the driver, DISK wired through `statfs`; TEMP stays honestly unknown


Three HUD fields the console showed as `unknown`, and a question of whether each could honestly show a value.

**GPU - a new field.** `GEN` answers "which console generation", and on a `ps4_game` that is unanswerable: the current generation's AGC driver is refused the category, so `obs_current_present()` returns "could not look" and the verdict collapses to `ps4_mode` (D255, correctly - "could not look at gen5" must never be reported as gen4). But that discards a fact the same probe holds: the **gnm graphics driver resolves** here - it renders the report and `165-gnm` passes. So a separate `GPU` field now reports the driver that resolves (`gnm` / `agc` / `gnm+agc`) via `obs_gpu_drivers()`, which asks the driver question rather than the generation one. `gnm` present names a driver, not a console - a current-generation console in compatibility mode exposes it too - so nothing here ages, and it needs no new provenance (the same two markers `005-generation` already keys on).

**DISK - wired through `statfs`.** Free bytes = `f_bavail × f_bsize`, read at FreeBSD's stable offsets (`f_bsize` 0x10, `f_bavail` 0x30 - `sys/mount.h`) out of an over-sized buffer, so the struct layout past those two words is never depended on. That is the discipline the version getters already use, and the honest way to call a query whose full vendor struct this program deliberately does not carry (D008). `statfs` moved from the mined corpus into `platform.h` - a name cannot be both a `const char` census entry and a callable symbol - and the corpus regenerated to exclude it, dropping libkernel from 789 to 788. `/download0` is a title's own writable mount; a path the sandbox refuses reports `unconfirmed`, not a wrong number.

**TEMP - stays `unknown`, deliberately.** No SoC-temperature query exists in the corpus at all - the "temp" matches are `template`/`temporary` - and inventing an arity or a struct for a thermal call is the sin this program exists to expose (D008). The field stays as the visible reminder that a provenanced temperature signature is wanted and not yet found. That is the honest state, not an oversight.

**NET / KEY - left as they are.** They are the serve-mode command-socket bind and the session secret, correctly `absent` on a packaged suite run that serves nothing; they light up in a serving build.

Status: **done**.

