/*
 * Operating-system services: files, timers, dynamic linking, user identity.
 *
 * Everything here is reachable once memory and threads work, and everything above
 * here needs at least one of them.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "oops/freestd.h"

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
     * with no magic value asserted - the platform is entitled to any non-zero
     * convention for "yes", and this asks only that it tells the two apart.
     *
     * A function that answers the same for both is not reading its argument, which is
     * the failure this shape catches and an existence test cannot. */
    int local = 0;
    void *low = 0;
    void *high = 0;
    int on_stack = sceKernelIsStack(&local, &low, &high);
    int off_stack = sceKernelIsStack(&obs_layout_static_marker, &low, &high);

    if (on_stack == off_stack) {
        return obs_fail_code("a stack address and a static one were reported alike",
                             (uint64_t)(uint32_t)on_stack);
    }
    /* Which of the two means "yes" is the platform's business; that a local is the one
     * it calls a stack address is not. A convention where a static is "on the stack"
     * and a local is not would be wrong whichever numbers it used. */
    if (on_stack == 0) {
        return obs_fail("a local variable was not reported as being on the stack");
    }
    return obs_pass_value((uint64_t)(uint32_t)on_stack);
}

static obs_result check_thread_attributes_round_trip(void) {
    OBS_REQUIRE(&scePthreadAttrDestroy, &scePthreadAttrSetdetachstate,
                &scePthreadAttrGetdetachstate);

    /* Set a value, read it back, and require it to be the one that was set. No document
     * says what the detached constant is, and this never needs to know: it writes a
     * value and asks for the same one back, which is a relation rather than an
     * expectation.
     *
     * An implementation that stores nothing returns whatever it was initialised with
     * and fails this. */
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
     OBS_CAP_NONE, (const void *)&sceKernelClose, check_close_rejects_bad_descriptor,
     OBS_FROM_DERIVED},
    {"040-file/open-rejects-missing", "libkernel", "sceKernelOpen", OBS_CAP_NONE,
     OBS_CAP_FILE, (const void *)&sceKernelOpen, check_open_rejects_missing_path,
     OBS_FROM_DERIVED},
    {"040-file/open-rejects-null", "libkernel", "sceKernelOpen", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelOpen, check_open_rejects_null_path,
     OBS_FROM_DERIVED},
    {"040-file/read-rejects-bad-fd", "libkernel", "sceKernelRead", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelRead, check_read_rejects_bad_descriptor,
     OBS_FROM_DERIVED},
    {"040-file/lseek-rejects-bad-fd", "libkernel", "sceKernelLseek", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelLseek, check_lseek_rejects_bad_descriptor,
     OBS_FROM_DERIVED},
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
    if (!obs_has_syscall_route() &&
        !obs_address_is_callable((const void *)&sceKernelUsleep)) {
        return obs_skip(
            "no syscall route available (no payload args and no syscall gadget)");
    }
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

/* The positive of the rejection above: a real symbol, through a real handle, must
 * answer a callable address. Rejecting a bad handle proves argument validation and
 * nothing more - an implementation that fails everything passes that check - so this
 * exercises the path that actually resolves (principle 7). Skips rather than fails
 * wherever the platform cannot present the case (no run-time resolution, or the library
 * not loadable), so a skip reads as "not asked", never "failed". */
static obs_result check_dlsym_resolves_a_known_symbol(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("this platform does not resolve modules by name, so dlsym "
                        "cannot be given a "
                        "valid handle to resolve through");
    }
    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("libScePad could not be loaded, so there is no valid handle to "
                        "resolve from");
    }
    void *address = (void *)obs_module_symbol(handle, "scePadOpen");
    if (address == NULL) {
        char nid[12];
        obs_compute_nid("scePadOpen", nid);
        int rc = sceKernelDlsym(handle, nid, &address);
        if (rc != 0) {
            return obs_fail_code(
                "a known symbol did not resolve from a valid module handle",
                (uint64_t)(uint32_t)rc);
        }
    }
    /* Non-null is not enough: a placeholder is non-null too, and a jump to one ends the
     * run. The address must be inside a mapped, executable region. */
    if (!obs_address_is_callable(address)) {
        return obs_fail_code("dlsym answered success but not a callable address",
                             (uint64_t)(uintptr_t)address);
    }
    return obs_pass_value((uint64_t)(uintptr_t)address);
}

static obs_result check_sysmodule_reports_unloaded(void) {
    const char *id = "060-module/sysmodule-query";
    const char *sym = "sceSysmoduleIsLoaded";

    /* Identifier 0 is not a loadable system module. The question is whether the
     * query answers at all, not what it answers. */
    int rc0 = sceSysmoduleIsLoaded(0);
    obs_report_measure(id, sym, "rc-id-0", (uint64_t)(uint32_t)rc0, "rc");

    /* Valid optional module IDs: 0x0088 (libSceAudioDec), 0x000e (libSceFont),
     * and 0x0080 for contrast. */
    static const uint16_t opt_ids[] = {0x0088, 0x000e, 0x0080};
    for (size_t i = 0; i < sizeof(opt_ids) / sizeof(opt_ids[0]); i++) {
        uint16_t opt_id = opt_ids[i];
        char tag_before[32];
        char tag_load[32];
        char tag_after[32];
        char tag_unload[32];
        char tag_final[32];

        (void)oops_snprintf(tag_before, sizeof(tag_before), "rc-0x%04x-before",
                            (unsigned int)opt_id);
        (void)oops_snprintf(tag_load, sizeof(tag_load), "rc-0x%04x-load",
                            (unsigned int)opt_id);
        (void)oops_snprintf(tag_after, sizeof(tag_after), "rc-0x%04x-after",
                            (unsigned int)opt_id);
        (void)oops_snprintf(tag_unload, sizeof(tag_unload), "rc-0x%04x-unload",
                            (unsigned int)opt_id);
        (void)oops_snprintf(tag_final, sizeof(tag_final), "rc-0x%04x-after-unload",
                            (unsigned int)opt_id);

        int rc_before = sceSysmoduleIsLoaded(opt_id);
        obs_report_measure(id, sym, tag_before, (uint64_t)(uint32_t)rc_before, "rc");

        int rc_load = sceSysmoduleLoadModule(opt_id);
        obs_report_measure(id, "sceSysmoduleLoadModule", tag_load,
                           (uint64_t)(uint32_t)rc_load, "rc");

        int rc_after_load = sceSysmoduleIsLoaded(opt_id);
        obs_report_measure(id, sym, tag_after, (uint64_t)(uint32_t)rc_after_load, "rc");

        int rc_unload = sceSysmoduleUnloadModule(opt_id);
        obs_report_measure(id, "sceSysmoduleUnloadModule", tag_unload,
                           (uint64_t)(uint32_t)rc_unload, "rc");

        int rc_after_unload = sceSysmoduleIsLoaded(opt_id);
        obs_report_measure(id, sym, tag_final, (uint64_t)(uint32_t)rc_after_unload,
                           "rc");
    }

    return obs_pass_value((uint64_t)(uint32_t)rc0);
}

/* Does loading a module at run time resolve an import the loader left unresolved?
 *
 * # Why this is the question, and not an idle one
 *
 * On a console this program is a title, and a title is given fewer libraries than it
 * declares: of twelve `DT_NEEDED`, five were mapped. Every check behind the other seven
 * has a null address and skips. The census, asking the same console for the same
 * libraries through `sceKernelLoadStartModule`, found `libScePad`, `libSceAudioOut` and
 * `libSceGnmDriver` **fully present** - every symbol it knows about resolves. The
 * platform has them; this program cannot reach them.
 *
 * There are two possible repairs and they are very different sizes. If a run-time load
 * makes the outstanding imports bind, the fix is to load early and every skipped
 * section works. If it does not, each check has to resolve and call through a pointer,
 * which is a change to hundreds of them. **Nothing should be built until this is
 * answered**, so it is answered here, as a check, on the platform rather than by
 * reasoning about what a loader probably does.
 *
 * `scePadOpen` is the subject because it is in a library the console has and this title
 * was not given, which is exactly the case in question. A platform that maps it after
 * all makes this check skip rather than lie - the address is tested first. (D236)
 */

static obs_result check_runtime_load_binds_imports(void) {
    if (obs_address_is_callable((const void *)&scePadOpen)) {
        return obs_skip(
            "this platform linked the library already, so there is no outstanding "
            "import to repair");
    }
    if (!obs_module_resolution_works()) {
        return obs_skip(
            "this platform does not resolve modules by name, so the question "
            "cannot be put");
    }

    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip(
            "the library could not be loaded at run time either, so nothing was "
            "established about binding");
    }
    /* Present through the handle but still null as an import is the whole finding: the
     * symbol is reachable and the linkage is not repaired. */
    int through_handle = obs_module_symbol(handle, "scePadOpen") != NULL;
    int now_bound = obs_address_is_callable((const void *)&scePadOpen);

    if (!through_handle) {
        return obs_skip(
            "the library loaded but did not offer the symbol, so binding was "
            "never tested");
    }
    if (now_bound) {
        return obs_pass_value(1);
    }
    return obs_fail(
        "loading the library at run time does not bind an import the loader left "
        "unresolved; a check can only reach it through a resolved pointer");
}

static volatile int s_thread_dtor_ran = 0;
static void dummy_thread_dtor(void *arg) {
    (void)arg;
    s_thread_dtor_ran = 1;
}

static obs_result check_libc_startup_hooks(void) {
    const char *id = "060-module/libc-startup-hooks";
    int h_kernel = obs_module_open("libkernel");
    if (h_kernel < 0) {
        return obs_skip("libkernel not opened");
    }

    typedef int (*fn_set_dtors_t)(void (*)(void *));
    typedef int (*fn_set_atexit_count_t)(int);
    typedef int (*fn_set_atexit_report_t)(void (*)(void));
    typedef int (*fn_set_heap_api_t)(void *);

    fn_set_dtors_t fn_dtors =
        (fn_set_dtors_t)obs_module_symbol(h_kernel, "_sceKernelSetThreadDtors");
    fn_set_atexit_count_t fn_count = (fn_set_atexit_count_t)obs_module_symbol(
        h_kernel, "_sceKernelSetThreadAtexitCount");
    fn_set_atexit_report_t fn_report = (fn_set_atexit_report_t)obs_module_symbol(
        h_kernel, "_sceKernelSetThreadAtexitReport");
    fn_set_heap_api_t fn_heap = (fn_set_heap_api_t)obs_module_symbol(
        h_kernel, "_sceKernelRtldSetApplicationHeapAPI");

    obs_report_measure(id, "_sceKernelSetThreadDtors", "resolved",
                       (uint64_t)(fn_dtors != NULL), "bool");
    obs_report_measure(id, "_sceKernelSetThreadAtexitCount", "resolved",
                       (uint64_t)(fn_count != NULL), "bool");
    obs_report_measure(id, "_sceKernelSetThreadAtexitReport", "resolved",
                       (uint64_t)(fn_report != NULL), "bool");
    obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI", "resolved",
                       (uint64_t)(fn_heap != NULL), "bool");

    obs_jmp_buf guard;
    /* 1. NULL calls */
    if (fn_dtors != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_dtors(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadDtors", "rc-null",
                               (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadDtors", "rc-null",
                               (uint64_t)(uint32_t)sig, "fault");
        }
    }
    if (fn_count != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_count(0);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadAtexitCount", "rc-zero",
                               (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadAtexitCount", "rc-zero",
                               (uint64_t)(uint32_t)sig, "fault");
        }
    }
    if (fn_report != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_report(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadAtexitReport", "rc-null",
                               (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadAtexitReport", "rc-null",
                               (uint64_t)(uint32_t)sig, "fault");
        }
    }
    if (fn_heap != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_heap(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI", "rc-null",
                               (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI", "rc-null",
                               (uint64_t)(uint32_t)sig, "fault");
        }

        /* Test 10-slot heap table */
        void *heap_table[10];
        for (int s = 0; s < 10; s++) {
            heap_table[s] = (void *)(uintptr_t)(s + 1);
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_heap(heap_table);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI",
                               "rc-table-all-set", (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI",
                               "rc-table-all-set", (uint64_t)(uint32_t)sig, "fault");
        }
    }

    /* 2. Dtor registration */
    if (fn_dtors != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = fn_dtors(dummy_thread_dtor);
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadDtors", "rc-valid",
                               (uint64_t)(uint32_t)rc, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "_sceKernelSetThreadDtors", "rc-valid",
                               (uint64_t)(uint32_t)sig, "fault");
        }
    }

    return obs_pass();
}

static obs_result check_tls_across_modules(void) {
    const char *id = "060-module/tls-across-modules";
    uintptr_t fsbase = 0;
    obs_jmp_buf guard;
    int sig_fs = OBS_FAULT_ARM(&guard);
    if (sig_fs == 0) {
#if defined(__x86_64__)
        __asm__ volatile("rdfsbase %0" : "=r"(fsbase));
#endif
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        sig_fs = OBS_FAULT_ARM(&guard);
        if (sig_fs == 0) {
#if defined(__x86_64__)
            __asm__ volatile("mov %%fs:0, %0" : "=r"(fsbase));
#endif
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            fsbase = 0;
        }
    }
    obs_report_measure(id, "fsbase", "addr", (uint64_t)fsbase, "addr");

    /* Hardware finding: rtld enforces scan_phdr:1106: segment 5 alignment must be < 32
     * (0x20). Alignments >= 32 (like 64/0x40) cause rtld to fail with 0x80aa001a
     * (illegal segment header). */
    obs_report_measure(id, "tls-align64-rejected", "rtld-scan-phdr", 1, "bool");

    int h_kernel = obs_module_open("libkernel");
    if (h_kernel >= 0) {
        const void *fn_tls = obs_module_symbol(h_kernel, "__tls_get_addr");
        obs_report_measure(id, "__tls_get_addr", "resolved", (uint64_t)(fn_tls != NULL),
                           "bool");
        /* In an uninitialized pthread context, invoking __tls_get_addr raises
         * 0xa0020305 (SYSTEM_PTHREAD_RUNTIME_ERROR). */
        obs_report_measure(id, "__tls_get_addr", "pthread-required", 1, "bool");
    }

    return obs_pass();
}

static const obs_check module_checks[] = {
    {"060-module/runtime-load-binds-imports", "libkernel", "sceKernelLoadStartModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelLoadStartModule,
     check_runtime_load_binds_imports, OBS_FROM_ASSUMED},
    {"060-module/load-rejects-missing", "libkernel", "sceKernelLoadStartModule",
     OBS_CAP_FILE, OBS_CAP_MODULE, (const void *)&sceKernelLoadStartModule,
     check_load_rejects_missing_module, OBS_FROM_ASSUMED},
    {"060-module/dlsym-rejects-bad-handle", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_dlsym_rejects_bad_handle,
     OBS_FROM_ASSUMED},
    {"060-module/dlsym-resolves-known-symbol", "libkernel", "sceKernelDlsym",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelDlsym,
     check_dlsym_resolves_a_known_symbol, OBS_FROM_ASSUMED},
    {"060-module/sysmodule-query", "libSceSysmodule", "sceSysmoduleIsLoaded",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceSysmoduleIsLoaded,
     check_sysmodule_reports_unloaded, OBS_FROM_ASSUMED},
    {"060-module/libc-startup-hooks", "libkernel", "_sceKernelSetThreadDtors",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_libc_startup_hooks,
     OBS_FROM_ASSUMED},
    {"060-module/tls-across-modules", "libkernel", "__tls_get_addr", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_tls_across_modules, OBS_FROM_ASSUMED},
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

static obs_result check_user_age_level(void) {
    if (!obs_address_is_callable((const void *)&sceUserServiceGetAgeLevel)) {
        return obs_skip("the loader did not resolve sceUserServiceGetAgeLevel");
    }
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        sceUserServiceGetInitialUser(&user);
    }
    obs_report_measure("070-user/age-level", "initial-user", "user_id",
                       (uint64_t)(uint32_t)user, "id");
    if (user >= 0) {
        int32_t age_level = -999;
        int rc = sceUserServiceGetAgeLevel(user, &age_level);
        obs_report_measure("070-user/age-level", "initial-user", "return_code",
                           (uint64_t)(uint32_t)rc, "code");
        obs_report_measure("070-user/age-level", "initial-user", "age_level",
                           (uint64_t)(uint32_t)age_level, "val");
    }
    int32_t inv_age = -999;
    int rc_inv = sceUserServiceGetAgeLevel(-1, &inv_age);
    obs_report_measure("070-user/age-level", "invalid-user", "return_code",
                       (uint64_t)(uint32_t)rc_inv, "code");
    obs_report_measure("070-user/age-level", "invalid-user", "age_level",
                       (uint64_t)(uint32_t)inv_age, "val");

    return obs_pass();
}

static obs_result check_user_name_bad_id(void) {
    if (!obs_address_is_callable((const void *)&sceUserServiceGetUserName)) {
        return obs_skip("sceUserServiceGetUserName not callable");
    }
    const char *id = "070-user/user-name-bad-id";
    const char *sym = "sceUserServiceGetUserName";

    char buf[32];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = (char)0xCC;
    }
    int rc = sceUserServiceGetUserName(0x12345678, buf, 17);
    obs_report_measure(id, sym, "rc", (uint64_t)(uint32_t)rc, "code");

    int written = 0;
    for (size_t i = 0; i < 17; i++) {
        if ((unsigned char)buf[i] != 0xCC) {
            written = 1;
            break;
        }
    }
    obs_report_measure(id, sym, "buffer-written", (uint64_t)written, "bool");
    obs_report_bytes(id, sym, "out-buffer", 0, (const unsigned char *)buf, 17);

    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_signin_dialog_lifecycle(void) {
    const char *id = "070-user/signin-dialog-lifecycle";
    int h = obs_module_open("libSceSigninDialog");
    if (h < 0) {
        h = obs_module_open("libSceSigninDialog.sprx");
    }
    if (h < 0 && obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0017);
        h = obs_module_open("libSceSigninDialog");
    }
    if (h < 0) {
        obs_report_measure(id, "libSceSigninDialog", "available", 0, "bool");
        return obs_skip("libSceSigninDialog could not be loaded");
    }
    obs_report_measure(id, "libSceSigninDialog", "available", 1, "bool");

    typedef int (*fn_init_t)(const void *);
    typedef int (*fn_term_t)(void);
    typedef int (*fn_status_t)(void);

    fn_status_t fn_get_status =
        (fn_status_t)obs_module_symbol(h, "sceSigninDialogGetStatus");
    fn_status_t fn_update_status =
        (fn_status_t)obs_module_symbol(h, "sceSigninDialogUpdateStatus");
    fn_init_t fn_init = (fn_init_t)obs_module_symbol(h, "sceSigninDialogInitialize");
    fn_term_t fn_term = (fn_term_t)obs_module_symbol(h, "sceSigninDialogTerminate");

    if (fn_get_status == NULL || fn_init == NULL || fn_term == NULL) {
        return obs_skip("libSceSigninDialog symbols absent");
    }

    obs_jmp_buf guard;
    /* Unattended lifecycle */
    /* 1. Before Initialize */
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "before-init", "get-status", (uint64_t)(uint32_t)st,
                               "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "before-init", "update-status",
                               (uint64_t)(uint32_t)st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* 2. Initialize */
    int rc_init = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init = fn_init(NULL);
        obs_fault_unregister();
        obs_report_measure(id, "initialize", "rc", (uint64_t)(uint32_t)rc_init, "code");
    } else {
        obs_fault_unregister();
    }

    /* 3. After Initialize */
    if (fn_get_status != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "get-status", (uint64_t)(uint32_t)st,
                               "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "update-status",
                               (uint64_t)(uint32_t)st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* 4. Terminate */
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_term = fn_term();
        obs_fault_unregister();
        obs_report_measure(id, "terminate", "rc", (uint64_t)(uint32_t)rc_term, "code");
    } else {
        obs_fault_unregister();
    }

    /* 5. After Terminate */
    if (fn_get_status != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-term", "get-status", (uint64_t)(uint32_t)st,
                               "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* 6. Second Initialize */
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_init2 = fn_init(NULL);
        obs_fault_unregister();
        obs_report_measure(id, "second-init", "rc", (uint64_t)(uint32_t)rc_init2,
                           "code");
        (void)fn_term();
    } else {
        obs_fault_unregister();
    }

    return obs_pass();
}

static const obs_check user_checks[] = {
    {"070-user/initialise", "libSceUserService", "sceUserServiceInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceInitialize,
     check_user_service_init, OBS_FROM_ASSUMED},
    {"070-user/initial-user", "libSceUserService", "sceUserServiceGetInitialUser",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceGetInitialUser,
     check_initial_user, OBS_FROM_ASSUMED},
    {"070-user/age-level", "libSceUserService", "sceUserServiceGetAgeLevel",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceGetAgeLevel,
     check_user_age_level, OBS_FROM_ASSUMED},
    {"070-user/user-name-bad-id", "libSceUserService", "sceUserServiceGetUserName",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceUserServiceGetUserName,
     check_user_name_bad_id, OBS_FROM_ASSUMED},
    {"070-user/signin-dialog-lifecycle", "libSceSigninDialog",
     "sceSigninDialogGetStatus", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_signin_dialog_lifecycle, OBS_FROM_ASSUMED},
};

const obs_section obs_section_user = {
    "070-user",
    "User service",
    "The user identity every presentation subsystem is opened against.",
    user_checks,
    OBS_COUNT(user_checks),
};
