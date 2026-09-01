# D026 - The tooling is Rust, in one binary

**decided** · 2026-08-19

`tool/` builds `obscene-tool`. The Python scripts are being replaced by subcommands,
starting with the NID chain.

**The stated reason for scripts had already expired.** The argument for Python was that
obSCEne should need nothing but clang - a real property for a conformance probe, since
one that needs an unusual toolchain is one most people cannot build. But `make check`
already required Python 3 and a POSIX shell, so the property was gone and the scripts
were being defended on grounds that no longer held.

Given that, compiled is better on every axis that matters here:

- **Most of this work is binary formats** - offsets, sizes, packed structures, byte
  order. `struct.pack` format strings and untyped integers are liabilities in exactly
  that work, and the failure mode is a file that is accepted and subtly wrong.
- **It ships as a binary**, so a user needs no runtime at all - strictly better than
  requiring Python.
- **`cargo test` instead of a shell script asserting exit codes.**
- It matches the rest of the ecosystem, so it is maintainable by the person who owns it.

**The deciding argument was sequencing, not cleanliness.** The largest piece -
`mkmodule`, which builds a vendor dynamic segment - is unwritten. The real question was
never "should the existing scripts be ported" but "what language should the next five
hundred lines be in", and porting afterwards is pure waste.

### It does not depend on the emulator

`orbistoun-nid` implements the same algorithm, and this deliberately does not use it.
A probe that depends on the emulator it measures is measuring that emulator's opinion
of itself, and the independence is worth more than the duplication costs.

**The duplication is made safe by a shared fixture, not by avoiding it.** Today's
byte-order confusion happened because there were two implementations and *no test
vector* - one documented big-endian, the other implemented little-endian, and nothing
could tell them apart. Both now check `sceKernelLoadStartModule -> wzvqT4UqKX8`, so
they cannot drift without a test going red. Two implementations with one fixture is a
sound arrangement; two with none is how this went wrong.

### Lints

Borrowed from the emulator side: `clippy::all` denied, `pedantic` on, plus
`arithmetic_side_effects` and `indexing_slicing` - because silent wrapping and
out-of-range indexing are the two ways this particular tool would produce a corrupt
module rather than an error.


