/*
 * What the platform wrote, recorded and not interpreted.
 *
 * # This section answers questions rather than asking them
 *
 * Every other section states an expectation and reports whether the platform met it.
 * This one has no expectation. It hands a function a buffer, calls it, and prints the
 * bytes.
 *
 * `docs/HARDWARE-PROBE.md` argues for exactly this and says why: on a console, "did
 * this behave correctly?" is uninteresting because the answer is yes. What cannot be
 * got any other way is *what the platform actually wrote*, and one hexdump settles a
 * structure layout permanently where four experiments guessing at it settle nothing.
 *
 * # Why this is allowed under D008
 *
 * D008 forbids calling a function whose structure layout is unknown, because a wrong
 * layout produces a call that succeeds and does the wrong thing silently. That is a
 * rule about *expectations* - about naming fields and reading them back at offsets
 * nobody confirmed.
 *
 * Dumping bytes needs no layout. It needs the arity, and a buffer larger than anything
 * the call could plausibly write. Both are obtainable: the arities below are read from
 * a published toolchain header, and the buffer is deliberately oversized and zeroed.
 *
 * So this section unblocks the surface BACKLOG §2 records as blocked, without weakening
 * the rule that blocked it. It never claims to know what a byte means.
 *
 * # The buffer is oversized on purpose, and that has a cost
 *
 * A function that writes more than the buffer holds corrupts the stack, and no header
 * can promise it will not. 256 bytes is far past the largest structure any of these is
 * documented to fill, and the guard bytes after it are checked - an overrun is reported
 * rather than silently trusted, which is the one failure this design could otherwise
 * hide.
 *
 * # Verdicts here are weak on purpose
 *
 * A pass means "it returned and wrote something", which is nearly the least this could
 * say. The value is in the `bytes` records, not the verdict, and inflating the verdict
 * would put a green line in a tally for what is really a measurement.
 */

#include "obscene/display.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "oops/krw.h"
void *memset(void *dest, int c, size_t n);

OBS_WEAK int sceVideoOutGetOutputStatus(int handle, void *status);
OBS_WEAK int sceVideoOutGetVblankStatus(int handle, void *status);
OBS_WEAK int sceVideoOutWaitVblank(int handle);
OBS_WEAK int sceKernelGetModuleInfoForUnwind(const void *addr, void *info);
OBS_WEAK int sceKernelQueryMemoryProtection(const void *addr, uint64_t *start,
                                            uint64_t *end, uint32_t *prot);

/* Far larger than anything these calls are documented to write. */
#define OBS_LAYOUT_BUFFER 256u
/* Written after the buffer and checked afterwards. A call that writes past its buffer
 * is the one fault this section could otherwise cause and not notice. */
#define OBS_LAYOUT_GUARD 64u
#define OBS_LAYOUT_PATTERN 0xC7u

/* The buffer is poisoned, not zeroed, and a copy of the poison is kept beside it.
 *
 * Zeroing loses a fact worth having on hardware: a field the platform writes as zero is
 * indistinguishable from a field it never touched. Both are a zero in the dump, and
 * only one of them means the field exists. A reserved word, a count of none, a null
 * pointer - all written, all invisible.
 *
 * Poisoning makes "changed" the test instead of "non-zero", and `obs_report_written`
 * compares the two. A byte the platform happens to write equal to the poison still
 * reads as untouched, which no single pattern avoids - so `obs_layout_patterns` holds
 * two and callers that care run both and take the union. (D008: the platform draws the
 * line, not us.) */
static const unsigned char obs_layout_patterns[] = {0xC7u, 0x3Au};

typedef struct {
    unsigned char buffer[OBS_LAYOUT_BUFFER];
    unsigned char guard[OBS_LAYOUT_GUARD];
    unsigned char before[OBS_LAYOUT_BUFFER];
} layout_probe;

static void layout_prepare_with(layout_probe *probe, unsigned char poison) {
    for (unsigned int i = 0; i < OBS_LAYOUT_BUFFER; i++) {
        probe->buffer[i] = poison;
        probe->before[i] = poison;
    }
    for (unsigned int i = 0; i < OBS_LAYOUT_GUARD; i++) {
        probe->guard[i] = OBS_LAYOUT_PATTERN;
    }
}

static void layout_prepare(layout_probe *probe) {
    layout_prepare_with(probe, obs_layout_patterns[0]);
}

static int layout_guard_intact(const layout_probe *probe) {
    for (unsigned int i = 0; i < OBS_LAYOUT_GUARD; i++) {
        if (probe->guard[i] != OBS_LAYOUT_PATTERN) {
            return 0;
        }
    }
    return 1;
}

/* The common shape: report what was written, then say the least the verdict can. */
static obs_result layout_report(const char *id, const char *symbol, const char *what,
                                const layout_probe *probe, int rc) {
    if (!layout_guard_intact(probe)) {
        /* Reported loudly and before anything else: the bytes below are still printed,
         * but a call that overran its buffer has already written somewhere it should
         * not and nothing after this point is trustworthy. */
        return obs_fail("the call wrote past the end of its buffer");
    }
    obs_report_written(id, symbol, what, probe->before, probe->buffer,
                       OBS_LAYOUT_BUFFER);

    /* The verdict counts what changed from the poison, exactly as the dump above does -
     * not what is non-zero. The dump was moved onto the poison basis and this counter
     * was left on the old one, so the two disagreed on the single case the poison
     * exists to catch: a call that writes zeroes emits a full `changed` dump and a
     * "wrote nothing" verdict in the same breath. `written` is the extent, the last
     * byte the call actually touched. (D008) */
    unsigned int written = 0;
    for (unsigned int i = 0; i < OBS_LAYOUT_BUFFER; i++) {
        if (probe->before[i] != probe->buffer[i]) {
            written = i + 1u;
        }
    }
    if (rc != 0) {
        /* Not a failure of this section. A call that refuses still tells us its refusal
         * code, and refusing is often correct for the arguments available here. */
        return obs_partial_value("the call refused; its bytes are recorded anyway",
                                 (uint64_t)(uint32_t)rc);
    }
    if (written == 0) {
        return obs_fail("the call succeeded and wrote nothing");
    }
    return obs_pass_value((uint64_t)written);
}

static obs_result check_direct_memory_query(void) {
    int (*fn_query)(sce_off_t, int, void *, size_t) = NULL;
    if (obs_address_is_callable((const void *)&sceKernelDirectMemoryQuery)) {
        fn_query = &sceKernelDirectMemoryQuery;
    }
    if (fn_query == NULL) {
        const payload_args_t *pargs = obs_get_payload_args();
        if (pargs != NULL && pargs->kexport_table != NULL) {
            char nid[12];
            obs_compute_nid("sceKernelDirectMemoryQuery", nid);
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                fn_query = (int (*)(sce_off_t, int, void *, size_t))ka;
            }
        }
    }
    if (fn_query == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *a = NULL;
        if (sceKernelDlsym(0x2001, "sceKernelDirectMemoryQuery", &a) == 0 &&
            obs_address_is_callable(a)) {
            fn_query = (int (*)(sce_off_t, int, void *, size_t))a;
        } else if (sceKernelDlsym(0x2001, "BHouLQzh0X0", &a) == 0 &&
                   obs_address_is_callable(a)) {
            fn_query = (int (*)(sce_off_t, int, void *, size_t))a;
        }
    }
    if (fn_query == NULL) {
        const void *sym_self =
            obs_module_symbol(OBS_HANDLE_SELF, "sceKernelDirectMemoryQuery");
        if (sym_self != NULL && obs_address_is_callable(sym_self)) {
            fn_query = (int (*)(sce_off_t, int, void *, size_t))sym_self;
        }
    }

    obs_report_measure("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                       "resolved", fn_query != NULL ? 1 : 0, "bool");
    if (fn_query == NULL) {
        return obs_skip("sceKernelDirectMemoryQuery is not available");
    }

    unsigned char buf[64];

    /* (1) Query offset 0x0 with flags = 0, passing a 64-byte buffer pre-filled with
     * 0xAA */
    memset(buf, 0xAA, sizeof(buf));
    for (unsigned int off = 0; off < 64u; off += 16u) {
        obs_report_bytes("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "before-flags-0", off, &buf[off], 16u);
    }
    int rc0 = fn_query(0, 0, buf, sizeof(buf));
    obs_report_measure("130-layout/direct-memory-query", "offset-0-flags-0", "rc",
                       (uint64_t)(uint32_t)rc0, "code");
    for (unsigned int off = 0; off < 64u; off += 16u) {
        obs_report_bytes("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "after-flags-0", off, &buf[off], 16u);
    }

    uint64_t f0_start = 0, f0_field1 = 0, f0_flags = 0;
    if (rc0 == 0) {
        memcpy(&f0_start, &buf[0], 8);
        memcpy(&f0_field1, &buf[8], 8);
        memcpy(&f0_flags, &buf[16], 8);
        obs_report_measure("130-layout/direct-memory-query", "flags-0-field-0-start",
                           "raw", f0_start, "address");
        obs_report_measure("130-layout/direct-memory-query",
                           "flags-0-field-1-extent-or-end", "raw", f0_field1, "raw");
        obs_report_measure("130-layout/direct-memory-query", "flags-0-field-2-flags",
                           "raw", f0_flags, "raw");
    }

    /* (2) Query offset 0x0 with flags = 1 (next-region walk) */
    memset(buf, 0xAA, sizeof(buf));
    for (unsigned int off = 0; off < 64u; off += 16u) {
        obs_report_bytes("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "before-flags-1", off, &buf[off], 16u);
    }
    int rc1 = fn_query(0, 1, buf, sizeof(buf));
    obs_report_measure("130-layout/direct-memory-query", "offset-0-flags-1", "rc",
                       (uint64_t)(uint32_t)rc1, "code");
    for (unsigned int off = 0; off < 64u; off += 16u) {
        obs_report_bytes("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "after-flags-1", off, &buf[off], 16u);
    }

    uint64_t f1_start = 0, f1_field1 = 0;
    if (rc1 == 0) {
        memcpy(&f1_start, &buf[0], 8);
        memcpy(&f1_field1, &buf[8], 8);
        obs_report_measure("130-layout/direct-memory-query", "flags-1-field-0-start",
                           "raw", f1_start, "address");
        obs_report_measure("130-layout/direct-memory-query",
                           "flags-1-field-1-extent-or-end", "raw", f1_field1, "raw");

        /* Query subsequent region to clarify field 2: region_end vs region_size */
        uint64_t next_probe_off =
            (f0_field1 > f0_start) ? f0_field1 : (f0_start + f0_field1);
        memset(buf, 0xAA, sizeof(buf));
        int rc_subseq = fn_query((sce_off_t)next_probe_off, 1, buf, sizeof(buf));
        obs_report_measure("130-layout/direct-memory-query", "subsequent-query", "rc",
                           (uint64_t)(uint32_t)rc_subseq, "code");
        if (rc_subseq == 0) {
            uint64_t sub_start = 0, sub_field1 = 0;
            memcpy(&sub_start, &buf[0], 8);
            memcpy(&sub_field1, &buf[8], 8);
            obs_report_measure("130-layout/direct-memory-query", "subseq-field-0-start",
                               "raw", sub_start, "address");
            obs_report_measure("130-layout/direct-memory-query",
                               "subseq-field-1-extent-or-end", "raw", sub_field1,
                               "raw");
            int is_absolute_end = (sub_field1 > sub_start && sub_start > 0);
            obs_report_measure("130-layout/direct-memory-query",
                               "field-1-is-absolute-end", "verdict",
                               (uint64_t)is_absolute_end, "bool");
        }
    }

    /* (3) Query past last valid physical address */
    size_t total_dmem = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetDirectMemorySize)) {
        total_dmem = sceKernelGetDirectMemorySize();
    }
    if (total_dmem == 0) {
        total_dmem = (size_t)16ULL * 1024ULL * 1024ULL * 1024ULL;
    }
    sce_off_t past_addr = (sce_off_t)total_dmem + (sce_off_t)0x10000000L;
    memset(buf, 0xAA, sizeof(buf));
    int rc_past0 = fn_query(past_addr, 0, buf, sizeof(buf));
    obs_report_measure("130-layout/direct-memory-query", "past-last-address-flags-0",
                       "rc", (uint64_t)(uint32_t)rc_past0, "code");

    memset(buf, 0xAA, sizeof(buf));
    int rc_past1 = fn_query(past_addr, 1, buf, sizeof(buf));
    obs_report_measure("130-layout/direct-memory-query", "past-last-address-flags-1",
                       "rc", (uint64_t)(uint32_t)rc_past1, "code");

    /* Also execute baseline layout probe */
    layout_probe probe;
    layout_prepare(&probe);
    int rc_base = fn_query(0, 0, probe.buffer, OBS_LAYOUT_BUFFER);
    return layout_report("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "info", &probe, rc_base);
}

/* What does the second argument select?
 *
 * # The question, and who is asking
 *
 * `sceKernelDirectMemoryQuery` is the most-called function in the corpus mined from
 * real titles - 87.6 million calls in one of them, ~99.9% of every call it makes. The
 * guest walks the memory map, refuses what it is shown, and walks it again. Whoever
 * implements this has to get the *contents of the destination buffer* right, and the
 * second argument selects something about what goes in it.
 *
 * Every observed call passes `1` there. `130-layout/direct-memory-query` above passes
 * `0`, which means the dump this program has been collecting answers a different call
 * from the one titles actually make.
 *
 * # Why this needs no console, and no layout
 *
 * Nothing here is compared against an expectation. The buffers are dumped and the
 * difference between them *is* the answer: whatever the flag selects is whatever
 * changes. That makes this the same instrument as the rest of `130-layout` - bytes, not
 * verdicts - and it is a measurement of whichever implementation runs it. An emulator
 * today says what that emulator does, which is worth having on its own, and the same
 * check on hardware later says what the platform does.
 *
 * It also stays clear of D008. No structure is named, no field is read, and the arity
 * is the one already established from the OpenOrbis toolchain headers. Adding a value
 * to an `int` argument invents nothing.
 *
 * # Four values, and why these four
 *
 * `0` for the baseline the existing check uses, `1` for the one titles pass, and `2`
 * and `4` because a flag argument that means anything is usually a bit set, and two
 * more single bits distinguish "a bit field" from "an enumerated selector" without
 * guessing what any bit means. Widening it further is a decision to make after these
 * four have said something.
 */

/* One quantity name per value, because a report emitting four dumps under one name
 * cannot say which flag produced which. A table rather than formatting: the runtime has
 * no string formatting and should not grow any for this. */
static const int obs_query_flags[] = {0, 1, 2, 4};
static const char *const obs_query_flag_name[] = {
    "flags-0",
    "flags-1",
    "flags-2",
    "flags-4",
};

static obs_result check_direct_memory_query_flags(void) {
    unsigned int answered = 0;
    unsigned int differing = 0;
    unsigned int refused = 0;
    /* The first buffer, kept to compare the rest against. Comparison happens here
     * rather than in a reader because "did anything change" is the whole finding, and a
     * report that only carried four hexdumps would leave every consumer to compute it -
     * including the ones that would get it wrong by comparing the guard or the trailing
     * zeroes. */
    unsigned char baseline[OBS_LAYOUT_BUFFER];
    int have_baseline = 0;

    for (unsigned int i = 0; i < OBS_COUNT(obs_query_flags); i++) {
        layout_probe probe;
        layout_prepare(&probe);
        int rc = sceKernelDirectMemoryQuery(0, obs_query_flags[i], probe.buffer,
                                            OBS_LAYOUT_BUFFER);
        if (!layout_guard_intact(&probe)) {
            /* Stop immediately. A call that overran its buffer has already written
             * somewhere it should not, and continuing to call it three more times with
             * the same buffer size is not a measurement, it is repeating the damage. */
            return obs_fail_code("the call wrote past the end of its buffer",
                                 (uint64_t)(uint32_t)obs_query_flags[i]);
        }
        obs_report_written("130-layout/direct-memory-query-flags",
                           "sceKernelDirectMemoryQuery", obs_query_flag_name[i],
                           probe.before, probe.buffer, OBS_LAYOUT_BUFFER);
        obs_report_measure("130-layout/direct-memory-query-flags",
                           "sceKernelDirectMemoryQuery", obs_query_flag_name[i],
                           (uint64_t)(int64_t)rc, "code");
        answered++;
        if (rc != 0) {
            /* Refusing is a real answer about the flag, and often the most informative
             * one: a value the platform rejects is a value that means something. */
            refused++;
            continue;
        }
        if (!have_baseline) {
            for (unsigned int b = 0; b < OBS_LAYOUT_BUFFER; b++) {
                baseline[b] = probe.buffer[b];
            }
            have_baseline = 1;
            continue;
        }
        for (unsigned int b = 0; b < OBS_LAYOUT_BUFFER; b++) {
            if (probe.buffer[b] != baseline[b]) {
                /* The offset of the first difference, which is what a reader needs to
                 * find the field the flag reaches without diffing two hexdumps by eye.
                 */
                obs_report_measure("130-layout/direct-memory-query-flags",
                                   "sceKernelDirectMemoryQuery", obs_query_flag_name[i],
                                   (uint64_t)b, "first-differing-byte");
                differing++;
                break;
            }
        }
    }

    if (answered == 0) {
        return obs_fail("the call was never reached");
    }
    if (refused == answered) {
        return obs_partial_value("every flag value was refused", (uint64_t)refused);
    }
    /* Zero differences is a real result and not a failure: it says the flag does not
     * change what this offset reports, on this implementation, which is exactly as much
     * of an answer as a difference would be. The value is how many of the accepted
     * values produced a different buffer. */
    return obs_pass_value((uint64_t)differing);
}

/* The memory types to allocate with. If the query's third field tracks the type asked,
 * that field *is* the type; if it stays constant while the type varies, it is some
 * other state and the type lives elsewhere. Three distinct types, which is enough to
 * tell tracking from constant. */
static const unsigned int obs_mem_type_values[] = {
    OBS_MEM_TYPE_WB_ONION,  /* 0 */
    OBS_MEM_TYPE_WC_GARLIC, /* 3 */
    OBS_MEM_TYPE_WB_GARLIC, /* 10 */
};
static const char *const obs_mem_type_name[] = {"wb-onion", "wc-garlic", "wb-garlic"};

/* What the third field of the query structure is - a memory type, or some other state.
 *
 * A hardware run read `3` from it for the region at the bottom of the map
 * (orbistoun#D398). Three is not a boolean, so whatever it once meant here it is not
 * "allocated"; the open question is whether it is the memory *type*. This settles it:
 * allocate a span with each of several types, query it back, and record the third field
 * for each. If the field equals the type asked, it is the type; if it is the same value
 * for every type, it is state and the type is somewhere this does not read. Nothing is
 * interpreted - the pairs are reported and the verdict states which pattern held. */
static obs_result check_memory_type_field(void) {
    OBS_REQUIRE(&sceKernelAllocateDirectMemory, &sceKernelDirectMemoryQuery,
                &sceKernelGetDirectMemorySize);

    unsigned int allocated = 0;
    unsigned int tracked = 0;
    long first_field = -1;
    int varied = 0;

    for (unsigned int i = 0; i < OBS_COUNT(obs_mem_type_values); i++) {
        sce_off_t offset = 0;
        /* One 16 KiB page, the platform's own granularity. A small span left un-freed
         * for the run is negligible, and freeing between allocations would add a call
         * whose own failure could be mistaken for this one's. */
        int rc = sceKernelAllocateDirectMemory(
            0, (sce_off_t)sceKernelGetDirectMemorySize(), 0x4000, 0x4000,
            (int)obs_mem_type_values[i], &offset);
        if (rc != 0) {
            obs_report_measure("130-layout/memory-type", obs_mem_type_name[i],
                               "alloc-refused", (uint64_t)(int64_t)rc, "code");
            continue;
        }
        allocated++;

        layout_probe probe;
        layout_prepare(&probe);
        /* Flag 1, which is what every observed query passes. */
        rc = sceKernelDirectMemoryQuery((sce_off_t)offset, 1, probe.buffer,
                                        OBS_LAYOUT_BUFFER);
        if (!layout_guard_intact(&probe)) {
            return obs_fail("the query wrote past its buffer");
        }
        if (rc != 0) {
            obs_report_measure("130-layout/memory-type", obs_mem_type_name[i],
                               "query-refused", (uint64_t)(int64_t)rc, "code");
            continue;
        }
        /* The third field: two eight-byte words in (start, end, then this). Read as a
         * small integer, which is what the `3` on hardware was. */
        unsigned int field = (unsigned int)probe.buffer[16] |
                             ((unsigned int)probe.buffer[17] << 8) |
                             ((unsigned int)probe.buffer[18] << 16) |
                             ((unsigned int)probe.buffer[19] << 24);
        obs_report_measure("130-layout/memory-type", obs_mem_type_name[i],
                           "third-field", (uint64_t)field, "value");
        if (field == obs_mem_type_values[i]) {
            tracked++;
        }
        if (first_field < 0) {
            first_field = (long)field;
        } else if ((long)field != first_field) {
            varied = 1;
        }
    }

    if (allocated == 0) {
        return obs_skip(
            "no direct memory could be allocated, so the field cannot be varied");
    }
    if (tracked == allocated) {
        return obs_pass_value(
            (uint64_t)tracked); /* the third field is the memory type */
    }
    if (!varied) {
        return obs_partial(
            "the third field is constant across types - it is state, not the type");
    }
    return obs_partial_value("the third field varies but is not the type asked",
                             (uint64_t)tracked);
}

static obs_result check_system_software_version(void) {
    /* A version structure, and the most portable thing to ask a console about itself.
     * Interesting on hardware for a second reason: the bytes say which firmware
     * produced every other record in the report, which is the context a hardware run
     * needs and cannot otherwise carry. */
    layout_probe probe;
    layout_prepare(&probe);
    int rc = sceKernelGetSystemSwVersion(probe.buffer);
    return layout_report("130-layout/system-software-version",
                         "sceKernelGetSystemSwVersion", "version", &probe, rc);
}

static obs_result check_resolution_status(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceUserServiceGetInitialUser);

    int borrowed = 0;
    int handle = -1;
    if (obs_display_holds_output()) {
        handle = obs_display_get_video_handle();
        if (handle > 0) {
            borrowed = 1;
        }
    }
    if (handle <= 0) {
        int32_t user = 0;
        if (sceUserServiceGetInitialUser(&user) != 0) {
            return obs_skip("no initial user to open an output for");
        }
        handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
        if (handle <= 0) {
            return obs_skip("no video output to query");
        }
    }
    layout_probe probe;
    layout_prepare(&probe);
    int rc = sceVideoOutGetResolutionStatus(handle, probe.buffer);
    obs_result result =
        layout_report("130-layout/resolution-status", "sceVideoOutGetResolutionStatus",
                      "status", &probe, rc);
    if (!borrowed) {
        (void)sceVideoOutClose(handle);
    }
    return result;
}

static obs_result check_video_out_output_status(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceUserServiceGetInitialUser);

    int borrowed = 0;
    int handle = -1;
    if (obs_display_holds_output()) {
        handle = obs_display_get_video_handle();
        if (handle > 0) {
            borrowed = 1;
        }
    }
    if (handle <= 0) {
        int32_t user = 0;
        if (sceUserServiceGetInitialUser(&user) != 0) {
            return obs_skip("no initial user to open an output for");
        }
        handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
        if (handle <= 0) {
            return obs_skip("no video output to query");
        }
    }

    const char *id = "130-layout/video-out-output-status";
    const char *sym = "sceVideoOutGetOutputStatus";

    uint8_t buf[64];
    uint8_t before[64];
    memset(buf, 0xC7, sizeof(buf));
    memset(before, 0xC7, sizeof(before));

    int rc_unreg = -1;
    obs_jmp_buf guard;
    int sig = 0;

    if (obs_address_is_callable((const void *)&sceVideoOutGetOutputStatus)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_unreg = sceVideoOutGetOutputStatus(handle, buf);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-unregistered", (uint64_t)(uint32_t)rc_unreg,
                               "rc");
            obs_report_written(id, sym, "status-unregistered", before, buf, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-unreg-faulted", (uint64_t)(uint32_t)sig,
                               "sig");
        }
    }

    /* Arm with buffer registered if direct memory is available and handle not borrowed
     */
    if (!borrowed &&
        obs_address_is_callable((const void *)&sceKernelAllocateDirectMemory) &&
        obs_address_is_callable((const void *)&sceKernelMapDirectMemory) &&
        obs_address_is_callable((const void *)&sceVideoOutRegisterBuffers) &&
        obs_address_is_callable((const void *)&sceVideoOutSetBufferAttribute)) {
        size_t vb_bytes = 1920u * 1088u * 4u;
        size_t vb_align = 2u * 1024u * 1024u;
        sce_off_t physical = 0;
        int mem_rc = sceKernelAllocateDirectMemory(
            0, (sce_off_t)sceKernelGetDirectMemorySize(), vb_bytes, vb_align,
            OBS_MEM_TYPE_WC_GARLIC, &physical);
        void *mapped = NULL;
        if (mem_rc == 0) {
            sceKernelMapDirectMemory(&mapped, vb_bytes,
                                     OBS_PROT_CPU_RW | OBS_PROT_GPU_READ |
                                         OBS_PROT_GPU_WRITE,
                                     0, physical, vb_align);
        }
        if (mapped != NULL) {
            unsigned char attr[256];
            memset(attr, 0, sizeof(attr));
            sceVideoOutSetBufferAttribute(attr, 0x80000000u, 1u, 0u, 1920u, 1080u,
                                          1920u);
            void *addresses[1] = {mapped};
            (void)sceVideoOutRegisterBuffers(handle, 0, addresses, 1, attr);

            uint8_t buf_reg[64];
            memset(buf_reg, 0xC7, sizeof(buf_reg));
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                int rc_reg = sceVideoOutGetOutputStatus(handle, buf_reg);
                obs_fault_unregister();
                obs_report_measure(id, sym, "rc-registered", (uint64_t)(uint32_t)rc_reg,
                                   "rc");
                obs_report_written(id, sym, "status-registered", before, buf_reg, 64);
            } else {
                obs_fault_unregister();
                obs_report_measure(id, sym, "rc-reg-faulted", (uint64_t)(uint32_t)sig,
                                   "sig");
            }

            /* Clean teardown order per videobuf convention:
             * 1. Close video out handle to unregister buffers from display controller
             * 2. Unmap virtual address range
             * 3. Release physical direct memory pages */
            (void)sceVideoOutClose(handle);
            handle = -1;

            if (obs_address_is_callable((const void *)&sceKernelMunmap)) {
                sceKernelMunmap(mapped, vb_bytes);
                mapped = NULL;
            }
            if (obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
                sceKernelReleaseDirectMemory(physical, vb_bytes);
                physical = 0;
            }
        }
    }

    /* NULL buffer test (guarded with OBS_FAULT_ARM against userland faults) */
    if (handle > 0 &&
        obs_address_is_callable((const void *)&sceVideoOutGetOutputStatus)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_null = sceVideoOutGetOutputStatus(handle, NULL);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-null", (uint64_t)(uint32_t)rc_null, "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-null-faulted", (uint64_t)(uint32_t)sig,
                               "sig");
        }
    }

    /* Bad handle test */
    if (obs_address_is_callable((const void *)&sceVideoOutGetOutputStatus)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_bad = sceVideoOutGetOutputStatus(-1, buf);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-bad-handle", (uint64_t)(uint32_t)rc_bad,
                               "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-bad-handle-faulted",
                               (uint64_t)(uint32_t)sig, "sig");
        }
    }

    if (!borrowed && handle > 0 &&
        obs_address_is_callable((const void *)&sceVideoOutClose)) {
        (void)sceVideoOutClose(handle);
    }
    return obs_pass();
}

/* REQ-20261006T1230Z-u7b2: sceVideoOutUnregisterBuffers */
static obs_result check_video_out_unregister_buffers(void) {
    const char *id = "130-layout/video-out-unregister-buffers";
    const char *sym = "sceVideoOutUnregisterBuffers";

    obs_report_measure(
        id, sym, "resolved",
        (uint64_t)obs_address_is_callable((const void *)&sceVideoOutUnregisterBuffers),
        "bool");

    /* 1. Bad handle test */
    if (obs_address_is_callable((const void *)&sceVideoOutUnregisterBuffers)) {
        obs_jmp_buf guard;
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_bad = sceVideoOutUnregisterBuffers(-1, 0);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-bad-handle", (uint64_t)(uint32_t)rc_bad,
                               "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "fault-bad-handle", 1u, "bool");
        }
    }

    /* 2. On the display handle (borrowed or open) */
    int borrowed = 0;
    int handle = -1;
    if (obs_display_holds_output()) {
        handle = obs_display_get_video_handle();
        if (handle > 0) {
            borrowed = 1;
        }
    }

    if (handle > 0 &&
        obs_address_is_callable((const void *)&sceVideoOutUnregisterBuffers)) {
        obs_jmp_buf guard;
        /* Unused set index 1 test */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_set1 = sceVideoOutUnregisterBuffers(handle, 1);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-unregister-unused-set1",
                               (uint64_t)(uint32_t)rc_set1, "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "fault-unused-set1", 1u, "bool");
        }

        /* Negative set index test */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_neg = sceVideoOutUnregisterBuffers(handle, -1);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-unregister-neg-set",
                               (uint64_t)(uint32_t)rc_neg, "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "fault-neg-set", 1u, "bool");
        }

        /* Active set 0 test (is it refused with 0x80290009 RESOURCE_BUSY while scanning
         * out?) */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_active = sceVideoOutUnregisterBuffers(handle, 0);
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-unregister-active-set0",
                               (uint64_t)(uint32_t)rc_active, "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "fault-active-set0", 1u, "bool");
        }
    }

    /* 3. Register -> Unregister -> Register-again sequence on secondary bus (1) or
     * fresh handle */
    int32_t user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        (void)sceUserServiceGetInitialUser(&user);
    }
    int test_handle = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        test_handle = sceVideoOutOpen(user, 1 /* OBS_VIDEO_BUS_SUB */, 0, 0);
        if (test_handle <= 0 && !borrowed) {
            test_handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
        }
    }

    if (test_handle > 0 &&
        obs_address_is_callable((const void *)&sceKernelAllocateDirectMemory) &&
        obs_address_is_callable((const void *)&sceKernelMapDirectMemory) &&
        obs_address_is_callable((const void *)&sceVideoOutUnregisterBuffers)) {

        obs_report_measure(id, sym, "test-handle-opened", (uint64_t)test_handle,
                           "handle");

        size_t vb_bytes = 1920u * 1088u * 4u;
        size_t vb_align = 2u * 1024u * 1024u;
        sce_off_t physA = 0;
        sce_off_t physB = 0;
        int mrcA = sceKernelAllocateDirectMemory(
            0, (sce_off_t)sceKernelGetDirectMemorySize(), vb_bytes, vb_align,
            OBS_MEM_TYPE_WC_GARLIC, &physA);
        int mrcB = sceKernelAllocateDirectMemory(
            0, (sce_off_t)sceKernelGetDirectMemorySize(), vb_bytes, vb_align,
            OBS_MEM_TYPE_WC_GARLIC, &physB);
        void *mapA = NULL;
        void *mapB = NULL;
        if (mrcA == 0) {
            sceKernelMapDirectMemory(&mapA, vb_bytes,
                                     OBS_PROT_CPU_RW | OBS_PROT_GPU_READ |
                                         OBS_PROT_GPU_WRITE,
                                     0, physA, vb_align);
        }
        if (mrcB == 0) {
            sceKernelMapDirectMemory(&mapB, vb_bytes,
                                     OBS_PROT_CPU_RW | OBS_PROT_GPU_READ |
                                         OBS_PROT_GPU_WRITE,
                                     0, physB, vb_align);
        }

        if (mapA != NULL && mapB != NULL) {
            unsigned char attr[256];
            memset(attr, 0, sizeof(attr));
            if (obs_address_is_callable(
                    (const void *)&sceVideoOutSetBufferAttribute2)) {
                sceVideoOutSetBufferAttribute2(attr, 0x80000000u, 1u, 1920u, 1080u, 0,
                                               0, 0);
            } else if (obs_address_is_callable(
                           (const void *)&sceVideoOutSetBufferAttribute)) {
                sceVideoOutSetBufferAttribute(attr, 0x80000000u, 1u, 0u, 1920u, 1080u,
                                              1920u);
            }

            /* Step 1: Register set 0 with buffer A */
            int reg1_rc = -1;
            if (obs_address_is_callable((const void *)&sceVideoOutRegisterBuffers2)) {
                struct {
                    void *data;
                } bufs[1];
                bufs[0].data = mapA;
                reg1_rc =
                    sceVideoOutRegisterBuffers2(test_handle, 0, 0, bufs, 1, attr, 0, 0);
            } else if (obs_address_is_callable(
                           (const void *)&sceVideoOutRegisterBuffers)) {
                void *addrs[1] = {mapA};
                reg1_rc = sceVideoOutRegisterBuffers(test_handle, 0, addrs, 1, attr);
            }
            obs_report_measure(id, sym, "rc-seq-register1", (uint64_t)(uint32_t)reg1_rc,
                               "rc");

            /* Step 2: Unregister set 0 */
            int unreg_rc = sceVideoOutUnregisterBuffers(test_handle, 0);
            obs_report_measure(id, sym, "rc-seq-unregister",
                               (uint64_t)(uint32_t)unreg_rc, "rc");

            /* Step 3: Register-again set 0 with buffer B */
            int reg2_rc = -1;
            if (obs_address_is_callable((const void *)&sceVideoOutRegisterBuffers2)) {
                struct {
                    void *data;
                } bufs[1];
                bufs[0].data = mapB;
                reg2_rc =
                    sceVideoOutRegisterBuffers2(test_handle, 0, 0, bufs, 1, attr, 0, 0);
            } else if (obs_address_is_callable(
                           (const void *)&sceVideoOutRegisterBuffers)) {
                void *addrs[1] = {mapB};
                reg2_rc = sceVideoOutRegisterBuffers(test_handle, 0, addrs, 1, attr);
            }
            obs_report_measure(id, sym, "rc-seq-reregister",
                               (uint64_t)(uint32_t)reg2_rc, "rc");

            /* Teardown */
            (void)sceVideoOutUnregisterBuffers(test_handle, 0);
            (void)sceVideoOutClose(test_handle);
            test_handle = -1;
            if (obs_address_is_callable((const void *)&sceKernelMunmap)) {
                sceKernelMunmap(mapA, vb_bytes);
                sceKernelMunmap(mapB, vb_bytes);
            }
            if (obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
                sceKernelReleaseDirectMemory(physA, vb_bytes);
                sceKernelReleaseDirectMemory(physB, vb_bytes);
            }
        } else {
            if (mapA != NULL &&
                obs_address_is_callable((const void *)&sceKernelMunmap)) {
                sceKernelMunmap(mapA, vb_bytes);
            }
            if (mapB != NULL &&
                obs_address_is_callable((const void *)&sceKernelMunmap)) {
                sceKernelMunmap(mapB, vb_bytes);
            }
            if (physA != 0 &&
                obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
                sceKernelReleaseDirectMemory(physA, vb_bytes);
            }
            if (physB != 0 &&
                obs_address_is_callable((const void *)&sceKernelReleaseDirectMemory)) {
                sceKernelReleaseDirectMemory(physB, vb_bytes);
            }
            if (test_handle > 0 &&
                obs_address_is_callable((const void *)&sceVideoOutClose)) {
                (void)sceVideoOutClose(test_handle);
            }
        }
    } else if (test_handle > 0 &&
               obs_address_is_callable((const void *)&sceVideoOutClose)) {
        (void)sceVideoOutClose(test_handle);
    }

    return obs_pass();
}

static obs_result check_video_out_vblank_status(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceUserServiceGetInitialUser);

    int borrowed = 0;
    int handle = -1;
    if (obs_display_holds_output()) {
        handle = obs_display_get_video_handle();
        if (handle > 0) {
            borrowed = 1;
        }
    }
    if (handle <= 0) {
        int32_t user = 0;
        if (sceUserServiceGetInitialUser(&user) != 0) {
            return obs_skip("no initial user to open an output for");
        }
        handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
        if (handle <= 0) {
            return obs_skip("no video output to query");
        }
    }

    const char *id = "130-layout/video-out-vblank-status";
    const char *sym = "sceVideoOutGetVblankStatus";

    uint8_t buf1[64];
    uint8_t buf2[64];
    uint8_t before[64];
    memset(buf1, 0xC7, sizeof(buf1));
    memset(buf2, 0xC7, sizeof(buf2));
    memset(before, 0xC7, sizeof(before));

    obs_jmp_buf guard;
    int sig = 0;

    int rc1 = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutGetVblankStatus)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc1 = sceVideoOutGetVblankStatus(handle, buf1);
            obs_fault_unregister();
            uint64_t count1 = *(const uint64_t *)(buf1 + 0);
            uint64_t proc_time1 = *(const uint64_t *)(buf1 + 8);
            uint64_t tsc1 = *(const uint64_t *)(buf1 + 16);
            obs_report_measure(id, sym, "rc-1", (uint64_t)(uint32_t)rc1, "rc");
            obs_report_measure(id, sym, "count-1", count1, "count");
            obs_report_measure(id, sym, "process-time-1", proc_time1, "us");
            obs_report_measure(id, sym, "tsc-1", tsc1, "tsc");
            obs_report_written(id, sym, "status-1", before, buf1, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-1-faulted", (uint64_t)(uint32_t)sig, "sig");
        }
    }

    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        sceKernelUsleep(100000);
    }

    int rc2 = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutGetVblankStatus)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc2 = sceVideoOutGetVblankStatus(handle, buf2);
            obs_fault_unregister();
            uint64_t count2 = *(const uint64_t *)(buf2 + 0);
            uint64_t proc_time2 = *(const uint64_t *)(buf2 + 8);
            uint64_t tsc2 = *(const uint64_t *)(buf2 + 16);
            obs_report_measure(id, sym, "rc-2", (uint64_t)(uint32_t)rc2, "rc");
            obs_report_measure(id, sym, "count-2", count2, "count");
            obs_report_measure(id, sym, "process-time-2", proc_time2, "us");
            obs_report_measure(id, sym, "tsc-2", tsc2, "tsc");
            obs_report_written(id, sym, "status-2", before, buf2, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, sym, "rc-2-faulted", (uint64_t)(uint32_t)sig, "sig");
        }
    }

    /* WaitVblank call: only attempt if callable, guarded with OBS_FAULT_ARM */
    if (obs_address_is_callable((const void *)&sceVideoOutWaitVblank) &&
        obs_address_is_callable((const void *)&sceKernelReadTsc)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t t0 = sceKernelReadTsc();
            int rc_wait = sceVideoOutWaitVblank(handle);
            uint64_t t1 = sceKernelReadTsc();
            obs_fault_unregister();
            obs_report_measure(id, "sceVideoOutWaitVblank", "rc",
                               (uint64_t)(uint32_t)rc_wait, "rc");
            obs_report_measure(id, "sceVideoOutWaitVblank", "duration-tsc", t1 - t0,
                               "tsc");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceVideoOutWaitVblank", "faulted",
                               (uint64_t)(uint32_t)sig, "sig");
        }
    }

    if (!borrowed && handle > 0 &&
        obs_address_is_callable((const void *)&sceVideoOutClose)) {
        (void)sceVideoOutClose(handle);
    }
    return obs_pass();
}

static obs_result check_module_info_for_unwind(void) {
    const char *id = "130-layout/unwind-module-info";
    const char *sym = "sceKernelGetModuleInfoForUnwind";

    const void *fn_raw = (const void *)&sceKernelGetModuleInfoForUnwind;
    if (!obs_address_is_callable(fn_raw)) {
        fn_raw = obs_module_symbol(1, sym);
    }
    if (!obs_address_is_callable(fn_raw)) {
        return obs_skip("sceKernelGetModuleInfoForUnwind not callable");
    }
    int (*fn_unwind)(const void *, void *) = (int (*)(const void *, void *))fn_raw;

    obs_jmp_buf guard;
    int sig = 0;

    /* Arm 1: probe text with 0x130 buffer */
    uint8_t buf[0x140];
    uint8_t before[0x140];
    memset(buf, 0xC7, sizeof(buf));
    memset(before, 0xC7, sizeof(before));
    *(size_t *)buf = 0x130;
    *(size_t *)before = 0x130;

    const void *text_addr = (const void *)&check_module_info_for_unwind;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_text = fn_unwind(text_addr, buf);
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-text", (uint64_t)(uint32_t)rc_text, "rc");
        obs_report_written(id, sym, "unwind-info", before, buf, 0x130);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-text-faulted", (uint64_t)(uint32_t)sig, "sig");
    }

    /* Arm 2: unmapped address */
    uint8_t buf2[0x140];
    memset(buf2, 0xC7, sizeof(buf2));
    *(size_t *)buf2 = 0x130;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_unmapped = fn_unwind((const void *)0xdeadbeef000ULL, buf2);
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-unmapped", (uint64_t)(uint32_t)rc_unmapped,
                           "rc");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-unmapped-faulted", (uint64_t)(uint32_t)sig,
                           "sig");
    }

    /* Arm 3: null buffer */
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_null = fn_unwind(text_addr, NULL);
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-null", (uint64_t)(uint32_t)rc_null, "rc");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-null-faulted", (uint64_t)(uint32_t)sig, "sig");
    }

    return obs_pass();
}

static uint8_t s_query_bss[4096];

static void query_prot_arm(const char *id, const char *name, const void *addr,
                           int (*fn)(const void *, uint64_t *, uint64_t *,
                                     uint32_t *)) {
    uint64_t start = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t end = 0xBBBBBBBBBBBBBBBBULL;
    uint32_t prot = 0xCCCCCCCCu;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc = fn(addr, &start, &end, &prot);
        obs_fault_unregister();
        obs_report_measure(id, name, "rc", (uint64_t)(uint32_t)rc, "rc");
        obs_report_measure(id, name, "start", start, "addr");
        obs_report_measure(id, name, "end", end, "addr");
        obs_report_measure(id, name, "prot", (uint64_t)prot, "prot");
        obs_report_measure(id, name, "bit-0x80", (uint64_t)((prot & 0x80) ? 1 : 0),
                           "bool");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, name, "faulted", (uint64_t)(uint32_t)sig, "sig");
    }
}

static obs_result check_query_memory_protection(void) {
    const char *id = "130-layout/query-memory-protection";
    const char *sym = "sceKernelQueryMemoryProtection";

    const void *fn_raw = (const void *)&sceKernelQueryMemoryProtection;
    if (!obs_address_is_callable(fn_raw)) {
        fn_raw = obs_module_symbol(1, sym);
    }
    if (!obs_address_is_callable(fn_raw)) {
        return obs_skip("sceKernelQueryMemoryProtection not callable");
    }
    int (*query_fn)(const void *, uint64_t *, uint64_t *, uint32_t *) =
        (int (*)(const void *, uint64_t *, uint64_t *, uint32_t *))fn_raw;

    /* 1. Text */
    query_prot_arm(id, "text", (const void *)&check_query_memory_protection, query_fn);

    /* 2. Data */
    query_prot_arm(id, "data", (const void *)&obs_section_layout, query_fn);

    /* 3. BSS */
    query_prot_arm(id, "bss", (const void *)s_query_bss, query_fn);

    /* 4. Unmapped */
    query_prot_arm(id, "unmapped", (const void *)0xdeadbeef000ULL, query_fn);

    /* 5. NULL params */
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_null =
            query_fn((const void *)&check_query_memory_protection, NULL, NULL, NULL);
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-null-params", (uint64_t)(uint32_t)rc_null,
                           "rc");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, sym, "rc-null-params-faulted", (uint64_t)(uint32_t)sig,
                           "sig");
    }

    return obs_pass();
}

/* How big is that structure? Asked of the platform, one size at a time.
 *
 * A call handed a size smaller than the structure it fills refuses; handed enough, it
 * works. So walking a ladder of sizes and finding the boundary **is** the structure
 * size, and nobody had to decide anything - which is the difference between `derived`
 * and `assumed`, and the whole reason this is worth more than one more hexdump. (D008)
 *
 * Every size on the ladder is within the buffer, so the call can never be invited to
 * write past it: the ladder stops at OBS_LAYOUT_BUFFER and the guard is checked after
 * every rung. A refusal is the *interesting* result here, which is the opposite of most
 * of this suite - so the verdict is about whether a boundary was found at all, not
 * about any single call.
 *
 * The ladder is dense at the low end and sparse above it because structures are small:
 * 24 and 32 are plausible sizes, 200 is not, and a rung costs a call. */
static const unsigned int obs_size_ladder[] = {
    1u, 2u, 4u, 8u, 12u, 16u, 20u, 24u, 32u, 40u, 48u, 64u, 96u, 128u, 192u, 256u};

static obs_result check_query_size_ladder(void) {
    OBS_REQUIRE(&sceKernelDirectMemoryQuery);

    unsigned int smallest_accepted = 0;
    unsigned int largest_rejected = 0;
    unsigned int accepted = 0;
    unsigned int rejected = 0;

    for (unsigned int i = 0; i < OBS_COUNT(obs_size_ladder); i++) {
        unsigned int size = obs_size_ladder[i];
        layout_probe probe;
        layout_prepare(&probe);
        int rc = sceKernelDirectMemoryQuery(0, 0, probe.buffer, (size_t)size);
        if (!layout_guard_intact(&probe)) {
            /* Stop at once. The call was given a size no larger than the buffer and
             * wrote past it anyway, which means the size argument does not bound what
             * it writes - a far more serious finding than any structure size, and
             * continuing to climb the ladder would repeat the damage with larger sizes.
             */
            return obs_fail_code(
                "the call wrote past its buffer for a size it was given",
                (uint64_t)size);
        }
        int ok = (rc == 0);
        obs_report_size("libkernel", "sceKernelDirectMemoryQuery", 3u, size, ok,
                        (uint64_t)(int64_t)rc);
        if (ok) {
            accepted++;
            if (smallest_accepted == 0) {
                smallest_accepted = size;
            }
        } else {
            rejected++;
            if (smallest_accepted == 0) {
                largest_rejected = size;
            }
        }
    }

    if (accepted == 0) {
        return obs_skip("every size was refused, so no boundary is visible from here");
    }
    if (rejected == 0) {
        /* Accepting one byte means the size is not being checked, which is a fact about
         * the call rather than a failure of the probe - and it says the structure size
         * cannot be recovered this way on this platform. */
        return obs_partial(
            "every size was accepted, so the size argument is not validated");
    }
    obs_report_measure("130-layout/query-size-ladder", "sceKernelDirectMemoryQuery",
                       "smallest-accepted", (uint64_t)smallest_accepted, "bytes");
    obs_report_measure("130-layout/query-size-ladder", "sceKernelDirectMemoryQuery",
                       "largest-refused", (uint64_t)largest_rejected, "bytes");
    return obs_pass();
}

/* Does a short declared size bound the write, or does the call fill the whole record
 * anyway?
 *
 * The ladder above proved every size is *accepted*; it never said what a small one
 * *wrote*. This does: poison the buffer past the declared size (but inside it), call,
 * and see whether anything past the size changed. It is the per-declared-size sibling
 * of the ladder's tail guard - that one catches a write past the whole 256-byte buffer,
 * this one catches a write past the 8 bytes the caller said it had.
 *
 * The distinction is a different contract, not a nicety. "Truncates at the size given"
 * and "ignores the size and writes the full structure" corrupt a small-buffer caller
 * differently, and an emulator has to pick one; this says which the hardware picked. */
static obs_result check_query_short_buffer_overrun(void) {
    OBS_REQUIRE(&sceKernelDirectMemoryQuery);

    unsigned int bounded = 0;
    unsigned int overran = 0;

    static const unsigned int declared[] = {8u, 16u, 24u, 32u};
    for (unsigned int i = 0; i < OBS_COUNT(declared); i++) {
        unsigned int size = declared[i];
        layout_probe probe;
        layout_prepare(&probe);
        int rc = sceKernelDirectMemoryQuery(0, 0, probe.buffer, (size_t)size);
        if (!layout_guard_intact(&probe)) {
            /* Past the whole buffer for a size a fraction of it: the size bounds
             * nothing, and climbing further would only repeat the damage with a larger
             * one. */
            return obs_fail_code("the call wrote past its buffer for a short size",
                                 (uint64_t)size);
        }
        if (rc != 0) {
            obs_report_measure("130-layout/short-buffer-overrun",
                               "sceKernelDirectMemoryQuery", "refused", (uint64_t)size,
                               "bytes");
            continue;
        }
        /* Accepted. Did it stay inside the size it was handed? Anything past `size`
         * that no longer matches the poison it was primed with is a byte written beyond
         * the bound. */
        int wrote_past = 0;
        for (unsigned int b = size; b < OBS_LAYOUT_BUFFER; b++) {
            if (probe.buffer[b] != probe.before[b]) {
                wrote_past = 1;
                break;
            }
        }
        if (wrote_past) {
            overran++;
            obs_report_measure("130-layout/short-buffer-overrun",
                               "sceKernelDirectMemoryQuery", "wrote-past-size",
                               (uint64_t)size, "bytes");
        } else {
            bounded++;
            obs_report_measure("130-layout/short-buffer-overrun",
                               "sceKernelDirectMemoryQuery", "bounded-by-size",
                               (uint64_t)size, "bytes");
        }
    }

    if (bounded == 0 && overran == 0) {
        return obs_skip("every short size was refused, so there is nothing to bound");
    }
    if (overran > 0 && bounded == 0) {
        /* Uniform: the size argument is decorative, the call writes the whole record. A
         * real finding about the contract, reported as partial so it stands out rather
         * than fails. */
        return obs_partial_value("the size argument does not bound the write",
                                 (uint64_t)overran);
    }
    if (overran > 0) {
        return obs_partial_value("some short sizes were not respected",
                                 (uint64_t)overran);
    }
    return obs_pass_value((uint64_t)bounded);
}

/* Function-pointer casts: clang-format 18 and 22 spell `int32_t(*)(...)` and
 * `int32_t (*)(...)` differently, and CI installs 18 while a developer here has 22.
 * Written either way the other version rewrites it, so these are fenced the way the
 * census lists are (D016). The marker below has to be bare - clang-format does not
 * recognise it with anything appended, which is why the first attempt did nothing. */
/* clang-format off */
static const void *layout_resolve_sym(const char *lib, const char *sym) {
    int h = obs_module_open(lib);
    const void *p = obs_module_symbol(h >= 0 ? h : 1, sym);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }
    p = obs_module_symbol(1, sym);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }
    p = obs_module_symbol(OBS_HANDLE_SELF, sym);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t kaddr = krw_dynlib_resolve_any(pid, sym);
        if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
            return (const void *)kaddr;
        }
    }
#endif
    return NULL;
}

static obs_result check_user_service_layout(void) {
    int (*fn_user_init)(const void *) =
        (int (*)(const void *))layout_resolve_sym("libSceUserService", "sceUserServiceInitialize");
    if (fn_user_init != NULL && obs_address_is_callable((const void *)fn_user_init)) {
        (void)fn_user_init(NULL);
    }

    int32_t (*fn_get_user_list)(int32_t *userIdList) =
        (int32_t (*)(int32_t *))layout_resolve_sym("libSceUserService",
                                                  "sceUserServiceGetLoginUserIdList");
    int32_t (*fn_get_initial_user)(int32_t *userId) =
        (int32_t (*)(int32_t *))layout_resolve_sym("libSceUserService",
                                                  "sceUserServiceGetInitialUser");
    int32_t (*fn_get_user_name)(int32_t userId, char *userName, size_t size) =
        (int32_t (*)(int32_t, char *, size_t))layout_resolve_sym(
            "libSceUserService", "sceUserServiceGetUserName");
    int32_t (*fn_get_age_level)(int32_t userId, int32_t *ageLevel) =
        (int32_t (*)(int32_t, int32_t *))layout_resolve_sym(
            "libSceUserService", "sceUserServiceGetAgeLevel");

    obs_report_measure("130-layout/user-service", "sceUserServiceGetLoginUserIdList",
                       "resolved", (uint64_t)(fn_get_user_list != NULL ? 1 : 0), "bool");
    obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel",
                       "resolved", (uint64_t)(fn_get_age_level != NULL ? 1 : 0), "bool");

    if (fn_get_user_list == NULL && fn_get_initial_user == NULL && fn_get_age_level == NULL) {
        return obs_skip("libSceUserService symbols not available in this context");
    }

    int32_t initial_user_id = -1;
    if (fn_get_initial_user != NULL &&
        obs_address_is_callable((const void *)fn_get_initial_user)) {
        int rc = fn_get_initial_user(&initial_user_id);
        obs_report_measure("130-layout/user-service", "sceUserServiceGetInitialUser",
                           "user_id", (uint64_t)(uint32_t)initial_user_id, "id");
        obs_report_measure("130-layout/user-service", "sceUserServiceGetInitialUser",
                           "return_code", (uint64_t)(uint32_t)rc, "code");
    }

    if (fn_get_age_level != NULL &&
        obs_address_is_callable((const void *)fn_get_age_level)) {
        if (initial_user_id >= 0) {
            int32_t age_lvl = -999;
            int rc = fn_get_age_level(initial_user_id, &age_lvl);
            obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel",
                               "user_id", (uint64_t)(uint32_t)initial_user_id, "id");
            obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel",
                               "return_code", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel",
                               "age_level", (uint64_t)(uint32_t)age_lvl, "val");
        }
        int32_t inv_age = -999;
        int rc_inv = fn_get_age_level(-1, &inv_age);
        obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel_invalid",
                           "return_code", (uint64_t)(uint32_t)rc_inv, "code");
        obs_report_measure("130-layout/user-service", "sceUserServiceGetAgeLevel_invalid",
                           "age_level", (uint64_t)(uint32_t)inv_age, "val");
    }

    layout_probe probe;
    layout_prepare(&probe);
    if (fn_get_user_list != NULL &&
        obs_address_is_callable((const void *)fn_get_user_list)) {
        int rc = fn_get_user_list((int32_t *)probe.buffer);
        obs_report_measure("130-layout/user-service",
                           "sceUserServiceGetLoginUserIdList", "return_code",
                           (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes("130-layout/user-service", "sceUserServiceGetLoginUserIdList",
                         "user_id_list", 0, probe.buffer, 64);
    }

    if (initial_user_id >= 0 && fn_get_user_name != NULL &&
        obs_address_is_callable((const void *)fn_get_user_name)) {
        char name_buf[128];
        for (unsigned int i = 0; i < sizeof(name_buf); i++)
            name_buf[i] = 0;
        int rc = fn_get_user_name(initial_user_id, name_buf, sizeof(name_buf));
        obs_report_measure("130-layout/user-service", "sceUserServiceGetUserName",
                           "return_code", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes("130-layout/user-service", "sceUserServiceGetUserName",
                         "user_name_raw", 0, (const unsigned char *)name_buf, 64);
    }

    return obs_pass_value((uint64_t)(uint32_t)initial_user_id);
}

/* REQ-20260927T2110Z-9a3e: sceUserServiceGetGamePresets */
static obs_result check_user_game_presets(void) {
    const char *check_id = "130-layout/user-game-presets";
    int (*fn_user_init)(const void *) =
        (int (*)(const void *))layout_resolve_sym("libSceUserService", "sceUserServiceInitialize");
    if (fn_user_init != NULL && obs_address_is_callable((const void *)fn_user_init)) {
        (void)fn_user_init(NULL);
    }

    int32_t (*fn_get_initial_user)(int32_t *userId) =
        (int32_t (*)(int32_t *))layout_resolve_sym("libSceUserService",
                                                  "sceUserServiceGetInitialUser");
    int32_t initial_user_id = 1;
    if (fn_get_initial_user != NULL && obs_address_is_callable((const void *)fn_get_initial_user)) {
        (void)fn_get_initial_user(&initial_user_id);
    }

    int (*fn_get_presets)(int32_t userId, void *presets) =
        (int (*)(int32_t, void *))layout_resolve_sym("libSceUserService", "sceUserServiceGetGamePresets");
    if (fn_get_presets == NULL) {
        fn_get_presets = (int (*)(int32_t, void *))layout_resolve_sym("libSceUserService", "$-sD02mFDBh4");
    }
    if (fn_get_presets == NULL && obs_address_is_callable((const void *)&sceUserServiceGetGamePresets_static)) {
        fn_get_presets = (int (*)(int32_t, void *))&sceUserServiceGetGamePresets_static;
    }

    int resolved = (fn_get_presets != NULL && obs_address_is_callable((const void *)fn_get_presets));
    obs_report_measure(check_id, "resolve", "resolved", (uint64_t)(resolved ? 1 : 0), "bool");
    obs_report_measure(check_id, "resolve", "addr", (uint64_t)(uintptr_t)fn_get_presets, "address");
    if (!resolved) {
        return obs_skip("sceUserServiceGetGamePresets not available");
    }

    obs_jmp_buf guard;

    /* Arm 1: initial_user, 0x30-byte record whose first u64 is 0x30 and rest 0xcc */
    uint8_t rec1[0x30];
    memset(rec1, 0xcc, sizeof(rec1));
    *(uint64_t *)rec1 = 0x30;
    int rc1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_get_presets(initial_user_id, rec1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_id, "size-0x30-init-user", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_bytes(check_id, "size-0x30-init-user", "presets-48b", 0, rec1, 48);

    /* Arm 2: first word 0 */
    uint8_t rec2[0x30];
    memset(rec2, 0xcc, sizeof(rec2));
    *(uint64_t *)rec2 = 0;
    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_get_presets(initial_user_id, rec2);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_id, "size-0-init-user", "rc", (uint64_t)(uint32_t)rc2, "code");
    obs_report_bytes(check_id, "size-0-init-user", "presets-48b", 0, rec2, 48);

    /* Arm 3: first word 0x40 */
    uint8_t rec3[0x40];
    memset(rec3, 0xcc, sizeof(rec3));
    *(uint64_t *)rec3 = 0x40;
    int rc3 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc3 = fn_get_presets(initial_user_id, rec3);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_id, "size-0x40-init-user", "rc", (uint64_t)(uint32_t)rc3, "code");
    obs_report_bytes(check_id, "size-0x40-init-user", "presets-48b", 0, rec3, 48);

    /* Arm 4: user id -1 */
    uint8_t rec4[0x30];
    memset(rec4, 0xcc, sizeof(rec4));
    *(uint64_t *)rec4 = 0x30;
    int rc4 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc4 = fn_get_presets(-1, rec4);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_id, "user-neg1", "rc", (uint64_t)(uint32_t)rc4, "code");
    obs_report_bytes(check_id, "user-neg1", "presets-48b", 0, rec4, 48);

    /* Arm 5: null record */
    int rc5 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc5 = fn_get_presets(initial_user_id, NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_id, "null-presets", "rc", (uint64_t)(uint32_t)rc5, "code");

    /* Arm 6: user 0 if initial_user != 0 */
    if (initial_user_id != 0) {
        uint8_t rec0[0x30];
        memset(rec0, 0xcc, sizeof(rec0));
        *(uint64_t *)rec0 = 0x30;
        int rc0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc0 = fn_get_presets(0, rec0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(check_id, "user-0", "rc", (uint64_t)(uint32_t)rc0, "code");
        obs_report_bytes(check_id, "user-0", "presets-48b", 0, rec0, 48);
    }

    return obs_pass();
}

static obs_result check_app_content_layout(void) {
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load = (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
        (void)fn_load(0x00B4); /* OOPS_SYSMODULE_APP_CONTENT */
    }

    int32_t (*fn_init)(const void *, void *) =
        (int32_t (*)(const void *, void *))layout_resolve_sym("libSceAppContent", "sceAppContentInitialize");
    int32_t (*fn_mount2)(uint32_t, char *) =
        (int32_t (*)(uint32_t, char *))layout_resolve_sym("libSceAppContent", "sceAppContentTemporaryDataMount2");

    obs_report_measure("130-layout/app-content", "sceAppContentInitialize",
                       "resolved", (uint64_t)(fn_init != NULL ? 1 : 0), "bool");
    obs_report_measure("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                       "resolved", (uint64_t)(fn_mount2 != NULL ? 1 : 0), "bool");

    if (fn_init == NULL && fn_mount2 == NULL) {
        return obs_skip("libSceAppContent symbols not available in this context");
    }

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        layout_probe init_probe;
        layout_prepare(&init_probe);
        layout_probe boot_probe1;
        layout_prepare(&boot_probe1);

        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            int32_t rc1 = fn_init((const void *)init_probe.buffer, (void *)boot_probe1.buffer);
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentInitialize",
                               "return_code_1", (uint64_t)(uint32_t)rc1, "code");
            obs_report_bytes("130-layout/app-content", "sceAppContentInitialize",
                             "boot_param_1", 0, boot_probe1.buffer, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentInitialize",
                               "faulted_1", (uint64_t)(uint32_t)sig, "signal");
        }

        layout_probe boot_probe2;
        layout_prepare(&boot_probe2);
        sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            int32_t rc2 = fn_init((const void *)init_probe.buffer, (void *)boot_probe2.buffer);
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentInitialize",
                               "return_code_2", (uint64_t)(uint32_t)rc2, "code");
            obs_report_bytes("130-layout/app-content", "sceAppContentInitialize",
                             "boot_param_2", 0, boot_probe2.buffer, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentInitialize",
                               "faulted_2", (uint64_t)(uint32_t)sig, "signal");
        }
    }

    if (fn_mount2 != NULL && obs_address_is_callable((const void *)fn_mount2)) {
        layout_probe mount_probe;
        layout_prepare(&mount_probe);
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            int32_t rc_mount = fn_mount2(0u, (char *)mount_probe.buffer);
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                               "return_code", (uint64_t)(uint32_t)rc_mount, "code");
            obs_report_bytes("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                             "mount_point", 0, mount_probe.buffer, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                               "faulted", (uint64_t)(uint32_t)sig, "signal");
        }

        layout_probe mount_probe_fmt;
        layout_prepare(&mount_probe_fmt);
        sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            int32_t rc_mount_fmt = fn_mount2(1u, (char *)mount_probe_fmt.buffer);
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                               "return_code_format", (uint64_t)(uint32_t)rc_mount_fmt, "code");
            obs_report_bytes("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                             "mount_point_format", 0, mount_probe_fmt.buffer, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/app-content", "sceAppContentTemporaryDataMount2",
                               "faulted_format", (uint64_t)(uint32_t)sig, "signal");
        }
    }

    return obs_pass();
}

static obs_result check_savedata_layout(void) {
    const char *check_name = "130-layout/savedata-layout";

    /* Sysmodule load for save data */
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load = (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    int load_rc = -1;
    if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
        load_rc = fn_load(0x00B6); /* OOPS_SYSMODULE_SAVE_DATA */
    } else if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        load_rc = sceSysmoduleLoadModule(0x00B6);
    }
    obs_report_measure(check_name, "sceSysmoduleLoadModule", "rc",
                       (uint64_t)(uint32_t)load_rc, "code");

    int32_t (*fn_init)(void *) =
        (int32_t (*)(void *))layout_resolve_sym("libSceSaveData_native", "$TywrFKCoLGY");
    if (fn_init == NULL) {
        fn_init = (int32_t (*)(void *))layout_resolve_sym("libSceSaveData_native", "sceSaveDataInitialize3");
    }
    if (fn_init == NULL) {
        fn_init = (int32_t (*)(void *))layout_resolve_sym("libSceSaveData", "sceSaveDataInitialize3");
    }
    if (fn_init == NULL && obs_address_is_callable((const void *)&sceSaveDataInitialize3_static)) {
        fn_init = (int32_t (*)(void *))&sceSaveDataInitialize3_static;
    }
    if (fn_init == NULL && obs_address_is_callable((const void *)&sceSaveDataInitialize3_named)) {
        fn_init = (int32_t (*)(void *))&sceSaveDataInitialize3_named;
    }

    int32_t (*fn_term)(void) =
        (int32_t (*)(void))layout_resolve_sym("libSceSaveData_native", "$yKDy8S5yLA0");
    if (fn_term == NULL) {
        fn_term = (int32_t (*)(void))layout_resolve_sym("libSceSaveData_native", "sceSaveDataTerminate");
    }
    if (fn_term == NULL) {
        fn_term = (int32_t (*)(void))layout_resolve_sym("libSceSaveData", "sceSaveDataTerminate");
    }
    if (fn_term == NULL && obs_address_is_callable((const void *)&sceSaveDataTerminate_static)) {
        fn_term = (int32_t (*)(void))&sceSaveDataTerminate_static;
    }
    if (fn_term == NULL && obs_address_is_callable((const void *)&sceSaveDataTerminate_named)) {
        fn_term = (int32_t (*)(void))&sceSaveDataTerminate_named;
    }

    obs_report_measure(check_name, "resolve", "init3-slot",
                       (uint64_t)(uintptr_t)fn_init, "address");
    obs_report_measure(check_name, "resolve", "init3-resolved",
                       (uint64_t)(fn_init != NULL && obs_address_is_callable((const void *)fn_init)), "bool");
    obs_report_measure(check_name, "resolve", "term-slot",
                       (uint64_t)(uintptr_t)fn_term, "address");
    obs_report_measure(check_name, "resolve", "term-resolved",
                       (uint64_t)(fn_term != NULL && obs_address_is_callable((const void *)fn_term)), "bool");

    if (fn_init == NULL || !obs_address_is_callable((const void *)fn_init)) {
        return obs_skip("libSceSaveData_native sceSaveDataInitialize3 slot not resolved/callable");
    }

    obs_jmp_buf guard;

    /* 1. Arm: zeroed 64-byte parameter block */
    uint8_t param_zero[64];
    memset(param_zero, 0, sizeof(param_zero));
    uint8_t param_zero_before[64];
    memcpy(param_zero_before, param_zero, 64);
    obs_report_bytes(check_name, "zero-param", "before", 0, param_zero_before, 64);

    int32_t rc_zero = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_zero = fn_init((void *)param_zero);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "zero-param", "rc",
                       (uint64_t)(uint32_t)rc_zero, "code");
    obs_report_bytes(check_name, "zero-param", "after", 0, param_zero, 64);
    obs_report_measure(check_name, "zero-param", "changed-bytes",
                       (uint64_t)(memcmp(param_zero_before, param_zero, 64) != 0), "bool");

    /* 2. Arm: second call after the first */
    uint8_t param_second[64];
    memset(param_second, 0, sizeof(param_second));
    uint8_t param_second_before[64];
    memcpy(param_second_before, param_second, 64);

    int32_t rc_second = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_second = fn_init((void *)param_second);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "second-call", "rc",
                       (uint64_t)(uint32_t)rc_second, "code");
    obs_report_bytes(check_name, "second-call", "after", 0, param_second, 64);
    obs_report_measure(check_name, "second-call", "changed-bytes",
                       (uint64_t)(memcmp(param_second_before, param_second, 64) != 0), "bool");

    /* 3. Arm: NULL parameter call */
    int32_t rc_null = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_null = fn_init(NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "null-param", "rc",
                       (uint64_t)(uint32_t)rc_null, "code");

    /* 4. Arm: Terminate */
    int32_t rc_term = -999;
    if (fn_term != NULL && obs_address_is_callable((const void *)fn_term)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            rc_term = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "terminate", "rc",
                       (uint64_t)(uint32_t)rc_term, "code");

    /* 5. Arm: Initialize3 post-terminate call */
    uint8_t param_post[64];
    memset(param_post, 0, sizeof(param_post));
    uint8_t param_post_before[64];
    memcpy(param_post_before, param_post, 64);

    int32_t rc_post = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_post = fn_init((void *)param_post);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "post-terminate", "rc",
                       (uint64_t)(uint32_t)rc_post, "code");
    obs_report_bytes(check_name, "post-terminate", "after", 0, param_post, 64);
    obs_report_measure(check_name, "post-terminate", "changed-bytes",
                       (uint64_t)(memcmp(param_post_before, param_post, 64) != 0), "bool");

    /* Clean up if post-terminate init succeeded */
    if (rc_post == 0 && fn_term != NULL && obs_address_is_callable((const void *)fn_term)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            (void)fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    return obs_pass();
}

static obs_result check_savedata_event_result(void) {
    const char *id = "130-layout/savedata-event-result";
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load = (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
        (void)fn_load(0x00B6); /* OOPS_SYSMODULE_SAVE_DATA */
    }

    typedef int (*fn_get_event_t)(void *, void *);
    fn_get_event_t fn_get_event =
        (fn_get_event_t)layout_resolve_sym("libSceSaveData", "sceSaveDataGetEventResult");

    obs_report_measure(id, "sceSaveDataGetEventResult", "resolved",
                       (uint64_t)(fn_get_event != NULL), "bool");
    if (fn_get_event == NULL || !obs_address_is_callable((const void *)fn_get_event)) {
        return obs_pass();
    }

    uint8_t event_buf[128];
    memset(event_buf, 0x5a, sizeof(event_buf));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc = fn_get_event(NULL, event_buf);
        obs_fault_unregister();
        obs_report_measure(id, "nothing-queued", "rc", (uint64_t)(uint32_t)rc, "code");

        size_t written = 0;
        for (size_t i = 0; i < sizeof(event_buf); i++) {
            if (event_buf[i] != 0x5a) {
                written++;
            }
        }
        obs_report_measure(id, "nothing-queued", "bytes-written", (uint64_t)written, "bytes");
        if (written > 0) {
            obs_report_bytes(id, "nothing-queued", "event-dump", 0, event_buf,
                             (uint32_t)(written > 64 ? 64 : written));
        }
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "nothing-queued", "fault-sig", (uint64_t)(uint32_t)sig, "sig");
    }

    return obs_pass();
}

static obs_result check_savedata_mount_capacity(void) {
    const char *id = "130-layout/savedata-mount-capacity";
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load = (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
        (void)fn_load(0x00B6);
    }

    typedef int (*fn_init_t)(void *);
    typedef int (*fn_term_t)(void);
    typedef int (*fn_mount2_t)(void *);
    typedef int (*fn_mount_info_t)(const void *, void *);

    fn_init_t fn_init = (fn_init_t)layout_resolve_sym("libSceSaveData_native", "$TywrFKCoLGY");
    if (fn_init == NULL) {
        fn_init = (fn_init_t)layout_resolve_sym("libSceSaveData_native", "sceSaveDataInitialize3");
    }
    if (fn_init == NULL) {
        fn_init = (fn_init_t)layout_resolve_sym("libSceSaveData", "sceSaveDataInitialize3");
    }

    fn_term_t fn_term = (fn_term_t)layout_resolve_sym("libSceSaveData_native", "$yKDy8S5yLA0");
    if (fn_term == NULL) {
        fn_term = (fn_term_t)layout_resolve_sym("libSceSaveData_native", "sceSaveDataTerminate");
    }
    if (fn_term == NULL) {
        fn_term = (fn_term_t)layout_resolve_sym("libSceSaveData", "sceSaveDataTerminate");
    }

    fn_mount2_t fn_mount2 = (fn_mount2_t)layout_resolve_sym("libSceSaveData_native", "$0kj-gTw5PtM");
    if (fn_mount2 == NULL) {
        fn_mount2 = (fn_mount2_t)layout_resolve_sym("libSceSaveData_native", "sceSaveDataMount2");
    }
    if (fn_mount2 == NULL) {
        fn_mount2 = (fn_mount2_t)layout_resolve_sym("libSceSaveData", "sceSaveDataMount2");
    }

    fn_mount_info_t fn_minfo = (fn_mount_info_t)layout_resolve_sym("libSceSaveData_native", "$q8-aBtFHles");
    if (fn_minfo == NULL) {
        fn_minfo = (fn_mount_info_t)layout_resolve_sym("libSceSaveData_native", "sceSaveDataGetMountInfo");
    }
    if (fn_minfo == NULL) {
        fn_minfo = (fn_mount_info_t)layout_resolve_sym("libSceSaveData", "sceSaveDataGetMountInfo");
    }
    if (fn_minfo == NULL) {
        fn_minfo = (fn_mount_info_t)layout_resolve_sym("libSceSaveData", "sceSaveDataGetMountInfo2");
    }

    obs_report_measure(id, "sceSaveDataGetMountInfo", "resolved",
                       (uint64_t)(fn_minfo != NULL), "bool");
    obs_report_measure(id, "sceSaveDataMount2", "resolved",
                       (uint64_t)(fn_mount2 != NULL), "bool");

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        layout_probe init_probe;
        layout_prepare(&init_probe);
        int irc = fn_init((void *)init_probe.buffer);
        obs_report_measure(id, "init", "rc", (uint64_t)(uint32_t)irc, "code");

        if (irc == 0) {
            uint8_t mount_buf[512];
            memset(mount_buf, 0, sizeof(mount_buf));
            *(uint64_t *)(void *)(mount_buf + 0x28) = 0x60;

            if (fn_mount2 != NULL && obs_address_is_callable((const void *)fn_mount2)) {
                obs_jmp_buf guard;
                int sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    int mrc = fn_mount2(mount_buf);
                    obs_fault_unregister();
                    obs_report_measure(id, "mount-blocks-60", "rc", (uint64_t)(uint32_t)mrc, "code");

                    if (fn_minfo != NULL && obs_address_is_callable((const void *)fn_minfo)) {
                        uint8_t info_buf[256];
                        memset(info_buf, 0xaa, sizeof(info_buf));
                        int irc2 = fn_minfo(mount_buf, info_buf);
                        obs_report_measure(id, "mount-blocks-60", "info-rc", (uint64_t)(uint32_t)irc2, "code");
                        if (irc2 == 0) {
                            uint64_t blocks = *(uint64_t *)(void *)(info_buf + 0);
                            uint64_t free_blocks = *(uint64_t *)(void *)(info_buf + 8);
                            obs_report_measure(id, "mount-blocks-60", "blocks", blocks, "blocks");
                            obs_report_measure(id, "mount-blocks-60", "free-blocks", free_blocks, "blocks");
                        }
                    }
                } else {
                    obs_fault_unregister();
                    obs_report_measure(id, "mount-blocks-60", "fault-sig", (uint64_t)(uint32_t)sig, "sig");
                }
            }
            if (fn_term != NULL && obs_address_is_callable((const void *)fn_term)) {
                (void)fn_term();
            }
        }
    }

    return obs_pass();
}

static obs_result check_common_dialog_layout(void) {
    int32_t (*fn_init)(void) =
        (int32_t (*)(void))layout_resolve_sym("libSceCommonDialog", "sceCommonDialogInitialize");

    obs_report_measure("130-layout/common-dialog", "sceCommonDialogInitialize",
                       "resolved", (uint64_t)(fn_init != NULL ? 1 : 0), "bool");

    if (fn_init == NULL) {
        return obs_skip("sceCommonDialogInitialize not available in this context");
    }

    if (obs_address_is_callable((const void *)fn_init)) {
        int32_t rc1 = fn_init();
        obs_report_measure("130-layout/common-dialog", "sceCommonDialogInitialize",
                           "return_code", (uint64_t)(uint32_t)rc1, "code");

        int32_t rc2 = fn_init();
        obs_report_measure("130-layout/common-dialog", "sceCommonDialogInitialize",
                           "return_code_2", (uint64_t)(uint32_t)rc2, "code");

        int32_t (*fn_msg_init)(void) =
            (int32_t (*)(void))layout_resolve_sym("libSceCommonDialog", "sceMsgDialogInitialize");
        if (fn_msg_init == NULL) {
            fn_msg_init = (int32_t (*)(void))layout_resolve_sym("libSceMsgDialog", "sceMsgDialogInitialize");
        }
        obs_report_measure("130-layout/common-dialog", "sceMsgDialogInitialize",
                           "resolved", (uint64_t)(fn_msg_init != NULL ? 1 : 0), "bool");
        if (fn_msg_init != NULL && obs_address_is_callable((const void *)fn_msg_init)) {
            int32_t mrc = fn_msg_init();
            obs_report_measure("130-layout/common-dialog", "sceMsgDialogInitialize",
                               "return_code", (uint64_t)(uint32_t)mrc, "code");
        }

        return obs_pass_value((uint64_t)(uint32_t)rc1);
    }

    return obs_skip("sceCommonDialogInitialize address not callable");
}
/* clang-format on */

/* Fenced for the same reason as above. */
/* clang-format off */
static obs_result check_pad_controller_layout(void) {
    int32_t (*fn_pad_init)(void) =
        (int32_t (*)(void))obs_module_symbol(OBS_HANDLE_SELF, "scePadInit");
    int32_t (*fn_pad_get_handle)(int32_t userId, int32_t type, int32_t index) =
        (int32_t (*)(int32_t, int32_t, int32_t))obs_module_symbol(OBS_HANDLE_SELF,
                                                                  "scePadGetHandle");
    int32_t (*fn_pad_get_info)(int32_t handle, void *info) =
        (int32_t (*)(int32_t, void *))obs_module_symbol(
            OBS_HANDLE_SELF, "scePadGetControllerInformation");

    if (fn_pad_get_handle == NULL) {
        return obs_skip("libScePad symbols not available in this context");
    }

    if (fn_pad_init != NULL && obs_address_is_callable((const void *)fn_pad_init)) {
        (void)fn_pad_init();
    }

    int32_t handle = -1;
    if (obs_address_is_callable((const void *)fn_pad_get_handle)) {
        handle =
            fn_pad_get_handle(0xFF, 0, 0); /* Primary / active user, standard pad 0 */
        obs_report_measure("130-layout/pad-controller", "scePadGetHandle", "handle",
                           (uint64_t)(uint32_t)handle, "handle");
    }

    if (handle >= 0 && fn_pad_get_info != NULL &&
        obs_address_is_callable((const void *)fn_pad_get_info)) {
        layout_probe probe;
        layout_prepare(&probe);
        int rc = fn_pad_get_info(handle, probe.buffer);
        obs_report_measure("130-layout/pad-controller",
                           "scePadGetControllerInformation", "return_code",
                           (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes("130-layout/pad-controller", "scePadGetControllerInformation",
                         "controller_info", 0, probe.buffer, 128);
    }

    return obs_pass_value((uint64_t)(uint32_t)handle);
}
/* clang-format on */

struct obs_ifaddrs_entry {
    struct obs_ifaddrs_entry *ifa_next;
    char *ifa_name;
    uint32_t ifa_flags;
    void *ifa_addr;
    void *ifa_netmask;
    void *ifa_dstaddr;
    void *ifa_data;
};

/* Fenced for the same reason as above. */
/* clang-format off */
static obs_result check_network_interface_layout(void) {
    int32_t (*fn_getifaddrs)(struct obs_ifaddrs_entry **ifap) =
        (int32_t (*)(struct obs_ifaddrs_entry **))obs_module_symbol(OBS_HANDLE_SELF,
                                                                    "getifaddrs");
    void (*fn_freeifaddrs)(struct obs_ifaddrs_entry *ifap) =
        (void (*)(struct obs_ifaddrs_entry *))obs_module_symbol(OBS_HANDLE_SELF,
                                                                "freeifaddrs");

    if (fn_getifaddrs == NULL) {
        fn_getifaddrs = (int32_t (*)(struct obs_ifaddrs_entry **))obs_module_symbol(
            OBS_HANDLE_SELF, "sceNetGetifaddrs");
        fn_freeifaddrs = (void (*)(struct obs_ifaddrs_entry *))obs_module_symbol(
            OBS_HANDLE_SELF, "sceNetFreeifaddrs");
    }

    if (fn_getifaddrs == NULL ||
        !obs_address_is_callable((const void *)fn_getifaddrs)) {
        return obs_skip("getifaddrs / sceNetGetifaddrs not callable");
    }

    struct obs_ifaddrs_entry *ifap = NULL;
    int rc = fn_getifaddrs(&ifap);
    obs_report_measure("130-layout/net-interfaces", "getifaddrs", "return_code",
                       (uint64_t)(uint32_t)rc, "code");

    uint32_t count = 0;
    if (rc == 0 && ifap != NULL) {
        struct obs_ifaddrs_entry *curr = ifap;
        while (curr != NULL && count < 16) {
            if (curr->ifa_name != NULL) {
                obs_report_measure("130-layout/net-interfaces", curr->ifa_name, "flags",
                                   (uint64_t)curr->ifa_flags, "flags");
            }
            if (curr->ifa_addr != NULL) {
                obs_report_bytes("130-layout/net-interfaces",
                                 curr->ifa_name ? curr->ifa_name : "iface", "sockaddr",
                                 0, (const unsigned char *)curr->ifa_addr, 16);
            }
            curr = curr->ifa_next;
            count++;
        }
        if (fn_freeifaddrs != NULL &&
            obs_address_is_callable((const void *)fn_freeifaddrs)) {
            fn_freeifaddrs(ifap);
        }
    }

    return obs_pass_value((uint64_t)count);
}
/* clang-format on */

/* Subclass memory allocator callback counters */
static uint64_t s_json_alloc_calls = 0;
static uint64_t s_json_dealloc_calls = 0;
static size_t s_json_alloc_sizes[16];
static size_t s_json_alloc_aligns[16];
static size_t s_json_alloc_sizes_count = 0;
static uint8_t s_json_dummy_heap[65536];
static size_t s_json_dummy_offset = 0;

static void *custom_json_alloc(void *this_ptr, size_t size, uint32_t align) {
    (void)this_ptr;
    if (s_json_alloc_sizes_count < 16) {
        s_json_alloc_sizes[s_json_alloc_sizes_count] = size;
        s_json_alloc_aligns[s_json_alloc_sizes_count] = align;
        s_json_alloc_sizes_count++;
    }
    s_json_alloc_calls++;
    uint32_t eff_align = (align < 8) ? 8 : align;
    size_t aligned_off =
        (s_json_dummy_offset + (eff_align - 1)) & ~((size_t)eff_align - 1);
    if (aligned_off + size > sizeof(s_json_dummy_heap)) {
        return NULL;
    }
    s_json_dummy_offset = aligned_off + size;
    return &s_json_dummy_heap[aligned_off];
}

static void custom_json_dealloc(void *this_ptr, void *ptr) {
    (void)this_ptr;
    (void)ptr;
    s_json_dealloc_calls++;
}

static void custom_json_dtor(void *this_ptr) {
    (void)this_ptr;
}

static int s_json2_rtld_handle = -1;

static void *layout_resolve_json_sym(int mod_h, const char *name, const char *nid) {
    char dollar_nid[64] = {0};
    char bare_nid[64] = {0};
    char suffixed_ca[64] = {0};
    char dollar_suffixed_ca[64] = {0};
    char suffixed_cb[64] = {0};
    char suffixed_cc[64] = {0};
    char suffixed_ab[64] = {0};

    const char *cand[10];
    size_t cand_count = 0;

    if (nid != NULL) {
        if (nid[0] == '$') {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "%s", nid);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid + 1);
        } else {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "$%s", nid);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid);
        }
        oops_snprintf(suffixed_ca, sizeof(suffixed_ca), "%s#C#A", bare_nid);
        oops_snprintf(dollar_suffixed_ca, sizeof(dollar_suffixed_ca), "$%s#C#A",
                      bare_nid);
        oops_snprintf(suffixed_cb, sizeof(suffixed_cb), "%s#C#B", bare_nid);
        oops_snprintf(suffixed_cc, sizeof(suffixed_cc), "%s#B#C", bare_nid);
        oops_snprintf(suffixed_ab, sizeof(suffixed_ab), "%s#A#B", bare_nid);

        cand[cand_count++] = suffixed_ca;
        cand[cand_count++] = dollar_suffixed_ca;
        cand[cand_count++] = bare_nid;
        cand[cand_count++] = dollar_nid;
        cand[cand_count++] = suffixed_cb;
        cand[cand_count++] = suffixed_cc;
        cand[cand_count++] = suffixed_ab;
    }
    if (name != NULL) {
        cand[cand_count++] = name;
    }

    int handles[128];
    size_t hcount = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        (void)sceKernelGetModuleList(handles, 128, &hcount);
    }
    if (hcount > 128) {
        hcount = 128;
    }

    int try_handles[160];
    size_t n_try = 0;
    if (s_json2_rtld_handle > 0) {
        try_handles[n_try++] = s_json2_rtld_handle;
    }
    if (mod_h > 0 && mod_h != s_json2_rtld_handle) {
        try_handles[n_try++] = mod_h;
    }
    for (size_t i = 0; i < hcount; i++) {
        if (handles[i] > 0 && handles[i] != mod_h &&
            handles[i] != s_json2_rtld_handle) {
            try_handles[n_try++] = handles[i];
        }
    }
    try_handles[n_try++] = 1;
    try_handles[n_try++] = 2;
    try_handles[n_try++] = 0x2001;
    try_handles[n_try++] = OBS_HANDLE_SELF;

    void *ptr = NULL;
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (size_t i = 0; i < n_try; i++) {
            int h = try_handles[i];
            for (size_t c = 0; c < cand_count; c++) {
                if (sceKernelDlsym(h, cand[c], &ptr) == 0 &&
                    obs_address_is_callable(ptr)) {
                    s_json2_rtld_handle = h;
                    return ptr;
                }
            }
        }
        for (int h = 1; h <= 256; h++) {
            for (size_t c = 0; c < cand_count; c++) {
                if (sceKernelDlsym(h, cand[c], &ptr) == 0 &&
                    obs_address_is_callable(ptr)) {
                    s_json2_rtld_handle = h;
                    return ptr;
                }
            }
        }
    }

    for (size_t i = 0; i < n_try; i++) {
        int h = try_handles[i];
        for (size_t c = 0; c < cand_count; c++) {
            const void *sym = obs_module_symbol(h, cand[c]);
            if (sym != NULL && obs_address_is_callable(sym)) {
                return (void *)(uintptr_t)sym;
            }
        }
    }

    for (size_t c = 0; c < cand_count; c++) {
        const void *sym = layout_resolve_sym("libSceJson2", cand[c]);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return (void *)(uintptr_t)sym;
        }
        sym = layout_resolve_sym("libSceJson", cand[c]);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return (void *)(uintptr_t)sym;
        }
    }

#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        for (size_t c = 0; c < cand_count; c++) {
            uintptr_t kaddr = krw_dynlib_resolve_any(pid, cand[c]);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (void *)kaddr;
            }
        }
    }
#endif
    return NULL;
}

/* clang-format off */
static obs_result check_json2_init_layout(void) {
    /* 1. Ensure libSceJson2 is loaded / opened via sceKernelLoadStartModule or sysmodule */
    int mod_h = -1;
    int load_res = 0;
    int load_rc = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        load_rc = sceKernelLoadStartModule("/system/common/lib/libSceJson2.sprx", 0, NULL, 0, NULL, &load_res);
        obs_report_measure("130-layout/json2-init", "sceKernelLoadStartModule", "rc",
                           (uint64_t)(uint32_t)load_rc, "code");
        obs_report_measure("130-layout/json2-init", "sceKernelLoadStartModule", "res",
                           (uint64_t)(uint32_t)load_res, "code");
        if (load_rc > 0) {
            mod_h = load_rc;
        }
    }
    int sys_h = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        int r = sceSysmoduleLoadModule(0x00e7); /* SCE_SYSMODULE_JSON2 */
        obs_report_measure("130-layout/json2-init", "sysmodule_load_0x00e7", "rc",
                           (uint64_t)(uint32_t)r, "code");
    }
    if (obs_address_is_callable((const void *)&sceSysmoduleGetModuleHandleInternal)) {
        int r = sceSysmoduleGetModuleHandleInternal(0x00e7, &sys_h);
        obs_report_measure("130-layout/json2-init", "sysmodule_get_handle_0x00e7", "rc",
                           (uint64_t)(uint32_t)r, "code");
        obs_report_measure("130-layout/json2-init", "sysmodule_get_handle_0x00e7", "handle",
                           (uint64_t)(int64_t)sys_h, "val");
    }
    if (mod_h <= 0) {
        mod_h = obs_module_open("libSceJson2");
    }
    if (mod_h <= 0) {
        mod_h = obs_module_open("libSceJson2.sprx");
    }
    if (mod_h <= 0) {
        mod_h = obs_module_open("libSceJson");
    }
    if (mod_h <= 0 && sys_h > 0) {
        mod_h = sys_h;
    }
    if (mod_h > 0) {
        obs_sysmodule_cache_handle("libSceJson2", mod_h);
        obs_sysmodule_cache_handle("libSceJson", mod_h);
    }
    obs_report_measure("130-layout/json2-init", "libSceJson2", "handle",
                       (uint64_t)(int64_t)mod_h, "val");

    /* 2. Resolve target symbols via link imports or dynamically */
    void (*fn_memalloc_ctor)(void *) =
        (void (*)(void *))&sceJsonMemAllocatorCtor_static;
    if (fn_memalloc_ctor == NULL) {
        fn_memalloc_ctor = (void (*)(void *))&sceJsonMemAllocatorCtor_named;
    }
    if (fn_memalloc_ctor == NULL) {
        fn_memalloc_ctor = (void (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json12MemAllocatorC2Ev", "$-hJRce8wn1U");
    }

    void (*fn_memalloc_dtor)(void *) =
        (void (*)(void *))&sceJsonMemAllocatorDtor_static;
    if (fn_memalloc_dtor == NULL) {
        fn_memalloc_dtor = (void (*)(void *))&sceJsonMemAllocatorDtor_named;
    }
    if (fn_memalloc_dtor == NULL) {
        fn_memalloc_dtor = (void (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json12MemAllocatorD2Ev", "$OcAgPxcq5Vk");
    }

    void (*fn_init_ctor)(void *) =
        (void (*)(void *))&sceJsonInitializerCtor_static;
    if (fn_init_ctor == NULL) {
        fn_init_ctor = (void (*)(void *))&sceJsonInitializerCtor_named;
    }
    if (fn_init_ctor == NULL) {
        fn_init_ctor = (void (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json11InitializerC1Ev", "$cK6bYHf-Q5E");
    }

    void (*fn_init_dtor)(void *) =
        (void (*)(void *))&sceJsonInitializerDtor_static;
    if (fn_init_dtor == NULL) {
        fn_init_dtor = (void (*)(void *))&sceJsonInitializerDtor_named;
    }
    if (fn_init_dtor == NULL) {
        fn_init_dtor = (void (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json11InitializerD1Ev", "$RujUxbr3haM");
    }

    int (*fn_initialize)(void *, const void *) =
        (int (*)(void *, const void *))&sceJsonInitializerInitialize_static;
    if (fn_initialize == NULL) {
        fn_initialize =
            (int (*)(void *, const void *))&sceJsonInitializerInitialize_named;
    }
    if (fn_initialize == NULL) {
        fn_initialize = (int (*)(void *, const void *))layout_resolve_json_sym(
            mod_h,
            "_ZN3sce4Json11Initializer10initializeEPKNS0_13InitParameterE",
            "$Cxwy7wHq4J0");
    }

    int (*fn_terminate)(void *) =
        (int (*)(void *))&sceJsonInitializerTerminate_static;
    if (fn_terminate == NULL) {
        fn_terminate = (int (*)(void *))&sceJsonInitializerTerminate_named;
    }
    if (fn_terminate == NULL) {
        fn_terminate = (int (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json11Initializer9terminateEv", "$PR5k1penBLM");
    }

    void (*fn_param2_ctor)(void *) =
        (void (*)(void *))&sceJsonInitParameter2Ctor_static;
    if (fn_param2_ctor == NULL) {
        fn_param2_ctor = (void (*)(void *))&sceJsonInitParameter2Ctor_named;
    }
    if (fn_param2_ctor == NULL) {
        fn_param2_ctor = (void (*)(void *))layout_resolve_json_sym(
            mod_h, "_ZN3sce4Json14InitParameter2C1Ev", "$WSOuge5IsCg");
    }

    void (*fn_param2_setalloc)(void *, void *, void *) =
        (void (*)(void *, void *, void *))&
            sceJsonInitParameter2SetAllocator_static;
    if (fn_param2_setalloc == NULL) {
        fn_param2_setalloc = (void (*)(void *, void *, void *))&
            sceJsonInitParameter2SetAllocator_named;
    }
    if (fn_param2_setalloc == NULL) {
        fn_param2_setalloc = (void (*)(void *, void *, void *))
            layout_resolve_json_sym(
                mod_h,
                "_ZN3sce4Json14InitParameter212setAllocatorEPNS0_"
                "12MemAllocatorEPv",
                "$I2QC8PYhJWY");
    }

    obs_report_measure("130-layout/json2-init", "resolve", "memalloc_ctor",
                       (uint64_t)(fn_memalloc_ctor != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "memalloc_dtor",
                       (uint64_t)(fn_memalloc_dtor != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "init_ctor",
                       (uint64_t)(fn_init_ctor != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "init_dtor",
                       (uint64_t)(fn_init_dtor != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "initialize",
                       (uint64_t)(fn_initialize != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "terminate",
                       (uint64_t)(fn_terminate != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "param2_ctor",
                       (uint64_t)(fn_param2_ctor != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "resolve", "param2_setalloc",
                       (uint64_t)(fn_param2_setalloc != NULL), "bool");
    obs_report_measure("130-layout/json2-init", "libSceJson2", "rtld_handle",
                       (uint64_t)(int64_t)s_json2_rtld_handle, "val");

    obs_jmp_buf guard;
    int sig = 0;

    /* (1) Initializer() on a 0x40-byte 0xcc-filled object: an after-dump of the object */
    uint8_t init_obj1[0x40];
    memset(init_obj1, 0xcc, sizeof(init_obj1));
    uint8_t init_obj1_before[0x40];
    memcpy(init_obj1_before, init_obj1, sizeof(init_obj1));

    if (fn_init_ctor != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            fn_init_ctor(init_obj1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/json2-init", "arm1-ctor", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_written("130-layout/json2-init", "Initializer::Initializer",
                       "ctor_written", init_obj1_before, init_obj1, 0x40);
    obs_report_bytes("130-layout/json2-init", "arm1-ctor", "obj-after", 0, init_obj1, 0x40);

    /* (2) MemAllocator subclass over malloc/free counting calls + InitParameter + initialize(&param) */
    uint8_t base_alloc[0x40];
    memset(base_alloc, 0xcc, sizeof(base_alloc));
    if (fn_memalloc_ctor != NULL) {
        fn_memalloc_ctor(base_alloc);
    }
    obs_report_bytes("130-layout/json2-init", "MemAllocator", "base-obj-after", 0, base_alloc, 0x40);
    void **base_vptr = *(void ***)base_alloc;
    obs_report_measure("130-layout/json2-init", "MemAllocator", "vptr", (uint64_t)(uintptr_t)base_vptr, "addr");

    static void *subclass_vtable[16];
    for (int i = 0; i < 16; i++) {
        subclass_vtable[i] =
            (base_vptr != NULL &&
             obs_address_is_callable((const void *)base_vptr) && i < 8)
                ? base_vptr[i]
                : NULL;
    }
    if (subclass_vtable[0] == NULL) {
        subclass_vtable[0] = (void *)&custom_json_dtor;
    }
    if (subclass_vtable[1] == NULL) {
        subclass_vtable[1] = (void *)&custom_json_dtor;
    }
    subclass_vtable[2] = (void *)&custom_json_alloc;
    subclass_vtable[3] = (void *)&custom_json_dealloc;

    uint8_t sub_alloc[0x40];
    memset(sub_alloc, 0xcc, sizeof(sub_alloc));
    if (fn_memalloc_ctor != NULL) {
        fn_memalloc_ctor(sub_alloc);
    }
    *(void ***)sub_alloc = subclass_vtable;

    uint8_t param[0x40];
    memset(param, 0xcc, sizeof(param));
    if (fn_param2_ctor != NULL) {
        fn_param2_ctor(param);
        if (fn_param2_setalloc != NULL) {
            fn_param2_setalloc(param, sub_alloc, (void *)0x12345678ULL);
        } else {
            *(void **)param = (void *)sub_alloc;
            *(void **)(param + 8) = (void *)0x12345678ULL;
        }
    } else {
        *(void **)param = (void *)sub_alloc;
        *(void **)(param + 8) = NULL;
    }
    uint8_t param_before[0x40];
    memcpy(param_before, param, sizeof(param));

    uint8_t init_obj2[0x40];
    memset(init_obj2, 0xcc, sizeof(init_obj2));
    if (fn_init_ctor != NULL) {
        fn_init_ctor(init_obj2);
    }
    uint8_t init_obj2_before[0x40];
    memcpy(init_obj2_before, init_obj2, sizeof(init_obj2));

    s_json_alloc_calls = 0;
    s_json_dealloc_calls = 0;
    s_json_alloc_sizes_count = 0;

    int rc_init2 = -999;
    if (fn_initialize != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init2 = fn_initialize(init_obj2, param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/json2-init", "arm2-init", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure("130-layout/json2-init", "arm2-init", "rc", (uint64_t)(uint32_t)rc_init2, "code");
    obs_report_written("130-layout/json2-init", "arm2-init", "obj-written", init_obj2_before, init_obj2, 0x40);
    obs_report_bytes("130-layout/json2-init", "arm2-init", "obj-after", 0, init_obj2, 0x40);
    obs_report_written("130-layout/json2-init", "arm2-init", "param-written", param_before, param, 0x40);
    obs_report_bytes("130-layout/json2-init", "arm2-init", "param-after", 0, param, 0x40);
    obs_report_measure("130-layout/json2-init", "arm2-init", "alloc-calls", s_json_alloc_calls, "count");
    obs_report_measure("130-layout/json2-init", "arm2-init", "dealloc-calls", s_json_dealloc_calls, "count");
    for (size_t si = 0; si < s_json_alloc_sizes_count; si++) {
        char sz_tag[32];
        oops_snprintf(sz_tag, sizeof(sz_tag), "alloc-size-%zu", si);
        obs_report_measure("130-layout/json2-init", "arm2-init", sz_tag, (uint64_t)s_json_alloc_sizes[si], "bytes");
        char al_tag[32];
        oops_snprintf(al_tag, sizeof(al_tag), "alloc-align-%zu", si);
        obs_report_measure("130-layout/json2-init", "arm2-init", al_tag, (uint64_t)s_json_alloc_aligns[si], "bytes");
    }

    /* (3) initialize again on the same object: the rc for a second initialisation */
    int rc_reinit = -999;
    if (fn_initialize != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reinit = fn_initialize(init_obj2, param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/json2-init", "arm3-reinit", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure("130-layout/json2-init", "arm3-reinit", "rc", (uint64_t)(uint32_t)rc_reinit, "code");

    /* (4) terminate(): the rc and the allocator's counts after it */
    int rc_term = -999;
    if (fn_terminate != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term = fn_terminate(init_obj2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure("130-layout/json2-init", "arm4-term", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure("130-layout/json2-init", "arm4-term", "rc", (uint64_t)(uint32_t)rc_term, "code");
    obs_report_measure("130-layout/json2-init", "arm4-term", "alloc-calls", s_json_alloc_calls, "count");
    obs_report_measure("130-layout/json2-init", "arm4-term", "dealloc-calls", s_json_dealloc_calls, "count");
    obs_report_bytes("130-layout/json2-init", "arm4-term", "obj-after", 0, init_obj2, 0x40);

    /* Also clean up dtors */
    if (fn_init_dtor != NULL) {
        fn_init_dtor(init_obj1);
        fn_init_dtor(init_obj2);
    }
    if (fn_memalloc_dtor != NULL) {
        fn_memalloc_dtor(base_alloc);
        fn_memalloc_dtor(sub_alloc);
    }

    return obs_pass();
}
/* clang-format on */

static obs_result check_font_render_glyph(void) {
    const char *id = "130-layout/font-render-glyph";
    int load_rc = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        load_rc = sceSysmoduleLoadModule(0x000eu /* libSceFont */);
    }
    obs_report_measure(id, "sceSysmoduleLoadModule", "rc-font",
                       (uint64_t)(uint32_t)load_rc, "code");

    const void *fn_render_sym = layout_resolve_sym("libSceFont", "$T8SuAoYzbtw");
    if (!fn_render_sym)
        fn_render_sym = layout_resolve_sym("libSceFont", "sceFontRenderCharGlyphImage");
    if (!fn_render_sym)
        fn_render_sym =
            obs_module_symbol(OBS_HANDLE_SELF, "sceFontRenderCharGlyphImage");
    if (!fn_render_sym)
        fn_render_sym = obs_module_symbol(1, "sceFontRenderCharGlyphImage");

    const void *fn_render_h_sym = layout_resolve_sym("libSceFont", "$msNZLVunB5A");
    if (!fn_render_h_sym)
        fn_render_h_sym =
            layout_resolve_sym("libSceFont", "sceFontRenderCharGlyphImageHorizontal");
    if (!fn_render_h_sym)
        fn_render_h_sym =
            obs_module_symbol(OBS_HANDLE_SELF, "sceFontRenderCharGlyphImageHorizontal");
    if (!fn_render_h_sym)
        fn_render_h_sym = obs_module_symbol(1, "sceFontRenderCharGlyphImageHorizontal");

    obs_report_measure(id, "sceFontRenderCharGlyphImage", "resolved",
                       fn_render_sym ? 1 : 0, "bool");
    obs_report_measure(id, "sceFontRenderCharGlyphImageHorizontal", "resolved",
                       fn_render_h_sym ? 1 : 0, "bool");

    uint8_t surface[64 * 64];
    memset(surface, 0, sizeof(surface));
    uint8_t res_block[64];
    uint8_t met_block[32];

    const uint32_t chars[] = {'A', ' '};
    for (size_t c = 0; c < 2; c++) {
        uint32_t ch = chars[c];
        char tag_res[32], tag_met[32];
        oops_snprintf(tag_res, sizeof(tag_res), "res-%c", ch == ' ' ? '_' : (char)ch);
        oops_snprintf(tag_met, sizeof(tag_met), "met-%c", ch == ' ' ? '_' : (char)ch);

        if (fn_render_sym != NULL && obs_address_is_callable(fn_render_sym)) {
            memset(res_block, 0xCC, sizeof(res_block));
            memset(met_block, 0xCC, sizeof(met_block));
            obs_jmp_buf buf;
            int sig = OBS_FAULT_ARM(&buf);
            int rc = -1;
            if (sig == 0) {
                int (*fn)(void *, uint32_t, void *, void *) =
                    (int (*)(void *, uint32_t, void *, void *))fn_render_sym;
                rc = fn(NULL, ch, res_block, met_block);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "sceFontRenderCharGlyphImage", tag_res,
                               (uint64_t)(uint32_t)rc, "code");
            obs_report_buffer(id, "sceFontRenderCharGlyphImage", tag_res, res_block,
                              sizeof(res_block));
            obs_report_buffer(id, "sceFontRenderCharGlyphImage", tag_met, met_block,
                              sizeof(met_block));
        }

        if (fn_render_h_sym != NULL && obs_address_is_callable(fn_render_h_sym)) {
            memset(res_block, 0xCC, sizeof(res_block));
            memset(met_block, 0xCC, sizeof(met_block));
            obs_jmp_buf buf;
            int sig = OBS_FAULT_ARM(&buf);
            int rc = -1;
            if (sig == 0) {
                int (*fn)(void *, uint32_t, void *, void *) =
                    (int (*)(void *, uint32_t, void *, void *))fn_render_h_sym;
                rc = fn(NULL, ch, res_block, met_block);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "sceFontRenderCharGlyphImageHorizontal", tag_res,
                               (uint64_t)(uint32_t)rc, "code");
            obs_report_buffer(id, "sceFontRenderCharGlyphImageHorizontal", tag_res,
                              res_block, sizeof(res_block));
            obs_report_buffer(id, "sceFontRenderCharGlyphImageHorizontal", tag_met,
                              met_block, sizeof(met_block));
        }
    }

    return obs_pass();
}

static uint64_t s_font_destroy_cb_calls = 0;
static void *s_font_destroy_cb_arg = NULL;
static void s_font_destroy_cb(void *arg) {
    s_font_destroy_cb_calls++;
    s_font_destroy_cb_arg = arg;
}

static obs_result check_font_memory_init(void) {
    const char *id = "130-layout/font-memory-init";

    /* Ensure libSceFont is loaded */
    int load_rc = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        load_rc = sceSysmoduleLoadModule(0x000eu /* libSceFont */);
    }
    obs_report_measure(id, "sceSysmoduleLoadModule", "rc-font",
                       (uint64_t)(uint32_t)load_rc, "code");

    int h_font = obs_module_open("libSceFont");
    if (h_font < 0) {
        h_font = obs_module_open("libSceFont.sprx");
    }
    if (h_font < 0 &&
        obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        int res = 0;
        int lr = sceKernelLoadStartModule("/system/common/lib/libSceFont.sprx", 0, NULL,
                                          0, NULL, &res);
        if (lr > 0)
            h_font = lr;
    }

    const void *fn_init = (const void *)&sceFontMemoryInit_static;
    if (!obs_address_is_callable(fn_init)) {
        fn_init = (const void *)&sceFontMemoryInit;
    }
    if (!obs_address_is_callable(fn_init) && h_font >= 0) {
        fn_init = obs_module_symbol(h_font, "sceFontMemoryInit");
        if (!fn_init)
            fn_init = obs_module_symbol(h_font, "$whrS4oksXc4");
    }
    if (!fn_init && obs_address_is_callable((const void *)&sceKernelDlsym) &&
        h_font > 0) {
        (void)sceKernelDlsym(h_font, "sceFontMemoryInit", (void **)&fn_init);
        if (!fn_init)
            (void)sceKernelDlsym(h_font, "$whrS4oksXc4", (void **)&fn_init);
    }
    if (!fn_init) {
        fn_init = layout_resolve_sym("libSceFont", "sceFontMemoryInit");
    }
    if (!fn_init) {
        fn_init = layout_resolve_sym("libSceFont", "$whrS4oksXc4");
    }

    const void *fn_term = (const void *)&sceFontMemoryTerm_static;
    if (!obs_address_is_callable(fn_term)) {
        fn_term = (const void *)&sceFontMemoryTerm;
    }
    if (!obs_address_is_callable(fn_term) && h_font >= 0) {
        fn_term = obs_module_symbol(h_font, "sceFontMemoryTerm");
        if (!fn_term)
            fn_term = obs_module_symbol(h_font, "$h6hIgxXEiEc");
    }
    if (!fn_term && obs_address_is_callable((const void *)&sceKernelDlsym) &&
        h_font > 0) {
        (void)sceKernelDlsym(h_font, "sceFontMemoryTerm", (void **)&fn_term);
        if (!fn_term)
            (void)sceKernelDlsym(h_font, "$h6hIgxXEiEc", (void **)&fn_term);
    }
    if (!fn_term) {
        fn_term = layout_resolve_sym("libSceFont", "sceFontMemoryTerm");
    }
    if (!fn_term) {
        fn_term = layout_resolve_sym("libSceFont", "$h6hIgxXEiEc");
    }

    obs_report_measure(id, "resolve", "init-callable",
                       fn_init && obs_address_is_callable(fn_init) ? 1 : 0, "bool");
    obs_report_measure(id, "resolve", "term-callable",
                       fn_term && obs_address_is_callable(fn_term) ? 1 : 0, "bool");

    /* Resolve mspace create / destroy */
    int h_libc = obs_module_open("libSceLibcInternal");
    void *(*fn_msp_create)(const char *, void *, size_t, uint64_t) =
        (void *(*)(const char *, void *, size_t, uint64_t)) &
        sceLibcMspaceCreate_static;
    if (!obs_address_is_callable((const void *)fn_msp_create)) {
        fn_msp_create =
            (void *(*)(const char *, void *, size_t, uint64_t)) & sceLibcMspaceCreate;
    }
    if (!obs_address_is_callable((const void *)fn_msp_create) && h_libc >= 0) {
        fn_msp_create =
            (void *(*)(const char *, void *, size_t, uint64_t))obs_module_symbol(
                h_libc, "sceLibcMspaceCreate");
        if (!fn_msp_create) {
            fn_msp_create =
                (void *(*)(const char *, void *, size_t, uint64_t))obs_module_symbol(
                    h_libc, "$-hn1tcVHq5Q");
        }
    }
    int (*fn_msp_destroy)(void *) = (int (*)(void *))&sceLibcMspaceDestroy_static;
    if (!obs_address_is_callable((const void *)fn_msp_destroy)) {
        fn_msp_destroy = (int (*)(void *))&sceLibcMspaceDestroy;
    }
    if (!obs_address_is_callable((const void *)fn_msp_destroy) && h_libc >= 0) {
        fn_msp_destroy =
            (int (*)(void *))obs_module_symbol(h_libc, "sceLibcMspaceDestroy");
        if (!fn_msp_destroy) {
            fn_msp_destroy = (int (*)(void *))obs_module_symbol(h_libc, "$W6SiVSiCDtI");
        }
    }

    if (!fn_init || !obs_address_is_callable(fn_init)) {
        return obs_skip("sceFontMemoryInit symbol not callable");
    }

    /* Allocate base buffer: 0x10000 bytes */
    static _Alignas(4096) uint8_t s_font_base[0x10000];
    uint8_t *base = s_font_base;
    memset(base, 0xcc, 0x10000);

    /* Allocate and init obj: 0x40 bytes */
    uint8_t obj[0x40];
    memset(obj, 0xcc, sizeof(obj));

    /* memIf: 0x40-byte table of five distinct sentinel pointers */
    void *memIf[8];
    memIf[0] = (void *)0x11110001ULL;
    memIf[1] = (void *)0x11110002ULL;
    memIf[2] = (void *)0x11110003ULL;
    memIf[3] = (void *)0x11110004ULL;
    memIf[4] = (void *)0x11110005ULL;
    memIf[5] = (void *)0x11110006ULL;
    memIf[6] = (void *)0x11110007ULL;
    memIf[7] = (void *)0x11110008ULL;

    void *destroyCb = (void *)0x22220001ULL;

    void *msp = (void *)0x33330001ULL;
    if (fn_msp_create && obs_address_is_callable((const void *)fn_msp_create)) {
        obs_jmp_buf g;
        if (OBS_FAULT_ARM(&g) == 0) {
            void *created_msp = fn_msp_create("font_msp", base, 0x10000, 0);
            obs_fault_unregister();
            if (created_msp)
                msp = created_msp;
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "msp", "created", (uint64_t)(uintptr_t)msp, "addr");

    /* Snapshots before call (1) */
    uint8_t obj_before[0x40];
    memcpy(obj_before, obj, sizeof(obj));
    uint8_t base_before[0x40];
    memcpy(base_before, base, sizeof(base_before));
    uint8_t memIf_before[0x40];
    memcpy(memIf_before, memIf, sizeof(memIf_before));

    /* (1) sceFontMemoryInit(obj, base, 0x10000, memIf, msp, destroyCb, 0) */
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int rc1 = -1;
    if (sig == 0) {
        rc1 =
            ((int (*)(void *, void *, uint32_t, void *, void *, void *,
                      uint64_t))fn_init)(obj, base, 0x10000, memIf, msp, destroyCb, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm1-init", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm1-init", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_written(id, "sceFontMemoryInit", "obj", obj_before, obj, sizeof(obj));
    obs_report_written(id, "sceFontMemoryInit", "base", base_before, base,
                       sizeof(base_before));
    obs_report_written(id, "sceFontMemoryInit", "memIf", memIf_before,
                       (const uint8_t *)memIf, sizeof(memIf));
    obs_report_bytes(id, "arm1-init", "obj-after", 0, obj, sizeof(obj));
    obs_report_bytes(id, "arm1-init", "base-after-head40", 0, base,
                     sizeof(base_before));
    obs_report_bytes(id, "arm1-init", "memIf-after", 0, (const uint8_t *)memIf,
                     sizeof(memIf));

    /* (2) Invalid parameters: obj = NULL, and size = 0 */
    int rc_null_obj = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null_obj =
            ((int (*)(void *, void *, uint32_t, void *, void *, void *,
                      uint64_t))fn_init)(NULL, base, 0x10000, memIf, msp, destroyCb, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm2-null-obj", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm2-null-obj", "rc", (uint64_t)(uint32_t)rc_null_obj,
                       "code");

    uint8_t obj_zero[0x40];
    memset(obj_zero, 0xcc, sizeof(obj_zero));
    int rc_zero_size = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_zero_size =
            ((int (*)(void *, void *, uint32_t, void *, void *, void *,
                      uint64_t))fn_init)(obj_zero, base, 0, memIf, msp, destroyCb, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm2-zero-size", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm2-zero-size", "rc", (uint64_t)(uint32_t)rc_zero_size,
                       "code");

    /* (3) sceFontMemoryTerm(obj) after (1) */
    uint8_t obj_pre_term[0x40];
    memcpy(obj_pre_term, obj, sizeof(obj));
    int rc3 = -1;
    if (fn_term && obs_address_is_callable(fn_term)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc3 = ((int (*)(void *))fn_term)(obj);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm3-term", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "arm3-term", "rc", (uint64_t)(uint32_t)rc3, "code");
    obs_report_written(id, "sceFontMemoryTerm", "obj-term", obj_pre_term, obj,
                       sizeof(obj));
    obs_report_bytes(id, "arm3-term", "obj-after-term", 0, obj, sizeof(obj));

    /* (3b) Re-init with callable callback and term cleanly */
    uint8_t obj_cb[0x40];
    memset(obj_cb, 0xcc, sizeof(obj_cb));
    uint8_t obj_cb_before[0x40];
    memcpy(obj_cb_before, obj_cb, sizeof(obj_cb));

    int rc_init_cb = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init_cb = ((int (*)(void *, void *, uint32_t, void *, void *, void *,
                               uint64_t))fn_init)(obj_cb, base, 0x10000, memIf, msp,
                                                  (void *)&s_font_destroy_cb, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "arm3b-init", "rc", (uint64_t)(uint32_t)rc_init_cb, "code");

    uint8_t obj_cb_pre_term[0x40];
    memcpy(obj_cb_pre_term, obj_cb, sizeof(obj_cb));
    int rc_term_cb = -1;
    if (fn_term && obs_address_is_callable(fn_term)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term_cb = ((int (*)(void *))fn_term)(obj_cb);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm3b-term", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "arm3b-term", "rc", (uint64_t)(uint32_t)rc_term_cb, "code");
    obs_report_measure(id, "arm3b-term", "cb-calls", s_font_destroy_cb_calls, "count");
    obs_report_measure(id, "arm3b-term", "cb-arg",
                       (uint64_t)(uintptr_t)s_font_destroy_cb_arg, "addr");
    obs_report_written(id, "sceFontMemoryTerm", "obj-term-cb", obj_cb_pre_term, obj_cb,
                       sizeof(obj_cb));
    obs_report_bytes(id, "arm3b-term", "obj-after-term-cb", 0, obj_cb, sizeof(obj_cb));

    /* Cleanup */
    if (msp != (void *)0x33330001ULL && fn_msp_destroy &&
        obs_address_is_callable((const void *)fn_msp_destroy)) {
        fn_msp_destroy(msp);
    }

    return obs_pass();
}

/* REQ-20261007T0850Z-fm02: sceFontSelectLibraryFt and sceFontCreateLibraryWithEdition
 * in PPSA21564's call shape */
static uint64_t s_font_ft_destroy_cb_calls = 0;
static void *s_font_ft_destroy_cb_arg = NULL;
static void s_font_ft_destroy_cb(void *arg) {
    s_font_ft_destroy_cb_calls++;
    s_font_ft_destroy_cb_arg = arg;
}

static uint64_t s_font_ft_alloc_calls = 0;
static uint64_t s_font_ft_free_calls = 0;
static void *(*s_fn_msp_malloc)(void *msp, size_t size) = NULL;
static void (*s_fn_msp_free)(void *msp, void *ptr) = NULL;

static void *s_font_ft_alloc(void *msp, size_t size) {
    s_font_ft_alloc_calls++;
    if (s_fn_msp_malloc && obs_address_is_callable((const void *)s_fn_msp_malloc)) {
        return s_fn_msp_malloc(msp, size);
    }
    return malloc(size);
}

static void s_font_ft_free(void *msp, void *ptr) {
    s_font_ft_free_calls++;
    if (s_fn_msp_free && obs_address_is_callable((const void *)s_fn_msp_free)) {
        s_fn_msp_free(msp, ptr);
    } else {
        free(ptr);
    }
}

static obs_result check_font_create_library_ft(void) {
    const char *id = "130-layout/font-create-library-ft";
    obs_jmp_buf guard;

    /* Sysmodule load */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x000eu /* libSceFont */);
    }
    int res = 0;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        (void)sceKernelLoadStartModule("/system/common/lib/libSceFont.sprx", 0, NULL, 0,
                                       NULL, &res);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceFontFt.sprx", 0, NULL,
                                       0, NULL, &res);
    }

    void *(*fn_select_ft)(uint64_t) =
        (void *(*)(uint64_t)) & sceFontSelectLibraryFt_static;
    if (!obs_address_is_callable((const void *)fn_select_ft)) {
        fn_select_ft = (void *(*)(uint64_t)) & sceFontSelectLibraryFt;
    }
    if (!obs_address_is_callable((const void *)fn_select_ft)) {
        fn_select_ft = (void *(*)(uint64_t))layout_resolve_sym(
            "libSceFontFt", "sceFontSelectLibraryFt");
    }
    if (!obs_address_is_callable((const void *)fn_select_ft)) {
        fn_select_ft =
            (void *(*)(uint64_t))layout_resolve_sym("libSceFontFt", "$oM+XCzVG3oM");
    }

    int (*fn_create_lib)(void *, void *, uint64_t, void **) = (int (*)(
        void *, void *, uint64_t, void **))&sceFontCreateLibraryWithEdition_static;
    if (!obs_address_is_callable((const void *)fn_create_lib)) {
        fn_create_lib = (int (*)(void *, void *, uint64_t,
                                 void **))&sceFontCreateLibraryWithEdition;
    }
    if (!obs_address_is_callable((const void *)fn_create_lib)) {
        fn_create_lib = (int (*)(void *, void *, uint64_t, void **))layout_resolve_sym(
            "libSceFont", "sceFontCreateLibraryWithEdition");
    }
    if (!obs_address_is_callable((const void *)fn_create_lib)) {
        fn_create_lib = (int (*)(void *, void *, uint64_t, void **))layout_resolve_sym(
            "libSceFont", "$n590hj5Oe-k");
    }

    int (*fn_destroy_lib)(void **) = (int (*)(void **))&sceFontDestroyLibrary_static;
    if (!obs_address_is_callable((const void *)fn_destroy_lib)) {
        fn_destroy_lib = (int (*)(void **))&sceFontDestroyLibrary;
    }
    if (!obs_address_is_callable((const void *)fn_destroy_lib)) {
        fn_destroy_lib =
            (int (*)(void **))layout_resolve_sym("libSceFont", "sceFontDestroyLibrary");
    }
    if (!obs_address_is_callable((const void *)fn_destroy_lib)) {
        fn_destroy_lib =
            (int (*)(void **))layout_resolve_sym("libSceFont", "$FXP359ygujs");
    }

    int (*fn_mem_init)(void *, void *, uint32_t, void *, void *, void *, uint64_t) =
        (int (*)(void *, void *, uint32_t, void *, void *, void *,
                 uint64_t))&sceFontMemoryInit_static;
    if (!obs_address_is_callable((const void *)fn_mem_init)) {
        fn_mem_init = (int (*)(void *, void *, uint32_t, void *, void *, void *,
                               uint64_t))&sceFontMemoryInit;
    }
    if (!obs_address_is_callable((const void *)fn_mem_init)) {
        fn_mem_init =
            (int (*)(void *, void *, uint32_t, void *, void *, void *,
                     uint64_t))layout_resolve_sym("libSceFont", "sceFontMemoryInit");
    }

    void *(*fn_msp_create)(const char *, void *, size_t, uint64_t) =
        (void *(*)(const char *, void *, size_t, uint64_t)) &
        sceLibcMspaceCreate_static;
    if (!obs_address_is_callable((const void *)fn_msp_create)) {
        fn_msp_create =
            (void *(*)(const char *, void *, size_t, uint64_t)) & sceLibcMspaceCreate;
    }
    if (!obs_address_is_callable((const void *)fn_msp_create)) {
        fn_msp_create =
            (void *(*)(const char *, void *, size_t, uint64_t))layout_resolve_sym(
                "libSceLibcInternal", "sceLibcMspaceCreate");
    }

    int (*fn_msp_destroy)(void *) = (int (*)(void *))&sceLibcMspaceDestroy_static;
    if (!obs_address_is_callable((const void *)fn_msp_destroy)) {
        fn_msp_destroy = (int (*)(void *))&sceLibcMspaceDestroy;
    }
    if (!obs_address_is_callable((const void *)fn_msp_destroy)) {
        fn_msp_destroy = (int (*)(void *))layout_resolve_sym("libSceLibcInternal",
                                                             "sceLibcMspaceDestroy");
    }

    s_fn_msp_malloc = (void *(*)(void *, size_t))layout_resolve_sym(
        "libSceLibcInternal", "sceLibcMspaceMalloc");
    if (!s_fn_msp_malloc) {
        s_fn_msp_malloc = (void *(*)(void *, size_t))layout_resolve_sym(
            "libSceLibcInternal", "$OJjm-QOIHlI");
    }
    s_fn_msp_free = (void (*)(void *, void *))layout_resolve_sym("libSceLibcInternal",
                                                                 "sceLibcMspaceFree");
    if (!s_fn_msp_free) {
        s_fn_msp_free = (void (*)(void *, void *))layout_resolve_sym(
            "libSceLibcInternal", "$Vla-Z+eXlxo");
    }

    obs_report_measure(
        id, "resolve", "select-ft-callable",
        fn_select_ft && obs_address_is_callable((const void *)fn_select_ft) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "create-lib-callable",
        fn_create_lib && obs_address_is_callable((const void *)fn_create_lib) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "destroy-lib-callable",
        fn_destroy_lib && obs_address_is_callable((const void *)fn_destroy_lib) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "mem-init-callable",
        fn_mem_init && obs_address_is_callable((const void *)fn_mem_init) ? 1 : 0,
        "bool");

    if (!fn_select_ft || !fn_create_lib || !fn_mem_init) {
        return obs_skip("required font symbols not callable");
    }

    /* (1) sceFontSelectLibraryFt(0) and (1) */
    void *sel0 = NULL;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sel0 = fn_select_ft(0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm1-select", "fault-0", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm1-select", "sel0", (uint64_t)(uintptr_t)sel0, "addr");
    if ((uintptr_t)sel0 >= 0x10000UL) {
        uint8_t sel0_buf[64];
        memset(sel0_buf, 0, sizeof(sel0_buf));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            memcpy(sel0_buf, sel0, 0x40);
            obs_fault_unregister();
            obs_report_bytes(id, "arm1-select", "sel0-40b", 0, sel0_buf, 0x40);
        } else {
            obs_fault_unregister();
        }
    }

    void *sel1 = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sel1 = fn_select_ft(1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm1-select", "fault-1", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm1-select", "sel1", (uint64_t)(uintptr_t)sel1, "addr");
    if ((uintptr_t)sel1 >= 0x10000UL) {
        uint8_t sel1_buf[64];
        memset(sel1_buf, 0, sizeof(sel1_buf));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            memcpy(sel1_buf, sel1, 0x40);
            obs_fault_unregister();
            obs_report_bytes(id, "arm1-select", "sel1-40b", 0, sel1_buf, 0x40);
        } else {
            obs_fault_unregister();
        }
    }

    /* Setup font memory object: mspace over 0x10000 buffer, destroy callback */
    static _Alignas(4096) uint8_t s_font_ft_base[0x10000];
    memset(s_font_ft_base, 0xcc, sizeof(s_font_ft_base));
    void *msp = NULL;
    if (fn_msp_create && obs_address_is_callable((const void *)fn_msp_create)) {
        msp = fn_msp_create("font_ft_msp", s_font_ft_base, sizeof(s_font_ft_base), 0);
    }

    /* Arm (2a): literal 5 sentinels from -fm01 arm 1 */
    uint8_t obj_sentinel[0x40];
    memset(obj_sentinel, 0xcc, sizeof(obj_sentinel));
    void *memIf_sentinel[8];
    memIf_sentinel[0] = (void *)0x11110001ULL;
    memIf_sentinel[1] = (void *)0x11110002ULL;
    memIf_sentinel[2] = (void *)0x11110003ULL;
    memIf_sentinel[3] = (void *)0x11110004ULL;
    memIf_sentinel[4] = (void *)0x11110005ULL;
    memIf_sentinel[5] = (void *)0x11110006ULL;
    memIf_sentinel[6] = (void *)0x11110007ULL;
    memIf_sentinel[7] = (void *)0x11110008ULL;
    (void)fn_mem_init(obj_sentinel, s_font_ft_base, sizeof(s_font_ft_base),
                      memIf_sentinel, msp, (void *)&s_font_ft_destroy_cb, 0);

    uint64_t lib_sentinel = 0xccccccccccccccccULL;
    int rc_sentinel = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_sentinel = fn_create_lib(obj_sentinel, sel0, 0x0007001000000000ULL,
                                    (void **)&lib_sentinel);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm2a-sentinel", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm2a-sentinel", "rc", (uint64_t)(uint32_t)rc_sentinel,
                       "code");
    obs_report_measure(id, "arm2a-sentinel", "library", lib_sentinel, "addr");

    /* Arm (2b): callable memIf allocator (so library is created, allowing dump &
     * destroy) */
    uint8_t obj[0x40];
    memset(obj, 0xcc, sizeof(obj));
    void *memIf_callable[8];
    memIf_callable[0] = (void *)&s_font_ft_alloc;
    memIf_callable[1] = (void *)&s_font_ft_free;
    memIf_callable[2] = (void *)&s_font_ft_alloc;
    memIf_callable[3] = (void *)&s_font_ft_alloc;
    memIf_callable[4] = (void *)&s_font_ft_alloc;
    memIf_callable[5] = NULL;
    memIf_callable[6] = NULL;
    memIf_callable[7] = NULL;
    (void)fn_mem_init(obj, s_font_ft_base, sizeof(s_font_ft_base), memIf_callable, msp,
                      (void *)&s_font_ft_destroy_cb, 0);

    uint8_t base_before[0x40];
    memcpy(base_before, s_font_ft_base, sizeof(base_before));
    uint8_t obj_before[0x40];
    memcpy(obj_before, obj, sizeof(obj));

    uint64_t library = 0xccccccccccccccccULL;
    int rc2 = -1;
    s_font_ft_alloc_calls = 0;
    s_font_ft_free_calls = 0;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_create_lib(obj, sel0, 0x0007001000000000ULL, (void **)&library);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm2-create", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm2-create", "rc", (uint64_t)(uint32_t)rc2, "code");
    obs_report_measure(id, "arm2-create", "library", library, "addr");
    obs_report_measure(id, "arm2-create", "alloc-calls", s_font_ft_alloc_calls,
                       "count");

    if (library >= 0x10000UL) {
        uint8_t lib_bytes[0x100];
        memset(lib_bytes, 0, sizeof(lib_bytes));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            memcpy(lib_bytes, (const void *)(uintptr_t)library, sizeof(lib_bytes));
            obs_fault_unregister();
            for (uint32_t off = 0; off < 0x100; off += 64) {
                obs_report_bytes(id, "arm2-create", "lib-bytes", off, lib_bytes + off,
                                 64);
            }
        } else {
            obs_fault_unregister();
        }
    }

    obs_report_bytes(id, "arm2-create", "obj-after", 0, obj, sizeof(obj));
    obs_report_written(id, "sceFontCreateLibraryWithEdition", "mspace-head40",
                       base_before, s_font_ft_base, sizeof(base_before));
    obs_report_bytes(id, "arm2-create", "mspace-after-head40", 0, s_font_ft_base,
                     sizeof(base_before));

    /* (3) memory = NULL and edition = 0 */
    uint64_t lib_null_mem = 0xccccccccccccccccULL;
    int rc_null_mem = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null_mem =
            fn_create_lib(NULL, sel0, 0x0007001000000000ULL, (void **)&lib_null_mem);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm3-null-mem", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm3-null-mem", "rc", (uint64_t)(uint32_t)rc_null_mem,
                       "code");
    obs_report_measure(id, "arm3-null-mem", "library", lib_null_mem, "addr");

    uint64_t lib_zero_ed = 0xccccccccccccccccULL;
    int rc_zero_ed = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_zero_ed = fn_create_lib(obj, sel0, 0, (void **)&lib_zero_ed);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm3-zero-ed", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm3-zero-ed", "rc", (uint64_t)(uint32_t)rc_zero_ed,
                       "code");
    obs_report_measure(id, "arm3-zero-ed", "library", lib_zero_ed, "addr");
    if (lib_zero_ed >= 0x10000UL && fn_destroy_lib) {
        (void)fn_destroy_lib((void **)&lib_zero_ed);
    }

    /* (4) sceFontDestroyLibrary(&library) */
    int rc4 = -1;
    if (fn_destroy_lib) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc4 = fn_destroy_lib((void **)&library);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm4-destroy", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "arm4-destroy", "rc", (uint64_t)(uint32_t)rc4, "code");
    obs_report_measure(id, "arm4-destroy", "library-after", library, "addr");
    obs_report_measure(id, "arm4-destroy", "free-calls", s_font_ft_free_calls, "count");

    /* Cleanup mspace */
    if (msp && fn_msp_destroy &&
        obs_address_is_callable((const void *)fn_msp_destroy)) {
        fn_msp_destroy(msp);
    }

    return obs_pass();
}

/* REQ-20261007T0915Z-ht01: sceHttp2Init after sceNetPoolCreate and sceSslInit */
static obs_result check_http2_init(void) {
    const char *id = "130-layout/http2-init";
    obs_jmp_buf guard;

    /* Sysmodule load */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0013u /* libSceNet */);
        (void)sceSysmoduleLoadModule(0x0017u /* libSceSsl */);
        (void)sceSysmoduleLoadModule(0x001bu /* libSceHttp2 */);
    }
    int res = 0;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        (void)sceKernelLoadStartModule("/system/common/lib/libSceNet.sprx", 0, NULL, 0,
                                       NULL, &res);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceSsl.sprx", 0, NULL, 0,
                                       NULL, &res);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceHttp2.sprx", 0, NULL,
                                       0, NULL, &res);
    }

    /* Initialize net */
    int (*fn_net_init)(void) = (int (*)(void))&sceNetInit;
    if (!obs_address_is_callable((const void *)fn_net_init)) {
        fn_net_init = (int (*)(void))layout_resolve_sym("libSceNet", "sceNetInit");
    }
    if (fn_net_init && obs_address_is_callable((const void *)fn_net_init)) {
        (void)fn_net_init();
    }

    int (*fn_pool_create)(const char *, int, int) =
        (int (*)(const char *, int, int))&sceNetPoolCreate_static;
    if (!obs_address_is_callable((const void *)fn_pool_create)) {
        fn_pool_create = (int (*)(const char *, int, int))&sceNetPoolCreate;
    }
    if (!obs_address_is_callable((const void *)fn_pool_create)) {
        fn_pool_create = (int (*)(const char *, int, int))layout_resolve_sym(
            "libSceNet", "sceNetPoolCreate");
    }
    if (!obs_address_is_callable((const void *)fn_pool_create)) {
        fn_pool_create = (int (*)(const char *, int, int))layout_resolve_sym(
            "libSceNet", "$dgJBaeJnGpo");
    }

    int (*fn_ssl_init)(size_t) = (int (*)(size_t))&sceSslInit_static;
    if (!obs_address_is_callable((const void *)fn_ssl_init)) {
        fn_ssl_init = (int (*)(size_t))&sceSslInit;
    }
    if (!obs_address_is_callable((const void *)fn_ssl_init)) {
        fn_ssl_init = (int (*)(size_t))layout_resolve_sym("libSceSsl", "sceSslInit");
    }
    if (!obs_address_is_callable((const void *)fn_ssl_init)) {
        fn_ssl_init = (int (*)(size_t))layout_resolve_sym("libSceSsl", "$hdpVEUDFW3s");
    }

    int (*fn_ssl_term)(int) = (int (*)(int))&sceSslTerm_static;
    if (!obs_address_is_callable((const void *)fn_ssl_term)) {
        fn_ssl_term = (int (*)(int))&sceSslTerm;
    }
    if (!obs_address_is_callable((const void *)fn_ssl_term)) {
        fn_ssl_term = (int (*)(int))layout_resolve_sym("libSceSsl", "sceSslTerm");
    }
    if (!obs_address_is_callable((const void *)fn_ssl_term)) {
        fn_ssl_term = (int (*)(int))layout_resolve_sym("libSceSsl", "$0K1yQ6Lv-Yc");
    }

    int (*fn_http2_init)(int, int, size_t, int) =
        (int (*)(int, int, size_t, int))&sceHttp2Init_static;
    if (!obs_address_is_callable((const void *)fn_http2_init)) {
        fn_http2_init = (int (*)(int, int, size_t, int))&sceHttp2Init;
    }
    if (!obs_address_is_callable((const void *)fn_http2_init)) {
        fn_http2_init = (int (*)(int, int, size_t, int))layout_resolve_sym(
            "libSceHttp2", "sceHttp2Init");
    }
    if (!obs_address_is_callable((const void *)fn_http2_init)) {
        fn_http2_init = (int (*)(int, int, size_t, int))layout_resolve_sym(
            "libSceHttp2", "$3JCe3lCbQ8A");
    }

    int (*fn_http2_term)(int) = (int (*)(int))&sceHttp2Term_static;
    if (!obs_address_is_callable((const void *)fn_http2_term)) {
        fn_http2_term = (int (*)(int))&sceHttp2Term;
    }
    if (!obs_address_is_callable((const void *)fn_http2_term)) {
        fn_http2_term = (int (*)(int))layout_resolve_sym("libSceHttp2", "sceHttp2Term");
    }
    if (!obs_address_is_callable((const void *)fn_http2_term)) {
        fn_http2_term = (int (*)(int))layout_resolve_sym("libSceHttp2", "$YiBUtz-pGkc");
    }

    obs_report_measure(
        id, "resolve", "pool-create-callable",
        fn_pool_create && obs_address_is_callable((const void *)fn_pool_create) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "ssl-init-callable",
        fn_ssl_init && obs_address_is_callable((const void *)fn_ssl_init) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "http2-init-callable",
        fn_http2_init && obs_address_is_callable((const void *)fn_http2_init) ? 1 : 0,
        "bool");
    obs_report_measure(
        id, "resolve", "http2-term-callable",
        fn_http2_term && obs_address_is_callable((const void *)fn_http2_term) ? 1 : 0,
        "bool");

    if (!fn_pool_create || !fn_ssl_init || !fn_http2_init || !fn_http2_term) {
        return obs_skip("required http2 / net / ssl symbols not callable");
    }

    /* (1) sceNetPoolCreate("http2", 0x40000, 0), then sceSslInit(0x40000) */
    int pool_id = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        pool_id = fn_pool_create("http2", 0x40000, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm1-pool-ssl", "pool-fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm1-pool-ssl", "pool-id", (uint64_t)(uint32_t)pool_id,
                       "id");

    int ssl_id = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        ssl_id = fn_ssl_init(0x40000);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm1-pool-ssl", "ssl-fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm1-pool-ssl", "ssl-id", (uint64_t)(uint32_t)ssl_id, "id");

    /* (2) sceHttp2Init(pool, ssl, 0x40000, 1): rc; if id, call again same way */
    int h2_id = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        h2_id = fn_http2_init(pool_id, ssl_id, 0x40000, 1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm2-init", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm2-init", "rc", (uint64_t)(uint32_t)h2_id, "code");

    if (h2_id >= 0) {
        int h2_second = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            h2_second = fn_http2_init(pool_id, ssl_id, 0x40000, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-init", "fault-second", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm2-init", "rc-second", (uint64_t)(uint32_t)h2_second,
                           "code");
        if (h2_second >= 0 && h2_second != h2_id) {
            (void)fn_http2_term(h2_second);
        }
    }

    /* (3) sceHttp2Init with pool id 0, and again with ssl id 0 */
    int rc_pool0 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_pool0 = fn_http2_init(0, ssl_id, 0x40000, 1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm3-zero-ids", "pool0-fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm3-zero-ids", "rc-pool0", (uint64_t)(uint32_t)rc_pool0,
                       "code");
    if (rc_pool0 >= 0) {
        (void)fn_http2_term(rc_pool0);
    }

    int rc_ssl0 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_ssl0 = fn_http2_init(pool_id, 0, 0x40000, 1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm3-zero-ids", "ssl0-fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm3-zero-ids", "rc-ssl0", (uint64_t)(uint32_t)rc_ssl0,
                       "code");
    if (rc_ssl0 >= 0) {
        (void)fn_http2_term(rc_ssl0);
    }

    /* (4) sceHttp2Term on (2)'s id: rc, and rc of second sceHttp2Term on it */
    int rc_term1 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_term1 = fn_http2_term(h2_id);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm4-term", "fault-1", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm4-term", "rc", (uint64_t)(uint32_t)rc_term1, "code");

    int rc_term2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_term2 = fn_http2_term(h2_id);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm4-term", "fault-2", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "arm4-term", "rc-second", (uint64_t)(uint32_t)rc_term2,
                       "code");

    /* Cleanup SSL */
    if (ssl_id >= 0 && fn_ssl_term &&
        obs_address_is_callable((const void *)fn_ssl_term)) {
        (void)fn_ssl_term(ssl_id);
    }

    return obs_pass();
}

static obs_result check_savedata_memory_5d20(void) {
    const char *check_name = "130-layout/savedata-memory-5d20";

    /* Sysmodule load for save data */
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load =
            (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    int load_rc = -1;
    if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
        load_rc = fn_load(0x00B6); /* OOPS_SYSMODULE_SAVE_DATA */
    } else if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        load_rc = sceSysmoduleLoadModule(0x00B6);
    }
    obs_report_measure(check_name, "sceSysmoduleLoadModule", "rc",
                       (uint64_t)(uint32_t)load_rc, "code");

    /* Static resolution first */
    int32_t (*fn_init)(void *) = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataInitialize3_static)) {
        fn_init = (int32_t (*)(void *))&sceSaveDataInitialize3_static;
    } else if (obs_address_is_callable((const void *)&sceSaveDataInitialize3_named)) {
        fn_init = (int32_t (*)(void *))&sceSaveDataInitialize3_named;
    } else {
        fn_init = (int32_t (*)(void *))layout_resolve_sym("libSceSaveData_native",
                                                          "$TywrFKCoLGY");
        if (fn_init == NULL) {
            fn_init = (int32_t (*)(void *))layout_resolve_sym("libSceSaveData_native",
                                                              "sceSaveDataInitialize3");
        }
        if (fn_init == NULL) {
            fn_init = (int32_t (*)(void *))layout_resolve_sym("libSceSaveData",
                                                              "sceSaveDataInitialize3");
        }
    }

    int32_t (*fn_term)(void) = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataTerminate_static)) {
        fn_term = (int32_t (*)(void))&sceSaveDataTerminate_static;
    } else if (obs_address_is_callable((const void *)&sceSaveDataTerminate_named)) {
        fn_term = (int32_t (*)(void))&sceSaveDataTerminate_named;
    } else {
        fn_term = (int32_t (*)(void))layout_resolve_sym("libSceSaveData_native",
                                                        "$yKDy8S5yLA0");
        if (fn_term == NULL) {
            fn_term = (int32_t (*)(void))layout_resolve_sym("libSceSaveData_native",
                                                            "sceSaveDataTerminate");
        }
        if (fn_term == NULL) {
            fn_term = (int32_t (*)(void))layout_resolve_sym("libSceSaveData",
                                                            "sceSaveDataTerminate");
        }
    }

    /* sceSaveDataSetupSaveDataMemory2: $oQySEUfgXRA (0xa10c921147e05d10) */
    int32_t (*fn_setup2)(const void *, void *) = NULL;
    if (obs_address_is_callable(
            (const void *)&sceSaveDataSetupSaveDataMemory2_static)) {
        fn_setup2 =
            (int32_t (*)(const void *, void *))&sceSaveDataSetupSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSetupSaveDataMemory2_named)) {
        fn_setup2 =
            (int32_t (*)(const void *, void *))&sceSaveDataSetupSaveDataMemory2_named;
    } else {
        fn_setup2 = (int32_t (*)(const void *, void *))layout_resolve_sym(
            "libSceSaveData_native", "$oQySEUfgXRA");
        if (fn_setup2 == NULL) {
            fn_setup2 = (int32_t (*)(const void *, void *))layout_resolve_sym(
                "libSceSaveData_native", "sceSaveDataSetupSaveDataMemory2");
        }
        if (fn_setup2 == NULL) {
            fn_setup2 = (int32_t (*)(const void *, void *))layout_resolve_sym(
                "libSceSaveData", "sceSaveDataSetupSaveDataMemory2");
        }
    }

    /* sceSaveDataSetSaveDataMemory2: $cduy9v4YmT4 (0x71dbb2f6fe18993e) */
    int32_t (*fn_set2)(const void *) = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataSetSaveDataMemory2_static)) {
        fn_set2 = (int32_t (*)(const void *))&sceSaveDataSetSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSetSaveDataMemory2_named)) {
        fn_set2 = (int32_t (*)(const void *))&sceSaveDataSetSaveDataMemory2_named;
    } else {
        fn_set2 = (int32_t (*)(const void *))layout_resolve_sym("libSceSaveData_native",
                                                                "$cduy9v4YmT4");
        if (fn_set2 == NULL) {
            fn_set2 = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData_native", "sceSaveDataSetSaveDataMemory2");
        }
        if (fn_set2 == NULL) {
            fn_set2 = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData", "sceSaveDataSetSaveDataMemory2");
        }
    }

    /* sceSaveDataGetSaveDataMemory2: $QwOO7vegnV8 (0x43038eeef7a09d5f) */
    int32_t (*fn_get2)(const void *) = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataGetSaveDataMemory2_static)) {
        fn_get2 = (int32_t (*)(const void *))&sceSaveDataGetSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataGetSaveDataMemory2_named)) {
        fn_get2 = (int32_t (*)(const void *))&sceSaveDataGetSaveDataMemory2_named;
    } else {
        fn_get2 = (int32_t (*)(const void *))layout_resolve_sym("libSceSaveData_native",
                                                                "$QwOO7vegnV8");
        if (fn_get2 == NULL) {
            fn_get2 = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData_native", "sceSaveDataGetSaveDataMemory2");
        }
        if (fn_get2 == NULL) {
            fn_get2 = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData", "sceSaveDataGetSaveDataMemory2");
        }
    }

    /* sceSaveDataSyncSaveDataMemory: $wiT9jeC7xPw (0xc224fd8de0bbc4fc) */
    int32_t (*fn_sync)(const void *) = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataSyncSaveDataMemory_static)) {
        fn_sync = (int32_t (*)(const void *))&sceSaveDataSyncSaveDataMemory_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSyncSaveDataMemory_named)) {
        fn_sync = (int32_t (*)(const void *))&sceSaveDataSyncSaveDataMemory_named;
    } else {
        fn_sync = (int32_t (*)(const void *))layout_resolve_sym("libSceSaveData_native",
                                                                "$wiT9jeC7xPw");
        if (fn_sync == NULL) {
            fn_sync = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData_native", "sceSaveDataSyncSaveDataMemory");
        }
        if (fn_sync == NULL) {
            fn_sync = (int32_t (*)(const void *))layout_resolve_sym(
                "libSceSaveData", "sceSaveDataSyncSaveDataMemory");
        }
    }

    obs_report_measure(check_name, "resolve", "init3-slot",
                       (uint64_t)(uintptr_t)fn_init, "address");
    obs_report_measure(
        check_name, "resolve", "init3-resolved",
        (uint64_t)(fn_init != NULL && obs_address_is_callable((const void *)fn_init)),
        "bool");
    obs_report_measure(check_name, "resolve", "term-slot", (uint64_t)(uintptr_t)fn_term,
                       "address");
    obs_report_measure(
        check_name, "resolve", "term-resolved",
        (uint64_t)(fn_term != NULL && obs_address_is_callable((const void *)fn_term)),
        "bool");
    obs_report_measure(check_name, "resolve", "setup2-slot",
                       (uint64_t)(uintptr_t)fn_setup2, "address");
    obs_report_measure(check_name, "resolve", "setup2-resolved",
                       (uint64_t)(fn_setup2 != NULL &&
                                  obs_address_is_callable((const void *)fn_setup2)),
                       "bool");
    obs_report_measure(check_name, "resolve", "set2-slot", (uint64_t)(uintptr_t)fn_set2,
                       "address");
    obs_report_measure(
        check_name, "resolve", "set2-resolved",
        (uint64_t)(fn_set2 != NULL && obs_address_is_callable((const void *)fn_set2)),
        "bool");
    obs_report_measure(check_name, "resolve", "get2-slot", (uint64_t)(uintptr_t)fn_get2,
                       "address");
    obs_report_measure(
        check_name, "resolve", "get2-resolved",
        (uint64_t)(fn_get2 != NULL && obs_address_is_callable((const void *)fn_get2)),
        "bool");
    obs_report_measure(check_name, "resolve", "sync-slot", (uint64_t)(uintptr_t)fn_sync,
                       "address");
    obs_report_measure(
        check_name, "resolve", "sync-resolved",
        (uint64_t)(fn_sync != NULL && obs_address_is_callable((const void *)fn_sync)),
        "bool");

    if (fn_setup2 == NULL || !obs_address_is_callable((const void *)fn_setup2)) {
        return obs_skip("sceSaveDataSetupSaveDataMemory2 not resolved/callable");
    }

    obs_jmp_buf guard;

    /* Initialize save data with NULL */
    int32_t init_rc = -1;
    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            init_rc = fn_init(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "init3", "rc", (uint64_t)(uint32_t)init_rc, "code");

    /* Determine initial user */
    int32_t user_id = -1;
    int (*fn_user_init)(const void *) = (int (*)(const void *))layout_resolve_sym(
        "libSceUserService", "sceUserServiceInitialize");
    int32_t (*fn_user_get)(int32_t *) = (int32_t (*)(int32_t *))layout_resolve_sym(
        "libSceUserService", "sceUserServiceGetInitialUser");
    if (fn_user_init && obs_address_is_callable((const void *)fn_user_init)) {
        (void)fn_user_init(NULL);
    }
    if (fn_user_get && obs_address_is_callable((const void *)fn_user_get)) {
        (void)fn_user_get(&user_id);
    }
    if (user_id < 0) {
        user_id = 0x10000000;
    }
    obs_report_measure(check_name, "context", "user-id", (uint64_t)(uint32_t)user_id,
                       "id");

    /* Prepare PPSA21564's parameter block:
     * u32 0, u32 initial user, u64 0x200000 (2 MB), then zeros to 0x40 bytes */
    uint8_t param[64];
    memset(param, 0, sizeof(param));
    *(uint32_t *)(void *)(param + 0) = 0;
    *(uint32_t *)(void *)(param + 4) = (uint32_t)user_id;
    *(uint64_t *)(void *)(param + 8) = 0x200000ULL;

    uint8_t param_before[64];
    memcpy(param_before, param, 64);

    /* (1) Arm 1: sceSaveDataSetupSaveDataMemory2(param, NULL) */
    obs_report_bytes(check_name, "setup2-null-result", "param-before", 0, param_before,
                     64);
    int32_t rc_arm1 = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_arm1 = fn_setup2(param, NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "setup2-null-result", "rc",
                       (uint64_t)(uint32_t)rc_arm1, "code");
    obs_report_bytes(check_name, "setup2-null-result", "param-after", 0, param, 64);
    obs_report_measure(check_name, "setup2-null-result", "changed-bytes",
                       (uint64_t)(memcmp(param_before, param, 64) != 0), "bool");

    /* (1) Arm 2: Second call after the first without terminating */
    uint8_t res_block2[64];
    memset(res_block2, 0xCC, sizeof(res_block2));
    int32_t rc_arm2 = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_arm2 = fn_setup2(param, res_block2);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "setup2-second-call", "rc",
                       (uint64_t)(uint32_t)rc_arm2, "code");
    obs_report_bytes(check_name, "setup2-second-call", "res-after", 0, res_block2, 64);

    /* Re-initialize cleanly to test fresh call with result pointer */
    if (fn_term && obs_address_is_callable((const void *)fn_term)) {
        (void)fn_term();
    }
    if (fn_init && obs_address_is_callable((const void *)fn_init)) {
        (void)fn_init(NULL);
    }

    /* (1) Arm 3: sceSaveDataSetupSaveDataMemory2(param, res) with fresh 0x40-byte 0xcc
     * block */
    memcpy(param, param_before, 64);
    uint8_t res_block[64];
    memset(res_block, 0xCC, sizeof(res_block));
    obs_report_bytes(check_name, "setup2-poison-result", "res-before", 0, res_block,
                     64);
    int32_t rc_arm3 = -999;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_arm3 = fn_setup2(param, res_block);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(check_name, "setup2-poison-result", "rc",
                       (uint64_t)(uint32_t)rc_arm3, "code");
    obs_report_bytes(check_name, "setup2-poison-result", "res-after", 0, res_block, 64);
    size_t written = 0;
    for (size_t b = 0; b < 64; b++) {
        if (res_block[b] != 0xCC) {
            written++;
        }
    }
    obs_report_measure(check_name, "setup2-poison-result", "bytes-written",
                       (uint64_t)written, "bytes");

    /* (2) Save-memory set, get, sync via authentic SDK struct parameters */
    typedef struct {
        void *buf;
        size_t buf_size;
        int64_t offset;
        uint8_t reserved[40];
    } obs_sd_mem_data;

    typedef struct {
        int32_t user_id;
        uint8_t padding[4];
        const obs_sd_mem_data *data;
        const void *param;
        const void *icon;
        uint32_t data_num;
        uint32_t slot_id;
        uint8_t reserved[24];
    } obs_sd_mem_set2;

    typedef struct {
        int32_t user_id;
        uint8_t padding[4];
        obs_sd_mem_data *data;
        void *param;
        void *icon;
        uint32_t slot_id;
        uint8_t reserved[28];
    } obs_sd_mem_get2;

    typedef struct {
        int32_t user_id;
        uint32_t slot_id;
        uint32_t option;
        uint8_t reserved[28];
    } obs_sd_mem_sync;

    uint8_t write_16[16] = {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                            0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51};
    obs_report_bytes(check_name, "save-mem-set", "write-16", 0, write_16, 16);

    obs_sd_mem_data set_data;
    memset(&set_data, 0, sizeof(set_data));
    set_data.buf = write_16;
    set_data.buf_size = sizeof(write_16);
    set_data.offset = 0;

    obs_sd_mem_set2 set_param;
    memset(&set_param, 0, sizeof(set_param));
    set_param.user_id = user_id;
    set_param.data = &set_data;
    set_param.data_num = 1;
    set_param.slot_id = 0;

    int32_t rc_set = -999;
    if (fn_set2 && obs_address_is_callable((const void *)fn_set2)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            rc_set = fn_set2(&set_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "save-mem-set", "rc", (uint64_t)(uint32_t)rc_set,
                       "code");

    uint8_t read_16[16];
    memset(read_16, 0xAA, sizeof(read_16));

    obs_sd_mem_data get_data;
    memset(&get_data, 0, sizeof(get_data));
    get_data.buf = read_16;
    get_data.buf_size = sizeof(read_16);
    get_data.offset = 0;

    obs_sd_mem_get2 get_param;
    memset(&get_param, 0, sizeof(get_param));
    get_param.user_id = user_id;
    get_param.data = &get_data;
    get_param.slot_id = 0;

    int32_t rc_get = -999;
    if (fn_get2 && obs_address_is_callable((const void *)fn_get2)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            rc_get = fn_get2(&get_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "save-mem-get", "rc", (uint64_t)(uint32_t)rc_get,
                       "code");
    obs_report_bytes(check_name, "save-mem-get", "read-16", 0, read_16, 16);
    obs_report_measure(check_name, "save-mem-get", "matches-written",
                       (uint64_t)(memcmp(write_16, read_16, 16) == 0), "bool");

    obs_sd_mem_sync sync_param;
    memset(&sync_param, 0, sizeof(sync_param));
    sync_param.user_id = user_id;
    sync_param.slot_id = 0;
    sync_param.option = 0;

    int32_t rc_sync = -999;
    if (fn_sync && obs_address_is_callable((const void *)fn_sync)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            rc_sync = fn_sync(&sync_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "save-mem-sync", "rc", (uint64_t)(uint32_t)rc_sync,
                       "code");

    /* Terminate save data */
    int32_t rc_term = -999;
    if (fn_term && obs_address_is_callable((const void *)fn_term)) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            rc_term = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(check_name, "terminate", "rc", (uint64_t)(uint32_t)rc_term,
                       "code");

    return obs_pass();
}

struct obs_statfs_layout {
    uint32_t f_version;
    uint32_t f_type;
    uint64_t f_flags;
    uint64_t f_bsize;
    uint64_t f_iosize;
    uint64_t f_blocks;
    uint64_t f_bfree;
    int64_t f_bavail;
    uint64_t f_files;
    int64_t f_ffree;
    uint64_t f_syncwrites;
    uint64_t f_asyncwrites;
    uint64_t f_syncreads;
    uint64_t f_asyncreads;
    uint64_t f_spare[10];
    uint32_t f_namemax;
    uint32_t f_owner;
    uint32_t f_fsid[2];
    char f_charspare[80];
    char f_fstypename[16];
    char f_mntfromname[88];
    char f_mntonname[88];
};

typedef int *(*obs_fn_error_t)(void);

static inline int *obs_get_errno_location(void) {
    int k_mod = obs_module_open("libkernel");
    obs_fn_error_t fn_err = NULL;
    if (k_mod >= 0) {
        fn_err = (obs_fn_error_t)(uintptr_t)obs_module_symbol(k_mod, "__error");
    }
    if (!fn_err && obs_address_is_callable((const void *)&__error)) {
        fn_err = &__error;
    }
    if (fn_err && obs_address_is_callable((const void *)fn_err)) {
        return fn_err();
    }
    return NULL;
}

static int obs_probe_statfs(const char *path, void *buf, int *out_errno) {
    int *err_ptr = obs_get_errno_location();
    if (err_ptr) {
        *err_ptr = 0;
    }

    *(uint32_t *)buf = 0x20140518u; /* STATFS_VERSION */

    int k_mod = obs_module_open("libkernel");
    int (*fn_statfs)(const char *, void *) = NULL;
    if (k_mod >= 0) {
        fn_statfs = (int (*)(const char *, void *))obs_module_symbol(k_mod, "_statfs");
        if (!fn_statfs) {
            fn_statfs =
                (int (*)(const char *, void *))obs_module_symbol(k_mod, "statfs");
        }
        if (!fn_statfs) {
            fn_statfs = (int (*)(const char *, void *))obs_module_symbol(
                k_mod, "$9e0667416c94bc62");
        }
        if (!fn_statfs) {
            fn_statfs =
                (int (*)(const char *, void *))obs_module_symbol(k_mod, "__sys_statfs");
        }
    }
    if (!fn_statfs && obs_address_is_callable((const void *)&statfs)) {
        fn_statfs = &statfs;
    }
    if (fn_statfs && obs_address_is_callable((const void *)fn_statfs)) {
        int rc = fn_statfs(path, buf);
        if (rc == 0) {
            if (out_errno)
                *out_errno = 0;
            return 0;
        }
        err_ptr = obs_get_errno_location();
        int err = err_ptr ? *err_ptr : 0;
        if (out_errno)
            *out_errno = err;
        return rc;
    }

    /* Fallback to direct syscalls via libkernel gadget */
    long sc =
        obs_invoke_syscall(555 /* SYS_nstatfs */, (long)path, (long)buf, 0, 0, 0, 0);
    if (sc == 0) {
        if (out_errno)
            *out_errno = 0;
        return 0;
    }
    err_ptr = obs_get_errno_location();
    int err = err_ptr ? *err_ptr : 0;
    if (err == 0 && sc > 0 && sc < 256) {
        err = (int)sc;
    }

    long sc396 =
        obs_invoke_syscall(396 /* SYS_statfs */, (long)path, (long)buf, 0, 0, 0, 0);
    if (sc396 == 0) {
        if (out_errno)
            *out_errno = 0;
        return 0;
    }
    if (err == 0) {
        err_ptr = obs_get_errno_location();
        err = err_ptr ? *err_ptr : 0;
        if (err == 0 && sc396 > 0 && sc396 < 256) {
            err = (int)sc396;
        }
    }
    if (out_errno)
        *out_errno = err;
    return (int)sc;
}

static int obs_probe_fstatfs(int fd, void *buf, int *out_errno) {
    int *err_ptr = obs_get_errno_location();
    if (err_ptr) {
        *err_ptr = 0;
    }

    *(uint32_t *)buf = 0x20140518u; /* STATFS_VERSION */

    int k_mod = obs_module_open("libkernel");
    int (*fn_fstatfs)(int, void *) = NULL;
    if (k_mod >= 0) {
        fn_fstatfs = (int (*)(int, void *))obs_module_symbol(k_mod, "_fstatfs");
        if (!fn_fstatfs) {
            fn_fstatfs = (int (*)(int, void *))obs_module_symbol(k_mod, "fstatfs");
        }
        if (!fn_fstatfs) {
            fn_fstatfs =
                (int (*)(int, void *))obs_module_symbol(k_mod, "$d7b31f7b50775fa5");
        }
        if (!fn_fstatfs) {
            fn_fstatfs =
                (int (*)(int, void *))obs_module_symbol(k_mod, "__sys_fstatfs");
        }
    }
    if (fn_fstatfs && obs_address_is_callable((const void *)fn_fstatfs)) {
        int rc = fn_fstatfs(fd, buf);
        if (rc == 0) {
            if (out_errno)
                *out_errno = 0;
            return 0;
        }
        err_ptr = obs_get_errno_location();
        int err = err_ptr ? *err_ptr : 0;
        if (out_errno)
            *out_errno = err;
        return rc;
    }

    /* Fallback to direct syscalls via libkernel gadget */
    long sc =
        obs_invoke_syscall(556 /* SYS_nfstatfs */, (long)fd, (long)buf, 0, 0, 0, 0);
    if (sc == 0) {
        if (out_errno)
            *out_errno = 0;
        return 0;
    }
    err_ptr = obs_get_errno_location();
    int err = err_ptr ? *err_ptr : 0;
    if (err == 0 && sc > 0 && sc < 256) {
        err = (int)sc;
    }

    long sc397 =
        obs_invoke_syscall(397 /* SYS_fstatfs */, (long)fd, (long)buf, 0, 0, 0, 0);
    if (sc397 == 0) {
        if (out_errno)
            *out_errno = 0;
        return 0;
    }
    if (err == 0) {
        err_ptr = obs_get_errno_location();
        err = err_ptr ? *err_ptr : 0;
        if (err == 0 && sc397 > 0 && sc397 < 256) {
            err = (int)sc397;
        }
    }
    if (out_errno)
        *out_errno = err;
    return (int)sc;
}

static obs_result check_app0_statfs(void) {
    const char *check_name = "130-layout/app0-statfs";

    /* Report the struct layout the probe passes (FreeBSD 12 472-byte statfs) */
    obs_report_measure(check_name, "struct-statfs", "size",
                       (uint64_t)sizeof(struct obs_statfs_layout), "bytes");
    obs_report_measure(check_name, "struct-statfs", "f_flags-offset",
                       (uint64_t)__builtin_offsetof(struct obs_statfs_layout, f_flags),
                       "offset");
    obs_report_measure(
        check_name, "struct-statfs", "f_fstypename-offset",
        (uint64_t)__builtin_offsetof(struct obs_statfs_layout, f_fstypename), "offset");
    obs_report_measure(
        check_name, "struct-statfs", "f_mntfromname-offset",
        (uint64_t)__builtin_offsetof(struct obs_statfs_layout, f_mntfromname),
        "offset");
    obs_report_measure(
        check_name, "struct-statfs", "f_mntonname-offset",
        (uint64_t)__builtin_offsetof(struct obs_statfs_layout, f_mntonname), "offset");

    static const char *const paths_to_stat[] = {
        "/",
        "/user/app/PPSA90000",
        "/user/data/PPSA90000",
        "/app0",
        "/user/app",
        "/user/data",
        "/user",
        "/system",
        "/system_ex",
        "/download0",
        "/user/app/ORB90000",
        "/user/app/ORBO00001",
        "/user/app/PPSA21564",
        "/user/app/PPSA25872",
        "/user/app/PPSA04263",
        "/user/app/PPSA02664",
        "/user/app/PPSA28061",
    };

    uint8_t stat_buf[1024];
    int app0_is_pfs = 0;

    for (size_t i = 0; i < sizeof(paths_to_stat) / sizeof(paths_to_stat[0]); i++) {
        const char *path = paths_to_stat[i];
        memset(stat_buf, 0, sizeof(stat_buf));

        int err = 0;
        int rc = obs_probe_statfs(path, stat_buf, &err);
        obs_report_measure(check_name, path, "statfs-rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_measure(check_name, path, "statfs-errno", (uint64_t)(uint32_t)err,
                           "errno");

        if (rc == 0) {
            uint64_t flags = *(const uint64_t *)(const void *)(stat_buf + 8);
            obs_report_measure(check_name, path, "flags", flags, "flags");
            int is_pfs = (memcmp(stat_buf + 280, "pfs", 3) == 0);
            obs_report_measure(check_name, path, "is-pfs", (uint64_t)is_pfs, "bool");
            obs_report_bytes(check_name, path, "f_fstypename", 0,
                             (const uint8_t *)(stat_buf + 280), 16);
            obs_report_bytes(check_name, path, "f_mntfromname", 0,
                             (const uint8_t *)(stat_buf + 296), 88);
            obs_report_bytes(check_name, path, "f_mntonname", 0,
                             (const uint8_t *)(stat_buf + 384), 88);
            obs_report_bytes(check_name, path, "raw-statfs-512b", 0,
                             (const uint8_t *)stat_buf, 512);
            if (obs_strcmp(path, "/app0") == 0 && is_pfs) {
                app0_is_pfs = 1;
            }
        }
    }

    /* Fallback: fstatfs on opened file descriptors */
    static const char *const targets_to_fstatfs[] = {
        "/user/app/PPSA90000/eboot.bin",
        "/user/data/PPSA90000/eboot.bin",
        "/system/common/lib/libkernel.sprx",
        "/user/app/PPSA90000",
        "/app0/eboot.bin",
        "/app0",
        "/",
        "/system",
    };

    int k_mod = obs_module_open("libkernel");
    int (*fn_open)(const char *, int, ...) = NULL;
    int (*fn_close)(int) = NULL;
    if (k_mod >= 0) {
        fn_open =
            (int (*)(const char *, int, ...))obs_module_symbol(k_mod, "sceKernelOpen");
        fn_close = (int (*)(int))obs_module_symbol(k_mod, "sceKernelClose");
    }

    for (size_t i = 0; i < sizeof(targets_to_fstatfs) / sizeof(targets_to_fstatfs[0]);
         i++) {
        const char *target = targets_to_fstatfs[i];
        int *err_ptr = obs_get_errno_location();
        if (err_ptr)
            *err_ptr = 0;
        int fd = -1;
        int open_err = 0;
        if (fn_open && obs_address_is_callable((const void *)fn_open)) {
            fd = fn_open(target, 0 /* O_RDONLY */, 0);
            if (fd < 0) {
                err_ptr = obs_get_errno_location();
                open_err = err_ptr ? *err_ptr : 0;
            }
        } else if (obs_address_is_callable((const void *)&sceKernelOpen)) {
            fd = sceKernelOpen(target, 0, 0);
            if (fd < 0) {
                err_ptr = obs_get_errno_location();
                open_err = err_ptr ? *err_ptr : 0;
            }
        } else {
            fd = (int)obs_invoke_syscall(5 /* SYS_open */, (long)target, 0, 0, 0, 0, 0);
            if (fd < 0) {
                err_ptr = obs_get_errno_location();
                open_err = err_ptr ? *err_ptr : 0;
            }
        }

        obs_report_measure(check_name, target, "open-rc", (uint64_t)(uint32_t)fd,
                           "code");
        if (fd < 0) {
            obs_report_measure(check_name, target, "open-errno",
                               (uint64_t)(uint32_t)open_err, "errno");
        } else {
            memset(stat_buf, 0, sizeof(stat_buf));
            int f_err = 0;
            int f_rc = obs_probe_fstatfs(fd, stat_buf, &f_err);
            obs_report_measure(check_name, target, "fstatfs-rc",
                               (uint64_t)(uint32_t)f_rc, "code");
            obs_report_measure(check_name, target, "fstatfs-errno",
                               (uint64_t)(uint32_t)f_err, "errno");

            if (f_rc == 0) {
                uint64_t f_flags = *(const uint64_t *)(const void *)(stat_buf + 8);
                obs_report_measure(check_name, target, "f_flags", f_flags, "flags");
                int is_pfs = (memcmp(stat_buf + 280, "pfs", 3) == 0);
                obs_report_measure(check_name, target, "is-pfs", (uint64_t)is_pfs,
                                   "bool");
                obs_report_bytes(check_name, target, "f_fstypename", 0,
                                 (const uint8_t *)(stat_buf + 280), 16);
                obs_report_bytes(check_name, target, "f_mntfromname", 0,
                                 (const uint8_t *)(stat_buf + 296), 88);
                obs_report_bytes(check_name, target, "f_mntonname", 0,
                                 (const uint8_t *)(stat_buf + 384), 88);
                if ((obs_strcmp(target, "/app0") == 0 ||
                     obs_strcmp(target, "/app0/eboot.bin") == 0) &&
                    is_pfs) {
                    app0_is_pfs = 1;
                }
            }

            if (fn_close && obs_address_is_callable((const void *)fn_close)) {
                fn_close(fd);
            } else if (obs_address_is_callable((const void *)&sceKernelClose)) {
                sceKernelClose(fd);
            } else {
                obs_invoke_syscall(6 /* SYS_close */, (long)fd, 0, 0, 0, 0, 0);
            }
        }
    }

    /* (4) -6e31's original APR resolve if /app0 was shown as PFS */
    if (app0_is_pfs) {
        const void *fn_apr_resolve = obs_module_symbol(
            k_mod >= 0 ? k_mod : 1, "sceKernelAprResolveFilepathsToIdsAndFileSizes");
        if (!fn_apr_resolve && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *a = NULL;
            if (sceKernelDlsym(k_mod >= 0 ? k_mod : 1,
                               "sceKernelAprResolveFilepathsToIdsAndFileSizes",
                               &a) == 0) {
                fn_apr_resolve = a;
            }
        }
        if (fn_apr_resolve && obs_address_is_callable(fn_apr_resolve)) {
            typedef int (*fn_apr_resolve_t)(const char **, uint32_t, void *,
                                            uint32_t *);
            fn_apr_resolve_t call_resolve = (fn_apr_resolve_t)fn_apr_resolve;
            const char *apr_paths[1] = {"/app0/eboot.bin"};
            uint8_t out_ids[64];
            uint32_t out_count = 0;
            memset(out_ids, 0, sizeof(out_ids));
            int apr_rc = call_resolve(apr_paths, 1, out_ids, &out_count);
            obs_report_measure(check_name, "apr-resolve", "rc",
                               (uint64_t)(uint32_t)apr_rc, "code");
            obs_report_measure(check_name, "apr-resolve", "out-count",
                               (uint64_t)out_count, "count");
            if (apr_rc == 0) {
                obs_report_bytes(check_name, "apr-resolve", "ids", 0, out_ids, 16);
            }
        }
    }

    return obs_pass();
}

static const obs_check layout_checks[] = {
    /* Assumed, uniformly, and the field is being used honestly: nothing here has an
     * expectation at all, and `assumed` is the closest the vocabulary comes to saying
     * "this is a measurement". The provenance of a hexdump is the hardware it came
     * from. */
    {"130-layout/direct-memory-query", "libkernel", "sceKernelDirectMemoryQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery,
     check_direct_memory_query, OBS_FROM_ASSUMED},
    /* Assumed, like the rest of the section, and for the reason stated above it: this
     * records bytes rather than asserting a value, and `assumed` is the closest the
     * vocabulary comes to "this is a measurement". */
    {"130-layout/direct-memory-query-flags", "libkernel", "sceKernelDirectMemoryQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery,
     check_direct_memory_query_flags, OBS_FROM_ASSUMED},
    {"130-layout/memory-type", "libkernel", "sceKernelAllocateDirectMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelAllocateDirectMemory,
     check_memory_type_field, OBS_FROM_ASSUMED},
    {"130-layout/system-software-version", "libkernel", "sceKernelGetSystemSwVersion",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelGetSystemSwVersion,
     check_system_software_version, OBS_FROM_ASSUMED},
    {"130-layout/resolution-status", "libSceVideoOut", "sceVideoOutGetResolutionStatus",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoOutGetResolutionStatus,
     check_resolution_status, OBS_FROM_ASSUMED},
    {"130-layout/video-out-unregister-buffers", "libSceVideoOut",
     "sceVideoOutUnregisterBuffers", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceVideoOutUnregisterBuffers, check_video_out_unregister_buffers,
     OBS_FROM_ASSUMED},
    /* The one check in this section whose provenance is not `assumed`: the platform
     * draws the boundary, this only reports where it fell. */
    {"130-layout/query-size-ladder", "libkernel", "sceKernelDirectMemoryQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery,
     check_query_size_ladder, OBS_FROM_DERIVED},
    /* Derived for the same reason as the ladder: the platform decides whether the size
     * bounds the write, this only records which way it fell. */
    {"130-layout/short-buffer-overrun", "libkernel", "sceKernelDirectMemoryQuery",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDirectMemoryQuery,
     check_query_short_buffer_overrun, OBS_FROM_DERIVED},
    {"130-layout/user-service-layout", "libSceUserService",
     "sceUserServiceGetLoginUserIdList", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_user_service_layout, check_user_service_layout,
     OBS_FROM_ASSUMED},
    {"130-layout/user-game-presets", "libSceUserService", "$-sD02mFDBh4", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_user_game_presets, OBS_FROM_ASSUMED},
    {"130-layout/pad-controller-layout", "libScePad", "scePadGetControllerInformation",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_pad_controller_layout,
     check_pad_controller_layout, OBS_FROM_ASSUMED},
    {"130-layout/network-interfaces", "libSceNet", "getifaddrs", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_network_interface_layout,
     check_network_interface_layout, OBS_FROM_ASSUMED},
    {"130-layout/app-content-layout", "libSceAppContent", "sceAppContentInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_app_content_layout,
     check_app_content_layout, OBS_FROM_ASSUMED},
    {"130-layout/common-dialog-layout", "libSceCommonDialog",
     "sceCommonDialogInitialize", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_common_dialog_layout, check_common_dialog_layout,
     OBS_FROM_ASSUMED},
    {"130-layout/savedata-layout", "libSceSaveData_native", "$TywrFKCoLGY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_savedata_layout,
     OBS_FROM_ASSUMED},
    {"130-layout/savedata-memory-5d20", "libSceSaveData_native", "$oQySEUfgXRA",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_savedata_memory_5d20,
     OBS_FROM_ASSUMED},
    {"130-layout/app0-statfs", "libkernel", "statfs", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_app0_statfs, check_app0_statfs, OBS_FROM_ASSUMED},
    {"130-layout/savedata-event-result", "libSceSaveData", "sceSaveDataGetEventResult",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_savedata_event_result,
     check_savedata_event_result, OBS_FROM_ASSUMED},
    {"130-layout/savedata-mount-capacity", "libSceSaveData", "sceSaveDataMount2",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_savedata_mount_capacity,
     check_savedata_mount_capacity, OBS_FROM_ASSUMED},
    {"130-layout/json2-init-layout", "libSceJson2", "$Cxwy7wHq4J0", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_json2_init_layout, OBS_FROM_ASSUMED},
    {"130-layout/video-out-output-status", "libSceVideoOut",
     "sceVideoOutGetOutputStatus", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_video_out_output_status, check_video_out_output_status,
     OBS_FROM_ASSUMED},
    {"130-layout/video-out-vblank-status", "libSceVideoOut",
     "sceVideoOutGetVblankStatus", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_video_out_vblank_status, check_video_out_vblank_status,
     OBS_FROM_ASSUMED},
    {"130-layout/unwind-module-info", "libkernel", "sceKernelGetModuleInfoForUnwind",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_module_info_for_unwind,
     check_module_info_for_unwind, OBS_FROM_ASSUMED},
    {"130-layout/query-memory-protection", "libkernel",
     "sceKernelQueryMemoryProtection", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_query_memory_protection, check_query_memory_protection,
     OBS_FROM_ASSUMED},
    {"130-layout/font-render-glyph", "libSceFont", "sceFontRenderCharGlyphImage",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_font_render_glyph,
     check_font_render_glyph, OBS_FROM_ASSUMED},
    {"130-layout/font-memory-init", "libSceFont", "sceFontMemoryInit", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_font_memory_init, check_font_memory_init,
     OBS_FROM_ASSUMED},
    {"130-layout/font-create-library-ft", "libSceFontFt", "sceFontSelectLibraryFt",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_font_create_library_ft,
     check_font_create_library_ft, OBS_FROM_ASSUMED},
    {"130-layout/http2-init", "libSceHttp2", "sceHttp2Init", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_http2_init, check_http2_init, OBS_FROM_ASSUMED},
};

const obs_section obs_section_layout = {
    "130-layout",
    "Bytes, not verdicts",
    "Hands a call an oversized buffer and prints what it wrote, without naming a "
    "single "
    "field. The instrument half of this program: on a console these records answer "
    "questions that no amount of testing can.",
    layout_checks,
    OBS_COUNT(layout_checks),
};
