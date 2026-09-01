# D022 - Symbol encoding is a build option, and the suffix is never committed

**partly superseded by D023** · 2026-08-19

`tools/nid.py` computes the hash a real loader resolves by, encodes it, and generates a
header of `__asm__` labels renaming every import to the encoded form.

A real loader does not resolve by name. It resolves by a hash of the name, and the
dynamic symbol table carries that hash already encoded. This program emits plain names,
which suits the host build and a loader that hashes names itself, and will not resolve
on anything behaving like real hardware.

**Two properties matter:**

- **The checks never change between builds.** Renaming happens on the declarations
  via `__asm__` labels, so the same C source produces either form. A probe whose
  checks differ between builds is measuring two different things.
- **The suffix is an input, never a constant.** It is not in this repository and will
  not be, matching how the emulator side treats it. Everything works with any suffix,
  which is what makes the encoding testable without the real one.

`nid.py selftest` pins the transform with no suffix at all: eleven characters of a
64-symbol alphabet carrying 66 bits with two of padding, reversible, injective, and
dependent on both name and suffix. A wrong alphabet or bit order produces names that
look entirely plausible and resolve to nothing, which is worth catching before it is
blamed on a loader.

