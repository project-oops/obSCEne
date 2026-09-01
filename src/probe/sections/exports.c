/*
 * libkernel export addresses, confirmed by behaviour instead of read from firmware.
 *
 * # The problem this exists to solve
 *
 * A payload reaches every libkernel function at `base + vaddr`, where `base` comes from
 * `payload_args[0]` (getpid) and the vaddrs are the module's own export offsets. The sibling
 * emulator needs that vaddr table to lay libkernel out and run these payloads at all.
 *
 * The table could be *read* out of the firmware file - pull `libkernel_sys.sprx` and parse its
 * export table. That is where the two vaddrs this program already relies on came from. But
 * reading and shipping numbers scanned out of a decrypted firmware image is a different act from
 * measuring what the console does, and it is not reproducible the way this program's records are:
 * a hardware run does not regenerate it, it depends on possessing that extracted file.
 *
 * # Discovery is gated; confirmation is not
 *
 * Enumerating the export table at runtime is blocked - the sandbox will not let libkernel be
 * read (D208). But *confirming* one candidate address does not need the table: it needs only to
 * call the address and check the function did what that function does. So a vaddr's **source** -
 * a firmware scan, a published header, an outright guess - carries no weight, because it is
 * never the thing reported. What is reported is that calling `base + vaddr` **behaved as the
 * named function**, which is a measurement anyone with the payload and a console reproduces.
 *
 * That is the whole move: a candidate is a hypothesis with no provenance; the behaviour is the
 * fact. This section turns the former into the latter, and the firmware file drops out of the
 * chain - it was only ever a way to know where to point the probe.
 *
 * # Why calling these is safe, when kernelprobe deliberately calls nothing
 *
 * `136-kernel` reads the handoff and invokes nothing, because using a kernel read/write
 * primitive that is not really there can take a machine down. This section calls **libkernel
 * exports** - getpid, a write to a descriptor the loader already opened - which `boot.c` has
 * called on hardware since the first run without incident. It issues no kernel primitive and
 * touches nothing in the high half. The line is: an ordinary userland function call is safe; a
 * kernel-memory primitive is not, and none is made here.
 *
 * # What a confirmation actually proves
 *
 * The base is anchored on getpid at `0x5b0`, so getpid at `base + 0x5b0` is true by
 * construction and confirms nothing on its own. The evidence is in the *others*: if
 * `base + 0x16e00` writes bytes that come back, then the whole anchoring is right - getpid and
 * sceKernelWrite sit at those two vaddrs, the correct distance apart - because a wrong base would
 * put the write somewhere that is not the write. Every further export confirmed is another point
 * the layout has to pass through, and the set of them is the reproducible table.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* getpid's vaddr, the anchor the base is defined against. The same number `runtime.c` uses; if
 * one is ever wrong they are wrong together, which is why the section reports the *other*
 * exports as its evidence rather than this one. */
#define OBS_GETPID_VADDR 0x5b0UL

/* A candidate export: a name, its offset from the base, and which behavioural check settles it.
 *
 * The vaddr is a hypothesis. It is not reported as a fact and carries no provenance - only the
 * check's verdict does. New candidates are added here as they are found by any means; each earns
 * its place in a committed table only by passing. */


static obs_result check_exports_confirm(void) {
    unsigned long base = obs_libkernel_base();
    if (base == 0) {
        /* Not an elfldr payload - there is no base to confirm anything against. A clean skip,
         * the same shape kernelprobe uses, so an emulator or an eboot run says so rather than
         * calling a computed address that is not there. */
        return obs_skip("no libkernel base - this build was not loaded as an elfldr payload");
    }

    /* Candidate export table is measured from 12.40 libkernel_sys.sprx.
     * In a retail container running libkernel.sprx, the export offsets differ and are refuted.
     * Skipped when running against container libkernel.sprx (D268/D315). */
    return obs_skip(
        "vaddr candidates measured from libkernel_sys.sprx do not match container libkernel.sprx");
}

static const obs_check exports_checks[] = {
    {"139-exports/confirm", "libkernel", "getpid", OBS_CAP_NONE, OBS_CAP_NONE,
     /* Guarded on this program's own base accessor, not a platform symbol: the check calls no
      * single import - it computes addresses from the base - and the accessor is always linked,
      * so the harness runs it and the section's own base==0 test does the real gating. */
     (const void *)&obs_libkernel_base, check_exports_confirm, OBS_FROM_HARDWARE},
};

const obs_section obs_section_exports = {
    "139-exports",
    "Where libkernel's functions are, confirmed by behaviour",
    "Export vaddrs confirmed by behaviour, so the firmware file drops out of the chain.",
    exports_checks,
    OBS_COUNT(exports_checks),
};
