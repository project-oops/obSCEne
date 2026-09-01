# D055 - The NID implementation now has an independent 42,010-name confirmation


Status: evidence, not a decision.

`ps4libdoc` publishes 42,010 established symbol names and 1,130,742 NIDs nobody has
recovered a name for. Running `crack` with those names as candidates and our own harvested
corpus as the known set:

```
# candidates 42010
# generator  reproduced 388 of 389 known pairs
# recovered  0 of 1130742
```

**388 of 389 is agreement, not a shortfall.** The one name in our corpus absent from
theirs is `sceAgcCreateShader`. AGC is the PS5 graphics API and that is a PS4 database, so
its absence is the expected result rather than a disagreement. Every name the two sets
share, they agree on.

That is worth stating plainly: this chain started from a single published test vector
(`sceKernelLoadStartModule` → `wzvqT4UqKX8`). It now agrees with a large independent
corpus on every shared name, which is a different order of confidence.

**`recovered 0 of 1130742` is also the expected answer** and should not be read as a
failure. Those NIDs are exactly the residue these names could not crack - a known name
hashing to one of them would not be in the unknown list. That file is a target list for a
candidate *generator*, which is the piece deliberately not built yet.

**What the database does not carry is library association**, and that is what stops the
8,572 vendor names in it from being a census expansion. An import with no library resolves
to nothing, which is why `mkmodule` refuses to build without one. The missing half would
have to come from the OpenOrbis headers, which group by header rather than by library -
close, and not the same thing.

