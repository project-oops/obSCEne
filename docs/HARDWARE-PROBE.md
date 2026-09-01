# On real hardware, obSCEne stops being a test suite

Scoping notes from the orbistoun side, written when a jailbroken hardware was on its way.
Nothing here is a decision - it is what the emulator side would ask for, why, and what it
would unblock. Take, leave or argue with any of it.

## The purpose inverts

Today obSCEne asks *"does this emulator behave correctly?"* and answers pass / partial /
fail / skip. Against real hardware that question is uninteresting - the answer is yes.

What becomes interesting is the opposite direction. On hardware obSCEne is the only thing
in either project that can **answer a question** rather than test an answer. It stops
being a suite and becomes an instrument.

That is a different job, and it wants different output. A `res|pass` record is worth
nothing from hardware. `here are the 24 bytes that function wrote` is worth an afternoon.

## What the emulator side cannot get any other way

Ranked by what is blocking work right now.

### 1. Structure layouts - raw bytes, not interpretations

Call a function, then hexdump the buffer it filled. Do not parse it, do not name the
fields, do not decide what they mean. **Print the bytes.**

orbistoun spent four experiments establishing that
`sceKernelDirectMemoryQuery` writes 24 bytes and that its guest ignores the return value.
One hexdump ends that class of work permanently.

Better still, and cheap to automate: **binary-search the size argument.** Pass 8, 16, 24,
32, 64 and find where the function starts refusing. That yields exact structure sizes for
every call that takes one, without anybody deciding anything.

### 2. The real memory map

Walk the direct-memory enumeration to completion and print every region - start, end,
type, flags, whatever else comes back.

This is the single thing blocking a commercial title in orbistoun today. We know a guest
**rejects** a map consisting of one free 8 GiB region, and we know nothing whatsoever
about what it would accept. A real map ends that immediately.

Worth capturing at two moments: at process start, and after a few allocations, so the
*shape of a change* is visible and not only the initial state.

### 3. Real error codes

Call everything with deliberately wrong arguments - null pointers, zero lengths, offsets
past the end, undersized structures - and record what comes back.

orbistoun's error values are invented placeholders that deliberately avoid colliding with
real ones. That is honest but it means every unimplemented stub fails in a way no guest
recognises, and a guest that does not recognise a failure often retries forever. A table
of real codes turns "unimplemented" into "fails the way the caller expects".

### 4. Resolution by name - the one that changes the most

**If the system can look a symbol up by name, obSCEne becomes a name oracle.**

orbistoun currently recovers names by generating candidates and hashing them, hoping for a
collision against a title's import table. It works, and it took 2.4 billion candidates and
94 seconds per module to name about a fifth of what a title imports.

A by-name lookup replaces that entirely: ask the hardware whether a name exists, get yes or
no. One question per name, and it works for functions **no title happens to import** -
which the collision method can never reach.

If `sceKernelDlsym` or an equivalent resolves by name after a module is loaded, a probe
that walks a candidate list and reports which resolve is worth more to the naming problem
than everything built for it so far.

### 5. A command buffer, from a binary we wrote

The GPU work is blocked on one artifact: a real command buffer plus the memory its shader
addresses point at. It is currently waiting on a commercial title booting far enough to
submit one, which is a long way off.

obSCEne could build a minimal draw, submit it, and dump the buffer and the pages it
references - and that unblocks the GPU side **without any title booting at all**.

### 6. Whether a GPU address is a CPU address

Allocate, write a known pattern, hand the address to the GPU, and see whether the GPU
reads the pattern back. One bit, no ambiguity.

Unified memory *suggests* the addresses are the same. The GPU work has been deliberately
paused rather than built on a suggestion, because if it is wrong every shader address
points at nothing.

## Records these would need

The existing `OBS|` format already does the hard part. What is missing is record types for
*measurements* rather than verdicts. Suggestions only - the shapes matter more than the
names:

| Record | Fields | For |
|---|---|---|
| `dump` | check id, label, address, length, hex | Structure layouts, buffers, anything |
| `region` | index, start, end, type, flags | One line per memory region |
| `err` | library, symbol, argument description, returned value | The error-code table |
| `resolve` | library, symbol, `present` / `absent`, address | The name oracle |
| `size` | library, symbol, argument index, accepted, rejected | Binary-searched structure sizes |

Two things worth holding to, both of which the format already gets right:

- **Hex, not interpretation.** A dump that says `start=0, end=0x200000000` has already
  decided the layout. One that says `00 00 …` has not, and can be re-read later when
  somebody knows better.
- **A record per fact, not per narrative.** Both sides diff these; prose does not diff.

## Remote commands

Feasible, and probably the highest-leverage single thing on this page.

### Why it matters

It converts the loop from *guess → rebuild → run → observe* into **ask**. On the emulator
side, establishing one fact about `sceKernelDirectMemoryQuery` cost ten runs of a
commercial title, ten rebuilds and about an hour. The same fact over a command socket is
one round trip.

It also makes an agent useful in a way it currently is not: a session that can ask the
hardware questions can close a loop in seconds that otherwise needs a human in it.

### Shape

A line protocol over TCP is enough. Something like:

```
resolve <library> <symbol>        -> present/absent, address
call <symbol> <arg>...            -> return value
read <addr> <len>                 -> hex
alloc <size>                      -> address
regions                           -> the memory map
modules                           -> what is loaded
```

Replies as `OBS|` records, so the same parsers work and a session is indistinguishable
from a report.

`resolve` and `read` alone would be worth building even if `call` never happened.

### What would actually be hard

Not the socket. These:

- **A faulting command kills the payload.** The first bad pointer ends the session, and if
  the exploit is not persistent it ends the *afternoon*. This is the real engineering
  cost - a supervisor that survives a faulting command, or a command executor that
  validates arguments before dereferencing anything.

  **Built, on the first of those two lines.** `pros supervise <module.elf>` watches the
  serving port and re-sends the same bytes through the loader when it stops answering. It
  is in prosperous rather than here because prosperous is already the thing that reaches a
  hardware - and because a supervisor and the sender being one program is what makes the
  restart unattended.

  It refuses to send while the probe is answering (two copies on one hardware gives results
  from an unknown one), and gives up after three *consecutive* dead starts while allowing
  unlimited faults that have a working session between them - because that is the normal
  case and a cap on the total would stop the useful half rather than the broken half.

  What it does **not** do is validate arguments before dereferencing. That second option
  stays open, and it is the one that would stop the crash rather than recover from it.
- **Reboots.** Most jailbreaks need re-running after a power cycle, so the hardware is not
  a stable endpoint. Worth designing for reconnect from the start rather than assuming a
  long-lived session.

  **Not addressed, and the supervisor above does not touch it.** Re-sending a payload needs
  a loader already listening; a power cycle takes the loader with it. That still needs a
  person, and the distinction matters - a supervisor that appeared to survive reboots would
  be trusted through one.
- **`write` is where it gets dangerous.** `read` and `call` cost a crash at worst.
  Arbitrary writes cost a crash *and* whatever state was being built. Read-only by default
  would be the cautious default, with writes behind something deliberate.
- **An unauthenticated command socket on a LAN.** Fine in a lab. Worth binding narrowly and
  not leaving it running.

## Two notes on provenance

Written because the emulator side is strict about this and the boundary runs through here.

**Hardware output is ground truth.** Everything obSCEne reports from real hardware is our
binary, running our commands, observing our own experiment. In orbistoun's terms that is
`observed` - the strongest category available, and unlike a baseline taken from another
emulator it cannot be wrong in the way that one can.

**The thing to keep out is structure, not facts.** obSCEne can read whatever it likes;
orbistoun cannot. Since knowledge flows this way, it is worth being deliberate: *what a
correct system does* travels freely - sizes, codes, layouts, behaviour. *How another
implementation is written* should not, because arriving by way of obSCEne would not make
it ours. Hardware output sidesteps the question entirely, which is one more argument for
prioritising it.

## Settled

The open questions on this page have been answered, and the sketch above has become
`docs/PROTOCOL.md`. What follows is what was decided, kept here because this page is where
someone looking for the hardware story will start.

**The protocol is specified before it is implemented** (D102). obSCEne owns it; another
implementation builds against it and has no say in it. Nine captured exchanges in
`docs/examples/protocol/` are part of the contract, so a consumer can be built and tested
with no hardware attached.

**The transport is obSCEne's own socket, not the operating system's.** On a general-purpose
stand-in it is tempting to run `sshd` and drive the probe over it, and that defeats the
purpose: the whole value of a stand-in is that it exercises **the same code path** the
hardware will run, and the hardware has no shell to attach to. Only the socket layer differs
per target - everything above it is shared.

**The probe listens and the driver connects.** The hardware has no DNS and no configuration
file; it has an address a person can read off a screen.

**Crashes are the normal case, not the exceptional one**, and the protocol is built around
that rather than despite it. `ack` is flushed before a command runs, so a command that ends
the process is legible from the other end. `died`, `timeout` and `lost` are three separate
outcomes and none of them is ever written as `returned 0`.

### The two targets answer different questions

They are not interchangeable and the docs must not let one stand in for the other.

| | answers | cannot answer |
|---|---|---|
| a general-purpose RDNA2 handheld | **instruction behaviour** - it is an ISA oracle, and the first one this project would have | anything about system libraries; it has none of them |
| the hardware | **library behaviour**, error codes, layouts, and eventually graphics submission | nothing about a part it is not |

A stand-in announces no `resolve` capability, which is how a driver discovers the difference
rather than assuming it.

### Every record carries the part that produced it

Not optional metadata. A figure measured on a stand-in part and later read as authoritative
for a different one is a wrong answer with nothing in the record to reveal it - which is
precisely the failure that had this project's sibling targeting the wrong device generation
for months with nothing saying so.

`hello` emits `part` records - device, driver, Mesa, firmware, operating system, probe
build - and the corpus writer denormalises them onto every record. The wire binds them to a
session identifier for brevity; the corpus does not, because a record that must be joined
against something else to be interpreted will eventually be read without it.

### Two notes carried over from the planning exchange

**Generating an instruction list from a compiler's own tables is fine here and only here.**
For a generator whose oracle is silicon, enumerating instructions from LLVM's target
descriptions introduces no circularity: the answer comes from the hardware either way. The
same technique applied to a *decoder* that is checked against LLVM would make the
differential test confirm only that LLVM agrees with itself. The recommendation does not
travel across that boundary and should not be repeated as though it did.

**"Complete verification per instruction" is overclaimed and this page will not repeat it.**
Exhaustive is real for unary half-precision - 65,536 inputs - and bounded for unary
single-precision at about 4.3e9. A two-operand single-precision instruction is about 1.8e19,
which is **sampling with good coverage**. That is a perfectly good thing to do and a bad
thing to call complete.

## Questions back - answered

Kept as asked, with what is now known.

1. **Does anything resolve a symbol by name at runtime?** If yes, item 4 is worth more
   than the rest of this page combined.
2. **Is the exploit persistent across reboots?** It changes whether a command socket is a
   session or an appointment.
3. **Can the probe survive a faulting call**, or does one bad argument cost a re-exploit?
   Determines whether `call` is safe to expose broadly or needs argument validation first.
4. **What does the probe already know about structure sizes** from making its checks pass
   under other emulators? Some of item 1 may already be written down.

## If only one thing gets built

**Dump records, and `resolve`.**

The failure mode is easy to picture and would be painful: the hardware arrives, a hundred
probes run, everything works - and the findings exist as a screenshot. Everything else
here is worth more once the output is ingestible, and worth much less before.

## Readiness, audited 2026-08-26

Asked directly - with hardware close, can obSCEne answer the six questions above? Audited by
tracing each question to the check that would answer it and to what that check does on the five
loaders that now complete the suite.

| | question | ready | what answers it |
|---|---|---|---|
| 1 | structure layouts | **yes** | `obs_report_buffer`, four sites; `130-layout/direct-memory-query` already reports *"the call refused; its bytes are recorded anyway"* - the hexdump-not-interpretation discipline, working |
| 2 | the real memory map | **yes** | `150-memory-map/walk` and `/after-allocation`, which is the two-moments capture this page asked for |
| 3 | real error codes | **yes** | `obs_report_error_code`, six sites |
| 4 | resolution by name | **yes** | `140-oracle/resolve-by-name`, skipping on every loader with *"nothing resolves by name, including a symbol known to be present"* - the correct answer about them, and it runs the moment something does |
| 5 | a command buffer | **half** | `165-gnm` calls the command *builders* and dumps their bytes, so the buffer arrives; it submits nothing, so the pages its shader addresses point at do not |
| 6 | GPU address vs CPU address | **no** | `obs_gpu_backend_available()` returns 0 on the hardware by design |

### Question 6 cannot be answered by the probe as it stands, and that is deliberate

`src/probe/gpu_gnm.c` refuses. The hardware submits compute through Gnm/Agc, the submission format is
partly public and available from essentially one source, and D008 says not to guess at it on a
target that took effort to reach. `GPU=1` does not change this - it changes the skip *reason*
from "built without OBS_GPU" to the backend refusing, which is more informative on hardware and
nothing more.

This is worth knowing before the hardware is in front of somebody rather than after: the GPU
questions are not one flag away. They are gated on confirming a submission format, and on
whether GPU access is reachable from an unsigned module at all - itself a hardware question.

**The order that follows from this is sequential.** A first run answers 1-4 and dumps the raw
material; a rebuild informed by those layouts is what could reach 5 and 6. Planning for one
session that answers everything would be planning for the wrong thing.

### Retrieval is the part that would make the rest moot, and it has three routes

| route | mechanism | proven by |
|---|---|---|
| file | `/data/obscene-report.txt`, then `/download0/` | the resume mechanism reads the same paths back |
| display | the probe draws its own report | four loader screenshots |
| network | `SERVE=1`, listens after reporting | `docs/PROTOCOL.md`, captured exchanges in `verify.sh` |

No single point of failure, and the file route doubles as the resume state, so the hardware that
kills the run partway is walked past on the next attempt exactly as an emulator is.

### The one gap worth closing first

**The `size` record does not exist.** This page proposes binary-searching a size argument -
pass 8, 16, 24, 32, 64, find where the call starts refusing - and calls it cheap to automate.
Nothing in `src/probe/sections/` varies a size argument systematically.

It is the best value of anything here: mechanical, no interpretation, and it yields exact
structure sizes for **every call that takes one** rather than the four that have a hexdump
site today. It also produces `derived` rather than `assumed` provenance, which is the ladder
this project is trying to climb.

## What else the hardware can be asked, beyond the six

Written 2026-08-26, with a real 12.40 machine about to run this. The six above are what was
blocking work. These are what the same trip could also collect, ordered by what they would be
worth to somebody writing an emulator afterwards.

Every one of them is the same trick: **make the platform draw the line, and record where it
fell.** That is what turns `assumed` into `derived`, and it is the only lever this project has
that does not involve deciding something.

### The provenance boundary, stated before the list rather than after

Metadata about the environment is fair game: module names, handles, sizes, addresses, which
symbols resolve, what a call returns. **The contents of vendor binaries are not.** Enumerating
that a module called `libkernel.sprx` is loaded at some address with some size is an
observation about the machine. Dumping or disassembling its text is reading a vendor binary,
which principle 6 forbids outright and which no result here is worth.

The line is not subtle and nothing below approaches it. It is written down because a probe that
can read memory is one bad idea away from crossing it.

### 1. The name oracle, run to exhaustion - probably the biggest prize here

Question 4 asks whether by-name resolution works at all. If it does, the follow-up is where the
value is: this repository already holds `data/mined-names.txt` and `data/unnamed-nids.txt`, and
a working `sceKernelDlsym` turns that corpus into a lookup instead of a search.

The alternative is what orbistoun does now: generate candidate names, hash them, hope for a
collision against a title's import table. 2.4 billion candidates and 94 seconds a module to
name about a fifth of what one title imports - and structurally incapable of naming anything no
title happens to import.

A by-name oracle answers one name per question, needs no title, and reaches the whole surface.
If resolution also returns an **address**, two names at one address are aliases and the address
ordering exposes the export layout for free.

This is worth building a dedicated pass for, and worth budgeting the most time to.

### 2. Constant-space enumeration - every invented constant, replaced

The size ladder in `130-layout/query-size-ladder` walks a size argument and finds where the
call starts refusing. **The identical mechanism works on any small argument**, and the
interesting ones are the enumerated values this project currently has to assume:

- the `flags` argument to `sceKernelDirectMemoryQuery` - four values are probed today; sweep 0
  to 255 and record which are accepted
- memory protection bits on `sceKernelMapDirectMemory`
- memory types on `sceKernelAllocateDirectMemory`

D008 forbids inventing a constant. This *derives* the valid set, and an emulator that validates
its arguments against a real accepted-set rejects what the hardware rejects rather than what
somebody guessed. Cheap, mechanical, and it applies to every enumerated argument in the census.

### 3. Differential dumps - which field is which, not just what the bytes are

A single hexdump says what a structure contains. Calling the same function twice with different
inputs and diffing the two buffers says **which field carries which input**, which is the part
somebody would otherwise reverse-engineer by staring.

`sceKernelDirectMemoryQuery(offset=0)` against `(offset=0x10000000)`: the bytes that differ are
where the start address lives. The bytes that stay the same across every input are constants or
reserved. That is a field map, derived, from two calls.

`obs_report_written` already does the poison-versus-after comparison this needs; the same
comparison across two *results* is the same code with different inputs.

### 4. The platform manifest - what firmware 12.40 actually has loaded

`sceKernelGetModuleList` deals only in handles and is safe outright; `sceKernelGetModuleInfo`
fills a structure whose layout the size ladder can now establish. Together they give the
module inventory of a real machine: names, handles, base addresses, segment sizes.

No emulator author has a reliable list of what a real system loads, in what order, at what
sizes. This is metadata about the environment and stays well inside the boundary above.

### 5. Handle shape - because emulators invent handle schemes and titles inspect them

Create several objects of several kinds and print the handles. Small integers or pointers?
Sequential or sparse? Does a semaphore handle collide with an event-flag handle, or are the
spaces disjoint or tagged?

Emulators pick a scheme that satisfies their own code. A title that inspects, compares or
tags a handle then breaks in a way that looks like a bug in something else entirely.

### 6. An error-code decision table, not an error-code list

Question 3 collects what comes back from wrong arguments. The stronger version distinguishes
*kinds* of wrong: null pointer, bad size, bad handle, bad flags, misaligned pointer - four or
five deliberate mistakes per call, each recorded separately.

The result is a table an implementer can act on. "Fails with 0x80020016" is a fact; "fails
with 0x80020016 **for a bad handle specifically**" tells an emulator which of its stubs should
return what, and a guest that recognises the failure stops retrying forever.

### 7. Alignment and boundary requirements

The ladder again, on a third axis: misaligned pointers, sizes that straddle a page, offsets at
and past the end. Which are refused? Emulators are typically permissive here, so a title that
depends on a rejection gets silence instead.

### 8. Everything tagged with the firmware it came from

`sysinfo|firmware` reports `unconfirmed|unknown` on every loader in the toolkit. On hardware it
should become known, and **every record in that report is then a fact about 12.40
specifically**. An untagged corpus ages badly; a tagged one stays useful after the next
firmware changes something, because the disagreement is attributable.

Worth checking before the trip that the firmware field actually populates, since a whole
corpus's long-term value rests on it.

### What should stay switched off

`HARDWARE=1` already excludes `BULK`, and that is the right call. The blind prober calls
unnamed NIDs to see what happens, which is a reasonable thing to do to an emulator and an
unreasonable thing to do to hardware somebody owns. Nothing here changes that.
