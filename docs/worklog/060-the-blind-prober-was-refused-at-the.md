# 2026-08-24 - the blind prober was refused at the door, twice


Two runs against shadPS4 reported `the run produced no bulk records and no unfinished check;
it did not start`, which reads as a fault in the prober. Neither was.

The first was a real bug: `bulk-sweep.sh` honoured `--run-timeout` on the host path and did
not pass it to `run-emulator.sh` on the module path, so the loader got run-emulator's own
70-second default. `910-bulk` runs last, after every other section, and on a loader that
needs longer the module was killed before the prober started.

The second was the default beside it. `bulk-sweep.sh` defaulted `GEN=5` while `sweep.sh`
defaulted `GENERATION=4` and carried a comment saying exactly why - the two drive the same
loaders. shadPS4 reads `EI_ABIVERSION` before a single guest instruction runs:

```text
elf.cpp:302 IsElfFile: e_ident[EI_ABIVERSION] expected 0x00 is (0x2)
linker.cpp:246 LoadModule: Provided file /app0/eboot.bin is not valid ELF file
```

That is in the loader's own log, which nothing was reading. `run-emulator.sh` now checks for
a refusal whenever a run produced no records and prints the loader's words plus the fix,
because "the loader refused the module" and "the run started and died early" are opposite
outcomes: one has an index to resume past and the other has nothing at all.

With `--generation 4` it walks. `sceKernelStat`, `sceKernelMkdir`, `sceKernelRmdir`,
`sceKernelUnlink`, `sceKernelRename` and `sceKernelMapNamedDirectMemory` all fault on
shadPS4 with an empty argument, each named by its own announcement.

### And a rule for working here

That sweep died at round seven with `the module build failed at index 32`. The build was
fine; **I had edited `tool/src/sections.rs` while the sweep was running against the same
mount**, and it was mid-edit. The VM builds from the live checkout, so a sweep and an edit
are not independent activities. Finish the edit, then start the sweep.

