# D211 - A general payload crt0 resolves obSCEne's imports on-device - proven on hardware, and the artifact that moves into selfish


D209 emitted a report by computing function addresses by hand. That does not scale to the real
suite, which calls hundreds of functions through ordinary C imports. A crt0 that resolves them
automatically is the piece that lets obSCEne's *actual code* run unchanged, and it now works on
the console.

### What it does, in order

1. **Finds its own load base** from `__ehdr_start` (a linker symbol elfldr relocates via the
   `R_X86_64_RELATIVE` entries it *does* apply) - measured `0x200000000` this run.
2. **Finds libkernel's base** from `payload_args[0]` (getpid) minus getpid's vaddr `0x5b0`.
3. **Walks its own dynamic tables** (`DT_JMPREL`, `DT_RELA`, `DT_SYMTAB`, `DT_STRTAB`), which
   are in the payload and readable.
4. **Patches every `JUMP_SLOT`/`GLOB_DAT`**: a symbol defined in the payload resolves to
   `base + st_value` (elfldr leaves internal PLT slots unresolved too - this was the crash that
   took two rounds to find); an undefined symbol is a real import, resolved by name.
5. **Jumps to the body**, which calls `sceKernelWrite` through a *normal* import and it works.

### The two lessons

- **A raw payload carries plain names, not encoded NIDs.** Encoding happens when `mkmodule`
  builds a *module*; a plain `clang -shared` payload keeps `sceKernelWrite` in its `.dynstr`.
  So the resolution table is `name -> vaddr`, and selfish computes the vaddr by hashing the
  name to its NID and finding that NID in the target library.
- **Internal calls go through the PLT under `-fPIC -shared`.** `obscene_main` had its own
  `JUMP_SLOT` and calling it crashed until the crt0 resolved defined-symbol slots to
  `base + st_value`. `-Bsymbolic`/`-fvisibility=hidden` would remove most of these; the crt0
  handles them regardless, which is more robust.

### Where it goes

This crt0 is the payload build path's runtime half. The crt0 stays in obSCEne:
selfish's charter excludes runtime ("a `crt` ... do not [belong here]"), so obSCEne owns
`payload/crt0.c` and the generator (`tool/examples/gen_payload_table.rs`), both consuming
selfish's format primitives. selfish gained only `dynamic_symbols` - reading `.dynsym` is
format knowledge. (D213) The per-firmware
vaddrs stay caller-supplied data - selfish's `dynlib::build` already takes a manifest the same
way. obSCEne becomes a consumer of that path. (D210 sorts what belongs where.)

Status: **hardware** - the general resolver ran on the console, 2026-08-27.

