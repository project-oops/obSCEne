# D017 - The C runtime is a first-class section, and it is where positive checks live

**decided** · 2026-08-19

`035-libc` checks `libSceLibcInternal` for behaviour: `calloc` must return zeroed
memory, `realloc` must preserve contents, `qsort` must actually sort, `snprintf` must
report the length it *would* have written.

**This was the largest omission in the first pass.** A probe was written freestanding
and then the guest was assumed to be freestanding too, which is wrong - a title
imports more of the C library than of any single vendor subsystem. It went unnoticed
because every other library needed research, and this one needed only remembering.

It is also the answer to D007. Everywhere else leans on negative checks because struct
layouts are unknown; here the whole interface is ISO C, so every signature is certain
and the checks can ask whether the function *does the right thing* rather than merely
whether it rejects nonsense. An implementation that fails everything passes every
negative check in the suite and not one of these.

Two details worth keeping:

- **`memcpy`, `memset` and `memmove` are excluded.** This program defines its own,
  because the compiler emits calls to them regardless of `-ffreestanding`. A local
  definition beats a weak import, so a check would have measured our implementation
  and passed - a test that passes by testing itself.
- **`qsort` makes the platform call back into guest code.** Nothing else in the
  program exercises that direction, and an emulator has to get it right.

The host build gains real signal from this too: it links a genuine C library, so these
are the first checks that can pass there for a non-trivial reason.

