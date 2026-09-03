# Documentation

Every file here is listed, and `obscene-tool doccheck` fails if one is not - an index that
silently stops covering the directory is worse than no index, because it reads as complete.


## The words

- [GLOSSARY.md](GLOSSARY.md) - what a check is, what a section is, the census, the sinks, and
  which execution context a result came from. Start here if `DT_INIT_ARRAY`, `ps4_mode` or
  "the census" are not words you already have. The collection's glossary covers standard ELF
  and the words that mean something else in the sibling repositories.

## What obSCEne is

- [DESIGN.md](DESIGN.md) - why it is shaped the way it is: what it announces before it acts,
  why it is written from the failure side, how the sections are ordered base-to-high-level,
  why presence and behaviour are separate questions, and what it refuses to invent. This was
  the middle of the README, where it sat between a reader and the build instructions.

## Building it

- [BUILDING.md](BUILDING.md) - `bin/obscene`, what you need installed, the two siblings it does
  not build without, the make variables, which shape reaches which loader, and
  what CI runs. Read this before `git clone`: a clone of only this repository does not build,
  and the failure is a missing directory rather than a missing dependency.

## Consuming obSCEne

- [OUTPUT.md](OUTPUT.md) - the report format. A contract; parsers exist.
- [PROTOCOL.md](PROTOCOL.md) - the command protocol, version 1. What the wire carries.
- [CLIENT.md](CLIENT.md) - the other half of the protocol: what a client author has to do
  about it, in the order they will hit it. Self-contained and copyable.
- [TOOLING.md](TOOLING.md) - `obscene-tool`, what each subcommand is for.

## Running it somewhere

- [INJECTOR.md](INJECTOR.md) - the native process injector: why a payload loaded the ordinary
  way lands in the previous generation's compatibility sandbox, and what taking over a live
  process buys that the sandbox refuses.
- [ARTIFACTS.md](ARTIFACTS.md) - **which file goes where**. Each shape reaches a different
  loader, they are told apart by two bytes, and sending the wrong one to the hardware cost a
  loader and a reboot. Read before sending anything anywhere.
- [BOOT.md](BOOT.md) - the load sequence from "the loader has your file" to your first
  instruction: `e_type`, `EI_ABIVERSION`, the vendor dynamic table, `DT_INIT` and who calls
  it, unresolved-import strategies, and what is still unknown.
- [HANDOVER-ORBISTOUN.md](HANDOVER-ORBISTOUN.md) - what obSCEne measured on the sibling
  project, and a process it suggests for turning a stub into a shaped implementation without
  reading anyone else's source. Written to be read cold from that side.
- [LOADING.md](LOADING.md) - what a loader has to handle, written from the module side.
  A list of things a module will do to you.
- [MODULE-FORMAT.md](MODULE-FORMAT.md) - what a loader requires of the module, and how each
  requirement was established.
- [COMPATIBILITY.md](COMPATIBILITY.md) - what each loader actually does with obSCEne.
  Generated from the reports.
- [EMULATORS.md](EMULATORS.md) - the emulator toolkit: what each is good for, and the
  provenance boundary when reading them.
- [HARDWARE.md](HARDWARE.md) - **what a real console answered.** The findings, each citing a
  record in `data/hardware/`: which libraries a title is actually given, what the census
  reached, and which ones end the process when loaded.
- [PLATFORM_LIBRARIES.md](PLATFORM_LIBRARIES.md) - **the retail SPRX filesystem and privilege model.**
  Partition layout (`/system/common/lib`, `/system_ex/common_ex/lib`, `/system/priv/lib`),
  the four privilege tiers (`app`, `sysmodule`, `system`, `root`), and differences from PS4.
- [HARDWARE-PROBE.md](HARDWARE-PROBE.md) - what changes when the target is real hardware
  rather than something standing in for one. Written before any of it ran, so it is the
  request where `HARDWARE.md` is the answer.
- [TRACER.md](TRACER.md) - design for the passive call/response tracer: the on-hardware
  companion that records what real titles call, decoded into this project's report format.
- [DECK.md](DECK.md) - the runbook for capturing and analysing a real RDNA2 GPU corpus.

## The GPU

- [GPU_SURFACE.md](GPU_SURFACE.md) - the GPU ISA surface and obSCEne's coverage of it.
  Generated; the source of truth is `data/gpu-surface.tsv`.
- [GNM.md](GNM.md) - the platform's GPU API, both axes: the PM4 command-builder probe and the
  compute-result path.

## Durable memory

- [DECISIONS.md](DECISIONS.md) - a generated index over `decisions/`, one file per entry,
  with a status column. Numbers are unique and gated; a citation resolves to exactly one
  file.
- [MILESTONES.md](MILESTONES.md) - a generated index over `milestones/`. **The firsts**, each with the artefact that proves it and
  the build that produced it. Deliberately short - a first is the point after which a class of
  work became possible, and it has to stay findable in a way a four-thousand-line worklog
  cannot make it. Ends with what is *not* done yet, so the list
  stays honest about its own scope.
- [WORKLOG.md](WORKLOG.md) - what was done, in order, plus surprises.
- [BACKLOG.md](BACKLOG.md) - a generated index over `backlog/`, with a status column.
  What is not done, ranked, with what was struck and why.
- [WORKFLOW.md](WORKFLOW.md) - how the whole thing fits together.

**These last four are dated records.** Counts inside them were true when written and are
not corrected afterwards - `obscene-tool doccheck` exempts `DECISIONS.md` and `WORKLOG.md` from the
accuracy checks for that reason. Correcting a log is falsifying it.

---

Start with [OUTPUT.md](OUTPUT.md) if you are consuming a report, [CLIENT.md](CLIENT.md) if
you are driving one over a socket, [LOADING.md](LOADING.md) if you are trying to run the
module, and [../CLAUDE.md](../CLAUDE.md) if you are changing the code.

## Adding to a log

DECISIONS, BACKLOG and MILESTONES are **directories with a generated index**. Add a file under
`decisions/`, `backlog/` or `milestones/`, then regenerate its table:

```bash
tools/split-decisions.sh --index obscene
tools/split-doc.sh --index obscene BACKLOG 2 backlog
```

Do not edit an index by hand - it is overwritten, and the splitter refuses to run over one.
The split exists because two sessions appending to one file collide, and because this log
reached 634,607 bytes, past the point where GitHub renders markdown at all.

`WORKLOG.md` is deliberately **not** split. It is the file most written during a session, and
at 344 KB it still renders.
