# D111 - One source, both generations: the display takes whichever video-out pair resolved


Status: derived - two independent implementations agree on the signatures, and the result
was confirmed on a loader of each generation.

The two console generations do not prefer different video-out entry points, they expose
**different ones**, and neither exposes the other's:

| | plain | `2` form |
|---|---|---|
| shadPS4 (previous) | yes | no |
| PS5PCEM (current) | no | yes |
| SharpEMU | yes | yes, both tagged `Gen4 \| Gen5` |

So a single module has to carry both and choose at run time. It chooses on **whether the
symbol resolved**, not on the detected generation: a weak import that came back null has
answered directly, where `005-generation` would infer it from other symbols and would be
wrong on a platform offering both (D110).

### The signatures are corroborated, which is what makes calling them allowed

D008 forbids calling a function whose argument shape is uncertain, and a framebuffer
descriptor is the worst possible place to be wrong. Two implementations sharing no code
agree exactly:

- **PS5PCEM**, a Zig signature naming each parameter in order;
- **SharpEMU**, register by register - `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, then the
  stack - and field by field into the structure.

Both give the same eight arguments in the same order, the same 0x50-byte attribute
structure with tiling at 0x04, width at 0x0C, height at 0x10, option at 0x18, pixel format
at 0x20, and the same 0x20-byte buffer descriptor with the address first.

The attribute structure is still passed as an opaque buffer, exactly as the previous
generation's is. The buffer *descriptor* is declared, because this program fills that one -
and a wrong layout there hands the display our stack as a pointer.

### The second fault, which the report had already answered

With the symbols found, the display then refused with "no initial user to open an output
for" - while the same run reported `070-user/initial-user pass 0x10000000` two hundred
lines later.

The display runs **before any section**, as the program's first platform interaction, so it
was asking an uninitialised user service. One loader answered anyway; the other refused.
Initialising before use is correct on every platform, so the fix is not a concession to the
strict loader - it is the order the interface documents, which the lenient one let this
program get away with ignoring.

### Result

`display|ready|1920x1080 framebuffer` on a current-generation loader, and the full report
drawn on screen: 26 of 26 sections, 499 of 499 checks, `REPORT COMPLETE`, generation
correctly read as current.

**No emulator-specific code was added.** The probe still branches only on which symbols
resolved, the generation marker, and explicit build flags - every mention of an emulator in
`src/` is a comment explaining why a decision was made.

