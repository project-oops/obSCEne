# 2026-08-30 - 139-exports: vaddrs confirmed by behaviour, so the firmware file leaves the chain


The emulator needs libkernel's export vaddrs. Reading them from the pulled `libkernel_sys.sprx`
is borderline (scanning firmware) and not reproducible the way a hardware record is. The fix is
the project's own hypothesis-then-confirm pattern: a candidate vaddr has no provenance; calling
`base + vaddr` and checking the function behaves is the measurement.

`139-exports` does exactly that - confirms getpid (the anchor) and sceKernelWrite (the evidence:
a wrong base puts the write somewhere that is not the write) by behaviour, reports each confirmed
vaddr, and skips cleanly off-console where there is no base. Runtime gained `obs_libkernel_base()`,
defined outside the payload-only block so a section reads it on any build.

Host build clean, verify well-formed, guards and caps at 168. On hardware it produces the first
reproducibly-confirmed export vaddrs; the table grows one confirmable signature at a time.

