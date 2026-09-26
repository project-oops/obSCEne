# Workflow

The loop from "the emulator does not run this" to "the emulator runs this", where each tool
fits, and which tools a person runs by hand.

## NIDs are not reversible

A module does not import `sceKernelWrite`. It imports `4wSze92BhLI`, which is the first eight
bytes of `SHA-1("sceKernelWrite" ‖ suffix)`, written in a compact alphabet. Hashing discards
information, so no computation turns a NID back into a name. There are two ways to name a NID:

1. **Lookup.** If the pair is already established, a table gives the name instantly. This
   covers most imports.
2. **Cracking.** Otherwise, hash candidate names until one matches. That is `crack`, and it is
   a campaign rather than a lookup.

| | what it does | reversible | who runs it |
|---|---|---|---|
| **hashing** | `sceKernelWrite` → 8 bytes | no | automatic, every build |
| **encoding** | those 8 bytes → the text `4wSze92BhLI` | yes | automatic, every build |
| **decoding** | `4wSze92BhLI` → back to the 8 bytes | yes | by hand, rarely |
| **cracking** | guess names, hash them, look for a match | the only way back | by hand, occasionally |

`decode` does not undo the hash. It undoes the text encoding and returns the same eight bytes.

## The loop

### 1. Build the emulator against public information

Public documentation, open-source toolchains, published interface descriptions.

### 2. Run real software and record what fails

The output is a list of NIDs the loader could not resolve, plus whatever fails afterwards. The
emulator records that list as data, one record per line: the NID, its library, its module, and
the module that wanted it. That list is a **demand ranking** - the NIDs that block real
software, ordered by how many things want them - and it is useful before any name is known.

### 3. Turn the NIDs into names

**The table.** `data/nid-corpus.txt` holds established pairs, harvested from emulator
resolution logs, which print the name they matched for every NID.

**The cracker,** for what the table does not cover:

```bash
obscene-tool crack --nids unresolved.txt --words candidates.txt --known data/nid-corpus.txt
```

It hashes every candidate and reports matches. The header comes first:

```
# candidates <n>
# generator  reproduced <k> of <k> known pairs
# recovered  <m> of <t>
```

The `reproduced` line says whether the candidate list regenerates names already known. A list
that cannot is not evidence about names that are not known. A match is proof; a miss means the
name was not guessed, not that it does not exist.

### 4. Implement the function and check its behaviour

Implementing is the emulator's work. Checking is obSCEne's: it calls the function and reports
what happened.

- A **missing** function shows up as an unresolved NID.
- A **stubbed** function resolves and returns zero to everything. `007-responsive` catches it
  by calling each function twice with inputs whose answers must differ.
- A **wrong** function resolves, varies with its input, and gives the wrong answer. The
  behavioural checks catch it.

An existence test cannot tell a stub from a wrong function, and the two need opposite work.

### 5. Repeat

`obscene-tool diff` compares two runs and reports what got worse, not what is failing.

## Automatic and manual commands

### Automatic

| | when | what it does |
|---|---|---|
| `mkmodule` | every `make module` | hashes every import name into a NID, builds the vendor tables, checks its own work |
| `derive` | inside `mkmodule` | re-derives the format constants from what it wrote, and fails the build if they disagree |
| hashing / encoding | inside `mkmodule` | never invoked directly |

### Manual

| | when to run it |
|---|---|
| `nid <name>` | an emulator logged an unknown NID and the question is which function it is |
| `crack` | a batch of unresolved NIDs has no names and is worth a campaign |
| `decode` | reading raw bytes out of a module's symbol table by hand |
| `pretty` / `verify` / `diff` | reading results; `verify` in CI, the other two by eye |
| `surface` | adding names to the census: edit `data/surface.txt`, then regenerate |
| `mine` | an emulator checkout has moved on and the corpus should see it |
| `gap` | listing what the emulators implement that obSCEne never reaches |

## Generated stubs and written checks

**Emulator stubs are generated.** With a NID-to-name table, a stub per entry is mechanical: log
the name, return an unimplemented error, carry on. shadPS4's `CommonStub` does this, which is
why its logs name the function rather than the hash. The table is what everything else turns
on:

```
NID → name table
   ├── generated stubs that log by name
   ├── resolution logs a human can read
   ├── unresolved-import lists worth prioritising
   └── a checklist of what to implement next
```

**obSCEne checks are written by hand.** A check encodes an expectation, and a generated
expectation is worth nothing. "`strlen` returns the number of characters" comes from a
standard; "closing an invalid handle returns non-zero" is a belief. Every check records which
(D044). The census is generated from a list because it claims only that a symbol exists.

## Running the loop

```bash
# One run: build (in WSL), fetch, run, extract the report.
sh scripts/run-emulator.sh --emulator <emulators>/shadps4/shadPS4.exe

# Every loader on the same module, with a screenshot and log each.
sh scripts/sweep-emulators.sh

# Grow the NID table from emulator logs. Merges, never replaces.
sh scripts/harvest-nids.sh reports/*.log

# Everything that must pass before a change is done. Run in WSL.
sh scripts/verify.sh
```

The emulators live in `<emulators>`, outside this repository, and the scripts default to the
shadPS4 there. [EMULATORS.md](EMULATORS.md) describes the kit.

A call that ends the process takes the rest of the suite with it. An exclusion sweep reads the
report for a `try` with no matching `res`, adds that check to the exclusion list
(`sweep-build.sh` builds with it), and runs again. Excluded checks stay in the report as skips
with their reason. A timeout leaves the same trace as a crash, so a timeout doubles the budget
and retries the same build; only an identical stopping point under twice the time is a hang
(D144). The first pass starts with an empty exclusion list, so a stale exclusion never hides a
check that no longer crashes.

Builds land on a Linux-local `BUILD` path, never the mounted `/mnt/c/...` tree: a Windows mount
cannot carry the execute bit, and the host binary that generates `symbols.txt` will not run
from it. Reports go to `reports/`. Calls from Git Bash into WSL set `MSYS_NO_PATHCONV=1`
([TOOLING.md](TOOLING.md#path-conversion-under-git-bash)).

## What obSCEne gives an emulator

- **An inventory.** `110-modules` asks the platform what it has loaded.
- **A stub map.** `007-responsive` says which functions read their arguments and which return
  zero to everything. A failure against a stub is absence, not incorrectness.
- **Behavioural checks** with `try`-before-call, so a call that takes the process down is named.
- **A diff** that reports what got worse.
- **A drawn report**, for when there is no working way to get text out.

## What an emulator gives obSCEne

A machine-readable list of every NID it failed to resolve, with its library and module. Names
are not required: an unresolved NID with a count is already actionable.

## The loop on a console

The console runs the same probe and produces the same report. The package builds in WSL,
installs from Windows (the console fetches it), and the report comes off the system log rather
than a pipe.

```bash
./bin/obscene deploy    # build, install, launch, and capture the report
./bin/obscene report    # capture the report again for a title already running
./bin/obscene recover   # read-only, after a crash: what the console recorded
```

`./bin/obscene help` lists every hardware verb. The repository `CLAUDE.md` ("Hardware
round-trip") says which side runs each half. [HARDWARE.md](HARDWARE.md) records what the
hardware answered.
