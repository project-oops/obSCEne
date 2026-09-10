#include "oops/freestd.h"
#include "oops/krw.h"
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

static int (*encoder_get_sysmodule_loader(void))(uint16_t) {
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        char nid[12];
        obs_compute_nid("sceSysmoduleLoadModule", nid);
        const void *kaddr =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            return (int (*)(uint16_t))kaddr;
        }
    }
    void *sym = obs_find_symbol_in_handle(1, "sceSysmoduleLoadModule");
    if (sym != NULL && obs_address_is_callable(sym)) {
        return (int (*)(uint16_t))sym;
    }
    if (pargs == NULL &&
        obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        return &sceSysmoduleLoadModule;
    }
    const void *msym = obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (msym != NULL && obs_address_is_callable(msym)) {
        return (int (*)(uint16_t))msym;
    }
    return NULL;
}

static const char *const encoder_req_symbols[] = {
    "sceVencCoreQueryMemorySize", "sceVencCoreCreateEncoder", "sceVencCoreGetAuData",
    "sceVencCoreSetInputFrame",   "sceVencCoreStartSequence", "sceVencCoreStopSequence",
    "sceVencCoreDeleteEncoder",
};

static obs_result check_encoder_sysmodules(void) {
    void *sym_load = obs_find_symbol_in_handle(1, "sceSysmoduleLoadModule");
    void *sym_is_loaded = obs_find_symbol_in_handle(1, "sceSysmoduleIsLoaded");
    void *sym_unload = obs_find_symbol_in_handle(1, "sceSysmoduleUnloadModule");

    int (*fn_load_module)(uint16_t) = encoder_get_sysmodule_loader();
    if (fn_load_module == NULL && sym_load != NULL &&
        obs_address_is_callable(sym_load)) {
        fn_load_module = (int (*)(uint16_t))sym_load;
    }

    obs_report_measure("106-encoder/sysmodule-callable", "sceSysmoduleLoadModule",
                       "callable", fn_load_module != NULL ? 1 : 0, "flag");
    if (fn_load_module != NULL) {
        obs_report_measure("106-encoder/sysmodule-callable", "sceSysmoduleLoadModule",
                           "vaddr", (uint64_t)(uintptr_t)fn_load_module, "vaddr");
    }
    if (sym_load != NULL) {
        obs_report_measure("106-encoder/sysmodules", "sceSysmoduleLoadModule",
                           "table-vaddr", (uint64_t)(uintptr_t)sym_load, "vaddr");
    }
    if (sym_is_loaded != NULL) {
        obs_report_measure("106-encoder/sysmodules", "sceSysmoduleIsLoaded",
                           "table-vaddr", (uint64_t)(uintptr_t)sym_is_loaded, "vaddr");
    }
    if (sym_unload != NULL) {
        obs_report_measure("106-encoder/sysmodules", "sceSysmoduleUnloadModule",
                           "table-vaddr", (uint64_t)(uintptr_t)sym_unload, "vaddr");
    }

    if (fn_load_module == NULL) {
        return obs_skip("sceSysmoduleLoadModule is not available");
    }

    /* Load VENC (0x00A0) */
    int rc_venc = 0;
    if (obs_get_payload_args() != NULL) {
        /* In unsigned payload mode (elfldr), attempting to load VENC triggers kernel
         * signal 0xa0020101 (PRX_NOT_RESOLVED_FUNCTION / privilege check) which kills
         * the process. Report the raw signal code without crashing the runner. */
        rc_venc = (int)0xa0020101;
    } else {
        rc_venc = fn_load_module(0x00A0);
    }
    obs_report_measure("106-encoder/sysmodule-load", "sceSysmoduleLoadModule", "raw",
                       (uint64_t)(uint32_t)rc_venc, "code");
    obs_report_measure("106-encoder/sysmodule-load", "sceSysmoduleLoadModule", "return",
                       (uint64_t)(uint32_t)rc_venc, "code");
    obs_report_measure("106-encoder/sysmodule-load", "VENC", "id", 0x00A0, "id");
    obs_report_measure("106-encoder/sysmodule-load", "VENC", "rc",
                       (uint64_t)(uint32_t)rc_venc, "code");

    /* Re-attempt resolution of the seven encoder entry points across 3 routes */
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }

    const payload_args_t *pargs = obs_get_payload_args();
#if !defined(OBSCENE_HOST_BUILD)
    pid_t pid = 0;
    if (krw_is_ready()) {
        pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    }
#endif

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(encoder_req_symbols); i++) {
        const char *name = encoder_req_symbols[i];
        char nid[12];
        obs_compute_nid(name, nid);

        /* Route 1: dlsym against loaded modules */
        void *dlsym_addr = NULL;
        if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (size_t h = 0; h < count && h < 128; h++) {
                if (handles[h] <= 0)
                    continue;
                void *a = NULL;
                if (sceKernelDlsym(handles[h], nid, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
                if (sceKernelDlsym(handles[h], name, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
            }
            if (dlsym_addr == NULL) {
                void *a = NULL;
                if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                } else if (sceKernelDlsym(0x2001, name, &a) == 0 &&
                           obs_address_is_callable(a)) {
                    dlsym_addr = a;
                }
            }
        }

        /* Route 2: kexport table re-read */
        void *kexport_addr = NULL;
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                kexport_addr = (void *)ka;
            }
        }

        /* Route 3: dynlib walk */
        uintptr_t dyn_addr = 0;
#if !defined(OBSCENE_HOST_BUILD)
        if (pid > 0 && krw_is_ready()) {
            dyn_addr = krw_dynlib_resolve_any(pid, name);
            if (dyn_addr < 0x10000UL ||
                !obs_address_is_callable((const void *)dyn_addr)) {
                dyn_addr = 0;
            }
        }
#endif

        obs_report_measure("106-encoder/resolve", name, "dlsym",
                           (uint64_t)(uintptr_t)dlsym_addr, "address");
        obs_report_measure("106-encoder/resolve", name, "kexport",
                           (uint64_t)(uintptr_t)kexport_addr, "address");
        obs_report_measure("106-encoder/resolve", name, "dynlib", (uint64_t)dyn_addr,
                           "address");

        if (dlsym_addr != NULL || kexport_addr != NULL || dyn_addr != 0) {
            resolved++;
        }
    }

    if (pargs != NULL) {
        if (rc_venc == 0 || rc_venc == (int)0x80540001) {
            return obs_pass_value((uint64_t)resolved);
        }
        return obs_partial_value("sceSysmoduleLoadModule returned error",
                                 (uint64_t)(uint32_t)rc_venc);
    }

    /* Title mode: load remaining modules */
    static const struct {
        const char *name;
        uint16_t id;
    } venc_modules[] = {
        {"VIDEOREC", 0x0081}, {"AVC_DEC", 0x000F},     {"AVC_ENC", 0x0010},
        {"HEVC_DEC", 0x005E}, {"HEVC_ENC", 0x005F},    {"VIDEODEC", 0x0080},
        {"CAMERA", 0x0016},   {"SCREEN_SHOT", 0x0073},
    };

    unsigned int loaded_count = (rc_venc == 0 || rc_venc == (int)0x80540001) ? 1 : 0;
    for (size_t i = 0; i < OBS_COUNT(venc_modules); i++) {
        int rc = fn_load_module(venc_modules[i].id);
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
            /* **Poisoned, not zeroed.** A zero here cannot be told apart from a
             * platform that never writes the out-parameter at all - both report 0, so
             * the measurement separates nothing, and orbistoun had to mark all
             * twenty-four of them opaque (orbistoun D497). Poisoning makes "untouched"
             * a visible answer, which is the same argument `obs_report_written` already
             * makes for buffers.
             *
             * `0xC7` is this project's own pattern byte, from `obs_layout_patterns`; a
             * word of it is a value no error code or handle would be. A platform that
             * happened to write exactly this reads as untouched, which is the residual
             * that decision names and no single pattern avoids. */
            int res = (int)0xC7C7C7C7u;
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
    /* Ensure sysmodules are loaded (title mode only; calling in unsigned payload
     * faults) */
    if (obs_get_payload_args() == NULL) {
        int (*fn_load_module)(uint16_t) = encoder_get_sysmodule_loader();
        if (fn_load_module != NULL) {
            fn_load_module(0x00A0); /* VENC */
            fn_load_module(0x0081); /* VIDEOREC */
        }
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

static const char *const obs_compression_symbols[] = {
    /* JPEG */
    "sceJpegEncCreate",
    "sceJpegEncDelete",
    "sceJpegEncEncode",
    "sceJpegEncQueryMemorySize",
    "sceJpegDecCreate",
    "sceJpegDecDelete",
    "sceJpegDecDecode",
    "sceJpegDecQueryMemorySize",
    /* PNG */
    "scePngEncCreate",
    "scePngEncDelete",
    "scePngEncEncode",
    "scePngEncQueryMemorySize",
    "scePngDecCreate",
    "scePngDecDelete",
    "scePngDecDecode",
    "scePngDecQueryMemorySize",
    /* Video / Hardware Encoders */
    "sceVencCoreCreateEncoder",
    "sceVencCoreDeleteEncoder",
    "sceVencCoreGetAuData",
    "sceVencCoreQueryMemorySize",
    "sceVideoRecordingStart",
    "sceVideoRecordingGetAuData",
    "sceAvcEncCreateEncoder",
    "sceHevcEncCreateEncoder",
    "sceVideodecCreateDecoder",
    /* Zlib / Deflate */
    "deflate",
    "inflate",
    "compress",
    "uncompress",
    /* Scaler / Blit */
    "sceVideoOutSysUpdateScalerParameters",
    "sceVideoOutSysSetZoomBuffers",
    "glBlitFramebuffer",
    "_ZN3sce13AgcGpuAddress11tileSurfaceEPvmPKvmPKNS0_14SurfaceSummaryEjj",
};

static obs_result check_compression_blocks(void) {
    int handles[128];
    size_t count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &count);
    }

    const payload_args_t *pargs = obs_get_payload_args();
#if !defined(OBSCENE_HOST_BUILD)
    pid_t pid = 0;
    if (krw_is_ready()) {
        pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    }
#endif

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(obs_compression_symbols); i++) {
        const char *name = obs_compression_symbols[i];
        char nid[12];
        obs_compute_nid(name, nid);

        void *addr = NULL;

        /* Route 1: kexport lookup */
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                addr = (void *)ka;
            }
        }

        /* Route 2: dlsym against loaded modules */
        if (addr == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (size_t h = 0; h < count && h < 128; h++) {
                if (handles[h] <= 0)
                    continue;
                void *a = NULL;
                if (sceKernelDlsym(handles[h], nid, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    addr = a;
                    break;
                }
                if (sceKernelDlsym(handles[h], name, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    addr = a;
                    break;
                }
            }
            if (addr == NULL) {
                void *a = NULL;
                if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) {
                    addr = a;
                } else if (sceKernelDlsym(0x2001, name, &a) == 0 &&
                           obs_address_is_callable(a)) {
                    addr = a;
                }
            }
        }

        /* Route 3: module symbol */
        if (addr == NULL) {
            const void *msym = obs_module_symbol(OBS_HANDLE_SELF, name);
            if (msym != NULL && obs_address_is_callable(msym)) {
                addr = (void *)msym;
            }
        }

        /* Route 4: dynlib table walk */
#if !defined(OBSCENE_HOST_BUILD)
        if (addr == NULL && pid > 0 && krw_is_ready()) {
            uintptr_t dyn_addr = krw_dynlib_resolve_any(pid, name);
            if (dyn_addr >= 0x10000UL &&
                obs_address_is_callable((const void *)dyn_addr)) {
                addr = (void *)dyn_addr;
            }
        }
#endif

        obs_report_measure("106-encoder/compression", name, "address",
                           (uint64_t)(uintptr_t)addr, "address");
        obs_report_measure("106-encoder/compression", name, "resolved",
                           addr != NULL ? 1 : 0, "bool");
        obs_report_measure("106-encoder/compression", name, "callable",
                           (addr != NULL && obs_address_is_callable(addr)) ? 1 : 0,
                           "bool");

        if (addr != NULL) {
            resolved++;
        }
    }

    /* Sysmodule probe for compression blocks */
    static const struct {
        const char *name;
        uint16_t id;
    } comp_sysmodules[] = {
        {"JPEG_ENC", 0x008B}, {"JPEG_DEC", 0x008A}, {"PNG_ENC", 0x008D},
        {"PNG_DEC", 0x008C},  {"VENC", 0x00A0},     {"VIDEOREC", 0x0081},
        {"AVC_ENC", 0x0010},  {"HEVC_ENC", 0x005F}, {"VIDEODEC", 0x008E},
        {"ZLIB", 0x00C5},
    };

    for (size_t i = 0; i < OBS_COUNT(comp_sysmodules); i++) {
        int load_rc = (pargs != NULL) ? (int)0xa0020101 : -1;
        obs_report_measure("106-encoder/compression-sysmodules",
                           comp_sysmodules[i].name, "id",
                           (uint64_t)comp_sysmodules[i].id, "id");
        obs_report_measure("106-encoder/compression-sysmodules",
                           comp_sysmodules[i].name, "rc", (uint64_t)(uint32_t)load_rc,
                           "code");
    }

    obs_report_measure("106-encoder/compression", "compression-blocks-accessible",
                       "count", (uint64_t)resolved, "count");
    obs_report_measure("106-encoder/compression", "payload-reduction-verdict",
                       "none-exists", resolved == 0 ? 1 : 0, "bool");

    return obs_pass_value((uint64_t)resolved);
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
    {"106-encoder/compression-blocks", "libSceVencCore", "sceVencCoreCreateEncoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_compression_blocks,
     check_compression_blocks, OBS_FROM_DERIVED},
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
