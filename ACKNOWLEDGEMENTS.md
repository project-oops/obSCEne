# Acknowledgements

Recording what was consulted is better hygiene than silence, because it keeps the
question answerable. Nothing here has been copied.

## The rule

**Facts about an interface, not anyone's expression.** Reading public interface
documentation, published design notes, an open-source toolchain's headers, or an
emulator's source to learn *what a format is* is ordinary engineering. Taking an
implementation is not, and this repository does not do it.

The practical distinction this project draws:

| | Position |
|---|---|
| **Symbol and library names** | Facts about an interface. A name is what the loader hashes; there is no other name that works. Used freely. |
| **Function signatures** | Interface facts. Needed to call anything at all. Used where confident, omitted where not (D008). |
| **Format constants** | Interface facts. A dynamic-table tag has one correct value, and reading it in a loader that has it right is how one was found to be wrong here (D038). |
| **Struct layouts** | Interface facts, but far easier to get subtly wrong. Omitted entirely so far - every check is written to avoid needing one. |
| **Implementation** | Never. Not adapted, not paraphrased, not carried across in any form. |

### A correction to an earlier version of this file

It said this project does not read emulator source. That was written when it was true and
it stopped being true, which is the ordinary way a provenance document goes wrong.

`DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were swapped in this project's tag table for weeks.
No amount of testing could have found it: the derivation tool checks that
`JMPREL + PLTRELSZ == RELA`, and addition is commutative, so a swapped pair satisfies the
identity exactly. It was found by reading a loader that had the constants the right way
round, and confirmed against a second one (D038).

Fourteen emulators and reference projects are now kept locally and read for exactly that
- see `docs/EMULATORS.md`. Several are GPL, and this program calls the platform's own
interface, so it has no need of their code and must not acquire any. What is taken is
what a format *is* and what a symbol is *called*.

Saying so plainly is the point of this file. A stricter rule that the work does not
actually follow protects nobody.

## References used

**The POSIX specification and FreeBSD documentation.** The target kernel is
FreeBSD-derived and much of its C library is POSIX with vendor naming. For the whole
`sceKernel*` file, memory and threading surface, and all of `scePthread*`, the
published specification is the reference - lawful, citable, and the strongest oracle
available for what these functions are supposed to do. This is where the majority of
the signatures in `include/obscene/platform.h` come from.

**The ELF specification.** For `obscene-tool imports`, which reads a built object's
dynamic symbol table directly rather than requiring binutils.

**Open-source emulators for the target platform.** Fourteen projects, listed in
`docs/EMULATORS.md`, kept locally and read for format constants and for which functions
each implements. Where two disagree about a constant, one is wrong, and knowing which is
worth more than any test this project can run against itself.

### The symbol databases, and how they relate

Two of the sources here look like separate corroboration and are not, so the relationship
is set out rather than left to be assumed.

**`ps4libdoc`** (`idc/ps4libdoc`) is the root. It is not a list somebody compiled: it
publishes symbol tables extracted from real hardware firmware, one branch per system
version from 1.05 to 9.00, as JSON per executable. Each symbol carries the identifier
**observed in the binary**, plus a name where one has been recovered - and recovering a
name means hashing candidates until one matches an observed identifier. Its `doc` branch
holds two summary files: 42,010 recovered names and 1,130,742 identifiers still unnamed.

**fpPS4's `ps4libdoc.pas`** is derived from it. The unit carries 78,372 identifier-to-name
pairs, of which 42,009 of ps4libdoc's 42,010 names are present, plus 36,363 more from
elsewhere. Named after its source, and a superset of it.

So the count of independent confirmations for this project's hash is **two, not three**:
the published test vector, and this database read through two files. That distinction was
got wrong once and corrected in D064; it is written here so it is not got wrong again.

**What the comparison establishes.** Hashing all 78,372 names reproduces all 78,372
identifiers. Since those identifiers were observed in firmware rather than computed, that
is agreement with real hardware values across a large sample, transitively - and it rules
out every byte-order, alphabet and digest-truncation mistake at once.

**Neither is copied.** Both are read where they live, under `<emulators>\src`. The
rule for the emulator sources applies unchanged: take facts, vendor nothing.
`data/nid-corpus.txt` holds only the 389 pairs this project harvested itself.

**A data artefact.** The first line of `known_names.txt` carries a byte-order mark, so a
reader that does not strip it sees `\ufeff__absvdi2` rather than `__absvdi2`. That is the
whole of the "42,009 of 42,010" discrepancy above.

**Open-source homebrew toolchains for the target platform.** Consulted as published
interface documentation for names and signatures that have no POSIX analogue - the
video, audio, controller, and system-service layers. Their headers describe an ABI
this project has no other way to learn; no source was read and nothing was copied.

## The NID hash constant

`data/hash-suffix.toml` carries a sixteen-byte constant used to salt the symbol-name
hash. Its provenance is worth setting out properly, because a constant with no
explanation is exactly the artefact that makes these questions awkward.

**What it is.** A salt on a name-mangling hash. It decrypts nothing, signs nothing and
authenticates nothing; its only effect is to make the mapping from names to
identifiers non-obvious. One constant, not per-title or per-firmware.

**Where it came from - sources, honestly characterised.**

| Source | What it is | Weight |
|---|---|---|
| [PS4 NIDs added to a hashcat bruteforcer](https://www.psxhax.com/threads/ps4-nids-and-ps3-nids-added-to-hashcat-bruteforcer-by-jarveson.2006/) (Jarveson) | Community fork of hashcat, states the constant and the algorithm | Moderate |
| [NID to function name resolver](https://www.psxhax.com/threads/ps4-nid-to-function-name-resolver-for-bin-lib-by-zer0xff.3222/) (Zer0xFF) | Community tool implementing the same scheme | Moderate |
| [Name2NID IDA plugin](https://www.psxhax.com/threads/ps4-name-2-nid-plugin-for-ida-7-0-7-2-released-by-socraticbliss.7073/) (SocraticBliss) | Community plugin, same scheme | Moderate |
| [More PS4 NIDs documented](https://www.psxhax.com/threads/more-playstation-4-nids-documented-for-ps4-devs-by-zil0g80-z80.2760/) (ZiL0G80) | Community NID listings | Moderate |

**These are community forum posts and community tools, not a specification.** There is
no vendor document, no standards body, and no formal wiki page this project was able to
retrieve - psdevwiki very likely carries it and returns 403 to automated fetches, so it
is not cited here rather than cited unverified.

Two corrections to an earlier version of this file. It said "a hashcat mode", implying
upstream hashcat; it is a **community fork**, not an upstream mode number. It also said
"publicly documented", which overstated forum posts and tool source as documentation.

The value was not extracted from any binary by this project. What its origin is beyond
those posts - who first recovered it and how - is **not known to this project**, and
saying so is more useful than implying a provenance chain that has not been traced.

**Why the weak sourcing does not sink it.** Every source above is a community post, any
of which could vanish or be wrong. So the constant is not trusted because of them - it
is trusted because it **verifies itself against public data**. A published pair exists -

    sceKernelLoadStartModule  ->  wzvqT4UqKX8

- and hashing that published name with this constant reproduces that published
identifier. Nothing else does: a wrong suffix, byte order, alphabet or bit packing each
yields a different string. `obscene-tool selftest` runs the check.

Reproduction needs a published name, a published identifier, and SHA-1. Nothing from a
hardware, and no dependence on any particular source still existing - which is a stronger
position than a citation, and the reason this is recorded as evidence rather than as an
attribution.

## The OpenOrbis PS4 toolchain

Downloaded, used out of repository, and deleted. Nothing from it is in this tree.

**What it was used for.** Building its own `hello_world` sample and reading the binary
that came out. That binary runs under an emulator, which made it the first known-good
reference this project had - every failure until then was ambiguous between "our module
is wrong" and "this emulator cannot run anything here".

**What was taken.** Facts about the format, and only facts:

- The eight remaining `DT_SCE_*` tag values, derived from the sample's dynamic table.
- `e_type` is `0xFE10` for an executable, not the `0xFE18` we had been writing.
- A module also carries `PT_SCE_RELRO`, `PT_INTERP` and `PT_TLS`.

**How, and why that matters.** By running `obscene-tool inspect` on the output and
reasoning about the values - not by reading the toolchain's source, its linker script,
or its stub libraries. Three tags were fixed by their values alone (a relocation form
constant, an entry size, an address outside the segment) and the other five by
arithmetic: the tables sit end to end, so `JMPREL + PLTRELSZ` lands exactly on `RELA`,
and `RELA + RELASZ` lands exactly on `HASH`. Two independent adjacencies, both exact.

That distinction is the whole point. A tag value copied from someone's parser is an
assertion; the same value derived from a binary and confirmed by arithmetic is a
measurement, and anyone can repeat it: download the toolchain, build the sample, inspect
the result. The derivation is recorded beside the constants in SELFish, at
`crates/selfish-elf/src/dynlib.rs` - it left this repository with the rest of the format
work (D200), and the reasoning went with it rather than being restated here.

## If you add a reference

Add it here in the same change. If you cannot say where a declaration came from, that
is the signal that it should not be there.

## Open-source emulators, consulted for loader behaviour

Two open-source emulators were read to find out why they rejected a module this project
built. Principle 6 names open-source toolchains as a sanctioned source, and a loader is
the only place the answer exists - a rejection tells you that something is wrong, never
what.

- **KytyPS5** (<https://github.com/KytyPS5/KytyPS5>), at the commit its own build banner
  reported. an emulator's own `loader/elf.h` gave the vendor dynamic tag numbers, which showed that
  two of ours were swapped (D036); `src/loader/runtimeLinker.cpp` gave the relocation
  types it accepts and how it treats an unresolved weak import; `src/libs/libC.cpp` and
  `src/libs/libKernel.cpp` gave which output functions it implements and where each one
  sends its bytes (D037).

Nothing was copied. What came back is a handful of integers, a list of which functions
exist, and an understanding of two failure paths - all of it re-implemented here from
the understanding rather than transcribed.

The derivation in `docs/MODULE-FORMAT.md` stands on its own evidence and is not replaced
by this. It produced nineteen correct tag values from arithmetic on a reference module,
and two wrong ones with identical confidence, because the property that separates an
offset from a size in a commutative sum is not observable from outside. Both halves are
worth keeping: derive what you can check, and read the thing that already knows when you
cannot.
