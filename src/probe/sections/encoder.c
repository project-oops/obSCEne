#include "common/freestd.h"
#include "common/krw.h"
#include "obscene/display.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

extern OBS_WEAK const char sceVencCoreCreateEncoder;
extern OBS_WEAK const char sceVencCoreGetAuData;
extern OBS_WEAK const char sceVencCoreQueryMemorySize;

static const char *const obs_venc_symbols[] = {
    "sceVencCoreCreateEncoder",     "sceVencCoreDeleteEncoder",
    "sceVencCoreGetAuData",         "sceVencCoreGetPicParams",
    "sceVencCoreMapTargetMemory",   "sceVencCoreMapTargetMemoryByPid",
    "sceVencCoreQueryHeader",       "sceVencCoreQueryMemorySize",
    "sceVencCoreQueryMemorySizeEx", "sceVencCoreQueryPreset",
    "sceVencCoreQueryPresetEx",     "sceVencCoreSetBitRate",
    "sceVencCoreSetInputFrame",     "sceVencCoreSetInputFrameByPid",
    "sceVencCoreSetInvalidFrame",   "sceVencCoreSetPasteImage",
    "sceVencCoreSetPicParams",      "sceVencCoreSetPictureType",
    "sceVencCoreSetPrivacyGuard",   "sceVencCoreStartSequence",
    "sceVencCoreStopSequence",      "sceVencCoreSyncEncode",
    "sceVencCoreUnmapTargetMemory", "sceVencCoreUnmapTargetMemoryByPid",
};

static const char *const obs_video_recording_symbols[] = {
    "sceVideoRecordingOpen",          "sceVideoRecordingClose",
    "sceVideoRecordingGetStatus",     "sceVideoRecordingQueryMemorySize",
    "sceVideoRecordingSetStatus",     "sceVideoRecordingStart",
    "sceVideoRecordingStop",          "sceVideoRecordingGetAuData",
    "sceVideoRecordingSetInputFrame",
};

static void *obs_find_symbol_in_handle(int handle, const char *name) {
    if (name == NULL)
        return NULL;
    char nid[12];
    obs_compute_nid(name, nid);

    /* 1. Try kernel export table */
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *kaddr =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            return (void *)kaddr;
        }
    }

    /* 2. Try sceKernelDlsym */
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *addr = NULL;
        if (handle > 0 && sceKernelDlsym(handle, nid, &addr) == 0 &&
            obs_address_is_callable(addr)) {
            return addr;
        }
        if (handle > 0 && sceKernelDlsym(handle, name, &addr) == 0 &&
            obs_address_is_callable(addr)) {
            return addr;
        }
    }
    return NULL;
}

static obs_result check_encoder_sysmodules(void) {
    if (!obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        return obs_skip("sceSysmoduleLoadModule is not available");
    }

    static const struct {
        const char *name;
        uint16_t id;
    } venc_modules[] = {
        {"VENC", 0x00A0},     {"VIDEOREC", 0x0081}, {"AVC_DEC", 0x000F},
        {"AVC_ENC", 0x0010},  {"HEVC_DEC", 0x005E}, {"HEVC_ENC", 0x005F},
        {"VIDEODEC", 0x0080}, {"CAMERA", 0x0016},   {"SCREEN_SHOT", 0x0073},
    };

    unsigned int loaded_count = 0;
    for (size_t i = 0; i < OBS_COUNT(venc_modules); i++) {
        int rc = sceSysmoduleLoadModule(venc_modules[i].id);
        obs_report_measure("106-encoder/sysmodule-load", venc_modules[i].name, "id",
                           (uint64_t)venc_modules[i].id, "id");
        obs_report_measure("106-encoder/sysmodule-load", venc_modules[i].name, "rc",
                           (uint64_t)(uint32_t)rc, "code");
        if (rc == 0 || rc == (int)0x80540001) {
            loaded_count++;
        }
    }

    return obs_pass_value((uint64_t)loaded_count);
}

static obs_result check_encoder_module_load(void) {
    static const char *const search_paths[] = {
        "/system/common/lib/libSceVencCore.sprx",
        "/system/priv/lib/libSceVencCore.sprx",
        "/system/sys/lib/libSceVencCore.sprx",
        "/system/lib/libSceVencCore.sprx",
        "/system_ex/common/lib/libSceVencCore.sprx",
        "/system_ex/priv/lib/libSceVencCore.sprx",
        "/system_ex/sys/lib/libSceVencCore.sprx",
        "/system_ex/lib/libSceVencCore.sprx",
        "/system_data/priv/lib/libSceVencCore.sprx",
        "/system_data/sys/lib/libSceVencCore.sprx",
        "/system_data/lib/libSceVencCore.sprx",
        "/RuC3TlgXmY/common/lib/libSceVencCore.sprx",
        "/RuC3TlgXmY/priv/lib/libSceVencCore.sprx",
        "/system/common/lib/libSceVideoRecording.sprx",
        "/system/priv/lib/libSceVideoRecording.sprx",
        "/system_ex/common/lib/libSceVideoRecording.sprx",
        "/system_ex/priv/lib/libSceVideoRecording.sprx",
        "/RuC3TlgXmY/common/lib/libSceVideoRecording.sprx",
        "/system/common/lib/libSceAvcEnc.sprx",
        "/system/priv/lib/libSceAvcEnc.sprx",
        "/system/common/lib/libSceHevcEnc.sprx",
        "/system/priv/lib/libSceHevcEnc.sprx",
        "/system/common/lib/libSceVideodec.sprx",
        "/system/priv/lib/libSceVideodec.sprx",
    };

    int loaded_handle = -1;
    for (size_t i = 0; i < OBS_COUNT(search_paths); i++) {
        if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            int res = 0;
            int h = sceKernelLoadStartModule(search_paths[i], 0, (void *)0, 0,
                                             (void *)0, &res);
            obs_report_measure("106-encoder/path-probe", search_paths[i], "handle",
                               (uint64_t)(uint32_t)h, "handle");
            obs_report_measure("106-encoder/path-probe", search_paths[i], "res",
                               (uint64_t)(uint32_t)res, "code");
            if (h > 0 && loaded_handle <= 0) {
                loaded_handle = h;
            }
        }
    }

    /* Get loaded module handles */
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }
    obs_report_measure("106-encoder/module-list", "total", "count", (uint64_t)count,
                       "modules");

    for (size_t i = 0; i < count && i < 128; i++) {
        if (handles[i] <= 0)
            continue;
        obs_report_measure("106-encoder/module-handle", "loaded", "handle",
                           (uint64_t)(uint32_t)handles[i], "handle");
    }

    if (loaded_handle > 0) {
        return obs_pass_value((uint64_t)loaded_handle);
    }
    if (count > 0) {
        return obs_partial_value("paths returned error codes; module list read",
                                 (uint64_t)count);
    }
    return obs_skip("no module handles retrieved");
}

static obs_result check_encoder_symbol_census(void) {
    /* Ensure sysmodules are loaded */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        sceSysmoduleLoadModule(0x00A0); /* VENC */
        sceSysmoduleLoadModule(0x0081); /* VIDEOREC */
    }

    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(obs_venc_symbols); i++) {
        const char *name = obs_venc_symbols[i];
        void *addr = NULL;
        int found_handle = -1;

        /* Try kernel export table first */
        addr = obs_find_symbol_in_handle(-1, name);
        if (addr != NULL) {
            found_handle = 0;
        }

        /* Try all loaded module handles */
        if (addr == NULL) {
            for (size_t h = 0; h < count && h < 128; h++) {
                if (handles[h] <= 0)
                    continue;
                addr = obs_find_symbol_in_handle(handles[h], name);
                if (addr != NULL) {
                    found_handle = handles[h];
                    break;
                }
            }
        }

        /* Try libkernel handle 0x2001 */
        if (addr == NULL) {
            addr = obs_find_symbol_in_handle(0x2001, name);
            if (addr != NULL) {
                found_handle = 0x2001;
            }
        }

        if (addr != NULL) {
            resolved++;
            obs_report_measure("106-encoder/symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            obs_report_measure("106-encoder/symbols", name, "handle",
                               (uint64_t)(uint32_t)found_handle, "handle");
            if (obs_address_is_callable(addr)) {
                obs_report_buffer("106-encoder/prologue", name, "code",
                                  (const unsigned char *)addr, 16);
            }
        } else {
            obs_report_measure("106-encoder/symbols", name, "unresolved", 0, "status");
        }
    }

    /* Probe VideoRecording symbols */
    for (size_t i = 0; i < OBS_COUNT(obs_video_recording_symbols); i++) {
        const char *name = obs_video_recording_symbols[i];
        void *addr = NULL;
        int found_handle = -1;

        addr = obs_find_symbol_in_handle(-1, name);
        if (addr != NULL) {
            found_handle = 0;
        }

        if (addr == NULL) {
            for (size_t h = 0; h < count && h < 128; h++) {
                if (handles[h] <= 0)
                    continue;
                addr = obs_find_symbol_in_handle(handles[h], name);
                if (addr != NULL) {
                    found_handle = handles[h];
                    break;
                }
            }
        }

        if (addr != NULL) {
            obs_report_measure("106-encoder/rec-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            obs_report_measure("106-encoder/rec-symbols", name, "handle",
                               (uint64_t)(uint32_t)found_handle, "handle");
            if (obs_address_is_callable(addr)) {
                obs_report_buffer("106-encoder/rec-prologue", name, "code",
                                  (const unsigned char *)addr, 16);
            }
        } else {
            obs_report_measure("106-encoder/rec-symbols", name, "unresolved", 0,
                               "status");
        }
    }

    if (resolved == OBS_COUNT(obs_venc_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some encoder symbols resolved", (uint64_t)resolved);
    }
    return obs_skip("no libSceVencCore symbols resolved");
}

static obs_result check_encoder_create_present(void) {
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }
    void *addr = obs_find_symbol_in_handle(-1, "sceVencCoreCreateEncoder");
    if (addr == NULL) {
        for (size_t h = 0; h < count && h < 128; h++) {
            if (handles[h] <= 0)
                continue;
            addr = obs_find_symbol_in_handle(handles[h], "sceVencCoreCreateEncoder");
            if (addr != NULL)
                break;
        }
    }
    if (addr == NULL) {
        addr = obs_find_symbol_in_handle(0x2001, "sceVencCoreCreateEncoder");
    }
    if (addr != NULL) {
        obs_report_measure("106-encoder/create-present", "sceVencCoreCreateEncoder",
                           "vaddr", (uint64_t)(uintptr_t)addr, "offset");
        return obs_pass();
    }
    return obs_skip("sceVencCoreCreateEncoder is not resolved");
}

static obs_result check_encoder_getaudata_present(void) {
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }
    void *addr = obs_find_symbol_in_handle(-1, "sceVencCoreGetAuData");
    if (addr == NULL) {
        for (size_t h = 0; h < count && h < 128; h++) {
            if (handles[h] <= 0)
                continue;
            addr = obs_find_symbol_in_handle(handles[h], "sceVencCoreGetAuData");
            if (addr != NULL)
                break;
        }
    }
    if (addr == NULL) {
        addr = obs_find_symbol_in_handle(0x2001, "sceVencCoreGetAuData");
    }
    if (addr != NULL) {
        obs_report_measure("106-encoder/getaudata-present", "sceVencCoreGetAuData",
                           "vaddr", (uint64_t)(uintptr_t)addr, "offset");
        return obs_pass();
    }
    return obs_skip("sceVencCoreGetAuData is not resolved");
}

static obs_result check_encoder_query_present(void) {
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }
    void *addr = obs_find_symbol_in_handle(-1, "sceVencCoreQueryMemorySize");
    if (addr == NULL) {
        for (size_t h = 0; h < count && h < 128; h++) {
            if (handles[h] <= 0)
                continue;
            addr = obs_find_symbol_in_handle(handles[h], "sceVencCoreQueryMemorySize");
            if (addr != NULL)
                break;
        }
    }
    if (addr == NULL) {
        addr = obs_find_symbol_in_handle(0x2001, "sceVencCoreQueryMemorySize");
    }
    if (addr != NULL) {
        obs_report_measure("106-encoder/query-present", "sceVencCoreQueryMemorySize",
                           "vaddr", (uint64_t)(uintptr_t)addr, "offset");
        return obs_pass();
    }
    return obs_skip("sceVencCoreQueryMemorySize is not resolved");
}

static obs_result check_related_video_modules(void) {
    static const char *const related_libs[] = {
        "libSceVideoRecording",
        "libSceMediaFrameworkInterface",
        "libSceVideoCoreServerInterface",
        "libSceAvcEnc",
        "libSceHevcEnc",
        "libSceVideodec",
    };
    unsigned int found = 0;
    for (size_t i = 0; i < OBS_COUNT(related_libs); i++) {
        int h = obs_module_open(related_libs[i]);
        if (h > 0) {
            found++;
            obs_report_measure("106-encoder/related-libs", related_libs[i], "handle",
                               (uint64_t)h, "handle");
        } else {
            obs_report_measure("106-encoder/related-libs", related_libs[i], "absent", 0,
                               "status");
        }
    }
    return obs_pass_value((uint64_t)found);
}

static const obs_check encoder_checks[] = {
    {"106-encoder/sysmodules", "libSceSysmodule", "sceSysmoduleLoadModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_sysmodules,
     check_encoder_sysmodules, OBS_FROM_DERIVED},
    {"106-encoder/module-load", "libSceVencCore", "sceSysmoduleLoadModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_module_load,
     check_encoder_module_load, OBS_FROM_DERIVED},
    {"106-encoder/symbols-census", "libSceVencCore", "sceVencCoreCreateEncoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_symbol_census,
     check_encoder_symbol_census, OBS_FROM_DERIVED},
    {"106-encoder/create-present", "libSceVencCore", "sceVencCoreCreateEncoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_create_present,
     check_encoder_create_present, OBS_FROM_ASSUMED},
    {"106-encoder/getaudata-present", "libSceVencCore", "sceVencCoreGetAuData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_getaudata_present,
     check_encoder_getaudata_present, OBS_FROM_ASSUMED},
    {"106-encoder/query-present", "libSceVencCore", "sceVencCoreQueryMemorySize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_encoder_query_present,
     check_encoder_query_present, OBS_FROM_ASSUMED},
    {"106-encoder/related-libs", "libSceVideoRecording", "sceVideoRecordingGetStatus",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_related_video_modules,
     check_related_video_modules, OBS_FROM_DERIVED},
};

const obs_section obs_section_encoder = {
    "106-encoder",
    "The hardware video encoder, reached",
    "Whether libSceVencCore and related video encoding libraries load at runtime, and "
    "whether "
    "their entry points resolve and can be hooked.",
    encoder_checks,
    OBS_COUNT(encoder_checks),
};
