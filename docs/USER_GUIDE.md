# Operator guide

How to build obSCEne, run it on a console, and read what it reports. The report format is
[OUTPUT.md](OUTPUT.md), the command protocol is [PROTOCOL.md](PROTOCOL.md), and which file
goes to which loader is [ARTIFACTS.md](ARTIFACTS.md).

## Build targets

| Target | Artifact | Execution context | Probing scope |
|---|---|---|---|
| `payload` | `build/obscene-probe-prospero.elf`, a plain ELF | Sent to the homebrew ELF loader on port 9021. Runs in the previous generation's compatibility sandbox (`ps4_mode`) | Kernel calls, memory mapping, errno encoding, POSIX sockets |
| `native` | `build/prospero/<TITLE_ID>/`, a title directory holding the fake-signed `eboot.bin`, `sce_sys/param.json` and `sce_sys/icon0.png` | Launched as a `BIG_APP` from `/data/homebrew/`, owning the display | GPU command buffers, video output and flips, controller input |
| `pkg` | `build/obscene-probe-orbis.pkg` | Installed as a previous-generation `ps4_game` title; its report file is sealed `0600` inside the title's sandbox | Save data mounting, filesystem sandbox boundaries, dynamic linking inside an installed title |

`TITLE_ID` defaults to `PPSA90000`; `TITLE_ID` or `CONTENT_ID` overrides it
(`scripts/build-native.sh`). Plain `make eboot` produces the standalone `eboot.bin` that goes
into the package, not the title directory.

## Building

Cross-compilation runs in WSL or a container; [BUILDING.md](BUILDING.md) has the toolchain.

```bash
make payload
make native
make pkg
```

## Running on a console

Register the console with `pros` first:

```powershell
pros.exe register <console-ip> --name <console-name>
pros.exe check
```

### Payload

```powershell
# Terminal 1: stream the system log.
pros.exe logs
# Terminal 2: send the ELF to the loader on port 9021.
pros.exe send build/obscene-probe-prospero.elf 9021
```

`./bin/obscene payload` builds, sends and captures in one step.

### Native title

```powershell
pros.exe restore build/prospero/PPSA90000 /data/homebrew/PPSA90000
pros.exe launch PPSA90000
```

`./bin/obscene native --deploy` builds the title and pushes it. While it runs, the display
shows the obSCEne HUD: the detected hardware generation (`GEN`), the graphics driver
(`GPU`, `gnm` or `agc`), and a running check counter with a pass/fail tally.

### Package

```powershell
pros.exe restore build/obscene-probe-orbis.pkg /data/pkg/obscene-probe-orbis.pkg
```

`./bin/obscene deploy` builds the package, installs it, launches it and captures the report.

### Console ports

| Port | Service | Use |
|---|---|---|
| 9021 | `elfldr` | receives a payload ELF |
| 2121 | `ftpsrv` | uploads title directories and packages, pulls report files |
| 3232 | `klogsrv` | streams the system log |

## Reading the telemetry

Every check writes an unbuffered announcement before it makes its call. Every line is
pipe-separated and begins `OBS|`:

```text
OBS|try|130-layout/kquery|libkernel|sceKernelVirtualQueryInfo
OBS|res|130-layout/kquery|pass|0x0||assumed
OBS|measure|130-layout/kquery|sceKernelVirtualQueryInfo|size|0x38|bytes
OBS|bytes|130-layout/kquery|sceKernelVirtualQueryInfo|extent|0x0|0010000000000000...
```

| Record | Meaning |
|---|---|
| `OBS\|try\|<check-id>\|<library>\|<symbol>` | the check is about to call `<symbol>`. A `try` with no matching `res` names the call that did not return |
| `OBS\|res\|<check-id>\|<status>\|<value>\|<detail>\|<provenance>` | the verdict (`pass`, `partial`, `fail`, `skip`, `crash` or `pending`), the returned value, and how far the expectation behind it is trusted |
| `OBS\|measure\|<check-id>\|<symbol>\|<quantity>\|<value>\|<unit>` | a measured property such as a structure size, an alignment or a timer frequency, with no verdict |
| `OBS\|bytes\|<check-id>\|<symbol>\|<label>\|<offset>\|<hex>` | one line of a byte-exact dump of a structure the platform filled in |

## Capturing reports

`./bin/obscene report` (or `obscene-tool report`) captures obSCEne's `OBS|` records from the
console system log into a plain text file:

```powershell
obscene-tool.exe report --seconds 120 --into reports/hardware/console-klog.txt
```

`reports/hardware/console-klog.txt` is the default destination. The file is what
`obscene-tool verify`, `obscene-tool diff` and `obscene-tool pretty` read. [TOOLING.md](TOOLING.md)
lists the flags.

`./bin/obscene pull-log [destination]` pulls the newest report file from the console over
FTP, into `reports/obscene-report.txt` by default. The sink tries these paths in order and
records which one answered: `/data/obscene-report.txt`, `/download0/obscene-report.txt`,
`/mnt/usb0/obscene-report.txt`, then `obscene-report.txt` beside the process.

## Troubleshooting

| Symptom | Meaning | Recovery |
|---|---|---|
| a `try` line and then nothing | the call did not return: the process faulted or hung | run `./bin/obscene recover` before relaunching, then reboot the console. The `try` line names the call |
| `pros send` reports connection refused on 9021 | the ELF loader is not running on the console | start the homebrew loader on the console |
