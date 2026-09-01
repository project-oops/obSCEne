# D155 - One source, as many binaries as the targets need. Single-binary was never the goal and is not a constraint to design around


Standing position, recorded because it was agreed in conversation and would otherwise be
re-litigated by whoever next hits a target that will not fit the others.

The requirement has always been **one source tree** - a probe whose findings can be compared
across platforms because the same checks ran. It is not one artifact. `GEN=4` and `GEN=5`
already produce different binaries from this source, and `host`, `payload` and `deck` are
three more.

That distinction is forced rather than chosen. `EI_ABIVERSION` sits in the ELF header and a
loader reads it *before* executing anything - Kyty's test is literally
`e_ident[EI_ABIVERSION] == 2` and shadPS4 refuses with `expected 0x00 is (0x2)`. A byte the
loader inspects before the guest runs cannot be decided by the guest. Everything after load
is runtime-detected and stays that way: `005-generation` infers the generation from which
graphics driver resolved, and the display takes whichever video-out pair exists, preferring
the older when both appear (D111, D140).

**So the rule is: runtime detection wherever the platform will answer at runtime, a build
target only where it physically cannot.** Nothing today is held back by this, and no split is
being made on spec. What this decision buys is that the next thing which genuinely cannot be
detected at runtime gets its own target immediately, instead of the source being contorted to
keep one artifact working for two consoles.

