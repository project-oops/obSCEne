# D226 - An eboot requires only the libraries the harness needs; the census probes the rest at run time

**Status:** decided
**Date:** 2026-09-26

An eboot links only the libraries the harness itself calls and declares them in `DT_NEEDED`;
`EBOOT_LIBS` fails the build if it would require more. The census (`900-surface`) keeps its symbol
names and, in an eboot, resolves each library at run time through `sceKernelLoadStartModule` and
each symbol through `sceKernelDlsym`, announcing before each and consulting a control (D015) so a
loader without name resolution is told from an absent library. Libraries in
`data/hardware/crashers.txt` are not opened. `module` and `payload` still link the full census.

**Why:** a system loader acts on `DT_NEEDED` before any guest code runs, so a library a title is
not given kills the console with nothing on record - the crash cannot be guarded, because the
program does not exist yet. Requiring a library and probing it are opposite claims. Resolving at
run time turns an unloadable library into a `res` line, which is more than linking ever told.

**Rejected:** linking the whole census into the eboot - a title is given far fewer libraries than
the census names, and the console dies in the loader.
