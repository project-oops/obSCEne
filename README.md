<p align="center">
  <img src="assets/logo.png" alt="obSCEne" width="200">
</p>

# ob**SCE**ne

**The Hardware Conformance Probe and Silicon Oracle for Prospero.**

obSCEne is a clean-room conformance probe suite for 8th and 9th generation console software (Orbis and Prospero), written in freestanding C (`-ffreestanding -nostdlib`). It calls platform system functions and submits GPU command buffers deliberately, one by one, recording exactly what the real operating system and silicon actually do.

Site: **[project-oops.github.io/obSCEne](https://project-oops.github.io/obSCEne/)**

| 📖 **[Operator Guide & Hardware Probing](docs/USER_GUIDE.md)** | ⚙️ **[Technical Reference & Protocol Specs](docs/README.md)** |
| :--- | :--- |
| *Running probes via 9021/BIG_APP, interpreting logs, and JSON reports.* | *Report grammar, wire protocol, GPU surface, and decision records.* |

---

## Role in THE LOOP

Within the [OOPS ecosystem](../docs/THE_LOOP.md), obSCEne serves as **The Silicon Oracle**:

```
[Orbistoun Emulator Hits Unknown Function / Struct]
                          │
                          ▼
            Formal Question Formulated
                          │
                          ▼
┌───────────────────────────────────────────────────┐
│ obSCEne Probe Dispatched via Prosperous           │
│ - Executed directly on PS5 hardware               │
│ - Probes memory layout, registers, error codes    │
└─────────────────────────┬─────────────────────────┘
                          │
                          ▼
┌───────────────────────────────────────────────────┐
│ Telemetry Logged to klog (OBS|measure, OBS|bytes) │
│ - Exact struct size and byte offsets              │
│ - Verified POSIX vs SCE error codes               │
└─────────────────────────┬─────────────────────────┘
                          │
                          ▼
[Orbistoun Lands Typed Implementation: known_by = "measured"]
```

When an emulator encounters an undocumented system call, the traditional approach is to guess or copy from leaked sources. In OOPS, we **ask the hardware directly**:
1. An automated test case or probe section is added to obSCEne.
2. [Prosperous](../prosperous/) delivers the probe to our physical PS5 (`192.168.1.211`).
3. The probe executes on the metal, logging exact return values and hex dumps of memory buffers to `klog`.
4. The verified telemetry is fed back into [Orbistoun](../orbistoun/) with `known_by: measured`, permanently closing the gap with 100% clean-room provenance.

---

## Developer Quickstart

### 1. Build the Probe
obSCEne cross-compiles for the FreeBSD-based console ABI using `clang` and `lld`:

```bash
./bin/obscene build    # compiles payload, module, and host test harness
./bin/obscene check    # runs verification suite (what CI runs)
./bin/obscene pkg      # creates installable package (ORBO00001 / PROO00001)
```

### 2. Why Three Target Builds? (`payload`, `eboot`, `pkg`)
On real console firmware, **system privileges, sandbox boundaries, and dynamic library resolution change based on how a process is launched**. Testing all three execution contexts (`./scripts/sweep.sh`) is essential to accurately map the platform:

| Build Shape | Delivery & Context | Privileges & Sandboxing | What It Measures |
|---|---|---|---|
| **`payload`** | Bare ELF sent to `:9021` via `elfldr` (`pros send`). | Runs in memory outside the title sandbox. Elevated kernel privileges; direct raw socket access. | Low-level kernel syscalls, direct page table allocations, raw device drivers, and POSIX sockets. |
| **`eboot`** | Signed SELF launched via `pros launch`. | Runs as a retail `BIG_APP` (`category 0`). Direct HDMI display ownership; controller focus. | Universal graphics queues (`libSceAgc`), video flip queues, DualSense controller polling, and retail app lifecycle. |
| **`pkg`** | Installed package under encrypted PFS filesystem. | Strict retail sandbox permissions (`0600`). Restricted filesystem; full OS security checks. | Save data mounting (`libSceSaveData`), background downloads (`BGFT`), entitlement checks, and retail sandboxing. |

*Note: A function that succeeds in `payload` might fail in `pkg` due to sandbox restrictions, and vice-versa. Running a full sweep across all three legs isolates OS capabilities from sandbox boundaries.*

### 3. Active Probing vs. Passive Telemetry: The Tracer
- **obSCEne is Active Probing**: We craft the C test cases, choose inputs, test boundary conditions, and measure returns.
- **[tracer](../oops-apps/src/tracer/) is Passive Observation**: A companion tool in `oops-apps` that hooks real, running commercial games on PS5 hardware. It captures real call sequences, valid constants, actual PM4 DCB command buffers, and compiled RDNA2 shader bytecode without modifying game code. Decoded traces feed directly into `orbistoun-corpus`.

### 4. Run Hardware Sweeps via Prosperous
```bash
# Verify console is reachable
pros.exe check

# Execute hardware sweep across all three legs
./scripts/sweep.sh
```

### 5. Read Hardware Telemetry Reports
Hardware logs are saved to `reports/hardware/<timestamp>-<context>.obs.log`:
- `OBS|sym`: Symbol census record (present vs absent in firmware).
- `OBS|measure`: Numeric return code or benchmark measurement.
- `OBS|bytes`: Hex dump of memory buffers or PM4 command packet streams.

---

## Architecture & Probe Sections

```
src/probe/
├── sections/
│   ├── agc.c           # RDNA2 GPU universal queues, PM4 draw packets, compute shaders
│   ├── kernelprobe.c   # Virtual memory queries, direct memory maps, syscall errno
│   ├── threads.c       # Mutexes, semaphores, condition variables, fibers
│   ├── display.c       # AGC/GNM video out scanout buffers and flip queues
│   ├── input.c         # DualSense pad buttons, analog stick deadzones, triggers
│   ├── audio.c         # PCM audio ports, volume control, buffer depth
│   ├── net.c           # POSIX socket bind, listen, accept, echo
│   └── save.c          # Save data mounting and directory structures
├── harness.c           # Standalone runner and structured test harness
└── min.c               # Minimal payload entry point
```

---

## Cross-Project Links

- **[Master OOPS Front Door](../README.md)** — Collection overview and building instructions.
- **[The OOPS Loop](../docs/THE_LOOP.md)** — Master ecosystem loop specification.
- **[Prosperous](../prosperous/)** — Hardware transport deploying probes and capturing `klog`.
- **[Orbistoun](../orbistoun/)** — Clean-room emulator consuming obSCEne measurements.
- **[SELFish](../selfish/)** — Formats compiler providing signed containers for probe legs.
- **[oops-sdk](../oops-sdk/)** — Freestanding C runtime used by obSCEne payloads.
