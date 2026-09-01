# Loading an obSCEne module: what a loader has to handle

Written from the module side, after getting obSCEne to load and run under an existing
loader. Everything here is something that was got wrong first and corrected on evidence;
the point is to save the same afternoon twice.

None of it is advice about how to build a loader. It is a list of things a module will do
to you, and it applies to any loader that tries to run one - the observations came from
several, and none of them is specific to the one that prompted writing it down.

## Detect the shape once, at load, and then stop asking

There are two module worlds and they share a file extension:

|  | *module* | *payload* |
|---|---|---|
| `e_type` | `0xFE10` (executable) / `0xFE18` (library) | `ET_DYN` (3) |
| Dynamic tags | `DT_SCE_*`, `0x610000xx` | standard `DT_*` |
| Tables live in | `PT_SCE_DYNLIBDATA` | ordinary sections |
| Symbol names | `<nid>#<lib>#<mod>` | plain |
| Dependencies | `DT_SCE_NEEDED_MODULE` | `DT_NEEDED` |

They are not variations on one format. Deciding once at load and dispatching to two
loaders is the shape that stays honest; branching inside shared code means every later
function has to re-derive which world it is in, and one of them eventually guesses.

`e_type` alone is enough to tell them apart, and it is available before anything else
is parsed.

## The dynamic table is order-sensitive

Four tags carry a **string-table offset** packed into the value rather than a pointer:
the module's own name, its exported library, and the name of every module and library
it imports from.

If you resolve those offsets as you walk the table, a module that declares them before
`DT_SCE_STRTAB` will send you through a base you have not set. obSCEne emitted them
first and every loader it was handed faulted, before any guest instruction, with
nothing logged.

Two ways out, and the second is better:

- Require the string table first, and say so when it is not.
- Walk the table twice - collect tags, then interpret. Then order stops mattering and
  a malformed module gets an error rather than a crash.

## Two integers, and why they cost a day

`DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were swapped here for weeks. The arithmetic that
derived every other tag could not catch it - `offset + size` sums to the same place
whichever is which - so the module confidently told every loader that its relocation
table was located at its own length.

Worth mentioning on the emulator side because of how it presented: relocations read from
the start of the vendor segment, which is the string table, so the "relocation types" in
the logs were fragments of symbol names. If you ever see a relocation type that looks
like ASCII, suspect a table pointer that is really a length.

## Say something when you refuse

The single most expensive property of the loaders obSCEne was tested against is that a
malformed module and a correct one produce the same silence. Every failure looked like:

    LoadModuleToMemory: program entry addr ...: 0x00000000051d5000
    Unhandled Exception code 0xc0000005

That is a fault inside the loader with the module's entry point printed one line above,
which reads as "it started and crashed" and is not what happened.

The failures that were **named** were fixed in minutes:

    Attempting to add too many segments!
    Unable to find library and module
    Not implemented (jmprel_type != DT_RELA)
    Not implemented (program->dynamic_info->jmprela_table == nullptr)

Each names the field and the expectation. That is the difference between a bug report
and a bisection: obSCEne found the ordering rule above by building twenty truncations
of its own dynamic table and running each one, because no loader would say.

## Things worth being explicit about

- **Segment count.** One loader accepts three `PT_LOAD` segments and refuses a fourth
  outright. If there is a limit, say the limit.
- **Both call forms.** A module built with `-fno-plt` routes imports through the GOT
  (`GLOB_DAT` in `DT_SCE_RELA`); an ordinary one uses the linkage table (`JUMP_SLOT` in
  `DT_SCE_JMPREL`). Real modules use the second. Accepting both costs little, and it
  is worth knowing that a loader which quietly handles only one will look, from the
  outside, exactly like a module that is malformed.
- **Unresolved imports.** Resolving an unknown NID to a stub that returns zero is
  reasonable. Resolving it to a small non-null value is not: a probe that guards with
  `address != NULL` will call it.
- **Output, and be careful how you stub it.** One emulator stubs `sceKernelWrite`,
  `write`, `putchar` and `sceKernelDebugOutText` alike, so a guest that runs perfectly
  cannot say so. The other implements `write` by returning the byte count and throwing
  the bytes away - which is worse, because a program selecting a working channel picks
  it and goes quiet. If a write is not implemented, fail it; do not report success.
  Implementing one real write path early is the difference between a conformance run
  and a run that produced no evidence.

- **Unresolved weak imports.** Resolving one to null is the documented answer and is
  easy to guard. One emulator resolves them to a valid allocated page instead, which no
  guest can distinguish from a real function - it will call it. If you use a sentinel,
  a page that faults on execute is far kinder than one that does not.

## What obSCEne gives you in return

A module that loads, resolves several hundred imports by NID, and calls platform
functions one at a time, announcing each before it calls it. A `try` record with no
matching `res` names the call that did not return.

`obscene-tool diff` compares two runs and reports what got *worse*, which is the
question worth asking of an emulator under development. A run where everything fails
and nothing changed is a clean exit.

Build it with `make module`. It needs clang and nothing else.
