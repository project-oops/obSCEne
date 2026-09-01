# D279 - canonical release artifact naming scheme across CI workflows and frontend


Status: decided.

Standardized artifact and GitHub Release asset names across all shapes:
- `obscene-payload.elf`: Plain ET_DYN compatibility payload (`make payload`).
- `obscene-injector.elf`: Native process injector (`make injector`).
- `obscene-module.elf`: Vendor ELF for emulators (`make module`).
- `obscene-eboot.zip`: Zipped fSELF `eboot.bin` for app0 installations (`make eboot`).
- `obscene.pkg`: Retail-style package installer (`make pkg`).

CI workflow (`.github/workflows/ci.yml`) uploads artifacts matching these explicit filenames. The landing page (`frontend/web/index.html`) parses GitHub release assets and provides clean labels (`Payload (.elf)`, `Injector (.elf)`, `Package (.pkg)`, `Eboot (.zip)`, `Module (.elf)`).



