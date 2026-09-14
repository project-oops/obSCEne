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
5. [Generating JSON Reports with `obscene-tool`](#5-generating-json-reports-with-obscene-tool)
6. [Operator Troubleshooting](#6-operator-troubleshooting)

---

## 1. Understanding the 3 Build Targets

Different platform capabilities require different execution privileges. obSCEne compiles into three distinct targets:

| Target | File Type | Privileges & Execution Context | Ideal Probing Scope |
| :--- | :--- | :--- | :--- |
| **`payload`** | Bare ELF | Runs directly via `elfldr` on `:9021`. Unsandboxed, kernel address space visible, direct POSIX sockets. | Kernel syscalls, memory mapping (`mmap`), errno encoding, raw CPU registers. |
| **`eboot`** | Signed Container | Runs in `/data/homebrew/` as a retail `BIG_APP`. HDMI screen ownership, universal graphics queues (`libSceAgc`). | GPU command buffers (PM4), DualSense input, video output, display flips. |
| **`pkg`** | Encrypted PFS | Installed on retail SSD (`/user/app/`). Strict retail sandbox (`0600`), isolated filesystem. | Title save mounting, filesystem sandbox boundaries, background download queues. |

---

## 2. Building the Probes

Cross-compilation requires a freestanding Clang 18+ toolchain (via WSL or Docker):

```bash
# In WSL or Linux container
cd obscene

# Build all 3 targets:
make payload   # Produces build/obscene-probe-prospero.elf
make native    # Produces build/prospero/PROO00001/
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
pros.exe restore build/prospero/PROO00001 /data/homebrew/PROO00001

# Launch the title
pros.exe launch PROO00001
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

obSCEne follows a strict **"Announce Before Attempting"** principle. Every check emits an unbuffered announcement *before* calling the operating system:

```text
[OBS] try sceKernelVirtualQueryInfo
[OBS] res sceKernelVirtualQueryInfo 0x0
[OBS] OBS|measure|kquery_size=0x38
[OBS] OBS|bytes|kquery_data=0010000000000000...
```

### Key Output Fields:
1. **`try <symbol>`**: The probe is about to invoke `<symbol>`. If this is the last line printed before a crash, that exact function caused the kernel hang.
2. **`res <symbol> <hex>`**: The function returned cleanly with return code `<hex>` (`0x0` = success).
3. **`OBS|measure|<key>=<value>`**: A measured silicon property (e.g. structure size, alignment, or timer frequency).
4. **`OBS|bytes|<key>=<hex>`**: A byte-exact hex dump of a kernel structure populated by hardware.

---

## 5. Generating JSON Reports with `obscene-tool`

To convert raw console logs into machine-readable JSON reports for Orbistoun HLE grounding:

```powershell
# In Windows PowerShell:
obscene-tool.exe report --input reports/hardware/latest.log --output reports/latest.json
```

This JSON report directly feeds the automated blame engine in `orbistoun-turn`.

---

## 6. Operator Troubleshooting

### Problem: Probe outputs `try [check]` and then completely freezes
- **Explanation**: You encountered an unhandled kernel exception or fatal page fault on hardware.
- **Recovery**: Reboot the console, note the offending check name, and flag it as an architectural wall in `worklog.md`.

### Problem: `pros send` reports connection refused on `:9021`
- **Explanation**: The `elfldr` daemon is not running on the console.
- **Recovery**: Re-run the jailbreak environment from the console browser.

