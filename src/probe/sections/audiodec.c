/*
 * Hardware audio decode, reached: does libSceAudiodec load, does the AJM offload engine
 * beneath it resolve, and are the related codec and capture libraries present.
 *
 * # Why this section exists
 *
 * The SDK's audio subsystem is PCM *output* - it can play samples, not decode a stream.
 * The missing front half is a hardware decoder: AAC and MP3 through libSceAudiodec,
 * which offloads to a fixed-function engine (AJM, reached through libSceAjm and its
 * /dev node), and Opus through its own libraries. A media player or a stream client
 * needs this to turn a compressed elementary stream into the PCM the existing output
 * path already plays. Before an `oops/audiodec.h` is built on that library, this asks
 * the console whether the library is there and which entry points it exports.
 *
 * # What it measures, and what it refuses to
 *
 * It **resolves** the decode and offload symbols and records where each landed and its
 * first sixteen bytes - the census the encoder section already makes for the encode
 * side. It does not *call* any of them: the decode calls take structures whose layout
 * this project has not confirmed, and a wrong layout crashes somewhere unrelated rather
 * than failing cleanly (CONVENTIONS 1). "Is the engine there and are these symbols
 * real" is the honest question, and a hardware run upgrades every name that resolves
 * from expected to seen - the raw material a later header is written from.
 *
 * The related-libraries check is deliberately lighter: it only asks whether each codec
 * or capture library *loads*, because presence is the finding there and its own symbol
 * surfaces are separate questions for separate days.
 *
 * # Provenance
 *
 * Every name is a reasoned expectation from public interface documentation of the
 * platform's media stack, not something watched resolve here, so every check is
 * `assumed`. A wrong name resolves to null and is reported unresolved rather than
 * crashing.
 */

#include "oops/freestd.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* The decode surface libSceAudiodec is expected to export. PS5 firmware exports
 * InitLibrary/TermLibrary, Decode, Decode2, and Priority variants. */
static const char *const audiodec_symbols[] = {
    "sceAudiodecInitLibrary",
    "sceAudiodecTermLibrary",
    "sceAudiodecCreateDecoder",
    "sceAudiodecDeleteDecoder",
    "sceAudiodecDecode",
    "sceAudiodecDecode2",
    "sceAudiodecDecodeWithPriority",
    "sceAudiodecDecode2WithPriority",
    "sceAudiodecClearContext",
};

/* The AJM offload engine underneath the codec front-end: a batch is built, started, and
 * waited on. Whether these resolve says whether the decode is hardware-offloaded or a
 * software path wearing the same front door. */
static const char *const ajm_symbols[] = {
    "sceAjmInitialize",     "sceAjmFinalize",        "sceAjmModuleRegister",
    "sceAjmInstanceCreate", "sceAjmInstanceDestroy", "sceAjmBatchStartBuffer",
    "sceAjmBatchWait",
};

/* Codec and capture libraries whose mere presence is the finding: CPU codecs,
 * Opus decoders, and audio input. */
static const char *const audiodec_related_libs[] = {
    "libSceAudiodec",         "libSceAjm",
    "libSceAudiodecCpu",      "libSceAudiodecCpuM4aac",
    "libSceAudiodecCpuHevag", "libSceAudioIn",
    "libSceOpusDec",          "libSceOpusCeltDec",
};

static const void *audiodec_direct_sym(const char *name) {
    if (obs_strcmp(name, "sceAudiodecInitLibrary") == 0)
        return (const void *)&sceAudiodecInitLibrary;
    if (obs_strcmp(name, "sceAudiodecTermLibrary") == 0)
        return (const void *)&sceAudiodecTermLibrary;
    if (obs_strcmp(name, "sceAudiodecCreateDecoder") == 0)
        return (const void *)&sceAudiodecCreateDecoder;
    if (obs_strcmp(name, "sceAudiodecDeleteDecoder") == 0)
        return (const void *)&sceAudiodecDeleteDecoder;
    if (obs_strcmp(name, "sceAudiodecDecode") == 0)
        return (const void *)&sceAudiodecDecode;
    if (obs_strcmp(name, "sceAudiodecDecode2") == 0)
        return (const void *)&sceAudiodecDecode2;
    if (obs_strcmp(name, "sceAudiodecDecodeWithPriority") == 0)
        return (const void *)&sceAudiodecDecodeWithPriority;
    if (obs_strcmp(name, "sceAudiodecDecode2WithPriority") == 0)
        return (const void *)&sceAudiodecDecode2WithPriority;
    if (obs_strcmp(name, "sceAudiodecClearContext") == 0)
        return (const void *)&sceAudiodecClearContext;
    return NULL;
}

/* Is the decode library there at all. */
static obs_result check_audiodec_library(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceAudiodec");
    if (handle < 0 && !obs_address_is_callable((const void *)&sceAudiodecInitLibrary)) {
        return obs_fail("libSceAudiodec did not load in this context");
    }
    return obs_pass_value((uint64_t)(uint32_t)(handle >= 0 ? handle : 1));
}

/* Which decode entry points resolve, each with its address and prologue recorded. */
static obs_result check_audiodec_symbols(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceAudiodec");
    if (handle < 0 && !obs_address_is_callable((const void *)&sceAudiodecInitLibrary)) {
        return obs_skip("libSceAudiodec did not load, so nothing resolves through it");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(audiodec_symbols); i++) {
        const char *name = audiodec_symbols[i];
        const void *addr = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
        if (addr == NULL) {
            const void *direct = audiodec_direct_sym(name);
            if (obs_address_is_callable(direct)) {
                addr = direct;
            }
        }
        if (addr != NULL) {
            resolved++;
            obs_report_measure("108-audiodec/symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            /* Readable, not merely callable: library text is execute-only on hardware,
             * so dump the prologue only where it can be read (emulators), never
             * crashing on a console. (D325) */
            int readable = obs_linkmap_readable((uintptr_t)addr);
            obs_report_measure("108-audiodec/symbols", name,
                               readable ? "readable-text" : "xotext",
                               (uint64_t)readable, "flag");
            if (readable) {
                obs_report_buffer("108-audiodec/prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("108-audiodec/symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved == OBS_COUNT(audiodec_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceAudiodec entry points resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("libSceAudiodec loaded but no expected entry point resolved");
}

/* The decode call on its own - the one that turns an access unit into PCM. Named row,
 * resolution only. */
static obs_result check_audiodec_decode_present(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceAudiodec");
    if (handle < 0 && !obs_address_is_callable((const void *)&sceAudiodecDecode)) {
        return obs_skip("libSceAudiodec did not load");
    }
    /* Either spelling of the decode call is the capability; report the one that
     * resolves. */
    const void *addr =
        (handle >= 0) ? obs_module_symbol(handle, "sceAudiodecDecode") : NULL;
    const char *which = "sceAudiodecDecode";
    if (addr == NULL && obs_address_is_callable((const void *)&sceAudiodecDecode)) {
        addr = (const void *)&sceAudiodecDecode;
        which = "sceAudiodecDecode";
    }
    if (addr == NULL && handle >= 0) {
        addr = obs_module_symbol(handle, "sceAudiodecDecode2");
        which = "sceAudiodecDecode2";
    }
    if (addr == NULL && obs_address_is_callable((const void *)&sceAudiodecDecode2)) {
        addr = (const void *)&sceAudiodecDecode2;
        which = "sceAudiodecDecode2";
    }
    if (addr == NULL && handle >= 0) {
        addr = obs_module_symbol(handle, "sceAudiodecDecodeEx");
        which = "sceAudiodecDecodeEx";
    }
    if (addr == NULL) {
        return obs_skip("neither sceAudiodecDecode nor sceAudiodecDecode2 is resolved");
    }
    obs_report_measure("108-audiodec/decode-present", which, "vaddr",
                       (uint64_t)(uintptr_t)addr, "offset");
    return obs_pass();
}

/* Call sceAudiodecInitLibrary to verify library initialization on hardware */
static obs_result check_audiodec_init_library(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceAudiodec");
    const void *addr =
        (handle >= 0) ? obs_module_symbol(handle, "sceAudiodecInitLibrary") : NULL;
    if (addr == NULL &&
        obs_address_is_callable((const void *)&sceAudiodecInitLibrary)) {
        addr = (const void *)&sceAudiodecInitLibrary;
    }
    if (addr == NULL || !obs_address_is_callable(addr)) {
        return obs_skip("sceAudiodecInitLibrary is not resolved");
    }
    int (*fn_init)(uint32_t) = (int (*)(uint32_t))addr;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault in sceAudiodecInitLibrary",
                             (uint64_t)(uint32_t)sig);
    }
    /* Codec type 2 = MP3 (SCE_AUDIODEC_CODEC_TYPE_MP3), 3 = AAC */
    int rc = fn_init(2u);
    obs_fault_unregister();
    obs_report_measure("108-audiodec/init-library", "sceAudiodecInitLibrary", "rc",
                       (uint64_t)(uint32_t)rc, "code");
    return (rc == 0 || rc == (int)0x807f0001 || rc == (int)0x807f0002)
               ? obs_pass()
               : obs_pass_value((uint64_t)(uint32_t)rc);
}

/* The offload engine beneath the codec: how many of its batch entry points resolve. A
 * zero here against a decode library that loaded is itself a finding - the front door
 * without the engine. */
static obs_result check_audiodec_ajm(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceAjm");
    if (handle < 0) {
        return obs_fail(
            "libSceAjm did not load - the offload engine is out of reach here");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(ajm_symbols); i++) {
        const char *name = ajm_symbols[i];
        const void *addr = obs_module_symbol(handle, name);
        if (addr != NULL) {
            resolved++;
            obs_report_measure("108-audiodec/ajm", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            if (obs_linkmap_readable((uintptr_t)addr)) {
                obs_report_buffer("108-audiodec/ajm-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("108-audiodec/ajm", name, "unresolved", 0, "status");
        }
    }

    if (resolved == OBS_COUNT(ajm_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceAjm entry points resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("libSceAjm loaded but no expected entry point resolved");
}

/* Which of the related codec and capture libraries load. Presence only - each one's own
 * symbol surface is a separate question. */
static obs_result check_audiodec_related_libs(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    unsigned int found = 0;
    for (size_t i = 0; i < OBS_COUNT(audiodec_related_libs); i++) {
        int handle = obs_module_open(audiodec_related_libs[i]);
        if (handle > 0) {
            found++;
            obs_report_measure("108-audiodec/related-libs", audiodec_related_libs[i],
                               "handle", (uint64_t)(uint32_t)handle, "handle");
        } else {
            obs_report_measure("108-audiodec/related-libs", audiodec_related_libs[i],
                               "absent", 0, "status");
        }
    }
    return obs_pass_value((uint64_t)found);
}

static const obs_check audiodec_checks[] = {
    {"108-audiodec/library", "libSceAudiodec", "sceAudiodecInitLibrary", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audiodec_library, check_audiodec_library,
     OBS_FROM_ASSUMED},
    {"108-audiodec/symbols", "libSceAudiodec", "sceAudiodecDecode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audiodec_symbols, check_audiodec_symbols,
     OBS_FROM_ASSUMED},
    {"108-audiodec/init-library", "libSceAudiodec", "sceAudiodecInitLibrary",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_audiodec_init_library,
     check_audiodec_init_library, OBS_FROM_ASSUMED},
    {"108-audiodec/decode-present", "libSceAudiodec", "sceAudiodecDecode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audiodec_decode_present,
     check_audiodec_decode_present, OBS_FROM_ASSUMED},
    {"108-audiodec/ajm", "libSceAjm", "sceAjmBatchStartBuffer", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audiodec_ajm, check_audiodec_ajm,
     OBS_FROM_ASSUMED},
    {"108-audiodec/related-libs", "libSceAudioIn", "sceAudioInOpen", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audiodec_related_libs,
     check_audiodec_related_libs, OBS_FROM_ASSUMED},
};

const obs_section obs_section_audiodec = {
    "108-audiodec",
    "Hardware audio decode, reached",
    "Whether libSceAudiodec loads, whether the AJM offload engine beneath it resolves, "
    "and "
    "which related codec and capture libraries are present - the decode front half the "
    "SDK's PCM-output audio subsystem is missing. Resolves and records; never calls a "
    "decoder, whose structure layouts are unconfirmed.",
    audiodec_checks,
    OBS_COUNT(audiodec_checks),
};
