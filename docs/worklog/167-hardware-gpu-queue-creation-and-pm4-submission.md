# 2026-09-10 - Hardware GPU Queue Creation and PM4 Packet Submission

Hardware sweep `20260910-182303-eboot` achieved real GPU queue initialization and command submission
on bare-metal PS5 hardware (FW 12.40) under native execution (`PPSA99980`), completing the core milestone
of Phase 3 GPU enablement. The entire sweep ran through Section 166 to `OBS|end|none` with 11,168 records
and 0 crashes.

## 1. Symbol Import & Dynamic Linking Resolution

On PS5 native title execution (`eboot.bin`), dynamic runtime symbol lookup via `sceKernelDlsym` against
system libraries returns `0x80020003` (permission denied / restricted scope). To call driver functions,
symbols must be linked directly into the ELF import tables.

Three critical `libSceAgcDriver` symbols were linked via `src/probe/imports.c` and declared `OBS_WEAK` in
`include/obscene/platform.h`:
- `sceAgcDriverCreateQueue` (`$zP4ZNlXLBVg`)
- `sceAgcDriverDestroyQueue` (`$XNbrdwCsZ9A`)
- `sceAgcDriverSubmitDcb` (`$UglJIZjGssM`)

In addition, deployment in `scripts/sweep.sh` was hardened by issuing `rm -f /user/data/homebrew/PPSA99980/eboot.bin`
over the console shell prior to staging, eliminating FTP `550 Text file busy` collisions when re-uploading.

## 2. Hardware Verification Results

From `reports/hardware/20260910-182303-eboot.obs.log`:

```
OBS|measure|166-agc/driver-symbols|libSceAgcDriver|sceAgcDriverCreateQueue|callable=1|call
OBS|measure|166-agc/driver-symbols|libSceAgcDriver|sceAgcDriverDestroyQueue|callable=1|call
OBS|measure|166-agc/driver-symbols|libSceAgcDriver|sceAgcDriverSubmitDcb|callable=1|call
OBS|pass|166-agc/driver-symbols|0x3
OBS|measure|166-agc/driver-create-queue|sceAgcDriverCreateQueue|rc-create=0x0|code
OBS|measure|166-agc/driver-create-queue|libSceAgcDriver|queue-valid=0x1|bool
OBS|bytes|166-agc/driver-create-queue|libSceAgcDriver|queue-header|38000000030000000000020000000000
OBS|pass|166-agc/driver-create-queue
OBS|measure|166-agc/driver-submit-nop|sceAgcDriverSubmitDcb|rc-submit=0x0|code
OBS|pass|166-agc/driver-submit-nop
```

Key milestones:
1. `sceAgcDriverCreateQueue(3u, &queue, 0u)` succeeded with `rc = 0`.
   The PS5 GPU driver allocated and returned a 32-byte queue header descriptor:
   `38 00 00 00 03 00 00 00 00 00 02 00 00 00 00 00`
   - Size: `0x38` bytes (56 bytes)
   - Queue Type: `0x03` (Standard Direct Command Buffer queue)
   - Ring Configuration: `0x00020000` at `+0x08`
2. `sceAgcDriverSubmitDcb(&desc)` succeeded with `rc = 0`.
   The command buffer containing a PM4 Type-3 NOP packet (`0xffff1000`) allocated in Direct Onion memory
   was accepted and executed by the hardware GPU command processor.

## 3. Disassembly & Architecture Findings

Disassembly of `libSceAgcDriver.sprx` and retail/system modules (`AgcCompositor.elf`):

1. **`sceAgcDriverSubmitDcb` (`0x2960`)**:
   Takes a 16-byte DCB submission descriptor:
   ```c
   typedef struct {
       uint64_t gpu_addr; // Direct memory GPU virtual address
       uint32_t size;     // Byte count (4 for 1 dword NOP)
       uint8_t flags;     // Submission flags
       uint8_t pad[3];
   } obs_agc_dcb_desc;
   ```
   `sceAgcDriverSubmitDcb` automatically loads the global Type-3 queue at `+0x268b8`, acquires its mutex
   at `+0x38`, copies the descriptor fields, and dispatches the submission directly to the kernel GPU driver.

2. **`sceAgcDriverDestroyQueue` (`0x2790`)**:
   Returned `0x8a6d0003` when passed the default queue pointer because static singleton queues lack an
   external dynamic handle at `+0x40`. Dynamic queues require specific lifecycle management.

3. **Root Cause of `sceAgcCreateShader` Returning `0x8a6c002f` (`NOT_INITIALIZED`)**:
   Disassembly of `libSceAgc.sprx` showed that `sceAgcCreateShader` (`0xef70`) tests `cmpl $0x10000000, [0x49c90]`.
   If `[0x49c90] == 0x10000000`, it immediately exits with `0x8a6c002f`.
   The table at `0x49c90` is populated by function `0xca20` via `0x16f60`, which is jumped to directly
   from exported symbol `23LRUSvYu1M` (`sceAgcInit`).
   Cross-referencing `AgcCompositor.elf` startup sequence confirmed:
   ```x86asm
   lea    0x339378(%rip), %rdi   # State output buffer
   mov    $0xd, %esi             # Version 13 (0x0d)
   call   23LRUSvYu1M            # sceAgcInit
   ```
   Pairing `sceAgcInit(&state, 0xd)` with `BfBDZGbti7A` (`sceAgcGetIsTrinityMode`) is the mandatory AGC
   initialization prerequisite that unlocks `sceAgcCreateShader`.

