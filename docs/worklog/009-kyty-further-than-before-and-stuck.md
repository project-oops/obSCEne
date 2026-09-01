# Kyty: further than before, and stuck somewhere specific


The second emulator now loads the module, maps it, sets up the guest stack and resolves
its libraries. It fails in relocation:

    unknown type: 1213294663

which is four bytes of ASCII from our own string table read as a relocation type.

What has been established, so the next attempt does not start from nothing:

- **The offsets are honoured.** A relocation with type `0xDEADBEEF` planted at a chosen
  offset came back as `unknown type: -559038737`. It reads where we point it.
- **Every relocation we emit is standard**: 899 `RELATIVE`, 386 `R_X86_64_64`, 7
  `GLOB_DAT`, 75 `JUMP_SLOT`. Audited, not assumed.
- **The error survives both tables being disabled.** With `RELASZ` and `PLTRELSZ` both
  zero it still reports a type, read from the record at the start of the segment. So
  the table it is walking is not one we declared.
- **Library resolution succeeds first.** Truncating the dynamic table before the last
  imported library gives `l == nullptr || m == nullptr` instead; the relocation error
  only appears once all fourteen are declared.

Stripping the section headers (D034) removed the ASCII reads here too - the "unknown
type" values are now plausible addresses rather than fragments of symbol names, so it
is reading real entries and misreading their fields.

Where it stops now is the empty jump-slot table, which is a direct consequence of
`-fno-plt`:

    Not implemented (program->dynamic_info->jmprela_table == nullptr)

Pointing `DT_SCE_JMPREL` at the relocations that do exist gets past that and into a
different misread, so its jump-slot handling expects genuine `JUMP_SLOT` entries rather
than the `GLOB_DAT` form `-fno-plt` produces.

That leaves a clean choice, and it was retested rather than assumed: building without
`-fno-plt` gives this emulator real jump-slot relocations, and the other one then
resolves nothing and makes no calls at all. So the flag stays. Getting both to run the
same binary needs the second emulator to accept the GOT form, or a way to emit both
without declaring the same relocations twice - which is exactly the duplication D034
removed.

### A fourth channel, also stubbed

`sceKernelDebugOutText` was the remaining candidate. Tested in a throwaway one-import
module rather than added to the probe, because its signature is the least confident
declaration this repository has and a wrong arity corrupts the stack.

It is stubbed too. Four output functions, all unimplemented in this emulator build:
`sceKernelWrite`, `write`, `putchar`, `sceKernelDebugOutText`.

The experiment was still worth running. The module **ran** - it reached its spin loop,
having called both imports - so the declaration is safe and the format is sound. It is
kept behind `OBSCENE_MIN_DEBUG_OUT`, not wired into the probe: adding a channel that
takes a NUL-terminated string to a report that writes byte ranges would mean buffering,
and buffering is the one thing the announce-before-attempting rule forbids.

**So there is no way to read a report out of this emulator**, and that is a fact about
the emulator. The channel selection picks the first channel that moves a byte and says
which in the `end` record; on a platform that implements any of them it will work
without further change.

### It runs to the end

With the section headers stripped (D034) and the entry point no longer returning
(D035), the probe completes under shadPS4: every section runs, 77 platform calls are
dispatched by NID, and the process ends without a fault. The only `Critical` lines left
in the log are the emulator's own, about system modules it could not preload.

Two things were being read as one. The garbage relocations were real and are fixed; the
crash at address `0x1` was never related to them, and was the exit path all along.

The remaining limitation is unchanged and is not ours: this emulator implements no
write path, so the report cannot be read out of it. Everything needed to read one is in
place the moment a platform provides any of the four channels.

### Final state

    imports resolved : 807
    platform calls   : 77 across 57 distinct functions
    guest faults     : 0

The probe loads, resolves, runs every section, and ends without a fault. It spins at
the end because this emulator has no `exit` either - which is the documented, honest
outcome rather than a return that would be indistinguishable from a crash.

`scripts/verify.sh` runs everything that has to pass: the tool's tests and lints, all
three targets, the tag derivation against the module just built, and the host harness.

