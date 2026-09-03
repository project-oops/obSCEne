# Glossary

The words obSCEne uses for its own machinery. For the vocabulary the whole collection shares -
standard ELF, and the words that mean different things in different repositories - see
[the collection's glossary](https://github.com/project-oops/OOPS/blob/main/docs/GLOSSARY.md).
For the file formats, see [SELFish's](https://github.com/project-oops/SELFish/blob/main/docs/GLOSSARY.md).

Where a term is settled by a decision, the decision is named. Where it is defined by a document
that is a contract, that document is named and this page does not restate it.

## The unit of work

**Check** - one question asked of a loader, with a verdict. The unit everything else is
counted in. A check has an id of the form `<section-id>/<slug>`, unique across the program,
which is the key when two runs are diffed - so it is never renamed casually.

**Section** - a group of related checks, and the ordering is the whole value of the report:
sections run base to high level, so a failure at the top is read before a failure at the
bottom, because the bottom depends on the top.

**Skipped, not failed** - a check whose prerequisites did not hold is skipped. Without that,
one broken allocator turns everything below it red and buries the one real fault. The same
applies to a check whose symbol the loader could not resolve: every platform declaration is
weak, so an absent function is skipped rather than jumped to.

**Status** - `pass`, `partial`, `fail`, `skip`. `partial` exists because an implementation
returning zero for everything would otherwise look perfect.

*[OUTPUT.md](OUTPUT.md) is the contract for all of this, including the record types and the
provenance grades (`assumed` through `hardware`) that say how much a verdict is worth.*

## What it knows about the platform

**Census** - the list of platform symbols obSCEne knows exist. Growing it is what turned
"present" from a weak signal into a measurement of the loader itself (D130). A census entry is
presence only: it says a name resolves, not that calling it does anything sensible.

**Corpus** - the mined name-to-NID pairs. A measurement product that happens to be stored as
data, which is why it lives here and not in SELFish: SELFish holds format facts, and a mined
list of a million identifiers is not one.

**Sweep** - a repeated run that narrows something down, usually by excluding what hangs and
running again until the remaining set completes.

**Presence against behaviour** - two separate questions, asked separately and never conflated.
A symbol can resolve and still do nothing.

## The program, and its shapes

**Probe** - obSCEne itself. Also the first word of three of its artifacts.

**Shape** - one of the forms the probe is built in. They are told apart by two bytes at offset
16, and sending the wrong one to real hardware has taken a loader down and cost a reboot.
[ARTIFACTS.md](ARTIFACTS.md) is the authority; read it before sending anything anywhere.

**Harness** - the part that runs the checks in order and emits records. Not the checks
themselves.

**Sink** - where a report goes. There is more than one, because a run that reaches the end and
a run that dies partway have to be told apart: the net sink connects back over a socket, the
system log catches what a crash leaves behind.

**Host build** - the probe compiled for an ordinary machine against stubs, so the harness can
be exercised without hardware. The expected outcome is a full sheet of red in the right order
with the dependency skips in the right places; if that is not what appears, the fault is in the
program rather than in anything it measures.

**Injector** - a different program, not a shape of the probe. It takes over a live
native-category process and runs the probe inside it. See [INJECTOR.md](INJECTOR.md).

## Where the probe is running

This distinction decides what a result is worth, and it is the one most easily got wrong.

| Context | What it means |
|---|---|
| **`ps4_mode`** | Running inside the previous generation's compatibility sandbox. The current-generation graphics driver cannot be mapped, dynamic introspection is refused, and `DT_DEBUG` is absent. A plain payload loaded the ordinary way lands here (D276), and so does the package, which installs as a `ps4_game` title (D255) |
| **native** | Running as, or inside, a current-generation title. The title directory launches here; the injector reaches it by taking over a process that is already running |

Reporting `unknown` or `gen4` for the first case misidentifies it, which is why `ps4_mode` is
its own answer (D255).

## Guards and gates

**Blind prober** / `BULK` - a mode that calls censused symbols indiscriminately. Useful for
mining, dangerous on hardware, and the build refuses to produce it together with `HARDWARE=1`.
CI asserts that refusal by trying it and requiring the build to fail, then greps the shipped
artifact to prove the mode is not in it.

**Fenced** - a region marked `/* clang-format off */`. Used for the generated census lists
(D016) and for casts the formatter's versions disagree about. The marker has to be bare:
anything appended to it and it is an ordinary comment that fences nothing.

## Where the rest is

- [the collection's glossary](https://github.com/project-oops/OOPS/blob/main/docs/GLOSSARY.md) - standard ELF, `DT_`/`PT_`, and the cross-repository word collisions
- [SELFish](https://github.com/project-oops/SELFish/blob/main/docs/GLOSSARY.md) - NID, fSELF, PFS, packages, the generation split
- [orbistoun](https://github.com/project-oops/Orbistoun/blob/main/docs/GLOSSARY.md) - guest execution, thunks, HLE
- [Prosperous](https://github.com/project-oops/Prosperous/blob/main/docs/GLOSSARY.md) - targets, chains, scan roots
