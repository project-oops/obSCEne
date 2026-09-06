/*
 * Disc access: can this process reach the optical drive.
 *
 * # Why this section exists
 *
 * The idea it serves: a background service that notices a PS2 disc going in and hands it to an
 * emulator - homebrew backwards compatibility. That needs two things from the drive: to open
 * the device at all, and to read raw sectors off it. This measures both.
 *
 * The whole point is to run it in each context obSCEne can - a plain payload, a ps4_mode title,
 * a native title - and compare. A payload is the one that matters: it is where a service would
 * live. A title runs inside the sandbox that exists precisely to keep a game away from the raw
 * drive, so a refusal there is the sandbox working as designed and is no cause for concern. The
 * report's own context line (005-generation and the ps4_mode detection) says which case a given
 * run is, so this section only has to measure access and let the comparison happen off the
 * records.
 *
 * # What it measures, honestly
 *
 * **Opening the device is the access question.** Which of the candidate device nodes the
 * process is permitted to open is the permission signal that separates a payload from a
 * sandboxed title. That is the primary result.
 *
 * **Reading a sector is the media question, and a different one.** A read only succeeds if a
 * disc is actually in the drive; with the tray empty it fails with no-media, which says nothing
 * about access. So the read result is reported faithfully and left for a reader who knows
 * whether a disc was loaded - a failure here is not a failure of *access*.
 *
 * # Provenance
 *
 * The device paths are a reasoned hypothesis (a FreeBSD-derived system names its optical drive
 * `cd0`), not a confirmed fact, which is why this is `assumed`. A wrong path fails to open and
 * is reported as unavailable rather than crashing, so the guess is legible either way - the same
 * discipline `150-memory-map` uses for a layout it cannot be sure of.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* One 2 KiB sector is a disc sector, and enough to prove a raw read reached media. */
#define OBS_DISC_SECTOR 2048u

/* Where a FreeBSD-derived system would expose the optical drive and a raw block device. A
 * hypothesis, checked by trying to open it; a path that is not there simply does not open. */
static const char *const disc_devices[] = {
    "/dev/cd0",
    "/dev/da0",
    "/dev/cd1",
};

/* Opens the first candidate device that will open, returning its handle and naming which one
 * through `which`, or a negative handle if none open. */
static int open_any_disc(unsigned int *which) {
    for (unsigned int i = 0; i < OBS_COUNT(disc_devices); i++) {
        int fd = sceKernelOpen(disc_devices[i], OBS_O_RDONLY, 0);
        if (fd >= 0) {
            if (which) {
                *which = i;
            }
            return fd;
        }
    }
    return -1;
}

/*
 * Can the process open the optical device at all - the access question.
 *
 * A handle back from any candidate means yes; none opening means the drive is out of reach in
 * this context, which for a payload is the answer that decides whether disc-driven BC is
 * possible and for a sandboxed title is expected.
 */
static obs_result check_disc_open(void) {
    if (!obs_address_is_callable((const void *)&sceKernelOpen) ||
        !obs_address_is_callable((const void *)&sceKernelClose)) {
        return obs_skip("the file entry points do not resolve in this process");
    }

    unsigned int which = 0;
    int fd = open_any_disc(&which);
    if (fd < 0) {
        return obs_fail("no optical device opened - the drive is out of reach here");
    }
    (void)sceKernelClose(fd);
    /* The value is the index of the device that opened, so a reader can see which node the
     * access was through without a second run. */
    return obs_pass_value((uint64_t)which);
}

/*
 * Given the device opens, can a raw sector be read off it.
 *
 * Reads sector zero. Bytes back proves raw access reached real media; an error is reported with
 * its code and is *not* a failure of access - most often it means the tray was empty when the
 * probe ran, which is a fact about the run, not the permission.
 */
static obs_result check_disc_raw_read(void) {
    if (!obs_address_is_callable((const void *)&sceKernelOpen) ||
        !obs_address_is_callable((const void *)&sceKernelRead) ||
        !obs_address_is_callable((const void *)&sceKernelClose)) {
        return obs_skip("the file entry points do not resolve in this process");
    }

    unsigned int which = 0;
    int fd = open_any_disc(&which);
    if (fd < 0) {
        return obs_skip("no optical device opened, so a read cannot be reached");
    }

    unsigned char sector[OBS_DISC_SECTOR];
    sce_ssize_t got = sceKernelRead(fd, sector, sizeof(sector));
    (void)sceKernelClose(fd);

    if (got < 0) {
        /* Faithfully reported, not judged: an empty tray lands here and is not a lack of
         * access. A reader who loaded a disc reads this as the real result. */
        return obs_partial_value("the device opened but the read did not return data",
                                 (uint64_t)(sce_off_t)got);
    }
    return obs_pass_value((uint64_t)(sce_off_t)got);
}

static const obs_check disc_checks[] = {
    {"045-disc/open", "libkernel", "sceKernelOpen", OBS_CAP_FILE, OBS_CAP_NONE,
     (const void *)&sceKernelOpen, check_disc_open, OBS_FROM_ASSUMED},
    {"045-disc/raw-read", "libkernel", "sceKernelRead", OBS_CAP_FILE, OBS_CAP_NONE,
     (const void *)&sceKernelRead, check_disc_raw_read, OBS_FROM_ASSUMED},
};

const obs_section obs_section_disc = {
    "045-disc",
    "Disc access",
    "Whether a homebrew process may open the optical drive and read raw sectors - the access a "
    "disc-driven backwards-compatibility service needs. A payload is the case that matters; a "
    "sandboxed title being refused is the sandbox working. Opening is the access question, "
    "reading is the media question, and they are reported apart.",
    disc_checks,
    OBS_COUNT(disc_checks),
};
