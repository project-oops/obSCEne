/*
 * The kernel, asked about itself by name.
 *
 * # Why this section exists at all
 *
 * Until now no run of this program has ever issued a sysctl, and the gap turned out to be
 * load-bearing somewhere else. The sibling emulator is stuck on precisely one value: a
 * payload asks for `kern.osrelease` exactly once, and on not getting an answer it likes it
 * reports that firmware detection failed and switches a feature off.
 *
 * That emulator will not invent the string, which is the right call - a plausible version
 * would send the guest down a path chosen by a number nobody measured, and the run would
 * look like it worked. So the value has to be measured, and this is the program that runs
 * on the machine that knows it.
 *
 * # Bytes, not a rendering of them
 *
 * Every value is reported as the bytes the platform wrote, the same way
 * `130-layout/system-software-version` reports the firmware string. That is not caution for
 * its own sake: this program does not know the width or encoding of a knob it has never
 * read, and a value printed as text has already had a decision applied to it that cannot be
 * undone by whoever reads the file. Bytes can be decoded later; a bad decode cannot be
 * recovered (D008).
 *
 * A refusal goes to the error-code record for the same reason it does everywhere else - on
 * hardware the code *is* the finding, and a table of real codes is what turns "unimplemented"
 * into "fails the way the caller expects".
 *
 * # Every name is reported, answered or not
 *
 * A name that comes back refused says the platform does not carry that knob, which a
 * reimplementation needs to get right just as much as a value. So the verdict is about the
 * *call* working rather than about any particular name being present.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Longer than any of these values is expected to be, and small enough to sit on the stack
 * beside a guard. A value that does not fit is reported as not fitting rather than
 * truncated into something that reads like a complete answer. */
#define OBS_SYSCTL_MAX 256

/* Written after the buffer and checked afterwards. The length is passed by pointer and a
 * platform may write more than it was told; if that happens here it is a finding in its own
 * right, and everything read afterwards is untrustworthy. */
#define OBS_SYSCTL_GUARD 0x5A

/* The names asked for, and why each earns a call.
 *
 * Deliberately short. Every entry costs a call on hardware and is here because something
 * currently guesses at it or invents it. */
static const char *const obs_sysctl_names[] = {
    /* The one something is actually blocked on. */
    "kern.osrelease",
    /* Says whether the kernel identifies as its upstream at all, which decides how far the
     * documented analogue can be relied on for everything else. */
    "kern.ostype",
    /* The long form, which conventionally carries a build date and configuration. */
    "kern.version",
    /* A core count, which the sibling emulator currently invents. */
    "hw.ncpu",
    /* Assumed rather than measured everywhere it is used. */
    "hw.pagesize",
    /* A second opinion on memory size, from a different subsystem than the one answering
     * sceKernelGetDirectMemorySize - worth having precisely because it is separate. */
    "hw.physmem",
    /* A third independent route to the counter frequency. Two agreeing measurements made it
     * believable; a third from another subsystem would settle it. */
    "machdep.tsc_freq",
};

/* What one name did. Distinguished so the verdict can say which failure happened rather
 * than counting them together. */
typedef enum {
    OBS_SYSCTL_ANSWERED,
    OBS_SYSCTL_REFUSED,
    OBS_SYSCTL_OVERRAN,
} obs_sysctl_outcome;

/* Asks for one name and reports whatever came back. */
static obs_sysctl_outcome obs_sysctl_ask(const char *id, const char *name) {
    unsigned char value[OBS_SYSCTL_MAX + 1];
    for (unsigned int i = 0; i < OBS_SYSCTL_MAX + 1; i++) {
        value[i] = 0;
    }
    value[OBS_SYSCTL_MAX] = OBS_SYSCTL_GUARD;

    size_t len = OBS_SYSCTL_MAX;
    int rc = sysctlbyname(name, value, &len, (void *)0, 0);

    if (value[OBS_SYSCTL_MAX] != OBS_SYSCTL_GUARD) {
        /* Said first, and the value deliberately not reported: the call wrote past a length
         * it was given, so whatever is in the buffer cannot be trusted and printing it would
         * dress corruption up as a measurement. */
        obs_report_error_code("libkernel", "sysctlbyname", name, (uint64_t)(uint32_t)rc);
        return OBS_SYSCTL_OVERRAN;
    }
    if (rc != 0) {
        obs_report_error_code("libkernel", "sysctlbyname", name, (uint64_t)(uint32_t)rc);
        return OBS_SYSCTL_REFUSED;
    }

    /* The length is a measurement in its own right - it says how wide the knob is, which is
     * the difference between an integer and a string before anything decodes either. */
    obs_report_measure(id, name, "length", (uint64_t)len, "bytes");

    if (len > OBS_SYSCTL_MAX) {
        /* The platform reported needing more room than it was given. An answer about the
         * value's size even though the value did not fit, so the length above stands and
         * there are no bytes to print. */
        return OBS_SYSCTL_ANSWERED;
    }
    if (len > 0) {
        obs_report_buffer(id, name, "value", value, (unsigned int)len);
    }
    return OBS_SYSCTL_ANSWERED;
}

/* The name something else is blocked on, asked on its own.
 *
 * Separate from the sweep so it has its own verdict. A value buried in a sweep that mostly
 * refuses is easy to miss, and this one is the reason the section was written.
 */
static obs_result check_sysctl_osrelease(void) {
    OBS_REQUIRE(&sysctlbyname);

    switch (obs_sysctl_ask("135-sysctl/osrelease", "kern.osrelease")) {
    case OBS_SYSCTL_OVERRAN:
        return obs_fail("the call wrote past the length it was given");
    case OBS_SYSCTL_REFUSED:
        return obs_fail("the platform would not answer kern.osrelease");
    case OBS_SYSCTL_ANSWERED:
    default:
        return obs_pass();
    }
}

/* Everything else worth asking, in one pass. */
static obs_result check_sysctl_names(void) {
    OBS_REQUIRE(&sysctlbyname);

    unsigned int answered = 0;
    unsigned int overran = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysctl_names); i++) {
        switch (obs_sysctl_ask("135-sysctl/names", obs_sysctl_names[i])) {
        case OBS_SYSCTL_ANSWERED:
            answered++;
            break;
        case OBS_SYSCTL_OVERRAN:
            overran++;
            break;
        case OBS_SYSCTL_REFUSED:
        default:
            break;
        }
    }

    if (overran > 0) {
        /* Ranked above everything else. A call that writes past a length it was given is a
         * more serious finding than any value it might have returned. */
        return obs_fail_code("a name wrote past the length it was given", (uint64_t)overran);
    }
    if (answered == 0) {
        /* Every name refused. The call resolved, so this says the platform carries none of
         * these knobs - a finding rather than a broken probe. */
        return obs_fail_code("the call resolved but answered nothing",
                             (uint64_t)OBS_COUNT(obs_sysctl_names));
    }
    if (answered < OBS_COUNT(obs_sysctl_names)) {
        return obs_partial_value("some names were refused", (uint64_t)answered);
    }
    return obs_pass_value((uint64_t)answered);
}

static const obs_check sysctl_checks[] = {
    {"135-sysctl/osrelease", "libkernel", "sysctlbyname", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sysctlbyname, check_sysctl_osrelease, OBS_FROM_DERIVED},
    {"135-sysctl/names", "libkernel", "sysctlbyname", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sysctlbyname, check_sysctl_names, OBS_FROM_DERIVED},
};

const obs_section obs_section_sysctl = {
    "135-sysctl",
    "What the kernel says about itself",
    "Named knobs read straight off the platform, reported as the bytes it wrote. Every name "
    "is reported answered or refused, because a knob that is absent is as much a fact about "
    "the machine as one that answers.",
    sysctl_checks,
    OBS_COUNT(sysctl_checks),
};
