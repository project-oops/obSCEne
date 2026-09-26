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

<!-- obscene:counts -->
**501 checks across 53 sections**, 39493 censused symbols across 372 libraries.

Of those checks, 82 rest on a public specification, 44 on the specification of the system this kernel derives from, 2 on independent implementations that agree, and 368 on this project's own reasoning. **3 have been confirmed on real hardware**, which is the number that limits what any of this can claim.
<!-- /obscene:counts -->

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
2. [Prosperous](../prosperous/) delivers the probe to a physical PS5 on the local network.
3. The probe executes on the metal, logging exact return values and hex dumps of memory buffers to `klog`.
4. The verified telemetry is fed back into [Orbistoun](../orbistoun/) with `known_by: measured`, permanently closing the gap with 100% clean-room provenance.

---

## Developer Quickstart

### 1. Build the Probe
obSCEne cross-compiles for the FreeBSD-based console ABI using `clang` and `lld`:

```bash
./bin/obscene build    # compiles payload, module, host test harness, and the injector
./bin/obscene check    # runs verification suite (what CI runs)
./bin/obscene pkg      # creates installable package (default title id ORBO00001)
```

### 2. Why Three Target Builds? (`payload`, `eboot`, `pkg`)
On real console firmware, **system privileges, sandbox boundaries, and dynamic library resolution change based on how a process is launched**. Testing all three execution contexts (`./scripts/sweep.sh`) is essential to accurately map the platform:

| Build Shape | Delivery & Context | Privileges & Sandboxing | What It Measures |
|---|---|---|---|
| **`payload`** | Bare ELF sent to `:9021` via `elfldr` (`pros send`). | Runs inside the previous generation's compatibility sandbox (`ps4_mode`) - not outside any sandbox. Dynamic introspection and the current-generation graphics driver are unavailable there. | Low-level kernel syscalls, direct page table allocations, raw device drivers, and POSIX sockets. |
| **`eboot`** | Fake-signed fSELF (`eboot.bin`) launched via `pros launch`. | Runs as a retail `BIG_APP` (`category 0`). Direct HDMI display ownership; controller focus. | Universal graphics queues (`libSceAgc`), video flip queues, DualSense controller polling, and retail app lifecycle. |
| **`pkg`** | Installed package under encrypted PFS filesystem. | Strict retail sandbox. The report file itself is sealed `0600` inside it. Restricted filesystem; full OS security checks. | Save data mounting (`libSceSaveData`), background downloads (`BGFT`), entitlement checks, and retail sandboxing. |

*Note: A function that succeeds in `payload` might fail in `pkg` due to sandbox restrictions, and vice-versa. Running a full sweep across all three legs isolates OS capabilities from sandbox boundaries.*

### 3. Active Probing vs. Passive Telemetry: The Tracer
- **obSCEne is Active Probing**: We craft the C test cases, choose inputs, test boundary conditions, and measure returns.
- **[tracer](../oops-apps/src/oops-payloads/tracer/) is Passive Observation**: A companion tool in `oops-apps` that hooks real, running commercial games on PS5 hardware. It captures real call sequences, valid constants, actual PM4 DCB command buffers, and compiled RDNA2 shader bytecode without modifying game code. Decoded traces feed directly into `orbistoun-corpus`.

### 4. Run Hardware Sweeps via Prosperous
```bash
# Verify console is reachable
pros.exe check

# Execute hardware sweep across all three legs
./scripts/sweep.sh
```

### 5. Read Hardware Telemetry Reports
Hardware logs are saved to `reports/hardware/<timestamp>-<context>.obs.log` or retrieved directly from persistent console sinks:
- `OBS|sym`: Symbol census record (present vs absent in firmware).
- `OBS|measure`: Numeric return code or benchmark measurement.
- `OBS|bytes`: Hex dump of memory buffers or PM4 command packet streams.

Pull the latest report file from physical console storage via prosperous FTP:
```bash
./bin/obscene pull-log                         # auto-discovers newest report-<ts>.txt into reports/obscene-report.txt
./bin/obscene verify reports/obscene-report.txt # verify conformance against format contract
```

---

## Architecture & Probe Sections

```
src/probe/
├── sections/      # the checks, one file per topic - agc.c, gnm.c, memory.c, thread.c, sync.c,
│                  # net.c, modules.c, posix.c, and 30-plus more; src/probe/registry.c is the
│                  # authoritative, ordered list of every section actually built
├── harness.c      # runs the checks in order and emits records
└── min.c          # minimal payload entry point
```

---

## Cross-Project Links

- **[Master OOPS Front Door](../README.md)** — Collection overview and building instructions.
- **[The OOPS Loop](../docs/THE_LOOP.md)** — Master ecosystem loop specification.
- **[Prosperous](../prosperous/)** — Hardware transport deploying probes and capturing `klog`.
- **[Orbistoun](../orbistoun/)** — Clean-room emulator consuming obSCEne measurements.
- **[SELFish](../selfish/)** — Formats compiler providing signed containers for probe legs.
- **[oops-sdk](../oops-sdk/)** — Freestanding C runtime used by obSCEne payloads.
