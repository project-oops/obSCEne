#include <stddef.h>
#include <stdint.h>

/* Empty unless the build names checks or sections to exclude. See
 * obs_check_is_excluded. */
#ifndef OBSCENE_EXCLUDE
#define OBSCENE_EXCLUDE ""
#endif

#include "common/freestd.h"
#include "common/krw.h"
#include "obscene/harness.h"
/* For `sceKernelLoadStartModule` and `sceKernelDlsym`, which `obs_module_open` uses.
 *
 * The harness did not need a platform declaration before: it runs checks, and the
 * checks called the platform. Asking whether a *library* is there is the harness's
 * business rather than any one check's - it is the guard that D226 showed was missing,
 * at the layer where the existing one could not reach - so the two calls that make it
 * possible live here. */
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
#define OBS_LOWEST_CALLABLE 0x400000UL

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

int obs_address_is_callable(const void *address) {
    return address != NULL && (uintptr_t)address >= OBS_LOWEST_CALLABLE;
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
static const char *const obs_module_prefixes[] = {
    "",
    "/system/common/lib/",
    "/system_ex/common_ex/lib/",
    "/app0/sce_module/",
};

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

typedef struct {
    const char *name;
    uint16_t id;
} obs_sysmodule_id_map;

static const obs_sysmodule_id_map obs_sysmodules[] = {
    {"libSceNet", 0x0001},      {"libSceHttp", 0x0002},
    {"libSceSsl", 0x0003},      {"libSceUserService", 0x0004},
    {"libSceSaveData", 0x0006}, {"libSceAudioOut", 0x000c},
    {"libSceVoice", 0x000e},    {"libSceAppInstUtil", 0x0014},
    {"libSceIme", 0x0017},      {"libSceCamera", 0x001d},
    {"libScePad", 0x0027},      {"libSceVideoOut", 0x0028},
};

int obs_module_open(const char *library) {
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

    /* 1. Check if already loaded in module list */
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList) &&
        obs_address_is_callable((const void *)&sceKernelGetModuleInfo)) {
        int mod_list[128];
        size_t mod_count = 0;
        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 && mod_count > 0) {
            for (size_t i = 0; i < mod_count && i < 128; i++) {
                int mod_id = mod_list[i];
                if (mod_id <= 0)
                    continue;
                unsigned char info[512];
                for (size_t k = 0; k < sizeof(info); k++)
                    info[k] = 0;
                *(size_t *)info = sizeof(info);
                if (sceKernelGetModuleInfo(mod_id, info) == 0) {
                    const char *mod_name = (const char *)(info + 8);
                    if (obs_strcmp(mod_name, library) == 0) {
                        return mod_id;
                    }
                }
            }
        }
    }

    if (obs_strcmp(library, "libkernel") == 0) {
        return 0x2001;
    }
    if (obs_strcmp(library, "libSceLibcInternal") == 0 ||
        obs_strcmp(library, "libc") == 0) {
        return 1;
    }

    /* 2. Try loading via sceKernelLoadStartModule on known path prefixes */
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        char path[128];
        for (unsigned int i = 0; i < OBS_COUNT(obs_module_prefixes); i++) {
            if (!obs_module_path(path, sizeof path, obs_module_prefixes[i], library)) {
                continue;
            }
            int handle = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, NULL);
            if (handle >= 0) {
                return handle;
            }
        }
    }

    /* 3. Try loading via sceSysmoduleLoadModule */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        for (unsigned int i = 0; i < OBS_COUNT(obs_sysmodules); i++) {
            if (obs_strcmp(library, obs_sysmodules[i].name) == 0) {
                int rc = sceSysmoduleLoadModule(obs_sysmodules[i].id);
                if (rc == 0 || rc == (int)0x80540001) {
                    if (obs_address_is_callable(
                            (const void *)&sceKernelGetModuleList) &&
                        obs_address_is_callable(
                            (const void *)&sceKernelGetModuleInfo)) {
                        int mod_list[128];
                        size_t mod_count = 0;
                        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 &&
                            mod_count > 0) {
                            for (size_t k = 0; k < mod_count && k < 128; k++) {
                                int mod_id = mod_list[k];
                                if (mod_id <= 0)
                                    continue;
                                unsigned char info[512];
                                for (size_t z = 0; z < sizeof(info); z++)
                                    info[z] = 0;
                                *(size_t *)info = sizeof(info);
                                if (sceKernelGetModuleInfo(mod_id, info) == 0) {
                                    const char *mod_name = (const char *)(info + 8);
                                    if (obs_strcmp(mod_name, library) == 0) {
                                        return mod_id;
                                    }
                                }
                            }
                        }
                    }
                    return 1;
                }
            }
        }
    }

    return -1;
}

const void *obs_module_symbol(int handle, const char *name) {
    if (name == NULL) {
        return NULL;
    }
    char nid[12];
    obs_compute_nid(name, nid);

    /* 0. Try kernel-extracted export table first if available (bypasses retail game DRM
     * block) */
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *kaddr =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            return kaddr;
        }
    }

    int (*fn_dlsym)(int, const char *, void **) = NULL;
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        fn_dlsym = (int (*)(int, const char *, void **))&sceKernelDlsym;
    } else if (pargs != NULL &&
               obs_address_is_callable((const void *)pargs->sys_dynlib_dlsym)) {
        fn_dlsym = (int (*)(int, const char *, void **))pargs->sys_dynlib_dlsym;
    }
    if (fn_dlsym == NULL) {
        return NULL;
    }
    void *address = NULL;
    /* 1. Try NID encoding (Sony SPRX export tables store 11-char NIDs) */
    if (handle >= 0 && fn_dlsym(handle, nid, &address) == 0 &&
        obs_address_is_callable(address)) {
        return address;
    }
    /* 2. Fallback: try plain ASCII name */
    if (handle >= 0 && fn_dlsym(handle, name, &address) == 0 &&
        obs_address_is_callable(address)) {
        return address;
    }
    /* 3. Global search handle 1 fallback */
    if (fn_dlsym(1, nid, &address) == 0 && obs_address_is_callable(address)) {
        return address;
    }
    if (fn_dlsym(1, name, &address) == 0 && obs_address_is_callable(address)) {
        return address;
    }
    /* 4. Global search handle 2 fallback */
    if (fn_dlsym(2, nid, &address) == 0 && obs_address_is_callable(address)) {
        return address;
    }
    if (fn_dlsym(2, name, &address) == 0 && obs_address_is_callable(address)) {
        return address;
    }
    /* 5. Iterate all loaded module IDs */
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        int mod_list[128];
        size_t mod_count = 0;
        if (sceKernelGetModuleList(mod_list, 128, &mod_count) == 0 && mod_count > 0) {
            for (size_t i = 0; i < mod_count && i < 128; i++) {
                int mod_id = mod_list[i];
                if (mod_id <= 0)
                    continue;
                if (fn_dlsym(mod_id, nid, &address) == 0 &&
                    obs_address_is_callable(address)) {
                    return address;
                }
                if (fn_dlsym(mod_id, name, &address) == 0 &&
                    obs_address_is_callable(address)) {
                    return address;
                }
            }
        }
    }
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
    }
}

obs_tally obs_run_all(void) {
    obs_tally total = {0, 0, 0, 0};
    unsigned int checks = 0;
    for (unsigned int s = 0; s < obs_section_count; s++) {
        checks += obs_sections[s]->check_count;
    }
    /* Opened before the first record is written, so the file holds the whole report
     * rather than everything after the point somebody remembered to open it.
     *
     * The result is reported either way. A run whose sink silently failed and a run
     * built without one look identical afterwards, and the difference is whether the
     * findings survived being switched off - so the ambiguity is removed here rather
     * than left for whoever goes looking for a file that is not there. */
    /* The previous report, read **before** the sink opens and truncates it.
     *
     * Ordering is the whole of the mechanism: `obs_sink_open` opens `O_TRUNC`, so one
     * line later there is nothing left to learn. What it learns is the check the last
     * run announced and never finished, which this run then skips. (D172) */
    obs_resume_load(OBSCENE_BUILD_ID, checks);
    const char *sink = obs_sink_open();

    obs_report_meta(obs_section_count, checks);
    obs_report_build();
    /* The measured context, next to the build: which environment this run's findings
     * came from (host, ps4-compat payload, ps5-native, a title eboot). The same binary
     * reports different things in each, and a reader diffing two runs needs to know
     * which is which. */
    {
        char context_name[40];
        char context_basis[112];
        obs_run_context(context_name, sizeof context_name, context_basis,
                        sizeof context_basis);
        obs_report_context(context_name, context_basis);
    }
    obs_report_sink(sink);
    obs_report_resume(obs_resume_skipped_count(), obs_resume_overflowed());
    /* The status readout the HUD draws, mirrored into the report so a reader that never
     * sees the screen gets the same facts (memory, VRAM, generation, gaps and all).
     * Observations, not verdicts - see obs_report_sysinfo. */
    obs_sysinfo_report();

    /* Output is seeded as available rather than proven: this report is being
     * written, so the stream works. The boot section still checks it explicitly,
     * because "works well enough for the harness" and "behaves correctly" are
     * different claims and only the second is worth reporting. */
    unsigned int available = OBS_CAP_OUTPUT;

    /* For the frontier record. `blocked` counts checks that never ran because something
     * they needed was never established - the suite behind the floor - and `deepest`
     * the last wholly green section. */
    unsigned int blocked = 0;
    unsigned int deepest = 0;

    /* Before any check, so the screen is alive from the start rather than after the
     * first section finishes. Safe when there is no display. */
    obs_screen_begin(obs_section_count, obs_total_checks());

    for (unsigned int s = 0; s < obs_section_count; s++) {
        const obs_section *section = obs_sections[s];
        obs_tally section_tally = {0, 0, 0, 0};
        obs_report_section(section);

        for (unsigned int c = 0; c < section->check_count; c++) {
            const obs_check *check = &section->checks[c];
            obs_result result;

            unsigned int missing = check->requires_caps & ~available;
            int excluded = obs_check_is_excluded(check->id);
            if (excluded) {
                /* Named at build time as one that takes the process down here. Reported
                 * as a skip with the reason, so a reader can see it was deliberate and
                 * a diff can see it stopped being run. */
                /* Two mechanisms, two messages, because they mean different things to a
                 * reader. One is an operator's judgement baked into this build; the
                 * other is this program's own observation from the last run of the same
                 * binary, and only the second is a *finding*. Sharing a sentence would
                 * also make the compatibility table's count of build-time exclusions
                 * silently wrong. */
                result =
                    excluded == OBS_EXCLUDED_AT_BUILD
                        ? obs_skip(
                              "excluded at build time: known to end the process on "
                              "this platform")
                        : obs_skip(
                              "did not return on the previous run of this build, so "
                              "it is skipped to get past it");
            } else if (!obs_address_is_callable(check->address)) {
                /* The loader did not resolve the symbol - which is **not** the same as
                 * the platform not having it, and saying so cost a whole afternoon's
                 * report.
                 *
                 * On a console this program is a title, and a title is given far fewer
                 * libraries than it asks for: of twelve `DT_NEEDED`, five were mapped.
                 * Every check behind the other seven reported "the symbol is not
                 * present on this platform" - and the run's own census, resolving the
                 * same libraries at run time through `sceKernelLoadStartModule`, found
                 * `libScePad`, `libSceAudioOut`, `libSceUserService`,
                 * `libSceGnmDriver`, `libSceNetCtl` and `libSceVideoOut` all partly
                 * present. The symbols were there. The sentence was wrong, and it was
                 * wrong in the direction that reads as a finding about the platform.
                 * (D235)
                 *
                 * The fix is to stop claiming, **not** to go and find out here. Loading
                 * the library to answer it was tried and it is the wrong place: it
                 * turns a skip - the one path in this loop that is supposed to do
                 * nothing - into one with side effects, in a program whose first
                 * principle is that a check announces before it acts. On hardware it
                 * loaded `libSceVideoRecording` on the way past `105-record`, which is
                 * one of the ten libraries that end the process, and took a
                 * thirteen-thousand-record run down to three hundred. (D235)
                 *
                 * So this says only what it saw. Whether the symbol exists is a
                 * question the census answers, by library, in the same report - and a
                 * reader who wants to know can look, which is the arrangement that was
                 * always available. */
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
                 * check returns - so a check that never returns left the last completed
                 * section up and nothing naming what was in flight. On a console the
                 * screen is the only channel, and "it stopped somewhere" is a much
                 * worse thing to carry off a hardware run than "it stopped in
                 * 015-sync/thread-churn".
                 *
                 * This is announce-before-attempting applied to the display, for the
                 * same reason the report does it: the announcement is only useful if it
                 * is made before the risk. (D174) */
                /* Recorded without a redraw: see obs_screen_attempt. */
                obs_screen_attempt(check->id);
                result = check->run();
                if (result.status == OBS_PASS) {
                    available |= check->provides_caps;
                }
            }

            obs_report_result(check, result);
            obs_screen_check(check->id, result);
            tally_add(&section_tally, result.status);
            tally_add(&total, result.status);
        }
        obs_report_section_tally(section, section_tally);
        obs_screen_section(section->id, section_tally);

        /* The deepest section that came out wholly green. Sections run base-first, so
         * this is how far up the stack the platform held together - a proxy for the
         * capability frontier below, and the one a reader can point at. */
        if (section_tally.fail == 0 && section_tally.partial == 0 &&
            section_tally.skip == 0 && section_tally.pass > 0) {
            deepest = s + 1u;
        }
    }

    /* How many capabilities the platform actually established. Counted by popcount over
     * the bitset rather than tracked separately, so it cannot drift from the thing the
     * dependency tests are reading. */
    unsigned int established = 0;
    for (unsigned int bit = 0; bit < 32u; bit++) {
        if ((available & (1u << bit)) != 0) {
            established++;
        }
    }
    obs_report_frontier(established, blocked, deepest);

    obs_report_tally(total);
    obs_report_end();
    return total;
}
