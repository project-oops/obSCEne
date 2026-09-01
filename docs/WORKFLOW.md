# How the whole thing works

Written for the emulator side, and written plainly. It covers the loop from "the
emulator does not run this" to "the emulator runs this", where the tools fit, which of
them a person runs by hand, and which run themselves.

## The one correction worth making first

A reasonable mental model of the loop goes:

> build the emulator from public information → run real software at it → watch what
> breaks → **decode the stuck symbol back into a method** → implement that method →
> repeat

Every step is right except the fourth, and it is the one everything else hangs off.

**You cannot decode a symbol back into a method.** A module does not import
`sceKernelWrite`. It imports `4wSze92BhLI`, which is the first eight bytes of
`SHA-1("sceKernelWrite" ‖ suffix)`, written in a compact alphabet. Hashing throws
information away on purpose. There is no computation that runs it backwards.

So the fourth step is really two steps with very different costs:

1. **Look it up.** If somebody has already established that `4wSze92BhLI` is
   `sceKernelWrite`, this is a table lookup and it is instant. This is what happens
   almost every time.
2. **Guess it.** If nobody has, the only route is to hash candidate names until one
   matches. That is `crack`, and it is a campaign rather than a lookup.

Everything below follows from that split.

## The four words that sound the same

| | what it does | reversible? | who runs it |
|---|---|---|---|
| **hashing** | `sceKernelWrite` → 8 bytes | **no** | automatic, every build |
| **encoding** | those 8 bytes → the text `4wSze92BhLI` | yes | automatic, every build |
| **decoding** | `4wSze92BhLI` → back to the 8 bytes | yes | by hand, rarely |
| **cracking** | guess names, hash them, look for a match | the only way back | by hand, occasionally |

If you remember one thing: **`decode` does not undo `hash`.** It undoes the text
formatting and hands you the same eight bytes you were stuck with. Opening the envelope
does not unshred the document.

## The loop, end to end

### 1. Build the emulator against public information

Unchanged from your model. Public documentation, open-source toolchains, published
interface descriptions.

### 2. Run something real at it and watch

Also unchanged. What comes out is a list of NIDs the loader could not resolve, plus
whatever went wrong afterwards.

**The emulator's job here is to record that list as data**, not prose. A NID, its
library, its module, and ideally which module wanted it - one record per line. That list
is a *demand ranking*: the NIDs that block real software, ordered by how many things
want them. It is useful before anyone knows what any of the names are.

### 3. Turn the NIDs into names

Two paths, in order of cost.

**The table.** `data/nid-corpus.txt` holds established pairs - 389 at the time of
writing, harvested from an emulator's own resolution logs, which print the name they
matched for every NID. Free, and it covers most of what any module imports.

**The cracker,** for what the table does not cover:

```bash
obscene-tool crack --nids unresolved.txt --words candidates.txt --known data/nid-corpus.txt
```

It hashes every candidate and reports matches. Read the header before the results:

```
# candidates 12480
# generator  reproduced 389 of 389 known pairs
# recovered  17 of 240
```

`reproduced 389 of 389` is the number that matters. It says the candidate list can
regenerate names *already known* - a list that cannot do that is not evidence about
names that are not known. If that number is short, a miss below means "we did not guess
it", not "it does not exist".

**A match is proof. A miss is nothing.** Those must never be read the same way.

### 4. Implement the function, and check it behaves

Implementing is the emulator's work. Checking is obSCEne's: it calls the function and
reports what actually happened, which is the part that catches an implementation that
exists and is wrong.

This is where the two halves meet, and it is worth being precise about the difference:

- **A missing function** shows up as an unresolved NID. Nothing to test yet.
- **A stubbed function** resolves and returns zero to everything. `007-responsive`
  catches these by calling each one twice with inputs whose answers must differ.
- **A wrong function** resolves, varies with its input, and gives the wrong answer.
  That is what the behavioural checks are for.

An existence test cannot tell the second from the third, and those need opposite work.

### 5. Repeat

`obscene-tool diff` compares two runs and reports what got **worse**, not what is
failing. Under an early emulator almost everything fails; a tool that called that a
regression would be ignored inside a day.

## Which commands does a person actually run?

The short answer: **almost none of them, almost never.**

### Automatic - runs itself, no human involved

| | when | what it does |
|---|---|---|
| `mkmodule` | every `make module` | hashes every import name into a NID, builds the vendor tables, checks its own work |
| `derive` | inside `mkmodule` | re-derives the format constants from what it just wrote, and fails the build if they disagree |
| hashing / encoding | inside `mkmodule` | you never invoke these; they are what `mkmodule` *is* |

If you only ever run `make module`, hashing and encoding happen hundreds of times and
you never type either word.

### By hand - occasionally, deliberately

| | when you would run it |
|---|---|
| `nid <name>` | "an emulator logged an unknown NID - which of my functions is that?" Answers in one line. |
| `crack` | you have a batch of unresolved NIDs nobody has named. A campaign, not a lookup. Run it when the list is worth the effort. |
| `decode` | almost never. Reading raw bytes out of a module's symbol table by hand. |
| `pretty` / `verify` / `diff` | reading results. `verify` in CI, the other two by eye. |
| `surface` | when adding names to the census - edit `data/surface.txt`, then regenerate. |
| `mine` | when an emulator checkout has moved on and the corpus should see it. |
| `gap` | "what do the emulators implement that we never touch?" |

## Are the stubs generated, or written?

Both, and the split matters.

**On the emulator side: generated, and they should be.** Once you have a NID→name
table, a stub per entry is mechanical - log the name, return an unimplemented error,
carry on. That is exactly what shadPS4's `CommonStub` does, and it is why its logs can
say `Stub: sceKernelWrite (nid: 4wSze92BhLI) called` rather than just reporting a hash.
Nobody should be hand-writing those.

**This is the real payoff of the NID table.** It is not a curiosity - it is the pivot
everything else turns on:

```
NID → name table
   ├── generated stubs that log by name
   ├── resolution logs a human can read
   ├── unresolved-import lists worth prioritising
   └── a checklist of what to implement next
```

Without the table you have hashes in your logs and no way to rank anything. With it,
every one of those falls out mechanically.

**On the probe side: written, deliberately.** Each obSCEne check is hand-written,
because a check encodes an *expectation* and a generated expectation is worth nothing.
"`strlen` returns the number of characters" comes from a standard; "closing an invalid
handle returns non-zero" is a belief somebody held. Every check records which
(`docs/DECISIONS.md`, D044), and generating them would erase exactly that distinction.

The census is the exception that proves the rule - 289 names generated from a list,
claiming only that a symbol exists, and nothing about what it does.

## Running the loop

Everything is `sh`, and the emulators are driven from it exactly as the builds are.

```bash
# One run: build (in the VM), fetch, run, extract the report.
sh scripts/run-emulator.sh --emulator C:\emu\shadPS4.exe

# A complete sweep: rounds until nothing kills the process.
sh scripts/sweep.sh --emulator C:\emu\shadPS4.exe

# Grow the NID table from whatever logs are lying around. Merges, never replaces.
sh scripts/harvest-nids.sh reports/*.log
```

```bash
# Everything that must pass before a change is done. Run this in the VM.
sh scripts/verify.sh
```

The emulators themselves live in `<emulators>`, outside this repository, and the
scripts default to the shadPS4 there. `docs/EMULATORS.md` says what is in the toolkit and
why the source is kept alongside the binaries.

`sweep.sh` is the interesting one. It runs, reads the report for a `try` with no matching
`res` - the announce-before-attempting invariant naming the exact call that did not
return - adds that check to the exclusion list, and goes again. A typical result:

```
round 1: 180 records, died in 040-file/open-rejects-null
round 2: 209 records, died in 080-video/flip-rate-rejects-bad-handle
round 3: 554 records, COMPLETE
```

Two calls that end the process, found and stepped over in three rounds without anyone
watching. Both stay in the report as skips with their reason.

**A timeout is not a crash, and one round cannot tell them apart.** Both leave the same
trace - a `try` with no `res` - so excluding on a timeout blames whichever check happened to
be running when the clock ran out. Two rounds *can* tell them apart: a check that was merely
unfinished gets further when given more time, and a hang stops in exactly the same place
however long it is left. So a timeout doubles the budget and retries the same build, and only
an identical stopping point under twice the time is called a hang (D144):

```
round 5: 65 records, the budget ran out at "015-sync/machine-kind"
  retrying at 480s to tell a hang from a check that was still going
round 6: 65 records, stopped at "015-sync/machine-kind" again
  unchanged at 480s after 240s, so it is hung rather than unfinished. Excluding it.
```

Against fpPS4 that walk took 33 records to a complete 742, over 44 exclusions and 35 rounds.
Two flags matter at that scale: `--resume` keeps an exclusion list a previous sweep proved,
because each exclusion costs two runs; and `--corpus 0` leaves the thirty thousand mined
targets out of the hunt, since the crashes a sweep is chasing live in the hand-written
checks. Resuming is off by default - the first pass must start empty, or a stale exclusion
hides a check that no longer crashes.

### The one environment hazard worth knowing

Git Bash rewrites anything that looks like a Unix path before a Windows program sees it,
so `--working-directory /home/ubuntu/obscene` reaches multipass as
`C:/Program Files/Git/home/ubuntu/obscene` and the command fails on a directory that does
not exist. Paths inside the VM must survive untouched, so every multipass call goes
through a one-line wrapper that sets `MSYS_NO_PATHCONV=1`.

**This is why these scripts used to be PowerShell.** CLAUDE.md said to use it for
multipass invocations for exactly this reason - and the reason was one environment
variable, not a language. Writing them in sh also deleted the hazard PowerShell brought
with it: it turns a native command's stderr into a terminating error, so multipass warning
about a file it had just copied correctly would kill a script, intermittently.

One thing left over: multipass copies a file to an NTFS target correctly and then exits
non-zero trying to set POSIX permissions on it. The scripts tolerate that and assert the
file exists instead, which is what they actually want to know.

`build/` is a second one, and unrelated: the VM mounts this repository and can create that
directory as root, after which the host cannot write into its own build directory. Reports
go to `reports/`.

## What obSCEne gives the emulator, concretely

- **An inventory.** `110-modules` asks the platform what it has loaded rather than
  testing a list somebody wrote.
- **A stub map.** `007-responsive` says which functions read their arguments and which
  return zero to everything. A failure against a stub is absence, not incorrectness.
- **Behavioural checks** with `try`-before-call, so a call that takes the process down
  is named rather than merely losing the run.
- **A diff** that reports what got worse.
- **A drawn report**, for when there is no working way to get text out - which happens.

## What the emulator gives obSCEne

One thing, and it is the input the whole loop needs: **a machine-readable list of every
NID it failed to resolve**, with its library and module. Names not required - an
unresolved NID with a count is already actionable, and cracking it is downstream work.

## The same loop, on a real console

Everything above is the emulator loop. The console runs the identical probe and answers the
same report, but the mechanics differ - the package builds in WSL, installs from Windows (the
console fetches it), and the report comes off the system log rather than a pipe (D233). That
whole round-trip is one command:

```bash
./bin/obscene deploy    # build, install, launch, and capture the report
./bin/obscene report    # capture the report again for a title already running
./bin/obscene recover   # read-only, after a crash: what the console recorded
```

`./bin/obscene help` lists every hardware verb, and `CLAUDE.md` ("Which side runs what") is the
runbook for why each half runs where it does. `docs/HARDWARE.md` is what a real PS5 actually
answered.
