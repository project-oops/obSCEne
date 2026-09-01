# D202 - A mined-corpus name that a section wants to call is declared in the section, not in `platform.h`


`CLAUDE.md` gives five steps for adding a check, and the first is to move the name out of the
census in `surface.h` into a real declaration in `platform.h` - because the census declares
every name as `const char` so the type system forbids calling it, and a name cannot be in both
places.

That step covers the **curated** census. `libSceVideoRecording` is in the **mined** one, and
the two are not symmetric:

- `surface.txt` has an `@called-elsewhere` block, read by `tool/src/surface.rs`, which is how a
  curated name is excluded from the generated census.
- `mined-names.txt` has no equivalent. `tool/src/mine.rs` reads no exclusion list, so a mined
  name cannot be moved out of `corpus.h` the way a curated one can.

Declaring `sceVideoRecordingQueryMemSize` in `platform.h` therefore failed to build, in
`surface.c` - the one translation unit that sees both headers. Four redefinitions, exactly the
breakage the documented step exists to prevent, reached from the direction the step does not
describe.

So the declarations live in `src/sections/record.c`, which `surface.c` does not include, and
the prototypes for the host stubs live beside them in `host_stubs.c` for the same reason.
`src/imports.c` already names this as one of its three kinds of import - *"the handful declared
ad hoc inside a section file"* - so the mapping had a home already.

**What this costs, stated rather than hidden:** the census still probes those four names for
presence while the section calls them. That is a duplication, and it is not a contradiction -
the census asks *is it there*, the section asks *what does it do* - but it is two places
naming one symbol, which is the shape of thing this project usually refuses.

The alternative is an exclusion mechanism for the mined corpus, matching the curated one. That
is the right fix and it is a change to a generator, which is a larger thing than this check
needed. Recorded here so that the second time somebody hits it, the decision is to build the
mechanism rather than to improvise around it again.

