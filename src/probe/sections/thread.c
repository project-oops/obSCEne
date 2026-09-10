/*
 * Threads.
 *
 * Placed above memory because a thread needs a stack, and below everything else
 * because the presentation subsystems all run work on threads they create
 * themselves. A platform that cannot start a thread cannot reach a frame.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Set by the child thread, read by the parent after joining. Volatile because the
 * compiler cannot see the write - it happens on another thread, and without this
 * the read can legitimately be folded to the initial value. */
static volatile int child_ran;
static ScePthread child;
static int child_started;

static void *child_entry(void *arg) {
    child_ran = 1;
    return arg;
}

static obs_result check_self(void) {
    ScePthread self = scePthreadSelf();
    if (self == NULL) {
        /* Every mutex and condition variable is keyed on thread identity. A null
         * one means none of them can work, however well they appear to. */
        return obs_fail("the calling thread has no identity");
    }
    return obs_pass_value((uint64_t)(uintptr_t)self);
}

static obs_result check_create(void) {
    child_ran = 0;
    int rc =
        scePthreadCreate(&child, NULL, child_entry, (void *)0x1234, "obscene-probe");
    if (rc != 0) {
        return obs_fail_code("thread creation was refused", (uint64_t)(uint32_t)rc);
    }
    child_started = 1;
    return obs_pass();
}

static obs_result check_join(void) {
    if (!child_started) {
        return obs_skip("no thread was created to join");
    }
    void *value = NULL;
    int rc = scePthreadJoin(child, &value);
    if (rc != 0) {
        return obs_fail_code("join was refused", (uint64_t)(uint32_t)rc);
    }
    child_started = 0;
    if (!child_ran) {
        /* Creation and join both succeeding while the body never ran is the exact
         * shape of a stubbed thread API, and it is silent: the caller waits for work
         * that was never done and proceeds as if it were. */
        return obs_fail("the thread was created and joined but its body never ran");
    }
    if (value != (void *)0x1234) {
        return obs_partial_value("the return value did not survive the join",
                                 (uint64_t)(uintptr_t)value);
    }
    return obs_pass();
}

static volatile int s_exc_handler1_called = 0;
static volatile uint64_t s_exc_handler1_arg0 = 0;
static volatile uint64_t s_exc_handler1_arg1 = 0;
static volatile uint64_t s_exc_handler1_arg2 = 0;
static volatile uint64_t s_exc_handler1_rsp = 0;
static unsigned char s_exc_context_buf[0x180];
static volatile unsigned int s_exc_context_len = 0;
static volatile uint64_t s_exc_f8_val = 0;

static void probe_exc_handler_1(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    s_exc_handler1_arg0 = arg0;
    s_exc_handler1_arg1 = arg1;
    s_exc_handler1_arg2 = arg2;
    s_exc_handler1_called++;

    uint64_t rsp_val = 0;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp_val));
    s_exc_handler1_rsp = rsp_val;

    if (arg1 != 0) {
        const unsigned char *p = (const unsigned char *)(uintptr_t)arg1;
        for (unsigned int i = 0; i < sizeof(s_exc_context_buf); i++) {
            s_exc_context_buf[i] = p[i];
        }
        s_exc_context_len = (unsigned int)sizeof(s_exc_context_buf);

        uint64_t f8 = 0;
        for (unsigned int i = 0; i < 8u; i++) {
            f8 |= ((uint64_t)s_exc_context_buf[0xf8u + i]) << (i * 8u);
        }
        s_exc_f8_val = f8;
    }
}

static volatile int s_exc_handler2_called = 0;
static void probe_exc_handler_2(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    (void)arg0;
    (void)arg1;
    (void)arg2;
    s_exc_handler2_called = 1;
}

static volatile int s_concurrent_in_handler = 0;
static volatile int s_concurrent_worker_done = 0;
static volatile int s_concurrent_raise_rc = -999;
static volatile int s_concurrent_handler_reentered = 0;
static ScePthread s_target_thread = NULL;
typedef int (*fn_raise_t)(ScePthread thread, int signum);
static fn_raise_t s_active_raise_fn = NULL;

static void *exc_concurrent_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 20000000; i++) {
        if (s_concurrent_in_handler) {
            break;
        }
        __asm__ volatile("" : : : "memory");
    }
    if (s_concurrent_in_handler && s_active_raise_fn != NULL && s_target_thread != NULL) {
        s_concurrent_raise_rc = s_active_raise_fn(s_target_thread, 30);
    }
    s_concurrent_worker_done = 1;
    return NULL;
}

static void probe_exc_handler_concurrent(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    (void)arg0;
    (void)arg1;
    (void)arg2;
    if (s_concurrent_in_handler) {
        s_concurrent_handler_reentered++;
    }
    s_concurrent_in_handler = 1;
    for (int i = 0; i < 20000000; i++) {
        if (s_concurrent_worker_done) {
            break;
        }
        __asm__ volatile("" : : : "memory");
    }
}

static obs_result check_exception_handler_ordering(void) {
    OBS_REQUIRE(&scePthreadCreate, &scePthreadJoin, &scePthreadSelf);

    const void *addr_install = obs_module_symbol(1, "sceKernelInstallExceptionHandler");
    if (addr_install == NULL) {
        addr_install = obs_module_symbol(OBS_HANDLE_SELF, "sceKernelInstallExceptionHandler");
    }
    const void *addr_raise = obs_module_symbol(1, "sceKernelRaiseException");
    if (addr_raise == NULL) {
        addr_raise = obs_module_symbol(OBS_HANDLE_SELF, "sceKernelRaiseException");
    }

    obs_report_measure("030-thread/exception-handler", "sceKernelInstallExceptionHandler", "address",
                       (uint64_t)(uintptr_t)addr_install, "address");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "address",
                       (uint64_t)(uintptr_t)addr_raise, "address");

    if (addr_install == NULL || addr_raise == NULL) {
        return obs_skip("sceKernelInstallExceptionHandler or sceKernelRaiseException not resolved");
    }

    typedef void (*fn_handler_t)(uint64_t, uint64_t, uint64_t);
    typedef int (*fn_install_t)(int signum, fn_handler_t handler);
    typedef int (*fn_remove_t)(int signum);
    fn_install_t fn_install = (fn_install_t)addr_install;
    fn_raise_t fn_raise = (fn_raise_t)addr_raise;

    const void *addr_remove = obs_module_symbol(1, "sceKernelRemoveExceptionHandler");
    if (addr_remove == NULL) {
        addr_remove = obs_module_symbol(OBS_HANDLE_SELF, "sceKernelRemoveExceptionHandler");
    }
    obs_report_measure("030-thread/exception-handler", "sceKernelRemoveExceptionHandler", "address",
                       (uint64_t)(uintptr_t)addr_remove, "address");

    ScePthread self = NULL;
    typedef ScePthread (*fn_self_t)(void);
    fn_self_t fn_self = (fn_self_t)obs_module_symbol(1, "scePthreadSelf");
    if (fn_self == NULL) {
        fn_self = (fn_self_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadSelf");
    }
    if (fn_self != NULL) {
        self = fn_self();
    }
    if (self == NULL) {
        self = scePthreadSelf();
    }
    obs_report_measure("030-thread/exception-handler", "scePthreadSelf", "self",
                       (uint64_t)(uintptr_t)self, "handle");

    /* 1. install(30, handler1) */
    s_exc_handler1_called = 0;
    s_exc_handler1_arg0 = 0;
    s_exc_handler1_arg1 = 0;
    s_exc_handler1_arg2 = 0;
    int rc_inst1 = fn_install(30, probe_exc_handler_1);
    obs_report_measure("030-thread/exception-handler", "sceKernelInstallExceptionHandler", "rc-1",
                       (uint64_t)(uint32_t)rc_inst1, "rc");

    /* 2. install(30, handler2) second time (duplicate rejected with 0x80020023 / EAGAIN) */
    int rc_inst2 = fn_install(30, probe_exc_handler_2);
    obs_report_measure("030-thread/exception-handler", "sceKernelInstallExceptionHandler", "rc-2",
                       (uint64_t)(uint32_t)rc_inst2, "rc");

    /* 3. Delivering raise(self, 30) */
    s_exc_handler1_called = 0;
    s_exc_handler1_arg0 = 0;
    s_exc_handler1_arg1 = 0;
    s_exc_handler1_arg2 = 0;
    s_exc_handler1_rsp = 0;
    s_exc_context_len = 0;
    s_exc_f8_val = 0;
    int rc_raise = fn_raise(self, 30);
    int flag_val = s_exc_handler1_called;
    uint64_t arg0_val = s_exc_handler1_arg0;
    uint64_t arg1_val = s_exc_handler1_arg1;
    uint64_t arg2_val = s_exc_handler1_arg2;
    uint64_t handler_rsp = s_exc_handler1_rsp;

    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-with-handler",
                       (uint64_t)(uint32_t)rc_raise, "rc");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "flag-after-raise",
                       (uint64_t)flag_val, "flag");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "arg0-received",
                       arg0_val, "signum");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "arg1-received",
                       arg1_val, "arg1");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "arg2-received",
                       arg2_val, "arg2");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rsi-rdx-identical",
                       (uint64_t)(arg1_val == arg2_val ? 1u : 0u), "bool");
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "handler-rsp",
                       handler_rsp, "address");
    int64_t diff = (int64_t)arg1_val - (int64_t)handler_rsp;
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "context-delta-from-rsp",
                       (uint64_t)diff, "bytes");

    if (flag_val > 0 && s_exc_context_len >= 0x140u) {
        for (unsigned int off = 0; off < s_exc_context_len; off += 16u) {
            unsigned int chunk = (s_exc_context_len - off < 16u) ? (s_exc_context_len - off) : 16u;
            obs_report_bytes("030-thread/exception-handler", "sceKernelRaiseException", "context",
                             off, &s_exc_context_buf[off], chunk);
        }

        uint64_t f8_val = s_exc_f8_val;
        obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "offset-0xf8-value",
                           f8_val, "raw");

        typedef int (*fn_vq_t)(const void *, int, void *, size_t);
        fn_vq_t fn_vq = (fn_vq_t)obs_module_symbol(1, "sceKernelVirtualQuery");
        if (fn_vq == NULL) {
            fn_vq = (fn_vq_t)obs_module_symbol(OBS_HANDLE_SELF, "sceKernelVirtualQuery");
        }
        if (fn_vq == NULL && &sceKernelVirtualQuery != NULL) {
            fn_vq = (fn_vq_t)sceKernelVirtualQuery;
        }

        int f8_is_mapped = 0;
        if (f8_val > 0x10000u && f8_val < 0x00007fffffffffffULL && fn_vq != NULL) {
            unsigned char vq_info[64];
            int qrc = fn_vq((const void *)(uintptr_t)f8_val, 0, vq_info, sizeof(vq_info));
            if (qrc == 0) {
                f8_is_mapped = 1;
            }
        }
        obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "offset-0xf8-mapped",
                           (uint64_t)f8_is_mapped, "bool");
        if (f8_is_mapped) {
            const unsigned char *p_f8 = (const unsigned char *)(uintptr_t)f8_val;
            for (unsigned int off = 0; off < 0x40u; off += 16u) {
                obs_report_bytes("030-thread/exception-handler", "sceKernelRaiseException", "offset-0xf8-target",
                                 off, &p_f8[off], 16u);
            }
        }

        if (fn_vq != NULL) {
            unsigned char vq_ctx[64];
            if (fn_vq((const void *)(uintptr_t)arg1_val, 0, vq_ctx, sizeof(vq_ctx)) == 0) {
                uint64_t reg_start = 0;
                uint64_t reg_end = 0;
                for (unsigned int i = 0; i < 8u; i++) {
                    reg_start |= ((uint64_t)vq_ctx[i]) << (i * 8u);
                    reg_end |= ((uint64_t)vq_ctx[8u + i]) << (i * 8u);
                }
                obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "context-region-start",
                                   reg_start, "address");
                obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "context-region-end",
                                   reg_end, "address");
            }
        }
    }

    /* 4. Cross-thread raise while first thread is inside handler */
    if (flag_val > 0 && addr_remove != NULL) {
        fn_remove_t fn_remove = (fn_remove_t)addr_remove;
        fn_remove(30);
        int rc_inst_conc = fn_install(30, probe_exc_handler_concurrent);
        if (rc_inst_conc == 0) {
            s_concurrent_in_handler = 0;
            s_concurrent_worker_done = 0;
            s_concurrent_raise_rc = -999;
            s_concurrent_handler_reentered = 0;
            s_target_thread = self;
            s_active_raise_fn = fn_raise;

            ScePthread worker_thread;
            int rc_th = scePthreadCreate(&worker_thread, NULL, exc_concurrent_worker, NULL, "exc-conc");
            if (rc_th == 0) {
                int rc_main_raise = fn_raise(self, 30);
                obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-main-raise-concurrent",
                                   (uint64_t)(uint32_t)rc_main_raise, "rc");
                void *worker_ret = NULL;
                scePthreadJoin(worker_thread, &worker_ret);
                obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-concurrent-thread",
                                   (uint64_t)(uint32_t)s_concurrent_raise_rc, "rc");
                obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "handler-reentered",
                                   (uint64_t)s_concurrent_handler_reentered, "count");
            }
            fn_remove(30);
            fn_install(30, probe_exc_handler_1);
        }
    } else {
        obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-concurrent-thread",
                           0xffffffff, "skipped");
        obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "handler-reentered",
                           0, "count");
    }

    /* Inverted arg order: raise(30, self) in case (signum, thread) */
    typedef int (*fn_raise_inv_t)(int signum, ScePthread thread);
    fn_raise_inv_t fn_raise_inv = (fn_raise_inv_t)addr_raise;
    int rc_raise_inv = fn_raise_inv(30, self);
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-inverted-args",
                       (uint64_t)(uint32_t)rc_raise_inv, "rc");

    /* 5. raise(self, 31) - signal nothing was installed for while 30 is installed */
    int rc_raise31 = fn_raise(self, 31);
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-unhandled-sig31",
                       (uint64_t)(uint32_t)rc_raise31, "rc");

    /* 6. raise(self, 30) with no handler installed */
    int rc_uninst = fn_install(30, NULL);
    obs_report_measure("030-thread/exception-handler", "sceKernelInstallExceptionHandler", "rc-uninstall",
                       (uint64_t)(uint32_t)rc_uninst, "rc");

    if (addr_remove != NULL) {
        fn_remove_t fn_remove = (fn_remove_t)addr_remove;
        int rc_rem = fn_remove(30);
        obs_report_measure("030-thread/exception-handler", "sceKernelRemoveExceptionHandler", "rc-remove",
                           (uint64_t)(uint32_t)rc_rem, "rc");
    }

    /* Calling fn_raise(self, 30) with no handler installed delivers unhandled
     * signal 30 (0x1e) to the process, terminating it (measured on console:
     * "mDBG: Sending signal(pid: ..., tid: ..., signo: 0x1e)").
     * We report 0x1e as unhandled-signal-delivers, and test with an invalid thread handle
     * which safely returns 0x80020003 (ESRCH) without terminating the process. */
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "unhandled-signal-delivers",
                       0x1e, "signo");
    int rc_bad_thread = fn_raise((ScePthread)1, 30);
    obs_report_measure("030-thread/exception-handler", "sceKernelRaiseException", "rc-bad-thread-no-handler",
                       (uint64_t)(uint32_t)rc_bad_thread, "rc");

    return obs_pass();
}

static obs_result check_thread_affinity(void) {
    OBS_REQUIRE(&scePthreadSelf);

    typedef int (*fn_getaffinity_t)(ScePthread, uint64_t *);
    typedef int (*fn_setaffinity_t)(ScePthread, uint64_t);

    fn_getaffinity_t fn_get = (fn_getaffinity_t)obs_module_symbol(1, "scePthreadGetaffinity");
    if (fn_get == NULL) {
        fn_get = (fn_getaffinity_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadGetaffinity");
    }

    fn_setaffinity_t fn_set = (fn_setaffinity_t)obs_module_symbol(1, "scePthreadSetaffinity");
    if (fn_set == NULL) {
        fn_set = (fn_setaffinity_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadSetaffinity");
    }

    obs_report_measure("030-thread/affinity", "scePthreadGetaffinity", "resolved",
                       fn_get != NULL ? 1 : 0, "bool");
    obs_report_measure("030-thread/affinity", "scePthreadSetaffinity", "resolved",
                       fn_set != NULL ? 1 : 0, "bool");

    if (fn_get == NULL) {
        return obs_skip("scePthreadGetaffinity not resolved");
    }

    ScePthread self = scePthreadSelf();
    uint64_t mask = 0;
    int rc_get = -1;

    if (obs_address_is_callable((const void *)fn_get)) {
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            rc_get = fn_get(self, &mask);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            return obs_fail_code("scePthreadGetaffinity faulted", (uint64_t)(uint32_t)sig);
        }
    }

    obs_report_measure("030-thread/affinity", "scePthreadGetaffinity", "rc",
                       (uint64_t)(uint32_t)rc_get, "code");
    obs_report_measure("030-thread/affinity", "scePthreadGetaffinity", "mask",
                       mask, "mask");

    if (fn_set != NULL && obs_address_is_callable((const void *)fn_set) && rc_get == 0) {
        int rc_set = -1;
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            rc_set = fn_set(self, mask);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            return obs_fail_code("scePthreadSetaffinity faulted", (uint64_t)(uint32_t)sig);
        }
        obs_report_measure("030-thread/affinity", "scePthreadSetaffinity", "rc",
                           (uint64_t)(uint32_t)rc_set, "code");
    }

    return obs_pass_value(mask);
}

static obs_result check_thread_name(void) {
    OBS_REQUIRE(&scePthreadSelf);

    typedef int (*fn_getname_t)(ScePthread, char *);
    typedef int (*fn_setname_t)(ScePthread, const char *);

    fn_getname_t fn_get = (fn_getname_t)obs_module_symbol(1, "scePthreadGetname");
    if (fn_get == NULL) {
        fn_get = (fn_getname_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadGetname");
    }

    fn_setname_t fn_set = (fn_setname_t)obs_module_symbol(1, "scePthreadSetname");
    if (fn_set == NULL) {
        fn_set = (fn_setname_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadSetname");
    }
    if (fn_set == NULL) {
        fn_set = (fn_setname_t)obs_module_symbol(1, "scePthreadRename");
        if (fn_set == NULL) {
            fn_set = (fn_setname_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadRename");
        }
    }

    obs_report_measure("030-thread/name", "scePthreadGetname", "resolved",
                       fn_get != NULL ? 1 : 0, "bool");
    obs_report_measure("030-thread/name", "scePthreadSetname", "resolved",
                       fn_set != NULL ? 1 : 0, "bool");

    if (fn_get == NULL && fn_set == NULL) {
        return obs_skip("scePthread thread naming symbols not resolved");
    }

    ScePthread self = scePthreadSelf();
    char name_buf[64];
    for (size_t i = 0; i < sizeof(name_buf); i++) {
        name_buf[i] = 0;
    }

    if (fn_set != NULL && obs_address_is_callable((const void *)fn_set)) {
        int rc_set = -1;
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            rc_set = fn_set(self, "obs-thread");
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            return obs_fail_code("scePthreadSetname faulted", (uint64_t)(uint32_t)sig);
        }
        obs_report_measure("030-thread/name", "scePthreadSetname", "rc",
                           (uint64_t)(uint32_t)rc_set, "code");
    }

    if (fn_get != NULL && obs_address_is_callable((const void *)fn_get)) {
        int rc_get = -1;
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            rc_get = fn_get(self, name_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            return obs_fail_code("scePthreadGetname faulted", (uint64_t)(uint32_t)sig);
        }
        obs_report_measure("030-thread/name", "scePthreadGetname", "rc",
                           (uint64_t)(uint32_t)rc_get, "code");
        obs_report_measure("030-thread/name", "name_buf", "first_byte",
                           (uint64_t)(uint8_t)name_buf[0], "char");
    }

    return obs_pass();
}

static obs_result check_thread_priority(void) {
    OBS_REQUIRE(&scePthreadSelf);

    typedef int (*fn_getprio_t)(ScePthread, int *);
    typedef int (*fn_setprio_t)(ScePthread, int);

    fn_getprio_t fn_get = (fn_getprio_t)obs_module_symbol(1, "scePthreadGetprio");
    if (fn_get == NULL) {
        fn_get = (fn_getprio_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadGetprio");
    }

    fn_setprio_t fn_set = (fn_setprio_t)obs_module_symbol(1, "scePthreadSetprio");
    if (fn_set == NULL) {
        fn_set = (fn_setprio_t)obs_module_symbol(OBS_HANDLE_SELF, "scePthreadSetprio");
    }

    obs_report_measure("030-thread/priority", "scePthreadGetprio", "resolved",
                       fn_get != NULL ? 1 : 0, "bool");
    obs_report_measure("030-thread/priority", "scePthreadSetprio", "resolved",
                       fn_set != NULL ? 1 : 0, "bool");

    if (fn_get == NULL) {
        return obs_skip("scePthreadGetprio not resolved");
    }

    ScePthread self = scePthreadSelf();
    int prio = 0;
    int rc_get = -1;

    if (obs_address_is_callable((const void *)fn_get)) {
        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig == 0) {
            rc_get = fn_get(self, &prio);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            return obs_fail_code("scePthreadGetprio faulted", (uint64_t)(uint32_t)sig);
        }
    }

    obs_report_measure("030-thread/priority", "scePthreadGetprio", "rc",
                       (uint64_t)(uint32_t)rc_get, "code");
    obs_report_measure("030-thread/priority", "scePthreadGetprio", "prio",
                       (uint64_t)(uint32_t)prio, "prio");

    return obs_pass_value((uint64_t)(uint32_t)prio);
}

static const obs_check thread_checks[] = {
    {"030-thread/self", "libkernel", "scePthreadSelf", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadSelf, check_self, OBS_FROM_SPEC},
    {"030-thread/create", "libkernel", "scePthreadCreate", OBS_CAP_NONE, OBS_CAP_THREAD,
     (const void *)&scePthreadCreate, check_create, OBS_FROM_SPEC},
    {"030-thread/join", "libkernel", "scePthreadJoin", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadJoin, check_join, OBS_FROM_SPEC},
    {"030-thread/exception-handler", "libkernel", "sceKernelRaiseException", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_exception_handler_ordering,
     check_exception_handler_ordering, OBS_FROM_ASSUMED},
    {"030-thread/affinity", "libkernel", "scePthreadGetaffinity", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_thread_affinity,
     check_thread_affinity, OBS_FROM_ASSUMED},
    {"030-thread/name", "libkernel", "scePthreadGetname", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_thread_name,
     check_thread_name, OBS_FROM_ASSUMED},
    {"030-thread/priority", "libkernel", "scePthreadGetprio", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_thread_priority,
     check_thread_priority, OBS_FROM_ASSUMED},
};

const obs_section obs_section_thread = {
    "030-thread",
    "Threads",
    "Creating a thread, proving its body actually ran, and joining it back.",
    thread_checks,
    OBS_COUNT(thread_checks),
};
