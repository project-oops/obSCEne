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
#include "oops/krw.h"
#include "oops/time.h"

OBS_WEAK int scePadGetHandle(int32_t userId, int32_t type, int32_t index);
OBS_WEAK int sceSigninDialogGetStatus_import(void) __asm__("$2m077aeC-PA");
OBS_WEAK int sceSigninDialogGetStatus_alt(void) __asm__("$8PiCp-07bdo");
OBS_WEAK int sceSigninDialogUpdateStatus_import(void) __asm__("$cE-FJJb1DQc");
OBS_WEAK int sceSigninDialogInitialize_import(const void *) __asm__("$mlYGfmqE3fQ");
OBS_WEAK int sceSigninDialogTerminate_import(void) __asm__("$LXlmS6PvJdU");

OBS_WEAK int sceAjmInitialize(uint64_t, void *) __asm__("$dl+4eHSzUu4");
OBS_WEAK int sceAjmModuleRegister(uint32_t, uint32_t, uint64_t) __asm__("$Q3dyFuwGn64");
OBS_WEAK int sceAjmFinalize(uint32_t) __asm__("$MHur6qCsUus");
OBS_WEAK int sceSystemServiceParamGetInt(int32_t, int32_t *) __asm__("$riz76fI4mn0");
OBS_WEAK int sceAudioOut2UserCreate(int32_t, uint64_t, void *) __asm__("$xywYcRB7nbQ");
OBS_WEAK int sceAudioOut2UserDestroy(int32_t) __asm__("$IaZXJ9M79uo");
OBS_WEAK int sceSystemServiceGetNoticeScreenSkipFlag(void *) __asm__("$3RQ5aQfnstU");
OBS_WEAK int sceKernelConvertLocaltimeToUtc(uint64_t, uint64_t, void *, void *,
                                            void *) __asm__("$0NTHN1NKONI");
OBS_WEAK int sceUserServiceGetAccessibilityVibration(int32_t,
                                                     void *) __asm__("$qWYHOFwqCxY");
OBS_WEAK int
sceUserServiceGetAccessibilityTriggerEffect(int32_t, void *) __asm__("$-3Y5GO+-i78");
OBS_WEAK int sceNpGetAccountCountryA(int32_t, void *) __asm__("$JT+t00a3TxA");
OBS_WEAK int sceAppContentInitialize(void *) __asm__("$R9lA82OraNs");
OBS_WEAK int
sceAppContentDownloadDataGetAvailableSpaceKb(const char *,
                                             void *) __asm__("$Gl6w5i0JokY");

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

static obs_result check_convert_utc_to_localtime(void) {
    if (!obs_address_is_callable((const void *)&sceKernelConvertUtcToLocaltime)) {
        return obs_skip("sceKernelConvertUtcToLocaltime is not callable");
    }

    const char *id = "050-time/convert-utc-to-localtime";
    const char *sym = "sceKernelConvertUtcToLocaltime";

    /* Fixed test timestamp: 1791288000LL = 2026-10-06 12:00:00 UTC */
    int64_t test_utc = 1791288000LL;
    int64_t local = 0;
    uint8_t timesec_buf[64];
    uint8_t dst_buf[64];
    memset(timesec_buf, 0xcc, sizeof(timesec_buf));
    memset(dst_buf, 0xcc, sizeof(dst_buf));

    obs_jmp_buf guard;
    int rc = -1;
    if (OBS_FAULT_ARM(&guard) == 0) {
        rc = sceKernelConvertUtcToLocaltime(
            test_utc, &local, (int *)(void *)timesec_buf, (int *)(void *)dst_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, sym, "fault", 1u, "bool");
        return obs_fail("fault during sceKernelConvertUtcToLocaltime");
    }

    obs_report_measure(id, sym, "rc", (uint64_t)(uint32_t)rc, "rc");
    obs_report_measure(id, sym, "utc-input", (uint64_t)test_utc, "seconds");
    obs_report_measure(id, sym, "local-time", (uint64_t)local, "seconds");
    obs_report_measure(id, sym, "calculated-offset",
                       (uint64_t)(int64_t)(local - test_utc), "seconds");
    obs_report_bytes(id, sym, "timesec-buf-32b", 0, timesec_buf, 32);
    obs_report_bytes(id, sym, "dst-buf-32b", 0, dst_buf, 32);

    int32_t offset_s = 0;
    int offset_rc = oops_time_utc_offset(test_utc, &offset_s);
    obs_report_measure(id, sym, "oops-offset-rc", (uint64_t)(uint32_t)offset_rc, "rc");
    obs_report_measure(id, sym, "oops-offset-seconds", (uint64_t)(int64_t)offset_s,
                       "seconds");

    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static const obs_check time_checks[] = {
    {"050-time/usleep", "libkernel", "sceKernelUsleep", OBS_CAP_TIME, OBS_CAP_NONE,
     (const void *)&sceKernelUsleep, check_usleep, OBS_FROM_DERIVED},
    {"050-time/convert-utc-to-localtime", "libkernel", "sceKernelConvertUtcToLocaltime",
     OBS_CAP_TIME, OBS_CAP_NONE, (const void *)&sceKernelConvertUtcToLocaltime,
     check_convert_utc_to_localtime, OBS_FROM_ASSUMED},
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
    (void)obs_sysmodule_load_id(0x80000024);
    (void)obs_sysmodule_load_id(0x0027);

    if (!obs_address_is_callable((const void *)&sceKernelDlsym)) {
        return obs_skip("sceKernelDlsym is not callable");
    }
    int (*fn_dlsym)(int, const char *, void **) =
        (int (*)(int, const char *, void **))&sceKernelDlsym;

    int handle = -1;
    if (obs_sysmodule_get_handle_id(0x80000024, &handle) != 0 || handle <= 1) {
        (void)obs_sysmodule_get_handle_id(0x0027, &handle);
    }
    if (handle <= 1) {
        handle = obs_find_module_handle_by_library("libScePad");
    }
    if (handle <= 1) {
        handle = obs_module_open("libScePad");
    }

    static const struct {
        const char *full_nid;
        const char *bare_nid;
        const char *name;
    } pad_candidates[] = {
        {"q1cHNfGycLI#H#I", "q1cHNfGycLI", "scePadRead"},
        {"u1GRHp+oWoY#H#I", "u1GRHp+oWoY", "scePadGetHandle"},
        {"RR4novUEENY#H#I", "RR4novUEENY", "scePadSetLightBar"},
        {"yFVnOdGxvZY#H#I", "yFVnOdGxvZY", "scePadSetVibration"},
        {"n3kSX62fgNo#H#I", "n3kSX62fgNo", "0xda809fad5f12799f"},
        {"xk0AcarP3V4#H#I", "xk0AcarP3V4", "scePadOpen"},
    };

    const void *address = NULL;
    if (handle > 1) {
        for (size_t i = 0; i < sizeof(pad_candidates) / sizeof(pad_candidates[0]);
             i++) {
            address = obs_dlsym_probe_handle(
                fn_dlsym, handle, pad_candidates[i].full_nid,
                pad_candidates[i].bare_nid, pad_candidates[i].name);
            if (address != NULL && obs_address_is_callable(address)) {
                break;
            }
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        for (size_t i = 0; i < sizeof(pad_candidates) / sizeof(pad_candidates[0]);
             i++) {
            address = obs_dlsym_probe_handle(
                fn_dlsym, 0x2001, pad_candidates[i].full_nid,
                pad_candidates[i].bare_nid, pad_candidates[i].name);
            if (address != NULL && obs_address_is_callable(address)) {
                break;
            }
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
            int mlist[128];
            size_t mcount = 0;
            if (sceKernelGetModuleList(mlist, 128, &mcount) == 0 && mcount > 0) {
                for (size_t m = 0; m < mcount && m < 128; m++) {
                    int mid = mlist[m];
                    if (mid <= 1) {
                        continue;
                    }
                    for (size_t i = 0;
                         i < sizeof(pad_candidates) / sizeof(pad_candidates[0]); i++) {
                        address = obs_dlsym_probe_handle(
                            fn_dlsym, mid, pad_candidates[i].full_nid,
                            pad_candidates[i].bare_nid, pad_candidates[i].name);
                        if (address != NULL && obs_address_is_callable(address)) {
                            handle = mid;
                            obs_sysmodule_cache_handle("libScePad", mid);
                            break;
                        }
                    }
                    if (address != NULL && obs_address_is_callable(address)) {
                        break;
                    }
                }
            }
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        for (int mid = 2; mid <= 256; mid++) {
            for (size_t i = 0; i < sizeof(pad_candidates) / sizeof(pad_candidates[0]);
                 i++) {
                address = obs_dlsym_probe_handle(
                    fn_dlsym, mid, pad_candidates[i].full_nid,
                    pad_candidates[i].bare_nid, pad_candidates[i].name);
                if (address != NULL && obs_address_is_callable(address)) {
                    handle = mid;
                    obs_sysmodule_cache_handle("libScePad", mid);
                    break;
                }
            }
            if (address != NULL && obs_address_is_callable(address)) {
                break;
            }
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        for (size_t i = 0; i < sizeof(pad_candidates) / sizeof(pad_candidates[0]);
             i++) {
            address =
                obs_module_symbol(handle > 1 ? handle : -1, pad_candidates[i].name);
            if (address != NULL && obs_address_is_callable(address)) {
                break;
            }
            address =
                obs_module_symbol(handle > 1 ? handle : -1, pad_candidates[i].bare_nid);
            if (address != NULL && obs_address_is_callable(address)) {
                break;
            }
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        if (obs_address_is_callable((const void *)&scePadRead)) {
            address = (const void *)&scePadRead;
        } else if (obs_address_is_callable((const void *)&scePadGetHandle)) {
            address = (const void *)&scePadGetHandle;
        }
    }

    if (address == NULL || !obs_address_is_callable(address)) {
        return obs_fail("a known symbol did not resolve from a valid module handle");
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

        obs_report_measure(id, "_sceKernelRtldSetApplicationHeapAPI",
                           "rc-table-all-set", 0x39e70, "code");
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
        uint8_t age_buf[0x40];
        memset(age_buf, 0xa5, sizeof(age_buf));
        int rc = sceUserServiceGetAgeLevel(user, (int32_t *)(void *)age_buf);
        obs_report_measure("070-user/age-level", "initial-user", "return_code",
                           (uint64_t)(uint32_t)rc, "code");
        obs_report_measure("070-user/age-level", "initial-user", "age_level",
                           *(const uint64_t *)(const void *)age_buf, "val");
        obs_report_bytes("070-user/age-level", "initial-user", "out-40h", 0, age_buf,
                         sizeof(age_buf));
    }
    int32_t inv_age = -999;
    int rc_inv = sceUserServiceGetAgeLevel(-1, &inv_age);
    obs_report_measure("070-user/age-level", "invalid-user", "return_code",
                       (uint64_t)(uint32_t)rc_inv, "code");
    obs_report_measure("070-user/age-level", "invalid-user", "age_level",
                       (uint64_t)(uint32_t)inv_age, "val");

    return obs_pass();
}

/* REQ-20261009T2215Z-ao01: register-zero calling helpers for AudioOut2 */
static inline int ao2_call_zero_regs(const void *fn) {
#if defined(__x86_64__)
    int rc = -1;
    __asm__ volatile("xor %%edi, %%edi\n\t"
                     "xor %%esi, %%esi\n\t"
                     "xor %%edx, %%edx\n\t"
                     "xor %%ecx, %%ecx\n\t"
                     "xor %%r8d, %%r8d\n\t"
                     "xor %%r9d, %%r9d\n\t"
                     "call *%1\n\t"
                     : "=a"(rc)
                     : "r"(fn)
                     : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "memory");
    return rc;
#else
    return ((int (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))fn)(
        0, 0, 0, 0, 0, 0);
#endif
}

static inline int ao2_call_user_create(const void *fn, int32_t user, void *out) {
#if defined(__x86_64__)
    int rc = -1;
    __asm__ volatile("movl %1, %%edi\n\t"
                     "movq %2, %%rsi\n\t"
                     "xor %%edx, %%edx\n\t"
                     "xor %%ecx, %%ecx\n\t"
                     "xor %%r8d, %%r8d\n\t"
                     "xor %%r9d, %%r9d\n\t"
                     "call *%3\n\t"
                     : "=a"(rc)
                     : "r"(user), "r"(out), "r"(fn)
                     : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "memory");
    return rc;
#else
    return ((int (*)(int32_t, void *, uint64_t, uint64_t, uint64_t, uint64_t))fn)(
        user, out, 0, 0, 0, 0);
#endif
}

static inline int ao2_call_user_destroy(const void *fn, int32_t handle) {
#if defined(__x86_64__)
    int rc = -1;
    __asm__ volatile("movl %1, %%edi\n\t"
                     "xor %%esi, %%esi\n\t"
                     "xor %%edx, %%edx\n\t"
                     "xor %%ecx, %%ecx\n\t"
                     "xor %%r8d, %%r8d\n\t"
                     "xor %%r9d, %%r9d\n\t"
                     "call *%2\n\t"
                     : "=a"(rc)
                     : "r"(handle), "r"(fn)
                     : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "memory");
    return rc;
#else
    return ((int (*)(int32_t))fn)(handle);
#endif
}

static inline int ao2_call_rdi_buffer(const void *fn, void *buf) {
#if defined(__x86_64__)
    int rc = -1;
    __asm__ volatile("movq %1, %%rdi\n\t"
                     "xor %%esi, %%esi\n\t"
                     "xor %%edx, %%edx\n\t"
                     "xor %%ecx, %%ecx\n\t"
                     "xor %%r8d, %%r8d\n\t"
                     "xor %%r9d, %%r9d\n\t"
                     "call *%2\n\t"
                     : "=a"(rc)
                     : "r"(buf), "r"(fn)
                     : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "memory");
    return rc;
#else
    return ((int (*)(void *, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))fn)(
        buf, 0, 0, 0, 0, 0);
#endif
}

static obs_result check_oneshot_setup_calls(void) {
    const char *id = "070-user/oneshot-setup-calls";
    obs_jmp_buf guard;

    int32_t init_user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        (void)sceUserServiceGetInitialUser(&init_user);
    }
    if (init_user < 0) {
        init_user = 0x1ea2f4d9; /* Known initial user fallback */
    }
    obs_report_measure(id, "setup", "initial-user", (uint64_t)(uint32_t)init_user,
                       "id");

    /* (3) FIRST: sceUserServiceGetAgeLevel(initial user, &out) */
    const void *fn_age = census_resolve_symbol(
        "libSceUserService", "$woNpu+45RLk", "sceUserServiceGetAgeLevel", NULL,
        (const void *)&sceUserServiceGetAgeLevel);
    obs_report_measure(id, "arm3-age-level", "resolved", (uint64_t)(fn_age != NULL),
                       "bool");
    if (fn_age && obs_address_is_callable(fn_age)) {
        uint8_t out[0x40];
        memset(out, 0xa5, sizeof(out));
        uint8_t out_before[0x40];
        memcpy(out_before, out, sizeof(out_before));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(int32_t, void *))fn_age)(init_user, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm3-age-level", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "arm3-age-level", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "arm3-age-level", "out-40h", 0, out, sizeof(out));
        obs_report_written(id, "sceUserServiceGetAgeLevel", "out-diff", out_before, out,
                           sizeof(out));
    }

    /* (1) sceAjmInitialize(0, &ctx) with ctx prefilled 0xff, then 2nd call */
    const void *fn_ajm =
        census_resolve_symbol("libSceAjm", "$7lKzdHi4X3Y", "sceAjmInitialize",
                              (const void *)&sceAjmInitialize, NULL);
    if (!fn_ajm) {
        fn_ajm = census_resolve_symbol("libSceAjm", "$dl+4eHSzUu4", NULL,
                                       (const void *)&sceAjmInitialize, NULL);
    }
    obs_report_measure(id, "arm1-ajm-init", "resolved", (uint64_t)(fn_ajm != NULL),
                       "bool");
    if (fn_ajm && obs_address_is_callable(fn_ajm)) {
        uint8_t ctx1[0x40];
        memset(ctx1, 0xff, sizeof(ctx1));
        uint8_t ctx1_before[0x40];
        memcpy(ctx1_before, ctx1, sizeof(ctx1_before));
        int sig = OBS_FAULT_ARM(&guard);
        int rc1 = -1;
        if (sig == 0) {
            rc1 = ((int (*)(uint64_t, void *))fn_ajm)(0, ctx1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm1-ajm-init", "call1-fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm1-ajm-init", "call1-rc", (uint64_t)(uint32_t)rc1,
                           "code");
        obs_report_bytes(id, "arm1-ajm-init", "call1-ctx", 0, ctx1, sizeof(ctx1));
        obs_report_written(id, "sceAjmInitialize", "call1-diff", ctx1_before, ctx1,
                           sizeof(ctx1));

        /* Second call with fresh slot ctx2 */
        uint8_t ctx2[0x40];
        memset(ctx2, 0xff, sizeof(ctx2));
        sig = OBS_FAULT_ARM(&guard);
        int rc2 = -1;
        if (sig == 0) {
            rc2 = ((int (*)(uint64_t, void *))fn_ajm)(0, ctx2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm1-ajm-init", "call2-fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm1-ajm-init", "call2-rc", (uint64_t)(uint32_t)rc2,
                           "code");
        obs_report_bytes(id, "arm1-ajm-init", "call2-ctx", 0, ctx2, sizeof(ctx2));

        /* (3) REQ-20261009T1430Z-aj01: sceAjmModuleRegister(ctx, codec, 0) */
        const void *fn_reg =
            census_resolve_symbol("libSceAjm", "$rp8G7BZyd0M", "sceAjmModuleRegister",
                                  (const void *)&sceAjmModuleRegister, NULL);
        if (!fn_reg) {
            fn_reg = census_resolve_symbol("libSceAjm", "$Q3dyFuwGn64",
                                           "sceAjmModuleRegister",
                                           (const void *)&sceAjmModuleRegister, NULL);
        }
        obs_report_measure(id, "arm1-ajm-module-register", "resolved",
                           (uint64_t)(fn_reg != NULL), "bool");
        uint32_t ctx1_id = *(uint32_t *)ctx1;
        if (fn_reg && obs_address_is_callable(fn_reg) && rc1 == 0) {
            for (uint32_t codec = 0; codec < 4; codec++) {
                char tag[32], tag_rpt[32];
                oops_snprintf(tag, sizeof(tag), "codec-%u-rc", codec);
                oops_snprintf(tag_rpt, sizeof(tag_rpt), "codec-%u-repeat-rc", codec);
                int r_reg = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    r_reg = ((int (*)(uint32_t, uint32_t, uint64_t))fn_reg)(ctx1_id,
                                                                            codec, 0);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "arm1-ajm-module-register", tag,
                                   (uint64_t)(uint32_t)r_reg, "code");

                /* Repeat call for same codec */
                int r_rpt = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    r_rpt = ((int (*)(uint32_t, uint32_t, uint64_t))fn_reg)(ctx1_id,
                                                                            codec, 0);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "arm1-ajm-module-register", tag_rpt,
                                   (uint64_t)(uint32_t)r_rpt, "code");
            }
        }

        /* (4) REQ-20261009T1430Z-aj01: sceAjmFinalize(ctx) */
        const void *fn_fin =
            census_resolve_symbol("libSceAjm", "$61KsoOqrezA", "sceAjmFinalize",
                                  (const void *)&sceAjmFinalize, NULL);
        if (!fn_fin) {
            fn_fin =
                census_resolve_symbol("libSceAjm", "$MHur6qCsUus", "sceAjmFinalize",
                                      (const void *)&sceAjmFinalize, NULL);
        }
        obs_report_measure(id, "arm1-ajm-finalize", "resolved",
                           (uint64_t)(fn_fin != NULL), "bool");
        if (fn_fin && obs_address_is_callable(fn_fin)) {
            int r_fin1 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                r_fin1 = ((int (*)(uint32_t))fn_fin)(ctx1_id);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "arm1-ajm-finalize", "ctx1-rc",
                               (uint64_t)(uint32_t)r_fin1, "code");

            if (rc2 == 0) {
                uint32_t ctx2_id = *(uint32_t *)ctx2;
                int r_fin2 = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    r_fin2 = ((int (*)(uint32_t))fn_fin)(ctx2_id);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "arm1-ajm-finalize", "ctx2-rc",
                                   (uint64_t)(uint32_t)r_fin2, "code");
            }
        }
    }

    /* (2) REQ-20261009T2215Z-ao01: sceAudioOut2Initialize and sceAudioOut2UserCreate
     * before and after module load */
    const void *fn_ao_init = census_resolve_symbol(
        "libSceAudioOut2", "$g2tViFIohHE", "sceAudioOut2Initialize",
        (const void *)&sceAudioOut2Initialize_static, NULL);
    const void *fn_ao_create = census_resolve_symbol(
        "libSceAudioOut2", "$xywYcRB7nbQ", "sceAudioOut2UserCreate",
        (const void *)&sceAudioOut2UserCreate, NULL);
    const void *fn_ao_destroy = census_resolve_symbol(
        "libSceAudioOut2", "$IaZXJ9M79uo", "sceAudioOut2UserDestroy",
        (const void *)&sceAudioOut2UserDestroy, NULL);
    if (!fn_ao_destroy) {
        fn_ao_destroy = census_resolve_symbol(
            "libSceAudioOut2", "$6vY70ydXpiE", "sceAudioOut2UserDestroy",
            (const void *)&sceAudioOut2UserDestroy, NULL);
    }
    obs_report_measure(id, "arm2-audio-out2-noload", "init-resolved",
                       (uint64_t)(fn_ao_init != NULL), "bool");
    obs_report_measure(id, "arm2-audio-out2-noload", "create-resolved",
                       (uint64_t)(fn_ao_create != NULL), "bool");
    obs_report_measure(id, "arm2-audio-out2-noload", "destroy-resolved",
                       (uint64_t)(fn_ao_destroy != NULL), "bool");

    int sig_ao = 0;

    /* Pass A: Without module load */
    if (fn_ao_init && obs_address_is_callable(fn_ao_init)) {
        sig_ao = OBS_FAULT_ARM(&guard);
        int rc_init1_noload = -1;
        if (sig_ao == 0) {
            rc_init1_noload = ao2_call_zero_regs(fn_ao_init);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-noload", "init1-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-noload", "init1-rc",
                           (uint64_t)(uint32_t)rc_init1_noload, "code");

        sig_ao = OBS_FAULT_ARM(&guard);
        int rc_init2_noload = -1;
        if (sig_ao == 0) {
            rc_init2_noload = ao2_call_zero_regs(fn_ao_init);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-noload", "init2-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-noload", "init2-rc",
                           (uint64_t)(uint32_t)rc_init2_noload, "code");
    }

    if (fn_ao_create && obs_address_is_callable(fn_ao_create)) {
        uint8_t out_noload[0x40];
        memset(out_noload, 0xa5, sizeof(out_noload));
        sig_ao = OBS_FAULT_ARM(&guard);
        int rc_create_noload = -1;
        if (sig_ao == 0) {
            rc_create_noload =
                ao2_call_user_create(fn_ao_create, init_user, out_noload);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-noload", "create-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-noload", "create-rc",
                           (uint64_t)(uint32_t)rc_create_noload, "code");
        obs_report_bytes(id, "arm2-audio-out2-noload", "out-40h", 0, out_noload,
                         sizeof(out_noload));
    }

    /* Sysmodule load candidate probes */
    int rc_mod_1c = -1;
    int rc_mod_1d = -1;
    int rc_mod_801c = -1;
    int rc_mod_sprx = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_mod_1c = sceSysmoduleLoadModule(0x001c);
        rc_mod_1d = sceSysmoduleLoadModule(0x001d);
    }
    rc_mod_801c = obs_sysmodule_load_id(0x8000001c);
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        int started = 0;
        rc_mod_sprx = sceKernelLoadStartModule(
            "/system/common/lib/libSceAudioOut2.sprx", 0, NULL, 0, NULL, &started);
    }
    obs_report_measure(id, "arm2-audio-out2-load", "sysmodule-001c-rc",
                       (uint64_t)(uint32_t)rc_mod_1c, "code");
    obs_report_measure(id, "arm2-audio-out2-load", "sysmodule-001d-rc",
                       (uint64_t)(uint32_t)rc_mod_1d, "code");
    obs_report_measure(id, "arm2-audio-out2-load", "sysmodule-8000001c-rc",
                       (uint64_t)(uint32_t)rc_mod_801c, "code");
    obs_report_measure(id, "arm2-audio-out2-load", "sprx-load-rc",
                       (uint64_t)(uint32_t)rc_mod_sprx, "code");

    /* Re-resolve symbols after load */
    fn_ao_init = census_resolve_symbol(
        "libSceAudioOut2", "$g2tViFIohHE", "sceAudioOut2Initialize",
        (const void *)&sceAudioOut2Initialize_static, NULL);
    fn_ao_create = census_resolve_symbol("libSceAudioOut2", "$xywYcRB7nbQ",
                                         "sceAudioOut2UserCreate",
                                         (const void *)&sceAudioOut2UserCreate, NULL);
    fn_ao_destroy = census_resolve_symbol("libSceAudioOut2", "$IaZXJ9M79uo",
                                          "sceAudioOut2UserDestroy",
                                          (const void *)&sceAudioOut2UserDestroy, NULL);
    if (!fn_ao_destroy) {
        fn_ao_destroy = census_resolve_symbol(
            "libSceAudioOut2", "$6vY70ydXpiE", "sceAudioOut2UserDestroy",
            (const void *)&sceAudioOut2UserDestroy, NULL);
    }
    obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-resolved",
                       (uint64_t)(fn_ao_destroy != NULL), "bool");
    obs_report_measure(id, "arm2-audio-out2", "destroy-resolved",
                       (uint64_t)(fn_ao_destroy != NULL), "bool");

    /* Pass B: With module load */
    int rc_init1_loaded = -1;
    int rc_init2_loaded = -1;
    if (fn_ao_init && obs_address_is_callable(fn_ao_init)) {
        sig_ao = OBS_FAULT_ARM(&guard);
        if (sig_ao == 0) {
            rc_init1_loaded = ao2_call_zero_regs(fn_ao_init);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-loaded", "init1-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-loaded", "init1-rc",
                           (uint64_t)(uint32_t)rc_init1_loaded, "code");

        sig_ao = OBS_FAULT_ARM(&guard);
        if (sig_ao == 0) {
            rc_init2_loaded = ao2_call_zero_regs(fn_ao_init);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-loaded", "init2-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-loaded", "init2-rc",
                           (uint64_t)(uint32_t)rc_init2_loaded, "code");

        /* (3) REQ-20261010T1030Z-ao02: Initialize with rdi pointing at 0x40-byte buffer
         * filled 0xa5 */
        uint8_t ao_init_buf[0x40];
        memset(ao_init_buf, 0xa5, sizeof(ao_init_buf));
        sig_ao = OBS_FAULT_ARM(&guard);
        int rc_init3_loaded = -1;
        if (sig_ao == 0) {
            rc_init3_loaded = ao2_call_rdi_buffer(fn_ao_init, ao_init_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-loaded", "init3-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-loaded", "init3-rc",
                           (uint64_t)(uint32_t)rc_init3_loaded, "code");
        obs_report_bytes(id, "arm2-audio-out2-loaded", "init3-buf-40h", 0, ao_init_buf,
                         sizeof(ao_init_buf));
    }

    uint8_t out_loaded[0x40];
    memset(out_loaded, 0xa5, sizeof(out_loaded));
    uint8_t out_loaded_before[0x40];
    memcpy(out_loaded_before, out_loaded, sizeof(out_loaded_before));
    int rc_create_loaded = -1;
    if (fn_ao_create && obs_address_is_callable(fn_ao_create)) {
        sig_ao = OBS_FAULT_ARM(&guard);
        if (sig_ao == 0) {
            rc_create_loaded =
                ao2_call_user_create(fn_ao_create, init_user, out_loaded);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-loaded", "create-fault",
                               (uint64_t)sig_ao, "signal");
        }
    }
    obs_report_measure(id, "arm2-audio-out2-loaded", "create-rc",
                       (uint64_t)(uint32_t)rc_create_loaded, "code");
    obs_report_bytes(id, "arm2-audio-out2-loaded", "out-40h", 0, out_loaded,
                     sizeof(out_loaded));
    obs_report_written(id, "sceAudioOut2UserCreate", "out-diff", out_loaded_before,
                       out_loaded, sizeof(out_loaded));
    obs_report_measure(id, "arm2-audio-out2", "user-created",
                       (uint64_t)(uint32_t)rc_create_loaded, "id");

    /* (4) sceAudioOut2UserDestroy */
    if (fn_ao_destroy && obs_address_is_callable(fn_ao_destroy)) {
        uint32_t handle_val = *(uint32_t *)out_loaded;
        int rc_dest = -1;
        if (handle_val != 0xa5a5a5a5) {
            sig_ao = OBS_FAULT_ARM(&guard);
            if (sig_ao == 0) {
                rc_dest = ao2_call_user_destroy(fn_ao_destroy, (int32_t)handle_val);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-handle-fault",
                                   (uint64_t)sig_ao, "signal");
            }
            obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-handle-rc",
                               (uint64_t)(uint32_t)rc_dest, "code");
        }

        if (rc_create_loaded >= 0 && (uint32_t)rc_create_loaded != handle_val) {
            int rc_dest_rc = -1;
            sig_ao = OBS_FAULT_ARM(&guard);
            if (sig_ao == 0) {
                rc_dest_rc = ao2_call_user_destroy(fn_ao_destroy, rc_create_loaded);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-retcode-rc",
                               (uint64_t)(uint32_t)rc_dest_rc, "code");
        }
        obs_report_measure(id, "arm2-audio-out2", "destroy-user-rc",
                           (uint64_t)(uint32_t)rc_dest, "code");

        /* sceAudioOut2UserDestroy(0) */
        sig_ao = OBS_FAULT_ARM(&guard);
        int rc_dest0 = -1;
        if (sig_ao == 0) {
            rc_dest0 = ao2_call_user_destroy(fn_ao_destroy, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-zero-fault",
                               (uint64_t)sig_ao, "signal");
        }
        obs_report_measure(id, "arm2-audio-out2-loaded", "destroy-zero-rc",
                           (uint64_t)(uint32_t)rc_dest0, "code");
        obs_report_measure(id, "arm2-audio-out2", "destroy-zero-rc",
                           (uint64_t)(uint32_t)rc_dest0, "code");
    }

    /* (4) sceSystemServiceGetNoticeScreenSkipFlag(&out) */
    const void *fn_skip = census_resolve_symbol(
        "libSceSystemService", "$3RQ5aQfnstU",
        "sceSystemServiceGetNoticeScreenSkipFlag",
        (const void *)&sceSystemServiceGetNoticeScreenSkipFlag, NULL);
    obs_report_measure(id, "arm4-notice-skip", "resolved", (uint64_t)(fn_skip != NULL),
                       "bool");
    if (fn_skip && obs_address_is_callable(fn_skip)) {
        uint8_t out[0x40];
        memset(out, 0xa5, sizeof(out));
        uint8_t out_before[0x40];
        memcpy(out_before, out, sizeof(out_before));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(void *))fn_skip)(out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm4-notice-skip", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm4-notice-skip", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "arm4-notice-skip", "out-40h", 0, out, sizeof(out));
        obs_report_written(id, "sceSystemServiceGetNoticeScreenSkipFlag", "out-diff",
                           out_before, out, sizeof(out));
    }

    /* (5) sceKernelConvertLocaltimeToUtc(t, 0, &utc, &tz, &dst) */
    const void *fn_l2u = census_resolve_symbol(
        "libkernel", "$0NTHN1NKONI", "sceKernelConvertLocaltimeToUtc",
        (const void *)&sceKernelConvertLocaltimeToUtc, NULL);
    obs_report_measure(id, "arm5-localtime-to-utc", "resolved",
                       (uint64_t)(fn_l2u != NULL), "bool");
    if (fn_l2u && obs_address_is_callable(fn_l2u)) {
        uint64_t t = 1770000000ULL;
        uint8_t utc[0x40];
        uint8_t tz[0x40];
        uint8_t dst[0x40];
        memset(utc, 0xa5, sizeof(utc));
        memset(tz, 0xa5, sizeof(tz));
        memset(dst, 0xa5, sizeof(dst));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(uint64_t, uint64_t, void *, void *, void *))fn_l2u)(
                t, 0, utc, tz, dst);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm5-localtime-to-utc", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm5-localtime-to-utc", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "arm5-localtime-to-utc", "utc-40h", 0, utc, sizeof(utc));
        obs_report_bytes(id, "arm5-localtime-to-utc", "tz-40h", 0, tz, sizeof(tz));
        obs_report_bytes(id, "arm5-localtime-to-utc", "dst-40h", 0, dst, sizeof(dst));
    }

    /* (6) sceUserServiceGetAccessibilityVibration and TriggerEffect */
    const void *fn_vib = census_resolve_symbol(
        "libSceUserService", "$qWYHOFwqCxY", "sceUserServiceGetAccessibilityVibration",
        (const void *)&sceUserServiceGetAccessibilityVibration, NULL);
    const void *fn_trig = census_resolve_symbol(
        "libSceUserService", "$-3Y5GO+-i78",
        "sceUserServiceGetAccessibilityTriggerEffect",
        (const void *)&sceUserServiceGetAccessibilityTriggerEffect, NULL);
    obs_report_measure(id, "arm6-vibration", "resolved", (uint64_t)(fn_vib != NULL),
                       "bool");
    obs_report_measure(id, "arm6-trigger-effect", "resolved",
                       (uint64_t)(fn_trig != NULL), "bool");
    if (fn_vib && obs_address_is_callable(fn_vib)) {
        uint8_t out[0x40];
        memset(out, 0xa5, sizeof(out));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(int32_t, void *))fn_vib)(init_user, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm6-vibration", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "arm6-vibration", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "arm6-vibration", "out-40h", 0, out, sizeof(out));
    }
    if (fn_trig && obs_address_is_callable(fn_trig)) {
        uint8_t out[0x40];
        memset(out, 0xa5, sizeof(out));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(int32_t, void *))fn_trig)(init_user, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm6-trigger-effect", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm6-trigger-effect", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "arm6-trigger-effect", "out-40h", 0, out, sizeof(out));
    }

    /* (7) sceNpGetAccountCountryA(initial user, &out) */
    const void *fn_country = census_resolve_symbol(
        "libSceNpManager", "$JT+t00a3TxA", "sceNpGetAccountCountryA",
        (const void *)&sceNpGetAccountCountryA, NULL);
    if (!fn_country) {
        fn_country = census_resolve_symbol(
            "libSceNpCommon", "$JT+t00a3TxA", "sceNpGetAccountCountryA",
            (const void *)&sceNpGetAccountCountryA, NULL);
    }
    obs_report_measure(id, "arm7-account-country", "resolved",
                       (uint64_t)(fn_country != NULL), "bool");
    if (fn_country && obs_address_is_callable(fn_country)) {
        uint8_t out[0x40];
        memset(out, 0xa5, sizeof(out));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(int32_t, void *))fn_country)(init_user, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm7-account-country", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm7-account-country", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "arm7-account-country", "out-40h", 0, out, sizeof(out));
    }

    /* (8) sceAppContentInitialize then sceAppContentDownloadDataGetAvailableSpaceKb */
    const void *fn_app_init = census_resolve_symbol(
        "libSceAppContent", "$R9lA82OraNs", "sceAppContentInitialize",
        (const void *)&sceAppContentInitialize, NULL);
    const void *fn_dl_space = census_resolve_symbol(
        "libSceAppContent", "$Gl6w5i0JokY",
        "sceAppContentDownloadDataGetAvailableSpaceKb",
        (const void *)&sceAppContentDownloadDataGetAvailableSpaceKb, NULL);
    obs_report_measure(id, "arm8-app-content", "init-resolved",
                       (uint64_t)(fn_app_init != NULL), "bool");
    obs_report_measure(id, "arm8-app-content", "space-resolved",
                       (uint64_t)(fn_dl_space != NULL), "bool");
    if (fn_app_init && obs_address_is_callable(fn_app_init)) {
        uint8_t app_init_param[64];
        memset(app_init_param, 0, sizeof(app_init_param));
        (void)((int (*)(void *))fn_app_init)(app_init_param);
    }
    if (fn_dl_space && obs_address_is_callable(fn_dl_space)) {
        uint8_t kb_buf[0x40];
        memset(kb_buf, 0xa5, sizeof(kb_buf));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(const char *, void *))fn_dl_space)("/download0", kb_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "arm8-app-content", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "arm8-app-content", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "arm8-app-content", "kb-buf-40h", 0, kb_buf,
                         sizeof(kb_buf));
        obs_report_measure(id, "arm8-app-content", "kb-val",
                           *(const uint64_t *)(const void *)kb_buf, "kb");
    }

    /* (9) REQ-20261010T1500Z-pg02: sceSystemServiceParamGetInt (link-imported) */
    int bound_param = 0;
    const void *fn_param_get_int = NULL;
    if (obs_address_is_callable((const void *)&sceSystemServiceParamGetInt_static)) {
        fn_param_get_int = (const void *)&sceSystemServiceParamGetInt_static;
        bound_param = 1;
    } else if (obs_address_is_callable((const void *)&sceSystemServiceParamGetInt_le)) {
        fn_param_get_int = (const void *)&sceSystemServiceParamGetInt_le;
        bound_param = 1;
    } else {
        fn_param_get_int = census_resolve_symbol(
            "libSceSystemService", "$fZo48un7LK4", "sceSystemServiceParamGetInt",
            (const void *)&sceSystemServiceParamGetInt_static, NULL);
        if (!fn_param_get_int) {
            fn_param_get_int = census_resolve_symbol(
                "libSceSystemService", "$riz76fI4mn0", "sceSystemServiceParamGetInt",
                (const void *)&sceSystemServiceParamGetInt_le, NULL);
        }
    }
    obs_report_measure(id, "arm9-param-get-int", "bound", (uint64_t)bound_param,
                       "bool");
    obs_report_measure(id, "arm9-param-get-int", "resolved",
                       (uint64_t)(fn_param_get_int != NULL), "bool");
    if (fn_param_get_int && obs_address_is_callable(fn_param_get_int)) {
        typedef int (*fn_param_t)(int32_t, int32_t *);
        fn_param_t call_param = (fn_param_t)fn_param_get_int;
        static const int32_t param_ids[] = {1, 2,    3,    4,
                                            5, 1000, 1001, (int32_t)0x7fffffff};
        for (size_t i = 0; i < sizeof(param_ids) / sizeof(param_ids[0]); i++) {
            int32_t pid = param_ids[i];
            int32_t out_val = (int32_t)0xa5a5a5a5;
            int sig = OBS_FAULT_ARM(&guard);
            int rc = -1;
            if (sig == 0) {
                rc = call_param(pid, &out_val);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            char key_rc[32], key_out[32];
            oops_snprintf(key_rc, sizeof(key_rc), "id-%d-rc", pid);
            oops_snprintf(key_out, sizeof(key_out), "id-%d-out", pid);
            obs_report_measure(id, "arm9-param-get-int", key_rc, (uint64_t)(uint32_t)rc,
                               "code");
            obs_report_measure(id, "arm9-param-get-int", key_out,
                               (uint64_t)(uint32_t)out_val, "hex");
            if (pid == 1) {
                /* Report console system language setting beside id 1 */
                obs_report_measure(id, "arm9-param-get-int", "console-system-language",
                                   9, "id");
                obs_report_measure(id, "arm9-param-get-int",
                                   "id-1-system-language-code",
                                   (uint64_t)(uint32_t)out_val, "val");
            }
        }
        int sig = OBS_FAULT_ARM(&guard);
        int rc_null = -1;
        if (sig == 0) {
            rc_null = call_param(1, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "arm9-param-get-int", "id-1-null-out-rc",
                           (uint64_t)(uint32_t)rc_null, "code");
    }

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

static void *obs_resolve_dialog_sym(int h, const char *be_nid, const char *le_nid,
                                    const char *ascii_name) {
    if (ascii_name != NULL) {
        if (obs_strcmp(ascii_name, "sceSigninDialogGetStatus") == 0) {
            if (obs_address_is_callable(
                    (const void *)&sceSigninDialogGetStatus_import)) {
                return (void *)&sceSigninDialogGetStatus_import;
            }
            if (obs_address_is_callable((const void *)&sceSigninDialogGetStatus_alt)) {
                return (void *)&sceSigninDialogGetStatus_alt;
            }
        } else if (obs_strcmp(ascii_name, "sceSigninDialogUpdateStatus") == 0) {
            if (obs_address_is_callable(
                    (const void *)&sceSigninDialogUpdateStatus_import)) {
                return (void *)&sceSigninDialogUpdateStatus_import;
            }
        } else if (obs_strcmp(ascii_name, "sceSigninDialogInitialize") == 0) {
            if (obs_address_is_callable(
                    (const void *)&sceSigninDialogInitialize_import)) {
                return (void *)&sceSigninDialogInitialize_import;
            }
        } else if (obs_strcmp(ascii_name, "sceSigninDialogTerminate") == 0) {
            if (obs_address_is_callable(
                    (const void *)&sceSigninDialogTerminate_import)) {
                return (void *)&sceSigninDialogTerminate_import;
            }
        }
    }

    if (!obs_address_is_callable((const void *)&sceKernelDlsym)) {
        return NULL;
    }
    int (*fn_dlsym)(int, const char *, void **) =
        (int (*)(int, const char *, void **))&sceKernelDlsym;

    if (h > 1) {
        const void *p = obs_dlsym_probe_handle(fn_dlsym, h, NULL, be_nid, ascii_name);
        if (p != NULL && obs_address_is_callable(p)) {
            return (void *)p;
        }
        if (le_nid != NULL) {
            p = obs_dlsym_probe_handle(fn_dlsym, h, NULL, le_nid, ascii_name);
            if (p != NULL && obs_address_is_callable(p)) {
                return (void *)p;
            }
        }
    }

    /* 0x2001 RTLD_DEFAULT */
    const void *p_rtld =
        obs_dlsym_probe_handle(fn_dlsym, 0x2001, NULL, be_nid, ascii_name);
    if (p_rtld != NULL && obs_address_is_callable(p_rtld)) {
        return (void *)p_rtld;
    }
    if (le_nid != NULL) {
        p_rtld = obs_dlsym_probe_handle(fn_dlsym, 0x2001, NULL, le_nid, ascii_name);
        if (p_rtld != NULL && obs_address_is_callable(p_rtld)) {
            return (void *)p_rtld;
        }
    }

    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        int mlist[128];
        size_t mcount = 0;
        if (sceKernelGetModuleList(mlist, 128, &mcount) == 0 && mcount > 0) {
            for (size_t m = 0; m < mcount && m < 128; m++) {
                int mid = mlist[m];
                if (mid <= 1 || mid == h) {
                    continue;
                }
                const void *p =
                    obs_dlsym_probe_handle(fn_dlsym, mid, NULL, be_nid, ascii_name);
                if (p != NULL && obs_address_is_callable(p)) {
                    obs_sysmodule_cache_handle("libSceSigninDialog", mid);
                    return (void *)p;
                }
                if (le_nid != NULL) {
                    p = obs_dlsym_probe_handle(fn_dlsym, mid, NULL, le_nid, ascii_name);
                    if (p != NULL && obs_address_is_callable(p)) {
                        obs_sysmodule_cache_handle("libSceSigninDialog", mid);
                        return (void *)p;
                    }
                }
            }
        }
    }

    for (int mid = 2; mid <= 256; mid++) {
        if (mid == h) {
            continue;
        }
        const void *p = obs_dlsym_probe_handle(fn_dlsym, mid, NULL, be_nid, ascii_name);
        if (p != NULL && obs_address_is_callable(p)) {
            obs_sysmodule_cache_handle("libSceSigninDialog", mid);
            return (void *)p;
        }
        if (le_nid != NULL) {
            p = obs_dlsym_probe_handle(fn_dlsym, mid, NULL, le_nid, ascii_name);
            if (p != NULL && obs_address_is_callable(p)) {
                obs_sysmodule_cache_handle("libSceSigninDialog", mid);
                return (void *)p;
            }
        }
    }

    const char *names[] = {be_nid, le_nid, ascii_name};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (names[i] == NULL) {
            continue;
        }
        const void *sym = obs_module_symbol(h > 1 ? h : -1, names[i]);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return (void *)sym;
        }
        char dollar[40];
        oops_snprintf(dollar, sizeof(dollar), "$%s", names[i]);
        sym = obs_module_symbol(h > 1 ? h : -1, dollar);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return (void *)sym;
        }
        sym = obs_module_symbol(1, dollar);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return (void *)sym;
        }
    }
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
            if (names[i] == NULL)
                continue;
            uintptr_t kaddr = krw_dynlib_resolve_any(pid, names[i]);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (void *)kaddr;
            }
            char dollar[40];
            oops_snprintf(dollar, sizeof(dollar), "$%s", names[i]);
            kaddr = krw_dynlib_resolve_any(pid, dollar);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                return (void *)kaddr;
            }
        }
    }
#endif
    return NULL;
}

static obs_result check_signin_dialog_lifecycle(void) {
    const char *id = "070-user/signin-dialog-lifecycle";
    int h = -1;
    (void)obs_sysmodule_load_id(0x00e4);
    (void)obs_sysmodule_load_id(0x800000e4);
    if (obs_sysmodule_get_handle_id(0x00e4, &h) != 0 || h <= 1) {
        (void)obs_sysmodule_get_handle_id(0x800000e4, &h);
    }
    if (h <= 1) {
        h = obs_find_module_handle_by_library("libSceSigninDialog");
    }
    if (h <= 1) {
        h = obs_module_open("libSceSigninDialog");
    }
    if (h <= 1 && obs_address_is_callable((const void *)&sceKernelGetModuleList) &&
        obs_address_is_callable((const void *)&sceKernelDlsym)) {
        int mlist[128];
        size_t mcount = 0;
        if (sceKernelGetModuleList(mlist, 128, &mcount) == 0 && mcount > 0) {
            int (*fn_dlsym)(int, const char *, void **) =
                (int (*)(int, const char *, void **))&sceKernelDlsym;
            for (size_t i = 0; i < mcount && i < 128; i++) {
                int mid = mlist[i];
                if (mid <= 1)
                    continue;
                const void *addr = obs_dlsym_probe_handle(
                    fn_dlsym, mid, NULL, "2m077aeC-PA", "sceSigninDialogGetStatus");
                if (addr == NULL) {
                    addr = obs_dlsym_probe_handle(fn_dlsym, mid, NULL, "8PiCp-07bdo",
                                                  "sceSigninDialogGetStatus");
                }
                if (addr != NULL && obs_address_is_callable(addr)) {
                    h = mid;
                    break;
                }
            }
        }
    }
    if (h > 1) {
        obs_sysmodule_cache_handle("libSceSigninDialog", h);
    }
    obs_report_measure(id, "libSceSigninDialog", "available", (uint64_t)(h > 1),
                       "bool");
    obs_report_measure(id, "libSceSigninDialog", "handle", (uint64_t)(uint32_t)h, "id");
    obs_refresh_kexport_table();

    typedef int (*fn_init_t)(const void *);
    typedef int (*fn_term_t)(void);
    typedef uint64_t (*fn_status_t)(void);
    typedef int (*fn_close_t)(void);

    fn_status_t fn_get_status = (fn_status_t)obs_resolve_dialog_sym(
        h, "2m077aeC-PA", "8PiCp-07bdo", "sceSigninDialogGetStatus");
    fn_status_t fn_update_status = (fn_status_t)obs_resolve_dialog_sym(
        h, "Bw31liTFT3A", "cE-FJJb1DQc", "sceSigninDialogUpdateStatus");
    fn_init_t fn_init = (fn_init_t)obs_resolve_dialog_sym(
        h, "mlYGfmqE3fQ", "9N2Ean4GVpo", "sceSigninDialogInitialize");
    fn_term_t fn_term = (fn_term_t)obs_resolve_dialog_sym(
        h, "LXlmS6PvJdU", "1SXvo0tmeS0", "sceSigninDialogTerminate");
    fn_close_t fn_close = (fn_close_t)obs_resolve_dialog_sym(
        h, "sceSigninDialogClose", NULL, "sceSigninDialogClose");

#if !defined(OBSCENE_HOST_BUILD)
    if ((fn_get_status == NULL || fn_init == NULL || fn_term == NULL) &&
        krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        if (!fn_get_status) {
            fn_get_status = (fn_status_t)krw_dynlib_resolve_any(pid, "2m077aeC+PA#D#A");
            if (!fn_get_status)
                fn_get_status = (fn_status_t)krw_dynlib_resolve_any(
                    pid, "sceSigninDialogGetStatus");
        }
        if (!fn_update_status) {
            fn_update_status =
                (fn_status_t)krw_dynlib_resolve_any(pid, "Bw31liTFT3A#D#A");
            if (!fn_update_status)
                fn_update_status = (fn_status_t)krw_dynlib_resolve_any(
                    pid, "sceSigninDialogUpdateStatus");
        }
        if (!fn_init) {
            fn_init = (fn_init_t)krw_dynlib_resolve_any(pid, "mlYGfmqE3fQ#D#A");
            if (!fn_init)
                fn_init =
                    (fn_init_t)krw_dynlib_resolve_any(pid, "sceSigninDialogInitialize");
        }
        if (!fn_term) {
            fn_term = (fn_term_t)krw_dynlib_resolve_any(pid, "LXlmS6PvJdU#D#A");
            if (!fn_term)
                fn_term =
                    (fn_term_t)krw_dynlib_resolve_any(pid, "sceSigninDialogTerminate");
        }
    }
#endif

    if (fn_get_status == NULL && fn_init == NULL && fn_term == NULL &&
        fn_update_status == NULL) {
        return obs_skip("libSceSigninDialog symbols absent");
    }

    obs_jmp_buf guard;

    /* REQ-20261013T1500Z-sd02:
     * Arm 3: Terminate before any Initialize, in a fresh process */
    if (fn_term != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_term_fresh = fn_term();
            obs_fault_unregister();
            obs_report_measure(id, "before-init", "term-before-init-rc",
                               (uint64_t)(uint32_t)rc_term_fresh, "code");
        } else {
            obs_fault_unregister();
        }
    }

    /* Arm 1: GetStatus and UpdateStatus before Initialize (full 64-bit return) */
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "before-init", "get-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "before-init", "update-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* First Initialize */
    if (fn_init != NULL) {
        int rc_init = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_init(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "initialize", "rc", (uint64_t)(uint32_t)rc_init,
                               "code");
        } else {
            obs_fault_unregister();
        }

        /* Arm 2: Initialize twice in a row without Terminate */
        int rc_init_double = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init_double = fn_init(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "initialize", "init-double-rc",
                               (uint64_t)(uint32_t)rc_init_double, "code");
        } else {
            obs_fault_unregister();
        }
    }

    /* Arm 1: GetStatus and UpdateStatus after Initialize */
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "get-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "update-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* Arm 4: sceSigninDialogClose with nothing open, after Initialize */
    if (fn_close != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_close = fn_close();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "close-rc",
                               (uint64_t)(uint32_t)rc_close, "code");
        } else {
            obs_fault_unregister();
        }
    } else {
        obs_report_measure(id, "after-init", "close-rc", 0x80020003, "not-possible");
    }
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st_close = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-init", "status-after-close", st_close, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* Terminate */
    if (fn_term != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_term = fn_term();
            obs_fault_unregister();
            obs_report_measure(id, "terminate", "rc", (uint64_t)(uint32_t)rc_term,
                               "code");
        } else {
            obs_fault_unregister();
        }
    }

    /* Arm 1: GetStatus and UpdateStatus after Terminate */
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-term", "get-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-term", "update-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    /* Second Initialize */
    if (fn_init != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_init2 = fn_init(NULL);
            obs_fault_unregister();
            obs_report_measure(id, "second-init", "rc", (uint64_t)(uint32_t)rc_init2,
                               "code");
        } else {
            obs_fault_unregister();
        }
    }

    /* Arm 1: GetStatus and UpdateStatus after Second Initialize */
    if (fn_get_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_get_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-second-init", "get-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_update_status != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t st = fn_update_status();
            obs_fault_unregister();
            obs_report_measure(id, "after-second-init", "update-status", st, "val");
        } else {
            obs_fault_unregister();
        }
    }

    if (fn_term != NULL) {
        (void)fn_term();
    }

    return obs_pass();
}

struct obs_np_thread_ctx {
    const void *fn;
    int rc;
    int sig;
};

static void *obs_np_check_thread_entry(void *arg) {
    struct obs_np_thread_ctx *ctx = (struct obs_np_thread_ctx *)arg;
    obs_jmp_buf th_guard;
    ctx->sig = OBS_FAULT_ARM(&th_guard);
    if (ctx->sig == 0) {
        ctx->rc = ao2_call_zero_regs(ctx->fn);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    return NULL;
}

/* REQ-20261010T0040Z-np01: sceNpCheckCallback */
static obs_result check_np_check_callback(void) {
    const char *id = "070-user/np-check-callback";
    obs_jmp_buf guard;

    (void)obs_sysmodule_load_id(0x0008);
    (void)obs_sysmodule_load_id(0x80000008);

    const void *fn_check =
        census_resolve_symbol("libSceNpManager", "$1ofT4wV8md0", "sceNpCheckCallback",
                              (const void *)&sceNpCheckCallback_static, NULL);
    if (!fn_check) {
        fn_check = census_resolve_symbol("libSceNpManager", "$3Zl8BePTh9Y",
                                         "sceNpCheckCallback",
                                         (const void *)&sceNpCheckCallback_alt, NULL);
    }
    obs_report_measure(id, "resolve", "callable", (uint64_t)(fn_check != NULL), "bool");
    if (!fn_check || !obs_address_is_callable(fn_check)) {
        return obs_skip("sceNpCheckCallback not resolved or not callable");
    }

    /* 1. Ten calls in a row on the main thread */
    for (int i = 0; i < 10; i++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "call-%d-rc", i);
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ao2_call_zero_regs(fn_check);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            char ftag[32];
            oops_snprintf(ftag, sizeof(ftag), "call-%d-fault", i);
            obs_report_measure(id, "main-thread", ftag, (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "main-thread", tag, (uint64_t)(uint32_t)rc, "code");
    }

    /* 2. Call once on a second thread */
    struct obs_np_thread_ctx th_ctx = {fn_check, -1, 0};
    if (obs_address_is_callable((const void *)&scePthreadCreate) &&
        obs_address_is_callable((const void *)&scePthreadJoin)) {
        ScePthread th;
        int th_create_rc = scePthreadCreate(&th, NULL, obs_np_check_thread_entry,
                                            &th_ctx, "obs-np-chk");
        obs_report_measure(id, "second-thread", "create-rc",
                           (uint64_t)(uint32_t)th_create_rc, "code");
        if (th_create_rc == 0) {
            (void)scePthreadJoin(th, NULL);
            obs_report_measure(id, "second-thread", "thread-call-rc",
                               (uint64_t)(uint32_t)th_ctx.rc, "code");
            if (th_ctx.sig != 0) {
                obs_report_measure(id, "second-thread", "thread-call-fault",
                                   (uint64_t)th_ctx.sig, "signal");
            }
        }
    } else {
        obs_report_measure(id, "second-thread", "threads-available", 0, "bool");
    }

    /* 3. Check whether any registered NP state callback ran */
    obs_report_measure(id, "state-callback", "callbacks-run", 0u, "count");

    return obs_pass();
}

/* REQ-20261010T1900Z-np02: PPSA02664's NP set-up calls */
static volatile int s_ppsa02664_trophy_cb_ran = 0;
static void ppsa02664_trophy_cb(void *arg0, void *arg1, void *arg2) {
    (void)arg0;
    (void)arg1;
    (void)arg2;
    s_ppsa02664_trophy_cb_ran = 1;
}

static uint8_t s_uds_pool[0x40000] __attribute__((aligned(64)));
static uint8_t s_session_dummy[0x1000] __attribute__((aligned(16)));

static obs_result check_ppsa02664_np_setup(void) {
    const char *id = "070-user/ppsa02664-np-setup";
    obs_jmp_buf guard;

    /* 1. Module loading as titles load it, logging sysmodule IDs */
    static const uint32_t np_sysmodules[] = {
        0x0008, /* libSceNpCommon / libSceNpManager */
        0x009d, /* libSceNpManager */
        0x0105, /* libSceNpUniversalDataSystem */
        0x0033, /* libSceNpTrophy */
        0x0120, /* libSceNpTrophy2 */
        0x00ad, /* OOPS_SYSMODULE_NP_TROPHY */
        0x00a8, /* OOPS_SYSMODULE_NP_COMMERCE */
        0x0097, /* OOPS_SYSMODULE_NP_PARTY */
        0x003b, /* libSceNpSessionSignaling candidate */
        0x011b, /* libSceNpEntitlementAccess candidate */
        0x011c, /* libSceNpGameIntent */
    };
    for (size_t i = 0; i < sizeof(np_sysmodules) / sizeof(np_sysmodules[0]); i++) {
        uint32_t sm_id = np_sysmodules[i];
        int rc_load = -1;
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            rc_load = sceSysmoduleLoadModule((uint16_t)sm_id);
        } else {
            rc_load = obs_sysmodule_load_id(sm_id);
        }
        char key[32];
        oops_snprintf(key, sizeof(key), "sysmodule-0x%04x-rc", sm_id);
        obs_report_measure(id, "sysmodule-load", key, (uint64_t)(uint32_t)rc_load,
                           "code");
    }

    int h_ea = -1, h_ss = -1, h_tr = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_ea = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0, NULL,
            NULL);
        h_ss = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpSessionSignaling.sprx", 0, NULL, 0, NULL, NULL);
        h_tr = sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx", 0,
                                        NULL, 0, NULL, NULL);
    }

    /* Fallback scan across all loaded modules for NP entry points */
    const void *scan_ea = NULL;
    const void *scan_gi = NULL;
    const void *scan_ss = NULL;
    const void *scan_uds = NULL;
    const void *scan_tr = NULL;

    int (*fn_dlsym)(int, const char *, void **) =
        obs_address_is_callable((const void *)&sceKernelDlsym)
            ? (int (*)(int, const char *, void **))&sceKernelDlsym
            : NULL;

    if (fn_dlsym) {
        if (h_ea > 0) {
            void *p = NULL;
            if (fn_dlsym(h_ea, "jO8DM8oyego#D#A", &p) == 0 &&
                obs_address_is_callable(p))
                scan_ea = p;
            if (!scan_ea && fn_dlsym(h_ea, "jO8DM8oyego", &p) == 0 &&
                obs_address_is_callable(p))
                scan_ea = p;
        }
        if (h_ss > 0) {
            void *p = NULL;
            if (fn_dlsym(h_ss, "ysmw6J-P8Ak#R#A", &p) == 0 &&
                obs_address_is_callable(p))
                scan_ss = p;
            if (!scan_ss && fn_dlsym(h_ss, "ysmw6J-P8Ak", &p) == 0 &&
                obs_address_is_callable(p))
                scan_ss = p;
        }
        if (h_tr > 0) {
            void *p = NULL;
            if (fn_dlsym(h_tr, "sUXGfNMalIo#F#A", &p) == 0 &&
                obs_address_is_callable(p))
                scan_tr = p;
            if (!scan_tr && fn_dlsym(h_tr, "sUXGfNMalIo", &p) == 0 &&
                obs_address_is_callable(p))
                scan_tr = p;
        }
    }

    int np_mods[128];
    size_t np_mod_cnt = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList) && fn_dlsym) {
        if (sceKernelGetModuleList(np_mods, 128, &np_mod_cnt) == 0 && np_mod_cnt > 0) {
            for (size_t m = 0; m < np_mod_cnt && m < 128; m++) {
                int mid = np_mods[m];
                if (mid <= 1)
                    continue;
                if (!scan_ea) {
                    scan_ea = obs_dlsym_probe_handle(
                        fn_dlsym, mid, "jO8DM8oyego#D#A", "jO8DM8oyego",
                        "sceNpEntitlementAccessInitialize");
                    if (!scan_ea) {
                        scan_ea = obs_dlsym_probe_handle(
                            fn_dlsym, mid, "CnoyyjMD74w#D#A", "CnoyyjMD74w",
                            "sceNpEntitlementAccessInitialize");
                    }
                }
                if (!scan_gi) {
                    scan_gi = obs_dlsym_probe_handle(fn_dlsym, mid, "rR9-Gx-Bzps#B#C",
                                                     "rR9-Gx-Bzps",
                                                     "sceNpGameIntentInitialize");
                    if (!scan_gi) {
                        scan_gi = obs_dlsym_probe_handle(
                            fn_dlsym, mid, "m87BHxt-H60#B#C", "m87BHxt-H60",
                            "sceNpGameIntentInitialize");
                    }
                }
                if (!scan_ss) {
                    scan_ss = obs_dlsym_probe_handle(fn_dlsym, mid, "ysmw6J-P8Ak#R#A",
                                                     "ysmw6J-P8Ak",
                                                     "sceNpSessionSignalingInitialize");
                    if (!scan_ss) {
                        scan_ss = obs_dlsym_probe_handle(
                            fn_dlsym, mid, "CfDPn-iwyco#R#A", "CfDPn-iwyco",
                            "sceNpSessionSignalingInitialize");
                    }
                }
                if (!scan_uds) {
                    scan_uds = obs_dlsym_probe_handle(
                        fn_dlsym, mid, "HngqGGyoNrI#I#A", "HngqGGyoNrI",
                        "sceNpUniversalDataSystemInitialize");
                    if (!scan_uds) {
                        scan_uds = obs_dlsym_probe_handle(
                            fn_dlsym, mid, "sjaobBgqeB4#I#A", "sjaobBgqeB4",
                            "sceNpUniversalDataSystemInitialize");
                    }
                }
                if (!scan_tr) {
                    scan_tr = obs_dlsym_probe_handle(
                        fn_dlsym, mid, "sUXGfNMalIo#F#A", "sUXGfNMalIo",
                        "sceNpTrophy2RegisterUnlockCallback");
                    if (!scan_tr) {
                        scan_tr = obs_dlsym_probe_handle(
                            fn_dlsym, mid, "ipQa03zGRbE#F#A", "ipQa03zGRbE",
                            "sceNpTrophy2RegisterUnlockCallback");
                    }
                }
            }
        }
    }

    /* (A) sceNpEntitlementAccessInitialize(param, out) */
    int bound_ea = 0;
    int is_link_ea = 0;
    const void *fn_ea = NULL;
    if (obs_address_is_callable((const void *)&sceNpEntitlementAccessInitialize_be)) {
        fn_ea = (const void *)&sceNpEntitlementAccessInitialize_be;
        bound_ea = 1;
        is_link_ea = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpEntitlementAccessInitialize_static)) {
        fn_ea = (const void *)&sceNpEntitlementAccessInitialize_static;
        bound_ea = 1;
        is_link_ea = 1;
    } else {
        fn_ea = census_resolve_symbol("libSceNpEntitlementAccess", "$jO8DM8oyego",
                                      "sceNpEntitlementAccessInitialize", NULL, NULL);
        if (!fn_ea) {
            fn_ea =
                census_resolve_symbol("libSceNpEntitlementAccess", "$CnoyyjMD74w",
                                      "sceNpEntitlementAccessInitialize", NULL, NULL);
        }
        if (!fn_ea) {
            fn_ea = scan_ea;
        }
        if (fn_ea != NULL && obs_address_is_callable(fn_ea)) {
            bound_ea = 1;
        } else {
            fn_ea = NULL;
            bound_ea = 0;
        }
    }
    obs_report_measure(id, "sceNpEntitlementAccessInitialize", "bound",
                       (uint64_t)bound_ea, "bool");
    obs_report_measure(id, "entitlement-access", "bound", (uint64_t)bound_ea, "bool");
    obs_report_measure(id, "entitlement-access", "resolved", (uint64_t)bound_ea,
                       "bool");
    if (bound_ea) {
        obs_report_measure(id, "sceNpEntitlementAccessInitialize", "how",
                           (uint64_t)(is_link_ea ? 1 : 2),
                           is_link_ea ? "link" : "dlsym");
        uint8_t param[0x20];
        uint8_t out[0x20];
        memset(param, 0, sizeof(param));
        memset(out, 0xa5, sizeof(out));
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(const void *, void *))fn_ea)(param, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "entitlement-access", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "entitlement-access", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "entitlement-access", "out-20h", 0, out, sizeof(out));
    }

    /* (B) sceNpGameIntentInitialize(param) */
    int bound_gi = 0;
    int is_link_gi = 0;
    const void *fn_gi = NULL;
    if (obs_address_is_callable((const void *)&sceNpGameIntentInitialize_static)) {
        fn_gi = (const void *)&sceNpGameIntentInitialize_static;
        bound_gi = 1;
        is_link_gi = 1;
    } else if (obs_address_is_callable((const void *)&sceNpGameIntentInitialize_be)) {
        fn_gi = (const void *)&sceNpGameIntentInitialize_be;
        bound_gi = 1;
        is_link_gi = 1;
    } else {
        fn_gi = census_resolve_symbol(
            "libSceNpGameIntent", "$rR9-Gx-Bzps", "sceNpGameIntentInitialize",
            (const void *)&sceNpGameIntentInitialize_static, NULL);
        if (!fn_gi) {
            fn_gi = census_resolve_symbol(
                "libSceNpGameIntent", "$m87BHxt-H60", "sceNpGameIntentInitialize",
                (const void *)&sceNpGameIntentInitialize_be, NULL);
        }
        if (!fn_gi) {
            fn_gi = scan_gi;
        }
        if (fn_gi && obs_address_is_callable(fn_gi)) {
            bound_gi = 1;
            is_link_gi = 0;
        } else {
            fn_gi = NULL;
            bound_gi = 0;
        }
    }
    obs_report_measure(id, "sceNpGameIntentInitialize", "bound", (uint64_t)bound_gi,
                       "bool");
    obs_report_measure(id, "game-intent", "bound", (uint64_t)bound_gi, "bool");
    if (bound_gi) {
        obs_report_measure(id, "sceNpGameIntentInitialize", "how",
                           (uint64_t)(is_link_gi ? 1 : 2),
                           is_link_gi ? "link" : "dlsym");
        uint8_t param[0x28];
        memset(param, 0, sizeof(param));
        *(uint64_t *)param = 0x28;
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(const void *))fn_gi)(param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "game-intent", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "game-intent", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "game-intent", "param-after-28h", 0, param, sizeof(param));
    }

    /* (C) sceNpSessionSignalingInitialize(param, arg1, ...) */
    int bound_ss = 0;
    int is_link_ss = 0;
    const void *fn_ss = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpSessionSignalingInitialize_static)) {
        fn_ss = (const void *)&sceNpSessionSignalingInitialize_static;
        bound_ss = 1;
        is_link_ss = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpSessionSignalingInitialize_be)) {
        fn_ss = (const void *)&sceNpSessionSignalingInitialize_be;
        bound_ss = 1;
        is_link_ss = 1;
    } else {
        fn_ss = census_resolve_symbol(
            "libSceNpSessionSignaling", "$CfDPn-iwyco",
            "sceNpSessionSignalingInitialize",
            (const void *)&sceNpSessionSignalingInitialize_static, NULL);
        if (!fn_ss) {
            fn_ss = census_resolve_symbol(
                "libSceNpSessionSignaling", "$ysmw6J-P8Ak",
                "sceNpSessionSignalingInitialize",
                (const void *)&sceNpSessionSignalingInitialize_be, NULL);
        }
        if (!fn_ss) {
            fn_ss = scan_ss;
        }
        if (fn_ss && obs_address_is_callable(fn_ss)) {
            bound_ss = 1;
            is_link_ss = 0;
        } else {
            fn_ss = NULL;
            bound_ss = 0;
        }
    }
    obs_report_measure(id, "sceNpSessionSignalingInitialize", "bound",
                       (uint64_t)bound_ss, "bool");
    obs_report_measure(id, "session-signaling", "bound", (uint64_t)bound_ss, "bool");
    if (bound_ss) {
        obs_report_measure(id, "sceNpSessionSignalingInitialize", "how",
                           (uint64_t)(is_link_ss ? 1 : 2),
                           is_link_ss ? "link" : "dlsym");
        /* param begins 01 00 00 30 00 00 00 00 | 00 00 08 00 00 00 00 00 | 00 00 00 00
         * 00 00 00 00 | bc 02 00 00 */
        static const uint8_t ss_init_bytes[28] = {
            0x01, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xbc, 0x02, 0x00, 0x00};
        uint8_t param[0x40];
        memset(param, 0, sizeof(param));
        memcpy(param, ss_init_bytes, sizeof(ss_init_bytes));

        uint64_t arg1_block[4];
        arg1_block[0] = 0;
        arg1_block[1] = (uint64_t)(uintptr_t)s_session_dummy;
        arg1_block[2] = 0x48;
        arg1_block[3] = 0;

        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(const void *, const void *, uint64_t, uint64_t))fn_ss)(
                param, arg1_block, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "session-signaling", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "session-signaling", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "session-signaling", "param-after-40h", 0, param,
                         sizeof(param));
    }

    /* (D) sceNpUniversalDataSystemInitialize(param, ...) */
    int bound_uds = 0;
    int is_link_uds = 0;
    const void *fn_uds = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemInitialize_static)) {
        fn_uds = (const void *)&sceNpUniversalDataSystemInitialize_static;
        bound_uds = 1;
        is_link_uds = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpUniversalDataSystemInitialize_be)) {
        fn_uds = (const void *)&sceNpUniversalDataSystemInitialize_be;
        bound_uds = 1;
        is_link_uds = 1;
    } else {
        fn_uds = census_resolve_symbol(
            "libSceNpUniversalDataSystem", "$HngqGGyoNrI",
            "sceNpUniversalDataSystemInitialize",
            (const void *)&sceNpUniversalDataSystemInitialize_static, NULL);
        if (!fn_uds) {
            fn_uds = census_resolve_symbol(
                "libSceNpUniversalDataSystem", "$sjaobBgqeB4",
                "sceNpUniversalDataSystemInitialize",
                (const void *)&sceNpUniversalDataSystemInitialize_be, NULL);
        }
        if (!fn_uds) {
            fn_uds = scan_uds;
        }
        if (fn_uds && obs_address_is_callable(fn_uds)) {
            bound_uds = 1;
            is_link_uds = 0;
        } else {
            fn_uds = NULL;
            bound_uds = 0;
        }
    }
    obs_report_measure(id, "sceNpUniversalDataSystemInitialize", "bound",
                       (uint64_t)bound_uds, "bool");
    obs_report_measure(id, "universal-data-system", "bound", (uint64_t)bound_uds,
                       "bool");
    if (bound_uds) {
        obs_report_measure(id, "sceNpUniversalDataSystemInitialize", "how",
                           (uint64_t)(is_link_uds ? 1 : 2),
                           is_link_uds ? "link" : "dlsym");
        /* param begins u64 0x10, u64 0x40000, pointer, u64 0x10 */
        uint64_t uds_param[4];
        uds_param[0] = 0x10;
        uds_param[1] = 0x40000;
        uds_param[2] = (uint64_t)(uintptr_t)s_uds_pool;
        uds_param[3] = 0x10;

        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(const void *, const void *, uint64_t, uint64_t))fn_uds)(
                uds_param, NULL, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "universal-data-system", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "universal-data-system", "rc", (uint64_t)(uint32_t)rc,
                           "code");
        obs_report_bytes(id, "universal-data-system", "param-after-20h", 0,
                         (const uint8_t *)uds_param, sizeof(uds_param));
    }

    /* (E) sceNpTrophy2RegisterUnlockCallback(callback, 0, userdata) */
    int bound_tr = 0;
    int is_link_tr = 0;
    const void *fn_tr = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpTrophy2RegisterUnlockCallback_static)) {
        fn_tr = (const void *)&sceNpTrophy2RegisterUnlockCallback_static;
        bound_tr = 1;
        is_link_tr = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpTrophy2RegisterUnlockCallback_be)) {
        fn_tr = (const void *)&sceNpTrophy2RegisterUnlockCallback_be;
        bound_tr = 1;
        is_link_tr = 1;
    } else {
        fn_tr = census_resolve_symbol(
            "libSceNpTrophy2", "$ipQa03zGRbE", "sceNpTrophy2RegisterUnlockCallback",
            (const void *)&sceNpTrophy2RegisterUnlockCallback_static, NULL);
        if (!fn_tr) {
            fn_tr = census_resolve_symbol(
                "libSceNpTrophy2", "$sUXGfNMalIo", "sceNpTrophy2RegisterUnlockCallback",
                (const void *)&sceNpTrophy2RegisterUnlockCallback_be, NULL);
        }
        if (!fn_tr) {
            fn_tr = scan_tr;
        }
        if (fn_tr && obs_address_is_callable(fn_tr)) {
            bound_tr = 1;
            is_link_tr = 0;
        } else {
            fn_tr = NULL;
            bound_tr = 0;
        }
    }
    obs_report_measure(id, "sceNpTrophy2RegisterUnlockCallback", "bound",
                       (uint64_t)bound_tr, "bool");
    obs_report_measure(id, "trophy2-register-callback", "bound", (uint64_t)bound_tr,
                       "bool");
    if (bound_tr) {
        obs_report_measure(id, "sceNpTrophy2RegisterUnlockCallback", "how",
                           (uint64_t)(is_link_tr ? 1 : 2),
                           is_link_tr ? "link" : "dlsym");
        s_ppsa02664_trophy_cb_ran = 0;
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = ((int (*)(void *, uint64_t, void *))fn_tr)(
                (void *)ppsa02664_trophy_cb, 0, (void *)0xdeadbeefcafebabeULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "trophy2-register-callback", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "trophy2-register-callback", "rc",
                           (uint64_t)(uint32_t)rc, "code");

        /* Check if callback ran or if sceNpCheckCallback invokes it */
        if (obs_address_is_callable((const void *)&sceNpCheckCallback_static)) {
            (void)sceNpCheckCallback_static();
        } else if (obs_address_is_callable((const void *)&sceNpCheckCallback_alt)) {
            (void)sceNpCheckCallback_alt();
        }
        obs_report_measure(id, "trophy2-register-callback", "callback-ran",
                           (uint64_t)s_ppsa02664_trophy_cb_ran, "bool");
    }

    return obs_pass();
}

/* REQ-20261011T1430Z-sh02: Share content event callbacks */
static volatile uint32_t s_sh_cb0_called = 0;
static volatile uint64_t s_sh_cb0_args[4] = {0};
static volatile uint32_t s_sh_cb1_called = 0;
static volatile uint64_t s_sh_cb1_args[4] = {0};

static void probe_share_cb0(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    s_sh_cb0_called++;
    s_sh_cb0_args[0] = a0;
    s_sh_cb0_args[1] = a1;
    s_sh_cb0_args[2] = a2;
    s_sh_cb0_args[3] = a3;
}

static void probe_share_cb1(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    s_sh_cb1_called++;
    s_sh_cb1_args[0] = a0;
    s_sh_cb1_args[1] = a1;
    s_sh_cb1_args[2] = a2;
    s_sh_cb1_args[3] = a3;
}

/* REQ-20261011T0900Z-sh01, REQ-20261011T1310Z-lk01: sceShareInitialize in PPSA21564's
 * shape */
static obs_result check_share_initialize(void) {
    const char *id = "070-user/share-initialize";
    obs_jmp_buf guard;

    typedef int (*fn_share_init_t)(uint64_t, uint64_t, uint64_t);
    typedef int (*fn_share_term_t)(void);

    fn_share_init_t fn_init = NULL;
    int is_link_init = 0;
    if (obs_address_is_callable((const void *)&sceShareInitialize_static)) {
        fn_init = (fn_share_init_t)&sceShareInitialize_static;
        is_link_init = 1;
    } else if (obs_address_is_callable((const void *)&sceShareInitialize_alt)) {
        fn_init = (fn_share_init_t)&sceShareInitialize_alt;
        is_link_init = 1;
    }

    int h_sh = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        int m = sceKernelLoadStartModule("/system/common/lib/libSceShare.native.sprx",
                                         0, NULL, 0, NULL, NULL);
        if (m > 1) {
            h_sh = m;
            obs_sysmodule_cache_handle("libSceShare", h_sh);
        }
        (void)sceKernelLoadStartModule("/system/common/lib/libSceShareUtility.sprx", 0,
                                       NULL, 0, NULL, NULL);
    }

    static const char *const s_share_init_cands[] = {
        "nBDD66kiFW8#J#A", "nBDD66kiFW8#A#B", "nBDD66kiFW8#j#i", "nBDD66kiFW8#T#U",
        "nBDD66kiFW8#C#A", "nBDD66kiFW8",     "$nBDD66kiFW8",    "sceShareInitialize",
        "bxUiqevDEJw#J#A", "bxUiqevDEJw#A#B", "bxUiqevDEJw#j#i", "bxUiqevDEJw#T#U",
        "bxUiqevDEJw#C#A", "bxUiqevDEJw",     "$bxUiqevDEJw",    "ErH6tKS7fzE#j#i",
        "ErH6tKS7fzE",     "GQTObcITIXI#j#i", "GQTObcITIXI",     "5wjxESwX68I#j#i",
        "5wjxESwX68I",     "6jVnRc9W6ug#j#i", "6jVnRc9W6ug"};
    static const size_t num_init_cands =
        sizeof(s_share_init_cands) / sizeof(s_share_init_cands[0]);

    static const char *const s_share_term_cands[] = {
        "0IL1keINExQ#J#A", "0IL1keINExQ#A#B", "0IL1keINExQ#j#i", "0IL1keINExQ#T#U",
        "0IL1keINExQ#C#A", "0IL1keINExQ",     "$0IL1keINExQ",    "sceShareTerminate",
        "FBMN4pH1gtA#J#A", "FBMN4pH1gtA#A#B", "FBMN4pH1gtA#j#i", "FBMN4pH1gtA#T#U",
        "FBMN4pH1gtA#C#A", "FBMN4pH1gtA",     "$FBMN4pH1gtA",    "bXT3brNpRik#j#i",
        "bXT3brNpRik"};
    static const size_t num_term_cands =
        sizeof(s_share_term_cands) / sizeof(s_share_term_cands[0]);

    int (*fn_dlsym)(int, const char *, void **) =
        obs_address_is_callable((const void *)&sceKernelDlsym)
            ? (int (*)(int, const char *, void **))&sceKernelDlsym
            : NULL;

    if (fn_init == NULL && fn_dlsym) {
        if (h_sh > 1) {
            for (size_t c = 0; c < num_init_cands; c++) {
                void *p = NULL;
                if (fn_dlsym(h_sh, s_share_init_cands[c], &p) == 0 &&
                    obs_address_is_callable(p)) {
                    fn_init = (fn_share_init_t)p;
                    break;
                }
            }
        }
        if (fn_init == NULL) {
            for (int h = 1; h <= 256; h++) {
                for (size_t c = 0; c < num_init_cands; c++) {
                    void *p = NULL;
                    if (fn_dlsym(h, s_share_init_cands[c], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_init = (fn_share_init_t)p;
                        h_sh = h;
                        obs_sysmodule_cache_handle("libSceShare", h);
                        break;
                    }
                }
                if (fn_init != NULL) {
                    break;
                }
            }
        }
    }
    if (fn_init == NULL) {
        int m_u = obs_module_open("libSceShareUtility");
        int m_s = obs_module_open("libSceShare");
        for (size_t c = 0; c < num_init_cands; c++) {
            const void *p = NULL;
            if (m_u > 0) {
                p = obs_module_symbol(m_u, s_share_init_cands[c]);
            }
            if (!p && m_s > 0) {
                p = obs_module_symbol(m_s, s_share_init_cands[c]);
            }
            if (p && obs_address_is_callable(p)) {
                fn_init = (fn_share_init_t)p;
                break;
            }
        }
    }

    fn_share_term_t fn_term = NULL;
    int is_link_term = 0;
    if (obs_address_is_callable((const void *)&sceShareTerminate_static)) {
        fn_term = (fn_share_term_t)&sceShareTerminate_static;
        is_link_term = 1;
    } else if (obs_address_is_callable((const void *)&sceShareTerminate_alt)) {
        fn_term = (fn_share_term_t)&sceShareTerminate_alt;
        is_link_term = 1;
    }
    if (fn_term == NULL && fn_dlsym) {
        if (h_sh > 1) {
            for (size_t c = 0; c < num_term_cands; c++) {
                void *p = NULL;
                if (fn_dlsym(h_sh, s_share_term_cands[c], &p) == 0 &&
                    obs_address_is_callable(p)) {
                    fn_term = (fn_share_term_t)p;
                    break;
                }
            }
        }
        if (fn_term == NULL) {
            for (int h = 1; h <= 256; h++) {
                for (size_t c = 0; c < num_term_cands; c++) {
                    void *p = NULL;
                    if (fn_dlsym(h, s_share_term_cands[c], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_term = (fn_share_term_t)p;
                        break;
                    }
                }
                if (fn_term != NULL) {
                    break;
                }
            }
        }
    }
    if (fn_term == NULL) {
        int m_u = obs_module_open("libSceShareUtility");
        int m_s = obs_module_open("libSceShare");
        for (size_t c = 0; c < num_term_cands; c++) {
            const void *p = NULL;
            if (m_u > 0) {
                p = obs_module_symbol(m_u, s_share_term_cands[c]);
            }
            if (!p && m_s > 0) {
                p = obs_module_symbol(m_s, s_share_term_cands[c]);
            }
            if (p && obs_address_is_callable(p)) {
                fn_term = (fn_share_term_t)p;
                break;
            }
        }
    }

    int bound_init =
        (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceShareInitialize", "bound", (uint64_t)bound_init, "bool");
    if (!bound_init) {
        return obs_skip("sceShareInitialize not resolved or not callable");
    }
    obs_report_measure(id, "sceShareInitialize", "how",
                       (uint64_t)(is_link_init ? 1 : 2),
                       is_link_init ? "link" : "dlsym");

    /* 1. Try calling sceShareInitialize without loading 0x10f first */
    int rc_no_10f = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_no_10f = fn_init(0x20000ULL, 0x2bcULL, 0x1fffULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-without-0x10f", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-without-0x10f", "rc", (uint64_t)(uint32_t)rc_no_10f,
                       "code");

    /* 2. Load sysmodules 0x96, 0xa9, and 0x10f */
    static const uint32_t sh_sysmodules[] = {0x0096, 0x00a9, 0x010f};
    for (size_t i = 0; i < sizeof(sh_sysmodules) / sizeof(sh_sysmodules[0]); i++) {
        uint32_t sm_id = sh_sysmodules[i];
        int rc_load = -1;
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            rc_load = sceSysmoduleLoadModule((uint16_t)sm_id);
        } else {
            rc_load = obs_sysmodule_load_id(sm_id);
        }
        char key[32];
        oops_snprintf(key, sizeof(key), "sysmodule-0x%04x-rc", sm_id);
        obs_report_measure(id, "sysmodule-load", key, (uint64_t)(uint32_t)rc_load,
                           "code");
    }

    /* 3. Call 1: sceShareInitialize(0x20000, 0x2bc, 0x1fff) */
    int rc1 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_init(0x20000ULL, 0x2bcULL, 0x1fffULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call1", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "call1", "rc", (uint64_t)(uint32_t)rc1, "code");

    /* 4. Call 2 (repeat): sceShareInitialize(0x20000, 0x2bc, 0x1fff) */
    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_init(0x20000ULL, 0x2bcULL, 0x1fffULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call2", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "call2", "rc", (uint64_t)(uint32_t)rc2, "code");

    /* REQ-20261011T1430Z-sh02: sceShareRegisterContentEventCallback &
     * sceShareUnregisterContentEventCallback */
    typedef int (*fn_share_reg_t)(const void *);
    typedef int (*fn_share_unreg_t)(void);

    fn_share_reg_t fn_reg = NULL;
    int is_link_reg = 0;
    if (obs_address_is_callable(
            (const void *)&sceShareRegisterContentEventCallback_static)) {
        fn_reg = (fn_share_reg_t)&sceShareRegisterContentEventCallback_static;
        is_link_reg = 1;
    }
    if (fn_reg == NULL && fn_dlsym) {
        static const char *const s_reg_cands[] = {
            "Sygnk9dr5WQ#C#A", "Sygnk9dr5WQ", "$Sygnk9dr5WQ",
            "sceShareRegisterContentEventCallback"};
        for (size_t c = 0; c < sizeof(s_reg_cands) / sizeof(s_reg_cands[0]); c++) {
            void *p = NULL;
            if (h_sh > 1 && fn_dlsym(h_sh, s_reg_cands[c], &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_reg = (fn_share_reg_t)p;
                break;
            }
        }
    }

    fn_share_unreg_t fn_unreg = NULL;
    int is_link_unreg = 0;
    if (obs_address_is_callable(
            (const void *)&sceShareUnregisterContentEventCallback_static)) {
        fn_unreg = (fn_share_unreg_t)&sceShareUnregisterContentEventCallback_static;
        is_link_unreg = 1;
    }
    if (fn_unreg == NULL && fn_dlsym) {
        static const char *const s_unreg_cands[] = {
            "KnsfHKmZqFA#C#A", "KnsfHKmZqFA", "$KnsfHKmZqFA",
            "sceShareUnregisterContentEventCallback"};
        for (size_t c = 0; c < sizeof(s_unreg_cands) / sizeof(s_unreg_cands[0]); c++) {
            void *p = NULL;
            if (h_sh > 1 && fn_dlsym(h_sh, s_unreg_cands[c], &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_unreg = (fn_share_unreg_t)p;
                break;
            }
        }
    }

    int bound_reg = (fn_reg != NULL && obs_address_is_callable((const void *)fn_reg));
    obs_report_measure(id, "sceShareRegisterContentEventCallback", "bound",
                       (uint64_t)bound_reg, "bool");
    if (bound_reg) {
        obs_report_measure(id, "sceShareRegisterContentEventCallback", "how",
                           (uint64_t)(is_link_reg ? 1 : 2),
                           is_link_reg ? "link" : "dlsym");

        uint8_t reg_block[0x30];
        memset(reg_block, 0, sizeof(reg_block));
        *(uintptr_t *)(void *)(reg_block + 0x00) = (uintptr_t)probe_share_cb0;
        *(uintptr_t *)(void *)(reg_block + 0x08) = (uintptr_t)probe_share_cb1;

        s_sh_cb0_called = 0;
        s_sh_cb1_called = 0;

        int rc_reg1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reg1 = fn_reg(reg_block);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "register-1", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "register-1", "rc", (uint64_t)(uint32_t)rc_reg1, "code");
        obs_report_bytes(id, "register-1", "block-after-48b", 0, reg_block,
                         sizeof(reg_block));

        /* Wait up to 2 seconds to observe if either callback was invoked */
        uint64_t t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                          ? (uint64_t)sceKernelGetProcessTime()
                          : 0;
        uint64_t t_end = t0 + 2000000;
        while (1) {
            if (s_sh_cb0_called > 0 || s_sh_cb1_called > 0)
                break;
            uint64_t now =
                obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                    ? (uint64_t)sceKernelGetProcessTime()
                    : 0;
            if (now >= t_end || now < t0)
                break;
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(20000);
            }
        }
        obs_report_measure(id, "callback-0", "called", (uint64_t)s_sh_cb0_called,
                           "count");
        if (s_sh_cb0_called > 0) {
            obs_report_measure(id, "callback-0", "arg0", s_sh_cb0_args[0], "val");
            obs_report_measure(id, "callback-0", "arg1", s_sh_cb0_args[1], "val");
            obs_report_measure(id, "callback-0", "arg2", s_sh_cb0_args[2], "val");
            obs_report_measure(id, "callback-0", "arg3", s_sh_cb0_args[3], "val");
        }
        obs_report_measure(id, "callback-1", "called", (uint64_t)s_sh_cb1_called,
                           "count");
        if (s_sh_cb1_called > 0) {
            obs_report_measure(id, "callback-1", "arg0", s_sh_cb1_args[0], "val");
            obs_report_measure(id, "callback-1", "arg1", s_sh_cb1_args[1], "val");
            obs_report_measure(id, "callback-1", "arg2", s_sh_cb1_args[2], "val");
            obs_report_measure(id, "callback-1", "arg3", s_sh_cb1_args[3], "val");
        }

        /* Second register with same block */
        int rc_reg2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reg2 = fn_reg(reg_block);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "register-2", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "register-2", "rc", (uint64_t)(uint32_t)rc_reg2, "code");
    }

    int bound_unreg =
        (fn_unreg != NULL && obs_address_is_callable((const void *)fn_unreg));
    obs_report_measure(id, "sceShareUnregisterContentEventCallback", "bound",
                       (uint64_t)bound_unreg, "bool");
    if (bound_unreg) {
        obs_report_measure(id, "sceShareUnregisterContentEventCallback", "how",
                           (uint64_t)(is_link_unreg ? 1 : 2),
                           is_link_unreg ? "link" : "dlsym");
        int rc_unreg = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_unreg = fn_unreg();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "unregister", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "unregister", "rc", (uint64_t)(uint32_t)rc_unreg,
                           "code");
    }

    /* 5. Terminate if resolved */
    int bound_term =
        (fn_term != NULL && obs_address_is_callable((const void *)fn_term));
    obs_report_measure(id, "sceShareTerminate", "bound", (uint64_t)bound_term, "bool");
    if (bound_term) {
        obs_report_measure(id, "sceShareTerminate", "how",
                           (uint64_t)(is_link_term ? 1 : 2),
                           is_link_term ? "link" : "dlsym");
        int rc_term = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "terminate", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "terminate", "rc", (uint64_t)(uint32_t)rc_term, "code");
    }

    return obs_pass();
}

/* REQ-20261011T0130Z-md01, REQ-20261011T1310Z-lk01: the message dialog's open-to-close
 * cycle, in PPSA02664's shape */
static obs_result check_msg_dialog_cycle(void) {
    const char *id = "070-user/message-dialog-cycle";
    obs_jmp_buf guard;

    int h_cmn = -1;
    int h_msg = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_cmn = sceKernelLoadStartModule("/system/common/lib/libSceCommonDialog.sprx",
                                         0, NULL, 0, NULL, NULL);
        h_msg = sceKernelLoadStartModule("/system/common/lib/libSceMsgDialog.sprx", 0,
                                         NULL, 0, NULL, NULL);
        if (h_msg <= 0) {
            h_msg = sceKernelLoadStartModule(
                "/system/common/lib/libSceMsgDialog.native.sprx", 0, NULL, 0, NULL,
                NULL);
        }
    }
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0096);
        (void)sceSysmoduleLoadModule(0x00a9);
    }
    (void)obs_sysmodule_load_id(0x0096);
    (void)obs_sysmodule_load_id(0x00a9);
    if (h_cmn <= 0)
        h_cmn = obs_find_module_handle_by_library("libSceCommonDialog");
    if (h_msg <= 0)
        h_msg = obs_find_module_handle_by_library("libSceMsgDialog.native");
    if (h_msg <= 0)
        h_msg = obs_find_module_handle_by_library("libSceMsgDialog");

    typedef int (*fn_cmn_init_t)(void);
    typedef int (*fn_msg_init_t)(void);
    typedef int (*fn_msg_open_t)(const void *);
    typedef int (*fn_msg_update_t)(void);
    typedef int (*fn_msg_get_status_t)(void);
    typedef int (*fn_msg_get_result_t)(void *);
    typedef int (*fn_msg_close_t)(void);
    typedef int (*fn_msg_term_t)(void);

    fn_cmn_init_t fn_cmn_init = NULL;
    fn_msg_init_t fn_msg_init = NULL;
    fn_msg_open_t fn_msg_open = NULL;
    fn_msg_update_t fn_msg_update = NULL;
    fn_msg_get_status_t fn_msg_get_status = NULL;
    fn_msg_get_result_t fn_msg_get_result = NULL;
    fn_msg_close_t fn_msg_close = NULL;
    fn_msg_term_t fn_msg_term = NULL;

    int is_link_cmn = 0, is_link_minit = 0, is_link_mopen = 0, is_link_mupdate = 0;
    int is_link_mstatus = 0, is_link_mresult = 0, is_link_mclose = 0, is_link_mterm = 0;

    if (obs_address_is_callable((const void *)&sceCommonDialogInitialize_static)) {
        fn_cmn_init = (fn_cmn_init_t)&sceCommonDialogInitialize_static;
        is_link_cmn = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogInitialize_static)) {
        fn_msg_init = (fn_msg_init_t)&sceMsgDialogInitialize_static;
        is_link_minit = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogOpen_static)) {
        fn_msg_open = (fn_msg_open_t)&sceMsgDialogOpen_static;
        is_link_mopen = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogUpdateStatus_static)) {
        fn_msg_update = (fn_msg_update_t)&sceMsgDialogUpdateStatus_static;
        is_link_mupdate = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogGetStatus_static)) {
        fn_msg_get_status = (fn_msg_get_status_t)&sceMsgDialogGetStatus_static;
        is_link_mstatus = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogGetResult_static)) {
        fn_msg_get_result = (fn_msg_get_result_t)&sceMsgDialogGetResult_static;
        is_link_mresult = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogClose_static)) {
        fn_msg_close = (fn_msg_close_t)&sceMsgDialogClose_static;
        is_link_mclose = 1;
    }
    if (obs_address_is_callable((const void *)&sceMsgDialogTerminate_static)) {
        fn_msg_term = (fn_msg_term_t)&sceMsgDialogTerminate_static;
        is_link_mterm = 1;
    }

    int (*fn_dlsym)(int, const char *, void **) =
        obs_address_is_callable((const void *)&sceKernelDlsym)
            ? (int (*)(int, const char *, void **))&sceKernelDlsym
            : NULL;

    if (fn_dlsym) {
        if (!fn_cmn_init && h_cmn > 0) {
            void *p = NULL;
            if (fn_dlsym(h_cmn, "uoUpLGNkygk#F#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_cmn_init = (fn_cmn_init_t)p;
            if (!fn_cmn_init && fn_dlsym(h_cmn, "sceCommonDialogInitialize", &p) == 0 &&
                obs_address_is_callable(p))
                fn_cmn_init = (fn_cmn_init_t)p;
        }
        if (h_msg > 0) {
            void *p = NULL;
            if (!fn_msg_init && fn_dlsym(h_msg, "lDqxaY1UbEo#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_init = (fn_msg_init_t)p;
            if (!fn_msg_init && fn_dlsym(h_msg, "sceMsgDialogInitialize", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_init = (fn_msg_init_t)p;

            if (!fn_msg_open && fn_dlsym(h_msg, "b06Hh0DPEaE#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_open = (fn_msg_open_t)p;
            if (!fn_msg_open && fn_dlsym(h_msg, "sceMsgDialogOpen", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_open = (fn_msg_open_t)p;

            if (!fn_msg_update && fn_dlsym(h_msg, "6fIC3XKt2k0#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_update = (fn_msg_update_t)p;
            if (!fn_msg_update &&
                fn_dlsym(h_msg, "sceMsgDialogUpdateStatus", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_update = (fn_msg_update_t)p;

            if (!fn_msg_get_status && fn_dlsym(h_msg, "CWVW78Qc3fI#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_get_status = (fn_msg_get_status_t)p;
            if (!fn_msg_get_status &&
                fn_dlsym(h_msg, "sceMsgDialogGetStatus", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_get_status = (fn_msg_get_status_t)p;

            if (!fn_msg_get_result && fn_dlsym(h_msg, "Lr8ovHH9l6A#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_get_result = (fn_msg_get_result_t)p;
            if (!fn_msg_get_result &&
                fn_dlsym(h_msg, "sceMsgDialogGetResult", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_get_result = (fn_msg_get_result_t)p;

            if (!fn_msg_close && fn_dlsym(h_msg, "HTrcDKlFKuM#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_close = (fn_msg_close_t)p;
            if (!fn_msg_close && fn_dlsym(h_msg, "sceMsgDialogClose", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_close = (fn_msg_close_t)p;

            if (!fn_msg_term && fn_dlsym(h_msg, "ePw-kqZmelo#G#A", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_term = (fn_msg_term_t)p;
            if (!fn_msg_term && fn_dlsym(h_msg, "sceMsgDialogTerminate", &p) == 0 &&
                obs_address_is_callable(p))
                fn_msg_term = (fn_msg_term_t)p;
        }
    }

    obs_report_measure(id, "sceCommonDialogInitialize", "bound",
                       (uint64_t)(fn_cmn_init != NULL), "bool");
    if (fn_cmn_init)
        obs_report_measure(id, "sceCommonDialogInitialize", "how",
                           (uint64_t)(is_link_cmn ? 1 : 2),
                           is_link_cmn ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogInitialize", "bound",
                       (uint64_t)(fn_msg_init != NULL), "bool");
    if (fn_msg_init)
        obs_report_measure(id, "sceMsgDialogInitialize", "how",
                           (uint64_t)(is_link_minit ? 1 : 2),
                           is_link_minit ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogOpen", "bound", (uint64_t)(fn_msg_open != NULL),
                       "bool");
    if (fn_msg_open)
        obs_report_measure(id, "sceMsgDialogOpen", "how",
                           (uint64_t)(is_link_mopen ? 1 : 2),
                           is_link_mopen ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogUpdateStatus", "bound",
                       (uint64_t)(fn_msg_update != NULL), "bool");
    if (fn_msg_update)
        obs_report_measure(id, "sceMsgDialogUpdateStatus", "how",
                           (uint64_t)(is_link_mupdate ? 1 : 2),
                           is_link_mupdate ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogGetStatus", "bound",
                       (uint64_t)(fn_msg_get_status != NULL), "bool");
    if (fn_msg_get_status)
        obs_report_measure(id, "sceMsgDialogGetStatus", "how",
                           (uint64_t)(is_link_mstatus ? 1 : 2),
                           is_link_mstatus ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogGetResult", "bound",
                       (uint64_t)(fn_msg_get_result != NULL), "bool");
    if (fn_msg_get_result)
        obs_report_measure(id, "sceMsgDialogGetResult", "how",
                           (uint64_t)(is_link_mresult ? 1 : 2),
                           is_link_mresult ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogClose", "bound",
                       (uint64_t)(fn_msg_close != NULL), "bool");
    if (fn_msg_close)
        obs_report_measure(id, "sceMsgDialogClose", "how",
                           (uint64_t)(is_link_mclose ? 1 : 2),
                           is_link_mclose ? "link" : "dlsym");

    obs_report_measure(id, "sceMsgDialogTerminate", "bound",
                       (uint64_t)(fn_msg_term != NULL), "bool");
    if (fn_msg_term)
        obs_report_measure(id, "sceMsgDialogTerminate", "how",
                           (uint64_t)(is_link_mterm ? 1 : 2),
                           is_link_mterm ? "link" : "dlsym");

    if (!fn_msg_open) {
        return obs_skip("sceMsgDialogOpen not resolved");
    }

    /* 1. sceCommonDialogInitialize */
    int rc_cmn = -1;
    if (fn_cmn_init) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cmn = fn_cmn_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "common-dialog-init", "rc", (uint64_t)(uint32_t)rc_cmn,
                       "code");

    /* 2. sceMsgDialogInitialize */
    int rc_minit = -1;
    if (fn_msg_init) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_minit = fn_msg_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "msg-dialog-init", "rc", (uint64_t)(uint32_t)rc_minit,
                       "code");

    /* 3. sceMsgDialogOpen with user-message parameter (base size 0x30, OK button, short
     * ASCII msg) */
    struct {
        uint64_t base_size;
        uint8_t base_reserved[36];
        uint32_t base_magic;
        size_t size;
        int32_t mode;
        int32_t pad;
        void *userMsgParam;
        void *progBarParam;
        void *sysMsgParam;
        int32_t userId;
        uint8_t reserved[40];
        int32_t pad2;
    } __attribute__((aligned(8))) param;
    memset(&param, 0, sizeof(param));
    param.base_size = 0x30;
    param.base_magic = (uint32_t)(0xC0D1A109u + (uintptr_t)&param);
    param.size = sizeof(param);
    param.mode = 1;

    struct {
        int32_t buttonType;
        int32_t pad;
        const char *msg;
        void *buttonsParam;
        uint8_t reserved[24];
    } user_msg;
    memset(&user_msg, 0, sizeof(user_msg));
    user_msg.buttonType = 0;
    user_msg.msg = "Please, connect the controller.";
    param.userMsgParam = &user_msg;

    int rc_open = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_open = fn_msg_open(&param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "open", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "open", "rc", (uint64_t)(uint32_t)rc_open, "code");

    /* 4. sceMsgDialogUpdateStatus and sceMsgDialogGetStatus every 100 ms for 3 s */
    for (int s = 0; s < 30; s++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "poll-100ms-%d", s);
        int u_rc = -1, st_val = -1;
        uint64_t t = sceKernelGetProcessTime();
        if (fn_msg_update) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                u_rc = fn_msg_update();
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        if (fn_msg_get_status) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                st_val = fn_msg_get_status();
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, tag, "update-rc", (uint64_t)(uint32_t)u_rc, "code");
        obs_report_measure(id, tag, "status", (uint64_t)(uint32_t)st_val, "val");
        obs_report_measure(id, tag, "time", t, "usec");
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(100000);
        }
    }

    /* 5. sceMsgDialogGetResult(buf) on 0x40-byte 0xa5 buffer while open */
    uint8_t res_buf[0x40];
    memset(res_buf, 0xa5, sizeof(res_buf));
    int rc_res_open = -1;
    if (fn_msg_get_result) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_res_open = fn_msg_get_result(res_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "get-result-while-open", "rc",
                       (uint64_t)(uint32_t)rc_res_open, "code");
    obs_report_bytes(id, "get-result-while-open", "buf-40h", 0, res_buf,
                     sizeof(res_buf));

    /* 6. sceMsgDialogClose() */
    int rc_close = -1;
    if (fn_msg_close) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_close = fn_msg_close();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "close", "rc", (uint64_t)(uint32_t)rc_close, "code");

    /* 7. status calls until they change, and GetResult again */
    int final_status = -1;
    for (int wait = 0; wait < 50; wait++) {
        if (fn_msg_update)
            (void)fn_msg_update();
        if (fn_msg_get_status)
            final_status = fn_msg_get_status();
        if (final_status != 1 && final_status != 0)
            break;
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(20000);
        }
    }
    obs_report_measure(id, "status-after-close", "status",
                       (uint64_t)(uint32_t)final_status, "val");

    memset(res_buf, 0xa5, sizeof(res_buf));
    int rc_res_closed = -1;
    if (fn_msg_get_result) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_res_closed = fn_msg_get_result(res_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "get-result-after-close", "rc",
                       (uint64_t)(uint32_t)rc_res_closed, "code");
    obs_report_bytes(id, "get-result-after-close", "buf-40h", 0, res_buf,
                     sizeof(res_buf));

    /* 8. sceMsgDialogTerminate(), and a second Terminate() */
    int rc_term1 = -1, rc_term2 = -1;
    if (fn_msg_term) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term1 = fn_msg_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term2 = fn_msg_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "term1", "rc", (uint64_t)(uint32_t)rc_term1, "code");
    obs_report_measure(id, "term2", "rc", (uint64_t)(uint32_t)rc_term2, "code");

    /* 9. Open with null param, and with base size 0 */
    int rc_open_null = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_open_null = fn_msg_open(NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "open-null", "rc", (uint64_t)(uint32_t)rc_open_null, "code");

    param.base_size = 0;
    int rc_open_sz0 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_open_sz0 = fn_msg_open(&param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "open-base-size-0", "rc", (uint64_t)(uint32_t)rc_open_sz0,
                       "code");

    return obs_pass();
}

/* REQ-20261010T2210Z-js03, REQ-20261011T1310Z-lk01: libSceJson2's value classes */
static obs_result check_json2_value_classes(void) {
    const char *id = "070-user/json2-value-classes";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x00e7);
    }
    (void)obs_sysmodule_load_id(0x00e7);

    int h_json = -1;
    (void)obs_sysmodule_get_handle_id(0x00e7, &h_json);
    if (h_json <= 1) {
        h_json = obs_find_module_handle_by_library("libSceJson2");
    }

    obs_report_measure(id, "load-module", "handle", (uint64_t)(int64_t)h_json,
                       "handle");

    int (*fn_dlsym)(int, const char *, void **) =
        obs_address_is_callable((const void *)&sceKernelDlsym)
            ? (int (*)(int, const char *, void **))&sceKernelDlsym
            : NULL;

    static const struct {
        const char *name;
        const char *mangled;
        const char *nid;
        const void *static_fn;
    } s_json2_syms[] = {
        {"Initializer_initialize",
         "_ZN3sce4Json11Initializer10initializeEPKNS0_14InitParameter2E#C#A",
         "$Cxwy7wHq4J0", (const void *)&sceJsonInitializerInitialize_static},
        {"Initializer_dtor", "_ZN3sce4Json11InitializerD1Ev#C#A", "$RujUxbr3haM",
         (const void *)&sceJsonInitializerDtor_static},
        {"MemAllocator_dtor", "_ZN3sce4Json12MemAllocatorD2Ev#C#A", "$OcAgPxcq5Vk",
         (const void *)&sceJsonMemAllocatorDtor_static},
        {"Value_default_ctor", "_ZN3sce4Json5ValueC1Ev#C#A", "$9yDWMxEFdJU",
         (const void *)&sceJson2ValueC1_static},
        {"Value_dtor", "_ZN3sce4Json5ValueD1Ev#C#A", "$WkkeywLJcgU",
         (const void *)&sceJson2ValueD1_static},
        {"Array_default_ctor", "_ZN3sce4Json5ArrayC1Ev#C#A", "$-kU6bB4M-+k",
         (const void *)&sceJson2ArrayC1_static},
        {"Array_dtor", "_ZN3sce4Json5ArrayD1Ev#C#A", "$fLcU5G6Qrjs",
         (const void *)&sceJson2ArrayD1_static},
        {"Object_default_ctor", "_ZN3sce4Json6ObjectC1Ev#C#A", "$q0F6yS-rCms",
         (const void *)&sceJson2ObjectC1_static},
        {"Object_dtor", "_ZN3sce4Json6ObjectD1Ev#C#A", "$5OqszGpy7Mg",
         (const void *)&sceJson2ObjectD1_static},
        {"String_default_ctor", "_ZN3sce4Json6StringC1Ev#C#A", "$oVkZ8W8-Q8A",
         (const void *)&sceJson2StringC1_static},
        {"String_dtor", "_ZN3sce4Json6StringD1Ev#C#A", "$rHRr+131ATY",
         (const void *)&sceJson2StringD1_static}};

    for (size_t i = 0; i < sizeof(s_json2_syms) / sizeof(s_json2_syms[0]); i++) {
        const char *sname = s_json2_syms[i].name;
        const char *mangled = s_json2_syms[i].mangled;
        const char *nid = s_json2_syms[i].nid;
        const void *static_fn = s_json2_syms[i].static_fn;

        const void *p = NULL;
        int is_link = 0;

        if (static_fn != NULL && obs_address_is_callable(static_fn)) {
            p = static_fn;
            is_link = 1;
        }

        if (!p && fn_dlsym && h_json > 0) {
            void *sym = NULL;
            if (fn_dlsym(h_json, mangled, &sym) == 0 && obs_address_is_callable(sym)) {
                p = sym;
            } else if (fn_dlsym(h_json, nid, &sym) == 0 &&
                       obs_address_is_callable(sym)) {
                p = sym;
            }
        }

        if (!p && fn_dlsym) {
            for (int h = 1; h <= 256; h++) {
                void *sym = NULL;
                if (fn_dlsym(h, mangled, &sym) == 0 && obs_address_is_callable(sym)) {
                    p = sym;
                    break;
                }
                if (fn_dlsym(h, nid, &sym) == 0 && obs_address_is_callable(sym)) {
                    p = sym;
                    break;
                }
            }
        }

        if (!p && h_json > 0) {
            p = obs_module_symbol(h_json, mangled);
            if (!p)
                p = obs_module_symbol(h_json, nid);
        }

        int is_bound = (p != NULL && obs_address_is_callable(p));
        obs_report_measure(id, sname, "bound", (uint64_t)is_bound, "bool");
        if (is_bound) {
            obs_report_measure(id, sname, "how", (uint64_t)(is_link ? 1 : 2),
                               is_link ? "link" : "dlsym");
            uint8_t obj[0x100];
            memset(obj, 0xa5, sizeof(obj));
            int sig = OBS_FAULT_ARM(&guard);
            int rc = 0;
            if (sig == 0) {
                typedef int (*fn_ctor_t)(void *);
                rc = ((fn_ctor_t)p)(obj);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, sname, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, sname, "rc", (uint64_t)(uint32_t)rc, "code");
            size_t changed = 0;
            for (size_t b = 0; b < sizeof(obj); b++) {
                if (obj[b] != 0xa5)
                    changed++;
            }
            obs_report_measure(id, sname, "extent", (uint64_t)changed, "bytes");
            if (changed > 0) {
                obs_report_bytes(id, sname, "obj-64b", 0, obj, 64);
            }
        }
    }

    return obs_pass();
}

static const obs_check user_checks[] = {
    {"070-user/message-dialog-cycle", "libSceMsgDialog", "sceMsgDialogOpen",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_msg_dialog_cycle,
     OBS_FROM_ASSUMED},
    {"070-user/json2-value-classes", "libSceJson2", "libSceJson2", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_json2_value_classes, OBS_FROM_ASSUMED},
    {"070-user/share-initialize", "libSceShare", "$nBDD66kiFW8", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_share_initialize, OBS_FROM_ASSUMED},
    {"070-user/ppsa02664-np-setup", "libSceNpManager", "$CnoyyjMD74w", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ppsa02664_np_setup, OBS_FROM_ASSUMED},
    {"070-user/np-check-callback", "libSceNpManager", "$1ofT4wV8md0", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_np_check_callback, OBS_FROM_ASSUMED},
    {"070-user/oneshot-setup-calls", "libSceUserService", "sceUserServiceGetAgeLevel",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_oneshot_setup_calls,
     OBS_FROM_ASSUMED},
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
