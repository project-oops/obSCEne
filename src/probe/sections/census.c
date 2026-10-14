/*
 * REQ-20261007T1045Z-cn01: Commercial title import surface census.
 *
 * Tier 1 functions imported by retail titles (PPSA02664, PPSA03416, PPSA21564,
 * PPSA25872, PPSA04263, PPSA28061).
 *
 * Each function is probed with the standard harness:
 * (1) Six arguments, each a separate 0x200-byte buffer filled with a distinct byte
 * (0xa1..0xa6), and the same call again with every argument 0. For each call: rc, fault
 * signal if any, and for each buffer whether it changed, with changed bytes. (2) Where
 * a function returns a pointer (> 0x10000), 0x80 bytes there. (3) Where (1) shows the
 * function writes an out-parameter in arg0, a second call reusing what the first wrote.
 */

#include <stdint.h>

#include "obscene/display.h"
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
OBS_WEAK uint64_t census_import_10(void *, void *, void *, void *, void *, void *) __asm__("$JT+t00a3TxA");
OBS_WEAK uint64_t census_named_10(void *, void *, void *, void *, void *, void *) __asm__("sceNpGetAccountCountryA");
OBS_WEAK uint64_t census_import_11(void *, void *, void *, void *, void *, void *) __asm__("$6vY70ydXpiE");
OBS_WEAK uint64_t census_import_11_iaz(void *, void *, void *, void *, void *, void *) __asm__("$IaZXJ9M79uo");
OBS_WEAK uint64_t census_named_11(void *, void *, void *, void *, void *, void *) __asm__("sceAudioOut2UserDestroy");
OBS_WEAK uint64_t census_import_12(void *, void *, void *, void *, void *, void *) __asm__("$-4GCfYdNF1s");
OBS_WEAK uint64_t census_named_12(void *, void *, void *, void *, void *, void *) __asm__("sceImeUpdate");
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
OBS_WEAK uint64_t census_import_22(void *, void *, void *, void *, void *, void *) __asm__("$7lKzdHi4X3Y");
OBS_WEAK uint64_t census_named_22(void *, void *, void *, void *, void *, void *) __asm__("sceAjmInitialize");
OBS_WEAK uint64_t census_import_23(void *, void *, void *, void *, void *, void *) __asm__("$wkQR9+xTFKY");
OBS_WEAK uint64_t census_import_24(void *, void *, void *, void *, void *, void *) __asm__("$Q07J7XpvhrU");
OBS_WEAK uint64_t census_import_25(void *, void *, void *, void *, void *, void *) __asm__("$eK-Jj9e7FKg");
OBS_WEAK uint64_t census_import_25_qbs(void *, void *, void *, void *, void *, void *) __asm__("$qBS714-Jr3g");
OBS_WEAK uint64_t census_import_26(void *, void *, void *, void *, void *, void *) __asm__("$HngqGGyoNrI");
OBS_WEAK uint64_t census_named_26(void *, void *, void *, void *, void *, void *) __asm__("sceNpUniversalDataSystemInitialize");
OBS_WEAK uint64_t census_import_27(void *, void *, void *, void *, void *, void *) __asm__("$n590hj5Oe-k");
OBS_WEAK uint64_t census_named_27(void *, void *, void *, void *, void *, void *) __asm__("sceFontCreateLibraryWithEdition");
OBS_WEAK uint64_t census_import_28(void *, void *, void *, void *, void *, void *) __asm__("$oM+XCzVG3oM");
OBS_WEAK uint64_t census_named_28(void *, void *, void *, void *, void *, void *) __asm__("sceFontSelectLibraryFt");
OBS_WEAK uint64_t census_import_29(void *, void *, void *, void *, void *, void *) __asm__("$zblkBXA1PCU");
OBS_WEAK uint64_t census_named_29(void *, void *, void *, void *, void *, void *) __asm__("sceUltConditionVariableSignal");

/* Checks 30-33: POSIX ids (REQ-20261007T2125Z-cn04) */
OBS_WEAK int census_named_getuid(void) __asm__("getuid");
OBS_WEAK int census_import_getuid(void) __asm__("$kg4x8Prhfxw");
OBS_WEAK int census_named_geteuid(void) __asm__("geteuid");
OBS_WEAK int census_import_geteuid(void) __asm__("$tvpHe5kBO4E");
OBS_WEAK int census_named_getgid(void) __asm__("getgid");
OBS_WEAK int census_import_getgid(void) __asm__("$AfuS23bX6kg");
OBS_WEAK int census_named_getegid(void) __asm__("getegid");
OBS_WEAK int census_import_getegid(void) __asm__("$72rYuYoDTWk");

/* Check 34: sceNpEntitlementAccessInitialize (REQ-20261007T2125Z-cn04) */
OBS_WEAK uint64_t census_named_34(void *, void *, void *, void *, void *, void *) __asm__("sceNpEntitlementAccessInitialize");
OBS_WEAK uint64_t census_import_34(void *, void *, void *, void *, void *, void *) __asm__("$jO8DM8oyego");

/* Checks 35-36: sceAudioOut2 additions (REQ-20261007T2300Z-cn05) */
OBS_WEAK uint64_t census_named_35(void *, void *, void *, void *, void *, void *) __asm__("sceAudioOut2Initialize");
OBS_WEAK uint64_t census_import_35(void *, void *, void *, void *, void *, void *) __asm__("$g2tViFIohHE");
OBS_WEAK uint64_t census_named_36(void *, void *, void *, void *, void *, void *) __asm__("sceAudioOut2UserCreate");
OBS_WEAK uint64_t census_import_36(void *, void *, void *, void *, void *, void *) __asm__("$xywYcRB7nbQ");

/* Checks 37-40: per-frame calls (REQ-20261007T1705Z-cn02) */
OBS_WEAK uint64_t census_import_37(void *, void *, void *, void *, void *, void *) __asm__("$eb+gvTYQcoY");
OBS_WEAK uint64_t census_import_38(void *, void *, void *, void *, void *, void *) __asm__("$MCJ8SkzsQxY");
OBS_WEAK uint64_t census_import_39(void *, void *, void *, void *, void *, void *) __asm__("$dbOlWdppb4o");
OBS_WEAK uint64_t census_import_40(void *, void *, void *, void *, void *, void *) __asm__("$V++UgBtQhn0");

/* Checks 42-43: font support calls (REQ-20261007T1705Z-cn02 addendum) */
OBS_WEAK int census_named_42(void *) __asm__("sceFontSupportSystemFonts");
OBS_WEAK int census_import_42(void *) __asm__("$SsRbbCiWoGw");
OBS_WEAK int census_named_43(void *) __asm__("sceFontSupportExternalFonts");
OBS_WEAK int census_import_43(void *) __asm__("$mz2iTY0MK4A");
OBS_WEAK int census_import_font_mem_init(void *, void *, uint32_t, void *, void *, void *, uint64_t) __asm__("$whrS4oksXc4");
OBS_WEAK void *sceLibcMspaceCreate(const char *, void *, size_t, uint64_t);
OBS_WEAK void *sceLibcMspaceMalloc(void *, size_t);
OBS_WEAK void sceLibcMspaceFree(void *, void *);

/* Checks 44-48: weak SDK calls (REQ-20261007T1815Z-cn03) */
OBS_WEAK int census_named_44(void *) __asm__("sceSystemServiceReceiveEvent");
OBS_WEAK int census_import_44(void *) __asm__("$656LMQSrg6U");
OBS_WEAK int census_named_45(void *) __asm__("sceUserServiceGetForegroundUser");
OBS_WEAK int census_import_45(void *) __asm__("$-sD02mFDBh4");
OBS_WEAK int census_named_46(int) __asm__("scePadSetProcessPrivilege");
OBS_WEAK int census_import_46(int) __asm__("$u1GRHp+oWoY");
OBS_WEAK int census_named_47(int32_t, int, int) __asm__("scePadGetHandle");
OBS_WEAK int census_named_47_init(void) __asm__("scePadInit");
OBS_WEAK int census_named_48(int32_t, int, int, void *) __asm__("sceKeyboardOpen");
OBS_WEAK int census_import_48(int32_t, int, int, void *) __asm__("$HJ+KnEHcaxI");
/* clang-format on */

static size_t census_build_passthrough_vs_container(uint8_t *out_buf,
                                                    size_t out_buf_size);
static size_t census_build_ps_container(uint8_t *out_buf, size_t out_buf_size);

const void *census_resolve_symbol(const char *library, const char *sym_or_nid,
                                  const char *name, const void *static_fn,
                                  const void *named_fn) {
    if (static_fn != NULL && obs_address_is_callable(static_fn)) {
        return static_fn;
    }
    if (named_fn != NULL && obs_address_is_callable(named_fn)) {
        return named_fn;
    }

    const char *target_lib = library;
    if (library != NULL) {
        if (obs_strcmp(library, "libSceLibcInternalExt") == 0) {
            target_lib = "libSceLibcInternal";
        }
    }

    int h = obs_find_module_handle_by_library(target_lib);
    if (h <= 1 && target_lib != library) {
        h = obs_find_module_handle_by_library(library);
    }
    if (h <= 1 && library != NULL) {
        if (obs_strcmp(library, "libSceAudioOut2") == 0) {
            (void)obs_sysmodule_load_id(0x8000001c);
            (void)obs_sysmodule_load_id(0x001c);
            (void)obs_sysmodule_get_handle_id(0x8000001c, &h);
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x001c, &h);
            }
            if (h <= 1) {
                h = obs_find_module_handle_by_library("libSceAudioOut2");
            }
            if (h <= 1) {
                h = obs_find_module_handle_by_library("libSceAudioOut");
            }
        } else if (obs_strcmp(library, "libSceAjm") == 0) {
            (void)obs_sysmodule_load_id(0x80000023);
            (void)obs_sysmodule_load_id(0x002e);
            (void)obs_sysmodule_get_handle_id(0x80000023, &h);
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x002e, &h);
            }
        } else if (obs_strcmp(library, "libSceNpUniversalDataSystem") == 0) {
            (void)obs_sysmodule_load_id(0x80000105);
            (void)obs_sysmodule_load_id(0x0105);
            (void)obs_sysmodule_get_handle_id(0x80000105, &h);
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x0105, &h);
            }
        } else if (obs_strcmp(library, "libSceUlt") == 0) {
            (void)obs_sysmodule_load_id(0x80000007);
            (void)obs_sysmodule_load_id(0x0007);
            (void)obs_sysmodule_load_id(0x0027);
            (void)obs_sysmodule_get_handle_id(0x80000007, &h);
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x0007, &h);
            }
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x0027, &h);
            }
        } else if (obs_strcmp(library, "libSceNpEntitlementAccess") == 0) {
            if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
                h = sceKernelLoadStartModule(
                    "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0,
                    NULL, NULL);
            }
            if (h <= 1) {
                h = obs_module_open("libSceNpEntitlementAccess.sprx");
            }
        } else if (obs_strcmp(library, "libSceNpSessionSignaling") == 0) {
            if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
                h = sceKernelLoadStartModule(
                    "/system/common/lib/libSceNpSessionSignaling.sprx", 0, NULL, 0,
                    NULL, NULL);
            }
            if (h <= 1) {
                h = obs_module_open("libSceNpSessionSignaling.sprx");
            }
        } else if (obs_strcmp(library, "libSceNpTrophy2") == 0) {
            (void)obs_sysmodule_load_id(0x0120);
            (void)obs_sysmodule_get_handle_id(0x0120, &h);
            if (h <= 1 &&
                obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
                h = sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx",
                                             0, NULL, 0, NULL, NULL);
            }
            if (h <= 1) {
                h = obs_module_open("libSceNpTrophy2.sprx");
            }
        } else if (obs_strcmp(library, "libSceSaveData_native") == 0) {
            (void)obs_sysmodule_load_id(0x8000000f);
            (void)obs_sysmodule_load_id(0x00b6);
            (void)obs_sysmodule_get_handle_id(0x8000000f, &h);
            if (h <= 1) {
                (void)obs_sysmodule_get_handle_id(0x00b6, &h);
            }
            if (h <= 1) {
                h = obs_find_module_handle_by_library("libSceSaveData_native");
            }
        }
    }
    if (h <= 1) {
        h = obs_module_open(target_lib);
    }
    if (h <= 1 && target_lib != library) {
        h = obs_module_open(library);
    }
    if (h <= 1 && (obs_strcmp(library, "libSceLibcInternalExt") == 0 ||
                   obs_strcmp(library, "libSceLibcInternal") == 0)) {
        h = obs_find_module_handle_by_library("libSceLibcInternal");
        if (h <= 1) {
            h = 1;
        }
    }

    char bare_nid[32] = {0};
    char dollar_nid[32] = {0};
    char cand_buf[200][48];
    const char *cand[200];
    size_t cand_count = 0;

    const char *pnid = NULL;
    if (name != NULL && name[0] != '\0') {
        pnid = obs_prospero_name_to_nid(name);
    }
    if (pnid == NULL && sym_or_nid != NULL && sym_or_nid[0] != '\0') {
        pnid = obs_prospero_name_to_nid(sym_or_nid);
    }
    if (pnid != NULL) {
        cand[cand_count++] = pnid;
    }

    const char *alt_nid = NULL;
    if (sym_or_nid != NULL) {
        if (obs_strcmp(sym_or_nid, "$IaZXJ9M79uo") == 0 ||
            obs_strcmp(sym_or_nid, "IaZXJ9M79uo") == 0) {
            alt_nid = "6vY70ydXpiE";
        } else if (obs_strcmp(sym_or_nid, "$6vY70ydXpiE") == 0 ||
                   obs_strcmp(sym_or_nid, "6vY70ydXpiE") == 0) {
            alt_nid = "IaZXJ9M79uo";
        } else if (obs_strcmp(sym_or_nid, "$dl+4eHSzUu4") == 0 ||
                   obs_strcmp(sym_or_nid, "dl+4eHSzUu4") == 0) {
            alt_nid = "7lKzdHi4X3Y";
        } else if (obs_strcmp(sym_or_nid, "$7lKzdHi4X3Y") == 0 ||
                   obs_strcmp(sym_or_nid, "7lKzdHi4X3Y") == 0) {
            alt_nid = "dl+4eHSzUu4";
        } else if (obs_strcmp(sym_or_nid, "$sjaobBgqeB4") == 0 ||
                   obs_strcmp(sym_or_nid, "sjaobBgqeB4") == 0) {
            alt_nid = "HngqGGyoNrI";
        } else if (obs_strcmp(sym_or_nid, "$HngqGGyoNrI") == 0 ||
                   obs_strcmp(sym_or_nid, "HngqGGyoNrI") == 0) {
            alt_nid = "sjaobBgqeB4";
        } else if (obs_strcmp(sym_or_nid, "$JTw1cAVkuc0") == 0 ||
                   obs_strcmp(sym_or_nid, "JTw1cAVkuc0") == 0) {
            alt_nid = "zblkBXA1PCU";
        } else if (obs_strcmp(sym_or_nid, "$zblkBXA1PCU") == 0 ||
                   obs_strcmp(sym_or_nid, "zblkBXA1PCU") == 0) {
            alt_nid = "JTw1cAVkuc0";
        } else if (obs_strcmp(sym_or_nid, "$qBS714-Jr3g") == 0 ||
                   obs_strcmp(sym_or_nid, "qBS714-Jr3g") == 0) {
            alt_nid = "eK-Jj9e7FKg";
        } else if (obs_strcmp(sym_or_nid, "$eK-Jj9e7FKg") == 0 ||
                   obs_strcmp(sym_or_nid, "eK-Jj9e7FKg") == 0) {
            alt_nid = "qBS714-Jr3g";
        }
    }

    if (sym_or_nid != NULL && sym_or_nid[0] != '\0') {
        const char *nid_raw = sym_or_nid;
        if (nid_raw[0] == '$') {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "%s", nid_raw);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid_raw + 1);
        } else {
            oops_snprintf(dollar_nid, sizeof(dollar_nid), "$%s", nid_raw);
            oops_snprintf(bare_nid, sizeof(bare_nid), "%s", nid_raw);
        }
        cand[cand_count++] = dollar_nid;
        cand[cand_count++] = bare_nid;

        for (char l = 'A'; l <= 'Z' && cand_count < 150; l++) {
            size_t idx = cand_count++;
            oops_snprintf(cand_buf[idx], sizeof(cand_buf[idx]), "%s#%c#A", bare_nid, l);
            cand[idx] = cand_buf[idx];

            idx = cand_count++;
            oops_snprintf(cand_buf[idx], sizeof(cand_buf[idx]), "$%s#%c#A", bare_nid,
                          l);
            cand[idx] = cand_buf[idx];
        }

        static const struct {
            char l1;
            char l2;
        } pairs[] = {{'A', 'B'}, {'B', 'C'}, {'C', 'D'}, {'D', 'E'}, {'E', 'F'},
                     {'F', 'G'}, {'G', 'H'}, {'H', 'I'}, {'I', 'J'}, {'J', 'K'},
                     {'K', 'L'}, {'L', 'M'}, {'M', 'N'}, {'N', 'O'}, {'O', 'P'},
                     {'P', 'Q'}, {'Q', 'R'}, {'R', 'S'}, {'S', 'T'}, {'T', 'U'},
                     {'U', 'V'}, {'V', 'W'}, {'W', 'X'}, {'X', 'Y'}, {'Y', 'Z'},
                     {'Z', 'a'}, {'a', 'b'}, {'b', 'c'}, {'c', 'd'}, {'d', 'e'}};
        for (size_t p = 0; p < sizeof(pairs) / sizeof(pairs[0]) && cand_count < 190;
             p++) {
            size_t idx = cand_count++;
            oops_snprintf(cand_buf[idx], sizeof(cand_buf[idx]), "%s#%c#%c", bare_nid,
                          pairs[p].l1, pairs[p].l2);
            cand[idx] = cand_buf[idx];

            idx = cand_count++;
            oops_snprintf(cand_buf[idx], sizeof(cand_buf[idx]), "$%s#%c#%c", bare_nid,
                          pairs[p].l1, pairs[p].l2);
            cand[idx] = cand_buf[idx];
        }
    }
    if (alt_nid != NULL && cand_count < 190) {
        cand[cand_count++] = alt_nid;
        size_t aidx = cand_count++;
        oops_snprintf(cand_buf[aidx], sizeof(cand_buf[aidx]), "$%s", alt_nid);
        cand[aidx] = cand_buf[aidx];
    }
    if (name != NULL && name[0] != '\0' && obs_strcmp(name, "<unknown>") != 0 &&
        cand_count < 199) {
        cand[cand_count++] = name;
    }

    int (*fn_dlsym)(int, const char *, void **) =
        (int (*)(int, const char *, void **))&sceKernelDlsym;

    /* Fast path: direct dlsym on resolved module handle */
    if (h >= 1 && obs_address_is_callable((const void *)fn_dlsym)) {
        const void *p = obs_dlsym_probe_handle(fn_dlsym, h, pnid, bare_nid, name);
        if (p != NULL && obs_address_is_callable(p)) {
            return p;
        }
        if (alt_nid != NULL) {
            p = obs_dlsym_probe_handle(fn_dlsym, h, NULL, alt_nid, name);
            if (p != NULL && obs_address_is_callable(p)) {
                return p;
            }
        }
        for (size_t c = 0; c < cand_count; c++) {
            void *sym = NULL;
            if (sceKernelDlsym(h, cand[c], &sym) == 0 && obs_address_is_callable(sym)) {
                return sym;
            }
        }
    }

    /* 0x2001 (RTLD_DEFAULT) fallback across all loaded modules */
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (size_t c = 0; c < cand_count; c++) {
            void *sym = NULL;
            if (sceKernelDlsym(0x2001, cand[c], &sym) == 0 &&
                obs_address_is_callable(sym)) {
                return sym;
            }
        }
    }

    /* Scan loaded module list */
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList) &&
        obs_address_is_callable((const void *)fn_dlsym)) {
        int mlist[128];
        size_t mcount = 0;
        if (sceKernelGetModuleList(mlist, 128, &mcount) == 0 && mcount > 0) {
            for (size_t m = 0; m < mcount && m < 128; m++) {
                int mid = mlist[m];
                if (mid <= 0 || mid == h) {
                    continue;
                }
                const void *p =
                    obs_dlsym_probe_handle(fn_dlsym, mid, pnid, bare_nid, name);
                if (p != NULL && obs_address_is_callable(p)) {
                    obs_sysmodule_cache_handle(target_lib, mid);
                    return p;
                }
                if (alt_nid != NULL) {
                    p = obs_dlsym_probe_handle(fn_dlsym, mid, NULL, alt_nid, name);
                    if (p != NULL && obs_address_is_callable(p)) {
                        obs_sysmodule_cache_handle(target_lib, mid);
                        return p;
                    }
                }
                for (size_t c = 0; c < cand_count; c++) {
                    void *sym = NULL;
                    if (sceKernelDlsym(mid, cand[c], &sym) == 0 &&
                        obs_address_is_callable(sym)) {
                        obs_sysmodule_cache_handle(target_lib, mid);
                        return sym;
                    }
                }
            }
        }
    }

    /* Sweep all module handles 1..256 */
    if (obs_address_is_callable((const void *)fn_dlsym)) {
        for (int mid = 1; mid <= 256; mid++) {
            if (mid == h) {
                continue;
            }
            for (size_t c = 0; c < cand_count; c++) {
                void *sym = NULL;
                if (fn_dlsym(mid, cand[c], &sym) == 0 && obs_address_is_callable(sym)) {
                    obs_sysmodule_cache_handle(target_lib, mid);
                    return sym;
                }
            }
        }
    }

    /* Probe handle 1 (eboot.bin) and handle 2 (libkernel) explicitly */
    if (obs_address_is_callable((const void *)fn_dlsym)) {
        if (h != 1) {
            const void *p = obs_dlsym_probe_handle(fn_dlsym, 1, pnid, bare_nid, name);
            if (p != NULL && obs_address_is_callable(p)) {
                return p;
            }
            if (alt_nid != NULL) {
                p = obs_dlsym_probe_handle(fn_dlsym, 1, NULL, alt_nid, name);
                if (p != NULL && obs_address_is_callable(p)) {
                    return p;
                }
            }
            for (size_t c = 0; c < cand_count; c++) {
                void *sym = NULL;
                if (sceKernelDlsym(1, cand[c], &sym) == 0 &&
                    obs_address_is_callable(sym)) {
                    return sym;
                }
            }
        }
        if (h != 2) {
            const void *p = obs_dlsym_probe_handle(fn_dlsym, 2, pnid, bare_nid, name);
            if (p != NULL && obs_address_is_callable(p)) {
                return p;
            }
            if (alt_nid != NULL) {
                p = obs_dlsym_probe_handle(fn_dlsym, 2, NULL, alt_nid, name);
                if (p != NULL && obs_address_is_callable(p)) {
                    return p;
                }
            }
            for (size_t c = 0; c < cand_count; c++) {
                void *sym = NULL;
                if (sceKernelDlsym(2, cand[c], &sym) == 0 &&
                    obs_address_is_callable(sym)) {
                    return sym;
                }
            }
        }
    }

    /* Try obs_module_symbol on handle h or loaded modules for primary names */
    int probe_h = (h >= 1) ? h : -1;
    if (pnid != NULL) {
        const void *sym = obs_module_symbol(probe_h, pnid);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return sym;
        }
    }
    if (sym_or_nid != NULL) {
        const void *sym = obs_module_symbol(probe_h, sym_or_nid);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return sym;
        }
    }
    if (name != NULL) {
        const void *sym = obs_module_symbol(probe_h, name);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return sym;
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

static void census_preload_modules(void);

static obs_result probe_census_symbol(const char *check_name, const char *library,
                                      const char *sym_or_nid, const char *name,
                                      const void *static_fn, const void *named_fn) {
    static int s_census_preloaded = 0;
    if (!s_census_preloaded) {
        s_census_preloaded = 1;
        census_preload_modules();
    }
    const void *fn =
        census_resolve_symbol(library, sym_or_nid, name, static_fn, named_fn);
    int callable = (fn != NULL && obs_address_is_callable(fn));
    int is_link = ((static_fn != NULL && fn == static_fn) ||
                   (named_fn != NULL && fn == named_fn));
    const char *sym_name = (name != NULL && name[0] != '\0') ? name : sym_or_nid;
    obs_report_measure(check_name, sym_name, "bound", (uint64_t)callable, "bool");
    if (!callable) {
        return obs_skip("symbol not resolved");
    }
    obs_report_measure(check_name, sym_name, "how", (uint64_t)(is_link ? 1 : 2),
                       is_link ? "link" : "dlsym");
    obs_report_measure(check_name, "resolve", "callable", (uint64_t)callable, "bool");

    uint8_t buf[6][0x200];
    uint8_t buf_orig[6][0x200];
    obs_jmp_buf guard;

    /* (1) Six arguments, each a separate 0x200-byte buffer filled with distinct byte
     * (0xa1..0xa6) */
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
        obs_report_measure(check_name, "call-buffers", "fault", (uint64_t)sig_buf,
                           "signal");
    } else {
        obs_report_measure(check_name, "call-buffers", "rc", rc_buf, "code");
        if (rc_buf >= 0x10000ULL && rc_buf < 0x00007fffffffffffULL) {
            uint8_t ptr_dump[0x80];
            int sig_read = OBS_FAULT_ARM(&guard);
            if (sig_read == 0) {
                memcpy(ptr_dump, (const void *)rc_buf, 0x80);
                obs_fault_unregister();
                obs_report_bytes(check_name, "call-buffers", "return-ptr-80b", 0,
                                 ptr_dump, 0x80);
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
            obs_report_written(check_name, "census", arg_name, buf_orig[i], buf[i],
                               0x200);
            for (unsigned int off = 0; off < 0x200; off += 64) {
                if (memcmp(buf[i] + off, buf_orig[i] + off, 64) != 0) {
                    obs_report_bytes(check_name, arg_name, "bytes", off, buf[i] + off,
                                     64);
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
        obs_report_measure(check_name, "call-zeros", "fault", (uint64_t)sig_zero,
                           "signal");
    } else {
        obs_report_measure(check_name, "call-zeros", "rc", rc_zero, "code");
        if (rc_zero >= 0x10000ULL && rc_zero < 0x00007fffffffffffULL) {
            uint8_t ptr_dump[0x80];
            int sig_read = OBS_FAULT_ARM(&guard);
            if (sig_read == 0) {
                memcpy(ptr_dump, (const void *)rc_zero, 0x80);
                obs_fault_unregister();
                obs_report_bytes(check_name, "call-zeros", "return-ptr-80b", 0,
                                 ptr_dump, 0x80);
            } else {
                obs_fault_unregister();
            }
        }
    }

    /* (3) Where (1) shows function writes an out-parameter in buf[0], second call
     * reusing what it wrote */
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

static void census_preload_modules(void) {
    obs_sysmodules_preload_and_cache();
    (void)obs_sysmodule_load_id(0x0007);     /* libSceUlt */
    (void)obs_sysmodule_load_id(0x001c);     /* libSceAudioOut */
    (void)obs_sysmodule_load_id(0x0095);     /* libSceIme */
    (void)obs_sysmodule_load_id(0x0014);     /* libSceAppInstUtil */
    (void)obs_sysmodule_load_id(0x002e);     /* libSceAjm */
    (void)obs_sysmodule_load_id(0x80000023); /* libSceAjm legacy */
    (void)obs_sysmodule_load_id(0x0084);     /* libSceFont */
    (void)obs_sysmodule_load_id(0x0096);     /* libSceImeDialog */
    (void)obs_sysmodule_load_id(0x0098);     /* libSceFontFt */
    (void)obs_sysmodule_load_id(0x009d);     /* libSceNpAuth / Manager */
    (void)obs_sysmodule_load_id(0x00e4);     /* libSceSigninDialog */
    (void)obs_sysmodule_load_id(0x8000000f); /* libSceSaveData */
    (void)obs_sysmodule_load_id(0x00cf);     /* libSceVideodec2 */
    (void)obs_sysmodule_load_id(0x00e7);     /* libSceJson2 */
    (void)obs_sysmodule_load_id(0x0105);     /* libSceNpUniversalDataSystem */
    (void)obs_sysmodule_load_id(0x0027);     /* libSceUlt PS4 */
    (void)obs_sysmodule_load_id(0x80000007); /* libSceUlt PS5 */
    (void)obs_sysmodule_load_id(0x00b6);     /* libSceSaveData PS4 */
    (void)obs_sysmodule_load_id(0x80000024); /* libScePad */
    (void)obs_sysmodule_load_id(0x80000011); /* libSceUserService */
    (void)obs_sysmodule_load_id(0x80000018); /* libSceCommonDialog */
    (void)obs_sysmodule_load_id(0x001c);     /* libSceAudioOut2 */
    (void)obs_sysmodule_load_id(0x8000001c); /* libSceAudioOut2 PS5 */
    (void)obs_sysmodule_load_id(0x80000105); /* libSceNpUniversalDataSystem PS5 */
    (void)obs_sysmodule_load_id(0x800000e4); /* libSceSigninDialog PS5 */
    obs_refresh_kexport_table();
}

static obs_result check_census_0(void) {
    census_preload_modules();
    return probe_census_symbol("200-census/libkernel/0x7ad80543eef3176c", "libkernel",
                               "$bBfz7kMF2Ho", "0x7ad80543eef3176c",
                               (const void *)&census_import_0, NULL);
}

static obs_result check_census_1(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelAprGetFileStat",
                               "libkernel", "$ApkYaHb8Sek", "sceKernelAprGetFileStat",
                               (const void *)&census_import_1,
                               (const void *)&census_named_1);
}

static obs_result check_census_2(void) {
    return probe_census_symbol(
        "200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIds", "libkernel",
        "$i3HWvW35jao", "sceKernelAprResolveFilepathsWithPrefixToIds",
        (const void *)&census_import_2, (const void *)&census_named_2);
}

static obs_result check_census_3(void) {
    return probe_census_symbol(
        "200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes",
        "libkernel", "$w5fcCG+t31g",
        "sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes",
        (const void *)&census_import_3, (const void *)&census_named_3);
}

static obs_result check_census_4(void) {
    return probe_census_symbol(
        "200-census/libkernel/sceKernelCheckedReleaseDirectMemory", "libkernel",
        "$hwVSPCmp5tM", "sceKernelCheckedReleaseDirectMemory",
        (const void *)&census_import_4, (const void *)&census_named_4);
}

static obs_result check_census_5(void) {
    return probe_census_symbol("200-census/libkernel/sceKernelGetGPI", "libkernel",
                               "$4oXYe9Xmk0Q", "sceKernelGetGPI",
                               (const void *)&census_import_5,
                               (const void *)&census_named_5);
}

static obs_result check_census_6(void) {
    return probe_census_symbol(
        "200-census/libSceSystemService/sceSystemServiceGetHdrToneMapLuminance",
        "libSceSystemService", "$mPpPxv5CZt4", "sceSystemServiceGetHdrToneMapLuminance",
        (const void *)&census_import_6, (const void *)&census_named_6);
}

static obs_result check_census_7(void) {
    return probe_census_symbol(
        "200-census/libSceSystemService/sceSystemServiceGetStatus",
        "libSceSystemService", "$rPo6tV8D9bM", "sceSystemServiceGetStatus",
        (const void *)&census_import_7, (const void *)&census_named_7);
}

static obs_result check_census_8(void) {
    return probe_census_symbol(
        "200-census/libSceUserService/sceUserServiceGetAgeLevel", "libSceUserService",
        "$woNpu+45RLk", "sceUserServiceGetAgeLevel", (const void *)&census_import_8,
        (const void *)&census_named_8);
}

static obs_result check_census_9(void) {
    return probe_census_symbol("200-census/libSceUserService/sceUserServiceGetEvent",
                               "libSceUserService", "$yH17Q6NWtVg",
                               "sceUserServiceGetEvent", (const void *)&census_import_9,
                               (const void *)&census_named_9);
}

static obs_result check_census_10(void) {
    return probe_census_symbol(
        "200-census/libSceNpManager/sceNpGetAccountCountryA", "libSceNpManager",
        "$JT+t00a3TxA", "sceNpGetAccountCountryA", (const void *)&census_import_10,
        (const void *)&census_named_10);
}

static obs_result check_census_11(void) {
    const void *static_fn = (const void *)&census_import_11_iaz;
    if (static_fn == NULL || !obs_address_is_callable(static_fn)) {
        static_fn = (const void *)&census_import_11;
    }
    if (static_fn == NULL || !obs_address_is_callable(static_fn)) {
        static_fn = (const void *)&census_named_11;
    }
    return probe_census_symbol("200-census/libSceAudioOut2/sceAudioOut2UserDestroy",
                               "libSceAudioOut2", "$IaZXJ9M79uo",
                               "sceAudioOut2UserDestroy", static_fn,
                               (const void *)&census_named_11);
}

static obs_result check_census_12(void) {
    return probe_census_symbol("200-census/libSceIme/sceImeUpdate", "libSceIme",
                               "$-4GCfYdNF1s", "sceImeUpdate",
                               (const void *)&census_import_12,
                               (const void *)&census_named_12);
}

static obs_result check_census_13(void) {
    return probe_census_symbol("200-census/libScePad/0xda809fad5f12799f", "libScePad",
                               "$n3kSX62fgNo", "0xda809fad5f12799f",
                               (const void *)&census_import_13, NULL);
}

static obs_result check_census_14(void) {
    return probe_census_symbol(
        "200-census/libSceVideoOut/sceVideoOutIsOutputSupported", "libSceVideoOut",
        "$Nv8c-Kb+DUM", "sceVideoOutIsOutputSupported", (const void *)&census_import_14,
        (const void *)&census_named_14);
}

static obs_result check_census_15(void) {
    return probe_census_symbol(
        "200-census/libSceSaveData_native/sceSaveDataGetSaveDataMemory2",
        "libSceSaveData_native", "$QwOO7vegnV8", "sceSaveDataGetSaveDataMemory2",
        (const void *)&census_import_15, (const void *)&census_named_15);
}

static obs_result check_census_16(void) {
    return probe_census_symbol(
        "200-census/libSceAgcDriver/sceAgcDriverGetDefaultOwner", "libSceAgcDriver",
        "$F0ZXt5q0ZTA", "sceAgcDriverGetDefaultOwner", (const void *)&census_import_16,
        (const void *)&census_named_16);
}

static obs_result check_census_17(void) {
    return probe_census_symbol(
        "200-census/libSceAgcDriver/sceAgcDriverGetResourceRegistrationMaxNameLength",
        "libSceAgcDriver", "$uJziRsODk1c",
        "sceAgcDriverGetResourceRegistrationMaxNameLength",
        (const void *)&census_import_17, (const void *)&census_named_17);
}

static obs_result check_census_18(void) {
    return probe_census_symbol("200-census/libSceAgcDriver/sceAgcDriverSetTFRing",
                               "libSceAgcDriver", "$XlNp7jzGiPo",
                               "sceAgcDriverSetTFRing", (const void *)&census_import_18,
                               (const void *)&census_named_18);
}

static obs_result check_census_19(void) {
    const char *id = "200-census/libSceAgc/0x0a4e469ba741de7d";
    obs_result res =
        probe_census_symbol(id, "libSceAgc", "$fd5Bp5tGTgo", "0x0a4e469ba741de7d",
                            (const void *)&census_import_19, NULL);
    const void *sym =
        census_resolve_symbol("libSceAgc", "$fd5Bp5tGTgo", "0x0a4e469ba741de7d",
                              (const void *)&census_import_19, NULL);
    if (sym && obs_address_is_callable(sym)) {
        /* REQ-20261009T0630Z-cn09 (1): 0x0a4e469ba741de7d(record, vs, ps, 0, 0, 0)
         * with record a 0x110-byte buffer filled with 0xa5, dumped full 0x110 */
        uint8_t record[0x110];
        memset(record, 0xa5, sizeof(record));
        uint8_t record_before[0x110];
        memcpy(record_before, record, sizeof(record_before));

        uint8_t vs_hdr[384];
        obs_agc_init_stage_hdr_retail(vs_hdr, 2u, 0xc8u);
        uint8_t ps_hdr[384];
        obs_agc_init_stage_hdr_retail(ps_hdr, 1u, 0x08u);

        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t rc =
                ((uint64_t (*)(void *, void *, void *, uint64_t, uint64_t,
                               uint64_t))sym)(record, vs_hdr, ps_hdr, 0, 0, 0);
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-a5", "rc", rc, "code");
            obs_report_bytes(id, "real-shape-a5", "record-out-110h", 0, record, 0x110);
            obs_report_written(id, "0x0a4e469ba741de7d", "record-change", record_before,
                               record, 0x110);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-a5", "fault", (uint64_t)sig, "signal");
        }

        /* REQ-20261009T1130Z-cn11: shaders created via sceAgcCreateShader */
        const void *payload = obs_agc_get_retail_payload();
        void *vs_obj = NULL;
        void *ps_obj = NULL;
        uint64_t rc_vs_create = 0xffffffff;
        uint64_t rc_ps_create = 0xffffffff;
        if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_vs_create = sceAgcCreateShader(&vs_obj, vs_hdr, payload, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "vs-create-fault",
                                   (uint64_t)sig, "signal");
            }

            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_ps_create = sceAgcCreateShader(&ps_obj, ps_hdr, payload, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "ps-create-fault",
                                   (uint64_t)sig, "signal");
            }
        }
        obs_report_measure(id, "created-shader-obj", "vs-create-rc", rc_vs_create,
                           "code");
        obs_report_measure(id, "created-shader-obj", "ps-create-rc", rc_ps_create,
                           "code");

        if (vs_obj != NULL) {
            uint8_t rec2[0x110];
            memset(rec2, 0xa5, sizeof(rec2));
            uint8_t rec2_before[0x110];
            memcpy(rec2_before, rec2, sizeof(rec2_before));

            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc =
                    ((uint64_t (*)(void *, void *, void *, uint64_t, uint64_t,
                                   uint64_t))sym)(rec2, vs_obj, ps_obj, 0, 0, 0);
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "rc", rc, "code");
                obs_report_bytes(id, "created-shader-obj", "record-out-110h", 0, rec2,
                                 0x110);
                obs_report_written(id, "0x0a4e469ba741de7d-obj", "record-change",
                                   rec2_before, rec2, 0x110);
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "fault", (uint64_t)sig,
                                   "signal");
            }

            /* Arm vs-only with ps = 0 to tell which argument it refuses */
            uint8_t rec3[0x110];
            memset(rec3, 0xa5, sizeof(rec3));
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc =
                    ((uint64_t (*)(void *, void *, void *, uint64_t, uint64_t,
                                   uint64_t))sym)(rec3, vs_obj, NULL, 0, 0, 0);
                obs_fault_unregister();
                obs_report_measure(id, "vs-only", "rc", rc, "code");
                obs_report_bytes(id, "vs-only", "record-out-110h", 0, rec3, 0x110);
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "vs-only", "fault", (uint64_t)sig, "signal");
            }
        }
    }
    return res;
}

static obs_result check_census_20(void) {
    obs_result res = probe_census_symbol(
        "200-census/libSceAgc/0x71040c4df8235e1d", "libSceAgc", "$HV4j+E0MBHE",
        "0x71040c4df8235e1d", (const void *)&census_import_20, NULL);

    const void *sym =
        census_resolve_symbol("libSceAgc", "$HV4j+E0MBHE", "0x71040c4df8235e1d",
                              (const void *)&census_import_20, NULL);
    if (sym && obs_address_is_callable(sym)) {
        /* REQ-20261007T2125Z-cn04 addendum 2: real shape */
        const char *id = "200-census/libSceAgc/0x71040c4df8235e1d";
        uint8_t rec[0x100];
        memset(rec, 0, sizeof(rec));
        static const uint8_t rec_init[24] = {
            0x75, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x02, 0x00, 0x00,
            0x00, 0x40, 0x00, 0x00, 0x42, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
        memcpy(rec, rec_init, sizeof(rec_init));
        uint8_t vs_hdr[0x100];
        memset(vs_hdr, 0, sizeof(vs_hdr));
        static const uint8_t hdr_magic[8] = {0x31, 0x32, 0x33, 0x34,
                                             0x18, 0x00, 0x00, 0x00};
        memcpy(vs_hdr, hdr_magic, sizeof(hdr_magic));
        uint8_t ps_hdr[0x100];
        memset(ps_hdr, 0, sizeof(ps_hdr));
        memcpy(ps_hdr, hdr_magic, sizeof(hdr_magic));
        uint8_t p1[0x100];
        uint8_t p2[0x100];
        memset(p1, 0xcc, sizeof(p1));
        memset(p2, 0xcc, sizeof(p2));

        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t rc = ((uint64_t (*)(void *, void *, void *, uint64_t, void *,
                                         void *))sym)(rec, vs_hdr, ps_hdr, 0, p1, p2);
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-with-ps", "rc", rc, "code");
            obs_report_bytes(id, "real-shape-with-ps", "rec-out", 0, rec, 64);
            obs_report_bytes(id, "real-shape-with-ps", "p1-out", 0, p1, 64);
            obs_report_bytes(id, "real-shape-with-ps", "p2-out", 0, p2, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-with-ps", "fault", (uint64_t)sig,
                               "signal");
        }

        memcpy(rec, rec_init, sizeof(rec_init));
        memset(p1, 0xcc, sizeof(p1));
        memset(p2, 0xcc, sizeof(p2));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t rc0 = ((uint64_t (*)(void *, void *, void *, uint64_t, void *,
                                          void *))sym)(rec, vs_hdr, NULL, 0, p1, p2);
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-ps0", "rc", rc0, "code");
            obs_report_bytes(id, "real-shape-ps0", "rec-out", 0, rec, 64);
            obs_report_bytes(id, "real-shape-ps0", "p1-out", 0, p1, 64);
            obs_report_bytes(id, "real-shape-ps0", "p2-out", 0, p2, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-ps0", "fault", (uint64_t)sig, "signal");
        }
    }
    return res;
}

static obs_result check_census_21(void) {
    const char *id = "200-census/libSceAgc/0xe486fe61994e8976";
    obs_result res =
        probe_census_symbol(id, "libSceAgc", "$dolOmWH+huQ", "0xe486fe61994e8976",
                            (const void *)&census_import_21, NULL);
    const void *sym =
        census_resolve_symbol("libSceAgc", "$dolOmWH+huQ", "0xe486fe61994e8976",
                              (const void *)&census_import_21, NULL);
    if (sym && obs_address_is_callable(sym)) {
        /* REQ-20261009T0630Z-cn09 (2): 0xe486fe61994e8976(state, vs, ps, p3, p4, 0),
         * with state a 0x110-byte 0xa5 buffer whose dwords +0x10 and +0x18 hold 0x10
         * and 0x1f, and p3, p4 0x110-byte 0xcc buffers. Dump full 0x110 bytes of all
         * three */
        uint8_t state[0x110];
        memset(state, 0xa5, sizeof(state));
        *(uint32_t *)(state + 0x10) = 0x10u;
        *(uint32_t *)(state + 0x18) = 0x1fu;
        uint8_t state_before[0x110];
        memcpy(state_before, state, sizeof(state_before));

        uint8_t p3[0x110];
        memset(p3, 0xcc, sizeof(p3));
        uint8_t p3_before[0x110];
        memcpy(p3_before, p3, sizeof(p3_before));

        uint8_t p4[0x110];
        memset(p4, 0xcc, sizeof(p4));
        uint8_t p4_before[0x110];
        memcpy(p4_before, p4, sizeof(p4_before));

        uint8_t vs_hdr[384];
        obs_agc_init_stage_hdr_retail(vs_hdr, 2u, 0xc8u);
        uint8_t ps_hdr[384];
        obs_agc_init_stage_hdr_retail(ps_hdr, 1u, 0x08u);

        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t rc =
                ((uint64_t (*)(void *, void *, void *, void *, void *, uint64_t))sym)(
                    state, vs_hdr, ps_hdr, p3, p4, 0);
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-a5", "rc", rc, "code");
            obs_report_bytes(id, "real-shape-a5", "state-out-110h", 0, state, 0x110);
            obs_report_written(id, "0xe486fe61994e8976", "state-change", state_before,
                               state, 0x110);
            obs_report_bytes(id, "real-shape-a5", "p3-out-110h", 0, p3, 0x110);
            obs_report_written(id, "0xe486fe61994e8976", "p3-change", p3_before, p3,
                               0x110);
            obs_report_bytes(id, "real-shape-a5", "p4-out-110h", 0, p4, 0x110);
            obs_report_written(id, "0xe486fe61994e8976", "p4-change", p4_before, p4,
                               0x110);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "real-shape-a5", "fault", (uint64_t)sig, "signal");
        }

        /* REQ-20261009T1130Z-cn11: shaders created via sceAgcCreateShader */
        const void *payload = obs_agc_get_retail_payload();
        void *vs_obj = NULL;
        void *ps_obj = NULL;
        uint64_t rc_vs_create = 0xffffffff;
        uint64_t rc_ps_create = 0xffffffff;
        if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_vs_create = sceAgcCreateShader(&vs_obj, vs_hdr, payload, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "vs-create-fault",
                                   (uint64_t)sig, "signal");
            }

            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_ps_create = sceAgcCreateShader(&ps_obj, ps_hdr, payload, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "ps-create-fault",
                                   (uint64_t)sig, "signal");
            }
        }
        obs_report_measure(id, "created-shader-obj", "vs-create-rc", rc_vs_create,
                           "code");
        obs_report_measure(id, "created-shader-obj", "ps-create-rc", rc_ps_create,
                           "code");

        if (vs_obj != NULL) {
            uint8_t st2[0x110];
            memset(st2, 0xa5, sizeof(st2));
            *(uint32_t *)(st2 + 0x10) = 0x10u;
            *(uint32_t *)(st2 + 0x18) = 0x1fu;
            uint8_t p3_2[0x110], p4_2[0x110];
            memset(p3_2, 0xcc, sizeof(p3_2));
            memset(p4_2, 0xcc, sizeof(p4_2));

            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc =
                    ((uint64_t (*)(void *, void *, void *, void *, void *,
                                   uint64_t))sym)(st2, vs_obj, ps_obj, p3_2, p4_2, 0);
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "rc", rc, "code");
                obs_report_bytes(id, "created-shader-obj", "state-out-110h", 0, st2,
                                 0x110);
                obs_report_bytes(id, "created-shader-obj", "p3-out-110h", 0, p3_2,
                                 0x110);
                obs_report_bytes(id, "created-shader-obj", "p4-out-110h", 0, p4_2,
                                 0x110);
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "created-shader-obj", "fault", (uint64_t)sig,
                                   "signal");
            }

            /* Arm vs-only with ps = 0 to tell which argument it refuses */
            uint8_t st3[0x110];
            memset(st3, 0xa5, sizeof(st3));
            *(uint32_t *)(st3 + 0x10) = 0x10u;
            *(uint32_t *)(st3 + 0x18) = 0x1fu;
            uint8_t p3_3[0x110], p4_3[0x110];
            memset(p3_3, 0xcc, sizeof(p3_3));
            memset(p4_3, 0xcc, sizeof(p4_3));
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc =
                    ((uint64_t (*)(void *, void *, void *, void *, void *,
                                   uint64_t))sym)(st3, vs_obj, NULL, p3_3, p4_3, 0);
                obs_fault_unregister();
                obs_report_measure(id, "vs-only", "rc", rc, "code");
                obs_report_bytes(id, "vs-only", "state-out-110h", 0, st3, 0x110);
                obs_report_bytes(id, "vs-only", "p3-out-110h", 0, p3_3, 0x110);
                obs_report_bytes(id, "vs-only", "p4-out-110h", 0, p4_3, 0x110);
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "vs-only", "fault", (uint64_t)sig, "signal");
            }
        }
    }
    return res;
}

/* ---- REQ-20261013T1110Z-sp01: PPSA04263 shader pair on 0x0a4e... and 0xe486... ----
 */
static obs_result check_sp01_shader_pair(void) {
    const char *id = "200-census/libSceAgc/ppsa04263-shader-pair";

    const void *fn_0a4e = (const void *)&census_import_19;
    if (!obs_address_is_callable(fn_0a4e)) {
        fn_0a4e = census_resolve_symbol("libSceAgc", "$fd5Bp5tGTgo",
                                        "0x0a4e469ba741de7d", NULL, NULL);
    }
    const void *fn_e486 = (const void *)&census_import_21;
    if (!obs_address_is_callable(fn_e486)) {
        fn_e486 = census_resolve_symbol("libSceAgc", "$dolOmWH+huQ",
                                        "0xe486fe61994e8976", NULL, NULL);
    }

    obs_report_measure(id, "0x0a4e469ba741de7d", "resolved",
                       (uint64_t)(fn_0a4e && obs_address_is_callable(fn_0a4e)), "bool");
    obs_report_measure(id, "0xe486fe61994e8976", "resolved",
                       (uint64_t)(fn_e486 && obs_address_is_callable(fn_e486)), "bool");

    static __attribute__((aligned(256))) uint8_t s_code_A[0x410];
    static __attribute__((aligned(256))) uint8_t s_code_B[0xe20];
    static __attribute__((aligned(64))) uint8_t s_obj_A[0x140];
    static __attribute__((aligned(64))) uint8_t s_obj_B[0x180];

    memset(s_code_A, 0, sizeof(s_code_A));
    memset(s_code_B, 0, sizeof(s_code_B));
    memset(s_obj_A, 0, sizeof(s_obj_A));
    memset(s_obj_B, 0, sizeof(s_obj_B));

    /* Shader A (stage 5, 5 SH registers, header 0x120, shader 0x410) */
    uint64_t *qw_A = (uint64_t *)(void *)s_obj_A;
    qw_A[0x00 / 8] = 0x0000001834333231ULL;
    qw_A[0x08 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0xd0);
    qw_A[0x10 / 8] = (uint64_t)(uintptr_t)s_code_A;
    qw_A[0x18 / 8] = 0x0000000000000000ULL;
    qw_A[0x20 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0xa8);
    qw_A[0x28 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x60);
    qw_A[0x30 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x90);
    qw_A[0x38 / 8] = 0x0000000000000000ULL;
    qw_A[0x40 / 8] = 0x0000041000000120ULL;
    qw_A[0x48 / 8] = 0x0000000b00000000ULL;
    qw_A[0x50 / 8] = 0x0000000000000005ULL;
    qw_A[0x58 / 8] = 0x0000000500050030ULL;
    qw_A[0x60 / 8] = 0x0000000000000000ULL;
    qw_A[0x68 / 8] = 0x00000000000002d5ULL;
    qw_A[0x70 / 8] = 0x0004000000000000ULL;
    qw_A[0x78 / 8] = 0x00000000a0000000ULL;
    qw_A[0x80 / 8] = 0x0000000000000000ULL;
    qw_A[0x88 / 8] = 0x0000000000000262ULL;
    qw_A[0x90 / 8] = 0x00040c0100030900ULL;
    qw_A[0x98 / 8] = 0x0003120300021002ULL;
    qw_A[0xa0 / 8] = 0x0000000000041504ULL;
    qw_A[0xa8 / 8] = 0x00433a3a0000010aULL;
    qw_A[0xb0 / 8] = 0x000000740000010bULL;
    qw_A[0xb8 / 8] = 0x030000050000014aULL;
    qw_A[0xc0 / 8] = 0x1d0003e000000100ULL;
    qw_A[0xc8 / 8] = 0x5624e9ee00000100ULL;
    qw_A[0xd0 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x108);
    qw_A[0xd8 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x120);
    qw_A[0xe0 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x120);
    qw_A[0xe8 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x120);
    qw_A[0xf0 / 8] = (uint64_t)(uintptr_t)(s_obj_A + 0x120);
    qw_A[0xf8 / 8] = 0x0000000b00000000ULL;
    qw_A[0x100 / 8] = 0x0000000000000000ULL;
    qw_A[0x108 / 8] = 0xffffffffffffffffULL;
    qw_A[0x110 / 8] = 0xffffffffffffffffULL;
    qw_A[0x118 / 8] = 0x00000002ffff0000ULL;

    /* Shader B (stage 7, 4 CX, 10 SH registers, header 0x158, shader 0xe20) */
    uint64_t *qw_B = (uint64_t *)(void *)s_obj_B;
    qw_B[0x00 / 8] = 0x0000001834333231ULL;
    qw_B[0x08 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x100);
    qw_B[0x10 / 8] = (uint64_t)(uintptr_t)s_code_B;
    qw_B[0x18 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0xe0);
    qw_B[0x20 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x90);
    qw_B[0x28 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x60);
    qw_B[0x30 / 8] = 0x0000000000000000ULL;
    qw_B[0x38 / 8] = 0x0000000000000000ULL;
    qw_B[0x40 / 8] = 0x00000e2000000158ULL;
    qw_B[0x48 / 8] = 0x0000000b00000000ULL;
    qw_B[0x50 / 8] = 0x0000000000000000ULL;
    qw_B[0x58 / 8] = 0x0000000a04070030ULL;
    qw_B[0x60 / 8] = 0x0001fe380000025bULL;
    qw_B[0x68 / 8] = 0x00000105000002d5ULL;
    qw_B[0x70 / 8] = 0x000a000000000000ULL;
    qw_B[0x78 / 8] = 0x0000000000000000ULL;
    qw_B[0x80 / 8] = 0x000000020000029bULL;
    qw_B[0x88 / 8] = 0x0000000000000262ULL;
    qw_B[0x90 / 8] = 0x005b732000000148ULL;
    qw_B[0x98 / 8] = 0x0000007400000149ULL;
    qw_B[0xa0 / 8] = 0x0000000000000100ULL;
    qw_B[0xa8 / 8] = 0x0000000000000100ULL;
    qw_B[0xb0 / 8] = 0x312c01d10000010aULL;
    qw_B[0xb8 / 8] = 0x006c00940000010bULL;
    qw_B[0xc0 / 8] = 0x0000000000000108ULL;
    qw_B[0xc8 / 8] = 0x0000000000000109ULL;
    qw_B[0xd0 / 8] = 0x47000df000000100ULL;
    qw_B[0xd8 / 8] = 0x8f0fcc1100000100ULL;
    qw_B[0xe0 / 8] = 0x00040049000002dbULL;
    qw_B[0xe8 / 8] = 0x4170000000000286ULL;
    qw_B[0xf0 / 8] = 0x3f80000000000287ULL;
    qw_B[0xf8 / 8] = 0x0000c338000002d6ULL;
    qw_B[0x100 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x138);
    qw_B[0x108 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x150);
    qw_B[0x110 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x150);
    qw_B[0x118 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x150);
    qw_B[0x120 / 8] = (uint64_t)(uintptr_t)(s_obj_B + 0x150);
    qw_B[0x128 / 8] = 0x0000000b00000000ULL;
    qw_B[0x130 / 8] = 0x0000000200000000ULL;
    qw_B[0x138 / 8] = 0xffffffffffffffffULL;
    qw_B[0x140 / 8] = 0xffffffffffff0008ULL;
    qw_B[0x148 / 8] = 0x0000ffffffffffffULL;
    qw_B[0x150 / 8] = 0x0000000080048000ULL;

    obs_report_bytes(id, "shader-A", "obj-60h", 0, s_obj_A, 0x60);
    obs_report_bytes(id, "shader-B", "obj-60h", 0, s_obj_B, 0x60);

    typedef uint64_t (*fn_agc_pair_t)(void *, void *, void *, void *, uint64_t,
                                      uint64_t);
    fn_agc_pair_t call_0a4e = (fn_agc_pair_t)fn_0a4e;
    fn_agc_pair_t call_e486 = (fn_agc_pair_t)fn_e486;

    uint8_t out[0x400];
    uint8_t aux[0x400];
    uint8_t state[0x400];

    obs_report_measure(id, "addresses", "out-addr", (uint64_t)(uintptr_t)out, "hex");
    obs_report_measure(id, "addresses", "aux-addr", (uint64_t)(uintptr_t)aux, "hex");
    obs_report_measure(id, "addresses", "state-addr", (uint64_t)(uintptr_t)state,
                       "hex");
    obs_report_measure(id, "addresses", "obj-A-addr", (uint64_t)(uintptr_t)s_obj_A,
                       "hex");
    obs_report_measure(id, "addresses", "obj-B-addr", (uint64_t)(uintptr_t)s_obj_B,
                       "hex");
    obs_report_measure(id, "addresses", "code-A-addr", (uint64_t)(uintptr_t)s_code_A,
                       "hex");
    obs_report_measure(id, "addresses", "code-B-addr", (uint64_t)(uintptr_t)s_code_B,
                       "hex");

    /* 1. 0x0a4e469ba741de7d(out, A, B, aux, 4, 0) with out and aux 0x400-byte buffers
     * filled 0xcc */
    if (call_0a4e && obs_address_is_callable((const void *)call_0a4e)) {
        obs_jmp_buf guard;
        memset(out, 0xcc, sizeof(out));
        memset(aux, 0xcc, sizeof(aux));
        int sig = OBS_FAULT_ARM(&guard);
        uint64_t rc1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc1 = call_0a4e(out, s_obj_A, s_obj_B, aux, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "0x0a4e-AB-arg5-4", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "0x0a4e-AB-arg5-4", "rc", rc1, "code");
        for (unsigned int off = 0; off < 0x200; off += 64) {
            obs_report_bytes(id, "0x0a4e-AB-arg5-4", "out-200h", off, out + off, 64);
            obs_report_bytes(id, "0x0a4e-AB-arg5-4", "aux-200h", off, aux + off, 64);
        }
        for (unsigned int off = 0; off < 0x180; off += 64) {
            obs_report_bytes(id, "0x0a4e-AB-arg5-4", "obj-A-180h", off, s_obj_A + off,
                             64);
            obs_report_bytes(id, "0x0a4e-AB-arg5-4", "obj-B-180h", off, s_obj_B + off,
                             64);
        }

        /* 3a. 0x0a4e again with arg5 = 1 */
        memset(out, 0xcc, sizeof(out));
        memset(aux, 0xcc, sizeof(aux));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc1_arg1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc1_arg1 = call_0a4e(out, s_obj_A, s_obj_B, aux, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0x0a4e-AB-arg5-1", "rc", rc1_arg1, "code");

        /* 3b. 0x0a4e with A and B swapped, arg5 = 4 */
        memset(out, 0xcc, sizeof(out));
        memset(aux, 0xcc, sizeof(aux));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc1_swap4 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc1_swap4 = call_0a4e(out, s_obj_B, s_obj_A, aux, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0x0a4e-BA-arg5-4", "rc", rc1_swap4, "code");

        /* 3c. 0x0a4e with A and B swapped, arg5 = 1 */
        memset(out, 0xcc, sizeof(out));
        memset(aux, 0xcc, sizeof(aux));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc1_swap1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc1_swap1 = call_0a4e(out, s_obj_B, s_obj_A, aux, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0x0a4e-BA-arg5-1", "rc", rc1_swap1, "code");
    }

    /* 2. 0xe486fe61994e8976(out, A, B, state, 4, 0) with out and state 0x400-byte
     * buffers filled 0xcc */
    if (call_e486 && obs_address_is_callable((const void *)call_e486)) {
        obs_jmp_buf guard;
        memset(out, 0xcc, sizeof(out));
        memset(state, 0xcc, sizeof(state));
        int sig = OBS_FAULT_ARM(&guard);
        uint64_t rc2 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc2 = call_e486(out, s_obj_A, s_obj_B, state, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "0xe486-AB-arg5-4", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "0xe486-AB-arg5-4", "rc", rc2, "code");
        for (unsigned int off = 0; off < 0x200; off += 64) {
            obs_report_bytes(id, "0xe486-AB-arg5-4", "out-200h", off, out + off, 64);
            obs_report_bytes(id, "0xe486-AB-arg5-4", "state-200h", off, state + off,
                             64);
        }

        /* 3d. 0xe486 again with arg5 = 1 */
        memset(out, 0xcc, sizeof(out));
        memset(state, 0xcc, sizeof(state));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc2_arg1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc2_arg1 = call_e486(out, s_obj_A, s_obj_B, state, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0xe486-AB-arg5-1", "rc", rc2_arg1, "code");
        for (unsigned int off = 0; off < 0x200; off += 64) {
            obs_report_bytes(id, "0xe486-AB-arg5-1", "out-200h", off, out + off, 64);
            obs_report_bytes(id, "0xe486-AB-arg5-1", "state-200h", off, state + off,
                             64);
        }

        /* 3e. 0xe486 with A and B swapped, arg5 = 4 */
        memset(out, 0xcc, sizeof(out));
        memset(state, 0xcc, sizeof(state));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc2_swap4 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc2_swap4 = call_e486(out, s_obj_B, s_obj_A, state, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0xe486-BA-arg5-4", "rc", rc2_swap4, "code");

        /* 3f. 0xe486 with A and B swapped, arg5 = 1 */
        memset(out, 0xcc, sizeof(out));
        memset(state, 0xcc, sizeof(state));
        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc2_swap1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc2_swap1 = call_e486(out, s_obj_B, s_obj_A, state, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "0xe486-BA-arg5-1", "rc", rc2_swap1, "code");
    }

    /* REQ-20261013T2000Z-sp03 arm 1: edited shader pair */
    static __attribute__((aligned(64))) uint8_t s_obj_A_edit[0x180];
    static __attribute__((aligned(64))) uint8_t s_obj_B_edit[0x180];
    memcpy(s_obj_A_edit, s_obj_A, sizeof(s_obj_A));
    memcpy(s_obj_B_edit, s_obj_B, sizeof(s_obj_B));

    uint64_t *qw_Ae = (uint64_t *)(void *)s_obj_A_edit;
    /* A's RSRC1 VGPR field (0x10a value bits 5:0) set below B's (0x00433a00 | 0x08 =
     * 0x00433a08) */
    qw_Ae[0xa8 / 8] = 0x00433a080000010aULL;
    /* A's RSRC2 user-SGPR field (0x10b bits 5:1) below B's (0x74 to 0x04) */
    qw_Ae[0xb0 / 8] = 0x000000040000010bULL;
    /* A count byte at +0x5c is 6 */
    qw_Ae[0x58 / 8] = 0x0000000600050030ULL;
    /* A given a third 0x100 entry (0x33333333) */
    qw_Ae[0xd0 / 8] = 0x3333333300000100ULL;
    /* Shift subsequent fields of A */
    qw_Ae[0x08 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0xd8);
    qw_Ae[0xd8 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0x110);
    qw_Ae[0xe0 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0x128);
    qw_Ae[0xe8 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0x128);
    qw_Ae[0xf0 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0x128);
    qw_Ae[0xf8 / 8] = (uint64_t)(uintptr_t)(s_obj_A_edit + 0x128);
    qw_Ae[0x100 / 8] = 0x0000000b00000000ULL;
    qw_Ae[0x108 / 8] = 0x0000000000000000ULL;
    qw_Ae[0x110 / 8] = 0xffffffffffffffffULL;
    qw_Ae[0x118 / 8] = 0xffffffffffffffffULL;
    qw_Ae[0x120 / 8] = 0x00000002ffff0000ULL;

    uint64_t *qw_Be = (uint64_t *)(void *)s_obj_B_edit;
    /* B's first two 0x100 values nonzero (0x11111111, 0x22222222) */
    qw_Be[0xa0 / 8] = 0x1111111100000100ULL;
    qw_Be[0xa8 / 8] = 0x2222222200000100ULL;

    /* Dumps of edited objects */
    obs_report_bytes(id, "shader-A-edit", "obj-60h", 0, s_obj_A_edit, 0x60);
    obs_report_bytes(id, "shader-B-edit", "obj-60h", 0, s_obj_B_edit, 0x60);

    /* 0x0a4e on edited pair */
    if (call_0a4e && obs_address_is_callable((const void *)call_0a4e)) {
        obs_jmp_buf guard_e;
        memset(out, 0xcc, sizeof(out));
        memset(aux, 0xcc, sizeof(aux));
        int sig = OBS_FAULT_ARM(&guard_e);
        uint64_t rc_e1 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc_e1 = call_0a4e(out, s_obj_A_edit, s_obj_B_edit, aux, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "edit-0x0a4e-AB-arg5-4", "rc", rc_e1, "code");
        for (unsigned int off = 0; off < 0x200; off += 64) {
            obs_report_bytes(id, "edit-0x0a4e-AB-arg5-4", "out-200h", off, out + off,
                             64);
            obs_report_bytes(id, "edit-0x0a4e-AB-arg5-4", "aux-200h", off, aux + off,
                             64);
        }
    }

    /* 0xe486 on edited pair */
    if (call_e486 && obs_address_is_callable((const void *)call_e486)) {
        obs_jmp_buf guard_e;
        memset(out, 0xcc, sizeof(out));
        memset(state, 0xcc, sizeof(state));
        int sig = OBS_FAULT_ARM(&guard_e);
        uint64_t rc_e2 = 0xffffffffffffffffULL;
        if (sig == 0) {
            rc_e2 = call_e486(out, s_obj_A_edit, s_obj_B_edit, state, 4, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "edit-0xe486-AB-arg5-4", "rc", rc_e2, "code");
        obs_report_bytes(id, "edit-0xe486-AB-arg5-4", "out-16b", 0, out, 16);
        for (unsigned int off = 0; off < 0x200; off += 64) {
            obs_report_bytes(id, "edit-0xe486-AB-arg5-4", "out-200h", off, out + off,
                             64);
            obs_report_bytes(id, "edit-0xe486-AB-arg5-4", "state-200h", off,
                             state + off, 64);
        }
    }

    return obs_pass();
}

static obs_result check_census_22(void) {
    return probe_census_symbol("200-census/libSceAjm/sceAjmInitialize", "libSceAjm",
                               "$7lKzdHi4X3Y", "sceAjmInitialize",
                               (const void *)&census_import_22,
                               (const void *)&census_named_22);
}

static obs_result check_census_23(void) {
    return probe_census_symbol("200-census/libSceAmpr/0xa61453ecf71144c2", "libSceAmpr",
                               "$wkQR9+xTFKY", "0xa61453ecf71144c2",
                               (const void *)&census_import_23, NULL);
}

static obs_result check_census_24(void) {
    return probe_census_symbol("200-census/libSceAmpr/0xb5866f7aedc94e43", "libSceAmpr",
                               "$Q07J7XpvhrU", "0xb5866f7aedc94e43",
                               (const void *)&census_import_24, NULL);
}

static obs_result check_census_25(void) {
    const void *static_fn = (const void *)&census_import_25_qbs;
    if (static_fn == NULL || !obs_address_is_callable(static_fn)) {
        static_fn = (const void *)&census_import_25;
    }
    return probe_census_symbol("200-census/libSceLibcInternalExt/0x78afc98fd7bb14a8",
                               "libSceLibcInternalExt", "$qBS714-Jr3g",
                               "0x78afc98fd7bb14a8", static_fn, NULL);
}

static obs_result check_census_26(void) {
    return probe_census_symbol(
        "200-census/libSceNpUniversalDataSystem/sceNpUniversalDataSystemInitialize",
        "libSceNpUniversalDataSystem", "$HngqGGyoNrI",
        "sceNpUniversalDataSystemInitialize", (const void *)&census_import_26,
        (const void *)&census_named_26);
}

static obs_result check_census_27(void) {
    return probe_census_symbol(
        "200-census/libSceFont/sceFontCreateLibraryWithEdition", "libSceFont",
        "$n590hj5Oe-k", "sceFontCreateLibraryWithEdition",
        (const void *)&census_import_27, (const void *)&census_named_27);
}

static obs_result check_census_28(void) {
    return probe_census_symbol("200-census/libSceFontFt/sceFontSelectLibraryFt",
                               "libSceFontFt", "$oM+XCzVG3oM", "sceFontSelectLibraryFt",
                               (const void *)&census_import_28,
                               (const void *)&census_named_28);
}

static obs_result check_census_29(void) {
    return probe_census_symbol(
        "200-census/libSceUlt/sceUltConditionVariableSignal", "libSceUlt",
        "$zblkBXA1PCU", "sceUltConditionVariableSignal",
        (const void *)&census_import_29, (const void *)&census_named_29);
}

static obs_result check_census_30(void) {
    const char *id = "200-census/libkernel/getuid";
    int (*fn)(void) = (int (*)(void))census_resolve_symbol(
        "libkernel", "$kg4x8Prhfxw", "getuid", (const void *)&census_import_getuid,
        (const void *)&census_named_getuid);
    if (!fn || !obs_address_is_callable((const void *)fn)) {
        return obs_skip("symbol not resolved");
    }
    int r1 = fn();
    int r2 = fn();
    obs_report_measure(id, "call-first", "rc", (uint64_t)r1, "id");
    obs_report_measure(id, "call-second", "rc", (uint64_t)r2, "id");
    return obs_pass();
}

static obs_result check_census_31(void) {
    const char *id = "200-census/libkernel/geteuid";
    int (*fn)(void) = (int (*)(void))census_resolve_symbol(
        "libkernel", "$tvpHe5kBO4E", "geteuid", (const void *)&census_import_geteuid,
        (const void *)&census_named_geteuid);
    if (!fn || !obs_address_is_callable((const void *)fn)) {
        return obs_skip("symbol not resolved");
    }
    int r1 = fn();
    int r2 = fn();
    obs_report_measure(id, "call-first", "rc", (uint64_t)r1, "id");
    obs_report_measure(id, "call-second", "rc", (uint64_t)r2, "id");
    return obs_pass();
}

static obs_result check_census_32(void) {
    const char *id = "200-census/libkernel/getgid";
    int (*fn)(void) = (int (*)(void))census_resolve_symbol(
        "libkernel", "$AfuS23bX6kg", "getgid", (const void *)&census_import_getgid,
        (const void *)&census_named_getgid);
    if (!fn || !obs_address_is_callable((const void *)fn)) {
        return obs_skip("symbol not resolved");
    }
    int r1 = fn();
    int r2 = fn();
    obs_report_measure(id, "call-first", "rc", (uint64_t)r1, "id");
    obs_report_measure(id, "call-second", "rc", (uint64_t)r2, "id");
    return obs_pass();
}

static obs_result check_census_33(void) {
    const char *id = "200-census/libkernel/getegid";
    int (*fn)(void) = (int (*)(void))census_resolve_symbol(
        "libkernel", "$72rYuYoDTWk", "getegid", (const void *)&census_import_getegid,
        (const void *)&census_named_getegid);
    if (!fn || !obs_address_is_callable((const void *)fn)) {
        return obs_skip("symbol not resolved");
    }
    int r1 = fn();
    int r2 = fn();
    obs_report_measure(id, "call-first", "rc", (uint64_t)r1, "id");
    obs_report_measure(id, "call-second", "rc", (uint64_t)r2, "id");
    return obs_pass();
}

static obs_result check_census_34(void) {
    const char *id =
        "200-census/libSceNpEntitlementAccess/sceNpEntitlementAccessInitialize";
    int load_res = 0;
    int h_mod = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_mod = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0, NULL,
            &load_res);
    }
    obs_report_measure(id, "module-load", "handle", (uint64_t)(int64_t)h_mod, "handle");
    obs_report_measure(id, "module-load", "rc", (uint64_t)(int64_t)h_mod, "code");
    obs_report_measure(id, "module-load", "load-res", (uint64_t)(uint32_t)load_res,
                       "code");

    const void *sym = NULL;
    if (h_mod > 0 && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        int rc_dlsym =
            sceKernelDlsym(h_mod, "sceNpEntitlementAccessInitialize", (void **)&sym);
        obs_report_measure(id, "module-dlsym", "rc", (uint64_t)(uint32_t)rc_dlsym,
                           "code");
        obs_report_measure(id, "module-dlsym", "sym", (uint64_t)(uintptr_t)sym, "addr");
        if (!sym) {
            rc_dlsym = sceKernelDlsym(h_mod, "$jO8DM8oyego", (void **)&sym);
            obs_report_measure(id, "module-dlsym-nid", "rc",
                               (uint64_t)(uint32_t)rc_dlsym, "code");
            obs_report_measure(id, "module-dlsym-nid", "sym", (uint64_t)(uintptr_t)sym,
                               "addr");
        }
    }
    if (!sym) {
        sym = census_resolve_symbol("libSceNpEntitlementAccess", "$jO8DM8oyego",
                                    "sceNpEntitlementAccessInitialize",
                                    (const void *)&census_import_34,
                                    (const void *)&census_named_34);
    }
    if (!sym && h_mod >= 0) {
        sym = obs_module_symbol(h_mod, "sceNpEntitlementAccessInitialize");
        if (!sym)
            sym = obs_module_symbol(h_mod, "$jO8DM8oyego");
    }
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    return probe_census_symbol(id, "libSceNpEntitlementAccess", "$jO8DM8oyego",
                               "sceNpEntitlementAccessInitialize", sym, sym);
}

static obs_result check_census_35(void) {
    return probe_census_symbol(
        "200-census/libSceAudioOut2/sceAudioOut2Initialize", "libSceAudioOut2",
        "$g2tViFIohHE", "sceAudioOut2Initialize", (const void *)&census_import_35,
        (const void *)&census_named_35);
}

static obs_result check_census_36(void) {
    return probe_census_symbol(
        "200-census/libSceAudioOut2/sceAudioOut2UserCreate", "libSceAudioOut2",
        "$xywYcRB7nbQ", "sceAudioOut2UserCreate", (const void *)&census_import_36,
        (const void *)&census_named_36);
}

static obs_result check_census_37(void) {
    const char *id = "200-census/libSceVideoOut/0x86721036bde0bf79";
    const void *sym = NULL;
    int is_link = 0;
    if (obs_address_is_callable(
            (const void *)&sceVideoOutUnknown_86721036bde0bf79_static)) {
        sym = (const void *)&sceVideoOutUnknown_86721036bde0bf79_static;
        is_link = 1;
    }
    if (!sym) {
        sym = census_resolve_symbol("libSceVideoOut", "$hnIQNr3gv3k",
                                    "0x86721036bde0bf79", NULL, NULL);
    }
    if (!sym) {
        sym = census_resolve_symbol("libSceVideoOut", "hnIQNr3gv3k#I#A",
                                    "0x86721036bde0bf79", NULL, NULL);
    }
    if (!sym) {
        sym = census_resolve_symbol("libSceVideoOut", "hnIQNr3gv3k",
                                    "0x86721036bde0bf79", NULL, NULL);
    }
    if (!sym && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        static const char *const s_vo_cands[] = {"hnIQNr3gv3k#I#A", "hnIQNr3gv3k",
                                                 "$hnIQNr3gv3k",    "eb-gvTYQcoY#I#A",
                                                 "eb-gvTYQcoY",     "$eb-gvTYQcoY"};
        for (int h = 1; h <= 256; h++) {
            for (size_t c = 0; c < sizeof(s_vo_cands) / sizeof(s_vo_cands[0]); c++) {
                void *p = NULL;
                if (sceKernelDlsym(h, s_vo_cands[c], &p) == 0 &&
                    obs_address_is_callable(p)) {
                    sym = p;
                    break;
                }
            }
            if (sym)
                break;
        }
    }

    int is_bound = (sym != NULL && obs_address_is_callable(sym));
    obs_report_measure(id, "0x86721036bde0bf79", "bound", (uint64_t)is_bound, "bool");
    if (!is_bound) {
        return obs_skip("symbol not resolved");
    }
    obs_report_measure(id, "0x86721036bde0bf79", "how", (uint64_t)(is_link ? 1 : 2),
                       is_link ? "link" : "dlsym");
    obs_report_measure(id, "resolve", "callable", (uint64_t)is_bound, "bool");

    int hvo = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        hvo = sceVideoOutOpen(255, 0, 0, NULL);
        if (hvo < 0) {
            hvo = sceVideoOutOpen(0, 0, 0, 0);
        }
    }
    obs_report_measure(id, "video-out-open", "hvo", (uint64_t)(uint32_t)hvo, "handle");

    obs_jmp_buf guard;

    /* REQ-20261013T1045Z-vf01: PPSA28061 per-frame shape (handle, buf, 0) */
    typedef int (*fn_vo_ppsa_t)(int handle, void *buf, int a2);
    fn_vo_ppsa_t fn_ppsa = (fn_vo_ppsa_t)sym;

    /* 1. buf 0x20 zeroed */
    uint8_t buf_zero[0x20];
    memset(buf_zero, 0, sizeof(buf_zero));
    int rc_pz = -1;
    int sig_pz = OBS_FAULT_ARM(&guard);
    if (sig_pz == 0) {
        rc_pz = fn_ppsa(hvo >= 0 ? hvo : 1, buf_zero, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ppsa-frame-zeroed", "rc", (uint64_t)(uint32_t)rc_pz,
                       "code");
    obs_report_bytes(id, "ppsa-frame-zeroed", "buf-out", 0, buf_zero, 0x20);

    /* 2. buf 0x20 filled 0xcc */
    uint8_t buf_cc[0x20];
    memset(buf_cc, 0xcc, sizeof(buf_cc));
    int rc_pcc = -1;
    int sig_pcc = OBS_FAULT_ARM(&guard);
    if (sig_pcc == 0) {
        rc_pcc = fn_ppsa(hvo >= 0 ? hvo : 1, buf_cc, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ppsa-frame-0xcc", "rc", (uint64_t)(uint32_t)rc_pcc, "code");
    obs_report_bytes(id, "ppsa-frame-0xcc", "buf-out", 0, buf_cc, 0x20);

    /* 3. Handle naming no port (0x7fffffff) */
    uint8_t buf_np[0x20];
    memset(buf_np, 0, sizeof(buf_np));
    int rc_pnp = -1;
    int sig_pnp = OBS_FAULT_ARM(&guard);
    if (sig_pnp == 0) {
        rc_pnp = fn_ppsa(0x7fffffff, buf_np, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ppsa-frame-no-port", "rc", (uint64_t)(uint32_t)rc_pnp,
                       "code");

    /* 4. buf null */
    int rc_pnull = -1;
    int sig_pnull = OBS_FAULT_ARM(&guard);
    if (sig_pnull == 0) {
        rc_pnull = fn_ppsa(hvo >= 0 ? hvo : 1, NULL, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ppsa-frame-null-buf", "rc", (uint64_t)(uint32_t)rc_pnull,
                       "code");

    typedef uint64_t (*fn_vo_n01_t)(int, void *, uint64_t, uint64_t, void *, uint64_t);
    fn_vo_n01_t fn_call = (fn_vo_n01_t)sym;

    /* REQ-20261010T2145Z-vo01: 10 calls back-to-back alternating arg3 (0 then 0x20) */
    uint8_t arg1_buf[256];
    uint8_t arg4_buf[4096];
    for (int i = 0; i < 10; i++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "call-back-to-back-%d", i);
        uint64_t arg3_val = (i % 2 == 1) ? 0x20ULL : 0ULL;
        memset(arg1_buf, 0xa5, sizeof(arg1_buf));
        memset(arg4_buf, 0x5a, sizeof(arg4_buf));

        uint64_t t_before = sceKernelGetProcessTime();
        uint64_t rc = 0;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_call(hvo >= 0 ? hvo : 1, arg1_buf, 0, arg3_val, arg4_buf, 0x100);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
        uint64_t t_after = sceKernelGetProcessTime();

        obs_report_measure(id, tag, "rc", rc, "code");
        obs_report_measure(id, tag, "arg3", arg3_val, "hex");
        obs_report_measure(id, tag, "usec", t_after - t_before, "time");

        size_t c1 = 0;
        for (size_t b = 0; b < sizeof(arg1_buf); b++) {
            if (arg1_buf[b] != 0xa5)
                c1++;
        }
        obs_report_measure(id, tag, "arg1-changed", (uint64_t)c1, "count");
        if (c1 > 0) {
            obs_report_bytes(id, tag, "arg1-out-64b", 0, arg1_buf, 64);
        }

        size_t c4 = 0;
        for (size_t b = 0; b < sizeof(arg4_buf); b++) {
            if (arg4_buf[b] != 0x5a)
                c4++;
        }
        obs_report_measure(id, tag, "arg4-changed", (uint64_t)c4, "count");
        if (c4 > 0) {
            obs_report_bytes(id, tag, "arg4-out-64b", 0, arg4_buf, 64);
        }
    }

    /* Second sequence of 10 calls with sceVideoOutSubmitFlip between calls */
    for (int i = 0; i < 10; i++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "call-flip-interleave-%d", i);
        if (hvo >= 0 && obs_address_is_callable((const void *)&sceVideoOutSubmitFlip)) {
            (void)sceVideoOutSubmitFlip(hvo, 0, 1, 0);
        }

        uint64_t arg3_val = (i % 2 == 1) ? 0x20ULL : 0ULL;
        memset(arg1_buf, 0xa5, sizeof(arg1_buf));
        memset(arg4_buf, 0x5a, sizeof(arg4_buf));

        uint64_t t_before = sceKernelGetProcessTime();
        uint64_t rc = 0;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_call(hvo >= 0 ? hvo : 1, arg1_buf, 0, arg3_val, arg4_buf, 0x100);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
        uint64_t t_after = sceKernelGetProcessTime();

        obs_report_measure(id, tag, "rc", rc, "code");
        obs_report_measure(id, tag, "arg3", arg3_val, "hex");
        obs_report_measure(id, tag, "usec", t_after - t_before, "time");

        size_t c1 = 0;
        for (size_t b = 0; b < sizeof(arg1_buf); b++) {
            if (arg1_buf[b] != 0xa5)
                c1++;
        }
        obs_report_measure(id, tag, "arg1-changed", (uint64_t)c1, "count");
        if (c1 > 0) {
            obs_report_bytes(id, tag, "arg1-out-64b", 0, arg1_buf, 64);
        }

        size_t c4 = 0;
        for (size_t b = 0; b < sizeof(arg4_buf); b++) {
            if (arg4_buf[b] != 0x5a)
                c4++;
        }
        obs_report_measure(id, tag, "arg4-changed", (uint64_t)c4, "count");
        if (c4 > 0) {
            obs_report_bytes(id, tag, "arg4-out-64b", 0, arg4_buf, 64);
        }
    }

    if (hvo >= 0 && obs_address_is_callable((const void *)&sceVideoOutClose)) {
        sceVideoOutClose(hvo);
    }
    return obs_pass();
}

static size_t census_build_passthrough_vs_container(uint8_t *out_buf,
                                                    size_t out_buf_size) {
    if (out_buf == NULL || out_buf_size < 384) {
        return 0;
    }
    memset(out_buf, 0, out_buf_size);

    /* 0x00: Magic "1234" */
    out_buf[0] = 0x31;
    out_buf[1] = 0x32;
    out_buf[2] = 0x33;
    out_buf[3] = 0x34;

    /* 0x04: Version 0x18 */
    *(uint32_t *)(out_buf + 0x04) = 0x18u;

    /* 0x20: sh_registers self-relative offset to 0x90 (0x90 - 0x20 = 0x70) */
    *(uint64_t *)(out_buf + 0x20) = 0x70u;

    /* 0x28: specials self-relative offset to 0x60 (0x60 - 0x28 = 0x38) */
    *(uint64_t *)(out_buf + 0x28) = 0x38u;

    /* 0x40: header_size */
    *(uint32_t *)(out_buf + 0x40) = 0x130u;

    /* 0x44: shader_size */
    *(uint32_t *)(out_buf + 0x44) = 256u;

    /* 0x4c: target ISA (0x1013 for Prospero GFX10.3) */
    *(uint32_t *)(out_buf + 0x4c) = 0x1013u;

    /* 0x58: special_sizes_bytes (40 bytes = 0x28) */
    *(uint16_t *)(out_buf + 0x58) = 0x28u;

    /* 0x5a: stage type 2 (VS) */
    out_buf[0x5a] = 2u;

    /* 0x5c: num_sh_registers */
    out_buf[0x5c] = 2u;

    /* 0x60: ShaderSpecialRegs */
    /* +0x00: ge_cntl (ShaderRegister: 0x25b, value 0x00000002) */
    *(uint32_t *)(out_buf + 0x60 + 0x00) = 0x25bu;
    *(uint32_t *)(out_buf + 0x60 + 0x04) = 0x00000002u;

    /* +0x08: vgt_shader_stages_en (ShaderRegister: 0x2d5, value with bit 25
     * PRIMGEN_PASSTHRU_EN) */
    *(uint32_t *)(out_buf + 0x60 + 0x08) = 0x2d5u;
    *(uint32_t *)(out_buf + 0x60 + 0x0c) = 0x02c12010u | (1u << 25);

    /* 0x90: SH registers sub-table for VS: reg_lo = 0xc8 */
    uint32_t *sh = (uint32_t *)(out_buf + 0x90);
    sh[0] = 0xc8u;
    sh[1] = 0u;
    sh[2] = 0xc9u;
    sh[3] = 0u;

    return 384;
}

static size_t census_build_ps_container(uint8_t *out_buf, size_t out_buf_size) {
    if (out_buf == NULL || out_buf_size < 384) {
        return 0;
    }
    memset(out_buf, 0, out_buf_size);

    /* 0x00: Magic "1234" */
    out_buf[0] = 0x31;
    out_buf[1] = 0x32;
    out_buf[2] = 0x33;
    out_buf[3] = 0x34;

    /* 0x04: Version 0x18 */
    *(uint32_t *)(out_buf + 0x04) = 0x18u;

    /* 0x20: sh_registers self-relative offset to 0x90 (0x90 - 0x20 = 0x70) */
    *(uint64_t *)(out_buf + 0x20) = 0x70u;

    /* 0x40: header_size */
    *(uint32_t *)(out_buf + 0x40) = 0x130u;

    /* 0x44: shader_size */
    *(uint32_t *)(out_buf + 0x44) = 256u;

    /* 0x4c: target ISA */
    *(uint32_t *)(out_buf + 0x4c) = 0x1013u;

    /* 0x5a: stage type 1 (PS) */
    out_buf[0x5a] = 1u;

    /* 0x5c: num_sh_registers */
    out_buf[0x5c] = 2u;

    /* 0x90: SH registers sub-table for PS: reg_lo = 0x08 */
    uint32_t *sh = (uint32_t *)(out_buf + 0x90);
    sh[0] = 0x08u;
    sh[1] = 0u;
    sh[2] = 0x09u;
    sh[3] = 0u;

    return 384;
}

static void *(*s_census_msp_malloc)(void *, size_t) = NULL;
static void (*s_census_msp_free)(void *, void *) = NULL;

static void census_font_destroy_cb(void *arg) {
    (void)arg;
}

static void *census_font_alloc(void *msp, size_t size) {
    if (s_census_msp_malloc && msp &&
        obs_address_is_callable((const void *)s_census_msp_malloc)) {
        void *p = s_census_msp_malloc(msp, size);
        if (p != NULL) {
            return p;
        }
    }
    return malloc(size);
}

static void census_font_free(void *msp, void *ptr) {
    if (s_census_msp_free && obs_address_is_callable((const void *)s_census_msp_free)) {
        s_census_msp_free(msp, ptr);
    } else {
        free(ptr);
    }
}

static void *census_create_font_library_instance(int idx, int *out_rc) {
    static _Alignas(4096) uint8_t s_c_base[2][0x80000];
    static uint8_t s_font_obj[2][0x80];
    static void *s_memIf[2][8];
    static void *s_created_lib[2] = {NULL, NULL};
    static int s_created_rc[2] = {-1, -1};

    if (idx < 0 || idx >= 2) {
        idx = 0;
    }
    if (s_created_lib[idx] != NULL) {
        if (out_rc) {
            *out_rc = s_created_rc[idx];
        }
        return s_created_lib[idx];
    }

    (void)obs_sysmodule_load_id(0x000e);
    (void)obs_sysmodule_load_id(0x0084);
    (void)obs_sysmodule_load_id(0x0098);
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x000eu);
    }
    int h_font = obs_module_open("libSceFont");
    if (h_font < 0) {
        h_font = obs_module_open("libSceFont.sprx");
    }
    int h_ft = obs_module_open("libSceFontFt");
    if (h_ft < 0) {
        h_ft = obs_module_open("libSceFontFt.sprx");
    }

    void *(*fn_select_ft)(uint64_t) = NULL;
    fn_select_ft = (void *(*)(uint64_t))census_resolve_symbol(
        "libSceFontFt", "$oM+XCzVG3oM", "sceFontSelectLibraryFt",
        (const void *)&census_import_28, (const void *)&census_named_28);
    if (!fn_select_ft && h_ft >= 0) {
        fn_select_ft =
            (void *(*)(uint64_t))obs_module_symbol(h_ft, "sceFontSelectLibraryFt");
    }

    int (*fn_create_lib)(void *, void *, uint64_t, void **) = NULL;
    fn_create_lib = (int (*)(void *, void *, uint64_t, void **))census_resolve_symbol(
        "libSceFont", "$n590hj5Oe-k", "sceFontCreateLibraryWithEdition",
        (const void *)&census_import_27, (const void *)&census_named_27);
    if (!fn_create_lib && h_font >= 0) {
        fn_create_lib = (int (*)(void *, void *, uint64_t, void **))obs_module_symbol(
            h_font, "sceFontCreateLibraryWithEdition");
    }

    int (*fn_mem_init)(void *, void *, uint32_t, void *, void *, void *, uint64_t) =
        NULL;
    fn_mem_init = (int (*)(void *, void *, uint32_t, void *, void *, void *, uint64_t))
        census_resolve_symbol("libSceFont", "$whrS4oksXc4", "sceFontMemoryInit",
                              (const void *)&census_import_font_mem_init, NULL);
    if (!fn_mem_init && h_font >= 0) {
        fn_mem_init = (int (*)(void *, void *, uint32_t, void *, void *, void *,
                               uint64_t))obs_module_symbol(h_font, "$whrS4oksXc4");
        if (!fn_mem_init) {
            fn_mem_init =
                (int (*)(void *, void *, uint32_t, void *, void *, void *,
                         uint64_t))obs_module_symbol(h_font, "sceFontMemoryInit");
        }
    }

    void *(*fn_msp_create)(const char *, void *, size_t, uint64_t) = NULL;
    fn_msp_create =
        (void *(*)(const char *, void *, size_t, uint64_t))census_resolve_symbol(
            "libSceLibcInternal", "$2nK1LwKkLyk", "sceLibcMspaceCreate",
            (const void *)&sceLibcMspaceCreate, NULL);

    s_census_msp_malloc = (void *(*)(void *, size_t))census_resolve_symbol(
        "libSceLibcInternal", "$OJjm-QOIHlI", "sceLibcMspaceMalloc",
        (const void *)&sceLibcMspaceMalloc, NULL);
    s_census_msp_free = (void (*)(void *, void *))census_resolve_symbol(
        "libSceLibcInternal", "$Vla-Z+eXlxo", "sceLibcMspaceFree",
        (const void *)&sceLibcMspaceFree, NULL);

    if (!fn_create_lib || !fn_mem_init) {
        if (out_rc) {
            *out_rc = -1;
        }
        return NULL;
    }

    void *msp = NULL;
    if (fn_msp_create && obs_address_is_callable((const void *)fn_msp_create)) {
        char msp_name[32];
        oops_snprintf(msp_name, sizeof(msp_name), "font_msp_%d", idx);
        msp = fn_msp_create(msp_name, s_c_base[idx], sizeof(s_c_base[idx]), 0);
    }

    s_memIf[idx][0] = (void *)&census_font_alloc;
    s_memIf[idx][1] = (void *)&census_font_free;
    s_memIf[idx][2] = (void *)&census_font_alloc;
    s_memIf[idx][3] = (void *)&census_font_alloc;
    s_memIf[idx][4] = (void *)&census_font_alloc;
    s_memIf[idx][5] = NULL;
    s_memIf[idx][6] = NULL;
    s_memIf[idx][7] = NULL;

    memset(s_font_obj[idx], 0, sizeof(s_font_obj[idx]));
    fn_mem_init(s_font_obj[idx], s_c_base[idx], sizeof(s_c_base[idx]), s_memIf[idx],
                msp, (void *)&census_font_destroy_cb, 0);

    void *sel0 = NULL;
    if (fn_select_ft && obs_address_is_callable((const void *)fn_select_ft)) {
        sel0 = fn_select_ft(0);
    }

    void *library = NULL;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int rc = -1;
    if (sig == 0) {
        rc = fn_create_lib(s_font_obj[idx], sel0, 0x0007001000000000ULL, &library);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    s_created_rc[idx] = rc;
    s_created_lib[idx] = library;
    if (out_rc) {
        *out_rc = rc;
    }
    return library;
}

static obs_result check_census_38(void) {
    const char *id = "200-census/libSceVideoOut/0x1643ec4c4a7c2230";
    const void *sym =
        census_resolve_symbol("libSceVideoOut", "$MCJ8SkzsQxY", "0x1643ec4c4a7c2230",
                              (const void *)&census_import_38, NULL);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceVideoOut", "$MCJ8SkzsQxY",
                              "0x1643ec4c4a7c2230", sym, NULL);

    int hvo = -1;
    int owns_hvo = 0;
    int vo_user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceInitialize)) {
        (void)sceUserServiceInitialize(NULL);
    }
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        (void)sceUserServiceGetInitialUser(&vo_user);
    }
    int rc_user = -1;
    int rc_ff = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        rc_user = sceVideoOutOpen(vo_user, OBS_VIDEO_BUS_MAIN, 0, NULL);
        obs_report_measure(id, "video-out-open-user", "rc", (uint64_t)(int64_t)rc_user,
                           "handle");
        if (rc_user > 0) {
            hvo = rc_user;
            owns_hvo = 1;
        } else {
            obs_report_measure(id, "video-out-open-user", "compositor-holds-output", 1u,
                               "bool");
            rc_ff = sceVideoOutOpen(0xff, OBS_VIDEO_BUS_MAIN, 0, NULL);
            obs_report_measure(id, "video-out-open-0xff", "rc",
                               (uint64_t)(int64_t)rc_ff, "handle");
            if (rc_ff > 0) {
                hvo = rc_ff;
                owns_hvo = 1;
            } else {
                obs_report_measure(id, "video-out-open-0xff", "compositor-holds-output",
                                   1u, "bool");
            }
        }
    }
    if (hvo <= 0) {
        hvo = obs_display_get_video_handle();
        obs_report_measure(id, "video-out-open", "compositor-holds-output", 1u, "bool");
        obs_report_measure(id, "video-out-open", "borrowed-handle",
                           (uint64_t)(int64_t)hvo, "handle");
    }
    obs_report_measure(id, "video-out-open", "hvo", (uint64_t)(int64_t)hvo, "handle");

    uint8_t p0[0x100];
    uint8_t p1[0x100];
    memset(p0, 0xcc, sizeof(p0));
    memset(p1, 0xcc, sizeof(p1));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc0 = ((uint64_t (*)(int, uint64_t, void *, uint64_t, uint64_t,
                                      void *))sym)(hvo, 0, p0, 0, 0, p1);
        obs_fault_unregister();
        obs_report_measure(id, "call-frame-arg0", "rc", rc0, "code");
        obs_report_bytes(id, "call-frame-arg0", "p0-out", 0, p0, 128);
        obs_report_bytes(id, "call-frame-arg0", "p1-out", 0, p1, 128);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-frame-arg0", "fault", (uint64_t)sig, "signal");
    }

    memset(p0, 0xcc, sizeof(p0));
    memset(p1, 0xcc, sizeof(p1));
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc1 = ((uint64_t (*)(int, uint64_t, void *, uint64_t, uint64_t,
                                      void *))sym)(hvo, 1, p0, 0, 0, p1);
        obs_fault_unregister();
        obs_report_measure(id, "call-frame-arg1", "rc", rc1, "code");
        obs_report_bytes(id, "call-frame-arg1", "p0-out", 0, p0, 128);
        obs_report_bytes(id, "call-frame-arg1", "p1-out", 0, p1, 128);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-frame-arg1", "fault", (uint64_t)sig, "signal");
    }

    /* REQ-20261013T1930Z-rf02: VideoOut calls on open handle */
    int h_vo_mod = obs_module_open("libSceVideoOut");
    if (h_vo_mod < 0)
        h_vo_mod = obs_module_open("libSceVideoOut.sprx");
    typedef int (*fn_eb_gv_t)(int, void *, int);
    fn_eb_gv_t fn_eb_gv = NULL;
    if (h_vo_mod > 0) {
        fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo_mod, "$eb-gvTYQcoY");
        if (!fn_eb_gv)
            fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo_mod, "$eb+gvTYQcoY");
        if (!fn_eb_gv)
            fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo_mod, "0x86721036bde0bf79");
    }
    if (!fn_eb_gv) {
        fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(OBS_HANDLE_SELF, "$eb-gvTYQcoY");
    }
    if (!fn_eb_gv && obs_address_is_callable(
                         (const void *)&sceVideoOutUnknown_86721036bde0bf79_static)) {
        fn_eb_gv = (fn_eb_gv_t)(void *)&sceVideoOutUnknown_86721036bde0bf79_static;
    }

    if (hvo > 0) {
        if (fn_eb_gv && obs_address_is_callable((const void *)fn_eb_gv)) {
            uint8_t buf0[0x20];
            memset(buf0, 0, sizeof(buf0));
            sig = OBS_FAULT_ARM(&guard);
            int rc0 = -1;
            if (sig == 0) {
                rc0 = fn_eb_gv(hvo, buf0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "eb-gvTYQcoY-zeroed", "rc", (uint64_t)(uint32_t)rc0,
                               "code");
            obs_report_bytes(id, "eb-gvTYQcoY-zeroed", "buf-20h", 0, buf0, 0x20);

            uint8_t bufcc[0x20];
            memset(bufcc, 0xcc, sizeof(bufcc));
            sig = OBS_FAULT_ARM(&guard);
            int rccc = -1;
            if (sig == 0) {
                rccc = fn_eb_gv(hvo, bufcc, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "eb-gvTYQcoY-0xcc", "rc", (uint64_t)(uint32_t)rccc,
                               "code");
            obs_report_bytes(id, "eb-gvTYQcoY-0xcc", "buf-20h", 0, bufcc, 0x20);
        }

        if (obs_address_is_callable((const void *)&sceVideoOutIsOutputSupported)) {
            int rc_d0a = sceVideoOutIsOutputSupported(hvo, (int)0xd000000a);
            obs_report_measure(id, "video-out", "supported-0xd000000a-rc",
                               (uint64_t)(uint32_t)rc_d0a, "code");

            int rc_0 = sceVideoOutIsOutputSupported(hvo, 0);
            obs_report_measure(id, "video-out", "supported-0-rc",
                               (uint64_t)(uint32_t)rc_0, "code");

            int rc_d00 = sceVideoOutIsOutputSupported(hvo, (int)0xd0000000);
            obs_report_measure(id, "video-out", "supported-0xd0000000-rc",
                               (uint64_t)(uint32_t)rc_d00, "code");
        }
    }

    if (owns_hvo && hvo >= 0 &&
        obs_address_is_callable((const void *)&sceVideoOutClose)) {
        sceVideoOutClose(hvo);
    }
    return obs_pass();
}

static obs_result check_census_39(void) {
    const char *id = "200-census/libSceAgc/0x8a6f69da59a5b375";
    const void *sym =
        census_resolve_symbol("libSceAgc", "$dbOlWdppb4o", "0x8a6f69da59a5b375",
                              (const void *)&census_import_39, NULL);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceAgc", "$dbOlWdppb4o", "0x8a6f69da59a5b375",
                              sym, NULL);

    uint8_t rec[0x100];
    memset(rec, 0xcc, sizeof(rec));
    uint8_t vs_hdr[384];
    census_build_passthrough_vs_container(vs_hdr, sizeof(vs_hdr));
    uint8_t ps_hdr[384];
    census_build_ps_container(ps_hdr, sizeof(ps_hdr));
    uint8_t ptr_arg[0x100];
    memset(ptr_arg, 0xcc, sizeof(ptr_arg));
    uint8_t reg_tbl[0x100];
    memset(reg_tbl, 0, sizeof(reg_tbl));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc =
            ((uint64_t (*)(void *, void *, void *, void *, void *, uint64_t))sym)(
                rec, vs_hdr, ps_hdr, ptr_arg, reg_tbl, 4);
        obs_fault_unregister();
        obs_report_measure(id, "call-real-shape", "rc", rc, "code");
        obs_report_bytes(id, "call-real-shape", "rec-out", 0, rec, 0x100);
        obs_report_bytes(id, "call-real-shape", "ptr-out", 0, ptr_arg, 64);
        obs_report_bytes(id, "call-real-shape", "reg-out", 0, reg_tbl, 0x100);

        int rec_has_2d5 = 0, rec_has_25b = 0;
        uint32_t rec_val_2d5 = 0, rec_val_25b = 0;
        for (size_t i = 0; i < 0x100 / 8; i++) {
            uint32_t r = ((uint32_t *)rec)[i * 2];
            uint32_t v = ((uint32_t *)rec)[i * 2 + 1];
            if (r == 0x2d5) {
                rec_has_2d5 = 1;
                rec_val_2d5 = v;
            }
            if (r == 0x25b) {
                rec_has_25b = 1;
                rec_val_25b = v;
            }
        }
        obs_report_measure(id, "call-real-shape", "rec-has-0x2d5",
                           (uint64_t)rec_has_2d5, "bool");
        obs_report_measure(id, "call-real-shape", "rec-val-0x2d5",
                           (uint64_t)rec_val_2d5, "hex");
        obs_report_measure(id, "call-real-shape", "rec-has-0x25b",
                           (uint64_t)rec_has_25b, "bool");
        obs_report_measure(id, "call-real-shape", "rec-val-0x25b",
                           (uint64_t)rec_val_25b, "hex");

        int reg_has_2d5 = 0, reg_has_25b = 0;
        uint32_t reg_val_2d5 = 0, reg_val_25b = 0;
        for (size_t i = 0; i < 0x100 / 8; i++) {
            uint32_t r = ((uint32_t *)reg_tbl)[i * 2];
            uint32_t v = ((uint32_t *)reg_tbl)[i * 2 + 1];
            if (r == 0x2d5) {
                reg_has_2d5 = 1;
                reg_val_2d5 = v;
            }
            if (r == 0x25b) {
                reg_has_25b = 1;
                reg_val_25b = v;
            }
        }
        obs_report_measure(id, "call-real-shape", "reg-has-0x2d5",
                           (uint64_t)reg_has_2d5, "bool");
        obs_report_measure(id, "call-real-shape", "reg-val-0x2d5",
                           (uint64_t)reg_val_2d5, "hex");
        obs_report_measure(id, "call-real-shape", "reg-has-0x25b",
                           (uint64_t)reg_has_25b, "bool");
        obs_report_measure(id, "call-real-shape", "reg-val-0x25b",
                           (uint64_t)reg_val_25b, "hex");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-real-shape", "fault", (uint64_t)sig, "signal");
    }

    const void *fn_prim_state = (const void *)&sceAgcCreatePrimState;
    if (!obs_address_is_callable(fn_prim_state)) {
        fn_prim_state = census_resolve_symbol("libSceAgc", NULL,
                                              "sceAgcCreatePrimState", NULL, NULL);
    }
    if (fn_prim_state && obs_address_is_callable(fn_prim_state)) {
        _Alignas(64) uint8_t prim_state[64];
        _Alignas(64) uint8_t sec_state[64];
        memset(prim_state, 0, sizeof(prim_state));
        memset(sec_state, 0, sizeof(sec_state));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_prim =
                ((int (*)(void *, void *, void *, const void *, int))fn_prim_state)(
                    prim_state, sec_state, NULL, vs_hdr, 4);
            obs_fault_unregister();
            obs_report_measure(id, "create-prim-state-passthrough", "rc",
                               (uint64_t)(uint32_t)rc_prim, "code");
            obs_report_bytes(id, "create-prim-state-passthrough", "prim-state-64b", 0,
                             prim_state, 64);
            obs_report_bytes(id, "create-prim-state-passthrough", "sec-state-64b", 0,
                             sec_state, 64);
        } else {
            obs_fault_unregister();
            uintptr_t fault_addr = obs_fault_last_address();
            obs_report_measure(id, "create-prim-state-passthrough", "fault",
                               (uint64_t)sig, "signal");
            obs_report_measure(id, "create-prim-state-passthrough", "fault-addr",
                               (uint64_t)fault_addr, "addr");
            const char *arg_which = "unknown";
            if (fault_addr >= (uintptr_t)prim_state &&
                fault_addr < (uintptr_t)prim_state + sizeof(prim_state)) {
                arg_which = "arg0-prim-state";
            } else if (fault_addr >= (uintptr_t)sec_state &&
                       fault_addr < (uintptr_t)sec_state + sizeof(sec_state)) {
                arg_which = "arg1-sec-state";
            } else if (fault_addr >= (uintptr_t)vs_hdr &&
                       fault_addr < (uintptr_t)vs_hdr + sizeof(vs_hdr)) {
                arg_which = "arg3-vs-hdr";
            }
            obs_report_measure(id, "create-prim-state-passthrough", "fault-arg",
                               (uint64_t)sig, arg_which);
        }
    }
    return obs_pass();
}

static obs_result check_census_40(void) {
    const char *id = "200-census/libSceAgc/0x7d86501b8094ef57";
    const void *sym =
        census_resolve_symbol("libSceAgc", "$V++UgBtQhn0", "0x7d86501b8094ef57",
                              (const void *)&census_import_40, NULL);
    if (!sym) {
        sym = census_resolve_symbol("libSceAgc", "$V--UgBtQhn0", NULL, NULL, NULL);
    }
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceAgc", "$V++UgBtQhn0", "0x7d86501b8094ef57",
                              sym, NULL);

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc1 = ((uint64_t (*)(void))sym)();
        uint64_t rc2 = ((uint64_t (*)(void))sym)();
        obs_fault_unregister();
        obs_report_measure(id, "call-twice", "rc1", rc1, "code");
        obs_report_measure(id, "call-twice", "rc2", rc2, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-twice", "fault", (uint64_t)sig, "signal");
    }
    return obs_pass();
}

static obs_result check_census_41(void) {
    const char *id = "200-census/libSceAgc/0x71040c4df8235e1d-real";
    const void *sym =
        census_resolve_symbol("libSceAgc", "$HV4j+E0MBHE", "0x71040c4df8235e1d",
                              (const void *)&census_import_20, NULL);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }

    uint8_t rec[0x100];
    memset(rec, 0, sizeof(rec));
    static const uint8_t rec_init[24] = {
        0x75, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x02, 0x00, 0x00,
        0x00, 0x40, 0x00, 0x00, 0x42, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
    memcpy(rec, rec_init, sizeof(rec_init));
    uint8_t vs_hdr[384];
    census_build_passthrough_vs_container(vs_hdr, sizeof(vs_hdr));
    uint8_t ps_hdr[384];
    census_build_ps_container(ps_hdr, sizeof(ps_hdr));

    uint8_t p1[0x100];
    uint8_t p2[0x100];
    memset(p1, 0xcc, sizeof(p1));
    memset(p2, 0xcc, sizeof(p2));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc = ((uint64_t (*)(void *, void *, void *, uint64_t, void *,
                                     void *))sym)(rec, vs_hdr, ps_hdr, 0, p1, p2);
        obs_fault_unregister();
        obs_report_measure(id, "real-shape-with-ps", "rc", rc, "code");
        obs_report_bytes(id, "real-shape-with-ps", "rec-out", 0, rec, 0x100);
        obs_report_bytes(id, "real-shape-with-ps", "p1-out", 0, p1, 64);
        obs_report_bytes(id, "real-shape-with-ps", "p2-out", 0, p2, 64);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "real-shape-with-ps", "fault", (uint64_t)sig, "signal");
    }

    memcpy(rec, rec_init, sizeof(rec_init));
    memset(p1, 0xcc, sizeof(p1));
    memset(p2, 0xcc, sizeof(p2));
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc0 = ((uint64_t (*)(void *, void *, void *, uint64_t, void *,
                                      void *))sym)(rec, vs_hdr, NULL, 0, p1, p2);
        obs_fault_unregister();
        obs_report_measure(id, "real-shape-ps0", "rc", rc0, "code");
        obs_report_bytes(id, "real-shape-ps0", "rec-out", 0, rec, 0x100);
        obs_report_bytes(id, "real-shape-ps0", "p1-out", 0, p1, 64);
        obs_report_bytes(id, "real-shape-ps0", "p2-out", 0, p2, 64);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "real-shape-ps0", "fault", (uint64_t)sig, "signal");
    }
    return obs_pass();
}

static obs_result check_census_42(void) {
    const char *id = "200-census/libSceFont/sceFontSupportSystemFonts";
    const void *sym = census_resolve_symbol(
        "libSceFont", "$SsRbbCiWoGw", "sceFontSupportSystemFonts",
        (const void *)&census_import_42, (const void *)&census_named_42);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceFont", "$SsRbbCiWoGw",
                              "sceFontSupportSystemFonts", sym, sym);

    int rc_create = -1;
    void *library = census_create_font_library_instance(0, &rc_create);
    obs_report_measure(id, "font-create-library", "rc", (uint64_t)(uint32_t)rc_create,
                       "code");
    obs_report_measure(id, "font-create-library", "library",
                       (uint64_t)(uintptr_t)library, "addr");
    obs_report_measure(id, "font-library-created", "library",
                       (uint64_t)(uintptr_t)library, "addr");
    if (library != NULL) {
        uint8_t lib_bytes[0x100];
        memset(lib_bytes, 0, sizeof(lib_bytes));
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *))sym)(library);
            obs_fault_unregister();
            obs_report_measure(id, "call-real-library", "rc", (uint64_t)(uint32_t)rc,
                               "code");
            memcpy(lib_bytes, library, sizeof(lib_bytes));
            obs_report_bytes(id, "call-real-library", "lib-out-100h", 0, lib_bytes,
                             0x100);
            int rc2 = ((int (*)(void *))sym)(library);
            obs_report_measure(id, "call-real-library-repeat", "rc",
                               (uint64_t)(uint32_t)rc2, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "call-real-library", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    return obs_pass();
}

static obs_result check_census_43(void) {
    const char *id = "200-census/libSceFont/sceFontSupportExternalFonts";
    const void *sym = census_resolve_symbol(
        "libSceFont", "$mz2iTY0MK4A", "sceFontSupportExternalFonts",
        (const void *)&census_import_43, (const void *)&census_named_43);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceFont", "$mz2iTY0MK4A",
                              "sceFontSupportExternalFonts", sym, sym);

    int rc_create = -1;
    void *library = census_create_font_library_instance(1, &rc_create);
    obs_report_measure(id, "font-create-library", "rc", (uint64_t)(uint32_t)rc_create,
                       "code");
    obs_report_measure(id, "font-create-library", "library",
                       (uint64_t)(uintptr_t)library, "addr");
    obs_report_measure(id, "font-library-created", "library",
                       (uint64_t)(uintptr_t)library, "addr");
    if (library != NULL) {
        uint8_t lib_bytes[0x100];
        memset(lib_bytes, 0, sizeof(lib_bytes));
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *, uint64_t, uint64_t))sym)(library, 0, 0);
            obs_fault_unregister();
            obs_report_measure(id, "call-real-library", "rc", (uint64_t)(uint32_t)rc,
                               "code");
            memcpy(lib_bytes, library, sizeof(lib_bytes));
            obs_report_bytes(id, "call-real-library", "lib-out-100h", 0, lib_bytes,
                             0x100);
            int rc2 = ((int (*)(void *, uint64_t, uint64_t))sym)(library, 0, 0);
            obs_report_measure(id, "call-real-library-repeat", "rc",
                               (uint64_t)(uint32_t)rc2, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "call-real-library", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    return obs_pass();
}

static obs_result check_census_44(void) {
    const char *id = "200-census/libSceSystemService/sceSystemServiceReceiveEvent";
    const void *sym = census_resolve_symbol(
        "libSceSystemService", "$656LMQSrg6U", "sceSystemServiceReceiveEvent",
        (const void *)&census_import_44, (const void *)&census_named_44);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceSystemService", "$656LMQSrg6U",
                              "sceSystemServiceReceiveEvent", sym, sym);

    uint8_t ev[0x200];
    memset(ev, 0xcc, sizeof(ev));
    obs_jmp_buf guard;

    for (int call_i = 1; call_i <= 3; call_i++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "call-repeat-%d", call_i);
        uint8_t ev_before[0x200];
        memcpy(ev_before, ev, sizeof(ev_before));
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *))sym)(ev);
            obs_fault_unregister();
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_written(id, "sceSystemServiceReceiveEvent", tag, ev_before, ev,
                               sizeof(ev));
            obs_report_bytes(id, tag, "ev-first64", 0, ev, 64);
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
    }
    return obs_pass();
}

static obs_result check_census_45(void) {
    const char *id = "200-census/libSceUserService/sceUserServiceGetForegroundUser";
    const void *sym = census_resolve_symbol(
        "libSceUserService", "$-sD02mFDBh4", "sceUserServiceGetForegroundUser",
        (const void *)&census_import_45, (const void *)&census_named_45);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceUserService", "$-sD02mFDBh4",
                              "sceUserServiceGetForegroundUser", sym, sym);

    int rc_user_init = -1;
    const void *fn_u_init = census_resolve_symbol(
        "libSceUserService", "$3oV7XpYj290", "sceUserServiceInitialize", NULL, NULL);
    if (!fn_u_init &&
        obs_address_is_callable((const void *)&sceUserServiceInitialize)) {
        fn_u_init = (const void *)&sceUserServiceInitialize;
    }
    if (fn_u_init && obs_address_is_callable(fn_u_init)) {
        rc_user_init = ((int (*)(void *))fn_u_init)(NULL);
    }
    obs_report_measure(id, "precondition-user-service-init", "rc",
                       (uint64_t)(uint32_t)rc_user_init, "code");

    int rc_pad_init = -1;
    const void *fn_pad_init = census_resolve_symbol(
        "libScePad", NULL, "scePadInit", NULL, (const void *)&census_named_47_init);
    if (!fn_pad_init && obs_address_is_callable((const void *)&scePadInit)) {
        fn_pad_init = (const void *)&scePadInit;
    }
    if (fn_pad_init && obs_address_is_callable(fn_pad_init)) {
        rc_pad_init = ((int (*)(void))fn_pad_init)();
    }
    obs_report_measure(id, "precondition-pad-init", "rc",
                       (uint64_t)(uint32_t)rc_pad_init, "code");

    int32_t init_id = -1;
    int rc_init_user = -1;
    const void *fn_u_init_user =
        census_resolve_symbol("libSceUserService", "$3oV7XpYj290",
                              "sceUserServiceGetInitialUser", NULL, NULL);
    if (!fn_u_init_user &&
        obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        fn_u_init_user = (const void *)&sceUserServiceGetInitialUser;
    }
    if (fn_u_init_user && obs_address_is_callable(fn_u_init_user)) {
        rc_init_user = ((int (*)(int32_t *))fn_u_init_user)(&init_id);
    }
    obs_report_measure(id, "get-initial-user", "rc", (uint64_t)(uint32_t)rc_init_user,
                       "code");
    obs_report_measure(id, "get-initial-user", "user-id", (uint64_t)(uint32_t)init_id,
                       "id");

    int32_t fg_id = -1;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc = ((int (*)(int32_t *))sym)(&fg_id);
        obs_fault_unregister();
        obs_report_measure(id, "get-fg-user", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "get-fg-user", "user-id", (uint64_t)(uint32_t)fg_id,
                           "id");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "get-fg-user", "fault", (uint64_t)sig, "signal");
    }

    int rc_priv = -1;
    const void *fn_priv = census_resolve_symbol(
        "libScePad", "$u1GRHp+oWoY", "scePadSetProcessPrivilege",
        (const void *)&census_import_46, (const void *)&census_named_46);
    if (fn_priv && obs_address_is_callable(fn_priv)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_priv = ((int (*)(int))fn_priv)(1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "pad-set-process-privilege-1", "rc",
                       (uint64_t)(uint32_t)rc_priv, "code");

    int32_t target_user = (init_id >= 0) ? init_id : fg_id;

    int h_pad = -1;
    const void *fn_pad_handle = census_resolve_symbol(
        "libScePad", NULL, "scePadGetHandle", NULL, (const void *)&census_named_47);
    if (fn_pad_handle && obs_address_is_callable(fn_pad_handle)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            h_pad = ((int (*)(int32_t, int, int))fn_pad_handle)(target_user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "pad-get-handle", "rc", (uint64_t)(uint32_t)h_pad, "code");
    obs_report_measure(id, "pad-get-handle", "user-id", (uint64_t)(uint32_t)target_user,
                       "id");

    int h_kb = -1;
    const void *fn_kb_open = census_resolve_symbol(
        "libSceKeyboard", "$HJ+KnEHcaxI", "sceKeyboardOpen",
        (const void *)&census_import_48, (const void *)&census_named_48);
    if (fn_kb_open && obs_address_is_callable(fn_kb_open)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            h_kb = ((int (*)(int32_t, int, int, void *))fn_kb_open)(target_user, 0, 0,
                                                                    NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "keyboard-open", "rc", (uint64_t)(uint32_t)h_kb, "code");
    obs_report_measure(id, "keyboard-open", "user-id", (uint64_t)(uint32_t)target_user,
                       "id");

    return obs_pass();
}

static obs_result check_census_46(void) {
    const char *id = "200-census/libScePad/scePadSetProcessPrivilege";
    const void *sym = census_resolve_symbol(
        "libScePad", "$u1GRHp+oWoY", "scePadSetProcessPrivilege",
        (const void *)&census_import_46, (const void *)&census_named_46);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libScePad", "$u1GRHp+oWoY",
                              "scePadSetProcessPrivilege", sym, sym);

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc = ((int (*)(int))sym)(1);
        obs_fault_unregister();
        obs_report_measure(id, "set-privilege-1", "rc", (uint64_t)(uint32_t)rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "set-privilege-1", "fault", (uint64_t)sig, "signal");
    }
    return obs_pass();
}

static obs_result check_census_47(void) {
    const char *id = "200-census/libScePad/scePadGetHandle";
    const void *sym = census_resolve_symbol("libScePad", NULL, "scePadGetHandle", NULL,
                                            (const void *)&census_named_47);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libScePad", NULL, "scePadGetHandle", sym, sym);

    int32_t fg_user = 0;
    const void *fn_fg = census_resolve_symbol(
        "libSceUserService", "$-sD02mFDBh4", "sceUserServiceGetForegroundUser",
        (const void *)&census_import_45, (const void *)&census_named_45);
    if (fn_fg && obs_address_is_callable(fn_fg)) {
        ((int (*)(int32_t *))fn_fg)(&fg_user);
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int h_before = ((int (*)(int32_t, int, int))sym)(fg_user, 0, 0);
        obs_fault_unregister();
        obs_report_measure(id, "before-pad-init", "handle",
                           (uint64_t)(uint32_t)h_before, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "before-pad-init", "fault", (uint64_t)sig, "signal");
    }

    const void *fn_init = census_resolve_symbol("libScePad", NULL, "scePadInit", NULL,
                                                (const void *)&census_named_47_init);
    if (fn_init && obs_address_is_callable(fn_init)) {
        int init_rc = ((int (*)(void))fn_init)();
        obs_report_measure(id, "pad-init", "rc", (uint64_t)(uint32_t)init_rc, "code");
    }

    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int h_after = ((int (*)(int32_t, int, int))sym)(fg_user, 0, 0);
        obs_fault_unregister();
        obs_report_measure(id, "after-pad-init", "handle", (uint64_t)(uint32_t)h_after,
                           "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "after-pad-init", "fault", (uint64_t)sig, "signal");
    }
    return obs_pass();
}

static obs_result check_census_48(void) {
    const char *id = "200-census/libSceKeyboard/sceKeyboardOpen";
    const void *sym = census_resolve_symbol(
        "libSceKeyboard", "$HJ+KnEHcaxI", "sceKeyboardOpen",
        (const void *)&census_import_48, (const void *)&census_named_48);
    if (!sym || !obs_address_is_callable(sym)) {
        return obs_skip("symbol not resolved");
    }
    (void)probe_census_symbol(id, "libSceKeyboard", "$HJ+KnEHcaxI", "sceKeyboardOpen",
                              sym, sym);

    int32_t fg_user = 0;
    const void *fn_fg = census_resolve_symbol(
        "libSceUserService", "$-sD02mFDBh4", "sceUserServiceGetForegroundUser",
        (const void *)&census_import_45, (const void *)&census_named_45);
    if (fn_fg && obs_address_is_callable(fn_fg)) {
        ((int (*)(int32_t *))fn_fg)(&fg_user);
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int h_null = ((int (*)(int32_t, int, int, void *))sym)(fg_user, 0, 0, NULL);
        obs_fault_unregister();
        obs_report_measure(id, "open-null-param", "rc", (uint64_t)(uint32_t)h_null,
                           "code");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "open-null-param", "fault", (uint64_t)sig, "signal");
    }

    uint8_t param[0x200];
    memset(param, 0xcc, sizeof(param));
    uint8_t param_before[0x200];
    memcpy(param_before, param, sizeof(param_before));
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int h_poison = ((int (*)(int32_t, int, int, void *))sym)(fg_user, 0, 0, param);
        obs_fault_unregister();
        obs_report_measure(id, "open-poison-param", "rc", (uint64_t)(uint32_t)h_poison,
                           "code");
        obs_report_written(id, "sceKeyboardOpen", "poison-param", param_before, param,
                           sizeof(param));
        obs_report_bytes(id, "open-poison-param", "param-first64", 0, param, 64);
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "open-poison-param", "fault", (uint64_t)sig, "signal");
    }
    return obs_pass();
}

static obs_result check_census_generic(void) {
    const obs_check *cur = obs_get_current_check();
    if (cur == NULL) {
        return obs_skip("no current check");
    }
    const char *name = cur->symbol;
    if (cur->id != NULL) {
        const char *p = cur->id;
        const char *last_slash = NULL;
        while (*p) {
            if (*p == '/') {
                last_slash = p;
            }
            p++;
        }
        if (last_slash != NULL) {
            name = last_slash + 1;
        }
    }
    return probe_census_symbol(cur->id, cur->library, cur->symbol, name, NULL, NULL);
}

static const obs_check obs_checks_census[] = {
    {"200-census/libkernel/0x7ad80543eef3176c", "libkernel", "$bBfz7kMF2Ho",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_0, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprGetFileStat", "libkernel", "$ApkYaHb8Sek",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_1, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIds", "libkernel",
     "$i3HWvW35jao", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_2,
     OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes",
     "libkernel", "$w5fcCG+t31g", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_3, OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelCheckedReleaseDirectMemory", "libkernel",
     "$hwVSPCmp5tM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_4,
     OBS_FROM_ASSUMED},
    {"200-census/libkernel/sceKernelGetGPI", "libkernel", "$4oXYe9Xmk0Q", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_5, OBS_FROM_ASSUMED},
    {"200-census/libSceSystemService/sceSystemServiceGetHdrToneMapLuminance",
     "libSceSystemService", "$mPpPxv5CZt4", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_6, OBS_FROM_ASSUMED},
    {"200-census/libSceSystemService/sceSystemServiceGetStatus", "libSceSystemService",
     "$rPo6tV8D9bM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_7,
     OBS_FROM_ASSUMED},
    {"200-census/libSceUserService/sceUserServiceGetAgeLevel", "libSceUserService",
     "$woNpu+45RLk", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_8,
     OBS_FROM_ASSUMED},
    {"200-census/libSceUserService/sceUserServiceGetEvent", "libSceUserService",
     "$yH17Q6NWtVg", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_9,
     OBS_FROM_ASSUMED},
    {"200-census/libSceNpManager/sceNpGetAccountCountryA", "libSceNpManager",
     "$JT+t00a3TxA", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_10,
     OBS_FROM_ASSUMED},
    {"200-census/libSceAudioOut2/sceAudioOut2UserDestroy", "libSceAudioOut2",
     "$IaZXJ9M79uo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_11,
     OBS_FROM_ASSUMED},
    {"200-census/libSceIme/sceImeUpdate", "libSceIme", "$-4GCfYdNF1s", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_12, OBS_FROM_ASSUMED},
    {"200-census/libScePad/0xda809fad5f12799f", "libScePad", "$n3kSX62fgNo",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_13, OBS_FROM_ASSUMED},
    {"200-census/libSceVideoOut/sceVideoOutIsOutputSupported", "libSceVideoOut",
     "$Nv8c-Kb+DUM", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_14,
     OBS_FROM_ASSUMED},
    {"200-census/libSceSaveData_native/sceSaveDataGetSaveDataMemory2",
     "libSceSaveData_native", "$QwOO7vegnV8", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_15, OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverGetDefaultOwner", "libSceAgcDriver",
     "$F0ZXt5q0ZTA", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_16,
     OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverGetResourceRegistrationMaxNameLength",
     "libSceAgcDriver", "$uJziRsODk1c", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_17, OBS_FROM_ASSUMED},
    {"200-census/libSceAgcDriver/sceAgcDriverSetTFRing", "libSceAgcDriver",
     "$XlNp7jzGiPo", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_18,
     OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x0a4e469ba741de7d", "libSceAgc", "$fd5Bp5tGTgo",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_19, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x71040c4df8235e1d", "libSceAgc", "$HV4j+E0MBHE",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_20, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0xe486fe61994e8976", "libSceAgc", "$dolOmWH+huQ",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_21, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/ppsa04263-shader-pair", "libSceAgc", "$fd5Bp5tGTgo",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_sp01_shader_pair, OBS_FROM_SPEC},
    {"200-census/libSceAjm/sceAjmInitialize", "libSceAjm", "$7lKzdHi4X3Y", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_22, OBS_FROM_ASSUMED},
    {"200-census/libSceAmpr/0xa61453ecf71144c2", "libSceAmpr", "$wkQR9+xTFKY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_23, OBS_FROM_ASSUMED},
    {"200-census/libSceAmpr/0xb5866f7aedc94e43", "libSceAmpr", "$Q07J7XpvhrU",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_24, OBS_FROM_ASSUMED},
    {"200-census/libSceLibcInternalExt/0x78afc98fd7bb14a8", "libSceLibcInternalExt",
     "$qBS714-Jr3g", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_25,
     OBS_FROM_ASSUMED},
    {"200-census/libSceNpUniversalDataSystem/sceNpUniversalDataSystemInitialize",
     "libSceNpUniversalDataSystem", "$HngqGGyoNrI", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_census_26, OBS_FROM_ASSUMED},
    {"200-census/libSceFont/sceFontCreateLibraryWithEdition", "libSceFont",
     "$n590hj5Oe-k", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_27,
     OBS_FROM_ASSUMED},
    {"200-census/libSceFontFt/sceFontSelectLibraryFt", "libSceFontFt", "$oM+XCzVG3oM",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_28, OBS_FROM_ASSUMED},
    {"200-census/libSceUlt/sceUltConditionVariableSignal", "libSceUlt", "$zblkBXA1PCU",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_29, OBS_FROM_ASSUMED},
    {"200-census/libkernel/getuid", "libkernel", "$kg4x8Prhfxw", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_30, OBS_FROM_ASSUMED},
    {"200-census/libkernel/geteuid", "libkernel", "$tvpHe5kBO4E", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_31, OBS_FROM_ASSUMED},
    {"200-census/libkernel/getgid", "libkernel", "$AfuS23bX6kg", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_32, OBS_FROM_ASSUMED},
    {"200-census/libkernel/getegid", "libkernel", "$72rYuYoDTWk", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_33, OBS_FROM_ASSUMED},
    {"200-census/libSceNpEntitlementAccess/sceNpEntitlementAccessInitialize",
     "libSceNpEntitlementAccess", "$jO8DM8oyego", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_census_34, OBS_FROM_ASSUMED},
    {"200-census/libSceAudioOut2/sceAudioOut2Initialize", "libSceAudioOut2",
     "$g2tViFIohHE", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_35,
     OBS_FROM_ASSUMED},
    {"200-census/libSceAudioOut2/sceAudioOut2UserCreate", "libSceAudioOut2",
     "$xywYcRB7nbQ", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_36,
     OBS_FROM_ASSUMED},
    {"200-census/libSceVideoOut/0x86721036bde0bf79", "libSceVideoOut", "$eb-gvTYQcoY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_37, OBS_FROM_ASSUMED},
    {"200-census/libSceVideoOut/0x1643ec4c4a7c2230", "libSceVideoOut", "$MCJ8SkzsQxY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_38, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x8a6f69da59a5b375", "libSceAgc", "$dbOlWdppb4o",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_39, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x7d86501b8094ef57", "libSceAgc", "$V++UgBtQhn0",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_40, OBS_FROM_ASSUMED},
    {"200-census/libSceAgc/0x71040c4df8235e1d-real", "libSceAgc", "$HV4j+E0MBHE",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_41, OBS_FROM_ASSUMED},
    {"200-census/libSceFont/sceFontSupportSystemFonts", "libSceFont", "$SsRbbCiWoGw",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_42, OBS_FROM_ASSUMED},
    {"200-census/libSceFont/sceFontSupportExternalFonts", "libSceFont", "$mz2iTY0MK4A",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_43, OBS_FROM_ASSUMED},
    {"200-census/libSceSystemService/sceSystemServiceReceiveEvent",
     "libSceSystemService", "$656LMQSrg6U", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_44, OBS_FROM_ASSUMED},
    {"200-census/libSceUserService/sceUserServiceGetForegroundUser",
     "libSceUserService", "$-sD02mFDBh4", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_census_45, OBS_FROM_ASSUMED},
    {"200-census/libScePad/scePadSetProcessPrivilege", "libScePad", "$u1GRHp+oWoY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_46, OBS_FROM_ASSUMED},
    {"200-census/libScePad/scePadGetHandle", "libScePad", "scePadGetHandle",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_47, OBS_FROM_ASSUMED},
    {"200-census/libSceKeyboard/sceKeyboardOpen", "libSceKeyboard", "$HJ+KnEHcaxI",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_census_48, OBS_FROM_ASSUMED},
#include "census_checks_gen.inc"
};

const obs_section obs_section_census = {
    "200-census",
    "Commercial title import surface census (Tier 1 & Tier 2)",
    "Standard harness probe for retail title imports",
    obs_checks_census,
    OBS_COUNT(obs_checks_census),
};
