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
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

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
    /* No OBS_REQUIRE, and there used to be one for `sceKernelGetDirectMemorySize`.
     *
     * D058 requires guarding every symbol a check calls *other than its own*, because
     * every platform declaration is weak and jumping to zero ends the run. Neither of
     * the two query checks here calls that function - it appeared in nothing but the
     * guard itself.
     *
     * A guard on an uncalled symbol is the inverse mistake, and it is not harmless: it
     * makes the check skip on a platform that has `sceKernelDirectMemoryQuery` and
     * happens not to export the other one, and the skip reads as "the symbol is not
     * present" about a symbol that is present. The announced symbol is guarded by the
     * harness, and it is the only one either check touches. */

    /* The call `docs/HARDWARE-PROBE.md` names first: four experiments on the emulator
     * side went into establishing that it writes 24 bytes, and one hexdump from
     * hardware ends that class of work permanently.
     *
     * Queried from offset zero, which is where any direct memory a platform has must
     * begin. */
    layout_probe probe;
    layout_prepare(&probe);
    int rc = sceKernelDirectMemoryQuery(0, 0, probe.buffer, OBS_LAYOUT_BUFFER);
    return layout_report("130-layout/direct-memory-query", "sceKernelDirectMemoryQuery",
                         "info", &probe, rc);
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

    /* Needs a real output to ask about, so it opens one rather than guessing a handle -
     * a query against an invalid handle would report its refusal code and no bytes,
     * which is the least useful of the outcomes available. */
    int32_t user = 0;
    if (sceUserServiceGetInitialUser(&user) != 0) {
        return obs_skip("no initial user to open an output for");
    }
    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (handle <= 0) {
        /* Two different reasons an open is refused, and only one is a fact about the
         * platform. If this run's own display path already opened and holds the main
         * output, this second open is refused *because of that* - the same
         * handle-still-held trap display.c documents (its D169 note) - and the
         * resolution is simply not a thing this run can go on to ask. Saying "no video
         * output to query" for that case reads as "the console has no display", which
         * on a run whose header shows `display|ready|1920x1080` is a plain untruth. So
         * name which. */
        if (obs_display_holds_output()) {
            return obs_skip(
                "the display path already holds the main output, so a second open here "
                "is refused - this run cannot add the resolution, but a display is up");
        }
        return obs_skip("no video output to query");
    }
    layout_probe probe;
    layout_prepare(&probe);
    int rc = sceVideoOutGetResolutionStatus(handle, probe.buffer);
    obs_result result =
        layout_report("130-layout/resolution-status", "sceVideoOutGetResolutionStatus",
                      "status", &probe, rc);
    (void)sceVideoOutClose(handle);
    return result;
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
static obs_result check_user_service_layout(void) {
    int32_t (*fn_get_user_list)(int32_t *userIdList) =
        (int32_t (*)(int32_t *))obs_module_symbol(OBS_HANDLE_SELF,
                                                  "sceUserServiceGetLoginUserIdList");
    int32_t (*fn_get_initial_user)(int32_t *userId) =
        (int32_t (*)(int32_t *))obs_module_symbol(OBS_HANDLE_SELF,
                                                  "sceUserServiceGetInitialUser");
    int32_t (*fn_get_user_name)(int32_t userId, char *userName, size_t size) =
        (int32_t (*)(int32_t, char *, size_t))obs_module_symbol(
            OBS_HANDLE_SELF, "sceUserServiceGetUserName");

    if (fn_get_user_list == NULL && fn_get_initial_user == NULL) {
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
    {"130-layout/pad-controller-layout", "libScePad", "scePadGetControllerInformation",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_pad_controller_layout,
     check_pad_controller_layout, OBS_FROM_ASSUMED},
    {"130-layout/network-interfaces", "libSceNet", "getifaddrs", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_network_interface_layout,
     check_network_interface_layout, OBS_FROM_ASSUMED},
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
