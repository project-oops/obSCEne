# 177 - CP read extent measured past DCB declared size; inputext reachability on hardware

**2026-09-23**

Target hardware sweep via `pros probe` against native title `PPSA90000` on FW 12.40. Full suite ran to completion cleanly (no crashes, console healthy). This run settled `REQ-20260922T2230Z-6e81` and tested `REQ-20260922T1905Z-9c31`.

## 1. CP Read Extent & DCB Termination (`166-agc/dcb-extent`, REQ-20260922T2230Z-6e81)

Mesa's `fbotexture` was faulting on frame 3 at `0x400020000` (8.5 KiB past the end of a 7936-byte DCB). The question was whether the CP prefetches past declared `desc.size`, whether terminating NOPs/alignment are required, or whether `sceKernelBatchMap` leaves 2 MiB mappings incomplete.

All five arms of `166-agc/dcb-extent` passed on live hardware (`OBS|res|166-agc/dcb-extent|pass|0x1`):

1. **`arm1-exact-11dw` PASSED**: Submitted exact 11 DWORDs (`desc.size = 11`) with 0 trailing NOPs and memory immediately past word 11 poisoned with `0xdeadbeef`. The CP executed exactly the 11 DWORDs and retired its fence (`fence-hit = 0x1`, `fence-val = 0xbeef0001`). No fault into poison memory.
2. **`arm2-mesa-1984dw` PASSED**: Submitted exact 1984 DWORDs (7936 bytes, the exact Mesa frame-3 size) with 0 trailing NOPs into poisoned memory. Retired cleanly (`fence-hit = 0x1`, `fence-val = 0xbeef0002`).
3. **`arm3-nop-sweep` PASSED**: Swept trailing NOP padding (0, 4, 16, 64 NOPs) after 1984 DWORDs. All 4 padding levels retired cleanly (`nop-pad-0 = 1`, `nop-pad-4 = 1`, `nop-pad-16 = 1`, `nop-pad-64 = 1`). 0 trailing NOPs is completely valid; cacheline / 256B alignment padding is not required.
4. **`arm4-guard-page` PASSED**: Placed DCB at the end of Page 0 (`0x4000 - size*4`) with Page 1 unmapped via `oops_mem_unmap`. Submitted without NOP padding. Retired cleanly (`fence-val = 0xbeef0004`, `fence-hit = 1`) with zero GPU protection faults. The CP does not prefetch across page boundaries past declared size.
5. **`arm5-2mb-map` PASSED**: Executed CP `PACKET3_DMA_DATA` copies across Page 0, 1, 7, 8 (`0x20000`, Mesa's exact crash VA), and 127 in a 2 MiB `sceKernelBatchMap` buffer. All pages were readable and writable without GPU MMU faults (`dma-val = 0x12345678`).

**Finding for oops-mesa:** The CP strictly stops at `desc.size`. The Mesa frame-3 fault at `0x400020000` is not hardware prefetch overrun or kernel batch-map omission; it points to an unmapped buffer or tracking bug in Mesa's own buffer manager.

## 2. Input Extension Reachability (`101-input-ext`, REQ-20260922T1905Z-9c31)

- `101-input-ext/reachability` measured both `libSceKeyboard` and `libSceMouse` as `handle = 0xffffffff` (`unavailable`).
- Standard big-app / homebrew native titles do not auto-load or map `libSceKeyboard` or `libSceMouse` into their address space. Direct calls into unmapped stubs faulted with SIGSEGV (`crash 0xb`), which the fault harness safely trapped and recovered from.
- Reaching `sceKeyboardReadState` / `sceMouseRead` requires dynamic module loading via `sceSysmoduleLoadModule` or specific privileges.

## Data delivered along the way

- `REQ-20260922T2230Z-6e81`: RESOLVED on the bus.
- `REQ-20260922T1905Z-9c31`: Updated with hardware finding (big-apps require explicit sysmodule load to reach keyboard/mouse).

