# D316 - Porthole input injection via Ghostpad Virtual Device Interface (VDI)

**decided** - 2026-09-03

---

## The Problem

Porthole receives 24-byte `PPAD` packets on TCP port 9806. To control the console and games:
1. The payload must inject these inputs into the PS5 input subsystem.
2. Standard `scePad*` APIs (`scePadReadState`) are consumers of input, not providers.
3. The platform's Virtual Device subsystem (`libScePad`) is the underlying mechanism used by the OS for Remote Play and virtual gamepads.

## Decision

1. **Ghostpad Virtual Device Interface (VDI)**:
   - Use `libScePad`'s virtual device calls (`scePadVirtualDeviceAddDevice`, `scePadVirtualDeviceInsertData`, and `scePadVirtualDeviceDeleteDevice`).
   - Self-resolve these symbols via the live kernel dispatch table walk (D277/D300).
   - Lazily register a virtual DualSense device (type 3) upon receiving the first input record for a given slot (0..3).

2. **Input Freshness Policy (State, Not Events)**:
   - Controller input is a continuous *state*, not an event stream.
   - Maintain per-slot sequence tracking (`last_sequence`).
   - If an input record arrives with a sequence number less than or equal to `last_sequence`, discard it immediately. This prevents replaying stale joystick positions or button presses caused by network jitter.
   - Slot tracking is strictly independent: activity on slot 0 never affects the sequence freshness of slot 1.

3. **Packet Translation**:
   - The 24-byte `PPAD` record layout was designed to match the Ghostpad pad structure starting at byte offset 8 (`uint32_t buttons`, 4 stick bytes, 2 trigger bytes).
   - `porthole_pad_apply` packs these fields into the 16-byte virtual pad input packet and dispatches it via `scePadVirtualDeviceInsertData`.

## Consequences

- Full controller input support on port 9806 with sub-millisecond dispatch.
- Zero libc and zero dynamic memory allocations in the input path.
- Host selftests verify sequence freshness, monotonic ordering, and multi-slot tracking without needing hardware.

