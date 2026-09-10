# 2026-09-10 - libSceAgc command buffer structure and crash elimination

Hardware sweep `20260910-174437-eboot` verified complete elimination of the five SIGBUS crashes
in Section 166 (`libSceAgc`) on PS5 FW 12.40 under native execution (`PPSA99980`). The suite ran
all the way to `OBS|end|none` with 11,155 records and 0 caught crashes.

## Command Buffer Writer Structure Layout

Disassembly of `libSceAgc.sprx` functions `sceAgcCbNop` (`0x2640`), `sceAgcCbReleaseMem` (`0x2700`),
`sceAgcDcbDmaData` (`0x47d0`), `sceAgcDcbWaitRegMem` (`0x6e00`), and `sceAgcDcbResetQueue` (`0x5fb0`)
revealed the exact memory layout of the libSceAgc command buffer writer structure:

- `+0x00` (`uint64_t begin`): Base pointer of the PM4 command buffer memory.
- `+0x08` (`uint64_t end`): End pointer of the currently active buffer window.
- `+0x10` (`uint64_t cur`): Current write cursor pointer where PM4 packets are written.
- `+0x18` (`uint64_t end2`): Capacity limit pointer.
- `+0x20` (`void (*overflow_cb)(void *ctx, ...)`): Callback function pointer executed when the
  writer runs out of space in the current command buffer segment.
- `+0x28` (`void *overflow_ctx`): User context pointer passed to `overflow_cb`.
- `+0x30` (`uint32_t reserved_dw`): Number of reserved 32-bit dwords required before writing.
- `+0x34` (`uint32_t pad34`): Padding for alignment.
- `+0x38` (`uint8_t cmdbuf[]`): Embedded/associated command buffer payload.

## Root Cause of the 5 SIGBUS Crashes

In earlier probes, `obs_agc_cb_probe` defined `pad[0x18]` spanning offsets `+0x20` to `+0x38`, and
`agc_cb_prepare()` poisoned this padding region with `0xCC`.

When command buffer emission routines ran:
1. They computed remaining capacity as `((end - cur) >> 2) - reserved_dw`. Because `reserved_dw`
   at offset `+0x30` was poisoned with `0xCCCCCCCC`, the arithmetic underflowed to negative.
2. This immediately tripped the buffer exhaustion check and branched to the overflow handler.
3. The overflow handler executed `call *0x20(%rdi)`. Because offset `+0x20` (`overflow_cb`) was
   poisoned with `0xCCCCCCCCCCCCCCCC`, it jumped directly to non-canonical/unmapped memory, triggering
   an immediate General Protection Fault / SIGBUS (`0xa`).
4. Additionally, `sceAgcDcbResetQueue` is not a constructor; it takes an active command buffer writer,
   emits a PM4 NOP (`0xffff1000`) and queue reset packets. Passing raw uninitialized memory triggered
   the same crash.

## Probe Refactoring

1. Explicit struct fields:
   `obs_agc_cb_probe` was updated with explicit `overflow_cb` (NULL), `overflow_ctx` (NULL),
   `reserved_dw` (0), and `pad34` (0).
2. Direct Memory Allocation:
   On native hardware, `get_agc_probe()` dynamically allocates Onion WB direct memory aligned to
   64 bytes via `oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION)` so that GPU command buffer
   space is genuinely DMA-addressable.
3. Call Frame & Stack Arguments:
   `agc_cb_run_two_pass()` now passes 12 clean zero arguments across AMD64 ABI register and stack
   slots to prevent stack garbage poisoning deeper function calls.
4. Reset Queue:
   `check_agc_dcb_reset_queue()` now targets `&probe->begin` of an initialized probe rather than raw
   poisoned bytes.

## Hardware Verification Results

From `reports/hardware/20260910-174437-eboot.obs.log`:

- `166-agc/cb-nop`: `pass|0x4` (emitted 4 bytes: PM4 NOP `0xffff1000`)
- `166-agc/cb-release-mem`: `pass|0x20` (emitted 32 bytes PM4 RELEASE_MEM)
- `166-agc/dcb-dma-data`: `pass|0x1c` (emitted 28 bytes PM4 DMA_DATA)
- `166-agc/dcb-wait-reg-mem`: `pass|0x38` (emitted 56 bytes PM4 WAIT_REG_MEM)
- `166-agc/dcb-reset-queue`: `pass|0x20` (emitted 32 bytes PM4 queue reset)
- `166-agc/create-shader`: `partial|0x8a6c002f` (refusal due to uninitialized AGC graphics context)
- `166-agc/dcb-constructor-audit`: `pass|0x0`
- `166-agc/patch-exclusion-guard`: `pass`

Section 166 tally: `7 pass | 1 partial | 0 fail | 1 skip | 0 crash | 0 timeout`.
Suite summary: 11,155 OBS records, `crashes(caught)=0`.
