/*
 * REQ-20261007T1045Z-cn01: Commercial title import surface census.
 *
 * Tier 1 functions imported by retail titles (PPSA02664, PPSA03416, PPSA21564,
 * PPSA25872, PPSA04263, PPSA28061).
 *
 * Each function is probed with the standard harness:
 * (1) Six arguments, each a separate 0x200-byte buffer filled with a distinct byte (0xa1..0xa6),
 *     and the same call again with every argument 0. For each call: rc, fault signal if any,
 *     and for each buffer whether it changed, with changed bytes.
 * (2) Where a function returns a pointer (> 0x10000), 0x80 bytes there.
 * (3) Where (1) shows the function writes an out-parameter in arg0, a second call reusing
 *     what the first wrote.
 */

#include <stdint.h>

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "oops/freestd.h"
#include "oops/krw.h"

/* clang-format off */
OBS_WEAK uint64_t census_import_0(void *, void *, void *, void *, void *, void *) __asm__("$bBfz7kMF2Ho");
OBS_WEAK uint64_t census_import_1(void *, void *, void *, void *, void *, void *) __asm__("$ApkYaHb8Sek");
OBS_WEAK uint64_t census_named_1(void *, void *, void *, void *, void *, void *) __asm__("sceKernelAprGetFileStat");
OBS_WEAK uint64_t census_import_2(void *, void *, void *, void *, void *, void *) __asm__("$i3HWvW35jao");
OBS_WEAK uint64_t census_named_2(void *, void *, void *, void *, void *, void *) __asm__("sceKernelAprResolveFilepathsWithPrefixToIds");
OBS_WEAK uint64_t census_import_3(void *, void *, void *, void *, void *, void *) __asm__("$w5fcCG+t31g");
OBS_WEAK uint64_t census_named_3(void *, void *, void *, void *, void *, void *) __asm__("sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes");
OBS_WEAK uint64_t census_import_4(void *, void *, void *, void *, void *, void *) __asm__("$hwVSPCmp5tM");
OBS_WEAK uint64_t census_named_4(void *, void *, void *, void *, void *, void *) __asm__("sceKernelCheckedReleaseDirectMemory");
OBS_WEAK uint64_t census_import_5(void *, void *, void *, void *, void *, void *) __asm__("$4oXYe9Xmk0Q");
OBS_WEAK uint64_t census_named_5(void *, void *, void *, void *, void *, void *) __asm__("sceKernelGetGPI");
OBS_WEAK uint64_t census_import_6(void *, void *, void *, void *, void *, void *) __asm__("$mPpPxv5CZt4");
OBS_WEAK uint64_t census_named_6(void *, void *, void *, void *, void *, void *) __asm__("sceSystemServiceGetHdrToneMapLuminance");
OBS_WEAK uint64_t census_import_7(void *, void *, void *, void *, void *, void *) __asm__("$rPo6tV8D9bM");
OBS_WEAK uint64_t census_named_7(void *, void *, void *, void *, void *, void *) __asm__("sceSystemServiceGetStatus");
OBS_WEAK uint64_t census_import_8(void *, void *, void *, void *, void *, void *) __asm__("$woNpu+45RLk");
OBS_WEAK uint64_t census_named_8(void *, void *, void *, void *, void *, void *) __asm__("sceUserServiceGetAgeLevel");
OBS_WEAK uint64_t census_import_9(void *, void *, void *, void *, void *, void *) __asm__("$yH17Q6NWtVg");
OBS_WEAK uint64_t census_named_9(void *, void *, void *, void *, void *, void *) __asm__("sceUserServiceGetEvent");
OBS_WEAK uint64_t census_import_13(void *, void *, void *, void *, void *, void *) __asm__("$n3kSX62fgNo");
OBS_WEAK uint64_t census_import_14(void *, void *, void *, void *, void *, void *) __asm__("$Nv8c-Kb+DUM");
OBS_WEAK uint64_t census_named_14(void *, void *, void *, void *, void *, void *) __asm__("sceVideoOutIsOutputSupported");
OBS_WEAK uint64_t census_import_15(void *, void *, void *, void *, void *, void *) __asm__("$QwOO7vegnV8");
OBS_WEAK uint64_t census_named_15(void *, void *, void *, void *, void *, void *) __asm__("sceSaveDataGetSaveDataMemory2");
OBS_WEAK uint64_t census_import_16(void *, void *, void *, void *, void *, void *) __asm__("$F0ZXt5q0ZTA");
OBS_WEAK uint64_t census_named_16(void *, void *, void *, void *, void *, void *) __asm__("sceAgcDriverGetDefaultOwner");
OBS_WEAK uint64_t census_import_17(void *, void *, void *, void *, void *, void *) __asm__("$uJziRsODk1c");
OBS_WEAK uint64_t census_named_17(void *, void *, void *, void *, void *, void *) __asm__("sceAgcDriverGetResourceRegistrationMaxNameLength");
OBS_WEAK uint64_t census_import_18(void *, void *, void *, void *, void *, void *) __asm__("$XlNp7jzGiPo");
OBS_WEAK uint64_t census_named_18(void *, void *, void *, void *, void *, void *) __asm__("sceAgcDriverSetTFRing");
OBS_WEAK uint64_t census_import_19(void *, void *, void *, void *, void *, void *) __asm__("$fd5Bp5tGTgo");
OBS_WEAK uint64_t census_import_20(void *, void *, void *, void *, void *, void *) __asm__("$HV4j+E0MBHE");
OBS_WEAK uint64_t census_import_21(void *, void *, void *, void *, void *, void *) __asm__("$dolOmWH+huQ");
OBS_WEAK uint64_t census_import_23(void *, void *, void *, void *, void *, void *) __asm__("$wkQR9+xTFKY");
OBS_WEAK uint64_t census_import_24(void *, void *, void *, void *, void *, void *) __asm__("$Q07J7XpvhrU");
OBS_WEAK uint64_t census_import_27(void *, void *, void *, void *, void *, void *) __asm__("$n590hj5Oe-k");
OBS_WEAK uint64_t census_named_27(void *, void *, void *, void *, void *, void *) __asm__("sceFontCreateLibraryWithEdition");
OBS_WEAK uint64_t census_import_28(void *, void *, void *, void *, void *, void *) __asm__("$oM+XCzVG3oM");
OBS_WEAK uint64_t census_named_28(void *, void *, void *, void *, void *, void *) __asm__("sceFontSelectLibraryFt");
/* clang-format on */

static const void *census_resolve_symbol(const char *library, const char *sym_or_nid,
                                         const char *name, const void *static_fn,
                                         const void *named_fn) {
    if (static_fn != NULL && obs_address_is_callable(static_fn)) {
        return static_fn;
    }
    if (named_fn != NULL && obs_address_is_callable(named_fn)) {
        return named_fn;
    }

    int h = obs_module_open(library);
    if (h < 0) {
        char sprx[64];
        oops_snprintf(sprx, sizeof(sprx), "%s.sprx", library);
        h = obs_module_open(sprx);
    }

    char bare_nid[32] = {0};
    char dollar_nid[32] = {0};
    char suffixed_ca[48] = {0};
    char dollar_suffixed_ca[48] = {0};
    char suffixed_cb[48] = {0};
    char suffixed_cc[48] = {0};
    char suffixed_ab[48] = {0};

    const char *cand[12];
    size_t cand_count = 0;

    if (sym_or_nid != NULL && sym_or_nid[0] != '\0') {
        const char *nid_raw = sym_or_nid;
        if (nid_raw[0] == '$') {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "%s", nid_raw);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid_raw + 1);
        } else {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "$%s", nid_raw);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid_raw);
        }
        oops_snprintf(suffixed_ca, sizeof(suffixed_ca), "%s#C#A", bare_nid);
        oops_snprintf(dollar_suffixed_ca, sizeof(dollar_suffixed_ca), "$%s#C#A", bare_nid);
        oops_snprintf(suffixed_cb, sizeof(suffixed_cb), "%s#C#B", bare_nid);
        oops_snprintf(suffixed_cc, sizeof(suffixed_cc), "%s#B#C", bare_nid);
        oops_snprintf(suffixed_ab, sizeof(suffixed_ab), "%s#A#B", bare_nid);

        cand[cand_count++] = dollar_nid;
        cand[cand_count++] = bare_nid;
        cand[cand_count++] = suffixed_ca;
        cand[cand_count++] = dollar_suffixed_ca;
        cand[cand_count++] = suffixed_cb;
        cand[cand_count++] = suffixed_cc;
        cand[cand_count++] = suffixed_ab;
    }
    if (name != NULL && name[0] != '\0' && obs_strcmp(name, "<unknown>") != 0 && name[0] != '0') {
        cand[cand_count++] = name;
    }

    int try_handles[4];
    size_t try_count = 0;
    if (h >= 0) {
        try_handles[try_count++] = h;
    }
    try_handles[try_count++] = 1;
    try_handles[try_count++] = OBS_HANDLE_SELF;
    try_handles[try_count++] = 0x2001;

    for (size_t t = 0; t < try_count; t++) {
        for (size_t c = 0; c < cand_count; c++) {
            const void *sym = obs_module_symbol(try_handles[t], cand[c]);
            if (sym != NULL && obs_address_is_callable(sym)) {
                return sym;
            }
        }
    }

#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        for (size_t c = 0; c < cand_count; c++) {
            uintptr_t kaddr = krw_dynlib_resolve_any(pid, cand[c]);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (const void *)kaddr;
            }
        }
    }
#endif

    return NULL;
}

static obs_result probe_census_symbol(const char *check_name, const char *library,
                                      const char *sym_or_nid, const char *name,
                                      const void *static_fn, const void *named_fn) {
    const void *fn = census_resolve_symbol(library, sym_or_nid, name, static_fn, named_fn);
    int callable = (fn != NULL && obs_address_is_callable(fn));
    obs_report_measure(check_name, "resolve", "callable", (uint64_t)callable, "bool");
    if (!callable) {
        return obs_skip("symbol not resolved");
    }

    uint8_t buf[6][0x200];
    uint8_t buf_orig[6][0x200];
    obs_jmp_buf guard;

    /* (1) Six arguments, each a separate 0x200-byte buffer filled with distinct byte (0xa1..0xa6) */
    for (int i = 0; i < 6; i++) {
        memset(buf[i], 0xa1 + i, 0x200);
        memcpy(buf_orig[i], buf[i], 0x200);
    }

    uint64_t rc_buf = 0;
    int sig_buf = OBS_FAULT_ARM(&guard);
    if (sig_buf == 0) {
        typedef uint64_t (*generic_fn6)(void *, void *, void *, void *, void *, void *);
        rc_buf = ((generic_fn6)fn)(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    if (sig_buf != 0) {
        obs_report_measure(check_name, "call-buffers", "fault", (uint64_t)sig_buf, "signal");
    } else {
        obs_report_measure(check_name, "call-buffers", "rc", rc_buf, "code");
        if (rc_buf >= 0x10000ULL && rc_buf < 0x00007fffffffffffULL) {
            uint8_t ptr_dump[0x80];
            int sig_read = OBS_FAULT_ARM(&guard);
            if (sig_read == 0) {
                memcpy(ptr_dump, (const void *)rc_buf, 0x80);
                obs_fault_unregister();
                obs_report_bytes(check_name, "call-buffers", "return-ptr-80b", 0, ptr_dump,
                                 0x80);
            } else {
                obs_fault_unregister();
            }
        }
    }

    /* Check whether each buffer changed, and report changed bytes */
    for (int i = 0; i < 6; i++) {
        char arg_name[16];
        oops_snprintf(arg_name, sizeof(arg_name), "arg%d", i);
        int changed = (memcmp(buf[i], buf_orig[i], 0x200) != 0);
        obs_report_measure(check_name, arg_name, "changed", (uint64_t)changed, "bool");
        if (changed) {
            obs_report_written(check_name, "census", arg_name, buf_orig[i], buf[i], 0x200);
            for (unsigned int off = 0; off < 0x200; off += 64) {
                if (memcmp(buf[i] + off, buf_orig[i] + off, 64) != 0) {
                    obs_report_bytes(check_name, arg_name, "bytes", off, buf[i] + off, 64);
                }
            }
        }
    }

    /* (1b) Same call again with every argument 0 */
    uint64_t rc_zero = 0;
    int sig_zero = OBS_FAULT_ARM(&guard);
    if (sig_zero == 0) {
        typedef uint64_t (*generic_fn6_zero)(uint64_t, uint64_t, uint64_t, uint64_t,
                                             uint64_t, uint64_t);
        rc_zero = ((generic_fn6_zero)fn)(0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    if (sig_zero != 0) {
        obs_report_measure(check_name, "call-zeros", "fault", (uint64_t)sig_zero, "signal");
    } else {
        obs_report_measure(check_name, "call-zeros", "rc", rc_zero, "code");
        if (rc_zero >= 0x10000ULL && rc_zero < 0x00007fffffffffffULL) {
            uint8_t ptr_dump[0x80];
            int sig_read = OBS_FAULT_ARM(&guard);
            if (sig_read == 0) {
                memcpy(ptr_dump, (const void *)rc_zero, 0x80);
                obs_fault_unregister();
                obs_report_bytes(check_name, "call-zeros", "return-ptr-80b", 0, ptr_dump,
                                 0x80);
            } else {
                obs_fault_unregister();
            }
        }
    }

    /* (3) Where (1) shows function writes an out-parameter in buf[0], second call reusing what it wrote */
    if (memcmp(buf[0], buf_orig[0], 0x200) != 0) {
        uint64_t out_val = *(const uint64_t *)(const void *)buf[0];
        uint64_t rc_reuse = 0;
        int sig_reuse = OBS_FAULT_ARM(&guard);
        if (sig_reuse == 0) {
            typedef uint64_t (*generic_fn6_reuse)(uint64_t, void *, void *, void *,
                                                  void *, void *);
            rc_reuse = ((generic_fn6_reuse)fn)(out_val, buf[1], buf[2], buf[3], buf[4],
                                               buf[5]);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        if (sig_reuse != 0) {
            obs_report_measure(check_name, "call-reuse", "fault", (uint64_t)sig_reuse,
                               "signal");
        } else {
            obs_report_measure(check_name, "call-reuse", "rc", rc_reuse, "code");
        }
    }

    return obs_pass();
}

static obs_result check_census_0(void) {
    return probe_census_symbol("200-census/libkernel/0x7ad80543eef3176c", "libkernel", "$bBfz7kMF2Ho",
                               "0x7ad80543eef3176c", (const void *)&census_import_0, NULL);
}

static obs_result check_census_1(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelAprGetFileStat", "libkernel", "$ApkYaHb8Sek",
                               "sceKernelAprGetFileStat", (const void *)&census_import_1, (const void *)&census_named_1);
}

static obs_result check_census_2(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIds", "libkernel", "$i3HWvW35jao",
                               "sceKernelAprResolveFilepathsWithPrefixToIds", (const void *)&census_import_2, (const void *)&census_named_2);
}

static obs_result check_census_3(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes", "libkernel", "$w5fcCG+t31g",
                               "sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes", (const void *)&census_import_3, (const void *)&census_named_3);
}

static obs_result check_census_4(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelCheckedReleaseDirectMemory", "libkernel", "$hwVSPCmp5tM",
                               "sceKernelCheckedReleaseDirectMemory", (const void *)&census_import_4, (const void *)&census_named_4);
}

static obs_result check_census_5(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelGetGPI", "libkernel", "$4oXYe9Xmk0Q",
                               "sceKernelGetGPI", (const void *)&census_import_5, (const void *)&census_named_5);
}

static obs_result check_census_6(void) {
    return probe_census_symbol("200-census/libSceSystemService/sceSystemServiceGetHdrToneMapLuminance", "libSceSystemService", "$mPpPxv5CZt4",
                               "sceSystemServiceGetHdrToneMapLuminance", (const void *)&census_import_6, (const void *)&census_named_6);
}

static obs_result check_census_7(void) {
    return probe_census_symbol("200-census/libSceSystemService/sceSystemServiceGetStatus", "libSceSystemService", "$rPo6tV8D9bM",
                               "sceSystemServiceGetStatus", (const void *)&census_import_7, (const void *)&census_named_7);
}

static obs_result check_census_8(void) {
    return probe_census_symbol("200-census/libSceUserService/sceUserServiceGetAgeLevel", "libSceUserService", "$woNpu+45RLk",
                               "sceUserServiceGetAgeLevel", (const void *)&census_import_8, (const void *)&census_named_8);
}

static obs_result check_census_9(void) {
    return probe_census_symbol("200-census/libSceUserService/sceUserServiceGetEvent", "libSceUserService", "$yH17Q6NWtVg",
                               "sceUserServiceGetEvent", (const void *)&census_import_9, (const void *)&census_named_9);
}

static obs_result check_census_10(void) {
    return probe_census_symbol("200-census/libSceNpManager/sceNpGetAccountCountryA", "libSceNpManager", "$JT+t00a3TxA",
                               "sceNpGetAccountCountryA", NULL, NULL);
}

static obs_result check_census_11(void) {
    return probe_census_symbol("200-census/libSceAudioOut2/sceAudioOut2UserDestroy", "libSceAudioOut2", "$IaZXJ9M79uo",
                               "sceAudioOut2UserDestroy", NULL, NULL);
}

static obs_result check_census_12(void) {
    return probe_census_symbol("200-census/libSceIme/sceImeUpdate", "libSceIme", "$-4GCfYdNF1s",
                               "sceImeUpdate", NULL, NULL);
}

static obs_result check_census_13(void) {
    return probe_census_symbol("200-census/libScePad/0xda809fad5f12799f", "libScePad", "$n3kSX62fgNo",
                               "0xda809fad5f12799f", (const void *)&census_import_13, NULL);
}

static obs_result check_census_14(void) {
    return probe_census_symbol("200-census/libSceVideoOut/sceVideoOutIsOutputSupported", "libSceVideoOut", "$Nv8c-Kb+DUM",
                               "sceVideoOutIsOutputSupported", (const void *)&census_import_14, (const void *)&census_named_14);
}

static obs_result check_census_15(void) {
    return probe_census_symbol("200-census/libSceSaveData_native/sceSaveDataGetSaveDataMemory2", "libSceSaveData_native", "$QwOO7vegnV8",
                               "sceSaveDataGetSaveDataMemory2", (const void *)&census_import_15, (const void *)&census_named_15);
}

static obs_result check_census_16(void) {
    return probe_census_symbol("200-census/libSceAgcDriver/sceAgcDriverGetDefaultOwner", "libSceAgcDriver", "$F0ZXt5q0ZTA",
                               "sceAgcDriverGetDefaultOwner", (const void *)&census_import_16, (const void *)&census_named_16);
}

static obs_result check_census_17(void) {
    return probe_census_symbol("200-census/libSceAgcDriver/sceAgcDriverGetResourceRegistrationMaxNameLength", "libSceAgcDriver", "$uJziRsODk1c",
                               "sceAgcDriverGetResourceRegistrationMaxNameLength", (const void *)&census_import_17, (const void *)&census_named_17);
}

static obs_result check_census_18(void) {
    return probe_census_symbol("200-census/libSceAgcDriver/sceAgcDriverSetTFRing", "libSceAgcDriver", "$XlNp7jzGiPo",
                               "sceAgcDriverSetTFRing", (const void *)&census_import_18, (const void *)&census_named_18);
}

static obs_result check_census_19(void) {
    return probe_census_symbol("200-census/libSceAgc/0x0a4e469ba741de7d", "libSceAgc", "$fd5Bp5tGTgo",
                               "0x0a4e469ba741de7d", (const void *)&census_import_19, NULL);
}

static obs_result check_census_20(void) {
    return probe_census_symbol("200-census/libSceAgc/0x71040c4df8235e1d", "libSceAgc", "$HV4j+E0MBHE",
                               "0x71040c4df8235e1d", (const void *)&census_import_20, NULL);
}

static obs_result check_census_21(void) {
    return probe_census_symbol("200-census/libSceAgc/0xe486fe61994e8976", "libSceAgc", "$dolOmWH+huQ",
                               "0xe486fe61994e8976", (const void *)&census_import_21, NULL);
}

static obs_result check_census_22(void) {
    return probe_census_symbol("200-census/libSceAjm/sceAjmInitialize", "libSceAjm", "$dl+4eHSzUu4",
                               "sceAjmInitialize", NULL, NULL);
}

static obs_result check_census_23(void) {
    return probe_census_symbol("200-census/libSceAmpr/0xa61453ecf71144c2", "libSceAmpr", "$wkQR9+xTFKY",
                               "0xa61453ecf71144c2", (const void *)&census_import_23, NULL);
}

static obs_result check_census_24(void) {
    return probe_census_symbol("200-census/libSceAmpr/0xb5866f7aedc94e43", "libSceAmpr", "$Q07J7XpvhrU",
                               "0xb5866f7aedc94e43", (const void *)&census_import_24, NULL);
}

static obs_result check_census_25(void) {
    return probe_census_symbol("200-census/libSceLibcInternalExt/0x78afc98fd7bb14a8", "libSceLibcInternalExt", "$qBS714-Jr3g",
                               "0x78afc98fd7bb14a8", NULL, NULL);
}

static obs_result check_census_26(void) {
    return probe_census_symbol("200-census/libSceNpUniversalDataSystem/sceNpUniversalDataSystemInitialize", "libSceNpUniversalDataSystem", "$sjaobBgqeB4",
                               "sceNpUniversalDataSystemInitialize", NULL, NULL);
}

static obs_result check_census_27(void) {
    return probe_census_symbol("200-census/libSceFont/sceFontCreateLibraryWithEdition", "libSceFont", "$n590hj5Oe-k",
                               "sceFontCreateLibraryWithEdition", (const void *)&census_import_27, (const void *)&census_named_27);
}

static obs_result check_census_28(void) {
    return probe_census_symbol("200-census/libSceFontFt/sceFontSelectLibraryFt", "libSceFontFt", "$oM+XCzVG3oM",
                               "sceFontSelectLibraryFt", (const void *)&census_import_28, (const void *)&census_named_28);
}

static obs_result check_census_29(void) {
    return probe_census_symbol("200-census/libSceUlt/sceUltConditionVariableSignal", "libSceUlt", "$JTw1cAVkuc0",
                               "sceUltConditionVariableSignal", NULL, NULL);
}

static const obs_check obs_checks_census[] = {
    {"200-census/libkernel/0x7ad80543eef3176c", "libkernel", "$bBfz7kMF2Ho", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_0, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprGetFileStat", "libkernel", "$ApkYaHb8Sek", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_1, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIds", "libkernel", "$i3HWvW35jao", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_2, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes", "libkernel", "$w5fcCG+t31g", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_3, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelCheckedReleaseDirectMemory", "libkernel", "$hwVSPCmp5tM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_4, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelGetGPI", "libkernel", "$4oXYe9Xmk0Q", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_5, OBS_FROM_ASSUMED},
    {"200-census/libSceSystemService/sceSystemServiceGetHdrToneMapLuminance", "libSceSystemService", "$mPpPxv5CZt4", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_6, OBS_FROM_ASSUMED},
    {"200-census/libSceSystemService/sceSystemServiceGetStatus", "libSceSystemService", "$rPo6tV8D9bM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_7, OBS_FROM_ASSUMED},
    {"200-census/libSceUserService/sceUserServiceGetAgeLevel", "libSceUserService", "$woNpu+45RLk", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_8, OBS_FROM_ASSUMED},
    {"200-census/libSceUserService/sceUserServiceGetEvent", "libSceUserService", "$yH17Q6NWtVg", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_9, OBS_FROM_ASSUMED},
    {"200-census/libSceNpManager/sceNpGetAccountCountryA", "libSceNpManager", "$JT+t00a3TxA", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_10, OBS_FROM_ASSUMED},
    {"200-census/libSceAudioOut2/sceAudioOut2UserDestroy", "libSceAudioOut2", "$IaZXJ9M79uo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_11, OBS_FROM_ASSUMED},
    {"200-census/libSceIme/sceImeUpdate", "libSceIme", "$-4GCfYdNF1s", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_12, OBS_FROM_ASSUMED},
    {"200-census/libScePad/0xda809fad5f12799f", "libScePad", "$n3kSX62fgNo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_13, OBS_FROM_ASSUMED},
    {"200-census/libSceVideoOut/sceVideoOutIsOutputSupported", "libSceVideoOut", "$Nv8c-Kb+DUM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_14, OBS_FROM_ASSUMED},
    {"200-census/libSceSaveData_native/sceSaveDataGetSaveDataMemory2", "libSceSaveData_native", "$QwOO7vegnV8", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_15, OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverGetDefaultOwner", "libSceAgcDriver", "$F0ZXt5q0ZTA", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_16, OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverGetResourceRegistrationMaxNameLength", "libSceAgcDriver", "$uJziRsODk1c", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_17, OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverSetTFRing", "libSceAgcDriver", "$XlNp7jzGiPo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_18, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x0a4e469ba741de7d", "libSceAgc", "$fd5Bp5tGTgo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_19, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x71040c4df8235e1d", "libSceAgc", "$HV4j+E0MBHE", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_20, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0xe486fe61994e8976", "libSceAgc", "$dolOmWH+huQ", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_21, OBS_FROM_ASSUMED},
    {"200-census/libSceAjm/sceAjmInitialize", "libSceAjm", "$dl+4eHSzUu4", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_22, OBS_FROM_ASSUMED},
    {"200-census/libSceAmpr/0xa61453ecf71144c2", "libSceAmpr", "$wkQR9+xTFKY", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_23, OBS_FROM_ASSUMED},
    {"200-census/libSceAmpr/0xb5866f7aedc94e43", "libSceAmpr", "$Q07J7XpvhrU", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_24, OBS_FROM_ASSUMED},
    {"200-census/libSceLibcInternalExt/0x78afc98fd7bb14a8", "libSceLibcInternalExt", "$qBS714-Jr3g", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_25, OBS_FROM_ASSUMED},
    {"200-census/libSceNpUniversalDataSystem/sceNpUniversalDataSystemInitialize", "libSceNpUniversalDataSystem", "$sjaobBgqeB4", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_26, OBS_FROM_ASSUMED},
    {"200-census/libSceFont/sceFontCreateLibraryWithEdition", "libSceFont", "$n590hj5Oe-k", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_27, OBS_FROM_ASSUMED},
    {"200-census/libSceFontFt/sceFontSelectLibraryFt", "libSceFontFt", "$oM+XCzVG3oM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_28, OBS_FROM_ASSUMED},
    {"200-census/libSceUlt/sceUltConditionVariableSignal", "libSceUlt", "$JTw1cAVkuc0", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_29, OBS_FROM_ASSUMED},
};

const obs_section obs_section_census = {
    "200-census",
    "Commercial title import surface census (Tier 1)",
    "Standard harness probe for retail title imports",
    obs_checks_census,
    OBS_COUNT(obs_checks_census),
};
