# Emulator toolkit

Reference copies of PS4 and PS5 emulators, with the format and symbol references that go
with them. They live in `<emulators>`, outside this repository: they are large third-party
binaries and source, and neither belongs in the history of a probe.

```
<emulators>\
    shadps4\        binary, 0.18.0 - the scripts' default
    kyty\           binary (KytyPS5)
    src\            source, shallow clones
<OOPS>/orbistoun\      the sibling project, built from its own tree
```

[COMPATIBILITY.md](COMPATIBILITY.md) holds what each loader does with obSCEne, generated
from the reports. This file holds what each loader is and how to drive it.

## Roster

### PS5

| | |
|---|---|
| `craziiEmu` | C#, built with dotnet. A derivative of SharpEMU. |
| `SharpEMU` | C#, Windows/Linux/macOS. The origin of craziiEmu's loader. |
| `PS5PCEM` | Zig, built with `zig build`. Implements the current generation's graphics interface (AGC, an RDNA2-to-SPIR-V shader translator). Its `module-info` tool is an independent parser for the module format. |
| `KytyPS5` | The `kyty\` binary. A fork of Kyty; refuses previous-generation executables. |
| `prosper` | C++20 user-space compatibility layer: guest code runs natively and the OS beneath it is reimplemented, the same architecture as orbistoun. Linux build only. |
| `ChonkyStation4` | C++, low-level: links the guest against real firmware `.sprx` modules. |
| `orbistoun` | The sibling project, a Rust HLE emulator. See below. |

### PS4

| | |
|---|---|
| `shadPS4` | The most complete. Its `CommonStub` logs by name, which is the source of `data/nid-corpus.txt`. |
| `Kyty` | Original Kyty, the 2022 clone in `src\Kyty`. Names every import it cannot resolve. |
| `fpPS4` | Pascal, a wholly independent implementation. |
| `GPCS4` | Graphics-focused. |
| `rpcsx` | Rust and C++. |
| `obliteration` | A PS4 kernel rewritten in Rust, run under a custom hypervisor. The closest thing to a written specification of kernel behaviour. |
| `orbital` | Virtualization-based. |

### Format and symbol references

| | |
|---|---|
| `OpenOrbis-PS4-Toolchain` | Headers under `include/orbis`. An open-source toolchain, an acceptable source for signatures under CLAUDE.md principle 6. The stub archives ship in releases, not in the repository. |
| `ps5-payload-dev/sdk`, `elfldr` | PS5 homebrew SDK and ELF loader. The nearest reference implementation of what obSCEne produces. |
| `ps4_module_loader` | An IDA loader for the vendor module format. An independent account of the dynamic tables. |
| `ps4libdoc` | The symbol database. See below. |

## Independence

Loaders that read each other count as one witness, not several.

- **craziiEmu and SharpEMU are one loader.** Their `SelfLoader.cs` files are near-identical and
  craziiEmu's copyright header names SharpEmu first. The compatibility table never presents
  them as two data points.
- **prosper reads Kyty and shadPS4** and cites them by name. Where it agrees with Kyty it is
  not independent evidence. Its own `CONFIDENCE:` annotations grade each claim: an annotated
  claim citing FreeBSD or a live capture is strong; an unannotated constant is a bare choice.
- **fpPS4's `ps4libdoc.pas` is derived from ps4libdoc**. Hashing its names reproduces
  every identifier, which confirms this project's hash against firmware-extracted values, but
  it is one source.
- **orbistoun and obSCEne are developed in concert**, so their agreement is weighed below
  agreement between third-party loaders. `obscene-tool consensus` prints the same warning.

## Built from source

A loader's behaviour is explained only from the source that built the binary running it
(D094). A binary still in use marks any claim about *why* it behaves as it does as a claim
about a different commit.

| | build |
|---|---|
| craziiEmu | `dotnet build CraziiEmu.slnx -c Release` |
| SharpEMU | `dotnet build SharpEmu.slnx -c Release` |
| fpPS4 | `lazbuild fpPS4.lpi`, plus its own FFmpeg archive |
| PS5PCEM | `zig build` |
| Kyty | MinGW/ninja, with `patches/kyty-buildable.patch` |
| shadPS4 | CMake with clang-cl, no Qt |
| prosper | CMake + Ninja, Linux (run in WSL) |
| ChonkyStation4 | CMake with clang-cl |

The `kyty\` binary is KytyPS5, a fork; its window title states the build
(`KytyPS5-<date>-<commit>`), and that commit is not in the `src\Kyty` clone. Nothing about the
binary's behaviour is explained from the clone. Ask the binary how to drive it.

### fpPS4

Free Pascal 3.2.2 from winget is sufficient; the README's 3.3.1 trunk is not required.

```bash
winget install Lazarus.Lazarus            # the IDE; ships lazbuild
winget install FreePascal.FreePascalCompiler
lazbuild fpPS4.lpi                        # there is only the default build mode
```

At runtime it needs FFmpeg 4.x shared libraries (`avutil-56`). The project's CI names the
source:

```bash
curl -kL https://github.com/red-prig/fpps4-bin/raw/main/ffmpeg.zip -o ffmpeg.zip && unzip ffmpeg.zip
```

### Kyty

Its CI uses MinGW/GCC, not the clang-cl its README suggests. `patches/kyty-buildable.patch`
makes `KYTY_WARNINGS_ARE_ERRORS` overridable (its bundled SDL2 collides with a current
Windows SDK declaration under `-Wshadow`) and adds a `KYTY_NO_LAUNCHER` switch that skips the
Qt launcher without compiling the emulator out.

### shadPS4

- `-DENABLE_QT_GUI=OFF` removes the Qt dependency.
- `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` lets CMake 4 accept its dependencies.
- On a shallow clone, `git submodule update --init` with no paths registers nothing. Init
  each path from `.gitmodules` individually, then run a second recursive pass for nested
  submodules such as `sirit/externals/SPIRV-Headers`.
- The compiler is clang-cl, as its CI sets:

```
-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
```

MSVC fails on `imgui_demo.cpp` and `libusb.h`; neither is a project defect.

### ChonkyStation4

It uses `__attribute__((sysv_abi))`, so it needs clang-cl. Its CMake needs `VULKAN_SDK` set
in the environment. Two lines need patching for a newer Vulkan-Hpp, where `float[4]` no
longer binds to `const std::array<float, 4>&`.

## Running them

```bash
BUILD=/tmp/obs-shad sh scripts/sweep-build.sh   # shadPS4, GEN=4, its own exclusion list
sh scripts/run-kyty.sh                          # Kyty, GEN=5, captures the unresolved log
sh scripts/run-emulator.sh --emulator <path-to-emulator-exe>
```

`scripts/sweep.sh` is the hardware round-trip behind `./bin/obscene sweep`, not an emulator
build step.

`run-emulator.sh` keys the launch form on the executable name, and its `timedout` field
records a harness timeout. There is one timeout, in the harness, for every loader.

| loader | launch |
|---|---|
| craziiEmu | `CraziiEmu-cli.exe --log-level=Info <module.elf>` |
| SharpEMU | `SharpEmu.exe <eboot.bin>` - positional argument, not `-g`, conventional filename. `--trace-imports=N` and `--log-level=debug` trace import resolution. |
| PS5PCEM | `game-run [--app0 <content-directory>] <eboot.bin>` |
| orbistoun | `orbistoun-cli run <path> --limit 0` |
| prosper | `boot_trace <dir>`, a directory containing `eboot.bin` |

### Generation marker

`e_ident[EI_ABIVERSION]` names the hardware generation: 2 for the current one, 0 for the
previous. The loaders differ, so it is a build option: `make module GEN=5` by default,
`GEN=4` for shadPS4 (D062).

| | wants | reason |
|---|---|---|
| shadPS4 | `0` | a previous-generation emulator; refuses anything else |
| Kyty | `0` or `2` | `Elf64::IsNextGen()` compares this byte against 2 |
| KytyPS5 | `2` for executables | rejects `!is_shared && !is_next_gen` |
| craziiEmu | `2` for current | selects its target generation the same way |

A wrong marker under shadPS4 prints `e_ident[EI_ABIVERSION] expected 0x00 is (0x2)` and
produces a zero-record run.

### Kyty output

`--printf-direction File --printf-output-file` captures Kyty's console output, which is
otherwise invisible to a parent process.

### orbistoun

```
<OOPS>/orbistoun\target\release\orbistoun-cli.exe
```

A research tool: `run` executes a guest in a worker process, alongside `inspect`, `imports`,
`verify`, `questions` and `worklist`. It carries its own symbol database and NID machinery,
derived from the same public sources as this project. `run` is a subcommand, so its launch
form contributes two arguments. `--limit 0` removes orbistoun's own execution budget (twenty
seconds by default), leaving the harness timeout as the only clock.

### prosper

```bash
MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu -- bash <OOPS>/obscene/scripts/prosper-run.sh --rounds 6
```

prosper opens no window, so it runs in the background. `scripts/prosper-run.sh` keeps the
report between rounds because the report is the resume state; `--fresh` deletes it. Each
blocker costs two rounds. `hle_registry_dump` prints prosper's function registry
(`real`, `placeholder`, or absent), a ground truth for `007-responsive`.

prosper's module reader has a raw-ELF fallback behind two recognised SELF magics. Its own
comment records that a file matching neither cleanly is mapped with wrong segment offsets
rather than refused, so a run that looks like a load can be garbage.

## Loader behaviour

Facts about each loader's source that explain its results.

### craziiEmu and SharpEMU

- `TryLoadTableBytes` (`SelfLoader.cs`) computes `guestAddr = location + imageBase`. A
  `DT_SCE_*` offset is relative to `PT_SCE_DYNLIBDATA`, not the image base, so the tables are
  read from the wrong place. The read succeeds against mapped memory, so the fallbacks below
  it never run and the failure surfaces as a stall with zero import stubs, not a load error.
  Standard OpenOrbis homebrew fails the same way.
- `PT_SCE_DYNLIBDATA` is declared (`ProgramHeader.cs`, `SceDynLibData = 0x61000000`) and
  referenced nowhere else.
- `HasImportMetadata` requires `StrTabOffset != 0`, treating a table at offset zero as absent.
  The local craziiEmu clone carries a one-line fix; SharpEMU does not.

### fpPS4

`ResolveImport` binds every unresolved function import to `print_stub`, which logs
`nop nid:` and calls `Sleep(INFINITE)`. One missing function freezes the run silently.
`patches/fpps4-probe-friendly.patch` makes the handler return zero.

### Kyty and KytyPS5

- KytyPS5 patches every unresolved relocation to a stub. It reads obSCEne's library names
  correctly (`libSceVideoOut` as `VideoOut_v1`).
- The 2022 clone writes address 0 into an unresolved relocation slot. `kyty_load_elf(path, 1)`
  prints relocations. obSCEne survives the nulls because `obs_address_is_callable()` tests
  `>= 0x1000` before any indirect call.
- A weak `Func` in the jmprela table goes to a generated trampoline that prints a stack trace
  and exits. [BOOT.md](BOOT.md) has the mechanism.
- Kyty's log names every import it cannot satisfy; `obscene-tool unresolved` turns it into a
  list of missing functions.

`patches/kyty-probe-friendly.patch` changes Kyty's behaviour, so a result under it never
occupies Kyty's row in `COMPATIBILITY.md`. `reports/kyty.txt` holds the stock result and
`reports/kyty-patched.txt` the patched one. `patches/README.md` has the reasoning (D176).

### ChonkyStation4

`AppLoader.cpp` requires a list of firmware modules and panics on the first missing one
(`libSceLibcInternal.sprx`). There is no flag to skip them. Its compatibility entry is
"requires firmware".

### prosper constants

- Mutex types: `{1, 2, 3, 4}`, refusing `0`; 1=ERRORCHECK, 2=RECURSIVE, 3=NORMAL agree with
  Kyty. prosper maps 4 to `ADAPTIVE_NP` (FreeBSD libthr), Kyty to `NORMAL`.
  `015-sync/mutex-recursion` distinguishes them.
- An unrecognised mutex type returns `EINVAL` (`0x16`), as POSIX specifies.
- `GetLoginUserIdList` fills four slots, first occupied and the rest `-1`, matching
  PS5PCEM's `[4]i32` and shadPS4's `USER_ID_INVALID`. The initial user id differs between
  implementations (prosper and Kyty `1`, shadPS4 `1000`, PS5PCEM `0x10000000`) and stays
  `assumed`.

## Control build

An OpenOrbis PS4 Toolchain v0.5.4 (`toolchain-llvm-18`) build of its stock `hello_world`
sample is the control for loader-blame questions (D097). The toolchain is installed outside
this repository in the build VM. It is open source, inside the provenance boundary.

The control's `link.x` embeds `/libexec/ld-elf.so.1` as the first sixteen bytes of `.text`
and points `PT_INTERP` at it. It ships as a SELF via `create-fself`, links with `-pie`,
`link.x` and `crt1.o`, and carries `PT_TLS`, `PT_GNU_EH_FRAME` and `PT_SCE_RELRO`
(`0x61000010`). Compare with `readelf -lW`.

## ps4libdoc

Two files on the `doc` branch:

- `known_names.txt` - established symbol names.
- `unknown_nids.txt` - NIDs with no recovered name. A target list for a generator; no known
  name hashes to one.

Cracking with its names as candidates against this project's corpus reproduces every shared
pair. `known_names.txt` carries no library association, so it does not extend the census on
its own: a census entry needs the exporting library, and `mkmodule` refuses an import
without one.

## Source reading

An emulator's loader is a written account of the format; where two disagree, one is wrong.
Reading loader constants caught the swapped `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` pair,
which the derivation tool's `JMPREL + PLTRELSZ == RELA` identity cannot detect. The
union of what the emulators implement maps the platform surface better than any one, and
where they disagree about a function, obSCEne settles it by calling it.

## Provenance

These are public sources: open-source emulators, an open-source toolchain and a public
symbol database, inside the boundary of CLAUDE.md principle 6.

**Read for facts, never copy.** Several are GPL. What is taken from them is what a format is
and what a symbol is called.

## Refreshing

Shallow clones, no submodules.

```bash
git -C <emulators>/src/shadPS4 pull --depth 1
```

`git fetch --unshallow` recovers history when needed.
