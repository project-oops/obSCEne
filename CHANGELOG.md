# Changelog

obSCEne ships as a **single rolling build** - no tagged versions, no semantic versioning. Every
push to `main` refreshes one `latest-main` prerelease, so the **short commit SHA is the version
number**.

**Five shapes are published, and they are not interchangeable.** They are told apart by two
bytes at offset 16, and sending the wrong one to real hardware has taken a loader down and cost
a reboot. Each release asset is named for the loader it is for; `docs/ARTIFACTS.md` is the
authority.

Each entry is headed by the SHA (+ date) that shipped it, newest first. Within an entry, changes
are grouped **Added / Changed / Fixed**.

Nothing has shipped yet. This is the initial commit, so no entry below carries a SHA and the CI
that would produce one has never run.

## [unreleased] - as of 2026-09-01

### Added

- **A freestanding C conformance probe** that builds with no vendor toolchain anywhere: clang,
  `lld`, its own linker script, its own platform declarations, its own format tooling.
- **It installs and launches on real hardware**, which is the whole point and was not true
  until the package and filesystem mount chain was understood end to end.
- **Five artifact shapes** built in CI - payload, injector, module, eboot and package - now
  published, each renamed to say which loader takes it. They were being built and thrown away.
- Standard 96-byte `sce_sys/keystone` generation in `scripts/build-pkg.sh` for PS5 AppPromoter
  compatibility.
- **A symbol census and a GPU probe workstream** carried as far as they can go without the
  hardware in front of them, so the hardware session is spent measuring rather than building.

### Changed

- Migrated payload runtime bootstrapping (`crt0.c`), privilege escalation (`escalation.c`,
  `escalation.h`), and `installer.c` to `SELFish/runtime/` as a shared SDK layer.
- Updated `scripts/build-payload.sh` and build scripts to consume runtime from `$SELFISH/runtime`.
- **Migrated onto the SELFish format crates**, deleting 2,801 lines and three local format
  snapshots. What stayed is the part that is a convention of this probe rather than a fact about
  a format.

### Fixed

- Fixed PKG generation and rejection on PS5: entry table ordering, name offsets, SC flags, and
  SFO alphabetical sorting.
- Defaulted generation target to `GEN=4` (fSELF) in `scripts/oops-rebuild-pkg.sh` and
  `build-pkg.sh` for PS5 homebrew execution.
- **The provenance guard now reads file contents**, not extensions. A 16 KB extensionless ELF
  had been sitting tracked in this repository, which is exactly what the guard exists to stop.
