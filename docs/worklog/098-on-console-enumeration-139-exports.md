# 2026-08-31 (on-console enumeration) - 139-exports enumerates the full defined export table from memory


Added `139-exports/enumerate` to section `139-exports` to enumerate `libkernel_sys`'s full dynamic export table directly from its in-memory image at `obs_libkernel_base()`. Under an unsandboxed `elfldr` payload, the runtime parses the mapped `PT_DYNAMIC` segment -> `.dynsym` and `.dynstr` tables, emitting each defined export (`st_shndx != 0 && st_value != 0`) as `OBS|measure|139-exports/enumerate|<encoded-NID>|vaddr|0x<vaddr>|offset`.

This achieves the goal of retiring offline firmware `.sprx` dumps. The live report stream is ingested by `obscene-tool vaddrs` to resolve NIDs against `data/mined-names.txt` into standard `name 0x<vaddr>` pairs. Provenance boundaries remain strict: virtual address offsets are measured runtime facts from hardware, while names come exclusively from obscene's open mined corpus. Documented in D268.


