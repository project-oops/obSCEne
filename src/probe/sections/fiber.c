/*
 * Fibers (libSceFiber).
 *
 * Cooperative multi-threading subsystem used by high-performance game engines
 * (e.g. job systems) to yield execution explicitly without kernel preemption overhead.
 *
 * Placed immediately after thread and stackattr sections.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

typedef void (*SceFiberEntry)(uint64_t argOnInitialize, uint64_t argOnRun);

typedef struct SceFiber {
    _Alignas(16) unsigned char storage[256];
} SceFiber;

typedef int (*fn_fiber_init_t)(SceFiber *fiber, const char *name, SceFiberEntry entry,
                               uint64_t argOnInitialize, void *addrContext,
                               size_t sizeContext, const void *optParam);
typedef int (*fn_fiber_run_t)(SceFiber *fiber, uint64_t argOnRun, uint64_t *argToFiber);
typedef int (*fn_fiber_switch_t)(SceFiber *fiber, uint64_t argOnSwitch,
                                 uint64_t *argToFiber);
typedef int (*fn_fiber_return_t)(uint64_t argOnReturn, uint64_t *argToThread);
typedef int (*fn_fiber_finalize_t)(SceFiber *fiber);
typedef int (*fn_fiber_self_t)(SceFiber **fiber);
typedef int (*fn_fiber_info_t)(const SceFiber *fiber, void *info);

static const void *fiber_resolve_sym(const char *sym) {
    const void *p = obs_module_symbol(1, sym);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }
    p = obs_module_symbol(OBS_HANDLE_SELF, sym);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }
    int h = obs_module_open("libSceFiber");
    if (h >= 0) {
        p = obs_module_symbol(h, sym);
        if (p != NULL && obs_address_is_callable(p)) {
            return p;
        }
    }
    return NULL;
}

static obs_result check_fiber_sysmodule(void) {
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load =
            (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }

    obs_report_measure("033-fiber/sysmodule", "sceSysmoduleLoadModule", "callable",
                       fn_load != NULL ? 1 : 0, "flag");

    if (fn_load == NULL || !obs_address_is_callable((const void *)fn_load)) {
        return obs_skip("sceSysmoduleLoadModule not available");
    }

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    int rc = -1;
    if (sig == 0) {
        rc = fn_load(0x0006); /* OOPS_SYSMODULE_FIBER */
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail_code("sceSysmoduleLoadModule faulted", (uint64_t)(uint32_t)sig);
    }

    obs_report_measure("033-fiber/sysmodule", "libSceFiber", "module_id", 0x0006, "id");
    obs_report_measure("033-fiber/sysmodule", "sceSysmoduleLoadModule", "rc",
                       (uint64_t)(uint32_t)rc, "code");

    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_fiber_symbols(void) {
    static const char *const syms[] = {
        "sceFiberInitialize", "_sceFiberInitializeImpl", "sceFiberRun",
        "sceFiberSwitch",     "sceFiberReturnToThread",  "sceFiberFinalize",
        "sceFiberGetSelf",    "sceFiberGetInfo",
    };

    unsigned int resolved_count = 0;
    for (size_t i = 0; i < OBS_COUNT(syms); i++) {
        const char *name = syms[i];
        const void *addr = fiber_resolve_sym(name);
        obs_report_measure("033-fiber/symbols", name, "vaddr",
                           (uint64_t)(uintptr_t)addr, "vaddr");
        obs_report_measure("033-fiber/symbols", name, "resolved", addr != NULL ? 1 : 0,
                           "bool");
        if (addr != NULL) {
            resolved_count++;
        }
    }

    if (resolved_count == 0) {
        return obs_skip("no libSceFiber symbols resolved");
    }
    return obs_pass_value((uint64_t)resolved_count);
}

static volatile int s_fiber_ran = 0;
static volatile uint64_t s_fiber_arg_init = 0;
static volatile uint64_t s_fiber_arg_run = 0;
static fn_fiber_return_t s_fn_return = NULL;

static void fiber_test_entry(uint64_t argOnInitialize, uint64_t argOnRun) {
    s_fiber_ran = 1;
    s_fiber_arg_init = argOnInitialize;
    s_fiber_arg_run = argOnRun;

    if (s_fn_return != NULL && obs_address_is_callable((const void *)s_fn_return)) {
        s_fn_return(0x7788u, NULL);
    }
}

static _Alignas(16) unsigned char s_fiber_stack[65536];

static obs_result check_fiber_lifecycle(void) {
    fn_fiber_init_t fn_init = (fn_fiber_init_t)fiber_resolve_sym("sceFiberInitialize");
    if (fn_init == NULL) {
        fn_init = (fn_fiber_init_t)fiber_resolve_sym("_sceFiberInitializeImpl");
    }
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    s_fn_return = (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    fn_fiber_finalize_t fn_finalize =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");

    if (fn_init == NULL || fn_run == NULL || s_fn_return == NULL ||
        fn_finalize == NULL) {
        return obs_skip("required fiber lifecycle symbols not resolved");
    }

    SceFiber fiber;
    for (size_t i = 0; i < sizeof(fiber.storage); i++) {
        fiber.storage[i] = 0;
    }
    for (size_t i = 0; i < sizeof(s_fiber_stack); i++) {
        s_fiber_stack[i] = 0;
    }
    s_fiber_ran = 0;
    s_fiber_arg_init = 0;
    s_fiber_arg_run = 0;

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fiber lifecycle faulted", (uint64_t)(uint32_t)sig);
    }

    int rc_init = fn_init(&fiber, "obs-fiber", fiber_test_entry, 0x1122u, s_fiber_stack,
                          sizeof(s_fiber_stack), NULL);
    obs_report_measure("033-fiber/lifecycle", "sceFiberInitialize", "rc",
                       (uint64_t)(uint32_t)rc_init, "code");

    if (rc_init != 0) {
        obs_fault_unregister();
        return obs_fail_code("sceFiberInitialize refused", (uint64_t)(uint32_t)rc_init);
    }

    uint64_t out_arg = 0;
    int rc_run = fn_run(&fiber, 0x3344u, &out_arg);
    obs_report_measure("033-fiber/lifecycle", "sceFiberRun", "rc",
                       (uint64_t)(uint32_t)rc_run, "code");
    obs_report_measure("033-fiber/lifecycle", "sceFiberRun", "fiber-ran",
                       (uint64_t)s_fiber_ran, "bool");
    obs_report_measure("033-fiber/lifecycle", "sceFiberRun", "arg-init-received",
                       s_fiber_arg_init, "arg");
    obs_report_measure("033-fiber/lifecycle", "sceFiberRun", "arg-run-received",
                       s_fiber_arg_run, "arg");

    int rc_fin = fn_finalize(&fiber);
    obs_report_measure("033-fiber/lifecycle", "sceFiberFinalize", "rc",
                       (uint64_t)(uint32_t)rc_fin, "code");

    obs_fault_unregister();
    if (!s_fiber_ran) {
        return obs_fail("fiber was initialized and run, but fiber body never executed");
    }
    return obs_pass();
}

static obs_result check_fiber_invalid_args(void) {
    fn_fiber_init_t fn_init = (fn_fiber_init_t)fiber_resolve_sym("sceFiberInitialize");
    if (fn_init == NULL) {
        fn_init = (fn_fiber_init_t)fiber_resolve_sym("_sceFiberInitializeImpl");
    }
    if (fn_init == NULL || !obs_address_is_callable((const void *)fn_init)) {
        return obs_skip("sceFiberInitialize not callable");
    }

    SceFiber fiber;
    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("sceFiberInitialize invalid args faulted",
                             (uint64_t)(uint32_t)sig);
    }

    /* 1. NULL fiber pointer */
    int rc_null_fiber =
        fn_init(NULL, "bad", fiber_test_entry, 0, s_fiber_stack, 4096, NULL);
    obs_report_measure("033-fiber/invalid-args", "null-fiber", "rc",
                       (uint64_t)(uint32_t)rc_null_fiber, "code");

    /* 2. Zero stack size */
    int rc_zero_stack =
        fn_init(&fiber, "bad", fiber_test_entry, 0, s_fiber_stack, 0, NULL);
    obs_report_measure("033-fiber/invalid-args", "zero-stack", "rc",
                       (uint64_t)(uint32_t)rc_zero_stack, "code");

    /* 3. NULL stack buffer */
    int rc_null_stack = fn_init(&fiber, "bad", fiber_test_entry, 0, NULL, 4096, NULL);
    obs_report_measure("033-fiber/invalid-args", "null-stack", "rc",
                       (uint64_t)(uint32_t)rc_null_stack, "code");

    obs_fault_unregister();
    return obs_pass();
}

static const obs_check fiber_checks[] = {
    {"033-fiber/sysmodule", "libSceSysmodule", "sceSysmoduleLoadModule", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_fiber_sysmodule, check_fiber_sysmodule,
     OBS_FROM_ASSUMED},
    {"033-fiber/symbols", "libSceFiber", "sceFiberInitialize", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_fiber_symbols, check_fiber_symbols,
     OBS_FROM_ASSUMED},
    {"033-fiber/lifecycle", "libSceFiber", "sceFiberRun", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_fiber_lifecycle, check_fiber_lifecycle, OBS_FROM_ASSUMED},
    {"033-fiber/invalid-args", "libSceFiber", "sceFiberInitialize", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_fiber_invalid_args, check_fiber_invalid_args,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_fiber = {
    "033-fiber",
    "Fibers",
    "Cooperative multitasking, fiber stack creation, switching and termination.",
    fiber_checks,
    OBS_COUNT(fiber_checks),
};
