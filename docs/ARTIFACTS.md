# Which file goes where

obSCEne builds several files. They are not interchangeable, they are told apart by two bytes,
and sending the wrong one to the hardware's loader took that loader down and cost a reboot.

**This page exists because the information was already in the repository and was read
afterwards.** It sat in a comment beside the Makefile rule that builds the thing, which is
exactly where somebody looks once they already know there is a distinction to look for.

---

## The shapes

**How the program got there is part of the platform.** Each row reaches a different loader, and
only one of them is the hardware's own - so they answer different questions, and a result from
one says nothing about the others.

| file | shape | `e_type` | `EI_ABIVERSION` | who loads it |
|---|---|---|---|---|
| `obscene-payload.elf` | plain `ET_DYN` | `0x0003` | `0` | a homebrew ELF loader, which maps the segments itself |
| `obscene-injector.elf` | plain `ET_DYN` | `0x0003` | `0` | a homebrew ELF loader, injects probe into native foreground process |
| `obscene-module.elf` | vendor ELF | `0xFE10` | `2` | emulators, through their "not a SELF" path |
| `obscene-eboot.zip` / `eboot.bin` | fSELF | - | - | **the system loader**, from an app directory |
| `obscene.pkg` | package | - | - | the installer, then the system loader |

The first three are built today. `eboot` and `pkg` are scoped and gated. (D180, D278)

Each has a one-import twin for proving the transport before trusting the payload:

| file | size | imports |
|---|---|---|
| `obscene-payload-min.elf` | ~3.8 KB | `sceKernelOpen`, `sceKernelWrite`, `sceKernelClose` |
| `obscene-min.module.elf` | ~180 KB | `sceKernelWrite` |

And `obscene-host`, a native binary with the platform stubbed, for checking the harness itself
without the hardware or an emulator.

---

## Building them

```sh
make payload      HARDWARE=1     # obscene-payload.elf    → the hardware
make injector     HARDWARE=1     # obscene-injector.elf   → the hardware (native process injector)
make payload-min  HARDWARE=1     # obscene-min.elf        → the hardware, transport test first
make module                      # obscene.module.elf     → emulators
make module-min                  # obscene-min.module.elf → emulators
make host                        # obscene-host           → this machine
```

`BUILD` must point somewhere local to the build VM - a Windows mount carries no execute bit, so
a binary built into the tree compiles fine and then refuses to run. (D012)

### The package, and the two machines it takes

`eboot` and `pkg` go through `selfish`, so a build has to compile selfish first - editing the
format library and rebuilding only obSCEne produces a package from the *previous* selfish, and
that difference does not surface until the hardware refuses the result. `./bin/obscene deploy` does
both in the right order (and installs, launches and captures the report - the whole round-trip):

```sh
./bin/obscene deploy               # or --build-only to stop after the package is built
```

which is `cargo build -p selfish-pkg -p selfish-cli` followed by:

```sh
make pkg GEN=5 HARDWARE=1 BUILD=$HOME/obs-pkg SELFISH=<OOPS>/selfish
```

**Building and installing happen on different machines, and neither can do the other's half.**

*Build in WSL*, because `pkg` needs `module`, which needs `$(BUILD)/symbols.txt`, which is
produced by **running** `obscene-host` - a POSIX binary. Windows has clang and can cross-compile
the freestanding targets, but it cannot run that host build.

*Install from Windows*, because `pkg_install` takes an **http url** and the hardware fetches it
with `Range` requests. Served from WSL the url carries its NAT address (`172.24.x.x`), which the
hardware cannot route to - it never fetches, and the failure looks like a rejected package rather
than an unreachable server. From Windows the server binds the LAN address and the hardware comes
for it:

```sh
cp \\wsl$\Ubuntu\home\<user>\obs-pkg\obscene.pkg .\obscene.pkg
obscene-tool.exe hw install .\obscene.pkg --seconds 180
```

The Windows `obscene-tool.exe` is built once with
`cargo build --bin obscene-tool` under `CARGO_TARGET_DIR=…\tool\target-win`.

**A package this toolchain builds is still refused by the hardware.** Every field it writes now
matches a real package, and the remaining wall is not a field: the header region is integrity
protected, and flipping a single byte at `0x600` - an area no field reads - turns a package that
installs into one that does not. See selfish's worklog for the measurement.

---

## `HARDWARE=1`, and why it is not optional

`910-bulk` calls every resolvable symbol with six zero arguments. On an emulator a bad call
kills a process. The list it walks contains `sceSystemServiceRequestPowerOff`,
`sceLncUtilSystemShutdown`, `sceShellCoreUtilRequestShutdown` and ten more of that kind.

**A blocklist cannot fix it.** Of the 67,053 entries, **50,344 are unnamed NIDs** - three
quarters of the sweep cannot be screened, because nobody knows what those functions are.

So `HARDWARE=1` with `BULK=1` is a compile error, not a warning:

```text
$ make module HARDWARE=1 BULK=1
Makefile:92: *** HARDWARE=1 and BULK=1 are mutually exclusive ...
```

Two further protections hold independently, and only one of them is a rule somebody has to
remember:

- **`BULK` is empty by default.** A convention, and a convention is what fails at one in the
  morning when somebody reuses the command line already in their shell.
- **Census symbols cannot be called, by type.** Every censused name is declared
  `extern OBS_WEAK const char`, so only its *address* is ever taken. Calling one does not
  compile. This is why a build that imports `sceSystemServiceRequestPowerOff` still cannot
  invoke it, and it is structural rather than remembered. (D179)

---

## Telling them apart, after the fact

Two bytes at offset 16:

```sh
od -An -tx1 -j16 -N2 <file>
```

| bytes | meaning |
|---|---|
| `03 00` | `ET_DYN` - the plain build, for the hardware |
| `10 fe` | `0xFE10`, `ET_SCE_DYNEXEC` - the vendor build, for emulators |
| `18 fe` | `0xFE18`, `ET_SCE_DYNAMIC` - a **shared library**, which is a bug here (D175) |

`obscene-tool hw send` performs this check and refuses a vendor module before it reaches a
hardware, because the loader cannot: `elfldr`'s own sanity test reads only the four magic bytes,
which both shapes share. It accepts the module, maps it, jumps to an entry expecting 35,518
resolved imports, and dies - silently, because it writes no log of its own unless `klogsrv` is
running. (D184)

---

## Where a report comes out

Not decided by the artifact, and deliberately so. The sink tries candidate paths in order and
**records which one answered**, which turns a guess into a measurement:

```
/data/obscene-report.txt      ← confirmed writable on hardware
/download0/obscene-report.txt
/mnt/usb0/obscene-report.txt
obscene-report.txt            ← relative to whatever the process's directory is
```

The `OBS|sink|...` record names the winner.

Descriptors 1 and 2 are also written, and where they land depends on **how the program was
launched, not what it is**. Sent to `elfldr`, they are the socket the ELF arrived on, so the
report streams back live to the sender. Launched any other way - installed as a package,
started from the home screen - they are `/dev/deci_stdout` and nobody is listening.

**So the socket is a convenience and never the mechanism.** A channel that only exists when the
program was started one particular way is a coincidence. (D183)

---

## The mistake this page is for

`elfldr` was handed `obscene-module.elf` - the vendor build, the shape emulators want because
they emulate the system loader. It accepted the connection, closed without a word, and stopped
listening. Recovery needed a reboot and a re-jailbreak, because `pldmgr` launches every payload
*through* `elfldr`, including `elfldr` itself.

The Makefile said so, beside the rule:

> The plain-ELF build. **A homebrew ELF loader running on the hardware takes this** - no
> emulator will touch it… it uses neither the vendor linker script nor `mkmodule`, because both
> exist solely to produce the shape this one must not have.

Correct, complete, and in the wrong place to be read first.
