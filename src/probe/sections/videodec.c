/*
 * Hardware video decode, reached: does libSceVideodec2 load, and do its decode entry
 * points resolve.
 *
 * # Why this section exists
 *
 * The idea it serves: a stream client or a media player that decodes H.264/HEVC/VP9
 * into caller-owned, GPU-visible memory and hands that buffer straight to the display
 * scanout - no CPU copy. That is a capability the SDK does not have yet (its display
 * path is linear SDR RGB and knows nothing about decode), and libSceVideodec2 is the
 * platform library that would provide it. Before anything is built on that library it
 * is worth knowing, from the console itself, whether it is present and whether the
 * entry points a decoder needs are exported.
 *
 * # What it measures, and what it refuses to
 *
 * It **resolves** the decode symbols and records where each one landed, plus the first
 * sixteen bytes of each - the same census the encoder section makes for libSceVencCore.
 * It does not *call* any of them. The decode calls take structures whose layout this
 * project has not confirmed, and a wrong layout does not fail cleanly - it corrupts the
 * stack and crashes somewhere unrelated (CONVENTIONS 1). So the honest question a probe
 * can ask here is "is the library there and are these symbols real", and that is the
 * question it asks. A hardware run turns every symbol that resolves from a name we
 * expect into a name we have seen, which is what a later `oops/videodec.h` would be
 * written from.
 *
 * # Provenance
 *
 * The library and symbol names are a reasoned expectation from public interface
 * documentation of the platform's media stack, not something this project has watched
 * resolve, so every check is `assumed`. A name that is wrong resolves to null and is
 * reported unresolved rather than crashing, so the guess is legible either way.
 */

#include "oops/freestd.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* The decode surface libSceVideodec2 is expected to export. Ordered as a decoder would
 * reach for them: size the working memory, take a compute queue, create the decoder,
 * feed it, drain it, and read back what came out. */
static const char *const videodec2_symbols[] = {
    "sceVideodec2QueryComputeMemoryInfo",
    "sceVideodec2AllocateComputeQueue",
    "sceVideodec2ReleaseComputeQueue",
    "sceVideodec2QueryDecoderMemoryInfo",
    "sceVideodec2CreateDecoder",
    "sceVideodec2DeleteDecoder",
    "sceVideodec2Decode",
    "sceVideodec2Flush",
    "sceVideodec2Reset",
    "sceVideodec2MapDirectMemory",
    "sceVideodec2GetPictureInfo",
    "sceVideodec2GetAvcPictureInfo",
};

static const char *const videodec_symbols[] = {
    "sceVideodecQueryResourceInfo",
    "sceVideodecMapMemory",
    "sceVideodecCreateDecoder",
    "sceVideodecDeleteDecoder",
    "sceVideodecDecode",
    "sceVideodecFlush",
    "sceVideodecReset",
};

static int obs_videodec_open(void) {
    int handle = obs_module_open("libSceVideodec2");
    if (handle < 0) {
        handle = obs_module_open("libSceVideodec");
    }
    return handle;
}

/* Sweep candidate sysmodule IDs (0x0001 - 0x0120) to detect which system modules
 * are valid and loadable on PS5 FW 12.40. */
static obs_result check_videodec_sysmodule_sweep(void) {
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = NULL;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        fn_load = &sceSysmoduleLoadModule;
    } else {
        const void *sym = obs_module_symbol(1, "sceSysmoduleLoadModule");
        if (sym != NULL && obs_address_is_callable(sym)) {
            fn_load = (fn_load_t)sym;
        }
    }
    if (fn_load == NULL) {
        return obs_skip("sceSysmoduleLoadModule is not available");
    }

    unsigned int loaded_count = 0;
    static const uint16_t sweep_ids[] = {
        0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x000b, 0x000c, 0x000e,
        0x000f, 0x0010, 0x0011, 0x0014, 0x0016, 0x0017, 0x001a, 0x001d, 0x0027, 0x0028,
        0x005e, 0x005f, 0x0073, 0x0080, 0x0081, 0x0083, 0x0084, 0x0088, 0x008a, 0x008b,
        0x008c, 0x008d, 0x008e, 0x008f, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009c,
        0x009d, 0x00a0, 0x00a2, 0x00a4, 0x00a5, 0x00a7, 0x00a8, 0x00a9, 0x00ab, 0x00ac,
        0x00ad, 0x00b4, 0x00b6, 0x00b7, 0x00ba, 0x00c5, 0x00cf, 0x0106, 0x010f,
    };

    for (size_t i = 0; i < OBS_COUNT(sweep_ids); i++) {
        uint16_t id = sweep_ids[i];
        int mod_list_before[128];
        size_t count_before = 0;
        if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
            (void)sceKernelGetModuleList(mod_list_before, 128, &count_before);
        }
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = fn_load(id);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            continue;
        }
        if (rc == 0 || rc == (int)0x80540001) {
            loaded_count++;
            char id_str[32] = "id_";
            obs_format_hex(id_str + 3, id);
            obs_report_measure("107-videodec/sysmodule-sweep", id_str, "rc",
                               (uint64_t)(uint32_t)rc, "code");
            if (rc == 0 &&
                obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
                int mod_list_after[128];
                size_t count_after = 0;
                if (sceKernelGetModuleList(mod_list_after, 128, &count_after) == 0 &&
                    count_after > 0) {
                    for (size_t a = 0; a < count_after && a < 128; a++) {
                        int mid = mod_list_after[a];
                        int was_present = 0;
                        for (size_t b = 0; b < count_before && b < 128; b++) {
                            if (mod_list_before[b] == mid) {
                                was_present = 1;
                                break;
                            }
                        }
                        if (!was_present && mid > 0) {
                            obs_sysmodule_cache_handle_by_id(id, mid);
                            break;
                        }
                    }
                }
            }
        }
    }
    return obs_pass_value((uint64_t)loaded_count);
}

/* Is the library there at all - the first thing anything decode-shaped needs to know.
 */
static obs_result check_videodec2_library(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_videodec_open();
    if (handle < 0) {
        return obs_fail(
            "neither libSceVideodec2 nor libSceVideodec loaded in this context");
    }
    return obs_pass_value((uint64_t)(uint32_t)handle);
}

static const void *videodec_direct_sym(const char *name) {
    if (obs_strcmp(name, "sceVideodec2CreateDecoder") == 0)
        return (const void *)&sceVideodec2CreateDecoder;
    if (obs_strcmp(name, "sceVideodec2DeleteDecoder") == 0)
        return (const void *)&sceVideodec2DeleteDecoder;
    if (obs_strcmp(name, "sceVideodec2Decode") == 0)
        return (const void *)&sceVideodec2Decode;
    if (obs_strcmp(name, "sceVideodec2Flush") == 0)
        return (const void *)&sceVideodec2Flush;
    if (obs_strcmp(name, "sceVideodec2Reset") == 0)
        return (const void *)&sceVideodec2Reset;
    if (obs_strcmp(name, "sceVideodec2QueryComputeMemoryInfo") == 0)
        return (const void *)&sceVideodec2QueryComputeMemoryInfo;
    if (obs_strcmp(name, "sceVideodec2QueryDecoderMemoryInfo") == 0)
        return (const void *)&sceVideodec2QueryDecoderMemoryInfo;
    if (obs_strcmp(name, "sceVideodec2AllocateComputeQueue") == 0)
        return (const void *)&sceVideodec2AllocateComputeQueue;
    if (obs_strcmp(name, "sceVideodec2ReleaseComputeQueue") == 0)
        return (const void *)&sceVideodec2ReleaseComputeQueue;
    if (obs_strcmp(name, "sceVideodec2MapDirectMemory") == 0)
        return (const void *)&sceVideodec2MapDirectMemory;
    if (obs_strcmp(name, "sceVideodec2GetPictureInfo") == 0)
        return (const void *)&sceVideodec2GetPictureInfo;
    if (obs_strcmp(name, "sceVideodec2GetAvcPictureInfo") == 0)
        return (const void *)&sceVideodec2GetAvcPictureInfo;
    return NULL;
}

/* Which of the expected decode entry points actually resolve, with where each landed
 * and its first sixteen bytes recorded for a later diff. Never calls one. */
static obs_result check_videodec2_symbols(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_videodec_open();
    if (handle < 0 &&
        !obs_address_is_callable((const void *)&sceVideodec2CreateDecoder)) {
        return obs_skip("neither libSceVideodec2 nor libSceVideodec loaded");
    }

    const char *const *syms = videodec2_symbols;
    size_t count = OBS_COUNT(videodec2_symbols);
    if (handle >= 0 && obs_module_symbol(handle, "sceVideodec2CreateDecoder") == NULL &&
        !obs_address_is_callable((const void *)&sceVideodec2CreateDecoder) &&
        obs_module_symbol(handle, "sceVideodecCreateDecoder") != NULL) {
        syms = videodec_symbols;
        count = OBS_COUNT(videodec_symbols);
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < count; i++) {
        const char *name = syms[i];
        const void *addr = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
        if (addr == NULL) {
            const void *direct = videodec_direct_sym(name);
            if (obs_address_is_callable(direct)) {
                addr = direct;
            }
        }
        if (addr != NULL) {
            resolved++;
            obs_report_measure("107-videodec/symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            /* Readable, not merely callable: library text is execute-only on hardware,
             * so dump the prologue only where it can be read (emulators), never
             * crashing on a console. (D325) */
            int readable = obs_linkmap_readable((uintptr_t)addr);
            obs_report_measure("107-videodec/symbols", name,
                               readable ? "readable-text" : "xotext",
                               (uint64_t)readable, "flag");
            if (readable) {
                obs_report_buffer("107-videodec/prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("107-videodec/symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved == count) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some video decode entry points resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("video decode library loaded but no expected entry point resolved");
}

/* Out-param capture: test sceVideodec2QueryComputeMemoryInfo with 4 KiB zeroed buffer
 */
static obs_result check_videodec2_query_compute_memory(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_videodec_open();
    if (handle < 0 &&
        !obs_address_is_callable((const void *)&sceVideodec2QueryComputeMemoryInfo)) {
        return obs_skip("neither libSceVideodec2 nor libSceVideodec loaded");
    }
    const void *addr =
        (handle >= 0) ? obs_module_symbol(handle, "sceVideodec2QueryComputeMemoryInfo")
                      : NULL;
    if (addr == NULL &&
        obs_address_is_callable((const void *)&sceVideodec2QueryComputeMemoryInfo)) {
        addr = (const void *)&sceVideodec2QueryComputeMemoryInfo;
    }
    if (addr == NULL || !obs_address_is_callable(addr)) {
        return obs_skip("sceVideodec2QueryComputeMemoryInfo is not resolved");
    }

    typedef int64_t (*query_fn)(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5);
    query_fn fn = (query_fn)addr;

#define OBS_VDEC_BUF_SIZE 4096u
    static uint8_t buf[OBS_VDEC_BUF_SIZE];
    static uint8_t before[OBS_VDEC_BUF_SIZE];
    for (size_t i = 0; i < OBS_VDEC_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int64_t rc = -1;
    if (sig == 0) {
        rc = fn(buf, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_skip("fault during sceVideodec2QueryComputeMemoryInfo");
    }

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_VDEC_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (written > 0) {
        obs_report_written("107-videodec/query-compute",
                           "sceVideodec2QueryComputeMemoryInfo", "out-param", before,
                           buf, OBS_VDEC_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("107-videodec/query-compute",
                       "sceVideodec2QueryComputeMemoryInfo", "untouched", before, buf,
                       OBS_VDEC_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("query returned error code and wrote nothing",
                                 (uint64_t)rc);
    }
    return obs_pass();
#undef OBS_VDEC_BUF_SIZE
}

/* The decoder-creation entry point on its own, so a reader has a named row for the
 * symbol that gates everything downstream of it. */
static obs_result check_videodec2_create_present(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_videodec_open();
    if (handle < 0 &&
        !obs_address_is_callable((const void *)&sceVideodec2CreateDecoder)) {
        return obs_skip("neither libSceVideodec2 nor libSceVideodec loaded");
    }
    const char *name = "sceVideodec2CreateDecoder";
    const void *addr = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
    if (addr == NULL &&
        obs_address_is_callable((const void *)&sceVideodec2CreateDecoder)) {
        addr = (const void *)&sceVideodec2CreateDecoder;
    }
    if (addr == NULL && handle >= 0) {
        name = "sceVideodecCreateDecoder";
        addr = obs_module_symbol(handle, name);
    }
    if (addr == NULL) {
        return obs_skip("create decoder entry point is not resolved");
    }
    obs_report_measure("107-videodec/create-present", name, "vaddr",
                       (uint64_t)(uintptr_t)addr, "offset");
    return obs_pass();
}

/* The decode call on its own - the one that turns an access unit into a frame. Named
 * row, resolution only. */
static obs_result check_videodec2_decode_present(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_videodec_open();
    if (handle < 0 && !obs_address_is_callable((const void *)&sceVideodec2Decode)) {
        return obs_skip("neither libSceVideodec2 nor libSceVideodec loaded");
    }
    const char *name = "sceVideodec2Decode";
    const void *addr = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
    if (addr == NULL && obs_address_is_callable((const void *)&sceVideodec2Decode)) {
        addr = (const void *)&sceVideodec2Decode;
    }
    if (addr == NULL && handle >= 0) {
        name = "sceVideodecDecode";
        addr = obs_module_symbol(handle, name);
    }
    if (addr == NULL) {
        return obs_skip("decode entry point is not resolved");
    }
    obs_report_measure("107-videodec/decode-present", name, "vaddr",
                       (uint64_t)(uintptr_t)addr, "offset");
    return obs_pass();
}

static const obs_check videodec_checks[] = {
    {"107-videodec/sysmodule-sweep", "libSceSysmodule", "sceSysmoduleLoadModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec_sysmodule_sweep,
     check_videodec_sysmodule_sweep, OBS_FROM_DERIVED},
    {"107-videodec/library", "libSceVideodec2", "sceVideodec2CreateDecoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec2_library,
     check_videodec2_library, OBS_FROM_ASSUMED},
    {"107-videodec/symbols", "libSceVideodec2", "sceVideodec2Decode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_videodec2_symbols, check_videodec2_symbols,
     OBS_FROM_ASSUMED},
    {"107-videodec/create-present", "libSceVideodec2", "sceVideodec2CreateDecoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec2_create_present,
     check_videodec2_create_present, OBS_FROM_ASSUMED},
    {"107-videodec/decode-present", "libSceVideodec2", "sceVideodec2Decode",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videodec2_decode_present,
     check_videodec2_decode_present, OBS_FROM_ASSUMED},
    {"107-videodec/query-compute", "libSceVideodec2",
     "sceVideodec2QueryComputeMemoryInfo", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_videodec2_query_compute_memory,
     check_videodec2_query_compute_memory, OBS_FROM_ASSUMED},
};

const obs_section obs_section_videodec = {
    "107-videodec",
    "Hardware video decode, reached",
    "Whether libSceVideodec2 loads and its decode entry points resolve - the "
    "decode-into-caller-owned-memory path a stream client or media player needs, and "
    "one "
    "the SDK's display layer does not yet have. Resolves and records; never calls a "
    "decoder, whose structure layouts are unconfirmed.",
    videodec_checks,
    OBS_COUNT(videodec_checks),
};
