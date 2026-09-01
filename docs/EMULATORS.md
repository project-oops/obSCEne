# The emulator toolkit

Reference copies of every PS4 and PS5 emulator we could find, plus the format and symbol
references that go with them. They live in `<emulators>`, deliberately outside this
repository - they are large third-party binaries and source, and neither belongs in the
history of a probe.

```
<emulators>\
    shadps4\        binary, 0.18.0 - the one the scripts default to
    kyty\           binary
    src\            source, shallow clones
<OOPS>/orbistoun\      the sibling project, built from its own tree
```

## What each loader actually did

See **[COMPATIBILITY.md](COMPATIBILITY.md)** - the results table, generated from the
reports, with screenshots. Kept there rather than here so this file stays about what each
loader *is* and that one stays about what each loader *did*.

## Running them

```bash
sh scripts/sweep.sh --build /tmp/obs-shad   # shadPS4, GEN=4, its own exclusion list
sh scripts/run-kyty.sh                     # Kyty, GEN=5, captures the unresolved log
```

craziiEmu is run directly, from where dotnet builds it:

```bash
CraziiEmu-cli.exe --log-level=Info <module.elf>
```

### The generation marker is not optional and not the same for all of them

`e_ident[EI_ABIVERSION]` says which hardware generation a module is for: 2 for the current
one, 0 for the previous. **The loaders disagree and all of them are right.**

| | wants | why |
|---|---|---|
| shadPS4 | `0` | a previous-generation emulator, and it refuses anything else outright |
| Kyty | `0` or `2` | `Elf64::IsNextGen()` is this byte against 2 |
| craziiEmu | `2` for current | picks its target generation the same way |

So it is a build option - `make module GEN=5` by default, `GEN=4` for shadPS4 - and not a
constant. `sweep.sh` passes 4 because it drives shadPS4; `run-kyty.sh` leaves the
default. Getting it wrong is not subtle: shadPS4 prints
`e_ident[EI_ABIVERSION] expected 0x00 is (0x2)` and produces a zero-record run (D062).

### Two things about Kyty that cost time

**Its output is capturable.** obSCEne learned to draw its report to the screen because
Kyty's console output is invisible to a parent process. That is true, and
`--printf-direction File --printf-output-file` removes the problem entirely. The drawn
report is still worth having for hardware; it was not needed for this.

**The binary is not built from the clone, and it is not even the same project.** This was
recorded as "the binary is newer than the repository's default branch", which was a guess
and was too kind. The window title states the build outright:

```
[Official build KytyPS5-2026-08-18-7e42513 | Release]
```

The clone is at `4733b7e`, **2022-10-03**, and `7e42513` is not an object in it. So the
binary is *KytyPS5* - a fork, four years further on - and the clone is original Kyty. That
is the whole reason the source describes a Lua configuration interface while the binary
takes command-line arguments: they are different programs.

Nothing about Kyty's behaviour may be explained from this clone until the fork is the thing
being read (D094). Ask the binary how to drive it; do not ask this source why it behaves as
it does.

## fpPS4: builds, loads the module completely, and now reports

Worth writing down because the build is fiddly and the result is encouraging.

**This heading used to end "produces no guest output", and that stopped being true.** fpPS4
now runs the suite to the end and emits 521 results over `sceKernelWrite`. What changed was
not fpPS4: it needs 44 checks excluded to get there, and the exclusion walk that finds them is
`scripts/sweep.sh`. The loader was reporting all along, behind a check that hangs at `strlen`
in the third section.

### Building it

Its README asks for Free Pascal **3.3.1 trunk** via fpcupdeluxe. **3.2.2 from winget works**,
which saves a source build of the compiler:

```bash
winget install Lazarus.Lazarus            # the IDE; ships lazbuild
winget install FreePascal.FreePascalCompiler
lazbuild fpPS4.lpi                        # not --build-mode=Release; there is only default
```

At runtime it needs FFmpeg **4.x shared** libraries - `avutil-56`, where current releases
ship `avutil-59`, so no package manager has them. The project's own CI names the source, and
using what it uses is the right answer:

```bash
curl -kL https://github.com/red-prig/fpps4-bin/raw/main/ffmpeg.zip -o ffmpeg.zip && unzip ffmpeg.zip
```

### What it does with our module

It parses **every** `DT_SCE_*` tag correctly - `NEEDED_MODULE`, `IMPORT_LIB` and
`IMPORT_LIB_ATTR` for all sixteen libraries, with attributes - maps the segments, spawns a
thread and reaches the entry point.

That is the furthest any loader but shadPS4 has gone, and since fpPS4 is a wholly
independent implementation in another language, its agreement about the vendor segment is
the strongest corroboration the module format work has.

**Then nothing.** No guest output at all, identically at 100 and 180 seconds, so it stops
rather than crawls. Its `sceKernelWrite` calls a real `_sys_write`, so descriptor 1 should
reach the stdout being captured. Whether the module faults at entry or the write goes
somewhere else is **not established**, and saying so is better than guessing at it.

## orbistoun: the sibling project, and the only loader here that is ours

Every other entry in this file is somebody else's work, surveyed from outside. orbistoun is
the project this one exchanges findings with - a PS5 HLE emulator in Rust, developed
alongside obSCEne and in the same repository group. It appears in `COMPATIBILITY.md` for the
first time as of 2026-08-25.

**That relationship is a reason for care, not for confidence.** Two projects written in
concert are the *least* independent pair in the toolkit, and `obscene-tool consensus` already
prints the general warning in its own output: agreement between implementations that read
each other is evidence, not witnesses. Where obSCEne and orbistoun agree about the platform,
that agreement is worth less than shadPS4 and PS5PCEM agreeing, because the two of us have
been talking. It is listed with the others and weighed below them.

### What it is

```
<OOPS>/orbistoun\target\release\orbistoun-cli.exe
```

A research tool rather than a game-runner: `run` executes a guest in a worker process, and
around it sit `inspect`, `imports`, `verify`, `questions` and `worklist` - commands for
finding out what a module needs and what is not implemented yet. It carries its own symbol
database (737 names at the time of writing) and its own NID machinery, independently derived
from the same public sources this project uses.

### How it is driven

```bash
orbistoun-cli run <path> --limit 0
```

A subcommand rather than a flag, which is why `run-emulator.sh` keys the launch form on the
executable name - the case for it contributes two arguments where shadPS4's contributes one
and PS5PCEM's contributes none.

`--limit 0` removes orbistoun's own execution budget, which defaults to twenty seconds. That
is deliberate: **two clocks is one too many.** With its budget in play a suite this size is
stopped part-way and reported as a guest that ended, which reads like a crash rather than a
deadline - exactly the confusion `run-emulator.sh`'s `timedout` field exists to keep out of
the record. One timeout, in the harness, for every loader alike.

### Where it stops, and why that is a useful result

It parses the container, maps two segments, and halts before building the thunk table: the
bare-ELF path locates data for none of the six program headers, so the dynamic table cannot
be read. `COMPATIBILITY.md` has the detail.

obSCEne emits a bare ELF because it cannot sign a SELF, and every other loader in this
toolkit has a "not a SELF, treat it as an ELF" path. Whether orbistoun's is meant to reach
guest code is a question for its authors, and it has been put to them rather than decided
here.

## Why craziiEmu resolves nothing, precisely

Two separate faults, found by running obSCEne against it. Its tag constants are correct -
all eight `DT_SCE_*` values match this project's exactly - so the failure is not what the
earlier note guessed ("defines eleven table tags and none of the identity tags").

### 1. A zero offset read as an absent table - fixed locally

`HasImportMetadata` required `StrTabOffset != 0`. These offsets are relative to the start
of `PT_SCE_DYNLIBDATA`, so **the first table in that segment legitimately begins at zero**
- and obSCEne's string table does. Every table parsed correctly and the module was then
reported as having no import metadata at all.

Sizes say whether a table is present; offsets do not. One line, patched in the local clone.

### 2. Table offsets resolved against the wrong base - diagnosed, not fixed

`TryLoadTableBytes` computes `guestAddr = location + imageBase`. But a `DT_SCE_*` offset is
relative to `PT_SCE_DYNLIBDATA`, not to the image base - so it reads the string table from
the ELF header, gets no valid symbol names, and ends with **1691 relocation descriptors and
0 recovered identifiers.**

**craziiEmu never locates that segment at all**: there is no reference to program header
type `0x61000000` anywhere in its loader. Fixing this means teaching it to find the segment
and resolve the vendor tables against it - perhaps forty lines across several call sites,
and a change to its architecture rather than a correction to it.

That is the whole reason it resolves nothing, and it is now a specific bug report rather
than a shrug.

## SharpEMU is craziiEmu, and both fail obSCEne identically

Added to the roster, built from source (`a2241d0`), and it produced the most useful
negative result in the project so far.

It parses the module **completely** - every segment mapped, every `DT_SCE_*` tag read with
the right value, `PT_DYNAMIC` located, entry point reached - and then resolves
**zero imports**:

```
[LOADER][INFO] EntryPoint: 0x00000008000279B0, ImportStubs: 0
[LOADER][INFO] Setup 0/0 import stubs (direct bridge, lle_redirects=0)
[LOADER][ERROR] Execution stalled with no import progress for 20s (imports=0).
```

The cause is stated outright in its own error text:

```
Unsupported relocation type 1332700516 (unknown) rejected (first at off=0x6956656373006174)
```

`0x6956656373006174` is not an offset. It is ASCII - `ta\0sceVi` - so the relocation table
is being read out of the **string table**. `SelfLoader.cs:2190` computes
`guestAddr = location + imageBase`, and a `DT_SCE_*` offset is relative to
`PT_SCE_DYNLIBDATA`, not to the image base.

The failure is silent by construction: that address *is* mapped, so `TryRead` succeeds and
the three correct fallbacks below it never run. **A read that succeeds with the wrong data
beats a read that fails**, which is why this presents as a stall twenty seconds later
rather than as a load error.

`PT_SCE_DYNLIBDATA` is declared - `ProgramHeader.cs:57`, `SceDynLibData = 0x61000000` - and
referenced nowhere else in the tree. The constant exists; nothing consumes it.

### They are one loader, not two

Both diagnoses above are already recorded for craziiEmu. That is not two emulators
independently confirming a finding:

| | |
|---|---|
| `SelfLoader.cs`, craziiEmu | 2859 lines, 16 uses of `TryLoadTableBytes` |
| `SelfLoader.cs`, SharpEMU | 2906 lines, 16 uses of `TryLoadTableBytes` |
| Differing lines, after renaming the namespace and ignoring line endings | **93 of 2906** |

97% identical, and craziiEmu's copyright header names SharpEmu on its *first* line:

```
// Copyright (C) 2026 SharpEmu Emulator Project
// Copyright (C) 2026 CraziiEmu Project
```

**So the two count as one data point, and the compatibility table must not present them as
two.** This is the same error as counting a skip as an opinion and counting an absence as a
disagreement - both of which this project has already made once each. Agreement between two
copies of the same code is not evidence of anything.

Of the 93 differing lines, none touches the relocation base. SharpEMU additionally still has
the `StrTabOffset != 0` fault - a table legitimately beginning at offset zero read as an
absent table - which craziiEmu only lacks because this project patched its clone.

### Running it

Positional argument, not `-g`, and it wants the conventional filename:

```bash
sh scripts/run-emulator.sh --emulator <emulators>/src/SharpEMU/artifacts/bin/Release/net10.0/win-x64/SharpEmu.exe
```

`--trace-imports=N` and `--log-level=debug` are worth knowing: the import trace is exactly
the resolution step that is failing.

## fpPS4, narrowed: three failures, and none of them is the ELF shape

D097 pointed at obSCEne for fpPS4 - the control runs there and this does not. The obvious
reading was that the module is malformed. It is not, and the minimal modules in `src/probe/min.c`
took the question apart in four runs.

**The guest runs.** `module-min-noimport` and `module-min` both reach `Entry:` and then hang,
which is exactly what those modules are built to do when the guest is executing. So loading,
mapping and control transfer all work.

**fpPS4 does not resolve our imports.** `module-min` reaches its one import and fpPS4 logs:

```
[main:37120] nop nid:libkernel:E304B37BDD8184B2:sceKernelWrite
```

That is `NewNopStub` in `rtl/stub_manager.pas` - the trampoline installed for an import it
could **not** find, which logs the call and returns. `module-min-debugout` gets the same
treatment for `sceKernelDebugOutText`.

Both are implemented and registered in its own source: `ps4_libkernel.pas:1897` maps
`$E304B37BDD8184B2` to `ps4_sceKernelWrite`, which reaches `_sys_write` and the device layer
that prints `[TTY]:` - the very channel the control's output arrives on. **The NID we ask for
and the NID it registers are the same value**, so the mismatch is in how the two sides agree
on the *library*, not on the symbol.

**The full module fails earlier still, and differently.** It produces **no** `nop nid:` lines
at all, where the minimal module produces one - so it dies before reaching a single import,
with `EAccessViolation` immediately after `Entry:`. Whatever stops the full build is not what
stops the minimal one, and the difference between them is scale: thousands of relocations and
a real linkage table against almost none.

That is consistent with the note already in `link/module.ld` - splitting `.got` from
`.got.plt` corrected a wrong `DT_SCE_PLTGOT` and *did not* make the linkage-table build work
on one loader.

So the fpPS4 work is: **relocation and linkage-table handling at scale**, then library-name
agreement on imports. Not `PT_INTERP`, not `PT_TLS`, not the SELF container.

### What PT_INTERP did change

Added because it was first on the D097 list, and it works - `readelf` reports
`[Requesting program interpreter: /libexec/ld-elf.so.1]`, and fpPS4 now prints that path
where before it printed nothing.

It did not fix the run, and it is worth keeping anyway: fpPS4 previously ended silently after
`Entry:`, and now raises `EAccessViolation` with a stack trace. **The same failure, reported
instead of swallowed.** shadPS4 is unaffected - 829 records, ran to the end, zero crashes.

## PS5PCEM: the first current-generation loader that runs obSCEne

Built from source (`zig build`, one command, no configuration) and it produced **702
records and ran to the end** - the third loader ever to finish the suite, and the first
targeting the current generation.

```
game-run [--app0 <content-directory>] <eboot.bin>
```

Its `module-info` tool is worth knowing about on its own: an independent parser for the
format this project synthesises, which nothing else in the toolkit provides. It reads
obSCEne exactly right - both loadable segments with correct sizes and flags, the entry, the
module name and version, and all sixteen `DT_NEEDED` entries added in D100.

### It is the honest one, and the numbers say so

| | shadPS4 | PS5PCEM |
|---|---|---|
| Pass / partial / fail / skip | 87/6/40/13 | 61/14/**7**/64 |
| Checks blocked behind a missing one | **5** | **2** |
| Census present | 383 / 383 | 236 / 383 |

Seven failures against forty, and **two blocked checks against five** - the one number that
cannot be moved by resolving more symbols (D067). It scores worse on presence and better on
everything that measures behaviour, which is precisely the pattern `COMPATIBILITY.md`
predicted for "a loader that resolves only what it implements".

The generation profile is coherent, which no other loader's is:

| library | present | absent |
|---|---|---|
| `libSceAgc` | **79** | 1 |
| `libSceAgcDriver` | 7 | 0 |
| `libSceGnmDriver` (previous generation) | 0 | **17** |
| `libScePosix` | 0 | 51 |

It implements the current generation's graphics interface and correctly lacks the previous
one's. shadPS4 is the mirror image - it reports all 87 current-generation graphics symbols
present through a generic stub for an interface it does not implement at all. This is the
first loader where `005-generation` has something unambiguous to read.

### On how it was written

238 commits in nineteen days from one author, peaking at 42 in a day, with uniformly
machine-shaped conventional-commit messages. It is heavily AI-assisted and there is no
point pretending otherwise.

That is not the same judgement as `ps5-emulator-controller` below, and the difference is
worth stating because both would look alike in a search listing. There is no web layer, no
funding file and no timestamp bot; it builds from source in one command; the commit subjects
name real problems (`dispatch irreducible CFG and VCC/EXEC predicates`); and **the code is
correct** - its parse of our module matches `readelf` byte for byte. How it was written is
a fact about the project. Whether it works is a separate question, and this repository
answers that kind of question by running the thing.

## ChonkyStation4 needs firmware, so it cannot be part of a firmwareless toolkit

Built from source and it runs - it just cannot run anything here.

Getting it to build took two things worth writing down: it uses
`__attribute__((sysv_abi))`, which MSVC cannot parse, so it needs **clang-cl**; and its
CMake needs `VULKAN_SDK` in the environment rather than merely installed. Two lines also
needed patching for a newer Vulkan-Hpp, where `float[4]` no longer binds to
`const std::array<float, 4>&`.

Then:

```
FATAL: Required sysmodule "libSceLibcInternal.sprx" does not exist
```

`AppLoader.cpp` lists a dozen firmware modules and calls `Helpers::panic` on the first one
missing. It is a **low-level** design: it links the guest against real firmware `.sprx`
files rather than reimplementing their interfaces. There is no flag to skip them, and there
should not be - the whole point of that approach is that the real modules provide the
behaviour.

So it is not a failure and not a gap in obSCEne. It is a loader that needs an input this
toolkit deliberately does not have, and the honest entry in a compatibility table is
"requires firmware" rather than a zero.

## Candidates surveyed 2026-08-22, and what a search result is worth

Six repositories, cloned and read rather than judged by their description. Three are worth
having, one is interesting and cannot run our module, and two are not emulators at all.

| | language | files | last commit | vendor ELF loader |
|---|---|---|---|---|
| `Force67/prosperity` | C++ | 3349 | 2026-08-15 | **yes** - `kern/lv2/sys_dynlib.cpp`, `kern/module.cpp` |
| `iStark/PS5PCEM` | **Zig** | 128 | 2026-08-21 | **yes** - `src/loader/dynamic.zig`, `elf.zig` |
| `liuk7071/ChonkyStation4` | C++ | 193 | 2026-08-20 | **yes** - `Loaders/ELF/ELFLoader.cpp` |
| `JaverHEyGG/ps5-emu` | C++ | 28 | 2026-08-22 | no |
| `daeken/EmpireOfSteel` | - | 6 | 2022-07-23 | no |
| `Weavelefoster/ps5-emulator-controller` | HTML | 9 | 2026-08-22 | no |

### The three worth adding

**prosperity** is the serious one: an `lv2` kernel layer, guest address-space management,
IPMI, per-title profiles, an Android target and a Nix flake. Its `sys_dynlib.cpp` is a
dynamic-linker implementation rather than a table reader, which is a different shape from
anything else in this toolkit.

**PS5PCEM** is the only Zig implementation and the only one whose stated scope reaches the
**current generation's graphics path** - AGC command streams and an RDNA2-to-SPIR-V shader
translator. obSCEne censuses 87 AGC symbols and has never had a loader that claims to
implement any of them.

**ChonkyStation4** is early and small, and its author has finished emulators for three
previous hardware. Worth tracking for that alone.

### The one that is real and cannot help

**ps5-emu** has no vendor tags anywhere, and its layout says why: `vmm/whpx.cpp`,
`devices/com16550.cpp`, `guest/hello.cpp`. It boots its own payload inside the **Windows
Hypervisor Platform** with a 16550 UART for output - hardware virtualisation rather than
recompilation, which is a genuinely different bet. It is at the "hello world on bare metal"
stage and has no hardware loader, so obSCEne cannot run in it. Worth reading, not worth
adding to the compatibility table.

### The two that are not emulators

**EmpireOfSteel** is described as a prototype C# emulator. Its default branch is six files:
a Dockerfile, a FreeBSD kernel configuration, and a 39 MB FreeBSD kernel built from it -
`interpreter /red/herring`, the hardware kernel's own interpreter string. No C# at all. Not
kept.

**ps5-emulator-controller** is an HTML page, a `FUNDING.txt`, and this:

```yaml
schedule:
  - cron: '*/30 * * * *'
      - name: Write update timestamp
```

A bot rewriting a timestamp file every thirty minutes so the repository reads as "updated
11 minutes ago" in a search listing. There is no emulator. Not kept.

**Which is the point of cloning them.** Four of these six sort near the top of a search for
"ps5 emulator", and two of those four have no emulator in them. Recency and a description
are not evidence, and the check costs one `git clone` and one `grep` for the vendor tags -
the same instinct as the control build (D097), applied to the toolkit instead of to a claim.

## The control: an OpenOrbis build, through every loader

obSCEne ran under exactly one loader, and its module format had been developed by iterating
against that loader until it was accepted. That is overfitting, and no amount of reasoning
about it settles who is at fault - so the missing piece was a **control**: a module built by
a standard toolchain, put through the same five loaders.

Toolchain: OpenOrbis PS4 Toolchain v0.5.4 (`toolchain-llvm-18`), installed **outside this
repository**, at `/home/ubuntu/openorbis` in the build VM. It is an open-source toolchain,
so it sits inside the provenance boundary that principle 6 draws (`ACKNOWLEDGEMENTS.md`
already cites it for signatures). Its stock `hello_world` sample, built unmodified.

### Result

| Loader | OpenOrbis `hello_world` | obSCEne | Whose fault |
|---|---|---|---|
| shadPS4 | runs | runs, 835 records | neither |
| fpPS4 | **runs** - `[TTY]:main: Hello world!` | reaches `Entry:`, process ends | **obSCEne** |
| SharpEMU | **fails**, and harder than obSCEne does | resolves 0 imports | **the emulator** |
| craziiEmu | same codebase, same failure | same failure | **the emulator** |
| KytyPS5 | **rejected outright** | loads and runs, no output | **obSCEne is ahead** |

Four results, and they do not point the same way. Taking them one at a time is the whole
value of having run it.

**fpPS4 - ours.** It prints the control's output over its own TTY channel, from both the
SELF and the bare ELF. So its loader, its import resolution and its output path all work,
and obSCEne dies at the entry point on the same binary. Nothing about fpPS4 needs to change
for obSCEne to run; something about obSCEne does.

**SharpEMU and craziiEmu - theirs, after all.** This was recorded as the emulator's fault,
then re-attributed to obSCEne on the strength of their README ("can currently load the
`eboot.bin` of real games"), then settled by the experiment. The control fails *worse*:

- as a fake-signed SELF, `NotSupportedException: SELF segment mapping for program header 4
  could not be resolved` - program header 4 is `PT_DYNAMIC`, and it never loads at all;
- as a bare ELF, the **identical** failure obSCEne produces - rejected relocations whose
  offsets are x86 machine code (`0x8B48D87D8B48...`, `mov rdi,[rbp-0x28]`).

An earlier note here explained that failure by obSCEne being small enough that the wrong
read stays inside a mapped region. **That was wrong** - a 1.8 MB standard ELF hits it too.
Size has nothing to do with it; `location + imageBase` is simply the wrong base for a
`DT_SCE_*` offset, and it breaks standard homebrew exactly as it breaks this project.

**KytyPS5 - obSCEne is ahead of the control.** It refuses the OpenOrbis module before
loading:

```
Not implemented (!is_shared && !is_next_gen) in .../loader/runtimeLinker.cpp:1956
```

A PS4 executable is neither shared nor next-generation, so KytyPS5 has dropped
previous-generation executables entirely. obSCEne built `GEN=5` satisfies it and runs. The
remaining gap there is output, not loading, which is the one case where BACKLOG 11 is the
right work.

### What obSCEne is missing, concretely

From `readelf -lW` on both. The control carries nine program headers; obSCEne carries six:

| | OpenOrbis | obSCEne |
|---|---|---|
| `PT_INTERP` -> `/libexec/ld-elf.so.1` | yes | **no** |
| `PT_TLS` | yes | **no** |
| `PT_GNU_EH_FRAME` | yes | **no** |
| `PT_SCE_RELRO` (`0x61000010`) | yes | **no** |
| `PT_SCE_PROCPARAM` (`0x61000001`) | yes | yes |
| `PT_SCE_DYNLIBDATA` (`0x61000000`, vaddr 0) | yes | yes |
| `e_type` | `0xfe10` | `0xfe18` |
| link mode | `-pie`, `link.x`, `crt1.o` | `-shared`, no script, own entry |
| imports | linked against `-lc -lkernel -lc++` | weak undefined symbols |
| shipped as | SELF container via `create-fself` | bare ELF |

The interpreter string is not incidental: `link.x` embeds `/libexec/ld-elf.so.1` as the
first sixteen bytes of `.text`, before any code, and points `PT_INTERP` at it.

That table is the work list, and it is evidence rather than a guess - which is what the
control was for.

## Built from source, not downloaded

**A binary that does not correspond to the source being read is worse than no source.**
The shadPS4 clone sat at `be21649` while the binary that produced every result reported
`v0.18.0` / `e3ce810` - different commits - and several findings were written up by
reading one to explain the other (D094).

| | built from source | notes |
|---|---|---|
| craziiEmu | yes | `dotnet build CraziiEmu.slnx -c Release` |
| fpPS4 | yes | `lazbuild fpPS4.lpi`, plus its own FFmpeg archive |
| SharpEMU | yes | `dotnet build SharpEmu.slnx -c Release` |
| prosper | **not yet** | cloned 2026-08-26 at `bb3e189`; CMake + Ninja, and it has a Windows path (a Windows launcher script in its own tree, MinGW-w64 UCRT64) as well as Linux |
| shadPS4 | **no** | needs cmake, ninja and Qt; outstanding |
| Kyty | **yes** | MinGW/ninja, not the clang-cl its README suggests - see below. Its downloaded binary is *newer* than its default branch, so the two are separate programs |

Where a binary is still in use, any claim about *why* it behaves as it does is a claim
about a different commit, and is marked as such.

**Kyty, specifically.** The README suggests clang-cl; its CI uses MinGW/GCC, which is what
works. Two build settings had to change to compile it at all on this machine, and they are
stored as `patches/kyty-buildable.patch` rather than left in a working tree nobody else has:
`KYTY_WARNINGS_ARE_ERRORS` made overridable (its bundled 2022 SDL2 declares a symbol the 2026
Windows SDK now also declares, so `-Wshadow` fires inside a dependency and `/WX` fails the
build), and a `KYTY_NO_LAUNCHER` switch so the Qt launcher can be skipped without
`Build_Tools` also compiling the emulator out.

**And a second patch that is not a build fix.** `patches/kyty-probe-friendly.patch` changes
what the emulator *does*, so anything measured under it is a result about a patched Kyty and
never occupies Kyty's row in `COMPATIBILITY.md`. `reports/kyty.txt` holds the stock result and
`reports/kyty-patched.txt` the other. `patches/README.md` has the reasoning; D176 has what it
bought and where it stops being worth doing.

## Why the source and not just the binaries

Because reading it has already paid for itself. `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ`
were swapped in our tag table for weeks. The derivation tool could never have caught it:
it checks that `JMPREL + PLTRELSZ == RELA`, and addition is commutative, so a swapped pair
satisfies the identity perfectly. It was found by reading a loader that had the constants
the other way round (D038).

That is the general shape of it. An emulator's loader is a written-down account of what
the format actually is, and where two of them disagree, one is wrong and it is worth
knowing which.

The second use is coverage. Each of these implements a different subset of the platform,
and the union of what they implement is a much better map of the real surface than any one
of them. Where they *disagree* about a function's behaviour, that is a question obSCEne
can settle by calling it.

## What is here

### PS5

| | |
|---|---|
| `craziiEmu` | Builds from source with dotnet. Two bugs found and diagnosed; one fixed locally. See below. Its constants independently confirmed the JMPREL/PLTRELSZ swap. |
| `SharpEMU` | Experimental, Windows/Linux/macOS. Not yet read. |

### PS4

| | |
|---|---|
| `shadPS4` | The most complete. Runs our whole suite; its `CommonStub` logs by name, which is where `data/nid-corpus.txt` came from. |
| `Kyty` | Loads and relocates the module, and **names every import it cannot resolve** - 252 of them, which shadPS4 never reports because it stubs everything. `obscene-tool unresolved` turns that log into a list of missing functions by name. |
| `prosper` | **The only independent implementation of orbistoun's own architecture** - a user-space compatibility layer rather than an emulator: guest code runs natively and the OS beneath it is reimplemented, the same bet orbistoun makes, in C++20 instead of Rust. Current-generation, actively developed, and it boots retail titles. Untested against obSCEne; see below. |
| `GPCS4` | Older, graphics-focused. |
| `fpPS4` | Pascal, and a completely independent implementation. **Builds and runs** - see below. Reads every `DT_SCE_*` tag in our module correctly, which is the strongest independent confirmation the format work has. |
| `rpcsx` | Rust and C++, actively developed. |
| `obliteration` | Not an emulator in the same sense: a PS4 *kernel* rewritten in Rust, run under a custom hypervisor. The closest thing to a written specification of kernel behaviour. |
| `orbital` | Virtualization-based. |

### Format and symbol references

| | |
|---|---|
| `OpenOrbis-PS4-Toolchain` | 189 headers under `include/orbis`. An open-source toolchain, which CLAUDE.md principle 6 names as an acceptable source for signatures. The `lib` directory holds only a README - the stub archives ship in releases, not in the repository. |
| `ps5-payload-dev/sdk`, `elfldr` | PS5 homebrew SDK and ELF loader. The nearest thing to a reference implementation of what we are producing. |
| `ps4_module_loader` | An IDA loader for the vendor module format. A third independent account of the dynamic tables. |
| `ps4libdoc` | The symbol database. See below. |

## `ps4libdoc`, and what it is worth

Two files, on the `doc` branch:

- `known_names.txt` - **42,010** established symbol names.
- `unknown_nids.txt` - **1,130,742** NIDs nobody has recovered a name for.

### It validated our hash against an independent corpus

Running the cracker with those 42,010 names as candidates and our own harvested corpus as
the known set:

```
# candidates 42010
# generator  reproduced 388 of 389 known pairs
# recovered  0 of 1130742
```

**388 of 389 is the right answer, not a shortfall.** The single name our corpus has that
theirs does not is `sceAgcCreateShader` - AGC is the PS5 graphics API, and this is a PS4
database. Every name the two sets share, they agree on. Our NID implementation now has an
independent 42,010-name confirmation, which is a great deal more than the one published
test vector it started with.

**`recovered 0 of 1130742` is also expected.** Those NIDs are precisely the residue that
these names could not crack; if a known name hashed to one of them it would not be in the
unknown list. The file is a target list for a *generator*, not for this word list.

### fpPS4 carries a superset of it, and that is one source, not two

fpPS4 embeds `ps4libdoc.pas`: 78,372 identifier-to-name pairs. Hashing every name
reproduces every identifier - 78,372 of 78,372.

**It is derived from this database, not independent of it.** 42,009 of ps4libdoc's 42,010
names are in it, plus 36,363 more, and the unit is named after its source. Presenting the
two as separate corroboration was a mistake and is corrected in D064.

The agreement is still worth having. ps4libdoc's identifiers are extracted from real
firmware rather than computed, so reproducing them means this project's hash matches values
observed on hardware, across a large sample - which rules out every byte-order, alphabet
and digest-truncation error at once. It just is not three witnesses.

### What it does not carry

**Library association.** A census entry needs to know which library exports a symbol -
an import with no library resolves to nothing at all, which is why `mkmodule` refuses to
build without one. `known_names.txt` is names only.

So the 8,572 vendor `sce*` names in it are not a census expansion on their own. The
missing half would have to come from somewhere else: the OpenOrbis headers group symbols
by header, which is close to but not the same as grouping them by library.

## Provenance

These are public sources: open-source emulators, an open-source toolchain, and a public
symbol database. That is the boundary CLAUDE.md principle 6 draws - no vendor headers, no
SDK, no decrypted material.

**Reading them for facts is fine; copying them is not.** Several are GPL, and obSCEne is a
probe that calls the platform's own interface - it has no need of an emulator's code and
must not acquire one. What is taken from them is what a format *is* and what a symbol is
*called*, neither of which is anyone's expression.

## Refreshing

Shallow clones, no submodules - they are read, not built.

```bash
git -C <emulators>/src/shadPS4 pull --depth 1
```

`git fetch --unshallow` in any of them if history is ever needed.

## Kyty resolves nothing, and the version you run decides how it fails

Two different programs get called "Kyty" here, and they behave differently enough that the
distinction has to lead. `<emulators>\kyty\` holds the release binary
`KytyPS5-2026-08-18-7e42513`; the clone at `<emulators>\src\Kyty\` is original Kyty at
`4733b7e` from 2022 - a different project, four years apart. D094 says a loader's behaviour
may only be explained from the source that built the binary running it, so the two get
separate paragraphs and no claim crosses between them.

### The release binary: everything patched to a stub

Its relocation log, 34,349 lines of it:

```
Relocate: unresolved non-PLT function patched to stub [38426]
  w3BY+tAEiQY[VideoOut_v1][VideoOut_v1.1][Func], Func, Weak
```

**34,349 relocations, 34,349 patched to stubs, zero resolved.** Tested at both sizes to rule
out scale: with the curated census only, 389 imports, **389 stubbed**. The module being large
is not the problem, and it is not a naming mismatch on our side either - it reads our library
names correctly, `libSceVideoOut` as `VideoOut_v1`, `libSceLibcInternal` as `LibcInternal_v1`,
consistently. **Why it resolves nothing cannot be answered from here**, because that revision
is not in this repository.

### The 2022 clone, built from source: everything resolved to null

Measured with `kyty_load_elf(path, 1)`, which turns on relocation printing:

| | |
|---|---|
| distinct imports requested | 35,518 |
| resolving to **address 0** | **35,431** |
| resolving to real host code | 305 symbols, over 494 relocation entries |

So this one does not stub - it writes a null into the relocation slot and moves on. The 305
that land somewhere real are the subsystems it actually implements, and the log names each:
`FileSystem::KernelWrite`, `AudioOut::AudioOutInit`, `Controller::PadGetControllerInformation`,
and so on.

**obSCEne survives that only because of a rule it already had.** `obs_address_is_callable()`
tests `>= 0x1000` before any indirect call, so 35,431 null imports are declined rather than
jumped to. A guest without that check dies on its first import.

### What actually kills it, and why patching is a separate question

Not the null imports. obSCEne dies at `puts`, which takes the *other* path - a weak `Func` in
the jmprela table, sent to a generated trampoline that prints a stack trace and calls `EXIT`.
One unimplemented function ends the process. `docs/BOOT.md` has the mechanism; D176 has what
happens when it is patched out, and why that stops being worth doing after two patches.

### What Kyty is uniquely good for

The inverse of a report: **absence, by name.** A stub-everything loader cannot tell you what
is missing, because nothing is. Kyty's log names every import it cannot satisfy - 252 against
the current module, including twelve `sceAgc*` entry points - and `obscene-tool unresolved`
turns that log into a list. That is worth more than the report it does not produce.

## Building shadPS4 from source, and the four things in the way

D094 says a loader's behaviour may only be explained from source that built the binary
running it. shadPS4 was the outstanding case - every finding this project quotes from its
source describes commit `be21649` while the binary reported `v0.18.0` / `e3ce810`.

It builds without Qt (`-DENABLE_QT_GUI=OFF`), so the obstacles were all environmental:

**CMake 4 rejects its dependencies.** `zlib-ng` and others declare compatibility below 3.5,
which CMake 4 removed support for. `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` restores it.

**`git submodule update --init` silently did nothing.** On this shallow clone it exited 0,
printed nothing, and registered no URLs - `git config --get submodule.externals/zlib-ng.url`
came back empty afterwards. Naming a single path worked. Looping over the 45 paths from
`.gitmodules` populated them.

**That missed the nested ones.** `sirit/externals/SPIRV-Headers` stayed empty, because the
per-path form takes no `--recursive`. A second recursive pass, once the top level existed,
filled them.

**And the compiler was wrong, which caused the rest.** Two failures - `imgui_demo.cpp`
converting `ImColor` to `ImVec4`, then `libusb.h` failing to parse `LIBUSB_CALL` in a
function-pointer typedef - both looked like project defects and were neither. **shadPS4's
own CI builds with `clang-cl`**, and the workflow file says so in one line:

```
-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
```

Both errors are MSVC declining constructs clang accepts. The right first question when a
project will not build is *what does it build with*, and that is a file in the repository
rather than a guess. The imgui patch was reverted once the compiler was corrected: a local
change made to work around the wrong toolchain is divergence for nothing, and divergence is
exactly what building from source exists to avoid.

None of these is a shadPS4 defect. They are what a two-year-old CMake convention, a shallow
clone and the wrong compiler do to any project of this size, and they are written down so
the next person does not rediscover them.

## prosper: reviewed, and it is not the fourth vote it looked like

Cloned 2026-08-26 at `bb3e189` into `<emulators>\src\prosper`. Reviewed by reading, not
yet built or run.

**Its own description is a user-space compatibility layer** - Proton-shaped, no CPU emulation,
guest code native on x86-64 with the OS, ABI and graphics stack reimplemented beneath. That is
orbistoun's architecture exactly, made independently in C++20 with ~3,200 commits behind it,
and it boots retail titles.

### The correction, which is the point of having reviewed it

The case for adding it was that a fourth mature implementation would promote several
one-source constants to corroborated **without needing hardware**. That case was wrong, and it
is worth writing down rather than quietly dropping.

**prosper reads Kyty and shadPS4, extensively and by name.** Its own comments cite
`Kyty Errno.h`, describe a structure as *"0x50 bytes, Kyty layout"*, and say a setter
*"mirrors Kyty's"*. So where it agrees with Kyty it is very often **not independent evidence** -
counting it as a second vote would be double-counting the first.

What it is instead is a **graded synthesis**, and that turns out to be more useful than a
fourth raw opinion. It annotates its own claims:

| `CONFIDENCE:` | count |
|---|---|
| HIGH | 40 |
| MED | 13 |
| MED-HIGH | 4 |
| LOW | 3 |

So the right way to read it is per-claim, not per-project:

- **Annotated, and citing FreeBSD or a live capture** - strong. FreeBSD is a legitimate citable
  source under our own provenance rule, and a "live capture" here means *what a retail title
  actually did* under prosper, which is guest-observed evidence. Not hardware, but real.
- **Unannotated bare constants** - no better than anyone else's guess, including ours.

### What the review actually produced

**The mutex type constants are now corroborated in shape and *contested* in one value.** Both
implementations accept exactly `{1, 2, 3, 4}` and refuse `0`, which is not the POSIX numbering
and was previously one project's word. They agree 1=ERRORCHECK, 2=RECURSIVE, 3=NORMAL. They
disagree about 4, and prosper says so deliberately:

> *"Weight Kyty DOWN here: no PS4 title it runs exercises adaptive self-lock; FreeBSD libthr is
> the platform contract. CONFIDENCE: HIGH (FreeBSD source + the live wedge → unwedge flip)."*

It maps 4 to `ADAPTIVE_NP` (deadlock-detecting on self-lock), where Kyty maps it to `NORMAL`.
**`015-sync/mutex-recursion` already distinguishes these**: it reports second-acquisition
behaviour per type, and a type that returns `EDEADLK` on self-lock is not the same observation
as one that hangs or succeeds. So there is now a named, specific disagreement that one hardware
run settles, rather than a blank. That is worth more than agreement would have been. (D177)

**Returning `EINVAL` for an unrecognised type is confirmed.** prosper returns `0x16`, which is
what POSIX specifies and what `patches/kyty-probe-friendly.patch` changes Kyty to do - arrived
at independently, so that patch is a correction rather than an accommodation.

**The user model gains a shape.** `GetLoginUserIdList` fills four slots, first occupied and the
rest `-1`:

```c
p[0] = 1; for (int i = 1; i < 4; i++) p[i] = -1;
```

Four slots matches PS5PCEM's `[4]i32`, and `-1` matches shadPS4's `USER_ID_INVALID`. Three
implementations now agree on the *shape* - capacity four, `-1` for empty - while still
disagreeing about the initial user's value. That distinction is exactly the one
`docs/HANDOVER-ORBISTOUN.md` asks for: the shape is corroborated and buildable, the value stays
`assumed` until hardware.

**The initial user id is 1**, which agrees with Kyty against shadPS4's `1000` and PS5PCEM's
`0x10000000` - but the line carries no annotation, so it is a bare choice and not evidence. Two
of four now say `1`; nobody has measured it.

### What it is still worth building for

Everything above came from reading. The reason to build it is the thing reading cannot
produce: **what it does with our module**. It takes an unpacked directory, the same shape
already staged for orbistoun, and its module reader has a raw-ELF fallback behind two
recognised SELF magics - so an unsigned bare ELF is not refused out of hand.

Note the hazard its own comment records, because it is our failure mode aimed back at us:

```cpp
// Missing this map is fatal-but-silent: the raw-ELF fallback below still finds the inner ELF,
// but every segment file offset is then wrong, so the guest maps garbage bytes.
```

A file that is neither a recognised SELF nor a clean ELF is **mapped with wrong offsets rather
than refused**. If obSCEne lands in that gap the result looks like a load and is garbage, which
is precisely the plausible-wrong-answer class. Worth watching for on the first run rather than
trusting.

at least as likely to hold the same assumption, and no retail title would ever reveal it.

## prosper: the sixth loader, headless, and the only one that publishes its own answers

`boot_trace <dir>` takes a directory containing `eboot.bin` and runs it with **no window at
all**. Everything else in this toolkit opens one on the desktop, and that is a real cost rather
than an aesthetic one: a run long enough to be inconvenient gets minimised, a minimised window
has no client area to capture, and the screenshot is the report for any loader whose text
channel does not work. prosper has no window and no channel problem, so it can be run as often
as wanted, in the background, while the machine is used for something else.

It is a Linux build. There is no Windows binary, so runs happen in WSL:

```bash
MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu -- bash <OOPS>/obscene/scripts/prosper-run.sh --rounds 6
```

`MSYS_NO_PATHCONV=1` because Git Bash otherwise rewrites `/mnt/...` into a Windows path and the
script is not found. `CLAUDE.md` claims that variable is no longer needed; that is true of the
case it was written about and false in general.

### It runs obSCEne, and the resume mechanism converges on it

```text
round 1  118 records  last=015-sync/semaphore
round 2  165          last=018-relational/semaphore-counts
round 3  165          last=018-relational/semaphore-counts
round 4  180          last=018-relational/semaphore-state-is-per-object
round 6  181          last=018-relational/handle-fits-its-out-parameter
```

Each blocker costs two rounds, which is D191's two-consecutive-failures rule working rather
than a stall. `scripts/prosper-run.sh` preserves the report between rounds because the report
*is* the resume state; `--fresh` deletes it and is rarely what you want.

### The thing only this loader can give us

`hle_registry_dump` prints prosper's own function registry - 1,201 `real`, 6 `placeholder`,
anything absent unimplemented and logged as such when called. **It states its own ground truth.**

`007-responsive` infers that same fact from behaviour alone. Cross-checked over every function
the section tests: **54 of 54 agreed**, 45 `responds` against `real` and 9 `silent` against
absent, with no disagreement in either direction. That is the first independent evidence the
heuristic measures what it claims to. (D195)

### Where it stops

A SIGSEGV inside prosper's own semaphore implementation - `rip` in host code with obSCEne
frames only as callers, and all five semaphore functions `real` in the registry, so this is a
real implementation faulting rather than a missing one. obSCEne's contribution was the dangling
`try` with no `res` that named the call before it was made, on a loader it had never met.

### What it does not establish

It is a fourth reader agreeing with us about module layout and a sixth loader running our code.
It is not the hardware.

## fpPS4, settled: it sleeps forever on an unimplemented import

The long-standing question about fpPS4 was whether the stalling was ours or theirs. It is
theirs, and it is a deliberate debugging aid rather than a bug:

```pascal
procedure print_stub(nid:QWORD;lib:PLIBRARY); MS_ABI_Default;
begin
 Writeln(StdErr,SysLogPrefix,'nop nid:',lib^.strName,...);
 //DebugBreak;
 Sleep(INFINITE);
end;
```

`ResolveImport` binds every unresolved function import to it. The commented-out `DebugBreak`
and `readln` show what it is for - freeze the process so a developer can attach and see which
function is missing. Unattended, it means one missing function costs the entire run *silently*:
the process stays alive and stops producing output, which looks exactly like a hang in the
guest. That is what "fpPS4 seems stuck on 9/27" always was.

Directly above it, unused, is `_nop_stub` - `xor %rax,%rax; ret`. The compiler mentions it on
every build: *"Local proc `_nop_stub` is not used"*.

**Nothing on obSCEne's side could have avoided it.** A call that never returns cannot be
detected by its caller. Announce-before-attempting is the mitigation, and it worked - the
dangling `try` named `strspn` precisely.

`patches/fpps4-probe-friendly.patch` makes the handler return zero. With it:

| | stock | patched |
|---|---|---|
| records | 36 | **36,631** |
| outcome | hung in `007-responsive/libc` | **complete, 27/27, 515/515** |
| wall clock | ran until killed | **2s** |
| unimplemented imports survived | 0 | **181** |

181 is the number that answers whether fpPS4 was worth continuing with. At two runs per blocker
under D191, the stock build needed roughly **362 runs** to converge; patched it needs one, and
it is now the fastest loader in the sweep. (D196)
