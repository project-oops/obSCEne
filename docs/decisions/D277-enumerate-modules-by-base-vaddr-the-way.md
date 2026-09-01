# D277 - enumerate modules by base+vaddr, the way past a payload's three shut doors


Status: assumed (pending a hardware run).

In payload mode all three obvious routes to the module list are shut: the *imported* list/info
calls are null (elfldr binds nothing), the *public* `sceKernelGetModuleInfo` is refused
(`0x80020016`, every size, D-note in 110-modules), and the link-map has no `DT_DEBUG`
(111-modlink/D275). But the payload has libkernel's base - the `139-exports` anchor - and
`data/hardware/libkernel-vaddrs.txt` (measured from 12.40 `libkernel_sys.sprx`) carries the
module functions themselves, including `sceKernelGetModuleInfoInternal` (0x36940) - the entry
point the public wrapper calls *after* its argument checks - alongside `sceKernelGetModuleList`
(0x36610). Section `112-modvaddr` reaches both at base+vaddr (the class 139-exports proved safe
to call), lists the handles, and names them through the internal call - past both the unbound
imports and the validation that refused the public one.

This replaced a wrong plan: reaching the process link-map via `_r_debug`. `_r_debug` is not a
libkernel export, so it meant sourcing the rtld binary separately - and it was unnecessary, the
libkernel module calls being right there in a table we already have. Provenance per 139-exports:
the vaddr is a hypothesis, only the behaviour is reported; the list arity is what `platform.h`
declares, the internal call is given the *public* arity (the one assumed thing), and only the
name is read, at the offset 110-modules already assumes - a wrong read shows as rubbish. Guarded
on `obs_libkernel_base`, gated by its own base==0 skip, registered late so a wrong hypothesis
loses only itself. `make host`/`make module` green, gates green; skips on host as designed. The
next payload run is the test of whether the internal entry point enumerates where the public was
refused.

