# 2026-09-01 (foreground retail game process & Title ID resolution) (D288)


Diagnosed why running retail titles (e.g. candidate Title ID `PPSA04263`) were not detected by `target_find_foreground_app()`:
1. **Root Cause Analysis**:
   - `proc->p_comm` stores the binary executable's basename (`eboot.bin`), NOT the Title ID (`PPSA...`). Checking `obs_strncmp(comm, "PPSA", 4)` never matched.
   - For `comm == "eboot.bin"`, D286 required `krw_get_proc_jaildir(pid) != 0`. On PS5, retail titles run under Sony's container sandbox which does not populate FreeBSD `p_fd->fd_jdir` at offset `0x20`. `krw_get_proc_jaildir()` returned 0, silently discarding the running game.
   - An arbitrary filter `pid > 100` needlessly skipped lower PIDs.
2. **Remediation**:
   - In `src/injector/target.c`, eliminated the bogus `jaildir != 0` requirement and `pid > 100` filter.
   - Added `get_proc_title_id(proc, out_title)` to scan `struct proc` for `PPSAxxxxx` or `CUSAxxxxx` Title ID patterns, allowing `target_find_by_name()` and `target_resolve()` to look up processes directly by Title ID.
   - In `target_find_foreground_app()`, any non-system process with `comm == "eboot.bin"` (excluding `mypid` and `NPXS...`/`NPXX...` system daemons) is identified as the running retail game.
   - In `src/injector/injector.c`, eliminated hardcoded game title references from error messages.

Verified: `make -C <OOPS>/obscene check BUILD=$HOME/obs` passes 100% clean.

