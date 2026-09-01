# D275 - a measured run-context axis, and the loaded-module link-map that feeds it


Status: assumed.

The report already carried two provenance signals - `OBS_FROM_*` on each check (where the
*claim* came from) and `OBS|build` (how the binary was built). Neither says which *environment*
a run measured in, and `build` cannot: the same `payload` binary runs in the ps4 compatibility
host today and, once an injector exists, inside a native process - and reports different things
in each, indistinguishably. So findings from two contexts could not be told apart, and a
"libSceAgc absent" from the BC host read the same as a real absence.

Added a third axis, orthogonal to `OBS_FROM_*`: `OBS|context|<delivery>/<generation>|<basis>`,
emitted next to `build`. Delivery (`host`/`payload`/`title`) comes from the build macro and the
payload anchor; generation (`ps4-bc`/`ps5-native`/`unknown-gpu`) is measured, not assumed, from
which GPU library is actually mapped. `from` is the provenance of the expectation; `context` is
the environment of the measurement.

The generation is read by walking the runtime linker's own link-map (`obs_linkmap_walk` in
runtime.c) - `DT_DEBUG` -> `r_debug.r_map` -> the `link_map` chain, offsets cited to FreeBSD
`<link.h>`/`<sys/link_elf.h>`, the same basis as the `dirent` walk. Syscall-free, so it works
where `sceKernelGetModuleInfo` is refused (measured: `0x80020016` for every size incl `0x160`).
The same walk backs section `111-modlink`, which reports the full inventory. No new imports, so
no surface/census change.

On hardware (the 36,361-record payload run) the walk returned nothing and `context` read
`payload/unknown-gpu`. That is consistent with this build's unbound-imports payload mode (D-note
in WORKLOG, same day): the walk and the ELF-scan resolver share the own-ELF-header find, and when
that fails both do. `obs_linkmap_walk` now takes a `reason` out-param so the next run names the
stage - "own ELF header not resident" (the resolver's cause too) versus "DT_DEBUG not populated"
(a runtime-linker that never processed this object) - which decides whether the fix is the
context or the method. `make host`/`make module` green; the section and the record both skip/mark
`unknown` honestly off a context that cannot answer. The native-host injector is where this walk
is expected to succeed, and `context` is what will prove it did.

