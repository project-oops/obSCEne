# 2026-09-03 - The conformance matrix, and why the filename is the metadata

```
tool tests   209  ->  220
```

`data/hardware/` had five files named on four different principles - a scope (`ps5-full`), a
record type (`ps5-imports`), a finding (`crashers`), a symbol class (`libkernel-vaddrs`) - and a
third hardware capture arrived with nowhere obvious to put it.

**None of those names says how the artefact was launched**, which is the axis that changes the
answers. The native title-eboot run answered `sceKernelLoadStartModule` differently from the
module-target runs, and nothing in either filename would have predicted it.

## The convention (D302)

```text
obscene-probe-<target>-<mode>-<privilege>-<category>.log
```

`hardware|orbistoun|shadps4|fpps4|host` × `ps5|ps4|elf` × `app|sysmodule|system|root` ×
`bigapp|systemapp|miniapp|none`.

`obscene-tool matrix` reads a directory and parses the names, so **the directory is the
matrix** - a cell nobody added is a cell nobody can forget to pass on a command line. That is
the difference from `consensus`, which takes `name=path` pairs from the caller.

### Category is a fourth axis because D301 measured it being one

The obvious scheme files by privilege alone. **D301, from this same day, says that is wrong**:
privilege tier and application category are independent, and the one governing direct memory
and display is *category*. At the same `root` privilege, category 0 got full Big App resources
while category 65536 got **zero bytes of direct memory** and `sceVideoOutOpen` refusing
`0x80290001`.

One cell for those two runs would make every memory and display measurement read as a
disagreement with itself.

## Two premises had moved, and both were written down as permanent

`consensus.rs` opens with *"Nothing in this suite carries hardware provenance, and until a
console is available nothing will"*, and builds a substitute oracle from majority agreement.
That was true when written and is not now.

So the matrix treats **hardware as the authority** rather than as a vote: an emulator
disagreeing with three other emulators is interesting, an emulator disagreeing with the console
is simply wrong. Where no hardware cell exists, disagreement is reported as **unsettled** and
the report says so rather than picking a winner.

`diff.rs` carries the other one - it *refuses* cross-target comparison, because two targets run
the same checks through different loaders. Right for a regression gate; the matrix exists
precisely to make that comparison meaningful by labelling each side with the shape it ran in.

Neither module is replaced. Majority agreement is still right for shapes no console has run.

## Gaps come from evidence, not from the cross-product

The full product is 240 cells, almost all meaningless - a raw ELF has no category, nothing runs
a mini app as root. So a gap is **a shape somebody has already produced that another target
lacks**. If hardware was captured as `ps5/root/bigapp`, an emulator that never ran it is a real
gap; a shape nobody has run anywhere is not.

Reported because a matrix showing only what it holds looks complete when it is two runs out of
twelve.

## What it says today, which is honestly nothing

```text
cells (0)

not named as captures (6)
  README.md: not a `.log`
  ps5-full.txt: not a `.log`
  ...

no captures - nothing was compared
```

**A file that does not parse is listed, never skipped.** A matrix that quietly omitted one
would report agreement it had not checked, which is the failure this suite exists to avoid.

Pointed at the native run under its proper name it reads it, marks it the authority, and finds
nothing to compare it against - which is the true state.

## Not done, deliberately

`ps5-full.txt` and `ps5-imports.txt` are both module-target hardware runs at the same privilege,
so they are **one cell** under this scheme - but they are not the same run twice, they are
different suite configurations (one emits `import` records). The honest fix is for the probe to
emit those unconditionally, at which point they collapse for a real reason rather than by
discarding a difference. `crashers.txt`, `libkernel-vaddrs.txt` and `ps5-sprx-manifest.tsv` are
derived findings rather than runs and want a separate home.

Both move committed evidence, so neither is done here.

**And the two older captures carry no `OBS|context` record at all**, only `OBS|build` - so there
is nothing in them to derive a mode or category from. The probe should emit `context`
unconditionally before anything tries to migrate them.

## Two things found on the way

- **The lint gate is red, and was before this.** Ten pre-existing errors: `doccheck.rs`
  arithmetic, `run` at 108 lines against a 100 limit, a redundant closure, four missing
  backticks and three collapsible ifs. `matrix.rs` is clean; adding one dispatch arm took `run`
  from 108 to 109.
- **`obscene-tool` overflows its stack on Windows**, even for `--help`. The clap tree across
  thirty-odd subcommands exceeds the 1 MB default main-thread stack of a Windows debug build;
  under WSL's 8 MB it is fine. Not new and not caused here, but worth knowing before anybody
  debugs a Windows run of the tool.

## State

`cargo test` in `tool/` - **220 passed**, 0 failed. `cargo fmt --check` clean. The 11 new tests
cover name parsing in both directions, the skip-is-not-a-disagreement rule, unsettled-without-
authority, and both halves of the gap rule.
