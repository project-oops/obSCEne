# D130 - The census is 5,909 symbols. Growing it turned "present" from a weak signal into a measurement of the loader


Status: derived - observed on two loaders.

`scripts/gen-corpus.py` generates `include/obscene/corpus.h` from `data/mined-names.txt`
and `src/sections/surface.c` walks it beside the curated census. 383 symbols to **5,909**,
a fifteen-fold increase, and the module now imports 6,089 symbols from the same 16
libraries.

**The same 16 deliberately.** Every library costs three vendor tags plus a `DT_NEEDED`, and
`link/module.ld` reserves 0x800 for the whole dynamic table - the 620 libraries in the
corpus would need twenty times that. Adding libraries is a separate change with its own
risk; this one adds no loader surface at all, which is why it could be tested in one step.

### What the bigger census immediately showed

| | shadPS4 | PS5PCEM |
|---|---|---|
| present | **5,909** | 797 |
| absent | **0** | 5,112 |

shadPS4 resolves **every symbol asked of it**, including libc internals it cannot possibly
implement. At 383 symbols its 383/383 read as thorough. At 5,909/5,909 it reads as what it
is: resolution that does not consult an implementation.

`COMPATIBILITY.md` has said since it was written that presence is not behaviour and that a
loader resolving everything to a shared stub scores full marks. **The census could not
demonstrate that until it was large enough to be absurd.** A number that only becomes
evidence at scale is worth the scale.

PS5PCEM's 797 is the other shape: it resolves what it implements. Neither number is a score,
and the difference between them is the finding.

### Three filters the corpus needed, each found by a build failing

- **Compiler builtins.** `__atomic_load`, `__sync_fetch_and_add_16` and thirteen others are
  clang intrinsics; declaring one as data is a redefinition. They are in the firmware
  tables legitimately - the runtime does export fallbacks - but only the compiler gets a
  say.
- **Host C library collisions.** `errno` is thread-local in glibc, and the mismatch is a
  *link* error naming no source line. The target build has no libc and would not care; the
  host build is not optional, so the corpus must build in both.
- **Names that are not C identifiers**, which need an assembler label to declare at all.
  None in this slice, and the count is printed rather than silently dropped.

