# obscene-injector - Native PS5 Process Injector

A freestanding ELF payload that consumes session kernel R/W to hijack a target process, map `obSCEne` segments, and execute the conformance probe natively.

---

## 1. Why the Injector Exists

As recorded in **D276**, standard payload execution (`make payload` loaded via `elfldr`) runs in the **PS4 backward-compatibility sandbox** (`payload/ps4-bc`). In that environment:
- Current-generation GPU APIs (`libSceAgc`) cannot be mapped or linked.
- Dynamic introspection (`sceKernelGetModuleInfo`, `sceKernelDlsym`) is refused with `0x80020016` / `0x80020003`.
- The runtime linker debug table (`DT_DEBUG`) is absent.

To measure real, native Prospero platform behavior (`payload/ps5-native`), `obSCEne` must run inside a native-category process (such as a launched retail title or system helper). `obscene-injector` bridges this gap.

---

## 2. Decoupled Architecture

The probe and injector are strictly decoupled:
- **The probe never links the injector.** `src/probe/sections/` and `src/probe/runtime.c` remain clean conformance probe code.
- **The injector never links the probe checks.** `src/injector/` links only `src/common/freestd.c` and its own files.
- **Shared freestanding layer.** `src/common/` holds freestanding helpers (`freestd.h`/`freestd.c`) and the kernel R/W contract (`krw.h`).

```
obscene/
  src/common/                ← Shared freestanding layer
    freestd.c  freestd.h     ← strlen, format, and memory helpers
    krw.h                    ← Kernel-R/W interface specification
  src/injector/              ← Injector payload (does NOT link probe checks)
    injector.c               ← Entry: init R/W → elevate creds → resolve target → load → hijack
    loader.c   loader.h      ← Map ELF into process (libelfldr-shaped)
    procctl.c  procctl.h     ← ptrace-style process control (attach / set-regs / cont)
    krw.c                    ← R/W implementation consuming session primitive
    target.c   target.h      ← Target process resolver (foreground app / name / pid)
  link/injector.ld           ← ET_DYN payload linker script (16 KiB page aligned)
  docs/INJECTOR.md           ← This specification
```

---

## 3. Kernel R/W Interface (`krw.h` / `krw.c`)

The injector receives `payload_args_t` from the session exploit chain (`kstuff-lite` / `elfldr`):

```c
typedef struct payload_args {
    int (*sys_dynlib_dlsym)(int, const char *, void *);
    int *rwpipe;
    int *rwpair;
    long kpipe_addr;
    long kdata_base_addr;
    int *payloadout;
} payload_args_t;
```

### Primitive Mechanics
- **Kernel Write**: Sets IPv6 socket packet options (`setsockopt(IPV6_PKTINFO)`) on `rwpair` to construct arbitrary kernel writes via `kpipe_addr`.
- **Kernel Read / Copyout**: Uses the modified pipe buffer flags to read kernel memory through `sys_read(rwpipe[0])`.
- **Credential Escalation**: `krw_elevate_current_process()` sets `authid = 0x4800000000010003` and enables full capability bits (`caps[16] = 0xff`), granting unrestricted `ptrace` access and unblocking protected syscalls.

---

## 4. Execution & Injection Flow: What Happens Under the Hood

When `./bin/obscene inject` is executed, the entire process is automated in a single round-trip:

```mermaid
sequenceDiagram
    participant CLI as ./bin/obscene inject
    participant Tool as obscene-tool (Prosperous)
    participant Loader as elfldr (:9021 on PS5)
    participant Injector as obscene-injector.elf
    participant Title as Native PS5 Title (Foreground)

    CLI->>CLI: 1. Builds obscene-payload.elf
    CLI->>CLI: 2. Embeds payload into obscene-injector.elf
    CLI->>Tool: 3. Invoke hw send + hw logs in parallel
    Tool->>Loader: 4. Transmit injector via prosperous (TCP :9021)
    Loader->>Injector: 5. Execute injector with session kernel R/W
    Injector->>Injector: 6. Elevate ucred (authid = 0x4800000000010003)
    Injector->>Title: 7. ptrace attach to foreground title
    Injector->>Title: 8. Map embedded probe segments & apply relocations
    Injector->>Title: 9. Set RIP = obscene_start and ptrace detach
    Title->>CLI: 10. obSCEne runs in ps5_mode and streams report (socket + klog)
```

### Detailed Pipeline Breakdown:

1. **Self-Contained Bundle Build**:
   - `make payload` produces `build/obscene-payload.elf` (the full conformance probe).
   - `make injector` links `src/injector/blob.S` to embed the probe ELF directly into the `.rodata` section of `build/obscene-injector.elf` (with runtime disk fallback to `/data/` if needed).

2. **Network Delivery via Prosperous**:
   - Relays to `obscene-tool hw send build/obscene-injector.elf`.
   - The tooling calls **Prosperous** (`pros_link`) to stream the self-contained injector over TCP port 9021 to `elfldr` on the PS5.
   - `scripts/payload-run.sh` concurrently initiates background log capture (`obscene-tool hw logs`).

3. **Kernel Privilege Elevation**:
   - `elfldr` executes `obscene-injector.elf` with the active session's `payload_args_t`.
   - `krw_init()` initializes the IPv6 socket-pair and pipe buffer primitives.
   - `krw_elevate_current_process()` patches the injector process `ucred`: sets `authid = 0x4800000000010003` and enables all capability bits, allowing unrestricted `ptrace` system calls.

4. **Target Attachment & ELF Loading**:
   - `target_resolve()` traverses the kernel `allproc` chain to locate the PID of the foreground native title (e.g. `eboot.bin`).
   - `procctl_attach(target_pid)` halts the title.
   - `loader_load_into_proc()` allocates virtual memory via remote `mmap`, copies `PT_LOAD` segments, applies `R_X86_64_RELATIVE` relocations, and enforces segment page permissions (`PROT_READ`, `PROT_WRITE`, `PROT_EXEC`) with 16 KiB page alignment.

5. **Thread Hijacking & Native Execution (`payload/ps5-native`)**:
   - The injector pushes the original `RIP` onto the target stack as a return address, sets `RIP` to `obscene_start`, and passes remote `payload_args_t` via `RDI`.
   - The injector restores its own credentials, detaches via `ptrace`, and allows the native title to resume.
   - The probe runs natively inside the retail/native game process with full Prospero subsystem access.

6. **Dual-Channel Capture**:
   - `scripts/payload-run.sh` collects the emitted records from both the incoming TCP return socket (`$into`) and the system log (`$klog`), outputting a unified report summary to the terminal.

---

## 5. Building the Injector

```bash
# Build the injector payload
make injector HARDWARE=1

# Build both payload and injector
make payload injector HARDWARE=1
```

Outputs:
- `build/obscene-payload.elf` (the conformance probe)
- `build/obscene-injector.elf` (the process injector)

---

## 6. How to Use: Getting into Native PS5 Mode (`ps5_mode`)

To evaluate native Prospero platform behavior (such as `libSceAgc`, native graphics drivers, and unrestricted introspection APIs), follow this runbook:

### Step 1: Pre-flight Checklist
1. **Jailbroken Console**: A PS5 running compatible firmware (1.00 - 13.00) with kernel exploit and `kstuff-lite` active.
2. **Payload Server**: Payload receiver listening on port 9021 (standard `elfldr` / `pldmgr` setup).
3. **Network Configuration**: Host machine connected to the target over LAN, with the
   target registered by name. Registrations live in `hardware.txt` under the tool's own
   data root - `obscene-tool hw` resolves `--name` through it (`tool/src/hardware.rs`).
   There is no `data/hardware/console.toml`; nothing reads one.

### Step 2: Launch a Native PS5 Title
1. On the console dashboard, launch any **native PS5 game or native application** (disc or digital title).
2. Keep the title running in the foreground (it will provide the native Prospero process context).

### Step 3: Run the Injection via `./bin/obscene`
From your development workstation, run:

```bash
# Build both artifacts, inject into the running title, and capture the report
./bin/obscene inject
```

#### Useful Flags:
- `--seconds N`: Duration of log capture window (default `90` seconds).
- `--into FILE`: Where to store the captured records (default `reports/hardware/injector-klog.txt`).
- `--build-only`: Compile artifacts without sending over the network.
- `--name NAME`: Target a specific console if multiple are registered.

Alternatively, if sending payloads manually:
```bash
# 1. Build artifacts
make payload injector HARDWARE=1

# 2. Stage probe ELF on console (if not embedded)
# Stored at /data/obscene.elf or /data/payload.elf via FTP/shsrv

# 3. Send injector to elfldr (port 9021)
obscene-tool hw send build/obscene-injector.elf --seconds 90
```

### Step 4: What the Injector Does
1. **Initializes Kernel R/W**: Consumes the session's socket-pair and pipe buffer primitive.
2. **Elevates Privileges**: Grants full root capabilities and `authid = 0x4800000000010003` to allow unrestricted `ptrace` control.
3. **Discovers Foreground Title**: Walks the kernel `allproc` table to find the running native game's PID.
4. **Maps Probe Segments**: Attaches to the title, allocates virtual address space via remote `mmap`, copies `PT_LOAD` segments, and applies relocations.
5. **Hijacks Thread & Executes**: Updates target registers (`rip` -> entry point, `rdi` -> runtime args) and detaches.

### Step 5: Verifying Native PS5 Execution
Inspect the resulting report header:

```text
OBS|build|payload|...
OBS|context|payload/ps5-native
```

- **`payload/ps4-bc`**: Indicates standard un-injected payload running in the PS4 compatibility sandbox.
- **`payload/ps5-native`**: Confirms successful injection and execution within a native Prospero process!

In `payload/ps5-native` mode:
- `libSceAgc` / `libSceAgcDriver` APIs load without `ABIVERSION mismatch`.
- Introspection functions and native system libraries execute with native category permissions.


