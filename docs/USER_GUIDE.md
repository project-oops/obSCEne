# obSCEne Operator Guide

Welcome to the **obSCEne** operator guide.

This guide explains how **hardware testers, homebrew operators, and researchers** can build, deploy, run, and interpret obSCEne conformance probes on physical console hardware.

If you are an AI coding agent, compiler architect, or low-level systems engineer seeking the formal wire protocol, sysctl offset tables, or decision records, consult the **[Technical Reference](README.md)**, **[PROTOCOL.md](PROTOCOL.md)**, and **[DECISIONS.md](DECISIONS.md)** instead.

---

## Table of Contents

1. [Understanding the 3 Build Targets](#1-understanding-the-3-build-targets)
2. [Building the Probes](#2-building-the-probes)
3. [Executing Probes on Real Hardware](#3-executing-probes-on-real-hardware)
   - [Method A: Fast Direct Socket (`payload` via port 9021)](#method-a-fast-direct-socket-payload-via-port-9021)
   - [Method B: Full Screen Application (`eboot` via BIG_APP)](#method-b-full-screen-application-eboot-via-big_app)
   - [Method C: Installed Retail Sandbox (`pkg`)](#method-c-installed-retail-sandbox-pkg)
4. [Reading & Interpreting the Telemetry](#4-reading--interpreting-the-telemetry)
5. [Capturing Reports with `obscene-tool`](#5-capturing-reports-with-obscene-tool)
6. [Operator Troubleshooting](#6-operator-troubleshooting)

---

## 1. Understanding the 3 Build Targets

Different platform capabilities require different execution privileges. obSCEne compiles into three distinct targets:

| Target | File Type | Privileges & Execution Context | Ideal Probing Scope |
| :--- | :--- | :--- | :--- |
| **`payload`** | Bare ELF | Runs directly via `elfldr` on `:9021`. Unsandboxed, kernel address space visible, direct POSIX sockets. | Kernel syscalls, memory mapping (`mmap`), errno encoding, raw CPU registers. |
| **`eboot`** | Fake-signed fSELF (`eboot.bin`) | Runs in `/data/homebrew/` as a retail `BIG_APP`. HDMI screen ownership, universal graphics queues (`libSceAgc`). | GPU command buffers (PM4), DualSense input, video output, display flips. |
| **`pkg`** | Encrypted PFS | Installed on retail SSD (`/user/app/`). Strict retail sandbox (`0600`), isolated filesystem. | Title save mounting, filesystem sandbox boundaries, background download queues. |

---

## 2. Building the Probes

Cross-compilation requires a freestanding Clang 18+ toolchain (via WSL or Docker):

```bash
# In WSL or Linux container
cd obscene

# Build all 3 targets:
make payload   # Produces build/obscene-probe-prospero.elf
make native    # Produces build/prospero/<TITLE_ID>/ (default TITLE_ID is PPSA90000)
make pkg       # Produces build/obscene-probe-orbis.pkg
```

---

## 3. Executing Probes on Real Hardware

Ensure your target console is registered in `pros`:
```powershell
pros.exe register 192.168.1.211 --name ps5-testbed
pros.exe check
```

### Method A: Fast Direct Socket (`payload` via port 9021)

This is the fastest method for routine OS and kernel probing:

```powershell
# 1. Start the kernel log streamer in terminal 1
pros.exe logs

# 2. In terminal 2, stream the bare ELF directly to elfldr (or use ./bin/obscene payload)
pros.exe send build/obscene-probe-prospero.elf 9021
```

The console will immediately execute the probe in memory and stream output back over `klog`.

---

### Method B: Full Screen Application (`eboot` via BIG_APP)

For graphics and AGC shader probes that need screen ownership:

```powershell
# Stage the directory into /data/homebrew (or use ./bin/obscene native --deploy)
pros.exe restore build/prospero/PPSA90000 /data/homebrew/PPSA90000

# Launch the title
pros.exe launch PPSA90000
```

The TV display will show the obSCEne HUD rendering real-time test progress.

---

### Method C: Installed Retail Sandbox (`pkg`)

For retail sandbox and filesystem permission checks:

```powershell
# Install the package via Prosperous
pros.exe restore build/obscene-probe-orbis.pkg /data/pkg/obscene-probe-orbis.pkg
# Launch via Prosperous GUI or console UI
```

---

## 4. Reading & Interpreting the Telemetry

obSCEne follows a strict **"Announce Before Attempting"** principle. Every check emits an unbuffered announcement *before* calling the operating system. Every line is pipe-separated and begins `OBS|` (see `docs/OUTPUT.md` for the full contract):

```text
OBS|try|130-layout/kquery|libkernel|sceKernelVirtualQueryInfo
OBS|res|130-layout/kquery|pass|0x0||assumed
OBS|measure|130-layout/kquery|sceKernelVirtualQueryInfo|size|0x38|bytes
OBS|bytes|130-layout/kquery|sceKernelVirtualQueryInfo|extent|0x0|0010000000000000...
```

### Key Output Fields:
1. **`OBS|try|<check-id>|<library>|<symbol>`**: The probe is about to invoke `<symbol>`. If this is the last line printed before a crash, that exact function caused the kernel hang.
2. **`OBS|res|<check-id>|<status>|<value>|<detail>|<provenance>`**: The check's verdict - `pass`, `partial`, `fail`, `skip`, `crash` or `pending` - with the returned value and how much the expectation behind it should be trusted.
3. **`OBS|measure|<check-id>|<symbol>|<quantity>|<value>|<unit>`**: A measured silicon property (e.g. structure size, alignment, or timer frequency), recorded with no verdict attached.
4. **`OBS|bytes|<check-id>|<symbol>|<label>|<offset>|<hex>`**: One line of a byte-exact hex dump of a kernel structure populated by hardware.

---

## 5. Capturing Reports with `obscene-tool`

`obscene-tool report` captures obscene's own `OBS|`-prefixed records off the console system log
into a plain text file - not JSON, and not a conversion of an existing log:

```powershell
# In Windows PowerShell:
obscene-tool.exe report --seconds 120 --into reports/hardware/console-klog.txt
```

The file it writes is exactly what `obscene-tool verify`, `diff` and `pretty` read. See
`docs/TOOLING.md` for the full set of `report` flags and `docs/OUTPUT.md` for the record format.

---

## 6. Operator Troubleshooting

### Problem: Probe outputs `OBS|try|...` and then completely freezes
- **Explanation**: You encountered an unhandled kernel exception or fatal page fault on hardware.
- **Recovery**: Reboot the console, note the offending check name, and flag it as an architectural wall in `worklog.md`.

### Problem: `pros send` reports connection refused on `:9021`
- **Explanation**: The `elfldr` daemon is not running on the console.
- **Recovery**: Re-run the jailbreak environment from the console browser.

