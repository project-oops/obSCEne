# D023 - The suffix is committed and labelled

**byte-order half superseded by D024** · 2026-08-19

`data/hash-suffix.toml` carries the value. `nid.py` defaults to it; `--suffix-file`
overrides.

**Reverses the first half of D022**, which had it supplied at build time and never
committed. The emulator side of this project reached the opposite conclusion with
better reasoning, and it is worth restating rather than merely deferring to:

- It is **not a key**. It decrypts nothing, signs nothing, protects nothing. It salts
  a name-mangling hash so the mapping from names to identifiers is non-obvious.
- Every emulator of this target necessarily contains it, because resolving imports is
  the central act of high-level emulation. Withholding it protects nobody.
- A **labelled** constant with its derivation written beside it is *better* provenance
  than no constant at all. The thing that makes a hex blob awkward to account for is
  the absence of an explanation, not its presence in the tree.

The second half of D022 stands: `nid.py selftest` exercises the encoding against
arbitrary values and needs no real suffix, so the transform stays testable
independently of whether this particular value is right.

### The byte-order claim here was wrong - see D024

This entry originally asserted big-endian on the strength of a prose comment. That is
disproven by a published test vector. The reasoning is left in place because the way
it went wrong is the instructive part.

