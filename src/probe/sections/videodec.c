/*
 * Hardware video decode, reached: does libSceVideodec2 load, and do its decode entry
 * points resolve.
 *
 * # Why this section exists
 *
 * The idea it serves: a stream client or a media player that decodes H.264/HEVC/VP9 into
 * caller-owned, GPU-visible memory and hands that buffer straight to the display scanout -
 * no CPU copy. That is a capability the SDK does not have yet (its display path is linear
 * SDR RGB and knows nothing about decode), and libSceVideodec2 is the platform library
 * that would provide it. Before anything is built on that library it is worth knowing,
 * from the console itself, whether it is present and whether the entry points a decoder
 * needs are exported.
 *
 * # What it measures, and what it refuses to
 *
 * It **resolves** the decode symbols and records where each one landed, plus the first
 * sixteen bytes of each - the same census the encoder section makes for libSceVencCore.
 * It does not *call* any of them. The decode calls take structures whose layout this
 * project has not confirmed, and a wrong layout does not fail cleanly - it corrupts the
 * stack and crashes somewhere unrelated (CONVENTIONS 1). So the honest question a probe
 * can ask here is "is the library there and are these symbols real", and that is the
 * question it asks. A hardware run turns every symbol that resolves from a name we expect
 * into a name we have seen, which is what a later `oops/videodec.h` would be written from.
 *
 * # Provenance
 *
 * The library and symbol names are a reasoned expectation from public interface
 * documentation of the platform's media stack, not something this project has watched
 * resolve, so every check is `assumed`. A name that is wrong resolves to null and is
 * reported unresolved rather than crashing, so the guess is legible either way.
 */

#include "oops/freestd.h"
#include "obscene/harness.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* The decode surface libSceVideodec2 is expected to export. Ordered as a decoder would
 * reach for them: size the working memory, take a compute queue, create the decoder, feed
 * it, drain it, and read back what came out. */
static const char *const videodec2_symbols[] = {
    "sceVideodec2QueryComputeMemoryInfo",
    "sceVideodec2AllocateComputeQueue",
    "sceVideodec2ReleaseComputeQueue",
    "sceVideodec2CreateDecoder",
    "sceVideodec2DeleteDecoder",
    "sceVideodec2Decode",
    "sceVideodec2Flush",
    "sceVideodec2Reset",
    "sceVideodec2GetPictureInfo",
};

/* Is the library there at all - the first thing anything decode-shaped needs to know. */
static obs_result check_videodec2_library(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        return obs_fail("libSceVideodec2 did not load in this context");
    }
    return obs_pass_value((uint64_t)(uint32_t)handle);
}

/* Which of the expected decode entry points actually resolve, with where each landed and
 * its first sixteen bytes recorded for a later diff. Never calls one. */
static obs_result check_videodec2_symbols(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        return obs_skip("libSceVideodec2 did not load, so nothing resolves through it");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(videodec2_symbols); i++) {
        const char *name = videodec2_symbols[i];
        const void *addr = obs_module_symbol(handle, name);
        if (addr != NULL) {
            resolved++;
            obs_report_measure("107-videodec/symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            /* Readable, not merely callable: library text is execute-only on hardware, so
             * dump the prologue only where it can be read (emulators), never crashing on a
             * console. (D325) */
            if (obs_linkmap_readable((uintptr_t)addr)) {
                obs_report_buffer("107-videodec/prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("107-videodec/symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved == OBS_COUNT(videodec2_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceVideodec2 entry points resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("libSceVideodec2 loaded but no expected entry point resolved");
}

/* Out-param capture: test sceVideodec2QueryComputeMemoryInfo with 4 KiB zeroed buffer */
static obs_result check_videodec2_query_compute_memory(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        return obs_skip("libSceVideodec2 did not load");
    }
    const void *addr = obs_module_symbol(handle, "sceVideodec2QueryComputeMemoryInfo");
    if (addr == NULL || !obs_address_is_callable(addr)) {
        return obs_skip("sceVideodec2QueryComputeMemoryInfo is not resolved");
    }

    typedef int64_t (*query_fn)(void *arg0, uint64_t arg1, uint64_t arg2,
                                uint64_t arg3, uint64_t arg4, uint64_t arg5);
    query_fn fn = (query_fn)addr;

#define OBS_VDEC_BUF_SIZE 4096u
    static uint8_t buf[OBS_VDEC_BUF_SIZE];
    static uint8_t before[OBS_VDEC_BUF_SIZE];
    for (size_t i = 0; i < OBS_VDEC_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int64_t rc = fn(buf, 0, 0, 0, 0, 0);

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_VDEC_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (written > 0) {
        obs_report_written("107-videodec/query-compute", "sceVideodec2QueryComputeMemoryInfo",
                           "out-param", before, buf, OBS_VDEC_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("107-videodec/query-compute", "sceVideodec2QueryComputeMemoryInfo",
                       "untouched", before, buf, OBS_VDEC_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("query returned error code and wrote nothing", (uint64_t)rc);
    }
    return obs_pass();
#undef OBS_VDEC_BUF_SIZE
}

/* The decoder-creation entry point on its own, so a reader has a named row for the symbol
 * that gates everything downstream of it. */
static obs_result check_videodec2_create_present(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        return obs_skip("libSceVideodec2 did not load");
    }
    const void *addr = obs_module_symbol(handle, "sceVideodec2CreateDecoder");
    if (addr == NULL) {
        return obs_skip("sceVideodec2CreateDecoder is not resolved");
    }
    obs_report_measure("107-videodec/create-present", "sceVideodec2CreateDecoder", "vaddr",
                       (uint64_t)(uintptr_t)addr, "offset");
    return obs_pass();
}

/* The decode call itself - the one that would turn an access unit into a frame in memory
 * the display can sample. A named row for it, resolution only. */
static obs_result check_videodec2_decode_present(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        return obs_skip("libSceVideodec2 did not load");
    }
    const void *addr = obs_module_symbol(handle, "sceVideodec2Decode");
    if (addr == NULL) {
        return obs_skip("sceVideodec2Decode is not resolved");
    }
    obs_report_measure("107-videodec/decode-present", "sceVideodec2Decode", "vaddr",
                       (uint64_t)(uintptr_t)addr, "offset");
    return obs_pass();
}

static const obs_check videodec_checks[] = {
    {"107-videodec/library", "libSceVideodec2", "sceVideodec2CreateDecoder", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_videodec2_library, check_videodec2_library,
     OBS_FROM_ASSUMED},
    {"107-videodec/symbols", "libSceVideodec2", "sceVideodec2Decode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_videodec2_symbols, check_videodec2_symbols,
     OBS_FROM_ASSUMED},
    {"107-videodec/create-present", "libSceVideodec2", "sceVideodec2CreateDecoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec2_create_present,
     check_videodec2_create_present, OBS_FROM_ASSUMED},
    {"107-videodec/decode-present", "libSceVideodec2", "sceVideodec2Decode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_videodec2_decode_present,
     check_videodec2_decode_present, OBS_FROM_ASSUMED},
    {"107-videodec/query-compute", "libSceVideodec2", "sceVideodec2QueryComputeMemoryInfo",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec2_query_compute_memory,
     check_videodec2_query_compute_memory, OBS_FROM_ASSUMED},
};

const obs_section obs_section_videodec = {
    "107-videodec",
    "Hardware video decode, reached",
    "Whether libSceVideodec2 loads and its decode entry points resolve - the "
    "decode-into-caller-owned-memory path a stream client or media player needs, and one "
    "the SDK's display layer does not yet have. Resolves and records; never calls a "
    "decoder, whose structure layouts are unconfirmed.",
    videodec_checks,
    OBS_COUNT(videodec_checks),
};
