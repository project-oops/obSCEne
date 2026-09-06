# 2026-09-03 (Porthole M1) Encoder sysmodule load and sceVencCore* entry-point self-resolution

Completed Milestone 1 (M1) of the Porthole payload: dynamic loading of the hardware video
encoder sysmodule (`0x00A0`) and self-resolution of `libSceVencCore` entry points to callable
pointers via loaded module export table traversal.

1. **Wire contract stable in `src/porthole/porthole.h`.**
   Preserved exact wire layout (ports 9805/9806, 24-byte `PPAD` record layout, and status codes).
   Exposed `porthole_encoder_api` to carry resolved entry points (`sceVencCoreQueryMemorySize`,
   `sceVencCoreCreateEncoder`, `sceVencCoreGetAuData`, `sceVencCoreSetInputFrame`,
   `sceVencCoreStartSequence`, `sceVencCoreStopSequence`, `sceVencCoreDeleteEncoder`,
   `sceVencCoreSyncEncode`), alongside `porthole_encoder_get_api()` and `porthole_set_payload_args()`.

2. **Freestanding self-resolution in `src/porthole/porthole.c`.**
   Implemented `porthole_encoder_open()`:
   - Resolves `sceSysmoduleLoadModule` dynamically (via weak platform import or kernel R/W dynlib resolution).
   - Invokes `sceSysmoduleLoadModule(0x00A0)` (`VENC`), confirmed on hardware to return `0x0`.
   - Walks the loaded module's live kernel dispatch table (`kproc + 0x3E8` via `krw_dynlib_resolve_any`
     and NID hashing, D277/D278) to resolve the `sceVencCore*` function pointers.
   - Validates that essential entry points are non-null and located in callable address space (`>= 0x10000`).
   - Cleanly returns `PORTHOLE_NO_ENCODER` on host builds, preserving host self-test semantics.

3. **Updated selftest in `src/porthole/porthole_selftest.c`.**
   Verified static assertions for 24-byte record layout, valid/malformed record decode, and
   confirmed that `porthole_encoder_open()` returns `PORTHOLE_NO_ENCODER` and `porthole_encoder_get_api()`
   returns NULL when executed on the host.

4. **Aligned `src/probe/sections/encoder.c` (`106-encoder`).**
   Updated `obs_find_symbol_in_handle` to walk the live kernel dispatch table when kernel R/W
   (`krw_is_ready()`) is active. This connects the probe's symbol census and presence checks
   (`create-present`, `getaudata-present`, `query-present`) directly to the dynamic export table.

Verified: `make -C src/porthole check` (host selftest), `make -C src/porthole skeleton`
(freestanding cross-compilation for `x86_64-unknown-freebsd`), and `make host` all pass cleanly.

