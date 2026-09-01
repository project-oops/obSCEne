# CLAUDE.md

How obSCEne is built and the constraints to honour when changing it.

**Read [the OOPS conventions](https://github.com/project-oops/OOPS/blob/main/docs/CONVENTIONS.md) first.** Provenance, naming, decision logs, worklogs and
gates are shared across [Orbistoun](https://github.com/project-oops/Orbistoun),
[obSCEne](https://github.com/project-oops/obSCEne),
[Prosperous](https://github.com/project-oops/Prosperous) and
[SELFish](https://github.com/project-oops/SELFish), and are stated once there. This file holds
only what obSCEne adds.

## Mission, in one breath

A conformance probe for Prospero-generation hardware, in freestanding C. It calls the
system functions a title would call, one at a time, and reports what each one
actually did. It is ordinary homebrew - nothing in it is emulator-specific.

## Principles

### 1. Announce before attempting

Every check writes its identity before making the call, unbuffered. A `try` record
with no matching `res` means exactly one thing: that call did not return.

This is the most important property in the program. Any change that buffers output,
batches records, or emits an announcement for a check that will not run has broken it.
A skipped check emits no `try`, for the same reason.

### 2. Nothing is invented

*The shared form is [OOPS conventions §3](https://github.com/project-oops/OOPS/blob/main/docs/CONVENTIONS.md#3-honest-failure-over-plausible-output);
below is how it binds a probe in particular.*

Where an arity, a constant or a struct layout is uncertain, the function is **left
out**. A wrong arity corrupts the stack and crashes somewhere unrelated; a wrong
constant makes the call succeed and do the wrong thing silently. Both cost far more
than the declaration saves.

This program's entire value is that its report can be trusted. Adding a function is a
matter of confirming the signature, not of making something compile. (D008)

### 3. The report is an interface

`docs/OUTPUT.md` is a contract and parsers exist. Field order and meaning do not
change without bumping the version; new fields append to the end of a line only.

Check identifiers are the key when diffing runs. Renaming one silently breaks every
historical comparison, so treat them as you would a public API.

### 4. Two builds, and the host one is not optional

`make host` runs the harness against stubs on an ordinary machine. It is what makes a
bug in the probe distinguishable from a bug in the thing being measured. Keep it
building and keep `make check` passing. (D001)

### 5. Naming, and where this project's ABI exception lands

The convention is shared - see [OOPS conventions §2](https://github.com/project-oops/OOPS/blob/main/docs/CONVENTIONS.md#2-naming-no-vendor-brands-in-prose-or-in-our-own-api).

**What is specific here:** the symbol and library name strings in
`include/obscene/platform.h` and the check tables are **ABI identifiers**. The import
hash is computed from the symbol name. They stay exactly as the platform spells them.

### 6. Provenance, and what it means for a probe

The boundary is shared - see [OOPS conventions §1](https://github.com/project-oops/OOPS/blob/main/docs/CONVENTIONS.md#1-provenance-is-a-hard-boundary).

What is specific here: **no vendor headers and no SDK.** Every declaration this program
makes comes from public interface documentation or an open-source toolchain. A probe that
borrowed a header would be reporting the vendor's opinion of the interface rather than
measuring it, which is the one thing it exists not to do. If you cannot say where a
declaration came from, that is the signal.

### 7. Positive checks are worth more than negative ones

Checking from the failure side proves argument validation and nothing else - an
implementation that fails everything passes every negative check. The memory round
trip and the thread that must actually run its body are what prove a function works.

The suite currently leans negative because layouts are unknown. That ratio should
shift, and every new confident signature is an opportunity to shift it. (D007)

### 8. Freestanding means freestanding

**The runtime** brings in no libc, no allocation, no floating point and no variadics. If
something needs a helper, it goes in `runtime.c` and it stays short - every function
added there is one more thing that can be wrong while diagnosing something else.

**The checks are a different matter, and this used to read as though they were not.**
`037-math` is thirteen floating-point checks and `035-libc/snprintf` calls a variadic,
because calling those is the entire job - the platform provides them and the probe is
here to find out whether they work. What the rule forbids is *this program* depending on
them to do its own work: no float in the harness, no `printf` to build a report line, no
allocation to hold results.

The distinction matters because the previous wording made two whole sections look like
violations of a stated principle, and a rule that the code visibly breaks is a rule
nobody applies.

## Working sessions

**Read `docs/DECISIONS.md` and `docs/WORKLOG.md` at the start of every session.** They
are the durable memory; the conversation that produced them is not.

- Every non-obvious choice gets a numbered entry in `DECISIONS.md` **as it is made**.
  Include the reasoning - that is what stops it being re-litigated.
- A choice made without input is status `assumed` and goes in the *Needs review*
  index. Assume freely and keep moving.
- Append to `WORKLOG.md` at the end of every completed unit of work. **Record
  surprises especially.**
- Run `make check` before logging anything as done.

## Building on Windows: use WSL, not multipass

**Do not use multipass. It was the build environment until 2026-08-26 and it is not any
more.** Everything runs in the WSL2 Ubuntu distro:

```bash
wsl.exe -d Ubuntu -- bash -lc 'export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"; cd <OOPS>/obscene && make check BUILD=$HOME/obs'
```

The repository is visible at `<OOPS>/obscene`, and so is `<OOPS>/prosperous`, which
`tool/Cargo.toml` now takes a path dependency on - under multipass that needed a separate
mount and broke the tool build until someone noticed.

### Why multipass was dropped, which is worth knowing before reviving it

Its VM **restarted underneath running commands**. The symptom is not a failure, it is
*different* failures: three consecutive `verify.sh` runs failed on `guards`/`caps`, then
`shaders`/`build-all`, then `cargo test`/`clippy`, and **every one of them passed when re-run
alone**. The cause was visible only in a background log:

```text
exec failed: cannot connect to the multipass socket
Starting obscene-build  /-\|/-\|...
```

Whichever gate happened to be executing when the VM went away failed. Hours went into
diagnosing that as a code problem. An intermittently-failing gate is worse than a failing one,
because it teaches you to re-run until green - which is exactly how the silent measurement loss
in D181 survived as long as it did.

It also had 4 GB of disk against WSL's 954 GB, and booted on every invocation.

### The rules that carry over unchanged

**`BUILD` must be a Linux-local path** - `$HOME/obs`, never `/mnt/c/...`. A Windows mount
cannot carry the execute bit, and `$(BUILD)/symbols.txt` is produced by *running* the host
binary, so `module` fails without it. Same for `CARGO_TARGET_DIR`, or every rebuild crawls
through the 9p mount. (D012)

**`MSYS_NO_PATHCONV=1` is still needed, and this file said otherwise for a day.** The claim was
that nothing passes a Linux path through Git Bash to a Windows program any more. Three things
do, and all three broke on it:

- `wsl.exe -d Ubuntu -- bash <OOPS>/obscene/script.sh` - Git Bash rewrites the `/mnt/...`
  argument to `C:/Program Files/Git/mnt/...` and the script "does not exist"
- `tasklist /FI "IMAGENAME eq x.exe"` - `/FI` is rewritten into a path and the filter silently
  stops filtering, which is worse than failing
- every call in `scripts/wsl.sh`, for the same reason multipass needed it: `wsl.exe` is as
  Windows a program as `multipass.exe` was, and the paths it is handed are Linux ones

The rule is unchanged from the multipass era and was never about multipass. (D199)

### The toolchain, if the distro is ever rebuilt

```bash
sudo apt-get install clang lld binutils gcc libc6-dev make
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
rustup component add clippy rustfmt
```

**`gcc` is the non-obvious one.** Rust invokes `cc` to link build scripts even when everything
else is compiled with clang, so a clang-only box fails with a wall of
`could not compile <crate> (build script)` and no mention of a missing linker. `clippy` and
`rustfmt` are absent from the minimal profile and `verify.sh` needs both.

### Which side runs what: WSL builds, Windows installs

Both, and the split is not a preference - it is a networking constraint that cost most of a
session to find.

**In practice you never pick a side - the verbs do.** The hardware round-trip is a set of
`./bin/obscene` verbs, and each runs whatever halves it needs (re-entering WSL to build, shelling
out to the Windows-native tool to install). This is the whole loop, from a code change to a report
other tools can read (D269):

```bash
# Build, install, launch, and capture the report - the entire round-trip, one command.
# Re-enters WSL to build; installs from Windows so the console can reach us; captures the report
# off the system log as the run happens. Flags pass through: --gen, --seconds, --name,
# --build-only, --deploy-only, --no-cache, --jobs.
./bin/obscene deploy

# Just capture the report, for a title already running (or relaunched by hand). Writes
# reports/hardware/console-klog.txt and prints the meta/build/tally/end lines so you can see at a
# glance whether the run finished.
./bin/obscene report --seconds 120

# After a crash, before any relaunch: what the console recorded, read-only.
./bin/obscene recover
```

The report lands in `reports/hardware/console-klog.txt` (gitignored). It is captured from the
**system log**, never pulled from disk: a packaged run's report file is sealed `0600` inside the
title's sandbox where ftpsrv cannot read it, and the system log is the one channel that leaves it
(D233, D237, D269). `./bin/obscene report` is `obscene-tool report`; it connects out, so it runs
from either side. `./bin/obscene help` lists every hardware verb; `scripts/README.md` maps each to
its script. **Reach the round-trip through these verbs, never by running one of the underlying
scripts directly** - that is how the scripts multiplied in the first place.

The lower-level `hw` subcommands the verbs are built from still split by direction:

| what | where | why |
|---|---|---|
| every C target (`host`, `module`, `payload`, `eboot`, `pkg`) | **WSL** | the host build is POSIX; see the section below |
| `hw check`, `send`, `logs`, `sh`, `ls`, `pull`, and `report` | either | these **connect out** to the hardware, and NAT forwards outbound fine |
| **`hw install`** | **Windows** | the hardware must connect **in**, to us |

`hw install` serves the package over HTTP and has the hardware fetch it (`pkg_install` takes a
URL; a bare path and `file://` are both refused). `pros_core::handover` binds the interface that
routes to the target, which is correct - but under WSL2's default NAT that interface is
`172.24.x.x`, an address the hardware cannot reach. The install then fails in the most misleading
way available: the shell prints its usual line, the hardware never sends a request, and the
handover reports `fetched 0 time(s)`. Nothing says "unreachable".

Run it from Windows and the same code binds `192.168.1.x`, the hardware fetches, and the log
fills with `libhttp/12.40 (PlayStation 5)` range requests.

```powershell
cd <OOPS>\obscene\tool
$env:CARGO_TARGET_DIR = "<OOPS>\obscene\tool\target-win"
cargo build --bin obscene-tool
.\target-win\debug\obscene-tool.exe hw install C:\path\to\obscene.pkg
```

The package itself is still built in WSL; only the serving of it has to happen where the hardware
can reach. `fetched 0 time(s)` in the install output is the tell - it means the hardware never
came, so the package was never judged, and nothing about the package is implicated.

**Mirrored networking would remove the split.** `networkingMode=mirrored` in `.wslconfig` gives
WSL the host's addresses; it was not tried, because a second build target proved quicker than
changing how every WSL session on the machine is networked.

### Building on Windows directly does not work, and the reason is structural

The module target is freestanding and cross-compiles fine - Windows has clang and `ld.lld`.
But `module` depends on `$(BUILD)/symbols.txt`, which is generated by **running**
`obscene-host --symbols`, and the host build is POSIX: `unistd.h`, BSD sockets in
`net_posix.c`, pthreads in `host_stubs.c`, plus `-Werror` against MSVC's `wcslen` conflict and
`getenv` deprecation. Porting that would add a permanent Windows-specific host path to a
project that has one clean POSIX one. It is not worth it; use WSL.

## Before adding a check

Five steps, in order, because the last one has caught a wrong check twice:

1. Move the name from the census in `surface.h` to a real declaration in `platform.h`.
   The census declares its names as `const char` so the type system forbids calling them,
   so a name cannot be in both places.
2. Add it to `src/probe/imports.c`, or `mkmodule` refuses the build - it will not guess which
   library an import comes from.
3. Add it to the `@called-elsewhere` block in `data/surface.txt` **and remove it from its
   group there**, or regenerating the census puts it back and the build breaks on the
   duplicate declaration. Ten names were left half-moved this way and the generator
   silently stopped working; `verify.sh` gates it now.
4. Give it a provenance: `OBS_FROM_SPEC` where ISO C or POSIX settles the answer,
   `OBS_FROM_ASSUMED` otherwise. Assumed is the default and is usually the honest one.
5. **Run it under `make host` before believing anything it says.** A check that has not
   passed a known-good implementation is not evidence. This has caught a probe whose two
   "must differ" inputs did not differ, and a set of checks placed before the capability
   they required.

A check that calls a symbol other than its own must test that address before calling
it. The harness only guards the one symbol in the check's table row; every platform
declaration is weak, so anything else the body reaches for can be null and jumping to
zero ends the run. This cost a segmentation fault on the host build. (D058)

A check that loops should report progress (see `015-sync/thread-churn`): a crash inside a
loop otherwise names only the check, and "fails on the first" and "fails on the fortieth"
are different bugs.

Anything that can block - a lock, a wait, a blocking read - is written as the `try` form
or not at all. A probe that hangs loses every check behind it, which has happened twice.

## Where things live

- `../selfish` - **the file formats, and this repository depends on it.** The import hash,
  the executable format, the vendor dynamic table in both directions, the container, the
  linker script, and the format tables that used to sit in `data/`. It is a sibling checkout,
  the same convention `pros-link` already uses; the repositories are checked out as a set.
  See D200 for what moved and D201 for what the move found.

  The line is: a **format** is shared, a **measurement** stays with whatever measured it. So
  the manifest saying which library resolves which name is still here, and so is the `$` sigil
  marking a symbol whose name *is* the identifier - that is a convention of this project's
  symbol tables, not a fact about the format.
- `data/` - the source of truth for everything generated, and now **only this project's own
  measurements**: `surface.txt` (the curated census), `mined-names.txt` and `unnamed-nids.txt`
  (the mined corpus), `gpu-surface.tsv` (the GPU ISA classification), `font.txt` (glyph art),
  `nid-corpus.txt`. These are data - judgements with prose attached - and they live outside
  the generators deliberately: retyping a judgement into a new language is how a
  transcription error gets into a census.

  The format tables that used to be here - `hash-suffix.toml`, `self-format.tsv`,
  `pkg-format.tsv` - went to `selfish`, because those are judgements *two other projects also
  need*, and a judgement with two homes eventually has two values.
- `include/obscene/platform.h` - every platform declaration and ABI constant.
- `src/probe/sections/` - the checks, grouped by layer.
- `src/probe/registry.c` - the running order, as one explicit list.
- `tool/` - the Rust tooling: `mkmodule`, `derive`, `verify`, `pretty`, `diff`, `nid`. The
  format work these do is `selfish`'s; what is here is the manifest, the census, the checks,
  and the gates.
- `tool/` also holds every gate and generator - `guards`, `caps`, `counts`, `doccheck`, `compat`,
  `protocol`, `corpus`, `decisions`, and the generators `surface`, `census`, `font`,
  `shaders`, `gpusurface`, `mine`. `docs/TOOLING.md` lists them. There is no Python in this
  repository; `scripts/` is orchestration only.
- `scripts/` - `verify.sh` runs everything that has to pass. `sweep-build.sh` rebuilds
  with the exclusion list, and must be the last build before anything runs the module:
  `build-all.sh` and `make check` both build without exclusions, which has twice handed
  a later step a module that walked into a known crash.

  **See `scripts/README.md` before adding one.** This directory reached fifty-four ad-hoc
  probes and forty-five of them were dead - thirty-seven without so much as a description.
  The cause is structural rather than sloppy: `wsl.exe -- bash -lc '...'` mangles its argument
  under Git Bash, so the only reliable way to run anything is to write a file, and every
  question asked became one. A question about a *file's contents* belongs in
  `../selfish/crates/*/examples/` as a probe that takes an argument; a fact about *our own
  output* belongs in a test. What is left here crosses a boundary neither of those can.
- `docs/ARTIFACTS.md` - which file goes where. Each shape reaches a different loader,
  they are told apart by two bytes at offset 16, and sending the vendor build to the hardware's
  homebrew loader took that loader down and cost a reboot. The Makefile said so in a comment
  beside the rule, which is where you look once you already know there is a distinction.
- `docs/OUTPUT.md` - the report contract.
- `docs/GPU_SURFACE.md` - the GPU ISA census: the AMD hardware math operations and obSCEne's
  coverage of each. Generated by `obscene-tool gpusurface` (source of truth is
  `data/gpu-surface.tsv`, cross-checked against LLVM's `IntrinsicsAMDGPU.td`).
- `reports/gpu-golden.txt` - a blessed snapshot of what the build VM's llvmpipe computes for
  every kernel. `scripts/gpu-golden.sh --capture` re-blesses it; `--check` (run by `verify.sh`)
  fails on any divergence on the matching device and skips on a different one. It catches the
  kernel-output regressions the reference oracle cannot (the transcendentals).
- `scripts/gpu-analyze.sh <corpus>` - the hardware-day analysis in one command: reference
  (`gpuref`) + exact diff (`gpudiff`) + ULP ranking (`gpustats`) over any GPU corpus. Works on
  the golden today; the same command over a Deck/PS5 corpus later. See `docs/DECK.md`.
- `docs/DECK.md` - the runbook for capturing and analysing a real RDNA2 corpus on a Steam Deck
  (build with `make deck`, run/serve, capture, analyse, diff against emulation).
- `docs/GNM.md` - the platform's GPU API (libSceGnmDriver), both axes: the command-builder PM4
  probe (built, `src/probe/sections/gnm.c`) and the compute-result-via-Gnm path (scoped, needs GCN
  shaders - feasible via LLVM's AMDGPU backend, gated on input struct-layout confirmation).
- `docs/EMULATORS.md` - the emulator toolkit in `<emulators>`: every emulator and
  format reference in the kit, what each is good for, and the provenance boundary when
  reading them. The run scripts default to the downloaded shadPS4 there - which D149 is a
  reason to revisit, since the build from its own source no longer loads the corpus module.
- `patches/` - local changes to other people's emulators, stored rather than left in a
  working tree nobody else has. Nothing applies them automatically and nothing in obSCEne
  depends on them. A loader patched only to *build* is an ordinary loader; one patched to
  change its **behaviour** may still be run, but its report never occupies that loader's row
  in `COMPATIBILITY.md` - `reports/kyty.txt` is the stock result, `reports/kyty-patched.txt`
  the other. A row naming a build only this machine has is an invented constant at a larger
  scale. (D176)
- `docs/DECISIONS.md`, `docs/WORKLOG.md` - durable memory.

