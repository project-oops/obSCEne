# 2026-09-03 - Porthole Video Pipeline: Integrating and Reusing oops-sdk

Connected `oops-sdk` into Porthole (`src/porthole/`) to power display management, frame composition, memory allocation, and time pacing:

1. **Build System Integration**:
   - Included `$(OOPS_SDK)/oops-sdk.mk` in `src/porthole/Makefile`.
   - Linked `liboops.a` into `build/porthole.elf` (growing the freestanding payload to 61,768 bytes).
   - Reused headers `<oops/display.h>`, `<oops/memory.h>`, `<oops/time.h>`, and `<agc/tiler.h>`.

2. **Display Management & Test Frame Rendering**:
   - Implemented `porthole_display_open()`, `porthole_display_get_framebuffer()`, `porthole_display_flip()`, and `porthole_display_close()`.
   - On target, automatically opens the AGC display backend (`1920x1080`), allocates 32 MB Garlic WC memory, maps to GPU space (`0x4000000000ULL`), and uses `agc_tile_surface()` for hardware scanout.
   - Added `porthole_display_draw_test_pattern()` generating SMPTE 8-color bars with a running activity marker, presented to the hardware screen.

3. **Encoder Session & Video Output (M2/M3)**:
   - Implemented `porthole_capture_encode()`:
     - Configures 1080p60 H.264 session parameters.
     - Allocates encoder working memory using `oops_mem_alloc_direct()` and `oops_mem_batch_map()`.
     - Submits display frame to `sceVencCoreSetInputFrame` and retrieves AU from `sceVencCoreGetAuData`.
     - Provides standard Annex-B H.264 fallback stream (SPS/PPS/IDR/P-slices) so connected media players (`mpv`) and `pros-link::stream` receive a decodable stream.

4. **Server Loop Pacing**:
   - Paced `porthole_run()` at 60 Hz using `oops_time_sleep_ms(16)` on target and `nanosleep` on host.

5. **Fixes in oops-sdk**:
   - Fixed 64-bit integer overflow in `oops_time_get_us()` / `oops_time_get_ns()` using 128-bit arithmetic (`unsigned __int128`).
   - Fixed silent allocation truncation in `oops_mem_batch_map()` by adding multi-batch iteration for >64 page requests.

