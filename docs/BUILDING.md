# Building obSCEne

There is one command and it is `bin/obscene`. It is a **front door, not an implementation** -
the build itself is the `Makefile`, and anything this script does not recognise is passed
straight through to `make`, so every target still works by name.

```bash
./bin/obscene check     # all three shapes, the tooling's tests, the host harness, verification
```

## What you need

| | |
|---|---|
| `clang` and `lld` | the probe is freestanding C. Nothing else compiles it |
| `make` | the build is a Makefile |
| a Rust toolchain | for `tool/`, the offline tooling. Not for the probe itself |
| `clang-format` | only for `fmt`, and only if you are changing the C |

**A conformance probe that needs a vendor toolchain to build is a probe most people cannot
run.** So it needs none: no SDK, no firmware, no signing keys. That constraint is the reason
anyone else can reproduce a report.

### Siblings

**obSCEne does not build from a clone of only this repository.** It resolves two of them by
relative path, as siblings, so the directory layout is a build requirement rather than a
convenience:

```
selfish      every platform file format - the module, the eboot, the package
prosperous   (and oops-libs, through it)
```

```bash
./bin/oops bootstrap obscene    # fetches exactly those, and nothing else
```

Without SELFish beside it, `host` builds and nothing that a loader or an emulator accepts
does. That is not an accident of layout: producing a *format* goes through SELFish precisely
so that a wrong magic or a stale tag cannot be introduced here in isolation.

`SELFISH ?= ../selfish` in the `Makefile` is the only knob, if your checkout is arranged
differently.

### Windows

There is usually neither `clang` nor `lld` under Git Bash. `oops` detects that and re-enters
through WSL rather than failing with a compiler error that reads as a code fault:

```bash
./bin/oops check obscene        # from the collection root, on Windows
```

`OOPS_NO_WSL=1` refuses instead of delegating. The path-translation traps this had to solve
are documented in
[the collection's BUILDING.md](https://github.com/project-oops/OOPS/blob/main/docs/BUILDING.md#windows-wsl-and-why-obscene-is-different)
- they matter because both fail by pointing at the wrong thing.

## The verbs

The seven shared ones, so `oops test obscene` and `./bin/obscene test` are one command reached
two ways:

| verb | what it does |
|---|---|
| `build` | module, payload, injector, host |
| `test` | **the tooling's** tests - `cd tool && cargo test` |
| `lint` | the tooling's lints, in full |
| `fmt` | format the C in place |
| `check` | the full gate. What CI runs |
| `clean` | remove `build/` |
| `doc` | the tooling's API docs |
| `pkg` | the installable package |

`test` is the tooling's own tests rather than `make check`. Those were the same command until
this script existed, which made `test` and `check` indistinguishable and said nothing true
about either.

And the tooling's own subcommands, named so CI reaches them the same way a person does rather
than reaching past the front door to `cargo` and a raw binary path:

| verb | what it does |
|---|---|
| `tool-build` | build `obscene-tool` |
| `guards` | the cross-repository symbol guards |
| `imports <file>` | what a built object imports |
| `verify <report>` | the report is in the documented format |
| `selftest` | the NID chain still derives |

`fmt` does not use `clang-format -i`. That writes by rename, which a mounted Windows share
refuses after the temp file is written - leaving `foo.c.temp-stream-XXXX` and no `foo.c`.
`scripts/format.sh` redirects and truncates instead.

`lint` is a script rather than a one-liner for a reason worth knowing before you simplify it:
the earlier version piped clippy into `grep ... || echo clean`, so clippy failing to *run*
printed `clean`. A tree nobody had linted was indistinguishable from a clean one.

## The hardware round-trip

obSCEne-only - no counterpart in the other three projects, because no other project sends
itself to a machine and reads back what happened. These are the single entry point for that
work: the scripts under `scripts/` are their implementation and are **not** run directly,
which is how that directory reached fifty-four ad-hoc probes once already.

| verb | what it does |
|---|---|
| `deploy` | the whole round-trip: build the package, install it, launch it, capture the report |
| `native` | lay out the ps5 native title directory (gen-5 eboot + `param.json` + `icon0.png`) |
| `native --deploy` | build the native title and push it to a scan root (default `/user/data`) via prosperous, where an auto-mounter registers it - see [D291](decisions/D291-native-deploy-uploads-the-title-dir.md) |
| `payload` | build the plain-ELF payload, run it through `elfldr`, capture the system log |
| `inject` / `injector` | the same, through the native process injector - see [INJECTOR.md](INJECTOR.md) |
| `report` | capture obSCEne's records from the system log into a file |
| `klog` | an alias for `report`, named after the channel it reads |
| `recover` | read-only: what the machine recorded, after a crash and before any relaunch |
| `prep` | send the `klogsrv`/`shsrv` payloads to bring the readable services up |
| `hwsweep` | iterate against hardware, excluding each call that does not return |
| `minbuild` | build the minimal diagnostic package |
| `digcheck` | does a built package still agree with its own digests? |

The report is captured from the **system log**, never pulled off disk: a packaged run's report
file is sealed `0600` inside the title's sandbox where `ftpsrv` cannot read it, and the system
log is the one channel that carries it out.

Which side runs what is not a preference. Every C target builds under WSL, and `hw install`
must run from Windows - under WSL2's default NAT the tool binds an address the machine cannot
reach, and the install fails by looking like nothing happened at all (`fetched 0 time(s)`).
The verbs handle that themselves; `CLAUDE.md` has the reasoning.

## Anything else goes to make

```bash
./bin/obscene eboot
./bin/obscene module-min
./bin/obscene payload HARDWARE=1 BUILD_ID=$(git rev-parse HEAD)
```

### The variables

| | default | what it does |
|---|---|---|
| `GEN` | `5` | which generation the module is marked for. `GEN=4` builds one a previous-generation emulator will accept, since those refuse a current-generation mark |
| `HARDWARE` | unset | the hardware-facing build |
| `BUILD_ID` | `dev` | stamped into the artefact, so a report names the build that produced it |
| `BULK` | unset | the blind prober. **Refused together with `HARDWARE=1`** |
| `BASELINE` | `build/baseline.txt` | what `make diff` compares against |

`HARDWARE=1 BULK=1` does not build, deliberately, and CI asserts that it still does not: a job
tries it and fails if it *succeeds*. Every shipped artefact is then checked for the string
`built without OBS_BULK`. A guard is not finished until somebody has made it fail.

### The shapes, and which loader each reaches

`docs/ARTIFACTS.md` is the authority and should be read before sending anything anywhere -
they are told apart by two bytes, and sending the wrong one to real hardware cost a loader and
a reboot.

| target | file | loader |
|---|---|---|
| `payload` | `build/obscene-payload.elf` | plain ELF, homebrew loader |
| `injector` | `build/obscene-injector.elf` | plain ELF, native process injector |
| `module` | `build/obscene.module.elf` | vendor ELF, emulators |
| `eboot` | `build/eboot.bin` (`obscene-eboot.zip`) | the system loader (gen-4 by default; `EBOOT_GEN=5` for the current-generation container) |
| `pkg` | `build/obscene.pkg` | the installer (ps4-format, previous-generation) |
| `native` | `build/native/<TITLE_ID>/` | a ps5 native title directory (gen-5 eboot); registered under `/user/app` by an auto-mounter or `AppInstallTitleDir` |
| `host` | `build/obscene-host` | your own machine |

**The host build matters more than it looks.** It runs the harness on an ordinary machine
against stubs that fail everything, so the framework is verifiable *before* any emulator can
load it. Without it the first run happens inside something that does not work yet, and a bug
in the probe is indistinguishable from a bug in the thing being measured.

## What `check` runs

`make check`, which is: build all three shapes, then

1. the tooling's tests
2. the host harness, into `build/host-report.txt`
3. `obscene-tool verify` on that report - it is in the documented format
4. `obscene-tool imports` on the built module

## The measurement loop

The report format exists for this: change something, re-run, ask whether it helped.

```bash
make host && ./build/obscene-host > baseline.txt
# ...change the emulator, or the target...
make diff BASELINE=baseline.txt
```

## What CI runs

`.github/workflows/ci.yml`, in six jobs - more than one because the shapes are built and
uploaded separately and each proves something different:

| job | what it establishes |
|---|---|
| build and verify | the tooling builds, tests and lints; the module builds; the host harness runs and its report verifies; the NID chain derives |
| shape: plain ELF | the `HARDWARE=1 BULK=1` guard still refuses, and no shipped artefact carries the blind prober |
| shape: vendor ELF | `e_type` really is `0xFE10`, read out of the file with `od` rather than trusted |
| shape: fSELF eboot | it builds |
| shape: package | it builds. `continue-on-error`, and honestly so |
| provenance | no binary or vendor-shaped material is tracked, and no path reserved for it |
| formatting | `clang-format --dry-run --Werror` over `src` and `include` |

Every one of them reaches through `bin/obscene`. The moment the command CI runs and the
command a person runs are different commands, one of them is untested - and it is always the
one nobody watches.

## From the collection

[OOPS](https://github.com/project-oops/OOPS) holds all four side by side:

```bash
./bin/oops check obscene        # also: build, test, pkg
```

That relays to this script rather than reimplementing anything, and it is the recommended way
in on Windows because of the WSL handling above.
[The collection's BUILDING.md](https://github.com/project-oops/OOPS/blob/main/docs/BUILDING.md)
covers `bootstrap`, `gates`, `all` and the rest.
