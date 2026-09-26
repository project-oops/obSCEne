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
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "oops/freestd.h"
#include "oops/krw.h"

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
    if (!obs_has_syscall_route() &&
        !obs_address_is_callable((const void *)&sceKernelAllocateDirectMemory)) {
        return obs_skip("no syscall route or library symbol available");
    }
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

    /* Record populated non-zero field offsets beyond offset 0x08
     * (REQ-20260914T1110Z-9b12) */
    for (unsigned int off = 16; off < OBS_VQ_BUF_LEN; off += 8) {
        uint64_t val = 0;
        for (unsigned int i = 0; i < 8; i++) {
            val |= ((uint64_t)after[off + i]) << (i * 8);
        }
        if (val != 0 && val != 0xAAAAAAAAAAAAAAAAULL) {
            char name[32];
            oops_snprintf(name, sizeof(name), "field-0x%02x", off);
            obs_report_measure("020-memory/virtual-query-mapped",
                               "sceKernelVirtualQuery", name, val, "offset-val");
        }
    }

    uint64_t start = 0, end = 0;
    for (unsigned int i = 0; i < 8; i++) {
        start |= ((uint64_t)after[i]) << (i * 8);
        end |= ((uint64_t)after[8 + i]) << (i * 8);
    }

    if (start > (uint64_t)(uintptr_t)mapped_at ||
        end < (uint64_t)(uintptr_t)mapped_at) {
        return obs_partial_value("virtual query range does not enclose mapped address",
                                 start);
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
        end |= ((uint64_t)after[8 + i]) << (i * 8);
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
        end |= ((uint64_t)after[8 + i]) << (i * 8);
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

    /* Test address 0x720000240000 - the exact address queried in
     * PPSA25872/orbistoun#D436 */
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
        return obs_fail_code("allocate main direct memory was refused",
                             (uint64_t)(uint32_t)rc);
    }

    void *virt = NULL;
    rc = sceKernelMapDirectMemory(&virt, OBS_ALLOC_LEN, OBS_PROT_CPU_RW, 0, physical,
                                  OBS_ALLOC_ALIGN);
    if (rc != 0 || virt == NULL) {
        sceKernelReleaseDirectMemory(physical, OBS_ALLOC_LEN);
        return obs_fail_code("mapping allocated main direct memory failed",
                             (uint64_t)(uint32_t)rc);
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
    /* How much the system will lend. Nothing is asserted about the figure - it varies
     * by console, by title and by what is already mapped - only that the call answers
     * and that the answer is not zero, since a platform with no flexible memory at all
     * cannot run anything that asks for some. */
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
    /* The configured total - the ceiling the available figure above counts down from,
     * and the one that does not move as memory is mapped. Reported as its value so a
     * consumer can seed a flexible-memory budget from it, the same way
     * flexible-available supplies the current figure. Nothing is asserted about the
     * number except that it is not zero: a platform with flexible memory at all cannot
     * configure none of it, and a zero here would be the function answering without
     * meaning to. */
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
     * and hands back an address, which is the whole distinction from direct memory and
     * the reason an emulator can implement one and not the other.
     *
     * The write-and-read-back is the part that makes this a positive check rather than
     * a negative one (CLAUDE.md principle 7): an implementation returning a plausible
     * address it has not actually mapped fails here and passes any check that only
     * looks at the return code. */
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

static obs_result check_reserve_virtual_range(void) {
    int (*fn_reserve)(void **, size_t, int, size_t) = NULL;
    if (obs_address_is_callable((const void *)&sceKernelReserveVirtualRange)) {
        fn_reserve = &sceKernelReserveVirtualRange;
    }
    if (fn_reserve == NULL) {
        const payload_args_t *pargs = obs_get_payload_args();
        if (pargs != NULL && pargs->kexport_table != NULL) {
            char nid[12];
            obs_compute_nid("sceKernelReserveVirtualRange", nid);
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                fn_reserve = (int (*)(void **, size_t, int, size_t))ka;
            }
        }
    }
    if (fn_reserve == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *a = NULL;
        if (sceKernelDlsym(0x2001, "sceKernelReserveVirtualRange", &a) == 0 &&
            obs_address_is_callable(a)) {
            fn_reserve = (int (*)(void **, size_t, int, size_t))a;
        } else if (sceKernelDlsym(0x2001, "7oxv3PPCumo", &a) == 0 &&
                   obs_address_is_callable(a)) {
            fn_reserve = (int (*)(void **, size_t, int, size_t))a;
        }
    }
    if (fn_reserve == NULL) {
        const void *sym_self =
            obs_module_symbol(OBS_HANDLE_SELF, "sceKernelReserveVirtualRange");
        if (sym_self != NULL && obs_address_is_callable(sym_self)) {
            fn_reserve = (int (*)(void **, size_t, int, size_t))sym_self;
        }
    }

    obs_report_measure("020-memory/reserve-virtual-range",
                       "sceKernelReserveVirtualRange", "resolved",
                       fn_reserve != NULL ? 1 : 0, "bool");
    if (fn_reserve == NULL) {
        return obs_skip("sceKernelReserveVirtualRange is not available");
    }

    /* 1. Well-formed call:
     * out_addr pointer to a 64-bit zeroed slot, len = 0x100000 (1 MiB), flags = 0,
     * align = 0x40000 (256 KiB) */
    void *out_addr = NULL;
    int rc = fn_reserve(&out_addr, 0x100000, 0, 0x40000);
    obs_report_measure("020-memory/reserve-virtual-range",
                       "sceKernelReserveVirtualRange", "rc", (uint64_t)(uint32_t)rc,
                       "code");
    obs_report_measure("020-memory/reserve-virtual-range",
                       "sceKernelReserveVirtualRange", "out_addr",
                       (uint64_t)(uintptr_t)out_addr, "address");
    obs_report_measure(
        "020-memory/reserve-virtual-range", "sceKernelReserveVirtualRange", "aligned",
        (out_addr != NULL && ((uintptr_t)out_addr % 0x40000 == 0)) ? 1 : 0, "bool");

    /* 2. Negative test: unaligned len (1 byte) */
    void *out_bad_len = NULL;
    int rc_bad_len = fn_reserve(&out_bad_len, 1, 0, 0x40000);
    obs_report_measure("020-memory/reserve-virtual-range", "unaligned-len", "rc",
                       (uint64_t)(uint32_t)rc_bad_len, "code");
    obs_report_measure("020-memory/reserve-virtual-range", "unaligned-len", "out_addr",
                       (uint64_t)(uintptr_t)out_bad_len, "address");

    /* 3. Negative test: unaligned align (3) */
    void *out_bad_align = NULL;
    int rc_bad_align = fn_reserve(&out_bad_align, 0x100000, 0, 3);
    obs_report_measure("020-memory/reserve-virtual-range", "unaligned-align", "rc",
                       (uint64_t)(uint32_t)rc_bad_align, "code");
    obs_report_measure("020-memory/reserve-virtual-range", "unaligned-align",
                       "out_addr", (uint64_t)(uintptr_t)out_bad_align, "address");

    /* Clean up the allocated range if succeeded */
    if (rc == 0 && out_addr != NULL) {
        if (obs_address_is_callable((const void *)&sceKernelMunmap)) {
            (void)sceKernelMunmap(out_addr, 0x100000);
        }
    }
    if (rc_bad_len == 0 && out_bad_len != NULL) {
        if (obs_address_is_callable((const void *)&sceKernelMunmap)) {
            (void)sceKernelMunmap(out_bad_len, 1);
        }
    }
    if (rc_bad_align == 0 && out_bad_align != NULL) {
        if (obs_address_is_callable((const void *)&sceKernelMunmap)) {
            (void)sceKernelMunmap(out_bad_align, 0x100000);
        }
    }

    if (rc == 0 && out_addr != NULL) {
        return obs_pass_value((uint64_t)(uintptr_t)out_addr);
    }
    return obs_fail_code("sceKernelReserveVirtualRange failed", (uint64_t)(uint32_t)rc);
}

static obs_result check_memory_direct_pools_sequence(void) {
    if (!obs_address_is_callable((const void *)&sceKernelGetDirectMemorySize)) {
        return obs_skip("sceKernelGetDirectMemorySize not callable");
    }

    /* 1. sceKernelGetDirectMemorySize() from clean start */
    size_t size1 = sceKernelGetDirectMemorySize();
    obs_report_measure("020-memory/direct-pools-sequence", "query1", "size",
                       (uint64_t)size1, "bytes");

    /* 2. sceKernelAllocateDirectMemory for large span */
    size_t span = 0x40000000UL; /* 1 GiB */
    sce_off_t phys1 = 0;
    int rc1 = -1;
    if (obs_address_is_callable((const void *)&sceKernelAllocateDirectMemory)) {
        rc1 = sceKernelAllocateDirectMemory(0, (sce_off_t)size1, span, 0x200000UL,
                                            OBS_MEM_TYPE_WB_ONION, &phys1);
        if (rc1 != 0) {
            span = 0x10000000UL; /* 256 MiB fallback */
            rc1 = sceKernelAllocateDirectMemory(0, (sce_off_t)size1, span, 0x200000UL,
                                                OBS_MEM_TYPE_WB_ONION, &phys1);
        }
    }
    obs_report_measure("020-memory/direct-pools-sequence", "alloc-direct", "rc",
                       (uint64_t)(uint32_t)rc1, "code");
    obs_report_measure("020-memory/direct-pools-sequence", "alloc-direct", "phys",
                       (uint64_t)phys1, "addr");
    obs_report_measure("020-memory/direct-pools-sequence", "alloc-direct", "span",
                       (uint64_t)span, "bytes");

    /* 3. sceKernelGetDirectMemorySize() again */
    size_t size2 = sceKernelGetDirectMemorySize();
    obs_report_measure("020-memory/direct-pools-sequence", "query2", "size",
                       (uint64_t)size2, "bytes");

    /* 4. sceKernelAllocateMainDirectMemory */
    sce_off_t phys2 = 0;
    int rc2 = -1;
    if (obs_address_is_callable((const void *)&sceKernelAllocateMainDirectMemory)) {
        rc2 = sceKernelAllocateMainDirectMemory(span, 0x200000UL, OBS_MEM_TYPE_WB_ONION,
                                                &phys2);
    }
    obs_report_measure("020-memory/direct-pools-sequence", "alloc-main", "rc",
                       (uint64_t)(uint32_t)rc2, "code");
    obs_report_measure("020-memory/direct-pools-sequence", "alloc-main", "phys",
                       (uint64_t)phys2, "addr");

    /* 5. sceKernelGetDirectMemorySize() third time */
    size_t size3 = sceKernelGetDirectMemorySize();
    obs_report_measure("020-memory/direct-pools-sequence", "query3", "size",
                       (uint64_t)size3, "bytes");

    /* Cleanup allocations */
    if (rc1 == 0 && phys1 != 0 &&
        obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
        sceKernelReleaseDirectMemory(phys1, span);
    }
    if (rc2 == 0 && phys2 != 0 &&
        obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
        sceKernelReleaseDirectMemory(phys2, span);
    }

    return obs_pass();
}

struct obs_batch_map_entry {
    void *vaddr;
    sce_off_t paddr;
    size_t len;
    uint8_t prot;
    uint8_t pad[3];
    uint32_t flags;
};

static obs_result check_gpu_va_window(void) {
    if (!obs_address_is_callable((const void *)&sceKernelBatchMap)) {
        return obs_skip("sceKernelBatchMap is not callable");
    }
    OBS_REQUIRE(&sceKernelAllocateMainDirectMemory);
    OBS_REQUIRE(&sceKernelReleaseDirectMemory);
    OBS_REQUIRE(&sceKernelMunmap);

    sce_off_t paddr = 0;
    size_t page_sz = 0x4000;
    int rc_alloc = sceKernelAllocateMainDirectMemory(page_sz, page_sz,
                                                     OBS_MEM_TYPE_WB_ONION, &paddr);
    obs_report_measure("020-memory/gpu-va-window", "setup", "rc-alloc",
                       (uint64_t)(uint32_t)rc_alloc, "code");
    obs_report_measure("020-memory/gpu-va-window", "setup", "paddr", (uint64_t)paddr,
                       "addr");
    if (rc_alloc != 0 || paddr == 0) {
        return obs_fail_code("direct memory allocation for GPU VA probe failed",
                             (uint64_t)(uint32_t)rc_alloc);
    }

    static const struct {
        const char *tag;
        uint64_t va;
    } candidates[] = {
        {"va-0x40000000", 0x40000000ULL},     /* 1 GiB */
        {"va-0x80000000", 0x80000000ULL},     /* 2 GiB */
        {"va-0x100000000", 0x100000000ULL},   /* 4 GiB */
        {"va-0x200000000", 0x200000000ULL},   /* 8 GiB - anchor */
        {"va-0x240000000", 0x240000000ULL},   /* 9 GiB - unmapped anchor companion */
        {"va-0x400000000", 0x400000000ULL},   /* 16 GiB */
        {"va-0x800000000", 0x800000000ULL},   /* 32 GiB */
        {"va-0x1000000000", 0x1000000000ULL}, /* 64 GiB */
        {"va-0x2000000000", 0x2000000000ULL}, /* 128 GiB */
        {"va-0x4000000000", 0x4000000000ULL}, /* 256 GiB */
        {"va-0x4040000000", 0x4040000000ULL}, /* 257 GiB - unmapped 256 GiB companion */
        {"va-0x8000000000", 0x8000000000ULL}, /* 512 GiB */
        {"va-0x10000000000", 0x10000000000ULL},   /* 1 TiB */
        {"va-0x20000000000", 0x20000000000ULL},   /* 2 TiB */
        {"va-0x40000000000", 0x40000000000ULL},   /* 4 TiB */
        {"va-0x80000000000", 0x80000000000ULL},   /* 8 TiB */
        {"va-0x100000000000", 0x100000000000ULL}, /* 16 TiB */
        {"va-0x400000000000", 0x400000000000ULL}, /* 64 TiB */
        {"va-0x7ffff0000000",
         0x7ffff0000000ULL}, /* ~128 TiB (top of 47-bit lower-canonical) */
        {"va-high-0xffff800000000000",
         0xffff800000000000ULL}, /* Upper canonical base */
        {"va-high-0xffffffffe0000000", 0xffffffffe0000000ULL}, /* Upper canonical max */
    };

    uint64_t lowest_success = 0;
    uint64_t highest_lower_canonical_success = 0;
    int upper_canonical_success = 0;
    int anchor_mapped = 0;

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        uint64_t c_va = candidates[i].va;
        const char *tag = candidates[i].tag;

        int was_already_mapped = 0;
        if (obs_address_is_callable((const void *)&sceKernelVirtualQuery)) {
            unsigned char vq_buf[OBS_VQ_BUF_LEN];
            if (sceKernelVirtualQuery((const void *)(uintptr_t)c_va, 0, vq_buf,
                                      sizeof(vq_buf)) == 0) {
                was_already_mapped = 1;
            }
        }

        int completed = 0;
        int rc = 0;
        int mapped = 0;

        if (was_already_mapped) {
            mapped = 1;
            rc = 0;
            completed = 1;
        } else {
            struct obs_batch_map_entry entry;
            entry.vaddr = (void *)(uintptr_t)c_va;
            entry.paddr = paddr;
            entry.len = page_sz;
            entry.prot = OBS_PROT_GPU_READ | OBS_PROT_GPU_WRITE;
            entry.pad[0] = entry.pad[1] = entry.pad[2] = 0;
            entry.flags = 0;

            rc = sceKernelBatchMap(&entry, 1, &completed);
            mapped = (rc == 0 && completed == 1);
            if (mapped) {
                sceKernelMunmap((void *)(uintptr_t)c_va, page_sz);
            }
        }

        obs_report_measure("020-memory/gpu-va-window", tag, "va", c_va, "addr");
        obs_report_measure("020-memory/gpu-va-window", tag, "rc",
                           (uint64_t)(uint32_t)rc, "code");
        obs_report_measure("020-memory/gpu-va-window", tag, "completed",
                           (uint64_t)completed, "count");
        obs_report_measure("020-memory/gpu-va-window", tag, "mapped", (uint64_t)mapped,
                           "bool");
        obs_report_measure("020-memory/gpu-va-window", tag, "pre-existing",
                           (uint64_t)was_already_mapped, "bool");

        if (mapped) {
            if (c_va == 0x200000000ULL) {
                anchor_mapped = 1;
            }
            if (c_va < 0x800000000000ULL) {
                if (lowest_success == 0 || c_va < lowest_success) {
                    lowest_success = c_va;
                }
                if (c_va > highest_lower_canonical_success) {
                    highest_lower_canonical_success = c_va;
                }
            } else {
                upper_canonical_success = 1;
            }
        }
    }

    obs_report_measure("020-memory/gpu-va-window", "summary", "anchor-mapped",
                       (uint64_t)anchor_mapped, "bool");
    obs_report_measure("020-memory/gpu-va-window", "summary", "low-bound",
                       lowest_success, "addr");
    obs_report_measure("020-memory/gpu-va-window", "summary", "high-lower-canonical",
                       highest_lower_canonical_success, "addr");
    obs_report_measure("020-memory/gpu-va-window", "summary",
                       "upper-canonical-reachable", (uint64_t)upper_canonical_success,
                       "bool");
    uint64_t extent =
        (highest_lower_canonical_success >= lowest_success && lowest_success > 0)
            ? (highest_lower_canonical_success - lowest_success + page_sz)
            : 0;
    obs_report_measure("020-memory/gpu-va-window", "summary", "contiguous-extent",
                       extent, "bytes");

    sceKernelReleaseDirectMemory(paddr, page_sz);

    if (!anchor_mapped) {
        return obs_fail("anchor address 0x200000000 failed to map");
    }
    return obs_pass_value(lowest_success);
}

static const obs_check memory_checks[] = {
    {"020-memory/gpu-va-window", "libkernel", "sceKernelBatchMap", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelBatchMap, check_gpu_va_window,
     OBS_FROM_ASSUMED},
    {"020-memory/reserve-virtual-range", "libkernel", "sceKernelReserveVirtualRange",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelReserveVirtualRange,
     check_reserve_virtual_range, OBS_FROM_ASSUMED},
    {"020-memory/direct-size", "libkernel", "sceKernelGetDirectMemorySize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelGetDirectMemorySize,
     check_direct_memory_size, OBS_FROM_ASSUMED},
    {"020-memory/allocate", "libkernel", "sceKernelAllocateDirectMemory", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelAllocateDirectMemory, check_allocate,
     OBS_FROM_ASSUMED},
    {"020-memory/direct-pools-sequence", "libkernel", "sceKernelAllocateDirectMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelAllocateDirectMemory,
     check_memory_direct_pools_sequence, OBS_FROM_ASSUMED},
    {"020-memory/map", "libkernel", "sceKernelMapDirectMemory", OBS_CAP_NONE,
     OBS_CAP_MEMORY, (const void *)&sceKernelMapDirectMemory, check_map,
     OBS_FROM_ASSUMED},
    {"020-memory/read-write", "obscene", "mapped memory", OBS_CAP_MEMORY, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_mapped_memory_behaves, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-mapped", "libkernel", "sceKernelVirtualQuery",
     OBS_CAP_MEMORY, OBS_CAP_NONE, (const void *)&sceKernelVirtualQuery,
     check_virtual_query_mapped, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-text", "libkernel", "sceKernelVirtualQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelVirtualQuery,
     check_virtual_query_text, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-stack", "libkernel", "sceKernelVirtualQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelVirtualQuery,
     check_virtual_query_stack, OBS_FROM_ASSUMED},
    {"020-memory/virtual-query-unmapped", "libkernel", "sceKernelVirtualQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelVirtualQuery,
     check_virtual_query_unmapped, OBS_FROM_ASSUMED},
    {"020-memory/unmap", "libkernel", "sceKernelMunmap", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelMunmap, check_unmap, OBS_FROM_DERIVED},
    {"020-memory/release", "libkernel", "sceKernelReleaseDirectMemory", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelReleaseDirectMemory, check_release,
     OBS_FROM_ASSUMED},
    {"020-memory/allocate-main", "libkernel", "sceKernelAllocateMainDirectMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelAllocateMainDirectMemory,
     check_allocate_main, OBS_FROM_ASSUMED},
    {"020-memory/unmap-rejects-null", "libkernel", "sceKernelMunmap", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelMunmap, check_unmap_rejects_null,
     OBS_FROM_DERIVED},
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
