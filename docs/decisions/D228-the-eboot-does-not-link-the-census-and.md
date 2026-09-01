# D228 - The eboot does not link the census, and the census says so


The change D227 argued for, made. An eboot compiles with `OBS_CENSUS_LINKED=0` and links
`symbols-no-census.txt`:

```text
before   vendor segment 3,587,232 bytes, 1427 tags, 35,518 symbols from 352 libraries
after    vendor segment   104,208 bytes,   75 tags,    204 symbols from  14 libraries
```

The eboot went from 10.5 MB to 619 KB, and `DT_NEEDED` from 352 to 14 - every one of them a
library some check calls by name.

### Only the eboot

`module` and `payload` still link all 352, and that is not an oversight. They run under a
homebrew loader that loads what it is told and under emulators that stub everything, where
taking 35,518 addresses is a free and faithful measurement. The census is not wrong; it is
wrong *for a shape that goes through a system loader*, and that shape is one of four.

### What the census reports instead

Not a sheet of absences. With nothing imported every name would test absent, and a report that
parses, counts, and is wrong throughout is worse than no measurement - so `census()` returns a
skip naming the reason. That is the same distinction the section's own control check exists to
protect: an absence has to mean one thing before a count of absences means anything.

### What it found on the way

`mkmodule` refused the first census-free build by name:

```text
error: 4 imported symbol(s) have no library ...
  sceAgcAcbAcquireMem  sceGnmDrawIndex  sceGnmSubmitCommandBuffers  sceGnmSubmitDone
```

Four symbols that checks **call** were declared only by the census. It worked because the census
imports every name it lists, so the association existed - in the wrong file, and only by
accident of a section that exists to *not* call anything. They are in `src/imports.c` now, under
the libraries the census itself names.

That is the gate doing what a gate is for: a latent inconsistency that no loader would have
reported, surfaced by removing the thing that was hiding it.

### Still to do

This stops the crash; it does not restore the measurement. A census that loads each library at
run time and resolves through it - `sceKernelLoadStartModule`, then `sceKernelDlsym` - reports a
library a title cannot load as a **finding**, which is more than linking ever told us and is the
thing this program exists to produce. Both functions are already declared and in the manifest.

