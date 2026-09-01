/*
 * The GPU command-building API: libSceGnmDriver, the console-native path to the GPU.
 *
 * # What this probes, and what it deliberately does not
 *
 * The GPU section (160-gpu) measures what the device *computes*, through public Vulkan - the
 * path a Steam Deck takes, which needs no vendor library. This section is the other axis: the
 * console's own GPU *API*, the sceGnm calls a title makes to drive the hardware. It is to the
 * GPU what the CPU NID probes are to the CPU - "does this call exist, is it reachable, what does
 * it do" - which is exactly what "probe every GPU call" was always meant to reach.
 *
 * It probes only the command *builders*, and only two of them. `sceGnmDispatchInitDefaultHardware
 * State` and `sceGnmDispatchDirect` take a caller's buffer and write PM4 command packets into it;
 * they touch no GPU and submit nothing, so calling them is as safe as any buffer-filling call
 * (the same class as 130-layout), and what they write is the PM4 encoding - the bytes a
 * command-processor emulator has to parse. That encoding is the finding: capture it here, and on
 * hardware day diff it against what an emulator produces.
 *
 * # Why only these two (D008)
 *
 * The vendor documents none of this. Their arities are confirmed instead by two independent open
 * reimplementations that agree exactly - shadPS4 and GPCS4 - which is the same standard that let
 * the sceNet transport be built (D107). `sceGnmSetCsShader` is left out precisely because those
 * two sources *disagree* on it (three arguments or four), and D008 forbids calling a function
 * whose arity is uncertain: a wrong one corrupts the stack and surfaces far from here. The
 * submitting calls (`sceGnmSubmitCommandBuffers`) and the shader-binding ones stay in the census,
 * uncalled - reaching a real compute result through Gnm needs a GCN shader this build does not
 * produce, which is a separate workstream.
 *
 * # Where it runs
 *
 * Nowhere, on the host build: libSceGnmDriver is absent, so the harness skips both checks as
 * "not present". It runs inside a loader that provides sceGnm - shadPS4 - the same way the sceNet
 * transport is exercised without a console. The PM4 it records there is that emulator's encoding;
 * on real hardware it is the vendor's, and the two are meant to be diffed.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Oversized on purpose: `sceGnmDispatchInitDefaultHardwareState` reserves 0x100 dwords, so the
 * buffer is comfortably past that, and a guard band behind it catches a call that writes further
 * than any of these should - the one fault this section could cause and not otherwise notice.
 * D008-safe: dumping bytes needs the arity and a buffer larger than the call can fill, not the
 * struct layout. */
#define OBS_GNM_DWORDS 512u
#define OBS_GNM_GUARD 64u
#define OBS_GNM_PATTERN 0xC7u

typedef struct {
    uint32_t cmdbuf[OBS_GNM_DWORDS];
    unsigned char guard[OBS_GNM_GUARD];
} gnm_probe;

static void gnm_prepare(gnm_probe *probe) {
    for (unsigned int i = 0; i < OBS_GNM_DWORDS; i++) {
        probe->cmdbuf[i] = 0u;
    }
    for (unsigned int i = 0; i < OBS_GNM_GUARD; i++) {
        probe->guard[i] = OBS_GNM_PATTERN;
    }
}

static int gnm_guard_intact(const gnm_probe *probe) {
    for (unsigned int i = 0; i < OBS_GNM_GUARD; i++) {
        if (probe->guard[i] != OBS_GNM_PATTERN) {
            return 0;
        }
    }
    return 1;
}

/* The last non-zero dword's index plus one: how many command dwords the call actually wrote,
 * which is the answer to "how big is this packet" that costs the most to establish otherwise. */
static unsigned int gnm_written_dwords(const gnm_probe *probe) {
    unsigned int written = 0;
    for (unsigned int i = 0; i < OBS_GNM_DWORDS; i++) {
        if (probe->cmdbuf[i] != 0u) {
            written = i + 1u;
        }
    }
    return written;
}

/* Dumps the command buffer as bytes, unless the call overran - in which case that is said first
 * and loudly, because nothing after an overrun is trustworthy. Returns 1 when the buffer was
 * dumped and the caller may judge it, 0 when it overran and the caller should stop. */
static int gnm_dump(const char *id, const char *symbol, const gnm_probe *probe, obs_result *bad) {
    if (!gnm_guard_intact(probe)) {
        *bad = obs_fail("the call wrote past the end of its command buffer");
        return 0;
    }
    obs_report_buffer(id, symbol, "pm4", (const unsigned char *)probe->cmdbuf,
                      (unsigned int)sizeof probe->cmdbuf);
    return 1;
}

/* sceGnmDispatchInitDefaultHardwareState(cmdbuf, size): writes the default compute hardware
 * state as PM4 and returns the reserved packet size (0x100 dwords), or 0 if the buffer is too
 * small. The buffer is well past 0x100, so 0 would be a surprise worth a partial. */
static obs_result check_gnm_dispatch_init(void) {
    static gnm_probe probe;
    gnm_prepare(&probe);
    uint32_t reserved = sceGnmDispatchInitDefaultHardwareState(probe.cmdbuf, OBS_GNM_DWORDS);
    obs_result bad;
    if (!gnm_dump("165-gnm/dispatch-init", "sceGnmDispatchInitDefaultHardwareState", &probe,
                  &bad)) {
        return bad;
    }
    if (reserved == 0u) {
        return obs_partial("the call reported the buffer too small and wrote nothing");
    }
    return obs_pass_value((uint64_t)reserved);
}

/* sceGnmDispatchDirect(cmdbuf, size, x, y, z, flags): writes a compute dispatch of x*y*z thread
 * groups as PM4, and requires size to be exactly 9. Returns 0 on success, negative on a rejected
 * argument - a refusal is not a failure of this section, only the call telling us its rule. */
static obs_result check_gnm_dispatch_direct(void) {
    static gnm_probe probe;
    gnm_prepare(&probe);
    int32_t rc = sceGnmDispatchDirect(probe.cmdbuf, 9u, 1u, 1u, 1u, 0u);
    obs_result bad;
    if (!gnm_dump("165-gnm/dispatch-direct", "sceGnmDispatchDirect", &probe, &bad)) {
        return bad;
    }
    if (rc != 0) {
        return obs_partial_value("the call refused; its bytes are recorded anyway",
                                 (uint64_t)(uint32_t)rc);
    }
    unsigned int written = gnm_written_dwords(&probe);
    if (written == 0u) {
        return obs_fail("the call succeeded and wrote nothing");
    }
    return obs_pass_value((uint64_t)written);
}

static const obs_check gnm_checks[] = {
    {"165-gnm/dispatch-init", "libSceGnmDriver", "sceGnmDispatchInitDefaultHardwareState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceGnmDispatchInitDefaultHardwareState,
     check_gnm_dispatch_init, OBS_FROM_ASSUMED},
    {"165-gnm/dispatch-direct", "libSceGnmDriver", "sceGnmDispatchDirect", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceGnmDispatchDirect, check_gnm_dispatch_direct,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_gnm = {
    "165-gnm",
    "GPU command building",
    "Calling the confirmed libSceGnmDriver command-builders and recording the PM4 they encode.",
    gnm_checks,
    OBS_COUNT(gnm_checks),
};
