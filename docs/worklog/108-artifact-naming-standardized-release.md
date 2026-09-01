# 2026-08-31 (artifact naming) - standardized release asset names across CI workflows and frontend (D279)


Standardized artifact and GitHub Release asset names across the repository:
- `payload`: `obscene-payload.elf` (staged to `dist/obscene-payload.elf`, uploaded as `obscene-payload.elf`).
- `injector`: `obscene-injector.elf` (staged to `dist/obscene-injector.elf`, uploaded as `obscene-injector.elf`).
- `module`: `obscene-module.elf` (staged to `dist/obscene-module.elf`, uploaded as `obscene-module.elf`).
- `eboot`: `obscene-eboot.zip` (containing `eboot.bin`, uploaded as `obscene-eboot.zip`).
- `pkg`: `obscene.pkg` (staged to `dist/obscene.pkg`, uploaded as `obscene.pkg`).

Updated:
- `.github/workflows/ci.yml`: Added `artifact-injector` job, updated all `upload-artifact` jobs to publish explicit release filenames.
- `frontend/web/index.html`: Added `assetLabel()` to map release asset filenames to user-facing badges/labels (`Payload (.elf)`, `Injector (.elf)`, `Package (.pkg)`, `Eboot (.zip)`, `Module (.elf)`).
- `Makefile`: Updated `payload` rule to output `$(BUILD)/obscene-payload.elf` (maintaining backward-compatible copy to `obscene.elf`), updated shapes summary.
- `docs/ARTIFACTS.md` & `docs/BUILDING.md`: Updated tables to reflect canonical filenames.
- `scripts/payload-run.sh` & `scripts/build-all.sh`: Updated path lookups to prefer `obscene-payload.elf`.

