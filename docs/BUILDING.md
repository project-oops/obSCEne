# Building obSCEne

The entry point is `bin/obscene`. It is a front door, not an implementation: the build is the
`Makefile`, and anything the script does not recognise passes straight through to `make`, so
every target works by name.

```bash
./bin/obscene check     # the shapes, the tooling's tests and lints, the host harness, verification
```

## Requirements

| | |
|---|---|
| `clang` and `lld` | the probe is freestanding C |
| `make` | the build is a Makefile |
| a Rust toolchain | for `tool/`, the offline tooling; not for the probe itself |
| `clang-format` | for `fmt`, when changing the C |

The build needs no SDK, no firmware and no signing keys, so anyone can reproduce a report.

### Siblings

obSCEne does not build from a clone of this repository alone. It resolves three siblings by
relative path, so the directory layout is a build requirement:

```
selfish      every platform file format - the module, the eboot, the package
prosperous   (and oops-libs, through it)
oops-sdk     the freestanding-C SDK: its sources link into the module and eboot, and
             payloads build on its runtime (Makefile includes its oops-sdk.mk)
```

```bash
./bin/oops bootstrap obscene    # fetches exactly those
```

Without SELFish, `host` builds and nothing a loader or emulator accepts does. Every format is
produced through SELFish, so a wrong magic or a stale tag cannot be introduced here alone.

`SELFISH ?= ../selfish` and `OOPS_SDK ?= ../oops-sdk` in the `Makefile` override the layout.

### Windows

Git Bash usually has neither `clang` nor `lld`. `oops` detects that and re-enters through WSL:

```bash
./bin/oops check obscene        # from the collection root, on Windows
```

`OOPS_NO_WSL=1` refuses instead of delegating. The path-translation rules are in
[the collection's BUILDING.md](https://github.com/project-oops/OOPS/blob/main/docs/BUILDING.md#windows-wsl-and-why-obscene-is-different).

## Verbs

The shared verbs, so `oops test obscene` and `./bin/obscene test` are one command:

| verb | what it does |
|---|---|
| `build` | module, payload, injector, host |
| `test` | the tooling's tests - `cd tool && cargo test` |
| `lint` | the tooling's lints, in full |
| `fmt` | format the C in place |
| `check` | the full gate, as CI runs it |
| `clean` | remove `build/` |
| `doc` | the tooling's API docs |
| `pkg` | the installable package |

`test` runs the tooling's own tests; `check` is the full gate.

The tooling's subcommands, reached the same way by CI and by a person:

| verb | what it does |
|---|---|
| `tool-build` | build `obscene-tool` |
| `guards` | the cross-repository symbol guards |
| `imports <file>` | what a built object imports |
| `verify <report>` | the report is in the documented format |
| `matrix` | compare every committed capture in the conformance matrix against hardware; exits 1 on divergence |
| `selftest` | the NID chain still derives |
| `payload-build` / `injector-build` | build that shape and stop, without sending it anywhere |

`fmt` does not use `clang-format -i`, which writes by rename; a mounted Windows share refuses
the rename and leaves `foo.c.temp-stream-XXXX` in place of `foo.c`. `scripts/format.sh`
redirects and truncates instead.

`lint` is a script so that clippy failing to run is a failure, not a clean result.

## Hardware round-trip

These verbs are the single entry point for sending the probe to a machine and reading back what
happened. The scripts under `scripts/` implement them and are not run directly.

| verb | what it does |
|---|---|
| `deploy` | the whole round-trip: build the package, install it, launch it, capture the report |
| `native` | lay out the ps5 native title directory (prospero eboot + `param.json` + `icon0.png`) |
| `native --deploy` | build the native title and push it to a scan root (default `/user/data`) via prosperous, where an auto-mounter registers it |
| `sweep` | the payload, package and native eboot legs in series |
| `payload` | build the plain-ELF payload, run it through `elfldr`, capture the system log |
| `inject` / `injector` | the same, through the native process injector - see [INJECTOR.md](INJECTOR.md) |
| `report` | capture obSCEne's records from the system log into a file |
| `klog` | an alias for `report`, named after the channel it reads |
| `pull-log` | pull the newest obSCEne report file from the console |
| `recover` | read-only: what the machine recorded, after a crash and before any relaunch |
| `prep` | send the `klogsrv`/`shsrv` payloads to bring the readable services up |
| `hwsweep` | iterate against hardware, excluding each call that does not return |
| `minbuild` | build the minimal diagnostic package |
| `digcheck` | whether a built package still agrees with its own digests |
| `restart-ui` | restart the system UI without rebooting |

The report is captured from the **system log**, never pulled off disk: a packaged run's report
file is sealed `0600` inside the title's sandbox where `ftpsrv` cannot read it.

Every C target builds under WSL. `hw install` runs from Windows: under WSL2's default NAT the
tool binds an address the machine cannot reach, and the install reports `fetched 0 time(s)`.
The verbs handle the split; the repository `CLAUDE.md` explains it.

## Make targets

```bash
./bin/obscene eboot
./bin/obscene module-min
./bin/obscene payload-build HARDWARE=1 BUILD_ID=$(git rev-parse HEAD)
```

### Variables

| | default | what it does |
|---|---|---|
| `TARGET` | `prospero` | hardware target: `orbis` (PS4), `neo` (PS4 Pro), `prospero` (PS5), `trinity` (PS5 Pro) |
| `GEN` | (alias) | alias for `TARGET` (`GEN=4` selects `orbis`, `GEN=5` selects `prospero`) |
| `HARDWARE` | unset | the hardware-facing build |
| `BUILD_ID` | `dev` | stamped into the artefact, so a report names the build that produced it |
| `BULK` | unset | the blind prober; **refused together with `HARDWARE=1`** |
| `BASELINE` | `build/baseline.txt` | what `make diff` compares against |

`HARDWARE=1 BULK=1` does not build. CI tries it and fails if it succeeds, then checks every
shipped artefact for the string `built without OBS_BULK`.

### Shapes

[ARTIFACTS.md](ARTIFACTS.md) is the authority; read it before sending anything anywhere. The
shapes are told apart by two bytes, and the wrong one takes a hardware loader down.

| target | file | loader |
|---|---|---|
| `payload` | `build/obscene-probe-prospero.elf` | plain ELF, homebrew loader |
| `injector` | `build/obscene-injector.elf` | plain ELF, native process injector |
| `module` | `build/obscene.module.elf` | vendor ELF, emulators |
| `eboot` | `build/eboot.bin` (`obscene-probe-prospero.zip`) | the system loader (orbis by default; `TARGET=prospero` for the current-generation container) |
| `pkg` | `build/obscene-probe-orbis.pkg` | the installer (ps4-format, previous-generation) |
| `native` | `build/prospero/<TITLE_ID>/` | a ps5 native title directory (prospero eboot); registered under `/user/app` by an auto-mounter or `AppInstallTitleDir` |
| `host` | `build/obscene-host` | your own machine |

The **host build** runs the harness on an ordinary machine against stubs that fail everything,
so the framework is verifiable before any emulator loads it and a probe bug is distinguishable
from a platform bug.

## The check target

`make check` builds the module, payload and host shapes, then runs:

1. the tooling's tests
2. formatting and lints (`scripts/lint.sh`)
3. the host harness, into `build/host-report.txt`
4. `obscene-tool verify` on that report
5. `obscene-tool imports` on the built module

## Measurement loop

```bash
make host && ./build/obscene-host > baseline.txt
# ...change the emulator, or the target...
make diff BASELINE=baseline.txt
```

## CI

`.github/workflows/ci.yml` builds and uploads each shape in its own job:

| job | what it establishes |
|---|---|
| build and verify | the tooling builds, tests and lints; the module builds; the host harness runs and its report verifies; the NID chain derives |
| shape: plain ELF payload | the `HARDWARE=1 BULK=1` guard still refuses, and no shipped artefact carries the blind prober |
| shape: native process injector | it builds |
| shape: vendor ELF | `e_type` is `0xFE10`, read out of the file rather than trusted |
| shape: title directory | it builds |
| shape: package | it builds; `continue-on-error` |
| provenance | no binary or vendor-shaped material is tracked, and no path reserved for it |
| formatting | `clang-format --dry-run --Werror` over `src` and `include` |
| publish the shapes | names each shape for the loader that takes it |

Every job reaches the build through `bin/obscene`, so CI and a person run the same command.

## From the collection

[OOPS](https://github.com/project-oops/OOPS) holds the projects side by side:

```bash
./bin/oops check obscene        # also: build, test, pkg
```

That relays to this script, and it is the recommended way in on Windows because of the WSL
handling above. [The collection's BUILDING.md](https://github.com/project-oops/OOPS/blob/main/docs/BUILDING.md)
covers `bootstrap`, `gates`, `all` and the rest.
