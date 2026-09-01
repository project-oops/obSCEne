/* ---- 111-modlink -----------------------------------------------------------
 *
 * The loaded-module inventory, read from the runtime linker's own link-map, with names and
 * bases - the answer `110-modules` cannot give in a locked context.
 *
 * # Why this sits next to 110-modules rather than inside it
 *
 * `110-modules` asks the platform. `sceKernelGetModuleList` returns a count of handles, and
 * `sceKernelGetModuleInfo` is meant to name each one. On this console, in the compatibility
 * context obSCEne is loaded into, that second call is refused for every buffer size tried -
 * `0x160` included - with `0x80020016`, and `sceKernelDlsym` is refused too. So the platform
 * hands back *how many* modules are loaded and not one of their names, which is exactly the
 * fact that decides a previous-generation host from a current one: is the current-generation
 * GPU library mapped, or only the previous one?
 *
 * This section asks nobody. It walks the runtime linker's own list of loaded objects through
 * `obs_linkmap_walk` (src/runtime.c), reading each object's name and base straight from memory
 * the loader already wrote - no syscall to be refused. The same walk backs `obs_run_context`,
 * which stamps every report with the execution context this section confirms.
 *
 * # Provenance - nothing here is invented or vendor-derived
 *
 * The layout the walk reads is standard and cited: the ELF dynamic array and `DT_DEBUG = 21`
 * are the ELF ABI; `r_debug` (`r_map` at offset 8) is FreeBSD `<sys/link_elf.h>`; `link_map`
 * (`l_addr` at 0, `l_name` at 8, `l_next` at 0x18) is FreeBSD `<link.h>`. The console is
 * FreeBSD-derived - the same citation the directory walk uses for `dirent`. The names are ABI
 * identifiers the loader stored; the bases are read off the machine. Nothing is copied off the
 * box, only the finding is reported.
 *
 * # It reports, it does not depend
 *
 * If the loader does not populate `DT_DEBUG` - some do not - `obs_linkmap_walk` returns zero
 * and the check skips saying so, rather than dereferencing a guessed pointer. A walk that does
 * run writes each `OBS|module|<name>|<base>` as it goes, so a fault partway through leaves a
 * record of how far it reached: the same discipline every probe here follows.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Long enough for a full sandbox path to a `.sprx`, which is what a name usually is. */
#define OBS_MODLINK_NAME_MAX 160

/* Copy a C string from a loader-written pointer, bounded, stopping at the first NUL and
 * refusing anything non-printable so a wrong pointer produces a visible "?" rather than bytes
 * pasted into a stream other tools parse. Returns the length, or 0 for nothing usable. */
static unsigned int obs_copy_cstr(const char *src, char *out, unsigned int max) {
    if (src == (const char *)0 || out == (char *)0 || max == 0) {
        return 0;
    }
    unsigned int n = 0;
    while (n + 1 < max) {
        unsigned char c = (unsigned char)src[n];
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
    return n;
}

/* Does `hay` contain `needle`? A tiny freestanding search, so the GPU verdict needs no libc
 * string call. */
static int obs_contains(const char *hay, const char *needle) {
    for (unsigned int i = 0; hay[i] != '\0'; i++) {
        unsigned int j = 0;
        while (needle[j] != '\0' && hay[i + j] == needle[j]) {
            j++;
        }
        if (needle[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

struct modlink_scan {
    unsigned int count;
    int agc;
    int gnm;
};

/* One loaded object: report it, and note whether it is a GPU library. Written before the walk
 * advances, so a fault on the next node still leaves this one on the stream. */
static int modlink_report_cb(const char *name, unsigned long base, void *user) {
    struct modlink_scan *s = (struct modlink_scan *)user;
    char buf[OBS_MODLINK_NAME_MAX];
    unsigned int have = obs_copy_cstr(name, buf, sizeof buf);
    obs_report_module(have ? buf : "?", (uint64_t)base);
    if (have) {
        if (obs_contains(buf, "libSceAgc")) {
            s->agc = 1;
        }
        if (obs_contains(buf, "libSceGnm")) {
            s->gnm = 1;
        }
    }
    s->count++;
    return 0; /* walk all */
}

static obs_result check_link_map(void) {
    struct modlink_scan scan = {0, 0, 0};
    const char *why = "ok";
    unsigned int walked = obs_linkmap_walk(modlink_report_cb, &scan, &why);
    if (walked == 0) {
        /* `why` names the stage: "own ELF header not resident" is the same cause as the
         * ELF-scan resolver failing (imports do not bind either); "DT_DEBUG not populated" is
         * the link-map being unavailable for an object a runtime linker never processed. The
         * distinction decides whether the fix is the context or the method. */
        return obs_skip(why);
    }

    /* The GPU verdict this whole section exists for: which generation's graphics library is
     * actually mapped in this process, decided by observing the loaded set rather than by
     * asking a call the compatibility host refuses. */
    if (scan.agc) {
        obs_report_sysinfo("modlink/gpu", "current-generation GPU library is mapped",
                           "libSceAgc is present among loaded modules - a native host");
    } else if (scan.gnm) {
        obs_report_sysinfo("modlink/gpu", "only the previous-generation GPU library is mapped",
                           "libSceGnm present, libSceAgc absent - a compatibility host");
    } else {
        obs_report_sysinfo("modlink/gpu", "no GPU library is mapped",
                           "neither libSceAgc nor libSceGnm is among the loaded modules");
    }
    return obs_pass_value((uint64_t)scan.count);
}

static const obs_check modlink_checks[] = {
    {"111-modlink/walk", "obscene", "(link-map walk)", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_link_map, OBS_FROM_DERIVED},
};

const obs_section obs_section_modlink = {
    "111-modlink",
    "Loaded modules, by link-map",
    "Reads the runtime linker's own list of loaded objects - names and bases - straight from "
    "memory, so the inventory survives a host that refuses sceKernelGetModuleInfo. Reports which "
    "generation's GPU library is actually mapped. Behaviour, not contents.",
    modlink_checks,
    OBS_COUNT(modlink_checks),
};
