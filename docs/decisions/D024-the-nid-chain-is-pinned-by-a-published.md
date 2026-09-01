# D024 - The NID chain is pinned by a published test vector, not by prose

**decided** · 2026-08-19

`nid.py selftest` checks a published (name, identifier) pair:

    sceKernelLoadStartModule  ->  wzvqT4UqKX8

Hashing that published name with the committed suffix and encoding the result
reproduces that published identifier. **The byte order is little-endian.**

**Written because the same mistake was made twice in one session, in both directions.**
`nid.py` was first written little-endian on reasoning. It was then changed to
big-endian because `hash-suffix.toml` said so in a confident comment claiming
verification against a real executable. It was then changed back, because a published
pair settled it in seconds and matched only little-endian.

The lesson is not about endianness. **Four independent things - suffix, byte order,
alphabet, bit packing - are each individually plausible when wrong, and each produces
eleven ordinary-looking characters that resolve to nothing.** No amount of reading
distinguishes a correct chain from a wrong one; one real pair distinguishes all four
at once. Prose describing a transform is not evidence about the transform.

It also makes the constant's provenance self-supporting. Anyone can re-derive the
check from a published name, a published identifier, and SHA-1, with nothing from a
console involved - which is a stronger position than a citation to a forum post, and
does not depend on that post still existing.

**One pair is enough to catch this class of error and is not enough to be
comfortable.** A second, from a different library, would turn a strong signal into a
redundant one.

