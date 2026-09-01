# D290 - the ctype table accessors get a library assignment


The eboot build failed at `mkmodule` with three imported symbols that had no library, so the module
could not say where to resolve them: `_Getpctype`, `_Getptolower`, `_Getptoupper`. These are the
ctype-table accessors the `isxxx`/`toxxx` macros expand to - `035-libc` and `007-responsive` call
ctype legitimately (a check that exercises libc is the job, per principle 8), and on this build the
macros expanded inline to the accessor symbols rather than to plain calls, leaving the accessors as
undefined references in the eboot ELF.

They were not in the census (`surface.txt`/`surface.h`), so the fix is the simple one the error
names: add them to `src/imports.c` under `libSceLibcInternal`, beside the `toupper`/`tolower`/`isxxx`
entries they back. The mined corpus places them there. No `platform.h` declaration is needed - nothing
calls them by name; they are pulled in by the macros - and no census move is needed since they were
never in a group.

This unblocked the eboot build at both generations. It surfaced only now because the build had been
green earlier the same session and a concurrent change in the probe re-exposed it; the assignment is
correct regardless of what pulled the symbols in.
