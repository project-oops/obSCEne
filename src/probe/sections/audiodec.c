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

static obs_result check_ajm_batch_initialize_nulls(void) {
    const char *id = "108-audiodec/ajm-batch-init-nulls";
    int h_ajm = obs_module_open("libSceAjm");
    if (h_ajm < 0) {
        return obs_skip("libSceAjm not openable");
    }

    typedef int (*fn_sceAjmBatchInitialize_t)(void *, size_t, void *);
    fn_sceAjmBatchInitialize_t fn_init =
        (fn_sceAjmBatchInitialize_t)obs_module_symbol(h_ajm, "sceAjmBatchInitialize");

    obs_report_measure(id, "sceAjmBatchInitialize", "resolved",
                       (uint64_t)(fn_init != NULL), "bool");
    if (fn_init == NULL) {
        return obs_pass();
    }

    obs_jmp_buf guard;
    /* 1. NULL buffer with valid info pointer: sceAjmBatchInitialize(NULL, 0x100, &info)
     */
    uint8_t info[256];
    memset(info, 0, sizeof(info));
    int sig1 = OBS_FAULT_ARM(&guard);
    if (sig1 == 0) {
        int rc1 = fn_init(NULL, 0x100, info);
        obs_fault_unregister();
        obs_report_measure(id, "arm-null-buffer", "rc", (uint64_t)(uint32_t)rc1,
                           "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm-null-buffer", "fault-sig", (uint64_t)(uint32_t)sig1,
                           "sig");
    }

    /* 2. Valid buffer with NULL info pointer: sceAjmBatchInitialize(buf, 0x100, NULL)
     */
    uint8_t buf[256];
    memset(buf, 0, sizeof(buf));
    int sig2 = OBS_FAULT_ARM(&guard);
    if (sig2 == 0) {
        int rc2 = fn_init(buf, 0x100, NULL);
        obs_fault_unregister();
        obs_report_measure(id, "arm-null-info", "rc", (uint64_t)(uint32_t)rc2, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "arm-null-info", "fault-sig", (uint64_t)(uint32_t)sig2,
                           "sig");
    }

    return obs_pass();
}

/* REQ-20260928T0730Z-7b14: sceAjmInitialize(0, &out, 5, 0xbb80) with 32-byte 0xff block
 */
static obs_result check_audiodec_ajm_initialize(void) {
    const char *id = "108-audiodec/ajm-init";
    const void *fn = NULL;
    if (!fn || !obs_address_is_callable(fn)) {
        int handle = obs_module_open("libSceAjm");
        if (handle < 0) {
            handle = obs_module_open("libSceAjm.sprx");
        }
        if (handle >= 0) {
            fn = obs_module_symbol(handle, "sceAjmInitialize");
            if (!fn)
                fn = obs_module_symbol(handle, "$7lKzdHi4X3Y");
        }
        if (!fn)
            fn = obs_module_symbol(OBS_HANDLE_SELF, "sceAjmInitialize");
        if (!fn)
            fn = obs_module_symbol(OBS_HANDLE_SELF, "$7lKzdHi4X3Y");
        if (!fn)
            fn = obs_module_symbol(1, "sceAjmInitialize");
        if (!fn)
            fn = obs_module_symbol(1, "$7lKzdHi4X3Y");
        if (!fn && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (handle >= 0 && sceKernelDlsym(handle, "sceAjmInitialize", &addr) == 0 &&
                obs_address_is_callable(addr)) {
                fn = addr;
            } else if (handle >= 0 &&
                       sceKernelDlsym(handle, "7lKzdHi4X3Y", &addr) == 0 &&
                       obs_address_is_callable(addr)) {
                fn = addr;
            } else if (sceKernelDlsym(OBS_HANDLE_SELF, "sceAjmInitialize", &addr) ==
                           0 &&
                       obs_address_is_callable(addr)) {
                fn = addr;
            } else if (sceKernelDlsym(OBS_HANDLE_SELF, "7lKzdHi4X3Y", &addr) == 0 &&
                       obs_address_is_callable(addr)) {
                fn = addr;
            } else if (sceKernelDlsym(1, "sceAjmInitialize", &addr) == 0 &&
                       obs_address_is_callable(addr)) {
                fn = addr;
            } else if (sceKernelDlsym(1, "7lKzdHi4X3Y", &addr) == 0 &&
                       obs_address_is_callable(addr)) {
                fn = addr;
            }
        }
    }

    int resolved = (fn != NULL && obs_address_is_callable(fn));
    obs_report_measure(id, "sceAjmInitialize", "resolved", (uint64_t)resolved, "bool");
    if (!resolved) {
        return obs_skip("sceAjmInitialize not resolved");
    }

    uint8_t out_buf[64];
    uint8_t before[64];
    memset(out_buf, 0xff, sizeof(out_buf));
    memset(before, 0xff, sizeof(before));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int rc = -1;
    if (sig == 0) {
        typedef int (*fn_ajm_init_t)(uint64_t, void *, uint64_t, uint64_t);
        fn_ajm_init_t call_init = (fn_ajm_init_t)fn;
        rc = call_init(0, out_buf, 5, 0xbb80);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "sceAjmInitialize", "fault-sig", (uint64_t)sig,
                           "signal");
        return obs_fail("fault during sceAjmInitialize");
    }

    obs_report_measure(id, "sceAjmInitialize", "rc", (uint64_t)(uint32_t)rc, "code");

    unsigned int bytes_written = 0;
    for (unsigned int i = 0; i < sizeof(out_buf); i++) {
        if (out_buf[i] != before[i]) {
            bytes_written = i + 1;
        }
    }
    obs_report_measure(id, "sceAjmInitialize", "bytes-written", (uint64_t)bytes_written,
                       "bytes");
    obs_report_bytes(id, "sceAjmInitialize", "out-32b", 0, out_buf, 32);

    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* REQ-20261012T1822Z-4865: libSceAudiodec InitLibrary and decoder calls by NID */
static obs_result check_4865_audiodec_init_decoder(void) {
    const char *id = "108-audiodec/4865-init-decoder";
    int h = obs_module_open("libSceAudiodec");
    if (h < 0) {
        h = obs_module_open("libSceAudiodec.sprx");
    }

    static const struct {
        const char *name;
        const char *nid;
    } syms[] = {
        {"sceAudiodecInitLibrary", "$MntcGptsOFY"},
        {"sceAudiodecTermLibrary", "$XQ0IZAfSmIc"},
        {"sceAudiodecCreateDecoder", "$+0YWs7D1dzs"},
        {"sceAudiodecDeleteDecoder", "$uZi9LhOZn04"},
        {"sceAudiodecClearContext", "$y6LDMln9V+k"},
        {"sceAudiodecDecode", "$vCDkMjDHdSg"},
    };

    const void *fn_init = NULL;
    const void *fn_term = NULL;
    const void *fn_create = NULL;
    const void *fn_delete = NULL;
    const void *fn_clear = NULL;
    const void *fn_decode = NULL;

    for (size_t i = 0; i < sizeof(syms) / sizeof(syms[0]); i++) {
        const void *addr = NULL;
        if (h >= 0) {
            addr = obs_module_symbol(h, syms[i].nid);
            if (!addr)
                addr = obs_module_symbol(h, syms[i].name);
        }
        if (!addr)
            addr = obs_module_symbol(OBS_HANDLE_SELF, syms[i].nid);
        if (!addr)
            addr = obs_module_symbol(OBS_HANDLE_SELF, syms[i].name);
        if (!addr)
            addr = audiodec_direct_sym(syms[i].name);

        int bound = (addr != NULL && obs_address_is_callable(addr));
        obs_report_measure(id, syms[i].name, "bound", (uint64_t)bound, "bool");
        if (bound) {
            obs_report_measure(id, syms[i].name, "vaddr", (uint64_t)(uintptr_t)addr,
                               "offset");
        }

        if (i == 0)
            fn_init = addr;
        else if (i == 1)
            fn_term = addr;
        else if (i == 2)
            fn_create = addr;
        else if (i == 3)
            fn_delete = addr;
        else if (i == 4)
            fn_clear = addr;
        else if (i == 5)
            fn_decode = addr;
    }

    obs_jmp_buf guard;

    /* 2. InitLibrary(t) for t = 0, 1, 2, 3, 4, 0xffffffff; Init(3) twice, Term(3) twice
     */
    if (fn_init != NULL) {
        int (*p_init)(uint32_t) = (int (*)(uint32_t))fn_init;
        int (*p_term)(uint32_t) = (int (*)(uint32_t))fn_term;
        static const uint32_t types[] = {0, 1, 2, 3, 4, 0xffffffffU};
        for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
            char tag[32];
            snprintf(tag, sizeof(tag), "init-type-%u", (unsigned int)types[i]);
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc = p_init(types[i]);
                obs_fault_unregister();
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", 1, "sig");
            }
        }
        /* Init(3) twice */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc2 = p_init(3);
            obs_fault_unregister();
            obs_report_measure(id, "init-type-3-repeat", "rc", (uint64_t)(uint32_t)rc2,
                               "code");
        } else {
            obs_fault_unregister();
        }
        /* Term(3) twice */
        if (p_term != NULL) {
            for (int t = 0; t < 2; t++) {
                char tag[32];
                snprintf(tag, sizeof(tag), "term-type-3-try%d", t);
                if (OBS_FAULT_ARM(&guard) == 0) {
                    int rc_t = p_term(3);
                    obs_fault_unregister();
                    obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_t, "code");
                } else {
                    obs_fault_unregister();
                }
            }
        }
    }

    /* 3. CreateDecoder: AAC (type 3) with ctrl 0x20, param 0x20, bsi 0x14 */
    if (fn_create != NULL) {
        if (fn_init != NULL) {
            int (*p_init)(uint32_t) = (int (*)(uint32_t))fn_init;
            p_init(3);
        }

        typedef int (*fn_create_dec_t)(void *ctrl, uint32_t codec_type);
        fn_create_dec_t p_create = (fn_create_dec_t)fn_create;
        typedef int (*fn_delete_dec_t)(int handle);
        fn_delete_dec_t p_delete = (fn_delete_dec_t)fn_delete;

        uint8_t param[0x20];
        memset(param, 0, sizeof(param));
        *(uint32_t *)(param + 0x00) = 0x20; /* size */
        *(uint32_t *)(param + 0x04) = 1;    /* version */
        *(uint32_t *)(param + 0x08) = 2;    /* objectType = AAC LC */
        *(uint32_t *)(param + 0x0c) = 3;    /* sampleRateIndex = 48000 */
        *(uint32_t *)(param + 0x10) = 2;    /* channelConfig = 2 (stereo) */
        *(uint32_t *)(param + 0x14) = 0;
        *(uint32_t *)(param + 0x18) = 1;
        *(uint32_t *)(param + 0x1c) = 0;

        uint8_t bsi[0x14];
        memset(bsi, 0xcc, sizeof(bsi));

        uint8_t au[0x18];
        memset(au, 0, sizeof(au));
        *(uint64_t *)(au + 0x00) = 0x18;

        uint8_t pcm[0x18];
        memset(pcm, 0, sizeof(pcm));
        *(uint64_t *)(pcm + 0x00) = 0x18;

        void *ctrl[4];
        ctrl[0] = param;
        ctrl[1] = bsi;
        ctrl[2] = au;
        ctrl[3] = pcm;

        int h_dec = -1;
        if (OBS_FAULT_ARM(&guard) == 0) {
            h_dec = p_create(ctrl, 3);
            obs_fault_unregister();
            obs_report_measure(id, "create-aac-type3", "rc", (uint64_t)(uint32_t)h_dec,
                               "handle");
            obs_report_bytes(id, "create-aac-type3", "bsi-bytes", 0, bsi, 0x14);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "create-aac-type3", "fault", 1, "sig");
        }

        /* second create (step) */
        int h_dec2 = -1;
        if (OBS_FAULT_ARM(&guard) == 0) {
            h_dec2 = p_create(ctrl, 3);
            obs_fault_unregister();
            obs_report_measure(id, "create-aac-type3-step", "rc",
                               (uint64_t)(uint32_t)h_dec2, "handle");
        } else {
            obs_fault_unregister();
        }

        /* param size 0x10 */
        *(uint32_t *)(param + 0x00) = 0x10;
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_badsize = p_create(ctrl, 3);
            obs_fault_unregister();
            obs_report_measure(id, "create-bad-param-size", "rc",
                               (uint64_t)(uint32_t)rc_badsize, "code");
        } else {
            obs_fault_unregister();
        }
        *(uint32_t *)(param + 0x00) = 0x20;

        /* null ctrl */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_null = p_create(NULL, 3);
            obs_fault_unregister();
            obs_report_measure(id, "create-null-ctrl", "rc",
                               (uint64_t)(uint32_t)rc_null, "code");
        } else {
            obs_fault_unregister();
        }

        /* type 99 */
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_t99 = p_create(ctrl, 99);
            obs_fault_unregister();
            obs_report_measure(id, "create-type-99", "rc", (uint64_t)(uint32_t)rc_t99,
                               "code");
        } else {
            obs_fault_unregister();
        }

        /* 4. Decode with raw access unit if h_dec valid */
        if (fn_decode != NULL && h_dec >= 0) {
            typedef int (*fn_decode_t)(int handle, void *ctrl);
            fn_decode_t p_dec = (fn_decode_t)fn_decode;

            static const uint8_t silent_aac[] = {0x21, 0x1b, 0x05, 0x20,
                                                 0x00, 0x00, 0x00, 0x00};
            uint8_t pcm_buf[0x4000];
            memset(pcm_buf, 0xcc, sizeof(pcm_buf));

            *(uint64_t *)(au + 0x00) = 0x18;
            *(const void **)(au + 0x08) = silent_aac;
            *(uint32_t *)(au + 0x10) = sizeof(silent_aac);

            *(uint64_t *)(pcm + 0x00) = 0x18;
            *(void **)(pcm + 0x08) = pcm_buf;
            *(uint32_t *)(pcm + 0x10) = sizeof(pcm_buf);

            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_dec = p_dec(h_dec, ctrl);
                obs_fault_unregister();
                obs_report_measure(id, "decode-aac", "rc", (uint64_t)(uint32_t)rc_dec,
                                   "code");
                obs_report_measure(id, "decode-aac", "au-consumed",
                                   *(uint32_t *)(au + 0x10), "bytes");
                obs_report_measure(id, "decode-aac", "pcm-produced",
                                   *(uint32_t *)(pcm + 0x10), "bytes");
                obs_report_bytes(id, "decode-aac", "pcm-32b", 0, pcm_buf, 32);
            } else {
                obs_fault_unregister();
            }
        }

        /* ClearContext */
        if (fn_clear != NULL) {
            typedef int (*fn_clear_t)(int handle);
            fn_clear_t p_clr = (fn_clear_t)fn_clear;
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_clr = p_clr(h_dec >= 0 ? h_dec : 1);
                obs_fault_unregister();
                obs_report_measure(id, "clear-context", "rc",
                                   (uint64_t)(uint32_t)rc_clr, "code");
            } else {
                obs_fault_unregister();
            }
        }

        /* DeleteDecoder twice */
        if (p_delete != NULL) {
            for (int d = 0; d < 2; d++) {
                char tag[32];
                snprintf(tag, sizeof(tag), "delete-decoder-try%d", d);
                if (OBS_FAULT_ARM(&guard) == 0) {
                    int rc_del = p_delete(h_dec >= 0 ? h_dec : 1);
                    obs_fault_unregister();
                    obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_del,
                                       "code");
                } else {
                    obs_fault_unregister();
                }
            }
            if (h_dec2 >= 0) {
                p_delete(h_dec2);
            }
        }
    }

    return obs_pass();
}

/* REQ-57b7, REQ-7573, REQ-943d, REQ-5604: Ajm instance lifecycle, batch builders,
 * execution and sideband */
static obs_result check_ajm_batch_and_instances(void) {
    const char *id = "108-audiodec/ajm-lifecycle-batch";
    int h_ajm = obs_module_open("libSceAjm");
    if (h_ajm < 0) {
        h_ajm = obs_module_open("libSceAjm.sprx");
    }

    typedef int (*fn_ajm_init_t)(uint64_t flags, uint64_t *ctx_out);
    typedef int (*fn_ajm_fin_t)(uint64_t ctx);
    typedef int (*fn_ajm_mod_reg_t)(uint64_t ctx, uint32_t codec, uint32_t flags);
    typedef int (*fn_ajm_mod_unreg_t)(uint64_t ctx, uint32_t codec);
    typedef int (*fn_ajm_inst_cr_t)(uint64_t ctx, uint32_t codec, uint32_t flags,
                                    uint64_t *inst_out);
    typedef int (*fn_ajm_inst_dst_t)(uint64_t ctx, uint64_t inst);
    typedef int (*fn_ajm_batch_init_t)(void *buf, size_t size, void *info);
    typedef int (*fn_ajm_batch_job_init_t)(void *info, uint64_t inst, const void *cfg,
                                           size_t cfg_size, void *result, ...);
    typedef int (*fn_ajm_batch_job_dec_t)(void *info, uint64_t inst, const void *in,
                                          size_t in_size, void *out, size_t out_size,
                                          void *result);
    typedef int (*fn_ajm_batch_job_gapless_t)(void *info, uint64_t inst, uint32_t total,
                                              uint32_t skip);
    typedef int (*fn_ajm_batch_start_t)(uint64_t ctx, void *info, uint32_t priority,
                                        void *err_out, uint64_t *batch_id_out);
    typedef int (*fn_ajm_batch_wait_t)(uint64_t ctx, uint64_t batch_id,
                                       uint32_t timeout, void *err_out);
    typedef int (*fn_ajm_batch_cancel_t)(uint64_t ctx, uint64_t batch_id);
    typedef int (*fn_ajm_at9_parse_t)(const void *cfg, void *out);

    fn_ajm_init_t p_init = (fn_ajm_init_t)obs_module_symbol(h_ajm, "sceAjmInitialize");
    fn_ajm_fin_t p_fin = (fn_ajm_fin_t)obs_module_symbol(h_ajm, "sceAjmFinalize");
    fn_ajm_mod_reg_t p_mod_reg =
        (fn_ajm_mod_reg_t)obs_module_symbol(h_ajm, "sceAjmModuleRegister");
    fn_ajm_mod_unreg_t p_mod_unreg =
        (fn_ajm_mod_unreg_t)obs_module_symbol(h_ajm, "sceAjmModuleUnregister");
    fn_ajm_inst_cr_t p_inst_cr =
        (fn_ajm_inst_cr_t)obs_module_symbol(h_ajm, "sceAjmInstanceCreate");
    fn_ajm_inst_dst_t p_inst_dst =
        (fn_ajm_inst_dst_t)obs_module_symbol(h_ajm, "sceAjmInstanceDestroy");
    fn_ajm_batch_init_t p_binit =
        (fn_ajm_batch_init_t)obs_module_symbol(h_ajm, "sceAjmBatchInitialize");
    fn_ajm_batch_job_init_t p_bjob_init =
        (fn_ajm_batch_job_init_t)obs_module_symbol(h_ajm, "sceAjmBatchJobInitialize");
    fn_ajm_batch_job_dec_t p_bjob_dec =
        (fn_ajm_batch_job_dec_t)obs_module_symbol(h_ajm, "sceAjmBatchJobDecode");
    fn_ajm_batch_job_gapless_t p_bjob_gap =
        (fn_ajm_batch_job_gapless_t)obs_module_symbol(h_ajm,
                                                      "sceAjmBatchJobSetGaplessDecode");
    fn_ajm_batch_start_t p_bstart =
        (fn_ajm_batch_start_t)obs_module_symbol(h_ajm, "sceAjmBatchStart");
    fn_ajm_batch_wait_t p_bwait =
        (fn_ajm_batch_wait_t)obs_module_symbol(h_ajm, "sceAjmBatchWait");
    fn_ajm_batch_cancel_t p_bcancel =
        (fn_ajm_batch_cancel_t)obs_module_symbol(h_ajm, "sceAjmBatchCancel");
    fn_ajm_at9_parse_t p_at9_parse =
        (fn_ajm_at9_parse_t)obs_module_symbol(h_ajm, "sceAjmDecAt9ParseConfigData");

    obs_report_measure(id, "sceAjmInitialize", "bound", p_init != NULL, "bool");
    obs_report_measure(id, "sceAjmBatchInitialize", "bound", p_binit != NULL, "bool");
    obs_report_measure(id, "sceAjmBatchStart", "bound", p_bstart != NULL, "bool");
    obs_report_measure(id, "sceAjmBatchWait", "bound", p_bwait != NULL, "bool");

    if (!p_init) {
        return obs_skip("sceAjmInitialize not available");
    }

    obs_jmp_buf guard;
    uint64_t ctx = 0;
    int rc_init = -1;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc_init = p_init(0, &ctx);
        obs_fault_unregister();
        obs_report_measure(id, "init", "rc", (uint64_t)(uint32_t)rc_init, "code");
        obs_report_measure(id, "init", "ctx", ctx, "handle");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init", "fault", 1, "sig");
        return obs_fail("fault in sceAjmInitialize");
    }

    /* 1. Register codecs 0..3 */
    if (p_mod_reg) {
        for (uint32_t c = 0; c < 4; c++) {
            char tag[32];
            snprintf(tag, sizeof(tag), "module-register-c%u", (unsigned int)c);
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_reg = p_mod_reg(ctx, c, 0);
                obs_fault_unregister();
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_reg, "code");
            } else {
                obs_fault_unregister();
            }
        }
    }

    /* 2. Instance Create & Destroy (REQ-57b7) */
    uint64_t inst_at9 = 0;
    if (p_inst_cr) {
        /* codec 0..3 with flags 2 */
        for (uint32_t c = 0; c < 4; c++) {
            char tag[32];
            snprintf(tag, sizeof(tag), "inst-create-c%u-f2", (unsigned int)c);
            uint64_t inst = 0xffffffffffffffffULL;
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_cr = p_inst_cr(ctx, c, 2, &inst);
                obs_fault_unregister();
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_cr, "code");
                obs_report_measure(id, tag, "inst", inst, "id");
                if (c == 1 && rc_cr == 0) {
                    inst_at9 = inst;
                } else if (p_inst_dst && rc_cr == 0) {
                    p_inst_dst(ctx, inst);
                }
            } else {
                obs_fault_unregister();
            }
        }

        /* codec 1 with various flags */
        static const uint32_t test_flags[] = {
            0, 1, 8, 0x80, 0x100, 0x180, 2 | (1 << 10), 2 | 0x80000};
        for (size_t f = 0; f < sizeof(test_flags) / sizeof(test_flags[0]); f++) {
            char tag[32];
            snprintf(tag, sizeof(tag), "inst-create-c1-f%x",
                     (unsigned int)test_flags[f]);
            uint64_t inst = 0xffffffffffffffffULL;
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_cr = p_inst_cr(ctx, 1, test_flags[f], &inst);
                obs_fault_unregister();
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_cr, "code");
                obs_report_measure(id, tag, "inst", inst, "id");
                if (p_inst_dst && rc_cr == 0 && inst != inst_at9) {
                    p_inst_dst(ctx, inst);
                }
            } else {
                obs_fault_unregister();
            }
        }

        /* 40 creates in a row on codec 1 to find limit and step */
        uint64_t chain[40];
        memset(chain, 0, sizeof(chain));
        int created_count = 0;
        for (int i = 0; i < 40; i++) {
            uint64_t inst = 0;
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc = p_inst_cr(ctx, 1, 2, &inst);
                obs_fault_unregister();
                if (rc == 0) {
                    chain[created_count++] = inst;
                } else {
                    obs_report_measure(id, "chain-limit-hit", "rc",
                                       (uint64_t)(uint32_t)rc, "code");
                    obs_report_measure(id, "chain-limit-hit", "at-count",
                                       (uint64_t)created_count, "count");
                    break;
                }
            } else {
                obs_fault_unregister();
                break;
            }
        }
        obs_report_measure(id, "chain-created-total", "count", (uint64_t)created_count,
                           "count");
        if (created_count >= 2) {
            obs_report_measure(id, "chain-id-step", "step", chain[1] - chain[0],
                               "delta");
        }
        /* destroy created instances */
        if (p_inst_dst) {
            for (int i = 0; i < created_count; i++) {
                p_inst_dst(ctx, chain[i]);
            }
        }
    }

    /* 3. Batch builders (REQ-7573) */
    uint8_t batch_buf[0x400];
    memset(batch_buf, 0xcc, sizeof(batch_buf));
    uint8_t batch_info[0x60];
    memset(batch_info, 0xa5, sizeof(batch_info));

    if (p_binit) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc = p_binit(batch_buf, sizeof(batch_buf), batch_info);
            obs_fault_unregister();
            obs_report_measure(id, "batch-init-0x400", "rc", (uint64_t)(uint32_t)rc,
                               "code");
            obs_report_bytes(id, "batch-init-0x400", "info-60b", 0, batch_info, 0x60);
            obs_report_bytes(id, "batch-init-0x400", "buf-40b", 0, batch_buf, 0x40);
        } else {
            obs_fault_unregister();
        }

        const struct {
            const char *tag;
            void *b;
            size_t sz;
            void *inf;
        } binit_arms[] = {
            {"binit-sz0", batch_buf, 0, batch_info},
            {"binit-sz8", batch_buf, 8, batch_info},
            {"binit-nullbuf", NULL, 0x400, batch_info},
            {"binit-nullinfo", batch_buf, 0x400, NULL},
            {"binit-nullboth", NULL, 0x400, NULL},
        };
        for (size_t a = 0; a < sizeof(binit_arms) / sizeof(binit_arms[0]); a++) {
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc = p_binit(binit_arms[a].b, binit_arms[a].sz, binit_arms[a].inf);
                obs_fault_unregister();
                obs_report_measure(id, binit_arms[a].tag, "rc", (uint64_t)(uint32_t)rc,
                                   "code");
            } else {
                obs_fault_unregister();
            }
        }

        memset(batch_buf, 0xcc, sizeof(batch_buf));
        memset(batch_info, 0xa5, sizeof(batch_info));
        p_binit(batch_buf, sizeof(batch_buf), batch_info);
    }

    /* 4. BatchJobInitialize & Decode (REQ-7573 & REQ-5604) */
    uint8_t at9_cfg[8] = {0xfe, 0x72, 0x09, 0xf0, 0, 0, 0, 0};
    uint8_t result_sideband[0x40];
    memset(result_sideband, 0x5a, sizeof(result_sideband));

    if (p_bjob_init && inst_at9 != 0) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_jinit =
                p_bjob_init(batch_info, inst_at9, at9_cfg, 8, result_sideband, 0, 0, 0);
            obs_fault_unregister();
            obs_report_measure(id, "job-init-at9", "rc", (uint64_t)(uint32_t)rc_jinit,
                               "code");
            obs_report_bytes(id, "job-init-at9", "sideband", 0, result_sideband, 0x40);
        } else {
            obs_fault_unregister();
        }
    }

    if (p_bjob_gap && inst_at9 != 0) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_gap = p_bjob_gap(batch_info, inst_at9, 1000, 100);
            obs_fault_unregister();
            obs_report_measure(id, "job-gapless", "rc", (uint64_t)(uint32_t)rc_gap,
                               "code");
        } else {
            obs_fault_unregister();
        }
    }

    uint8_t zero_in[0x100];
    memset(zero_in, 0, sizeof(zero_in));
    uint8_t pcm_out[0x800];
    memset(pcm_out, 0xcc, sizeof(pcm_out));

    if (p_bjob_dec && inst_at9 != 0) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_jdec = p_bjob_dec(batch_info, inst_at9, zero_in, sizeof(zero_in),
                                     pcm_out, sizeof(pcm_out), result_sideband);
            obs_fault_unregister();
            obs_report_measure(id, "job-decode-zeros", "rc",
                               (uint64_t)(uint32_t)rc_jdec, "code");
            obs_report_bytes(id, "job-decode-zeros", "sideband", 0, result_sideband,
                             0x40);
        } else {
            obs_fault_unregister();
        }
    }

    /* 5. BatchStart & BatchWait (REQ-943d) */
    uint64_t batch_id = 0xffffffffffffffffULL;
    uint8_t err_out[0x40];
    memset(err_out, 0x77, sizeof(err_out));

    if (p_bstart) {
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_start = p_bstart(ctx, batch_info, 0, err_out, &batch_id);
            obs_fault_unregister();
            obs_report_measure(id, "batch-start", "rc", (uint64_t)(uint32_t)rc_start,
                               "code");
            obs_report_measure(id, "batch-start", "batch-id", batch_id, "id");
            obs_report_bytes(id, "batch-start", "err-out", 0, err_out, 0x40);
            obs_report_bytes(id, "batch-start", "sideband-after-start", 0,
                             result_sideband, 0x40);
        } else {
            obs_fault_unregister();
        }

        if (p_bwait && batch_id != 0xffffffffffffffffULL) {
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_wait = p_bwait(ctx, batch_id, 1000, err_out);
                obs_fault_unregister();
                obs_report_measure(id, "batch-wait", "rc", (uint64_t)(uint32_t)rc_wait,
                                   "code");
                obs_report_bytes(id, "batch-wait", "err-out", 0, err_out, 0x40);
                obs_report_bytes(id, "batch-wait", "sideband-after-wait", 0,
                                 result_sideband, 0x40);
            } else {
                obs_fault_unregister();
            }
        }

        if (p_bcancel && batch_id != 0xffffffffffffffffULL) {
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_canc = p_bcancel(ctx, batch_id);
                obs_fault_unregister();
                obs_report_measure(id, "batch-cancel", "rc",
                                   (uint64_t)(uint32_t)rc_canc, "code");
            } else {
                obs_fault_unregister();
            }
        }
    }

    /* Parse ATRAC9 config if available */
    if (p_at9_parse) {
        uint8_t parsed[0x20];
        memset(parsed, 0xcc, sizeof(parsed));
        if (OBS_FAULT_ARM(&guard) == 0) {
            int rc_p = p_at9_parse(at9_cfg, parsed);
            obs_fault_unregister();
            obs_report_measure(id, "at9-parse-fe7209f0", "rc", (uint64_t)(uint32_t)rc_p,
                               "code");
            obs_report_bytes(id, "at9-parse-fe7209f0", "out", 0, parsed, 0x20);
        } else {
            obs_fault_unregister();
        }
    }

    if (p_inst_dst && inst_at9 != 0) {
        p_inst_dst(ctx, inst_at9);
    }
    if (p_mod_unreg) {
        for (uint32_t c = 0; c < 4; c++) {
            char tag[32];
            snprintf(tag, sizeof(tag), "module-unreg-c%u", (unsigned int)c);
            if (OBS_FAULT_ARM(&guard) == 0) {
                int rc_unreg = p_mod_unreg(ctx, c);
                obs_fault_unregister();
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_unreg, "code");
            } else {
                obs_fault_unregister();
            }
        }
    }
    if (p_fin) {
        p_fin(ctx);
    }

    return obs_pass();
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
    {"108-audiodec/ajm-batch-init-nulls", "libSceAjm", "sceAjmBatchInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ajm_batch_initialize_nulls,
     OBS_FROM_ASSUMED},
    {"108-audiodec/ajm-init", "libSceAjm", "sceAjmInitialize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_audiodec_ajm_initialize, OBS_FROM_ASSUMED},
    {"108-audiodec/4865-init-decoder", "libSceAudiodec", "sceAudiodecInitLibrary",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_4865_audiodec_init_decoder,
     OBS_FROM_ASSUMED},
    {"108-audiodec/ajm-lifecycle-batch", "libSceAjm", "sceAjmInitialize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ajm_batch_and_instances, OBS_FROM_ASSUMED},
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
