# 2026-09-02 (ps5 native title) gen-5 eboot, a two-id identity, and `native --deploy` to a scan root


Brought the native-title path from a deeplink-only stub to a deployable, current-generation title,
and hardened the on-console SELF audit that feeds selfish's format table.

1. **Native title carries a gen-5 eboot.** `build-native.sh` now stages `eboot.bin` (via
   `selfish native --root`) beside `sce_sys/{param.json,icon0.png}`, so the title launches its own
   code rather than only deeplinking - correct on a kstuff console, where a fake-signed fSELF loads.
   `make native` builds gen-5 by default: the eboot's `mkself` was hardcoded `--generation 4` while
   `mkmodule` already took `$(EBOOT_GEN)`, so un-hardcoding it and setting `native: EBOOT_GEN := 5`
   makes the container `54 14 F5 EE`. selfish needed nothing new - `--generation 5` existed. (D289)
   Unblocked by adding the ctype table accessors `_Getpctype`/`_Getptolower`/`_Getptoupper` to
   `imports.c`, which the `035-libc`/`007-responsive` ctype macros expand to. (D290)

2. **One identity home, then two ids.** The title/content id was copied into both build scripts;
   consolidated into `data/identity.toml` (toml, read with `sed` - selfish is *told* the id as an
   argument and never reads the file). (D288) Then split it: the package (`OBSC00001`) and the native
   title (`OBSC00002`) get distinct ids from `content_id` / `content_id_native`, so both can be
   installed side by side. selfish must not vary an id it is handed - that would be inventing
   identity - so obSCEne derives both. (D292)

3. **`native --deploy` pushes the title to a scan root via prosperous.** New `obscene-tool hw
   install-native <dir>` uploads a title directory through `pros_core::transfer::upload` (FTP STOR
   per file), the native counterpart to `hw install`. It targets `/user/data` by default - one of
   the directories ShadowMountPlus *scans* and registers into `/user/app`; `/user/app` itself is
   never scanned, so a copy there is inert. `--into` picks a USB drive instead. Proven end to end
   against `192.168.1.211`. (D291)

4. **selfaudit hardening.** `obs_read_header` is bounded by the caller's buffer (a 0x40-into-32-byte
   overrun in `metadata-differential` produced 45,792 phantom "installed titles"). (D284) Every
   fixed header row now reports its measured value, not only on a difference - the values are already
   public in selfish's table. (D285) Added `048-selfaudit/container-structure`, a raw measured dump
   of a real gen-5 container (header, segment table, ex_info/npdrm) so selfish has the numbers a
   gen-5 package is built from - decoding stays in selfish's table, not re-encoded here. (D286)

Verified: `make host`, `make eboot`, `make native` (gen-5) and the tool build all pass; the native
build yields `titleId OBSC00002`; the package reader yields `OBSC00001`; `hw install-native` uploaded
to the live console. The eboot build had been broken mid-session by a concurrent probe change
(`runtime.c` module-enumeration; `_Getpctype`), both since cleared.
