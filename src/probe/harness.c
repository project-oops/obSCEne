#include <stddef.h>
#include <stdint.h>

/* Empty unless the build names checks or sections to exclude. See
 * obs_check_is_excluded. */
#ifndef OBSCENE_EXCLUDE
#define OBSCENE_EXCLUDE ""
#endif

#include "oops/freestd.h"
#include "oops/krw.h"
#include "obscene/harness.h"
/* For `sceKernelLoadStartModule` and `sceKernelDlsym`, which `obs_module_open` uses.
 *
 * The harness did not need a platform declaration before: it runs checks, and the
 * checks called the platform. Asking whether a *library* is there is the harness's
 * business rather than any one check's - it is the guard that D226 showed was missing,
 * at the layer where the existing one could not reach - so the two calls that make it
 * possible live here. */
#include "obscene/fault.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sink.h"
#include "obscene/screen.h"
#include "obscene/sysinfo.h"

/*
 * The run loop.
 *
 * Sections run in array order, base layers first, so a failure in the video section
 * is read against a memory subsystem already known to work. Within a section, checks
 * run in order too, and a check whose prerequisites are unmet is skipped rather than
 * attempted - one broken allocator would otherwise turn every later check red and
 * bury the one real fault.
 */

/* The lowest address that could possibly hold code.
 *
 * Null is the documented answer for an unresolved weak import, and for a long time it
 * was the only one this checked for. An emulator was then found resolving symbols it
 * did not recognise to small non-null values - and a guard that only rejects null
 * accepts 1, calls it, and takes the whole run down partway through with no way to
 * tell which check did it.
 *
 * The first page is never mapped on any platform this program targets: dereferencing
 * near-null is how null-pointer bugs are caught, and a system that mapped it would
 * lose that. So an address below it is not a function, whatever the loader said.
 *
 * Deliberately generous rather than clever. The cost of rejecting a real function here
 * is one check reported absent; the cost of accepting a bad one is every check after
 * it. */
#define OBS_LOWEST_CALLABLE 0x1000UL

/* Checks named at build time as ones that take the process down.
 *
 * A conformance probe against a platform under development will meet calls that do not
 * merely fail but end the run - and everything after such a call is lost. Here that was
 * one null-path check out of seventy-nine, and it cost the eight sections behind it,
 * including the census, which is the section whose answer is most useful when
 * everything else has gone red.
 *
 * # This is not for making a report look better
 *
 * A crash is a finding, and the default build has an empty list precisely so that it
 * stays one. `docs/OUTPUT.md` treats an excluded check as a skip with a reason, so it
 * is visible in the report and a diff sees it stop being run (Skip ranks below Fail).
 * Excluding a check to hide a failure would show up as exactly that.
 *
 * What it is for is the second run: once a crash is known and recorded, excluding it
 * buys the rest of the suite. Both runs are worth having and they answer different
 * questions.
 *
 * Set with `make module EXCLUDE="010-kernel/foo 040-file/bar"`. An entry with no `/`
 * names a whole section - `EXCLUDE="035-libc"` - for a loader that fails a layer rather
 * than a check.
 */
static int obs_id_matches(const char *id, const char *list, size_t id_len) {
    size_t at = 0;
    while (list[at] != '\0') {
        while (list[at] == ' ') {
            at++;
        }
        size_t start = at;
        int has_slash = 0;
        while (list[at] != '\0' && list[at] != ' ') {
            if (list[at] == '/') {
                has_slash = 1;
            }
            at++;
        }
        size_t len = at - start;
        if (len == 0) {
            continue;
        }
        if (len == id_len) {
            size_t i = 0;
            while (i < len && list[start + i] == id[i]) {
                i++;
            }
            if (i == len) {
                return 1;
            }
        }
        /* A bare section name excludes the whole section.
         *
         * An entry with no `/` is a section rather than a check, and matches any id
         * that begins with it and continues with `/`. The slash is required so
         * `035-libc` cannot swallow a hypothetical `035-libc-wide`; a prefix test alone
         * would.
         *
         * This exists because a loader can fail a whole layer rather than a check.
         * fpPS4 does not return from *any* of the twenty checks in `035-libc`, and the
         * exclusion walk found that one check at a time, two runs apiece - forty runs
         * to learn one fact, ending in a report whose libc section is twenty separate
         * skips.
         *
         * One entry says the true thing instead: the section does not run on this
         * loader. That is a better report as well as a cheaper sweep, because twenty
         * skips invite the reader to look for twenty causes.
         */
        if (!has_slash && id_len > len && id[len] == '/') {
            size_t i = 0;
            while (i < len && list[start + i] == id[i]) {
                i++;
            }
            if (i == len) {
                return 1;
            }
        }
    }
    return 0;
}

int obs_check_is_excluded(const char *id) {
    if (id == NULL) {
        return 0;
    }
    size_t len = 0;
    while (id[len] != '\0') {
        len++;
    }
    if (obs_id_matches(id, OBSCENE_EXCLUDE, len)) {
        return OBS_EXCLUDED_AT_BUILD;
    }
    /* The runtime half, and the one that matters.
     *
     * `OBSCENE_EXCLUDE` above is a build-time list, which means a loader that crashes
     * needs its own module - and then two reports are two programs rather than two
     * measurements. This asks the previous run instead: it announced a check and never
     * finished it, so this run skips that one and gets further. Same binary on every
     * loader, converging over repeats rather than over rebuilds. (D172) */
    return obs_resume_is_skipped(id) ? OBS_EXCLUDED_BY_PREVIOUS_RUN : 0;
}

/* How many checks the whole registry holds.
 *
 * The meta record already reports this; the screen needs it too, for a progress bar
 * that means anything before the first section finishes. Counted rather than stored,
 * because a stored count is one more thing to forget to update. */
static unsigned int obs_total_checks(void) {
    unsigned int total = 0;
    for (unsigned int s = 0; s < obs_section_count; s++) {
        total += obs_sections[s]->check_count;
    }
    return total;
}

const char *obs_provenance_name(obs_provenance from) {
    switch (from) {
    case OBS_FROM_IMPLEMENTATIONS:
        return "implementations";
    case OBS_FROM_SPEC:
        return "spec";
    case OBS_FROM_DOCUMENTED:
        return "documented";
    case OBS_FROM_DERIVED:
        return "derived";
    case OBS_FROM_HARDWARE:
        return "hardware";
    case OBS_FROM_ASSUMED:
    default:
        return "assumed";
    }
}

static uintptr_t s_plt_start = 0;
static uintptr_t s_plt_end = 0;

void obs_set_plt_bounds(uintptr_t start, uintptr_t end) {
    s_plt_start = start;
    s_plt_end = end;
}

int obs_address_is_callable(const void *address) {
    if (address == NULL) {
        return 0;
    }
    uintptr_t addr = (uintptr_t)address;
    if (addr < OBS_LOWEST_CALLABLE) {
        return 0;
    }
    /* Platform text (e.g. libkernel 0x800000000 - 0x900000000) is Execute-Only (XO) on
     * PS5. Dereferencing code bytes causes SYSTEM_XO_VIOLATION. */
    if (addr >= 0x800000000ULL && addr < 0x900000000ULL) {
        return 1;
    }
    if (s_plt_start != 0 && addr >= s_plt_start && addr < s_plt_end) {
        if (addr < s_plt_start + 16) {
            return 0;
        }
        const uint8_t *code = (const uint8_t *)address;
        if (code[0] == 0xff && code[1] == 0x25) {
            int32_t disp = *(const int32_t *)(const void *)(code + 2);
            const uint64_t *got_slot =
                (const uint64_t *)(const void *)(code + 6 + disp);
            uint64_t target = *got_slot;
            if (target == 0 || (target >= s_plt_start && target < s_plt_end) ||
                target < OBS_LOWEST_CALLABLE) {
                return 0;
            }
            return 1;
        }
        return 0;
    }
    return 1;
}

/* Where a library might be, tried in order.
 *
 * The bare name first, for a loader that searches. **No absolute path can be written
 * down here**: the sandbox prefix is randomised per boot - a crash dump showed
 * `/muU0ZXGZGP/common/lib/libkernel.sprx` - so the two below are the documented
 * locations rather than the one a title is actually given, and whether either is
 * accepted is exactly the sort of thing this program exists to find out.
 *
 * Candidates tried in order with the caller reporting the outcome is the same shape
 * `sink.c` uses for its write path and `runtime.c` for its output channel. Guessing one
 * and failing silently is what that shape exists to avoid. */
typedef struct {
    const char *prefix;
    obs_module_tier tier;
} obs_path_prefix;

static const obs_path_prefix obs_module_path_prefixes[] = {
    {"", OBS_TIER_APP},
    {"/system/common/lib/", OBS_TIER_APP},
    {"/app0/sce_module/", OBS_TIER_APP},
    {"/system_ex/app/PPSA90000/sce_module/", OBS_TIER_APP},
    {"/system_ex/common_ex/lib/", OBS_TIER_SYSTEM},
    {"/system/priv/lib/", OBS_TIER_ROOT},
};

const char *obs_module_tier_name(obs_module_tier tier) {
    switch (tier) {
    case OBS_TIER_APP:
        return "app";
    case OBS_TIER_SYSMODULE:
        return "sysmodule";
    case OBS_TIER_SYSTEM:
        return "system";
    case OBS_TIER_ROOT:
        return "root";
    case OBS_TIER_UNKNOWN:
    default:
        return "unknown";
    }
}

/* Build `<prefix><library>.sprx` into `dest`, or 0 if it will not fit.
 *
 * By hand rather than with `snprintf`, for the reason `host_main.c` gives about `atoi`:
 * this program measures that function, and a runtime that borrowed the thing it is
 * testing would be reporting on itself. Bounded at every step for the same reason. */
static int obs_module_path(char *dest, unsigned int size, const char *prefix,
                           const char *library) {
    static const char suffix[] = ".sprx";
    unsigned int at = 0;
    for (const char *p = prefix; *p != '\0'; p++) {
        if (at + 1u >= size) {
            return 0;
        }
        dest[at++] = *p;
    }
    for (const char *p = library; *p != '\0'; p++) {
        if (at + 1u >= size) {
            return 0;
        }
        dest[at++] = *p;
    }
    for (unsigned int i = 0; i + 1u < sizeof suffix; i++) {
        if (at + 1u >= size) {
            return 0;
        }
        dest[at++] = suffix[i];
    }
    dest[at] = '\0';
    return 1;
}

static int obs_library_is_unsafe(const char *library) {
    if (library == NULL) {
        return 0;
    }
#if defined(OBS_UNSAFE_LIBRARIES)
    const char *at = OBS_UNSAFE_LIBRARIES;
    while (*at != '\0') {
        while (*at == ' ')
            at++;
        const char *name = library;
        const char *scan = at;
        while (*scan != '\0' && *scan != ' ' && *name != '\0' && *scan == *name) {
            scan++;
            name++;
        }
        if (*name == '\0' && (*scan == '\0' || *scan == ' ')) {
            return 1;
        }
        while (*at != '\0' && *at != ' ')
            at++;
    }
#endif
    return 0;
}

typedef struct {
    const char *name;
    uint32_t id;
    int cached_handle;
} obs_sysmodule_id_map;

static obs_sysmodule_id_map obs_sysmodules[] = {
    {"libSceNet", 0x0001, -1},
    {"libSceHttp", 0x0002, -1},
    {"libSceSsl", 0x0003, -1},
    {"libSceUserService", 0x80000011, -1},
    {"libSceSaveData", 0x8000000f, -1},
    {"libSceSaveData_native", 0x8000000f, -1},
    {"libSceSaveData", 0x00b6, -1},
    {"libSceSaveData_native", 0x00b6, -1},
    {"libSceShareUtility", 0x010f, -1},
    {"libSceShare", 0x010f, -1},
    {"libSceAudioOut", 0, -1},
    {"libSceAudioOut2", 0, -1},
    {"libSceVoice", 0x000e, -1},
    {"libSceAppInstUtil", 0x0014, -1},
    {"libSceIme", 0x00000095, -1},
    {"libSceCamera", 0x001d, -1},
    {"libScePad", 0x80000024, -1},
    {"libSceVideoOut", 0x0028, -1},
    {"libSceVideodec2", 0x00cf, -1},
    {"libSceVideodec", 0x00cf, -1},
    {"libSceAudiodec", 0x0088, -1},
    {"libSceAudio3d", 0x00a7, -1},
    {"libSceKeyboard", 0x0106, -1},
    {"libSceMouse", 0x00a9, -1},
    {"libSceAppContent", 0x00b4, -1},
    {"libSceCommonDialog", 0, -1},
    {"libSceMsgDialog.native", 0x00a4, -1},
    {"libSceMsgDialog", 0x00a4, -1},
    {"libSceJson2", 0x00e7, -1},
    {"libSceJson", 0x0080, -1},
    {"libSceFont", 0, -1},
    {"libSceFontFt", 0, -1},
    {"libSceUlt", 0x0007, -1},
    {"libSceAjm", 0x80000023, -1},
    {"libSceNpManager", 0x009d, -1},
    {"libSceSigninDialog", 0x00e4, -1},
    {"libSceFont.native", 0, -1},
    {"libSceNpUniversalDataSystem", 0x0105, -1},
    {"libSceNpGameIntent", 0x011c, -1},
    {"libSceNpTrophy2", 0x0120, -1},
    {"libSceNpSessionSignaling", 0, -1},
    {"libSceNpEntitlementAccess", 0, -1},
    {"libSceLibcInternal", 0, -1},
    {"libSceLibcInternalExt", 0, -1},
    {"libScePngDec", 0x008c, -1},
    {"libScePngEnc", 0x008d, -1},
    {"libSceSaveDataDialog.native", 0x00a0, -1},
    {"libSceSaveDataDialog", 0x00a0, -1},
    {"libSceErrorDialog", 0x00ac, -1},
    {"libSceImeDialog", 0x0096, -1},
    {"libSceWebBrowserDialog", 0x00ab, -1},
    {"libScePlayerInvitationDialog", 0x00a2, -1},
    {"libScePlayerSelectionDialog", 0, -1},
    {"libSceSaveData", 0x00b6, -1},
    {"libSceSaveData_native", 0x00b6, -1},
};

void obs_sysmodule_cache_handle(const char *name, int handle) {
    if (name == NULL || handle <= 1) {
        return;
    }
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_strcmp(name, obs_sysmodules[i].name) == 0) {
            obs_sysmodules[i].cached_handle = handle;
        }
    }
}

void obs_sysmodule_cache_handle_by_id(uint32_t id, int handle) {
    if (handle <= 1) {
        return;
    }
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_sysmodules[i].id == id) {
            obs_sysmodules[i].cached_handle = handle;
        }
    }
}

static const void *obs_krw_find_symbol(const char *nid11, int *handle_out);

static int (*s_sysmodule_load_internal)(uint32_t) = NULL;
static int (*s_sysmodule_get_handle_internal)(uint32_t, int *) = NULL;
static int s_sysmodule_fns_initialized = 0;

static void obs_init_sysmodule_fns(void) {
    obs_boot_note("obscene: init_sysmodule_fns\n");
    if (s_sysmodule_fns_initialized) {
        return;
    }
    s_sysmodule_fns_initialized = 1;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModuleInternal)) {
        s_sysmodule_load_internal = &sceSysmoduleLoadModuleInternal;
    }
    if (obs_address_is_callable((const void *)&sceSysmoduleGetModuleHandleInternal)) {
        s_sysmodule_get_handle_internal = &sceSysmoduleGetModuleHandleInternal;
    }

    if ((s_sysmodule_load_internal == NULL ||
         s_sysmodule_get_handle_internal == NULL) &&
        obs_address_is_callable((const void *)&sceKernelDlsym)) {
        if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
            int mod_list[128];
            size_t mod_count = 0;
            if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 &&
                mod_count > 0) {
                for (size_t i = 0; i < mod_count && i < 128; i++) {
                    int mid = mod_list[i];
                    if (mid <= 1) {
                        continue;
                    }
                    void *p = NULL;
                    if (s_sysmodule_load_internal == NULL) {
                        if ((sceKernelDlsym(mid, "39iV5E1HoCk#C#A", &p) == 0 ||
                             sceKernelDlsym(mid, "39iV5E1HoCk", &p) == 0) &&
                            obs_address_is_callable(p)) {
                            s_sysmodule_load_internal = (int (*)(uint32_t))p;
                        }
                    }
                    if (s_sysmodule_get_handle_internal == NULL) {
                        if ((sceKernelDlsym(mid, "D8cuU4d72xM#C#A", &p) == 0 ||
                             sceKernelDlsym(mid, "D8cuU4d72xM", &p) == 0) &&
                            obs_address_is_callable(p)) {
                            s_sysmodule_get_handle_internal =
                                (int (*)(uint32_t, int *))p;
                        }
                    }
                    if (s_sysmodule_load_internal != NULL &&
                        s_sysmodule_get_handle_internal != NULL) {
                        obs_sysmodule_cache_handle("libSceSysmodule", mid);
                        break;
                    }
                }
            }
        }
    }

#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        int dummy_h = -1;
        if (s_sysmodule_load_internal == NULL) {
            const void *fn = obs_krw_find_symbol("39iV5E1HoCk", &dummy_h);
            if (fn != NULL && obs_address_is_callable(fn)) {
                s_sysmodule_load_internal = (int (*)(uint32_t))fn;
            }
        }
        if (s_sysmodule_get_handle_internal == NULL) {
            const void *fn = obs_krw_find_symbol("D8cuU4d72xM", &dummy_h);
            if (fn != NULL && obs_address_is_callable(fn)) {
                s_sysmodule_get_handle_internal = (int (*)(uint32_t, int *))fn;
            }
        }
    }
#endif
}

int obs_sysmodule_load_id(uint32_t id) {
    obs_init_sysmodule_fns();
    int rc = -1;
    if (s_sysmodule_load_internal != NULL) {
        rc = s_sysmodule_load_internal(id);
    } else if (obs_address_is_callable((const void *)&sceSysmoduleLoadModuleInternal)) {
        rc = sceSysmoduleLoadModuleInternal(id);
    }
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        if (id == 0x8000000f) {
            (void)sceSysmoduleLoadModule(0x00b6);
        } else if (id == 0x80000024) {
            (void)sceSysmoduleLoadModule(0x0027);
        } else if (id == 0x80000023) {
            (void)sceSysmoduleLoadModule(0x002e);
        }
        int rc2 = sceSysmoduleLoadModule((uint16_t)id);
        if (rc < 0 || rc2 == 0) {
            rc = rc2;
        }
    }
    return rc;
}

int obs_sysmodule_get_handle_id(uint32_t id, int *h_out) {
    obs_init_sysmodule_fns();
    int rc = -1;
    if (s_sysmodule_get_handle_internal != NULL) {
        rc = s_sysmodule_get_handle_internal(id, h_out);
        if (rc == 0 && h_out != NULL && *h_out > 1) {
            return 0;
        }
        if (id & 0xffff0000) {
            rc = s_sysmodule_get_handle_internal((uint16_t)id, h_out);
            if (rc == 0 && h_out != NULL && *h_out > 1) {
                return 0;
            }
        } else {
            rc = s_sysmodule_get_handle_internal(0x80000000U | id, h_out);
            if (rc == 0 && h_out != NULL && *h_out > 1) {
                return 0;
            }
        }
    }
    if (obs_address_is_callable((const void *)&sceSysmoduleGetModuleHandleInternal)) {
        rc = sceSysmoduleGetModuleHandleInternal(id, h_out);
        if (rc == 0 && h_out != NULL && *h_out > 1) {
            return 0;
        }
        if (id & 0xffff0000) {
            rc = sceSysmoduleGetModuleHandleInternal((uint16_t)id, h_out);
            if (rc == 0 && h_out != NULL && *h_out > 1) {
                return 0;
            }
        } else {
            rc = sceSysmoduleGetModuleHandleInternal(0x80000000U | id, h_out);
            if (rc == 0 && h_out != NULL && *h_out > 1) {
                return 0;
            }
        }
    }
    return rc;
}

const void *obs_dlsym_probe_handle(int (*fn_dlsym)(int, const char *, void **), int h,
                                   const char *full_nid, const char *bare_nid,
                                   const char *ascii_name);

static int obs_mod_name_match(const char *mod_name, const char *lib) {
    if (mod_name == NULL || lib == NULL) {
        return 0;
    }
    if (obs_strcmp(mod_name, lib) == 0) {
        return 1;
    }
    if (obs_strcmp(lib, "libSceSaveData_native") == 0 &&
        obs_strcmp(mod_name, "libSceSaveData") == 0) {
        return 1;
    }
    if (obs_strcmp(lib, "libSceAudioOut2") == 0 &&
        obs_strcmp(mod_name, "libSceAudioOut") == 0) {
        return 1;
    }
    size_t i = 0;
    while (mod_name[i] != '\0' && lib[i] != '\0') {
        char c1 = mod_name[i];
        char c2 = lib[i];
        if (c1 != c2 && !((c1 == '.' && c2 == '_') || (c1 == '_' && c2 == '.'))) {
            return 0;
        }
        i++;
    }
    if (mod_name[i] == '\0' && lib[i] == '\0') {
        return 1;
    }
    if (lib[i] == '\0' && (mod_name[i] == '.' || mod_name[i] == '\0')) {
        return 1;
    }
    return 0;
}

int obs_find_module_handle_by_library(const char *library) {
    if (library == NULL) {
        return -1;
    }
    const char *lookup_lib = library;
    if (obs_strcmp(library, "libSceAudioOut2") == 0) {
        lookup_lib = "libSceAudioOut";
    }
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_strcmp(library, obs_sysmodules[i].name) == 0 ||
            obs_strcmp(lookup_lib, obs_sysmodules[i].name) == 0) {
            int ch = obs_sysmodules[i].cached_handle;
            if (ch > 1) {
                return ch;
            }
        }
    }
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        int mod_list[128];
        size_t mod_count = 0;
        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 && mod_count > 0) {
            if (obs_address_is_callable((const void *)&sceKernelGetModuleInfo)) {
                for (size_t i = 0; i < mod_count && i < 128; i++) {
                    int mod_id = mod_list[i];
                    if (mod_id <= 1)
                        continue;
                    unsigned char info[512];
                    for (size_t k = 0; k < sizeof(info); k++)
                        info[k] = 0;
                    *(size_t *)info = sizeof(info);
                    if (sceKernelGetModuleInfo(mod_id, info) == 0) {
                        const char *mod_name = (const char *)(info + 8);
                        if (obs_mod_name_match(mod_name, library)) {
                            obs_sysmodule_cache_handle(library, mod_id);
                            return mod_id;
                        }
                    }
                }
            }
            if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
                static const struct {
                    const char *lib;
                    const char *sig;
                    const char *alt_sig;
                } sig_map[] = {
                    {"libScePad", "q1cHNfGycLI#H#I", "scePadRead"},
                    {"libSceSigninDialog", "2m077aeC+PA", "sceSigninDialogGetStatus"},
                    {"libSceSaveData", "0z45PIH+SNI#T#U", "sceSaveDataMount2"},
                    {"libSceSaveData_native", "ZP4e7rlzOUk#J#A", "ZP4e7rlzOUk#A#B"},
                    {"libSceAudioOut", "ekNvsT22rsY#G#H", "sceAudioOutInit"},
                    {"libSceAudioOut2", "g2tViFIohHE#H#A", "A89PZmqwrSQ#H#A"},
                    {"libSceIme", "-4GCfYdNF1s#b#c", "sceImeUpdate"},
                    {"libSceImeDialog", "IADmD4tScBY", "sceImeDialogGetStatus"},
                    {"libSceCommonDialog", "uoUpLGNkygk", "sceCommonDialogInitialize"},
                    {"libSceAjm", "dl+4eHSzUu4#C#A", "sceAjmInitialize"},
                    {"libSceShareUtility", "nBDD66kiFW8#J#A", "sceShareInitialize"},
                    {"libSceShare", "nBDD66kiFW8#J#A", "sceShareInitialize"},
                    {"libSceNpUniversalDataSystem", "sjaobBgqeB4#I#A",
                     "sceNpUniversalDataSystemInitialize"},
                    {"libSceNpGameIntent", "rR9-Gx-Bzps#B#C",
                     "sceNpGameIntentInitialize"},
                    {"libSceNpTrophy2", "sUXGfNMalIo#F#A",
                     "sceNpTrophy2RegisterUnlockCallback"},
                    {"libSceNpSessionSignaling", "ysmw6J-P8Ak#R#A",
                     "sceNpSessionSignalingInitialize"},
                    {"libSceNpEntitlementAccess", "jO8DM8oyego#D#A",
                     "sceNpEntitlementAccessInitialize"},
                    {"libSceUlt", "grs2pbc2awM",
                     "sceUltUlthreadRuntimeGetWorkAreaSize"},
                    {"libScePngDec", "m0uW+8pFyaw", "scePngDecCreate"},
                    {"libScePngEnc", "7aGTPfrqT9s", "scePngEncCreate"},
                    {"libSceSaveDataDialog", "s9e3+YpRnzw",
                     "sceSaveDataDialogInitialize"},
                    {"libSceSaveDataDialog.native", "s9e3+YpRnzw",
                     "sceSaveDataDialogInitialize"},
                    {"libSceMsgDialog", "b06Hh0DPEaE#G#A", "sceMsgDialogOpen"},
                    {"libSceMsgDialog.native", "b06Hh0DPEaE#G#A", "sceMsgDialogOpen"},
                    {"libSceErrorDialog", "I88KChlynSs", "sceErrorDialogInitialize"},
                    {"libSceWebBrowserDialog", "jqb7HntFQFc",
                     "sceWebBrowserDialogInitialize"},
                    {"libSceVideodec2", "CNNRoRYd8XI", "sceVideodec2CreateDecoder"},
                    {"libSceVideodec", "CNNRoRYd8XI", "sceVideodecCreateDecoder"},
                    {"libSceVdecsw", "0moTubWCsTM", "sceVdecswQueryComputeMemoryInfo"},
                    {"libSceAvPlayer", "aS66RI0gGgo", "sceAvPlayerInit"},
                    {"libSceNgs2", "koBbCMvOKWw", "sceNgs2SystemCreate"},
                    {"libSceAudio3d", "UmCvjSmuZIw", "sceAudio3dInitialize"},
                    {"libSceAudioIn", "5NE8Sjc7VC8", "sceAudioInOpen"},
                    {"libSceVideoRecording", "7QoTYzVuzto", "sceVideoRecordingOpen"},
                    {"libSceLibcInternal", "gQX+4GDQjpM#C#A", "malloc"},
                    {"libSceLibcInternalExt", "qBS714-Jr3g#D#A", "malloc"},
                    {"libSceFont", "3G4zhgKuxE8#C#A", "sceFontRenderCharGlyphImage"},
                    {"libSceFont.native", "3G4zhgKuxE8#C#A",
                     "sceFontRenderCharGlyphImage"},
                };
                const char *sig = NULL;
                const char *alt_sig = NULL;
                for (size_t s = 0; s < OBS_COUNT(sig_map); s++) {
                    if (obs_strcmp(library, sig_map[s].lib) == 0) {
                        sig = sig_map[s].sig;
                        alt_sig = sig_map[s].alt_sig;
                        break;
                    }
                }
                if (sig != NULL) {
                    char bare_sig[16];
                    size_t blen = 0;
                    while (blen < 11 && sig[blen] != '\0' && sig[blen] != '#') {
                        bare_sig[blen] = sig[blen];
                        blen++;
                    }
                    bare_sig[blen] = '\0';
                    for (size_t k = 0; k < mod_count && k < 128; k++) {
                        int mid = mod_list[k];
                        if (mid <= 1)
                            continue;
                        const void *addr = obs_dlsym_probe_handle(
                            (int (*)(int, const char *, void **))&sceKernelDlsym, mid,
                            sig, bare_sig, alt_sig);
                        if (addr == NULL &&
                            obs_strcmp(library, "libSceAudioOut2") == 0) {
                            addr = obs_dlsym_probe_handle(
                                (int (*)(int, const char *, void **))&sceKernelDlsym,
                                mid, "6vY70ydXpiE#H#A", "6vY70ydXpiE",
                                "sceAudioOut2PortCreate");
                        }
                        if (addr != NULL && obs_address_is_callable(addr)) {
                            obs_sysmodule_cache_handle(library, mid);
                            return mid;
                        }
                    }
                    for (int mid = 1; mid <= 256; mid++) {
                        const void *addr = obs_dlsym_probe_handle(
                            (int (*)(int, const char *, void **))&sceKernelDlsym, mid,
                            sig, bare_sig, alt_sig);
                        if (addr == NULL &&
                            obs_strcmp(library, "libSceAudioOut2") == 0) {
                            addr = obs_dlsym_probe_handle(
                                (int (*)(int, const char *, void **))&sceKernelDlsym,
                                mid, "6vY70ydXpiE#H#A", "6vY70ydXpiE",
                                "sceAudioOut2PortCreate");
                        }
                        if (addr != NULL && obs_address_is_callable(addr)) {
                            obs_sysmodule_cache_handle(library, mid);
                            return mid;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

void obs_sysmodules_preload_and_cache(void) {
    obs_init_sysmodule_fns();
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_library_is_unsafe(obs_sysmodules[i].name) ||
            obs_sysmodules[i].id == 0) {
            continue;
        }
        obs_boot_note("obscene: preload ");
        obs_boot_note(obs_sysmodules[i].name);
        obs_boot_note("\n");
        (void)obs_sysmodule_load_id(obs_sysmodules[i].id);
        int h = -1;
        if (obs_sysmodule_get_handle_id(obs_sysmodules[i].id, &h) == 0 && h > 1) {
            obs_sysmodules[i].cached_handle = h;
        }
    }
    (void)obs_sysmodule_load_id(0x80000023); /* libSceAjm legacy */
    (void)obs_sysmodule_load_id(0x002e);     /* libSceAjm PS4 */
    (void)obs_sysmodule_load_id(0x0007);     /* libSceUlt */
    (void)obs_sysmodule_load_id(0x0027);     /* libSceUlt PS4 */
    (void)obs_sysmodule_load_id(0x00b6);     /* libSceSaveData PS4 */
    (void)obs_sysmodule_load_id(0x8000000f); /* libSceSaveData PS5 */
    (void)obs_sysmodule_load_id(0x80000024); /* libScePad */
    (void)obs_sysmodule_load_id(0x00e4);     /* libSceSigninDialog */
    (void)obs_sysmodule_load_id(0x0105);     /* libSceNpUniversalDataSystem */
    (void)obs_sysmodule_load_id(0x0096);     /* libSceImeDialog */
    (void)obs_sysmodule_load_id(0x0095);     /* libSceIme */
    (void)obs_sysmodule_load_id(0x00cf);     /* libSceVideodec2 */
    (void)obs_sysmodule_load_id(0x008e);     /* libSceVideodec */
    (void)obs_sysmodule_load_id(0x008c);     /* libScePngDec */
    (void)obs_sysmodule_load_id(0x00e7);     /* libSceJson2 */
    (void)obs_sysmodule_load_id(0x00a4);     /* libSceMsgDialog */

    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_sysmodules[i].cached_handle <= 1) {
            int h = obs_find_module_handle_by_library(obs_sysmodules[i].name);
            if (h > 1) {
                obs_sysmodules[i].cached_handle = h;
            }
        }
    }

    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        (void)sceKernelLoadStartModule("/system/common/lib/libSceSaveData.native.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceShare.native.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("libSceSysUtil.sprx", 0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/priv/lib/libSceFsInternalForVsh.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule(
            "/system/common/lib/libSceSaveDataDialog.native.sprx", 0, NULL, 0, NULL,
            NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceSaveDataDialog.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceMsgDialog.native.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceMsgDialog.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceWebBrowserDialog.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceUlt.sprx", 0, NULL, 0,
                                       NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceAvPlayer.native.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceAvPlayer.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceNgs2.native.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceNgs2.sprx", 0, NULL, 0,
                                       NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceAudio3d.sprx", 0, NULL,
                                       0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceAudioIn.sprx", 0, NULL,
                                       0, NULL, NULL);
        (void)sceKernelLoadStartModule(
            "/system/common/lib/libSceVideoRecording.native.sprx", 0, NULL, 0, NULL,
            NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceVideoRecording.sprx",
                                       0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("libSceMsgDialog.sprx", 0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("libSceWebBrowserDialog.sprx", 0, NULL, 0, NULL,
                                       NULL);
        (void)sceKernelLoadStartModule("libSceSaveDataDialog.sprx", 0, NULL, 0, NULL,
                                       NULL);
        (void)sceKernelLoadStartModule("libSceSaveDataDialog.native.sprx", 0, NULL, 0,
                                       NULL, NULL);
    }

    int h_sd = obs_find_module_handle_by_library("libSceSaveData_native");
    if (h_sd <= 1) {
        h_sd = obs_find_module_handle_by_library("libSceSaveData");
    }
    if (h_sd > 1) {
        obs_sysmodule_cache_handle("libSceSaveData", h_sd);
        obs_sysmodule_cache_handle("libSceSaveData_native", h_sd);
    }
    int h_sh = obs_find_module_handle_by_library("libSceShare");
    if (h_sh <= 1) {
        h_sh = obs_find_module_handle_by_library("libSceShareUtility");
    }
    if (h_sh > 1) {
        obs_sysmodule_cache_handle("libSceShare", h_sh);
        obs_sysmodule_cache_handle("libSceShareUtility", h_sh);
    }
    int h_libc = obs_find_module_handle_by_library("libSceLibcInternal");
    if (h_libc > 1) {
        obs_sysmodule_cache_handle("libSceLibcInternalExt", h_libc);
    }

    obs_refresh_kexport_table();
}

static const struct {
    const char *name;
    const char *nid;
} obs_prospero_nids[] = {
    {"_sceKernelSetThreadDtors", "rNhWz+lvOMU"},
    {"_sceKernelSetThreadAtexitCount", "pB-yGZ2nQ9o"},
    {"_sceKernelSetThreadAtexitReport", "WhCc1w3EhSI"},
    {"_sceKernelRtldSetApplicationHeapAPI", "p5EcQeEeJAE"},
    {"sceMsgDialogClose", "HTrcDKlFKuM#G#A"},
    {"sceMsgDialogGetResult", "Lr8ovHH9l6A#G#A"},
    {"sceMsgDialogGetStatus", "CWVW78Qc3fI#G#A"},
    {"sceMsgDialogInitialize", "lDqxaY1UbEo#G#A"},
    {"sceMsgDialogOpen", "b06Hh0DPEaE#G#A"},
    {"sceMsgDialogProgressBarInc", "Gc5k1qcK4fs#G#A"},
    {"sceMsgDialogProgressBarSetMsg", "6H-71OdrpXM#G#A"},
    {"sceMsgDialogProgressBarSetValue", "wTpfglkmv34#G#A"},
    {"sceMsgDialogTerminate", "ePw-kqZmelo#G#A"},
    {"sceMsgDialogUpdateStatus", "6fIC3XKt2k0#G#A"},
    {"sceFontRenderCharGlyphImage", "3G4zhgKuxE8#U#V"},
    {"sceFontRenderCharGlyphImageHorizontal", "kAenWy1Zw5o#U#V"},
    {"sceFontCreateLibraryWithEdition", "n590hj5Oe-k#U#V"},
    {"sceSaveDataMount2", "0z45PIH+SNI#T#U"},
    {"sceSaveDataMount3", "ZP4e7rlzOUk#T#U"},
    {"sceSaveDataGetMountInfo", "65VH0Qaaz6s#T#U"},
    {"sceSaveDataInitialize3", "TywrFKCoLGY#T#U"},
    {"sceSaveDataTerminate", "yKDy8S5yLA0#T#U"},
    {"sceSaveDataGetSaveDataMemory2", "QwOO7vegnV8#T#U"},
    {"sceSaveDataSetupSaveDataMemory2", "oQySEUfgXRA#T#U"},
    {"sceSigninDialogGetStatus", "2m077aeC+PA"},
    {"sceSigninDialogUpdateStatus", "Bw31liTFT3A"},
    {"sceSigninDialogInitialize", "mlYGfmqE3fQ"},
    {"sceSigninDialogTerminate", "LXlmS6PvJdU"},
    {"sceAudioOut2UserDestroy", "IaZXJ9M79uo#H#A"},
    {"sceAudioOut2PortCreate", "A89PZmqwrSQ#H#A"},
    {"IaZXJ9M79uo", "IaZXJ9M79uo#H#A"},
    {"6vY70ydXpiE", "6vY70ydXpiE#H#A"},
    {"sceImeUpdate", "-4GCfYdNF1s#b#c"},
    {"sceAjmInitialize", "dl+4eHSzUu4"},
    {"0x78afc98fd7bb14a8", "qBS714-Jr3g#D#A"},
    {"qBS714-Jr3g", "qBS714-Jr3g#D#A"},
    {"sceNpUniversalDataSystemInitialize", "sjaobBgqeB4"},
    {"s6W4Zl4Slgk", "s6W4Zl4Slgk"},
    {"sceNpUniversalDataSystem_0996125e66b8a5b3", "s6W4Zl4Slgk"},
    {"sceUltConditionVariableSignal", "JTw1cAVkuc0"},
    {"scePadOpen", "xk0AcarP3V4#H#I"},
    {"scePadRead", "q1cHNfGycLI#H#I"},
    {"scePadGetHandle", "u1GRHp+oWoY#H#I"},
    {"scePadSetLightBar", "RR4novUEENY#H#I"},
    {"scePadSetVibration", "yFVnOdGxvZY#H#I"},
    {"0z45PIH-SNI", "0z45PIH-SNI#T#U"},
    {"0kj-gTw5PtM", "0kj-gTw5PtM#T#U"},
    {"q8-aBtFHles", "q8-aBtFHles#T#U"},
    {"2m077aeC-PA", "2m077aeC-PA"},
    {"8PiCp-07bdo", "8PiCp-07bdo"},
    {"8PiCp+07bdo", "8PiCp+07bdo"},
    {"cE_FJJb1DQc", "cE_FJJb1DQc"},
    {"cE/FJJb1DQc", "cE/FJJb1DQc"},
    {"7lKzdHi4X3Y", "7lKzdHi4X3Y"},
    {"eK-Jj9e7FKg", "eK-Jj9e7FKg"},
    {"eK+Jj9e7FKg", "eK+Jj9e7FKg"},
    {"HngqGGyoNrI", "HngqGGyoNrI"},
    {"zblkBXA1PCU", "zblkBXA1PCU"},
    {"sceAudioOut2Initialize", "g2tViFIohHE#H#A"},
    {"sceAudioOut2UserCreate", "xywYcRB7nbQ#H#A"},
    {"g2tViFIohHE", "g2tViFIohHE#H#A"},
    {"xywYcRB7nbQ", "xywYcRB7nbQ#H#A"},
    {"getuid", "kg4x8Prhfxw"},
    {"geteuid", "tvpHe5kBO4E"},
    {"getgid", "AfuS23bX6kg"},
    {"getegid", "72rYuYoDTWk"},
    {"sceNpEntitlementAccessInitialize", "jO8DM8oyego"},
    {"jO8DM8oyego", "jO8DM8oyego"},
    {"0x86721036bde0bf79", "eb+gvTYQcoY"},
    {"eb+gvTYQcoY", "eb+gvTYQcoY"},
    {"0x1643ec4c4a7c2230", "MCJ8SkzsQxY"},
    {"MCJ8SkzsQxY", "MCJ8SkzsQxY"},
    {"0x8a6f69da59a5b375", "dbOlWdppb4o"},
    {"dbOlWdppb4o", "dbOlWdppb4o"},
    {"0x7d86501b8094ef57", "V++UgBtQhn0"},
    {"V++UgBtQhn0", "V++UgBtQhn0"},
    {"sceFontSupportSystemFonts", "SsRbbCiWoGw#U#V"},
    {"sceFontSupportExternalFonts", "mz2iTY0MK4A#U#V"},
    {"sceSystemServiceReceiveEvent", "656LMQSrg6U"},
    {"656LMQSrg6U", "656LMQSrg6U"},
    {"sceUserServiceGetForegroundUser", "-sD02mFDBh4"},
    {"-sD02mFDBh4", "-sD02mFDBh4"},
    {"scePadSetProcessPrivilege", "scePadSetProcessPrivilege"},
    {"sceKeyboardOpen", "HJ+KnEHcaxI"},
    {"sceKernelGetProcParam", "959qrazPIrg"},
    {"sceKernelGetSanitizerMallocReplaceExternal", "py6L8jiVAN8"},
    {"sceKernelGetSanitizerNewReplaceExternal", "bnZxYgAFeA0"},
    {"_sceKernelRtldSetApplicationHeapAPI", "p5EcQeEeJAE"},
    {"malloc_stats_fast", "KuOuD58hqn4"},
    {"sceLibcHeapGetTraceInfo", "NWtTN10cJzE"},
    {"sceKernelConfiguredFlexibleMemorySize", "n1-v6FgU7MQ"},
    {"sceKernelAvailableFlexibleMemorySize", "aNz11fnnzi4"},
    {"sceImeKeyboardOpen", "eaFXjfJv3xs"},
    {"sceImeKeyboardClose", "PMVehSlfZ94"},
    {"sceSystemServiceGetNoticeScreenSkipFlag", "3RQ5aQfnstU"},
    {"sceKernelConvertLocaltimeToUtc", "0NTHN1NKONI"},
    {"sceUserServiceGetAccessibilityVibration", "qWYHOFwqCxY"},
    {"sceUserServiceGetAccessibilityTriggerEffect", "-3Y5GO+-i78"},
    {"sceAppContentDownloadDataGetAvailableSpaceKb", "Gl6w5i0JokY"},
    {"sceNpGetAccountCountryA", "JT+t00a3TxA"},
    {"sceAjmModuleRegister", "Q3dyFuwGn64"},
    {"Q3dyFuwGn64", "Q3dyFuwGn64"},
    {"rp8G7BZyd0M", "rp8G7BZyd0M"},
    {"sceAjmFinalize", "MHur6qCsUus"},
    {"MHur6qCsUus", "MHur6qCsUus"},
    {"61KsoOqrezA", "61KsoOqrezA"},
    {"sceSystemServiceParamGetInt", "riz76fI4mn0"},
    {"riz76fI4mn0", "riz76fI4mn0"},
    {"0x6dd715e95973a4fc", "-KRzWekV120"},
    {"-KRzWekV120", "-KRzWekV120"},
    {"sceAgcDriverSetTFRing", "XlNp7jzGiPo"},
    {"XlNp7jzGiPo", "XlNp7jzGiPo"},
    {"sceCesRefersUcsProfileCp1252", "LPzYZ+FR0BI"},
    {"LPzYZ+FR0BI", "LPzYZ+FR0BI"},
    {"sceCesSbcToUtf8", "xTd54EEL1Ao"},
    {"xTd54EEL1Ao", "xTd54EEL1Ao"},
    {"sceCesUtf8ToSbc", "3Q1gOWWarcw"},
    {"3Q1gOWWarcw", "3Q1gOWWarcw"},
    {"sceAudioOut2MasteringInit", "XHl38ZNknbs"},
    {"XHl38ZNknbs", "XHl38ZNknbs"},
    {"sceAudioOut2MasteringSetParam", "v8iOE+j8a5o"},
    {"v8iOE+j8a5o", "v8iOE+j8a5o"},
    {"sceAudioOut2MasteringGetState", "VZidxi2cYh0"},
    {"VZidxi2cYh0", "VZidxi2cYh0"},
    {"sceAudioOut2MasteringTerm", "2bbBBOkH4CY"},
    {"2bbBBOkH4CY", "2bbBBOkH4CY"},
    {"sceAudioOut2GetSystemState", "bkBN+CMLwRc"},
    {"bkBN+CMLwRc", "bkBN+CMLwRc"},
    {"sceAudioOut2PortRegister", "Bph+nEAV4WY"},
    {"Bph+nEAV4WY", "Bph+nEAV4WY"},
    {"sceAudioOut2PortUnregister", "16Ob5ACgOYk"},
    {"16Ob5ACgOYk", "16Ob5ACgOYk"},
    {"sceRtcGetCurrentClockLocalTime", "ZPD1YOKI+Kw"},
    {"ZPD1YOKI+Kw", "ZPD1YOKI+Kw"},
    {"sceRtcGetCurrentClock", "8lfvnRMqwEM"},
    {"8lfvnRMqwEM", "8lfvnRMqwEM"},
    {"sceRtcGetCurrentClockUtc", "qZ8tGzZ-LnQ"},
    {"qZ8tGzZ-LnQ", "qZ8tGzZ-LnQ"},
    {"sceRtcGetCurrentNetworkTick", "zO9UL3qIINQ"},
    {"zO9UL3qIINQ", "zO9UL3qIINQ"},
    {"sceRtcGetTickResolution", "jMNwqYr4R-k"},
    {"jMNwqYr4R-k", "jMNwqYr4R-k"},
    {"sceRtcGetTick", "8w-H19ip48I"},
    {"8w-H19ip48I", "8w-H19ip48I"},
    {"sceRtcSetTick", "ueega6v3GUw"},
    {"ueega6v3GUw", "ueega6v3GUw"},
    {"sceRtcCheckValid", "lPEBYdVX0XQ"},
    {"lPEBYdVX0XQ", "lPEBYdVX0XQ"},
    {"sceRtcIsLeapYear", "Ug8pCwQvh0c"},
    {"Ug8pCwQvh0c", "Ug8pCwQvh0c"},
    {"sceRtcGetDaysInMonth", "3O7Ln8AqJ1o"},
    {"3O7Ln8AqJ1o", "3O7Ln8AqJ1o"},
    {"sceRtcGetDayOfWeek", "CyIK-i4XdgQ"},
    {"CyIK-i4XdgQ", "CyIK-i4XdgQ"},
    {"sceRtcGetTime_t", "BtqmpTRXHgk"},
    {"BtqmpTRXHgk", "BtqmpTRXHgk"},
    {"sceRtcSetTime_t", "bDEVVP4bTjQ"},
    {"bDEVVP4bTjQ", "bDEVVP4bTjQ"},
    {"sceRtcGetWin32FileTime", "jfRO0uTjtzA"},
    {"jfRO0uTjtzA", "jfRO0uTjtzA"},
    {"sceRtcCompareTick", "fNaZ4DbzHAE"},
    {"fNaZ4DbzHAE", "fNaZ4DbzHAE"},
    {"sceRtcConvertUtcToLocalTime", "M1TvFst-jrM"},
    {"M1TvFst-jrM", "M1TvFst-jrM"},
    {"sceRtcConvertLocalTimeToUtc", "8Yr143yEnRo"},
    {"8Yr143yEnRo", "8Yr143yEnRo"},
    {"sceRtcTickAddMonths", "CL6y9q-XbuQ"},
    {"CL6y9q-XbuQ", "CL6y9q-XbuQ"},
    {"sceRtcTickAddYears", "-5y2uJ62qS8"},
    {"-5y2uJ62qS8", "-5y2uJ62qS8"},
    {"sceRtcFormatRFC3339", "WJ3rqFwymew"},
    {"WJ3rqFwymew", "WJ3rqFwymew"},
    {"sceRtcParseRFC3339", "99bMGglFW3I"},
    {"99bMGglFW3I", "99bMGglFW3I"},
    /* libScePngDec */
    {"scePngDecCreate", "m0uW+8pFyaw"},
    {"m0uW+8pFyaw", "m0uW+8pFyaw"},
    {"scePngDecDecode", "WC216DD3El4"},
    {"WC216DD3El4", "WC216DD3El4"},
    {"scePngDecDelete", "QbD+eENEwo8"},
    {"QbD+eENEwo8", "QbD+eENEwo8"},
    {"scePngDecParseHeader", "U6h4e5JRPaQ"},
    {"U6h4e5JRPaQ", "U6h4e5JRPaQ"},
    {"scePngDecQueryMemorySize", "-6srIGbLTIU"},
    {"-6srIGbLTIU", "-6srIGbLTIU"},
    /* libScePngEnc */
    {"scePngEncCreate", "7aGTPfrqT9s"},
    {"7aGTPfrqT9s", "7aGTPfrqT9s"},
    {"scePngEncDelete", "RUrWdwTWZy8"},
    {"RUrWdwTWZy8", "RUrWdwTWZy8"},
    {"scePngEncEncode", "xgDjJKpcyHo"},
    {"xgDjJKpcyHo", "xgDjJKpcyHo"},
    {"scePngEncQueryMemorySize", "9030RnBDoh4"},
    {"9030RnBDoh4", "9030RnBDoh4"},
    /* libSceSaveDataDialog */
    {"sceSaveDataDialogClose", "fH46Lag88XY"},
    {"fH46Lag88XY", "fH46Lag88XY"},
    {"sceSaveDataDialogGetResult", "yEiJ-qqr6Cg"},
    {"yEiJ-qqr6Cg", "yEiJ-qqr6Cg"},
    {"sceSaveDataDialogGetStatus", "ERKzksauAJA"},
    {"ERKzksauAJA", "ERKzksauAJA"},
    {"sceSaveDataDialogInitialize", "s9e3+YpRnzw"},
    {"s9e3+YpRnzw", "s9e3+YpRnzw"},
    {"sceSaveDataDialogIsReadyToDisplay", "en7gNVnh878"},
    {"en7gNVnh878", "en7gNVnh878"},
    {"sceSaveDataDialogOpen", "4tPhsP6FpDI"},
    {"4tPhsP6FpDI", "4tPhsP6FpDI"},
    {"sceSaveDataDialogProgressBarInc", "V-uEeFKARJU"},
    {"V-uEeFKARJU", "V-uEeFKARJU"},
    {"sceSaveDataDialogProgressBarSetValue", "hay1CfTmLyA"},
    {"hay1CfTmLyA", "hay1CfTmLyA"},
    {"sceSaveDataDialogTerminate", "YuH2FA7azqQ"},
    {"YuH2FA7azqQ", "YuH2FA7azqQ"},
    {"sceSaveDataDialogUpdateStatus", "KK3Bdg1RWK0"},
    {"KK3Bdg1RWK0", "KK3Bdg1RWK0"},
    /* libSceVideodec2 */
    {"sceVideodec2QueryComputeMemoryInfo", "RnDibcGCPKw"},
    {"RnDibcGCPKw", "RnDibcGCPKw"},
    {"sceVideodec2AllocateComputeQueue", "eD+X2SmxUt4"},
    {"eD+X2SmxUt4", "eD+X2SmxUt4"},
    {"sceVideodec2ReleaseComputeQueue", "UvtA3FAiF4Y"},
    {"UvtA3FAiF4Y", "UvtA3FAiF4Y"},
    {"sceVideodec2QueryDecoderMemoryInfo", "qqMCwlULR+E"},
    {"qqMCwlULR+E", "qqMCwlULR+E"},
    {"sceVideodec2CreateDecoder", "CNNRoRYd8XI"},
    {"CNNRoRYd8XI", "CNNRoRYd8XI"},
    {"sceVideodec2DeleteDecoder", "jwImxXRGSKA"},
    {"jwImxXRGSKA", "jwImxXRGSKA"},
    {"sceVideodec2Decode", "852F5+q6+iM"},
    {"852F5+q6+iM", "852F5+q6+iM"},
    {"sceVideodec2Flush", "l1hXwscLuCY"},
    {"l1hXwscLuCY", "l1hXwscLuCY"},
    {"sceVideodec2Reset", "wJXikG6QFN8"},
    {"wJXikG6QFN8", "wJXikG6QFN8"},
    {"sceVideodec2MapDirectMemory", "hIIZXUsMeI8"},
    {"hIIZXUsMeI8", "hIIZXUsMeI8"},
    {"sceVideodec2GetPictureInfo", "NtXRa3dRzU0"},
    {"NtXRa3dRzU0", "NtXRa3dRzU0"},
    {"sceVideodec2GetAvcPictureInfo", "kjrLbcyhEiw"},
    {"kjrLbcyhEiw", "kjrLbcyhEiw"},
    /* libSceVdecsw */
    {"sceVdecswQueryComputeMemoryInfo", "0moTubWCsTM"},
    {"0moTubWCsTM", "0moTubWCsTM"},
    {"sceVdecswQueryDecoderMemoryInfo", "A+2M7EivuOU"},
    {"A+2M7EivuOU", "A+2M7EivuOU"},
    {"sceVdecswCreateDecoder", "+L5ArV1tPGA"},
    {"+L5ArV1tPGA", "+L5ArV1tPGA"},
    {"sceVdecswDeleteDecoder", "ecUtPX+dBYk"},
    {"ecUtPX+dBYk", "ecUtPX+dBYk"},
    {"sceVdecswDecode", "WOL2D8k4Hm4"},
    {"WOL2D8k4Hm4", "WOL2D8k4Hm4"},
    /* libSceUlt */
    {"sceUltUlthreadRuntimeGetWorkAreaSize", "grs2pbc2awM"},
    {"grs2pbc2awM", "grs2pbc2awM"},
    {"sceUltUlthreadRuntimeInit", "4fDQRY2HTTI"},
    {"4fDQRY2HTTI", "4fDQRY2HTTI"},
    {"sceUltUlthreadRuntimeTerm", "tEuKrfYffyw"},
    {"tEuKrfYffyw", "tEuKrfYffyw"},
    {"sceUltUlthreadRuntimeResourcePoolGetWorkAreaSize", "F-4XgwbTwss"},
    {"F-4XgwbTwss", "F-4XgwbTwss"},
    {"sceUltUlthreadRuntimeResourcePoolInit", "8ItwDu5LmYM"},
    {"8ItwDu5LmYM", "8ItwDu5LmYM"},
    {"sceUltUlthreadRuntimeResourcePoolTerm", "sUguDZ0T6oU"},
    {"sUguDZ0T6oU", "sUguDZ0T6oU"},
    {"_sceUltUlthreadCreate", "znI3q8S7KQ4"},
    {"znI3q8S7KQ4", "znI3q8S7KQ4"},
    {"sceUltUlthreadJoin", "gCeAI57LGgI"},
    {"gCeAI57LGgI", "gCeAI57LGgI"},
    {"sceUltUlthreadYield", "HFd-lpjGxJA"},
    {"HFd-lpjGxJA", "HFd-lpjGxJA"},
    {"sceUltMutexInit", "Mn3vKvM370E"},
    {"Mn3vKvM370E", "Mn3vKvM370E"},
    {"sceUltMutexLock", "8hEGkR1pfr8"},
    {"8hEGkR1pfr8", "8hEGkR1pfr8"},
    {"sceUltMutexUnlock", "h0XebKiMBtk"},
    {"h0XebKiMBtk", "h0XebKiMBtk"},
    /* MsgDialog */
    {"sceMsgDialogInitialize", "lDqxaY1UbEo"},
    {"lDqxaY1UbEo", "lDqxaY1UbEo"},
    {"sceMsgDialogTerminate", "ePw-kqZmelo"},
    {"ePw-kqZmelo", "ePw-kqZmelo"},
    {"sceMsgDialogOpen", "b06Hh0DPEaE"},
    {"b06Hh0DPEaE", "b06Hh0DPEaE"},
    {"sceMsgDialogClose", "HTrcDKlFKuM"},
    {"HTrcDKlFKuM", "HTrcDKlFKuM"},
    {"sceMsgDialogGetStatus", "CWVW78Qc3fI"},
    {"CWVW78Qc3fI", "CWVW78Qc3fI"},
    {"sceMsgDialogGetResult", "Lr8ovHH9l6A"},
    {"Lr8ovHH9l6A", "Lr8ovHH9l6A"},
    {"sceMsgDialogProgressBarSetValue", "wTpfglkmv34"},
    {"wTpfglkmv34", "wTpfglkmv34"},
    {"sceMsgDialogProgressBarInc", "Gc5k1qcK4fs"},
    {"Gc5k1qcK4fs", "Gc5k1qcK4fs"},
    {"sceMsgDialogUpdateStatus", "6fIC3XKt2k0"},
    {"6fIC3XKt2k0", "6fIC3XKt2k0"},
    /* ErrorDialog */
    {"sceErrorDialogInitialize", "I88KChlynSs"},
    {"I88KChlynSs", "I88KChlynSs"},
    {"sceErrorDialogTerminate", "9XAxK2PMwk8"},
    {"9XAxK2PMwk8", "9XAxK2PMwk8"},
    {"sceErrorDialogOpen", "M2ZF-ClLhgY"},
    {"M2ZF-ClLhgY", "M2ZF-ClLhgY"},
    {"sceErrorDialogClose", "ekXHb1kDBl0"},
    {"ekXHb1kDBl0", "ekXHb1kDBl0"},
    {"sceErrorDialogGetStatus", "t2FvHRXzgqk"},
    {"t2FvHRXzgqk", "t2FvHRXzgqk"},
    /* WebBrowserDialog */
    {"sceWebBrowserDialogInitialize", "jqb7HntFQFc"},
    {"jqb7HntFQFc", "jqb7HntFQFc"},
    {"sceWebBrowserDialogTerminate", "ocHtyBwHfys"},
    {"ocHtyBwHfys", "ocHtyBwHfys"},
    {"sceWebBrowserDialogOpen", "FraP7debcdg"},
    {"FraP7debcdg", "FraP7debcdg"},
    {"sceWebBrowserDialogClose", "PSK+Eik919Q"},
    {"PSK+Eik919Q", "PSK+Eik919Q"},
    {"sceWebBrowserDialogGetStatus", "CFTG6a8TjOU"},
    {"CFTG6a8TjOU", "CFTG6a8TjOU"},
    {"sceWebBrowserDialogUpdateStatus", "Wit4LjeoeX4"},
    {"Wit4LjeoeX4", "Wit4LjeoeX4"},
    {"sceWebBrowserDialogGetResult", "vCaW0fgVQmc"},
    {"vCaW0fgVQmc", "vCaW0fgVQmc"},
    /* CommonDialog */
    {"sceCommonDialogInitialize", "uoUpLGNkygk"},
    {"uoUpLGNkygk", "uoUpLGNkygk"},
    {"sceCommonDialogIsUsed", "BQ3tey0JmQM"},
    {"BQ3tey0JmQM", "BQ3tey0JmQM"},
    /* ImeDialog */
    {"sceImeDialogGetStatus", "IADmD4tScBY"},
    {"IADmD4tScBY", "IADmD4tScBY"},
    {"sceImeDialogGetResult", "x01jxu+vxlc"},
    {"x01jxu+vxlc", "x01jxu+vxlc"},
    {"sceImeDialogInit", "NUeBrN7hzf0"},
    {"NUeBrN7hzf0", "NUeBrN7hzf0"},
    {"sceImeDialogAbort", "oBmw4xrmfKs"},
    {"oBmw4xrmfKs", "oBmw4xrmfKs"},
    {"sceImeDialogTerm", "gyTyVn+bXMw"},
    {"gyTyVn+bXMw", "gyTyVn+bXMw"},
    {"sceImeDialogGetPanelSize", "wqsJvRXwl58"},
    {"wqsJvRXwl58", "wqsJvRXwl58"},
    /* AvPlayer */
    {"sceAvPlayerInit", "aS66RI0gGgo"},
    {"aS66RI0gGgo", "aS66RI0gGgo"},
    {"sceAvPlayerClose", "NkJwDzKmIlw"},
    {"NkJwDzKmIlw", "NkJwDzKmIlw"},
    {"sceAvPlayerAddSource", "KMcEa+rHsIo"},
    {"KMcEa+rHsIo", "KMcEa+rHsIo"},
    {"sceAvPlayerStart", "ET4Gr-Uu07s"},
    {"ET4Gr-Uu07s", "ET4Gr-Uu07s"},
    {"sceAvPlayerStop", "ZC17w3vB5Lo"},
    {"ZC17w3vB5Lo", "ZC17w3vB5Lo"},
    {"sceAvPlayerPause", "9y5v+fGN4Wk"},
    {"9y5v+fGN4Wk", "9y5v+fGN4Wk"},
    {"sceAvPlayerResume", "w5moABNwnRY"},
    /* SaveDataDialog */
    {"sceSaveDataDialogInitialize", "s9e3+YpRnzw"},
    {"s9e3+YpRnzw", "s9e3+YpRnzw"},
    {"sceSaveDataDialogOpen", "4tPhsP6FpDI"},
    {"4tPhsP6FpDI", "4tPhsP6FpDI"},
    {"sceSaveDataDialogClose", "fH46Lag88XY"},
    {"fH46Lag88XY", "fH46Lag88XY"},
    {"sceSaveDataDialogGetStatus", "ERKzksauAJA"},
    {"ERKzksauAJA", "ERKzksauAJA"},
    {"sceSaveDataDialogGetResult", "yEiJ-qqr6Cg"},
    {"yEiJ-qqr6Cg", "yEiJ-qqr6Cg"},
    {"sceSaveDataDialogTerminate", "YuH2FA7azqQ"},
    {"YuH2FA7azqQ", "YuH2FA7azqQ"},
    {"sceSaveDataDialogUpdateStatus", "KK3Bdg1RWK0"},
    {"KK3Bdg1RWK0", "KK3Bdg1RWK0"},
    {"sceSaveDataDialogIsReadyToDisplay", "en7gNVnh878"},
    {"en7gNVnh878", "en7gNVnh878"},
    {"sceSaveDataDialogProgressBarInc", "V-uEeFKARJU"},
    {"V-uEeFKARJU", "V-uEeFKARJU"},
    {"sceSaveDataDialogProgressBarSetValue", "hay1CfTmLyA"},
    {"hay1CfTmLyA", "hay1CfTmLyA"},
    /* PngDec */
    {"scePngDecCreate", "m0uW+8pFyaw"},
    {"m0uW+8pFyaw", "m0uW+8pFyaw"},
    {"scePngDecDecode", "WC216DD3El4"},
    {"WC216DD3El4", "WC216DD3El4"},
    {"scePngDecDelete", "QbD+eENEwo8"},
    {"QbD+eENEwo8", "QbD+eENEwo8"},
    {"scePngDecParseHeader", "U6h4e5JRPaQ"},
    {"U6h4e5JRPaQ", "U6h4e5JRPaQ"},
    {"scePngDecQueryMemorySize", "-6srIGbLTIU"},
    {"-6srIGbLTIU", "-6srIGbLTIU"},
    /* PngEnc */
    {"scePngEncCreate", "7aGTPfrqT9s"},
    {"7aGTPfrqT9s", "7aGTPfrqT9s"},
    {"scePngEncDelete", "RUrWdwTWZy8"},
    {"RUrWdwTWZy8", "RUrWdwTWZy8"},
    {"scePngEncEncode", "xgDjJKpcyHo"},
    {"xgDjJKpcyHo", "xgDjJKpcyHo"},
    {"scePngEncQueryMemorySize", "9030RnBDoh4"},
    {"9030RnBDoh4", "9030RnBDoh4"},
    /* Ult */
    {"_sceUltUlthreadCreate", "znI3q8S7KQ4"},
    {"znI3q8S7KQ4", "znI3q8S7KQ4"},
    {"_sceUltUlthreadRuntimeCreate", "jw9FkZBXo-g"},
    {"jw9FkZBXo-g", "jw9FkZBXo-g"},
    {"sceUltUlthreadRuntimeGetWorkAreaSize", "grs2pbc2awM"},
    {"grs2pbc2awM", "grs2pbc2awM"},
    {"sceUltUlthreadRuntimeDestroy", "-gxcs521SvA"},
    {"-gxcs521SvA", "-gxcs521SvA"},
    {"_sceUltWaitingQueueResourcePoolCreate", "YiHujOG9vXY"},
    {"YiHujOG9vXY", "YiHujOG9vXY"},
    {"sceUltWaitingQueueResourcePoolGetWorkAreaSize", "WIWV1Qd7PFU"},
    {"WIWV1Qd7PFU", "WIWV1Qd7PFU"},
    {"sceUltWaitingQueueResourcePoolDestroy", "or55417wcDk"},
    {"or55417wcDk", "or55417wcDk"},
    {"sceUltUlthreadExit", "kF--s3xZjco"},
    {"kF--s3xZjco", "kF--s3xZjco"},
    {"sceUltUlthreadJoin", "gCeAI57LGgI"},
    {"gCeAI57LGgI", "gCeAI57LGgI"},
    {"sceUltUlthreadYield", "HFd-lpjGxJA"},
    {"HFd-lpjGxJA", "HFd-lpjGxJA"},
    {"_sceUltMutexCreate", "mmt8Sa6tL6c"},
    {"mmt8Sa6tL6c", "mmt8Sa6tL6c"},
    {"sceUltMutexLock", "8hEGkR1pfr8"},
    {"8hEGkR1pfr8", "8hEGkR1pfr8"},
    {"sceUltMutexUnlock", "h0XebKiMBtk"},
    {"h0XebKiMBtk", "h0XebKiMBtk"},
    {"sceUltMutexDestroy", "jW+HnafeS3Y"},
    {"jW+HnafeS3Y", "jW+HnafeS3Y"},

    {"w5moABNwnRY", "w5moABNwnRY"},
    {"sceAvPlayerGetAudioData", "Wnp1OVcrZgk"},
    {"Wnp1OVcrZgk", "Wnp1OVcrZgk"},
    {"sceAvPlayerGetVideoData", "o3+RWnHViSg"},
    {"o3+RWnHViSg", "o3+RWnHViSg"},
    /* Ngs2 */
    {"sceNgs2SystemCreate", "koBbCMvOKWw"},
    {"koBbCMvOKWw", "koBbCMvOKWw"},
    {"sceNgs2SystemDestroy", "u-WrYDaJA3k"},
    {"u-WrYDaJA3k", "u-WrYDaJA3k"},
    {"sceNgs2RackCreate", "cLV4aiT9JpA"},
    {"cLV4aiT9JpA", "cLV4aiT9JpA"},
    {"sceNgs2RackDestroy", "lCqD7oycmIM"},
    {"lCqD7oycmIM", "lCqD7oycmIM"},
    /* Audio3d */
    {"sceAudio3dInitialize", "UmCvjSmuZIw"},
    {"UmCvjSmuZIw", "UmCvjSmuZIw"},
    {"sceAudio3dPortCreate", "UHFOgVNz0kk"},
    {"UHFOgVNz0kk", "UHFOgVNz0kk"},
    {"sceAudio3dPortDestroy", "Mw9mRQtWepY"},
    {"Mw9mRQtWepY", "Mw9mRQtWepY"},
    /* AudioIn */
    {"sceAudioInOpen", "5NE8Sjc7VC8"},
    {"5NE8Sjc7VC8", "5NE8Sjc7VC8"},
    {"sceAudioInInput", "LozEOU8+anM"},
    {"LozEOU8+anM", "LozEOU8+anM"},
    /* VideoRecording */
    {"sceVideoRecordingOpen", "7QoTYzVuzto"},
    {"7QoTYzVuzto", "7QoTYzVuzto"},
    {"sceVideoRecordingClose", "KHvkPQJDMLk"},
    {"KHvkPQJDMLk", "KHvkPQJDMLk"},
    /* Np */
    {"sceNpGetState", "eQH7nWPcAgc"},
    {"eQH7nWPcAgc", "eQH7nWPcAgc"},
    {"sceNpGetNpReachabilityState", "e-ZuhGEoeC4"},
    {"e-ZuhGEoeC4", "e-ZuhGEoeC4"},
    {"sceNpWebApi2Initialize", "+o9816YQhqQ"},
    {"+o9816YQhqQ", "+o9816YQhqQ"},
    {"sceNpWebApi2PushEventRegisterCallback", "fY3QqeNkF8k"},
    {"fY3QqeNkF8k", "fY3QqeNkF8k"},
    /* Ajm */
    {"sceAjmInitialize", "7lKzdHi4X3Y"},
    {"7lKzdHi4X3Y", "7lKzdHi4X3Y"},
    {"sceAjmFinalize", "61KsoOqrezA"},
    {"61KsoOqrezA", "61KsoOqrezA"},
    {"sceAjmModuleRegister", "rp8G7BZyd0M"},
    {"rp8G7BZyd0M", "rp8G7BZyd0M"},
    {"sceAjmModuleUnregister", "ovjVUrbDLlo"},
    {"ovjVUrbDLlo", "ovjVUrbDLlo"},
    {"sceAjmInstanceCreate", "n+Bpg6wDGgM"},
    {"n+Bpg6wDGgM", "n+Bpg6wDGgM"},
    {"sceAjmInstanceDestroy", "Gs78q7jbskU"},
    {"Gs78q7jbskU", "Gs78q7jbskU"},
    {"sceAjmBatchInitialize", "fIgQe9VFajI"},
    {"fIgQe9VFajI", "fIgQe9VFajI"},
    {"sceAjmBatchJobInitialize", "yc1xEzo2M3s"},
    {"yc1xEzo2M3s", "yc1xEzo2M3s"},
    {"sceAjmBatchJobDecode", "7D3eH4Wx1d8"},
    {"7D3eH4Wx1d8", "7D3eH4Wx1d8"},
    {"sceAjmBatchJobSetGaplessDecode", "mLW3K6YwQUo"},
    {"mLW3K6YwQUo", "mLW3K6YwQUo"},
    {"sceAjmBatchStart", "o3KloJ2f0+Y"},
    {"o3KloJ2f0+Y", "o3KloJ2f0+Y"},
    {"sceAjmBatchWait", "hsAyMHzsov4"},
    {"hsAyMHzsov4", "hsAyMHzsov4"},
    {"sceAjmBatchCancel", "sEmsR4nXUDU"},
    {"sEmsR4nXUDU", "sEmsR4nXUDU"},
    {"sceAjmBatchErrorDump", "6XEWbwUi8Fk"},
    {"6XEWbwUi8Fk", "6XEWbwUi8Fk"},
    {"sceAjmDecAt9ParseConfigData", "1t3ixYNXyuc"},
    {"1t3ixYNXyuc", "1t3ixYNXyuc"},
    /* Audiodec */
    {"sceAudiodecInitLibrary", "MntcGptsOFY"},
    {"MntcGptsOFY", "MntcGptsOFY"},
    {"sceAudiodecTermLibrary", "XQ0IZAfSmIc"},
    {"XQ0IZAfSmIc", "XQ0IZAfSmIc"},
    {"sceAudiodecCreateDecoder", "+0YWs7D1dzs"},
    {"+0YWs7D1dzs", "+0YWs7D1dzs"},
    {"sceAudiodecDeleteDecoder", "uZi9LhOZn04"},
    {"uZi9LhOZn04", "uZi9LhOZn04"},
    {"sceAudiodecClearContext", "y6LDMln9V+k"},
    {"y6LDMln9V+k", "y6LDMln9V+k"},
    {"sceAudiodecDecode", "vCDkMjDHdSg"},
    {"vCDkMjDHdSg", "vCDkMjDHdSg"},
    /* AudioIn */
    {"sceAudioInOpen", "L1Q7N0o80eQ"},
    {"L1Q7N0o80eQ", "L1Q7N0o80eQ"},
    {"sceAudioInInput", "c2o+TznEjC4"},
    {"c2o+TznEjC4", "c2o+TznEjC4"},
    {"sceAudioInClose", "ryNneGyWHiY"},
    {"ryNneGyWHiY", "ryNneGyWHiY"},
    /* Audio3d */
    {"sceAudio3dInitialize", "jGSuKY2vYFI"},
    {"jGSuKY2vYFI", "jGSuKY2vYFI"},
    {"sceAudio3dTerminate", "J+ezaEtTbVk"},
    {"J+ezaEtTbVk", "J+ezaEtTbVk"},
    {"sceAudio3dGetDefaultOpenParameters", "Ali5hjqjbyI"},
    {"Ali5hjqjbyI", "Ali5hjqjbyI"},
    {"sceAudio3dPortOpen", "AFlJTCvD4F0"},
    {"AFlJTCvD4F0", "AFlJTCvD4F0"},
    {"sceAudio3dPortClose", "KbVN5TlqJTs"},
    {"KbVN5TlqJTs", "KbVN5TlqJTs"},
    {"sceAudio3dPortFlush", "MUPgYsWr4WQ"},
    {"MUPgYsWr4WQ", "MUPgYsWr4WQ"},
    {"sceAudio3dObjectReserve", "Yycdznmt7Yw"},
    {"Yycdznmt7Yw", "Yycdznmt7Yw"},
    {"sceAudio3dObjectSetAttributes", "5WW42jeH7OI"},
    {"5WW42jeH7OI", "5WW42jeH7OI"},
    {"sceAudio3dPortAdvance", "lw0qrdSjZt8"},
    {"lw0qrdSjZt8", "lw0qrdSjZt8"},
    {"sceAudio3dPortGetQueueLevel", "YaaDbDwKpFM"},
    {"YaaDbDwKpFM", "YaaDbDwKpFM"},
    {"sceAudio3dPortPush", "VEVhZ9qd4ZY"},
    {"VEVhZ9qd4ZY", "VEVhZ9qd4ZY"},
    {"sceAudio3dPortSetAttribute", "Yq9bfUQ0uJg"},
    {"Yq9bfUQ0uJg", "Yq9bfUQ0uJg"},
    /* Ngs2 */
    {"sceNgs2SystemCreate", "bCnOywhbgJI"},
    {"bCnOywhbgJI", "bCnOywhbgJI"},
    {"sceNgs2SystemDestroy", "eQOJNmCr9bs"},
    {"eQOJNmCr9bs", "eQOJNmCr9bs"},
    {"sceNgs2SystemQueryBufferSize", "Pql5tIhAAaY"},
    {"Pql5tIhAAaY", "Pql5tIhAAaY"},
    {"sceNgs2RackCreate", "kCb9JGp4tXA"},
    {"kCb9JGp4tXA", "kCb9JGp4tXA"},
    {"sceNgs2RackDestroy", "g5icjO6DKpQ"},
    {"g5icjO6DKpQ", "g5icjO6DKpQ"},
    {"sceNgs2CalcWaveformBlock", "3pCNbVM11UA"},
    {"3pCNbVM11UA", "3pCNbVM11UA"},
    {"sceNgs2ParseWaveformData", "hyVLT2VlOYk"},
    {"hyVLT2VlOYk", "hyVLT2VlOYk"},
    {"sceNgs2RackGetVoiceHandle", "MwmHz8pAdAo"},
    {"MwmHz8pAdAo", "MwmHz8pAdAo"},
    {"sceNgs2RackQueryBufferSize", "0eFLVCfWVds"},
    {"0eFLVCfWVds", "0eFLVCfWVds"},
    {"sceNgs2SystemRender", "i0VnXM/C9fc"},
    {"i0VnXM/C9fc", "i0VnXM/C9fc"},
    {"sceNgs2SystemResetOption", "AQkj7C0f3PY"},
    {"AQkj7C0f3PY", "AQkj7C0f3PY"},
    {"sceNgs2VoiceControl", "uu94irFOGpA"},
    {"uu94irFOGpA", "uu94irFOGpA"},
    {"sceNgs2VoiceGetState", "/TOuuAQ/buE"},
    {"/TOuuAQ/buE", "/TOuuAQ/buE"},
    {"sceNgs2VoiceGetStateFlags", "rEh728kXk3w"},
    {"rEh728kXk3w", "rEh728kXk3w"},
    /* Acm */
    {"sceAcmContextCreate", "yVy3Yp-lhWQ"},
    {"yVy3Yp-lhWQ", "yVy3Yp-lhWQ"},
    {"sceAcmContextDestroy", "H9n0CIwBGIw"},
    {"H9n0CIwBGIw", "H9n0CIwBGIw"},
    {"sceAcmBatchInitialize", "YvE+byJO5lk"},
    {"YvE+byJO5lk", "YvE+byJO5lk"},
    {"sceAcmBatchStart", "9xm5cglXkwM"},
    {"9xm5cglXkwM", "9xm5cglXkwM"},
    {"sceAcmBatchWait", "sSRXGYF3s0Q"},
    {"sSRXGYF3s0Q", "sSRXGYF3s0Q"},
    /* VideoRecording */
    {"sceVideoRecordingGetStatus", "fZJQzFK4Gv4"},
    {"fZJQzFK4Gv4", "fZJQzFK4Gv4"},
    {"sceVideoRecordingQueryMemSize", "ZvWzS2wTIMc"},
    {"ZvWzS2wTIMc", "ZvWzS2wTIMc"},
    {"sceVideoRecordingSetInfo", "Fc8qxlKINYQ"},
    {"Fc8qxlKINYQ", "Fc8qxlKINYQ"},
    {"sceVideoRecordingStart", "tWoe9IlGAhs"},
    {"tWoe9IlGAhs", "tWoe9IlGAhs"},
    {"sceVideoRecordingStop", "OOFxrMY+mfI"},
    {"OOFxrMY+mfI", "OOFxrMY+mfI"},
    /* AvPlayer */
    {"sceAvPlayerAddSourceEx", "x8uvuFOPZhU"},
    {"x8uvuFOPZhU", "x8uvuFOPZhU"},
    {"sceAvPlayerPostInit", "HD1YKVU26/M"},
    {"HD1YKVU26/M", "HD1YKVU26/M"},
    {"sceAvPlayerSetLogCallback", "eBTreZ84JFY"},
    {"eBTreZ84JFY", "eBTreZ84JFY"},
    /* MsgDialog */
    {"sceMsgDialogInitialize", "lDqxaY1UbEo"},
    {"lDqxaY1UbEo", "lDqxaY1UbEo"},
    {"sceMsgDialogOpen", "b06Hh0DPEaE"},
    {"b06Hh0DPEaE", "b06Hh0DPEaE"},
    {"sceMsgDialogGetStatus", "CWVW78Qc3fI"},
    {"CWVW78Qc3fI", "CWVW78Qc3fI"},
    {"sceMsgDialogUpdateStatus", "6fIC3XKt2k0"},
    {"6fIC3XKt2k0", "6fIC3XKt2k0"},
    {"sceMsgDialogGetResult", "Lr8ovHH9l6A"},
    {"Lr8ovHH9l6A", "Lr8ovHH9l6A"},
    {"sceMsgDialogClose", "HTrcDKlFKuM"},
    {"HTrcDKlFKuM", "HTrcDKlFKuM"},
    {"sceMsgDialogTerminate", "ePw-kqZmelo"},
    {"ePw-kqZmelo", "ePw-kqZmelo"},
    {"sceMsgDialogProgressBarInc", "Gc5k1qcK4fs"},
    {"Gc5k1qcK4fs", "Gc5k1qcK4fs"},
    {"sceMsgDialogProgressBarSetValue", "wTpfglkmv34"},
    {"wTpfglkmv34", "wTpfglkmv34"},
    {"sceMsgDialogProgressBarSetMsg", "6H-71OdrpXM"},
    {"6H-71OdrpXM", "6H-71OdrpXM"},
    /* CommonDialog */
    {"sceCommonDialogInitialize", "uoUpLGNkygk"},
    {"uoUpLGNkygk", "uoUpLGNkygk"},
    {"sceCommonDialogIsUsed", "BQ3tey0JmQM"},
    {"BQ3tey0JmQM", "BQ3tey0JmQM"},
    /* ImeDialog */
    {"sceImeDialogInit", "NUeBrN7hzf0"},
    {"NUeBrN7hzf0", "NUeBrN7hzf0"},
    {"sceImeDialogGetStatus", "IADmD4tScBY"},
    {"IADmD4tScBY", "IADmD4tScBY"},
    {"sceImeDialogGetResult", "x01jxu+vxlc"},
    {"x01jxu+vxlc", "x01jxu+vxlc"},
    {"sceImeDialogAbort", "oBmw4xrmfKs"},
    {"oBmw4xrmfKs", "oBmw4xrmfKs"},
    {"sceImeDialogTerm", "gyTyVn+bXMw"},
    {"gyTyVn+bXMw", "gyTyVn+bXMw"},
    /* ErrorDialog */
    {"sceErrorDialogInitialize", "I88KChlynSs"},
    {"I88KChlynSs", "I88KChlynSs"},
    {"sceErrorDialogOpen", "M2ZF-ClLhgY"},
    {"M2ZF-ClLhgY", "M2ZF-ClLhgY"},
    {"sceErrorDialogGetStatus", "t2FvHRXzgqk"},
    {"t2FvHRXzgqk", "t2FvHRXzgqk"},
    {"sceErrorDialogUpdateStatus", "WWiGuh9XfgQ"},
    {"WWiGuh9XfgQ", "WWiGuh9XfgQ"},
    {"sceErrorDialogClose", "ekXHb1kDBl0"},
    {"ekXHb1kDBl0", "ekXHb1kDBl0"},
    {"sceErrorDialogTerminate", "9XAxK2PMwk8"},
    {"9XAxK2PMwk8", "9XAxK2PMwk8"},
    /* NP & UDS & AvPlayer */
    {"sceNpUniversalDataSystemCreateEvent", "p+GcLqwpL9M"},
    {"p+GcLqwpL9M", "p+GcLqwpL9M"},
    {"sceNpUniversalDataSystemPostEvent", "CzkKf7ahIyU"},
    {"CzkKf7ahIyU", "CzkKf7ahIyU"},
    {"sceNpUniversalDataSystemDestroyEvent", "wG+84pnNIuo"},
    {"wG+84pnNIuo", "wG+84pnNIuo"},
    {"sceNpUniversalDataSystemDestroyEventPropertyObject", "kKUH0Viib3c"},
    {"kKUH0Viib3c", "kKUH0Viib3c"},
    {"sceNpUniversalDataSystemRegisterContext", "tpFJ8LIKvPw"},
    {"tpFJ8LIKvPw", "tpFJ8LIKvPw"},
    {"sceNpEntitlementAccessInitialize", "jO8DM8oyego"},
    {"jO8DM8oyego", "jO8DM8oyego"},
    {"sceNpSessionSignalingInitialize", "ysmw6J-P8Ak"},
    {"ysmw6J-P8Ak", "ysmw6J-P8Ak"},
    {"sceNpTrophy2CreateContext", "Bagshr7OQ6Q"},
    {"Bagshr7OQ6Q", "Bagshr7OQ6Q"},
    {"sceNpTrophy2RegisterUnlockCallback", "sUXGfNMalIo"},
    {"sUXGfNMalIo", "sUXGfNMalIo"},
    {"sceNpTrophy2UnregisterUnlockCallback", "wVqxM58sIKs"},
    {"wVqxM58sIKs", "wVqxM58sIKs"},
    {"sceNpGetState", "eQH7nWPcAgc"},
    {"eQH7nWPcAgc", "eQH7nWPcAgc"},
    {"sceNpSetNpTitleId", "Ec63y59l9tw"},
    {"Ec63y59l9tw", "Ec63y59l9tw"},
    {"sceNpWebApi2Initialize", "+o9816YQhqQ"},
    {"+o9816YQhqQ", "+o9816YQhqQ"},
    {"sceNpWebApi2PushEventCreateHandle", "WV1GwM32NgY"},
    {"WV1GwM32NgY", "WV1GwM32NgY"},
    {"sceNpGameIntentInitialize", "m87BHxt-H60"},
    {"m87BHxt-H60", "m87BHxt-H60"},
    {"sceNpGameIntentTerminate", "0HBYxYAjmf0"},
    {"0HBYxYAjmf0", "0HBYxYAjmf0"},
    {"sceVideoRecordingSetInfo", "Fc8qxlKINYQ"},
    {"Fc8qxlKINYQ", "Fc8qxlKINYQ"},
    {"sceContentSearchInit", "dPj4ZtRcIWk"},
    {"dPj4ZtRcIWk", "dPj4ZtRcIWk"},
    {"sceContentDeleteInitialize", "zoxb0wEChEM"},
    {"zoxb0wEChEM", "zoxb0wEChEM"},
    {"sceAvPlayerInitEx", "o9eWRkSL+M4"},
    {"o9eWRkSL+M4", "o9eWRkSL+M4"},
    {"sceAvPlayerAddSourceEx", "x8uvuFOPZhU"},
    {"x8uvuFOPZhU", "x8uvuFOPZhU"},
    {"sceAvPlayerIsActive", "UbQoYawOsfY"},
    {"UbQoYawOsfY", "UbQoYawOsfY"},
    {"sceAvPlayerClose", "NkJwDzKmIlw"},
    {"NkJwDzKmIlw", "NkJwDzKmIlw"},
};

const char *obs_prospero_name_to_nid(const char *name) {
    if (name == NULL || *name == '\0') {
        return NULL;
    }
    const char *lookup = (name[0] == '$') ? (name + 1) : name;
    for (size_t i = 0; i < OBS_COUNT(obs_prospero_nids); i++) {
        if (obs_strcmp(name, obs_prospero_nids[i].name) == 0 ||
            obs_strcmp(lookup, obs_prospero_nids[i].name) == 0 ||
            obs_strcmp(lookup, obs_prospero_nids[i].nid) == 0) {
            return obs_prospero_nids[i].nid;
        }
        const char *pnid = obs_prospero_nids[i].nid;
        if (pnid != NULL) {
            size_t k = 0;
            while (k < 11 && lookup[k] != '\0' && lookup[k] == pnid[k]) {
                k++;
            }
            if (k == 11 && (lookup[11] == '\0' || lookup[11] == '#')) {
                return pnid;
            }
        }
    }
    return NULL;
}

static int obs_is_nid_string(const char *s) {
    if (s == NULL) {
        return 0;
    }
    if (s[0] == 's' && s[1] == 'c' && s[2] == 'e') {
        return 0;
    }
    if (s[0] == '0' && s[1] == 'x') {
        return 0;
    }
    for (size_t i = 0; i < 11; i++) {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '/' || c == '_')) {
            return 0;
        }
    }
    if (s[11] != '\0' && s[11] != '#') {
        return 0;
    }
    return 1;
}

#if !defined(OBSCENE_HOST_BUILD)
static int obs_nid_match(const char *a, const char *b, size_t n) {
    if (a == NULL || b == NULL) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca == '\0' || cb == '\0') {
            return (ca == cb);
        }
        if (ca == cb) {
            continue;
        }
        if ((ca == '+' && cb == '-') || (ca == '-' && cb == '+')) {
            continue;
        }
        if ((ca == '/' && cb == '_') || (ca == '_' && cb == '/') ||
            (ca == '/' && cb == '-') || (ca == '-' && cb == '/') ||
            (ca == '_' && cb == '-') || (ca == '-' && cb == '_')) {
            continue;
        }
        return 0;
    }
    return 1;
}
#endif

static const void *obs_krw_find_symbol(const char *nid11, int *handle_out) {
#if !defined(OBSCENE_HOST_BUILD)
    if (!krw_is_ready() || nid11 == NULL || nid11[0] == '\0') {
        return NULL;
    }
    pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    uintptr_t kproc = krw_get_proc(pid);
    if (kproc == 0) {
        return NULL;
    }
    uintptr_t kaddr = 0;
    if (krw_copyout(kproc + 0x3E8, &kaddr, sizeof(kaddr)) != 0 || kaddr == 0) {
        return NULL;
    }
    uintptr_t cur = 0;
    if (krw_copyout(kaddr, &cur, sizeof(cur)) != 0 || cur == 0) {
        return NULL;
    }
    static uint8_t s_find_tab_buf[16384];
    static uint8_t s_find_nid_buf[16384];

    int count = 0;
    while (cur != 0 && count < 256) {
        uint8_t mrec[0x180];
        if (krw_copyout(cur, mrec, sizeof(mrec)) != 0) {
            break;
        }
        uint32_t sel = *(uint32_t *)(mrec + 0x28);
        uint64_t module_base = *(uint64_t *)(mrec + 0x30);
        uintptr_t dispatch_kaddr = *(uintptr_t *)(mrec + 0x148);
        if (dispatch_kaddr != 0 && module_base != 0) {
            uint8_t dtab[0x120];
            if (krw_copyout(dispatch_kaddr, dtab, sizeof(dtab)) == 0) {
                uint64_t kaddr_table = *(uint64_t *)(dtab + 0x28);
                uint64_t table_size = *(uint64_t *)(dtab + 0x30);
                uint64_t nid_strbase = *(uint64_t *)(dtab + 0x38);
                if (kaddr_table != 0 && table_size != 0 && nid_strbase != 0) {
                    size_t copy_sz = (table_size > sizeof(s_find_tab_buf))
                                         ? sizeof(s_find_tab_buf)
                                         : (size_t)table_size;
                    if (krw_copyout((uintptr_t)kaddr_table, s_find_tab_buf, copy_sz) ==
                            0 &&
                        krw_copyout((uintptr_t)nid_strbase, s_find_nid_buf,
                                    sizeof(s_find_nid_buf)) == 0) {
                        for (size_t off_ent = 0; off_ent + 24 <= copy_sz;
                             off_ent += 24) {
                            const uint8_t *ent = s_find_tab_buf + off_ent;
                            uint32_t off = *(const uint32_t *)ent;
                            uint64_t foff = *(const uint64_t *)(ent + 8);
                            if (foff == 0 || off + 11 >= sizeof(s_find_nid_buf)) {
                                continue;
                            }
                            const char *sym_name = (const char *)(s_find_nid_buf + off);
                            if (obs_nid_match(sym_name, nid11, 11)) {
                                uintptr_t func_addr = (uintptr_t)(module_base + foff);
                                if (obs_address_is_callable((const void *)func_addr)) {
                                    if (handle_out != NULL) {
                                        *handle_out = (int)sel;
                                    }
                                    return (const void *)func_addr;
                                }
                            }
                        }
                    }
                }
            }
        }
        uintptr_t next = *(uintptr_t *)mrec;
        if (next == 0 || next == cur) {
            break;
        }
        cur = next;
        count++;
    }
#else
    (void)nid11;
    (void)handle_out;
#endif
    return NULL;
}

void obs_refresh_kexport_table(void) {
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        static obs_kexport_table_t s_refresh_kexport_table;
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        s_refresh_kexport_table.count = 0;
        s_refresh_kexport_table.capacity = OBS_KEXPORT_MAX;
        if (krw_dump_all_exports(pid, &s_refresh_kexport_table) == 0 &&
            s_refresh_kexport_table.count > 0) {
            obs_set_payload_kexport_table(&s_refresh_kexport_table);
        }
    }
#endif
}

static int obs_is_real_symbol_name(const char *symbol) {
    if (symbol == NULL || *symbol == '\0') {
        return 0;
    }
    for (const char *at = symbol; *at != '\0'; at++) {
        if (*at == '(' || *at == ')' || *at == ' ') {
            return 0;
        }
    }
    return 1;
}

int obs_module_open_tier(const char *library, obs_module_tier *tier_out) {
    if (tier_out != NULL) {
        *tier_out = OBS_TIER_UNKNOWN;
    }
    if (library == NULL) {
        return -1;
    }

#if defined(OBS_UNSAFE_LIBRARIES)
    const char *at = OBS_UNSAFE_LIBRARIES;
    while (*at != '\0') {
        while (*at == ' ')
            at++;
        const char *name = library;
        const char *scan = at;
        while (*scan != '\0' && *scan != ' ' && *name != '\0' && *scan == *name) {
            scan++;
            name++;
        }
        if (*name == '\0' && (*scan == '\0' || *scan == ' ')) {
            return -1;
        }
        while (*at != '\0' && *at != ' ')
            at++;
    }
#endif

    /* 0. Check if already cached from previous sysmodule load */
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_strcmp(library, obs_sysmodules[i].name) == 0) {
            if (obs_sysmodules[i].cached_handle > 1) {
                if (tier_out != NULL) {
                    *tier_out = OBS_TIER_SYSMODULE;
                }
                return obs_sysmodules[i].cached_handle;
            }
        }
    }

    /* 1. Check if already loaded in module list */
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        int mod_list[128];
        size_t mod_count = 0;
        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 && mod_count > 0) {
            if (obs_address_is_callable((const void *)&sceKernelGetModuleInfo)) {
                for (size_t i = 0; i < mod_count && i < 128; i++) {
                    int mod_id = mod_list[i];
                    if (mod_id <= 1)
                        continue;
                    unsigned char info[512];
                    for (size_t k = 0; k < sizeof(info); k++)
                        info[k] = 0;
                    *(size_t *)info = sizeof(info);
                    if (sceKernelGetModuleInfo(mod_id, info) == 0) {
                        const char *mod_name = (const char *)(info + 8);
                        if (obs_mod_name_match(mod_name, library)) {
                            if (tier_out != NULL) {
                                *tier_out = OBS_TIER_APP;
                            }
                            obs_sysmodule_cache_handle(library, mod_id);
                            return mod_id;
                        }
                    }
                }
            }
            /* Fallback: if sceKernelGetModuleInfo is refused (0x80020016), match via
             * known section symbols */
            if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
                for (unsigned int s = 0; s < obs_section_count; s++) {
                    const obs_section *sec = obs_sections[s];
                    if (sec == NULL)
                        continue;
                    for (unsigned int c = 0; c < sec->check_count; c++) {
                        const obs_check *chk = &sec->checks[c];
                        if (chk->library != NULL &&
                            obs_strcmp(chk->library, library) == 0 &&
                            chk->symbol != NULL &&
                            obs_is_real_symbol_name(chk->symbol)) {
                            char nid[12];
                            obs_compute_nid(chk->symbol, nid);
                            for (size_t k = 0; k < mod_count && k < 128; k++) {
                                int mid = mod_list[k];
                                if (mid <= 1)
                                    continue;
                                void *sym_addr = NULL;
                                if (sceKernelDlsym(mid, nid, &sym_addr) == 0 &&
                                    obs_address_is_callable(sym_addr)) {
                                    if (tier_out != NULL) {
                                        *tier_out = OBS_TIER_APP;
                                    }
                                    obs_sysmodule_cache_handle(library, mid);
                                    return mid;
                                }
                                if (sceKernelDlsym(mid, chk->symbol, &sym_addr) == 0 &&
                                    obs_address_is_callable(sym_addr)) {
                                    if (tier_out != NULL) {
                                        *tier_out = OBS_TIER_APP;
                                    }
                                    obs_sysmodule_cache_handle(library, mid);
                                    return mid;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (obs_strcmp(library, "libSceLibcInternal") == 0 ||
        obs_strcmp(library, "libSceLibcInternalExt") == 0) {
        int h = obs_find_module_handle_by_library("libSceLibcInternal");
        if (h > 1) {
            if (tier_out != NULL) {
                *tier_out = OBS_TIER_SYSMODULE;
            }
            return h;
        }
    }
    if (obs_strcmp(library, "libSceAudioOut2") == 0 ||
        obs_strcmp(library, "libSceAudioOut") == 0) {
        int h = obs_find_module_handle_by_library("libSceAudioOut2");
        if (h <= 1) {
            h = obs_find_module_handle_by_library("libSceAudioOut");
        }
        if (h > 1) {
            obs_sysmodule_cache_handle("libSceAudioOut2", h);
            obs_sysmodule_cache_handle("libSceAudioOut", h);
            if (tier_out != NULL) {
                *tier_out = OBS_TIER_SYSMODULE;
            }
            return h;
        }
    }
    if (obs_strcmp(library, "libkernel") == 0) {
        if (tier_out != NULL) {
            *tier_out = OBS_TIER_APP;
        }
        return 0x2001;
    }
    if (obs_strcmp(library, "libSceLibcInternal") == 0 ||
        obs_strcmp(library, "libc") == 0) {
        if (tier_out != NULL) {
            *tier_out = OBS_TIER_APP;
        }
        return 1;
    }

    /* 2. Try loading via sysmodule if listed in obs_sysmodules */
    for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
        if (obs_strcmp(library, obs_sysmodules[i].name) == 0) {
            if (obs_sysmodules[i].cached_handle > 1) {
                if (tier_out != NULL) {
                    *tier_out = OBS_TIER_SYSMODULE;
                }
                return obs_sysmodules[i].cached_handle;
            }
            if (obs_sysmodules[i].id != 0) {
                (void)obs_sysmodule_load_id(obs_sysmodules[i].id);
                int mod_h = -1;
                if (obs_sysmodule_get_handle_id(obs_sysmodules[i].id, &mod_h) == 0 &&
                    mod_h > 1) {
                    obs_sysmodules[i].cached_handle = mod_h;
                    if (tier_out != NULL) {
                        *tier_out = OBS_TIER_SYSMODULE;
                    }
                    return mod_h;
                }
            }
            obs_refresh_kexport_table();
            int probed_h = obs_find_module_handle_by_library(library);
            if (probed_h > 1) {
                if (tier_out != NULL) {
                    *tier_out = OBS_TIER_SYSMODULE;
                }
                obs_sysmodule_cache_handle(library, probed_h);
                return probed_h;
            }
            break;
        }
    }

    /* 3. Try loading via sceKernelLoadStartModule on known path prefixes (app PRXs
     * only) */
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule) &&
        obs_strncmp(library, "libSce", 6) != 0 &&
        obs_strncmp(library, "libkernel", 9) != 0 &&
        obs_strncmp(library, "libc", 4) != 0) {
        char path[128];
        for (unsigned int i = 0; i < OBS_COUNT(obs_module_path_prefixes); i++) {
            if (obs_strncmp(obs_module_path_prefixes[i].prefix, "/system", 7) == 0 ||
                obs_module_path_prefixes[i].prefix[0] == '\0') {
                continue;
            }
            if (!obs_module_path(path, sizeof path, obs_module_path_prefixes[i].prefix,
                                 library)) {
                continue;
            }
            int handle = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, NULL);
            if (handle > 1) {
                if (tier_out != NULL) {
                    *tier_out = obs_module_path_prefixes[i].tier;
                }
                obs_sysmodule_cache_handle(library, handle);
                return handle;
            }
        }
    }

    /* 4. Try finding module handle via loaded modules or sig_map */
    int probed_h = obs_find_module_handle_by_library(library);
    if (probed_h > 1) {
        if (tier_out != NULL) {
            *tier_out = OBS_TIER_SYSMODULE;
        }
        obs_sysmodule_cache_handle(library, probed_h);
        return probed_h;
    }

    /* 5. On PS5 retail userland, try loading the bare <library>.sprx directly */
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        char sprx_name[128];
        snprintf(sprx_name, sizeof(sprx_name), "%s.sprx", library);
        int handle = sceKernelLoadStartModule(sprx_name, 0, NULL, 0, NULL, NULL);
        if (handle <= 1) {
            snprintf(sprx_name, sizeof(sprx_name), "/system/common/lib/%s.sprx",
                     library);
            handle = sceKernelLoadStartModule(sprx_name, 0, NULL, 0, NULL, NULL);
        }
        if (handle <= 1) {
            snprintf(sprx_name, sizeof(sprx_name),
                     "/system_ex/app/PPSA90000/sce_module/%s.sprx", library);
            handle = sceKernelLoadStartModule(sprx_name, 0, NULL, 0, NULL, NULL);
        }
        if (handle <= 1) {
            snprintf(sprx_name, sizeof(sprx_name),
                     "/system_ex/app/PPSA90000/sce_module/%s.native.sprx", library);
            handle = sceKernelLoadStartModule(sprx_name, 0, NULL, 0, NULL, NULL);
        }
        if (handle <= 1 && obs_strncmp(library, "libSceSaveDataDialog", 20) == 0) {
            handle = sceKernelLoadStartModule(
                "/system/common/lib/libSceSaveDataDialog.sprx", 0, NULL, 0, NULL, NULL);
            if (handle <= 1) {
                handle = sceKernelLoadStartModule(
                    "/system/common/lib/libSceSaveDataDialog.native.sprx", 0, NULL, 0,
                    NULL, NULL);
            }
        }
        if (handle > 1) {
            if (tier_out != NULL) {
                *tier_out = OBS_TIER_SYSMODULE;
            }
            obs_sysmodule_cache_handle(library, handle);
            return handle;
        }
    }

    return -1;
}

int obs_module_open(const char *library) {
    obs_module_tier tier;
    return obs_module_open_tier(library, &tier);
}

static const void *obs_dlsym_try(int (*fn_dlsym)(int, const char *, void **), int h,
                                 const char *name) {
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    void *address = NULL;
    if (fn_dlsym(h, name, &address) == 0 && obs_address_is_callable(address)) {
        return address;
    }
    if (name[0] != '$') {
        char dollar[64];
        dollar[0] = '$';
        size_t d = 0;
        while (d < 60 && name[d] != '\0') {
            dollar[1 + d] = name[d];
            d++;
        }
        dollar[1 + d] = '\0';
        if (fn_dlsym(h, dollar, &address) == 0 && obs_address_is_callable(address)) {
            return address;
        }
    }
    char swapped[64];
    int has_swap = 0;
    size_t s = 0;
    while (s < 60 && name[s] != '\0') {
        char c = name[s];
        if (c == '+') {
            c = '-';
            has_swap = 1;
        } else if (c == '-') {
            c = '+';
            has_swap = 1;
        } else if (c == '/') {
            c = '_';
            has_swap = 1;
        } else if (c == '_') {
            c = '/';
            has_swap = 1;
        }
        swapped[s] = c;
        s++;
    }
    swapped[s] = '\0';
    if (has_swap) {
        if (fn_dlsym(h, swapped, &address) == 0 && obs_address_is_callable(address)) {
            return address;
        }
        if (swapped[0] != '$') {
            char dollar_swapped[64];
            dollar_swapped[0] = '$';
            size_t ds = 0;
            while (ds < 60 && swapped[ds] != '\0') {
                dollar_swapped[1 + ds] = swapped[ds];
                ds++;
            }
            dollar_swapped[1 + ds] = '\0';
            if (fn_dlsym(h, dollar_swapped, &address) == 0 &&
                obs_address_is_callable(address)) {
                return address;
            }
        }
    }
    return NULL;
}

const void *obs_dlsym_probe_handle(int (*fn_dlsym)(int, const char *, void **), int h,
                                   const char *full_nid, const char *bare_nid,
                                   const char *ascii_name) {
    if (h < 0 || fn_dlsym == NULL) {
        return NULL;
    }
    const void *address = NULL;
    if (full_nid != NULL && full_nid[0] != '\0') {
        address = obs_dlsym_try(fn_dlsym, h, full_nid);
        if (address != NULL) {
            return address;
        }
    }
    if (bare_nid != NULL && bare_nid[0] != '\0' &&
        (full_nid == NULL || obs_strcmp(bare_nid, full_nid) != 0)) {
        address = obs_dlsym_try(fn_dlsym, h, bare_nid);
        if (address != NULL) {
            return address;
        }
    }
    if (ascii_name != NULL && ascii_name[0] != '\0' &&
        (full_nid == NULL || obs_strcmp(ascii_name, full_nid) != 0) &&
        (bare_nid == NULL || obs_strcmp(ascii_name, bare_nid) != 0)) {
        address = obs_dlsym_try(fn_dlsym, h, ascii_name);
        if (address != NULL) {
            return address;
        }
    }
    if (bare_nid != NULL && bare_nid[0] != '\0' && obs_strlen(bare_nid) == 11) {
        char suffixed[32];
        for (size_t b = 0; b < 11; b++) {
            suffixed[b] = bare_nid[b];
        }
        static const char *const s_common_tags[] = {
            "#j#i", "#H#A", "#a#H", "#T#U", "#J#A", "#A#B", "#C#A", "#b#c",
            "#G#H", "#F#A", "#R#A", "#D#A", "#I#A", "#B#C", "#G#A", "#H#B",
            "#I#B", "#J#B", "#K#A", "#d#d", "#b#b", "#c#c", "#e#e"};
        for (size_t t = 0; t < sizeof(s_common_tags) / sizeof(s_common_tags[0]); t++) {
            for (size_t k = 0; k < 5; k++) {
                suffixed[11 + k] = s_common_tags[t][k];
            }
            address = obs_dlsym_try(fn_dlsym, h, suffixed);
            if (address != NULL) {
                return address;
            }
        }

        suffixed[11] = '#';
        suffixed[13] = '#';

        static const struct {
            char l1;
            char l2;
        } s_consec_pairs[] = {
            {'A', 'B'}, {'B', 'C'}, {'C', 'D'}, {'D', 'E'}, {'E', 'F'}, {'F', 'G'},
            {'G', 'H'}, {'H', 'I'}, {'I', 'J'}, {'J', 'K'}, {'K', 'L'}, {'L', 'M'},
            {'M', 'N'}, {'N', 'O'}, {'O', 'P'}, {'P', 'Q'}, {'Q', 'R'}, {'R', 'S'},
            {'S', 'T'}, {'T', 'U'}, {'U', 'V'}, {'V', 'W'}, {'W', 'X'}, {'X', 'Y'},
            {'Y', 'Z'}, {'Z', 'a'}, {'a', 'b'}, {'b', 'c'}, {'c', 'd'}, {'d', 'e'}};
        for (size_t p = 0; p < sizeof(s_consec_pairs) / sizeof(s_consec_pairs[0]);
             p++) {
            suffixed[12] = s_consec_pairs[p].l1;
            suffixed[14] = s_consec_pairs[p].l2;
            suffixed[15] = '\0';
            address = obs_dlsym_try(fn_dlsym, h, suffixed);
            if (address != NULL) {
                return address;
            }
        }

        for (char l = 'a'; l <= 'z'; l++) {
            suffixed[12] = l;
            suffixed[14] = l;
            suffixed[15] = '\0';
            address = obs_dlsym_try(fn_dlsym, h, suffixed);
            if (address != NULL) {
                return address;
            }
        }
        for (char l = 'A'; l <= 'Z'; l++) {
            suffixed[12] = l;
            suffixed[14] = l;
            suffixed[15] = '\0';
            address = obs_dlsym_try(fn_dlsym, h, suffixed);
            if (address != NULL) {
                return address;
            }
        }

        static const char versions[] = {'A', 'B', 'C', 'D'};
        for (size_t v = 0; v < sizeof(versions); v++) {
            suffixed[14] = versions[v];
            suffixed[15] = '\0';
            for (char l = 'A'; l <= 'Z'; l++) {
                suffixed[12] = l;
                address = obs_dlsym_try(fn_dlsym, h, suffixed);
                if (address != NULL) {
                    return address;
                }
            }
            for (char l = 'a'; l <= 'z'; l++) {
                suffixed[12] = l;
                address = obs_dlsym_try(fn_dlsym, h, suffixed);
                if (address != NULL) {
                    return address;
                }
            }
        }
    }
    return NULL;
}

const void *obs_module_symbol(int handle, const char *name) {
    if (name == NULL) {
        return NULL;
    }
    char nid[32];
    char bare_nid[12];
    for (size_t z = 0; z < sizeof(nid); z++) {
        nid[z] = '\0';
    }
    for (size_t z = 0; z < sizeof(bare_nid); z++) {
        bare_nid[z] = '\0';
    }
    const char *pnid = obs_prospero_name_to_nid(name);
    if (pnid != NULL) {
        size_t z = 0;
        while (z < 31 && pnid[z] != '\0') {
            nid[z] = pnid[z];
            z++;
        }
        nid[z] = '\0';
    } else if (name[0] == '$') {
        size_t j = 0;
        while (j < 31 && name[1 + j] != '\0') {
            nid[j] = name[1 + j];
            j++;
        }
        nid[j] = '\0';
    } else if (obs_is_nid_string(name)) {
        size_t z = 0;
        while (z < 31 && name[z] != '\0') {
            nid[z] = name[z];
            z++;
        }
        nid[z] = '\0';
    } else {
        obs_compute_nid(name, nid);
    }
    for (size_t b = 0; b < 11 && nid[b] != '\0' && nid[b] != '#'; b++) {
        bare_nid[b] = nid[b];
    }
    bare_nid[11] = '\0';

    /* 0. Try kernel-extracted export table first if available (bypasses retail game DRM
     * block) */
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *kaddr = obs_kexport_lookup(
            (const obs_kexport_table_t *)pargs->kexport_table, bare_nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            return kaddr;
        }
    }

    /* 0b. Try kernel module dispatch table lookup directly */
    int found_handle = -1;
    const void *krw_addr = obs_krw_find_symbol(bare_nid, &found_handle);
    if (krw_addr != NULL && obs_address_is_callable(krw_addr)) {
        return krw_addr;
    }

    /* Function-pointer casts, fenced as in runtime.c and layout.c: clang-format 18 and
     * 22 space them differently, and CI runs 18 while a dev here runs 22 (D016). */
    /* clang-format off */
    int (*fn_dlsym)(int, const char *, void **) = NULL;
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        fn_dlsym = (int (*)(int, const char *, void **))&sceKernelDlsym;
    } else if (pargs != NULL &&
               obs_address_is_callable((const void *)pargs->sys_dynlib_dlsym)) {
        fn_dlsym = (int (*)(int, const char *, void **))pargs->sys_dynlib_dlsym;
    }
    /* clang-format on */
    if (fn_dlsym == NULL) {
        return NULL;
    }
    const char *ascii_name = (name[0] == '$') ? (name + 1) : name;

    /* 1. Try on requested handle */
    if (handle > 1) {
        const void *addr =
            obs_dlsym_probe_handle(fn_dlsym, handle, nid, bare_nid, ascii_name);
        if (addr != NULL) {
            return addr;
        }
    }

    /* 2. Global search 0x2001 (RTLD_DEFAULT) fallback */
    const void *addr_rtld =
        obs_dlsym_probe_handle(fn_dlsym, 0x2001, nid, bare_nid, ascii_name);
    if (addr_rtld != NULL) {
        return addr_rtld;
    }

    /* 3. Iterate cached sysmodule handles */
    for (unsigned int s = 0; s < OBS_COUNT(obs_sysmodules); s++) {
        int sh = obs_sysmodules[s].cached_handle;
        if (sh > 1 && sh != handle) {
            const void *addr =
                obs_dlsym_probe_handle(fn_dlsym, sh, nid, bare_nid, ascii_name);
            if (addr != NULL) {
                return addr;
            }
        }
    }

    /* 4. Global search handle 2 (libkernel / libc) fallback */
    if (handle != 2) {
        const void *addr =
            obs_dlsym_probe_handle(fn_dlsym, 2, nid, bare_nid, ascii_name);
        if (addr != NULL) {
            return addr;
        }
    }

    /* 5. Iterate all loaded module IDs */
    int mod_list[128];
    for (size_t k = 0; k < 128; k++)
        mod_list[k] = 0;
    size_t mod_count = 0;
    int got_list = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 && mod_count > 0) {
            got_list = 1;
        }
    }
#if !defined(OBSCENE_HOST_BUILD)
    if (!got_list) {
        if (obs_invoke_syscall(592, (long)mod_list, 128, (long)&mod_count, 0, 0, 0) ==
                0 &&
            mod_count > 0) {
            got_list = 1;
        }
    }
#endif
    if (got_list) {
        for (size_t i = 0; i < mod_count && i < 128; i++) {
            int mod_id = mod_list[i];
            if (mod_id > 1 && mod_id != handle && mod_id != 2) {
                const void *addr =
                    obs_dlsym_probe_handle(fn_dlsym, mod_id, nid, bare_nid, ascii_name);
                if (addr != NULL) {
                    return addr;
                }
            }
        }
    }

    /* 6. Fallback search handle 1 (eboot) & handle 0 (self) */
    if (handle != 1) {
        const void *addr =
            obs_dlsym_probe_handle(fn_dlsym, 1, nid, bare_nid, ascii_name);
        if (addr != NULL) {
            return addr;
        }
    }
    if (handle != 0) {
        const void *addr =
            obs_dlsym_probe_handle(fn_dlsym, 0, nid, bare_nid, ascii_name);
        if (addr != NULL) {
            return addr;
        }
    }

    /* 7. Live kernel dispatch table walk via KRW (bypasses retail title dlsym
     * isolation) */
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t kaddr = krw_dynlib_resolve_any(pid, nid);
        if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
            return (const void *)kaddr;
        }
        if (bare_nid[0] != '\0' && obs_strcmp(bare_nid, nid) != 0) {
            kaddr = krw_dynlib_resolve_any(pid, bare_nid);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (const void *)kaddr;
            }
        }
        if (ascii_name != NULL && ascii_name[0] != '\0') {
            kaddr = krw_dynlib_resolve_any(pid, ascii_name);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (const void *)kaddr;
            }
        }
    }
#endif

    return NULL;
}

int obs_module_resolution_works(void) {
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        return 1;
    }
    /* Three states, because "not asked yet" and "asked, and no" are different and zero
     * can only mean one of them. */
    static int decided = 0;
    if (decided == 0) {
        decided = obs_module_open("libkernel") >= 0 ? 1 : -1;
    }
    return decided == 1;
}

/* Storage behind OBS_NO_SYMBOL. Its address is all that matters. */
const char obs_no_symbol_marker = 0;

/* Whether a peripheral is attached, by opening and closing it once. Run-level facts,
 * reported so a peripheral probe's PENDING reads against what was plugged in. Each is
 * fully guarded: a loader without the symbol resolves it weak-null and the device reads
 * absent. (D328) */
static int32_t obs_peripheral_user(void) {
    /* Inert during harness startup to keep user service uninitialized for
     * fresh-process checks. */
    return -1;
}

static int obs_peripheral_pad(void) {
    if (!obs_address_is_callable((const void *)&scePadOpen)) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&scePadInit)) {
        scePadInit();
    }
    int32_t user = obs_peripheral_user();
    if (user < 0) {
        return 0;
    }
    int handle = scePadOpen(user, 0, 0, NULL);
    if (handle <= 0) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&scePadClose)) {
        scePadClose(handle);
    }
    return 1;
}

static int obs_peripheral_keyboard(void) {
    if (!obs_address_is_callable((const void *)&sceKeyboardOpen)) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&sceKeyboardInit)) {
        sceKeyboardInit();
    }
    int32_t user = obs_peripheral_user();
    if (user < 0) {
        return 0;
    }
    int handle = sceKeyboardOpen(user, 0, 0, NULL);
    if (handle < 0) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&sceKeyboardClose)) {
        sceKeyboardClose(handle);
    }
    return 1;
}

static int obs_peripheral_mouse(void) {
    if (!obs_address_is_callable((const void *)&sceMouseOpen)) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&sceMouseInit)) {
        sceMouseInit();
    }
    int32_t user = obs_peripheral_user();
    if (user < 0) {
        return 0;
    }
    int handle = sceMouseOpen(user, 0, 0, NULL);
    if (handle < 0) {
        return 0;
    }
    if (obs_address_is_callable((const void *)&sceMouseClose)) {
        sceMouseClose(handle);
    }
    return 1;
}

static int obs_peripheral_audio(void) {
    /* Inert during harness startup to keep audio uninitialized for fresh-process
     * checks. */
    return 0;
}

static void tally_add(obs_tally *tally, obs_status status) {
    switch (status) {
    case OBS_PASS:
        tally->pass++;
        break;
    case OBS_PARTIAL:
        tally->partial++;
        break;
    case OBS_FAIL:
        tally->fail++;
        break;
    case OBS_SKIP:
        tally->skip++;
        break;
    case OBS_CRASH:
        tally->crash++;
        break;
    case OBS_PENDING:
        tally->pending++;
        break;
    }
}

static const obs_check *s_current_check = NULL;

const obs_check *obs_get_current_check(void) {
    return s_current_check;
}

obs_tally obs_run_all(void) {
    obs_boot_note("obscene: obs_run_all enter\n");
    /* Arm the fault guard before any check runs, so a call that would end the
     * process is caught and recorded instead. A no-op where the primitives cannot
     * be resolved, in which case a faulting check ends the run exactly as it did
     * before. (D325) */
    obs_fault_init();

    obs_tally total = {0, 0, 0, 0, 0, 0};
    unsigned int checks = 0;
    for (unsigned int s = 0; s < obs_section_count; s++) {
        checks += obs_sections[s]->check_count;
    }
    obs_boot_note("obscene: before obs_resume_load\n");
    obs_resume_load(OBSCENE_BUILD_ID, checks);
    obs_boot_note("obscene: before obs_sink_open\n");
    const char *sink = obs_sink_open();
    obs_boot_note("obscene: after obs_sink_open\n");

    obs_report_meta(obs_section_count, checks);
    obs_report_build();
    /* The measured context, next to the build: which environment this run's
     * findings came from (host, ps4-compat payload, ps5-native, a title eboot). The
     * same binary reports different things in each, and a reader diffing two runs
     * needs to know which is which. */
    {
        char context_name[40];
        char context_basis[112];
        obs_run_context(context_name, sizeof context_name, context_basis,
                        sizeof context_basis);
        obs_report_context(context_name, context_basis);
    }
    obs_report_sink(sink);
    obs_report_guard(obs_fault_available(), obs_fault_detail());
    /* What is plugged in, so the peripheral probes' PENDING reads against it.
     * (D328) */
    obs_report_peripherals(obs_peripheral_pad(), obs_peripheral_keyboard(),
                           obs_peripheral_mouse(), obs_peripheral_audio());
    /* Whether the enumeration could look at all, so a module handle of 0x0 below
     * reads as "not seen" rather than "absent" - the distinction payload mode could
     * not make. (D329) */
    obs_report_resolution(
        obs_module_resolution_works(),
        obs_module_resolution_works()
            ? "module list and dlsym resolve here"
            : "no module list and no dlsym here (payload mode): a module "
              "handle of 0x0 below means not seen, not absent");
    obs_report_resume(obs_resume_skipped_count(), obs_resume_overflowed());
    /* The status readout the HUD draws, mirrored into the report so a reader that
     * never sees the screen gets the same facts (memory, VRAM, generation, gaps and
     * all). Observations, not verdicts - see obs_report_sysinfo. */
    obs_sysinfo_report();

    /* Output is seeded as available rather than proven: this report is being
     * written, so the stream works. The boot section still checks it explicitly,
     * because "works well enough for the harness" and "behaves correctly" are
     * different claims and only the second is worth reporting. */
    unsigned int available = OBS_CAP_OUTPUT;

    /* For the frontier record. `blocked` counts checks that never ran because
     * something they needed was never established - the suite behind the floor -
     * and `deepest` the last wholly green section. */
    unsigned int blocked = 0;
    unsigned int deepest = 0;

    obs_boot_note("obscene: before obs_screen_begin\n");
    obs_screen_begin(obs_section_count, obs_total_checks());
    obs_boot_note("obscene: after obs_screen_begin\n");

    if (obs_pltauth_is_failed()) {
        obs_boot_note("obscene: obs_pltauth_is_failed is TRUE!\n");
        return total;
    }

    obs_boot_note("obscene: starting section loop\n");
    uint64_t run_start_us = obs_time_now_us();
    obs_report_time_start(run_start_us);

    for (unsigned int s = 0; s < obs_section_count; s++) {
        const obs_section *section = obs_sections[s];
        obs_tally section_tally = {0, 0, 0, 0, 0, 0};
        obs_report_section(section);
        uint64_t section_start_us = obs_time_now_us();

        for (unsigned int c = 0; c < section->check_count; c++) {
            const obs_check *check = &section->checks[c];
            obs_result result;

            unsigned int missing = check->requires_caps & ~available;
            int excluded = obs_check_is_excluded(check->id);
            if (excluded) {
                /* Named at build time as one that takes the process down here.
                 * Reported as a skip with the reason, so a reader can see it was
                 * deliberate and a diff can see it stopped being run. */
                /* Two mechanisms, two messages, because they mean different things
                 * to a reader. One is an operator's judgement baked into this
                 * build; the other is this program's own observation from the last
                 * run of the same binary, and only the second is a *finding*.
                 * Sharing a sentence would also make the compatibility table's
                 * count of build-time exclusions silently wrong. */
                result =
                    excluded == OBS_EXCLUDED_AT_BUILD
                        ? obs_skip(
                              "excluded at build time: known to end the process on "
                              "this platform")
                        : obs_skip("did not return on the previous run of this "
                                   "build, so "
                                   "it is skipped to get past it");
            } else if (!obs_address_is_callable(check->address)) {
                /* The loader did not resolve the symbol - which is **not** the same
                 * as the platform not having it, and saying so cost a whole
                 * afternoon's report.
                 *
                 * On a console this program is a title, and a title is given far
                 * fewer libraries than it asks for: of twelve `DT_NEEDED`, five
                 * were mapped. Every check behind the other seven reported "the
                 * symbol is not present on this platform" - and the run's own
                 * census, resolving the same libraries at run time through
                 * `sceKernelLoadStartModule`, found `libScePad`, `libSceAudioOut`,
                 * `libSceUserService`, `libSceGnmDriver`, `libSceNetCtl` and
                 * `libSceVideoOut` all partly present. The symbols were there. The
                 * sentence was wrong, and it was wrong in the direction that reads
                 * as a finding about the platform. (D235)
                 *
                 * The fix is to stop claiming, **not** to go and find out here.
                 * Loading the library to answer it was tried and it is the wrong
                 * place: it turns a skip - the one path in this loop that is
                 * supposed to do nothing - into one with side effects, in a program
                 * whose first principle is that a check announces before it acts.
                 * On hardware it loaded `libSceVideoRecording` on the way past
                 * `105-record`, which is one of the ten libraries that end the
                 * process, and took a thirteen-thousand-record run down to three
                 * hundred. (D235)
                 *
                 * So this says only what it saw. Whether the symbol exists is a
                 * question the census answers, by library, in the same report - and
                 * a reader who wants to know can look, which is the arrangement
                 * that was always available. */
                result =
                    obs_skip("the loader did not resolve this symbol for this build");
            } else if (missing != 0) {
                /* Not attempted, so nothing is announced - a `try` line with no
                 * result would look like a crash to anything reading the tail. */
                blocked++;
                result = obs_skip("a prerequisite capability was not established");
            } else {
                obs_report_attempt(check);
                /* And on screen, before the call.
                 *
                 * The screen only ever showed results, which are recorded *after* a
                 * check returns - so a check that never returns left the last
                 * completed section up and nothing naming what was in flight. On a
                 * console the screen is the only channel, and "it stopped
                 * somewhere" is a much worse thing to carry off a hardware run than
                 * "it stopped in 015-sync/thread-churn".
                 *
                 * This is announce-before-attempting applied to the display, for
                 * the same reason the report does it: the announcement is only
                 * useful if it is made before the risk. (D174) */
                /* Recorded without a redraw: see obs_screen_attempt. */
                obs_screen_attempt(check->id);
                /* The fault guard. A check that faults - the futex did, inside
                 * libkernel - lands back here as a crash rather than ending the
                 * run, so the sections behind it still run and the crash is a
                 * record instead of a silent stop. The try is already on the wire
                 * (obs_report_attempt above), so a crash caught here turns that try
                 * into a `crash` res rather than leaving it dangling. (D325) */
                obs_jmp_buf guard;
                int faulted = OBS_FAULT_ARM(&guard);
                uint64_t check_start_us = obs_time_now_us();
                s_current_check = check;
                if (faulted == 0) {
                    result = check->run();
                    obs_fault_unregister();
                    if (result.status == OBS_PASS) {
                        available |= check->provides_caps;
                    }
                } else {
                    obs_fault_unregister();
                    result = obs_crash(faulted);
                }
                s_current_check = NULL;
                uint64_t check_dur_us = obs_time_now_us() - check_start_us;
                if (check_dur_us > 50000u) {
                    obs_report_time_check(check->id, check_dur_us);
                }
            }

            obs_report_result(check, result);
            obs_screen_check(check->id, result);
            tally_add(&section_tally, result.status);
            tally_add(&total, result.status);
        }
        uint64_t section_dur_us = obs_time_now_us() - section_start_us;
        obs_report_section_tally(section, section_tally, section_dur_us);
        obs_report_time_section(section->id, section_dur_us);
        obs_screen_section(section->id, section_tally);

        /* The deepest section that came out wholly green. Sections run base-first,
         * so this is how far up the stack the platform held together - a proxy for
         * the capability frontier below, and the one a reader can point at. */
        if (section_tally.fail == 0 && section_tally.partial == 0 &&
            section_tally.skip == 0 && section_tally.pass > 0) {
            deepest = s + 1u;
        }
    }

    uint64_t total_run_us = obs_time_now_us() - run_start_us;

    /* How many capabilities the platform actually established. Counted by popcount
     * over the bitset rather than tracked separately, so it cannot drift from the
     * thing the dependency tests are reading. */
    unsigned int established = 0;
    for (unsigned int bit = 0; bit < 32u; bit++) {
        if ((available & (1u << bit)) != 0) {
            established++;
        }
    }
    obs_report_frontier(established, blocked, deepest);

    obs_report_tally(total);
    obs_report_time_total(total_run_us);
    obs_report_end(total_run_us);
    obs_sink_close();
    return total;
}
