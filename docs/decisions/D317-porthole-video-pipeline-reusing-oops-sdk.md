# D317 - Porthole video pipeline reusing oops-sdk display and direct memory

**decided** - 2026-09-03

---

## Context

Porthole provides target video streaming out over port 9805 (Annex-B H.264) and controller input injection in over port 9806 (PPAD) without proprietary remote play protocols (`prosperous/docs/VIDEO.md`).
`oops-sdk` provides hardware display management (`oops_display_*`), direct memory allocation and batch mapping (`oops_mem_*`), and RDNA2 macro-tiling (`agc_tile_surface`).

## Decision

1. **Reuse `oops-sdk` as the Display and Memory Foundation**:
   Instead of writing separate memory allocators or direct display register writes in Porthole, `src/porthole/Makefile` includes `$(OOPS_SDK)/oops-sdk.mk` and links `liboops.a` into `porthole.elf`.
2. **Display Management and Test Pattern Generation**:
   `porthole_display_open()` invokes `oops_display_open(OOPS_DISPLAY_BACKEND_AUTO, 1920, 1080)`. On PS5, this automatically maps 32 MB WC Garlic memory, establishes double buffers, renders SMPTE color bars + frame activity markers into the linear framebuffer, and tiles to scanout via `oops_display_flip()`.
3. **Encoder Session Working Memory**:
   `porthole_capture_encode()` allocates the direct memory working buffer required by `sceVencCoreCreateEncoder` using `oops_mem_alloc_direct()` (Garlic WC) and `oops_mem_batch_map()`, ensuring hardware video encoder sessions instantiate cleanly.
4. **Annex-B H.264 Stream Delivery**:
   Frame data is fed to `sceVencCoreSetInputFrame` and retrieved via `sceVencCoreGetAuData`, with an Annex-B fallback test stream emitting periodic SPS/PPS/IDR and non-IDR slices for network clients (`mpv` and `pros-link::stream`).

