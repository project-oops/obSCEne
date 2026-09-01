/*
 * Direct memory: reserve physical pages, map them, prove they behave like memory,
 * then give them back.
 *
 * This is the first section that holds state between checks, and it is deliberate.
 * A single allocate-map-write-read-unmap-release round trip exercised as five
 * reported steps says *where* the chain broke; the same thing done inside one check
 * would only say that it did.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* A modest, well-aligned request. Large enough to span more than one page so a
 * loader that only maps the first one is caught, small enough that a constrained
 * host has no excuse. */
#define OBS_ALLOC_LEN (64u * 1024u)
#define OBS_ALLOC_ALIGN (16u * 1024u)

/* Size of buffer passed to sceKernelVirtualQuery to record the hardware layout. */
#define OBS_VQ_BUF_LEN 128u

/* Carried between checks. The physical address is only meaningful while the
 * allocation is held. */
static sce_off_t allocated_at;
static int allocation_held;
static void *mapped_at;
static int mapping_held;

static obs_result check_direct_memory_size(void) {
    size_t size = sceKernelGetDirectMemorySize();
    if (size == 0) {
        return obs_fail("the platform reports no direct memory at all");
    }
    /* Under 256 MiB is not a plausible figure for this class of hardware, and a
     * title sizing its heaps from it would make bad decisions quietly. */
    if (size < (256u * 1024u * 1024u)) {
        return obs_partial_value("direct memory size is implausibly small",
                                 (uint64_t)size);
    }
    return obs_pass_value((uint64_t)size);
}

static obs_result check_allocate(void) {
    OBS_REQUIRE(&sceKernelGetDirectMemorySize);
    sce_off_t physical = 0;
    int rc = sceKernelAllocateDirectMemory(0, (sce_off_t)sceKernelGetDirectMemorySize(),
                                           OBS_ALLOC_LEN, OBS_ALLOC_ALIGN,
                                           OBS_MEM_TYPE_WB_ONION, &physical);
    if (rc != 0) {
        return obs_fail_code("allocation was refused", (uint64_t)(uint32_t)rc);
    }
    if (physical % OBS_ALLOC_ALIGN != 0) {
        /* Honouring the length but not the alignment is a real and subtle failure:
         * everything works until something requires aligned physical memory. */
        allocated_at = physical;
        allocation_held = 1;
        return obs_partial_value("the returned address ignores the requested alignment",
                                 (uint64_t)physical);
    }
    allocated_at = physical;
    allocation_held = 1;
    return obs_pass_value((uint64_t)physical);
}

static obs_result check_map(void) {
    if (!allocation_held) {
        return obs_skip("nothing was allocated to map");
    }
    void *address = NULL;
    int rc = sceKernelMapDirectMemory(&address, OBS_ALLOC_LEN, OBS_PROT_CPU_RW, 0,
                                      allocated_at, OBS_ALLOC_ALIGN);
    if (rc != 0) {
        return obs_fail_code("mapping was refused", (uint64_t)(uint32_t)rc);
    }
    if (address == NULL) {
        /* Success with a null pointer is worse than an error: the caller proceeds. */
        return obs_fail("mapping reported success and returned no address");
    }
    mapped_at = address;
    mapping_held = 1;
    return obs_pass_value((uint64_t)(uintptr_t)address);
}

static obs_result check_mapped_memory_behaves(void) {
    if (!mapping_held) {
        return obs_skip("nothing was mapped to write to");
    }
    volatile unsigned char *p = (volatile unsigned char *)mapped_at;
    /* Touch the first and last byte. A mapping that covers only the first page is a
     * plausible emulator bug and reads perfectly until the moment it does not. */
    p[0] = 0xa5;
    p[OBS_ALLOC_LEN - 1] = 0x5a;
    if (p[0] != 0xa5) {
        return obs_fail("the first byte did not read back");
    }
    if (p[OBS_ALLOC_LEN - 1] != 0x5a) {
        return obs_fail("the last byte did not read back; the mapping is short");
    }
    return obs_pass();
}

static obs_result check_virtual_query_mapped(void) {
    OBS_REQUIRE(&sceKernelVirtualQuery);
    if (!mapping_held) {
        return obs_skip("nothing was mapped to query");
    }

    unsigned char before[OBS_VQ_BUF_LEN];
    unsigned char after[OBS_VQ_BUF_LEN];
    for (unsigned int i = 0; i < OBS_VQ_BUF_LEN; i++) {
        before[i] = 0xAA;
        after[i] = 0xAA;
    }

    int rc = sceKernelVirtualQuery(mapped_at, 0, after, OBS_VQ_BUF_LEN);
    if (rc != 0) {
        return obs_fail_code("virtual query on mapped direct memory refused",
                             (uint64_t)(uint32_t)rc);
    }

    obs_report_written("020-memory/virtual-query-mapped", "sceKernelVirtualQuery",
                       "query_info", before, after, OBS_VQ_BUF_LEN);

    uint64_t start = 0, end = 0;
    for (unsigned int i = 0; i < 8; i++) {
        start |= ((uint64_t)after[i]) << (i * 8);
        end   |= ((uint64_t)after[8 + i]) << (i * 8);
    }

    if (start > (uint64_t)(uintptr_t)mapped_at || end < (uint64_t)(uintptr_t)mapped_at) {
        return obs_partial_value("virtual query range does not enclose mapped address", start);
    }

    return obs_pass_value(end - start);
}

static obs_result check_virtual_query_text(void) {
    OBS_REQUIRE(&sceKernelVirtualQuery);

    unsigned char before[OBS_VQ_BUF_LEN];
    unsigned char after[OBS_VQ_BUF_LEN];
    for (unsigned int i = 0; i < OBS_VQ_BUF_LEN; i++) {
        before[i] = 0xAA;
        after[i] = 0xAA;
    }

    const void *code_ptr = (const void *)&check_virtual_query_text;
    int rc = sceKernelVirtualQuery(code_ptr, 0, after, OBS_VQ_BUF_LEN);
    if (rc != 0) {
        return obs_fail_code("virtual query on code address refused",
                             (uint64_t)(uint32_t)rc);
    }

    obs_report_written("020-memory/virtual-query-text", "sceKernelVirtualQuery",
                       "query_info", before, after, OBS_VQ_BUF_LEN);

    uint64_t start = 0, end = 0;
    for (unsigned int i = 0; i < 8; i++) {
        start |= ((uint64_t)after[i]) << (i * 8);
        end   |= ((uint64_t)after[8 + i]) << (i * 8);
    }
    return obs_pass_value(end - start);
}

static obs_result check_virtual_query_stack(void) {
    OBS_REQUIRE(&sceKernelVirtualQuery);

    unsigned char before[OBS_VQ_BUF_LEN];
    unsigned char after[OBS_VQ_BUF_LEN];
    for (unsigned int i = 0; i < OBS_VQ_BUF_LEN; i++) {
        before[i] = 0xAA;
        after[i] = 0xAA;
    }

    int rc = sceKernelVirtualQuery((const void *)&after[0], 0, after, OBS_VQ_BUF_LEN);
    if (rc != 0) {
        return obs_fail_code("virtual query on stack address refused",
                             (uint64_t)(uint32_t)rc);
    }

    obs_report_written("020-memory/virtual-query-stack", "sceKernelVirtualQuery",
                       "query_info", before, after, OBS_VQ_BUF_LEN);

    uint64_t start = 0, end = 0;
    for (unsigned int i = 0; i < 8; i++) {
        start |= ((uint64_t)after[i]) << (i * 8);
        end   |= ((uint64_t)after[8 + i]) << (i * 8);
    }
    return obs_pass_value(end - start);
}

static obs_result check_virtual_query_unmapped(void) {
    OBS_REQUIRE(&sceKernelVirtualQuery);

    unsigned char before[OBS_VQ_BUF_LEN];
    unsigned char after[OBS_VQ_BUF_LEN];
    for (unsigned int i = 0; i < OBS_VQ_BUF_LEN; i++) {
        before[i] = 0xAA;
        after[i] = 0xAA;
    }

    /* Test address 0x720000240000 - the exact address queried in PPSA25872/orbistoun#D436 */
    const void *unmapped = (const void *)0x720000240000ULL;
    int rc = sceKernelVirtualQuery(unmapped, 0, after, OBS_VQ_BUF_LEN);
    if (rc == 0) {
        obs_report_written("020-memory/virtual-query-unmapped", "sceKernelVirtualQuery",
                           "query_info", before, after, OBS_VQ_BUF_LEN);
        return obs_partial("virtual query on unmapped address reported success");
    }

    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_unmap(void) {
    if (!mapping_held) {
        return obs_skip("nothing was mapped to unmap");
    }
    int rc = sceKernelMunmap(mapped_at, OBS_ALLOC_LEN);
    if (rc != 0) {
        return obs_fail_code("unmapping was refused", (uint64_t)(uint32_t)rc);
    }
    mapping_held = 0;
    mapped_at = NULL;
    return obs_pass();
}

static obs_result check_release(void) {
    if (!allocation_held) {
        return obs_skip("nothing was allocated to release");
    }
    int rc = sceKernelReleaseDirectMemory(allocated_at, OBS_ALLOC_LEN);
    if (rc != 0) {
        return obs_fail_code("release was refused", (uint64_t)(uint32_t)rc);
    }
    allocation_held = 0;
    return obs_pass();
}

static obs_result check_allocate_main(void) {
    OBS_REQUIRE(&sceKernelAllocateMainDirectMemory);
    OBS_REQUIRE(&sceKernelMapDirectMemory);
    OBS_REQUIRE(&sceKernelMunmap);
    OBS_REQUIRE(&sceKernelReleaseDirectMemory);

    sce_off_t physical = 0;
    int rc = sceKernelAllocateMainDirectMemory(OBS_ALLOC_LEN, OBS_ALLOC_ALIGN,
                                               OBS_MEM_TYPE_WB_ONION, &physical);
    if (rc != 0) {
        return obs_fail_code("allocate main direct memory was refused", (uint64_t)(uint32_t)rc);
    }

    void *virt = NULL;
    rc = sceKernelMapDirectMemory(&virt, OBS_ALLOC_LEN, OBS_PROT_CPU_RW, 0,
                                  physical, OBS_ALLOC_ALIGN);
    if (rc != 0 || virt == NULL) {
        sceKernelReleaseDirectMemory(physical, OBS_ALLOC_LEN);
        return obs_fail_code("mapping allocated main direct memory failed", (uint64_t)(uint32_t)rc);
    }

    volatile unsigned char *p = (volatile unsigned char *)virt;
    p[0] = 0x55;
    p[OBS_ALLOC_LEN - 1] = 0xAA;
    int ok = (p[0] == 0x55 && p[OBS_ALLOC_LEN - 1] == 0xAA);

    sceKernelMunmap(virt, OBS_ALLOC_LEN);
    sceKernelReleaseDirectMemory(physical, OBS_ALLOC_LEN);

    if (!ok) {
        return obs_fail("main direct memory read/write test failed");
    }
    return obs_pass_value((uint64_t)physical);
}

static obs_result check_unmap_rejects_null(void) {
    /* Negative check. Unmapping nothing must be an error, not a quiet success. */
    int rc = sceKernelMunmap(NULL, OBS_ALLOC_LEN);
    if (rc == 0) {
        return obs_partial("unmapping a null address reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_flexible_available(void) {
    /* How much the system will lend. Nothing is asserted about the figure - it varies by
     * console, by title and by what is already mapped - only that the call answers and
     * that the answer is not zero, since a platform with no flexible memory at all cannot
     * run anything that asks for some. */
    size_t available = 0;
    int rc = sceKernelAvailableFlexibleMemorySize(&available);
    if (rc != 0) {
        return obs_fail_code("the flexible memory size could not be read",
                             (uint64_t)(uint32_t)rc);
    }
    if (available == 0) {
        return obs_fail("the platform reports no flexible memory at all");
    }
    return obs_pass_value((uint64_t)available);
}

static obs_result check_flexible_configured(void) {
    /* The configured total - the ceiling the available figure above counts down from, and the
     * one that does not move as memory is mapped. Reported as its value so a consumer can seed a
     * flexible-memory budget from it, the same way flexible-available supplies the current
     * figure. Nothing is asserted about the number except that it is not zero: a platform with
     * flexible memory at all cannot configure none of it, and a zero here would be the function
     * answering without meaning to. */
    size_t configured = 0;
    int rc = sceKernelConfiguredFlexibleMemorySize(&configured);
    if (rc != 0) {
        return obs_fail_code("the configured flexible memory size could not be read",
                             (uint64_t)(uint32_t)rc);
    }
    if (configured == 0) {
        return obs_fail("the platform reports no configured flexible memory at all");
    }
    return obs_pass_value((uint64_t)configured);
}

static obs_result check_flexible_round_trip(void) {
    OBS_REQUIRE(&sceKernelReleaseFlexibleMemory);

    /* Map, write, read back, release - the same shape as the direct-memory round trip,
     * against the other allocation path.
     *
     * The difference that matters: no offset is chosen here. The system finds the pages
     * and hands back an address, which is the whole distinction from direct memory and the
     * reason an emulator can implement one and not the other.
     *
     * The write-and-read-back is the part that makes this a positive check rather than a
     * negative one (CLAUDE.md principle 7): an implementation returning a plausible
     * address it has not actually mapped fails here and passes any check that only looks
     * at the return code. */
    const size_t len = 0x4000;
    void *address = NULL;
    int rc = sceKernelMapFlexibleMemory(&address, len, OBS_PROT_CPU_RW, 0);
    if (rc != 0) {
        return obs_fail_code("flexible memory could not be mapped",
                             (uint64_t)(uint32_t)rc);
    }
    if (address == NULL) {
        return obs_fail("mapping reported success and handed back nothing");
    }

    volatile unsigned char *bytes = (volatile unsigned char *)address;
    bytes[0] = 0xA5;
    bytes[len - 1] = 0x5A;
    int held = (bytes[0] == 0xA5 && bytes[len - 1] == 0x5A);

    rc = sceKernelReleaseFlexibleMemory(address, len);
    if (!held) {
        return obs_fail("mapped memory did not hold what was written to it");
    }
    if (rc != 0) {
        return obs_fail_code("flexible memory could not be released",
                             (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)(uintptr_t)address);
}

static const obs_check memory_checks[] = {
    {"020-memory/direct-size", "libkernel", "sceKernelGetDirectMemorySize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelGetDirectMemorySize,
     check_direct_memory_size, OBS_FROM_ASSUMED},
    {"020-memory/allocate", "libkernel", "sceKernelAllocateDirectMemory", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelAllocateDirectMemory, check_allocate, OBS_FROM_ASSUMED},
    {"020-memory/map", "libkernel", "sceKernelMapDirectMemory", OBS_CAP_NONE,
     OBS_CAP_MEMORY, (const void *)&sceKernelMapDirectMemory, check_map, OBS_FROM_ASSUMED},
    {"020-memory/read-write", "obscene", "mapped memory", OBS_CAP_MEMORY, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_mapped_memory_behaves, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-mapped", "libkernel", "sceKernelVirtualQuery", OBS_CAP_MEMORY, OBS_CAP_NONE,
     (const void *)&sceKernelVirtualQuery, check_virtual_query_mapped, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-text", "libkernel", "sceKernelVirtualQuery", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelVirtualQuery, check_virtual_query_text, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-stack", "libkernel", "sceKernelVirtualQuery", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelVirtualQuery, check_virtual_query_stack, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-unmapped", "libkernel", "sceKernelVirtualQuery", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelVirtualQuery, check_virtual_query_unmapped, OBS_FROM_ASSUMED},
    {"020-memory/unmap", "libkernel", "sceKernelMunmap", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelMunmap, check_unmap, OBS_FROM_DERIVED},
    {"020-memory/release", "libkernel", "sceKernelReleaseDirectMemory", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelReleaseDirectMemory, check_release, OBS_FROM_ASSUMED},
    {"020-memory/allocate-main", "libkernel", "sceKernelAllocateMainDirectMemory", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelAllocateMainDirectMemory, check_allocate_main, OBS_FROM_ASSUMED},
    {"020-memory/unmap-rejects-null", "libkernel", "sceKernelMunmap", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelMunmap, check_unmap_rejects_null, OBS_FROM_DERIVED},
    {"020-memory/flexible-available", "libkernel",
     "sceKernelAvailableFlexibleMemorySize", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelAvailableFlexibleMemorySize, check_flexible_available,
     OBS_FROM_ASSUMED},
    {"020-memory/flexible-configured", "libkernel",
     "sceKernelConfiguredFlexibleMemorySize", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelConfiguredFlexibleMemorySize, check_flexible_configured,
     OBS_FROM_ASSUMED},
    {"020-memory/flexible-round-trip", "libkernel", "sceKernelMapFlexibleMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelMapFlexibleMemory,
     check_flexible_round_trip, OBS_FROM_ASSUMED},
};

const obs_section obs_section_memory = {
    "020-memory",
    "Direct memory",
    "A full reserve, map, use, unmap and release cycle, reported step by step.",
    memory_checks,
    OBS_COUNT(memory_checks),
};
