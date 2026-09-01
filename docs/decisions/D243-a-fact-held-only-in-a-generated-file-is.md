# D243 - A fact held only in a generated file is held nowhere


*status: decided*

`sysctlbyname` was declared in `platform.h` and named `libkernel` by every call site in
`src/sections/sysctl.c`, and it was never added to `src/imports.c`. The build worked anyway,
because the *generated* `corpus.h` still listed it and the manifest check found the association
there.

Regenerating `corpus.h` dropped it - correctly, as a duplicate of a `platform.h` declaration -
and the build broke two steps later with `1 imported symbol(s) have no library`, in a session
that had not touched either file.

The rule in `CLAUDE.md` already says to add an import to `src/imports.c`. What this adds is why
skipping it can appear to work: a generated file was standing in for the manifest, and a
generated file is not a place to keep anything, because the next regeneration is entitled to
remove it.

