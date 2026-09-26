# Build artifacts

obSCEne builds several files. They are not interchangeable: each reaches a different loader,
and they are told apart by two bytes. The Makefile comment beside each rule states the same
mapping.

## Shapes

Each row reaches a different loader, and only the system loader is the hardware's own, so a
result from one row says nothing about the others.

| file | shape | `e_type` | `EI_ABIVERSION` | who loads it |
|---|---|---|---|---|
| `obscene-probe-prospero.elf` | plain `ET_DYN` | `0x0003` | `0` | a homebrew ELF loader, which maps the segments itself |
| `obscene-injector.elf` | plain `ET_DYN` | `0x0003` | `0` | a homebrew ELF loader; attaches the probe to the foreground native process |
| `obscene.module.elf` | vendor ELF | `0xFE10` | `2` | emulators, through their "not a SELF" path |
| `obscene-probe-prospero.zip` / `eboot.bin` | fSELF | - | - | the system loader, from an app directory |
| `obscene-probe-orbis.pkg` | package | - | - | the installer, then the system loader (previous-generation format) |
| `build/prospero/<TITLE_ID>/` | title directory | - | - | the system loader, from `/user/app/<TITLE_ID>`: a prospero `eboot.bin` beside `sce_sys/{param.json,icon0.png}` |

`eboot`, `pkg` and `native` go through selfish. The `eboot` container is orbis by default and
prospero with `TARGET=prospero` (or `EBOOT_GEN=5`). The `native` title carries a prospero eboot
and is deployed to a scan root that an auto-mounter registers into `/user/app`.
(D180, D278)

Each transport has a minimal twin for proving the transport before trusting the full build:

| file | imports |
|---|---|
| `obscene-min.elf` | `sceKernelOpen`, `sceKernelWrite`, `sceKernelClose` |
| `obscene-min.module.elf` | `sceKernelWrite` |

`obscene-host` is a native binary with the platform stubbed, for checking the harness without
hardware or an emulator.

## Building

```sh
make payload      HARDWARE=1     # obscene-probe-prospero.elf → the hardware
make injector     HARDWARE=1     # obscene-injector.elf   → the hardware (native process attach)
make payload-min  HARDWARE=1     # obscene-min.elf        → the hardware, transport test first
make module                      # obscene.module.elf     → emulators
make module-min                  # obscene-min.module.elf → emulators
make host                        # obscene-host           → this machine
```

`BUILD` points at a directory local to the build VM. A Windows mount carries no execute bit, so
a binary built into the tree compiles and then refuses to run. (D012)

### Package

`eboot` and `pkg` go through selfish, so selfish is compiled first; a package built against a
stale selfish carries the previous format code. `./bin/obscene deploy` builds both in order,
then installs, launches and captures the report:

```sh
./bin/obscene deploy               # --build-only stops after the package is built
```

The build half is `cargo build -p selfish-pkg -p selfish-cli` followed by:

```sh
make pkg GEN=5 HARDWARE=1 BUILD=$HOME/obs-pkg SELFISH=<OOPS>/selfish
```

Building and installing run on different machines:

- **Build in WSL.** `pkg` needs `module`, which needs `$(BUILD)/symbols.txt`, which is produced
  by running `obscene-host`, a POSIX binary.
- **Install from Windows.** `pkg_install` takes an http url, and the hardware fetches it with
  `Range` requests. Served from WSL, the url carries the WSL NAT address, which the hardware
  cannot route to; it never fetches, and the failure looks like a rejected package. From
  Windows the server binds the LAN address.

```sh
cp <wsl-build-dir>/obscene-probe-orbis.pkg .\obscene-probe-orbis.pkg
obscene-tool.exe hw install .\obscene-probe-orbis.pkg --seconds 180
```

The Windows `obscene-tool.exe` is built with `cargo build --bin obscene-tool` under
`CARGO_TARGET_DIR=<OOPS>\obscene\tool\target-win`.

## `HARDWARE=1`

`910-bulk` calls every resolvable symbol with six zero arguments. The list it walks includes
`sceSystemServiceRequestPowerOff`, `sceLncUtilSystemShutdown`,
`sceShellCoreUtilRequestShutdown` and others of that kind, and most censused entries are
unnamed NIDs that no blocklist can screen.

`HARDWARE=1` with `BULK=1` is therefore a build error:

```text
$ make module HARDWARE=1 BULK=1
*** HARDWARE=1 and BULK=1 are mutually exclusive ...
```

Two further protections hold independently:

- `BULK` is empty by default.
- **Census symbols cannot be called, by type.** Every censused name is declared
  `extern OBS_WEAK const char`, so only its address is taken and a call does not compile. A
  build that imports `sceSystemServiceRequestPowerOff` cannot invoke it.

## Identifying a file

Two bytes at offset 16:

```sh
od -An -tx1 -j16 -N2 <file>
```

| bytes | meaning |
|---|---|
| `03 00` | `ET_DYN` - the plain build, for the hardware's homebrew ELF loader |
| `10 fe` | `0xFE10`, `ET_SCE_DYNEXEC` - the vendor build, for emulators |
| `18 fe` | `0xFE18`, `ET_SCE_DYNAMIC` - a shared library, which is a defect for the main module |

`obscene-tool hw send` performs this check and refuses a vendor module. The homebrew ELF
loader (`elfldr`) checks only the four magic bytes, which both shapes share: handed a vendor
module, it maps it, jumps to an entry that expects its imports resolved, and stops. It writes no
log of its own unless `klogsrv` is running, and restoring it needs a console restart, because
the payload manager launches every payload through `elfldr`, including `elfldr` itself. (D184)

## Report location

The sink tries candidate paths in order and records which one answered in its `OBS|sink|...`
record. The list is `obs_sink_paths` in `src/probe/sink.c`; `/data` is writable on hardware.

Descriptors 1 and 2 are also written, and where they land depends on how the program was
launched. Sent to `elfldr`, they are the socket the ELF arrived on, so the report streams back
to the sender. Launched any other way, as an installed package or from the home screen, they
are `/dev/deci_stdout` with no reader. The socket is a convenience and never the report
mechanism. (D183)
