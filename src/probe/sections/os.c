/*
 * Operating-system services: files, timers, dynamic linking, user identity.
 *
 * Everything here is reachable once memory and threads work, and everything above
 * here needs at least one of them.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* ---- 040-file -------------------------------------------------------------- */

static obs_result check_close_rejects_bad_descriptor(void) {
    int rc = sceKernelClose(OBS_FD_INVALID);
    if (rc == 0) {
        /* An implementation that closes anything you hand it will silently accept a
         * double close later, and the corruption surfaces somewhere else entirely. */
        return obs_partial("closing an invalid descriptor reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_open_rejects_missing_path(void) {
    OBS_REQUIRE(&sceKernelClose);
    /* A path no filesystem layout places anything at. If this opens, the path
     * resolver is answering rather than looking. */
    int fd = sceKernelOpen("/obscene/definitely/not/here", OBS_O_RDONLY, 0);
    if (fd >= 0) {
        sceKernelClose(fd);
        return obs_fail_code("a nonexistent path opened successfully", (uint64_t)fd);
    }
    return obs_pass_value((uint64_t)(uint32_t)fd);
}

static obs_result check_open_rejects_null_path(void) {
    OBS_REQUIRE(&sceKernelClose);
    int fd = sceKernelOpen(NULL, OBS_O_RDONLY, 0);
    if (fd >= 0) {
        sceKernelClose(fd);
        return obs_fail_code("a null path opened successfully", (uint64_t)fd);
    }
    return obs_pass_value((uint64_t)(uint32_t)fd);
}

static obs_result check_read_rejects_bad_descriptor(void) {
    char buf[16];
    sce_ssize_t n = sceKernelRead(OBS_FD_INVALID, buf, sizeof(buf));
    if (n >= 0) {
        return obs_partial_value("reading an invalid descriptor reported success",
                                 (uint64_t)n);
    }
    return obs_pass_value((uint64_t)n);
}

static obs_result check_lseek_rejects_bad_descriptor(void) {
    sce_off_t off = sceKernelLseek(OBS_FD_INVALID, 0, 0);
    if (off >= 0) {
        return obs_partial_value("seeking an invalid descriptor reported success",
                                 (uint64_t)off);
    }
    return obs_pass_value((uint64_t)off);
}

static int obs_layout_static_marker = 0;

static obs_result check_is_stack(void) {
    /* A local is on the stack and a static is not. Two calls whose answers must differ,
     * with no magic value asserted - the platform is entitled to any non-zero convention
     * for "yes", and this asks only that it tells the two apart.
     *
     * A function that answers the same for both is not reading its argument, which is the
     * failure this shape catches and an existence test cannot. */
    int local = 0;
    void *low = 0;
    void *high = 0;
    int on_stack = sceKernelIsStack(&local, &low, &high);
    int off_stack = sceKernelIsStack(&obs_layout_static_marker, &low, &high);

    if (on_stack == off_stack) {
        return obs_fail_code("a stack address and a static one were reported alike",
                             (uint64_t)(uint32_t)on_stack);
    }
    /* Which of the two means "yes" is the platform's business; that a local is the one it
     * calls a stack address is not. A convention where a static is "on the stack" and a
     * local is not would be wrong whichever numbers it used. */
    if (on_stack == 0) {
        return obs_fail("a local variable was not reported as being on the stack");
    }
    return obs_pass_value((uint64_t)(uint32_t)on_stack);
}

static obs_result check_thread_attributes_round_trip(void) {
    OBS_REQUIRE(&scePthreadAttrDestroy, &scePthreadAttrSetdetachstate,
                &scePthreadAttrGetdetachstate);

    /* Set a value, read it back, and require it to be the one that was set. No document
     * says what the detached constant is, and this never needs to know: it writes a value
     * and asks for the same one back, which is a relation rather than an expectation.
     *
     * An implementation that stores nothing returns whatever it was initialised with and
     * fails this. */
    ScePthreadAttr attr = 0;
    int rc = scePthreadAttrInit(&attr);
    if (rc != 0) {
        return obs_fail_code("thread attributes could not be created",
                             (uint64_t)(uint32_t)rc);
    }

    int initial = -1;
    if (scePthreadAttrGetdetachstate(&attr, &initial) != 0) {
        (void)scePthreadAttrDestroy(&attr);
        return obs_fail("a fresh attribute set would not report its detach state");
    }
    /* Whatever it starts as, ask for the other one. Choosing a constant would be
     * asserting which value means detached, which nothing here establishes. */
    int wanted = (initial == 0) ? 1 : 0;
    if (scePthreadAttrSetdetachstate(&attr, wanted) != 0) {
        (void)scePthreadAttrDestroy(&attr);
        return obs_fail("the detach state could not be set");
    }
    int read_back = -1;
    if (scePthreadAttrGetdetachstate(&attr, &read_back) != 0) {
        (void)scePthreadAttrDestroy(&attr);
        return obs_fail("the detach state could not be read back");
    }
    (void)scePthreadAttrDestroy(&attr);

    if (read_back != wanted) {
        return obs_fail_code("the detach state read back was not the one set",
                             (uint64_t)(uint32_t)read_back);
    }
    return obs_pass_value((uint64_t)(uint32_t)wanted);
}

static const obs_check file_checks[] = {
    {"040-file/close-rejects-bad-fd", "libkernel", "sceKernelClose", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelClose, check_close_rejects_bad_descriptor, OBS_FROM_DERIVED},
    {"040-file/open-rejects-missing", "libkernel", "sceKernelOpen", OBS_CAP_NONE,
     OBS_CAP_FILE, (const void *)&sceKernelOpen, check_open_rejects_missing_path, OBS_FROM_DERIVED},
    {"040-file/open-rejects-null", "libkernel", "sceKernelOpen", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelOpen, check_open_rejects_null_path, OBS_FROM_DERIVED},
    {"040-file/read-rejects-bad-fd", "libkernel", "sceKernelRead", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelRead, check_read_rejects_bad_descriptor, OBS_FROM_DERIVED},
    {"040-file/lseek-rejects-bad-fd", "libkernel", "sceKernelLseek", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelLseek, check_lseek_rejects_bad_descriptor, OBS_FROM_DERIVED},
    {"010-kernel/is-stack", "libkernel", "sceKernelIsStack", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelIsStack, check_is_stack, OBS_FROM_SPEC},
    {"010-kernel/thread-attributes", "libkernel", "scePthreadAttrInit", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadAttrInit,
     check_thread_attributes_round_trip, OBS_FROM_SPEC},
};

const obs_section obs_section_file = {
    "040-file",
    "Filesystem",
    "Descriptor handling, checked from the failure side where no layout is assumed.",
    file_checks,
    OBS_COUNT(file_checks),
};

/* ---- 050-time -------------------------------------------------------------- */

static obs_result check_usleep(void) {
    OBS_REQUIRE(&sceKernelGetProcessTime);
    uint64_t before = sceKernelGetProcessTime();
    int rc = sceKernelUsleep(2000);
    uint64_t after = sceKernelGetProcessTime();
    if (rc != 0) {
        return obs_fail_code("a short sleep was refused", (uint64_t)(uint32_t)rc);
    }
    if (after < before) {
        return obs_fail("the clock went backwards across a sleep");
    }
    /* Process time counts CPU time, so a sleeping thread may legitimately accrue
     * almost none. This checks that the call returns and the clock stays sane, not
     * that it slept for a particular duration - asserting the latter would make the
     * check fail on a loaded host for no useful reason. */
    return obs_pass_value(after - before);
}

static const obs_check time_checks[] = {
    {"050-time/usleep", "libkernel", "sceKernelUsleep", OBS_CAP_TIME, OBS_CAP_NONE,
     (const void *)&sceKernelUsleep, check_usleep, OBS_FROM_DERIVED},
};

const obs_section obs_section_time = {
    "050-time",
    "Timers and sleeping",
    "Yielding the processor and coming back with the clock still monotonic.",
    time_checks,
    OBS_COUNT(time_checks),
};

/* ---- 060-module ------------------------------------------------------------ */

static obs_result check_load_rejects_missing_module(void) {
    int result = 0;
    int rc = sceKernelLoadStartModule("/obscene/no/such/module.sprx", 0, NULL, 0, NULL,
                                      &result);
    if (rc >= 0) {
        return obs_fail_code("a nonexistent module loaded successfully",
                             (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_dlsym_rejects_bad_handle(void) {
    void *address = NULL;
    int rc = sceKernelDlsym(OBS_HANDLE_INVALID, "obscene_no_such_symbol", &address);
    if (rc == 0) {
        return obs_fail_code("a symbol resolved from an invalid module handle",
                             (uint64_t)(uintptr_t)address);
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* The positive of the rejection above: a real symbol, through a real handle, must answer a callable
 * address. Rejecting a bad handle proves argument validation and nothing more - an implementation that
 * fails everything passes that check - so this exercises the path that actually resolves (principle 7).
 * Skips rather than fails wherever the platform cannot present the case (no run-time resolution, or the
 * library not loadable), so a skip reads as "not asked", never "failed". */
static obs_result check_dlsym_resolves_a_known_symbol(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("this platform does not resolve modules by name, so dlsym cannot be given a "
                        "valid handle to resolve through");
    }
    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("libScePad could not be loaded, so there is no valid handle to resolve from");
    }
    void *address = (void *)obs_module_symbol(handle, "scePadOpen");
    if (address == NULL) {
        char nid[12];
        obs_compute_nid("scePadOpen", nid);
        int rc = sceKernelDlsym(handle, nid, &address);
        if (rc != 0) {
            return obs_fail_code("a known symbol did not resolve from a valid module handle",
                                 (uint64_t)(uint32_t)rc);
        }
    }
    /* Non-null is not enough: a placeholder is non-null too, and a jump to one ends the run. The
     * address must be inside a mapped, executable region. */
    if (!obs_address_is_callable(address)) {
        return obs_fail_code("dlsym answered success but not a callable address",
                             (uint64_t)(uintptr_t)address);
    }
    return obs_pass_value((uint64_t)(uintptr_t)address);
}

static obs_result check_sysmodule_reports_unloaded(void) {
    /* Identifier 0 is not a loadable system module. The question is whether the
     * query answers at all, not what it answers. */
    int rc = sceSysmoduleIsLoaded(0);
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* Does loading a module at run time resolve an import the loader left unresolved?
 *
 * # Why this is the question, and not an idle one
 *
 * On a console this program is a title, and a title is given fewer libraries than it declares:
 * of twelve `DT_NEEDED`, five were mapped. Every check behind the other seven has a null
 * address and skips. The census, asking the same console for the same libraries through
 * `sceKernelLoadStartModule`, found `libScePad`, `libSceAudioOut` and `libSceGnmDriver` **fully
 * present** - every symbol it knows about resolves. The platform has them; this program cannot
 * reach them.
 *
 * There are two possible repairs and they are very different sizes. If a run-time load makes
 * the outstanding imports bind, the fix is to load early and every skipped section works. If it
 * does not, each check has to resolve and call through a pointer, which is a change to hundreds
 * of them. **Nothing should be built until this is answered**, so it is answered here, as a
 * check, on the platform rather than by reasoning about what a loader probably does.
 *
 * `scePadOpen` is the subject because it is in a library the console has and this title was not
 * given, which is exactly the case in question. A platform that maps it after all makes this
 * check skip rather than lie - the address is tested first. (D236)
 */

static obs_result check_runtime_load_binds_imports(void) {
    if (obs_address_is_callable((const void *)&scePadOpen)) {
        return obs_skip("this platform linked the library already, so there is no outstanding "
                        "import to repair");
    }
    if (!obs_module_resolution_works()) {
        return obs_skip("this platform does not resolve modules by name, so the question "
                        "cannot be put");
    }

    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("the library could not be loaded at run time either, so nothing was "
                        "established about binding");
    }
    /* Present through the handle but still null as an import is the whole finding: the symbol
     * is reachable and the linkage is not repaired. */
    int through_handle = obs_module_symbol(handle, "scePadOpen") != NULL;
    int now_bound = obs_address_is_callable((const void *)&scePadOpen);

    if (!through_handle) {
        return obs_skip("the library loaded but did not offer the symbol, so binding was "
                        "never tested");
    }
    if (now_bound) {
        return obs_pass_value(1);
    }
    return obs_fail("loading the library at run time does not bind an import the loader left "
                    "unresolved; a check can only reach it through a resolved pointer");
}

static const obs_check module_checks[] = {
    {"060-module/runtime-load-binds-imports", "libkernel", "sceKernelLoadStartModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelLoadStartModule,
     check_runtime_load_binds_imports, OBS_FROM_ASSUMED},
    {"060-module/load-rejects-missing", "libkernel", "sceKernelLoadStartModule",
     OBS_CAP_FILE, OBS_CAP_MODULE, (const void *)&sceKernelLoadStartModule,
     check_load_rejects_missing_module, OBS_FROM_ASSUMED},
    {"060-module/dlsym-rejects-bad-handle", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_dlsym_rejects_bad_handle, OBS_FROM_ASSUMED},
    {"060-module/dlsym-resolves-known-symbol", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_dlsym_resolves_a_known_symbol, OBS_FROM_ASSUMED},
    {"060-module/sysmodule-query", "libSceSysmodule", "sceSysmoduleIsLoaded",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceSysmoduleIsLoaded,
     check_sysmodule_reports_unloaded, OBS_FROM_ASSUMED},
};

const obs_section obs_section_module = {
    "060-module",
    "Dynamic linking",
    "Loading modules and resolving symbols, checked from the failure side.",
    module_checks,
    OBS_COUNT(module_checks),
};

/* ---- 070-user -------------------------------------------------------------- */

static obs_result check_user_service_init(void) {
    int rc = sceUserServiceInitialize(NULL);
    if (rc != 0) {
        return obs_fail_code("the user service refused to initialise",
                             (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_initial_user(void) {
    int32_t user = -1;
    int rc = sceUserServiceGetInitialUser(&user);
    if (rc != 0) {
        return obs_fail_code("no initial user could be determined",
                             (uint64_t)(uint32_t)rc);
    }
    if (user < 0) {
        /* Every subsystem below takes a user identifier. Success with a negative one
         * means each of them will be opened against a user that does not exist. */
        return obs_fail_code("success reported alongside an invalid user identifier",
                             (uint64_t)(uint32_t)user);
    }
    return obs_pass_value((uint64_t)(uint32_t)user);
}

static const obs_check user_checks[] = {
    {"070-user/initialise", "libSceUserService", "sceUserServiceInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceInitialize,
     check_user_service_init, OBS_FROM_ASSUMED},
    {"070-user/initial-user", "libSceUserService", "sceUserServiceGetInitialUser",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceGetInitialUser,
     check_initial_user, OBS_FROM_ASSUMED},
};

const obs_section obs_section_user = {
    "070-user",
    "User service",
    "The user identity every presentation subsystem is opened against.",
    user_checks,
    OBS_COUNT(user_checks),
};
