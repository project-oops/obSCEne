# From "the loader has your file" to your first instruction

What happens between a loader opening a module and the guest's first instruction, what each
loader here does differently, and which parts are still unknown.

**Everything below is either measured from a run in this repository or citable to a line of
open source.** Where it is neither, it says so. That is the difference between this file and a
homebrew wiki: a claim here can be checked, and several are anchored to source tokens by
`obscene-tool claims` so they cannot rot silently.

---

## The short version

```
loader opens the file
  → is it a SELF (signed) or a bare ELF?
  → header checks: e_ident[EI_OSABI], e_ident[EI_ABIVERSION], e_type
  → map PT_LOAD segments at a base address
  → read the dynamic table (the *vendor* one, not the standard one)
  → resolve imports, or patch what it cannot resolve
  → run module initialisers                    ← DT_INIT, and loaders disagree here
  → fetch the entry point                      ← only from a *non-shared* module
  → call the entry
```

Two of those steps have caught this project out, both discovered on 2026-08-25, and both are
things a stock toolchain gets wrong by omission rather than by error.

---

## `e_type`: executable or library, and it is not a formality

| value | name | meaning |
|---|---|---|
| `0xFE00` | `ET_SCE_EXEC` | fixed-address executable |
| `0xFE10` | `ET_SCE_DYNEXEC` | **executable**, position-independent - what `eboot.bin` is |
| `0xFE18` | `ET_SCE_DYNAMIC` | **shared library** - what a `.prx` is |

Cited from Kyty's own header, which is the clearest statement of it in any source here:

```cpp
constexpr Elf64_Half ET_DYNEXEC = 0xfe10; // Executable file
constexpr Elf64_Half ET_DYNAMIC = 0xfe18; // Shared
```

**This distinction decides whether your entry point is ever called.** A loader that respects
it treats `0xFE18` as a library: it runs the module's initialisers and then looks *elsewhere*
for a process to start. Kyty makes both halves explicit:

```cpp
// StartAllModules - initialisers, for shared modules only
for (auto* p : m_programs) { if (p->elf->IsShared()) { StartModule(p, …); } }

// GetEntry - an entry, from a non-shared module only
for (const auto* p : m_programs) { if (!p->elf->IsShared()) { return p->elf->GetEntry() + p->base_vaddr; } }
return 0;
```

obSCEne declared `0xFE18` for months. On Kyty that produced a module which loaded, relocated,
ran its initialiser, reported `Execute: Main`, called nothing, and exited cleanly - a silent
no-op with no error anywhere. The documentation asserted `0xFE18` was "the dynamic-executable
type", which is the opposite of what it means.

**The trap:** a loader that does *not* distinguish will run a `0xFE18` module perfectly well,
so this can be wrong for a long time without a symptom. shadPS4, fpPS4 and PS5PCEM all ran
obSCEne with the wrong type; only Kyty noticed.

---

## `EI_ABIVERSION`: the generation marker, read before anything executes

Byte 8 of the ELF header. `0` for the previous hardware generation, `2` for the current one.
A loader reads it *before* running a single guest instruction, so it cannot be negotiated at
run time - it is a build flag (`GEN=4` / `GEN=5` here).

Loaders disagree, and all of them are right to:

- shadPS4 is a previous-generation emulator and **refuses** `2` outright:
  `IsElfFile: e_ident[EI_ABIVERSION] expected 0x00 is (0x2)` → `not valid ELF file`
- Kyty reads `2` as current-generation (`IsNextGen()` is literally `e_ident[EI_ABIVERSION] == 2`)
- craziiEmu reads it the same way

`EI_OSABI` must be `0x09` (FreeBSD). `lld` sets it; GNU `ld` does not.

---

## The dynamic table is the vendor's, not the standard one

A module for the hardware carries **two** things a normal ELF does not:

- `PT_SCE_DYNLIBDATA` (`0x61000000`) - the string, symbol, hash and relocation tables
- vendor `DT_SCE_*` tags in the dynamic segment, whose values are *offsets into that segment*
  rather than virtual addresses

A loader ignores every standard dynamic tag, so **a module without the vendor segment loads
and resolves nothing**. `obscene-tool mkmodule` builds it, and `obscene-tool derive`
re-derives the tag assignment from a finished module and fails if it does not reproduce.

`docs/MODULE-FORMAT.md` has the field-level detail.

---

## `DT_INIT`: who calls it, and what happens when it is absent

The module initialiser. **Five loaders, four different answers**, each citable:

| loader | calls it for an executable | guards it being absent |
|---|---|---|
| shadPS4 | no - `Execute()` goes straight to `RunMainEntry` | n/a |
| fpPS4 | no - the call site is commented out (`ps4_elf.pas:2814`) | only `-1` |
| PS5PCEM | no - deliberately, see below | yes: must resolve to executable memory |
| **Kyty** | **yes** | **no** |
| orbistoun | not yet; parses the tags (recorded in their own decision log) | yes, with a test |

<!-- obscene:claim file=src/probe/crt.c contains=obs_module_init -->
PS5PCEM's reason is the one that matters beyond emulation:

> The executable's PS5 CRT entry calls its own `DT_INIT` routine. Calling that routine here as
> well runs global constructors twice; intrusive registration lists then contain the same node
> twice and can become cyclic.

**So the hardware's own CRT is reported to call it.** A module without a `DT_INIT` is therefore
a risk on hardware, not just a Kyty inconvenience.

Kyty calls it with no check that the tag exists:

```cpp
return run_ini_fini(program->dynamic_info->init_vaddr + program->base_vaddr, …);
// run_ini_fini:
return reinterpret_cast<module_ini_fini_func_t>(addr)(args, argp, func);
```

With no tag, `init_vaddr` is zero, so it calls `base_vaddr + 0` and **executes the ELF header**
- `\x7fELF` interpreted as machine code. The observed symptom is an access violation at
`base + 2`.

A stock linker has no reason to emit `DT_INIT`, so this is absent by default. obSCEne now
carries an empty one (`obs_module_init`), and `mkmodule` emits the tag **only when the symbol
exists** - a `DT_INIT` of zero is worse than none, because it is precisely what an unguarded
loader calls.

---

## Bare ELF versus SELF

A retail `eboot.bin` is a **SELF**: a signed container wrapping the ELF, signed with keys
nobody outside the vendor has. That is a wall rather than a gap.

**A fake SELF is a different object, and this file used to conflate the two.** It is the same
container with a known dummy where the signature goes, which the hardware running a kernel patch
accepts - no signing is involved at all, which is how homebrew ships on the previous hardware.
So "it cannot sign one, and that is not going to change" was true of retail and wrong about
the shape that matters.

The distinction is load-bearing rather than pedantic: **an fSELF is the only thing the system
loader will take**, and therefore the only way to answer a single question in the *What is
still unknown* section below - every item there is a question about the loader. A bare ELF
reaches a homebrew loader, which maps the segments itself, so it reports what the libraries do
and nothing about the platform's own path. See D180.

Until `obscene-tool` grows an `mkself`, obSCEne emits a bare ELF.

Every loader in this toolkit has a "not a SELF, treat it as an ELF" path:

```
IsSelfFile: Not a SELF file. Magic mismatch current = 0x464c457f expected = 0x1d3d154f
```

orbistoun is the exception so far: its bare-ELF path reports `wrapper none (bare ELF)` and then
`mapped segments []` - it locates data for none of the program headers, so the dynamic table
cannot be read and it halts before imports. Raised with them; not resolved.

---

## Where a loader puts you

Base addresses observed, which matter when reading a crash address:

| loader | base |
|---|---|
| Kyty | `0x0000000900000000` |
| orbistoun | `0x0000400000000000` |

A fault address minus the base gives the module-relative offset. `base + small` almost always
means a relocation resolved to zero and had the base added - which is what "no `DT_INIT`" looks
like from the outside.

---

## Unresolved imports: three strategies, and none of them is wrong

What a loader does with an import it cannot satisfy determines what a probe can measure.

- **shadPS4 resolves everything to a generic stub.** Nothing appears unresolved; the address
  census reports ~35,000 of ~35,000 present for libraries it does not implement. This is why
  `900-surface/control` exists and why its failure marks a census `(void)`.
- **Kyty patches each one individually and names it in the log**, which makes its log the best
  available source for *absence* - 252 named missing functions against the current module.
  `obscene-tool unresolved` turns that log into names.
- **PS5PCEM resolves only what it implements.** It answers 3,709 calls where shadPS4 answers
  32,275, and refuses 80.5% of them where shadPS4 returns zero to 96.4%.

**A guest cannot tell a stub from an implementation by looking at an address.** obSCEne's
`obs_address_is_callable()` tests `>= 0x1000`, and Kyty's stub sentinel is `0x200000000` - a
perfectly plausible address. There is no test that separates them, which is the whole argument
for calling things and recording what came back.

---



---

<!-- obscene:claim file=src/probe/harness.c contains=obs_address_is_callable -->
## What a loader does with an import it cannot satisfy - the mechanism

The section above says *what* the three strategies are. This is *how* the third one works,
because the shape of it decides what a probe can survive.

Kyty's is the instructive one, since it is the only loader here that makes the unresolved
case reachable rather than silently harmless. Every declaration in obSCEne is weak, so an
unresolved import is a weak `Func` in the jmprela table, and `relocate()` routes exactly that
case to a generated trampoline:

```cpp
value = reinterpret_cast<Jit::CallPlt*>(program->custom_call_plt_vaddr)->GetAddr(index);
```

`CallPlt` is a table of 16-byte stubs, one per relocation index, each of them:

```
68 <index32>     push  <relocation index>
E9 <rel32>       jmp   <table header>
```

and the header, in turn:

```
49 BB <pltgot64>  movabs r11, pltgot_vaddr
41 FF 73 08       push   QWORD PTR [r11+0x8]     ; the Program pointer, pltgot[1]
41 FF 63 10       jmp    QWORD PTR [r11+0x10]    ; the resolver, pltgot[2]
```

This is ordinary lazy binding: `pltgot[1]` and `pltgot[2]` are the link-map and resolver
slots, and the resolver is entered by `jmp` with two words already pushed. The stack it sees
is `[rsp]` = Program, `[rsp+8]` = index, `[rsp+16]` = the guest's return address - which is
why Kyty's handler reads `stack[-1]`, `stack[0]` and `stack[1]` off a by-value struct.

**Kyty's resolver then prints a stack trace and calls `EXIT`.** Calling a single unimplemented
function ends the process. That is a deliberate and defensible choice for running titles: a
game that reaches an unimplemented function is not going to work, and failing loudly beats
failing subtly two hundred frames later.

It is the opposite of what a conformance probe needs, and there is no way to be defensive
about it from the guest side, because **a guest cannot tell a stub from an implementation by
looking at an address**. `obs_address_is_callable()` tests `>= 0x1000`; a trampoline is a
perfectly ordinary address. That is the whole argument for calling things and recording what
came back. (D176)

### The three shapes, and what each costs a probe

| loader | unresolved import becomes | what a probe loses |
|---|---|---|
| shadPS4 | one generic stub, returns 0 | nothing - but presence becomes meaningless, hence `(void)` |
| fpPS4 | same shape | same |
| PS5PCEM | left unresolved; only what it implements is bound | nothing; the honest case |
| Kyty | a trampoline that terminates the process | everything after the first one |

---

## Two loaders disagreeing is a finding about the platform

The point of running five of these is not a league table. It is that **an internal
contradiction inside one loader is evidence about the interface**, and it shows up nowhere
else.

### Which user id opens a display - unknown

Kyty's `UserServiceGetInitialUser` returns `1`. Kyty's `VideoOutOpen` refuses anything that is
not `255` or `0`. Both are in the same build, so the documented sequence - ask for the initial
user, open the display for that user - cannot succeed.

Somebody's assumption is wrong and there is no way to tell whose from here. `255` is the
system sentinel; a real initial user is presumably a real user id, which is what the `1`
suggests. shadPS4, fpPS4 and PS5PCEM all accept whatever they are handed, so they cast no
vote. **Unmeasured on hardware.**

<!-- obscene:claim file=src/probe/sections/sync.c contains=OBS_MUTEX_TYPE_CANDIDATES 5 -->
### Mutex type constants - one-based, and not the POSIX values

Kyty maps the argument to `scePthreadMutexattrSettype` explicitly:

```cpp
case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
case 2: ptype = PTHREAD_MUTEX_RECURSIVE;  break;
case 3:
case 4: ptype = PTHREAD_MUTEX_NORMAL;     break;
default: EXIT("invalid type: %d\n", type);
```

So the accepted set is `{1, 2, 3, 4}` - and POSIX `PTHREAD_MUTEX_NORMAL` is `0`, which this
rejects outright. `015-sync/mutexattr-round-trip` had been sweeping `0..3`: a quarter of its
range spent on a value one implementation refuses, and never trying one it accepts. Widened to
`0..4`, with `0` kept because a platform refusing it is itself a result. (D177)

This is `IMPLEMENTATIONS` - one emulator's reading. The check still sweeps and reports rather
than encoding the mapping, and hardware settles it.

### `scePthreadMutexattrSettype` on a bad argument

POSIX specifies `EINVAL`. Kyty calls `EXIT`. Terminating the process is not an implementation
of that interface, and it is the second instance of the same pattern as the import trampoline:
an input the loader does not recognise is treated as fatal rather than as an error to return.

---
## What is still unknown

Stated plainly, because a reference that only lists what is known is misleading.

- **Everything here is emulator-derived. There are 0 hardware confirmations.** Every table
  above describes what emulators do, and emulators are somebody's reading of the platform.
- **Whether the hardware calls `DT_INIT` for an executable.** PS5PCEM's comment says the CRT
  does. Nobody here has watched hardware do it.
- **The `.sce_process_param` contents.** obSCEne emits the segment with only a leading size
  field and zeros after it - deliberately, because a wrong value in a field a loader acts on is
  worse than an obviously empty one. Which fields a loader reads is unmeasured.
- **Whether a real loader requires a SELF.** Every emulator here accepts a bare ELF. That may
  be an emulator convenience rather than a platform behaviour.
- **What `sceKernelDirectMemoryQuery`'s second argument selects.** Titles pass `1`; shadPS4
  ignores it entirely (four flag values, byte-identical buffers); PS5PCEM refuses all four with
  EINVAL. See `130-layout/direct-memory-query-flags`.
- **Whether `0xFE00` (fixed-address executable) is usable at all.** Never tried.
- **The entry point's calling convention.** Kyty passes an `EntryParams` block with `argc` and
  `argv`; whether hardware passes the same shape is unverified.
- **Which user id `sceVideoOutOpen` accepts.** Kyty's `UserServiceGetInitialUser` returns `1`
  and its `VideoOutOpen` refuses anything but `255` or `0` - a contradiction inside one build,
  so one of the two is wrong and nothing here says which. The other three loaders accept
  whatever they are handed and cast no vote.
- **The mutex type constants.** One implementation says `{1, 2, 3, 4}` with `0` invalid, which
  is not the POSIX numbering. One reading, unconfirmed. (D177)
- **Whether an unrecognised argument should be an error or a refusal.** POSIX says `EINVAL` for
  a bad mutex type; whether the platform agrees, or validates at all, is unmeasured - and
  "accepts everything" is a real possible answer that would change what several checks mean.

---

---

## Measured on hardware

The first entries in this file that are not emulator-derived. A jailbroken hardware at a known
address, an FTP server on it, and a directory listing - nothing executed, nothing dumped.

### A title is two files

```
app0/eboot.bin             the executable, a SELF
app0/sce_sys/param.json    196 bytes
```

plus whatever `.sprx` libraries it carries. That is the whole shape.

**It is `param.json`, not `param.sfo`.** The previous hardware used a binary `.sfo`; this one
uses JSON, and the difference would have been asserted the wrong way round from memory. Four
fields, from a system application on the hardware:

```json
{
  "applicationCategoryType": 33554432,
  "localizedParameters": {
    "defaultLanguage": "en-US",
    "en-US": { "titleName": "..." }
  },
  "titleId": "..."
}
```

### `/data` is writable, and the sink guessed it right

`obs_sink_paths` lists four candidates and the `sink` record reports which one worked - the
comment beside it says the list "is a set of guesses and the `sink` record says which one was
right, which turns the guessing into a measurement".

`/data` is present and world-writable on hardware, and it is the **first** candidate in that
list. The guess holds; the measurement still has to be taken by a run that gets there.

### A title-registration service is a fourth loader

Hardware running one scans a directory - `/data/homebrew` on this hardware - and registers what it
finds through the platform's own install path, so the title is launched exactly as a retail
title is. That means an app directory is enough to reach the system loader; a package is not
required for it.

It remains worth building anyway, because **the installer is itself a loader** and covering it
is covering a mechanism the other three do not touch. (D180)

### What this does not establish

Everything above is a filesystem observation. **No guest instruction has executed on hardware.**
Every behavioural claim in this file is still emulator-derived, and the tables above have not
moved. The first artifact sent to the hardware was the vendor-shaped module, which went to a
homebrew ELF loader that had no idea what to do with it and exited - a mistake about which
shape goes where, and the reason those shapes now have names that say so.

## The lesson that keeps repeating

Five separate times this week, a mechanism stopped working, reported something reasonable
while it did, and nobody noticed - because review reads reports, and the report looked fine.

- A check that skipped for months while a document called it a pass (D158)
- A sweep index with no denominator, read as 95% when it was 1.1% (D163)
- A comment inside a table row that hid a check from every gate (D168)
- `e_type 0xFE18` documented as "the dynamic-executable type" (D173)
- `obscene-tool compat` exiting non-zero and printing **nothing**, so a run that regenerated
  no table looked like a run that regenerated one - and the stale table was then read out as
  the current result. The tool wanted `--write`; without it, and without `--check`, it had no
  work to do and said so by returning a bare failure code. Fixed to name what it wanted.

Four of the five were caught by **comparing two independent readings of the same fact**. The
other was caught by building someone else's emulator from source and watching it disagree.
That is the method this file is written from.

The fifth has a second lesson on top, and it is about the reader rather than the tool: the
output was piped through `tail`, which discarded the exit code along with the silence. A
diagnostic destroyed while looking at it is worse than one that was never produced, because
the absence is then read as a clean result.
