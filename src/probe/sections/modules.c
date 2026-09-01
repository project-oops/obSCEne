/*
 * What is actually loaded.
 *
 * Every other section tests a list this program carries: someone wrote a name down, and
 * the check asks whether it works. That is only ever as complete as whoever maintained
 * the list, and a symbol nobody thought of is a symbol nobody tests. Two of the
 * functions an emulator fixed in its last release were not in our census at all - not
 * broken, not reported absent, simply never asked about.
 *
 * This section asks the machine instead. It enumerates the modules the platform has
 * loaded and reports them, so a run says what is *there* rather than only what was
 * expected to be there.
 *
 * # On hardware this becomes the inventory
 *
 * Run against an emulator it says what that emulator claims to provide, which is useful
 * on its own - a name in this list that fails a behavioural check is a different
 * problem from a name that is missing entirely. Run on a real console it is the
 * authoritative list, and the difference between the two is the gap an emulator has
 * left.
 *
 * # Two calls, and only one of them needs a struct
 *
 * `sceKernelGetModuleList` deals in handles. No layout, no risk, and the count alone is
 * a result worth reporting.
 *
 * `sceKernelGetModuleInfo` fills a structure this program does not fully know. It is
 * given a large zeroed buffer with its leading size field set, and only the two fields
 * at the front are read: the size the platform wrote back, and the name that follows
 * it. Anything deeper would be a guessed offset producing confident nonsense, which is
 * worse than reporting nothing (D008).
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* More modules than a console is expected to have loaded, so a full list is never
 * silently truncated into a smaller, plausible-looking one. */
#define OBS_MODULE_MAX 256

/* Comfortably larger than the info structure is believed to be. The platform is told
 * the size in the leading field and writes no more than that. */
#define OBS_MODULE_INFO_BYTES 1024

/* The name follows the size field. This is the one offset assumed here, and it is the
 * shallowest one available: a structure a platform fills and a caller reads forward
 * from has to start with its own length. If it is wrong, the name reads as rubbish and
 * is reported as rubbish - visible, not silent. */
#define OBS_MODULE_NAME_OFFSET 8
#define OBS_MODULE_NAME_MAX 64

/* How much of the structure to report when the assumed offset finds no name. Enough to
 * see a size field, a name and the start of whatever follows. */
#define OBS_MODULE_INFO_WORDS 8

static unsigned int obs_module_count = 0;
static int obs_module_handles[OBS_MODULE_MAX];

static obs_result check_module_list(void) {
    obs_module_count = 0;

    size_t written = 0;
    int rc = sceKernelGetModuleList(obs_module_handles, OBS_MODULE_MAX, &written);
    if (rc != 0) {
        return obs_fail_code("the module list could not be read",
                             (uint64_t)(uint32_t)rc);
    }
    if (written == 0) {
        /* A platform running this program has at least this program loaded, so an
         * empty list is a working call with an answer that cannot be true. */
        return obs_fail("the platform reports no loaded modules at all");
    }
    if (written > OBS_MODULE_MAX) {
        return obs_partial_value(
            "more modules than this can hold; the list is truncated",
            (uint64_t)written);
    }
    obs_module_count = (unsigned int)written;
    return obs_pass_value((uint64_t)written);
}

/* Copies a name out of the info buffer, stopping at the first NUL and refusing anything
 * that is not printable. A platform that fills the structure differently produces bytes
 * rather than a name, and those must not be pasted into a report that other tools
 * parse. */
static int obs_copy_name(const unsigned char *info, char *out, size_t max) {
    size_t n = 0;
    while (n + 1 < max) {
        unsigned char c = info[OBS_MODULE_NAME_OFFSET + n];
        if (c == 0) {
            break;
        }
        if (c < 0x20 || c > 0x7E) {
            return 0;
        }
        out[n] = (char)c;
        n++;
    }
    out[n] = '\0';
    return n > 0;
}

static obs_result check_module_names(void) {
    if (obs_module_count == 0) {
        return obs_skip("the module list was not read, so there is nothing to name");
    }

    unsigned int named = 0;
    unsigned int unreadable = 0;
    /* The first refusal, kept so the report says *why* rather than only how many. A
     * count of failures is the least useful number available when every one of them
     * failed the same way. */
    int first_rc = 0;

    for (unsigned int i = 0; i < obs_module_count; i++) {
        /* Handle 0x0 refuses regardless of layout - a hardware run showed the list
         * starts with it - so naming it would count a handle failure as a layout
         * failure. Skip it; the describable handles come after. */
        if (obs_module_handles[i] == 0) {
            continue;
        }
        unsigned char info[OBS_MODULE_INFO_BYTES];
        for (unsigned int b = 0; b < OBS_MODULE_INFO_BYTES; b++) {
            info[b] = 0;
        }
        /* The leading field is the structure's own size, which is how a platform knows
         * how much it may write. Set before every call, because the call may overwrite
         * it with the size it actually used. */
        info[0] = (unsigned char)(OBS_MODULE_INFO_BYTES & 0xFF);
        info[1] = (unsigned char)((OBS_MODULE_INFO_BYTES >> 8) & 0xFF);

        int rc = sceKernelGetModuleInfo(obs_module_handles[i], info);
        if (rc != 0) {
            if (unreadable == 0) {
                first_rc = rc;
            }
            unreadable++;
            continue;
        }

        char name[OBS_MODULE_NAME_MAX];
        if (obs_copy_name(info, name, sizeof name)) {
            obs_report_module(name, (uint64_t)(uint32_t)obs_module_handles[i]);
            named++;
        } else {
            /* The assumed offset did not find a name. Rather than try another one and
             * report whatever it happens to land on, report the front of the structure
             * as it actually is - the layout is then derived from data instead of
             * guessed, which is the same argument that fixed the dynamic tags.
             *
             * Only the first module, and only the first few words: this is a diagnostic
             * for a layout that is not yet known, not a memory dump. */
            if (unreadable == 0) {
                for (unsigned int w = 0; w < OBS_MODULE_INFO_WORDS; w++) {
                    uint64_t v = 0;
                    for (unsigned int b = 0; b < 8; b++) {
                        v |= (uint64_t)info[w * 8 + b] << (b * 8);
                    }
                    obs_report_module_word(w * 8, v);
                }
            }
            unreadable++;
        }
    }

    if (named == 0) {
        if (first_rc != 0) {
            return obs_fail_code("the platform would not describe any module",
                                 (uint64_t)(uint32_t)first_rc);
        }
        return obs_fail_code("modules were described but not nameably; the layout is "
                             "not what this assumes",
                             (uint64_t)unreadable);
    }
    if (unreadable > 0) {
        return obs_partial_value("some modules could not be named",
                                 (uint64_t)unreadable);
    }
    return obs_pass_value((uint64_t)named);
}

/* ---- Asking the platform, rather than reading an address --------------------
 *
 * # The census cannot tell a stub from an implementation, and this can
 *
 * `900-surface` answers "is this symbol here?" by taking its address, and a loader that
 * resolves every unresolved import to a shared do-nothing stub answers *yes* to all of
 * them - shadPS4 reports 35,337 of 35,337 present for libraries it does not implement,
 * and `900-surface/control` fails on exactly that basis (D156). A resolved address is
 * the loader's opinion.
 *
 * `sceKernelLoadStartModule` is a different question with a different kind of answer.
 * It returns the platform's own error code for a module it does not have, and a handle
 * for one it does. There is no stub to hide behind: either the module was loaded or a
 * number says why not.
 *
 * # Recorded, never asserted
 *
 * Nothing here states an expectation. This program does not know which modules a given
 * firmware ships, and inventing a list of "should be present" would put a guess where
 * the report should carry a fact - the same reasoning `130-layout` and `140-oracle` are
 * built on. Each attempt emits a `measure` record carrying the code the platform
 * returned, and the verdict is only about whether the *mechanism* worked at all.
 *
 * # Why so few modules, and why these
 *
 * Five, all named in the loader diagnostics this project has already collected from its
 * own runs - `libc.prx` and `libSceFios2.prx` are the two shadPS4 names when it refuses
 * the corpus build (D149). Naming a module is free; the cost is the call, and a call
 * that *starts* a module runs its initialiser. Five known names is enough to establish
 * whether the instrument answers at all, and a wider sweep is a decision to make once
 * it has.
 */

/* The path a module is asked for by. Both forms appear in loader diagnostics; the
 * system one is what a firmware carries and the app-local one is what a title ships. */
static const char *const obs_module_paths[] = {
    "/system/common/lib/libc.prx",
    "/system/common/lib/libSceFios2.prx",
    "/system/common/lib/libSceLibcInternal.sprx",
    "/system/common/lib/libSceSysmodule.sprx",
    "/app0/sce_module/libc.prx",
    "/app0/sce_module/libSceFios2.prx",
    "libkernel.prx",
    "libkernel.sprx",
};

/* One quantity name per path, because a report emitting the same quantity five times
 * can say a module loaded without saying which. Built as a table rather than formatted:
 * the runtime has no string formatting and should not grow any for this. */
static const char *const obs_module_quantity[] = {
    "system-libc", "system-fios2", "system-libc-internal", "system-sysmodule",
    "app0-libc",   "app0-fios2",   "libkernel-prx",        "libkernel-sprx",
};

static obs_result check_module_load(void) {
    OBS_REQUIRE(&sceKernelLoadStartModule);

    unsigned int answered = 0;
    unsigned int loaded = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_module_paths); i++) {
        int started = 0;
        /* The trailing arguments are the documented no-argument form: no argv, no
         * flags, no options, and a place to put the module's own return value. */
        int rc = sceKernelLoadStartModule(obs_module_paths[i], 0, 0, 0, 0, &started);
        obs_report_measure("110-modules/load", "sceKernelLoadStartModule",
                           obs_module_quantity[i], (uint64_t)(int64_t)rc, "code");
        answered++;
        /* A non-negative return is a module handle. Negative is the platform saying why
         * not, which is the answer this check exists to collect. */
        if (rc >= 0) {
            loaded++;
        }
    }

    if (answered == 0) {
        return obs_fail("the loader was never reached");
    }
    /* Zero loaded is a real answer, not a failure: a firmwareless loader has no modules
     * to give, and it said so with a code per attempt. What would be a failure is the
     * call never returning, which the announcement above would name. */
    return obs_pass_value((uint64_t)loaded);
}

/* Does a loaded module yield a symbol by name?
 *
 * The other half of the runtime question. A handle alone says a module exists;
 * resolving a name through it says the module carries that symbol - which is what the
 * address census claims and cannot substantiate. Skipped rather than guessed when
 * nothing loaded, because asking a handle nobody obtained is not a question. */
static obs_result check_module_symbol(void) {
    OBS_REQUIRE(&sceKernelLoadStartModule, &sceKernelDlsym);

    int handle = -1;
    for (unsigned int i = 0; i < OBS_COUNT(obs_module_paths); i++) {
        int started = 0;
        int rc = sceKernelLoadStartModule(obs_module_paths[i], 0, 0, 0, 0, &started);
        if (rc >= 0) {
            handle = rc;
            break;
        }
    }
    if (handle < 0) {
        return obs_skip("no module loaded, so there is no handle to ask");
    }

    void *address = (void *)obs_module_symbol(handle, "memcpy");
    int rc = (address != NULL) ? 0 : -1;
    if (address == NULL) {
        char nid[12];
        obs_compute_nid("memcpy", nid);
        rc = sceKernelDlsym(handle, nid, &address);
        if (rc != 0) {
            rc = sceKernelDlsym(handle, "memcpy", &address);
        }
    }
    obs_report_measure("110-modules/symbol", "sceKernelDlsym", "memcpy",
                       (uint64_t)(int64_t)rc, "code");
    if (rc != 0) {
        return obs_partial_value("a module loaded but would not resolve a symbol",
                                 (uint64_t)(int64_t)rc);
    }
    if (address == 0) {
        return obs_fail("dlsym reported success and returned a null address");
    }
    return obs_pass();
}

/* Candidate sizes for the info structure's leading length field.
 *
 * # Why a ladder rather than one value
 *
 * The last hardware run set this field to the buffer size - 1024 - and every module
 * came back refused with the invalid-argument code, while `sceKernelGetModuleList` in
 * the same run happily returned thirty-one handles. So the platform will describe
 * modules; it would not accept the *call*.
 *
 * A kernel filling a caller-supplied structure has to know which layout the caller
 * compiled against, and the usual way it finds out is this field. "Big enough" is then
 * exactly wrong: the value has to match a size the kernel recognises, and 1024 is not
 * one. Sweeping finds the size it wants without anybody having to know the layout first
 * - which is the whole argument for measuring a structure rather than guessing it
 * (D008).
 *
 * The rungs are ordinary structure sizes for something holding a name, a few paths and
 * a handful of segment descriptors, plus round numbers either side. */
static const unsigned int obs_module_info_sizes[] = {
    0x100, 0x110, 0x118, 0x120, 0x130, 0x140, 0x150, 0x160, 0x170,
    0x180, 0x1a0, 0x1a8, 0x1b0, 0x200, 0x220, 0x280, 0x300, 0x400,
};

/* Written past the declared size and checked afterwards, so a call that writes more
 * than it was told is caught here rather than corrupting whatever followed. */
#define OBS_MODULE_GUARD 0xC7

/* The first handle worth describing, or zero if there is none.
 *
 * # Why this is not just handle[0]
 *
 * A hardware run showed the list begins with handle `0x0`, and
 * `sceKernelGetModuleInfo(0, ...)` refuses with the invalid-argument code no matter
 * what size it is handed - so the size ladder built against handle[0] rejected
 * everything and said nothing about the layout, when the layout was never what it was
 * testing (D008: a check that fails for the wrong reason reports a wrong finding). The
 * console enumerates real modules at `0x2`, `0x11`, `0x13` and up; this picks the first
 * of those.
 *
 * `0x2001` is skipped as well: it is a live handle in the list, but it is the one the
 * payloads treat as a system module and its describe behaviour may differ from an
 * ordinary one - so the layout is measured against a plain module first, and the
 * special case is its own question. */
static int obs_first_describable_handle(void) {
    for (unsigned int i = 0; i < obs_module_count; i++) {
        int h = obs_module_handles[i];
        if (h != 0 && h != 0x2001) {
            return h;
        }
    }
    return 0;
}

static obs_result check_module_info_size(void) {
    if (obs_module_count == 0) {
        return obs_skip(
            "the module list was not read, so there is nothing to describe");
    }

    int handle = obs_first_describable_handle();
    if (handle == 0) {
        return obs_skip(
            "no describable handle in the list - only 0x0 and the system handle");
    }

    unsigned int accepted = 0;
    unsigned int smallest_accepted = 0;
    int first_rc = 0;

    for (unsigned int i = 0; i < OBS_COUNT(obs_module_info_sizes); i++) {
        unsigned int size = obs_module_info_sizes[i];
        unsigned char info[OBS_MODULE_INFO_BYTES];
        for (unsigned int b = 0; b < OBS_MODULE_INFO_BYTES; b++) {
            info[b] = (b >= size) ? (unsigned char)OBS_MODULE_GUARD : (unsigned char)0;
        }
        /* The full sixty-four bits, little-endian. The neighbouring check writes only
         * two bytes, which is the same number for the values it used - but a ladder
         * that climbs past sixteen bits would silently stop changing the field. */
        for (unsigned int b = 0; b < 8; b++) {
            info[b] = (unsigned char)((size >> (b * 8)) & 0xFF);
        }

        int rc = sceKernelGetModuleInfo(handle, info);
        obs_report_size("libkernel", "sceKernelGetModuleInfo", 1u, size, rc == 0,
                        (uint64_t)(int64_t)rc);

        if (rc != 0) {
            if (first_rc == 0) {
                first_rc = rc;
            }
            continue;
        }

        /* Accepted. Check the guard before reading anything: a call that overran the
         * size it was given has already written somewhere it should not, and the dump
         * below would be describing damage rather than a layout. */
        int overran = 0;
        for (unsigned int b = size; b < OBS_MODULE_INFO_BYTES; b++) {
            if (info[b] != (unsigned char)OBS_MODULE_GUARD) {
                overran = 1;
                break;
            }
        }
        if (overran) {
            return obs_fail_code("the call wrote past the size it was given",
                                 (uint64_t)size);
        }

        accepted++;
        if (smallest_accepted == 0) {
            smallest_accepted = size;
            /* The first size that works is the one worth dumping, and only that one:
             * the point is a layout, and repeating the dump for every larger size that
             * also works would bury it. */
            obs_report_buffer("110-modules/info-size", "sceKernelGetModuleInfo", "info",
                              info, size);
        }
    }

    if (accepted == 0) {
        return obs_fail_code("no size was accepted", (uint64_t)(uint32_t)first_rc);
    }
    obs_report_measure("110-modules/info-size", "sceKernelGetModuleInfo",
                       "smallest-accepted", (uint64_t)smallest_accepted, "bytes");
    return obs_pass_value((uint64_t)smallest_accepted);
}

/* Every handle the platform handed out, whether or not it can be named.
 *
 * # Why this is worth a check of its own
 *
 * The naming check reports a handle only alongside a name, so a run where naming fails
 * - as the last hardware run did - reports no handles at all. That threw away the more
 * useful half: the last run's `sceKernelLoadStartModule` records showed application
 * modules getting small integers and a system module getting `0x2001`, and the sibling
 * emulator has two payloads dying on exactly that constant, never having identified it.
 * It is a handle.
 *
 * If system and application modules occupy distinct numeric ranges, the ranges are the
 * finding, and they are visible from the list alone without describing anything.
 */
static obs_result check_module_handles(void) {
    if (obs_module_count == 0) {
        return obs_skip("the module list was not read, so there are no handles");
    }

    for (unsigned int i = 0; i < obs_module_count; i++) {
        obs_report_measure("110-modules/handles", "sceKernelGetModuleList", "handle",
                           (uint64_t)(uint32_t)obs_module_handles[i], "handle");
    }
    return obs_pass_value((uint64_t)obs_module_count);
}

static const obs_check module_checks[] = {
    {"110-modules/load", "libkernel", "sceKernelLoadStartModule", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelLoadStartModule, check_module_load,
     OBS_FROM_ASSUMED},
    {"110-modules/symbol", "libkernel", "sceKernelDlsym", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelDlsym, check_module_symbol, OBS_FROM_ASSUMED},

    {"110-modules/list", "libkernel", "sceKernelGetModuleList", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetModuleList, check_module_list,
     OBS_FROM_ASSUMED},
    {"110-modules/names", "libkernel", "sceKernelGetModuleInfo", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetModuleInfo, check_module_names,
     OBS_FROM_ASSUMED},
    {"110-modules/handles", "libkernel", "sceKernelGetModuleList", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetModuleList, check_module_handles,
     OBS_FROM_ASSUMED},
    {"110-modules/info-size", "libkernel", "sceKernelGetModuleInfo", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetModuleInfo, check_module_info_size,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_modules = {
    "110-modules",
    "What is loaded",
    "The platform's own inventory rather than this program's list. On hardware this is "
    "authoritative; against an emulator it is what that emulator claims to provide, "
    "and "
    "the difference between the two is the gap.",
    module_checks,
    OBS_COUNT(module_checks),
};
