# D268 - On-console libkernel export table enumeration replaces offline sprx dumps


The libkernel vaddr table (`orbistoun-firmware/data/libkernel-vaddrs.txt`, 1,867 entries) was previously produced by reading the real 12.40 `libkernel_sys.sprx`: `selfish`'s `libkernel_vaddrs` example dumped `(encoded-NID, vaddr)` from the file's `.dynsym`, and `obscene-tool vaddrs` resolved NIDs to names via the mined corpus (`data/mined-names.txt`). While functional, that workflow depended on pulling decrypted firmware files onto developer machines.

### The Shift: Live In-Memory Enumeration

Under an unsandboxed `elfldr` payload, `libkernel` is mapped directly in the process address space at `obs_libkernel_base()`. Instead of confirming only a handful of hardcoded candidate addresses behaviorally (`139-exports/confirm`), `139-exports/enumerate` walks `libkernel`'s in-memory `PT_DYNAMIC` segment -> `.dynsym` and `.dynstr` directly at runtime.

For every defined export (`st_shndx != 0 && st_value != 0`), `139-exports/enumerate` emits:
```text
OBS|measure|139-exports/enumerate|<encoded-NID>|vaddr|0x<st_value>|offset
```
Streaming these measurements out over the report channel allows `obscene-tool vaddrs` to ingest the live console report and resolve NIDs against `data/mined-names.txt`.

### Provenance Boundaries Preserved

Nothing vendor-derived is committed to the repository:
1. Virtual address offsets (`vaddr`) are **measurements** of the running system on real hardware.
2. Symbol names come strictly from `obscene`'s own mined corpus (`data/mined-names.txt`).
3. The offline `.sprx` dump is retired, making a jailbroken console running `obscene` the sole, definitive source of truth for the `libkernel` vaddr table.

Status: **done**.

