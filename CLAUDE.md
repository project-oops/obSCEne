# CLAUDE.md

Read [AGENTS.md](../AGENTS.md), [CONVENTIONS](../docs/CONVENTIONS.md) and
[STYLE](../docs/STYLE.md) first. This file holds only what obSCEne adds to them.

obSCEne is a conformance probe for Prospero-generation hardware, in freestanding C. It calls
the system functions a title would call, one at a time, and reports what each one did. It is
ordinary homebrew with nothing emulator-specific in it.

The gate is `./bin/obscene check` (`make check`); `scripts/verify.sh` runs everything that has
to pass.

## Probe principles

- **Announce before attempting.** Every check writes its identity, unbuffered, before making
  the call. A `try` record with no matching `res` means that call did not return. Never buffer
  output, batch records, or announce a check that will not run; a skipped check emits no `try`.
- **Nothing is invented.** Where an arity, a constant or a struct layout is uncertain, the
  function is left out. Adding a function means confirming its signature. (D008)
- **The report is an interface.** [docs/OUTPUT.md](docs/OUTPUT.md) is a contract with
  parsers. Field order and meaning change only with a version bump; new fields append to the
  end of a line. Check identifiers are the key for diffing runs and are treated as a public
  API.
- **The host build is required.** `make host` runs the harness against stubs on an ordinary
  machine, which separates a probe bug from a platform bug. Keep it building. (D001)
- **ABI identifiers stay as spelled.** The symbol and library name strings in
  `include/obscene/platform.h` and the check tables are ABI identifiers; the import hash is
  computed from the name.
- **No vendor headers and no SDK.** Every declaration comes from public interface
  documentation or an open-source toolchain. A declaration with no nameable source is a defect.
- **Positive checks over negative ones.** A negative check proves only argument validation;
  memory round trips and threads that run their body prove a function works. Every new
  confident signature is a chance to add a positive check. (D007)
- **The runtime is freestanding.** The harness uses no libc, allocation, floating point or
  variadics. Helpers go in `src/probe/runtime.c` and stay short. Checks may call those
  facilities (`037-math`, `035-libc/snprintf`), because measuring them is their job; the
  harness never depends on them for its own work.

## Before adding a check

1. Move the name from the census in `include/obscene/surface.h` to a real declaration in
   `platform.h`. The census declares names as `const char`, so a name cannot be in both.
2. Add it to `src/probe/imports.c`, or `mkmodule` refuses the build.
3. Add it to the `@called-elsewhere` block in `data/surface.txt` and remove it from its group
   there, or regenerating the census reintroduces it. `verify.sh` gates this.
4. Give it a provenance: `OBS_FROM_SPEC` where ISO C or POSIX settles the answer,
   `OBS_FROM_ASSUMED` otherwise. Assumed is the default.
5. Run it under `make host` before believing anything it reports. A check that has not passed
   a known-good implementation is not evidence.

- A check that calls a symbol other than its own tests that address for null first. The
  harness guards only the symbol in the check's table row, and every platform declaration is
  weak. (D058)
- A check that loops reports progress (see `015-sync/thread-churn`), so a crash names the
  iteration.
- Anything that can block - a lock, a wait, a blocking read - is written in the `try` form or
  not at all. A hang loses every check behind it.

## Building

C builds run in the WSL2 `Ubuntu` distribution; multipass is not used. Building natively on
Windows is not supported: `module` needs `$(BUILD)/symbols.txt`, produced by running the POSIX
host build.

```bash
# Full gate from Git Bash, through WSL.
wsl.exe -d Ubuntu -- bash -lc 'export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"; cd <OOPS>/obscene && make check BUILD=$HOME/obs'
```

- `BUILD` and `CARGO_TARGET_DIR` are Linux-local paths (`$HOME/...`), never `/mnt/c/...`. A
  Windows mount carries no execute bit, and `symbols.txt` comes from running the host binary.
  (D012)
- `tool/Cargo.toml` takes a path dependency on `../prosperous`, so that sibling must be
  checked out.
- Set `MSYS_NO_PATHCONV=1` for any Git Bash call that hands a Linux path or a `/`-flag to a
  Windows program: `wsl.exe ... bash <path>`, `tasklist /FI ...`, and every call in
  `scripts/wsl.sh`. (D199)

Distribution toolchain:

```bash
sudo apt-get install clang lld binutils gcc libc6-dev make
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
rustup component add clippy rustfmt
```

`gcc` is required because Rust links build scripts with `cc`; `verify.sh` needs `clippy` and
`rustfmt`.

## Hardware round-trip

Reach the round-trip through the `./bin/obscene` verbs, never by running the underlying
scripts. Each verb runs its halves on the right side (WSL to build, Windows to install).
`./bin/obscene help` lists the verbs; `scripts/README.md` maps each to its script. (D269)

```bash
# Build, install, launch and capture the report; flags: --gen --seconds --name --build-only --deploy-only --no-cache --jobs
./bin/obscene deploy
# Capture the report from a title already running.
./bin/obscene report --seconds 120
# After a crash, before any relaunch: what the console recorded, read-only.
./bin/obscene recover
```

The report lands in `reports/hardware/console-klog.txt` (gitignored). It is captured from the
system log, never pulled from disk: a packaged run's report file is sealed `0600` inside the
title's sandbox.

| `hw` subcommand | Runs on | Reason |
|---|---|---|
| every C target (`host`, `module`, `payload`, `eboot`, `pkg`) | WSL | the host build is POSIX |
| `hw check`, `send`, `logs`, `sh`, `ls`, `pull`, `report` | either | these connect out to the hardware |
| `hw install` | Windows | the hardware connects in to fetch the package, and WSL2's NAT address is unreachable from it |

`hw install` output of `fetched 0 time(s)` means the hardware never reached the server; the
package is not implicated.

```powershell
# hw install from Windows; the package itself is built in WSL.
cd <OOPS>\obscene\tool
$env:CARGO_TARGET_DIR = "<OOPS>\obscene\tool\target-win"
cargo build --bin obscene-tool
.\target-win\debug\obscene-tool.exe hw install <path-to>\obscene.pkg
```

- `scripts/sweep-build.sh` rebuilds with the exclusion list and is the last build before
  anything runs the module; `build-all.sh` and `make check` build without exclusions.
- Sending the vendor-format build to the hardware's homebrew loader takes that loader down.
  [docs/ARTIFACTS.md](docs/ARTIFACTS.md) says which file goes to which loader.

## Where things live

- `../selfish` - the file formats: import hash, executable format, vendor dynamic table,
  container, linker script and format tables. A format is shared there; a measurement stays
  here, including the name-to-library manifest and the `$` sigil convention.
- `../oops-sdk` - the freestanding-C SDK; the `Makefile` includes its `oops-sdk.mk`.
- `data/` - this project's own measurements and the source for everything generated:
  `surface.txt`, `mined-names.txt`, `unnamed-nids.txt`, `gpu-surface.tsv`, `font.txt`,
  `nid-corpus.txt`.
- `include/obscene/platform.h` - every platform declaration and ABI constant.
- `src/probe/sections/` - the checks, grouped by layer.
- `src/probe/registry.c` - the running order, as one explicit list.
- `tool/` - the Rust tooling, gates and generators; [docs/TOOLING.md](docs/TOOLING.md) lists
  them. There is no Python in this repository.
- `scripts/` - orchestration only. Read [scripts/README.md](scripts/README.md) before adding
  one: a question about a file's contents is a probe in `../selfish/crates/*/examples/`, a
  fact about our own output is a test.
- `scripts/gpu-analyze.sh <corpus>` - reference, exact diff and ULP ranking over a GPU corpus.
- [docs/OUTPUT.md](docs/OUTPUT.md) - the report contract.
- [docs/ARTIFACTS.md](docs/ARTIFACTS.md) - which artifact goes to which loader.
- [docs/GPU_SURFACE.md](docs/GPU_SURFACE.md) - the GPU ISA census, generated by
  `obscene-tool gpusurface` from `data/gpu-surface.tsv`.
- [docs/DECK.md](docs/DECK.md) - capturing and analysing an RDNA2 corpus on a Steam Deck
  (`make deck`).
- [docs/GNM.md](docs/GNM.md) - probing the platform's GPU command interface.
- [docs/EMULATORS.md](docs/EMULATORS.md) - the emulator kit in `<emulators>` and the
  provenance boundary for reading it.
- `patches/` - local changes to other emulators, applied by nothing. A report from a loader
  patched to change its behaviour never occupies that loader's row in
  [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md). (D176)
