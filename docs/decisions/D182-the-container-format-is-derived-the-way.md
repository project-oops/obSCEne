# D182 - The container format is derived the way NIDs already are: binaries outside, provenance-headed data inside


The question was whether reading a package to build a package writer crosses principle 6 - *no
code written while reading vendor binaries*. It does, if the reading and the writing are the
same act. The NID chain already solved this, and the solution was sitting in the repository
unrecognised.

`obscene-tool mine` takes `--firmware` pointing at extracted trees **outside** the repository,
and writes `data/mined-names.txt` with this at the top:

```
# mined-from: ChonkyStation4@d181f4d GPCS4@88480de PS5PCEM@f87d2a8 SharpEMU@a2241d0
#             craziiEmu@8a13647 fpPS4@04cefd4 ps4_module_loader@657f9d3 shadPS4@be21649
# firmware: 1.05,1.06,1.76,2.00,2.57,...,9.00,misc
```

and `data/unnamed-nids.txt` states its source outright: *"Identifiers observed in firmware
modules with no known name."* So the project has been deriving from vendor binaries since
early on, under an arrangement nobody wrote down as a policy:

- the binaries are never tracked, and the build never reads them;
- what enters the repository is **text with a header naming exactly what it came from**;
- the derivation is re-runnable by anyone holding the same inputs.

That header is the proof. Not a promise about what was read - a statement checkable by someone
who has the inputs and can re-run the miner, and falsifiable by someone who does and gets
something else.

### The wrinkle, and why it is not one

Mining NIDs extracts a **table**. A package builder is **code**, and code shaped by reading a
binary converges on that binary - which is the thing the rule is actually about.

It resolves through this repository's own architecture rather than by argument. `CLAUDE.md`
already says of `data/`:

> the source of truth for everything generated … These are data - judgements with prose
> attached - and they live outside the generators deliberately: retyping a judgement into a
> new language is how a transcription error gets into a census.

So the container format is **written down as data**, not as code. A field table - offset, size,
meaning, and where each entry was established - lives in `data/` with a provenance header.
`mkpkg` reads that table. The generator never opens a package, exactly as `census` never opens
a firmware module.

This is also what `mkmodule` did, arrived at from the other direction: *"every fixup here came
from a rejection, not a specification"*. The knowledge was recorded, then the tool was written
from the record.

### The chain

```
titles/                    gitignored. Derivation input and conformance oracle. Never read by a build.
data/pkg-format.tsv        the field table, provenance header naming each source by title id and hash
obscene-tool pkgmine       refreshes the table from titles/
obscene-tool mkpkg         builds a package from the table - never reads titles/
obscene-tool pkgcheck      compares an output against a real package: pass or fail, nothing else
```

`pkgcheck` cannot run in CI, because 43-81MB of packages are not a repository fixture and the
provenance job refuses tracked binaries outright. So the package chain is verified locally and
the documentation has to say so, or the gap becomes invisible - which is the failure this
project keeps meeting from other directions.

Gated on `mkself` regardless: a package wraps an `eboot.bin` rather than replacing one. (D180)

Status: **decided** for the arrangement; `pkgmine`, `mkpkg` and `pkgcheck` are unimplemented.

