/*
 * Fibers (libSceFiber).
 *
 * Cooperative multi-threading subsystem used by high-performance game engines
 * (e.g. job systems) to yield execution explicitly without kernel preemption overhead.
 *
 * Placed immediately after thread and stackattr sections.
 */

#include <stddef.h>
#define memset __builtin_memset
#define memcpy __builtin_memcpy

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

#if defined(__x86_64__)
static inline void set_mxcsr(uint32_t val) {
    __asm__ volatile("ldmxcsr %0" : : "m"(val));
}
static inline uint32_t get_mxcsr(void) {
    uint32_t val = 0;
    __asm__ volatile("stmxcsr %0" : "=m"(val));
    return val;
}
#endif

typedef void (*SceFiberEntry)(uint64_t argOnInitialize, uint64_t argOnRun);

typedef struct SceFiber {
    _Alignas(16) unsigned char storage[256];
} SceFiber;

typedef int (*fn_fiber_init_t)(SceFiber *fiber, const char *name, SceFiberEntry entry,
                               uint64_t argOnInitialize, void *addrContext,
                               size_t sizeContext, const void *optParam);
typedef int (*fn_fiber_init_impl_t)(SceFiber *fiber, const char *name,
                                    SceFiberEntry entry, uint64_t argOnInitialize,
                                    void *addrContext, size_t sizeContext,
                                    const void *optParam, uint64_t flags);
typedef int (*fn_fiber_run_t)(SceFiber *fiber, uint64_t argOnRun, uint64_t *argToFiber);
typedef int (*fn_fiber_switch_t)(SceFiber *fiber, uint64_t argOnSwitch,
                                 uint64_t *argToFiber);
typedef int (*fn_fiber_return_t)(uint64_t argOnReturn, uint64_t *argToThread);
typedef int (*fn_fiber_finalize_t)(SceFiber *fiber);
typedef int (*fn_fiber_self_t)(SceFiber **fiber);
typedef int (*fn_fiber_info_t)(const SceFiber *fiber, void *info);

static const void *fiber_resolve_sym(const char *sym) {
    const char *nid = NULL;
    if (obs_strcmp(sym, "_sceFiberInitializeImpl") == 0) {
        nid = "$hVYD7Ou2pCQ";
    } else if (obs_strcmp(sym, "sceFiberInitialize") == 0) {
        nid = "$O0QBhJn6rI4";
    } else if (obs_strcmp(sym, "sceFiberRun") == 0) {
        nid = "$a0LLrZWac0M";
    } else if (obs_strcmp(sym, "sceFiberSwitch") == 0) {
        nid = "$PFT2S-tJ7Uk";
    } else if (obs_strcmp(sym, "sceFiberReturnToThread") == 0) {
        nid = "$B0ZX2hx9DMw";
    } else if (obs_strcmp(sym, "sceFiberFinalize") == 0) {
        nid = "$JeNX5F-NzQU";
    } else if (obs_strcmp(sym, "sceFiberGetSelf") == 0) {
        nid = "$p+zLIOg27zU";
    } else if (obs_strcmp(sym, "sceFiberGetInfo") == 0) {
        nid = "$uq2Y5BFz0PE";
    }

    const void *p = obs_module_symbol(1, sym);
    if (!p && nid)
        p = obs_module_symbol(1, nid);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }

    p = obs_module_symbol(OBS_HANDLE_SELF, sym);
    if (!p && nid)
        p = obs_module_symbol(OBS_HANDLE_SELF, nid);
    if (p != NULL && obs_address_is_callable(p)) {
        return p;
    }

    int h = obs_module_open("libSceFiber");
    if (h < 0) {
        h = obs_module_open("libSceFiber.sprx");
    }
    if (h >= 0) {
        p = obs_module_symbol(h, sym);
        if (!p && nid)
            p = obs_module_symbol(h, nid);
        if (p != NULL && obs_address_is_callable(p)) {
            return p;
        }
    }

    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *addr = NULL;
        if (h >= 0 && sceKernelDlsym(h, sym, &addr) == 0 &&
            obs_address_is_callable(addr))
            return addr;
        if (h >= 0 && nid && sceKernelDlsym(h, nid + 1, &addr) == 0 &&
            obs_address_is_callable(addr))
            return addr;
        if (sceKernelDlsym(OBS_HANDLE_SELF, sym, &addr) == 0 &&
            obs_address_is_callable(addr))
            return addr;
        if (nid && sceKernelDlsym(OBS_HANDLE_SELF, nid + 1, &addr) == 0 &&
            obs_address_is_callable(addr))
            return addr;
        if (sceKernelDlsym(1, sym, &addr) == 0 && obs_address_is_callable(addr))
            return addr;
        if (nid && sceKernelDlsym(1, nid + 1, &addr) == 0 &&
            obs_address_is_callable(addr))
            return addr;

        int mod_handles[128];
        size_t mod_count = 0;
        if (obs_address_is_callable((const void *)&sceKernelGetModuleList) &&
            sceKernelGetModuleList(mod_handles, 128, &mod_count) == 0) {
            for (size_t m = 0; m < mod_count; m++) {
                if (sceKernelDlsym(mod_handles[m], sym, &addr) == 0 &&
                    obs_address_is_callable(addr))
                    return addr;
                if (nid && sceKernelDlsym(mod_handles[m], nid + 1, &addr) == 0 &&
                    obs_address_is_callable(addr))
                    return addr;
            }
        }
    }
    return NULL;
}

static int call_fiber_init(SceFiber *fiber, const char *name, SceFiberEntry entry,
                           uint64_t argOnInitialize, void *addrContext,
                           size_t sizeContext, const void *optParam) {
    fn_fiber_init_impl_t fn_impl =
        (fn_fiber_init_impl_t)fiber_resolve_sym("_sceFiberInitializeImpl");
    if (fn_impl != NULL && obs_address_is_callable((const void *)fn_impl)) {
        return fn_impl(fiber, name, entry, argOnInitialize, addrContext, sizeContext,
                       optParam, 0x9000000ULL);
    }
    fn_fiber_init_t fn_init = (fn_fiber_init_t)fiber_resolve_sym("sceFiberInitialize");
    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        return fn_init(fiber, name, entry, argOnInitialize, addrContext, sizeContext,
                       optParam);
    }
    return -1;
}

static obs_result check_fiber_sysmodule(void) {
    int rc = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc = sceSysmoduleLoadModule(0x0006); /* OOPS_SYSMODULE_FIBER */
    } else {
        typedef int (*fn_load_t)(uint16_t id);
        fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
        if (fn_load == NULL) {
            fn_load =
                (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
        }
        if (fn_load != NULL && obs_address_is_callable((const void *)fn_load)) {
            rc = fn_load(0x0006);
        }
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

static _Alignas(16) unsigned char s_fiber_stack[65536];

/* REQ-20260927T1415Z-5cd3 (1): Lifecycle & Values Out */
static volatile int s_lifecycle_runs = 0;
static volatile uint64_t s_lifecycle_back1 = 0;
static volatile int s_lifecycle_ret1_rc = -1;
static volatile uint64_t s_lifecycle_init_arg = 0;
static volatile uint64_t s_lifecycle_run1_arg = 0;

static void fiber_lifecycle_entry(uint64_t argOnInitialize, uint64_t argOnRun) {
    s_lifecycle_runs++;
    s_lifecycle_init_arg = argOnInitialize;
    s_lifecycle_run1_arg = argOnRun;

    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (!fn_ret)
        return;

    uint64_t back = 0;
    int rc1 = fn_ret(0x7788u, &back);
    s_lifecycle_back1 = back;
    s_lifecycle_ret1_rc = rc1;

    /* Resumed by second sceFiberRun */
    s_lifecycle_runs++;
    uint64_t back2 = 0;
    fn_ret(0x99aau, &back2);
}

static obs_result check_fiber_lifecycle(void) {
    const char *id = "033-fiber/lifecycle";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_finalize_t fn_finalize =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");

    if (fn_run == NULL || fn_finalize == NULL) {
        return obs_skip("required fiber lifecycle symbols not resolved");
    }

    SceFiber fiber;
    memset(&fiber, 0, sizeof(fiber));
    memset(s_fiber_stack, 0, sizeof(s_fiber_stack));
    s_lifecycle_runs = 0;
    s_lifecycle_back1 = 0;
    s_lifecycle_ret1_rc = -1;
    s_lifecycle_init_arg = 0;
    s_lifecycle_run1_arg = 0;

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fiber lifecycle faulted", (uint64_t)(uint32_t)sig);
    }

    int rc_init = call_fiber_init(&fiber, "obs-life", fiber_lifecycle_entry, 0x1122u,
                                  s_fiber_stack, sizeof(s_fiber_stack), NULL);
    obs_report_measure(id, "sceFiberInitialize", "rc", (uint64_t)(uint32_t)rc_init,
                       "code");

    if (rc_init != 0) {
        obs_fault_unregister();
        return obs_fail_code("sceFiberInitialize refused", (uint64_t)(uint32_t)rc_init);
    }

    /* First run */
    uint64_t out_arg1 = 0;
    int rc_run1 = fn_run(&fiber, 0x3344u, &out_arg1);
    obs_report_measure(id, "sceFiberRun-1", "rc", (uint64_t)(uint32_t)rc_run1, "code");
    obs_report_measure(id, "sceFiberRun-1", "out-arg", out_arg1, "arg");
    obs_report_measure(id, "sceFiberRun-1", "arg-init-received", s_lifecycle_init_arg,
                       "arg");
    obs_report_measure(id, "sceFiberRun-1", "arg-run-received", s_lifecycle_run1_arg,
                       "arg");

    /* Second run with argOnRunTo 0x5566 */
    uint64_t out_arg2 = 0;
    int rc_run2 = fn_run(&fiber, 0x5566u, &out_arg2);
    obs_report_measure(id, "entry-resumed", "back", s_lifecycle_back1, "arg");
    obs_report_measure(id, "entry-resumed", "ret1-rc",
                       (uint64_t)(uint32_t)s_lifecycle_ret1_rc, "code");
    obs_report_measure(id, "sceFiberRun-2", "rc", (uint64_t)(uint32_t)rc_run2, "code");
    obs_report_measure(id, "sceFiberRun-2", "out-arg", out_arg2, "arg");

    int rc_fin = fn_finalize(&fiber);
    obs_report_measure(id, "sceFiberFinalize", "rc", (uint64_t)(uint32_t)rc_fin,
                       "code");

    obs_fault_unregister();
    if (s_lifecycle_runs < 2) {
        return obs_fail("fiber body did not complete both runs");
    }
    return obs_pass();
}

/* REQ-20260927T1415Z-5cd3 (2): Switch A <-> B */
static SceFiber s_fiber_a;
static SceFiber s_fiber_b;
static _Alignas(16) unsigned char s_stack_a[65536];
static _Alignas(16) unsigned char s_stack_b[65536];

static volatile uint64_t s_sw_a_back = 0;
static volatile int s_sw_a_rc = -1;
static volatile uint64_t s_sw_b_arg_run = 0;
static volatile int s_sw_b_self_rc = -1;
static volatile SceFiber *s_sw_b_self_ptr = NULL;
static volatile uint64_t s_sw_b_back = 0;
static volatile int s_sw_b_rc = -1;

static void fiber_switch_entry_b(uint64_t argOnInitialize, uint64_t argOnRun) {
    (void)argOnInitialize;
    s_sw_b_arg_run = argOnRun;

    fn_fiber_self_t fn_self = (fn_fiber_self_t)fiber_resolve_sym("sceFiberGetSelf");
    if (fn_self) {
        SceFiber *self = NULL;
        s_sw_b_self_rc = fn_self(&self);
        s_sw_b_self_ptr = self;
    }

    fn_fiber_switch_t fn_sw = (fn_fiber_switch_t)fiber_resolve_sym("sceFiberSwitch");
    if (fn_sw) {
        uint64_t b_back = 0;
        s_sw_b_rc = fn_sw(&s_fiber_a, 0xBAu, &b_back);
        s_sw_b_back = b_back;
    }

    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (fn_ret)
        fn_ret(0xbbbb, NULL);
}

static void fiber_switch_entry_a(uint64_t argOnInitialize, uint64_t argOnRun) {
    (void)argOnInitialize;
    (void)argOnRun;
    fn_fiber_switch_t fn_sw = (fn_fiber_switch_t)fiber_resolve_sym("sceFiberSwitch");
    if (fn_sw) {
        uint64_t a_back = 0;
        s_sw_a_rc = fn_sw(&s_fiber_b, 0xABu, &a_back);
        s_sw_a_back = a_back;
    }

    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (fn_ret) {
        fn_ret(0xaaaa, NULL);
    }
}

static obs_result check_fiber_switch(void) {
    const char *id = "033-fiber/switch";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_finalize_t fn_finalize =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    if (!fn_run || !fn_finalize)
        return obs_skip("fiber primitives missing");

    memset(&s_fiber_a, 0, sizeof(s_fiber_a));
    memset(&s_fiber_b, 0, sizeof(s_fiber_b));
    call_fiber_init(&s_fiber_a, "fiber_a", fiber_switch_entry_a, 1, s_stack_a,
                    sizeof(s_stack_a), NULL);
    call_fiber_init(&s_fiber_b, "fiber_b", fiber_switch_entry_b, 2, s_stack_b,
                    sizeof(s_stack_b), NULL);

    uint64_t out_arg = 0;
    int rc_run = fn_run(&s_fiber_a, 0x11u, &out_arg);
    obs_report_measure(id, "sceFiberRun-A", "rc", (uint64_t)(uint32_t)rc_run, "code");
    obs_report_measure(id, "fiber-B", "argOnRun", s_sw_b_arg_run, "arg");
    obs_report_measure(id, "fiber-B", "get-self-rc", (uint64_t)(uint32_t)s_sw_b_self_rc,
                       "code");
    obs_report_measure(id, "fiber-B", "self-matches",
                       (uint64_t)(s_sw_b_self_ptr == &s_fiber_b), "bool");
    obs_report_measure(id, "fiber-B", "switch-rc", (uint64_t)(uint32_t)s_sw_b_rc,
                       "code");
    obs_report_measure(id, "fiber-A", "switch-rc", (uint64_t)(uint32_t)s_sw_a_rc,
                       "code");
    obs_report_measure(id, "fiber-A", "a-back", s_sw_a_back, "arg");
    obs_report_measure(id, "sceFiberRun-A", "out-arg", out_arg, "arg");

    fn_finalize(&s_fiber_a);
    fn_finalize(&s_fiber_b);
    return obs_pass();
}

/* REQ-20260927T1415Z-5cd3 (3): Refusals */
static volatile int s_ref_run_inside_rc = -1;
static volatile int s_ref_sw_self_rc = -1;
static volatile int s_ref_fin_inside_rc = -1;

static void fiber_refusals_entry(uint64_t arg1, uint64_t arg2) {
    (void)arg1;
    (void)arg2;
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_switch_t fn_sw = (fn_fiber_switch_t)fiber_resolve_sym("sceFiberSwitch");
    fn_fiber_finalize_t fn_fin =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    fn_fiber_self_t fn_self = (fn_fiber_self_t)fiber_resolve_sym("sceFiberGetSelf");

    SceFiber *self = NULL;
    if (fn_self)
        fn_self(&self);

    if (fn_run && self) {
        uint64_t out = 0;
        s_ref_run_inside_rc = fn_run(self, 0, &out);
    }
    if (fn_sw && self) {
        uint64_t out = 0;
        s_ref_sw_self_rc = fn_sw(self, 0, &out);
    }
    if (fn_fin && self) {
        s_ref_fin_inside_rc = fn_fin(self);
    }

    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (fn_ret)
        fn_ret(0, NULL);
}

static void fiber_dummy_entry(uint64_t arg1, uint64_t arg2) {
    (void)arg1;
    (void)arg2;
    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (fn_ret)
        fn_ret(0, NULL);
}

static obs_result check_fiber_refusals(void) {
    const char *id = "033-fiber/refusals";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_switch_t fn_sw = (fn_fiber_switch_t)fiber_resolve_sym("sceFiberSwitch");
    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    fn_fiber_finalize_t fn_fin =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    fn_fiber_init_impl_t fn_impl =
        (fn_fiber_init_impl_t)fiber_resolve_sym("_sceFiberInitializeImpl");

    if (!fn_run || !fn_fin || !fn_ret || !fn_sw) {
        return obs_skip("fiber primitives missing");
    }

    uint64_t out = 0;
    /* 1. sceFiberRun(NULL, ...) */
    int rc_r_null = fn_run(NULL, 0, &out);
    obs_report_measure(id, "run-null", "rc", (uint64_t)(uint32_t)rc_r_null, "code");

    /* 2. sceFiberRun on uninitialised (zeroed) fiber */
    SceFiber zero_fib;
    memset(&zero_fib, 0, sizeof(zero_fib));
    int rc_r_zero = fn_run(&zero_fib, 0, &out);
    obs_report_measure(id, "run-uninit", "rc", (uint64_t)(uint32_t)rc_r_zero, "code");

    /* 3, 4, 8: Calls from inside running fiber */
    SceFiber inside_fib;
    memset(&inside_fib, 0, sizeof(inside_fib));
    call_fiber_init(&inside_fib, "ref_in", fiber_refusals_entry, 0, s_fiber_stack,
                    sizeof(s_fiber_stack), NULL);
    fn_run(&inside_fib, 0, &out);
    obs_report_measure(id, "run-from-inside", "rc",
                       (uint64_t)(uint32_t)s_ref_run_inside_rc, "code");
    obs_report_measure(id, "switch-to-self", "rc", (uint64_t)(uint32_t)s_ref_sw_self_rc,
                       "code");
    obs_report_measure(id, "finalize-from-inside", "rc",
                       (uint64_t)(uint32_t)s_ref_fin_inside_rc, "code");
    fn_fin(&inside_fib);

    /* 5. sceFiberSwitch from thread (not on a fiber) */
    SceFiber dummy_fib;
    memset(&dummy_fib, 0, sizeof(dummy_fib));
    call_fiber_init(&dummy_fib, "dummy", fiber_dummy_entry, 0, s_fiber_stack,
                    sizeof(s_fiber_stack), NULL);
    int rc_sw_th = fn_sw(&dummy_fib, 0, &out);
    obs_report_measure(id, "switch-from-thread", "rc", (uint64_t)(uint32_t)rc_sw_th,
                       "code");

    /* 6. sceFiberReturnToThread from thread */
    int rc_ret_th = fn_ret(0, &out);
    obs_report_measure(id, "return-from-thread", "rc", (uint64_t)(uint32_t)rc_ret_th,
                       "code");

    /* 7. sceFiberFinalize(NULL) */
    int rc_fin_null = fn_fin(NULL);
    obs_report_measure(id, "finalize-null", "rc", (uint64_t)(uint32_t)rc_fin_null,
                       "code");

    /* 9. sceFiberFinalize of initialized, never-run fiber */
    int rc_fin_never = fn_fin(&dummy_fib);
    obs_report_measure(id, "finalize-never-run", "rc", (uint64_t)(uint32_t)rc_fin_never,
                       "code");

    /* 10. _sceFiberInitializeImpl variations */
    if (fn_impl) {
        SceFiber f_var;
        int rc_null_ent = fn_impl(&f_var, "null_ent", NULL, 0, s_fiber_stack,
                                  sizeof(s_fiber_stack), NULL, 0x9000000ULL);
        obs_report_measure(id, "init-null-entry", "rc", (uint64_t)(uint32_t)rc_null_ent,
                           "code");

        int rc_sz64 = fn_impl(&f_var, "sz64", fiber_dummy_entry, 0, s_fiber_stack, 64,
                              NULL, 0x9000000ULL);
        obs_report_measure(id, "init-size-64", "rc", (uint64_t)(uint32_t)rc_sz64,
                           "code");

        int rc_sz1024 = fn_impl(&f_var, "sz1024", fiber_dummy_entry, 0, s_fiber_stack,
                                1024, NULL, 0x9000000ULL);
        obs_report_measure(id, "init-size-1024", "rc", (uint64_t)(uint32_t)rc_sz1024,
                           "code");

        int rc_misalign =
            fn_impl(&f_var, "misalign8", fiber_dummy_entry, 0, s_fiber_stack + 8,
                    sizeof(s_fiber_stack) - 16, NULL, 0x9000000ULL);
        obs_report_measure(id, "init-misaligned-8", "rc",
                           (uint64_t)(uint32_t)rc_misalign, "code");

        /* Second initialize of initialized, never-run fiber */
        SceFiber f_twice;
        fn_impl(&f_twice, "init1", fiber_dummy_entry, 0, s_fiber_stack,
                sizeof(s_fiber_stack), NULL, 0x9000000ULL);
        int rc_twice = fn_impl(&f_twice, "init2", fiber_dummy_entry, 0, s_fiber_stack,
                               sizeof(s_fiber_stack), NULL, 0x9000000ULL);
        obs_report_measure(id, "init-twice", "rc", (uint64_t)(uint32_t)rc_twice,
                           "code");
        fn_fin(&f_twice);
    }

    return obs_pass();
}

/* REQ-20260927T1415Z-5cd3 (4): The Object (256 bytes) */
static obs_result check_fiber_object_layout(void) {
    const char *id = "033-fiber/object-layout";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_finalize_t fn_fin =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    if (!fn_run || !fn_fin)
        return obs_skip("fiber primitives missing");

    SceFiber fib;
    memset(fib.storage, 0xcc, sizeof(fib.storage));

    int rc_init = call_fiber_init(&fib, "fib_obj", fiber_dummy_entry, 0x42,
                                  s_fiber_stack, sizeof(s_fiber_stack), NULL);
    obs_report_measure(id, "init", "rc", (uint64_t)(uint32_t)rc_init, "code");
    obs_report_bytes(id, "init", "obj-after-init-256b", 0, fib.storage, 256);

    uint64_t out = 0;
    int rc_run = fn_run(&fib, 0x123, &out);
    obs_report_measure(id, "run", "rc", (uint64_t)(uint32_t)rc_run, "code");
    obs_report_bytes(id, "run", "obj-after-run-256b", 0, fib.storage, 256);

    int rc_fin = fn_fin(&fib);
    obs_report_measure(id, "finalize", "rc", (uint64_t)(uint32_t)rc_fin, "code");
    obs_report_bytes(id, "finalize", "obj-after-finalize-256b", 0, fib.storage, 256);

    return obs_pass();
}

/* REQ-20260927T1415Z-5cd3 (6): Float state / MXCSR */
static volatile uint32_t s_fiber_entry_mxcsr = 0;

static void fiber_float_entry(uint64_t arg1, uint64_t arg2) {
    (void)arg1;
    (void)arg2;
#if defined(__x86_64__)
    s_fiber_entry_mxcsr = get_mxcsr();
#endif
    fn_fiber_return_t fn_ret =
        (fn_fiber_return_t)fiber_resolve_sym("sceFiberReturnToThread");
    if (fn_ret)
        fn_ret(0, NULL);
}

static obs_result check_fiber_float_state(void) {
    const char *id = "033-fiber/float-state";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_finalize_t fn_fin =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    if (!fn_run || !fn_fin)
        return obs_skip("fiber primitives missing");

    SceFiber fib;
#if defined(__x86_64__)
    set_mxcsr(0x7f80);
#endif
    call_fiber_init(&fib, "fib_flt", fiber_float_entry, 0, s_fiber_stack,
                    sizeof(s_fiber_stack), NULL);

#if defined(__x86_64__)
    set_mxcsr(0x1f80);
#endif
    uint64_t out = 0;
    fn_run(&fib, 0, &out);
    obs_report_measure(id, "entry-read", "mxcsr", (uint64_t)s_fiber_entry_mxcsr, "hex");

    fn_fin(&fib);
    return obs_pass();
}

/* REQ-20260927T1415Z-5cd3 (5): Entry returns without switching */
static void fiber_bare_return_entry(uint64_t arg1, uint64_t arg2) {
    (void)arg1;
    (void)arg2;
    /* Returns without calling sceFiberReturnToThread or sceFiberSwitch */
}

static obs_result check_fiber_entry_return(void) {
    const char *id = "033-fiber/entry-return";
    fn_fiber_run_t fn_run = (fn_fiber_run_t)fiber_resolve_sym("sceFiberRun");
    fn_fiber_finalize_t fn_fin =
        (fn_fiber_finalize_t)fiber_resolve_sym("sceFiberFinalize");
    if (!fn_run || !fn_fin)
        return obs_skip("fiber primitives missing");

    SceFiber fib;
    call_fiber_init(&fib, "fib_ret", fiber_bare_return_entry, 0, s_fiber_stack,
                    sizeof(s_fiber_stack), NULL);

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int rc = -1;
    if (sig == 0) {
        uint64_t out = 0;
        rc = fn_run(&fib, 0, &out);
        obs_fault_unregister();
        obs_report_measure(id, "bare-return", "survived", 1, "bool");
        obs_report_measure(id, "bare-return", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "bare-return", "out", out, "arg");
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "bare-return", "survived", 0, "bool");
        obs_report_measure(id, "bare-return", "fault-sig", (uint64_t)sig, "signal");
    }

    fn_fin(&fib);
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
    {"033-fiber/switch", "libSceFiber", "sceFiberSwitch", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_fiber_switch, OBS_FROM_ASSUMED},
    {"033-fiber/refusals", "libSceFiber", "sceFiberRun", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_fiber_refusals, OBS_FROM_ASSUMED},
    {"033-fiber/object-layout", "libSceFiber", "_sceFiberInitializeImpl", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_fiber_object_layout, OBS_FROM_ASSUMED},
    {"033-fiber/float-state", "libSceFiber", "sceFiberRun", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_fiber_float_state, OBS_FROM_ASSUMED},
    {"033-fiber/entry-return", "libSceFiber", "sceFiberRun", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_fiber_entry_return, OBS_FROM_ASSUMED},
};

const obs_section obs_section_fiber = {
    "033-fiber",
    "Fibers",
    "Cooperative multitasking, fiber stack creation, switching and termination.",
    fiber_checks,
    OBS_COUNT(fiber_checks),
};
