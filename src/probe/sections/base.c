/*
 * Base layer: the report stream, then the process it runs in.
 *
 * Nothing above this section means anything if these fail. If the number formatter
 * is wrong, every value in the report is wrong; if the write path is wrong, there is
 * no report at all.
 */

#include "obscene/display.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

/* ---- 000-boot -------------------------------------------------------------- */

/* Compares against a literal without libc. */
static int same_text(const char *a, const char *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static obs_result check_formatting(void) {
    /* Self-check before trusting any number this program prints. A formatter that
     * reverses its digits produces a report that looks entirely plausible and is
     * quietly useless, and no platform failure would ever reveal it. */
    char buf[OBS_NUM_MAX];
    size_t n = obs_format_u64(buf, 1234567890u);
    if (n != 10 || !same_text(buf, "1234567890", 10)) {
        return obs_fail("decimal formatting is wrong");
    }
    n = obs_format_hex(buf, 0xdeadbeefu);
    if (n != 10 || !same_text(buf, "0xdeadbeef", 10)) {
        return obs_fail("hexadecimal formatting is wrong");
    }
    n = obs_format_i64(buf, -42);
    if (n != 3 || !same_text(buf, "-42", 3)) {
        return obs_fail("signed formatting is wrong");
    }
    /* The most negative value has no positive counterpart; negating it in signed
     * arithmetic is undefined rather than merely wrong, so it is worth pinning. */
    n = obs_format_i64(buf, (-9223372036854775807LL - 1));
    if (n != 20 || !same_text(buf, "-9223372036854775808", 20)) {
        return obs_fail("the most negative value does not format");
    }
    return obs_pass();
}

static obs_result check_write_returns_count(void) {
    /* A write must report how many bytes it took. Returning zero, or the buffer
     * length regardless of what happened, are both common stub shortcuts and both
     * produce a report stream that silently truncates under load. */
    static const char probe[] = "";
    sce_ssize_t n = sceKernelWrite(OBS_FD_STDOUT, probe, 0);
    if (n < 0) {
        return obs_fail_code("a zero-length write reported an error", (uint64_t)n);
    }
    if (n != 0) {
        return obs_partial_value("a zero-length write claimed to write bytes",
                                 (uint64_t)n);
    }
    return obs_pass();
}

/* REQ-20261010T1320Z-ao03: sceAudioOut2Initialize as the first AudioOut2 call in a
 * fresh process */
static obs_result check_audioout2_fresh_process(void) {
    const char *id = "000-boot/audioout2-fresh-process";
    obs_jmp_buf guard;

    /* (1) PPSA04263 order:
     * sceSysmoduleLoadModule(0x96), (0x95), sceSysmoduleIsLoaded(0xb4) */
    int rc_96 = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_96 = sceSysmoduleLoadModule(0x0096);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceSysmoduleLoadModule", "rc-0x96",
                       (uint64_t)(uint32_t)rc_96, "code");

    int rc_95 = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_95 = sceSysmoduleLoadModule(0x0095);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceSysmoduleLoadModule", "rc-0x95",
                       (uint64_t)(uint32_t)rc_95, "code");

    int rc_b4 = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleIsLoaded)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_b4 = sceSysmoduleIsLoaded(0x00b4);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceSysmoduleIsLoaded", "rc-0xb4", (uint64_t)(uint32_t)rc_b4,
                       "code");

    /* Resolve sceAudioOut2Initialize */
    const void *fn_init = (const void *)&sceAudioOut2Initialize_static;
    if (!obs_address_is_callable(fn_init)) {
        fn_init = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2Initialize");
    }
    if (!obs_address_is_callable(fn_init)) {
        fn_init = obs_module_symbol(OBS_HANDLE_SELF, "$g2tViFIohHE");
    }
    if (!obs_address_is_callable(fn_init)) {
        fn_init = obs_module_symbol(1, "sceAudioOut2Initialize");
    }
    if (!obs_address_is_callable(fn_init)) {
        fn_init = obs_module_symbol(1, "$g2tViFIohHE");
    }
    if (!obs_address_is_callable(fn_init)) {
        int h_ao = obs_module_open("libSceAudioOut2");
        if (h_ao > 0) {
            fn_init = obs_module_symbol(h_ao, "sceAudioOut2Initialize");
            if (!fn_init) {
                fn_init = obs_module_symbol(h_ao, "$g2tViFIohHE");
            }
        }
    }

    obs_report_measure(id, "sceAudioOut2Initialize", "resolved",
                       (uint64_t)(fn_init && obs_address_is_callable(fn_init)), "bool");

    /* (2) sceAudioOut2Initialize() with every argument register zero */
    int rc_init1 = -1;
    if (fn_init && obs_address_is_callable(fn_init)) {
        typedef int (*fn_zero_args_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t);
        fn_zero_args_t fn_call_zero = (fn_zero_args_t)fn_init;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init1 = fn_call_zero(0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceAudioOut2Initialize", "fault-first",
                               (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "sceAudioOut2Initialize", "rc-first-zero-regs",
                       (uint64_t)(uint32_t)rc_init1, "code");

    /* (3) a second Initialize, rc */
    int rc_init2 = -1;
    if (fn_init && obs_address_is_callable(fn_init)) {
        typedef int (*fn_zero_args_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t);
        fn_zero_args_t fn_call_zero = (fn_zero_args_t)fn_init;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init2 = fn_call_zero(0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceAudioOut2Initialize", "fault-second",
                               (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "sceAudioOut2Initialize", "rc-second",
                       (uint64_t)(uint32_t)rc_init2, "code");

    /* (4) sceAudioOut2UserCreate(initial user, &out) rc */
    int32_t user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0) {
            user = -1;
        }
    }
    if (user < 0) {
        user = 0x1ea2f4d9;
    }

    const void *fn_create = (const void *)&sceAudioOut2UserCreate_static;
    if (!obs_address_is_callable(fn_create)) {
        fn_create = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2UserCreate");
    }
    if (!obs_address_is_callable(fn_create)) {
        fn_create = obs_module_symbol(OBS_HANDLE_SELF, "$xywYcRB7nbQ");
    }
    if (!obs_address_is_callable(fn_create)) {
        fn_create = obs_module_symbol(1, "sceAudioOut2UserCreate");
    }
    if (!obs_address_is_callable(fn_create)) {
        fn_create = obs_module_symbol(1, "$xywYcRB7nbQ");
    }
    if (!obs_address_is_callable(fn_create)) {
        int h_ao = obs_module_open("libSceAudioOut2");
        if (h_ao > 0) {
            fn_create = obs_module_symbol(h_ao, "sceAudioOut2UserCreate");
            if (!fn_create) {
                fn_create = obs_module_symbol(h_ao, "$xywYcRB7nbQ");
            }
        }
    }

    obs_report_measure(id, "sceAudioOut2UserCreate", "resolved",
                       (uint64_t)(fn_create && obs_address_is_callable(fn_create)),
                       "bool");

    void *user_handle = (void *)0xccccccccccccccccULL;
    int rc_create = -1;
    if (fn_create && obs_address_is_callable(fn_create)) {
        typedef int (*user_create_fn)(int32_t, void **);
        user_create_fn f_create = (user_create_fn)fn_create;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_create = f_create(user, &user_handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceAudioOut2UserCreate", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "sceAudioOut2UserCreate", "rc",
                       (uint64_t)(uint32_t)rc_create, "code");
    obs_report_measure(id, "sceAudioOut2UserCreate", "handle",
                       (uint64_t)(uintptr_t)user_handle, "addr");

    if (user_handle != NULL && (uintptr_t)user_handle >= 0x10000UL &&
        (uintptr_t)user_handle != 0xccccccccccccccccULL) {
        uint8_t uobj[64];
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            const uint8_t *src = (const uint8_t *)user_handle;
            for (size_t i = 0; i < sizeof(uobj); i++) {
                uobj[i] = src[i];
            }
            obs_fault_unregister();
            obs_report_bytes(id, "sceAudioOut2UserCreate", "handle-obj-40b", 0, uobj,
                             sizeof(uobj));
        } else {
            obs_fault_unregister();
        }

        const void *fn_destroy = (const void *)&sceAudioOut2UserDestroy_static;
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = (const void *)&sceAudioOut2UserDestroy_alt;
        }
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2UserDestroy");
        }
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = obs_module_symbol(OBS_HANDLE_SELF, "$IaZXJ9M79uo");
        }
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = obs_module_symbol(1, "sceAudioOut2UserDestroy");
        }
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = obs_module_symbol(1, "$IaZXJ9M79uo");
        }
        if (fn_destroy && obs_address_is_callable(fn_destroy)) {
            int rc_dest = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_dest = ((int (*)(void *))fn_destroy)(user_handle);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "sceAudioOut2UserDestroy", "rc",
                               (uint64_t)(uint32_t)rc_dest, "code");
        }
    }

    /* REQ-20261010T1430Z-ao04: PPSA04263 context and user set-up in order */
    int h_ao = obs_module_open("libSceAudioOut2");
    int sig = 0;

    const void *fn_ctx_reset = (const void *)&sceAudioOut2ContextResetParam_static;
    if (!obs_address_is_callable(fn_ctx_reset)) {
        fn_ctx_reset =
            obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextResetParam");
    }
    if (!obs_address_is_callable(fn_ctx_reset)) {
        fn_ctx_reset = obs_module_symbol(OBS_HANDLE_SELF, "$t5YrizufpQc");
    }
    if (!obs_address_is_callable(fn_ctx_reset)) {
        fn_ctx_reset = obs_module_symbol(1, "sceAudioOut2ContextResetParam");
    }
    if (!obs_address_is_callable(fn_ctx_reset)) {
        fn_ctx_reset = obs_module_symbol(1, "$t5YrizufpQc");
    }
    if (!obs_address_is_callable(fn_ctx_reset) && h_ao > 0) {
        fn_ctx_reset = obs_module_symbol(h_ao, "sceAudioOut2ContextResetParam");
        if (!fn_ctx_reset) {
            fn_ctx_reset = obs_module_symbol(h_ao, "$t5YrizufpQc");
        }
    }

    const void *fn_ctx_query = (const void *)&sceAudioOut2ContextQueryMemory_static;
    if (!obs_address_is_callable(fn_ctx_query)) {
        fn_ctx_query =
            obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextQueryMemory");
    }
    if (!obs_address_is_callable(fn_ctx_query)) {
        fn_ctx_query = obs_module_symbol(OBS_HANDLE_SELF, "$pDmme7Bgm6E");
    }
    if (!obs_address_is_callable(fn_ctx_query)) {
        fn_ctx_query = obs_module_symbol(1, "sceAudioOut2ContextQueryMemory");
    }
    if (!obs_address_is_callable(fn_ctx_query)) {
        fn_ctx_query = obs_module_symbol(1, "$pDmme7Bgm6E");
    }
    if (!obs_address_is_callable(fn_ctx_query) && h_ao > 0) {
        fn_ctx_query = obs_module_symbol(h_ao, "sceAudioOut2ContextQueryMemory");
        if (!fn_ctx_query) {
            fn_ctx_query = obs_module_symbol(h_ao, "$pDmme7Bgm6E");
        }
    }

    const void *fn_ctx_create = (const void *)&sceAudioOut2ContextCreate_static;
    if (!obs_address_is_callable(fn_ctx_create)) {
        fn_ctx_create = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextCreate");
    }
    if (!obs_address_is_callable(fn_ctx_create)) {
        fn_ctx_create = obs_module_symbol(OBS_HANDLE_SELF, "$0x6o1VVAYSY");
    }
    if (!obs_address_is_callable(fn_ctx_create)) {
        fn_ctx_create = obs_module_symbol(1, "sceAudioOut2ContextCreate");
    }
    if (!obs_address_is_callable(fn_ctx_create)) {
        fn_ctx_create = obs_module_symbol(1, "$0x6o1VVAYSY");
    }
    if (!obs_address_is_callable(fn_ctx_create) && h_ao > 0) {
        fn_ctx_create = obs_module_symbol(h_ao, "sceAudioOut2ContextCreate");
        if (!fn_ctx_create) {
            fn_ctx_create = obs_module_symbol(h_ao, "$0x6o1VVAYSY");
        }
    }

    obs_report_measure(
        id, "sceAudioOut2ContextResetParam", "resolved",
        (uint64_t)(fn_ctx_reset && obs_address_is_callable(fn_ctx_reset)), "bool");
    obs_report_measure(
        id, "sceAudioOut2ContextQueryMemory", "resolved",
        (uint64_t)(fn_ctx_query && obs_address_is_callable(fn_ctx_query)), "bool");
    obs_report_measure(
        id, "sceAudioOut2ContextCreate", "resolved",
        (uint64_t)(fn_ctx_create && obs_address_is_callable(fn_ctx_create)), "bool");

    /* (1) ContextResetParam on a 0x40-byte block filled 0xa5, rc and the block after */
    uint8_t ctx_param[64];
    for (size_t i = 0; i < sizeof(ctx_param); i++) {
        ctx_param[i] = 0xa5;
    }
    int rc_reset = -1;
    if (fn_ctx_reset && obs_address_is_callable(fn_ctx_reset)) {
        typedef int (*fn_ctx_reset_t)(void *);
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reset = ((fn_ctx_reset_t)fn_ctx_reset)(ctx_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextResetParam", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "ContextResetParam", "rc", (uint64_t)(uint32_t)rc_reset,
                       "code");
    obs_report_bytes(id, "ContextResetParam", "param-40h", 0, ctx_param,
                     sizeof(ctx_param));

    /* (2) The title's values written over the first 0x18 bytes:
     * u32 4, 0x48, 0, 2 at +0x00 and u64 0x100000100 at +0x10.
     * Then ContextQueryMemory(param, &size) with size filled 0xa5, rc and size. */
    *(uint32_t *)(void *)(ctx_param + 0x00) = 4u;
    *(uint32_t *)(void *)(ctx_param + 0x04) = 0x48u;
    *(uint32_t *)(void *)(ctx_param + 0x08) = 0u;
    *(uint32_t *)(void *)(ctx_param + 0x0c) = 2u;
    *(uint64_t *)(void *)(ctx_param + 0x10) = 0x100000100ULL;

    uint64_t mem_size = 0xa5a5a5a5a5a5a5a5ULL;
    int rc_query = -1;
    if (fn_ctx_query && obs_address_is_callable(fn_ctx_query)) {
        typedef int (*fn_ctx_query_t)(void *, uint64_t *);
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_query = ((fn_ctx_query_t)fn_ctx_query)(ctx_param, &mem_size);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextQueryMemory", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "ContextQueryMemory", "rc", (uint64_t)(uint32_t)rc_query,
                       "code");
    obs_report_measure(id, "ContextQueryMemory", "size", mem_size, "bytes");

    /* (3) ContextCreate(param, mem, size, &context) with mem 16-aligned and size bytes,
     * context filled 0xa5, rc and the 8 bytes of context after. */
    static _Alignas(16) uint8_t s_ao_ctx_mem[0x200000];
    void *ctx_mem_ptr = (void *)s_ao_ctx_mem;
    uint64_t alloc_size =
        (mem_size > 0 && mem_size <= sizeof(s_ao_ctx_mem)) ? mem_size : 0x10000u;
    void *context_out = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_ctx_create = -1;
    if (fn_ctx_create && obs_address_is_callable(fn_ctx_create)) {
        typedef int (*fn_ctx_create_t)(void *, void *, uint64_t, void **);
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ctx_create = ((fn_ctx_create_t)fn_ctx_create)(ctx_param, ctx_mem_ptr,
                                                             alloc_size, &context_out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextCreate", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "ContextCreate", "rc", (uint64_t)(uint32_t)rc_ctx_create,
                       "code");
    obs_report_measure(id, "ContextCreate", "context", (uint64_t)(uintptr_t)context_out,
                       "addr");
    uint8_t ctx_bytes[8];
    for (size_t i = 0; i < 8; i++) {
        ctx_bytes[i] = ((const uint8_t *)&context_out)[i];
    }
    obs_report_bytes(id, "ContextCreate", "context-bytes", 0, ctx_bytes,
                     sizeof(ctx_bytes));

    /* (4) sceAudioOut2UserCreate(0xff, &user), rc and user after */
    void *user_ff = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_user_ff = -1;
    if (fn_create && obs_address_is_callable(fn_create)) {
        typedef int (*user_create_fn)(int32_t, void **);
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user_ff = ((user_create_fn)fn_create)(0xff, &user_ff);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "UserCreate-0xff", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "UserCreate-0xff", "rc", (uint64_t)(uint32_t)rc_user_ff,
                       "code");
    obs_report_measure(id, "UserCreate-0xff", "user", (uint64_t)(uintptr_t)user_ff,
                       "addr");

    /* REQ-20261011T0010Z-ao05: AudioOut2 port create, attributes, context run, and
     * destroy */
    typedef int (*fn_port_create_t)(void *, const void *, void **);
    typedef int (*fn_port_destroy_t)(void *);
    typedef int (*fn_port_get_state_t)(void *, void *);
    typedef int (*fn_port_set_attr_t)(void *, const void *, uint32_t);
    typedef int (*fn_ctx_get_queue_t)(void *, uint32_t *);
    typedef int (*fn_ctx_push_t)(void *, const void *, uint32_t);
    typedef int (*fn_ctx_advance_t)(void *);
    typedef int (*fn_ctx_destroy_t)(void *);
    typedef int (*fn_user_destroy_t)(void *);

    fn_port_create_t fn_port_create =
        obs_address_is_callable((const void *)&sceAudioOut2PortCreate_static)
            ? (fn_port_create_t)&sceAudioOut2PortCreate_static
            : (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_alt)
                   ? (fn_port_create_t)&sceAudioOut2PortCreate_alt
                   : NULL);
    if (!fn_port_create && h_ao > 0) {
        fn_port_create =
            (fn_port_create_t)obs_module_symbol(h_ao, "sceAudioOut2PortCreate");
        if (!fn_port_create)
            fn_port_create = (fn_port_create_t)obs_module_symbol(h_ao, "$A89PZmqwrSQ");
    }

    fn_port_destroy_t fn_port_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_static)
            ? (fn_port_destroy_t)&sceAudioOut2PortDestroy_static
            : (obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_alt)
                   ? (fn_port_destroy_t)&sceAudioOut2PortDestroy_alt
                   : NULL);
    if (!fn_port_destroy && h_ao > 0) {
        fn_port_destroy =
            (fn_port_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2PortDestroy");
        if (!fn_port_destroy)
            fn_port_destroy =
                (fn_port_destroy_t)obs_module_symbol(h_ao, "$cd+Rtw+D1x8");
    }

    fn_port_get_state_t fn_port_get_state =
        obs_address_is_callable((const void *)&sceAudioOut2PortGetState_static)
            ? (fn_port_get_state_t)&sceAudioOut2PortGetState_static
            : (obs_address_is_callable((const void *)&sceAudioOut2PortGetState_alt)
                   ? (fn_port_get_state_t)&sceAudioOut2PortGetState_alt
                   : NULL);
    if (!fn_port_get_state && h_ao > 0) {
        fn_port_get_state =
            (fn_port_get_state_t)obs_module_symbol(h_ao, "sceAudioOut2PortGetState");
        if (!fn_port_get_state)
            fn_port_get_state =
                (fn_port_get_state_t)obs_module_symbol(h_ao, "$gatEUKG+Ea4");
    }

    fn_port_set_attr_t fn_port_set_attr =
        obs_address_is_callable((const void *)&sceAudioOut2PortSetAttributes_static)
            ? (fn_port_set_attr_t)&sceAudioOut2PortSetAttributes_static
            : (obs_address_is_callable((const void *)&sceAudioOut2PortSetAttributes_alt)
                   ? (fn_port_set_attr_t)&sceAudioOut2PortSetAttributes_alt
                   : NULL);
    if (!fn_port_set_attr && h_ao > 0) {
        fn_port_set_attr = (fn_port_set_attr_t)obs_module_symbol(
            h_ao, "sceAudioOut2PortSetAttributes");
        if (!fn_port_set_attr)
            fn_port_set_attr =
                (fn_port_set_attr_t)obs_module_symbol(h_ao, "$8XTArSPyWHk");
    }

    fn_ctx_get_queue_t fn_ctx_get_queue =
        obs_address_is_callable((const void *)&sceAudioOut2ContextGetQueueLevel_static)
            ? (fn_ctx_get_queue_t)&sceAudioOut2ContextGetQueueLevel_static
            : (obs_address_is_callable(
                   (const void *)&sceAudioOut2ContextGetQueueLevel_alt)
                   ? (fn_ctx_get_queue_t)&sceAudioOut2ContextGetQueueLevel_alt
                   : NULL);
    if (!fn_ctx_get_queue && h_ao > 0) {
        fn_ctx_get_queue = (fn_ctx_get_queue_t)obs_module_symbol(
            h_ao, "sceAudioOut2ContextGetQueueLevel");
        if (!fn_ctx_get_queue)
            fn_ctx_get_queue =
                (fn_ctx_get_queue_t)obs_module_symbol(h_ao, "$R7d0F1g2qsU");
    }

    fn_ctx_push_t fn_ctx_push =
        obs_address_is_callable((const void *)&sceAudioOut2ContextPush_static)
            ? (fn_ctx_push_t)&sceAudioOut2ContextPush_static
            : (obs_address_is_callable((const void *)&sceAudioOut2ContextPush_alt)
                   ? (fn_ctx_push_t)&sceAudioOut2ContextPush_alt
                   : NULL);
    if (!fn_ctx_push && h_ao > 0) {
        fn_ctx_push = (fn_ctx_push_t)obs_module_symbol(h_ao, "sceAudioOut2ContextPush");
        if (!fn_ctx_push)
            fn_ctx_push = (fn_ctx_push_t)obs_module_symbol(h_ao, "$aII9h5nli9U");
    }

    fn_ctx_advance_t fn_ctx_advance =
        obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_static)
            ? (fn_ctx_advance_t)&sceAudioOut2ContextAdvance_static
            : (obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_alt)
                   ? (fn_ctx_advance_t)&sceAudioOut2ContextAdvance_alt
                   : NULL);
    if (!fn_ctx_advance && h_ao > 0) {
        fn_ctx_advance =
            (fn_ctx_advance_t)obs_module_symbol(h_ao, "sceAudioOut2ContextAdvance");
        if (!fn_ctx_advance)
            fn_ctx_advance = (fn_ctx_advance_t)obs_module_symbol(h_ao, "$PE2zHMqLSHs");
    }

    fn_ctx_destroy_t fn_ctx_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2ContextDestroy_static)
            ? (fn_ctx_destroy_t)&sceAudioOut2ContextDestroy_static
            : (obs_address_is_callable((const void *)&sceAudioOut2ContextDestroy_alt)
                   ? (fn_ctx_destroy_t)&sceAudioOut2ContextDestroy_alt
                   : NULL);
    if (!fn_ctx_destroy && h_ao > 0) {
        fn_ctx_destroy =
            (fn_ctx_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2ContextDestroy");
        if (!fn_ctx_destroy)
            fn_ctx_destroy = (fn_ctx_destroy_t)obs_module_symbol(h_ao, "$on6ZH7Abo10");
    }

    fn_user_destroy_t fn_user_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2UserDestroy_static)
            ? (fn_user_destroy_t)&sceAudioOut2UserDestroy_static
            : (obs_address_is_callable((const void *)&sceAudioOut2UserDestroy_alt)
                   ? (fn_user_destroy_t)&sceAudioOut2UserDestroy_alt
                   : NULL);
    if (!fn_user_destroy && h_ao > 0) {
        fn_user_destroy =
            (fn_user_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2UserDestroy");
        if (!fn_user_destroy)
            fn_user_destroy =
                (fn_user_destroy_t)obs_module_symbol(h_ao, "$6vY70ydXpiE");
    }

    /* REQ-20261012T0537Z-so01: sceAudioOut2GetSpeakerInfo resolve */
    typedef int (*fn_spk_info_t)(void *, uint32_t);
    fn_spk_info_t fn_spk_info = NULL;
    int is_link_spk_info = 0;
    if (obs_address_is_callable((const void *)&sceAudioOut2GetSpeakerInfo_static)) {
        fn_spk_info = (fn_spk_info_t)&sceAudioOut2GetSpeakerInfo_static;
        is_link_spk_info = 1;
    }
    if (!fn_spk_info && h_ao > 0) {
        fn_spk_info = (fn_spk_info_t)obs_module_symbol(h_ao, "DImz2Ft9E2g#H#A");
        if (!fn_spk_info)
            fn_spk_info = (fn_spk_info_t)obs_module_symbol(h_ao, "$DImz2Ft9E2g");
        if (!fn_spk_info)
            fn_spk_info =
                (fn_spk_info_t)obs_module_symbol(h_ao, "sceAudioOut2GetSpeakerInfo");
    }
    if (!fn_spk_info && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "DImz2Ft9E2g#H#A", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_spk_info = (fn_spk_info_t)p;
                break;
            }
            if (sceKernelDlsym(h, "$DImz2Ft9E2g", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_spk_info = (fn_spk_info_t)p;
                break;
            }
            if (sceKernelDlsym(h, "sceAudioOut2GetSpeakerInfo", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_spk_info = (fn_spk_info_t)p;
                break;
            }
        }
    }
    int bound_spk_info =
        (fn_spk_info != NULL && obs_address_is_callable((const void *)fn_spk_info));
    obs_report_measure(id, "sceAudioOut2GetSpeakerInfo", "bound",
                       (uint64_t)bound_spk_info, "bool");
    if (bound_spk_info) {
        obs_report_measure(id, "sceAudioOut2GetSpeakerInfo", "how",
                           (uint64_t)(is_link_spk_info ? 1 : 2),
                           is_link_spk_info ? "link" : "dlsym");
        /* (2) with no port created: flag 0 and 1 */
        for (uint32_t flag = 0; flag <= 1; flag++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "GetSpeakerInfo-noport-flag%u", flag);
            uint8_t info[0x100];
            memset(info, 0xcc, sizeof(info));
            int rc_spk = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_spk = fn_spk_info(info, flag);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            size_t changed = 0;
            for (size_t b = 0; b < sizeof(info); b++) {
                if (info[b] != 0xcc)
                    changed++;
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_spk, "code");
            obs_report_measure(id, tag, "changed-bytes", (uint64_t)changed, "count");
            obs_report_bytes(id, tag, "info-16b", 0, info, 16);
        }
    } else {
        obs_report_measure(id, "sceAudioOut2GetSpeakerInfo", "status", 0,
                           "not-possible");
    }

    /* (1) PortCreate(context, param, &port) with channels 2, then 8 */
    int bound_port_create = (fn_port_create != NULL &&
                             obs_address_is_callable((const void *)fn_port_create));
    int is_link_port_create =
        (fn_port_create == (fn_port_create_t)&sceAudioOut2PortCreate_static ||
         fn_port_create == (fn_port_create_t)&sceAudioOut2PortCreate_alt);
    obs_report_measure(id, "sceAudioOut2PortCreate", "bound",
                       (uint64_t)bound_port_create, "bool");
    if (bound_port_create) {
        obs_report_measure(id, "sceAudioOut2PortCreate", "how",
                           (uint64_t)(is_link_port_create ? 1 : 2),
                           is_link_port_create ? "link" : "dlsym");
    }

    uint8_t pparam_2ch[64];
    memset(pparam_2ch, 0, sizeof(pparam_2ch));
    *(uint32_t *)(void *)(pparam_2ch + 0x00) = 0;
    *(uint32_t *)(void *)(pparam_2ch + 0x04) = (uint32_t)(2 << 8);
    *(uint32_t *)(void *)(pparam_2ch + 0x08) = 48000;
    *(uint32_t *)(void *)(pparam_2ch + 0x0c) = 2;
    *(uint64_t *)(void *)(pparam_2ch + 0x10) = (uint64_t)(uintptr_t)user_ff;
    void *port_2ch = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_p2 = -1;
    if (fn_port_create && obs_address_is_callable((const void *)fn_port_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p2 = fn_port_create(context_out, pparam_2ch, &port_2ch);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortCreate-2ch", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "PortCreate-2ch", "rc", (uint64_t)(uint32_t)rc_p2, "code");
    obs_report_measure(id, "PortCreate-2ch", "port", (uint64_t)(uintptr_t)port_2ch,
                       "addr");
    obs_report_bytes(id, "PortCreate-2ch", "param-40h", 0, pparam_2ch,
                     sizeof(pparam_2ch));

    uint8_t pparam_8ch[64];
    memset(pparam_8ch, 0, sizeof(pparam_8ch));
    *(uint32_t *)(void *)(pparam_8ch + 0x00) = 0;
    *(uint32_t *)(void *)(pparam_8ch + 0x04) = (uint32_t)(8 << 8);
    *(uint32_t *)(void *)(pparam_8ch + 0x08) = 48000;
    *(uint32_t *)(void *)(pparam_8ch + 0x0c) = 2;
    *(uint64_t *)(void *)(pparam_8ch + 0x10) = (uint64_t)(uintptr_t)user_ff;
    void *port_8ch = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_p8 = -1;
    if (fn_port_create && obs_address_is_callable((const void *)fn_port_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p8 = fn_port_create(context_out, pparam_8ch, &port_8ch);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortCreate-8ch", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "PortCreate-8ch", "rc", (uint64_t)(uint32_t)rc_p8, "code");
    obs_report_measure(id, "PortCreate-8ch", "port", (uint64_t)(uintptr_t)port_8ch,
                       "addr");
    obs_report_bytes(id, "PortCreate-8ch", "param-40h", 0, pparam_8ch,
                     sizeof(pparam_8ch));

    /* REQ-20261011T1440Z-ao07: PortCreate with +0x0c = 3, then 1, 4, 0 (8ch, 48000),
     * then +0x0c = 3 (2ch) */
    static const struct {
        uint32_t type;
        uint32_t channels;
        const char *tag;
    } ao07_ports[] = {
        {3, 8, "PortCreate-8ch-type3"}, {1, 8, "PortCreate-8ch-type1"},
        {4, 8, "PortCreate-8ch-type4"}, {0, 8, "PortCreate-8ch-type0"},
        {3, 2, "PortCreate-2ch-type3"},
    };
    for (size_t pidx = 0; pidx < sizeof(ao07_ports) / sizeof(ao07_ports[0]); pidx++) {
        uint32_t ptype = ao07_ports[pidx].type;
        uint32_t pch = ao07_ports[pidx].channels;
        const char *ptag = ao07_ports[pidx].tag;

        uint8_t pparam[64];
        memset(pparam, 0, sizeof(pparam));
        *(uint32_t *)(void *)(pparam + 0x00) = 0;
        *(uint32_t *)(void *)(pparam + 0x04) = (uint32_t)(pch << 8);
        *(uint32_t *)(void *)(pparam + 0x08) = 48000;
        *(uint32_t *)(void *)(pparam + 0x0c) = ptype;
        *(uint64_t *)(void *)(pparam + 0x10) = (uint64_t)(uintptr_t)user_ff;

        void *port_out = (void *)0xa5a5a5a5a5a5a5a5ULL;
        int rc_p = -1;
        if (fn_port_create && obs_address_is_callable((const void *)fn_port_create)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p = fn_port_create(context_out, pparam, &port_out);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, ptag, "fault", (uint64_t)sig, "signal");
            }
        }
        obs_report_measure(id, ptag, "rc", (uint64_t)(uint32_t)rc_p, "code");
        obs_report_measure(id, ptag, "port", (uint64_t)(uintptr_t)port_out, "addr");
        obs_report_bytes(id, ptag, "param-40h", 0, pparam, sizeof(pparam));
    }

    /* (2) PortGetState and PortSetAttributes on created port */
    void *test_port =
        (rc_p2 == 0 && (uintptr_t)port_2ch >= 0x10000UL) ? port_2ch : port_8ch;
    for (int iter = 1; iter <= 2; iter++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "PortGetState-iter%d", iter);
        uint8_t state_buf[0x100];
        memset(state_buf, 0xa5, sizeof(state_buf));
        int rc_state = -1;
        if (fn_port_get_state &&
            obs_address_is_callable((const void *)fn_port_get_state)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_state = fn_port_get_state(test_port, state_buf);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
        }
        size_t changed_state = 0;
        for (size_t b = 0; b < sizeof(state_buf); b++) {
            if (state_buf[b] != 0xa5)
                changed_state++;
        }
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_state, "code");
        obs_report_measure(id, tag, "changed-bytes", (uint64_t)changed_state, "count");
        obs_report_bytes(id, tag, "state-buf", 0, state_buf, sizeof(state_buf));

        for (uint32_t n = 0; n <= 1; n++) {
            snprintf(tag, sizeof(tag), "PortSetAttributes-n%u-iter%d", n, iter);
            uint8_t attr_buf[0x100];
            memset(attr_buf, 0xa5, sizeof(attr_buf));
            int rc_attr = -1;
            if (fn_port_set_attr &&
                obs_address_is_callable((const void *)fn_port_set_attr)) {
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_attr = fn_port_set_attr(test_port, attr_buf, n);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                    obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
                }
            }
            size_t changed_attr = 0;
            for (size_t b = 0; b < sizeof(attr_buf); b++) {
                if (attr_buf[b] != 0xa5)
                    changed_attr++;
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_attr, "code");
            obs_report_measure(id, tag, "changed-bytes", (uint64_t)changed_attr,
                               "count");
        }
    }

    /* REQ-20261012T0537Z-so01: (1) GetSpeakerInfo with port created: flag 0 and 1 */
    if (bound_spk_info) {
        for (uint32_t flag = 0; flag <= 1; flag++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "GetSpeakerInfo-port-flag%u", flag);
            uint8_t info[0x100];
            memset(info, 0xcc, sizeof(info));
            int rc_spk = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_spk = fn_spk_info(info, flag);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            size_t changed = 0;
            for (size_t b = 0; b < sizeof(info); b++) {
                if (info[b] != 0xcc)
                    changed++;
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_spk, "code");
            obs_report_measure(id, tag, "changed-bytes", (uint64_t)changed, "count");
            obs_report_bytes(id, tag, "info-16b", 0, info, 16);
        }
    }

    /* (3) ContextGetQueueLevel, ContextPush, ContextAdvance */
    for (int iter = 1; iter <= 2; iter++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "ContextGetQueueLevel-iter%d", iter);
        uint32_t qlevel = 0xa5a5a5a5U;
        uint64_t t_before = sceKernelGetProcessTime();
        int rc_ql = -1;
        if (fn_ctx_get_queue &&
            obs_address_is_callable((const void *)fn_ctx_get_queue)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_ql = fn_ctx_get_queue(context_out, &qlevel);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        uint64_t t_after = sceKernelGetProcessTime();
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_ql, "code");
        obs_report_measure(id, tag, "level", (uint64_t)qlevel, "count");
        obs_report_measure(id, tag, "usec", t_after - t_before, "time");

        for (uint32_t n = 0; n <= 1; n++) {
            snprintf(tag, sizeof(tag), "ContextPush-n%u-iter%d", n, iter);
            uint8_t zero_buf[0x400];
            memset(zero_buf, 0, sizeof(zero_buf));
            t_before = sceKernelGetProcessTime();
            int rc_push = -1;
            if (fn_ctx_push && obs_address_is_callable((const void *)fn_ctx_push)) {
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_push = fn_ctx_push(context_out, zero_buf, n);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
            }
            t_after = sceKernelGetProcessTime();
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_push, "code");
            obs_report_measure(id, tag, "usec", t_after - t_before, "time");
        }

        snprintf(tag, sizeof(tag), "ContextAdvance-iter%d", iter);
        t_before = sceKernelGetProcessTime();
        int rc_adv = -1;
        if (fn_ctx_advance && obs_address_is_callable((const void *)fn_ctx_advance)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_adv = fn_ctx_advance(context_out);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        t_after = sceKernelGetProcessTime();
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_adv, "code");
        obs_report_measure(id, tag, "usec", t_after - t_before, "time");
    }

    /* REQ-20261011T1130Z-ao06, REQ-20261011T1310Z-lk01:
     * sceAudioOut2GetSpeakerArrayMemorySize and speaker-array calls */
    typedef uint64_t (*fn_spk_mem_size_t)(uint32_t, uint32_t, uint32_t);
    fn_spk_mem_size_t fn_spk_mem_size = NULL;
    int spk_is_link = 0;
    if (obs_address_is_callable(
            (const void *)&sceAudioOut2GetSpeakerArrayMemorySize_static)) {
        fn_spk_mem_size =
            (fn_spk_mem_size_t)&sceAudioOut2GetSpeakerArrayMemorySize_static;
        spk_is_link = 1;
    }
    if (!fn_spk_mem_size && h_ao > 0) {
        fn_spk_mem_size = (fn_spk_mem_size_t)obs_module_symbol(h_ao, "G1YOKDJYX2Y#H#A");
        if (!fn_spk_mem_size)
            fn_spk_mem_size =
                (fn_spk_mem_size_t)obs_module_symbol(h_ao, "G1YOKDJYX2Y#A#B");
        if (!fn_spk_mem_size)
            fn_spk_mem_size = (fn_spk_mem_size_t)obs_module_symbol(h_ao, "G1YOKDJYX2Y");
        if (!fn_spk_mem_size)
            fn_spk_mem_size = (fn_spk_mem_size_t)obs_module_symbol(
                h_ao, "sceAudioOut2GetSpeakerArrayMemorySize");
    }
    if (!fn_spk_mem_size && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        static const char *const s_spk_size_cands[] = {
            "G1YOKDJYX2Y#H#A", "G1YOKDJYX2Y#A#B",
            "G1YOKDJYX2Y#j#i", "G1YOKDJYX2Y",
            "$G1YOKDJYX2Y",    "sceAudioOut2GetSpeakerArrayMemorySize"};
        for (int h = 1; h <= 256; h++) {
            for (size_t c = 0;
                 c < sizeof(s_spk_size_cands) / sizeof(s_spk_size_cands[0]); c++) {
                void *p = NULL;
                if (sceKernelDlsym(h, s_spk_size_cands[c], &p) == 0 &&
                    obs_address_is_callable(p)) {
                    fn_spk_mem_size = (fn_spk_mem_size_t)p;
                    break;
                }
            }
            if (fn_spk_mem_size)
                break;
        }
    }
    int spk_bound = (fn_spk_mem_size != NULL &&
                     obs_address_is_callable((const void *)fn_spk_mem_size));
    obs_report_measure(id, "sceAudioOut2GetSpeakerArrayMemorySize", "bound",
                       (uint64_t)spk_bound, "bool");
    if (spk_bound) {
        obs_report_measure(id, "sceAudioOut2GetSpeakerArrayMemorySize", "how",
                           (uint64_t)(spk_is_link ? 1 : 2),
                           spk_is_link ? "link" : "dlsym");
    }

    uint64_t spk_size_8_0_1 = 0;
    if (spk_bound) {
        static const struct {
            uint32_t a;
            uint32_t b;
            uint32_t c;
        } s_spk_tuples[] = {{8, 0, 1}, {2, 0, 1}, {6, 0, 1},
                            {8, 1, 1}, {8, 0, 0}, {8, 0, 2}};
        for (size_t t = 0; t < sizeof(s_spk_tuples) / sizeof(s_spk_tuples[0]); t++) {
            uint32_t a = s_spk_tuples[t].a;
            uint32_t b = s_spk_tuples[t].b;
            uint32_t c = s_spk_tuples[t].c;
            for (int iter = 1; iter <= 2; iter++) {
                char tag[64];
                snprintf(tag, sizeof(tag), "GetSpeakerArrayMemorySize-%u-%u-%u-iter%d",
                         a, b, c, iter);
                uint64_t ret_size = 0;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    ret_size = fn_spk_mem_size(a, b, c);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                    obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
                }
                obs_report_measure(id, tag, "size", ret_size, "bytes");
                if (a == 8 && b == 0 && c == 1 && iter == 1) {
                    spk_size_8_0_1 = ret_size;
                }
            }
        }
    }
    (void)spk_size_8_0_1;

    /* REQ-20261012T1822Z-b15b: libSceAudioOut2 speaker arrays, mastering, system state,
     * port register */
    /* Arm 1: SpeakerArrayCreate and SpeakerArrayDestroy */
    typedef int (*fn_sa_create_t)(void **, const void *, const void *);
    typedef int (*fn_sa_destroy_t)(void *);
    typedef int (*fn_sa_coeff_t)(void *, void *, uint32_t, uint32_t);
    typedef int (*fn_sa_ambi_coeff_t)(void *, uint32_t, void *, uint32_t);

    fn_sa_create_t fn_sa_create =
        obs_address_is_callable((const void *)&sceAudioOut2SpeakerArrayCreate_static)
            ? (fn_sa_create_t)(void *)(uintptr_t)&sceAudioOut2SpeakerArrayCreate_static
            : NULL;
    if (!fn_sa_create && h_ao > 0) {
        fn_sa_create = (fn_sa_create_t)obs_module_symbol(h_ao, "+k91hoTuoA8#H#A");
        if (!fn_sa_create)
            fn_sa_create = (fn_sa_create_t)obs_module_symbol(h_ao, "$+k91hoTuoA8");
        if (!fn_sa_create)
            fn_sa_create = (fn_sa_create_t)obs_module_symbol(
                h_ao, "sceAudioOut2SpeakerArrayCreate");
    }

    fn_sa_destroy_t fn_sa_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2SpeakerArrayDestroy_static)
            ? (fn_sa_destroy_t)(void
                                    *)(uintptr_t)&sceAudioOut2SpeakerArrayDestroy_static
            : NULL;
    if (!fn_sa_destroy && h_ao > 0) {
        fn_sa_destroy = (fn_sa_destroy_t)obs_module_symbol(h_ao, "erCWQR5eKiQ#H#A");
        if (!fn_sa_destroy)
            fn_sa_destroy = (fn_sa_destroy_t)obs_module_symbol(h_ao, "$erCWQR5eKiQ");
        if (!fn_sa_destroy)
            fn_sa_destroy = (fn_sa_destroy_t)obs_module_symbol(
                h_ao, "sceAudioOut2SpeakerArrayDestroy");
    }

    fn_sa_coeff_t fn_sa_coeff =
        obs_address_is_callable(
            (const void *)&sceAudioOut2GetSpeakerArrayCoefficients_static)
            ? (fn_sa_coeff_t)(void
                                  *)(uintptr_t)&sceAudioOut2GetSpeakerArrayCoefficients_static
            : NULL;
    if (!fn_sa_coeff && h_ao > 0) {
        fn_sa_coeff = (fn_sa_coeff_t)obs_module_symbol(h_ao, "4BlZurolOAo#H#A");
        if (!fn_sa_coeff)
            fn_sa_coeff = (fn_sa_coeff_t)obs_module_symbol(h_ao, "$4BlZurolOAo");
        if (!fn_sa_coeff)
            fn_sa_coeff = (fn_sa_coeff_t)obs_module_symbol(
                h_ao, "sceAudioOut2GetSpeakerArrayCoefficients");
    }

    fn_sa_ambi_coeff_t fn_sa_ambi_coeff =
        obs_address_is_callable(
            (const void *)&sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients_static)
            ? (fn_sa_ambi_coeff_t)(void
                                       *)(uintptr_t)&sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients_static
            : NULL;
    if (!fn_sa_ambi_coeff && h_ao > 0) {
        fn_sa_ambi_coeff =
            (fn_sa_ambi_coeff_t)obs_module_symbol(h_ao, "28QqMnuuJ9Y#H#A");
        if (!fn_sa_ambi_coeff)
            fn_sa_ambi_coeff =
                (fn_sa_ambi_coeff_t)obs_module_symbol(h_ao, "$28QqMnuuJ9Y");
        if (!fn_sa_ambi_coeff)
            fn_sa_ambi_coeff = (fn_sa_ambi_coeff_t)obs_module_symbol(
                h_ao, "sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients");
    }

    obs_report_measure(id, "SpeakerArrayCreate", "bound",
                       (uint64_t)(fn_sa_create != NULL), "bool");
    obs_report_measure(id, "SpeakerArrayDestroy", "bound",
                       (uint64_t)(fn_sa_destroy != NULL), "bool");
    obs_report_measure(id, "GetSpeakerArrayCoefficients", "bound",
                       (uint64_t)(fn_sa_coeff != NULL), "bool");
    obs_report_measure(id, "GetSpeakerArrayAmbisonicsCoefficients", "bound",
                       (uint64_t)(fn_sa_ambi_coeff != NULL), "bool");

    void *sa_handles[34];
    memset(sa_handles, 0, sizeof(sa_handles));
    uint8_t vbap_param[0x40];
    uint8_t ambi_param[0x40];
    memset(vbap_param, 0, sizeof(vbap_param));
    memset(ambi_param, 0, sizeof(ambi_param));

    if (fn_sa_create && obs_address_is_callable(fn_sa_create)) {
        /* 33 creates in loop */
        for (int i = 0; i < 33; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "SpeakerArrayCreate-iter%d", i + 1);
            void *sa = (void *)0xccccccccccccccccULL;
            int rc_c = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_c = fn_sa_create(&sa, vbap_param, ambi_param);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_c, "code");
            obs_report_measure(id, tag, "sa", (uint64_t)(uintptr_t)sa, "addr");
            obs_report_bytes(id, tag, "sa-8b", 0, (const uint8_t *)&sa, 8);
            sa_handles[i] = sa;
        }

        /* null sa */
        int rc_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_null = fn_sa_create(NULL, vbap_param, ambi_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SpeakerArrayCreate-null-sa", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SpeakerArrayCreate-null-sa", "rc",
                           (uint64_t)(uint32_t)rc_null, "code");
    }

    /* Arm 2: Coefficients on a live speaker array (e.g. sa_handles[0]) */
    void *live_sa = sa_handles[0];
    if (fn_sa_coeff && obs_address_is_callable(fn_sa_coeff)) {
        float out8[8];
        memset(out8, 0xcc, sizeof(out8));
        int rc_cf0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cf0 = fn_sa_coeff(live_sa, out8, 8, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSpeakerArrayCoefficients-8-h0", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSpeakerArrayCoefficients-8-h0", "rc",
                           (uint64_t)(uint32_t)rc_cf0, "code");
        obs_report_bytes(id, "GetSpeakerArrayCoefficients-8-h0", "out-32b", 0,
                         (const uint8_t *)out8, 32);

        /* heightAware 1 */
        memset(out8, 0xcc, sizeof(out8));
        int rc_cf1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cf1 = fn_sa_coeff(live_sa, out8, 8, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSpeakerArrayCoefficients-8-h1", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSpeakerArrayCoefficients-8-h1", "rc",
                           (uint64_t)(uint32_t)rc_cf1, "code");
        obs_report_bytes(id, "GetSpeakerArrayCoefficients-8-h1", "out-32b", 0,
                         (const uint8_t *)out8, 32);

        /* count 0 */
        int rc_cf_c0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cf_c0 = fn_sa_coeff(live_sa, out8, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetSpeakerArrayCoefficients-c0", "rc",
                           (uint64_t)(uint32_t)rc_cf_c0, "code");

        /* count 257 */
        static float out_large[300];
        memset(out_large, 0xcc, sizeof(out_large));
        int rc_cf_c257 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cf_c257 = fn_sa_coeff(live_sa, out_large, 257, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetSpeakerArrayCoefficients-c257", "rc",
                           (uint64_t)(uint32_t)rc_cf_c257, "code");
    }

    if (fn_sa_ambi_coeff && obs_address_is_callable(fn_sa_ambi_coeff)) {
        float out16[16];
        memset(out16, 0xcc, sizeof(out16));
        int rc_ac0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ac0 = fn_sa_ambi_coeff(live_sa, 0, out16, 16);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSpeakerArrayAmbisonicsCoefficients-ch0", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSpeakerArrayAmbisonicsCoefficients-ch0", "rc",
                           (uint64_t)(uint32_t)rc_ac0, "code");
        obs_report_bytes(id, "GetSpeakerArrayAmbisonicsCoefficients-ch0", "out-64b", 0,
                         (const uint8_t *)out16, 64);

        /* channel 64 */
        memset(out16, 0xcc, sizeof(out16));
        int rc_ac64 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ac64 = fn_sa_ambi_coeff(live_sa, 64, out16, 16);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSpeakerArrayAmbisonicsCoefficients-ch64",
                               "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSpeakerArrayAmbisonicsCoefficients-ch64", "rc",
                           (uint64_t)(uint32_t)rc_ac64, "code");
        obs_report_bytes(id, "GetSpeakerArrayAmbisonicsCoefficients-ch64", "out-64b", 0,
                         (const uint8_t *)out16, 64);
    }

    /* Destroy SpeakerArrays: live, twice, and on 0 */
    if (fn_sa_destroy && obs_address_is_callable(fn_sa_destroy)) {
        int rc_d1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d1 = fn_sa_destroy(sa_handles[0]);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SpeakerArrayDestroy-live", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SpeakerArrayDestroy-live", "rc",
                           (uint64_t)(uint32_t)rc_d1, "code");

        /* twice (double destroy) */
        int rc_d2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d2 = fn_sa_destroy(sa_handles[0]);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SpeakerArrayDestroy-twice", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SpeakerArrayDestroy-twice", "rc",
                           (uint64_t)(uint32_t)rc_d2, "code");

        /* on 0 */
        int rc_d0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d0 = fn_sa_destroy(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SpeakerArrayDestroy-zero", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SpeakerArrayDestroy-zero", "rc",
                           (uint64_t)(uint32_t)rc_d0, "code");

        /* Clean up remaining handles */
        for (int i = 1; i < 33; i++) {
            if (sa_handles[i] && sa_handles[i] != (void *)0xccccccccccccccccULL) {
                (void)fn_sa_destroy(sa_handles[i]);
            }
        }
    }

    /* Arm 3: MasteringInit, MasteringSetParam, MasteringGetState, MasteringTerm */
    typedef int (*fn_m_init_t)(void);
    typedef int (*fn_m_param_t)(const void *);
    typedef int (*fn_m_state_t)(void *);
    typedef int (*fn_m_term_t)(void);

    fn_m_init_t fn_m_init =
        obs_address_is_callable((const void *)&sceAudioOut2MasteringInit_static)
            ? (fn_m_init_t)&sceAudioOut2MasteringInit_static
            : NULL;
    if (!fn_m_init && h_ao > 0) {
        fn_m_init = (fn_m_init_t)obs_module_symbol(h_ao, "$XHl38ZNknbs");
        if (!fn_m_init)
            fn_m_init =
                (fn_m_init_t)obs_module_symbol(h_ao, "sceAudioOut2MasteringInit");
    }

    fn_m_param_t fn_m_param =
        obs_address_is_callable((const void *)&sceAudioOut2MasteringSetParam_static)
            ? (fn_m_param_t)&sceAudioOut2MasteringSetParam_static
            : NULL;
    if (!fn_m_param && h_ao > 0) {
        fn_m_param = (fn_m_param_t)obs_module_symbol(h_ao, "$v8iOE+j8a5o");
        if (!fn_m_param)
            fn_m_param =
                (fn_m_param_t)obs_module_symbol(h_ao, "sceAudioOut2MasteringSetParam");
    }

    fn_m_state_t fn_m_state =
        obs_address_is_callable((const void *)&sceAudioOut2MasteringGetState_static)
            ? (fn_m_state_t)&sceAudioOut2MasteringGetState_static
            : NULL;
    if (!fn_m_state && h_ao > 0) {
        fn_m_state = (fn_m_state_t)obs_module_symbol(h_ao, "$VZidxi2cYh0");
        if (!fn_m_state)
            fn_m_state =
                (fn_m_state_t)obs_module_symbol(h_ao, "sceAudioOut2MasteringGetState");
    }

    fn_m_term_t fn_m_term =
        obs_address_is_callable((const void *)&sceAudioOut2MasteringTerm_static)
            ? (fn_m_term_t)&sceAudioOut2MasteringTerm_static
            : NULL;
    if (!fn_m_term && h_ao > 0) {
        fn_m_term = (fn_m_term_t)obs_module_symbol(h_ao, "$2bbBBOkH4CY");
        if (!fn_m_term)
            fn_m_term =
                (fn_m_term_t)obs_module_symbol(h_ao, "sceAudioOut2MasteringTerm");
    }

    obs_report_measure(id, "MasteringInit", "bound", (uint64_t)(fn_m_init != NULL),
                       "bool");
    obs_report_measure(id, "MasteringSetParam", "bound", (uint64_t)(fn_m_param != NULL),
                       "bool");
    obs_report_measure(id, "MasteringGetState", "bound", (uint64_t)(fn_m_state != NULL),
                       "bool");
    obs_report_measure(id, "MasteringTerm", "bound", (uint64_t)(fn_m_term != NULL),
                       "bool");

    if (fn_m_init && obs_address_is_callable(fn_m_init)) {
        int rc_mi = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_mi = fn_m_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "MasteringInit", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "MasteringInit", "rc", (uint64_t)(uint32_t)rc_mi,
                           "code");
    }

    if (fn_m_param && obs_address_is_callable(fn_m_param)) {
        uint8_t m_param[0x40];
        memset(m_param, 0, sizeof(m_param));
        int rc_mp = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_mp = fn_m_param(m_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "MasteringSetParam", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "MasteringSetParam", "rc", (uint64_t)(uint32_t)rc_mp,
                           "code");
    }

    if (fn_m_state && obs_address_is_callable(fn_m_state)) {
        uint8_t m_state[0x40];
        memset(m_state, 0xcc, sizeof(m_state));
        int rc_ms = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ms = fn_m_state(m_state);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "MasteringGetState", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "MasteringGetState", "rc", (uint64_t)(uint32_t)rc_ms,
                           "code");
        obs_report_bytes(id, "MasteringGetState", "state-40h", 0, m_state, 0x40);
    }

    if (fn_m_term && obs_address_is_callable(fn_m_term)) {
        int rc_mt = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_mt = fn_m_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "MasteringTerm", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "MasteringTerm", "rc", (uint64_t)(uint32_t)rc_mt,
                           "code");
    }

    /* Arm 4: GetSystemState */
    typedef int (*fn_sys_state_t)(void *);
    fn_sys_state_t fn_sys_state =
        obs_address_is_callable((const void *)&sceAudioOut2GetSystemState_static)
            ? (fn_sys_state_t)&sceAudioOut2GetSystemState_static
            : NULL;
    if (!fn_sys_state && h_ao > 0) {
        fn_sys_state = (fn_sys_state_t)obs_module_symbol(h_ao, "$bkBN+CMLwRc");
        if (!fn_sys_state)
            fn_sys_state =
                (fn_sys_state_t)obs_module_symbol(h_ao, "sceAudioOut2GetSystemState");
    }
    obs_report_measure(id, "GetSystemState", "bound", (uint64_t)(fn_sys_state != NULL),
                       "bool");
    if (fn_sys_state && obs_address_is_callable(fn_sys_state)) {
        uint8_t sys_state[0x40];
        memset(sys_state, 0xcc, sizeof(sys_state));
        int rc_ss = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ss = fn_sys_state(sys_state);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSystemState", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSystemState", "rc", (uint64_t)(uint32_t)rc_ss,
                           "code");
        obs_report_bytes(id, "GetSystemState", "state-40h", 0, sys_state, 0x40);

        /* null */
        int rc_ss_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ss_null = fn_sys_state(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSystemState-null", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetSystemState-null", "rc",
                           (uint64_t)(uint32_t)rc_ss_null, "code");
    }

    /* Arm 5: PortRegister and PortUnregister */
    typedef int (*fn_port_reg_t)(void *, void *);
    fn_port_reg_t fn_port_reg =
        obs_address_is_callable((const void *)&sceAudioOut2PortRegister_static)
            ? (fn_port_reg_t)&sceAudioOut2PortRegister_static
            : NULL;
    if (!fn_port_reg && h_ao > 0) {
        fn_port_reg = (fn_port_reg_t)obs_module_symbol(h_ao, "$Bph+nEAV4WY");
        if (!fn_port_reg)
            fn_port_reg =
                (fn_port_reg_t)obs_module_symbol(h_ao, "sceAudioOut2PortRegister");
    }

    fn_port_reg_t fn_port_unreg =
        obs_address_is_callable((const void *)&sceAudioOut2PortUnregister_static)
            ? (fn_port_reg_t)&sceAudioOut2PortUnregister_static
            : NULL;
    if (!fn_port_unreg && h_ao > 0) {
        fn_port_unreg = (fn_port_reg_t)obs_module_symbol(h_ao, "$16Ob5ACgOYk");
        if (!fn_port_unreg)
            fn_port_unreg =
                (fn_port_reg_t)obs_module_symbol(h_ao, "sceAudioOut2PortUnregister");
    }

    obs_report_measure(id, "PortRegister", "bound", (uint64_t)(fn_port_reg != NULL),
                       "bool");
    obs_report_measure(id, "PortUnregister", "bound", (uint64_t)(fn_port_unreg != NULL),
                       "bool");

    void *live_port = port_2ch;
    if (fn_port_reg && obs_address_is_callable(fn_port_reg)) {
        /* live ctx + live port */
        int rc_pr1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr1 = fn_port_reg(context_out, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortRegister-live", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortRegister-live", "rc", (uint64_t)(uint32_t)rc_pr1,
                           "code");

        /* twice */
        int rc_pr2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr2 = fn_port_reg(context_out, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortRegister-twice", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortRegister-twice", "rc", (uint64_t)(uint32_t)rc_pr2,
                           "code");

        /* foreign ctx */
        int rc_prf = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_prf = fn_port_reg((void *)(uintptr_t)0xdeadbeefULL, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortRegister-foreign-ctx", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortRegister-foreign-ctx", "rc",
                           (uint64_t)(uint32_t)rc_prf, "code");
    }

    if (fn_port_unreg && obs_address_is_callable(fn_port_unreg)) {
        /* live ctx + live port */
        int rc_pu1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pu1 = fn_port_unreg(context_out, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortUnregister-live", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortUnregister-live", "rc", (uint64_t)(uint32_t)rc_pu1,
                           "code");

        /* twice */
        int rc_pu2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pu2 = fn_port_unreg(context_out, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortUnregister-twice", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortUnregister-twice", "rc", (uint64_t)(uint32_t)rc_pu2,
                           "code");

        /* foreign ctx */
        int rc_puf = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_puf = fn_port_unreg((void *)(uintptr_t)0xdeadbeefULL, live_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortUnregister-foreign-ctx", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortUnregister-foreign-ctx", "rc",
                           (uint64_t)(uint32_t)rc_puf, "code");
    }

    /* REQ-20261012T0537Z-so01: (3) sceRemoteplayInitialize & (4)
     * sceRemoteplayGetConnectionStatus */
    typedef int (*fn_rp_init_t)(void *, size_t);
    typedef int (*fn_rp_status_t)(int32_t, void *);
    typedef int (*fn_rp_term_t)(void);

    fn_rp_init_t fn_rp_init = NULL;
    int is_link_rp_init = 0;
    if (obs_address_is_callable((const void *)&sceRemoteplayInitialize_static)) {
        fn_rp_init = (fn_rp_init_t)&sceRemoteplayInitialize_static;
        is_link_rp_init = 1;
    }
    if (!fn_rp_init)
        fn_rp_init = (fn_rp_init_t)obs_module_symbol(OBS_HANDLE_SELF, "$k1SwgkMSOM8");
    if (!fn_rp_init)
        fn_rp_init = (fn_rp_init_t)obs_module_symbol(1, "$k1SwgkMSOM8");
    if (!fn_rp_init) {
        int h_rp = obs_module_open("libSceRemoteplay");
        if (h_rp > 0) {
            fn_rp_init = (fn_rp_init_t)obs_module_symbol(h_rp, "$k1SwgkMSOM8");
            if (!fn_rp_init)
                fn_rp_init =
                    (fn_rp_init_t)obs_module_symbol(h_rp, "sceRemoteplayInitialize");
        }
    }

    fn_rp_status_t fn_rp_status = NULL;
    int is_link_rp_status = 0;
    if (obs_address_is_callable(
            (const void *)&sceRemoteplayGetConnectionStatus_static)) {
        fn_rp_status = (fn_rp_status_t)&sceRemoteplayGetConnectionStatus_static;
        is_link_rp_status = 1;
    }
    if (!fn_rp_status)
        fn_rp_status =
            (fn_rp_status_t)obs_module_symbol(OBS_HANDLE_SELF, "$g3PNjYKWqnQ");
    if (!fn_rp_status)
        fn_rp_status = (fn_rp_status_t)obs_module_symbol(1, "$g3PNjYKWqnQ");
    if (!fn_rp_status) {
        int h_rp = obs_module_open("libSceRemoteplay");
        if (h_rp > 0) {
            fn_rp_status = (fn_rp_status_t)obs_module_symbol(h_rp, "$g3PNjYKWqnQ");
            if (!fn_rp_status)
                fn_rp_status = (fn_rp_status_t)obs_module_symbol(
                    h_rp, "sceRemoteplayGetConnectionStatus");
        }
    }

    fn_rp_term_t fn_rp_term = NULL;
    int is_link_rp_term = 0;
    if (obs_address_is_callable((const void *)&sceRemoteplayTerminate_static)) {
        fn_rp_term = (fn_rp_term_t)&sceRemoteplayTerminate_static;
        is_link_rp_term = 1;
    }
    if (!fn_rp_term)
        fn_rp_term = (fn_rp_term_t)obs_module_symbol(OBS_HANDLE_SELF, "$BOwybKVa3Do");
    if (!fn_rp_term)
        fn_rp_term = (fn_rp_term_t)obs_module_symbol(1, "$BOwybKVa3Do");
    if (!fn_rp_term) {
        int h_rp = obs_module_open("libSceRemoteplay");
        if (h_rp > 0) {
            fn_rp_term = (fn_rp_term_t)obs_module_symbol(h_rp, "$BOwybKVa3Do");
            if (!fn_rp_term)
                fn_rp_term =
                    (fn_rp_term_t)obs_module_symbol(h_rp, "sceRemoteplayTerminate");
        }
    }

    int bound_rp_init =
        (fn_rp_init != NULL && obs_address_is_callable((const void *)fn_rp_init));
    obs_report_measure(id, "sceRemoteplayInitialize", "bound", (uint64_t)bound_rp_init,
                       "bool");
    if (bound_rp_init) {
        obs_report_measure(id, "sceRemoteplayInitialize", "how",
                           (uint64_t)(is_link_rp_init ? 1 : 2),
                           is_link_rp_init ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceRemoteplayInitialize", "status", 0, "not-possible");
    }

    int bound_rp_status =
        (fn_rp_status != NULL && obs_address_is_callable((const void *)fn_rp_status));
    obs_report_measure(id, "sceRemoteplayGetConnectionStatus", "bound",
                       (uint64_t)bound_rp_status, "bool");
    if (bound_rp_status) {
        obs_report_measure(id, "sceRemoteplayGetConnectionStatus", "how",
                           (uint64_t)(is_link_rp_status ? 1 : 2),
                           is_link_rp_status ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceRemoteplayGetConnectionStatus", "status", 0,
                           "not-possible");
    }

    int bound_rp_term =
        (fn_rp_term != NULL && obs_address_is_callable((const void *)fn_rp_term));
    obs_report_measure(id, "sceRemoteplayTerminate", "bound", (uint64_t)bound_rp_term,
                       "bool");
    if (bound_rp_term) {
        obs_report_measure(id, "sceRemoteplayTerminate", "how",
                           (uint64_t)(is_link_rp_term ? 1 : 2),
                           is_link_rp_term ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceRemoteplayTerminate", "status", 0, "not-possible");
    }

    if (bound_rp_init) {
        static _Alignas(16) uint8_t s_rp_heap[0x1800];
        memset(s_rp_heap, 0xcc, sizeof(s_rp_heap));
        int rc_rp1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rp1 = fn_rp_init(s_rp_heap, sizeof(s_rp_heap));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceRemoteplayInitialize", "fault", (uint64_t)sig,
                               "signal");
        }
        size_t changed_rp = 0;
        for (size_t b = 0; b < sizeof(s_rp_heap); b++) {
            if (s_rp_heap[b] != 0xcc)
                changed_rp++;
        }
        obs_report_measure(id, "sceRemoteplayInitialize", "rc",
                           (uint64_t)(uint32_t)rc_rp1, "code");
        obs_report_measure(id, "sceRemoteplayInitialize", "changed-bytes",
                           (uint64_t)changed_rp, "count");

        int rc_rp2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rp2 = fn_rp_init(s_rp_heap, sizeof(s_rp_heap));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "sceRemoteplayInitialize", "rc-second",
                           (uint64_t)(uint32_t)rc_rp2, "code");

        if (bound_rp_status) {
            uint8_t status_buf[8];
            memset(status_buf, 0xcc, sizeof(status_buf));
            int rc_status = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_status = fn_rp_status(user, status_buf);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "sceRemoteplayGetConnectionStatus", "fault",
                                   (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "sceRemoteplayGetConnectionStatus", "rc",
                               (uint64_t)(uint32_t)rc_status, "code");
            obs_report_bytes(id, "sceRemoteplayGetConnectionStatus", "status-8b", 0,
                             status_buf, 8);
        }

        if (bound_rp_term) {
            int rc_t = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_t = fn_rp_term();
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "sceRemoteplayTerminate", "rc",
                               (uint64_t)(uint32_t)rc_t, "code");
        }
    }

    /* (4) PortDestroy, ContextDestroy, UserDestroy */
    int rc_pdest2 = -1, rc_pdest8 = -1, rc_cdest = -1, rc_udest = -1;
    if (fn_port_destroy && obs_address_is_callable((const void *)fn_port_destroy)) {
        if (rc_p2 == 0 && (uintptr_t)port_2ch >= 0x10000UL) {
            rc_pdest2 = fn_port_destroy(port_2ch);
        }
        if (rc_p8 == 0 && (uintptr_t)port_8ch >= 0x10000UL) {
            rc_pdest8 = fn_port_destroy(port_8ch);
        }
    }
    obs_report_measure(id, "PortDestroy-2ch", "rc", (uint64_t)(uint32_t)rc_pdest2,
                       "code");
    obs_report_measure(id, "PortDestroy-8ch", "rc", (uint64_t)(uint32_t)rc_pdest8,
                       "code");

    if (fn_ctx_destroy && obs_address_is_callable((const void *)fn_ctx_destroy)) {
        if (rc_ctx_create == 0 && (uintptr_t)context_out >= 0x10000UL) {
            rc_cdest = fn_ctx_destroy(context_out);
        }
    }
    obs_report_measure(id, "ContextDestroy", "rc", (uint64_t)(uint32_t)rc_cdest,
                       "code");

    if (fn_user_destroy && obs_address_is_callable((const void *)fn_user_destroy)) {
        if (rc_user_ff == 0 && (uintptr_t)user_ff >= 0x10000UL) {
            rc_udest = fn_user_destroy(user_ff);
        }
    }
    obs_report_measure(id, "UserDestroy-0xff", "rc", (uint64_t)(uint32_t)rc_udest,
                       "code");

    /* (5) the same with the initial user, rc, for comparison */
    void *user_initial = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_user_init = -1;
    if (fn_create && obs_address_is_callable(fn_create)) {
        typedef int (*user_create_fn)(int32_t, void **);
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user_init = ((user_create_fn)fn_create)(user, &user_initial);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "UserCreate-initial", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "UserCreate-initial", "rc", (uint64_t)(uint32_t)rc_user_init,
                       "code");
    obs_report_measure(id, "UserCreate-initial", "user",
                       (uint64_t)(uintptr_t)user_initial, "addr");
    if (rc_user_init == 0 && user_initial != NULL &&
        (uintptr_t)user_initial >= 0x10000UL) {
        const void *fn_destroy = (const void *)&sceAudioOut2UserDestroy_static;
        if (!obs_address_is_callable(fn_destroy)) {
            fn_destroy = (const void *)&sceAudioOut2UserDestroy_alt;
        }
        if (fn_destroy && obs_address_is_callable(fn_destroy)) {
            ((int (*)(void *))fn_destroy)(user_initial);
        }
    }

    return obs_pass();
}

static obs_result check_write_rejects_bad_descriptor(void) {
    /* The first negative check, and the shape most of this program uses: an
     * implementation that returns success for an obviously invalid argument has not
     * implemented the function, it has implemented a constant. */
    static const char probe[] = "x";
    sce_ssize_t n = sceKernelWrite(OBS_FD_INVALID, probe, 1);
    if (n >= 0) {
        return obs_partial_value("writing to an invalid descriptor reported success",
                                 (uint64_t)n);
    }
    return obs_pass_value((uint64_t)n);
}

/* Reads the stack pointer as the caller of this function saw it.
 *
 * `__builtin_frame_address(0)` is the frame base of this function. On x86-64 SysV that
 * is a known distance from the stack pointer at entry, but the distance depends on what
 * the compiler did with the frame - so the value is not used absolutely, only for its
 * alignment, which the ABI fixes regardless. */
static uintptr_t obs_frame_alignment(void) {
    return (uintptr_t)__builtin_frame_address(0) & 0x0Fu;
}

static obs_result check_stack_alignment(void) {
    /* # Why this is worth a check of its own
     *
     * x86-64 SysV requires the stack to be sixteen-byte aligned at a `call`, which
     * leaves `rsp % 16 == 8` on entry to the callee once the return address is pushed.
     * Compiled code assumes it, and uses the assumption to place aligned spills - so a
     * violation does not fail immediately. It fails the first time something spills a
     * vector register, which may be in a completely different function, long
     * afterwards, and only once the implementation is large enough to want one.
     *
     * That makes it invisible until it is expensive. The orbistoun side found exactly
     * this shape: 370 of 372 guest calls arriving misaligned for the entire life of the
     * project, undetected until an implementation grew big enough to hit it.
     *
     * **No loader passes this by accident**, and a loader that jumps to an entry point
     * rather than calling one gets it wrong without any other symptom.
     *
     * # What it can and cannot see
     *
     * It reads its own frame, so it measures the alignment this program is running with
     * - which is what a platform function called from here will inherit. It cannot see
     * how a *callback* is entered, so a loader that gets ordinary calls right and
     * callbacks wrong would pass. Stated rather than implied. */
    uintptr_t offset = obs_frame_alignment();

    /* A frame base is sixteen-byte aligned in a correctly entered function, whatever
     * the compiler chose to do above it. Anything else means the stack was already
     * skewed before this program started running. */
    if (offset != 0) {
        return obs_fail_code("the stack is not sixteen-byte aligned", (uint64_t)offset);
    }
    return obs_pass();
}

#if defined(OBS_FAULT_SELFTEST)
/* Proves the fault guard, on any build. It dereferences a deliberately bad pointer; if
 * the guard works the harness records this as a crash and the suite runs on, and if it
 * does not the run ends here - which is itself the answer. Off by default: it is a test
 * of the harness, not a measurement of the platform. Build with `make host
 * FAULT_SELFTEST=1`. */
static obs_result check_fault_guard_selftest(void) {
    volatile int *p = (volatile int *)0x8; /* below OBS_LOWEST_CALLABLE, never mapped */
    int v = *p;                            /* faults; the guard should catch it */
    return obs_pass_value(
        (uint64_t)(unsigned int)v); /* only reached if it did not fault */
}
#endif

/* REQ-20261011T1640Z-us01: Unity User Service getters in fresh process */
static obs_result check_unity_user_service_chain(void) {
    const char *id = "000-boot/unity-user-service-chain";
    obs_jmp_buf guard;

    typedef int (*fn_u_init_t)(const void *);
    typedef int (*fn_u_get_init_user_t)(int32_t *);
    typedef int (*fn_u_get_name_t)(int32_t, char *, size_t);
    typedef int (*fn_u_get_age_t)(int32_t, int32_t *);
    typedef int (*fn_u_get_presets_t)(int32_t, void *);
    typedef int (*fn_u_get_acc_t)(int32_t, void *);

    fn_u_init_t fn_init = NULL;
    int is_link_init = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceInitialize_static)) {
        fn_init = (fn_u_init_t)&sceUserServiceInitialize_static;
        is_link_init = 1;
    } else if (obs_address_is_callable((const void *)&sceUserServiceInitialize)) {
        fn_init = (fn_u_init_t)&sceUserServiceInitialize;
        is_link_init = 1;
    }
    int bound_init =
        (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceUserServiceInitialize", "bound", (uint64_t)bound_init,
                       "bool");
    if (bound_init) {
        obs_report_measure(id, "sceUserServiceInitialize", "how",
                           (uint64_t)(is_link_init ? 1 : 2),
                           is_link_init ? "link" : "dlsym");
    }

    fn_u_get_init_user_t fn_get_init_user = NULL;
    int is_link_get_init_user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        fn_get_init_user = (fn_u_get_init_user_t)&sceUserServiceGetInitialUser_static;
        is_link_get_init_user = 1;
    } else if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        fn_get_init_user = (fn_u_get_init_user_t)&sceUserServiceGetInitialUser;
        is_link_get_init_user = 1;
    }
    int bound_get_init_user = (fn_get_init_user != NULL &&
                               obs_address_is_callable((const void *)fn_get_init_user));
    obs_report_measure(id, "sceUserServiceGetInitialUser", "bound",
                       (uint64_t)bound_get_init_user, "bool");
    if (bound_get_init_user) {
        obs_report_measure(id, "sceUserServiceGetInitialUser", "how",
                           (uint64_t)(is_link_get_init_user ? 1 : 2),
                           is_link_get_init_user ? "link" : "dlsym");
    }

    fn_u_get_name_t fn_get_name = NULL;
    int is_link_get_name = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetUserName_static)) {
        fn_get_name = (fn_u_get_name_t)&sceUserServiceGetUserName_static;
        is_link_get_name = 1;
    } else if (obs_address_is_callable((const void *)&sceUserServiceGetUserName)) {
        fn_get_name = (fn_u_get_name_t)&sceUserServiceGetUserName;
        is_link_get_name = 1;
    }
    int bound_get_name =
        (fn_get_name != NULL && obs_address_is_callable((const void *)fn_get_name));
    obs_report_measure(id, "sceUserServiceGetUserName", "bound",
                       (uint64_t)bound_get_name, "bool");
    if (bound_get_name) {
        obs_report_measure(id, "sceUserServiceGetUserName", "how",
                           (uint64_t)(is_link_get_name ? 1 : 2),
                           is_link_get_name ? "link" : "dlsym");
    }

    fn_u_get_age_t fn_get_age = NULL;
    int is_link_get_age = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetAgeLevel_static)) {
        fn_get_age = (fn_u_get_age_t)&sceUserServiceGetAgeLevel_static;
        is_link_get_age = 1;
    } else if (obs_address_is_callable((const void *)&sceUserServiceGetAgeLevel)) {
        fn_get_age = (fn_u_get_age_t)&sceUserServiceGetAgeLevel;
        is_link_get_age = 1;
    }
    int bound_get_age =
        (fn_get_age != NULL && obs_address_is_callable((const void *)fn_get_age));
    obs_report_measure(id, "sceUserServiceGetAgeLevel", "bound",
                       (uint64_t)bound_get_age, "bool");
    if (bound_get_age) {
        obs_report_measure(id, "sceUserServiceGetAgeLevel", "how",
                           (uint64_t)(is_link_get_age ? 1 : 2),
                           is_link_get_age ? "link" : "dlsym");
    }

    fn_u_get_presets_t fn_get_presets = NULL;
    int is_link_get_presets = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetGamePresets_static)) {
        fn_get_presets = (fn_u_get_presets_t)&sceUserServiceGetGamePresets_static;
        is_link_get_presets = 1;
    }
    int bound_get_presets = (fn_get_presets != NULL &&
                             obs_address_is_callable((const void *)fn_get_presets));
    obs_report_measure(id, "sceUserServiceGetGamePresets", "bound",
                       (uint64_t)bound_get_presets, "bool");
    if (bound_get_presets) {
        obs_report_measure(id, "sceUserServiceGetGamePresets", "how",
                           (uint64_t)(is_link_get_presets ? 1 : 2),
                           is_link_get_presets ? "link" : "dlsym");
    }

    fn_u_get_acc_t fn_chat = NULL;
    int is_link_chat = 0;
    if (obs_address_is_callable(
            (const void *)&sceUserServiceGetAccessibilityChatTranscription_static)) {
        fn_chat =
            (fn_u_get_acc_t)&sceUserServiceGetAccessibilityChatTranscription_static;
        is_link_chat = 1;
    }
    int bound_chat =
        (fn_chat != NULL && obs_address_is_callable((const void *)fn_chat));
    obs_report_measure(id, "sceUserServiceGetAccessibilityChatTranscription", "bound",
                       (uint64_t)bound_chat, "bool");
    if (bound_chat) {
        obs_report_measure(id, "sceUserServiceGetAccessibilityChatTranscription", "how",
                           (uint64_t)(is_link_chat ? 1 : 2),
                           is_link_chat ? "link" : "dlsym");
    }

    fn_u_get_acc_t fn_delay = NULL;
    int is_link_delay = 0;
    if (obs_address_is_callable(
            (const void *)&sceUserServiceGetAccessibilityPressAndHoldDelay_static)) {
        fn_delay =
            (fn_u_get_acc_t)&sceUserServiceGetAccessibilityPressAndHoldDelay_static;
        is_link_delay = 1;
    }
    int bound_delay =
        (fn_delay != NULL && obs_address_is_callable((const void *)fn_delay));
    obs_report_measure(id, "sceUserServiceGetAccessibilityPressAndHoldDelay", "bound",
                       (uint64_t)bound_delay, "bool");
    if (bound_delay) {
        obs_report_measure(id, "sceUserServiceGetAccessibilityPressAndHoldDelay", "how",
                           (uint64_t)(is_link_delay ? 1 : 2),
                           is_link_delay ? "link" : "dlsym");
    }

    fn_u_get_acc_t fn_trigger = NULL;
    int is_link_trigger = 0;
    if (obs_address_is_callable(
            (const void *)&sceUserServiceGetAccessibilityTriggerEffect_static)) {
        fn_trigger =
            (fn_u_get_acc_t)&sceUserServiceGetAccessibilityTriggerEffect_static;
        is_link_trigger = 1;
    }
    int bound_trigger =
        (fn_trigger != NULL && obs_address_is_callable((const void *)fn_trigger));
    obs_report_measure(id, "sceUserServiceGetAccessibilityTriggerEffect", "bound",
                       (uint64_t)bound_trigger, "bool");
    if (bound_trigger) {
        obs_report_measure(id, "sceUserServiceGetAccessibilityTriggerEffect", "how",
                           (uint64_t)(is_link_trigger ? 1 : 2),
                           is_link_trigger ? "link" : "dlsym");
    }

    fn_u_get_acc_t fn_vib = NULL;
    int is_link_vib = 0;
    if (obs_address_is_callable(
            (const void *)&sceUserServiceGetAccessibilityVibration_static)) {
        fn_vib = (fn_u_get_acc_t)&sceUserServiceGetAccessibilityVibration_static;
        is_link_vib = 1;
    }
    int bound_vib = (fn_vib != NULL && obs_address_is_callable((const void *)fn_vib));
    obs_report_measure(id, "sceUserServiceGetAccessibilityVibration", "bound",
                       (uint64_t)bound_vib, "bool");
    if (bound_vib) {
        obs_report_measure(id, "sceUserServiceGetAccessibilityVibration", "how",
                           (uint64_t)(is_link_vib ? 1 : 2),
                           is_link_vib ? "link" : "dlsym");
    }

    int sig = 0;

    /* 1. sceUserServiceInitialize(NULL) */
    int rc_init = -1;
    if (bound_init) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_init(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "init", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "init", "rc", (uint64_t)(uint32_t)rc_init, "code");
    }

    /* 2. sceUserServiceGetInitialUser(&user) */
    int32_t user = -1;
    int rc_user = -1;
    if (bound_get_init_user) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user = fn_get_init_user(&user);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-initial-user", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "get-initial-user", "rc", (uint64_t)(uint32_t)rc_user,
                           "code");
        obs_report_measure(id, "get-initial-user", "user", (uint64_t)(uint32_t)user,
                           "id");
    }

    /* 3. sceUserServiceGetUserName(user, buf, 0x11) */
    if (bound_get_name) {
        char name_buf[0x20];
        memset(name_buf, 0xa5, sizeof(name_buf));
        uint8_t before[0x20];
        memset(before, 0xa5, sizeof(before));
        int rc_name = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_name = fn_get_name(user, name_buf, 0x11);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-user-name", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-user-name", "rc", (uint64_t)(uint32_t)rc_name,
                           "code");
        obs_report_written(id, "get-user-name", "bytes-changed", before,
                           (const unsigned char *)name_buf, 0x11);
        obs_report_bytes(id, "get-user-name", "name-buf-17b", 0,
                         (const unsigned char *)name_buf, 0x11);
    }

    /* 4. sceUserServiceGetAgeLevel(user, &out) */
    if (bound_get_age) {
        uint8_t age_buf[16];
        memset(age_buf, 0xa5, sizeof(age_buf));
        uint8_t before[16];
        memset(before, 0xa5, sizeof(before));
        int rc_age = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_age = fn_get_age(user, (int32_t *)(void *)age_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-age-level", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-age-level", "rc", (uint64_t)(uint32_t)rc_age,
                           "code");
        obs_report_written(id, "get-age-level", "bytes-changed", before, age_buf,
                           sizeof(age_buf));
        obs_report_measure(id, "get-age-level", "val",
                           (uint64_t)*(const uint32_t *)(const void *)age_buf, "val");
    }

    /* 5. sceUserServiceGetGamePresets(user, rec) (rec u64 0x28 then 0s) */
    if (bound_get_presets) {
        uint8_t rec[0x40];
        memset(rec, 0, sizeof(rec));
        *(uint64_t *)(void *)rec = 0x28;
        uint8_t before[0x40];
        memcpy(before, rec, sizeof(rec));
        int rc_presets = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_presets = fn_get_presets(user, rec);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-game-presets", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "get-game-presets", "rc", (uint64_t)(uint32_t)rc_presets,
                           "code");
        obs_report_written(id, "get-game-presets", "bytes-changed", before, rec,
                           sizeof(rec));
        obs_report_bytes(id, "get-game-presets", "rec-after-40h", 0, rec, sizeof(rec));
    }

    /* 6. sceUserServiceGetAccessibilityChatTranscription(user, &out) */
    if (bound_chat) {
        uint8_t chat_buf[0x40];
        memset(chat_buf, 0xa5, sizeof(chat_buf));
        uint8_t before[0x40];
        memset(before, 0xa5, sizeof(before));
        int rc_chat = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_chat = fn_chat(user, chat_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-accessibility-chat-transcription", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-accessibility-chat-transcription", "rc",
                           (uint64_t)(uint32_t)rc_chat, "code");
        obs_report_written(id, "get-accessibility-chat-transcription", "bytes-changed",
                           before, chat_buf, sizeof(chat_buf));
    }

    /* 7. sceUserServiceGetAccessibilityPressAndHoldDelay(user, &out) */
    if (bound_delay) {
        uint8_t delay_buf[0x40];
        memset(delay_buf, 0xa5, sizeof(delay_buf));
        uint8_t before[0x40];
        memset(before, 0xa5, sizeof(before));
        int rc_delay = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_delay = fn_delay(user, delay_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-accessibility-press-and-hold-delay", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-accessibility-press-and-hold-delay", "rc",
                           (uint64_t)(uint32_t)rc_delay, "code");
        obs_report_written(id, "get-accessibility-press-and-hold-delay",
                           "bytes-changed", before, delay_buf, sizeof(delay_buf));
    }

    /* 8. sceUserServiceGetAccessibilityTriggerEffect(user, &out) */
    if (bound_trigger) {
        uint8_t trig_buf[0x40];
        memset(trig_buf, 0xa5, sizeof(trig_buf));
        uint8_t before[0x40];
        memset(before, 0xa5, sizeof(before));
        int rc_trig = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_trig = fn_trigger(user, trig_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-accessibility-trigger-effect", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-accessibility-trigger-effect", "rc",
                           (uint64_t)(uint32_t)rc_trig, "code");
        obs_report_written(id, "get-accessibility-trigger-effect", "bytes-changed",
                           before, trig_buf, sizeof(trig_buf));
    }

    /* 9. sceUserServiceGetAccessibilityVibration(user, &out) */
    if (bound_vib) {
        uint8_t vib_buf[0x40];
        memset(vib_buf, 0xa5, sizeof(vib_buf));
        uint8_t before[0x40];
        memset(before, 0xa5, sizeof(before));
        int rc_vib = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vib = fn_vib(user, vib_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-accessibility-vibration", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "get-accessibility-vibration", "rc",
                           (uint64_t)(uint32_t)rc_vib, "code");
        obs_report_written(id, "get-accessibility-vibration", "bytes-changed", before,
                           vib_buf, sizeof(vib_buf));
    }

    /* 10. sceUserServiceGetAgeLevel once more for user 0xff */
    if (bound_get_age) {
        uint8_t age_buf_ff[16];
        memset(age_buf_ff, 0xa5, sizeof(age_buf_ff));
        uint8_t before_ff[16];
        memset(before_ff, 0xa5, sizeof(before_ff));
        int rc_age_ff = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_age_ff = fn_get_age((int32_t)0xff, (int32_t *)(void *)age_buf_ff);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "get-age-level-user-0xff", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "get-age-level-user-0xff", "rc",
                           (uint64_t)(uint32_t)rc_age_ff, "code");
        obs_report_written(id, "get-age-level-user-0xff", "bytes-changed", before_ff,
                           age_buf_ff, sizeof(age_buf_ff));
        obs_report_measure(id, "get-age-level-user-0xff", "val",
                           (uint64_t)*(const uint32_t *)(const void *)age_buf_ff,
                           "val");
    }

    return obs_pass();
}

/* REQ-20261011T1745Z-np03: Np/AppContent startup calls in fresh process */
static volatile int s_trophy2_cb_ran = 0;
static void np03_trophy_cb(void *a0, void *a1, void *a2);
static void np03_trophy_cb(void *a0, void *a1, void *a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    s_trophy2_cb_ran = 1;
}

static obs_result check_np_appcontent_startup(void) {
    const char *id = "000-boot/np-appcontent-startup";
    obs_jmp_buf guard;

    /* Load sysmodules */
    static const uint16_t np_sysmods[] = {0x0008, 0x009d, 0x0105, 0x0033, 0x0120,
                                          0x003b, 0x011b, 0x0016, 0x00e7};
    for (size_t i = 0; i < sizeof(np_sysmods) / sizeof(np_sysmods[0]); i++) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(np_sysmods[i]);
        }
        (void)obs_sysmodule_load_id(np_sysmods[i]);
    }

    /* Also load SPRXs if callable */
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        (void)sceKernelLoadStartModule(
            "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0, NULL,
            NULL);
        (void)sceKernelLoadStartModule(
            "/system/common/lib/libSceNpSessionSignaling.sprx", 0, NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule(
            "/system/common/lib/libSceNpUniversalDataSystem.sprx", 0, NULL, 0, NULL,
            NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceAppContent.sprx", 0,
                                       NULL, 0, NULL, NULL);
        (void)sceKernelLoadStartModule("/system/common/lib/libSceJson2.sprx", 0, NULL,
                                       0, NULL, NULL);
    }

    /* Resolvers */
    typedef int (*fn_ea_init_t)(const void *, void *);
    typedef int (*fn_ss_init_t)(const void *, const void *, uint64_t, uint64_t);
    typedef int (*fn_tr_create_t)(uint64_t *, int32_t, uint64_t, uint64_t);
    typedef int (*fn_uds_create_t)(uint64_t *, int32_t, uint64_t, uint64_t);
    typedef int (*fn_tr_cb_t)(uint64_t, uint64_t, void *, void *);
    typedef int (*fn_get_acc_id_t)(int32_t, void *);
    typedef int (*fn_app_init_t)(void *);
    typedef int (*fn_app_space_t)(const char *, void *);
    typedef void (*fn_json_set_size_t)(void *, size_t);
    typedef void (*fn_json_ctor_t)(void *);

    fn_ea_init_t fn_ea =
        obs_address_is_callable((const void *)&sceNpEntitlementAccessInitialize_be)
            ? (fn_ea_init_t)&sceNpEntitlementAccessInitialize_be
            : (obs_address_is_callable(
                   (const void *)&sceNpEntitlementAccessInitialize_static)
                   ? (fn_ea_init_t)&sceNpEntitlementAccessInitialize_static
                   : NULL);
    int is_link_ea = (fn_ea != NULL);

    fn_ss_init_t fn_ss =
        obs_address_is_callable((const void *)&sceNpSessionSignalingInitialize_be)
            ? (fn_ss_init_t)&sceNpSessionSignalingInitialize_be
            : (obs_address_is_callable(
                   (const void *)&sceNpSessionSignalingInitialize_static)
                   ? (fn_ss_init_t)&sceNpSessionSignalingInitialize_static
                   : NULL);
    int is_link_ss = (fn_ss != NULL);

    fn_tr_create_t fn_tr_ctx =
        obs_address_is_callable((const void *)&sceNpTrophy2CreateContext_static)
            ? (fn_tr_create_t)&sceNpTrophy2CreateContext_static
            : NULL;
    int is_link_tr_ctx = (fn_tr_ctx != NULL);

    fn_uds_create_t fn_uds_ctx =
        obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemCreateContext_static)
            ? (fn_uds_create_t)&sceNpUniversalDataSystemCreateContext_static
            : NULL;
    int is_link_uds_ctx = (fn_uds_ctx != NULL);

    fn_tr_cb_t fn_tr_cb =
        obs_address_is_callable((const void *)&sceNpTrophy2RegisterUnlockCallback_be)
            ? (fn_tr_cb_t)&sceNpTrophy2RegisterUnlockCallback_be
            : (obs_address_is_callable(
                   (const void *)&sceNpTrophy2RegisterUnlockCallback_static)
                   ? (fn_tr_cb_t)&sceNpTrophy2RegisterUnlockCallback_static
                   : NULL);
    int is_link_tr_cb = (fn_tr_cb != NULL);

    fn_get_acc_id_t fn_acc_id =
        obs_address_is_callable((const void *)&sceNpGetAccountIdA_static)
            ? (fn_get_acc_id_t)&sceNpGetAccountIdA_static
            : NULL;
    int is_link_acc_id = (fn_acc_id != NULL);

    fn_app_init_t fn_app_init =
        obs_address_is_callable((const void *)&sceAppContentInitialize_static)
            ? (fn_app_init_t)&sceAppContentInitialize_static
            : NULL;

    fn_app_space_t fn_app_space =
        obs_address_is_callable(
            (const void *)&sceAppContentDownloadDataGetAvailableSpaceKb_static)
            ? (fn_app_space_t)&sceAppContentDownloadDataGetAvailableSpaceKb_static
            : NULL;
    int is_link_app_space = (fn_app_space != NULL);

    fn_json_set_size_t fn_json_set_size =
        obs_address_is_callable(
            (const void *)&sceJsonInitParameter2SetFileBufferSize_static)
            ? (fn_json_set_size_t)&sceJsonInitParameter2SetFileBufferSize_static
            : (obs_address_is_callable(
                   (const void *)&sceJsonInitParameter2SetFileBufferSize_named)
                   ? (fn_json_set_size_t)&sceJsonInitParameter2SetFileBufferSize_named
                   : NULL);
    int is_link_json_set_size = (fn_json_set_size != NULL);

    fn_json_ctor_t fn_json_ctor =
        obs_address_is_callable((const void *)&sceJsonInitParameter2Ctor_static)
            ? (fn_json_ctor_t)&sceJsonInitParameter2Ctor_static
            : (obs_address_is_callable((const void *)&sceJsonInitParameter2Ctor_named)
                   ? (fn_json_ctor_t)&sceJsonInitParameter2Ctor_named
                   : NULL);

    /* Report bound / how for every function */
    int bound_ea = (fn_ea != NULL);
    obs_report_measure(id, "sceNpEntitlementAccessInitialize", "bound",
                       (uint64_t)bound_ea, "bool");
    if (bound_ea) {
        obs_report_measure(id, "sceNpEntitlementAccessInitialize", "how",
                           (uint64_t)(is_link_ea ? 1 : 2),
                           is_link_ea ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpEntitlementAccessInitialize", "status", 0,
                           "not-possible");
    }

    int bound_ss = (fn_ss != NULL);
    obs_report_measure(id, "sceNpSessionSignalingInitialize", "bound",
                       (uint64_t)bound_ss, "bool");
    if (bound_ss) {
        obs_report_measure(id, "sceNpSessionSignalingInitialize", "how",
                           (uint64_t)(is_link_ss ? 1 : 2),
                           is_link_ss ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpSessionSignalingInitialize", "status", 0,
                           "not-possible");
    }

    int bound_tr_ctx = (fn_tr_ctx != NULL);
    obs_report_measure(id, "sceNpTrophy2CreateContext", "bound", (uint64_t)bound_tr_ctx,
                       "bool");
    if (bound_tr_ctx) {
        obs_report_measure(id, "sceNpTrophy2CreateContext", "how",
                           (uint64_t)(is_link_tr_ctx ? 1 : 2),
                           is_link_tr_ctx ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpTrophy2CreateContext", "status", 0,
                           "not-possible");
    }

    int bound_uds_ctx = (fn_uds_ctx != NULL);
    obs_report_measure(id, "sceNpUniversalDataSystemCreateContext", "bound",
                       (uint64_t)bound_uds_ctx, "bool");
    if (bound_uds_ctx) {
        obs_report_measure(id, "sceNpUniversalDataSystemCreateContext", "how",
                           (uint64_t)(is_link_uds_ctx ? 1 : 2),
                           is_link_uds_ctx ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpUniversalDataSystemCreateContext", "status", 0,
                           "not-possible");
    }

    int bound_tr_cb = (fn_tr_cb != NULL);
    obs_report_measure(id, "sceNpTrophy2RegisterUnlockCallback", "bound",
                       (uint64_t)bound_tr_cb, "bool");
    if (bound_tr_cb) {
        obs_report_measure(id, "sceNpTrophy2RegisterUnlockCallback", "how",
                           (uint64_t)(is_link_tr_cb ? 1 : 2),
                           is_link_tr_cb ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpTrophy2RegisterUnlockCallback", "status", 0,
                           "not-possible");
    }

    int bound_acc_id = (fn_acc_id != NULL);
    obs_report_measure(id, "sceNpGetAccountIdA", "bound", (uint64_t)bound_acc_id,
                       "bool");
    if (bound_acc_id) {
        obs_report_measure(id, "sceNpGetAccountIdA", "how",
                           (uint64_t)(is_link_acc_id ? 1 : 2),
                           is_link_acc_id ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpGetAccountIdA", "status", 0, "not-possible");
    }

    int bound_app_space = (fn_app_space != NULL);
    obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "bound",
                       (uint64_t)bound_app_space, "bool");
    if (bound_app_space) {
        obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "how",
                           (uint64_t)(is_link_app_space ? 1 : 2),
                           is_link_app_space ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "status",
                           0, "not-possible");
    }

    int bound_json = (fn_json_set_size != NULL);
    obs_report_measure(id, "sce::Json::InitParameter2::setFileBufferSize", "bound",
                       (uint64_t)bound_json, "bool");
    if (bound_json) {
        obs_report_measure(id, "sce::Json::InitParameter2::setFileBufferSize", "how",
                           (uint64_t)(is_link_json_set_size ? 1 : 2),
                           is_link_json_set_size ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sce::Json::InitParameter2::setFileBufferSize", "status",
                           0, "not-possible");
    }

    /* Initial user */
    int32_t user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        (void)sceUserServiceGetInitialUser_static(&user);
    } else if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        (void)sceUserServiceGetInitialUser(&user);
    }

    int sig = 0;

    /* 1. sceNpEntitlementAccessInitialize(param, out) */
    if (bound_ea) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "entitlement-access-call%d", iter);
            uint8_t param[0x40];
            memset(param, 0, sizeof(param));
            if (iter == 2) {
                *(uint64_t *)(void *)param = 0x40;
            }
            uint8_t out[0x40];
            memset(out, 0xa5, sizeof(out));
            uint8_t param_before[0x40], out_before[0x40];
            memcpy(param_before, param, sizeof(param));
            memcpy(out_before, out, sizeof(out));

            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_ea(param, out);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_written(id, tag, "param-bytes-changed", param_before, param,
                               sizeof(param));
            obs_report_written(id, tag, "out-bytes-changed", out_before, out,
                               sizeof(out));
        }
    }

    /* 1b. sceNpSessionSignalingInitialize(param, out) */
    if (bound_ss) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "session-signaling-call%d", iter);
            uint8_t param[0x40];
            memset(param, 0, sizeof(param));
            if (iter == 2) {
                *(uint64_t *)(void *)param = 0x40;
            }
            uint8_t out[0x40];
            memset(out, 0xa5, sizeof(out));
            uint8_t param_before[0x40], out_before[0x40];
            memcpy(param_before, param, sizeof(param));
            memcpy(out_before, out, sizeof(out));

            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_ss(param, out, 0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_written(id, tag, "param-bytes-changed", param_before, param,
                               sizeof(param));
            obs_report_written(id, tag, "out-bytes-changed", out_before, out,
                               sizeof(out));
        }
    }

    /* 2a. sceNpTrophy2CreateContext(&ctx, user, 0, 0) */
    uint64_t last_tr_ctx = 0;
    if (bound_tr_ctx) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "trophy2-create-context-call%d", iter);
            uint64_t ctx = 0xa5a5a5a5a5a5a5a5ULL;
            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_tr_ctx(&ctx, user, 0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure(id, tag, "ctx", ctx, "handle");
            if (rc == 0)
                last_tr_ctx = ctx;
        }
    }

    /* 2b. sceNpUniversalDataSystemCreateContext(&ctx, user, 0, 0) */
    if (bound_uds_ctx) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "uds-create-context-call%d", iter);
            uint64_t ctx = 0xa5a5a5a5a5a5a5a5ULL;
            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_uds_ctx(&ctx, user, 0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure(id, tag, "ctx", ctx, "handle");
        }
    }

    /* 3. sceNpTrophy2RegisterUnlockCallback(ctx, 0, fn, arg) on that context */
    if (bound_tr_cb) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "trophy2-register-callback-call%d", iter);
            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_tr_cb(last_tr_ctx, 0, (void *)np03_trophy_cb, (void *)0x1234);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
        }
    }

    /* 4. sceNpGetAccountIdA(user, &out) - the 8 bytes written */
    if (bound_acc_id) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "get-account-id-call%d", iter);
            uint8_t out[8];
            memset(out, 0xa5, sizeof(out));
            uint8_t before[8];
            memset(before, 0xa5, sizeof(before));
            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_acc_id(user, out);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_written(id, tag, "out-bytes-changed", before, out, sizeof(out));
            obs_report_measure(id, tag, "account-id",
                               *(const uint64_t *)(const void *)out, "id");
        }
    }

    /* 5. sceAppContentDownloadDataGetAvailableSpaceKb(mountPoint, &kb) after
     * sceAppContentInitialize */
    if (fn_app_init && obs_address_is_callable((const void *)fn_app_init)) {
        uint8_t app_init_param[64];
        memset(app_init_param, 0, sizeof(app_init_param));
        (void)fn_app_init(app_init_param);
    }
    if (bound_app_space) {
        for (int iter = 1; iter <= 2; iter++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "appcontent-space-call%d", iter);
            uint8_t kb[0x40];
            memset(kb, 0xa5, sizeof(kb));
            uint8_t before[0x40];
            memset(before, 0xa5, sizeof(before));
            int rc = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_app_space("/download0", kb);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_written(id, tag, "kb-bytes-changed", before, kb, sizeof(kb));
            obs_report_measure(id, tag, "kb-val", *(const uint64_t *)(const void *)kb,
                               "kb");
        }
    }

    /* 6. sce::Json::InitParameter2::setFileBufferSize(param, 0x1000) */
    if (bound_json) {
        /* Call 1: constructed (ctor called on 0xcc-filled block) */
        {
            const char *tag = "json-set-file-buffer-size-call1";
            uint8_t param[0x40];
            memset(param, 0xcc, sizeof(param));
            if (fn_json_ctor) {
                fn_json_ctor(param);
            }
            uint8_t before[0x40];
            memcpy(before, param, sizeof(param));
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                fn_json_set_size(param, 0x1000);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_written(id, tag, "param-bytes-changed", before, param,
                               sizeof(param));
            obs_report_bytes(id, tag, "param-after-40h", 0, param, sizeof(param));
        }

        /* Call 2: direct 0xcc-filled block */
        {
            const char *tag = "json-set-file-buffer-size-call2";
            uint8_t param[0x40];
            memset(param, 0xcc, sizeof(param));
            uint8_t before[0x40];
            memcpy(before, param, sizeof(param));
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                fn_json_set_size(param, 0x1000);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_written(id, tag, "param-bytes-changed", before, param,
                               sizeof(param));
            obs_report_bytes(id, tag, "param-after-40h", 0, param, sizeof(param));
        }
    }

    return obs_pass();
}

static int32_t get_boot_user(void) {
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        if (sceUserServiceGetInitialUser_static(&user) != 0) {
            user = -1;
        }
    } else if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0) {
            user = -1;
        }
    }
    if (user < 0) {
        user = 0x1ea2f4d9;
    }
    return user;
}

/* REQ-20261012T0020Z-ce01: ContentExport init2 & term */
struct ce_cb_log {
    int is_free;
    uint64_t arg;
    uint64_t ret;
};
static struct ce_cb_log s_ce_logs[64];
static size_t s_ce_log_count = 0;
static _Alignas(16) uint8_t s_ce_alloc_pool[0x100000];
static size_t s_ce_pool_offset = 0;

static void *ce_alloc_cb(size_t size) {
    size_t align_sz = (size + 15) & ~15ULL;
    void *res = NULL;
    if (s_ce_pool_offset + align_sz <= sizeof(s_ce_alloc_pool)) {
        res = &s_ce_alloc_pool[s_ce_pool_offset];
        s_ce_pool_offset += align_sz;
    }
    if (s_ce_log_count < 64) {
        s_ce_logs[s_ce_log_count].is_free = 0;
        s_ce_logs[s_ce_log_count].arg = size;
        s_ce_logs[s_ce_log_count].ret = (uint64_t)(uintptr_t)res;
        s_ce_log_count++;
    }
    return res;
}

static void ce_free_cb(void *ptr) {
    if (s_ce_log_count < 64) {
        s_ce_logs[s_ce_log_count].is_free = 1;
        s_ce_logs[s_ce_log_count].arg = (uint64_t)(uintptr_t)ptr;
        s_ce_logs[s_ce_log_count].ret = 0;
        s_ce_log_count++;
    }
}

static obs_result check_content_export_init(void) {
    const char *id = "000-boot/content-export-init";
    obs_jmp_buf guard;

    typedef int (*fn_ce_init2_t)(void *);
    typedef int (*fn_ce_term_t)(void);

    fn_ce_init2_t fn_init2 = NULL;
    int is_link_init2 = 0;
    if (obs_address_is_callable((const void *)&sceContentExportInit2_static)) {
        fn_init2 = (fn_ce_init2_t)&sceContentExportInit2_static;
        is_link_init2 = 1;
    }
    if (!fn_init2)
        fn_init2 = (fn_ce_init2_t)obs_module_symbol(OBS_HANDLE_SELF, "$0GnN4QCgIfs");
    if (!fn_init2)
        fn_init2 = (fn_ce_init2_t)obs_module_symbol(1, "$0GnN4QCgIfs");
    if (!fn_init2) {
        int h = obs_module_open("libSceContentExport");
        if (h > 0) {
            fn_init2 = (fn_ce_init2_t)obs_module_symbol(h, "$0GnN4QCgIfs");
            if (!fn_init2)
                fn_init2 = (fn_ce_init2_t)obs_module_symbol(h, "sceContentExportInit2");
        }
    }

    fn_ce_term_t fn_term = NULL;
    int is_link_term = 0;
    if (obs_address_is_callable((const void *)&sceContentExportTerm_static)) {
        fn_term = (fn_ce_term_t)&sceContentExportTerm_static;
        is_link_term = 1;
    }
    if (!fn_term)
        fn_term = (fn_ce_term_t)obs_module_symbol(OBS_HANDLE_SELF, "$+KDWny9Y-6k");
    if (!fn_term)
        fn_term = (fn_ce_term_t)obs_module_symbol(1, "$+KDWny9Y-6k");
    if (!fn_term) {
        int h = obs_module_open("libSceContentExport");
        if (h > 0) {
            fn_term = (fn_ce_term_t)obs_module_symbol(h, "$+KDWny9Y-6k");
            if (!fn_term)
                fn_term = (fn_ce_term_t)obs_module_symbol(h, "sceContentExportTerm");
        }
    }

    int bound_init2 =
        (fn_init2 != NULL && obs_address_is_callable((const void *)fn_init2));
    obs_report_measure(id, "sceContentExportInit2", "bound", (uint64_t)bound_init2,
                       "bool");
    if (bound_init2) {
        obs_report_measure(id, "sceContentExportInit2", "how",
                           (uint64_t)(is_link_init2 ? 1 : 2),
                           is_link_init2 ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceContentExportInit2", "status", 0, "not-possible");
    }

    int bound_term =
        (fn_term != NULL && obs_address_is_callable((const void *)fn_term));
    obs_report_measure(id, "sceContentExportTerm", "bound", (uint64_t)bound_term,
                       "bool");
    if (bound_term) {
        obs_report_measure(id, "sceContentExportTerm", "how",
                           (uint64_t)(is_link_term ? 1 : 2),
                           is_link_term ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceContentExportTerm", "status", 0, "not-possible");
    }

    if (!bound_init2) {
        return obs_pass();
    }

    s_ce_log_count = 0;
    s_ce_pool_offset = 0;

    /* 1. Init2 with title block: +00 alloc cb, +08 free cb, +10..+2f 0, +30..+3f
     * cc */
    uint8_t ce_param[0x40];
    memset(ce_param, 0xcc, sizeof(ce_param));
    *(void **)(void *)(ce_param + 0x00) = (void *)&ce_alloc_cb;
    *(void **)(void *)(ce_param + 0x08) = (void *)&ce_free_cb;
    memset(ce_param + 0x10, 0, 0x20);

    size_t before_cnt = s_ce_log_count;
    int rc1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_init2(ce_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "Init2-first", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "Init2-first", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_bytes(id, "Init2-first", "param-40h", 0, ce_param, sizeof(ce_param));
    obs_report_measure(id, "Init2-first", "callbacks",
                       (uint64_t)(s_ce_log_count - before_cnt), "count");
    for (size_t i = before_cnt; i < s_ce_log_count; i++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "Init2-first-cb%zu", i - before_cnt);
        obs_report_measure(id, tag, "is-free", (uint64_t)s_ce_logs[i].is_free, "bool");
        obs_report_measure(id, tag, "arg", s_ce_logs[i].arg, "addr");
        if (!s_ce_logs[i].is_free) {
            obs_report_measure(id, tag, "ret", s_ce_logs[i].ret, "addr");
        }
    }

    /* 2. Same call again */
    before_cnt = s_ce_log_count;
    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_init2(ce_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "Init2-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "Init2-second", "rc", (uint64_t)(uint32_t)rc2, "code");
    obs_report_measure(id, "Init2-second", "callbacks",
                       (uint64_t)(s_ce_log_count - before_cnt), "count");
    for (size_t i = before_cnt; i < s_ce_log_count; i++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "Init2-second-cb%zu", i - before_cnt);
        obs_report_measure(id, tag, "is-free", (uint64_t)s_ce_logs[i].is_free, "bool");
        obs_report_measure(id, tag, "arg", s_ce_logs[i].arg, "addr");
        if (!s_ce_logs[i].is_free) {
            obs_report_measure(id, tag, "ret", s_ce_logs[i].ret, "addr");
        }
    }

    /* 3. Term */
    before_cnt = s_ce_log_count;
    int rc3 = -1;
    if (bound_term) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc3 = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Term", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "Term", "rc", (uint64_t)(uint32_t)rc3, "code");
    obs_report_measure(id, "Term", "callbacks", (uint64_t)(s_ce_log_count - before_cnt),
                       "count");
    for (size_t i = before_cnt; i < s_ce_log_count; i++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "Term-cb%zu", i - before_cnt);
        obs_report_measure(id, tag, "is-free", (uint64_t)s_ce_logs[i].is_free, "bool");
        obs_report_measure(id, tag, "arg", s_ce_logs[i].arg, "addr");
    }

    return obs_pass();
}

/* REQ-20261012T0445Z-av01: AvPlayer lifecycle */
struct av_cb_log {
    uint32_t cb_id;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t ret;
};
static struct av_cb_log s_av_logs[128];
static size_t s_av_log_count = 0;
static _Alignas(64) uint8_t s_av_alloc_pool[4 * 1024 * 1024];
static size_t s_av_pool_offset = 0;

static void *av_alloc_buffer(size_t size, size_t align) {
    if (align < 64)
        align = 64;
    size_t cur = (size_t)(uintptr_t)&s_av_alloc_pool[s_av_pool_offset];
    size_t aligned = (cur + (align - 1)) & ~(align - 1);
    size_t off = aligned - (size_t)(uintptr_t)s_av_alloc_pool;
    if (off + size <= sizeof(s_av_alloc_pool)) {
        s_av_pool_offset = off + size;
        return (void *)(uintptr_t)aligned;
    }
    return NULL;
}

static void *av_cb_0(void *obj, uint64_t a1, uint64_t a2) {
    uint32_t size = (uint32_t)a1;
    uint32_t align = (uint32_t)a2;
    if (a2 > a1 && a1 <= 256) {
        size = (uint32_t)a2;
        align = (uint32_t)a1;
    }
    void *p = av_alloc_buffer(size, align ? align : 32);
    if (s_av_log_count < 128) {
        s_av_logs[s_av_log_count].cb_id = 0;
        s_av_logs[s_av_log_count].a0 = (uint64_t)(uintptr_t)obj;
        s_av_logs[s_av_log_count].a1 = a1;
        s_av_logs[s_av_log_count].a2 = a2;
        s_av_logs[s_av_log_count].ret = (uint64_t)(uintptr_t)p;
        s_av_log_count++;
    }
    return p;
}

static void av_cb_1(void *obj, void *ptr, uint64_t a2) {
    if (s_av_log_count < 128) {
        s_av_logs[s_av_log_count].cb_id = 1;
        s_av_logs[s_av_log_count].a0 = (uint64_t)(uintptr_t)obj;
        s_av_logs[s_av_log_count].a1 = (uint64_t)(uintptr_t)ptr;
        s_av_logs[s_av_log_count].a2 = a2;
        s_av_logs[s_av_log_count].ret = 0;
        s_av_log_count++;
    }
}

static void *av_cb_2(void *obj, uint64_t a1, uint64_t a2) {
    uint32_t size = (uint32_t)a1;
    uint32_t align = (uint32_t)a2;
    if (a2 > a1 && a1 <= 256) {
        size = (uint32_t)a2;
        align = (uint32_t)a1;
    }
    void *p = av_alloc_buffer(size, align ? align : 32);
    if (s_av_log_count < 128) {
        s_av_logs[s_av_log_count].cb_id = 2;
        s_av_logs[s_av_log_count].a0 = (uint64_t)(uintptr_t)obj;
        s_av_logs[s_av_log_count].a1 = a1;
        s_av_logs[s_av_log_count].a2 = a2;
        s_av_logs[s_av_log_count].ret = (uint64_t)(uintptr_t)p;
        s_av_log_count++;
    }
    return p;
}

static void av_cb_3(void *obj, void *ptr, uint64_t a2) {
    if (s_av_log_count < 128) {
        s_av_logs[s_av_log_count].cb_id = 3;
        s_av_logs[s_av_log_count].a0 = (uint64_t)(uintptr_t)obj;
        s_av_logs[s_av_log_count].a1 = (uint64_t)(uintptr_t)ptr;
        s_av_logs[s_av_log_count].a2 = a2;
        s_av_logs[s_av_log_count].ret = 0;
        s_av_log_count++;
    }
}

static struct {
    uint64_t user;
    char msg[128];
} s_av_user_logs[32];
static size_t s_av_user_log_count = 0;

static void av_log_cb(void *user, const char *msg) {
    if (s_av_user_log_count < 32) {
        s_av_user_logs[s_av_user_log_count].user = (uint64_t)(uintptr_t)user;
        if (msg) {
            size_t len = 0;
            while (msg[len] && len < sizeof(s_av_user_logs[0].msg) - 1) {
                s_av_user_logs[s_av_user_log_count].msg[len] = msg[len];
                len++;
            }
            s_av_user_logs[s_av_user_log_count].msg[len] = '\0';
        } else {
            s_av_user_logs[s_av_user_log_count].msg[0] = '\0';
        }
        s_av_user_log_count++;
    }
}

static obs_result check_avplayer_lifecycle(void) {
    const char *id = "000-boot/avplayer-lifecycle";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        obs_jmp_buf g;
        if (OBS_FAULT_ARM(&g) == 0) {
            sceSysmoduleLoadModule(0x00a5);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    int h_av = obs_module_open("libSceAvPlayer");

    static const struct {
        const char *name;
        const char *nid;
        const void *static_fn;
    } av_census[] = {
        {"sceAvPlayerAddSource", "$KMcEa+rHsIo", NULL},
        {"sceAvPlayerAddSourceEx", "$x8uvuFOPZhU",
         (const void *)&sceAvPlayerAddSourceEx_static},
        {"sceAvPlayerChangeStream", "$buMCiJftcfw", NULL},
        {"sceAvPlayerClose", "$NkJwDzKmIlw", (const void *)&sceAvPlayerClose_static},
        {"sceAvPlayerCurrentTime", "$wwM99gjFf1Y", NULL},
        {"sceAvPlayerDisableStream", "$BOVKAzRmuTQ", NULL},
        {"sceAvPlayerEnableStream", "$ODJK2sn9w4A", NULL},
        {"sceAvPlayerGetAudioData", "$Wnp1OVcrZgk", NULL},
        {"sceAvPlayerGetStreamInfo", "$d8FcbzfAdQw", NULL},
        {"sceAvPlayerGetStreamInfoEx", "$ctTAcF5DiKQ", NULL},
        {"sceAvPlayerGetVideoData", "$o3+RWnHViSg", NULL},
        {"sceAvPlayerGetVideoDataEx", "$JdksQu8pNdQ", NULL},
        {"sceAvPlayerInit", "$aS66RI0gGgo", NULL},
        {"sceAvPlayerInitEx", "$o9eWRkSL+M4", (const void *)&sceAvPlayerInitEx_static},
        {"sceAvPlayerIsActive", "$UbQoYawOsfY",
         (const void *)&sceAvPlayerIsActive_static},
        {"sceAvPlayerJumpToTime", "$XC9wM+xULz8", NULL},
        {"sceAvPlayerPause", "$9y5v+fGN4Wk", NULL},
        {"sceAvPlayerPostInit", "$HD1YKVU26-M",
         (const void *)&sceAvPlayerPostInit_static},
        {"sceAvPlayerPrintf", "$agig-iDRrTE", NULL},
        {"sceAvPlayerResume", "$w5moABNwnRY", NULL},
        {"sceAvPlayerSetAvSyncMode", "$k-q+xOxdc3E", NULL},
        {"sceAvPlayerSetLogCallback", "$eBTreZ84JFY",
         (const void *)&sceAvPlayerSetLogCallback_static},
        {"sceAvPlayerSetLooping", "$OVths0xGfho", NULL},
        {"sceAvPlayerSetTrickSpeed", "$av8Z++94rs0", NULL},
        {"sceAvPlayerStart", "$ET4Gr-Uu07s", NULL},
        {"sceAvPlayerStop", "$ZC17w3vB5Lo", (const void *)&sceAvPlayerStop_static},
        {"sceAvPlayerStreamCount", "$hdTyRzCXQeQ",
         (const void *)&sceAvPlayerStreamCount_static},
        {"sceAvPlayerVprintf", "$yN7Jhuv8g24", NULL},
    };

    typedef int (*fn_init_ex_t)(const void *, void *);
    typedef int (*fn_post_init_t)(void *, const void *);
    typedef int (*fn_is_active_t)(void *);
    typedef int (*fn_stream_cnt_t)(void *);
    typedef int (*fn_add_src_ex_t)(void *, int, const char *);
    typedef int (*fn_close_t)(void *);
    typedef int (*fn_set_log_t)(void *, void *);
    typedef int (*fn_stop_t)(void *);

    fn_init_ex_t fn_init_ex = NULL;
    fn_post_init_t fn_post_init = NULL;
    fn_is_active_t fn_is_active = NULL;
    fn_stream_cnt_t fn_stream_cnt = NULL;
    fn_add_src_ex_t fn_add_src_ex = NULL;
    fn_close_t fn_close = NULL;
    fn_set_log_t fn_set_log = NULL;
    fn_stop_t fn_stop = NULL;

    for (size_t i = 0; i < sizeof(av_census) / sizeof(av_census[0]); i++) {
        const char *name = av_census[i].name;
        const char *nid = av_census[i].nid;
        const void *fn = av_census[i].static_fn;
        int is_link = 0;
        if (fn && obs_address_is_callable(fn)) {
            is_link = 1;
        } else {
            fn = obs_module_symbol(OBS_HANDLE_SELF, nid);
            if (!fn)
                fn = obs_module_symbol(1, nid);
            if (!fn && h_av > 0) {
                fn = obs_module_symbol(h_av, nid);
                if (!fn)
                    fn = obs_module_symbol(h_av, name);
            }
        }
        int bound = (fn != NULL && obs_address_is_callable(fn));
        obs_report_measure(id, name, "bound", (uint64_t)bound, "bool");
        if (bound) {
            obs_report_measure(id, name, "how", (uint64_t)(is_link ? 1 : 2),
                               is_link ? "link" : "dlsym");
        } else {
            obs_report_measure(id, name, "status", 0, "not-possible");
        }

        if (bound) {
            if (same_text(name, "sceAvPlayerInitEx", 17))
                fn_init_ex = (fn_init_ex_t)fn;
            else if (same_text(name, "sceAvPlayerPostInit", 19))
                fn_post_init = (fn_post_init_t)fn;
            else if (same_text(name, "sceAvPlayerIsActive", 19))
                fn_is_active = (fn_is_active_t)fn;
            else if (same_text(name, "sceAvPlayerStreamCount", 22))
                fn_stream_cnt = (fn_stream_cnt_t)fn;
            else if (same_text(name, "sceAvPlayerAddSourceEx", 22))
                fn_add_src_ex = (fn_add_src_ex_t)fn;
            else if (same_text(name, "sceAvPlayerClose", 16))
                fn_close = (fn_close_t)fn;
            else if (same_text(name, "sceAvPlayerSetLogCallback", 25))
                fn_set_log = (fn_set_log_t)fn;
            else if (same_text(name, "sceAvPlayerStop", 15))
                fn_stop = (fn_stop_t)fn;
        }
    }

    if (!fn_init_ex) {
        return obs_pass();
    }

    s_av_log_count = 0;
    s_av_user_log_count = 0;
    s_av_pool_offset = 0;

    int sig = 0;

    if (fn_set_log) {
        int rc_set_log = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set_log = fn_set_log((void *)&av_log_cb, (void *)0x1000);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "SetLogCallback", "rc", (uint64_t)(uint32_t)rc_set_log,
                           "code");
        obs_report_measure(id, "SetLogCallback", "callbacks",
                           (uint64_t)s_av_user_log_count, "count");
        for (size_t i = 0; i < s_av_user_log_count; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "SetLogCallback-cb%zu", i);
            obs_report_measure(id, tag, "user", s_av_user_logs[i].user, "addr");
            obs_report_measure(id, tag, "len", (uint64_t)strlen(s_av_user_logs[i].msg),
                               "count");
        }
    }

    uint8_t av_init_block[0x230];
    memset(av_init_block, 0, sizeof(av_init_block));
    *(uint64_t *)(void *)(av_init_block + 0x00) = 0x230ULL;
    *(uint64_t *)(void *)(av_init_block + 0x08) = 0x5a5a0000ULL;
    *(void **)(void *)(av_init_block + 0x10) = (void *)&av_cb_0;
    *(void **)(void *)(av_init_block + 0x18) = (void *)&av_cb_1;
    *(void **)(void *)(av_init_block + 0x20) = (void *)&av_cb_2;
    *(void **)(void *)(av_init_block + 0x28) = (void *)&av_cb_3;
    *(uint32_t *)(void *)(av_init_block + 0x70) = 0;
    *(uint8_t *)(void *)(av_init_block + 0x74) = 0;
    *(uint64_t *)(void *)(av_init_block + 0x80) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0xb0) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0xe0) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x110) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x140) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x170) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x1a0) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x1d0) = 0xffULL;
    *(uint64_t *)(void *)(av_init_block + 0x200) = 0xffULL;
    *(uint32_t *)(void *)(av_init_block + 0x228) = 6;

    uint8_t av_post_block[0x80];
    memset(av_post_block, 0, sizeof(av_post_block));
    *(uint32_t *)(void *)(av_post_block + 0x00) = 0x800000;
    *(uint32_t *)(void *)(av_post_block + 0x08) = 1;
    *(uint16_t *)(void *)(av_post_block + 0x1c) = 4;
    *(uint8_t *)(void *)(av_post_block + 0x1e) = 0;

    /* 1. InitEx */
    uint64_t handle = 0xccccccccccccccccULL;
    size_t before_cnt = s_av_log_count;
    int rc_init = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init = fn_init_ex(av_init_block, &handle);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "InitEx-first", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "InitEx-first", "rc", (uint64_t)(uint32_t)rc_init, "code");
    obs_report_bytes(id, "InitEx-first", "handle-8b", 0, (const uint8_t *)&handle, 8);
    obs_report_measure(id, "InitEx-first", "callbacks",
                       (uint64_t)(s_av_log_count - before_cnt), "count");
    for (size_t i = before_cnt; i < s_av_log_count; i++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "InitEx-first-cb%zu", i - before_cnt);
        obs_report_measure(id, tag, "id", (uint64_t)s_av_logs[i].cb_id, "id");
        obs_report_measure(id, tag, "a0", s_av_logs[i].a0, "val");
        obs_report_measure(id, tag, "a1", s_av_logs[i].a1, "val");
        obs_report_measure(id, tag, "a2", s_av_logs[i].a2, "val");
        obs_report_measure(id, tag, "ret", s_av_logs[i].ret, "addr");
    }

    /* 2. PostInit */
    if (fn_post_init && rc_init == 0) {
        before_cnt = s_av_log_count;
        int rc_post = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_post = fn_post_init((void *)(uintptr_t)handle, av_post_block);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PostInit", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "PostInit", "rc", (uint64_t)(uint32_t)rc_post, "code");
        obs_report_measure(id, "PostInit", "callbacks",
                           (uint64_t)(s_av_log_count - before_cnt), "count");
    }

    /* 2b. Stop on player with no source after PostInit per rf03 */
    if (fn_stop && rc_init == 0) {
        before_cnt = s_av_log_count;
        size_t before_user_cnt = s_av_user_log_count;
        int rc_stop = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_stop = fn_stop((void *)(uintptr_t)handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Stop-no-source", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Stop-no-source", "rc", (uint64_t)(uint32_t)rc_stop,
                           "code");
        obs_report_measure(id, "Stop-no-source", "callbacks",
                           (uint64_t)(s_av_log_count - before_cnt), "count");
        for (size_t i = before_cnt; i < s_av_log_count; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "Stop-no-source-cb%zu", i - before_cnt);
            obs_report_measure(id, tag, "id", (uint64_t)s_av_logs[i].cb_id, "id");
            obs_report_measure(id, tag, "a0", s_av_logs[i].a0, "val");
            obs_report_measure(id, tag, "a1", s_av_logs[i].a1, "val");
            obs_report_measure(id, tag, "a2", s_av_logs[i].a2, "val");
        }
        obs_report_measure(id, "Stop-no-source", "user-callbacks",
                           (uint64_t)(s_av_user_log_count - before_user_cnt), "count");
    }

    /* 3. IsActive and StreamCount with no source */
    if (fn_is_active && rc_init == 0) {
        int act = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            act = fn_is_active((void *)(uintptr_t)handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "IsActive-before-source", "active",
                           (uint64_t)(uint32_t)act, "val");
    }
    if (fn_stream_cnt && rc_init == 0) {
        int sc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sc = fn_stream_cnt((void *)(uintptr_t)handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "StreamCount-before-source", "count",
                           (uint64_t)(uint32_t)sc, "count");
    }

    /* 4. AddSourceEx on missing path, then IsActive */
    if (fn_add_src_ex && rc_init == 0) {
        int rc_add = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_add = fn_add_src_ex((void *)(uintptr_t)handle, 0, "/app0/none.mp4");
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "AddSourceEx-missing", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "AddSourceEx-missing", "rc", (uint64_t)(uint32_t)rc_add,
                           "code");
        if (fn_is_active) {
            int act2 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                act2 = fn_is_active((void *)(uintptr_t)handle);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "IsActive-after-source", "active",
                               (uint64_t)(uint32_t)act2, "val");
        }
    }

    /* 5. Close */
    if (fn_close && rc_init == 0) {
        before_cnt = s_av_log_count;
        int rc_close = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_close = fn_close((void *)(uintptr_t)handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Close-first", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Close-first", "rc", (uint64_t)(uint32_t)rc_close,
                           "code");
        obs_report_measure(id, "Close-first", "callbacks",
                           (uint64_t)(s_av_log_count - before_cnt), "count");
        for (size_t i = before_cnt; i < s_av_log_count; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "Close-first-cb%zu", i - before_cnt);
            obs_report_measure(id, tag, "id", (uint64_t)s_av_logs[i].cb_id, "id");
            obs_report_measure(id, tag, "a0", s_av_logs[i].a0, "val");
            obs_report_measure(id, tag, "a1", s_av_logs[i].a1, "val");
            obs_report_measure(id, tag, "a2", s_av_logs[i].a2, "val");
        }
    }

    /* 6. Repeat Steps 1 and 5 for second handle */
    uint64_t handle2 = 0xccccccccccccccccULL;
    before_cnt = s_av_log_count;
    int rc_init2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init2 = fn_init_ex(av_init_block, &handle2);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "InitEx-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "InitEx-second", "rc", (uint64_t)(uint32_t)rc_init2, "code");
    obs_report_bytes(id, "InitEx-second", "handle-8b", 0, (const uint8_t *)&handle2, 8);
    obs_report_measure(id, "InitEx-second", "callbacks",
                       (uint64_t)(s_av_log_count - before_cnt), "count");
    for (size_t i = before_cnt; i < s_av_log_count; i++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "InitEx-second-cb%zu", i - before_cnt);
        obs_report_measure(id, tag, "id", (uint64_t)s_av_logs[i].cb_id, "id");
        obs_report_measure(id, tag, "a0", s_av_logs[i].a0, "val");
        obs_report_measure(id, tag, "a1", s_av_logs[i].a1, "val");
        obs_report_measure(id, tag, "a2", s_av_logs[i].a2, "val");
        obs_report_measure(id, tag, "ret", s_av_logs[i].ret, "addr");
    }

    if (fn_close && rc_init2 == 0) {
        before_cnt = s_av_log_count;
        int rc_close2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_close2 = fn_close((void *)(uintptr_t)handle2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Close-second", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Close-second", "rc", (uint64_t)(uint32_t)rc_close2,
                           "code");
        obs_report_measure(id, "Close-second", "callbacks",
                           (uint64_t)(s_av_log_count - before_cnt), "count");
        for (size_t i = before_cnt; i < s_av_log_count; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "Close-second-cb%zu", i - before_cnt);
            obs_report_measure(id, tag, "id", (uint64_t)s_av_logs[i].cb_id, "id");
            obs_report_measure(id, tag, "a0", s_av_logs[i].a0, "val");
            obs_report_measure(id, tag, "a1", s_av_logs[i].a1, "val");
            obs_report_measure(id, tag, "a2", s_av_logs[i].a2, "val");
        }
    }

    /* REQ-20261012T2011Z-625e: Follow-up arms */
    /* 7. InitEx with block size 0x100 */
    uint8_t small_block[0x230];
    memcpy(small_block, av_init_block, sizeof(small_block));
    *(uint64_t *)(void *)(small_block + 0x00) = 0x100ULL;
    uint64_t handle_small = 0;
    int rc_small = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_small = fn_init_ex(small_block, &handle_small);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "InitEx-size-0x100", "rc", (uint64_t)(uint32_t)rc_small,
                       "code");
    if (rc_small == 0 && fn_close) {
        fn_close((void *)(uintptr_t)handle_small);
    }

    /* 8. InitEx with NULL callback slots */
    uint8_t null_cb_block[0x230];
    memcpy(null_cb_block, av_init_block, sizeof(null_cb_block));
    *(void **)(void *)(null_cb_block + 0x10) = NULL; /* null alloc */
    uint64_t handle_nullcb = 0;
    int rc_nullcb = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_nullcb = fn_init_ex(null_cb_block, &handle_nullcb);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "InitEx-null-alloc-cb", "rc", (uint64_t)(uint32_t)rc_nullcb,
                       "code");
    if (rc_nullcb == 0 && fn_close) {
        fn_close((void *)(uintptr_t)handle_nullcb);
    }

    /* 9. PostInit with all-zero block */
    uint64_t handle3 = 0;
    int rc_init3 = fn_init_ex(av_init_block, &handle3);
    if (rc_init3 == 0) {
        if (s_av_log_count > 0) {
            uint64_t alloc_ret = s_av_logs[s_av_log_count - 1].ret;
            obs_report_measure(id, "InitEx-handle", "equals-alloc-ptr",
                               (uint64_t)(handle3 == alloc_ret), "bool");
            obs_report_measure(id, "InitEx-handle", "offset-from-alloc",
                               handle3 - alloc_ret, "delta");
        }

        uint8_t zero_post[0x80];
        memset(zero_post, 0, sizeof(zero_post));
        int rc_post_zero = -1;
        if (fn_post_init) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_post_zero = fn_post_init((void *)(uintptr_t)handle3, zero_post);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "PostInit-all-zeros", "rc",
                               (uint64_t)(uint32_t)rc_post_zero, "code");
        }

        /* 10. AddSourceEx on real file (param.json) */
        if (fn_add_src_ex) {
            int rc_add_real = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_add_real = fn_add_src_ex((void *)(uintptr_t)handle3, 0,
                                            "/app0/sce_sys/param.json");
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "AddSourceEx-param-json", "rc",
                               (uint64_t)(uint32_t)rc_add_real, "code");
        }

        if (fn_close) {
            fn_close((void *)(uintptr_t)handle3);
        }
    }

    return obs_pass();
}

/* REQ-20261012T0710Z-gu01: GameUpdate startup and census */
static obs_result check_game_update_startup(void) {
    const char *id = "000-boot/game-update-startup";
    obs_jmp_buf guard;

    int h_gu = obs_module_open("libSceGameUpdate");

    static const struct {
        const char *name;
        const char *nid;
        const void *static_fn;
    } gu_census[] = {
        {"sceGameUpdateInitialize", "$YJtKLttI9fM",
         (const void *)&sceGameUpdateInitialize_static},
        {"sceGameUpdateTerminate", "$NSH-C-OmoNI",
         (const void *)&sceGameUpdateTerminate_static},
        {"sceGameUpdateAbortRequest", "$d1CNGEOaK28",
         (const void *)&sceGameUpdateAbortRequest_static},
        {"sceGameUpdateCheck", "$LYVV9z8+owM",
         (const void *)&sceGameUpdateCheck_static},
        {"sceGameUpdateCreateRequest", "$UvcvKaFvupA",
         (const void *)&sceGameUpdateCreateRequest_static},
        {"sceGameUpdateDeleteRequest", "$bcCyjHN5sn0",
         (const void *)&sceGameUpdateDeleteRequest_static},
    };

    typedef uint64_t (*fn_gu_init_t)(uint64_t, uint64_t);
    typedef int (*fn_gu_term_t)(void);

    fn_gu_init_t fn_init = NULL;
    fn_gu_term_t fn_term = NULL;

    for (size_t i = 0; i < sizeof(gu_census) / sizeof(gu_census[0]); i++) {
        const char *name = gu_census[i].name;
        const char *nid = gu_census[i].nid;
        const void *fn = gu_census[i].static_fn;
        int is_link = 0;
        if (fn && obs_address_is_callable(fn)) {
            is_link = 1;
        } else {
            fn = obs_module_symbol(OBS_HANDLE_SELF, nid);
            if (!fn)
                fn = obs_module_symbol(1, nid);
            if (!fn && h_gu > 0) {
                fn = obs_module_symbol(h_gu, nid);
                if (!fn)
                    fn = obs_module_symbol(h_gu, name);
            }
        }
        int bound = (fn != NULL && obs_address_is_callable(fn));
        obs_report_measure(id, name, "bound", (uint64_t)bound, "bool");
        if (bound) {
            obs_report_measure(id, name, "how", (uint64_t)(is_link ? 1 : 2),
                               is_link ? "link" : "dlsym");
        } else {
            obs_report_measure(id, name, "status", 0, "not-possible");
        }

        if (bound) {
            if (same_text(name, "sceGameUpdateInitialize", 23)) {
                fn_init = (fn_gu_init_t)fn;
            } else if (same_text(name, "sceGameUpdateTerminate", 22)) {
                fn_term = (fn_gu_term_t)fn;
            }
        }
    }

    if (!fn_init) {
        return obs_pass();
    }

    /* 1. sceGameUpdateInitialize(0x1318, 0) */
    uint64_t rc1 = 0xffffffffffffffffULL;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_init(0x1318, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "Initialize-1318-first", "fault", (uint64_t)sig,
                           "signal");
    }
    obs_report_measure(id, "Initialize-1318-first", "rc", rc1, "val");

    /* 2. Same call again */
    uint64_t rc2 = 0xffffffffffffffffULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_init(0x1318, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "Initialize-1318-second", "fault", (uint64_t)sig,
                           "signal");
    }
    obs_report_measure(id, "Initialize-1318-second", "rc", rc2, "val");

    /* 3. sceGameUpdateInitialize(0, 0) */
    uint64_t rc0 = 0xffffffffffffffffULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc0 = fn_init(0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "Initialize-0-0", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "Initialize-0-0", "rc", rc0, "val");

    /* 4. Terminate */
    if (fn_term) {
        int rc_t = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_t = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Terminate", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Terminate", "rc", (uint64_t)(uint32_t)rc_t, "code");
    }

    return obs_pass();
}

/* REQ-20261012T0728Z-pu01: Pad, IME and UDS calls */
static obs_result check_pad_ime_uds(void) {
    const char *id = "000-boot/pad-ime-uds";
    obs_jmp_buf guard;

    typedef int (*fn_pad_info_t)(int, void *);
    typedef int (*fn_pad_reset_t)(int);
    typedef int (*fn_ime_key_t)(uint32_t, void *);
    typedef int (*fn_uds_handle_t)(uint64_t *);

    fn_pad_info_t fn_pad_info = NULL;
    int is_link_pi = 0;
    if (obs_address_is_callable((const void *)&scePadGetControllerInformation_static)) {
        fn_pad_info = (fn_pad_info_t)&scePadGetControllerInformation_static;
        is_link_pi = 1;
    }
    if (!fn_pad_info &&
        obs_address_is_callable((const void *)&scePadGetControllerInformation)) {
        fn_pad_info = (fn_pad_info_t)&scePadGetControllerInformation;
        is_link_pi = 1;
    }
    if (!fn_pad_info) {
        fn_pad_info = (fn_pad_info_t)obs_module_symbol(OBS_HANDLE_SELF, "$gjP9-KQzoUk");
        if (!fn_pad_info)
            fn_pad_info = (fn_pad_info_t)obs_module_symbol(1, "$gjP9-KQzoUk");
        if (!fn_pad_info) {
            int h = obs_module_open("libScePad");
            if (h > 0) {
                fn_pad_info = (fn_pad_info_t)obs_module_symbol(h, "$gjP9-KQzoUk");
                if (!fn_pad_info)
                    fn_pad_info = (fn_pad_info_t)obs_module_symbol(
                        h, "scePadGetControllerInformation");
            }
        }
    }
    int bound_pi =
        (fn_pad_info != NULL && obs_address_is_callable((const void *)fn_pad_info));
    obs_report_measure(id, "scePadGetControllerInformation", "bound",
                       (uint64_t)bound_pi, "bool");
    if (bound_pi) {
        obs_report_measure(id, "scePadGetControllerInformation", "how",
                           (uint64_t)(is_link_pi ? 1 : 2),
                           is_link_pi ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePadGetControllerInformation", "status", 0,
                           "not-possible");
    }

    fn_pad_reset_t fn_pad_reset = NULL;
    int is_link_pr = 0;
    if (obs_address_is_callable((const void *)&scePadResetOrientation_static)) {
        fn_pad_reset = (fn_pad_reset_t)&scePadResetOrientation_static;
        is_link_pr = 1;
    }
    if (!fn_pad_reset &&
        obs_address_is_callable((const void *)&scePadResetOrientation)) {
        fn_pad_reset = (fn_pad_reset_t)&scePadResetOrientation;
        is_link_pr = 1;
    }
    if (!fn_pad_reset) {
        fn_pad_reset =
            (fn_pad_reset_t)obs_module_symbol(OBS_HANDLE_SELF, "$rIZnR6eSpvk");
        if (!fn_pad_reset)
            fn_pad_reset = (fn_pad_reset_t)obs_module_symbol(1, "$rIZnR6eSpvk");
        if (!fn_pad_reset) {
            int h = obs_module_open("libScePad");
            if (h > 0) {
                fn_pad_reset = (fn_pad_reset_t)obs_module_symbol(h, "$rIZnR6eSpvk");
                if (!fn_pad_reset)
                    fn_pad_reset =
                        (fn_pad_reset_t)obs_module_symbol(h, "scePadResetOrientation");
            }
        }
    }
    int bound_pr =
        (fn_pad_reset != NULL && obs_address_is_callable((const void *)fn_pad_reset));
    obs_report_measure(id, "scePadResetOrientation", "bound", (uint64_t)bound_pr,
                       "bool");
    if (bound_pr) {
        obs_report_measure(id, "scePadResetOrientation", "how",
                           (uint64_t)(is_link_pr ? 1 : 2),
                           is_link_pr ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePadResetOrientation", "status", 0, "not-possible");
    }

    fn_ime_key_t fn_ime_key = NULL;
    int is_link_ik = 0;
    if (obs_address_is_callable((const void *)&sceImeKeyboardGetResourceId_static)) {
        fn_ime_key = (fn_ime_key_t)&sceImeKeyboardGetResourceId_static;
        is_link_ik = 1;
    }
    if (!fn_ime_key) {
        fn_ime_key = (fn_ime_key_t)obs_module_symbol(OBS_HANDLE_SELF, "$dKadqZFgKKQ");
        if (!fn_ime_key)
            fn_ime_key = (fn_ime_key_t)obs_module_symbol(1, "$dKadqZFgKKQ");
        if (!fn_ime_key) {
            int h = obs_module_open("libSceIme");
            if (h > 0) {
                fn_ime_key = (fn_ime_key_t)obs_module_symbol(h, "$dKadqZFgKKQ");
                if (!fn_ime_key)
                    fn_ime_key = (fn_ime_key_t)obs_module_symbol(
                        h, "sceImeKeyboardGetResourceId");
            }
        }
    }
    int bound_ik =
        (fn_ime_key != NULL && obs_address_is_callable((const void *)fn_ime_key));
    obs_report_measure(id, "sceImeKeyboardGetResourceId", "bound", (uint64_t)bound_ik,
                       "bool");
    if (bound_ik) {
        obs_report_measure(id, "sceImeKeyboardGetResourceId", "how",
                           (uint64_t)(is_link_ik ? 1 : 2),
                           is_link_ik ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceImeKeyboardGetResourceId", "status", 0,
                           "not-possible");
    }

    fn_uds_handle_t fn_uds_handle = NULL;
    int is_link_uh = 0;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemCreateHandle_static)) {
        fn_uds_handle = (fn_uds_handle_t)&sceNpUniversalDataSystemCreateHandle_static;
        is_link_uh = 1;
    }
    if (!fn_uds_handle) {
        fn_uds_handle =
            (fn_uds_handle_t)obs_module_symbol(OBS_HANDLE_SELF, "$hT0IAEvN+M0");
        if (!fn_uds_handle)
            fn_uds_handle = (fn_uds_handle_t)obs_module_symbol(1, "$hT0IAEvN+M0");
        if (!fn_uds_handle) {
            int h = obs_module_open("libSceNpUniversalDataSystem");
            if (h > 0) {
                fn_uds_handle = (fn_uds_handle_t)obs_module_symbol(h, "$hT0IAEvN+M0");
                if (!fn_uds_handle)
                    fn_uds_handle = (fn_uds_handle_t)obs_module_symbol(
                        h, "sceNpUniversalDataSystemCreateHandle");
            }
        }
    }
    int bound_uh =
        (fn_uds_handle != NULL && obs_address_is_callable((const void *)fn_uds_handle));
    obs_report_measure(id, "sceNpUniversalDataSystemCreateHandle", "bound",
                       (uint64_t)bound_uh, "bool");
    if (bound_uh) {
        obs_report_measure(id, "sceNpUniversalDataSystemCreateHandle", "how",
                           (uint64_t)(is_link_uh ? 1 : 2),
                           is_link_uh ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpUniversalDataSystemCreateHandle", "status", 0,
                           "not-possible");
    }

    /* 1. Pad setup, info, reset */
    int32_t user = get_boot_user();
    if (obs_address_is_callable((const void *)&scePadInit)) {
        scePadInit();
    }
    int pad_h = -1;
    if (obs_address_is_callable((const void *)&scePadOpen)) {
        pad_h = scePadOpen(user, 0, 0, NULL);
    }
    obs_report_measure(id, "scePadOpen", "handle", (uint64_t)(int64_t)pad_h, "handle");

    if (bound_pi) {
        uint8_t info[0x80];
        memset(info, 0xcc, sizeof(info));
        uint8_t before[0x80];
        memset(before, 0xcc, sizeof(before));
        int rc_pi = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pi = fn_pad_info(pad_h, info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PadGetInfo-open", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "PadGetInfo-open", "rc", (uint64_t)(uint32_t)rc_pi,
                           "code");
        obs_report_written(id, "PadGetInfo-open", "bytes-changed", before, info,
                           sizeof(info));
        obs_report_bytes(id, "PadGetInfo-open", "info-80h", 0, info, sizeof(info));

        uint8_t bad_info[0x80];
        memset(bad_info, 0xcc, sizeof(bad_info));
        int rc_bad_pi = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad_pi = fn_pad_info(0x7fffffff, bad_info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "PadGetInfo-7fffffff", "rc",
                           (uint64_t)(uint32_t)rc_bad_pi, "code");
    }

    if (bound_pr) {
        int rc_pr = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr = fn_pad_reset(pad_h);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PadResetOrientation-open", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PadResetOrientation-open", "rc",
                           (uint64_t)(uint32_t)rc_pr, "code");

        int rc_bad_pr = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad_pr = fn_pad_reset(0x7fffffff);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "PadResetOrientation-7fffffff", "rc",
                           (uint64_t)(uint32_t)rc_bad_pr, "code");
    }

    if (pad_h > 0 && obs_address_is_callable((const void *)&scePadClose)) {
        scePadClose(pad_h);
    }

    /* 2. Ime keyboard resource id (1 and user) */
    if (bound_ik) {
        uint8_t ime_out1[32];
        memset(ime_out1, 0xcc, sizeof(ime_out1));
        uint8_t before1[32];
        memset(before1, 0xcc, sizeof(before1));
        int rc_ik1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ik1 = fn_ime_key(1, ime_out1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ImeGetResource-id1", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "ImeGetResource-id1", "rc", (uint64_t)(uint32_t)rc_ik1,
                           "code");
        obs_report_written(id, "ImeGetResource-id1", "bytes-changed", before1, ime_out1,
                           sizeof(ime_out1));
        obs_report_bytes(id, "ImeGetResource-id1", "out-32b", 0, ime_out1,
                         sizeof(ime_out1));

        uint8_t ime_out_u[32];
        memset(ime_out_u, 0xcc, sizeof(ime_out_u));
        uint8_t before_u[32];
        memset(before_u, 0xcc, sizeof(before_u));
        int rc_iku = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_iku = fn_ime_key((uint32_t)user, ime_out_u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ImeGetResource-user", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "ImeGetResource-user", "rc", (uint64_t)(uint32_t)rc_iku,
                           "code");
        obs_report_written(id, "ImeGetResource-user", "bytes-changed", before_u,
                           ime_out_u, sizeof(ime_out_u));
        obs_report_bytes(id, "ImeGetResource-user", "out-32b", 0, ime_out_u,
                         sizeof(ime_out_u));
    }

    /* 3. UDS CreateHandle twice after CreateContext */
    if (bound_uh) {
        typedef int (*fn_uds_init_t)(const void *, const void *, uint64_t, uint64_t);
        typedef int (*fn_uds_ctx_t)(uint64_t *, int32_t, uint64_t, uint64_t);
        fn_uds_init_t fn_uds_init =
            obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemInitialize_static)
                ? (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_static
                : (obs_address_is_callable(
                       (const void *)&sceNpUniversalDataSystemInitialize_be)
                       ? (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_be
                       : NULL);
        fn_uds_ctx_t fn_uds_ctx =
            obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemCreateContext_static)
                ? (fn_uds_ctx_t)&sceNpUniversalDataSystemCreateContext_static
                : NULL;
        if (fn_uds_init) {
            fn_uds_init(NULL, NULL, 0, 0);
        }
        uint64_t ctx = 0;
        int rc_ctx = -1;
        if (fn_uds_ctx) {
            rc_ctx = fn_uds_ctx(&ctx, user, 0, 0);
        }
        obs_report_measure(id, "UdsCreateContext", "rc", (uint64_t)(uint32_t)rc_ctx,
                           "code");

        uint64_t h1 = 0xccccccccccccccccULL;
        int rc_h1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_h1 = fn_uds_handle(&h1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "UdsCreateHandle-first", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "UdsCreateHandle-first", "rc", (uint64_t)(uint32_t)rc_h1,
                           "code");
        obs_report_bytes(id, "UdsCreateHandle-first", "handle-8b", 0,
                         (const uint8_t *)&h1, 8);

        uint64_t h2 = 0xccccccccccccccccULL;
        int rc_h2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_h2 = fn_uds_handle(&h2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "UdsCreateHandle-second", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "UdsCreateHandle-second", "rc",
                           (uint64_t)(uint32_t)rc_h2, "code");
        obs_report_bytes(id, "UdsCreateHandle-second", "handle-8b", 0,
                         (const uint8_t *)&h2, 8);
    }

    return obs_pass();
}

/* REQ-20261012T0635Z-ag01: ag01 aux calls */
static obs_result check_ag01_aux_calls(void) {
    const char *id = "000-boot/ag01-aux-calls";
    obs_jmp_buf guard;

    typedef int (*fn_zoom_t)(int32_t, void *);
    typedef int (*fn_vo_supp_t)(int, int);
    typedef int (*fn_sd_mem_t)(const void *);

    fn_zoom_t fn_zoom = NULL;
    int is_link_zoom = 0;
    if (obs_address_is_callable(
            (const void *)&sceUserServiceGetAccessibilityZoomEnabled_static)) {
        fn_zoom = (fn_zoom_t)&sceUserServiceGetAccessibilityZoomEnabled_static;
        is_link_zoom = 1;
    }
    if (!fn_zoom) {
        fn_zoom = (fn_zoom_t)obs_module_symbol(OBS_HANDLE_SELF, "$hD-H81EN9Vg");
        if (!fn_zoom)
            fn_zoom = (fn_zoom_t)obs_module_symbol(1, "$hD-H81EN9Vg");
        if (!fn_zoom) {
            int h = obs_module_open("libSceUserService");
            if (h > 0) {
                fn_zoom = (fn_zoom_t)obs_module_symbol(h, "$hD-H81EN9Vg");
                if (!fn_zoom)
                    fn_zoom = (fn_zoom_t)obs_module_symbol(
                        h, "sceUserServiceGetAccessibilityZoomEnabled");
            }
        }
    }
    int bound_zoom =
        (fn_zoom != NULL && obs_address_is_callable((const void *)fn_zoom));
    obs_report_measure(id, "sceUserServiceGetAccessibilityZoomEnabled", "bound",
                       (uint64_t)bound_zoom, "bool");
    if (bound_zoom) {
        obs_report_measure(id, "sceUserServiceGetAccessibilityZoomEnabled", "how",
                           (uint64_t)(is_link_zoom ? 1 : 2),
                           is_link_zoom ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceUserServiceGetAccessibilityZoomEnabled", "status", 0,
                           "not-possible");
    }

    fn_vo_supp_t fn_vo_supp = NULL;
    int is_link_vo = 0;
    if (obs_address_is_callable((const void *)&sceVideoOutIsOutputSupported_static)) {
        fn_vo_supp = (fn_vo_supp_t)&sceVideoOutIsOutputSupported_static;
        is_link_vo = 1;
    }
    if (!fn_vo_supp &&
        obs_address_is_callable((const void *)&sceVideoOutIsOutputSupported)) {
        fn_vo_supp = (fn_vo_supp_t)&sceVideoOutIsOutputSupported;
        is_link_vo = 1;
    }
    if (!fn_vo_supp) {
        fn_vo_supp = (fn_vo_supp_t)obs_module_symbol(OBS_HANDLE_SELF, "$Nv8c-Kb+DUM");
        if (!fn_vo_supp)
            fn_vo_supp = (fn_vo_supp_t)obs_module_symbol(1, "$Nv8c-Kb+DUM");
        if (!fn_vo_supp) {
            int h = obs_module_open("libSceVideoOut");
            if (h > 0) {
                fn_vo_supp = (fn_vo_supp_t)obs_module_symbol(h, "$Nv8c-Kb+DUM");
                if (!fn_vo_supp)
                    fn_vo_supp = (fn_vo_supp_t)obs_module_symbol(
                        h, "sceVideoOutIsOutputSupported");
            }
        }
    }
    int bound_vo =
        (fn_vo_supp != NULL && obs_address_is_callable((const void *)fn_vo_supp));
    obs_report_measure(id, "sceVideoOutIsOutputSupported", "bound", (uint64_t)bound_vo,
                       "bool");
    if (bound_vo) {
        obs_report_measure(id, "sceVideoOutIsOutputSupported", "how",
                           (uint64_t)(is_link_vo ? 1 : 2),
                           is_link_vo ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceVideoOutIsOutputSupported", "status", 0,
                           "not-possible");
    }

    fn_sd_mem_t fn_sd_set = NULL;
    int is_link_sd_set = 0;
    if (obs_address_is_callable((const void *)&sceSaveDataSetSaveDataMemory2_static)) {
        fn_sd_set = (fn_sd_mem_t)&sceSaveDataSetSaveDataMemory2_static;
        is_link_sd_set = 1;
    }
    if (!fn_sd_set) {
        fn_sd_set = (fn_sd_mem_t)obs_module_symbol(OBS_HANDLE_SELF, "$cduy9v4YmT4");
        if (!fn_sd_set)
            fn_sd_set = (fn_sd_mem_t)obs_module_symbol(1, "$cduy9v4YmT4");
        if (!fn_sd_set) {
            int h = obs_module_open("libSceSaveData");
            if (h > 0) {
                fn_sd_set = (fn_sd_mem_t)obs_module_symbol(h, "$cduy9v4YmT4");
                if (!fn_sd_set)
                    fn_sd_set = (fn_sd_mem_t)obs_module_symbol(
                        h, "sceSaveDataSetSaveDataMemory2");
            }
        }
    }
    int bound_sd_set =
        (fn_sd_set != NULL && obs_address_is_callable((const void *)fn_sd_set));
    obs_report_measure(id, "sceSaveDataSetSaveDataMemory2", "bound",
                       (uint64_t)bound_sd_set, "bool");
    if (bound_sd_set) {
        obs_report_measure(id, "sceSaveDataSetSaveDataMemory2", "how",
                           (uint64_t)(is_link_sd_set ? 1 : 2),
                           is_link_sd_set ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceSaveDataSetSaveDataMemory2", "status", 0,
                           "not-possible");
    }

    fn_sd_mem_t fn_sd_sync = NULL;
    int is_link_sd_sync = 0;
    if (obs_address_is_callable((const void *)&sceSaveDataSyncSaveDataMemory_static)) {
        fn_sd_sync = (fn_sd_mem_t)&sceSaveDataSyncSaveDataMemory_static;
        is_link_sd_sync = 1;
    }
    if (!fn_sd_sync) {
        fn_sd_sync = (fn_sd_mem_t)obs_module_symbol(OBS_HANDLE_SELF, "$wiT9jeC7xPw");
        if (!fn_sd_sync)
            fn_sd_sync = (fn_sd_mem_t)obs_module_symbol(1, "$wiT9jeC7xPw");
        if (!fn_sd_sync) {
            int h = obs_module_open("libSceSaveData");
            if (h > 0) {
                fn_sd_sync = (fn_sd_mem_t)obs_module_symbol(h, "$wiT9jeC7xPw");
                if (!fn_sd_sync)
                    fn_sd_sync = (fn_sd_mem_t)obs_module_symbol(
                        h, "sceSaveDataSyncSaveDataMemory");
            }
        }
    }
    int bound_sd_sync =
        (fn_sd_sync != NULL && obs_address_is_callable((const void *)fn_sd_sync));
    obs_report_measure(id, "sceSaveDataSyncSaveDataMemory", "bound",
                       (uint64_t)bound_sd_sync, "bool");
    if (bound_sd_sync) {
        obs_report_measure(id, "sceSaveDataSyncSaveDataMemory", "how",
                           (uint64_t)(is_link_sd_sync ? 1 : 2),
                           is_link_sd_sync ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceSaveDataSyncSaveDataMemory", "status", 0,
                           "not-possible");
    }

    int32_t user = get_boot_user();

    /* 1. Zoom */
    if (bound_zoom) {
        uint8_t zoom_out[8];
        memset(zoom_out, 0xcc, sizeof(zoom_out));
        int rc_z = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_z = fn_zoom(user, zoom_out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ZoomEnabled", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "ZoomEnabled", "rc", (uint64_t)(uint32_t)rc_z, "code");
        obs_report_bytes(id, "ZoomEnabled", "out-8b", 0, zoom_out, 8);
    }

    /* 2. VideoOutIsOutputSupported (0, 0) and (0, 1) */
    int vo_h = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        vo_h = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, NULL);
        if (vo_h <= 0 && user != 0xff) {
            vo_h = sceVideoOutOpen(0xff, OBS_VIDEO_BUS_MAIN, 0, NULL);
        }
    }
    if (bound_vo) {
        int rc_vo0 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vo0 = fn_vo_supp(0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "IsOutputSupported-0-0", "rc",
                           (uint64_t)(uint32_t)rc_vo0, "code");

        int rc_vo1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vo1 = fn_vo_supp(0, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "IsOutputSupported-0-1", "rc",
                           (uint64_t)(uint32_t)rc_vo1, "code");
    }
    if (vo_h > 0 && obs_address_is_callable((const void *)&sceVideoOutClose)) {
        sceVideoOutClose(vo_h);
    }

    /* 3. SaveData memory */
    uint8_t sd_mem[0x80];
    memset(sd_mem, 0, sizeof(sd_mem));
    if (bound_sd_set) {
        int rc_s = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_s = fn_sd_set(sd_mem);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SetSaveDataMemory2", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SetSaveDataMemory2", "rc", (uint64_t)(uint32_t)rc_s,
                           "code");
    }
    if (bound_sd_sync) {
        int rc_sy = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sy = fn_sd_sync(sd_mem);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SyncSaveDataMemory", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SyncSaveDataMemory", "rc", (uint64_t)(uint32_t)rc_sy,
                           "code");
    }

    return obs_pass();
}

/* REQ-20261012T1123Z-ap01: AudioOut2 mono ports, attributes and destroy */
static obs_result check_audioout2_mono_ports(void) {
    const char *id = "000-boot/audioout2-mono-ports";
    obs_jmp_buf guard;

    int h_ao = obs_module_open("libSceAudioOut2");

    typedef int (*fn_init_t)(void);
    typedef int (*fn_reset_param_t)(void *);
    typedef int (*fn_query_mem_t)(void *, uint64_t *);
    typedef int (*fn_ctx_create_t)(void *, void *, uint64_t, void **);
    typedef int (*fn_user_create_t)(int32_t, void **);
    typedef int (*fn_port_create_t)(void *, const void *, void **);
    typedef int (*fn_port_set_attr_t)(void *, const void *, uint32_t);
    typedef int (*fn_port_destroy_t)(void *);

    fn_init_t fn_init =
        obs_address_is_callable((const void *)&sceAudioOut2Initialize_static)
            ? (fn_init_t)&sceAudioOut2Initialize_static
            : (fn_init_t)obs_module_symbol(h_ao > 0 ? h_ao : OBS_HANDLE_SELF,
                                           "sceAudioOut2Initialize");
    if (!fn_init && h_ao > 0)
        fn_init = (fn_init_t)obs_module_symbol(h_ao, "$g2tViFIohHE");

    fn_reset_param_t fn_reset_param =
        obs_address_is_callable((const void *)&sceAudioOut2ContextResetParam_static)
            ? (fn_reset_param_t)&sceAudioOut2ContextResetParam_static
            : (fn_reset_param_t)obs_module_symbol(h_ao > 0 ? h_ao : OBS_HANDLE_SELF,
                                                  "sceAudioOut2ContextResetParam");
    if (!fn_reset_param && h_ao > 0)
        fn_reset_param = (fn_reset_param_t)obs_module_symbol(h_ao, "$t5YrizufpQc");

    fn_query_mem_t fn_query_mem =
        obs_address_is_callable((const void *)&sceAudioOut2ContextQueryMemory_static)
            ? (fn_query_mem_t)&sceAudioOut2ContextQueryMemory_static
            : (fn_query_mem_t)obs_module_symbol(h_ao > 0 ? h_ao : OBS_HANDLE_SELF,
                                                "sceAudioOut2ContextQueryMemory");
    if (!fn_query_mem && h_ao > 0)
        fn_query_mem = (fn_query_mem_t)obs_module_symbol(h_ao, "$pDmme7Bgm6E");

    fn_ctx_create_t fn_ctx_create =
        obs_address_is_callable((const void *)&sceAudioOut2ContextCreate_static)
            ? (fn_ctx_create_t)&sceAudioOut2ContextCreate_static
            : (fn_ctx_create_t)obs_module_symbol(h_ao > 0 ? h_ao : OBS_HANDLE_SELF,
                                                 "sceAudioOut2ContextCreate");
    if (!fn_ctx_create && h_ao > 0)
        fn_ctx_create = (fn_ctx_create_t)obs_module_symbol(h_ao, "$0x6o1VVAYSY");

    fn_user_create_t fn_user_create =
        obs_address_is_callable((const void *)&sceAudioOut2UserCreate_static)
            ? (fn_user_create_t)&sceAudioOut2UserCreate_static
            : (fn_user_create_t)obs_module_symbol(h_ao > 0 ? h_ao : OBS_HANDLE_SELF,
                                                  "sceAudioOut2UserCreate");
    if (!fn_user_create && h_ao > 0)
        fn_user_create = (fn_user_create_t)obs_module_symbol(h_ao, "$xywYcRB7nbQ");

    fn_port_create_t fn_port_create = NULL;
    int is_link_create = 0;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_static)) {
        fn_port_create = (fn_port_create_t)&sceAudioOut2PortCreate_static;
        is_link_create = 1;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_alt)) {
        fn_port_create = (fn_port_create_t)&sceAudioOut2PortCreate_alt;
        is_link_create = 1;
    }
    if (!fn_port_create && h_ao > 0) {
        fn_port_create =
            (fn_port_create_t)obs_module_symbol(h_ao, "sceAudioOut2PortCreate");
        if (!fn_port_create)
            fn_port_create = (fn_port_create_t)obs_module_symbol(h_ao, "$A89PZmqwrSQ");
    }

    fn_port_set_attr_t fn_port_set_attr = NULL;
    int is_link_attr = 0;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortSetAttributes_static)) {
        fn_port_set_attr = (fn_port_set_attr_t)&sceAudioOut2PortSetAttributes_static;
        is_link_attr = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAudioOut2PortSetAttributes_alt)) {
        fn_port_set_attr = (fn_port_set_attr_t)&sceAudioOut2PortSetAttributes_alt;
        is_link_attr = 1;
    }
    if (!fn_port_set_attr && h_ao > 0) {
        fn_port_set_attr = (fn_port_set_attr_t)obs_module_symbol(
            h_ao, "sceAudioOut2PortSetAttributes");
        if (!fn_port_set_attr)
            fn_port_set_attr =
                (fn_port_set_attr_t)obs_module_symbol(h_ao, "$8XTArSPyWHk");
    }

    fn_port_destroy_t fn_port_destroy = NULL;
    int is_link_dest = 0;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_static)) {
        fn_port_destroy = (fn_port_destroy_t)&sceAudioOut2PortDestroy_static;
        is_link_dest = 1;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_alt)) {
        fn_port_destroy = (fn_port_destroy_t)&sceAudioOut2PortDestroy_alt;
        is_link_dest = 1;
    }
    if (!fn_port_destroy && h_ao > 0) {
        fn_port_destroy =
            (fn_port_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2PortDestroy");
        if (!fn_port_destroy)
            fn_port_destroy =
                (fn_port_destroy_t)obs_module_symbol(h_ao, "$cd+Rtw+D1x8");
    }

    int bound_pc = (fn_port_create != NULL &&
                    obs_address_is_callable((const void *)fn_port_create));
    obs_report_measure(id, "sceAudioOut2PortCreate", "bound", (uint64_t)bound_pc,
                       "bool");
    if (bound_pc) {
        obs_report_measure(id, "sceAudioOut2PortCreate", "how",
                           (uint64_t)(is_link_create ? 1 : 2),
                           is_link_create ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2PortCreate", "status", 0, "not-possible");
    }

    int bound_psa = (fn_port_set_attr != NULL &&
                     obs_address_is_callable((const void *)fn_port_set_attr));
    obs_report_measure(id, "sceAudioOut2PortSetAttributes", "bound",
                       (uint64_t)bound_psa, "bool");
    if (bound_psa) {
        obs_report_measure(id, "sceAudioOut2PortSetAttributes", "how",
                           (uint64_t)(is_link_attr ? 1 : 2),
                           is_link_attr ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2PortSetAttributes", "status", 0,
                           "not-possible");
    }

    int bound_pd = (fn_port_destroy != NULL &&
                    obs_address_is_callable((const void *)fn_port_destroy));
    obs_report_measure(id, "sceAudioOut2PortDestroy", "bound", (uint64_t)bound_pd,
                       "bool");
    if (bound_pd) {
        obs_report_measure(id, "sceAudioOut2PortDestroy", "how",
                           (uint64_t)(is_link_dest ? 1 : 2),
                           is_link_dest ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2PortDestroy", "status", 0, "not-possible");
    }

    if (!bound_pc) {
        return obs_pass();
    }

    /* Context set-up and sceAudioOut2UserCreate(0xff) */
    if (fn_init)
        fn_init();
    uint8_t ctx_param[0x80];
    memset(ctx_param, 0, sizeof(ctx_param));
    if (fn_reset_param)
        fn_reset_param(ctx_param);
    uint64_t mem_size = 0;
    if (fn_query_mem)
        fn_query_mem(ctx_param, &mem_size);
    static _Alignas(16) uint8_t s_ao_mono_ctx_mem[0x200000];
    uint64_t alloc_size =
        (mem_size > 0 && mem_size <= sizeof(s_ao_mono_ctx_mem)) ? mem_size : 0x10000u;
    void *context_out = NULL;
    if (fn_ctx_create)
        fn_ctx_create(ctx_param, s_ao_mono_ctx_mem, alloc_size, &context_out);

    void *user_ff = (void *)0xa5a5a5a5a5a5a5a5ULL;
    int rc_user_ff = -1;
    if (fn_user_create) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user_ff = fn_user_create(0xff, &user_ff);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "UserCreate-0xff", "rc", (uint64_t)(uint32_t)rc_user_ff,
                       "code");
    obs_report_measure(id, "UserCreate-0xff", "user", (uint64_t)(uintptr_t)user_ff,
                       "addr");

    /* 1. 36 iterations of PortCreate(0, param, &port) */
    void *first_port = NULL;
    for (int i = 0; i < 36; i++) {
        char tag[32];
        snprintf(tag, sizeof(tag), "PortCreate-%02d", i);

        uint8_t param[0x38];
        memset(param, 0, sizeof(param));
        *(uint32_t *)(param + 0x00) = 0x100;
        *(uint32_t *)(param + 0x04) = 0x100;
        *(uint32_t *)(param + 0x08) = 48000;
        *(uint32_t *)(param + 0x0c) = 0;
        *(uint64_t *)(param + 0x10) = (uint64_t)(uintptr_t)user_ff;
        uint8_t before_param[0x38];
        memcpy(before_param, param, sizeof(param));

        void *port_out = (void *)0xccccccccccccccccULL;
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_port_create(0, param, &port_out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
        int param_changed = (memcmp(before_param, param, sizeof(param)) != 0) ? 1 : 0;
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, tag, "param-changed", (uint64_t)param_changed, "bool");
        obs_report_bytes(id, tag, "port", 0, (const uint8_t *)&port_out, 8);

        if (rc == 0 && first_port == NULL) {
            first_port = port_out;
        }
    }

    if (first_port == NULL) {
        first_port = (void *)(uintptr_t)0xffffffffffffffffULL;
    }

    /* 2. PortSetAttributes */
    struct obs_ao2_attr {
        uint32_t id;
        uint32_t pad;
        uint64_t value_ptr;
        uint64_t size;
    };

    if (bound_psa) {
        /* n = 1: {0, &u64 v, 8} with v = 0 */
        uint64_t v = 0;
        struct obs_ao2_attr attrs1[1];
        attrs1[0].id = 0;
        attrs1[0].pad = 0;
        attrs1[0].value_ptr = (uint64_t)(uintptr_t)&v;
        attrs1[0].size = 8;

        int rc_n1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_n1 = fn_port_set_attr(first_port, attrs1, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SetAttributes-n1", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SetAttributes-n1", "rc", (uint64_t)(uint32_t)rc_n1,
                           "code");
        obs_report_measure(id, "SetAttributes-n1", "v-before", 0, "val");
        obs_report_measure(id, "SetAttributes-n1", "v-after", v, "val");
        obs_report_bytes(id, "SetAttributes-n1", "v-bytes", 0, (const uint8_t *)&v, 8);

        /* n = 3: {0, &u64 v, 8}, {8, &u32 a, 4}, {4, &u32 b, 4} */
        uint64_t v3 = 0;
        uint32_t a = 0;
        uint32_t b = 0;
        struct obs_ao2_attr attrs3[3];
        attrs3[0].id = 0;
        attrs3[0].pad = 0;
        attrs3[0].value_ptr = (uint64_t)(uintptr_t)&v3;
        attrs3[0].size = 8;
        attrs3[1].id = 8;
        attrs3[1].pad = 0;
        attrs3[1].value_ptr = (uint64_t)(uintptr_t)&a;
        attrs3[1].size = 4;
        attrs3[2].id = 4;
        attrs3[2].pad = 0;
        attrs3[2].value_ptr = (uint64_t)(uintptr_t)&b;
        attrs3[2].size = 4;

        int rc_n3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_n3 = fn_port_set_attr(first_port, attrs3, 3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SetAttributes-n3", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SetAttributes-n3", "rc", (uint64_t)(uint32_t)rc_n3,
                           "code");
        obs_report_measure(id, "SetAttributes-n3", "v-before", 0, "val");
        obs_report_measure(id, "SetAttributes-n3", "v-after", v3, "val");
        obs_report_measure(id, "SetAttributes-n3", "a-before", 0, "val");
        obs_report_measure(id, "SetAttributes-n3", "a-after", (uint64_t)a, "val");
        obs_report_measure(id, "SetAttributes-n3", "b-before", 0, "val");
        obs_report_measure(id, "SetAttributes-n3", "b-after", (uint64_t)b, "val");

        /* 3. The same n = 1 call on port 0xffffffffffffffff */
        uint64_t v_inv = 0;
        struct obs_ao2_attr attrs_inv[1];
        attrs_inv[0].id = 0;
        attrs_inv[0].pad = 0;
        attrs_inv[0].value_ptr = (uint64_t)(uintptr_t)&v_inv;
        attrs_inv[0].size = 8;

        int rc_inv = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_inv = fn_port_set_attr((void *)(uintptr_t)0xffffffffffffffffULL,
                                      attrs_inv, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SetAttributes-n1-invalid", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "SetAttributes-n1-invalid", "rc",
                           (uint64_t)(uint32_t)rc_inv, "code");
    }

    /* 4. PortDestroy on first, repeat, and invalid */
    if (bound_pd) {
        int rc_d1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d1 = fn_port_destroy(first_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortDestroy-first", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortDestroy-first", "rc", (uint64_t)(uint32_t)rc_d1,
                           "code");

        int rc_d2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d2 = fn_port_destroy(first_port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortDestroy-repeat", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortDestroy-repeat", "rc", (uint64_t)(uint32_t)rc_d2,
                           "code");

        int rc_d3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_d3 = fn_port_destroy((void *)(uintptr_t)0xffffffffffffffffULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PortDestroy-invalid", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PortDestroy-invalid", "rc", (uint64_t)(uint32_t)rc_d3,
                           "code");
    }

    return obs_pass();
}

/* REQ-20261012T1232Z-ux01: pad extended info, UDS NID 0x0996125e66b8a5b3, download
 * space */
static obs_result check_ux01_calls(void) {
    const char *id = "000-boot/ux01-calls";
    obs_jmp_buf guard;

    /* 1. scePadDeviceClassGetExtendedInformation */
    typedef int (*fn_pad_ext_t)(int, void *);
    fn_pad_ext_t fn_pad_ext = NULL;
    int is_link_pe = 0;
    if (obs_address_is_callable(
            (const void *)&scePadDeviceClassGetExtendedInformation_static)) {
        fn_pad_ext = (fn_pad_ext_t)&scePadDeviceClassGetExtendedInformation_static;
        is_link_pe = 1;
    } else if (obs_address_is_callable(
                   (const void *)&scePadDeviceClassGetExtendedInformation_be)) {
        fn_pad_ext = (fn_pad_ext_t)&scePadDeviceClassGetExtendedInformation_be;
        is_link_pe = 1;
    }
    if (!fn_pad_ext) {
        fn_pad_ext = (fn_pad_ext_t)obs_module_symbol(OBS_HANDLE_SELF, "$AcslpN1jHR8");
        if (!fn_pad_ext)
            fn_pad_ext =
                (fn_pad_ext_t)obs_module_symbol(OBS_HANDLE_SELF, "$Hx1j3aQlywE");
        if (!fn_pad_ext) {
            int h = obs_module_open("libScePad");
            if (h > 0) {
                fn_pad_ext = (fn_pad_ext_t)obs_module_symbol(h, "$AcslpN1jHR8");
                if (!fn_pad_ext)
                    fn_pad_ext = (fn_pad_ext_t)obs_module_symbol(h, "$Hx1j3aQlywE");
                if (!fn_pad_ext)
                    fn_pad_ext = (fn_pad_ext_t)obs_module_symbol(
                        h, "scePadDeviceClassGetExtendedInformation");
            }
        }
    }
    int bound_pe =
        (fn_pad_ext != NULL && obs_address_is_callable((const void *)fn_pad_ext));
    obs_report_measure(id, "scePadDeviceClassGetExtendedInformation", "bound",
                       (uint64_t)bound_pe, "bool");
    if (bound_pe) {
        obs_report_measure(id, "scePadDeviceClassGetExtendedInformation", "how",
                           (uint64_t)(is_link_pe ? 1 : 2),
                           is_link_pe ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePadDeviceClassGetExtendedInformation", "status", 0,
                           "not-possible");
    }

    /* 2. libSceNpUniversalDataSystem NID 0x0996125e66b8a5b3 */
    typedef uint64_t (*fn_uds_0996_t)(void *, void *, void *, void *, void *, void *);
    fn_uds_0996_t fn_uds_0996 = NULL;
    int is_link_u0996 = 0;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystem_0996125e66b8a5b3_static)) {
        fn_uds_0996 = (fn_uds_0996_t)&sceNpUniversalDataSystem_0996125e66b8a5b3_static;
        is_link_u0996 = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpUniversalDataSystem_0996125e66b8a5b3_be)) {
        fn_uds_0996 = (fn_uds_0996_t)&sceNpUniversalDataSystem_0996125e66b8a5b3_be;
        is_link_u0996 = 1;
    }
    if (!fn_uds_0996) {
        fn_uds_0996 = (fn_uds_0996_t)obs_module_symbol(OBS_HANDLE_SELF, "$s6W4Zl4Slgk");
        if (!fn_uds_0996)
            fn_uds_0996 =
                (fn_uds_0996_t)obs_module_symbol(OBS_HANDLE_SELF, "$CZYSXma4pbM");
        if (!fn_uds_0996) {
            int h = obs_module_open("libSceNpUniversalDataSystem");
            if (h > 0) {
                fn_uds_0996 = (fn_uds_0996_t)obs_module_symbol(h, "$s6W4Zl4Slgk");
                if (!fn_uds_0996)
                    fn_uds_0996 = (fn_uds_0996_t)obs_module_symbol(h, "$CZYSXma4pbM");
                if (!fn_uds_0996)
                    fn_uds_0996 = (fn_uds_0996_t)obs_module_symbol(
                        h, "sceNpUniversalDataSystemCreateEventPropertyObject");
                if (!fn_uds_0996)
                    fn_uds_0996 =
                        (fn_uds_0996_t)obs_module_symbol(h, "0x0996125e66b8a5b3");
            }
        }
    }
    int bound_u0996 =
        (fn_uds_0996 != NULL && obs_address_is_callable((const void *)fn_uds_0996));
    obs_report_measure(id, "sceNpUniversalDataSystem_0996125e66b8a5b3", "bound",
                       (uint64_t)bound_u0996, "bool");
    if (bound_u0996) {
        obs_report_measure(id, "sceNpUniversalDataSystem_0996125e66b8a5b3", "how",
                           (uint64_t)(is_link_u0996 ? 1 : 2),
                           is_link_u0996 ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpUniversalDataSystem_0996125e66b8a5b3", "status", 0,
                           "not-possible");
    }

    /* 3. sceAppContentDownloadDataGetAvailableSpaceKb */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x00B4); /* OOPS_SYSMODULE_APP_CONTENT */
    }
    typedef int (*fn_dl_space_t)(const char *, void *);
    fn_dl_space_t fn_dl_space = NULL;
    int is_link_dls = 0;
    if (obs_address_is_callable(
            (const void *)&sceAppContentDownloadDataGetAvailableSpaceKb_static)) {
        fn_dl_space =
            (fn_dl_space_t)&sceAppContentDownloadDataGetAvailableSpaceKb_static;
        is_link_dls = 1;

    } else if (obs_address_is_callable(
                   (const void *)&sceAppContentDownloadDataGetAvailableSpaceKb_be)) {
        fn_dl_space = (fn_dl_space_t)&sceAppContentDownloadDataGetAvailableSpaceKb_be;
        is_link_dls = 1;
    }
    if (!fn_dl_space) {
        fn_dl_space = (fn_dl_space_t)obs_module_symbol(OBS_HANDLE_SELF, "$Gl6w5i0JokY");
        if (!fn_dl_space)
            fn_dl_space =
                (fn_dl_space_t)obs_module_symbol(OBS_HANDLE_SELF, "$RqIJLeawXho");
        if (!fn_dl_space) {
            int h = obs_module_open("libSceAppContent");
            if (h > 0) {
                fn_dl_space = (fn_dl_space_t)obs_module_symbol(h, "$Gl6w5i0JokY");
                if (!fn_dl_space)
                    fn_dl_space = (fn_dl_space_t)obs_module_symbol(h, "$RqIJLeawXho");
                if (!fn_dl_space)
                    fn_dl_space = (fn_dl_space_t)obs_module_symbol(
                        h, "sceAppContentDownloadDataGetAvailableSpaceKb");
            }
        }
    }
    int bound_dls =
        (fn_dl_space != NULL && obs_address_is_callable((const void *)fn_dl_space));
    obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "bound",
                       (uint64_t)bound_dls, "bool");
    if (bound_dls) {
        obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "how",
                           (uint64_t)(is_link_dls ? 1 : 2),
                           is_link_dls ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "status",
                           0, "not-possible");
    }

    /* Execution of 1: Pad extended info */
    if (bound_pe) {
        int32_t user = get_boot_user();
        if (obs_address_is_callable((const void *)&scePadInit)) {
            scePadInit();
        }
        int pad_h = -1;
        if (obs_address_is_callable((const void *)&scePadOpen)) {
            pad_h = scePadOpen(user, 0, 0, NULL);
        }
        uint8_t cinfo[0x80];
        memset(cinfo, 0, sizeof(cinfo));
        int rc_cinfo = -1;
        if (pad_h > 0 && obs_address_is_callable(
                             (const void *)&scePadGetControllerInformation_static)) {
            rc_cinfo = scePadGetControllerInformation_static(pad_h, cinfo);
        }
        obs_report_measure(id, "pad-controller-info", "rc",
                           (uint64_t)(uint32_t)rc_cinfo, "code");
        obs_report_bytes(id, "pad-controller-info", "cinfo-32b", 0, cinfo, 32);

        uint8_t out[0x100];
        memset(out, 0xcc, sizeof(out));
        uint8_t before[0x100];
        memset(before, 0xcc, sizeof(before));
        int rc_pe = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pe = fn_pad_ext(pad_h, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PadGetExtendedInfo-open", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PadGetExtendedInfo-open", "rc",
                           (uint64_t)(uint32_t)rc_pe, "code");
        obs_report_written(id, "PadGetExtendedInfo-open", "bytes-changed", before, out,
                           sizeof(out));
        obs_report_bytes(id, "PadGetExtendedInfo-open", "out-20h", 0, out, 0x20);
        obs_report_bytes(id, "PadGetExtendedInfo-open", "out-80h", 0, out, 0x80);

        uint8_t bad_out[0x100];
        memset(bad_out, 0xcc, sizeof(bad_out));
        int rc_bad_pe = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad_pe = fn_pad_ext(0x7fffffff, bad_out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "PadGetExtendedInfo-7fffffff", "rc",
                           (uint64_t)(uint32_t)rc_bad_pe, "code");

        /* REQ-20261015T0210Z-pd01: scePadDeviceClassParseData */
        typedef int (*fn_pad_parse_t)(int, const void *, void *);
        fn_pad_parse_t fn_pdp = NULL;
        int is_link_pdp = 0;
        if (obs_address_is_callable((const void *)&scePadDeviceClassParseData_static)) {
            fn_pdp = (fn_pad_parse_t)&scePadDeviceClassParseData_static;
            is_link_pdp = 1;
        }
        if (!fn_pdp) {
            fn_pdp = (fn_pad_parse_t)census_resolve_symbol(
                "libScePad", "$IHPqcbc0zCA", "scePadDeviceClassParseData", NULL, NULL);
        }
        if (!fn_pdp) {
            fn_pdp = (fn_pad_parse_t)obs_module_symbol(OBS_HANDLE_SELF, "$IHPqcbc0zCA");
            if (fn_pdp)
                is_link_pdp = 1;
        }
        if (!fn_pdp) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0) {
                fn_pdp = (fn_pad_parse_t)obs_module_symbol(h_pad, "$IHPqcbc0zCA");
                if (!fn_pdp)
                    fn_pdp = (fn_pad_parse_t)obs_module_symbol(
                        h_pad, "scePadDeviceClassParseData");
            }
        }
        int bound_pdp =
            (fn_pdp != NULL && obs_address_is_callable((const void *)fn_pdp));
        obs_report_measure(id, "scePadDeviceClassParseData", "bound",
                           (uint64_t)bound_pdp, "bool");
        if (bound_pdp) {
            obs_report_measure(id, "scePadDeviceClassParseData", "how",
                               (uint64_t)(is_link_pdp ? 1 : 2),
                               is_link_pdp ? "link" : "dlsym");
            uint8_t read_data[0x80];
            memset(read_data, 0, sizeof(read_data));
            if (obs_address_is_callable((const void *)&scePadRead)) {
                scePadRead(pad_h, read_data, 1);
            }

            /* Arm 2: One parse twice */
            uint8_t out1[0x100];
            memset(out1, 0xcc, sizeof(out1));
            int rc_p1 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p1 = fn_pdp(pad_h, read_data, out1);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "pad-device-class-parse-1", "rc",
                               (uint64_t)(uint32_t)rc_p1, "code");
            obs_report_bytes(id, "pad-device-class-parse-1", "out-100b", 0, out1,
                             0x100);

            uint8_t out2[0x100];
            memset(out2, 0xcc, sizeof(out2));
            int rc_p2 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p2 = fn_pdp(pad_h, read_data, out2);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "pad-device-class-parse-2", "rc",
                               (uint64_t)(uint32_t)rc_p2, "code");
            obs_report_bytes(id, "pad-device-class-parse-2", "out-100b", 0, out2,
                             0x100);

            /* Arm 3: Null out */
            int rc_p_null = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p_null = fn_pdp(pad_h, read_data, NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "pad-device-class-parse-null", "rc",
                               (uint64_t)(uint32_t)rc_p_null, "code");
        } else {
            obs_report_measure(id, "scePadDeviceClassParseData", "status", 0,
                               "not-possible");
        }

        /* REQ-20261013T1500Z-pd02: Pad reset lightbar and trigger effect */
        typedef int (*fn_pad_rl_t)(int);
        typedef int (*fn_pad_te_t)(int, const void *);
        fn_pad_rl_t fn_rl =
            obs_address_is_callable((const void *)&scePadResetLightBar_static)
                ? (fn_pad_rl_t)&scePadResetLightBar_static
                : NULL;
        fn_pad_te_t fn_te =
            obs_address_is_callable((const void *)&scePadSetTriggerEffect_static)
                ? (fn_pad_te_t)&scePadSetTriggerEffect_static
                : NULL;

        if (!fn_rl) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_rl = (fn_pad_rl_t)obs_module_symbol(h_pad, "$DscD1i9HX1w");
        }
        if (!fn_te) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_te = (fn_pad_te_t)obs_module_symbol(h_pad, "$2JgFB2n9oUM");
        }

        if (fn_rl) {
            int rc_rl_open = fn_rl(pad_h);
            obs_report_measure(id, "PadResetLightBar-open", "rc",
                               (uint64_t)(uint32_t)rc_rl_open, "code");
            int rc_rl_bad = fn_rl(0x7fffffff);
            obs_report_measure(id, "PadResetLightBar-7fffffff", "rc",
                               (uint64_t)(uint32_t)rc_rl_bad, "code");
        }

        if (fn_te) {
            uint8_t tr_param[0x80] = {0};
            tr_param[0] = 3; /* both triggers */
            int rc_te_open = fn_te(pad_h, tr_param);
            obs_report_measure(id, "PadSetTriggerEffect-open", "rc",
                               (uint64_t)(uint32_t)rc_te_open, "code");

            int rc_te_null = fn_te(pad_h, NULL);
            obs_report_measure(id, "PadSetTriggerEffect-null-param", "rc",
                               (uint64_t)(uint32_t)rc_te_null, "code");

            int rc_te_bad = fn_te(0x7fffffff, tr_param);
            obs_report_measure(id, "PadSetTriggerEffect-7fffffff", "rc",
                               (uint64_t)(uint32_t)rc_te_bad, "code");
        }

        /* REQ-20261013T2030Z-pt01: TiltCorrection & GetTriggerEffectState */
        typedef int (*fn_pad_tilt_t)(int, int);
        fn_pad_tilt_t fn_tilt =
            obs_address_is_callable((const void *)&scePadSetTiltCorrectionState_static)
                ? (fn_pad_tilt_t)&scePadSetTiltCorrectionState_static
                : NULL;
        if (!fn_tilt) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_tilt = (fn_pad_tilt_t)obs_module_symbol(h_pad, "$vDLMoJLde8I");
        }
        if (!fn_tilt) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_tilt = (fn_pad_tilt_t)obs_module_symbol(
                    h_pad, "scePadSetTiltCorrectionState");
        }

        if (fn_tilt) {
            int rc_t1 = fn_tilt(pad_h, 1);
            obs_report_measure(id, "PadSetTiltCorrectionState-open-1", "rc",
                               (uint64_t)(uint32_t)rc_t1, "code");
            int rc_t0 = fn_tilt(pad_h, 0);
            obs_report_measure(id, "PadSetTiltCorrectionState-open-0", "rc",
                               (uint64_t)(uint32_t)rc_t0, "code");
            int rc_tbad = fn_tilt(0x7fffffff, 0);
            obs_report_measure(id, "PadSetTiltCorrectionState-7fffffff", "rc",
                               (uint64_t)(uint32_t)rc_tbad, "code");
        }

        typedef int (*fn_pad_gte_t)(int, void *, size_t);
        fn_pad_gte_t fn_gte =
            obs_address_is_callable((const void *)&scePadGetTriggerEffectState_static)
                ? (fn_pad_gte_t)&scePadGetTriggerEffectState_static
                : NULL;
        if (!fn_gte) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_gte = (fn_pad_gte_t)obs_module_symbol(h_pad, "$znaWI0gpuo8");
        }
        if (!fn_gte) {
            int h_pad = obs_module_open("libScePad");
            if (h_pad > 0)
                fn_gte = (fn_pad_gte_t)obs_module_symbol(h_pad,
                                                         "scePadGetTriggerEffectState");
        }

        if (fn_gte) {
            uint8_t gte_buf[0x40];
            memset(gte_buf, 0xcc, sizeof(gte_buf));
            int rc_gte_open = fn_gte(pad_h, gte_buf, 0x40);
            obs_report_measure(id, "PadGetTriggerEffectState-open", "rc",
                               (uint64_t)(uint32_t)rc_gte_open, "code");
            obs_report_bytes(id, "PadGetTriggerEffectState-open", "state-40b", 0,
                             gte_buf, 0x40);

            int rc_gte_null = fn_gte(pad_h, NULL, 0x40);
            obs_report_measure(id, "PadGetTriggerEffectState-null-param", "rc",
                               (uint64_t)(uint32_t)rc_gte_null, "code");

            memset(gte_buf, 0xcc, sizeof(gte_buf));
            int rc_gte_bad = fn_gte(0x7fffffff, gte_buf, 0x40);
            obs_report_measure(id, "PadGetTriggerEffectState-7fffffff", "rc",
                               (uint64_t)(uint32_t)rc_gte_bad, "code");
        }

        if (pad_h > 0 && obs_address_is_callable((const void *)&scePadClose)) {
            scePadClose(pad_h);
        }
    }

    /* Execution of 2: UDS 0x0996125e66b8a5b3 */
    if (bound_u0996) {
        int32_t user = get_boot_user();
        typedef int (*fn_uds_init_t)(const void *, const void *, uint64_t, uint64_t);
        typedef int (*fn_uds_ctx_t)(uint64_t *, int32_t, uint64_t, uint64_t);
        typedef int (*fn_uds_handle_t)(uint64_t *);

        fn_uds_init_t fn_uds_init =
            obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemInitialize_static)
                ? (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_static
                : (obs_address_is_callable(
                       (const void *)&sceNpUniversalDataSystemInitialize_be)
                       ? (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_be
                       : NULL);
        fn_uds_ctx_t fn_uds_ctx =
            obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemCreateContext_static)
                ? (fn_uds_ctx_t)&sceNpUniversalDataSystemCreateContext_static
                : NULL;
        fn_uds_handle_t fn_uds_handle =
            obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemCreateHandle_static)
                ? (fn_uds_handle_t)&sceNpUniversalDataSystemCreateHandle_static
                : NULL;

        int rc_uds_mod = -1;
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            rc_uds_mod = sceSysmoduleLoadModule(0x0105);
        }
        obs_report_measure(id, "uds-setup", "sysmodule-load-0x0105-rc",
                           (uint64_t)(uint32_t)rc_uds_mod, "code");

        static uint8_t s_uds_pool_ux[0x40000] __attribute__((aligned(64)));
        uint64_t uds_param[4];
        uds_param[0] = 0x10;
        uds_param[1] = 0x40000;
        uds_param[2] = (uint64_t)(uintptr_t)s_uds_pool_ux;
        uds_param[3] = 0x10;

        int rc_uds_init = -1;
        if (fn_uds_init)
            rc_uds_init = fn_uds_init(uds_param, NULL, 0, 0);
        obs_report_measure(id, "uds-setup", "init-rc", (uint64_t)(uint32_t)rc_uds_init,
                           "code");

        uint64_t ctx = 0;
        int rc_uds_ctx = -1;
        if (fn_uds_ctx)
            rc_uds_ctx = fn_uds_ctx(&ctx, user, 0, 0);
        obs_report_measure(id, "uds-setup", "create-context-rc",
                           (uint64_t)(uint32_t)rc_uds_ctx, "code");
        obs_report_measure(id, "uds-setup", "context-val", ctx, "hex");

        uint64_t h = 0;
        int rc_uds_h = -1;
        if (fn_uds_handle)
            rc_uds_h = fn_uds_handle(&h);
        obs_report_measure(id, "uds-setup", "create-handle-rc",
                           (uint64_t)(uint32_t)rc_uds_h, "code");
        obs_report_measure(id, "uds-setup", "handle-val", h, "hex");

        /* REQ-20261013T1730Z-rf01 item 4:
         * s6W4Zl4Slgk(context, handle, name, name + 1, buf, buf) with name "activityId"
         * and buf 0x100 of 0xcc */
        const char *name_act = "activityId";
        uint8_t uds_b4[0x100];
        uint8_t uds_b5[0x100];
        memset(uds_b4, 0xcc, sizeof(uds_b4));
        memset(uds_b5, 0xcc, sizeof(uds_b5));
        uint8_t before_uds_cc[0x100];
        memset(before_uds_cc, 0xcc, sizeof(before_uds_cc));

        uint64_t rc_uds_act = 0xffffffffffffffffULL;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_uds_act =
                fn_uds_0996((void *)(uintptr_t)ctx, (void *)(uintptr_t)h,
                            (void *)name_act, (void *)(name_act + 1), uds_b4, uds_b5);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "uds-activity-call", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "uds-activity-call", "rc", rc_uds_act, "code");
        obs_report_written(id, "uds-activity-call", "buf4-changed", before_uds_cc,
                           uds_b4, sizeof(uds_b4));
        obs_report_written(id, "uds-activity-call", "buf5-changed", before_uds_cc,
                           uds_b5, sizeof(uds_b5));
        if (memcmp(before_uds_cc, uds_b4, sizeof(uds_b4)) != 0) {
            obs_report_bytes(id, "uds-activity-call", "buf4-out", 0, uds_b4, 64);
        }
        if (memcmp(before_uds_cc, uds_b5, sizeof(uds_b5)) != 0) {
            obs_report_bytes(id, "uds-activity-call", "buf5-out", 0, uds_b5, 64);
        }

        /* Then the same with every argument zero */
        uint64_t rc_uds_zero = 0xffffffffffffffffULL;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_uds_zero = fn_uds_0996(NULL, NULL, NULL, NULL, NULL, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "uds-allzero-call", "rc", rc_uds_zero, "code");

        /* Census pair: every argument zero */
        uint64_t rc0 = 0;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc0 = fn_uds_0996(NULL, NULL, NULL, NULL, NULL, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "uds-0996125e-allzero", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "uds-0996125e-allzero", "rc", rc0, "code");

        /* Census pair: every argument a 0x100-byte block filled 0xcc */
        uint8_t b0[0x100], b1[0x100], b2[0x100], b3[0x100], b4[0x100], b5[0x100];
        uint8_t before_cc[0x100];
        memset(b0, 0xcc, 0x100);
        memset(b1, 0xcc, 0x100);
        memset(b2, 0xcc, 0x100);
        memset(b3, 0xcc, 0x100);
        memset(b4, 0xcc, 0x100);
        memset(b5, 0xcc, 0x100);
        memset(before_cc, 0xcc, 0x100);

        uint64_t rc1 = 0;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc1 = fn_uds_0996(b0, b1, b2, b3, b4, b5);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "uds-0996125e-allbuf", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "uds-0996125e-allbuf", "rc", rc1, "code");

        uint8_t *bufs[6] = {b0, b1, b2, b3, b4, b5};
        for (int k = 0; k < 6; k++) {
            char tag_ch[32];
            snprintf(tag_ch, sizeof(tag_ch), "b%d-changed", k);
            obs_report_written(id, "uds-0996125e-allbuf", tag_ch, before_cc, bufs[k],
                               0x100);
            if (memcmp(before_cc, bufs[k], 0x100) != 0) {
                char tag_by[32];
                snprintf(tag_by, sizeof(tag_by), "b%d-bytes", k);
                obs_report_bytes(id, "uds-0996125e-allbuf", tag_by, 0, bufs[k], 0x80);
            }
        }

        /* REQ-20261015T0653Z-ud01: SetString and DestroyHandle */
        typedef int (*fn_uds_set_str_t)(uint64_t, const char *, const char *);
        typedef int (*fn_uds_dst_h_t)(uint64_t);

        fn_uds_set_str_t fn_setstr = NULL;
        int is_link_setstr = 0;
        if (obs_address_is_callable(
                (const void
                     *)&sceNpUniversalDataSystemEventPropertyObjectSetString_static)) {
            fn_setstr = &sceNpUniversalDataSystemEventPropertyObjectSetString_static;
            is_link_setstr = 1;
        } else {
            fn_setstr = (fn_uds_set_str_t)census_resolve_symbol(
                "libSceNpUniversalDataSystem", "$MfDb+4Nln64",
                "sceNpUniversalDataSystemEventPropertyObjectSetString", NULL, NULL);
            if (!fn_setstr) {
                fn_setstr = (fn_uds_set_str_t)obs_module_symbol(OBS_HANDLE_SELF,
                                                                "$MfDb+4Nln64");
                if (fn_setstr)
                    is_link_setstr = 1;
            }
            if (!fn_setstr) {
                fn_setstr = (fn_uds_set_str_t)obs_module_symbol(OBS_HANDLE_SELF,
                                                                "$MfDb-4Nln64");
                if (fn_setstr)
                    is_link_setstr = 1;
            }
        }

        fn_uds_dst_h_t fn_dst_h = NULL;
        int is_link_dst_h = 0;
        if (obs_address_is_callable(
                (const void *)&sceNpUniversalDataSystemDestroyHandle_static)) {
            fn_dst_h = &sceNpUniversalDataSystemDestroyHandle_static;
            is_link_dst_h = 1;
        } else {
            fn_dst_h = (fn_uds_dst_h_t)census_resolve_symbol(
                "libSceNpUniversalDataSystem", "$AUIHb7jUX3I",
                "sceNpUniversalDataSystemDestroyHandle", NULL, NULL);
            if (!fn_dst_h) {
                fn_dst_h =
                    (fn_uds_dst_h_t)obs_module_symbol(OBS_HANDLE_SELF, "$AUIHb7jUX3I");
                if (fn_dst_h)
                    is_link_dst_h = 1;
            }
        }

        int bound_setstr =
            (fn_setstr != NULL && obs_address_is_callable((const void *)fn_setstr));
        obs_report_measure(id, "sceNpUniversalDataSystemEventPropertyObjectSetString",
                           "bound", (uint64_t)bound_setstr, "bool");
        if (bound_setstr) {
            obs_report_measure(
                id, "sceNpUniversalDataSystemEventPropertyObjectSetString", "how",
                (uint64_t)(is_link_setstr ? 1 : 2), is_link_setstr ? "link" : "dlsym");
        } else {
            obs_report_measure(id,
                               "sceNpUniversalDataSystemEventPropertyObjectSetString",
                               "status", 0, "not-possible");
        }

        int bound_dst_h =
            (fn_dst_h != NULL && obs_address_is_callable((const void *)fn_dst_h));
        obs_report_measure(id, "sceNpUniversalDataSystemDestroyHandle", "bound",
                           (uint64_t)bound_dst_h, "bool");
        if (bound_dst_h) {
            obs_report_measure(id, "sceNpUniversalDataSystemDestroyHandle", "how",
                               (uint64_t)(is_link_dst_h ? 1 : 2),
                               is_link_dst_h ? "link" : "dlsym");
        } else {
            obs_report_measure(id, "sceNpUniversalDataSystemDestroyHandle", "status", 0,
                               "not-possible");
        }

        /* 2. SetString on property object */
        uint64_t prop_obj = *(uint64_t *)b0;
        int rc_cpo = (prop_obj != 0 && prop_obj != 0xccccccccccccccccULL) ? 0 : -1;
        obs_report_measure(id, "uds-create-prop-obj", "rc", (uint64_t)(uint32_t)rc_cpo,
                           "code");

        if (fn_setstr && bound_setstr) {
            int rc_s1 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_s1 = fn_setstr(prop_obj, "activityId", "a1");
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "uds-set-string-a1", "rc", (uint64_t)(uint32_t)rc_s1,
                               "code");

            int rc_s2 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_s2 = fn_setstr(prop_obj, "activityId", "a2");
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "uds-set-string-a2", "rc", (uint64_t)(uint32_t)rc_s2,
                               "code");

            int rc_snull = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_snull = fn_setstr(prop_obj, "activityId", NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "uds-set-string-null", "rc",
                               (uint64_t)(uint32_t)rc_snull, "code");
        }

        /* 3. DestroyHandle twice */
        if (fn_dst_h && bound_dst_h) {
            int rc_dh1 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_dh1 = fn_dst_h(h);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "uds-destroy-handle-1", "rc",
                               (uint64_t)(uint32_t)rc_dh1, "code");

            int rc_dh2 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_dh2 = fn_dst_h(h);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "uds-destroy-handle-2", "rc",
                               (uint64_t)(uint32_t)rc_dh2, "code");
        }
    }

    /* Execution of 3: AppContent download space */
    if (bound_dls) {
        if (obs_address_is_callable((const void *)&sceAppContentInitialize_static)) {
            sceAppContentInitialize_static(NULL);
        } else {
            int h = obs_module_open("libSceAppContent");
            if (h > 0) {
                typedef int (*fn_app_init_t)(void *);
                fn_app_init_t fn_ai =
                    (fn_app_init_t)obs_module_symbol(h, "sceAppContentInitialize");
                if (!fn_ai)
                    fn_ai = (fn_app_init_t)obs_module_symbol(h, "$R9lA82OraNs");
                if (fn_ai)
                    fn_ai(NULL);
            }
        }
        char mp0[16] = {0};
        snprintf(mp0, sizeof(mp0), "/download0");
        char mp1[16] = {0};
        snprintf(mp1, sizeof(mp1), "/download1");

        uint64_t kb0 = 0xccccccccccccccccULL;
        int rc_dls = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_dls = fn_dl_space(mp0, &kb0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "DownloadDataGetAvailableSpaceKb", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "DownloadDataGetAvailableSpaceKb", "rc",
                           (uint64_t)(uint32_t)rc_dls, "code");
        obs_report_bytes(id, "DownloadDataGetAvailableSpaceKb", "kb-bytes", 0,
                         (const uint8_t *)&kb0, 8);
        obs_report_measure(id, "download0-kb", "rc", (uint64_t)(uint32_t)rc_dls,
                           "code");
        obs_report_bytes(id, "download0-kb", "kb-bytes", 0, (const uint8_t *)&kb0, 8);

        /* 2. kb null */
        int rc_dls_null_kb = -1;
        int sig_null_kb = OBS_FAULT_ARM(&guard);
        if (sig_null_kb == 0) {
            rc_dls_null_kb = fn_dl_space(mp0, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "download0-null-kb", "rc",
                           (uint64_t)(uint32_t)rc_dls_null_kb, "code");

        /* 3. mount null */
        uint64_t kb_null_mp = 0xccccccccccccccccULL;
        int rc_dls_null_mp = -1;
        int sig_null_mp = OBS_FAULT_ARM(&guard);
        if (sig_null_mp == 0) {
            rc_dls_null_mp = fn_dl_space(NULL, &kb_null_mp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "null-mount-kb", "rc",
                           (uint64_t)(uint32_t)rc_dls_null_mp, "code");

        /* 4. /download1 */
        uint64_t kb1 = 0xccccccccccccccccULL;
        int rc_dls1 = -1;
        int sig1 = OBS_FAULT_ARM(&guard);
        if (sig1 == 0) {
            rc_dls1 = fn_dl_space(mp1, &kb1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "download1-kb", "rc", (uint64_t)(uint32_t)rc_dls1,
                           "code");
        obs_report_bytes(id, "download1-kb", "kb-bytes", 0, (const uint8_t *)&kb1, 8);
    }

    /* TemporaryData space queries (REQ-pc01 & REQ-0442) */
    typedef int (*fn_tm_mount_t)(int, void *);
    typedef int (*fn_tm_space_t)(const char *, void *);
    typedef int (*fn_tm_format_t)(const char *);
    typedef int (*fn_ac_mount_t)(uint32_t, const char *, void *);
    typedef int (*fn_ac_unmount_t)(const char *);

    fn_tm_mount_t fn_t_mount =
        obs_address_is_callable((const void *)&sceAppContentTemporaryDataMount2_static)
            ? (fn_tm_mount_t)&sceAppContentTemporaryDataMount2_static
            : NULL;
    fn_tm_space_t fn_t_space =
        obs_address_is_callable(
            (const void *)&sceAppContentTemporaryDataGetAvailableSpaceKb_static)
            ? (fn_tm_space_t)&sceAppContentTemporaryDataGetAvailableSpaceKb_static
            : NULL;
    fn_tm_format_t fn_t_format =
        obs_address_is_callable((const void *)&sceAppContentTemporaryDataFormat_static)
            ? (fn_tm_format_t)&sceAppContentTemporaryDataFormat_static
            : NULL;
    fn_ac_mount_t fn_ac_mount =
        obs_address_is_callable((const void *)&sceAppContentAddcontMount_static)
            ? (fn_ac_mount_t)&sceAppContentAddcontMount_static
            : NULL;
    fn_ac_unmount_t fn_ac_unmount =
        obs_address_is_callable((const void *)&sceAppContentAddcontUnmount_static)
            ? (fn_ac_unmount_t)&sceAppContentAddcontUnmount_static
            : NULL;

    int h_ac = obs_module_open("libSceAppContent");
    if (h_ac > 0) {
        if (!fn_t_mount)
            fn_t_mount = (fn_tm_mount_t)obs_module_symbol(h_ac, "$buYbeLOGWmA");
        if (!fn_t_space)
            fn_t_space = (fn_tm_space_t)obs_module_symbol(h_ac, "$SaKib2Ug0yI");
        if (!fn_t_format)
            fn_t_format = (fn_tm_format_t)obs_module_symbol(h_ac, "$a5N7lAG0y2Q");
        if (!fn_ac_mount)
            fn_ac_mount = (fn_ac_mount_t)obs_module_symbol(h_ac, "$b-Q+iFzM3e4");
        if (!fn_ac_unmount)
            fn_ac_unmount = (fn_ac_unmount_t)obs_module_symbol(h_ac, "$3rHWaV-1KC4");
    }

    uint8_t t_mp[16];
    memset(t_mp, 0xcc, sizeof(t_mp));
    if (fn_t_mount) {
        int rc_tm = -1;
        int sig_tm = OBS_FAULT_ARM(&guard);
        if (sig_tm == 0) {
            rc_tm = fn_t_mount(1, t_mp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "temp-mount2", "rc", (uint64_t)(uint32_t)rc_tm, "code");
        obs_report_bytes(id, "temp-mount2", "mp-16b", 0, t_mp, 16);
    }

    if (fn_t_space) {
        uint64_t tkb = 0xccccccccccccccccULL;
        int rc_ts = -1;
        int sig_ts = OBS_FAULT_ARM(&guard);
        if (sig_ts == 0) {
            rc_ts = fn_t_space(t_mp[0] == '/' ? (const char *)t_mp : "/temp0", &tkb);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "temp0-kb", "rc", (uint64_t)(uint32_t)rc_ts, "code");
        obs_report_bytes(id, "temp0-kb", "kb-bytes", 0, (const uint8_t *)&tkb, 8);

        /* with nonexistent mount */
        uint64_t nkb = 0xccccccccccccccccULL;
        int rc_nonexist = -1;
        sig_ts = OBS_FAULT_ARM(&guard);
        if (sig_ts == 0) {
            rc_nonexist = fn_t_space("/nonexistent", &nkb);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "temp-nonexistent-kb", "rc",
                           (uint64_t)(uint32_t)rc_nonexist, "code");
    }

    /* TemporaryDataFormat */
    if (fn_t_format) {
        int rc_fmt = -1;
        int sig_fmt = OBS_FAULT_ARM(&guard);
        if (sig_fmt == 0) {
            rc_fmt = fn_t_format(t_mp[0] == '/' ? (const char *)t_mp : "/temp0");
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "temp-format-mp", "rc", (uint64_t)(uint32_t)rc_fmt,
                           "code");

        int rc_fmt_null = -1;
        sig_fmt = OBS_FAULT_ARM(&guard);
        if (sig_fmt == 0) {
            rc_fmt_null = fn_t_format(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "temp-format-null", "rc",
                           (uint64_t)(uint32_t)rc_fmt_null, "code");
    }

    /* AddcontMount & AddcontUnmount */
    if (fn_ac_mount) {
        char label_buf[20];
        memset(label_buf, 0, sizeof(label_buf));
        memcpy(label_buf, "OBSCENEPROBE0000000", 19);
        uint8_t ac_mp[16];
        memset(ac_mp, 0xcc, sizeof(ac_mp));

        int rc_ac = -1;
        int sig_ac = OBS_FAULT_ARM(&guard);
        if (sig_ac == 0) {
            rc_ac = fn_ac_mount(0, label_buf, ac_mp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "addcont-mount", "rc", (uint64_t)(uint32_t)rc_ac,
                           "code");
        obs_report_bytes(id, "addcont-mount", "mp-16b", 0, ac_mp, 16);

        int rc_ac_null_lbl = -1;
        sig_ac = OBS_FAULT_ARM(&guard);
        if (sig_ac == 0) {
            rc_ac_null_lbl = fn_ac_mount(0, NULL, ac_mp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "addcont-mount-null-lbl", "rc",
                           (uint64_t)(uint32_t)rc_ac_null_lbl, "code");

        int rc_ac_null_mp = -1;
        sig_ac = OBS_FAULT_ARM(&guard);
        if (sig_ac == 0) {
            rc_ac_null_mp = fn_ac_mount(0, label_buf, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "addcont-mount-null-mp", "rc",
                           (uint64_t)(uint32_t)rc_ac_null_mp, "code");
    }

    if (fn_ac_unmount) {
        char unmount_mp[16];
        memset(unmount_mp, 0, sizeof(unmount_mp));
        memcpy(unmount_mp, "/addcont0", 9);

        int rc_un = -1;
        int sig_un = OBS_FAULT_ARM(&guard);
        if (sig_un == 0) {
            rc_un = fn_ac_unmount(unmount_mp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "addcont-unmount", "rc", (uint64_t)(uint32_t)rc_un,
                           "code");

        int rc_un_null = -1;
        sig_un = OBS_FAULT_ARM(&guard);
        if (sig_un == 0) {
            rc_un_null = fn_ac_unmount(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "addcont-unmount-null", "rc",
                           (uint64_t)(uint32_t)rc_un_null, "code");
    }

    /* Param.sfo download data size */
    obs_report_measure(id, "param-sfo", "download-data-size", 0, "bytes");

    return obs_pass();
}

/* REQ-20261012T1251Z-pg01: PlayGo startup sequence */
static obs_result check_playgo_startup(void) {
    const char *id = "000-boot/playgo-startup";
    obs_jmp_buf guard;

    int h_pg = obs_module_open("libScePlayGo");

    typedef int (*fn_pg_init_t)(const void *);
    typedef int (*fn_pg_open_t)(int32_t *, uint32_t);
    typedef int (*fn_pg_speed_t)(int32_t, uint32_t);
    typedef int (*fn_pg_chunk_t)(int32_t, void *, uint32_t, uint32_t *);
    typedef int (*fn_pg_todo_t)(int32_t, void *, uint32_t, uint32_t *);
    typedef int (*fn_pg_close_t)(int32_t);
    typedef int (*fn_pg_term_t)(void);

    fn_pg_init_t fn_init = NULL;
    int is_link_init = 0;
    if (obs_address_is_callable((const void *)&scePlayGoInitialize_static)) {
        fn_init = (fn_pg_init_t)&scePlayGoInitialize_static;
        is_link_init = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoInitialize_be)) {
        fn_init = (fn_pg_init_t)&scePlayGoInitialize_be;
        is_link_init = 1;
    }
    if (!fn_init) {
        fn_init = (fn_pg_init_t)obs_module_symbol(OBS_HANDLE_SELF, "$sUaKk5WGzrY");
        if (!fn_init)
            fn_init = (fn_pg_init_t)obs_module_symbol(OBS_HANDLE_SELF, "$ts6GlZOKRrE");
        if (!fn_init && h_pg > 0) {
            fn_init = (fn_pg_init_t)obs_module_symbol(h_pg, "$sUaKk5WGzrY");
            if (!fn_init)
                fn_init = (fn_pg_init_t)obs_module_symbol(h_pg, "$ts6GlZOKRrE");
            if (!fn_init)
                fn_init = (fn_pg_init_t)obs_module_symbol(h_pg, "scePlayGoInitialize");
        }
    }
    int bound_init =
        (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "scePlayGoInitialize", "bound", (uint64_t)bound_init,
                       "bool");
    if (bound_init) {
        obs_report_measure(id, "scePlayGoInitialize", "how",
                           (uint64_t)(is_link_init ? 1 : 2),
                           is_link_init ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoInitialize", "status", 0, "not-possible");
    }

    fn_pg_open_t fn_open = NULL;
    int is_link_open = 0;
    if (obs_address_is_callable((const void *)&scePlayGoOpen_static)) {
        fn_open = (fn_pg_open_t)&scePlayGoOpen_static;
        is_link_open = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoOpen_be)) {
        fn_open = (fn_pg_open_t)&scePlayGoOpen_be;
        is_link_open = 1;
    }
    if (!fn_open) {
        fn_open = (fn_pg_open_t)obs_module_symbol(OBS_HANDLE_SELF, "$YawcWmumUTM");
        if (!fn_open)
            fn_open = (fn_pg_open_t)obs_module_symbol(OBS_HANDLE_SELF, "$M1Gma1ocrGE");
        if (!fn_open && h_pg > 0) {
            fn_open = (fn_pg_open_t)obs_module_symbol(h_pg, "$YawcWmumUTM");
            if (!fn_open)
                fn_open = (fn_pg_open_t)obs_module_symbol(h_pg, "$M1Gma1ocrGE");
            if (!fn_open)
                fn_open = (fn_pg_open_t)obs_module_symbol(h_pg, "scePlayGoOpen");
        }
    }
    int bound_open =
        (fn_open != NULL && obs_address_is_callable((const void *)fn_open));
    obs_report_measure(id, "scePlayGoOpen", "bound", (uint64_t)bound_open, "bool");
    if (bound_open) {
        obs_report_measure(id, "scePlayGoOpen", "how", (uint64_t)(is_link_open ? 1 : 2),
                           is_link_open ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoOpen", "status", 0, "not-possible");
    }

    fn_pg_speed_t fn_speed = NULL;
    int is_link_speed = 0;
    if (obs_address_is_callable((const void *)&scePlayGoSetInstallSpeed_static)) {
        fn_speed = (fn_pg_speed_t)&scePlayGoSetInstallSpeed_static;
        is_link_speed = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoSetInstallSpeed_be)) {
        fn_speed = (fn_pg_speed_t)&scePlayGoSetInstallSpeed_be;
        is_link_speed = 1;
    }
    if (!fn_speed) {
        fn_speed = (fn_pg_speed_t)obs_module_symbol(OBS_HANDLE_SELF, "$c91RT00cAOA");
        if (!fn_speed)
            fn_speed =
                (fn_pg_speed_t)obs_module_symbol(OBS_HANDLE_SELF, "$4AAcTU9R3XM");
        if (!fn_speed && h_pg > 0) {
            fn_speed = (fn_pg_speed_t)obs_module_symbol(h_pg, "$c91RT00cAOA");
            if (!fn_speed)
                fn_speed = (fn_pg_speed_t)obs_module_symbol(h_pg, "$4AAcTU9R3XM");
            if (!fn_speed)
                fn_speed =
                    (fn_pg_speed_t)obs_module_symbol(h_pg, "scePlayGoSetInstallSpeed");
        }
    }
    int bound_speed =
        (fn_speed != NULL && obs_address_is_callable((const void *)fn_speed));
    obs_report_measure(id, "scePlayGoSetInstallSpeed", "bound", (uint64_t)bound_speed,
                       "bool");
    if (bound_speed) {
        obs_report_measure(id, "scePlayGoSetInstallSpeed", "how",
                           (uint64_t)(is_link_speed ? 1 : 2),
                           is_link_speed ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoSetInstallSpeed", "status", 0, "not-possible");
    }

    fn_pg_chunk_t fn_chunk = NULL;
    int is_link_chunk = 0;
    if (obs_address_is_callable((const void *)&scePlayGoGetChunkId_static)) {
        fn_chunk = (fn_pg_chunk_t)&scePlayGoGetChunkId_static;
        is_link_chunk = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoGetChunkId_be)) {
        fn_chunk = (fn_pg_chunk_t)&scePlayGoGetChunkId_be;
        is_link_chunk = 1;
    }
    if (!fn_chunk) {
        fn_chunk = (fn_pg_chunk_t)obs_module_symbol(OBS_HANDLE_SELF, "$EPJUwdTFd+8");
        if (!fn_chunk)
            fn_chunk =
                (fn_pg_chunk_t)obs_module_symbol(OBS_HANDLE_SELF, "$73fF1MFU8hA");
        if (!fn_chunk && h_pg > 0) {
            fn_chunk = (fn_pg_chunk_t)obs_module_symbol(h_pg, "$EPJUwdTFd+8");
            if (!fn_chunk)
                fn_chunk = (fn_pg_chunk_t)obs_module_symbol(h_pg, "$73fF1MFU8hA");
            if (!fn_chunk)
                fn_chunk =
                    (fn_pg_chunk_t)obs_module_symbol(h_pg, "scePlayGoGetChunkId");
        }
    }
    int bound_chunk =
        (fn_chunk != NULL && obs_address_is_callable((const void *)fn_chunk));
    obs_report_measure(id, "scePlayGoGetChunkId", "bound", (uint64_t)bound_chunk,
                       "bool");
    if (bound_chunk) {
        obs_report_measure(id, "scePlayGoGetChunkId", "how",
                           (uint64_t)(is_link_chunk ? 1 : 2),
                           is_link_chunk ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoGetChunkId", "status", 0, "not-possible");
    }

    fn_pg_todo_t fn_todo = NULL;
    int is_link_todo = 0;
    if (obs_address_is_callable((const void *)&scePlayGoGetToDoList_static)) {
        fn_todo = (fn_pg_todo_t)&scePlayGoGetToDoList_static;
        is_link_todo = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoGetToDoList_be)) {
        fn_todo = (fn_pg_todo_t)&scePlayGoGetToDoList_be;
        is_link_todo = 1;
    }
    if (!fn_todo) {
        fn_todo = (fn_pg_todo_t)obs_module_symbol(OBS_HANDLE_SELF, "$rebACSvzfjY");
        if (!fn_todo)
            fn_todo = (fn_pg_todo_t)obs_module_symbol(OBS_HANDLE_SELF, "$Nn7zKwnA5q0");
        if (!fn_todo && h_pg > 0) {
            fn_todo = (fn_pg_todo_t)obs_module_symbol(h_pg, "$rebACSvzfjY");
            if (!fn_todo)
                fn_todo = (fn_pg_todo_t)obs_module_symbol(h_pg, "$Nn7zKwnA5q0");
            if (!fn_todo)
                fn_todo = (fn_pg_todo_t)obs_module_symbol(h_pg, "scePlayGoGetToDoList");
        }
    }
    int bound_todo =
        (fn_todo != NULL && obs_address_is_callable((const void *)fn_todo));
    obs_report_measure(id, "scePlayGoGetToDoList", "bound", (uint64_t)bound_todo,
                       "bool");
    if (bound_todo) {
        obs_report_measure(id, "scePlayGoGetToDoList", "how",
                           (uint64_t)(is_link_todo ? 1 : 2),
                           is_link_todo ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoGetToDoList", "status", 0, "not-possible");
    }

    fn_pg_close_t fn_close = NULL;
    int is_link_close = 0;
    if (obs_address_is_callable((const void *)&scePlayGoClose_static)) {
        fn_close = (fn_pg_close_t)&scePlayGoClose_static;
        is_link_close = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoClose_be)) {
        fn_close = (fn_pg_close_t)&scePlayGoClose_be;
        is_link_close = 1;
    }
    if (!fn_close) {
        fn_close = (fn_pg_close_t)obs_module_symbol(OBS_HANDLE_SELF, "$Lw5lRyM1ylE");
        if (!fn_close)
            fn_close =
                (fn_pg_close_t)obs_module_symbol(OBS_HANDLE_SELF, "$Uco1I0dlDi8");
        if (!fn_close && h_pg > 0) {
            fn_close = (fn_pg_close_t)obs_module_symbol(h_pg, "$Lw5lRyM1ylE");
            if (!fn_close)
                fn_close = (fn_pg_close_t)obs_module_symbol(h_pg, "$Uco1I0dlDi8");
            if (!fn_close)
                fn_close = (fn_pg_close_t)obs_module_symbol(h_pg, "scePlayGoClose");
        }
    }
    int bound_close =
        (fn_close != NULL && obs_address_is_callable((const void *)fn_close));
    obs_report_measure(id, "scePlayGoClose", "bound", (uint64_t)bound_close, "bool");
    if (bound_close) {
        obs_report_measure(id, "scePlayGoClose", "how",
                           (uint64_t)(is_link_close ? 1 : 2),
                           is_link_close ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoClose", "status", 0, "not-possible");
    }

    fn_pg_term_t fn_term = NULL;
    int is_link_term = 0;
    if (obs_address_is_callable((const void *)&scePlayGoTerminate_static)) {
        fn_term = (fn_pg_term_t)&scePlayGoTerminate_static;
        is_link_term = 1;
    } else if (obs_address_is_callable((const void *)&scePlayGoTerminate_be)) {
        fn_term = (fn_pg_term_t)&scePlayGoTerminate_be;
        is_link_term = 1;
    }
    if (!fn_term) {
        fn_term = (fn_pg_term_t)obs_module_symbol(OBS_HANDLE_SELF, "$8TNG4BG09zA");
        if (!fn_term)
            fn_term = (fn_pg_term_t)obs_module_symbol(OBS_HANDLE_SELF, "$MPe0EeBGM-E");
        if (!fn_term && h_pg > 0) {
            fn_term = (fn_pg_term_t)obs_module_symbol(h_pg, "$8TNG4BG09zA");
            if (!fn_term)
                fn_term = (fn_pg_term_t)obs_module_symbol(h_pg, "$MPe0EeBGM-E");
            if (!fn_term)
                fn_term = (fn_pg_term_t)obs_module_symbol(h_pg, "scePlayGoTerminate");
        }
    }
    int bound_term =
        (fn_term != NULL && obs_address_is_callable((const void *)fn_term));
    obs_report_measure(id, "scePlayGoTerminate", "bound", (uint64_t)bound_term, "bool");
    if (bound_term) {
        obs_report_measure(id, "scePlayGoTerminate", "how",
                           (uint64_t)(is_link_term ? 1 : 2),
                           is_link_term ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "scePlayGoTerminate", "status", 0, "not-possible");
    }

    if (!bound_init) {
        return obs_pass();
    }

    /* 1. scePlayGoInitialize */
    static _Alignas(16) uint8_t s_playgo_buf[0x200000];
    memset(s_playgo_buf, 0xcc, sizeof(s_playgo_buf));
    struct {
        uint64_t buf_ptr;
        uint32_t buf_size;
        uint32_t zero;
    } pg_param;
    pg_param.buf_ptr = (uint64_t)(uintptr_t)s_playgo_buf;
    pg_param.buf_size = 0x200000;
    pg_param.zero = 0;
    uint8_t before_param[16];
    memcpy(before_param, &pg_param, 16);

    int rc_init1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init1 = fn_init(&pg_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "PlayGoInitialize-1", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "PlayGoInitialize-1", "rc", (uint64_t)(uint32_t)rc_init1,
                       "code");
    obs_report_written(id, "PlayGoInitialize-1", "param-changed", before_param,
                       (const uint8_t *)&pg_param, 16);

    size_t changed_bytes = 0;
    for (size_t b = 0; b < sizeof(s_playgo_buf); b++) {
        if (s_playgo_buf[b] != 0xcc)
            changed_bytes++;
    }
    obs_report_measure(id, "PlayGoInitialize-1", "buf-changed-bytes",
                       (uint64_t)changed_bytes, "count");
    obs_report_bytes(id, "PlayGoInitialize-1", "buf-first-40h", 0, s_playgo_buf, 0x40);

    /* Second call */
    int rc_init2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init2 = fn_init(&pg_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "PlayGoInitialize-2", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "PlayGoInitialize-2", "rc", (uint64_t)(uint32_t)rc_init2,
                       "code");

    /* 2. scePlayGoOpen */
    int32_t handle = (int32_t)0xcccccccc;
    if (bound_open) {
        int rc_open = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_open = fn_open(&handle, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoOpen", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "PlayGoOpen", "rc", (uint64_t)(uint32_t)rc_open, "code");
        obs_report_bytes(id, "PlayGoOpen", "handle", 0, (const uint8_t *)&handle, 4);
    }

    /* 3. scePlayGoSetInstallSpeed */
    if (bound_speed) {
        int rc_speed = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_speed = fn_speed(handle, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoSetInstallSpeed", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PlayGoSetInstallSpeed", "rc",
                           (uint64_t)(uint32_t)rc_speed, "code");
    }

    /* 4. scePlayGoGetChunkId */
    static uint8_t ids[2000];
    memset(ids, 0xcc, sizeof(ids));
    uint32_t chunk_count = 0xcccccccc;
    int rc_chunk = -1;
    if (bound_chunk) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_chunk = fn_chunk(handle, ids, 1000, &chunk_count);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoGetChunkId", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PlayGoGetChunkId", "rc", (uint64_t)(uint32_t)rc_chunk,
                           "code");
        obs_report_bytes(id, "PlayGoGetChunkId", "count", 0,
                         (const uint8_t *)&chunk_count, 4);
        obs_report_bytes(id, "PlayGoGetChunkId", "ids-first-16b", 0, ids, 16);
    }

    /* 5. scePlayGoGetToDoList */
    if (bound_todo) {
        uint32_t todo_count =
            (rc_chunk == 0 && chunk_count != 0xcccccccc && chunk_count > 0)
                ? chunk_count
                : 1;
        static uint8_t list[0x100];
        memset(list, 0xcc, sizeof(list));
        uint32_t entries = 0xcccccccc;
        int rc_todo = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_todo = fn_todo(handle, list, todo_count, &entries);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoGetToDoList", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "PlayGoGetToDoList", "rc", (uint64_t)(uint32_t)rc_todo,
                           "code");
        obs_report_bytes(id, "PlayGoGetToDoList", "entries", 0,
                         (const uint8_t *)&entries, 4);
        obs_report_bytes(id, "PlayGoGetToDoList", "list-first-20h", 0, list, 0x20);
    }

    /* 6. scePlayGoClose & scePlayGoTerminate */
    if (bound_close) {
        int rc_close = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_close = fn_close(handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoClose", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "PlayGoClose", "rc", (uint64_t)(uint32_t)rc_close,
                           "code");
    }

    if (bound_term) {
        int rc_term = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_term = fn_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "PlayGoTerminate", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "PlayGoTerminate", "rc", (uint64_t)(uint32_t)rc_term,
                           "code");
    }

    return obs_pass();
}

/* REQ-20261012T1552Z-aw01: sceAmprCommandBufferWriteAddress_04_00 in PPSA04263's shape
 */
static obs_result check_aw01_ampr_write_address(void) {
    const char *id = "000-boot/ampr-write-address";
    obs_jmp_buf guard;

    int h_ampr = obs_module_open("libSceAmpr");
    if (h_ampr < 0) {
        h_ampr = obs_module_open("libSceAmpr.sprx");
    }

    const void *fn_ctor = (const void *)&sceAmprCommandBufferConstructor_static;
    if (!obs_address_is_callable(fn_ctor) && h_ampr > 0) {
        fn_ctor = obs_module_symbol(h_ampr, "sceAmprCommandBufferConstructor");
        if (!fn_ctor) {
            fn_ctor = obs_module_symbol(h_ampr, "$8aI7R7WaOlc");
        }
    }

    const void *fn_setbuf = (const void *)&sceAmprCommandBufferSetBuffer_static;
    if (!obs_address_is_callable(fn_setbuf)) {
        fn_setbuf = (const void *)&sceAmprCommandBufferSetBuffer;
    }
    if (!obs_address_is_callable(fn_setbuf) && h_ampr > 0) {
        fn_setbuf = obs_module_symbol(h_ampr, "sceAmprCommandBufferSetBuffer");
        if (!fn_setbuf) {
            fn_setbuf = obs_module_symbol(h_ampr, "$N-FSPA4S3nI");
        }
    }

    typedef int (*fn_reset_t)(void *);
    fn_reset_t fn_reset = NULL;
    if (obs_address_is_callable((const void *)&sceAmprCommandBufferReset_alt)) {
        fn_reset = (fn_reset_t)(void *)&sceAmprCommandBufferReset_alt;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprCommandBufferReset_static)) {
        fn_reset = (fn_reset_t)(void *)&sceAmprCommandBufferReset_static;
    }
    if (!fn_reset && h_ampr > 0) {
        fn_reset = (fn_reset_t)obs_module_symbol(h_ampr, "$voD27PUOpG0");
        if (!fn_reset)
            fn_reset = (fn_reset_t)obs_module_symbol(h_ampr, "$baQO9ez2gL4");
        if (!fn_reset)
            fn_reset =
                (fn_reset_t)obs_module_symbol(h_ampr, "sceAmprCommandBufferReset");
    }

    /* Symbol 1: sceAmprCommandBufferWriteAddress_04_00 */
    typedef int (*fn_write_addr_t)(void *, void *, uint32_t, uint32_t);
    fn_write_addr_t fn_write_addr = NULL;
    int is_link_wa = 0;
    if (obs_address_is_callable(
            (const void *)&sceAmprCommandBufferWriteAddress_04_00_static)) {
        fn_write_addr = (fn_write_addr_t)&sceAmprCommandBufferWriteAddress_04_00_static;
        is_link_wa = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprCommandBufferWriteAddress_04_00_url)) {
        fn_write_addr = (fn_write_addr_t)&sceAmprCommandBufferWriteAddress_04_00_url;
        is_link_wa = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprCommandBufferWriteAddress_04_00_le)) {
        fn_write_addr = (fn_write_addr_t)&sceAmprCommandBufferWriteAddress_04_00_le;
        is_link_wa = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprCommandBufferWriteAddress_04_00_named)) {
        fn_write_addr = (fn_write_addr_t)&sceAmprCommandBufferWriteAddress_04_00_named;
        is_link_wa = 1;
    }
    if (!fn_write_addr) {
        fn_write_addr =
            (fn_write_addr_t)obs_module_symbol(OBS_HANDLE_SELF, "$j0+3uJMxYJY");
        if (!fn_write_addr)
            fn_write_addr =
                (fn_write_addr_t)obs_module_symbol(OBS_HANDLE_SELF, "$j0-3uJMxYJY");
        if (!fn_write_addr)
            fn_write_addr =
                (fn_write_addr_t)obs_module_symbol(OBS_HANDLE_SELF, "$lmAxk7i3T48");
        if (!fn_write_addr)
            fn_write_addr = (fn_write_addr_t)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAmprCommandBufferWriteAddress_04_00");
        if (!fn_write_addr && h_ampr > 0) {
            fn_write_addr = (fn_write_addr_t)obs_module_symbol(h_ampr, "$j0+3uJMxYJY");
            if (!fn_write_addr)
                fn_write_addr =
                    (fn_write_addr_t)obs_module_symbol(h_ampr, "$j0-3uJMxYJY");
            if (!fn_write_addr)
                fn_write_addr =
                    (fn_write_addr_t)obs_module_symbol(h_ampr, "$lmAxk7i3T48");
            if (!fn_write_addr)
                fn_write_addr = (fn_write_addr_t)obs_module_symbol(
                    h_ampr, "sceAmprCommandBufferWriteAddress_04_00");
        }
    }
    int bound_wa =
        (fn_write_addr != NULL && obs_address_is_callable((const void *)fn_write_addr));
    obs_report_measure(id, "sceAmprCommandBufferWriteAddress_04_00", "bound",
                       (uint64_t)bound_wa, "bool");
    if (bound_wa) {
        obs_report_measure(id, "sceAmprCommandBufferWriteAddress_04_00", "how",
                           (uint64_t)(is_link_wa ? 1 : 2),
                           is_link_wa ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAmprCommandBufferWriteAddress_04_00", "status", 0,
                           "not-possible");
    }

    /* Symbol 2: sceAmprMeasureCommandSizeWriteAddress_04_00 */
    void *fn_meas = NULL;
    int is_link_meas = 0;
    if (obs_address_is_callable(
            (const void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_static)) {
        fn_meas = (void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_static;
        is_link_meas = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_le)) {
        fn_meas = (void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_le;
        is_link_meas = 1;
    } else if (obs_address_is_callable(
                   (const void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_named)) {
        fn_meas = (void *)&sceAmprMeasureCommandSizeWriteAddress_04_00_named;
        is_link_meas = 1;
    }
    if (!fn_meas) {
        fn_meas = (void *)obs_module_symbol(OBS_HANDLE_SELF, "$4fgtGfXDrFc");
        if (!fn_meas)
            fn_meas = (void *)obs_module_symbol(OBS_HANDLE_SELF, "$V6zD9Rkt+OE");
        if (!fn_meas)
            fn_meas = (void *)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAmprMeasureCommandSizeWriteAddress_04_00");
        if (!fn_meas && h_ampr > 0) {
            fn_meas = (void *)obs_module_symbol(h_ampr, "$4fgtGfXDrFc");
            if (!fn_meas)
                fn_meas = (void *)obs_module_symbol(h_ampr, "$V6zD9Rkt+OE");
            if (!fn_meas)
                fn_meas = (void *)obs_module_symbol(
                    h_ampr, "sceAmprMeasureCommandSizeWriteAddress_04_00");
        }
    }
    int bound_meas = (fn_meas != NULL && obs_address_is_callable(fn_meas));
    obs_report_measure(id, "sceAmprMeasureCommandSizeWriteAddress_04_00", "bound",
                       (uint64_t)bound_meas, "bool");
    if (bound_meas) {
        obs_report_measure(id, "sceAmprMeasureCommandSizeWriteAddress_04_00", "how",
                           (uint64_t)(is_link_meas ? 1 : 2),
                           is_link_meas ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAmprMeasureCommandSizeWriteAddress_04_00", "status",
                           0, "not-possible");
    }

    if (bound_wa) {
        static _Alignas(64) uint8_t obj[0x200];
        static _Alignas(64) uint8_t cbuf[0x200];
        static _Alignas(64) uint8_t buf[0x1000];
        static _Alignas(8) uint8_t slot[8];

        memset(obj, 0xcc, sizeof(obj));
        memset(cbuf, 0xcc, sizeof(cbuf));
        memset(buf, 0xcc, sizeof(buf));
        memset(slot, 0xcc, sizeof(slot));

        if (fn_ctor) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ((void (*)(void *, void *, void *, uint64_t, void *, uint64_t))fn_ctor)(
                    obj, obj, cbuf, 0, cbuf, 0x47d);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "ctor", "fault", (uint64_t)sig, "signal");
            }
        }
        if (fn_setbuf) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                int rc_sb = ((int (*)(void *, void *, uint32_t, uint64_t, uint64_t,
                                      uint64_t))fn_setbuf)(obj, buf, 0x1000, 0, 0, 0);
                obs_fault_unregister();
                obs_report_measure(id, "set-buffer", "rc", (uint64_t)(uint32_t)rc_sb,
                                   "code");
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "set-buffer", "fault", (uint64_t)sig, "signal");
            }
        }

        uint8_t obj_pre_1[0x40];
        memcpy(obj_pre_1, obj, 0x40);
        uint8_t buf_pre_1[0x1000];
        memcpy(buf_pre_1, buf, 0x1000);

        uint32_t val = 0x12345678;

        /* Call 1 */
        int sig1 = OBS_FAULT_ARM(&guard);
        uint64_t rc1 = 0xffffffffffffffffULL;
        if (sig1 == 0) {
            rc1 = (uint64_t)(uint32_t)fn_write_addr(obj, slot, val, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-1", "fault", (uint64_t)sig1,
                               "signal");
        }
        obs_report_measure(id, "write-address-1", "rc", rc1, "code");
        obs_report_written(id, "sceAmprCommandBufferWriteAddress_04_00", "obj-1",
                           obj_pre_1, obj, 0x40);
        obs_report_bytes(id, "write-address-1", "obj-before-40b", 0, obj_pre_1, 0x40);
        obs_report_bytes(id, "write-address-1", "obj-after-40b", 0, obj, 0x40);

        size_t chg_buf_1 = 0;
        for (size_t i = 0; i < 0x1000; i++) {
            if (buf[i] != buf_pre_1[i])
                chg_buf_1++;
        }
        obs_report_measure(id, "write-address-1", "buf-changed-bytes", chg_buf_1,
                           "bytes");
        obs_report_bytes(id, "write-address-1", "buf-after-40b", 0, buf, 0x40);
        obs_report_measure(id, "write-address-1", "slot-addr",
                           (uint64_t)(uintptr_t)slot, "addr");
        obs_report_bytes(id, "write-address-1", "slot-8b", 0, slot, 8);

        /* Call 2 */
        uint8_t buf_pre_2[0x1000];
        memcpy(buf_pre_2, buf, 0x1000);

        int sig2 = OBS_FAULT_ARM(&guard);
        uint64_t rc2 = 0xffffffffffffffffULL;
        if (sig2 == 0) {
            rc2 = (uint64_t)(uint32_t)fn_write_addr(obj, slot, val, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-2", "fault", (uint64_t)sig2,
                               "signal");
        }
        obs_report_measure(id, "write-address-2", "rc", rc2, "code");
        size_t chg_buf_2 = 0;
        for (size_t i = 0; i < 0x1000; i++) {
            if (buf[i] != buf_pre_2[i])
                chg_buf_2++;
        }
        obs_report_measure(id, "write-address-2", "buf-changed-bytes", chg_buf_2,
                           "bytes");
        obs_report_bytes(id, "write-address-2", "buf-after-40b", 0, buf, 0x40);
        obs_report_bytes(id, "write-address-2", "buf-after-80b", 0x40, buf + 0x40,
                         0x40);

        /* REQ-20261014T1710Z-ar01: Reset after two commands and append after reset */
        obs_report_bytes(id, "ar01-reset-after-2", "obj-before-20b", 0, obj, 0x20);
        uint64_t rc_reset = 0xffffffffffffffffULL;
        if (fn_reset) {
            int sig_rst = OBS_FAULT_ARM(&guard);
            if (sig_rst == 0) {
                rc_reset = (uint64_t)(uint32_t)fn_reset(obj);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "ar01-reset-after-2", "fault", (uint64_t)sig_rst,
                                   "signal");
            }
        }
        obs_report_measure(id, "ar01-reset-after-2", "rc", rc_reset, "code");
        obs_report_bytes(id, "ar01-reset-after-2", "obj-after-20b", 0, obj, 0x20);

        /* Arm 2: The append after it */
        int sig_post = OBS_FAULT_ARM(&guard);
        uint64_t rc_post = 0xffffffffffffffffULL;
        if (sig_post == 0) {
            rc_post = (uint64_t)(uint32_t)fn_write_addr(obj, slot, 0x87654321, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ar01-append-after-reset", "fault",
                               (uint64_t)sig_post, "signal");
        }
        obs_report_measure(id, "ar01-append-after-reset", "rc", rc_post, "code");
        uint32_t bytes_used_post = *(const uint32_t *)(obj + 4);
        uint32_t cmd_cnt_post = *(const uint32_t *)(obj + 8);
        obs_report_measure(id, "ar01-append-after-reset", "bytes-used-plus4",
                           (uint64_t)bytes_used_post, "bytes");
        obs_report_measure(id, "ar01-append-after-reset", "cmd-count-plus8",
                           (uint64_t)cmd_cnt_post, "count");
        obs_report_bytes(id, "ar01-append-after-reset", "buf-after-30b", 0, buf, 0x30);

        /* Arm 3: An append that does not fit (0x20-byte buffer) */
        static _Alignas(64) uint8_t obj_small[0x200];
        static _Alignas(64) uint8_t cbuf_small[0x200];
        static _Alignas(64) uint8_t buf_small[0x20];
        memset(obj_small, 0xcc, sizeof(obj_small));
        memset(cbuf_small, 0xcc, sizeof(cbuf_small));
        memset(buf_small, 0xcc, sizeof(buf_small));
        if (fn_ctor) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ((void (*)(void *, void *, void *, uint64_t, void *, uint64_t))fn_ctor)(
                    obj_small, obj_small, cbuf_small, 0, cbuf_small, 0x47d);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        if (fn_setbuf) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                int rc_sb =
                    ((int (*)(void *, void *, uint32_t, uint64_t, uint64_t,
                              uint64_t))fn_setbuf)(obj_small, buf_small, 0x20, 0, 0, 0);
                obs_fault_unregister();
                obs_report_measure(id, "ar01-overflow-setbuf", "rc",
                                   (uint64_t)(uint32_t)rc_sb, "code");
            } else {
                obs_fault_unregister();
            }
        }
        for (int i = 0; i < 3; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "ar01-overflow-append-%d", i + 1);
            int sig_ovf = OBS_FAULT_ARM(&guard);
            uint64_t rc_ovf = 0xffffffffffffffffULL;
            if (sig_ovf == 0) {
                rc_ovf =
                    (uint64_t)(uint32_t)fn_write_addr(obj_small, slot, 0x12345678, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig_ovf, "signal");
            }
            obs_report_measure(id, tag, "rc", rc_ovf, "code");
            uint32_t bu = *(const uint32_t *)(obj_small + 4);
            uint32_t cc = *(const uint32_t *)(obj_small + 8);
            obs_report_measure(id, tag, "bytes-used-plus4", (uint64_t)bu, "bytes");
            obs_report_measure(id, tag, "cmd-count-plus8", (uint64_t)cc, "count");
            obs_report_bytes(id, tag, "obj-20b", 0, obj_small, 0x20);
            obs_report_bytes(id, tag, "buf-20b", 0, buf_small, 0x20);
            if (rc_ovf != 0) {
                break;
            }
        }

        /* REQ-20261012T1900Z-aw02: fresh buffer with high stack address, specific
         * values, and misaligned address */
        static _Alignas(64) uint8_t obj2[0x200];
        static _Alignas(64) uint8_t cbuf2[0x200];
        static _Alignas(64) uint8_t fresh_buf[0x1000];
        memset(obj2, 0xcc, sizeof(obj2));
        memset(cbuf2, 0xcc, sizeof(cbuf2));
        memset(fresh_buf, 0xcc, sizeof(fresh_buf));

        if (fn_ctor) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ((void (*)(void *, void *, void *, uint64_t, void *, uint64_t))fn_ctor)(
                    obj2, obj2, cbuf2, 0, cbuf2, 0x47d);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        if (fn_setbuf) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ((int (*)(void *, void *, uint32_t, uint64_t, uint64_t,
                          uint64_t))fn_setbuf)(obj2, fresh_buf, 0x1000, 0, 0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }

        /* 1. slot_hi on stack (above 4 GiB, bits above 31 set) */
        uint8_t stack_slot_raw[32];
        memset(stack_slot_raw, 0xcc, sizeof(stack_slot_raw));
        void *slot_hi = (void *)(((uintptr_t)stack_slot_raw + 7) & ~7ULL);

        obs_report_measure(id, "write-address-slot-hi", "slot-addr",
                           (uint64_t)(uintptr_t)slot_hi, "addr");
        int sig_hi = OBS_FAULT_ARM(&guard);
        uint64_t rc_hi = 0xffffffffffffffffULL;
        if (sig_hi == 0) {
            rc_hi = (uint64_t)(uint32_t)fn_write_addr(obj2, slot_hi, 0x87654321, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-slot-hi", "fault", (uint64_t)sig_hi,
                               "signal");
        }
        obs_report_measure(id, "write-address-slot-hi", "rc", rc_hi, "code");
        obs_report_bytes(id, "write-address-slot-hi", "packet-12b", 0, fresh_buf, 12);

        /* 2a. Value 0x00000003 */
        obs_report_measure(id, "write-address-val-3", "slot-addr",
                           (uint64_t)(uintptr_t)slot_hi, "addr");
        int sig_v3 = OBS_FAULT_ARM(&guard);
        uint64_t rc_v3 = 0xffffffffffffffffULL;
        if (sig_v3 == 0) {
            rc_v3 = (uint64_t)(uint32_t)fn_write_addr(obj2, slot_hi, 0x00000003, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-val-3", "fault", (uint64_t)sig_v3,
                               "signal");
        }
        obs_report_measure(id, "write-address-val-3", "rc", rc_v3, "code");
        obs_report_bytes(id, "write-address-val-3", "packet-12b", 0, fresh_buf + 12,
                         12);

        /* 2b. Value 0xfffffffd */
        obs_report_measure(id, "write-address-val-fffffffd", "slot-addr",
                           (uint64_t)(uintptr_t)slot_hi, "addr");
        int sig_vfd = OBS_FAULT_ARM(&guard);
        uint64_t rc_vfd = 0xffffffffffffffffULL;
        if (sig_vfd == 0) {
            rc_vfd = (uint64_t)(uint32_t)fn_write_addr(obj2, slot_hi, 0xfffffffd, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-val-fffffffd", "fault",
                               (uint64_t)sig_vfd, "signal");
        }
        obs_report_measure(id, "write-address-val-fffffffd", "rc", rc_vfd, "code");
        obs_report_bytes(id, "write-address-val-fffffffd", "packet-12b", 0,
                         fresh_buf + 24, 12);

        /* 3. Slot address 4-aligned but not 8-aligned */
        uint8_t stack_misalign_raw[48];
        memset(stack_misalign_raw, 0xcc, sizeof(stack_misalign_raw));
        uintptr_t base_aligned = ((uintptr_t)stack_misalign_raw + 15) & ~7ULL;
        void *slot_misalign = (void *)(base_aligned + 4);

        obs_report_measure(id, "write-address-misaligned-4", "slot-addr",
                           (uint64_t)(uintptr_t)slot_misalign, "addr");
        int sig_mis = OBS_FAULT_ARM(&guard);
        uint64_t rc_mis = 0xffffffffffffffffULL;
        if (sig_mis == 0) {
            rc_mis =
                (uint64_t)(uint32_t)fn_write_addr(obj2, slot_misalign, 0x87654321, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "write-address-misaligned-4", "fault",
                               (uint64_t)sig_mis, "signal");
        }
        obs_report_measure(id, "write-address-misaligned-4", "rc", rc_mis, "code");
        obs_report_bytes(id, "write-address-misaligned-4", "packet-12b", 0,
                         fresh_buf + 36, 12);
    }

    if (bound_meas) {
        static _Alignas(8) uint8_t slot_m[8];
        memset(slot_m, 0xcc, sizeof(slot_m));
        uint32_t val = 0x12345678;

        /* 4a. Measure size with no arguments */
        int sig_m1 = OBS_FAULT_ARM(&guard);
        uint64_t rc_m1 = 0xffffffffffffffffULL;
        if (sig_m1 == 0) {
            rc_m1 = (uint64_t)(uint32_t)((int (*)(void))fn_meas)();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "measure-size-noargs", "fault", (uint64_t)sig_m1,
                               "signal");
        }
        obs_report_measure(id, "measure-size-noargs", "rc", rc_m1, "code");

        /* 4b. Measure size with (address, value, 0) */
        int sig_m2 = OBS_FAULT_ARM(&guard);
        uint64_t rc_m2 = 0xffffffffffffffffULL;
        if (sig_m2 == 0) {
            rc_m2 = (uint64_t)(uint32_t)((int (*)(void *, uint32_t, uint32_t))fn_meas)(
                slot_m, val, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "measure-size-args", "fault", (uint64_t)sig_m2,
                               "signal");
        }
        obs_report_measure(id, "measure-size-args", "rc", rc_m2, "code");
    }

    return obs_pass();
}

/* REQ-20261012T2140Z-ap02: does a blocking sceAudioOut2ContextPush pace a pump that
 * publishes PCM? */
static obs_result check_ap02_audioout2_pcm_pump(void) {
    const char *id = "000-boot/audioout2-pcm-pump";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        obs_jmp_buf g;
        if (OBS_FAULT_ARM(&g) == 0) {
            sceSysmoduleLoadModule(0x0096);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    int h_ao = obs_module_open("libSceAudioOut2");

    typedef int (*fn_ao_init_t)(void);
    typedef int (*fn_ao_reset_param_t)(void *);
    typedef int (*fn_ao_query_mem_t)(void *, uint64_t *);
    typedef int (*fn_ao_ctx_create_t)(void *, void *, uint64_t, void **);
    typedef int (*fn_ao_user_create_t)(int32_t, void **);
    typedef int (*fn_ao_port_create_t)(void *, const void *, void **);
    typedef int (*fn_ao_port_set_attr_t)(void *, const void *, uint32_t);
    typedef int (*fn_ao_port_get_state_t)(void *, void *);
    typedef int (*fn_ao_get_queue_t)(void *, uint32_t *, uint32_t *);
    typedef int (*fn_ao_push_t)(void *, uint32_t);
    typedef int (*fn_ao_advance_t)(void *);
    typedef int (*fn_ao_port_destroy_t)(void *);
    typedef int (*fn_ao_ctx_destroy_t)(void *);
    typedef int (*fn_ao_user_destroy_t)(void *);

    fn_ao_init_t fn_init =
        obs_address_is_callable((const void *)&sceAudioOut2Initialize_static)
            ? (fn_ao_init_t)&sceAudioOut2Initialize_static
            : NULL;
    if (!fn_init) {
        fn_init =
            (fn_ao_init_t)obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2Initialize");
        if (!fn_init && h_ao > 0)
            fn_init = (fn_ao_init_t)obs_module_symbol(h_ao, "sceAudioOut2Initialize");
        if (!fn_init && h_ao > 0)
            fn_init = (fn_ao_init_t)obs_module_symbol(h_ao, "$g2tViFIohHE");
    }

    fn_ao_reset_param_t fn_reset_param =
        obs_address_is_callable((const void *)&sceAudioOut2ContextResetParam_static)
            ? (fn_ao_reset_param_t)&sceAudioOut2ContextResetParam_static
            : NULL;
    if (!fn_reset_param) {
        fn_reset_param = (fn_ao_reset_param_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2ContextResetParam");
        if (!fn_reset_param && h_ao > 0)
            fn_reset_param = (fn_ao_reset_param_t)obs_module_symbol(
                h_ao, "sceAudioOut2ContextResetParam");
        if (!fn_reset_param && h_ao > 0)
            fn_reset_param =
                (fn_ao_reset_param_t)obs_module_symbol(h_ao, "$t5YrizufpQc");
    }

    fn_ao_query_mem_t fn_query_mem =
        obs_address_is_callable((const void *)&sceAudioOut2ContextQueryMemory_static)
            ? (fn_ao_query_mem_t)&sceAudioOut2ContextQueryMemory_static
            : NULL;
    if (!fn_query_mem) {
        fn_query_mem = (fn_ao_query_mem_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2ContextQueryMemory");
        if (!fn_query_mem && h_ao > 0)
            fn_query_mem = (fn_ao_query_mem_t)obs_module_symbol(
                h_ao, "sceAudioOut2ContextQueryMemory");
        if (!fn_query_mem && h_ao > 0)
            fn_query_mem = (fn_ao_query_mem_t)obs_module_symbol(h_ao, "$pDmme7Bgm6E");
    }

    fn_ao_ctx_create_t fn_ctx_create =
        obs_address_is_callable((const void *)&sceAudioOut2ContextCreate_static)
            ? (fn_ao_ctx_create_t)&sceAudioOut2ContextCreate_static
            : NULL;
    if (!fn_ctx_create) {
        fn_ctx_create = (fn_ao_ctx_create_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2ContextCreate");
        if (!fn_ctx_create && h_ao > 0)
            fn_ctx_create = (fn_ao_ctx_create_t)obs_module_symbol(
                h_ao, "sceAudioOut2ContextCreate");
        if (!fn_ctx_create && h_ao > 0)
            fn_ctx_create = (fn_ao_ctx_create_t)obs_module_symbol(h_ao, "$0x6o1VVAYSY");
    }

    fn_ao_user_create_t fn_user_create =
        obs_address_is_callable((const void *)&sceAudioOut2UserCreate_static)
            ? (fn_ao_user_create_t)&sceAudioOut2UserCreate_static
            : NULL;
    if (!fn_user_create) {
        fn_user_create = (fn_ao_user_create_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2UserCreate");
        if (!fn_user_create && h_ao > 0)
            fn_user_create =
                (fn_ao_user_create_t)obs_module_symbol(h_ao, "sceAudioOut2UserCreate");
        if (!fn_user_create && h_ao > 0)
            fn_user_create =
                (fn_ao_user_create_t)obs_module_symbol(h_ao, "$xywYcRB7nbQ");
    }

    fn_ao_port_create_t fn_port_create = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_static)) {
        fn_port_create = (fn_ao_port_create_t)&sceAudioOut2PortCreate_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_alt)) {
        fn_port_create = (fn_ao_port_create_t)&sceAudioOut2PortCreate_alt;
    } else {
        fn_port_create = (fn_ao_port_create_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2PortCreate");
        if (!fn_port_create && h_ao > 0)
            fn_port_create =
                (fn_ao_port_create_t)obs_module_symbol(h_ao, "sceAudioOut2PortCreate");
        if (!fn_port_create && h_ao > 0)
            fn_port_create =
                (fn_ao_port_create_t)obs_module_symbol(h_ao, "$A89PZmqwrSQ");
    }

    fn_ao_port_set_attr_t fn_port_set_attr = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortSetAttributes_static)) {
        fn_port_set_attr = (fn_ao_port_set_attr_t)&sceAudioOut2PortSetAttributes_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceAudioOut2PortSetAttributes_alt)) {
        fn_port_set_attr = (fn_ao_port_set_attr_t)&sceAudioOut2PortSetAttributes_alt;
    } else {
        fn_port_set_attr = (fn_ao_port_set_attr_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2PortSetAttributes");
        if (!fn_port_set_attr && h_ao > 0)
            fn_port_set_attr = (fn_ao_port_set_attr_t)obs_module_symbol(
                h_ao, "sceAudioOut2PortSetAttributes");
        if (!fn_port_set_attr && h_ao > 0)
            fn_port_set_attr =
                (fn_ao_port_set_attr_t)obs_module_symbol(h_ao, "$8XTArSPyWHk");
    }

    fn_ao_port_get_state_t fn_port_get_state = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortGetState_static)) {
        fn_port_get_state = (fn_ao_port_get_state_t)&sceAudioOut2PortGetState_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortGetState_alt)) {
        fn_port_get_state = (fn_ao_port_get_state_t)&sceAudioOut2PortGetState_alt;
    } else {
        fn_port_get_state = (fn_ao_port_get_state_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2PortGetState");
        if (!fn_port_get_state && h_ao > 0)
            fn_port_get_state = (fn_ao_port_get_state_t)obs_module_symbol(
                h_ao, "sceAudioOut2PortGetState");
        if (!fn_port_get_state && h_ao > 0)
            fn_port_get_state =
                (fn_ao_port_get_state_t)obs_module_symbol(h_ao, "$gatEUKG+Ea4");
    }

    fn_ao_get_queue_t fn_get_queue = NULL;
    if (obs_address_is_callable(
            (const void *)&sceAudioOut2ContextGetQueueLevel_static)) {
        fn_get_queue =
            (fn_ao_get_queue_t)(void
                                    *)(uintptr_t)&sceAudioOut2ContextGetQueueLevel_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceAudioOut2ContextGetQueueLevel_alt)) {
        fn_get_queue =
            (fn_ao_get_queue_t)(void *)(uintptr_t)&sceAudioOut2ContextGetQueueLevel_alt;
    } else {
        fn_get_queue = (fn_ao_get_queue_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2ContextGetQueueLevel");
        if (!fn_get_queue && h_ao > 0)
            fn_get_queue = (fn_ao_get_queue_t)obs_module_symbol(h_ao, "$R7d0F1g2qsU");
        if (!fn_get_queue && h_ao > 0)
            fn_get_queue = (fn_ao_get_queue_t)obs_module_symbol(h_ao, "$xao2WBd0t0c");
    }

    fn_ao_push_t fn_push = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2ContextPush_static)) {
        fn_push = (fn_ao_push_t)(void *)(uintptr_t)&sceAudioOut2ContextPush_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2ContextPush_alt)) {
        fn_push = (fn_ao_push_t)(void *)(uintptr_t)&sceAudioOut2ContextPush_alt;
    } else {
        fn_push =
            (fn_ao_push_t)obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextPush");
        if (!fn_push && h_ao > 0)
            fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "sceAudioOut2ContextPush");
        if (!fn_push && h_ao > 0)
            fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "$aII9h5nli9U");
        if (!fn_push && h_ao > 0)
            fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "$1YvlmYc9gmg");
    }

    fn_ao_advance_t fn_advance = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_static)) {
        fn_advance = (fn_ao_advance_t)&sceAudioOut2ContextAdvance_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_alt)) {
        fn_advance = (fn_ao_advance_t)&sceAudioOut2ContextAdvance_alt;
    } else {
        fn_advance = (fn_ao_advance_t)obs_module_symbol(OBS_HANDLE_SELF,
                                                        "sceAudioOut2ContextAdvance");
        if (!fn_advance && h_ao > 0)
            fn_advance = (fn_ao_advance_t)obs_module_symbol(h_ao, "$PE2zHMqLSHs");
        if (!fn_advance && h_ao > 0)
            fn_advance = (fn_ao_advance_t)obs_module_symbol(h_ao, "$e0iLyhyzTTw");
    }

    fn_ao_port_destroy_t fn_port_destroy = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_static)) {
        fn_port_destroy = (fn_ao_port_destroy_t)&sceAudioOut2PortDestroy_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_alt)) {
        fn_port_destroy = (fn_ao_port_destroy_t)&sceAudioOut2PortDestroy_alt;
    } else if (h_ao > 0) {
        fn_port_destroy =
            (fn_ao_port_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2PortDestroy");
        if (!fn_port_destroy)
            fn_port_destroy =
                (fn_ao_port_destroy_t)obs_module_symbol(h_ao, "$cd+Rtw+D1x8");
    }

    fn_ao_ctx_destroy_t fn_ctx_destroy = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2ContextDestroy_static)) {
        fn_ctx_destroy = (fn_ao_ctx_destroy_t)&sceAudioOut2ContextDestroy_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2ContextDestroy_alt)) {
        fn_ctx_destroy = (fn_ao_ctx_destroy_t)&sceAudioOut2ContextDestroy_alt;
    } else if (h_ao > 0) {
        fn_ctx_destroy =
            (fn_ao_ctx_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2ContextDestroy");
        if (!fn_ctx_destroy)
            fn_ctx_destroy =
                (fn_ao_ctx_destroy_t)obs_module_symbol(h_ao, "$on6ZH7Abo10");
    }

    fn_ao_user_destroy_t fn_user_destroy = NULL;
    if (obs_address_is_callable((const void *)&sceAudioOut2UserDestroy_static)) {
        fn_user_destroy = (fn_ao_user_destroy_t)&sceAudioOut2UserDestroy_static;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2UserDestroy_alt)) {
        fn_user_destroy = (fn_ao_user_destroy_t)&sceAudioOut2UserDestroy_alt;
    } else if (h_ao > 0) {
        fn_user_destroy =
            (fn_ao_user_destroy_t)obs_module_symbol(h_ao, "sceAudioOut2UserDestroy");
        if (!fn_user_destroy)
            fn_user_destroy =
                (fn_ao_user_destroy_t)obs_module_symbol(h_ao, "$6vY70ydXpiE");
    }

    int sig = 0;
    int rc_init = -1;
    if (fn_init && obs_address_is_callable((const void *)fn_init)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceAudioOut2Initialize", "rc", (uint64_t)(uint32_t)rc_init,
                       "code");

    uint8_t ctx_param[64];
    memset(ctx_param, 0, sizeof(ctx_param));
    if (fn_reset_param && obs_address_is_callable((const void *)fn_reset_param)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            fn_reset_param(ctx_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    /* PPSA04263 parameters: {4, 0x48, 0, 2, 0x100, 1} */
    *(uint32_t *)(void *)(ctx_param + 0x00) = 4u;
    *(uint32_t *)(void *)(ctx_param + 0x04) = 0x48u;
    *(uint32_t *)(void *)(ctx_param + 0x08) = 0u;
    *(uint32_t *)(void *)(ctx_param + 0x0c) = 2u;
    *(uint32_t *)(void *)(ctx_param + 0x10) = 0x100u;
    *(uint32_t *)(void *)(ctx_param + 0x14) = 1u;

    uint64_t mem_size = 0;
    int rc_qm = -1;
    if (fn_query_mem && obs_address_is_callable((const void *)fn_query_mem)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_qm = fn_query_mem(ctx_param, &mem_size);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "ContextQueryMemory", "rc", (uint64_t)(uint32_t)rc_qm,
                       "code");
    obs_report_measure(id, "ContextQueryMemory", "size", mem_size, "bytes");

    static _Alignas(16) uint8_t s_pcm_ctx_mem[0x200000];
    uint64_t alloc_size =
        (mem_size > 0 && mem_size <= sizeof(s_pcm_ctx_mem)) ? mem_size : 0x10000u;
    void *context = (void *)0xffffffffffffffffULL;
    int rc_ctx = -1;
    if (fn_ctx_create && obs_address_is_callable((const void *)fn_ctx_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ctx = fn_ctx_create(ctx_param, s_pcm_ctx_mem, alloc_size, &context);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "ContextCreate", "rc", (uint64_t)(uint32_t)rc_ctx, "code");
    obs_report_measure(id, "ContextCreate", "context", (uint64_t)(uintptr_t)context,
                       "addr");

    void *user = (void *)0xffffffffffffffffULL;
    int rc_user = -1;
    if (fn_user_create && obs_address_is_callable((const void *)fn_user_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user = fn_user_create(0xff, &user);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "UserCreate", "rc", (uint64_t)(uint32_t)rc_user, "code");
    obs_report_measure(id, "UserCreate", "user", (uint64_t)(uintptr_t)user, "addr");

    /* 2-channel port 0 */
    uint8_t pparam[64];
    memset(pparam, 0, sizeof(pparam));
    *(uint32_t *)(void *)(pparam + 0x00) = 0;
    *(uint32_t *)(void *)(pparam + 0x04) = (uint32_t)(2 << 8);
    *(uint32_t *)(void *)(pparam + 0x08) = 48000;
    *(uint32_t *)(void *)(pparam + 0x0c) = 0;
    *(uint64_t *)(void *)(pparam + 0x10) = (uint64_t)(uintptr_t)user;

    void *port = (void *)0xffffffffffffffffULL;
    int rc_port = -1;
    if (fn_port_create && obs_address_is_callable((const void *)fn_port_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_port = fn_port_create(context, pparam, &port);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "PortCreate", "rc", (uint64_t)(uint32_t)rc_port, "code");
    obs_report_measure(id, "PortCreate", "port", (uint64_t)(uintptr_t)port, "addr");

    /* 256 stereo float frames of non-zero PCM */
    static float s_pcm_buf[256 * 2];
    for (size_t i = 0; i < 256 * 2; i++) {
        s_pcm_buf[i] = 0.25f;
    }

    struct obs_ao2_attr {
        uint32_t id;
        uint32_t pad;
        uint64_t value_ptr;
        uint64_t size;
    };

    static uint64_t s_pcm_ptr = 0;
    s_pcm_ptr = (uint64_t)(uintptr_t)s_pcm_buf;

    struct obs_ao2_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.id = 0;
    attr.pad = 0;
    attr.value_ptr = (uint64_t)(uintptr_t)&s_pcm_ptr;
    attr.size = 8;

    int rc_attr = -1;
    if (fn_port_set_attr && obs_address_is_callable((const void *)fn_port_set_attr)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_attr = fn_port_set_attr(port, &attr, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "PortSetAttributes-init", "rc", (uint64_t)(uint32_t)rc_attr,
                       "code");

    if (rc_attr != 0 && fn_port_set_attr &&
        obs_address_is_callable((const void *)fn_port_set_attr)) {
        struct obs_ao2_attr attr_dir;
        memset(&attr_dir, 0, sizeof(attr_dir));
        attr_dir.id = 0;
        attr_dir.pad = 0;
        attr_dir.value_ptr = (uint64_t)(uintptr_t)s_pcm_buf;
        attr_dir.size = 8;
        int rc_dir = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_dir = fn_port_set_attr(port, &attr_dir, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "PortSetAttributes-init-direct", "rc",
                           (uint64_t)(uint32_t)rc_dir, "code");
        if (rc_dir == 0) {
            attr = attr_dir;
            rc_attr = rc_dir;
        }
    }

    uint64_t tsc_freq = sceKernelGetTscFrequency ? sceKernelGetTscFrequency() : 0;
    obs_report_measure(id, "sceKernelGetTscFrequency", "freq", tsc_freq, "hz");

    uint8_t state_buf[0x100];
    memset(state_buf, 0, sizeof(state_buf));

    /* (1) 1 second blocking pump: PortGetState, PortSetAttributes, ContextAdvance(0),
     * ContextPush(0, 1) */
    uint64_t blk_ticks[40] = {0};
    int blk_rc[40] = {0};
    uint32_t blk_queued[40] = {0};
    uint32_t blk_avail[40] = {0};
    uint64_t blk_count = 0;

    if (tsc_freq > 0 && fn_push && obs_address_is_callable((const void *)fn_push)) {
        uint64_t t_start = sceKernelReadTsc ? sceKernelReadTsc() : 0;
        uint64_t t_end = t_start + tsc_freq;
        while ((sceKernelReadTsc ? sceKernelReadTsc() : 0) < t_end) {
            if (fn_port_get_state)
                fn_port_get_state(port, state_buf);
            if (fn_port_set_attr)
                fn_port_set_attr(port, &attr, 1);
            if (fn_advance)
                fn_advance(context);

            if (blk_count < 40) {
                uint64_t tb = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                int rc_p = fn_push(context, 1);
                uint64_t ta = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                blk_ticks[blk_count] = ta - tb;
                blk_rc[blk_count] = rc_p;
                uint32_t q = 0, a = 0;
                if (fn_get_queue)
                    fn_get_queue(context, &q, &a);
                blk_queued[blk_count] = q;
                blk_avail[blk_count] = a;
            } else {
                fn_push(context, 1);
            }
            blk_count++;
        }
    }

    for (int p = 0; p < 40; p++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "ContextPush-block-%d", p);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)blk_rc[p], "code");
        obs_report_measure(id, tag, "ticks", blk_ticks[p], "ticks");
        obs_report_measure(id, tag, "queued", (uint64_t)blk_queued[p], "count");
        obs_report_measure(id, tag, "available", (uint64_t)blk_avail[p], "count");
    }
    obs_report_measure(id, "block-pump", "iterations", blk_count, "count");

    /* (2) 1 second non-blocking pump: ContextPush(0, 0) */
    uint64_t nonblk_ticks[40] = {0};
    int nonblk_rc[40] = {0};
    uint32_t nonblk_queued[40] = {0};
    uint32_t nonblk_avail[40] = {0};
    uint64_t nonblk_count = 0;

    if (tsc_freq > 0 && fn_push && obs_address_is_callable((const void *)fn_push)) {
        uint64_t t_start = sceKernelReadTsc ? sceKernelReadTsc() : 0;
        uint64_t t_end = t_start + tsc_freq;
        while ((sceKernelReadTsc ? sceKernelReadTsc() : 0) < t_end) {
            if (fn_port_get_state)
                fn_port_get_state(port, state_buf);
            if (fn_port_set_attr)
                fn_port_set_attr(port, &attr, 1);
            if (fn_advance)
                fn_advance(context);

            if (nonblk_count < 40) {
                uint64_t tb = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                int rc_p = fn_push(context, 0);
                uint64_t ta = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                nonblk_ticks[nonblk_count] = ta - tb;
                nonblk_rc[nonblk_count] = rc_p;
                uint32_t q = 0, a = 0;
                if (fn_get_queue)
                    fn_get_queue(context, &q, &a);
                nonblk_queued[nonblk_count] = q;
                nonblk_avail[nonblk_count] = a;
            } else {
                fn_push(context, 0);
            }
            nonblk_count++;
        }
    }

    for (int p = 0; p < 40; p++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "ContextPush-nonblock-%d", p);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)nonblk_rc[p], "code");
        obs_report_measure(id, tag, "ticks", nonblk_ticks[p], "ticks");
        obs_report_measure(id, tag, "queued", (uint64_t)nonblk_queued[p], "count");
        obs_report_measure(id, tag, "available", (uint64_t)nonblk_avail[p], "count");
    }
    obs_report_measure(id, "nonblock-pump", "iterations", nonblk_count, "count");

    /* Clean up */
    if (fn_port_destroy && (uintptr_t)port != 0xffffffffffffffffULL) {
        fn_port_destroy(port);
    }
    if (fn_ctx_destroy && (uintptr_t)context != 0xffffffffffffffffULL) {
        fn_ctx_destroy(context);
    }
    if (fn_user_destroy && (uintptr_t)user != 0xffffffffffffffffULL) {
        fn_user_destroy(user);
    }

    return obs_pass();
}

/* REQ-20261012T1900Z-ac02: PPSA21564's AudioOut2 context in a process that has made no
 * context
 */
static obs_result check_ac02_audioout2_ppsa21564_context(void) {
    const char *id = "000-boot/audioout2-ppsa21564-context";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        obs_jmp_buf g;
        if (OBS_FAULT_ARM(&g) == 0) {
            sceSysmoduleLoadModule(0x0096);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    int h_ao = obs_module_open("libSceAudioOut2");

    typedef int (*fn_ao_init_t)(void);
    typedef int (*fn_ao_reset_param_t)(void *);
    typedef int (*fn_ao_query_mem_t)(void *, uint64_t *);
    typedef int (*fn_ao_ctx_create_t)(void *, void *, uint64_t, void **);
    typedef int (*fn_ao_user_create_t)(int32_t, void **);
    typedef int (*fn_ao_port_create_t)(void *, const void *, void **);
    typedef int (*fn_ao_port_destroy_t)(void *);
    typedef int (*fn_ao_ctx_destroy_t)(void *);
    typedef int (*fn_ao_user_destroy_t)(void *);

    const void *fn_init = (const void *)&sceAudioOut2Initialize_static;
    int is_link_init = 0;
    if (obs_address_is_callable(fn_init)) {
        is_link_init = 1;
    } else {
        fn_init = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2Initialize");
        if (!fn_init)
            fn_init = obs_module_symbol(OBS_HANDLE_SELF, "$g2tViFIohHE");
        if (!fn_init && h_ao > 0) {
            fn_init = obs_module_symbol(h_ao, "sceAudioOut2Initialize");
            if (!fn_init)
                fn_init = obs_module_symbol(h_ao, "$g2tViFIohHE");
        }
    }
    int bound_init = (fn_init != NULL && obs_address_is_callable(fn_init));
    obs_report_measure(id, "sceAudioOut2Initialize", "bound", (uint64_t)bound_init,
                       "bool");
    if (bound_init) {
        obs_report_measure(id, "sceAudioOut2Initialize", "how",
                           (uint64_t)(is_link_init ? 1 : 2),
                           is_link_init ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2Initialize", "status", 0, "not-possible");
    }

    const void *fn_reset_param = (const void *)&sceAudioOut2ContextResetParam_static;
    int is_link_reset = 0;
    if (obs_address_is_callable(fn_reset_param)) {
        is_link_reset = 1;
    } else {
        fn_reset_param =
            obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextResetParam");
        if (!fn_reset_param)
            fn_reset_param = obs_module_symbol(OBS_HANDLE_SELF, "$t5YrizufpQc");
        if (!fn_reset_param && h_ao > 0) {
            fn_reset_param = obs_module_symbol(h_ao, "sceAudioOut2ContextResetParam");
            if (!fn_reset_param)
                fn_reset_param = obs_module_symbol(h_ao, "$t5YrizufpQc");
        }
    }
    int bound_reset =
        (fn_reset_param != NULL && obs_address_is_callable(fn_reset_param));
    obs_report_measure(id, "sceAudioOut2ContextResetParam", "bound",
                       (uint64_t)bound_reset, "bool");
    if (bound_reset) {
        obs_report_measure(id, "sceAudioOut2ContextResetParam", "how",
                           (uint64_t)(is_link_reset ? 1 : 2),
                           is_link_reset ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2ContextResetParam", "status", 0,
                           "not-possible");
    }

    const void *fn_query_mem = (const void *)&sceAudioOut2ContextQueryMemory_static;
    int is_link_query = 0;
    if (obs_address_is_callable(fn_query_mem)) {
        is_link_query = 1;
    } else {
        fn_query_mem =
            obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextQueryMemory");
        if (!fn_query_mem)
            fn_query_mem = obs_module_symbol(OBS_HANDLE_SELF, "$pDmme7Bgm6E");
        if (!fn_query_mem && h_ao > 0) {
            fn_query_mem = obs_module_symbol(h_ao, "sceAudioOut2ContextQueryMemory");
            if (!fn_query_mem)
                fn_query_mem = obs_module_symbol(h_ao, "$pDmme7Bgm6E");
        }
    }
    int bound_query = (fn_query_mem != NULL && obs_address_is_callable(fn_query_mem));
    obs_report_measure(id, "sceAudioOut2ContextQueryMemory", "bound",
                       (uint64_t)bound_query, "bool");
    if (bound_query) {
        obs_report_measure(id, "sceAudioOut2ContextQueryMemory", "how",
                           (uint64_t)(is_link_query ? 1 : 2),
                           is_link_query ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2ContextQueryMemory", "status", 0,
                           "not-possible");
    }

    const void *fn_ctx_create = (const void *)&sceAudioOut2ContextCreate_static;
    int is_link_ctx_create = 0;
    if (obs_address_is_callable(fn_ctx_create)) {
        is_link_ctx_create = 1;
    } else {
        fn_ctx_create = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextCreate");
        if (!fn_ctx_create)
            fn_ctx_create = obs_module_symbol(OBS_HANDLE_SELF, "$0x6o1VVAYSY");
        if (!fn_ctx_create && h_ao > 0) {
            fn_ctx_create = obs_module_symbol(h_ao, "sceAudioOut2ContextCreate");
            if (!fn_ctx_create)
                fn_ctx_create = obs_module_symbol(h_ao, "$0x6o1VVAYSY");
        }
    }
    int bound_ctx_create =
        (fn_ctx_create != NULL && obs_address_is_callable(fn_ctx_create));
    obs_report_measure(id, "sceAudioOut2ContextCreate", "bound",
                       (uint64_t)bound_ctx_create, "bool");
    if (bound_ctx_create) {
        obs_report_measure(id, "sceAudioOut2ContextCreate", "how",
                           (uint64_t)(is_link_ctx_create ? 1 : 2),
                           is_link_ctx_create ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2ContextCreate", "status", 0,
                           "not-possible");
    }

    const void *fn_user_create = (const void *)&sceAudioOut2UserCreate_static;
    int is_link_user_create = 0;
    if (obs_address_is_callable(fn_user_create)) {
        is_link_user_create = 1;
    } else {
        fn_user_create = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2UserCreate");
        if (!fn_user_create)
            fn_user_create = obs_module_symbol(OBS_HANDLE_SELF, "$xywYcRB7nbQ");
        if (!fn_user_create && h_ao > 0) {
            fn_user_create = obs_module_symbol(h_ao, "sceAudioOut2UserCreate");
            if (!fn_user_create)
                fn_user_create = obs_module_symbol(h_ao, "$xywYcRB7nbQ");
        }
    }
    int bound_user_create =
        (fn_user_create != NULL && obs_address_is_callable(fn_user_create));
    obs_report_measure(id, "sceAudioOut2UserCreate", "bound",
                       (uint64_t)bound_user_create, "bool");
    if (bound_user_create) {
        obs_report_measure(id, "sceAudioOut2UserCreate", "how",
                           (uint64_t)(is_link_user_create ? 1 : 2),
                           is_link_user_create ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2UserCreate", "status", 0, "not-possible");
    }

    const void *fn_port_create = (const void *)&sceAudioOut2PortCreate_static;
    int is_link_port_create = 0;
    if (obs_address_is_callable(fn_port_create)) {
        is_link_port_create = 1;
    } else if (obs_address_is_callable((const void *)&sceAudioOut2PortCreate_alt)) {
        fn_port_create = (const void *)&sceAudioOut2PortCreate_alt;
        is_link_port_create = 1;
    } else {
        fn_port_create = obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2PortCreate");
        if (!fn_port_create)
            fn_port_create = obs_module_symbol(OBS_HANDLE_SELF, "$A89PZmqwrSQ");
        if (!fn_port_create && h_ao > 0) {
            fn_port_create = obs_module_symbol(h_ao, "sceAudioOut2PortCreate");
            if (!fn_port_create)
                fn_port_create = obs_module_symbol(h_ao, "$A89PZmqwrSQ");
            if (!fn_port_create)
                fn_port_create = obs_module_symbol(h_ao, "$JK2wamZPzwM");
        }
    }
    int bound_port_create =
        (fn_port_create != NULL && obs_address_is_callable(fn_port_create));
    obs_report_measure(id, "sceAudioOut2PortCreate", "bound",
                       (uint64_t)bound_port_create, "bool");
    if (bound_port_create) {
        obs_report_measure(id, "sceAudioOut2PortCreate", "how",
                           (uint64_t)(is_link_port_create ? 1 : 2),
                           is_link_port_create ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceAudioOut2PortCreate", "status", 0, "not-possible");
    }

    const void *fn_port_destroy = (const void *)&sceAudioOut2PortDestroy_static;
    if (!obs_address_is_callable(fn_port_destroy)) {
        fn_port_destroy = (const void *)&sceAudioOut2PortDestroy_alt;
    }
    if (!obs_address_is_callable(fn_port_destroy) && h_ao > 0) {
        fn_port_destroy = obs_module_symbol(h_ao, "sceAudioOut2PortDestroy");
        if (!fn_port_destroy)
            fn_port_destroy = obs_module_symbol(h_ao, "$cd+Rtw+D1x8");
        if (!fn_port_destroy)
            fn_port_destroy = obs_module_symbol(h_ao, "$H9eDD7eR33E");
    }

    const void *fn_ctx_destroy = (const void *)&sceAudioOut2ContextDestroy_static;
    if (!obs_address_is_callable(fn_ctx_destroy)) {
        fn_ctx_destroy = (const void *)&sceAudioOut2ContextDestroy_alt;
    }
    if (!obs_address_is_callable(fn_ctx_destroy) && h_ao > 0) {
        fn_ctx_destroy = obs_module_symbol(h_ao, "sceAudioOut2ContextDestroy");
        if (!fn_ctx_destroy)
            fn_ctx_destroy = obs_module_symbol(h_ao, "$on6ZH7Abo10");
        if (!fn_ctx_destroy)
            fn_ctx_destroy = obs_module_symbol(h_ao, "$f37wK4+X5m0");
    }

    const void *fn_user_destroy = (const void *)&sceAudioOut2UserDestroy_static;
    if (!obs_address_is_callable(fn_user_destroy)) {
        fn_user_destroy = (const void *)&sceAudioOut2UserDestroy_alt;
    }
    if (!obs_address_is_callable(fn_user_destroy) && h_ao > 0) {
        fn_user_destroy = obs_module_symbol(h_ao, "sceAudioOut2UserDestroy");
        if (!fn_user_destroy)
            fn_user_destroy = obs_module_symbol(h_ao, "$IaZXJ9M79uo");
        if (!fn_user_destroy)
            fn_user_destroy = obs_module_symbol(h_ao, "$6vY70ydXpiE");
    }

    int sig = 0;

    /* 1. sceAudioOut2Initialize */
    int rc_init = -1;
    if (fn_init && obs_address_is_callable(fn_init)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = ((fn_ao_init_t)(uintptr_t)fn_init)();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceAudioOut2Initialize", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "sceAudioOut2Initialize", "rc", (uint64_t)(uint32_t)rc_init,
                       "code");

    /* ContextResetParam(param) on a 0x20-byte block filled 0xcc */
    uint8_t param[0x20];
    memset(param, 0xcc, sizeof(param));
    int rc_reset = -1;
    if (fn_reset_param && obs_address_is_callable(fn_reset_param)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reset = ((fn_ao_reset_param_t)(uintptr_t)fn_reset_param)(param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextResetParam", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "ContextResetParam", "rc", (uint64_t)(uint32_t)rc_reset,
                       "code");
    obs_report_bytes(id, "ContextResetParam", "param-after-reset", 0, param,
                     sizeof(param));

    /* then the title's words written over it:
     * u32 0x14, 0x200, 0, 1, 0x200, 1 at +0x00..+0x14, the rest zero. */
    *(uint32_t *)(void *)(param + 0x00) = 0x14u;
    *(uint32_t *)(void *)(param + 0x04) = 0x200u;
    *(uint32_t *)(void *)(param + 0x08) = 0u;
    *(uint32_t *)(void *)(param + 0x0c) = 1u;
    *(uint32_t *)(void *)(param + 0x10) = 0x200u;
    *(uint32_t *)(void *)(param + 0x14) = 1u;
    memset(param + 0x18, 0, sizeof(param) - 0x18);
    obs_report_bytes(id, "ContextResetParam", "param-title-words", 0, param,
                     sizeof(param));

    /* 2. sceAudioOut2ContextQueryMemory(param, &size) - rc and the 8-byte size */
    uint64_t mem_size = 0xccccccccccccccccULL;
    int rc_query = -1;
    if (fn_query_mem && obs_address_is_callable(fn_query_mem)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_query = ((fn_ao_query_mem_t)(uintptr_t)fn_query_mem)(param, &mem_size);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextQueryMemory", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "ContextQueryMemory", "rc", (uint64_t)(uint32_t)rc_query,
                       "code");
    obs_report_measure(id, "ContextQueryMemory", "size", mem_size, "bytes");
    obs_report_bytes(id, "ContextQueryMemory", "size-8b", 0, (const uint8_t *)&mem_size,
                     8);

    /* 3. sceAudioOut2ContextCreate(param, memory, size, &context) - memory 16-aligned,
     * of the size (2) answered; rc and the context's 8 bytes. */
    static _Alignas(0x1000) uint8_t s_ac02_mem[4 * 1024 * 1024];
    void *memory = (void *)s_ac02_mem;
    uint64_t alloc_size =
        (mem_size > 0 && mem_size <= sizeof(s_ac02_mem)) ? mem_size : 0x10000u;
    memset(s_ac02_mem, 0xcc,
           alloc_size <= sizeof(s_ac02_mem) ? alloc_size : sizeof(s_ac02_mem));

    void *context = (void *)0xffffffffffffffffULL;
    int rc_ctx = -1;
    if (fn_ctx_create && obs_address_is_callable(fn_ctx_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ctx = ((fn_ao_ctx_create_t)(uintptr_t)fn_ctx_create)(
                param, memory, alloc_size, &context);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextCreate", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "ContextCreate", "rc", (uint64_t)(uint32_t)rc_ctx, "code");
    obs_report_measure(id, "ContextCreate", "context", (uint64_t)(uintptr_t)context,
                       "addr");
    obs_report_bytes(id, "ContextCreate", "context-8b", 0, (const uint8_t *)&context,
                     8);

    /* If that refuses, also try with 3rd arg 0 and measured order (param, 0x1000, size,
     * &context) */
    if (rc_ctx != 0 && fn_ctx_create && obs_address_is_callable(fn_ctx_create)) {
        void *context_zero = (void *)0xffffffffffffffffULL;
        int rc_zero = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_zero = ((fn_ao_ctx_create_t)(uintptr_t)fn_ctx_create)(param, memory, 0,
                                                                     &context_zero);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "ContextCreate-zero-arg3", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "ContextCreate-zero-arg3", "rc",
                           (uint64_t)(uint32_t)rc_zero, "code");
        obs_report_measure(id, "ContextCreate-zero-arg3", "context",
                           (uint64_t)(uintptr_t)context_zero, "addr");
        obs_report_bytes(id, "ContextCreate-zero-arg3", "context-8b", 0,
                         (const uint8_t *)&context_zero, 8);
        if (rc_zero == 0) {
            context = context_zero;
            rc_ctx = rc_zero;
        } else {
            void *context_meas = (void *)0xffffffffffffffffULL;
            int rc_meas = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_meas = ((fn_ao_ctx_create_t)(uintptr_t)fn_ctx_create)(
                    param, (void *)(uintptr_t)0x1000, alloc_size, &context_meas);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "ContextCreate-measured-order", "fault",
                                   (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "ContextCreate-measured-order", "rc",
                               (uint64_t)(uint32_t)rc_meas, "code");
            obs_report_measure(id, "ContextCreate-measured-order", "context",
                               (uint64_t)(uintptr_t)context_meas, "addr");
            obs_report_bytes(id, "ContextCreate-measured-order", "context-8b", 0,
                             (const uint8_t *)&context_meas, 8);
            if (rc_meas == 0) {
                context = context_meas;
                rc_ctx = rc_meas;
            }
        }
    }

    /* 4. sceAudioOut2UserCreate(0xff, &user) - rc and the user's 8 bytes */
    void *user = (void *)0xffffffffffffffffULL;
    int rc_user = -1;
    if (fn_user_create && obs_address_is_callable(fn_user_create)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_user = ((fn_ao_user_create_t)(uintptr_t)fn_user_create)(0xff, &user);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "UserCreate-0xff", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "UserCreate-0xff", "rc", (uint64_t)(uint32_t)rc_user,
                       "code");
    obs_report_measure(id, "UserCreate-0xff", "user", (uint64_t)(uintptr_t)user,
                       "addr");
    obs_report_bytes(id, "UserCreate-0xff", "user-8b", 0, (const uint8_t *)&user, 8);

    /* REQ-20261012T2050Z-ac03: first word of an AudioOut2 port's parameters
     * PortCreate(context, param, &port) with param 0x40 bytes zeroed, then u32 N,
     * 0x800 (8 ch), 48000, type (0 or 2), user at +0x10, port filled 0xff,
     * for N = 0, 1, 2, 3, 7, 0x100 without destroying ports between them. */
    static const uint32_t n_vals[] = {0, 1, 2, 3, 7, 0x100};
    void *port0 = (void *)0xffffffffffffffffULL;

    for (int t_idx = 0; t_idx < 2; t_idx++) {
        uint32_t ptype = (t_idx == 0) ? 0 : 2;
        for (size_t i = 0; i < sizeof(n_vals) / sizeof(n_vals[0]); i++) {
            uint32_t n_val = n_vals[i];
            char tag[64];
            snprintf(tag, sizeof(tag), "PortCreate-type%u-N0x%x", ptype, n_val);

            uint8_t p_param[0x40];
            memset(p_param, 0, sizeof(p_param));
            *(uint32_t *)(void *)(p_param + 0x00) = n_val;
            *(uint32_t *)(void *)(p_param + 0x04) = 0x800; /* 8 channels */
            *(uint32_t *)(void *)(p_param + 0x08) = 48000;
            *(uint32_t *)(void *)(p_param + 0x0c) = ptype;
            *(uint64_t *)(void *)(p_param + 0x10) = (uint64_t)(uintptr_t)user;

            uint8_t p_before[0x40];
            memcpy(p_before, p_param, sizeof(p_param));

            void *p_out = (void *)0xffffffffffffffffULL;
            int rc_p = -1;
            if (fn_port_create && obs_address_is_callable(fn_port_create)) {
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_p = ((fn_ao_port_create_t)(uintptr_t)fn_port_create)(
                        context, p_param, &p_out);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                    obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
                }
            }
            int param_changed = (memcmp(p_param, p_before, sizeof(p_param)) != 0);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_p, "code");
            obs_report_measure(id, tag, "port", (uint64_t)(uintptr_t)p_out, "addr");
            obs_report_measure(id, tag, "param-changed", (uint64_t)param_changed,
                               "bool");
            obs_report_bytes(id, tag, "port-8b", 0, (const uint8_t *)&p_out, 8);
            if (param_changed) {
                obs_report_bytes(id, tag, "param-changed-bytes", 0, p_param,
                                 sizeof(p_param));
            }
            if (t_idx == 0 && i == 0 && rc_p == 0) {
                port0 = p_out;
            }
        }
    }

    /* REQ-20261012T1814Z-24b2: AudioOut2 Context runtime */
    typedef int (*fn_ao_get_queue_t)(void *, uint32_t *, uint32_t *);
    typedef int (*fn_ao_push_t)(void *, uint32_t);
    typedef int (*fn_ao_advance_t)(void *);

    fn_ao_get_queue_t fn_get_queue =
        obs_address_is_callable((const void *)&sceAudioOut2ContextGetQueueLevel_static)
            ? (fn_ao_get_queue_t)(void
                                      *)(uintptr_t)&sceAudioOut2ContextGetQueueLevel_static
            : (obs_address_is_callable(
                   (const void *)&sceAudioOut2ContextGetQueueLevel_alt)
                   ? (fn_ao_get_queue_t)(void
                                             *)(uintptr_t)&sceAudioOut2ContextGetQueueLevel_alt
                   : NULL);
    if (!fn_get_queue) {
        fn_get_queue = (fn_ao_get_queue_t)obs_module_symbol(
            OBS_HANDLE_SELF, "sceAudioOut2ContextGetQueueLevel");
        if (!fn_get_queue)
            fn_get_queue =
                (fn_ao_get_queue_t)obs_module_symbol(OBS_HANDLE_SELF, "$R7d0F1g2qsU");
        if (!fn_get_queue)
            fn_get_queue =
                (fn_ao_get_queue_t)obs_module_symbol(OBS_HANDLE_SELF, "$xao2WBd0t0c");
        if (!fn_get_queue && h_ao > 0) {
            fn_get_queue = (fn_ao_get_queue_t)obs_module_symbol(h_ao, "$R7d0F1g2qsU");
            if (!fn_get_queue)
                fn_get_queue =
                    (fn_ao_get_queue_t)obs_module_symbol(h_ao, "$xao2WBd0t0c");
        }
    }

    fn_ao_push_t fn_push =
        obs_address_is_callable((const void *)&sceAudioOut2ContextPush_static)
            ? (fn_ao_push_t)(void *)(uintptr_t)&sceAudioOut2ContextPush_static
            : (obs_address_is_callable((const void *)&sceAudioOut2ContextPush_alt)
                   ? (fn_ao_push_t)(void *)(uintptr_t)&sceAudioOut2ContextPush_alt
                   : NULL);
    if (!fn_push) {
        fn_push =
            (fn_ao_push_t)obs_module_symbol(OBS_HANDLE_SELF, "sceAudioOut2ContextPush");
        if (!fn_push)
            fn_push = (fn_ao_push_t)obs_module_symbol(OBS_HANDLE_SELF, "$aII9h5nli9U");
        if (!fn_push)
            fn_push = (fn_ao_push_t)obs_module_symbol(OBS_HANDLE_SELF, "$1YvlmYc9gmg");
        if (!fn_push && h_ao > 0) {
            fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "$aII9h5nli9U");
            if (!fn_push)
                fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "$1YvlmYc9gmg");
        }
    }

    fn_ao_advance_t fn_advance =
        obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_static)
            ? (fn_ao_advance_t)&sceAudioOut2ContextAdvance_static
            : (obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_alt)
                   ? (fn_ao_advance_t)&sceAudioOut2ContextAdvance_alt
                   : NULL);
    if (!fn_advance) {
        fn_advance = (fn_ao_advance_t)obs_module_symbol(OBS_HANDLE_SELF,
                                                        "sceAudioOut2ContextAdvance");
        if (!fn_advance)
            fn_advance =
                (fn_ao_advance_t)obs_module_symbol(OBS_HANDLE_SELF, "$PE2zHMqLSHs");
        if (!fn_advance)
            fn_advance =
                (fn_ao_advance_t)obs_module_symbol(OBS_HANDLE_SELF, "$e0iLyhyzTTw");
        if (!fn_advance && h_ao > 0) {
            fn_advance = (fn_ao_advance_t)obs_module_symbol(h_ao, "$PE2zHMqLSHs");
            if (!fn_advance)
                fn_advance = (fn_ao_advance_t)obs_module_symbol(h_ao, "$e0iLyhyzTTw");
        }
    }

    obs_report_measure(id, "ContextGetQueueLevel", "resolved",
                       (uint64_t)(fn_get_queue != NULL), "bool");
    obs_report_measure(id, "ContextPush", "resolved", (uint64_t)(fn_push != NULL),
                       "bool");
    obs_report_measure(id, "ContextAdvance", "resolved", (uint64_t)(fn_advance != NULL),
                       "bool");

    if (fn_get_queue && obs_address_is_callable((const void *)fn_get_queue)) {
        /* 1. ContextGetQueueLevel before any push: slots filled 0xff, dump 8 bytes each
         */
        uint64_t slot_q = 0xffffffffffffffffULL;
        uint64_t slot_a = 0xffffffffffffffffULL;
        int rc_ql = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ql = fn_get_queue(context, (uint32_t *)(void *)&slot_q,
                                 (uint32_t *)(void *)&slot_a);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetQueueLevel-init", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetQueueLevel-init", "rc", (uint64_t)(uint32_t)rc_ql,
                           "code");
        obs_report_bytes(id, "GetQueueLevel-init", "queued-slot-8b", 0,
                         (const uint8_t *)&slot_q, 8);
        obs_report_bytes(id, "GetQueueLevel-init", "available-slot-8b", 0,
                         (const uint8_t *)&slot_a, 8);

        /* Arms: queued NULL, available NULL, both NULL, ctx 1, ctx 0xffffffffffffffff
         */
        int rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_get_queue(context, NULL, (uint32_t *)(void *)&slot_a);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetQueueLevel-null-queued", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_get_queue(context, (uint32_t *)(void *)&slot_q, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetQueueLevel-null-avail", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_get_queue(context, NULL, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetQueueLevel-null-both", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_get_queue((void *)1, (uint32_t *)(void *)&slot_q,
                                  (uint32_t *)(void *)&slot_a);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetQueueLevel-ctx-1", "rc", (uint64_t)(uint32_t)rc_arm,
                           "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm =
                fn_get_queue((void *)0xffffffffffffffffULL, (uint32_t *)(void *)&slot_q,
                             (uint32_t *)(void *)&slot_a);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetQueueLevel-ctx-minus1", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");
    }

    /* 2. ContextAdvance(ctx) */
    if (fn_advance && obs_address_is_callable((const void *)fn_advance)) {
        uint64_t t_before = sceKernelReadTsc ? sceKernelReadTsc() : 0;
        int rc_adv = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_adv = fn_advance(context);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        uint64_t t_after = sceKernelReadTsc ? sceKernelReadTsc() : 0;
        obs_report_measure(id, "ContextAdvance-1", "rc", (uint64_t)(uint32_t)rc_adv,
                           "code");
        obs_report_measure(id, "ContextAdvance-1", "ticks", t_after - t_before,
                           "ticks");

        /* Second Advance without push */
        rc_adv = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_adv = fn_advance(context);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextAdvance-2", "rc", (uint64_t)(uint32_t)rc_adv,
                           "code");

        /* Arms: ctx 1 and ctx -1 */
        int rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_advance((void *)1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextAdvance-ctx-1", "rc", (uint64_t)(uint32_t)rc_arm,
                           "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_advance((void *)0xffffffffffffffffULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextAdvance-ctx-minus1", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");
    }

    /* 3. ContextPush(ctx, 0) non-blocking 6 times */
    if (fn_push && obs_address_is_callable((const void *)fn_push)) {
        for (int p = 0; p < 6; p++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "ContextPush-nonblock-%d", p);
            uint64_t tb = sceKernelReadTsc ? sceKernelReadTsc() : 0;
            int rc_p = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p = fn_push(context, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            uint64_t ta = sceKernelReadTsc ? sceKernelReadTsc() : 0;
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_p, "code");
            obs_report_measure(id, tag, "ticks", ta - tb, "ticks");

            if (fn_get_queue && obs_address_is_callable((const void *)fn_get_queue)) {
                uint32_t q = 0, a = 0;
                fn_get_queue(context, &q, &a);
                obs_report_measure(id, tag, "queued", (uint64_t)q, "count");
                obs_report_measure(id, tag, "available", (uint64_t)a, "count");
            }
        }

        /* Push(ctx, 1) blocking 20 times */
        for (int p = 0; p < 20; p++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "ContextPush-block-%d", p);
            uint64_t tb = sceKernelReadTsc ? sceKernelReadTsc() : 0;
            int rc_p = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p = fn_push(context, 1);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            uint64_t ta = sceKernelReadTsc ? sceKernelReadTsc() : 0;
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_p, "code");
            obs_report_measure(id, tag, "ticks", ta - tb, "ticks");
        }

        /* Arms: ctx 1, ctx -1, flag 2 */
        int rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_push((void *)1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextPush-ctx-1", "rc", (uint64_t)(uint32_t)rc_arm,
                           "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_push((void *)0xffffffffffffffffULL, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextPush-ctx-minus1", "rc",
                           (uint64_t)(uint32_t)rc_arm, "code");

        rc_arm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_arm = fn_push(context, 2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ContextPush-flag-2", "rc", (uint64_t)(uint32_t)rc_arm,
                           "code");
    }

    /* Clean up */
    if (fn_port_destroy && obs_address_is_callable(fn_port_destroy)) {
        if ((uintptr_t)port0 != 0xffffffffffffffffULL && port0 != NULL) {
            ((fn_ao_port_destroy_t)(uintptr_t)fn_port_destroy)(port0);
        }
    }
    if (fn_ctx_destroy && obs_address_is_callable(fn_ctx_destroy)) {
        if (rc_ctx == 0 && (uintptr_t)context != 0xffffffffffffffffULL) {
            ((fn_ao_ctx_destroy_t)(uintptr_t)fn_ctx_destroy)(context);
        }
    }
    if (fn_user_destroy && obs_address_is_callable(fn_user_destroy)) {
        if (rc_user == 0 && (uintptr_t)user != 0xffffffffffffffffULL) {
            ((fn_ao_user_destroy_t)(uintptr_t)fn_user_destroy)(user);
        }
    }

    return obs_pass();
}

/* ---- REQ-20261012T1827Z-1a71: Hardware queries and POSIX priority ---------------- */
static obs_result check_hardware_queries(void) {
    const char *id = "000-boot/hardware-queries";
    obs_jmp_buf guard;
    int sig;

    /* 1. sceKernelGetCpuFrequency() twice 1s apart */
    uint64_t freq1 = 0, freq2 = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetCpuFrequency)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            freq1 = sceKernelGetCpuFrequency();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCpuFrequency-1", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCpuFrequency-1", "rax", freq1, "hz");

        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(1000000u);
        }

        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            freq2 = sceKernelGetCpuFrequency();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCpuFrequency-2", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCpuFrequency-2", "rax", freq2, "hz");
    } else {
        obs_report_measure(id, "GetCpuFrequency", "status", 0, "not-possible");
    }

    /* 2. sceKernelGetCpuTemperature(&out) & sceKernelGetSocSensorTemperature */
    uint8_t out_temp[32];
    memset(out_temp, 0xcc, sizeof(out_temp));
    int rc_cpu_temp = -1;
    if (obs_address_is_callable((const void *)&sceKernelGetCpuTemperature)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cpu_temp = sceKernelGetCpuTemperature((int *)(void *)out_temp);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCpuTemperature", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCpuTemperature", "rc",
                           (uint64_t)(uint32_t)rc_cpu_temp, "code");
        obs_report_bytes(id, "GetCpuTemperature", "out-32b", 0, out_temp,
                         sizeof(out_temp));

        int rc_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_null = sceKernelGetCpuTemperature(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCpuTemperature-null", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCpuTemperature-null", "rc",
                           (uint64_t)(uint32_t)rc_null, "code");
    } else {
        obs_report_measure(id, "GetCpuTemperature", "status", 0, "not-possible");
    }

    static const int sensors[] = {0, 1, 0xff};
    for (size_t s = 0; s < sizeof(sensors) / sizeof(sensors[0]); s++) {
        int sensor = sensors[s];
        char tag[64];
        snprintf(tag, sizeof(tag), "GetSocSensorTemperature-%d", sensor);
        memset(out_temp, 0xcc, sizeof(out_temp));
        int rc_soc = -1;
        if (obs_address_is_callable((const void *)&sceKernelGetSocSensorTemperature)) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_soc =
                    sceKernelGetSocSensorTemperature(sensor, (int *)(void *)out_temp);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_soc, "code");
            obs_report_bytes(id, tag, "out-32b", 0, out_temp, sizeof(out_temp));
        } else {
            obs_report_measure(id, tag, "status", 0, "not-possible");
        }
    }
    if (obs_address_is_callable((const void *)&sceKernelGetSocSensorTemperature)) {
        int rc_soc_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_soc_null = sceKernelGetSocSensorTemperature(0, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetSocSensorTemperature-null", "fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetSocSensorTemperature-null", "rc",
                           (uint64_t)(uint32_t)rc_soc_null, "code");
    }

    /* 3. sceKernelGetCurrentFanDuty(&out) */
    uint8_t out_fan[32];
    memset(out_fan, 0xcc, sizeof(out_fan));
    int rc_fan = -1;
    if (obs_address_is_callable((const void *)&sceKernelGetCurrentFanDuty)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_fan = sceKernelGetCurrentFanDuty((int *)(void *)out_fan);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentFanDuty", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCurrentFanDuty", "rc", (uint64_t)(uint32_t)rc_fan,
                           "code");
        obs_report_bytes(id, "GetCurrentFanDuty", "out-32b", 0, out_fan,
                         sizeof(out_fan));

        int rc_fan_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_fan_null = sceKernelGetCurrentFanDuty(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentFanDuty-null", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetCurrentFanDuty-null", "rc",
                           (uint64_t)(uint32_t)rc_fan_null, "code");
    } else {
        obs_report_measure(id, "GetCurrentFanDuty", "status", 0, "not-possible");
    }

    /* 4. sceKernelGetHwModelName(buf) */
    uint8_t model_buf[32];
    memset(model_buf, 0xcc, sizeof(model_buf));
    int rc_model = -1;
    if (obs_address_is_callable((const void *)&sceKernelGetHwModelName)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_model = sceKernelGetHwModelName((char *)(void *)model_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetHwModelName", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "GetHwModelName", "rc", (uint64_t)(uint32_t)rc_model,
                           "code");
        obs_report_bytes(id, "GetHwModelName", "buf-32b", 0, model_buf,
                         sizeof(model_buf));
    } else {
        obs_report_measure(id, "GetHwModelName", "status", 0, "not-possible");
    }

    /* 5. sceKernelGetHwSerialNumber(buf): NEVER print serial value, only byte count and
     * character classes! */
    uint8_t serial_buf[32];
    memset(serial_buf, 0xcc, sizeof(serial_buf));
    int rc_serial = -1;
    if (obs_address_is_callable((const void *)&sceKernelGetHwSerialNumber)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_serial = sceKernelGetHwSerialNumber((char *)(void *)serial_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetHwSerialNumber", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetHwSerialNumber", "rc", (uint64_t)(uint32_t)rc_serial,
                           "code");
        size_t digits = 0, letters = 0, nul_pos = 32, other = 0;
        for (size_t b = 0; b < sizeof(serial_buf); b++) {
            uint8_t c = serial_buf[b];
            if (c == 0 && nul_pos == 32) {
                nul_pos = b;
            }
            if (c >= '0' && c <= '9')
                digits++;
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                letters++;
            else if (c != 0 && c != 0xcc)
                other++;
        }
        obs_report_measure(id, "GetHwSerialNumber", "nul-pos", (uint64_t)nul_pos,
                           "offset");
        obs_report_measure(id, "GetHwSerialNumber", "digit-count", (uint64_t)digits,
                           "count");
        obs_report_measure(id, "GetHwSerialNumber", "letter-count", (uint64_t)letters,
                           "count");
        obs_report_measure(id, "GetHwSerialNumber", "other-count", (uint64_t)other,
                           "count");

        int rc_ser_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ser_null = sceKernelGetHwSerialNumber(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetHwSerialNumber-null", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "GetHwSerialNumber-null", "rc",
                           (uint64_t)(uint32_t)rc_ser_null, "code");
    } else {
        obs_report_measure(id, "GetHwSerialNumber", "status", 0, "not-possible");
    }

    /* 6. sched_get_priority_max and min for policies 0, 1, 2, 3, 99 */
    static const int policies[] = {0, 1, 2, 3, 99};
    for (size_t p = 0; p < sizeof(policies) / sizeof(policies[0]); p++) {
        int pol = policies[p];
        char tag_max[64], tag_min[64];
        snprintf(tag_max, sizeof(tag_max), "sched_get_priority_max-%d", pol);
        snprintf(tag_min, sizeof(tag_min), "sched_get_priority_min-%d", pol);

        if (obs_address_is_callable((const void *)&sched_get_priority_max)) {
            int ret_max = -999;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ret_max = sched_get_priority_max(pol);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag_max, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag_max, "ret", (uint64_t)(int64_t)ret_max, "int");
        } else {
            obs_report_measure(id, tag_max, "status", 0, "not-possible");
        }

        if (obs_address_is_callable((const void *)&sched_get_priority_min)) {
            int ret_min = -999;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                ret_min = sched_get_priority_min(pol);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag_min, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag_min, "ret", (uint64_t)(int64_t)ret_min, "int");
        } else {
            obs_report_measure(id, tag_min, "status", 0, "not-possible");
        }
    }

    return obs_pass();
}

/* ---- REQ-20261012T1822Z-8b13: Zoom follow focus -----------------------------------
 */
static obs_result check_user_service_zoom_follow_focus(void) {
    const char *id = "000-boot/user-service-zoom-follow-focus";
    obs_jmp_buf guard;
    int sig;

    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0) {
            user = -1;
        }
    }
    if (user < 0) {
        user = 0x1ea2f4d9;
    }

    const void *fn_follow =
        (const void *)&sceUserServiceGetAccessibilityZoomFollowFocus;
    if (!obs_address_is_callable(fn_follow)) {
        fn_follow = obs_module_symbol(OBS_HANDLE_SELF,
                                      "sceUserServiceGetAccessibilityZoomFollowFocus");
    }
    if (!obs_address_is_callable(fn_follow)) {
        fn_follow = obs_module_symbol(OBS_HANDLE_SELF, "$O6IW1-Dwm-w");
    }
    if (!obs_address_is_callable(fn_follow)) {
        int h = obs_module_open("libSceUserService");
        if (h > 0) {
            fn_follow = obs_module_symbol(h, "$O6IW1-Dwm-w");
            if (!fn_follow)
                fn_follow = obs_module_symbol(
                    h, "sceUserServiceGetAccessibilityZoomFollowFocus");
        }
    }

    typedef int (*fn_zoom_follow_t)(int32_t, void *);
    fn_zoom_follow_t fn_call = (fn_zoom_follow_t)fn_follow;

    if (!fn_call || !obs_address_is_callable((const void *)fn_call)) {
        obs_report_measure(id, "ZoomFollowFocus", "status", 0, "not-possible");
        return obs_pass();
    }

    int32_t test_users[] = {user, 0, -1, (int32_t)0x12345678};
    for (size_t u = 0; u < sizeof(test_users) / sizeof(test_users[0]); u++) {
        int32_t u_id = test_users[u];
        char tag[64];
        snprintf(tag, sizeof(tag), "ZoomFollowFocus-user-%d", (int)u_id);
        uint8_t out[8];
        memset(out, 0xcc, sizeof(out));
        int rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_call(u_id, out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, tag, "out-8b", 0, out, sizeof(out));
    }

    int rc_null = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null = fn_call(user, NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "ZoomFollowFocus-null", "fault", (uint64_t)sig,
                           "signal");
    }
    obs_report_measure(id, "ZoomFollowFocus-null", "rc", (uint64_t)(uint32_t)rc_null,
                       "code");

    return obs_pass();
}

/* ---- REQ-20261012T1816Z-7af9: Net InetPton and ErrnoLoc ---------------------------
 */
static obs_result check_net_inet_pton(void) {
    const char *id = "000-boot/net-inet-pton";
    obs_jmp_buf guard;
    int sig;

    (void)obs_sysmodule_load_id(0x0001);

    const void *fn_pton = (const void *)&sceNetInetPton;
    if (!obs_address_is_callable(fn_pton)) {
        fn_pton = obs_module_symbol(OBS_HANDLE_SELF, "sceNetInetPton");
    }
    if (!obs_address_is_callable(fn_pton)) {
        fn_pton = obs_module_symbol(OBS_HANDLE_SELF, "$StXq3-Upp_A");
    }

    const void *fn_errno = (const void *)&sceNetErrnoLoc;
    if (!obs_address_is_callable(fn_errno)) {
        fn_errno = obs_module_symbol(OBS_HANDLE_SELF, "sceNetErrnoLoc");
    }
    if (!obs_address_is_callable(fn_errno)) {
        fn_errno = obs_module_symbol(OBS_HANDLE_SELF, "$lIoG852wAx0");
    }

    typedef int (*fn_pton_t)(int, const char *, void *);
    typedef int *(*fn_errno_t)(void);
    fn_pton_t f_pton = (fn_pton_t)fn_pton;
    fn_errno_t f_errno = (fn_errno_t)fn_errno;

    if (!f_pton || !obs_address_is_callable((const void *)f_pton)) {
        obs_report_measure(id, "sceNetInetPton", "status", 0, "not-possible");
        return obs_pass();
    }

    int *errno_ptr1 = NULL;
    int *errno_ptr2 = NULL;
    if (f_errno && obs_address_is_callable((const void *)f_errno)) {
        errno_ptr1 = f_errno();
        errno_ptr2 = f_errno();
    }
    obs_report_measure(id, "sceNetErrnoLoc", "addr", (uint64_t)(uintptr_t)errno_ptr1,
                       "addr");
    obs_report_measure(id, "sceNetErrnoLoc", "addr2", (uint64_t)(uintptr_t)errno_ptr2,
                       "addr");
    obs_report_measure(id, "sceNetErrnoLoc", "ptrs-equal",
                       (uint64_t)(errno_ptr1 == errno_ptr2), "bool");

    static const struct {
        int af;
        const char *src;
        const char *tag;
    } text_arms[] = {
        {2, "192.168.1.2", "v4-192.168.1.2"},
        {2, "0.0.0.0", "v4-0.0.0.0"},
        {2, "255.255.255.255", "v4-255.255.255.255"},
        {2, "256.1.1.1", "v4-256.1.1.1"},
        {2, "1.2.3", "v4-1.2.3"},
        {2, "1.2.3.4.5", "v4-1.2.3.4.5"},
        {2, "01.2.3.4", "v4-01.2.3.4"},
        {2, " 1.2.3.4", "v4-leading-space"},
        {2, "1.2.3.4 ", "v4-trailing-space"},
        {2, "", "v4-empty"},
        {28, "::1", "v6-loopback"},
        {28, "::ffff:1.2.3.4", "v6-v4-mapped"},
        {28, "fe80::1:2", "v6-fe80"},
        {28, "1::2::3", "v6-double-colon-twice"},
        {28, "12345::1", "v6-5-digit-hex"},
        {28, "1111:2222:3333:4444:5555:6666:7777:8888", "v6-39char"},
        {28, "1111:2222:3333:4444:5555:6666:7777:8888:9999", "v6-47char"},
        {99, "192.168.1.1", "af-99"},
        {1, "192.168.1.1", "af-1-unix"},
    };

    uint8_t dst32[32];
    for (size_t a = 0; a < sizeof(text_arms) / sizeof(text_arms[0]); a++) {
        if (errno_ptr1)
            *errno_ptr1 = 0x7777;
        memset(dst32, 0xa5, sizeof(dst32));
        int rc = f_pton(text_arms[a].af, text_arms[a].src, dst32);
        obs_report_measure(id, text_arms[a].tag, "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, text_arms[a].tag, "dst-32b", 0, dst32, sizeof(dst32));
        if (errno_ptr1) {
            obs_report_measure(id, text_arms[a].tag, "errno", (uint64_t)*errno_ptr1,
                               "code");
        }
    }

    if (errno_ptr1)
        *errno_ptr1 = 0x7777;
    memset(dst32, 0xa5, sizeof(dst32));
    int rc_null_src = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null_src = f_pton(2, NULL, dst32);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "pton-null-src", "rc", (uint64_t)(uint32_t)rc_null_src,
                       "code");
    if (errno_ptr1)
        obs_report_measure(id, "pton-null-src", "errno", (uint64_t)*errno_ptr1, "code");

    if (errno_ptr1)
        *errno_ptr1 = 0x7777;
    int rc_null_dst = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null_dst = f_pton(2, "192.168.1.1", NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "pton-null-dst", "rc", (uint64_t)(uint32_t)rc_null_dst,
                       "code");
    if (errno_ptr1)
        obs_report_measure(id, "pton-null-dst", "errno", (uint64_t)*errno_ptr1, "code");

    if (obs_address_is_callable((const void *)&sceNetSocketClose)) {
        if (errno_ptr1)
            *errno_ptr1 = 0;
        int rc_fail = sceNetSocketClose(9999);
        int err_after_fail = errno_ptr1 ? *errno_ptr1 : 0;
        obs_report_measure(id, "errno-persist-fail", "rc", (uint64_t)(uint32_t)rc_fail,
                           "code");
        obs_report_measure(id, "errno-persist-fail", "errno", (uint64_t)err_after_fail,
                           "code");

        int rc_succ = f_pton(2, "192.168.1.1", dst32);
        int err_after_succ = errno_ptr1 ? *errno_ptr1 : 0;
        obs_report_measure(id, "errno-persist-succ", "rc", (uint64_t)(uint32_t)rc_succ,
                           "code");
        obs_report_measure(id, "errno-persist-succ", "errno", (uint64_t)err_after_succ,
                           "code");
        obs_report_measure(id, "errno-persist", "overwritten-by-success",
                           (uint64_t)(err_after_fail != err_after_succ), "bool");
    }

    return obs_pass();
}

/* ---- REQ-20261012T1943Z-41aa: sceSysmoduleIsLoaded lifecycle -------------------- */
static obs_result check_41aa_sysmodule_isloaded_lifecycle(void) {
    const char *id = "000-boot/sysmodule-isloaded-lifecycle";

    if (!obs_address_is_callable((const void *)&sceSysmoduleIsLoaded) ||
        !obs_address_is_callable((const void *)&sceSysmoduleLoadModule) ||
        !obs_address_is_callable((const void *)&sceSysmoduleUnloadModule)) {
        obs_report_measure(id, "sysmodule-lifecycle", "status", 0, "not-possible");
        return obs_pass();
    }

    int rc_0 = sceSysmoduleIsLoaded(0);
    obs_report_measure(id, "isloaded-0", "rc", (uint64_t)(uint32_t)rc_0, "code");

    int rc_7fff = sceSysmoduleIsLoaded(0x7fff);
    obs_report_measure(id, "isloaded-7fff", "rc", (uint64_t)(uint32_t)rc_7fff, "code");

    obs_report_measure(id, "probe-eboot", "imports-appcontent", 1, "bool");

    static const uint16_t mod_ids[] = {0x00b4, 0x000e, 0x0080, 0x0007, 0x001b,
                                       0x0096, 0x0008, 0x0015, 0x001c, 0x0024};
    for (size_t i = 0; i < sizeof(mod_ids) / sizeof(mod_ids[0]); i++) {
        uint16_t mid = mod_ids[i];
        char tag_pfx[32];
        snprintf(tag_pfx, sizeof(tag_pfx), "mod-0x%04x", mid);

        char tag[64];
        int rc1 = sceSysmoduleIsLoaded(mid);
        snprintf(tag, sizeof(tag), "%s-step1-isloaded-before", tag_pfx);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc1, "code");

        int rc2 = sceSysmoduleLoadModule(mid);
        snprintf(tag, sizeof(tag), "%s-step2-load", tag_pfx);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc2, "code");

        int rc3 = sceSysmoduleIsLoaded(mid);
        snprintf(tag, sizeof(tag), "%s-step3-isloaded-after-load", tag_pfx);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc3, "code");

        int rc4 = sceSysmoduleUnloadModule(mid);
        snprintf(tag, sizeof(tag), "%s-step4-unload", tag_pfx);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc4, "code");

        int rc5 = sceSysmoduleIsLoaded(mid);
        snprintf(tag, sizeof(tag), "%s-step5-isloaded-after-unload", tag_pfx);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc5, "code");
    }

    return obs_pass();
}

/* ---- REQ-20261012T1822Z-074b: libSceNetCtl callbacks and status ------------------ */
static volatile int s_netctl_cb_count = 0;
static volatile int s_netctl_last_event = -1;
static volatile void *s_netctl_last_arg = NULL;
static volatile uint64_t s_netctl_cb_tid = 0;

static void s_netctl_callback_fn(int eventType, void *arg) {
    s_netctl_cb_count++;
    s_netctl_last_event = eventType;
    s_netctl_last_arg = arg;
    if (obs_address_is_callable((const void *)&scePthreadSelf)) {
        s_netctl_cb_tid = (uint64_t)(uintptr_t)scePthreadSelf();
    }
}

static obs_result check_netctl_callbacks(void) {
    const char *id = "000-boot/netctl-callbacks";

    (void)obs_sysmodule_load_id(0x0001 /* libSceNet */);
    (void)obs_sysmodule_load_id(0x0002 /* libSceNetCtl */);

    typedef int (*fn_nc_init_t)(void);
    typedef int (*fn_nc_term_t)(void);
    typedef int (*fn_nc_check_cb_t)(void);
    typedef int (*fn_nc_reg_cb_t)(void (*)(int, void *), void *, int *);
    typedef int (*fn_nc_unreg_cb_t)(int);
    typedef int (*fn_nc_get_state_t)(int *);
    typedef int (*fn_nc_get_result_t)(int, int *);
    typedef int (*fn_nc_get_nat_t)(void *);

    fn_nc_init_t f_init = (fn_nc_init_t)&sceNetCtlInit;
    fn_nc_term_t f_term = (fn_nc_term_t)&sceNetCtlTerm;
    fn_nc_check_cb_t f_check = (fn_nc_check_cb_t)&sceNetCtlCheckCallback;
    fn_nc_reg_cb_t f_reg = (fn_nc_reg_cb_t)&sceNetCtlRegisterCallback;
    fn_nc_unreg_cb_t f_unreg = (fn_nc_unreg_cb_t)&sceNetCtlUnregisterCallback;
    fn_nc_get_state_t f_state = (fn_nc_get_state_t)&sceNetCtlGetState;
    fn_nc_get_result_t f_result = (fn_nc_get_result_t)&sceNetCtlGetResult;
    fn_nc_get_nat_t f_nat = (fn_nc_get_nat_t)&sceNetCtlGetNatInfo;

    int bound_reg = obs_address_is_callable((const void *)f_reg);
    obs_report_measure(id, "sceNetCtlRegisterCallback", "bound", (uint64_t)bound_reg,
                       "bool");
    if (!bound_reg) {
        obs_report_measure(id, "netctl-callbacks", "status", 0, "not-possible");
        return obs_pass();
    }

    int cid_pre = (int)0xccccccccU;
    int rc_chk_pre = f_check ? f_check() : -1;
    obs_report_measure(id, "pre-init-CheckCallback", "rc",
                       (uint64_t)(uint32_t)rc_chk_pre, "code");

    int rc_reg_pre = f_reg(s_netctl_callback_fn, 0, &cid_pre);
    obs_report_measure(id, "pre-init-RegisterCallback", "rc",
                       (uint64_t)(uint32_t)rc_reg_pre, "code");

    int state_pre = (int)0xccccccccU;
    int rc_st_pre = f_state ? f_state(&state_pre) : -1;
    obs_report_measure(id, "pre-init-GetState", "rc", (uint64_t)(uint32_t)rc_st_pre,
                       "code");
    obs_report_measure(id, "pre-init-GetState", "state", (uint64_t)(uint32_t)state_pre,
                       "code");

    int rc_init = f_init ? f_init() : -1;
    obs_report_measure(id, "sceNetCtlInit", "rc", (uint64_t)(uint32_t)rc_init, "code");

    int cids[9];
    for (int i = 0; i < 9; i++) {
        cids[i] = (int)0xccccccccU;
        char tag[64];
        snprintf(tag, sizeof(tag), "RegisterCallback-%d", i);
        int rc_r = f_reg(s_netctl_callback_fn, (void *)(uintptr_t)(i + 1), &cids[i]);
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_r, "code");
        obs_report_measure(id, tag, "cid", (uint64_t)(uint32_t)cids[i], "code");
        obs_report_bytes(id, tag, "cid-4b", 0, (const uint8_t *)&cids[i], 4);
    }
    int cid_dup = (int)0xccccccccU;
    int rc_dup = f_reg(s_netctl_callback_fn, (void *)0x42, &cid_dup);
    obs_report_measure(id, "RegisterCallback-dup-fn", "rc", (uint64_t)(uint32_t)rc_dup,
                       "code");
    int cid_nfn = (int)0xccccccccU;
    int rc_nfn = f_reg(NULL, 0, &cid_nfn);
    obs_report_measure(id, "RegisterCallback-null-fn", "rc", (uint64_t)(uint32_t)rc_nfn,
                       "code");
    int rc_ncid = f_reg(s_netctl_callback_fn, 0, NULL);
    obs_report_measure(id, "RegisterCallback-null-cid", "rc",
                       (uint64_t)(uint32_t)rc_ncid, "code");

    if (f_check) {
        for (int c = 0; c < 3; c++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "CheckCallback-%d", c + 1);
            int pre_cnt = s_netctl_cb_count;
            int rc_c = f_check();
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_c, "code");
            obs_report_measure(id, tag, "invocations",
                               (uint64_t)(s_netctl_cb_count - pre_cnt), "count");
            obs_report_measure(id, tag, "last-event",
                               (uint64_t)(uint32_t)s_netctl_last_event, "code");
            obs_report_measure(id, tag, "last-arg",
                               (uint64_t)(uintptr_t)s_netctl_last_arg, "addr");
            obs_report_measure(id, tag, "thread-id", s_netctl_cb_tid, "id");
        }
    }

    if (f_unreg) {
        int rc_un_live = f_unreg(cids[0]);
        obs_report_measure(id, "UnregisterCallback-live", "rc",
                           (uint64_t)(uint32_t)rc_un_live, "code");

        int rc_un_again = f_unreg(cids[0]);
        obs_report_measure(id, "UnregisterCallback-again", "rc",
                           (uint64_t)(uint32_t)rc_un_again, "code");

        int rc_un_8 = f_unreg(8);
        obs_report_measure(id, "UnregisterCallback-id-8", "rc",
                           (uint64_t)(uint32_t)rc_un_8, "code");

        int rc_un_neg = f_unreg(-1);
        obs_report_measure(id, "UnregisterCallback-id-minus1", "rc",
                           (uint64_t)(uint32_t)rc_un_neg, "code");
    }

    if (f_state) {
        int state_post = (int)0xccccccccU;
        int rc_st = f_state(&state_post);
        obs_report_measure(id, "GetState-post-init", "rc", (uint64_t)(uint32_t)rc_st,
                           "code");
        obs_report_measure(id, "GetState-post-init", "state",
                           (uint64_t)(uint32_t)state_post, "code");
        obs_report_bytes(id, "GetState-post-init", "state-4b", 0,
                         (const uint8_t *)&state_post, 4);

        int rc_st_null = f_state(NULL);
        obs_report_measure(id, "GetState-null", "rc", (uint64_t)(uint32_t)rc_st_null,
                           "code");
    }

    if (f_result) {
        for (int ev = 1; ev <= 3; ev++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "GetResult-event-%d", ev);
            int res_out = (int)0xccccccccU;
            int rc_r = f_result(ev, &res_out);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_r, "code");
            obs_report_measure(id, tag, "result", (uint64_t)(uint32_t)res_out, "code");
            obs_report_bytes(id, tag, "res-4b", 0, (const uint8_t *)&res_out, 4);
        }
        int rc_res_null = f_result(1, NULL);
        obs_report_measure(id, "GetResult-null", "rc", (uint64_t)(uint32_t)rc_res_null,
                           "code");
    }

    if (f_nat) {
        uint8_t nat_buf[0x20];
        memset(nat_buf, 0xcc, sizeof(nat_buf));
        int rc_nat = f_nat(nat_buf);
        obs_report_measure(id, "GetNatInfo", "rc", (uint64_t)(uint32_t)rc_nat, "code");
        obs_report_bytes(id, "GetNatInfo", "buf-20h", 0, nat_buf, sizeof(nat_buf));
    }

    if (f_term) {
        (void)f_term();
    }

    return obs_pass();
}

/* ---- REQ-20261012T1816Z-c901: UDP loopback sendto / recvfrom ----------------------
 */
struct obs_sockaddr_in {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

static obs_result check_udp_loopback(void) {
    const char *id = "000-boot/udp-loopback";

    (void)obs_sysmodule_load_id(0x0001 /* libSceNet */);

    if (!obs_address_is_callable((const void *)&sceNetSocket) ||
        !obs_address_is_callable((const void *)&sceNetBind) ||
        !obs_address_is_callable((const void *)&sceNetSendto) ||
        !obs_address_is_callable((const void *)&sceNetRecvfrom) ||
        !obs_address_is_callable((const void *)&sceNetGetsockname)) {
        obs_report_measure(id, "udp-loopback", "status", 0, "not-possible");
        return obs_pass();
    }

    int *(*fn_errno)(void) = (int *(*)(void)) & sceNetErrnoLoc;

    int sock_a = sceNetSocket("sockA", 2 /* AF_INET */, 2 /* SOCK_DGRAM */, 0);
    int sock_b = sceNetSocket("sockB", 2 /* AF_INET */, 2 /* SOCK_DGRAM */, 0);
    obs_report_measure(id, "socket-A", "fd", (uint64_t)sock_a, "fd");
    obs_report_measure(id, "socket-B", "fd", (uint64_t)sock_b, "fd");

    if (sock_a >= 0 && sock_b >= 0) {
        struct obs_sockaddr_in bind_addr;
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_len = sizeof(bind_addr);
        bind_addr.sin_family = 2;        /* AF_INET */
        bind_addr.sin_port = 0;          /* ephemeral */
        bind_addr.sin_addr = 0x0100007f; /* 127.0.0.1 */

        int rc_bind = sceNetBind(sock_b, &bind_addr, sizeof(bind_addr));
        obs_report_measure(id, "bind-B", "rc", (uint64_t)(uint32_t)rc_bind, "code");

        struct obs_sockaddr_in bound_addr_b;
        memset(&bound_addr_b, 0, sizeof(bound_addr_b));
        uint32_t namelen = sizeof(bound_addr_b);
        int rc_gsn = sceNetGetsockname(sock_b, &bound_addr_b, &namelen);
        obs_report_measure(id, "getsockname-B", "rc", (uint64_t)(uint32_t)rc_gsn,
                           "code");
        obs_report_measure(id, "getsockname-B", "port", (uint64_t)bound_addr_b.sin_port,
                           "code");

        const char *dgram_text = "obscene-dgram";
        int rc_send = sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b,
                                   sizeof(bound_addr_b));
        obs_report_measure(id, "sendto-A-to-B", "rc", (uint64_t)(uint32_t)rc_send,
                           "code");

        uint8_t buf64[64];
        memset(buf64, 0xa5, sizeof(buf64));
        uint8_t from32[32];
        memset(from32, 0xa5, sizeof(from32));
        uint32_t fromlen = 32;
        int rc_recv = sceNetRecvfrom(sock_b, buf64, 64, 0, from32, &fromlen);
        obs_report_measure(id, "recvfrom-B", "rc", (uint64_t)(uint32_t)rc_recv, "code");
        obs_report_bytes(id, "recvfrom-B", "buf-64b", 0, buf64, sizeof(buf64));
        obs_report_bytes(id, "recvfrom-B", "from-32b", 0, from32, sizeof(from32));
        obs_report_measure(id, "recvfrom-B", "fromlen-after", (uint64_t)fromlen,
                           "bytes");

        (void)sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b,
                           sizeof(bound_addr_b));
        int rc_null_from = sceNetRecvfrom(sock_b, buf64, 64, 0, NULL, NULL);
        obs_report_measure(id, "recvfrom-null-from", "rc",
                           (uint64_t)(uint32_t)rc_null_from, "code");

        (void)sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b,
                           sizeof(bound_addr_b));
        memset(from32, 0xa5, sizeof(from32));
        uint32_t fromlen4 = 4;
        int rc_fromlen4 = sceNetRecvfrom(sock_b, buf64, 64, 0, from32, &fromlen4);
        obs_report_measure(id, "recvfrom-fromlen-4", "rc",
                           (uint64_t)(uint32_t)rc_fromlen4, "code");
        obs_report_measure(id, "recvfrom-fromlen-4", "fromlen-after",
                           (uint64_t)fromlen4, "bytes");
        obs_report_bytes(id, "recvfrom-fromlen-4", "from-32b", 0, from32,
                         sizeof(from32));

        int rc_empty_nb =
            sceNetRecvfrom(sock_b, buf64, 64, 0x80 /* MSG_DONTWAIT */, NULL, NULL);
        obs_report_measure(id, "recvfrom-empty-nb", "rc",
                           (uint64_t)(uint32_t)rc_empty_nb, "code");
        if (fn_errno && fn_errno()) {
            obs_report_measure(id, "recvfrom-empty-nb", "errno", (uint64_t)*fn_errno(),
                               "code");
        }

        (void)sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b,
                           sizeof(bound_addr_b));
        memset(buf64, 0xa5, sizeof(buf64));
        int rc_trunc = sceNetRecvfrom(sock_b, buf64, 4, 0, NULL, NULL);
        obs_report_measure(id, "recvfrom-trunc-4", "rc", (uint64_t)(uint32_t)rc_trunc,
                           "code");
        obs_report_bytes(id, "recvfrom-trunc-4", "buf-4b", 0, buf64, 4);

        int rc_trunc_next =
            sceNetRecvfrom(sock_b, buf64, 64, 0x80 /* MSG_DONTWAIT */, NULL, NULL);
        obs_report_measure(id, "recvfrom-after-trunc", "rc",
                           (uint64_t)(uint32_t)rc_trunc_next, "code");

        int rc_tolen0 = sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b, 0);
        obs_report_measure(id, "sendto-tolen-0", "rc", (uint64_t)(uint32_t)rc_tolen0,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-tolen-0", "errno", (uint64_t)*fn_errno(),
                               "code");

        int rc_tolen8 = sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b, 8);
        obs_report_measure(id, "sendto-tolen-8", "rc", (uint64_t)(uint32_t)rc_tolen8,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-tolen-8", "errno", (uint64_t)*fn_errno(),
                               "code");

        int rc_tonull = sceNetSendto(sock_a, dgram_text, 13, 0, NULL, 0);
        obs_report_measure(id, "sendto-to-null", "rc", (uint64_t)(uint32_t)rc_tonull,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-to-null", "errno", (uint64_t)*fn_errno(),
                               "code");

        struct obs_sockaddr_in testnet;
        memset(&testnet, 0, sizeof(testnet));
        testnet.sin_len = sizeof(testnet);
        testnet.sin_family = 2;
        testnet.sin_port = 0x0900;
        testnet.sin_addr = 0x010200c0; /* 192.0.2.1 */
        int rc_testnet =
            sceNetSendto(sock_a, dgram_text, 13, 0, &testnet, sizeof(testnet));
        obs_report_measure(id, "sendto-testnet", "rc", (uint64_t)(uint32_t)rc_testnet,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-testnet", "errno", (uint64_t)*fn_errno(),
                               "code");

        int rc_0byte =
            sceNetSendto(sock_a, dgram_text, 0, 0, &bound_addr_b, sizeof(bound_addr_b));
        obs_report_measure(id, "sendto-0-byte", "rc", (uint64_t)(uint32_t)rc_0byte,
                           "code");

        int rc_bad_send =
            sceNetSendto(9999, dgram_text, 13, 0, &bound_addr_b, sizeof(bound_addr_b));
        obs_report_measure(id, "sendto-bad-fd", "rc", (uint64_t)(uint32_t)rc_bad_send,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-bad-fd", "errno", (uint64_t)*fn_errno(),
                               "code");

        int rc_bad_recv = sceNetRecvfrom(9999, buf64, 64, 0, NULL, NULL);
        obs_report_measure(id, "recvfrom-bad-fd", "rc", (uint64_t)(uint32_t)rc_bad_recv,
                           "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "recvfrom-bad-fd", "errno", (uint64_t)*fn_errno(),
                               "code");

        int stream_sock =
            sceNetSocket("streamS", 2 /* AF_INET */, 1 /* SOCK_STREAM */, 0);
        if (stream_sock >= 0) {
            int rc_str_send = sceNetSendto(stream_sock, dgram_text, 13, 0,
                                           &bound_addr_b, sizeof(bound_addr_b));
            obs_report_measure(id, "sendto-unconnected-stream", "rc",
                               (uint64_t)(uint32_t)rc_str_send, "code");
            if (fn_errno && fn_errno())
                obs_report_measure(id, "sendto-unconnected-stream", "errno",
                                   (uint64_t)*fn_errno(), "code");

            int rc_str_recv = sceNetRecvfrom(stream_sock, buf64, 64, 0, NULL, NULL);
            obs_report_measure(id, "recvfrom-unconnected-stream", "rc",
                               (uint64_t)(uint32_t)rc_str_recv, "code");
            if (fn_errno && fn_errno())
                obs_report_measure(id, "recvfrom-unconnected-stream", "errno",
                                   (uint64_t)*fn_errno(), "code");
            sceNetSocketClose(stream_sock);
        }

        sceNetSocketClose(sock_a);
        sceNetSocketClose(sock_b);

        int rc_closed_send = sceNetSendto(sock_a, dgram_text, 13, 0, &bound_addr_b,
                                          sizeof(bound_addr_b));
        obs_report_measure(id, "sendto-closed-fd", "rc",
                           (uint64_t)(uint32_t)rc_closed_send, "code");
        if (fn_errno && fn_errno())
            obs_report_measure(id, "sendto-closed-fd", "errno", (uint64_t)*fn_errno(),
                               "code");
    }

    return obs_pass();
}

/* ---- REQ-20261012T1816Z-50c8: Net Resolver ----------------------------------------
 */
static obs_result check_net_resolver(void) {
    const char *id = "000-boot/net-resolver";

    (void)obs_sysmodule_load_id(0x0001 /* libSceNet */);

    int h_net = obs_module_open("libSceNet");
    if (h_net < 0) {
        obs_report_measure(id, "net-resolver", "status", 0, "not-possible");
        return obs_pass();
    }

    typedef int (*fn_res_create_t)(const char *name, int pool, int flags);
    typedef int (*fn_res_destroy_t)(int rid);
    typedef int (*fn_res_start_ntoa_t)(int rid, const char *hostname, void *addr,
                                       int timeout, int retry, int flags);
    typedef int (*fn_pool_create_t)(const char *name, int size, int flags);
    typedef int (*fn_pool_destroy_t)(int pool);
    typedef int *(*fn_errno_loc_t)(void);

    fn_res_create_t fn_res_create =
        obs_address_is_callable((const void *)&sceNetResolverCreate_static)
            ? (fn_res_create_t)(void *)&sceNetResolverCreate_static
            : (fn_res_create_t)obs_module_symbol(h_net, "sceNetResolverCreate");
    int how_create =
        (fn_res_create == (fn_res_create_t)(void *)&sceNetResolverCreate_static)
            ? 1
            : (fn_res_create ? 2 : 0);

    fn_res_destroy_t fn_res_destroy =
        obs_address_is_callable((const void *)&sceNetResolverDestroy_static)
            ? (fn_res_destroy_t)(void *)&sceNetResolverDestroy_static
            : (fn_res_destroy_t)obs_module_symbol(h_net, "sceNetResolverDestroy");
    int how_destroy =
        (fn_res_destroy == (fn_res_destroy_t)(void *)&sceNetResolverDestroy_static)
            ? 1
            : (fn_res_destroy ? 2 : 0);

    fn_res_start_ntoa_t fn_start_ntoa =
        obs_address_is_callable((const void *)&sceNetResolverStartNtoa_static)
            ? (fn_res_start_ntoa_t)(void *)&sceNetResolverStartNtoa_static
            : (fn_res_start_ntoa_t)obs_module_symbol(h_net, "sceNetResolverStartNtoa");
    int how_start_ntoa =
        (fn_start_ntoa == (fn_res_start_ntoa_t)(void *)&sceNetResolverStartNtoa_static)
            ? 1
            : (fn_start_ntoa ? 2 : 0);

    fn_pool_create_t fn_pool_create =
        (fn_pool_create_t)obs_module_symbol(h_net, "sceNetPoolCreate");
    fn_pool_destroy_t fn_pool_destroy =
        (fn_pool_destroy_t)obs_module_symbol(h_net, "sceNetPoolDestroy");
    fn_errno_loc_t fn_errno =
        (fn_errno_loc_t)obs_module_symbol(h_net, "sceNetErrnoLoc");

    obs_report_measure(id, "sceNetResolverCreate", "resolved",
                       (uint64_t)(fn_res_create != NULL), "bool");
    obs_report_measure(id, "sceNetResolverCreate", "how", (uint64_t)how_create,
                       how_create == 1 ? "import"
                                       : (how_create == 2 ? "dlsym" : "not-possible"));
    obs_report_measure(id, "sceNetResolverDestroy", "resolved",
                       (uint64_t)(fn_res_destroy != NULL), "bool");
    obs_report_measure(
        id, "sceNetResolverDestroy", "how", (uint64_t)how_destroy,
        how_destroy == 1 ? "import" : (how_destroy == 2 ? "dlsym" : "not-possible"));
    obs_report_measure(id, "sceNetResolverStartNtoa", "resolved",
                       (uint64_t)(fn_start_ntoa != NULL), "bool");
    obs_report_measure(id, "sceNetResolverStartNtoa", "how", (uint64_t)how_start_ntoa,
                       how_start_ntoa == 1
                           ? "import"
                           : (how_start_ntoa == 2 ? "dlsym" : "not-possible"));

    if (!fn_res_create || !fn_res_destroy || !fn_start_ntoa) {
        obs_report_measure(id, "net-resolver", "status", 0, "not-possible");
        return obs_pass();
    }

    /* 1. sceNetResolverCreate("obs-r1", pool, 0) twice: both ids (pool = fresh
     * sceNetPoolCreate id; also once with pool 0) */
    int pool = -1;
    if (fn_pool_create) {
        pool = fn_pool_create("obs-res-pool", 16384, 0);
        obs_report_measure(id, "pool-create", "id", (uint64_t)(uint32_t)pool, "id");
    }

    int rid1 = fn_res_create("obs-r1", pool, 0);
    obs_report_measure(id, "create-1-pool", "id", (uint64_t)(uint32_t)rid1, "id");
    if (rid1 < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "create-1-pool", "errno", (uint64_t)*fn_errno(), "code");
    }

    int rid2 = fn_res_create("obs-r2", pool, 0);
    obs_report_measure(id, "create-2-pool", "id", (uint64_t)(uint32_t)rid2, "id");
    if (rid2 < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "create-2-pool", "errno", (uint64_t)*fn_errno(), "code");
    }

    int rid_p0 = fn_res_create("obs-r0", 0, 0);
    obs_report_measure(id, "create-pool-0", "id", (uint64_t)(uint32_t)rid_p0, "id");
    if (rid_p0 < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "create-pool-0", "errno", (uint64_t)*fn_errno(), "code");
    }

    /* 2. Create with flags 1; with name NULL */
    int rc_flags1 = fn_res_create("obs-flags1", pool, 1);
    obs_report_measure(id, "create-flags-1", "rc", (uint64_t)(uint32_t)rc_flags1,
                       "code");
    if (rc_flags1 < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "create-flags-1", "errno", (uint64_t)*fn_errno(),
                           "code");
    }

    int rc_namenull = fn_res_create(NULL, pool, 0);
    obs_report_measure(id, "create-name-null", "rc", (uint64_t)(uint32_t)rc_namenull,
                       "code");
    if (rc_namenull < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "create-name-null", "errno", (uint64_t)*fn_errno(),
                           "code");
    }

    /* 3. StartNtoa(rid, host, addr, timeout, retry, flags), addr a 16-byte 0xa5 buffer
     */
    int live_rid = (rid1 >= 0) ? rid1 : ((rid2 >= 0) ? rid2 : rid_p0);
    if (live_rid >= 0) {
        /* a. "nonexistent.invalid", timeout 1000000, retry 0 */
        uint8_t addr16[16];
        memset(addr16, 0xa5, sizeof(addr16));
        uint64_t t0 = sceKernelGetProcessTime();
        int rc_badhost =
            fn_start_ntoa(live_rid, "nonexistent.invalid", addr16, 1000000, 0, 0);
        uint64_t t1 = sceKernelGetProcessTime();
        obs_report_measure(id, "start-ntoa-nonexistent", "rc",
                           (uint64_t)(uint32_t)rc_badhost, "code");
        obs_report_measure(id, "start-ntoa-nonexistent", "elapsed-us",
                           (t1 >= t0 ? (t1 - t0) : 0), "us");
        obs_report_bytes(id, "start-ntoa-nonexistent", "addr16", 0, addr16, 16);
        if (rc_badhost < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-nonexistent", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        /* b. "127.0.0.1" (short-circuit?) */
        memset(addr16, 0xa5, sizeof(addr16));
        t0 = sceKernelGetProcessTime();
        int rc_loop = fn_start_ntoa(live_rid, "127.0.0.1", addr16, 1000000, 0, 0);
        t1 = sceKernelGetProcessTime();
        obs_report_measure(id, "start-ntoa-loopback-literal", "rc",
                           (uint64_t)(uint32_t)rc_loop, "code");
        obs_report_measure(id, "start-ntoa-loopback-literal", "elapsed-us",
                           (t1 >= t0 ? (t1 - t0) : 0), "us");
        obs_report_bytes(id, "start-ntoa-loopback-literal", "addr16", 0, addr16, 16);
        if (rc_loop < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-loopback-literal", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        /* c. "localhost" */
        memset(addr16, 0xa5, sizeof(addr16));
        t0 = sceKernelGetProcessTime();
        int rc_local = fn_start_ntoa(live_rid, "localhost", addr16, 1000000, 0, 0);
        t1 = sceKernelGetProcessTime();
        obs_report_measure(id, "start-ntoa-localhost", "rc",
                           (uint64_t)(uint32_t)rc_local, "code");
        obs_report_measure(id, "start-ntoa-localhost", "elapsed-us",
                           (t1 >= t0 ? (t1 - t0) : 0), "us");
        obs_report_bytes(id, "start-ntoa-localhost", "addr16", 0, addr16, 16);
        if (rc_local < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-localhost", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        /* d. timeout -1; retry -1; flags 2; flags 0x10000 */
        int rc_tmo_neg = fn_start_ntoa(live_rid, "127.0.0.1", addr16, -1, 0, 0);
        obs_report_measure(id, "start-ntoa-timeout-neg1", "rc",
                           (uint64_t)(uint32_t)rc_tmo_neg, "code");
        if (rc_tmo_neg < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-timeout-neg1", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        int rc_ret_neg = fn_start_ntoa(live_rid, "127.0.0.1", addr16, 1000000, -1, 0);
        obs_report_measure(id, "start-ntoa-retry-neg1", "rc",
                           (uint64_t)(uint32_t)rc_ret_neg, "code");
        if (rc_ret_neg < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-retry-neg1", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        int rc_flg2 = fn_start_ntoa(live_rid, "127.0.0.1", addr16, 1000000, 0, 2);
        obs_report_measure(id, "start-ntoa-flags-2", "rc", (uint64_t)(uint32_t)rc_flg2,
                           "code");
        if (rc_flg2 < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-flags-2", "errno", (uint64_t)*fn_errno(),
                               "code");
        }

        int rc_flg10000 =
            fn_start_ntoa(live_rid, "127.0.0.1", addr16, 1000000, 0, 0x10000);
        obs_report_measure(id, "start-ntoa-flags-0x10000", "rc",
                           (uint64_t)(uint32_t)rc_flg10000, "code");
        if (rc_flg10000 < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-flags-0x10000", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        /* e. host NULL; addr NULL; rid 9999 */
        int rc_host_null = fn_start_ntoa(live_rid, NULL, addr16, 1000000, 0, 0);
        obs_report_measure(id, "start-ntoa-host-null", "rc",
                           (uint64_t)(uint32_t)rc_host_null, "code");
        if (rc_host_null < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-host-null", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        int rc_addr_null = fn_start_ntoa(live_rid, "127.0.0.1", NULL, 1000000, 0, 0);
        obs_report_measure(id, "start-ntoa-addr-null", "rc",
                           (uint64_t)(uint32_t)rc_addr_null, "code");
        if (rc_addr_null < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-addr-null", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        int rc_bad_rid = fn_start_ntoa(9999, "127.0.0.1", addr16, 1000000, 0, 0);
        obs_report_measure(id, "start-ntoa-bad-rid", "rc",
                           (uint64_t)(uint32_t)rc_bad_rid, "code");
        if (rc_bad_rid < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-bad-rid", "errno", (uint64_t)*fn_errno(),
                               "code");
        }

        /* f. real name query (optional arm) */
        memset(addr16, 0xa5, sizeof(addr16));
        int rc_real = fn_start_ntoa(live_rid, "playstation.com", addr16, 2000000, 0, 0);
        obs_report_measure(id, "start-ntoa-real-name", "rc",
                           (uint64_t)(uint32_t)rc_real, "code");
        obs_report_bytes(id, "start-ntoa-real-name", "addr16", 0, addr16, 16);
        if (rc_real < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-real-name", "errno",
                               (uint64_t)*fn_errno(), "code");
        }
    }

    /* 4. sceNetResolverDestroy on live id, same id again, and 9999 */
    if (rid1 >= 0) {
        int rc_des1 = fn_res_destroy(rid1);
        obs_report_measure(id, "destroy-live", "rc", (uint64_t)(uint32_t)rc_des1,
                           "code");
        if (rc_des1 < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "destroy-live", "errno", (uint64_t)*fn_errno(),
                               "code");
        }

        /* rid after Destroy (arm 3e continuation) */
        uint8_t addr16[16];
        memset(addr16, 0xa5, sizeof(addr16));
        int rc_after_des = fn_start_ntoa(rid1, "127.0.0.1", addr16, 1000000, 0, 0);
        obs_report_measure(id, "start-ntoa-after-destroy", "rc",
                           (uint64_t)(uint32_t)rc_after_des, "code");
        if (rc_after_des < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "start-ntoa-after-destroy", "errno",
                               (uint64_t)*fn_errno(), "code");
        }

        int rc_des_again = fn_res_destroy(rid1);
        obs_report_measure(id, "destroy-again", "rc", (uint64_t)(uint32_t)rc_des_again,
                           "code");
        if (rc_des_again < 0 && fn_errno && fn_errno()) {
            obs_report_measure(id, "destroy-again", "errno", (uint64_t)*fn_errno(),
                               "code");
        }
    }

    int rc_des_9999 = fn_res_destroy(9999);
    obs_report_measure(id, "destroy-9999", "rc", (uint64_t)(uint32_t)rc_des_9999,
                       "code");
    if (rc_des_9999 < 0 && fn_errno && fn_errno()) {
        obs_report_measure(id, "destroy-9999", "errno", (uint64_t)*fn_errno(), "code");
    }

    if (rid2 >= 0) {
        fn_res_destroy(rid2);
    }
    if (rid_p0 >= 0) {
        fn_res_destroy(rid_p0);
    }
    if (pool >= 0 && fn_pool_destroy) {
        fn_pool_destroy(pool);
    }

    return obs_pass();
}

/* ---- REQ-20261012T1822Z-1338: NpGameIntent lifecycle ------------------------------
 */
static obs_result check_np_game_intent(void) {
    const char *id = "000-boot/np-game-intent";
    obs_jmp_buf guard;

    (void)obs_sysmodule_load_id(0x011c /* libSceNpGameIntent */);
    int h_gi = obs_module_open("libSceNpGameIntent");
    if (h_gi < 0) {
        obs_report_measure(id, "np-game-intent", "status", 0, "not-possible");
        return obs_pass();
    }

    typedef int (*fn_gi_init_t)(const void *param);
    typedef int (*fn_gi_term_t)(void);
    typedef int (*fn_gi_recv_t)(void *intent);
    typedef int (*fn_gi_get_prop_str_t)(const void *data, const char *key, char *buf,
                                        size_t bufSize);

    const char *how = "import";
    fn_gi_init_t fn_init =
        (fn_gi_init_t)(const void *)&sceNpGameIntentInitialize_static;
    fn_gi_term_t fn_term = (fn_gi_term_t)(const void *)&sceNpGameIntentTerminate_static;
    fn_gi_recv_t fn_recv =
        (fn_gi_recv_t)(const void *)&sceNpGameIntentReceiveIntent_static;
    fn_gi_get_prop_str_t fn_get_prop_str =
        (fn_gi_get_prop_str_t)(const void
                                   *)&sceNpGameIntentGetPropertyValueString_static;

    if (!obs_address_is_callable((const void *)fn_init)) {
        fn_init = (fn_gi_init_t)obs_module_symbol(h_gi, "sceNpGameIntentInitialize");
        how = "lib";
    }
    if (!obs_address_is_callable((const void *)fn_term)) {
        fn_term = (fn_gi_term_t)obs_module_symbol(h_gi, "sceNpGameIntentTerminate");
    }
    if (!obs_address_is_callable((const void *)fn_recv)) {
        fn_recv = (fn_gi_recv_t)obs_module_symbol(h_gi, "sceNpGameIntentReceiveIntent");
    }
    if (!obs_address_is_callable((const void *)fn_get_prop_str)) {
        fn_get_prop_str = (fn_gi_get_prop_str_t)obs_module_symbol(
            h_gi, "sceNpGameIntentGetPropertyValueString");
    }

    obs_report_measure(id, "sceNpGameIntentInitialize", "resolved",
                       (uint64_t)obs_address_is_callable((const void *)fn_init),
                       "bool");
    obs_report_measure(id, "sceNpGameIntentTerminate", "resolved",
                       (uint64_t)obs_address_is_callable((const void *)fn_term),
                       "bool");
    obs_report_measure(id, "sceNpGameIntentReceiveIntent", "resolved",
                       (uint64_t)obs_address_is_callable((const void *)fn_recv),
                       "bool");
    obs_report_measure(id, "sceNpGameIntentGetPropertyValueString", "resolved",
                       (uint64_t)obs_address_is_callable((const void *)fn_get_prop_str),
                       "bool");
    if (obs_address_is_callable((const void *)fn_init)) {
        obs_report_measure(id, "sceNpGameIntentInitialize", "how",
                           obs_strcmp(how, "import") == 0 ? 1 : 2, how);
    }

    if (!fn_recv || !fn_term || !fn_init) {
        obs_report_measure(id, "np-game-intent", "status", 0, "not-possible");
        return obs_pass();
    }

    /* 1. Before Initialize: ReceiveIntent(&intent) with intent 0x4040 filled 0xcc */
    static uint8_t intent_buf[0x4040];
    memset(intent_buf, 0xcc, sizeof(intent_buf));

    int sig = OBS_FAULT_ARM(&guard);
    int rc_recv_before = -1;
    if (sig == 0) {
        rc_recv_before = fn_recv(intent_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "recv-before-init", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "recv-before-init", "rc", (uint64_t)(uint32_t)rc_recv_before,
                       "code");

    size_t changed_before = 0;
    for (size_t i = 0; i < sizeof(intent_buf); i++) {
        if (intent_buf[i] != 0xcc)
            changed_before++;
    }
    obs_report_measure(id, "recv-before-init", "changed-bytes",
                       (uint64_t)changed_before, "bytes");
    for (uint32_t off = 0; off < 0x140; off += 64) {
        obs_report_bytes(id, "recv-before-init", "intent-dump", off, intent_buf + off,
                         64);
    }

    int rc_term_before = fn_term();
    obs_report_measure(id, "term-before-init", "rc", (uint64_t)(uint32_t)rc_term_before,
                       "code");

    /* 2. After Initialize(...) */
    uint8_t init_param[0x28];
    memset(init_param, 0, sizeof(init_param));
    *(uint64_t *)init_param = 0x28;
    int rc_init = fn_init(init_param);
    obs_report_measure(id, "initialize", "rc", (uint64_t)(uint32_t)rc_init, "code");

    memset(intent_buf, 0xcc, sizeof(intent_buf));
    sig = OBS_FAULT_ARM(&guard);
    int rc_recv_after = -1;
    if (sig == 0) {
        rc_recv_after = fn_recv(intent_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "recv-after-init", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "recv-after-init", "rc", (uint64_t)(uint32_t)rc_recv_after,
                       "code");

    size_t changed_after = 0;
    for (size_t i = 0; i < sizeof(intent_buf); i++) {
        if (intent_buf[i] != 0xcc)
            changed_after++;
    }
    obs_report_measure(id, "recv-after-init", "changed-bytes", (uint64_t)changed_after,
                       "bytes");
    for (uint32_t off = 0; off < 0x140; off += 64) {
        obs_report_bytes(id, "recv-after-init", "intent-dump", off, intent_buf + off,
                         64);
    }

    if (fn_get_prop_str) {
        char val_buf[0x40];
        memset(val_buf, 0xcc, sizeof(val_buf));
        int rc_prop =
            fn_get_prop_str(intent_buf + 0x134, "activityId", val_buf, sizeof(val_buf));
        obs_report_measure(id, "get-prop-activityId", "rc", (uint64_t)(uint32_t)rc_prop,
                           "code");
        obs_report_bytes(id, "get-prop-activityId", "buf", 0, (const uint8_t *)val_buf,
                         sizeof(val_buf));

        int rc_prop_sz0 = fn_get_prop_str(intent_buf + 0x134, "activityId", val_buf, 0);
        obs_report_measure(id, "get-prop-bufsize-0", "rc",
                           (uint64_t)(uint32_t)rc_prop_sz0, "code");

        int rc_prop_nullkey =
            fn_get_prop_str(intent_buf + 0x134, NULL, val_buf, sizeof(val_buf));
        obs_report_measure(id, "get-prop-null-key", "rc",
                           (uint64_t)(uint32_t)rc_prop_nullkey, "code");
    }

    /* 3. Terminate() twice - rc; ReceiveIntent after Terminate - rc */
    int rc_term1 = fn_term();
    obs_report_measure(id, "term-1", "rc", (uint64_t)(uint32_t)rc_term1, "code");

    int rc_term2 = fn_term();
    obs_report_measure(id, "term-2", "rc", (uint64_t)(uint32_t)rc_term2, "code");

    int rc_recv_post = fn_recv(intent_buf);
    obs_report_measure(id, "recv-post-term", "rc", (uint64_t)(uint32_t)rc_recv_post,
                       "code");

    return obs_pass();
}

/* ---- REQ-20261012T1816Z-ffa8: Http URI parse --------------------------------------
 */
static obs_result check_http_uri_parse(void) {
    const char *id = "000-boot/http-uri-parse";
    obs_jmp_buf guard;
    int sig;

    (void)obs_sysmodule_load_id(0x0001);
    (void)obs_sysmodule_load_id(0x001a /* libSceHttp */);
    int h_http = obs_module_open("libSceHttp");
    if (h_http < 0)
        h_http = obs_module_open("libSceHttp.sprx");

    const char *how = "import";
    const void *fn_parse = (const void *)&sceHttpUriParse_static;
    if (!obs_address_is_callable(fn_parse)) {
        fn_parse = (const void *)&sceHttpUriParse;
    }
    if (!obs_address_is_callable(fn_parse)) {
        fn_parse = obs_module_symbol(OBS_HANDLE_SELF, "sceHttpUriParse");
        how = "self";
    }
    if (!obs_address_is_callable(fn_parse)) {
        fn_parse = obs_module_symbol(OBS_HANDLE_SELF, "$W7jgfwKlZiE");
        how = "self";
    }
    if (!obs_address_is_callable(fn_parse) && h_http > 0) {
        fn_parse = obs_module_symbol(h_http, "sceHttpUriParse");
        how = "lib";
        if (!fn_parse)
            fn_parse = obs_module_symbol(h_http, "$W7jgfwKlZiE");
    }
    if (!fn_parse && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$W7jgfwKlZiE", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_parse = p;
                how = "dlsym";
                break;
            }
            if (sceKernelDlsym(h, "sceHttpUriParse", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_parse = p;
                how = "dlsym";
                break;
            }
        }
    }

    obs_report_measure(id, "sceHttpUriParse", "bound",
                       (uint64_t)(fn_parse && obs_address_is_callable(fn_parse)),
                       "bool");
    if (fn_parse && obs_address_is_callable(fn_parse)) {
        obs_report_measure(id, "sceHttpUriParse", "how",
                           obs_strcmp(how, "import") == 0 ? 1 : 2, how);
    }

    typedef int (*fn_uri_parse_t)(void *, const char *, void *, uint64_t *, size_t);
    fn_uri_parse_t f_parse = (fn_uri_parse_t)fn_parse;

    if (!f_parse || !obs_address_is_callable((const void *)f_parse)) {
        obs_report_measure(id, "sceHttpUriParse", "status", 0, "not-possible");
        return obs_pass();
    }

    static const char *const uris[] = {"https://user:pw@example.com:8443/a/b?x=1#frag",
                                       "http://host/",
                                       "https://host",
                                       "https://[::1]:8080/p",
                                       "mailto:x@y.z",
                                       "/relative/path?q",
                                       "http://host:99999/",
                                       "http://host:/",
                                       "",
                                       "ht tp://host/"};

    uint8_t out[96];
    uint8_t pool[512];
    uint64_t req_1 = 0;

    for (size_t u = 0; u < sizeof(uris) / sizeof(uris[0]); u++) {
        char tag[48];
        snprintf(tag, sizeof(tag), "uri-%zu", u + 1);
        memset(out, 0xa5, sizeof(out));
        memset(pool, 0xa5, sizeof(pool));
        uint64_t required = 0xa5a5a5a5a5a5a5a5ULL;
        int rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = f_parse(out, uris[u], pool, &required, sizeof(pool));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, tag, "required", required, "bytes");
        obs_report_bytes(id, tag, "out-96b", 0, out, sizeof(out));
        obs_report_bytes(id, tag, "pool-64b", 0, pool, 64);
        if (u == 0)
            req_1 = required;
    }

    uint64_t req_query = 0xa5a5a5a5a5a5a5a5ULL;
    int rc_11 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_11 = f_parse(NULL, uris[0], NULL, &req_query, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "arm11-size-query", "rc", (uint64_t)(uint32_t)rc_11, "code");
    obs_report_measure(id, "arm11-size-query", "required", req_query, "bytes");

    int rc_12 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_12 = f_parse(NULL, uris[0], NULL, NULL, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "arm12-null-all", "rc", (uint64_t)(uint32_t)rc_12, "code");

    if (req_1 > 0 && req_1 <= sizeof(pool)) {
        memset(out, 0xa5, sizeof(out));
        memset(pool, 0xa5, sizeof(pool));
        uint64_t req13 = 0xa5a5a5a5a5a5a5a5ULL;
        int rc_13 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_13 = f_parse(out, uris[0], pool, &req13, req_1 - 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "arm13-pool-small", "rc", (uint64_t)(uint32_t)rc_13,
                           "code");
        obs_report_measure(id, "arm13-pool-small", "required", req13, "bytes");
    }

    if (req_1 > 0 && req_1 <= sizeof(pool)) {
        memset(out, 0xa5, sizeof(out));
        memset(pool, 0xa5, sizeof(pool));
        uint64_t req14 = 0xa5a5a5a5a5a5a5a5ULL;
        int rc_14 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_14 = f_parse(out, uris[0], pool, &req14, req_1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "arm14-pool-exact", "rc", (uint64_t)(uint32_t)rc_14,
                           "code");
        obs_report_measure(id, "arm14-pool-exact", "required", req14, "bytes");
    }

    memset(out, 0xa5, sizeof(out));
    memset(pool, 0xa5, sizeof(pool));
    uint64_t req15 = 0xa5a5a5a5a5a5a5a5ULL;
    int rc_15 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_15 = f_parse(out, NULL, pool, &req15, sizeof(pool));
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "arm15-null-uri", "rc", (uint64_t)(uint32_t)rc_15, "code");

    return obs_pass();
}

/* ---- REQ-20261012T1822Z-0028: CesCs and Psml --------------------------------------
 */
static obs_result check_cescs_psml(void) {
    const char *id = "000-boot/cescs-psml";
    obs_jmp_buf guard;
    int sig;

    int h_ces = obs_module_open("libSceCes");
    if (h_ces < 0) {
        (void)obs_sysmodule_load_id(0x0006);
        h_ces = obs_module_open("libSceCes");
    }

    const void *fn_profile = NULL;
    if (obs_address_is_callable((const void *)&sceCesRefersUcsProfileCp1252_static)) {
        fn_profile = (const void *)&sceCesRefersUcsProfileCp1252_static;
    }
    if (!fn_profile &&
        obs_address_is_callable((const void *)&sceCesRefersUcsProfileCp1252)) {
        fn_profile = (const void *)&sceCesRefersUcsProfileCp1252;
    }
    if (!fn_profile && h_ces >= 0) {
        fn_profile = obs_module_symbol(h_ces, "$LPzYZ+FR0BI");
        if (!fn_profile)
            fn_profile = obs_module_symbol(h_ces, "LPzYZ+FR0BI#H#A");
        if (!fn_profile)
            fn_profile = obs_module_symbol(h_ces, "sceCesRefersUcsProfileCp1252");
        if (!fn_profile)
            fn_profile = obs_module_symbol(h_ces, "$EtBR4WfY_Cw");
    }
    if (!fn_profile && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$LPzYZ+FR0BI", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_profile = p;
                break;
            }
            if (sceKernelDlsym(h, "sceCesRefersUcsProfileCp1252", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_profile = p;
                break;
            }
        }
    }

    const void *fn_sbc2utf8 = NULL;
    if (obs_address_is_callable((const void *)&sceCesSbcToUtf8_static)) {
        fn_sbc2utf8 = (const void *)&sceCesSbcToUtf8_static;
    }
    if (!fn_sbc2utf8 && obs_address_is_callable((const void *)&sceCesSbcToUtf8)) {
        fn_sbc2utf8 = (const void *)&sceCesSbcToUtf8;
    }
    if (!fn_sbc2utf8 && h_ces >= 0) {
        fn_sbc2utf8 = obs_module_symbol(h_ces, "$xTd54EEL1Ao");
        if (!fn_sbc2utf8)
            fn_sbc2utf8 = obs_module_symbol(h_ces, "xTd54EEL1Ao#H#A");
        if (!fn_sbc2utf8)
            fn_sbc2utf8 = obs_module_symbol(h_ces, "sceCesSbcToUtf8");
        if (!fn_sbc2utf8)
            fn_sbc2utf8 = obs_module_symbol(h_ces, "$CtQLQeB5N8U");
    }
    if (!fn_sbc2utf8 && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$xTd54EEL1Ao", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_sbc2utf8 = p;
                break;
            }
            if (sceKernelDlsym(h, "sceCesSbcToUtf8", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_sbc2utf8 = p;
                break;
            }
        }
    }

    const void *fn_utf82sbc = NULL;
    if (obs_address_is_callable((const void *)&sceCesUtf8ToSbc_static)) {
        fn_utf82sbc = (const void *)&sceCesUtf8ToSbc_static;
    }
    if (!fn_utf82sbc && obs_address_is_callable((const void *)&sceCesUtf8ToSbc)) {
        fn_utf82sbc = (const void *)&sceCesUtf8ToSbc;
    }
    if (!fn_utf82sbc && h_ces >= 0) {
        fn_utf82sbc = obs_module_symbol(h_ces, "$3Q1gOWWarcw");
        if (!fn_utf82sbc)
            fn_utf82sbc = obs_module_symbol(h_ces, "3Q1gOWWarcw#H#A");
        if (!fn_utf82sbc)
            fn_utf82sbc = obs_module_symbol(h_ces, "sceCesUtf8ToSbc");
        if (!fn_utf82sbc)
            fn_utf82sbc = obs_module_symbol(h_ces, "$zK2aZTlgDd0");
    }
    if (!fn_utf82sbc && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$3Q1gOWWarcw", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_utf82sbc = p;
                break;
            }
            if (sceKernelDlsym(h, "sceCesUtf8ToSbc", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_utf82sbc = p;
                break;
            }
        }
    }

    typedef const void *(*fn_profile_t)(void);
    typedef int (*fn_sbc2utf8_t)(const void *, uint32_t, void *, size_t, size_t *);
    typedef int (*fn_utf82sbc_t)(const void *, size_t, size_t *, const void *,
                                 uint32_t *);

    const void *profile = NULL;
    if (fn_profile && obs_address_is_callable(fn_profile)) {
        profile = ((fn_profile_t)fn_profile)();
    }
    obs_report_measure(id, "RefersUcsProfileCp1252", "ptr",
                       (uint64_t)(uintptr_t)profile, "addr");
    if (profile && obs_address_is_callable(profile)) {
        obs_report_bytes(id, "RefersUcsProfileCp1252", "profile-20h", 0,
                         (const uint8_t *)profile, 0x20);
    }

    if (profile && fn_sbc2utf8 && obs_address_is_callable(fn_sbc2utf8)) {
        fn_sbc2utf8_t f_conv = (fn_sbc2utf8_t)fn_sbc2utf8;
        static const uint32_t sbc_vals[] = {0x41, 0x80, 0x81, 0xFF};
        for (size_t i = 0; i < sizeof(sbc_vals) / sizeof(sbc_vals[0]); i++) {
            uint32_t b = sbc_vals[i];
            char tag[64];
            snprintf(tag, sizeof(tag), "SbcToUtf8-0x%02x", b);
            uint8_t out[16];
            memset(out, 0xcc, sizeof(out));
            size_t out_len = 0xcccc;
            int rc = f_conv(profile, b, out, 4, &out_len);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure(id, tag, "len", (uint64_t)out_len, "bytes");
            obs_report_bytes(id, tag, "out-4b", 0, out, 4);
        }

        /* out null */
        size_t null_len = 0xcccc;
        int rc_null_out = f_conv(profile, 0x80, NULL, 4, &null_len);
        obs_report_measure(id, "SbcToUtf8-out-null", "rc",
                           (uint64_t)(uint32_t)rc_null_out, "code");
        obs_report_measure(id, "SbcToUtf8-out-null", "len", (uint64_t)null_len,
                           "bytes");

        /* max 1 for 0x80 */
        uint8_t out1[16];
        memset(out1, 0xcc, sizeof(out1));
        size_t len1 = 0xcccc;
        int rc_max1 = f_conv(profile, 0x80, out1, 1, &len1);
        obs_report_measure(id, "SbcToUtf8-0x80-max1", "rc", (uint64_t)(uint32_t)rc_max1,
                           "code");
        obs_report_measure(id, "SbcToUtf8-0x80-max1", "len", (uint64_t)len1, "bytes");
        obs_report_bytes(id, "SbcToUtf8-0x80-max1", "out-1b", 0, out1, 1);
    }

    if (profile && fn_utf82sbc && obs_address_is_callable(fn_utf82sbc)) {
        fn_utf82sbc_t f_u2s = (fn_utf82sbc_t)fn_utf82sbc;
        static const struct {
            const char *str;
            size_t len;
            const char *name;
        } utf_cases[] = {{"A", 1, "A"},
                         {"\xE2\x82\xAC", 3, "euro"},
                         {"\xE2\x82", 2, "truncated"},
                         {"\xC0\x80", 2, "overlong"},
                         {"\xED\xA0\x80", 3, "surrogate"},
                         {"\xF0\x9F\x98\x80", 4, "unmappable"}};
        for (size_t i = 0; i < sizeof(utf_cases) / sizeof(utf_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "Utf8ToSbc-%s", utf_cases[i].name);
            size_t used = 0xcccc;
            uint32_t sbc = 0xcccccccc;
            int rc = f_u2s(utf_cases[i].str, utf_cases[i].len, &used, profile, &sbc);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure(id, tag, "used", (uint64_t)used, "bytes");
            obs_report_measure(id, tag, "sbc", (uint64_t)sbc, "byte");
        }

        /* max 0 */
        size_t used0 = 0xcccc;
        uint32_t sbc0 = 0xcccccccc;
        int rc_max0 = f_u2s("A", 0, &used0, profile, &sbc0);
        obs_report_measure(id, "Utf8ToSbc-max0", "rc", (uint64_t)(uint32_t)rc_max0,
                           "code");
        obs_report_measure(id, "Utf8ToSbc-max0", "used", (uint64_t)used0, "bytes");
        obs_report_measure(id, "Utf8ToSbc-max0", "sbc", (uint64_t)sbc0, "byte");

        /* sbc null */
        size_t used_ns = 0xcccc;
        int rc_null_sbc = f_u2s("A", 1, &used_ns, profile, NULL);
        obs_report_measure(id, "Utf8ToSbc-null-sbc", "rc",
                           (uint64_t)(uint32_t)rc_null_sbc, "code");
        obs_report_measure(id, "Utf8ToSbc-null-sbc", "used", (uint64_t)used_ns,
                           "bytes");
    }

    int rc_137 = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_137 = sceSysmoduleLoadModule(0x0137);
    }
    obs_report_measure(id, "Psml-load-0x137", "rc", (uint64_t)(uint32_t)rc_137, "code");

    int h_psml = obs_module_open("libScePsml");
    const void *fn_psml1 = obs_module_symbol(OBS_HANDLE_SELF, "$3WVD91e12ZQ");
    const void *fn_psml2 = obs_module_symbol(OBS_HANDLE_SELF, "$+2KpvixvL6E");
    if (!fn_psml1 && h_psml > 0) {
        fn_psml1 = obs_module_symbol(h_psml, "$3WVD91e12ZQ");
    }
    if (!fn_psml2 && h_psml > 0) {
        fn_psml2 = obs_module_symbol(h_psml, "$+2KpvixvL6E");
    }
    if (!fn_psml1 && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$3WVD91e12ZQ", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_psml1 = p;
                break;
            }
        }
    }
    if (!fn_psml2 && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            void *p = NULL;
            if (sceKernelDlsym(h, "$+2KpvixvL6E", &p) == 0 &&
                obs_address_is_callable(p)) {
                fn_psml2 = p;
                break;
            }
        }
    }
    obs_report_measure(id, "Psml-3WVD91e12ZQ", "bound", (uint64_t)(fn_psml1 != NULL),
                       "bool");
    obs_report_measure(id, "Psml-+2KpvixvL6E", "bound", (uint64_t)(fn_psml2 != NULL),
                       "bool");

    if (fn_psml1 && obs_address_is_callable(fn_psml1)) {
        int rc1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc1 = ((int (*)(void))fn_psml1)();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Psml-call1", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Psml-call1", "rc", (uint64_t)(uint32_t)rc1, "code");
    }

    if (fn_psml2 && obs_address_is_callable(fn_psml2)) {
        uint8_t out40[0x40];
        memset(out40, 0xcc, sizeof(out40));
        static uint8_t buf1000[0x1000];
        memset(buf1000, 0, sizeof(buf1000));
        int rc2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc2 = ((int (*)(void *, void *))fn_psml2)(out40, buf1000);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "Psml-call2", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "Psml-call2", "rc", (uint64_t)(uint32_t)rc2, "code");
        obs_report_bytes(id, "Psml-call2", "out-40b", 0, out40, sizeof(out40));
    }

    return obs_pass();
}

static obs_result check_rtc_datetime(void) {
    const char *id = "000-boot/rtc-datetime";
    obs_jmp_buf guard;
    int sig;

    (void)obs_module_open("libSceRtc");

    /* Arm 1: GetCurrentClockLocalTime, GetCurrentClock, GetCurrentClockUtc,
     * GetCurrentNetworkTick, GetTickResolution */
    uint8_t dt_local[0x18];
    memset(dt_local, 0xcc, sizeof(dt_local));
    int rc_local = -1;
    if (obs_address_is_callable((const void *)&sceRtcGetCurrentClockLocalTime_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_local = sceRtcGetCurrentClockLocalTime_static(dt_local);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentClockLocalTime", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "GetCurrentClockLocalTime", "rc",
                       (uint64_t)(uint32_t)rc_local, "code");
    obs_report_bytes(id, "GetCurrentClockLocalTime", "dt-18h", 0, dt_local,
                     sizeof(dt_local));

    uint8_t dt_clock60[0x18];
    memset(dt_clock60, 0xcc, sizeof(dt_clock60));
    int rc_clock60 = -1;
    if (obs_address_is_callable((const void *)&sceRtcGetCurrentClock_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_clock60 = sceRtcGetCurrentClock_static(dt_clock60, 60);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentClock-tz60", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "GetCurrentClock-tz60", "rc", (uint64_t)(uint32_t)rc_clock60,
                       "code");
    obs_report_bytes(id, "GetCurrentClock-tz60", "dt-18h", 0, dt_clock60,
                     sizeof(dt_clock60));

    uint8_t dt_utc[0x18];
    memset(dt_utc, 0xcc, sizeof(dt_utc));
    int rc_utc = -1;
    if (obs_address_is_callable((const void *)&sceRtcGetCurrentClockUtc_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_utc = sceRtcGetCurrentClockUtc_static(dt_utc);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentClockUtc", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "GetCurrentClockUtc", "rc", (uint64_t)(uint32_t)rc_utc,
                       "code");
    obs_report_bytes(id, "GetCurrentClockUtc", "dt-18h", 0, dt_utc, sizeof(dt_utc));

    uint64_t net_tick = 0xccccccccccccccccULL;
    int rc_net_tick = -1;
    if (obs_address_is_callable((const void *)&sceRtcGetCurrentNetworkTick_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_net_tick = sceRtcGetCurrentNetworkTick_static(&net_tick);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetCurrentNetworkTick", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "GetCurrentNetworkTick", "rc",
                       (uint64_t)(uint32_t)rc_net_tick, "code");
    obs_report_measure(id, "GetCurrentNetworkTick", "tick", net_tick, "usec");
    obs_report_bytes(id, "GetCurrentNetworkTick", "tick-8b", 0,
                     (const uint8_t *)&net_tick, 8);

    uint32_t resolution = 0;
    if (obs_address_is_callable((const void *)&sceRtcGetTickResolution_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            resolution = sceRtcGetTickResolution_static();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetTickResolution", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "GetTickResolution", "resolution", (uint64_t)resolution,
                       "hz");

    /* Arm 2: GetTick / SetTick for 2026-10-12 12:34:56.789012, null, month 13 */
    struct {
        uint16_t year;
        uint16_t month;
        uint16_t day;
        uint16_t hour;
        uint16_t minute;
        uint16_t second;
        uint32_t microsecond;
    } dt_spec = {2026, 10, 12, 12, 34, 56, 789012};

    uint64_t tick_spec = 0xccccccccccccccccULL;
    int rc_gt = -1;
    if (obs_address_is_callable((const void *)&sceRtcGetTick_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_gt = sceRtcGetTick_static(&dt_spec, &tick_spec);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "GetTick-2026", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "GetTick-2026", "rc", (uint64_t)(uint32_t)rc_gt, "code");
    obs_report_measure(id, "GetTick-2026", "tick", tick_spec, "usec");
    obs_report_bytes(id, "GetTick-2026", "tick-8b", 0, (const uint8_t *)&tick_spec, 8);

    uint8_t dt_roundtrip[0x18];
    memset(dt_roundtrip, 0xcc, sizeof(dt_roundtrip));
    int rc_st = -1;
    if (obs_address_is_callable((const void *)&sceRtcSetTick_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_st = sceRtcSetTick_static(dt_roundtrip, &tick_spec);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "SetTick-2026", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "SetTick-2026", "rc", (uint64_t)(uint32_t)rc_st, "code");
    obs_report_bytes(id, "SetTick-2026", "dt-18h", 0, dt_roundtrip,
                     sizeof(dt_roundtrip));

    /* null checks */
    if (obs_address_is_callable((const void *)&sceRtcGetTick_static)) {
        int rc_gt_n1 = -1, rc_gt_n2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_gt_n1 = sceRtcGetTick_static(NULL, &tick_spec);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_gt_n2 = sceRtcGetTick_static(&dt_spec, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetTick-null-dt", "rc", (uint64_t)(uint32_t)rc_gt_n1,
                           "code");
        obs_report_measure(id, "GetTick-null-tick", "rc", (uint64_t)(uint32_t)rc_gt_n2,
                           "code");
    }

    if (obs_address_is_callable((const void *)&sceRtcSetTick_static)) {
        int rc_st_n1 = -1, rc_st_n2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_st_n1 = sceRtcSetTick_static(NULL, &tick_spec);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_st_n2 = sceRtcSetTick_static(dt_roundtrip, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "SetTick-null-dt", "rc", (uint64_t)(uint32_t)rc_st_n1,
                           "code");
        obs_report_measure(id, "SetTick-null-tick", "rc", (uint64_t)(uint32_t)rc_st_n2,
                           "code");
    }

    /* month 13 */
    if (obs_address_is_callable((const void *)&sceRtcGetTick_static)) {
        struct {
            uint16_t year;
            uint16_t month;
            uint16_t day;
            uint16_t hour;
            uint16_t minute;
            uint16_t second;
            uint32_t microsecond;
        } dt_m13 = {2026, 13, 12, 12, 34, 56, 789012};
        uint64_t tick_m13 = 0xccccccccccccccccULL;
        int rc_m13 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_m13 = sceRtcGetTick_static(&dt_m13, &tick_m13);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "GetTick-month13", "rc", (uint64_t)(uint32_t)rc_m13,
                           "code");
        obs_report_measure(id, "GetTick-month13", "tick", tick_m13, "usec");
    }

    /* Arm 3: CheckValid edge cases */
    if (obs_address_is_callable((const void *)&sceRtcCheckValid_static)) {
        static const struct {
            const char *name;
            uint16_t y, m, d, h, min, s;
            uint32_t us;
            int is_null;
        } valid_cases[] = {
            {"year-0", 0, 10, 12, 12, 34, 56, 0, 0},
            {"year-10000", 10000, 10, 12, 12, 34, 56, 0, 0},
            {"month-0", 2026, 0, 12, 12, 34, 56, 0, 0},
            {"month-13", 2026, 13, 12, 12, 34, 56, 0, 0},
            {"apr-31", 2026, 4, 31, 12, 34, 56, 0, 0},
            {"feb-29-2025", 2025, 2, 29, 12, 34, 56, 0, 0},
            {"hour-24", 2026, 10, 12, 24, 34, 56, 0, 0},
            {"min-60", 2026, 10, 12, 12, 60, 56, 0, 0},
            {"sec-60", 2026, 10, 12, 12, 34, 60, 0, 0},
            {"usec-1000000", 2026, 10, 12, 12, 34, 56, 1000000, 0},
            {"null", 0, 0, 0, 0, 0, 0, 0, 1},
        };
        for (size_t i = 0; i < sizeof(valid_cases) / sizeof(valid_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "CheckValid-%s", valid_cases[i].name);
            struct {
                uint16_t y, m, d, h, min, s;
                uint32_t us;
            } cv = {valid_cases[i].y, valid_cases[i].m,   valid_cases[i].d,
                    valid_cases[i].h, valid_cases[i].min, valid_cases[i].s,
                    valid_cases[i].us};
            int rc_v = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_v = sceRtcCheckValid_static(valid_cases[i].is_null ? NULL : &cv);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_v, "code");
        }
    }

    /* Arm 4: IsLeapYear, GetDaysInMonth, GetDayOfWeek */
    if (obs_address_is_callable((const void *)&sceRtcIsLeapYear_static)) {
        static const int leap_years[] = {0, 2000, 2100, 2024};
        for (size_t i = 0; i < sizeof(leap_years) / sizeof(leap_years[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "IsLeapYear-%d", leap_years[i]);
            int rc_ly = sceRtcIsLeapYear_static(leap_years[i]);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_ly, "code");
        }
    }

    if (obs_address_is_callable((const void *)&sceRtcGetDaysInMonth_static)) {
        static const struct {
            int y, m;
        } dim_cases[] = {{2024, 2}, {2024, 13}, {0, 1}};
        for (size_t i = 0; i < sizeof(dim_cases) / sizeof(dim_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "GetDaysInMonth-%d-%d", dim_cases[i].y,
                     dim_cases[i].m);
            int rc_dim = sceRtcGetDaysInMonth_static(dim_cases[i].y, dim_cases[i].m);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_dim, "code");
        }
    }

    if (obs_address_is_callable((const void *)&sceRtcGetDayOfWeek_static)) {
        static const struct {
            int y, m, d;
        } dow_cases[] = {{2026, 10, 12}, {2026, 13, 1}, {0, 13, 1}, {2026, 2, 30}};
        for (size_t i = 0; i < sizeof(dow_cases) / sizeof(dow_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "GetDayOfWeek-%d-%d-%d", dow_cases[i].y,
                     dow_cases[i].m, dow_cases[i].d);
            int rc_dow = sceRtcGetDayOfWeek_static(dow_cases[i].y, dow_cases[i].m,
                                                   dow_cases[i].d);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_dow, "code");
        }
    }

    /* Arm 5: GetTime_t, SetTime_t, GetWin32FileTime, CompareTick,
     * ConvertUtcToLocalTime, TickAddMonths, TickAddYears */
    if (obs_address_is_callable((const void *)&sceRtcGetTime_t_static)) {
        struct {
            uint16_t y, m, d, h, min, s;
            uint32_t us;
        } dt1969 = {1969, 12, 31, 23, 59, 59, 0};
        struct {
            uint16_t y, m, d, h, min, s;
            uint32_t us;
        } dt2026_t = {2026, 10, 12, 12, 34, 56, 0};
        uint64_t t1969 = 0xccccccccccccccccULL;
        uint64_t t2026_val = 0xccccccccccccccccULL;
        int rc_t1969 = sceRtcGetTime_t_static(&dt1969, &t1969);
        int rc_t2026 = sceRtcGetTime_t_static(&dt2026_t, &t2026_val);
        obs_report_measure(id, "GetTime_t-1969", "rc", (uint64_t)(uint32_t)rc_t1969,
                           "code");
        obs_report_measure(id, "GetTime_t-1969", "time_t", t1969, "sec");
        obs_report_measure(id, "GetTime_t-2026", "rc", (uint64_t)(uint32_t)rc_t2026,
                           "code");
        obs_report_measure(id, "GetTime_t-2026", "time_t", t2026_val, "sec");
    }

    if (obs_address_is_callable((const void *)&sceRtcSetTime_t_static)) {
        uint8_t dt_neg[0x18];
        memset(dt_neg, 0xcc, sizeof(dt_neg));
        int rc_st_neg = sceRtcSetTime_t_static(dt_neg, -1);
        obs_report_measure(id, "SetTime_t-neg1", "rc", (uint64_t)(uint32_t)rc_st_neg,
                           "code");
        obs_report_bytes(id, "SetTime_t-neg1", "dt-18h", 0, dt_neg, sizeof(dt_neg));
    }

    if (obs_address_is_callable((const void *)&sceRtcGetWin32FileTime_static)) {
        struct {
            uint16_t y, m, d, h, min, s;
            uint32_t us;
        } dt1600 = {1600, 1, 1, 0, 0, 0, 0};
        uint64_t ft1600 = 0xccccccccccccccccULL;
        int rc_ft = sceRtcGetWin32FileTime_static(&dt1600, &ft1600);
        obs_report_measure(id, "GetWin32FileTime-1600", "rc", (uint64_t)(uint32_t)rc_ft,
                           "code");
        obs_report_measure(id, "GetWin32FileTime-1600", "filetime", ft1600, "count");
    }

    if (obs_address_is_callable((const void *)&sceRtcCompareTick_static)) {
        uint64_t t1 = 1000ULL, t2 = 2000ULL;
        int rc_cmp_lt = sceRtcCompareTick_static(&t1, &t2);
        int rc_cmp_eq = sceRtcCompareTick_static(&t1, &t1);
        int rc_cmp_gt = sceRtcCompareTick_static(&t2, &t1);
        obs_report_measure(id, "CompareTick-lt", "rc", (uint64_t)(uint32_t)rc_cmp_lt,
                           "code");
        obs_report_measure(id, "CompareTick-eq", "rc", (uint64_t)(uint32_t)rc_cmp_eq,
                           "code");
        obs_report_measure(id, "CompareTick-gt", "rc", (uint64_t)(uint32_t)rc_cmp_gt,
                           "code");
    }

    if (obs_address_is_callable((const void *)&sceRtcConvertUtcToLocalTime_static) &&
        obs_address_is_callable((const void *)&sceRtcConvertLocalTimeToUtc_static)) {
        uint8_t local_conv[0x18];
        uint8_t utc_roundtrip[0x18];
        memset(local_conv, 0xcc, sizeof(local_conv));
        memset(utc_roundtrip, 0xcc, sizeof(utc_roundtrip));
        int rc_u2l = sceRtcConvertUtcToLocalTime_static(dt_utc, local_conv);
        int rc_l2u = sceRtcConvertLocalTimeToUtc_static(local_conv, utc_roundtrip);
        obs_report_measure(id, "ConvertUtcToLocalTime", "rc",
                           (uint64_t)(uint32_t)rc_u2l, "code");
        obs_report_measure(id, "ConvertLocalTimeToUtc", "rc",
                           (uint64_t)(uint32_t)rc_l2u, "code");
        obs_report_bytes(id, "ConvertUtcToLocalTime", "local-18h", 0, local_conv,
                         sizeof(local_conv));
        obs_report_bytes(id, "ConvertLocalTimeToUtc", "utc-18h", 0, utc_roundtrip,
                         sizeof(utc_roundtrip));
    }

    if (obs_address_is_callable((const void *)&sceRtcTickAddMonths_static)) {
        struct {
            uint16_t y, m, d, h, min, s;
            uint32_t us;
        } dt_jan31 = {2026, 1, 31, 12, 0, 0, 0};
        uint64_t t_jan31 = 0;
        uint64_t t_add_m = 0xccccccccccccccccULL;
        (void)sceRtcGetTick_static(&dt_jan31, &t_jan31);
        int rc_tam = sceRtcTickAddMonths_static(&t_add_m, &t_jan31, 1);
        obs_report_measure(id, "TickAddMonths-jan31-1", "rc",
                           (uint64_t)(uint32_t)rc_tam, "code");
        obs_report_measure(id, "TickAddMonths-jan31-1", "tick", t_add_m, "usec");
    }

    if (obs_address_is_callable((const void *)&sceRtcTickAddYears_static)) {
        struct {
            uint16_t y, m, d, h, min, s;
            uint32_t us;
        } dt_feb29 = {2024, 2, 29, 12, 0, 0, 0};
        uint64_t t_feb29 = 0;
        uint64_t t_add_y = 0xccccccccccccccccULL;
        (void)sceRtcGetTick_static(&dt_feb29, &t_feb29);
        int rc_tay = sceRtcTickAddYears_static(&t_add_y, &t_feb29, 1);
        obs_report_measure(id, "TickAddYears-feb29-1", "rc", (uint64_t)(uint32_t)rc_tay,
                           "code");
        obs_report_measure(id, "TickAddYears-feb29-1", "tick", t_add_y, "usec");
    }

    /* Arm 6: FormatRFC3339 and ParseRFC3339 */
    if (obs_address_is_callable((const void *)&sceRtcFormatRFC3339_static)) {
        static const int tz_cases[] = {0, 540, -60, 1440};
        for (size_t i = 0; i < sizeof(tz_cases) / sizeof(tz_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "FormatRFC3339-tz%d", tz_cases[i]);
            char rfc_buf[64];
            memset(rfc_buf, 0, sizeof(rfc_buf));
            int rc_fmt = sceRtcFormatRFC3339_static(rfc_buf, &tick_spec, tz_cases[i]);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_fmt, "code");
            obs_report_bytes(id, tag, "rfc-str", 0, (const uint8_t *)rfc_buf,
                             (unsigned int)strlen(rfc_buf));
        }
        /* null tick */
        char rfc_buf_null[64];
        memset(rfc_buf_null, 0, sizeof(rfc_buf_null));
        int rc_fmt_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_fmt_null = sceRtcFormatRFC3339_static(rfc_buf_null, NULL, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "FormatRFC3339-null-tick", "rc",
                           (uint64_t)(uint32_t)rc_fmt_null, "code");
    }

    if (obs_address_is_callable((const void *)&sceRtcParseRFC3339_static)) {
        static const char *parse_cases[] = {
            "2026-10-12T12:34:56.789Z",
            "2026-10-12T12:34:56+09:00",
            "2026-10-12 12:34:56Z",
            "2026-10-12T12:34:56",
        };
        for (size_t i = 0; i < sizeof(parse_cases) / sizeof(parse_cases[0]); i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "ParseRFC3339-case%zu", i + 1);
            uint64_t parsed_tick = 0xccccccccccccccccULL;
            int rc_parse = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_parse = sceRtcParseRFC3339_static(&parsed_tick, parse_cases[i]);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc_parse, "code");
            obs_report_measure(id, tag, "tick", parsed_tick, "usec");
        }
    }

    return obs_pass();
}

#include "wave1_checks.inc"
#include "wave2_checks.inc"

/* REQ-20261014T0300Z-rf04 Arm 2: UniversalDataSystem lifecycle & s6W4Zl4Slgk */
static obs_result check_rf04_uds(void) {
    const char *id = "000-boot/uds-s6W4Zl4Slgk";
    obs_jmp_buf guard;

    /* 1. sceSysmoduleLoadModule(0x0105) rc */
    int rc_mod = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_mod = sceSysmoduleLoadModule(0x0105);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceSysmoduleLoadModule", "rc-0x0105",
                       (uint64_t)(uint32_t)rc_mod, "code");

    /* 2. Resolve UDS functions */
    typedef int (*fn_uds_init_t)(const void *, const void *, uint64_t, uint64_t);
    typedef int (*fn_uds_ctx_t)(uint64_t *, int32_t, uint64_t, uint64_t);
    typedef int (*fn_uds_handle_t)(uint64_t *);
    typedef uint64_t (*fn_uds_call_t)(void *, void *, void *, void *, void *, void *);

    int h_uds = obs_module_open("libSceNpUniversalDataSystem");
    fn_uds_init_t fn_uds_init =
        (fn_uds_init_t)obs_module_symbol(h_uds, "sceNpUniversalDataSystemInitialize");
    if (!fn_uds_init)
        fn_uds_init = (fn_uds_init_t)obs_module_symbol(h_uds, "$HngqGGyoNrI");
    if (!fn_uds_init)
        fn_uds_init = (fn_uds_init_t)obs_module_symbol(h_uds, "$sjaobBgqeB4");

    fn_uds_ctx_t fn_uds_ctx =
        (fn_uds_ctx_t)obs_module_symbol(h_uds, "sceNpUniversalDataSystemCreateContext");
    if (!fn_uds_ctx)
        fn_uds_ctx = (fn_uds_ctx_t)obs_module_symbol(h_uds, "$5zBnau1uIEo");

    fn_uds_handle_t fn_uds_h = (fn_uds_handle_t)obs_module_symbol(
        h_uds, "sceNpUniversalDataSystemCreateHandle");
    if (!fn_uds_h)
        fn_uds_h = (fn_uds_handle_t)obs_module_symbol(h_uds, "$hT0IAEvN+M0");

    fn_uds_call_t fn_s6W4 = (fn_uds_call_t)obs_module_symbol(h_uds, "$s6W4Zl4Slgk");
    if (!fn_s6W4)
        fn_s6W4 = (fn_uds_call_t)obs_module_symbol(h_uds, "s6W4Zl4Slgk");
    if (!fn_s6W4 && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        (void)sceKernelDlsym(h_uds, "s6W4Zl4Slgk", (void **)&fn_s6W4);
    }

    /* 3. sceNpUniversalDataSystemInitialize with REQ-np02's 0x20-byte block */
    static uint8_t s_uds_pool[0x40000] __attribute__((aligned(64)));
    uint64_t uds_param[4] = {0x10, 0x40000, (uint64_t)(uintptr_t)s_uds_pool, 0x10};
    int rc_init = -1;
    if (fn_uds_init) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_uds_init(uds_param, NULL, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceNpUniversalDataSystemInitialize", "rc",
                       (uint64_t)(uint32_t)rc_init, "code");
    obs_report_bytes(id, "sceNpUniversalDataSystemInitialize", "param-block", 0,
                     (const unsigned char *)uds_param, sizeof(uds_param));

    if (rc_init == (int)0x80553118) {
        return obs_pass();
    }

    /* 4. CreateContext and CreateHandle */
    int32_t user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0)
            user = 0x1ea2f4d9;
    } else {
        user = 0x1ea2f4d9;
    }
    uint64_t ctx = 0;
    int rc_ctx = -1;
    if (fn_uds_ctx) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ctx = fn_uds_ctx(&ctx, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceNpUniversalDataSystemCreateContext", "rc",
                       (uint64_t)(uint32_t)rc_ctx, "code");
    obs_report_measure(id, "sceNpUniversalDataSystemCreateContext", "ctx-val", ctx,
                       "hex");

    uint64_t handle = 0;
    int rc_h = -1;
    if (fn_uds_h) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_h = fn_uds_h(&handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "sceNpUniversalDataSystemCreateHandle", "rc",
                       (uint64_t)(uint32_t)rc_h, "code");
    obs_report_measure(id, "sceNpUniversalDataSystemCreateHandle", "handle-val", handle,
                       "hex");

    /* 5. s6W4Zl4Slgk(ctx, handle, "activityId", name+1, buf, buf) */
    const char *name = "activityId";
    uint8_t buf4[0x100], buf5[0x100];
    uint8_t before[0x100];
    memset(buf4, 0xcc, sizeof(buf4));
    memset(buf5, 0xcc, sizeof(buf5));
    memset(before, 0xcc, sizeof(before));

    uint64_t rc_act = 0xffffffffffffffffULL;
    int sig_act = 0;
    if (fn_s6W4) {
        sig_act = OBS_FAULT_ARM(&guard);
        if (sig_act == 0) {
            rc_act = fn_s6W4((void *)(uintptr_t)ctx, (void *)(uintptr_t)handle,
                             (void *)name, (void *)(name + 1), buf4, buf5);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    if (sig_act != 0) {
        obs_report_measure(id, "s6W4Zl4Slgk-activity", "fault", (uint64_t)sig_act,
                           "signal");
    }
    obs_report_measure(id, "s6W4Zl4Slgk-activity", "rc", rc_act, "code");
    obs_report_written(id, "s6W4Zl4Slgk-activity", "buf4-changed", before, buf4,
                       sizeof(buf4));
    obs_report_written(id, "s6W4Zl4Slgk-activity", "buf5-changed", before, buf5,
                       sizeof(buf5));

    /* 6. All zero call: s6W4Zl4Slgk(0, 0, 0, 0, 0, 0) */
    uint64_t rc_zero = 0xffffffffffffffffULL;
    int sig_zero = 0;
    if (fn_s6W4) {
        sig_zero = OBS_FAULT_ARM(&guard);
        if (sig_zero == 0) {
            rc_zero = fn_s6W4(NULL, NULL, NULL, NULL, NULL, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    if (sig_zero != 0) {
        obs_report_measure(id, "s6W4Zl4Slgk-allzero", "fault", (uint64_t)sig_zero,
                           "signal");
    }
    obs_report_measure(id, "s6W4Zl4Slgk-allzero", "rc", rc_zero, "code");

    return obs_pass();
}

static const obs_check boot_checks[] = {
    {"000-boot/audioout2-fresh-process", "libSceAudioOut2", "$g2tViFIohHE",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_audioout2_fresh_process,
     OBS_FROM_SPEC},
#if defined(OBS_FAULT_SELFTEST)
    {"000-boot/fault-guard-self-test", "obscene", "(self-check)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_fault_guard_selftest, OBS_FROM_SPEC},
#endif
    {"000-boot/png-dec-enc", "libScePngDec", "scePngDecCreate", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_png_dec_enc, OBS_FROM_SPEC},
    {"000-boot/savedata-dialog", "libSceSaveDataDialog", "sceSaveDataDialogOpen",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_savedata_dialog, OBS_FROM_SPEC},
    {"000-boot/system-dialogs", "libSceCommonDialog", "sceCommonDialogInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_system_dialogs, OBS_FROM_SPEC},
    {"000-boot/sce-ult", "libSceUlt", "_sceUltUlthreadCreate", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_sce_ult, OBS_FROM_SPEC},
    {"000-boot/number-formatting", "obscene", "obs_format_u64", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_formatting, OBS_FROM_ASSUMED},
    {"000-boot/write-returns-count", "libkernel", "sceKernelWrite", OBS_CAP_NONE,
     OBS_CAP_OUTPUT, (const void *)&sceKernelWrite, check_write_returns_count,
     OBS_FROM_DERIVED},
    {"000-boot/sysmodule-isloaded-lifecycle", "libSceSysmodule", "sceSysmoduleIsLoaded",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_41aa_sysmodule_isloaded_lifecycle,
     OBS_FROM_SPEC},
    {"000-boot/uds-s6W4Zl4Slgk", "libSceNpUniversalDataSystem", "$s6W4Zl4Slgk",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_rf04_uds, OBS_FROM_SPEC},
    {"000-boot/uds-create-event", "libSceNpUniversalDataSystem", "$p+GcLqwpL9M",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ue01_uds_events, OBS_FROM_SPEC},
    {"000-boot/np05-routes-calls", "libSceNpManager", "$eQH7nWPcAgc", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_np05_routes_and_calls, OBS_FROM_SPEC},
    {"000-boot/sd02-savedata-avplayer", "libSceSaveData", "$T98d0hFw90k", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_sd02_savedata_avplayer, OBS_FROM_SPEC},
    {"000-boot/ge-indx-offset", "libSceAgc", "sceAgcDcbDrawIndexOffset", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ix01_ge_indx_offset, OBS_FROM_SPEC},
    {"000-boot/rf04-apr-app0-savedata", "libkernel", "$gEpBkcwxUjw", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_rf04_apr_app0_savedata, OBS_FROM_SPEC},
    {"000-boot/ap01-audio-propagation-acm", "libSceAudioPropagation", "$7xyAxrusLko",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ap01_audio_propagation_acm,
     OBS_FROM_SPEC},
    {"000-boot/audioout2-port-set-attributes", "libSceAudioOut2", "$0x6o1VVAYSY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ap03_audioout2_attributes,
     OBS_FROM_SPEC},
    {"000-boot/audioout2-pcm-pump", "libSceAudioOut2", "$0x6o1VVAYSY", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ap02_audioout2_pcm_pump, OBS_FROM_SPEC},
    {"000-boot/audioout2-ppsa21564-context", "libSceAudioOut2", "$0x6o1VVAYSY",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ac02_audioout2_ppsa21564_context,
     OBS_FROM_SPEC},
    {"000-boot/unity-user-service-chain", "libSceUserService", "$j3YMu1MVNNo",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_unity_user_service_chain,
     OBS_FROM_SPEC},
    {"000-boot/np-appcontent-startup", "libSceNpManager", "$CnoyyjMD74w", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_np_appcontent_startup, OBS_FROM_SPEC},
    {"000-boot/content-export-init", "libSceContentExport", "$0GnN4QCgIfs",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_content_export_init,
     OBS_FROM_SPEC},
    {"000-boot/avplayer-lifecycle", "libSceAvPlayer", "$o9eWRkSL+M4", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_avplayer_lifecycle, OBS_FROM_SPEC},
    {"000-boot/game-update-startup", "libSceGameUpdate", "$YJtKLttI9fM", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_game_update_startup, OBS_FROM_SPEC},
    {"000-boot/pad-ime-uds", "libScePad", "$gjP9-KQzoUk", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_pad_ime_uds, OBS_FROM_SPEC},
    {"000-boot/ag01-aux-calls", "libSceAgcDriver", "$vJXuU1HuDmc", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ag01_aux_calls, OBS_FROM_SPEC},
    {"000-boot/audioout2-mono-ports", "libSceAudioOut2", "$A89PZmqwrSQ", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_audioout2_mono_ports, OBS_FROM_SPEC},
    {"000-boot/ux01-calls", "libScePad", "$AcslpN1jHR8", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_ux01_calls, OBS_FROM_SPEC},
    {"000-boot/playgo-startup", "libScePlayGo", "$sUaKk5WGzrY", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_playgo_startup, OBS_FROM_SPEC},
    {"000-boot/ampr-write-address", "libSceAmpr", "$j0+3uJMxYJY", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_aw01_ampr_write_address, OBS_FROM_SPEC},
    {"000-boot/hardware-queries", "libkernel", "sceKernelGetCpuFrequency", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_hardware_queries, OBS_FROM_SPEC},
    {"000-boot/user-service-zoom-follow-focus", "libSceUserService", "$O6IW1-Dwm-w",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_user_service_zoom_follow_focus,
     OBS_FROM_SPEC},
    {"000-boot/net-inet-pton", "libSceNet", "sceNetInetPton", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_net_inet_pton, OBS_FROM_SPEC},
    {"000-boot/http-uri-parse", "libSceHttp", "sceHttpUriParse", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_http_uri_parse, OBS_FROM_SPEC},
    {"000-boot/netctl-callbacks", "libSceNetCtl", "sceNetCtlRegisterCallback",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_netctl_callbacks, OBS_FROM_SPEC},
    {"000-boot/udp-loopback", "libSceNet", "sceNetSendto", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_udp_loopback, OBS_FROM_SPEC},
    {"000-boot/net-resolver", "libSceNet", "sceNetResolverCreate", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_net_resolver, OBS_FROM_SPEC},
    {"000-boot/np-game-intent", "libSceNpGameIntent", "sceNpGameIntentInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_np_game_intent, OBS_FROM_SPEC},
    {"000-boot/cescs-psml", "libSceCes", "sceCesRefersUcsProfileCp1252", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_cescs_psml, OBS_FROM_SPEC},
    {"000-boot/rtc-datetime", "libSceRtc", "sceRtcGetCurrentClockLocalTime",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_rtc_datetime, OBS_FROM_SPEC},
    {"000-boot/ag02-shapes", "libSceVideoOut", "$eb-gvTYQcoY", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_ag02_shapes, OBS_FROM_SPEC},
    {"000-boot/ampr-command-buffer-queries", "libSceAmpr", "$ULvXMDz56po", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_aw03_ampr_command_buffer, OBS_FROM_SPEC},
    {"000-boot/write-rejects-bad-fd", "libkernel", "sceKernelWrite", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelWrite, check_write_rejects_bad_descriptor,
     OBS_FROM_DERIVED},
    {"000-boot/ly01-layer-export-topology7", "libSceAgc", "sceAgcDcbDrawIndexOffset",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_ly01_layer_export_topology7,
     OBS_FROM_SPEC},
    {"000-boot/gt01-npmanager", "libSceNpManager", "sceNpSetNpTitleId", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt01_npmanager, OBS_FROM_SPEC},
    {"000-boot/gt01-contentdelete", "libSceContentDelete", "sceContentDeleteInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt01_contentdelete,
     OBS_FROM_SPEC},
    {"000-boot/gt01-net-sockinfo", "libSceNet", "sceNetGetSockInfo", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt01_net_sockinfo, OBS_FROM_SPEC},
    {"000-boot/gt01-voice", "libSceVoice", "sceVoiceInit", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_gt01_voice, OBS_FROM_SPEC},
    {"000-boot/gt02-voice", "libSceVoice", "sceVoiceInit", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_gt02_voice, OBS_FROM_SPEC},
    {"000-boot/gt02-net-sockinfo", "libSceNet", "sceNetGetSockInfo", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt02_net_sockinfo, OBS_FROM_SPEC},
    {"000-boot/gt02-ajm-batch", "libSceAjm", "sceAjmBatchStart", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt02_ajm_batch, OBS_FROM_SPEC},
    {"000-boot/gt02-contentdelete", "libSceContentDelete", "sceContentDeleteInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt02_contentdelete,
     OBS_FROM_SPEC},
    {"000-boot/gt03-contentdelete", "libSceContentDelete", "sceContentDeleteInitialize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt03_contentdelete,
     OBS_FROM_SPEC},
    {"000-boot/gt03-net-sockinfo", "libSceNet", "sceNetGetSockInfo", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt03_net_sockinfo, OBS_FROM_SPEC},
    {"000-boot/gt03-ajm-batch", "libSceAjm", "sceAjmBatchStart", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt03_ajm_batch, OBS_FROM_SPEC},
    {"000-boot/gt04-memory-path", "libc", "stat", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_gt04_memory_path, OBS_FROM_SPEC},
    {"000-boot/gt04-notice-skip", "libSceSystemService",
     "sceSystemServiceGetNoticeScreenSkipFlag", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_gt04_notice_skip, OBS_FROM_SPEC},
    {"000-boot/gt05-pad-and-np", "libScePad", "scePadReadState", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt05_pad_and_np, OBS_FROM_SPEC},
    {"000-boot/gt06-np-callbacks", "libSceNpManager", "sceNpRegisterStateCallbackA",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt06_np_callbacks, OBS_FROM_SPEC},
    {"000-boot/gt07-stubbed-calls", "libSceVoice", "sceVoiceSetThreadsParams",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_gt07_stubbed_calls,
     OBS_FROM_SPEC},
    {"000-boot/gt09-placeholders", "libSceWebBrowserDialog",
     "sceWebBrowserDialogInitialize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_gt09_placeholders, OBS_FROM_SPEC},
    {"000-boot/stack-alignment", "obscene", "(self-check)", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_stack_alignment, OBS_FROM_SPEC},
};

const obs_section obs_section_boot = {
    "000-boot",
    "Boot and report stream",
    "Establishes that the report itself can be trusted before anything is reported.",
    boot_checks,
    OBS_COUNT(boot_checks),
};

/* ---- 010-kernel ------------------------------------------------------------ */

static obs_result check_process_time(void) {
    obs_report_measure("010-kernel/syscall-route", "syscall", "payload-args",
                       obs_get_payload_args() != NULL ? 1 : 0, "flag");
    obs_report_measure("010-kernel/syscall-route", "syscall", "gadget",
                       (uint64_t)obs_syscall_gadget_address(), "address");
    uint64_t first = sceKernelGetProcessTime();
    /* Burn a little time without sleeping, so this stays independent of the timer
     * section running later. */
    volatile uint64_t spin = 0;
    for (uint64_t i = 0; i < 200000u; i++) {
        spin += i;
    }
    (void)spin;
    uint64_t second = sceKernelGetProcessTime();
    if (second < first) {
        return obs_fail_code("process time went backwards", second);
    }
    if (second == first) {
        /* A constant clock is the classic stub, and it deadlocks anything that waits
         * on elapsed time. Amber rather than red: a coarse real clock can legitimately
         * not have ticked. */
        return obs_partial_value("process time did not advance", second);
    }
    return obs_pass_value(second - first);
}

static obs_result check_process_time_counter(void) {
    uint64_t first = sceKernelGetProcessTimeCounter();
    uint64_t second = sceKernelGetProcessTimeCounter();
    if (second < first) {
        return obs_fail_code("the counter went backwards", second);
    }
    if (first == 0 && second == 0) {
        return obs_partial("the counter reads zero");
    }
    return obs_pass_value(second);
}

static obs_result check_tsc_frequency(void) {
    uint64_t hz = sceKernelGetTscFrequency();
    if (hz == 0) {
        /* Everything that converts counter ticks to seconds divides by this. Zero is
         * not a slow clock, it is a fault waiting to happen somewhere unrelated. */
        return obs_fail("the counter frequency is zero");
    }
    /* Anything below a megahertz or above a terahertz is not a plausible timestamp
     * counter, and a wrong frequency makes every derived duration wrong by a factor
     * nobody will think to question. */
    if (hz < 1000000u || hz > 1000000000000u) {
        return obs_partial_value("the counter frequency is implausible", hz);
    }
    return obs_pass_value(hz);
}

static const obs_check kernel_checks[] = {
    {"010-kernel/process-time", "libkernel", "sceKernelGetProcessTime", OBS_CAP_NONE,
     OBS_CAP_TIME, (const void *)&sceKernelGetProcessTime, check_process_time,
     OBS_FROM_ASSUMED},
    {"010-kernel/process-time-counter", "libkernel", "sceKernelGetProcessTimeCounter",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelGetProcessTimeCounter,
     check_process_time_counter, OBS_FROM_ASSUMED},
    {"010-kernel/tsc-frequency", "libkernel", "sceKernelGetTscFrequency", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetTscFrequency, check_tsc_frequency,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_kernel = {
    "010-kernel",
    "Kernel core",
    "Process identity and the clocks everything else measures itself against.",
    kernel_checks,
    OBS_COUNT(kernel_checks),
};
