# Glossary

The words obSCEne uses for its own machinery. For the vocabulary the whole collection shares -
standard ELF, and the words that mean different things in different repositories - see
[the collection's glossary](https://github.com/project-oops/OOPS/blob/main/docs/GLOSSARY.md).
For the file formats, see [SELFish's](https://github.com/project-oops/SELFish/blob/main/docs/GLOSSARY.md).

Where a decision settles a term, the decision is named. Where a contract document defines it,
that document is named and this page does not restate it.

## Checks and reports

**Check** - one question asked of a loader, with a verdict. A check has an id of the form
`<section-id>/<slug>`, unique across the program. The id is the key when two runs are diffed,
so it is treated as a public API.

**Section** - a group of related checks. Sections run from base to high level, so a failure at
the top is read before the failures below that depend on it.

**Skipped, not failed** - a check whose prerequisites did not hold is skipped, so one broken
allocator does not turn everything below it red. A check whose symbol the loader could not
resolve is also skipped: every platform declaration is weak, so an absent function is skipped
rather than jumped to.

**Status** - `pass`, `partial`, `fail`, `skip`. `partial` exists so that an implementation
returning zero for everything does not look perfect.

[OUTPUT.md](OUTPUT.md) is the contract for all of this, including the record types and the
provenance grades (`assumed` through `hardware`) that say how much a verdict is worth.

## Platform knowledge

**Census** - the list of platform symbols obSCEne knows exist. A broad census makes presence a
measurement of the loader itself. A census entry is presence only: it says a name
resolves, not that calling it does anything sensible.

**Corpus** - the mined name-to-NID pairs. It is a measurement product stored as data, so it
lives here and not in SELFish, which holds format facts.

**Sweep** - a repeated run that narrows something down, usually by excluding what hangs and
running again until the remaining set completes.

**Presence and behaviour** - two separate questions, asked separately. A symbol can resolve and
still do nothing.

## The program and its shapes

**Probe** - obSCEne itself. Also the first word of three of its artifact names.

**Shape** - one of the forms the probe is built in. Shapes are told apart by two bytes at
offset 16, and sending the wrong one to hardware takes a loader down.
[ARTIFACTS.md](ARTIFACTS.md) is the authority; read it before sending anything anywhere.

**Harness** - the part that runs the checks in order and emits records. Not the checks
themselves.

**Sink** - where a report goes. There is more than one, so a run that reaches the end and a run
that dies partway can be told apart: the net sink connects back over a socket, and the system
log catches what a crash leaves behind.

**Host build** - the probe compiled for an ordinary machine against stubs, so the harness runs
without hardware. The expected outcome is a full sheet of red in the right order with the
dependency skips in the right places; anything else is a fault in the program rather than in
what it measures.

**Injector** - a separate program, not a shape of the probe. It takes over a live
native-category process and runs the probe inside it. See [INJECTOR.md](INJECTOR.md).

## Execution context

The context decides what a result is worth.

| Context | What it means |
|---|---|
| **`ps4_mode`** | Running inside the previous generation's compatibility sandbox. The current-generation graphics driver cannot be mapped, dynamic introspection is refused, and `DT_DEBUG` is absent. A plain payload loaded the ordinary way lands here, and so does the package, which installs as a `ps4_game` title |
| **native** | Running as, or inside, a current-generation title. The title directory launches here; the injector reaches it by taking over a process that is already running |

`ps4_mode` is its own answer rather than `unknown` or `gen4`, which would misidentify it.

## Guards

**Blind prober** / `BULK` - a mode that calls censused symbols indiscriminately. Useful for
mining and dangerous on hardware; the build refuses to produce it together with `HARDWARE=1`.
CI tries that combination and requires the build to fail, then greps the shipped artifact to
confirm the mode is absent.

**Fenced** - a region marked `/* clang-format off */`. Used for the generated census lists
and for casts the formatter's versions disagree about. The marker is bare: anything
appended to it makes it an ordinary comment that fences nothing.

## Other glossaries

- [the collection's glossary](https://github.com/project-oops/OOPS/blob/main/docs/GLOSSARY.md) - standard ELF, `DT_`/`PT_`, and the cross-repository word collisions
- [SELFish](https://github.com/project-oops/SELFish/blob/main/docs/GLOSSARY.md) - NID, fSELF, PFS, packages, the generation split
- [orbistoun](https://github.com/project-oops/Orbistoun/blob/main/docs/GLOSSARY.md) - guest execution, thunks, HLE
- [Prosperous](https://github.com/project-oops/Prosperous/blob/main/docs/GLOSSARY.md) - targets, chains, scan roots
