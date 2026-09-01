/*
 * The kernel, reached by number instead of by name.
 *
 * # Why a probe would ever do this
 *
 * Every other section calls a named function and reports what it did. This one goes past the
 * names, because four shipped payloads do - `elfldr`, `pldmgr`, `klogsrv` and `shsrv` all
 * resolve two symbols, build themselves a way into the kernel, and from then on talk to it
 * directly. An emulator that serves every named function perfectly still cannot run any of
 * them, and nothing in a name-based census would say why.
 *
 * # The way in is not an instruction, it is an offset
 *
 * A `syscall` instruction in this program's own text is expected to fail: the platform wants
 * one issued from inside the system library, which is exactly why those payloads do not write
 * one. What they do instead is resolve an ordinary function - `getpid` - take its address,
 * **add ten**, and call that. Ten bytes in is the instruction inside the wrapper rather than
 * the wrapper's prologue.
 *
 * So the offset is the finding as much as anything reached through it. It was read out of four
 * running programs, not guessed, and this is the first chance to see whether the platform
 * agrees. If the gadget works, the convention is real; if it does not, an emulator that built
 * its stubs around it has built them around a misreading.
 *
 * # What is asked once the way in exists
 *
 * Call 649, which is the one that matters. Every one of those payloads asks for it before it
 * will bring up its own runtime linker and gives up when it gets nothing, printing
 * `Unable to initialize rtld`. It is called as `(2, 8, out)` and answers a **pointer**; the
 * caller then reads sixteen bits at offset 0x16 of what that points at and compares them
 * against firmware bands.
 *
 * This program reads none of that. It reports the returned pointer, and dumps the bytes it
 * points at, because a hexdump settles a layout permanently where an interpretation settles
 * nothing (D008).
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* How far into a resolved function the instruction is said to be.
 *
 * Read off four payloads, all of which use the same number. Not special-cased anywhere in this
 * check beyond being the offset it tries - if it is wrong, the call fails and that is the
 * result. */
#define OBS_GADGET_OFFSET 10

/* The call every open-toolchain payload stops on. */
#define OBS_CALL_SYSTEM_VERSION 649

/* Its first two arguments, exactly as the payloads pass them. */
#define OBS_SYSTEM_VERSION_KIND 2
#define OBS_SYSTEM_VERSION_LEN 8

/* How much of the answered structure to dump.
 *
 * Comfortably past the one field anything has been seen to read, so the report says what is
 * around it rather than only what was expected. */
#define OBS_SYSTEM_VERSION_BYTES 64

/* The gadget, once resolved.
 *
 * A function of six arguments whose first is the call number, which is the shape the payloads
 * call it with: they shift their own arguments down one register and let the number land in
 * the accumulator. Declaring it this way and passing the number first reproduces that exactly
 * without this program writing any assembly. */
typedef int64_t (*obs_kernel_call)(int64_t number, int64_t a, int64_t b, int64_t c);

static obs_kernel_call obs_gadget = 0;

/* Resolves the gadget the payloads use, and says whether it is there.
 *
 * Kept separate from the call that uses it so the report distinguishes *could not build a way
 * in* from *the way in did not work* - which are different findings about the platform and
 * would otherwise share one verdict.
 */
static obs_result check_gadget_resolves(void) {
    OBS_REQUIRE(&sceKernelDlsym);

    void *address = 0;
    /* Module 1, as the payloads ask. They fall back to another handle when this fails, and the
     * fallback is not exercised here: if handle 1 does not answer, that is the finding. */
    int rc = sceKernelDlsym(1, "getpid", &address);
    if (rc != 0) {
        return obs_fail_code("the platform would not resolve getpid", (uint64_t)(uint32_t)rc);
    }
    if (address == 0) {
        return obs_fail("resolving getpid reported success and answered nothing");
    }

    obs_report_measure("137-kernelcall/gadget", "sceKernelDlsym", "getpid", (uint64_t)address,
                       "address");
    obs_gadget = (obs_kernel_call)((unsigned char *)address + OBS_GADGET_OFFSET);
    obs_report_measure("137-kernelcall/gadget", "sceKernelDlsym", "gadget",
                       (uint64_t)obs_gadget, "address");
    return obs_pass_value((uint64_t)OBS_GADGET_OFFSET);
}

/* Asks the kernel what system this is, the way a payload does.
 *
 * **This is the check that can end the run**, and deliberately so: it calls into an address
 * derived by arithmetic on another function, which is either the convention four payloads rely
 * on or a jump into the middle of an instruction. The harness announces before attempting, so
 * a `try` with no `res` says precisely that the gadget was not real.
 */
static obs_result check_system_version(void) {
    if (obs_gadget == 0) {
        return obs_skip("no gadget was built, so there is no way to ask");
    }

    /* Zeroed first, so a call that writes nothing is distinguishable from one that writes a
     * null - the two look identical afterwards otherwise. */
    void *answer = 0;
    int64_t rc = obs_gadget(OBS_CALL_SYSTEM_VERSION, OBS_SYSTEM_VERSION_KIND,
                            OBS_SYSTEM_VERSION_LEN, (int64_t)(void *)&answer);

    obs_report_measure("137-kernelcall/system-version", "call-649", "returned",
                       (uint64_t)rc, "code");
    if (rc != 0) {
        return obs_fail_code("the call was refused", (uint64_t)rc);
    }
    if (answer == 0) {
        return obs_partial("the call succeeded and wrote no pointer");
    }

    obs_report_measure("137-kernelcall/system-version", "call-649", "pointer",
                       (uint64_t)answer, "address");
    /* The bytes, uninterpreted. What a consumer wants is the layout, and a dump settles it
     * where a reading of one field does not. */
    obs_report_buffer("137-kernelcall/system-version", "call-649", "info",
                      (const unsigned char *)answer, OBS_SYSTEM_VERSION_BYTES);
    return obs_pass_value((uint64_t)answer);
}

static const obs_check kernelcall_checks[] = {
    {"137-kernelcall/gadget", "libkernel", "sceKernelDlsym", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelDlsym, check_gadget_resolves, OBS_FROM_ASSUMED},
    {"137-kernelcall/system-version", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_system_version, OBS_FROM_ASSUMED},
};

const obs_section obs_section_kernelcall = {
    "137-kernelcall",
    "The kernel, by number",
    "Past the names, the way the payloads go. Builds the gadget they build and asks the one "
    "call every one of them stops on, reporting the bytes it answers with rather than a "
    "reading of them.",
    kernelcall_checks,
    OBS_COUNT(kernelcall_checks),
};
