# D302 - The conformance matrix - one capture per launch shape, named for the shape

**Status**: [decided]
**Date**: 2026-09-03
**Context**: `data/hardware/` had accumulated five files named on four different principles, and a third hardware capture arrived with nowhere obvious to put it.

---

## The problem

The directory named its contents by whatever seemed salient at the time:

| file | named for |
|---|---|
| `ps5-full.txt` | the suite's *scope* |
| `ps5-imports.txt` | a *record type* it added |
| `crashers.txt` | a *finding* |
| `libkernel-vaddrs.txt` | a *symbol class* |
| `ps5-sprx-manifest.tsv` | a *survey* |

Four principles, five files. None of the names says **how the artefact was launched**, which
is the axis that changes the answers: a native title-eboot run answered `sceKernelLoadStartModule`
differently from the module-target runs, and nothing in either filename would have predicted it.

## The convention

A capture is named for the cell it fills, and nothing else identifies it:

```text
obscene-probe-<target>-<mode>-<privilege>-<category>.log
```

| axis | values | from |
|---|---|---|
| target | `hardware` `orbistoun` `shadps4` `fpps4` `host` | who ran it |
| mode | `ps5` `ps4` `elf` | native title, backwards-compatible title, raw payload |
| privilege | `app` `sysmodule` `system` `root` | the authority id in the SELF header (D301) |
| category | `bigapp` `systemapp` `miniapp` `none` | `applicationCategoryType` (D301) |

`obscene-tool matrix` reads the directory and parses the names. **The directory is the matrix**,
so a cell nobody added is a cell nobody can forget to pass on a command line - which is the
difference from `consensus`, where the caller supplies `name=path` pairs by hand.

### Category is an axis because D301 measured it being one

The obvious scheme has three axes and files by privilege alone. That would be wrong, and this
project's own measurement from the same day says so: privilege tier and application category
are **independent**, and the one governing direct memory and display is *category*. At the same
`root` privilege, category 0 got full Big App resources while category 65536 got
**zero bytes of direct memory** and `sceVideoOutOpen` refusing `0x80290001`.

Filing those two runs in one cell would make every memory and display measurement read as a
disagreement with itself, and the report would blame whichever target was named second.

### `none` is a category, and not a default

A raw ELF carries no `param.json`, so it has no `applicationCategoryType`. Recording one would
be inventing a fact about a file that does not have the field, which is principle 2.

## Latest per cell, overwritten in place

A capture is **not** accumulated with a date suffix. The history of a cell is the file's
history, and reconstructing `git log` by hand in a directory listing is strictly worse than
using it.

The argument against - that the *spread across runs* is the finding, since only disagreement
distinguishes a constant from a per-boot calibration - is real but does not need dated files:
each cell is a different run, so the spread is visible **across the matrix** as well as across
time. And where a value genuinely varies between two runs of the same cell, `git log -p` on
that one file is a better record than two files whose relationship is implied by their names.

## Hardware is the authority, not a vote

`consensus.rs` opens by stating that *nothing in this suite carries hardware provenance, and
until a console is available nothing will*, and builds a substitute oracle out of
majority-agreement between emulators. That was correct when it was written.

**A console answers now.** So where a `hardware` cell is present, it settles the question and
the other cells are measured against it - an emulator disagreeing with three other emulators is
interesting, and an emulator disagreeing with the console is simply wrong. Where no hardware
cell is present, disagreement is reported as **unsettled** rather than blamed on anybody, and
the report says so in as many words.

`consensus` is not replaced: majority agreement is still the right tool for the shapes no
console has run.

## Missing cells come from evidence, not from the cross-product

The full product is 5 x 3 x 4 x 4 = 240 cells, almost all meaningless - a raw ELF has no
category, nothing runs a mini app as root. So a gap is defined as **a shape somebody has
already produced that another target lacks**: if hardware was captured as `ps5/root/bigapp`,
an emulator that never ran that shape is a real gap; a shape nobody has run anywhere is not.

Reported because a matrix showing only the cells it holds looks complete when it is two runs
out of twelve.

## What is not settled

**The existing five files are not migrated by this decision.** Two of them are the awkward
case: `ps5-full.txt` and `ps5-imports.txt` are both `module`-target hardware runs at the same
privilege, so they are one cell under this scheme - but they are not the same run repeated,
they are different *suite configurations* (one emits `import` records, the other does not).
The right fix is for the probe to emit those records unconditionally, at which point they
collapse to one cell honestly rather than by discarding a difference.

The other three - `crashers.txt`, `libkernel-vaddrs.txt`, `ps5-sprx-manifest.tsv` - are
**derived findings, not runs**, and want a separate home rather than a cell.

Neither is done here, because both discard or move committed evidence and that is a decision
for the person whose console produced it.

**Also unresolved:** the two older captures carry no `OBS|context` record at all, only
`OBS|build`. So there is nothing in them to derive a mode or a category from, and the probe
should start emitting `context` unconditionally before anything tries.
