# D212 - obSCEne is built by selfish, deployed by prosperous, and runs on the console - the toolchain is real


The three-part goal (built via selfish / deployed via prosperous / running) is met for the
socket report. A payload compiled from an ordinary body (plain C imports) is
linked with obSCEne's crt0 (`payload/crt0.c`) against a resolution table obSCEne generated
(`obscene-tool` example `gen_payload_table`, which consumes selfish's `dynamic_symbols` +
`Nid` to read the pulled `libkernel_sys.sprx`), sent with **prosperous** (`hw send` -> `pros_link`), and emits its full
report over the socket. Nothing hand-computed, no external SDK.

### The payload build path, as it now stands

1. compile the body + selfish's crt0 to objects;
2. stage-link to discover the imports the body carries (plain names);
3. `gen_payload_table` hashes each name to its NID (`selfish-nid`), finds it in the target
   libraries (`selfish-elf`), and emits `name -> (library, vaddr)`;
4. final link crt0 + body + table into a plain `ET_DYN`, 16 KB-aligned, `-Bsymbolic`.

The per-firmware vaddrs are the target's, read from its own files - the separation D210 asked
for. selfish owns the mechanism and the shape; obSCEne (or any caller) supplies the target.

### Three bugs, each a real property of the platform

- **`.bss` is not zeroed.** elfldr maps LOAD segments but leaves the `MemSiz > FileSiz` tail
  uninitialised, so every zero-init static holds garbage. It cost a resolver that read an
  uninitialised base cache as real. The crt0 now zeros `__bss_start`.._end first - which the
  whole of obSCEne's real code will depend on, being full of zero-init statics.
- **`-Bsymbolic` is mandatory.** Without it the crt0's own table globals are reached through
  `GLOB_DAT` slots that are not patched until the crt0 runs - which needs those globals. A
  chicken-and-egg the flag removes by binding internal references locally (RELATIVE relocs,
  which elfldr *does* apply).
- **Internal PLT slots need resolving too.** A defined symbol called through the PLT gets a
  `JUMP_SLOT` elfldr also leaves null; the crt0 resolves those to `base + st_value`.

### What is left

The full 515-check suite needs the same treatment at scale: it imports across many libraries,
so the crt0's multi-library path (load + `GetModuleInfo` for non-libkernel bases, already
written) gets exercised, and the table grows. And rendering still needs a foreground context
(D210) - the eboot/pkg homebrew-app path, which is selfish's next format to complete.

Status: **hardware** - built by selfish, run on the console, 2026-08-27.

