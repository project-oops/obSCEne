# D267 - The package must stage `sce_module/`, and a rewrite dropped it


**The package must stage `sce_module/`, and a rewrite dropped it.**

A run died on hardware before the probe with `PRX_SCE_MODULE_LOAD_ERROR` - the kernel log's
"Lack of a .prx file in /app0/sce_module is detected". The eboot loaded its four system
libraries and was then refused for having no modules in `/app0/sce_module`.

The cause was not the self-header table or the on-console audit, which a concurrent thread
suspected: those are runtime data and a runtime section, consumed *after* load, and the probe
never runs when the loader rejects the container first. The added `sceKernelGetdents` import is
`libkernel` - a system library already loaded, not a new dependency. None of it touches what the
eboot needs in `sce_module`.

The real cause was `scripts/build-pkg.sh`. When it was rewritten to build the image through
`selfish image --root`, the app tree it stages was reduced to `eboot.bin` alone. `make pkg`
still *built* the stubs into `$BUILD/sce_module` (via the `sce-module` prerequisite), but nothing
copied them into the tree the image is made from - so the filesystem carried the eboot and none
of the `.prx` the loader requires. The script now stages `$BUILD/sce_module`, and refuses with a
clear message if it is absent rather than shipping a package that dies at launch. Verified: the
built image now carries `sce_module/libc.prx` and `sce_module/libSceFios2.prx`.

**On what those `.prx` are, since it looks alarming and is not.** They are **not vendor modules**
and no vendor code enters the repository or the package. They are built by this toolchain from
`src/sce_module.c` - a stub whose `module_start`/`module_stop` return success and do nothing -
and named `libc.prx` / `libSceFios2.prx` only because those are the filenames the loader looks
for. The loader refuses to launch a title with an empty `sce_module`; obSCEne satisfies that
with its own empty modules, the same "fake, and it says so" principle as the SELF and the
package. The real Sony libraries are loaded by the console from its own `/…/common/lib` at
runtime, where obSCEne probes them - never shipped, never in the repo.

