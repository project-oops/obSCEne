# Module boot sequence

What happens between a loader opening a module and the guest's first instruction, and how
the loaders obSCEne runs under differ at each step. Every statement here is measured from a
run in this repository or cited to a line of open source; `obscene-tool claims` anchors
several of them to source tokens.

## Sequence

```
loader opens the file
  → is it a SELF (signed) or a bare ELF?
  → header checks: e_ident[EI_OSABI], e_ident[EI_ABIVERSION], e_type
  → map PT_LOAD segments at a base address
  → read the dynamic table (the vendor one, not the standard one)
  → resolve imports, or patch what it cannot resolve
  → run module initialisers                    ← DT_INIT, and loaders disagree here
  → fetch the entry point                      ← only from a non-shared module
  → call the entry
```

## `e_type`

| value | name | meaning |
|---|---|---|
| `0xFE00` | `ET_SCE_EXEC` | fixed-address executable |
| `0xFE10` | `ET_SCE_DYNEXEC` | position-independent executable - what `eboot.bin` is |
| `0xFE18` | `ET_SCE_DYNAMIC` | shared library - what a `.prx` is |

Kyty's header states the pair:

```cpp
constexpr Elf64_Half ET_DYNEXEC = 0xfe10; // Executable file
constexpr Elf64_Half ET_DYNAMIC = 0xfe18; // Shared
```

The type decides whether the entry point is called. A loader that respects it treats
`0xFE18` as a library: it runs the module's initialisers and takes the entry from a different,
non-shared module. Kyty makes both halves explicit:

```cpp
// StartAllModules - initialisers, for shared modules only
for (auto* p : m_programs) { if (p->elf->IsShared()) { StartModule(p, …); } }

// GetEntry - an entry, from a non-shared module only
for (const auto* p : m_programs) { if (!p->elf->IsShared()) { return p->elf->GetEntry() + p->base_vaddr; } }
return 0;
```

A `0xFE18` main module on Kyty loads, relocates, runs its initialiser, reports
`Execute: Main`, calls nothing and exits cleanly. shadPS4, fpPS4 and PS5PCEM do not
distinguish the two types and run either. obSCEne's main module is `0xFE10`.

## `EI_ABIVERSION` and `EI_OSABI`

`EI_ABIVERSION` is byte 8 of the ELF header: `0` for the previous hardware generation, `2`
for the current one. A loader reads it before any guest instruction runs, so it is a build
flag (`GEN=4` / `GEN=5`), not a runtime choice.

- shadPS4 is a previous-generation emulator and refuses `2`:
  `IsElfFile: e_ident[EI_ABIVERSION] expected 0x00 is (0x2)` → `not valid ELF file`
- Kyty reads `2` as current-generation (`IsNextGen()` is `e_ident[EI_ABIVERSION] == 2`).
- craziiEmu reads it the same way.

`EI_OSABI` is `0x09` (FreeBSD). `lld` sets it; GNU `ld` does not.

## Vendor dynamic table

A module carries vendor `DT_SCE_*` tags and, in the previous-generation convention, a
`PT_SCE_DYNLIBDATA` segment (`0x61000000`) holding the string, symbol, hash and relocation
tables. A loader ignores the standard tags it does not expect, so a module without the vendor
table loads and resolves nothing. `obscene-tool mkmodule` writes it and `obscene-tool derive`
re-derives the tag assignment from a finished module. [MODULE-FORMAT.md](MODULE-FORMAT.md)
has the field-level detail.

## `DT_INIT`

The module initialiser. Loaders differ on whether they call it for an executable and whether
they guard its absence:

| loader | calls it for an executable | guards it being absent |
|---|---|---|
| shadPS4 | no - `Execute()` goes straight to `RunMainEntry` | n/a |
| fpPS4 | no - the call site is commented out (`ps4_elf.pas:2814`) | only `-1` |
| PS5PCEM | no, by design | yes: must resolve to executable memory |
| Kyty | yes | no |

<!-- obscene:claim file=src/probe/crt.c contains=obs_module_init -->
PS5PCEM's reason, from its source:

> The executable's PS5 CRT entry calls its own `DT_INIT` routine. Calling that routine here as
> well runs global constructors twice; intrusive registration lists then contain the same node
> twice and can become cyclic.

The platform's own CRT is therefore reported to call `DT_INIT`, which makes a module without
one a hardware concern as well as a Kyty one.

Kyty calls it with no check that the tag exists:

```cpp
return run_ini_fini(program->dynamic_info->init_vaddr + program->base_vaddr, …);
// run_ini_fini:
return reinterpret_cast<module_ini_fini_func_t>(addr)(args, argp, func);
```

With no tag, `init_vaddr` is zero and Kyty calls `base_vaddr + 0`, executing the ELF header as
code; the symptom is an access violation at `base + 2`.

A stock linker emits no `DT_INIT`. obSCEne carries an empty initialiser, `obs_module_init`, and
`mkmodule` emits the tag only when that symbol exists. A `DT_INIT` of zero is worse than none,
because zero is what an unguarded loader calls.

## Bare ELF and SELF

A retail `eboot.bin` is a **SELF**: a signed container wrapping the ELF, signed with keys only
the vendor holds.

A **fake SELF** (fSELF) is the same container with a known dummy in place of the signature. A
console set up for homebrew accepts it, and it is the only shape the system loader takes. A
bare ELF reaches a homebrew loader, which maps the segments itself, so a run from a bare ELF
measures the libraries and says nothing about the platform's own load path. (D180)
[ARTIFACTS.md](ARTIFACTS.md) lists which build produces which shape.

Every emulator in the toolkit has a "not a SELF, treat it as an ELF" path:

```
IsSelfFile: Not a SELF file. Magic mismatch current = 0x464c457f expected = 0x1d3d154f
```

## Base addresses

Where each loader maps the module, for reading a crash address:

| loader | base |
|---|---|
| Kyty | `0x0000000900000000` |
| orbistoun | `0x0000400000000000` |

A fault address minus the base is the module-relative offset. `base + small` usually means a
pointer resolved to zero and had the base added, which is how a missing `DT_INIT` looks from
outside.

<!-- obscene:claim file=src/probe/harness.c contains=obs_address_is_callable -->
## Unresolved imports

What a loader does with an import it cannot satisfy decides what a probe can measure.

| loader | unresolved import becomes | what a probe loses |
|---|---|---|
| shadPS4 | one generic stub, returns 0 | nothing - but presence is meaningless, so a failed `900-surface/control` marks the census `(void)` |
| fpPS4 | same shape | same |
| PS5PCEM | left unresolved; only what it implements is bound | nothing |
| Kyty | a trampoline that terminates the process | everything after the first one |

Kyty names each unresolved import in its log, which makes that log the best available source
for absence; `obscene-tool unresolved` turns it into names.

### Kyty's trampoline

Every declaration in obSCEne is weak, so an unresolved import is a weak `Func` in the jmprela
table, and Kyty's `relocate()` routes that case to a generated trampoline:

```cpp
value = reinterpret_cast<Jit::CallPlt*>(program->custom_call_plt_vaddr)->GetAddr(index);
```

`CallPlt` is a table of 16-byte stubs, one per relocation index:

```
68 <index32>     push  <relocation index>
E9 <rel32>       jmp   <table header>
```

and the header:

```
49 BB <pltgot64>  movabs r11, pltgot_vaddr
41 FF 73 08       push   QWORD PTR [r11+0x8]     ; the Program pointer, pltgot[1]
41 FF 63 10       jmp    QWORD PTR [r11+0x10]    ; the resolver, pltgot[2]
```

This is ordinary lazy binding: `pltgot[1]` and `pltgot[2]` are the link-map and resolver
slots, and the resolver is entered by `jmp` with two words pushed. It sees `[rsp]` = Program,
`[rsp+8]` = index and `[rsp+16]` = the guest's return address. The resolver prints a stack
trace and calls `EXIT`, so calling one unimplemented function ends the process.

A guest cannot tell a stub from an implementation by its address. `obs_address_is_callable()`
tests `>= 0x1000`, and Kyty's trampolines and stub sentinel (`0x200000000`) are ordinary
addresses. obSCEne therefore calls each function and records what came back. (D176)

## Loader disagreements

A contradiction inside one loader is evidence about the interface.

### Initial user and display open

Kyty's `UserServiceGetInitialUser` returns `1`, and its `VideoOutOpen` refuses anything other
than `255` or `0`. Both are in the same build, so the sequence "ask for the initial user, open
the display for that user" cannot succeed on it. `255` is the system sentinel. shadPS4, fpPS4
and PS5PCEM accept any value.

<!-- obscene:claim file=src/probe/sections/sync.c contains=OBS_MUTEX_TYPE_CANDIDATES 5 -->
### Mutex type constants

Kyty maps the argument to `scePthreadMutexattrSettype` explicitly:

```cpp
case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
case 2: ptype = PTHREAD_MUTEX_RECURSIVE;  break;
case 3:
case 4: ptype = PTHREAD_MUTEX_NORMAL;     break;
default: EXIT("invalid type: %d\n", type);
```

The accepted set is `{1, 2, 3, 4}`; POSIX `PTHREAD_MUTEX_NORMAL` is `0`, which it rejects.
`015-sync/mutexattr-round-trip` sweeps `0..4`, keeping `0` because a platform refusing it is a
result. The check reports rather than encoding the mapping, since this is one implementation's
reading.

### Invalid arguments

POSIX specifies `EINVAL` for a bad mutex type; Kyty calls `EXIT`. Like the import trampoline,
it treats an unrecognised input as fatal rather than returning an error.

## Title layout on hardware

A title is two files, plus any `.sprx` libraries it carries:

```
app0/eboot.bin             the executable, a SELF
app0/sce_sys/param.json
```

The metadata is `param.json`, not the previous generation's binary `param.sfo`. Its fields,
from a system application:

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

`/data` is present and world-writable, and it is a candidate in the report sink's path list
(`src/probe/sink.c`).

## Title-registration service

A title-registration service scans a directory (`/data/homebrew`) and registers what it finds
through the platform's own install path, so the title launches exactly as a retail title does.
An app directory is therefore enough to reach the system loader. The installer is itself a
loader, and a package covers that mechanism. (D180)
