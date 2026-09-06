# D300 - Porthole encoder self-resolution: loading VENC sysmodule and export table traversal

Status: decided.
Date: 2026-09-03

Hardware testing of section `106-encoder` on 2026-09-01 settled the Porthole go/no-go:
`sceSysmoduleLoadModule(0x00A0)` (`VENC`) returns `0x0` from an unsigned payload, and the module
is mapped in the process (handle `0x14`). However, like all unsigned payload execution on PS5,
its imports do not auto-bind (`sceVencCore*` unresolved via static tables, and `sceKernelDlsym`
refuses under retail SDK constraints, D244).

### Decision

1. **Load VENC sysmodule dynamically (`0x00A0`)**:
   `porthole_encoder_open()` resolves `sceSysmoduleLoadModule` dynamically (via weak platform import
   or kernel R/W dynlib resolution) and loads the VENC module.

2. **Self-resolve `libSceVencCore` entry points via live export table walk (D277/D278)**:
   Rather than relying on bound import tables or `sceKernelDlsym`, the payload inspects the loaded
   module's live kernel dispatch table (`kproc + 0x3E8` via `krw_dynlib_resolve_any` / `krw_dynlib_resolve`).
   The Sony kernel maintains an in-memory dispatch table containing 11-character NIDs and function offsets
   relative to the module base (`module_base + func_offset`).

3. **Callable verification without guessing parameter layouts (D008)**:
   The essential entry points (`sceVencCoreQueryMemorySize`, `sceVencCoreCreateEncoder`, `sceVencCoreGetAuData`,
   `sceVencCoreSetInputFrame`, `sceVencCoreStartSequence`, `sceVencCoreStopSequence`, `sceVencCoreDeleteEncoder`,
   `sceVencCoreSyncEncode`) are resolved to valid virtual addresses (`>= 0x10000`) and stored in
   `porthole_encoder_api`. Struct-taking calls are reserved for M2 and the protocol harness to prevent
   stack corruption from unconfirmed parameter layouts.

4. **Honest host behavior**:
   On host builds or off-console environments, `porthole_encoder_open()` returns `PORTHOLE_NO_ENCODER`,
   leaving the wire contract selftest passing and distinguishable from on-console execution.

