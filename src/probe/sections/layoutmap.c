/*
 * Where things actually are - the platform's address-space layout, measured.
 *
 * # The gap this fills
 *
 * Every other section that touches the symbol census reports whether a name is *present*.
 * None reports *where*. That was fine while the question was "does this function exist" and
 * useless the moment the question became "what is at resolver + 0x2885e00" - which is exactly
 * the question the open-toolchain payloads pose. They resolve one function, add a
 * firmware-specific offset measured in tens of megabytes, and use the result; an emulator
 * that knows every function exists but not where any of them sits cannot follow them a single
 * step (see the sibling project's orbistoun#D404).
 *
 * # What it measures
 *
 * A spread of named functions across the platform's major libraries, each resolved to its
 * run-time address. The addresses are the finding: functions cluster by the module they live
 * in, so a column of them reveals where each module was loaded and how far it spans, and the
 * distance between two of them is the kind of fixed offset a payload bakes in. One run of this
 * on hardware is a map that an emulator can lay its own modules out against, or at least read
 * a payload's arithmetic against.
 *
 * # Why by name and not by walking modules
 *
 * `sceKernelGetModuleInfo` would give bases directly, but its structure layout is not yet
 * known (that is its own probe). `dlsym` needs no layout - it answers an address for a name -
 * so this maps the space today rather than after the struct is cracked. The two will
 * cross-check when both have run.
 *
 * # Nothing here is a vendor header
 *
 * The names are ABI identifiers, resolved at run time; the addresses are read off the machine.
 * No layout, offset or base is declared by this program - it asks and reports.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* The handle the payloads resolve through first. On hardware it behaves as a global search;
 * the payloads fall back to the libkernel handle when it misses, and so does this. */
#define OBS_SEARCH_HANDLE 1
/* libkernel's module handle, confirmed by its position in the module list and by
 * LoadStartModule returning it. The fallback when the global search does not answer. */
#define OBS_LIBKERNEL_HANDLE 0x2001

/* Names chosen to span the libraries a payload or title actually touches, so the addresses
 * they resolve to sample the whole loaded layout rather than one corner of it. Grouped only
 * for the reader; the resolve does not care which library a name is in, and reporting which
 * one it landed near is the point. */
static const char *const obs_layout_names[] = {
    /* The resolver itself and the gadget donor - the two every payload starts from, so the
     * base its offsets are measured against is in here. */
    "sceKernelDlsym",
    "getpid",
    /* Core libkernel: memory, threads, timing, modules. */
    "sceKernelAllocateDirectMemory",
    "sceKernelMapDirectMemory",
    "sceKernelDirectMemoryQuery",
    "sceKernelGetDirectMemorySize",
    "scePthreadCreate",
    "scePthreadJoin",
    "sceKernelUsleep",
    "sceKernelGetProcessTime",
    "sceKernelReadTsc",
    "sceKernelLoadStartModule",
    "sceKernelGetModuleList",
    "sceKernelGetModuleInfo",
    "sceKernelOpen",
    "sceKernelRead",
    "sceKernelWrite",
    "sceKernelClose",
    /* The C library the console ships, both the public and the internal spelling. */
    "memcpy",
    "memset",
    "strlen",
    "malloc",
    "free",
    "snprintf",
    "printf",
    /* Higher libraries, to reach the far ends of the space. */
    "sceSysmoduleLoadModule",
    "sceUserServiceGetLoginUserIdList",
    "sceVideoOutOpen",
    "sceAudioOutOpen",
    "scePadOpen",
};

/* Resolves one name and reports its address, answering whether it was found.
 *
 * Tries the global search first, then the libkernel handle, exactly as the payloads do - so a
 * name reported absent here is one they would not have resolved either. */
static int obs_layout_resolve(const char *name) {
    void *address = (void *)obs_module_symbol(1, name);
    if (address == NULL) {
        int rc = sceKernelDlsym(OBS_SEARCH_HANDLE, name, &address);
        if (rc != 0 || address == 0) {
            rc = sceKernelDlsym(OBS_LIBKERNEL_HANDLE, name, &address);
        }
    }
    if (address == 0) {
        return 0;
    }
    obs_report_measure("138-layout/addresses", name, "address", (uint64_t)(uintptr_t)address, "address");
    return 1;
}

static obs_result check_layout_addresses(void) {
    OBS_REQUIRE(&sceKernelDlsym);

    unsigned int found = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_layout_names); i++) {
        if (obs_layout_resolve(obs_layout_names[i])) {
            found++;
        }
    }

    if (found == 0) {
        return obs_fail("nothing resolved, so no address could be mapped");
    }
    if (found < OBS_COUNT(obs_layout_names)) {
        return obs_partial_value("some names did not resolve", (uint64_t)found);
    }
    return obs_pass_value((uint64_t)found);
}

static obs_result check_layout_np_cpp_webapi(void) {
    OBS_REQUIRE(&sceKernelDlsym);

    int handle = 0;
    if (&sceKernelLoadStartModule != NULL) {
        handle = sceKernelLoadStartModule("libSceNpCppWebApi.sprx", 0, NULL, 0, NULL, NULL);
    }

    void *init_addr = NULL;
    void *target_nid_addr = NULL;

    /* 1. Try global handle 1, then specific handle if loaded */
    int rc1 = sceKernelDlsym(OBS_SEARCH_HANDLE,
        "_ZN3sce2Np9CppWebApi6Common10initializeERKNS2_10InitParamsERNS2_10LibContextE",
        &init_addr);
    if ((rc1 != 0 || init_addr == NULL) && handle > 0) {
        sceKernelDlsym(handle,
            "_ZN3sce2Np9CppWebApi6Common10initializeERKNS2_10InitParamsERNS2_10LibContextE",
            &init_addr);
    }

    /* 2. Try the unknown leaderboard NID 0xa9721c01ca796f63 */
    static const char *const nid_aliases[] = {
        "0xa9721c01ca796f63",
        "a9721c01ca796f63",
        "#a9721c01ca796f63",
        NULL
    };

    for (int i = 0; nid_aliases[i] != NULL && target_nid_addr == NULL; i++) {
        int r = sceKernelDlsym(OBS_SEARCH_HANDLE, nid_aliases[i], &target_nid_addr);
        if ((r != 0 || target_nid_addr == NULL) && handle > 0) {
            sceKernelDlsym(handle, nid_aliases[i], &target_nid_addr);
        }
    }

    if (init_addr != NULL) {
        obs_report_measure("138-layout/np-cpp-webapi", "Common::initialize", "address",
                           (uint64_t)(uintptr_t)init_addr, "address");
    }

    if (target_nid_addr != NULL) {
        obs_report_measure("138-layout/np-cpp-webapi", "0xa9721c01ca796f63", "address",
                           (uint64_t)(uintptr_t)target_nid_addr, "address");
        return obs_pass_value((uint64_t)(uintptr_t)target_nid_addr);
    }

    if (init_addr != NULL) {
        return obs_partial_value("libSceNpCppWebApi present, but target NID not resolved by name",
                                 (uint64_t)(uintptr_t)init_addr);
    }

    return obs_skip("libSceNpCppWebApi not loaded in this process context");
}

static const obs_check layoutmap_checks[] = {
    {"138-layout/addresses", "libkernel", "sceKernelDlsym", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelDlsym, check_layout_addresses, OBS_FROM_ASSUMED},
    {"138-layout/np-cpp-webapi", "libkernel", "sceKernelDlsym", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelDlsym, check_layout_np_cpp_webapi, OBS_FROM_ASSUMED},
};

const obs_section obs_section_layoutmap = {
    "138-layout",
    "Where things are",
    "A spread of named functions resolved to their run-time addresses, so a run maps the "
    "platform's loaded layout - the one thing a payload's firmware-offset arithmetic needs and "
    "no presence census can give.",
    layoutmap_checks,
    OBS_COUNT(layoutmap_checks),
};
