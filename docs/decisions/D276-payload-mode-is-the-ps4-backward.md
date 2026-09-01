# D276 - payload mode is the ps4 backward-compatibility context


Status: assumed (measured, composite).

An elfldr payload - `make payload`, the shape that runs the whole suite on hardware today -
executes in the **PS4 backward-compatibility environment**, not natively. This is the environment
`OBS|context` (D275) names `payload/ps4-bc`. Recorded here because it is the frame every current
hardware report has to be read in, and because it is the thing that decides which findings are
"absent on this platform" versus "absent because this is the compatibility sandbox."

The evidence is composite and consistent:

- **Delivery.** The package is a previous-generation `.pkg`; it installs through the
  compatibility path and is badged as the older hardware (D267 and `scripts/build-pkg.sh`). A
  title installed that way is a PS4 title to the console.
- **The GPU library.** `005-generation` finds the previous generation's driver (`libSceGnm`)
  resolves and the current generation's (`libSceAgc`) does not - and the loader refuses a
  ps4-category title the current-generation libraries with an `EI_ABIVERSION` mismatch it prints
  to the system log (see `src/sections/generation.c`). The two are the same fact: ps4 category,
  ps4 graphics stack, no RDNA2/`libSceAgc`.
- **Imports.** elfldr resolves none; only the base+vaddr-reachable checks survive (`136-kernel`,
  `139-exports`), everything behind an import skips. (WORKLOG, same day.)
- **Introspection is denied three independent ways**, which is what a locked BC sandbox looks
  like: `sceKernelGetModuleInfo` refused (`0x80020016` for every buffer size, `0x160` included),
  `sceKernelDlsym` refused (`0x80020003`), and the runtime-linker debug link-map absent
  (`DT_DEBUG` not populated - elfldr maps the payload without rtld registration, D275).

What this does **not** mean: it is not the signing wall and not a packaging defect - the package
installs and launches. It is the delivery. Native (`ps5-native`) execution - out of the BC
sandbox, `libSceAgc` reachable - requires running obSCEne inside a native-category process, for
which prosperous/elfldr expose no route today (see prosperous's own analysis); the standing plan
is a Ghostpad-class injector into a `launch`-ed native title. Until then, **read every payload
report as ps4-bc**, and let `OBS|context` say so on the line.

