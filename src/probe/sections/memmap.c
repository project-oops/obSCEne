/*
 * The platform's memory map, walked to completion.
 *
 * # Why this is worth a section of its own
 *
 * `docs/HARDWARE-PROBE.md` calls this "the single thing blocking a commercial title" on
 * the emulator side: a guest is known to reject a map consisting of one large free
 * region, and nothing is known about what it would accept. A real map ends that
 * immediately, and no amount of reasoning substitutes for it.
 *
 * # The problem this has to solve honestly
 *
 * Walking an enumeration means knowing where the next offset lives in the structure the
 * query returns - and that is a layout, which D008 forbids assuming. A wrong guess here
 * does not fail loudly: it either loops forever on the same region or walks off into
 * nonsense while producing plausible-looking output, which is the worst possible failure
 * for a record whose whole purpose is to be believed later.
 *
 * So the walk **states its hypothesis and checks itself against it**:
 *
 *   * the hypothesis is that the second sixty-four-bit value is where the next query
 *     should start, which is the ordinary shape of such a structure and is what one
 *     emulator's bytes look like;
 *   * every step must *advance* - a query returning an offset at or below the previous
 *     one stops the walk immediately and is reported as `stalled`;
 *   * the raw bytes of every region are dumped regardless, so the authoritative record
 *     survives even if the hypothesis is wrong;
 *   * and the fields are named `first` and `second`, for their position, never `start`
 *     and `end`.
 *
 * If the hypothesis is wrong the walk yields one region and says so. That is a small,
 * honest result rather than a large, confident, wrong one.
 *
 * # Captured twice, deliberately
 *
 * At entry, and again after an allocation. The document asks for both, and the reason is
 * that the *shape of a change* says things a snapshot cannot - which region an allocation
 * came out of, whether the map is split or annotated, whether anything moves.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Bigger than any structure this query is documented to fill, and guarded. */
#define OBS_REGION_BUFFER 128u
#define OBS_REGION_GUARD 32u
#define OBS_REGION_PATTERN 0x5Au

/* A map with more regions than this is either enormous or a walk that is not converging.
 * Either way, stopping and saying how many were seen beats filling a report. */
#define OBS_REGION_MAX 64u

static uint64_t read_u64(const unsigned char *bytes, unsigned int offset) {
    uint64_t value = 0;
    for (unsigned int i = 0; i < 8u; i++) {
        value |= (uint64_t)bytes[offset + i] << (i * 8u);
    }
    return value;
}

/* Walks the map from `start`, dumping each region. Returns how many were seen. */
static unsigned int walk_map(const char *id, sce_off_t start, unsigned int *stalled_at) {
    unsigned char buffer[OBS_REGION_BUFFER + OBS_REGION_GUARD];
    sce_off_t offset = start;
    uint64_t previous = 0;
    unsigned int seen = 0;
    *stalled_at = 0;

    while (seen < OBS_REGION_MAX) {
        for (unsigned int i = 0; i < OBS_REGION_BUFFER; i++) {
            buffer[i] = 0;
        }
        for (unsigned int i = 0; i < OBS_REGION_GUARD; i++) {
            buffer[OBS_REGION_BUFFER + i] = OBS_REGION_PATTERN;
        }

        int rc = sceKernelDirectMemoryQuery(offset, 0, buffer, OBS_REGION_BUFFER);
        if (rc != 0) {
            /* The ordinary end of a walk: a query past the last region is refused. Not a
             * failure, and the code is worth having, so it goes in the error table. */
            obs_report_error_code("libkernel", "sceKernelDirectMemoryQuery",
                                  "offset past the last region",
                                  (uint64_t)(uint32_t)rc);
            break;
        }
        for (unsigned int i = 0; i < OBS_REGION_GUARD; i++) {
            if (buffer[OBS_REGION_BUFFER + i] != OBS_REGION_PATTERN) {
                /* Overran its buffer. Stop at once: everything after this point in memory
                 * is suspect and continuing would compound it. */
                *stalled_at = seen;
                return seen;
            }
        }

        obs_report_buffer(id, "sceKernelDirectMemoryQuery", "region", buffer,
                          OBS_REGION_BUFFER);

        uint64_t first = read_u64(buffer, 0);
        uint64_t second = read_u64(buffer, 8);
        /* The hypothesis, and the check on it. A walk that does not advance is a walk
         * whose idea of "next" is wrong, and one more step would produce the same bytes
         * again forever. */
        int advancing = (seen == 0) || (second > previous);
        obs_report_region(seen, first, second, advancing);
        seen++;

        if (!advancing) {
            *stalled_at = seen;
            break;
        }
        previous = second;
        offset = (sce_off_t)second;
    }
    return seen;
}

static obs_result check_memory_map(void) {
    unsigned int stalled = 0;
    unsigned int seen = walk_map("150-memory-map/walk", 0, &stalled);

    if (seen == 0) {
        return obs_fail("the map could not be queried at all");
    }
    if (stalled != 0) {
        /* The honest outcome when the layout hypothesis does not hold: one or two regions
         * and a statement that the walk stopped, rather than sixty-four plausible lines
         * of nonsense. */
        return obs_partial_value("the walk stopped advancing; the offset hypothesis may "
                                 "be wrong for this platform",
                                 (uint64_t)seen);
    }
    return obs_pass_value((uint64_t)seen);
}

static obs_result check_memory_map_after_allocation(void) {
    OBS_REQUIRE(&sceKernelAllocateDirectMemory, &sceKernelReleaseDirectMemory,
                &sceKernelGetDirectMemorySize);

    /* The same walk with something taken out of it. The document asks for two captures
     * because the difference is what says which region an allocation came from, and
     * whether the map is split, annotated or unchanged - none of which a single snapshot
     * can show. */
    const size_t size = 0x4000;
    sce_off_t physical = 0;
    int rc = sceKernelAllocateDirectMemory(0, (sce_off_t)sceKernelGetDirectMemorySize(),
                                           size, 0x4000, OBS_MEM_TYPE_WB_ONION,
                                           &physical);
    if (rc != 0) {
        return obs_skip("nothing could be allocated, so there is no change to see");
    }

    unsigned int stalled = 0;
    unsigned int seen = walk_map("150-memory-map/after-allocation", 0, &stalled);
    (void)sceKernelReleaseDirectMemory(physical, size);

    if (seen == 0) {
        return obs_fail("the map could not be queried after an allocation");
    }
    /* Deliberately not compared against the first walk. What changed is for a reader
     * diffing two sets of records, and a verdict claiming to know what a difference means
     * would be this check inventing the layout it spent the section avoiding. */
    return obs_pass_value((uint64_t)seen);
}

static const obs_check memmap_checks[] = {
    {"150-memory-map/walk", "libkernel", "sceKernelDirectMemoryQuery", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery, check_memory_map,
     OBS_FROM_ASSUMED},
    {"150-memory-map/after-allocation", "libkernel", "sceKernelDirectMemoryQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery,
     check_memory_map_after_allocation, OBS_FROM_ASSUMED},
};

const obs_section obs_section_memmap = {
    "150-memory-map",
    "The map, walked",
    "Every region the platform will describe, dumped as bytes and listed by position. The "
    "walk states where it believes the next offset lives and stops the moment it stops "
    "advancing, so a wrong guess yields one region rather than sixty-four wrong ones.",
    memmap_checks,
    OBS_COUNT(memmap_checks),
};
