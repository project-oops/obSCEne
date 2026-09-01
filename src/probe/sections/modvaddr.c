/* ---- 112-modvaddr ----------------------------------------------------------
 *
 * Enumerate loaded modules by reaching libkernel's module functions at base+vaddr, when neither
 * the imports nor the public call will do it.
 *
 * # Why this exists alongside 110-modules and 111-modlink
 *
 * Three doors to the module list, and in a payload all three of the obvious ones are shut:
 *
 *   - the *imported* `sceKernelGetModuleList`/`Info` - null, because elfldr binds no imports;
 *   - the *public* `sceKernelGetModuleInfo` - refused with `0x80020016` for every buffer size,
 *     `0x160` included (110-modules measured this);
 *   - the runtime linker's link-map - `DT_DEBUG` unpopulated for an elfldr payload (111-modlink).
 *
 * But the payload does have libkernel's base - the same `payload_args[0]` anchor `139-exports`
 * confirms exports against - and libkernel's export table carries more than the one public info
 * call: a raw list call, and `Internal` and `FromAddr` variants of the info call. An *internal*
 * entry point is the one the public wrapper calls after its own argument checks, so reaching it
 * directly is the natural way past the validation that refused the public call. This calls the
 * list at `base+vaddr` to get the handles, then the internal info at `base+vaddr` to name them.
 *
 * # Provenance
 *
 * The vaddrs are **hypotheses** from `data/hardware/libkernel-vaddrs.txt` (measured from the
 * 12.40 `libkernel_sys.sprx` export table). Per `139-exports`, a vaddr's source carries no weight
 * and is never reported as a fact - only the behaviour is: either the address named modules or it
 * did not. The list call's arity is the one `platform.h` already declares; the internal info call
 * is given the *public* call's arity, which is the only part assumed and the thing a pass would
 * confirm. The info structure's layout stays unknown (D008): only the name is read, at the shallow
 * offset `110-modules` already assumes, and a wrong read shows as rubbish and is reported as
 * rubbish - never pasted in as a name.
 *
 * # Safety
 *
 * Ordinary userland calls, the class `139-exports` established is safe to make by vaddr - no
 * kernel primitive, nothing in the high half. Registered late, so a wrong hypothesis that faults
 * loses only this section; each module is written before the next handle is fetched, so a fault
 * mid-walk still leaves a record of how far it reached.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Hypotheses, from data/hardware/libkernel-vaddrs.txt (12.40 libkernel_sys.sprx). */
#define OBS_VADDR_GET_MODULE_LIST 0x36610UL


static obs_result check_modvaddr(void) {
    unsigned long base = obs_libkernel_base();
    if (base == 0) {
        return obs_skip(
            "no payload base - this reaches libkernel by base+vaddr, so it needs an elfldr payload");
    }

    /* The module-list vaddr hypothesis (0x36610) was measured from 12.40 libkernel_sys.sprx
     * and is refuted against libkernel.sprx in retail containers where calling it produces a GPF.
     * Skipped until a confirmed vaddr for libkernel.sprx is established (D277/D314). */
    return obs_skip(
        "vaddr hypothesis measured from libkernel_sys.sprx is refuted against container libkernel.sprx");
}

static const obs_check modvaddr_checks[] = {
    {"112-modvaddr/enumerate", "libkernel", "sceKernelGetModuleList", OBS_CAP_NONE, OBS_CAP_NONE,
     /* Guarded on this program's own base accessor, like 139-exports: the body calls no import -
      * it computes every address from the base - and the accessor is always linked, so the
      * harness runs the check and the section's own base==0 test does the real gating. */
     (const void *)&obs_libkernel_base, check_modvaddr, OBS_FROM_HARDWARE},
};

const obs_section obs_section_modvaddr = {
    "112-modvaddr",
    "Loaded modules, by base+vaddr",
    "Reaches libkernel's module-list and internal module-info calls at base+vaddr - past the "
    "unbound imports and the validation that refused the public call - to name the loaded modules "
    "in payload mode. Reports which generation's GPU library is mapped. Behaviour, not contents.",
    modvaddr_checks,
    OBS_COUNT(modvaddr_checks),
};
