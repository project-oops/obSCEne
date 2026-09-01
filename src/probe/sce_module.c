/*
 * A minimal module for `/app0/sce_module`, which a title is required to have.
 *
 * # Why this exists
 *
 * With the process parameters supplied (D219) the eboot loads four platform libraries
 * and then the process dies before its entry point with:
 *
 *     # exception: 0xa0020102 (PRX_SCE_MODULE_LOAD_ERROR)
 *     # === Lack of a .prx file in /app0/sce_module is detected!!! ===
 *
 * A title ships its own modules in that directory and the system loads them at startup.
 * A real package carries two there. **Those are vendor modules and this project does
 * not redistribute them**, so what goes in the directory is built here, by this
 * toolchain, out of this file.
 *
 * # What it does, which is nothing
 *
 * It exists to be loaded. `module_start` returns success and `module_stop` returns
 * success, and between them there is deliberately no behaviour at all: a stub that did
 * something would be a second thing that can be wrong while diagnosing the first.
 *
 * The import is not decoration. A module with no imports carries no relocation tables,
 * and this loader requires the full set - `PLTGOT`, `JMPREL`, `PLTRELSZ`, `RELA`,
 * `RELASZ`, `RELAENT` - whether or not there is anything to relocate (D218). So one
 * symbol is referenced, by address and never called, purely so the tables exist.
 */

#include <stddef.h>

/* Declared here rather than included from platform.h, for the same reason `min.c`
 * declares its own: this must not acquire the probe's four hundred declarations by the
 * back door. */
__attribute__((weak)) long sceKernelWrite(int fd, const void *buf, unsigned long len);

int module_start(unsigned long argc, const void *argv);
int module_stop(unsigned long argc, const void *argv);

int module_start(unsigned long argc, const void *argv) {
    /* A call that never happens, which is not the same as no call at all.
     *
     * Taking the symbol's *address* was the first attempt and it is not enough: that is
     * a `GLOB_DAT` relocation in `.rela.dyn`, and the loader wants the
     * procedure-linkage tables too. It counts them and says which are missing:
     *
     *     [rtld] ERROR preprocess_dt_entries:9632: C: ah 1  pg 1  jr 0  pr 1  prs 0  rl
     * 1
     *
     * `jr` and `prs` are `DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ`, and only a real call
     * site produces them. So there is one, behind a condition on a parameter the caller
     * decides - which the compiler cannot fold away and the system will never satisfy,
     * because `module_start` is called with an argument count and not with `ULONG_MAX`.
     * (D222)
     */
    if (&sceKernelWrite != NULL && argc == ~0UL) {
        (void)sceKernelWrite(-1, argv, 0);
    }
    return 0;
}

int module_stop(unsigned long argc, const void *argv) {
    (void)argc;
    (void)argv;
    return 0;
}
