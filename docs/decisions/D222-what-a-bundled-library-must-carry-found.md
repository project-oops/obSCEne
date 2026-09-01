# D222 - What a bundled library must carry, found one refusal at a time


Four, each named exactly by the loader, in the order they appeared:

- **Header placement.** `verify_ehdr: offset 0x0 end 0x190` - the first segment started
  underneath the program header table. That is selfish D078 and a third linker script.
- **`EI_ABIVERSION`.** `### ERROR: ABIVERSION mismatch ... val 2`. A bundled library and the
  eboot that loads it must agree, and `sce-module` was taking the defaults while `eboot-min`
  was not.
- **`DT_SCE_ORIGINAL_FILENAME`.** `preprocess_dt_entries:9600: C: orig fn 0  mod info 1` - one
  module-info tag, zero filename tags. selfish D079.
- **The procedure-linkage tables.** `preprocess_dt_entries:9632: C: ah 1 pg 1 jr 0 pr 1 prs 0
  rl 1`. The stub referenced `sceKernelWrite` by address, which is a `GLOB_DAT` relocation and
  not a `JUMP_SLOT`, so `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were absent. This loader requires
  the full set whether or not a module has anything to relocate, which D218 had already found
  for the eboot and which generalises. The stub now has a call behind a condition the system
  will never satisfy - `argc == ~0UL` - purely so the tables exist.

Every one of these was a single build and a single launch, because the loader named the field.
That is worth saying plainly after D075, where an error naming a structure sent the work in the
wrong direction for a long stretch: this loader is *usually* precise, and the two red herrings
so far (`Failed to load SCE_DYNLIBDATA`, `Unsupported ELF e_type` on a `.prx`) were both cases
where the message came from a layer above the one that failed.

