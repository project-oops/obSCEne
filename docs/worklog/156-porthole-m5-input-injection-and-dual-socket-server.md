# 2026-09-03 - Porthole M4/M5: Controller input injection (Ghostpad VDI) and dual-socket network server

Implemented the controller input injection path (Milestone 5) and the dual-socket server loop (Milestone 4) for Porthole in `src/porthole/`:

1. **Ghostpad Virtual Device Interface (`libScePad`)**:
   - Declared `porthole_pad_api` exposing `scePadInit`, `scePadVirtualDeviceAddDevice`, `scePadVirtualDeviceInsertData`, and `scePadVirtualDeviceDeleteDevice`.
   - In freestanding target C, dynamically resolved these symbols via kernel dispatch table walk (`krw_dynlib_resolve_any` / D277).
   - Lazily created virtual DualSense device (type 3) per active slot (0..3).
   - Mapped `porthole_pad`'s buttons, stick axes, and analog trigger pressure bytes directly to the 16-byte target pad structure, dispatched via `scePadVirtualDeviceInsertData`.

2. **Input Freshness Policy**:
   - Per `prosperous/docs/VIDEO.md`, input is state, not an event stream.
   - Enforced per-slot sequence tracking (`s_slots[slot].last_sequence`).
   - Out-of-order or duplicate packets are silently dropped so stale stick inputs never replay.

3. **Dual-Socket Server Loop (`porthole_run`)**:
   - Bind and listen on port 9805 (video out) and port 9806 (input in).
   - Serviced non-blocking reads of 24-byte `PPAD` frames from connected clients on port 9806, invoking `porthole_pad_decode()` and `porthole_pad_apply()`.
   - Set up frame loop on port 9805 for Annex-B H.264 stream delivery.
   - Added `porthole_stop()` to break the loop cleanly.

4. **Testing**:
   - Added comprehensive host assertions in `porthole_selftest.c`: verified sequence monotonicity, stale-packet dropping, independent slot tracking, and slot bounds checking.
   - Verified freestanding target compilation `porthole.elf` (35,720 bytes) and host suite (`make host`).

