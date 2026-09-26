# Loader requirements

What a loader has to handle to run an obSCEne module, written from the module side. It
applies to any loader that runs one; none of it is specific to a single emulator.

## Module and payload shapes

Two formats share a file extension:

|  | module | payload |
|---|---|---|
| `e_type` | `0xFE10` (executable) / `0xFE18` (library) | `ET_DYN` (3) |
| Dynamic tags | `DT_SCE_*`, `0x610000xx` | standard `DT_*` |
| Tables live in | `PT_SCE_DYNLIBDATA` | ordinary sections |
| Symbol names | `<nid>#<lib>#<mod>` | plain |
| Dependencies | `DT_SCE_NEEDED_MODULE` | `DT_NEEDED` |

They are separate formats, not variations on one. `e_type` alone tells them apart and is
available before anything else is parsed, so a loader decides once at load and dispatches to
one of two code paths. Branching inside shared code leaves every later function re-deriving
which format it is in. [MODULE-FORMAT.md](MODULE-FORMAT.md) has the table conventions.

## Dynamic table order

Four tags carry a string-table offset in their value rather than a pointer: the module's own
name, its exported library, and the name of every module and library it imports from.

A loader that resolves those offsets while walking the table faults on a module that declares
them before `DT_SCE_STRTAB`. Two ways to handle it:

- Require the string table first, and report it when it is not.
- Walk the table twice - collect the tags, then interpret them. Order then does not matter and
  a malformed module gets an error rather than a crash. This is the better of the two.

## Relocation table pointers

`DT_SCE_JMPREL` holds an offset and `DT_SCE_PLTRELSZ` a size. Swapped, `offset + size` sums to
the same place, so consistency arithmetic cannot catch it, and relocations are read from the
start of the vendor segment, which is the string table. A relocation type that looks like
ASCII points at a table pointer that is really a length.

## Error reporting

A loader that faults on a malformed module and one that faults inside a correct one produce the
same output:

    LoadModuleToMemory: program entry addr ...: 0x00000000051d5000
    Unhandled Exception code 0xc0000005

That is a fault inside the loader with the module's entry point printed one line above, and it
reads as "it started and crashed". A refusal that names the field and the expectation turns a
bisection into a bug report:

    Attempting to add too many segments!
    Unable to find library and module
    Not implemented (jmprel_type != DT_RELA)
    Not implemented (program->dynamic_info->jmprela_table == nullptr)

## Loader checklist

- **Segment count.** One loader accepts three `PT_LOAD` segments and refuses a fourth. A loader
  with a limit states the limit.
- **Both call forms.** A module built with `-fno-plt` routes imports through the GOT
  (`GLOB_DAT` in `DT_SCE_RELA`); an ordinary one uses the linkage table (`JUMP_SLOT` in
  `DT_SCE_JMPREL`). Real modules use the second. A loader that handles only one looks, from
  outside, exactly like a malformed module.
- **Unresolved imports.** Resolving an unknown NID to a stub that returns zero is reasonable.
  Resolving it to a small non-null value is not: a probe that guards with `address != NULL`
  calls it.
- **Unresolved weak imports.** Null is the documented answer and is easy to guard. A valid
  allocated page is indistinguishable from a real function, and the guest calls it. A sentinel
  page that faults on execute is the safe alternative.
- **Output.** One emulator stubs `sceKernelWrite`, `write`, `putchar` and
  `sceKernelDebugOutText` alike, so a guest that runs cannot say so. Another implements `write`
  by returning the byte count and discarding the bytes, so a program choosing a working channel
  picks that one and goes quiet. An unimplemented write fails; it never reports success. One
  real write path is the difference between a conformance run and a run with no evidence.

## What obSCEne provides

A module that resolves its imports by NID and calls platform functions one at a time,
announcing each before the call. A `try` record with no matching `res` names the call that did
not return. [OUTPUT.md](OUTPUT.md) is the report format.

`obscene-tool diff` compares two runs and reports what got worse. A run where everything fails
and nothing changed is a clean exit.

`make module` builds it; [ARTIFACTS.md](ARTIFACTS.md) lists every build and its loader.
