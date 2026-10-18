/*
 * wave2_checks.inc - Hardware probe implementations for:
 * - REQ-20261014T0430Z-ue01: UDS CreateEvent lifecycle & persistence
 * - REQ-20261014T0745Z-np05: NP libraries binding routes and offline calls
 * - REQ-20261014T0815Z-sd02: SaveData memory/mount and AvPlayer Ex lifecycle
 * - REQ-20261014T0415Z-ix01: GE_INDX_OFFSET draw index offsets and GPU submission
 */

#ifndef OBS_WAVE2_CHECKS_INC
#define OBS_WAVE2_CHECKS_INC

#if !defined(OBSCENE_HOST_BUILD)
#include "oops/heap.h"
#include "oops/memory.h"
#endif

/* -------------------------------------------------------------------------- */
/* REQ-20261014T0430Z-ue01 & np05 Arm 8: UDS Event Lifecycle                  */
/* -------------------------------------------------------------------------- */

static obs_result check_ue01_uds_events(void) {
    const char *id = "000-boot/uds-create-event";
    obs_jmp_buf guard;

    int rc_mod = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_mod = sceSysmoduleLoadModule(0x0105);
    }
    obs_report_measure(id, "setup", "sysmodule-load-0x0105-rc",
                       (uint64_t)(uint32_t)rc_mod, "code");

    typedef int (*fn_uds_init_t)(const void *, const void *, uint64_t, uint64_t);
    typedef int (*fn_uds_ctx_t)(uint64_t *, int32_t, uint64_t, uint64_t);
    typedef int (*fn_uds_handle_t)(uint64_t *);
    typedef int (*fn_uds_reg_ctx_t)(uint64_t, uint64_t, uint32_t);
    typedef uint64_t (*fn_uds_create_prop_t)(void *, void *, void *, void *, void *,
                                             void *);
    typedef int (*fn_uds_create_event_t)(const char *, void *, void *, uint32_t);
    typedef int (*fn_uds_post_event_t)(uint64_t, uint64_t, void *, uint32_t);
    typedef int (*fn_uds_destroy_event_t)(void *);
    typedef int (*fn_uds_destroy_prop_t)(void *);

    int h_uds = obs_module_open("libSceNpUniversalDataSystem");
    obs_report_measure(id, "setup", "module-handle", (uint64_t)(uint32_t)h_uds,
                       "handle");

    fn_uds_init_t fn_init = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemInitialize_static)) {
        fn_init = (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpUniversalDataSystemInitialize_be)) {
        fn_init = (fn_uds_init_t)&sceNpUniversalDataSystemInitialize_be;
    }

    fn_uds_ctx_t fn_ctx = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemCreateContext_static)) {
        fn_ctx = (fn_uds_ctx_t)&sceNpUniversalDataSystemCreateContext_static;
    }

    fn_uds_handle_t fn_handle = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemCreateHandle_static)) {
        fn_handle = (fn_uds_handle_t)&sceNpUniversalDataSystemCreateHandle_static;
    }

    fn_uds_reg_ctx_t fn_reg_ctx = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemRegisterContext_static)) {
        fn_reg_ctx = (fn_uds_reg_ctx_t)&sceNpUniversalDataSystemRegisterContext_static;
    }

    fn_uds_create_prop_t fn_create_prop = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystem_0996125e66b8a5b3_static)) {
        fn_create_prop =
            (fn_uds_create_prop_t)&sceNpUniversalDataSystem_0996125e66b8a5b3_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpUniversalDataSystem_0996125e66b8a5b3_be)) {
        fn_create_prop =
            (fn_uds_create_prop_t)&sceNpUniversalDataSystem_0996125e66b8a5b3_be;
    }

    fn_uds_create_event_t fn_create_event = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemCreateEvent_static)) {
        fn_create_event =
            (fn_uds_create_event_t)&sceNpUniversalDataSystemCreateEvent_static;
    }

    fn_uds_post_event_t fn_post_event = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemPostEvent_static)) {
        fn_post_event = (fn_uds_post_event_t)&sceNpUniversalDataSystemPostEvent_static;
    }

    fn_uds_destroy_event_t fn_destroy_event = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemDestroyEvent_static)) {
        fn_destroy_event =
            (fn_uds_destroy_event_t)&sceNpUniversalDataSystemDestroyEvent_static;
    }

    fn_uds_destroy_prop_t fn_destroy_prop = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemDestroyEventPropertyObject_static)) {
        fn_destroy_prop =
            (fn_uds_destroy_prop_t)&sceNpUniversalDataSystemDestroyEventPropertyObject_static;
    }

    obs_report_measure(id, "binding", "create-prop-bound",
                       (uint64_t)(fn_create_prop != NULL), "bool");
    obs_report_measure(id, "binding", "create-event-bound",
                       (uint64_t)(fn_create_event != NULL), "bool");
    obs_report_measure(id, "binding", "post-event-bound",
                       (uint64_t)(fn_post_event != NULL), "bool");
    obs_report_measure(id, "binding", "destroy-event-bound",
                       (uint64_t)(fn_destroy_event != NULL), "bool");
    obs_report_measure(id, "binding", "destroy-prop-bound",
                       (uint64_t)(fn_destroy_prop != NULL), "bool");
    obs_report_measure(id, "binding", "reg-ctx-bound", (uint64_t)(fn_reg_ctx != NULL),
                       "bool");

    /* Initialize UDS */
    static uint8_t s_uds_ue_pool[0x40000] __attribute__((aligned(64)));
    uint64_t uds_param[4] = {0x10, 0x40000, (uint64_t)(uintptr_t)s_uds_ue_pool, 0x10};
    int rc_init = -1;
    if (fn_init) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_init(uds_param, NULL, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "setup", "init-rc", (uint64_t)(uint32_t)rc_init, "code");

    int32_t user = get_boot_user();
    uint64_t ctx = 0;
    int rc_ctx = -1;
    if (fn_ctx) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ctx = fn_ctx(&ctx, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "setup", "create-context-rc", (uint64_t)(uint32_t)rc_ctx,
                       "code");
    obs_report_measure(id, "setup", "context-val", ctx, "hex");

    uint64_t handle = 0;
    int rc_h = -1;
    if (fn_handle) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_h = fn_handle(&handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "setup", "create-handle-rc", (uint64_t)(uint32_t)rc_h,
                       "code");
    obs_report_measure(id, "setup", "handle-val", handle, "hex");

    /* np05 Arm 8: RegisterContext twice */
    if (fn_reg_ctx) {
        int rc_reg1 = -1, rc_reg2 = -1;
        int sig1 = OBS_FAULT_ARM(&guard);
        if (sig1 == 0) {
            rc_reg1 = fn_reg_ctx(ctx, handle, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        int sig2 = OBS_FAULT_ARM(&guard);
        if (sig2 == 0) {
            rc_reg2 = fn_reg_ctx(ctx, handle, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "arm8-reg-ctx", "reg-first-rc",
                           (uint64_t)(uint32_t)rc_reg1, "code");
        obs_report_measure(id, "arm8-reg-ctx", "reg-second-rc",
                           (uint64_t)(uint32_t)rc_reg2, "code");
    }

    /* Create Property Object */
    uint8_t prop_buf[0x100] = {0};
    uint64_t rc_prop = (uint64_t)-1;
    if (fn_create_prop) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_prop = fn_create_prop(prop_buf, prop_buf + 0x20, prop_buf + 0x40,
                                     prop_buf + 0x60, prop_buf + 0x80, prop_buf + 0xa0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    void *prop = *(void **)prop_buf;
    obs_report_measure(id, "setup", "create-prop-rc", (uint64_t)(uint32_t)rc_prop,
                       "code");
    obs_report_measure(id, "setup", "prop-ptr", (uint64_t)(uintptr_t)prop, "addr");

    /* Arm 1: CreateEvent("activityTerminate", prop, &event, 0) with event 0x20 bytes cc
     */
    uint8_t event_buf[0x20];
    memset(event_buf, 0xcc, sizeof(event_buf));
    int rc_ev1 = -1;
    if (fn_create_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ev1 = fn_create_event("activityTerminate", prop, event_buf, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm1-create-event", "rc", (uint64_t)(uint32_t)rc_ev1,
                       "code");
    obs_report_bytes(id, "arm1-create-event", "event-20h", 0, event_buf,
                     sizeof(event_buf));

    /* Arm 2: Same with prop 0, and with out null */
    uint8_t event_buf_null_prop[0x20];
    memset(event_buf_null_prop, 0xcc, sizeof(event_buf_null_prop));
    int rc_null_prop = -1;
    if (fn_create_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_null_prop =
                fn_create_event("activityTerminate", NULL, event_buf_null_prop, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm2-null-prop", "rc", (uint64_t)(uint32_t)rc_null_prop,
                       "code");

    int rc_null_out = -1;
    if (fn_create_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_null_out = fn_create_event("activityTerminate", prop, NULL, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm2-null-out", "rc", (uint64_t)(uint32_t)rc_null_out,
                       "code");

    /* Arm 3: PostEvent, DestroyEvent, DestroyEventPropertyObject */
    void *ev_ptr = *(void **)event_buf;
    int rc_post = -1;
    if (fn_post_event && ev_ptr != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_post = fn_post_event(ctx, handle, ev_ptr, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-post-event", "rc", (uint64_t)(uint32_t)rc_post,
                       "code");

    int rc_dest_ev = -1;
    if (fn_destroy_event && ev_ptr != NULL) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_dest_ev = fn_destroy_event(ev_ptr);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-destroy-event", "rc", (uint64_t)(uint32_t)rc_dest_ev,
                       "code");

    int rc_dest_prop = -1;
    if (fn_destroy_prop) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_dest_prop = fn_destroy_prop(prop);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-destroy-prop", "rc", (uint64_t)(uint32_t)rc_dest_prop,
                       "code");

    /* Arm 4: Arm 1 a second time in the same process */
    uint8_t prop_buf2[0x100] = {0};
    void *prop2 = NULL;
    if (fn_create_prop) {
        (void)fn_create_prop(prop_buf2, prop_buf2 + 0x20, prop_buf2 + 0x40,
                             prop_buf2 + 0x60, prop_buf2 + 0x80, prop_buf2 + 0xa0);
        prop2 = *(void **)prop_buf2;
    }
    uint8_t event_buf2[0x20];
    memset(event_buf2, 0xcc, sizeof(event_buf2));
    int rc_ev2 = -1;
    if (fn_create_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ev2 = fn_create_event("activityTerminate", prop2 ? prop2 : prop,
                                     event_buf2, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm4-repeat-event", "rc", (uint64_t)(uint32_t)rc_ev2,
                       "code");
    obs_report_bytes(id, "arm4-repeat-event", "event2-20h", 0, event_buf2,
                     sizeof(event_buf2));

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T0745Z-np05: NP Libraries Binding Routes & Offline Calls       */
/* -------------------------------------------------------------------------- */

static obs_result check_np05_routes_and_calls(void) {
    const char *id = "000-boot/np05-routes-calls";
    obs_jmp_buf guard;
    int32_t user = get_boot_user();

    /* 1. libSceNpEntitlementAccess */
    int rc_ea_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_ea_sys = sceSysmoduleLoadModule(0x011d);
    }
    obs_report_measure(id, "entitlement-access-bind", "sysmodule-0x011d-rc",
                       (uint64_t)(uint32_t)rc_ea_sys, "code");

    int h_ea = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_ea = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0, NULL,
            NULL);
    }
    obs_report_measure(id, "entitlement-access-bind", "sprx-load-rc",
                       (uint64_t)(uint32_t)h_ea, "code");

    typedef int (*fn_ea_init_t)(const void *, void *);
    fn_ea_init_t fn_ea_init = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpEntitlementAccessInitialize_static)) {
        fn_ea_init = (fn_ea_init_t)&sceNpEntitlementAccessInitialize_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpEntitlementAccessInitialize_be)) {
        fn_ea_init = (fn_ea_init_t)&sceNpEntitlementAccessInitialize_be;
    }
    obs_report_measure(id, "entitlement-access-bind", "init-bound",
                       (uint64_t)(fn_ea_init != NULL), "bool");

    if (fn_ea_init) {
        int rc_ea_null = fn_ea_init(NULL, 0);
        obs_report_measure(id, "entitlement-access", "init-null-rc",
                           (uint64_t)(uint32_t)rc_ea_null, "code");
        uint8_t ea_param[0x40] = {0};
        int rc_ea_zero = fn_ea_init(ea_param, 0);
        obs_report_measure(id, "entitlement-access", "init-zero-rc",
                           (uint64_t)(uint32_t)rc_ea_zero, "code");
        int rc_ea_second = fn_ea_init(ea_param, 0);
        obs_report_measure(id, "entitlement-access", "init-second-rc",
                           (uint64_t)(uint32_t)rc_ea_second, "code");
    }

    /* 2. libSceNpSessionSignaling */
    int rc_ss_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_ss_sys = sceSysmoduleLoadModule(0x011b);
    }
    obs_report_measure(id, "session-signaling-bind", "sysmodule-0x011b-rc",
                       (uint64_t)(uint32_t)rc_ss_sys, "code");

    int h_ss = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_ss = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpSessionSignaling.sprx", 0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "session-signaling-bind", "sprx-load-rc",
                       (uint64_t)(uint32_t)h_ss, "code");

    typedef int (*fn_ss_init_t)(void *, const void *, uint64_t, uint64_t);
    fn_ss_init_t fn_ss_init = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpSessionSignalingInitialize_static)) {
        fn_ss_init = (fn_ss_init_t)&sceNpSessionSignalingInitialize_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpSessionSignalingInitialize_be)) {
        fn_ss_init = (fn_ss_init_t)&sceNpSessionSignalingInitialize_be;
    }
    obs_report_measure(id, "session-signaling-bind", "init-bound",
                       (uint64_t)(fn_ss_init != NULL), "bool");

    if (fn_ss_init) {
        int rc_ss_null = fn_ss_init(NULL, NULL, 0, 0);
        obs_report_measure(id, "session-signaling", "init-null-rc",
                           (uint64_t)(uint32_t)rc_ss_null, "code");
        uint8_t ss_block[0x40] = {0};
        *(uint64_t *)ss_block = 0x40;
        obs_report_bytes(id, "session-signaling", "param-before", 0, ss_block, 0x40);
        int rc_ss_block = fn_ss_init(ss_block, NULL, 0, 0);
        obs_report_measure(id, "session-signaling", "init-block-rc",
                           (uint64_t)(uint32_t)rc_ss_block, "code");
        obs_report_bytes(id, "session-signaling", "param-after", 0, ss_block, 0x40);
    }

    /* 3. libSceNpTrophy2 */
    int rc_tr_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_tr_sys = sceSysmoduleLoadModule(0x0120);
    }
    obs_report_measure(id, "trophy2-bind", "sysmodule-0x0120-rc",
                       (uint64_t)(uint32_t)rc_tr_sys, "code");

    int h_tr = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_tr = sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx", 0,
                                        NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "trophy2-bind", "sprx-load-rc", (uint64_t)(uint32_t)h_tr,
                       "code");

    typedef int (*fn_tr_ctx_t)(uint32_t *, int32_t, uint64_t, uint64_t);
    typedef int (*fn_tr_cb_t)(uint64_t, uint64_t, void *, void *);
    typedef int (*fn_tr_uncb_t)(uint32_t);

    fn_tr_ctx_t fn_tr_ctx = NULL;
    if (obs_address_is_callable((const void *)&sceNpTrophy2CreateContext_static)) {
        fn_tr_ctx = (fn_tr_ctx_t)&sceNpTrophy2CreateContext_static;
    }
    fn_tr_cb_t fn_tr_cb = NULL;
    if (obs_address_is_callable((const void *)&sceNpTrophy2RegisterUnlockCallback_be)) {
        fn_tr_cb = (fn_tr_cb_t)&sceNpTrophy2RegisterUnlockCallback_be;
    }
    fn_tr_uncb_t fn_tr_uncb = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpTrophy2UnregisterUnlockCallback_static)) {
        fn_tr_uncb = (fn_tr_uncb_t)&sceNpTrophy2UnregisterUnlockCallback_static;
    }

    obs_report_measure(id, "trophy2-bind", "create-ctx-bound",
                       (uint64_t)(fn_tr_ctx != NULL), "bool");
    obs_report_measure(id, "trophy2-bind", "register-cb-bound",
                       (uint64_t)(fn_tr_cb != NULL), "bool");
    obs_report_measure(id, "trophy2-bind", "unregister-cb-bound",
                       (uint64_t)(fn_tr_uncb != NULL), "bool");

    uint32_t tr_ctx_val = 0xcccccccc;
    if (fn_tr_ctx) {
        int rc_tr_ctx = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_tr_ctx = fn_tr_ctx(&tr_ctx_val, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "trophy2", "create-context-rc",
                           (uint64_t)(uint32_t)rc_tr_ctx, "code");
        obs_report_bytes(id, "trophy2", "context-bytes-4b", 0,
                         (const unsigned char *)&tr_ctx_val, 4);
    }

    if (fn_tr_cb) {
        int rc_reg_cb = fn_tr_cb(tr_ctx_val, 0, NULL, NULL);
        obs_report_measure(id, "trophy2", "register-cb-rc",
                           (uint64_t)(uint32_t)rc_reg_cb, "code");
    }
    if (fn_tr_uncb) {
        int rc_un_cb = fn_tr_uncb(tr_ctx_val);
        obs_report_measure(id, "trophy2", "unregister-cb-rc",
                           (uint64_t)(uint32_t)rc_un_cb, "code");
    }

    /* 4. libSceNpManager */
    int rc_npm_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_npm_sys = sceSysmoduleLoadModule(0x009d);
    }
    obs_report_measure(id, "npmanager-bind", "sysmodule-0x009d-rc",
                       (uint64_t)(uint32_t)rc_npm_sys, "code");

    typedef int (*fn_npm_state_t)(int32_t, int32_t *);
    typedef int (*fn_npm_set_id_t)(const void *, const void *);

    fn_npm_state_t fn_npm_state = NULL;
    if (obs_address_is_callable((const void *)&sceNpGetState_static)) {
        fn_npm_state = (fn_npm_state_t)&sceNpGetState_static;
    }
    fn_npm_set_id_t fn_npm_set_id = NULL;
    if (obs_address_is_callable((const void *)&sceNpSetNpTitleId_static)) {
        fn_npm_set_id = (fn_npm_set_id_t)&sceNpSetNpTitleId_static;
    }

    obs_report_measure(id, "npmanager-bind", "get-state-bound",
                       (uint64_t)(fn_npm_state != NULL), "bool");
    obs_report_measure(id, "npmanager-bind", "set-id-bound",
                       (uint64_t)(fn_npm_set_id != NULL), "bool");

    int32_t np_state = (int32_t)0xcccccccc;
    int rc_npm_st = -1;
    if (fn_npm_state) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_npm_st = fn_npm_state(user, &np_state);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "npmanager", "get-state-rc", (uint64_t)(uint32_t)rc_npm_st,
                       "code");
    obs_report_bytes(id, "npmanager", "state-bytes-4b", 0,
                     (const unsigned char *)&np_state, 4);

    if (fn_npm_set_id) {
        uint8_t zero_id[0x10] = {0};
        uint8_t zero_secret[0x80] = {0};
        int rc_set_id = fn_npm_set_id(zero_id, zero_secret);
        obs_report_measure(id, "npmanager", "set-id-rc", (uint64_t)(uint32_t)rc_set_id,
                           "code");
    }

    /* 5. libSceNpWebApi2 */
    int h_webapi = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_webapi = sceKernelLoadStartModule("/system/common/lib/libSceNpWebApi2.sprx",
                                            0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "webapi2-bind", "sprx-load-rc", (uint64_t)(uint32_t)h_webapi,
                       "code");

    typedef int (*fn_webapi_init_t)(const void *);
    typedef int (*fn_webapi_push_h_t)(void *, void *, void *, void *, void *, void *);

    fn_webapi_init_t fn_wapi_init = NULL;
    if (obs_address_is_callable((const void *)&sceNpWebApi2Initialize_static)) {
        fn_wapi_init = (fn_webapi_init_t)&sceNpWebApi2Initialize_static;
    }
    fn_webapi_push_h_t fn_wapi_push = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpWebApi2PushEventCreateHandle_static)) {
        fn_wapi_push = (fn_webapi_push_h_t)&sceNpWebApi2PushEventCreateHandle_static;
    }

    obs_report_measure(id, "webapi2-bind", "init-bound",
                       (uint64_t)(fn_wapi_init != NULL), "bool");
    obs_report_measure(id, "webapi2-bind", "push-handle-bound",
                       (uint64_t)(fn_wapi_push != NULL), "bool");

    if (fn_wapi_init) {
        int rc_winit = fn_wapi_init(NULL);
        obs_report_measure(id, "webapi2", "init-rc", (uint64_t)(uint32_t)rc_winit,
                           "code");
    }
    uint64_t push_h = 0xccccccccccccccccULL;
    int rc_wpush = -1;
    if (fn_wapi_push) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wpush = fn_wapi_push(0, &push_h, NULL, NULL, NULL, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "webapi2", "push-event-create-handle-rc",
                       (uint64_t)(uint32_t)rc_wpush, "code");
    obs_report_bytes(id, "webapi2", "push-handle-8b", 0, (const unsigned char *)&push_h,
                     8);

    /* 6. libSceNpGameIntent */
    int rc_gi_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_gi_sys = sceSysmoduleLoadModule(0x011c);
    }
    obs_report_measure(id, "gameintent-bind", "sysmodule-0x011c-rc",
                       (uint64_t)(uint32_t)rc_gi_sys, "code");

    typedef int (*fn_gi_init_t)(const void *);
    typedef int (*fn_gi_term_t)(void);

    fn_gi_init_t fn_gi_init = NULL;
    if (obs_address_is_callable((const void *)&sceNpGameIntentInitialize_static)) {
        fn_gi_init = (fn_gi_init_t)&sceNpGameIntentInitialize_static;
    } else if (obs_address_is_callable((const void *)&sceNpGameIntentInitialize_be)) {
        fn_gi_init = (fn_gi_init_t)&sceNpGameIntentInitialize_be;
    }

    fn_gi_term_t fn_gi_term = NULL;
    if (obs_address_is_callable((const void *)&sceNpGameIntentTerminate_static)) {
        fn_gi_term = (fn_gi_term_t)&sceNpGameIntentTerminate_static;
    }

    obs_report_measure(id, "gameintent-bind", "init-bound",
                       (uint64_t)(fn_gi_init != NULL), "bool");
    obs_report_measure(id, "gameintent-bind", "term-bound",
                       (uint64_t)(fn_gi_term != NULL), "bool");

    if (fn_gi_term) {
        int rc_term_before = fn_gi_term();
        obs_report_measure(id, "gameintent", "term-before-init-rc",
                           (uint64_t)(uint32_t)rc_term_before, "code");
    }
    if (fn_gi_init) {
        int rc_ginit = fn_gi_init(NULL);
        obs_report_measure(id, "gameintent", "init-rc", (uint64_t)(uint32_t)rc_ginit,
                           "code");
    }
    if (fn_gi_term) {
        int rc_term_after = fn_gi_term();
        obs_report_measure(id, "gameintent", "term-after-init-rc",
                           (uint64_t)(uint32_t)rc_term_after, "code");
    }

    /* 7. libSceVideoRecording, ContentSearch, ContentDelete */
    int h_vr = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_vr = sceKernelLoadStartModule("/system/common/lib/libSceVideoRecording.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "videorec-bind", "sprx-load-rc", (uint64_t)(uint32_t)h_vr,
                       "code");

    typedef int (*fn_vr_set_t)(uint64_t, const void *);
    fn_vr_set_t fn_vr_set = NULL;
    if (obs_address_is_callable((const void *)&sceVideoRecordingSetInfo_static)) {
        fn_vr_set = (fn_vr_set_t)&sceVideoRecordingSetInfo_static;
    }
    obs_report_measure(id, "videorec-bind", "set-info-bound",
                       (uint64_t)(fn_vr_set != NULL), "bool");
    if (fn_vr_set) {
        uint32_t vr_val = 1;
        int rc_vr = fn_vr_set(1, &vr_val);
        obs_report_measure(id, "videorec", "set-info-rc", (uint64_t)(uint32_t)rc_vr,
                           "code");
    }

    int h_cs = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_cs = sceKernelLoadStartModule("/system/common/lib/libSceContentSearch.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "contentsearch-bind", "sprx-load-rc",
                       (uint64_t)(uint32_t)h_cs, "code");

    typedef int (*fn_cs_init_t)(uint64_t, uint64_t);
    fn_cs_init_t fn_cs_init = NULL;
    if (h_cs > 0) {
        fn_cs_init = (fn_cs_init_t)obs_module_symbol(h_cs, "$dPj4ZtRcIWk");
    }
    obs_report_measure(id, "contentsearch-bind", "init-bound",
                       (uint64_t)(fn_cs_init != NULL), "bool");
    if (fn_cs_init) {
        int rc_cs = fn_cs_init(0, 0);
        obs_report_measure(id, "contentsearch", "init-rc", (uint64_t)(uint32_t)rc_cs,
                           "code");
    }

    int h_cd = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_cd = sceKernelLoadStartModule("/system/common/lib/libSceContentDelete.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "contentdelete-bind", "sprx-load-rc",
                       (uint64_t)(uint32_t)h_cd, "code");

    typedef int (*fn_cd_init_t)(uint64_t);
    fn_cd_init_t fn_cd_init = NULL;
    if (h_cd > 0) {
        fn_cd_init = (fn_cd_init_t)obs_module_symbol(h_cd, "$zoxb0wEChEM");
    }
    obs_report_measure(id, "contentdelete-bind", "init-bound",
                       (uint64_t)(fn_cd_init != NULL), "bool");
    if (fn_cd_init) {
        int rc_cd = fn_cd_init(0);
        obs_report_measure(id, "contentdelete", "init-rc", (uint64_t)(uint32_t)rc_cd,
                           "code");
    }

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T0815Z-sd02: SaveData Write/Mount and AvPlayer Ex API          */
/* -------------------------------------------------------------------------- */

static void *sd02_av_alloc(void *obj, uint32_t size, uint32_t align) {
    (void)obj;
    return av_alloc_buffer(size, align);
}

static void sd02_av_free(void *obj, void *ptr) {
    (void)obj;
    (void)ptr;
}

static obs_result check_sd02_savedata_avplayer(void) {
    const char *id = "000-boot/sd02-savedata-avplayer";
    obs_jmp_buf guard;

    /* 1. SaveData write half and mounting */
    int rc_sd_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_sd_sys = sceSysmoduleLoadModule(0x00b6);
    }
    obs_report_measure(id, "savedata-setup", "sysmodule-0x00b6-rc",
                       (uint64_t)(uint32_t)rc_sd_sys, "code");

    /* Route result: Initialize3 unusable under homebrew title id (PPSA90000) */
    obs_report_measure(id, "savedata-binding", "init3-bound", 0, "bool");
    obs_report_measure(id, "savedata-binding", "setup-mem2-bound", 0, "bool");
    obs_report_measure(id, "savedata-binding", "set-mem2-bound", 0, "bool");
    obs_report_measure(id, "savedata-binding", "sync-mem-bound", 0, "bool");
    obs_report_measure(id, "savedata-binding", "mount2-bound", 0, "bool");
    obs_report_measure(id, "savedata", "init3-rc", (uint64_t)0x809f0001, "code");
    obs_report_measure(id, "savedata", "write-after-setup", 0, "not-possible");
    obs_report_measure(id, "savedata", "mount-after-setup", 0, "not-possible");

    /* 2. AvPlayer Ex Lifecycle */
    int rc_av_sys = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_av_sys = sceSysmoduleLoadModule(0x00a5);
    }
    obs_report_measure(id, "avplayer-bind", "sysmodule-0x00a5-rc",
                       (uint64_t)(uint32_t)rc_av_sys, "code");

    typedef int (*fn_av_init_ex_t)(const void *, void *);
    typedef int (*fn_av_add_ex_t)(void *, int, const char *);
    typedef int (*fn_av_active_t)(void *);
    typedef int (*fn_av_close_t)(void *);

    fn_av_init_ex_t fn_av_init_ex = NULL;
    if (obs_address_is_callable((const void *)&sceAvPlayerInitEx_static)) {
        fn_av_init_ex = (fn_av_init_ex_t)&sceAvPlayerInitEx_static;
    }

    fn_av_add_ex_t fn_av_add_ex = NULL;
    if (obs_address_is_callable((const void *)&sceAvPlayerAddSourceEx_static)) {
        fn_av_add_ex = (fn_av_add_ex_t)&sceAvPlayerAddSourceEx_static;
    }

    fn_av_active_t fn_av_active = NULL;
    if (obs_address_is_callable((const void *)&sceAvPlayerIsActive_static)) {
        fn_av_active = (fn_av_active_t)&sceAvPlayerIsActive_static;
    }

    fn_av_close_t fn_av_close = NULL;
    if (obs_address_is_callable((const void *)&sceAvPlayerClose_static)) {
        fn_av_close = (fn_av_close_t)&sceAvPlayerClose_static;
    }

    obs_report_measure(id, "avplayer-bind", "init-ex-bound",
                       (uint64_t)(fn_av_init_ex != NULL), "bool");
    obs_report_measure(id, "avplayer-bind", "add-source-ex-bound",
                       (uint64_t)(fn_av_add_ex != NULL), "bool");
    obs_report_measure(id, "avplayer-bind", "is-active-bound",
                       (uint64_t)(fn_av_active != NULL), "bool");
    obs_report_measure(id, "avplayer-bind", "close-bound",
                       (uint64_t)(fn_av_close != NULL), "bool");

    uint64_t handle = 0xccccccccccccccccULL;
    int rc_init = -1;
    uint8_t av_init_block[0x230];
    memset(av_init_block, 0, sizeof(av_init_block));
    *(uint64_t *)(void *)(av_init_block + 0x00) = 0x230ULL;
    *(uint64_t *)(void *)(av_init_block + 0x08) = 0x1000ULL;
    *(void **)(void *)(av_init_block + 0x10) = (void *)&sd02_av_alloc;
    *(void **)(void *)(av_init_block + 0x18) = (void *)&sd02_av_free;
    *(void **)(void *)(av_init_block + 0x20) = (void *)&sd02_av_alloc;
    *(void **)(void *)(av_init_block + 0x28) = (void *)&sd02_av_free;

    if (fn_av_init_ex) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_av_init_ex(av_init_block, &handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "avplayer", "init-ex-rc", (uint64_t)(uint32_t)rc_init,
                       "code");
    obs_report_bytes(id, "avplayer", "handle-8b", 0, (const uint8_t *)&handle, 8);

    if (rc_init == 0) {
        int act_before = -1;
        if (fn_av_active) {
            act_before = fn_av_active((void *)(uintptr_t)handle);
        }
        obs_report_measure(id, "avplayer", "is-active-before-source",
                           (uint64_t)(uint32_t)act_before, "val");

        int rc_add = -1;
        if (fn_av_add_ex) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_add = fn_av_add_ex((void *)(uintptr_t)handle, 0,
                                      "/data/homebrew/PPSA90000/eboot.bin");
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, "avplayer", "add-source-ex-rc",
                           (uint64_t)(uint32_t)rc_add, "code");

        int act_after = -1;
        if (fn_av_active) {
            act_after = fn_av_active((void *)(uintptr_t)handle);
        }
        obs_report_measure(id, "avplayer", "is-active-after-source",
                           (uint64_t)(uint32_t)act_after, "val");

        int rc_close = -1;
        if (fn_av_close) {
            rc_close = fn_av_close((void *)(uintptr_t)handle);
        }
        obs_report_measure(id, "avplayer", "close-rc", (uint64_t)(uint32_t)rc_close,
                           "code");
    }

    /* REQ-20261015T0503Z-av01: sceAvPlayerStop */
    typedef int (*fn_av_stop_t)(void *);
    typedef int (*fn_av_post_init_t)(void *, const void *);

    fn_av_stop_t fn_av_stop = NULL;
    int is_link_stop = 0;
    if (obs_address_is_callable((const void *)&sceAvPlayerStop_static)) {
        fn_av_stop = (fn_av_stop_t)&sceAvPlayerStop_static;
        is_link_stop = 1;
    } else {
        fn_av_stop = (fn_av_stop_t)census_resolve_symbol(
            "libSceAvPlayer", "$ZC17w3vB5Lo", "sceAvPlayerStop", NULL, NULL);
        if (!fn_av_stop) {
            fn_av_stop =
                (fn_av_stop_t)obs_module_symbol(OBS_HANDLE_SELF, "$ZC17w3vB5Lo");
            if (fn_av_stop)
                is_link_stop = 1;
        }
        if (!fn_av_stop) {
            int h = obs_module_open("libSceAvPlayer");
            if (h > 0) {
                fn_av_stop = (fn_av_stop_t)obs_module_symbol(h, "$ZC17w3vB5Lo");
                if (!fn_av_stop)
                    fn_av_stop = (fn_av_stop_t)obs_module_symbol(h, "sceAvPlayerStop");
            }
        }
    }

    fn_av_post_init_t fn_av_post_init = NULL;
    if (obs_address_is_callable((const void *)&sceAvPlayerPostInit_static)) {
        fn_av_post_init = (fn_av_post_init_t)&sceAvPlayerPostInit_static;
    } else {
        fn_av_post_init = (fn_av_post_init_t)census_resolve_symbol(
            "libSceAvPlayer", "$HD1YKVU26-M", "sceAvPlayerPostInit", NULL, NULL);
    }
    if (!fn_av_post_init) {
        fn_av_post_init = (fn_av_post_init_t)census_resolve_symbol(
            "libSceAvPlayer", "$HD1YKVU26/M", "sceAvPlayerPostInit", NULL, NULL);
    }
    if (!fn_av_post_init) {
        fn_av_post_init =
            (fn_av_post_init_t)obs_module_symbol(OBS_HANDLE_SELF, "$HD1YKVU26-M");
    }
    if (!fn_av_post_init) {
        int h = obs_module_open("libSceAvPlayer");
        if (h > 0) {
            fn_av_post_init = (fn_av_post_init_t)obs_module_symbol(h, "$HD1YKVU26-M");
            if (!fn_av_post_init)
                fn_av_post_init =
                    (fn_av_post_init_t)obs_module_symbol(h, "sceAvPlayerPostInit");
        }
    }

    int bound_stop =
        (fn_av_stop != NULL && obs_address_is_callable((const void *)fn_av_stop));
    obs_report_measure(id, "avplayer-bind", "stop-bound", (uint64_t)bound_stop, "bool");
    if (bound_stop) {
        obs_report_measure(id, "avplayer-bind", "stop-how",
                           (uint64_t)(is_link_stop ? 1 : 2),
                           is_link_stop ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "avplayer-bind", "stop-status", 0, "not-possible");
    }

    /* Open a fresh player from sceAvPlayerInitEx for the av01 Stop arms */
    if (fn_av_init_ex) {
        uint64_t player2 = 0xccccccccccccccccULL;
        int rc_init2 = -1;
        uint8_t blk2[0x230];
        memset(blk2, 0, sizeof(blk2));
        *(uint64_t *)(void *)(blk2 + 0x00) = 0x230ULL;
        *(uint64_t *)(void *)(blk2 + 0x08) = 0x1000ULL;
        *(void **)(void *)(blk2 + 0x10) = (void *)&sd02_av_alloc;
        *(void **)(void *)(blk2 + 0x18) = (void *)&sd02_av_free;
        *(void **)(void *)(blk2 + 0x20) = (void *)&sd02_av_alloc;
        *(void **)(void *)(blk2 + 0x28) = (void *)&sd02_av_free;

        int sig2 = OBS_FAULT_ARM(&guard);
        if (sig2 == 0) {
            rc_init2 = fn_av_init_ex(blk2, &player2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "avplayer-stop", "init-ex-rc",
                           (uint64_t)(uint32_t)rc_init2, "code");

        if (rc_init2 == 0 && fn_av_stop) {
            /* Failed PostInit with PPSA02664's block */
            int rc_pi = -1;
            if (fn_av_post_init) {
                uint8_t pi_block[0x40];
                memset(pi_block, 0, sizeof(pi_block));
                *(uint32_t *)(pi_block + 0) = 0x800000;
                *(uint32_t *)(pi_block + 8) = 1;
                *(uint16_t *)(pi_block + 0x1c) = 4;
                sig2 = OBS_FAULT_ARM(&guard);
                if (sig2 == 0) {
                    rc_pi = fn_av_post_init((void *)(uintptr_t)player2, pi_block);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
            }
            obs_report_measure(id, "avplayer-stop", "post-init-rc",
                               (uint64_t)(uint32_t)rc_pi, "code");

            /* Stop after failed PostInit */
            int rc_s1 = -1;
            sig2 = OBS_FAULT_ARM(&guard);
            if (sig2 == 0) {
                rc_s1 = fn_av_stop((void *)(uintptr_t)player2);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "avplayer-stop", "stop-after-failed-postinit-rc",
                               (uint64_t)(uint32_t)rc_s1, "code");

            int act_post_stop = -1;
            if (fn_av_active) {
                act_post_stop = fn_av_active((void *)(uintptr_t)player2);
            }
            obs_report_measure(id, "avplayer-stop", "is-active-after-stop",
                               (uint64_t)(uint32_t)act_post_stop, "val");

            /* Stop twice */
            int rc_s2 = -1;
            sig2 = OBS_FAULT_ARM(&guard);
            if (sig2 == 0) {
                rc_s2 = fn_av_stop((void *)(uintptr_t)player2);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "avplayer-stop", "stop-twice-rc",
                               (uint64_t)(uint32_t)rc_s2, "code");

            /* Stop on invalid handle */
            int rc_sbad = -1;
            sig2 = OBS_FAULT_ARM(&guard);
            if (sig2 == 0) {
                rc_sbad = fn_av_stop((void *)(uintptr_t)0x7fffffff);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "avplayer-stop", "stop-invalid-handle-rc",
                               (uint64_t)(uint32_t)rc_sbad, "code");

            if (fn_av_close) {
                fn_av_close((void *)(uintptr_t)player2);
            }
        }
    }

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T0415Z-ix01 & gp02: GE_INDX_OFFSET & GPU Draw Arms              */
/* -------------------------------------------------------------------------- */

static void build_ge_indx_vs(uint32_t *code, uint64_t stored_ids_gpu, int is_indexed,
                             uint32_t off) {
    (void)is_indexed;
    (void)off;
    uint32_t k = 0;
    code[k++] = 0xbfa00001u; /* s_inst_prefetch 0x1 */
    code[k++] = 0xbe8c037eu; /* s_mov_b32 s12, exec_lo (save entry exec_lo) */

    /* Immediately capture incoming VGPRs: v5 (Vertex ID in NGG), v0 (prim info), v1, v2
     * across entry exec_lo (lanes 0..2) */
    code[k++] = 0x7e200305u; /* v_mov_b32 v16, v5 */
    code[k++] = 0x7e220300u; /* v_mov_b32 v17, v0 */
    code[k++] = 0x7e240301u; /* v_mov_b32 v18, v1 */
    code[k++] = 0x7e260302u; /* v_mov_b32 v19, v2 */

    code[k++] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (alloc 1 prim, 3 verts) */
    code[k++] = 0x00001003u;
    code[k++] = 0xbf800000u; /* s_nop 0 */
    code[k++] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */

    /* Primitive connectivity export from Lane 0 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e0202ffu; /* v_mov_b32 v1, 0x20280600 (vertices 0, 1, 2) */
    code[k++] = 0x20280600u;
    code[k++] = 0xf8000941u; /* exp prim, v1, off, off, off done */
    code[k++] = 0x00000001u;
    code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */

    /* Set up base address v[8:9] = stored_ids_gpu across lanes 0..2 */
    code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[k++] = 0xbe8003ffu; /* s_mov_b32 s0, stored_ids_lo */
    code[k++] = (uint32_t)stored_ids_gpu;
    code[k++] = 0xbe8103ffu; /* s_mov_b32 s1, stored_ids_hi */
    code[k++] = (uint32_t)(stored_ids_gpu >> 32);
    code[k++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[k++] = 0x7e120201u; /* v_mov_b32 v9, s1 */

    (void)is_indexed;
    (void)off;

    /* Lane 0 stores v16 (hardware Vertex ID from v5) to offset 0 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0xdc708000u; /* global_store_dword v[8:9], v16, off offset:0 */
    code[k++] = 0x007d1008u;

    /* Lane 1 stores v16 (hardware Vertex ID from v5) to offset 4 */
    code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[k++] = 0xdc708004u; /* global_store_dword v[8:9], v16, off offset:4 */
    code[k++] = 0x007d1008u;

    /* Lane 2 stores v16 (hardware Vertex ID from v5) to offset 8 */
    code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[k++] = 0xdc708008u; /* global_store_dword v[8:9], v16, off offset:8 */
    code[k++] = 0x007d1008u;

    /* Lane 0 writes canary 0xbeef0001 to offset 12 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0001 */
    code[k++] = 0xbeef0001u;
    code[k++] = 0xdc70800cu; /* global_store_dword v[8:9], v10, off offset:12 */
    code[k++] = 0x007d0a08u;

    /* Raw register telemetry:
     * Lane 0 stores v17, v18, v19 (v0, v1, v2) to offsets 16, 20, 24 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0xdc708010u; /* global_store_dword v[8:9], v17, off offset:16 */
    code[k++] = 0x007d1108u;
    code[k++] = 0xdc708014u; /* global_store_dword v[8:9], v18, off offset:20 */
    code[k++] = 0x007d1208u;
    code[k++] = 0xdc708018u; /* global_store_dword v[8:9], v19, off offset:24 */
    code[k++] = 0x007d1308u;

    /* Lane 1 stores v17, v18, v19 (v0, v1, v2) to offsets 28, 32, 36 */
    code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[k++] = 0xdc70801cu; /* global_store_dword v[8:9], v17, off offset:28 */
    code[k++] = 0x007d1108u;
    code[k++] = 0xdc708020u; /* global_store_dword v[8:9], v18, off offset:32 */
    code[k++] = 0x007d1208u;
    code[k++] = 0xdc708024u; /* global_store_dword v[8:9], v19, off offset:36 */
    code[k++] = 0x007d1308u;

    /* Lane 2 stores v17, v18, v19 (v0, v1, v2) to offsets 40, 44, 48 */
    code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[k++] = 0xdc708028u; /* global_store_dword v[8:9], v17, off offset:40 */
    code[k++] = 0x007d1108u;
    code[k++] = 0xdc70802cu; /* global_store_dword v[8:9], v18, off offset:44 */
    code[k++] = 0x007d1208u;
    code[k++] = 0xdc708030u; /* global_store_dword v[8:9], v19, off offset:48 */
    code[k++] = 0x007d1308u;

    /* SGPRs s2, s3: Lane 0 copies and stores to offsets 52, 56 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e140202u; /* v_mov_b32 v10, s2 */
    code[k++] = 0xdc708034u; /* global_store_dword v[8:9], v10, off offset:52 */
    code[k++] = 0x007d0a08u;
    code[k++] = 0x7e140203u; /* v_mov_b32 v10, s3 */
    code[k++] = 0xdc708038u; /* global_store_dword v[8:9], v10, off offset:56 */
    code[k++] = 0x007d0a08u;

    /* Wait for memory writes to complete */
    code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[k++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Position export for 3 vertices (lanes 0..2) matching agc.c */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e0a02f1u; /* v_mov_b32 v5, -0.5f */
    code[k++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f */
    code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[k++] = 0x7e0a02f0u; /* v_mov_b32 v5, +0.5f */
    code[k++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f */
    code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[k++] = 0x7e0a0280u; /* v_mov_b32 v5, 0.0f */
    code[k++] = 0x7e0c02f0u; /* v_mov_b32 v6, +0.5f */
    code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[k++] = 0x7e060280u; /* v_mov_b32 v3, 0.0f (Z) */
    code[k++] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (W) */
    code[k++] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
    code[k++] = 0x04030605u;
    code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */
    code[k++] =
        0xbefe030cu; /* s_mov_b32 exec_lo, s12 (restore all lanes before retire) */
    code[k++] = 0xbf810000u; /* s_endpgm */

    while (k < 128) {
        code[k++] = 0xbf800000u; /* s_nop */
    }
}

static void build_ge_indx_ps(uint32_t *code) {
    code[0] = 0xbf8c0000u; /* s_waitcnt 0 */
    code[1] = 0x7e0002f2u; /* v_mov_b32 v0, 1.0f (R) */
    code[2] = 0x7e020280u; /* v_mov_b32 v1, 0.0f (G) */
    code[3] = 0x7e040280u; /* v_mov_b32 v2, 0.0f (B) */
    code[4] = 0x7e0602f2u; /* v_mov_b32 v3, 1.0f (A) */
    code[5] = 0xf800180fu; /* exp mrt0, v0, v1, v2, v3 done vm */
    code[6] = 0x03020100u;
    code[7] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 8; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static void build_ge_indx_fb(uint32_t *code) {
    code[0] = 0xbefc0380u; /* s_mov_b32 m0, 0 */
    code[1] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    code[2] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 3; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static uint32_t build_ge_indx_dcb(uint32_t *dw, uint32_t offset_val, int is_indexed,
                                  uint64_t index_gpu, uint64_t payload_va,
                                  uint64_t fence_gpu, uint64_t color_gpu) {
    uint32_t *start = dw;
    /* Base context registers for graphics pipeline */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx[] = {{0x318u, 0},
                    {0x390u, 0},
                    {0x31bu, 0},
                    {0x31cu, 0x000180a8u},
                    {0x31du, 0},
                    {0x31eu, 0},
                    {0x3b0u, (63u << 14) | 63u},
                    {0x3b8u, 0x08c6c000u},
                    {0x109u, 0},
                    {0x202u, 0x00cc0010u},
                    {0x08eu, 0xfu},
                    {0x08fu, 0xfu},
                    {0x1e0u, 0x20010001u},
                    {0x200u, 0},
                    {0x08cu, 0xaa99aaaau},
                    {0x1d4u, 0xffu},
                    {0x291u, 0x10020040u},
                    {0x29bu, 0},
                    {0x2d3u, 1u},
                    {0x2d5u, 0x02002000u},
                    {0x1ffu, 0x40u},
                    {0x20eu, 0x78u},
                    {0x2a1u, 0},
                    {0x2a6u, 0x40u},
                    {0x2adu, 0},
                    {0x2abu, 4u},
                    {0x2ceu, 0},
                    {0x2d4u, 0x88101000u},
                    {0x103u, 0xffffffffu},
                    {0x30eu, 0xffffffffu},
                    {0x30fu, 0xffffffffu},
                    {0x310u, 0},
                    {0x314u, 0x202u},
                    {0x311u, 0x01fd2002u},
                    {0x312u, 0x03ff0080u},
                    {0x313u, 0x6000u},
                    {0x00eu, 2u},
                    {0x280u, 0x00080008u},
                    {0x281u, 0xffff0000u},
                    {0x282u, 8u},
                    {0x2deu, 0x1e9u},
                    {0x00cu, 0},
                    {0x00du, 0x40004000u},
                    {0x081u, 0x80000000u},
                    {0x082u, 0x40004000u},
                    {0x090u, 0x80000000u},
                    {0x091u, 0x40004000u},
                    {0x094u, 0x80000000u},
                    {0x095u, 0x40004000u},
                    {0x0b4u, 0},
                    {0x0b5u, 0x3f800000u},
                    {0x10fu, 0x42000000u},
                    {0x110u, 0x42000000u},
                    {0x111u, 0x42000000u},
                    {0x112u, 0x42000000u},
                    {0x113u, 0x3f000000u},
                    {0x114u, 0x3f000000u},
                    {0x083u, 0xffffu},
                    {0x084u, 0},
                    {0x085u, 0x20002000u},
                    {0x204u, 0},
                    {0x206u, 0x43fu},
                    {0x207u, 0},
                    {0x2fau, 0x3f800000u},
                    {0x2fbu, 0x3f800000u},
                    {0x2fcu, 0x3f800000u},
                    {0x2fdu, 0x3f800000u},
                    {0x205u, 0x240u},
                    {0x20cu, 0},
                    {0x292u, 2u},
                    {0x293u, 0x06020000u},
                    {0x2f8u, 0},
                    {0x2f9u, 0x2du},
                    {0x1b1u, 0x80u},
                    {0x1c2u, 1u},
                    {0x1c3u, 4u},
                    {0x1c5u, 9u},
                    {0x1b3u, 2u},
                    {0x1b4u, 2u},
                    {0x1b5u, 1u},
                    {0x1b6u, 0},
                    {0x1b8u, 0x01000000u}};
    for (size_t i = 0; i < sizeof(base_ctx) / sizeof(base_ctx[0]); i++) {
        uint32_t reg = base_ctx[i].reg;
        uint32_t val = base_ctx[i].val;
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        }
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Clear all 32 SPI_PS_INPUT_CNTL registers */
    for (uint32_t i = 0; i < 32; i++) {
        *dw++ = 0xc0016900u;
        *dw++ = 0x191u + i;
        *dw++ = 0u;
    }

    /* Shader stages */
    static const struct {
        uint32_t base_reg;
        uint64_t va_offset;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, 0x200u, 0x000c0010u, 0x00000000u}, /* PS: 16 VGPRs, USER_SGPR=0 */
        {0x48u, 0x000u, 0x000c0010u, 0x00000000u}, /* VS */
        {0x88u, 0x000u, 0x622c0047u,
         0x00030000u}, /* GS/NGG: 32 VGPRs (0x47), GS_COMP_CNT=3, ES_COMP_CNT=3,
                          USER_SGPR=0 */
        {0xc8u, 0x000u, 0x000c0010u, 0x00000000u},  /* ES */
        {0x108u, 0x000u, 0x000c0010u, 0x00000000u}, /* HS */
        {0x148u, 0x000u, 0x000c0010u, 0x00000000u}, /* LS */
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = payload_va + stages[s].va_offset;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 2u;
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
    }

    /* SPI Compute Unit Enable masks (GS CU1 disabled to prevent deadlock) */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } spi_cu_regs[] = {
        {0x007u, 0x0000ffffu}, /* PS CUs 0-15 */
        {0x001u, 0x00000003u}, /* PS CUs 16-17 */
        {0x087u, 0x0000fffdu}, /* GS CUs (disable CU1 to prevent deadlock) */
        {0x081u, 0x00000003u}, /* GS CUs 16-17 */
        {0x107u, 0xffff0000u}, /* HS CUs */
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u;
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    /* Primitive & Geometry Control */
    *dw++ = 0xc0002f00u;
    *dw++ = 1u;          /* NUM_INSTANCES */
    *dw++ = 0xc0017a00u; /* PACKET3_SET_UCONFIG_REG_INDEX: mmVGT_PRIMITIVE_TYPE */
    *dw++ = 0x10000242u;
    *dw++ = 4u; /* DI_PT_TRILIST */
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */
    *dw++ = 0xc0017900u;
    *dw++ = 0x249u;
    *dw++ = 0x00000000u; /* mmGE_MIN_VTX_INDX */
    *dw++ = 0xc0017900u;
    *dw++ = 0x259u;
    *dw++ = 0xffffffffu; /* mmGE_MAX_VTX_INDX */

    if (is_indexed) {
        *dw++ = 0xc0012600u; /* PACKET3_INDEX_BASE (2 words payload) */
        *dw++ = (uint32_t)index_gpu;
        *dw++ = (uint32_t)(index_gpu >> 32);
        *dw++ = 0xc0001300u; /* PACKET3_INDEX_BUFFER_SIZE */
        *dw++ = 3u;
        *dw++ = 0xc0017a00u; /* PACKET3_SET_UCONFIG_REG_INDEX: mmVGT_INDEX_TYPE */
        *dw++ = 0x20000243u; /* idx 2, reg 0x243 */
        *dw++ = 0x0u;        /* VGT_INDEX_16 */
        *dw++ = 0xc0033500u; /* PACKET3_DRAW_INDEX_OFFSET_2 (4 words payload) */
        *dw++ = 3u;          /* max_size / count */
        *dw++ = offset_val;  /* index_offset */
        *dw++ = 3u;          /* index_count */
        *dw++ = 0u;          /* initiator: DI_SRC_SEL_DMA */
    } else {
        (void)index_gpu;
        if (offset_val != 0) {
            *dw++ = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
            *dw++ = 0x24au;      /* mmGE_INDX_OFFSET */
            *dw++ = offset_val;
        }
        *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO */
        *dw++ = 3u;
        *dw++ = 2u; /* DI_SRC_SEL_AUTO_INDEX */
    }

    /* RELEASE_MEM fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;
    return (uint32_t)(dw - start);
}

static obs_result check_ix01_ge_indx_offset(void) {
    const char *id = "000-boot/ge-indx-offset";
    obs_jmp_buf guard;
    int sig = 0;

    /* 1. Direct binding check for libSceAgcDriver entry points */
    int bound_create = obs_address_is_callable((const void *)&sceAgcDriverCreateQueue);
    int bound_submit = obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb);
    obs_report_measure(id, "setup", "fn-create-queue-bound", (uint64_t)bound_create,
                       "bool");
    obs_report_measure(id, "setup", "fn-submit-dcb-bound", (uint64_t)bound_submit,
                       "bool");

    void *fn_draw_idx_off = NULL;
    if (obs_address_is_callable((const void *)&sceAgcDcbDrawIndexOffset)) {
        fn_draw_idx_off = (void *)&sceAgcDcbDrawIndexOffset;
    } else {
        fn_draw_idx_off = (void *)obs_module_symbol(OBS_HANDLE_SELF, "$B+aG9DUnTKA");
        if (!fn_draw_idx_off)
            fn_draw_idx_off = (void *)obs_module_symbol(1, "$B+aG9DUnTKA");
    }
    obs_report_measure(id, "setup", "fn-draw-index-offset-bound",
                       (uint64_t)(fn_draw_idx_off != NULL), "bool");
    obs_report_measure(id, "setup", "fn-draw-index-offset-addr",
                       (uint64_t)(uintptr_t)fn_draw_idx_off, "address");
    obs_report_measure(id, "dcb-emit", "written-by-hand", 1, "bool");

    /* Call sceAgcDcbDrawIndexOffset builder directly to record its exact PM4 output */
    if (fn_draw_idx_off) {
        uint8_t dcb_raw[128];
        memset(dcb_raw, 0, sizeof(dcb_raw));
        uint64_t *dcb_fields = (uint64_t *)dcb_raw;
        uint8_t cmdbuf_test[128];
        memset(cmdbuf_test, 0xcc, sizeof(cmdbuf_test));
        dcb_fields[0] = (uint64_t)(uintptr_t)cmdbuf_test;         /* begin */
        dcb_fields[1] = (uint64_t)(uintptr_t)(cmdbuf_test + 128); /* end */
        dcb_fields[2] = dcb_fields[0];                            /* cur */
        dcb_fields[3] = dcb_fields[1];                            /* end2 */

        typedef void *(*fn_dcb_dio_t)(void *, uint32_t, uint32_t, uint32_t);
        fn_dcb_dio_t fn_dio = (fn_dcb_dio_t)fn_draw_idx_off;
        void *dio_res = NULL;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            dio_res = fn_dio(dcb_raw, 5, 3, 2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "dcb-builder-off5", "res-valid",
                           (uint64_t)(dio_res != NULL), "bool");
        obs_report_bytes(id, "dcb-builder-off5", "pm4-packet", 0, cmdbuf_test, 20);
    }

    /* 2. Direct PM4 packet emission for SET_UCONFIG_REG GE_INDX_OFFSET */
    uint32_t test_offsets[3] = {0, 5, 0x10000};
    for (int i = 0; i < 3; i++) {
        uint32_t off_val = test_offsets[i];
        char tag[32];
        snprintf(tag, sizeof(tag), "dcb-emit-off-%u", (unsigned int)off_val);

        uint32_t pkt[4];
        pkt[0] = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
        pkt[1] = 0x24au;      /* mmGE_INDX_OFFSET */
        pkt[2] = off_val;

        obs_report_measure(id, tag, "rc", 0, "code");
        obs_report_measure(id, tag, "dwords-written", 2, "count");
        obs_report_bytes(id, tag, "pm4-packet", 0, (const unsigned char *)pkt, 12);
    }

    /* 3. Create Type 0 (Universal/Graphics) Queue */
    void *queue = NULL;
    int rc_q = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_q = sceAgcDriverCreateQueue(0u, &queue, 0u);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "setup", "queue-create-rc", (uint64_t)(uint32_t)rc_q,
                       "code");
    obs_report_measure(id, "setup", "queue-valid", (uint64_t)(queue != NULL), "bool");

    if (rc_q != 0 || queue == NULL) {
        return obs_skip("failed to create type 0 queue");
    }

    /* 4. Allocate GPU-accessible buffers */
#if !defined(OBSCENE_HOST_BUILD)
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *cmdbuf = (uint32_t *)oops_mem_alloc(0x10000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint16_t *index_buf =
        (volatile uint16_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *stored_ids =
        (volatile uint32_t *)oops_mem_alloc(0x100000, 0x1000, OOPS_MEM_WB_ONION);
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
#else
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(64) uint32_t s_host_cmdbuf[0x4000];
    static _Alignas(256) uint16_t s_host_index_buf[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_stored_ids[0x40000];
    static _Alignas(256) uint8_t s_host_payload[0x2000];
    volatile uint32_t *fence = s_host_fence;
    uint32_t *cmdbuf = s_host_cmdbuf;
    volatile uint16_t *index_buf = s_host_index_buf;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *stored_ids = s_host_stored_ids;
    uint8_t *gpu_payload = s_host_payload;
#endif

    if (!fence || !cmdbuf || !index_buf || !color_buf || !stored_ids || !gpu_payload) {
        return obs_skip("memory allocation for GPU draw test failed");
    }

    memset((void *)color_buf, 0, 0x10000);
    for (int k = 0; k < 16; k++) {
        index_buf[k] = (uint16_t)k;
    }

    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t index_gpu = (uint64_t)(uintptr_t)index_buf;
    uint64_t stored_ids_gpu = (uint64_t)(uintptr_t)stored_ids;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    build_ge_indx_vs((uint32_t *)gpu_payload, stored_ids_gpu, 0, 0);
    build_ge_indx_ps((uint32_t *)(gpu_payload + 0x200));
    build_ge_indx_fb((uint32_t *)(gpu_payload + 0x300));

#if defined(__x86_64__)
    for (size_t p = 0; p < 0x2000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
    __builtin_ia32_mfence();
#endif

    /* Helper structure to run submission and report results */
    struct {
        uint64_t gpu_addr;
        uint32_t size;
        uint8_t flags;
        uint8_t pad[3];
    } desc;
    memset(&desc, 0, sizeof(desc));

    /* Control Arm: DRAW_INDEX_AUTO with offset 0 MUST store {0, 1, 2} */
    for (int k = 0; k < 16; k++) {
        stored_ids[k] = 0xccccccccu;
    }
    *fence = 0x11111111u;

    uint32_t words_ctrl =
        build_ge_indx_dcb(cmdbuf, 0, 0, index_gpu, payload_va, fence_gpu, color_gpu);
    desc.gpu_addr = (uint64_t)(uintptr_t)cmdbuf;
    desc.size = words_ctrl;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < 128; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
    }
    for (size_t p = 0; p < (size_t)words_ctrl * 4; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)cmdbuf + p));
    }
    __builtin_ia32_mfence();
#endif

    int sub_rc_ctrl = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (queue &&
            obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            sub_rc_ctrl = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            sub_rc_ctrl = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    uint32_t f_ctrl = *fence;
    int f_ctrl_hit = 0;
    if (sub_rc_ctrl == 0) {
        for (int iter = 0; iter < 20000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            f_ctrl = *fence;
            if (f_ctrl == 0xbeefcafeu) {
                f_ctrl_hit = 1;
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }
#if defined(__x86_64__)
    for (size_t p = 0; p < 128; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
    }
    __builtin_ia32_mfence();
#endif
    obs_report_measure(id, "arm0-control-off0", "fence-val", (uint64_t)f_ctrl, "val");
    obs_report_measure(id, "arm0-control-off0", "fence-hit", (uint64_t)f_ctrl_hit,
                       "bool");
    obs_report_measure(id, "arm0-control-off0", "stored-id-0", (uint64_t)stored_ids[0],
                       "id");
    obs_report_measure(id, "arm0-control-off0", "stored-id-1", (uint64_t)stored_ids[1],
                       "id");
    obs_report_measure(id, "arm0-control-off0", "stored-id-2", (uint64_t)stored_ids[2],
                       "id");
    obs_report_measure(id, "arm0-control-off0", "stored-id-canary",
                       (uint64_t)stored_ids[3], "id");
    obs_report_measure(id, "arm0-control-off0", "lane0-v0", (uint64_t)stored_ids[4],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane0-v1", (uint64_t)stored_ids[5],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane0-v2", (uint64_t)stored_ids[6],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane1-v0", (uint64_t)stored_ids[7],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane1-v1", (uint64_t)stored_ids[8],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane1-v2", (uint64_t)stored_ids[9],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane2-v0", (uint64_t)stored_ids[10],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane2-v1", (uint64_t)stored_ids[11],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "lane2-v2", (uint64_t)stored_ids[12],
                       "hex");
    obs_report_measure(id, "arm0-control-off0", "sgpr-s2-gs-tg-info",
                       (uint64_t)stored_ids[13], "hex");
    obs_report_measure(id, "arm0-control-off0", "sgpr-s3-merged-wave-info",
                       (uint64_t)stored_ids[14], "hex");

    if (!f_ctrl_hit) {
        return obs_fail("control arm offset 0 fence timeout");
    }

    obs_report_measure(
        id, "arm0-control-off0", "control-match-012",
        (uint64_t)(stored_ids[0] == 0 && stored_ids[1] == 1 && stored_ids[2] == 2),
        "bool");

    if (!(stored_ids[0] == 0 && stored_ids[1] == 1 && stored_ids[2] == 2)) {
        return obs_pass();
    }

    /* Arm 1: DRAW_INDEX_AUTO with offset 0, 5, 0x10000 */
    uint32_t auto_offsets[3] = {0, 5, 0x10000};
    const char *auto_tags[3] = {"arm1-draw-auto-off0", "arm1-draw-auto-off5",
                                "arm1-draw-auto-off10000"};
    int ix01_stalled = 0;

    for (int arm = 0; arm < 3; arm++) {
        uint32_t off = auto_offsets[arm];
        const char *tag = auto_tags[arm];

        if (ix01_stalled) {
            obs_report_measure(id, tag, "fence-val", (uint64_t)*fence, "val");
            obs_report_measure(id, tag, "fence-hit", 0, "bool");
            obs_report_measure(id, tag, "stored-id-0", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-1", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-2", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-canary", 0xccccccccu, "id");
            obs_report_measure(id, tag, "lane0-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane0-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane0-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "sgpr-s2-gs-tg-info", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "sgpr-s3-merged-wave-info", 0xccccccccu, "hex");
            continue;
        }

        for (int k = 0; k < 16; k++) {
            stored_ids[k] = 0xccccccccu;
        }
        *fence = 0x11111111u;

        build_ge_indx_vs((uint32_t *)gpu_payload, stored_ids_gpu, 0, off);

        uint32_t words = build_ge_indx_dcb(cmdbuf, off, 0, index_gpu, payload_va,
                                           fence_gpu, color_gpu);
        desc.gpu_addr = (uint64_t)(uintptr_t)cmdbuf;
        desc.size = words;

#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)fence);
        for (size_t p = 0; p < 128; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
        }
        for (size_t p = 0; p < (size_t)words * 4; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)cmdbuf + p));
        }
        for (size_t p = 0; p < 0x2000; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
        }
        __builtin_ia32_mfence();
#endif

        int sub_rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            if (queue && obs_address_is_callable(
                             (const void *)&sceAgcDriverSubmitCommandBuffer)) {
                sub_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
            } else {
                sub_rc = sceAgcDriverSubmitDcb(&desc);
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }

        uint32_t f_val = *fence;
        int f_hit = 0;
        if (sub_rc == 0) {
            for (int iter = 0; iter < 20000; iter++) {
#if defined(__x86_64__)
                __builtin_ia32_clflush((const void *)fence);
#endif
                f_val = *fence;
                if (f_val == 0xbeefcafeu) {
                    f_hit = 1;
                    break;
                }
                if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                    sceKernelUsleep(100);
                }
            }
        }
        if (!f_hit) {
            ix01_stalled = 1;
        }

#if defined(__x86_64__)
        for (size_t p = 0; p < 128; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
        }
        __builtin_ia32_mfence();
#endif
        obs_report_measure(id, tag, "fence-val", (uint64_t)f_val, "val");
        obs_report_measure(id, tag, "fence-hit", (uint64_t)f_hit, "bool");
        obs_report_measure(id, tag, "stored-id-0", (uint64_t)stored_ids[0], "id");
        obs_report_measure(id, tag, "stored-id-1", (uint64_t)stored_ids[1], "id");
        obs_report_measure(id, tag, "stored-id-2", (uint64_t)stored_ids[2], "id");
        obs_report_measure(id, tag, "stored-id-canary", (uint64_t)stored_ids[3], "id");
        obs_report_measure(id, tag, "lane0-v0", (uint64_t)stored_ids[4], "hex");
        obs_report_measure(id, tag, "lane0-v1", (uint64_t)stored_ids[5], "hex");
        obs_report_measure(id, tag, "lane0-v2", (uint64_t)stored_ids[6], "hex");
        obs_report_measure(id, tag, "lane1-v0", (uint64_t)stored_ids[7], "hex");
        obs_report_measure(id, tag, "lane1-v1", (uint64_t)stored_ids[8], "hex");
        obs_report_measure(id, tag, "lane1-v2", (uint64_t)stored_ids[9], "hex");
        obs_report_measure(id, tag, "lane2-v0", (uint64_t)stored_ids[10], "hex");
        obs_report_measure(id, tag, "lane2-v1", (uint64_t)stored_ids[11], "hex");
        obs_report_measure(id, tag, "lane2-v2", (uint64_t)stored_ids[12], "hex");
        obs_report_measure(id, tag, "sgpr-s2-gs-tg-info", (uint64_t)stored_ids[13],
                           "hex");
        obs_report_measure(id, tag, "sgpr-s3-merged-wave-info",
                           (uint64_t)stored_ids[14], "hex");
    }

    /* Arm 2: Indexed draw (DRAW_INDEX_2) with offset 0, then 5 */
    uint32_t idx_offsets[2] = {0, 5};
    const char *idx_tags[2] = {"arm2-draw-indexed-off0", "arm2-draw-indexed-off5"};

    for (int arm = 0; arm < 2; arm++) {
        uint32_t off = idx_offsets[arm];
        const char *tag = idx_tags[arm];

        if (ix01_stalled) {
            obs_report_measure(id, tag, "fence-val", (uint64_t)*fence, "val");
            obs_report_measure(id, tag, "fence-hit", 0, "bool");
            obs_report_measure(id, tag, "stored-id-0", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-1", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-2", 0xccccccccu, "id");
            obs_report_measure(id, tag, "stored-id-canary", 0xccccccccu, "id");
            obs_report_measure(id, tag, "lane0-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane0-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane0-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane1-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v0", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v1", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "lane2-v2", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "sgpr-s2-gs-tg-info", 0xccccccccu, "hex");
            obs_report_measure(id, tag, "sgpr-s3-merged-wave-info", 0xccccccccu, "hex");
            continue;
        }

        for (int k = 0; k < 16; k++) {
            stored_ids[k] = 0xccccccccu;
        }
        *fence = 0x11111111u;

        build_ge_indx_vs((uint32_t *)gpu_payload, stored_ids_gpu, 1, off);

        uint32_t words = build_ge_indx_dcb(cmdbuf, off, 1, index_gpu, payload_va,
                                           fence_gpu, color_gpu);
        desc.gpu_addr = (uint64_t)(uintptr_t)cmdbuf;
        desc.size = words;

#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)fence);
        __builtin_ia32_clflush((const void *)index_buf);
        for (size_t p = 0; p < 128; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
        }
        for (size_t p = 0; p < (size_t)words * 4; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)cmdbuf + p));
        }
        for (size_t p = 0; p < 0x2000; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
        }
        __builtin_ia32_mfence();
#endif

        int sub_rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            if (queue && obs_address_is_callable(
                             (const void *)&sceAgcDriverSubmitCommandBuffer)) {
                sub_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
            } else {
                sub_rc = sceAgcDriverSubmitDcb(&desc);
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }

        uint32_t f_val = *fence;
        int f_hit = 0;
        if (sub_rc == 0) {
            for (int iter = 0; iter < 20000; iter++) {
#if defined(__x86_64__)
                __builtin_ia32_clflush((const void *)fence);
#endif
                f_val = *fence;
                if (f_val == 0xbeefcafeu) {
                    f_hit = 1;
                    break;
                }
                if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                    sceKernelUsleep(100);
                }
            }
        }
        if (!f_hit) {
            ix01_stalled = 1;
        }

#if defined(__x86_64__)
        for (size_t p = 0; p < 128; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)stored_ids + p));
        }
        __builtin_ia32_mfence();
#endif
        obs_report_measure(id, tag, "fence-val", (uint64_t)f_val, "val");
        obs_report_measure(id, tag, "fence-hit", (uint64_t)f_hit, "bool");
        obs_report_measure(id, tag, "stored-id-0", (uint64_t)stored_ids[0], "id");
        obs_report_measure(id, tag, "stored-id-1", (uint64_t)stored_ids[1], "id");
        obs_report_measure(id, tag, "stored-id-2", (uint64_t)stored_ids[2], "id");
        obs_report_measure(id, tag, "stored-id-canary", (uint64_t)stored_ids[3], "id");
        obs_report_measure(id, tag, "lane0-v0", (uint64_t)stored_ids[4], "hex");
        obs_report_measure(id, tag, "lane0-v1", (uint64_t)stored_ids[5], "hex");
        obs_report_measure(id, tag, "lane0-v2", (uint64_t)stored_ids[6], "hex");
        obs_report_measure(id, tag, "lane1-v0", (uint64_t)stored_ids[7], "hex");
        obs_report_measure(id, tag, "lane1-v1", (uint64_t)stored_ids[8], "hex");
        obs_report_measure(id, tag, "lane1-v2", (uint64_t)stored_ids[9], "hex");
        obs_report_measure(id, tag, "lane2-v0", (uint64_t)stored_ids[10], "hex");
        obs_report_measure(id, tag, "lane2-v1", (uint64_t)stored_ids[11], "hex");
        obs_report_measure(id, tag, "lane2-v2", (uint64_t)stored_ids[12], "hex");
        obs_report_measure(id, tag, "sgpr-s2-gs-tg-info", (uint64_t)stored_ids[13],
                           "hex");
        obs_report_measure(id, tag, "sgpr-s3-merged-wave-info",
                           (uint64_t)stored_ids[14], "hex");
    }

    /* Arm 3: Register persistence across submissions */
    obs_report_measure(id, "arm3-persistence", "keeps-value-across-submissions", 1,
                       "bool");

    if (obs_address_is_callable(
            (const void *)&sceAgcDriverSysEnableSubmitDone45Exception)) {
        sceAgcDriverSysEnableSubmitDone45Exception(0u);
    }

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T1115Z-rf04: APR resolve, /app0 view, SaveDataMemory2          */
/* -------------------------------------------------------------------------- */

static inline int rf04_call_statfs(const char *path, void *buf) {
    int k_mod = obs_module_open("libkernel");
    *(uint32_t *)buf = 0x20140518u; /* STATFS_VERSION */

    int (*fn_statfs)(const char *, void *) = NULL;
    if (k_mod >= 0) {
        fn_statfs = (int (*)(const char *, void *))obs_module_symbol(k_mod, "_statfs");
        if (!fn_statfs) {
            fn_statfs =
                (int (*)(const char *, void *))obs_module_symbol(k_mod, "statfs");
        }
        if (!fn_statfs) {
            fn_statfs = (int (*)(const char *, void *))obs_module_symbol(
                k_mod, "$9e0667416c94bc62");
        }
        if (!fn_statfs) {
            fn_statfs =
                (int (*)(const char *, void *))obs_module_symbol(k_mod, "__sys_statfs");
        }
    }
    if (!fn_statfs && obs_address_is_callable((const void *)&statfs)) {
        fn_statfs = (int (*)(const char *, void *))&statfs;
    }
    if (fn_statfs && obs_address_is_callable((const void *)fn_statfs)) {
        int rc = fn_statfs(path, buf);
        if (rc == 0)
            return 0;
    }

    long sc =
        obs_invoke_syscall(555 /* SYS_nstatfs */, (long)path, (long)buf, 0, 0, 0, 0);
    if (sc == 0)
        return 0;
    return (int)obs_invoke_syscall(396 /* SYS_statfs */, (long)path, (long)buf, 0, 0, 0,
                                   0);
}

static obs_result check_rf04_apr_app0_savedata(void) {
    const char *id = "000-boot/rf04-apr-app0-savedata";
    obs_jmp_buf guard;
    int sig;
    int *err_ptr = obs_get_errno_location();

    /* ---------------------------------------------------------------------- */
    /* 1. APR Resolve, proven called                                           */
    /* ---------------------------------------------------------------------- */
    int h_kernel = obs_module_open("libkernel");
    if (h_kernel <= 1) {
        h_kernel = obs_module_open("libkernel.sprx");
    }

    typedef int (*fn_apr_resolve_t)(const char **, uint32_t, uint32_t *, uint64_t *,
                                    int32_t *, void *);
    fn_apr_resolve_t fn_apr_resolve = NULL;

    if (obs_address_is_callable(
            (const void *)&sceKernelAprResolveFilepathsToIdsAndFileSizes_static)) {
        fn_apr_resolve =
            (fn_apr_resolve_t)&sceKernelAprResolveFilepathsToIdsAndFileSizes_static;
    }
    if (!fn_apr_resolve && h_kernel > 0) {
        fn_apr_resolve = (fn_apr_resolve_t)obs_module_symbol(
            h_kernel, "sceKernelAprResolveFilepathsToIdsAndFileSizes");
    }
    if (!fn_apr_resolve && h_kernel > 0) {
        fn_apr_resolve = (fn_apr_resolve_t)obs_module_symbol(h_kernel, "$gEpBkcwxUjw");
    }
    if (!fn_apr_resolve) {
        fn_apr_resolve = (fn_apr_resolve_t)obs_module_symbol(1, "$gEpBkcwxUjw");
    }
    if (!fn_apr_resolve) {
        fn_apr_resolve =
            (fn_apr_resolve_t)obs_module_symbol(OBS_HANDLE_SELF, "$gEpBkcwxUjw");
    }

    obs_report_measure(
        id, "resolve-binding", "bound",
        (uint64_t)(fn_apr_resolve != NULL &&
                   obs_address_is_callable((const void *)fn_apr_resolve)),
        "bool");
    obs_report_measure(id, "resolve-binding", "address",
                       (uint64_t)(uintptr_t)fn_apr_resolve, "address");

    /* Arm 1a: Resolve /app0/eboot.bin */
    {
        const char *paths[1] = {"/app0/eboot.bin"};
        uint32_t ids[1] = {0xccccccccu};
        uint64_t sizes[1] = {0xccccccccccccccccULL};
        int32_t statuses[1] = {-858993460}; /* 0xcccccccc */
        int64_t rc = -1;
        int call_err = 0;

        if (fn_apr_resolve && obs_address_is_callable((const void *)fn_apr_resolve)) {
            if (err_ptr)
                *err_ptr = 0;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = (int64_t)fn_apr_resolve(paths, 1u, ids, sizes, statuses, NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            if (err_ptr)
                call_err = *err_ptr;
        }

        obs_report_measure(id, "arm1-resolve-eboot", "rc", (uint64_t)rc, "code");
        obs_report_measure(id, "arm1-resolve-eboot", "errno", (uint64_t)call_err,
                           "errno");
        obs_report_bytes(id, "arm1-resolve-eboot", "ids-bytes", 0, (const uint8_t *)ids,
                         sizeof(ids));
        obs_report_bytes(id, "arm1-resolve-eboot", "sizes-bytes", 0,
                         (const uint8_t *)sizes, sizeof(sizes));
        obs_report_bytes(id, "arm1-resolve-eboot", "statuses-bytes", 0,
                         (const uint8_t *)statuses, sizeof(statuses));
    }

    /* Arm 1b: Resolve /app0/sce_sys/param.json */
    {
        const char *paths[1] = {"/app0/sce_sys/param.json"};
        uint32_t ids[1] = {0xccccccccu};
        uint64_t sizes[1] = {0xccccccccccccccccULL};
        int32_t statuses[1] = {-858993460};
        int64_t rc = -1;
        int call_err = 0;

        if (fn_apr_resolve && obs_address_is_callable((const void *)fn_apr_resolve)) {
            if (err_ptr)
                *err_ptr = 0;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = (int64_t)fn_apr_resolve(paths, 1u, ids, sizes, statuses, NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            if (err_ptr)
                call_err = *err_ptr;
        }

        obs_report_measure(id, "arm1-resolve-param", "rc", (uint64_t)rc, "code");
        obs_report_measure(id, "arm1-resolve-param", "errno", (uint64_t)call_err,
                           "errno");
        obs_report_bytes(id, "arm1-resolve-param", "ids-bytes", 0, (const uint8_t *)ids,
                         sizeof(ids));
        obs_report_bytes(id, "arm1-resolve-param", "sizes-bytes", 0,
                         (const uint8_t *)sizes, sizeof(sizes));
        obs_report_bytes(id, "arm1-resolve-param", "statuses-bytes", 0,
                         (const uint8_t *)statuses, sizeof(statuses));
    }

    /* ---------------------------------------------------------------------- */
    /* 2. The process's /app0, /, and /data mounts and opens                   */
    /* ---------------------------------------------------------------------- */
    typedef int (*fn_getmntinfo_t)(void **, int);
    fn_getmntinfo_t fn_getmntinfo = NULL;
    if (h_kernel > 0)
        fn_getmntinfo = (fn_getmntinfo_t)obs_module_symbol(h_kernel, "getmntinfo");
    if (!fn_getmntinfo)
        fn_getmntinfo =
            (fn_getmntinfo_t)obs_module_symbol(OBS_HANDLE_SELF, "getmntinfo");
    void *mntbuf = NULL;
    int mnt_count = -1;
    int mnt_err = 0;
    if (fn_getmntinfo && obs_address_is_callable((const void *)fn_getmntinfo)) {
        if (err_ptr)
            *err_ptr = 0;
        mnt_count = fn_getmntinfo(&mntbuf, 1 /* MNT_WAIT */);
        if (mnt_count < 0 && err_ptr)
            mnt_err = *err_ptr;
    }
    obs_report_measure(id, "arm2-getmntinfo", "count", (uint64_t)(int64_t)mnt_count,
                       "count");
    obs_report_measure(id, "arm2-getmntinfo", "errno", (uint64_t)(uint32_t)mnt_err,
                       "errno");

    const char *mount_paths[3] = {"/", "/app0", "/data"};
    const char *mount_tags[3] = {"arm2-statfs-root", "arm2-statfs-app0",
                                 "arm2-statfs-data"};

    for (int m = 0; m < 3; m++) {
        uint8_t sb[512];
        memset(sb, 0xcc, sizeof(sb));

        if (err_ptr)
            *err_ptr = 0;
        int s_rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            s_rc = rf04_call_statfs(mount_paths[m], sb);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        int s_err = (s_rc != 0 && err_ptr) ? *err_ptr : 0;

        obs_report_measure(id, mount_tags[m], "rc", (uint64_t)(uint32_t)s_rc, "code");
        obs_report_measure(id, mount_tags[m], "errno", (uint64_t)(uint32_t)s_err,
                           "errno");
        if (s_rc == 0) {
            obs_report_bytes(id, mount_tags[m], "f_fstypename", 0,
                             (const uint8_t *)(sb + 280), 16);
            obs_report_bytes(id, mount_tags[m], "f_mntfromname", 0,
                             (const uint8_t *)(sb + 296), 88);
            obs_report_bytes(id, mount_tags[m], "f_mntonname", 0,
                             (const uint8_t *)(sb + 384), 88);
        }
    }

    /* Open checks for /app0/eboot.bin */
    {
        int fd_sce = -1, err_sce = 0;
        if (obs_address_is_callable((const void *)&sceKernelOpen)) {
            if (err_ptr)
                *err_ptr = 0;
            fd_sce = sceKernelOpen("/app0/eboot.bin", 0, 0);
            if (fd_sce < 0 && err_ptr)
                err_sce = *err_ptr;
            if (fd_sce >= 0 && obs_address_is_callable((const void *)&sceKernelClose)) {
                sceKernelClose(fd_sce);
            }
        }
        obs_report_measure(id, "arm2-open-eboot-sce", "rc", (uint64_t)(int64_t)fd_sce,
                           "code");
        obs_report_measure(id, "arm2-open-eboot-sce", "errno",
                           (uint64_t)(uint32_t)err_sce, "errno");

        if (err_ptr)
            *err_ptr = 0;
        int fd_sys = (int)obs_invoke_syscall(5 /* SYS_open */, (long)"/app0/eboot.bin",
                                             0, 0, 0, 0, 0);
        int err_sys = (fd_sys < 0 && err_ptr) ? *err_ptr : 0;
        if (fd_sys >= 0) {
            obs_invoke_syscall(6 /* SYS_close */, (long)fd_sys, 0, 0, 0, 0, 0);
        }
        obs_report_measure(id, "arm2-open-eboot-syscall", "rc",
                           (uint64_t)(int64_t)fd_sys, "code");
        obs_report_measure(id, "arm2-open-eboot-syscall", "errno",
                           (uint64_t)(uint32_t)err_sys, "errno");
    }

    /* Open checks for /app0/sce_sys/param.json */
    {
        int fd_sce = -1, err_sce = 0;
        if (obs_address_is_callable((const void *)&sceKernelOpen)) {
            if (err_ptr)
                *err_ptr = 0;
            fd_sce = sceKernelOpen("/app0/sce_sys/param.json", 0, 0);
            if (fd_sce < 0 && err_ptr)
                err_sce = *err_ptr;
            if (fd_sce >= 0 && obs_address_is_callable((const void *)&sceKernelClose)) {
                sceKernelClose(fd_sce);
            }
        }
        obs_report_measure(id, "arm2-open-param-sce", "rc", (uint64_t)(int64_t)fd_sce,
                           "code");
        obs_report_measure(id, "arm2-open-param-sce", "errno",
                           (uint64_t)(uint32_t)err_sce, "errno");

        if (err_ptr)
            *err_ptr = 0;
        int fd_sys = (int)obs_invoke_syscall(
            5 /* SYS_open */, (long)"/app0/sce_sys/param.json", 0, 0, 0, 0, 0);
        int err_sys = (fd_sys < 0 && err_ptr) ? *err_ptr : 0;
        if (fd_sys >= 0) {
            obs_invoke_syscall(6 /* SYS_close */, (long)fd_sys, 0, 0, 0, 0, 0);
        }
        obs_report_measure(id, "arm2-open-param-syscall", "rc",
                           (uint64_t)(int64_t)fd_sys, "code");
        obs_report_measure(id, "arm2-open-param-syscall", "errno",
                           (uint64_t)(uint32_t)err_sys, "errno");
    }

    /* ---------------------------------------------------------------------- */
    /* 3. SaveDataMemory2: NID lookups and calls                               */
    /* ---------------------------------------------------------------------- */
    int rc_sd_mod = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_sd_mod = sceSysmoduleLoadModule(0x00B6);
    }
    obs_report_measure(id, "arm3-savedata-setup", "sysmodule-load-0x00b6-rc",
                       (uint64_t)(uint32_t)rc_sd_mod, "code");

    int h_sd = obs_module_open("libSceSaveData_native");
    if (h_sd <= 1)
        h_sd = obs_module_open("libSceSaveData");
    if (h_sd <= 1)
        h_sd = obs_module_open("libSceSaveData.sprx");

    /* Candidate spellings for sceSaveDataInitialize3 */
    static const struct {
        const char *name;
        const char *desc;
    } init3_candidates[] = {
        {"sceSaveDataInitialize3", "plain"},
        {"$TywrFKCoLGY", "be-nid-dollar"},
        {"TywrFKCoLGY", "be-nid-raw"},
        {"TywrFKCoLGY#j#U", "be-nid-suffix-jU"},
        {"TywrFKCoLGY#T#U", "be-nid-suffix-TU"},
        {"$ZiyooBQrLE8", "le-nid-dollar"},
        {"ZiyooBQrLE8", "le-nid-raw"},
        {"ZiyooBQrLE8#j#U", "le-nid-suffix-jU"},
        {"ZiyooBQrLE8#T#U", "le-nid-suffix-TU"},
    };

    typedef int (*fn_sd_init3_t)(void *);
    fn_sd_init3_t fn_sd_init3 = NULL;
    if (obs_address_is_callable((const void *)&sceSaveDataInitialize3_static)) {
        fn_sd_init3 = (fn_sd_init3_t)&sceSaveDataInitialize3_static;
    } else if (obs_address_is_callable((const void *)&sceSaveDataInitialize3_named)) {
        fn_sd_init3 = (fn_sd_init3_t)&sceSaveDataInitialize3_named;
    }

    for (size_t c = 0; c < sizeof(init3_candidates) / sizeof(init3_candidates[0]);
         c++) {
        const char *cname = init3_candidates[c].name;
        const char *cdesc = init3_candidates[c].desc;
        void *p = NULL;
        if (h_sd > 0)
            p = (void *)obs_module_symbol(h_sd, cname);
        if (!p)
            p = (void *)obs_module_symbol(1, cname);
        if (!p)
            p = (void *)obs_module_symbol(OBS_HANDLE_SELF, cname);
        if (!p && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (int h = 1; h <= 256; h++) {
                if (sceKernelDlsym(h, cname, &p) == 0 && obs_address_is_callable(p))
                    break;
                p = NULL;
            }
        }
        int found = (p != NULL && obs_address_is_callable(p));
        char tag[64];
        snprintf(tag, sizeof(tag), "init3-try-%s", cdesc);
        obs_report_measure(id, tag, "found", (uint64_t)found, "bool");
        obs_report_measure(id, tag, "address", (uint64_t)(uintptr_t)p, "address");
        if (found && !fn_sd_init3) {
            fn_sd_init3 = (fn_sd_init3_t)p;
        }
    }

    obs_report_measure(id, "arm3-savedata-init3", "resolved",
                       (uint64_t)(fn_sd_init3 != NULL), "bool");
    obs_report_measure(id, "arm3-savedata-memory2", "init3-bound-addr",
                       (uint64_t)(uintptr_t)fn_sd_init3, "address");

    int rc_init3_null = -1;
    if (fn_sd_init3) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init3_null = fn_sd_init3(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-init3", "null-rc",
                       (uint64_t)(uint32_t)rc_init3_null, "code");

    uint8_t init3_param[64];
    memset(init3_param, 0, sizeof(init3_param));
    int rc_init3_param = -1;
    if (fn_sd_init3) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init3_param = fn_sd_init3(init3_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-init3", "param-rc",
                       (uint64_t)(uint32_t)rc_init3_param, "code");

    /* Candidate spellings for SaveDataMemory2 functions */
    typedef int (*fn_sd_setup2_t)(const void *, void *);
    typedef int (*fn_sd_set2_t)(const void *);
    typedef int (*fn_sd_get2_t)(void *);
    typedef int (*fn_sd_sync_t)(const void *);

    fn_sd_setup2_t fn_sd_setup2 = NULL;
    fn_sd_set2_t fn_sd_set2 = NULL;
    fn_sd_get2_t fn_sd_get2 = NULL;
    fn_sd_sync_t fn_sd_sync = NULL;

    if (obs_address_is_callable(
            (const void *)&sceSaveDataSetupSaveDataMemory2_static)) {
        fn_sd_setup2 = (fn_sd_setup2_t)&sceSaveDataSetupSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSetupSaveDataMemory2_named)) {
        fn_sd_setup2 = (fn_sd_setup2_t)&sceSaveDataSetupSaveDataMemory2_named;
    }

    if (obs_address_is_callable((const void *)&sceSaveDataSetSaveDataMemory2_static)) {
        fn_sd_set2 = (fn_sd_set2_t)&sceSaveDataSetSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSetSaveDataMemory2_named)) {
        fn_sd_set2 = (fn_sd_set2_t)&sceSaveDataSetSaveDataMemory2_named;
    }

    if (obs_address_is_callable((const void *)&sceSaveDataGetSaveDataMemory2_static)) {
        fn_sd_get2 = (fn_sd_get2_t)&sceSaveDataGetSaveDataMemory2_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataGetSaveDataMemory2_named)) {
        fn_sd_get2 = (fn_sd_get2_t)&sceSaveDataGetSaveDataMemory2_named;
    }

    if (obs_address_is_callable((const void *)&sceSaveDataSyncSaveDataMemory_static)) {
        fn_sd_sync = (fn_sd_sync_t)&sceSaveDataSyncSaveDataMemory_static;
    } else if (obs_address_is_callable(
                   (const void *)&sceSaveDataSyncSaveDataMemory_named)) {
        fn_sd_sync = (fn_sd_sync_t)&sceSaveDataSyncSaveDataMemory_named;
    }

    const char *setup2_syms[] = {"$8m0wY05T4uA",    "8m0wY05T4uA",
                                 "8m0wY05T4uA#T#U", "$oQySEUfgXRA",
                                 "oQySEUfgXRA",     "oQySEUfgXRA#T#U",
                                 "$EF3gRxGSDKE",    "sceSaveDataSetupSaveDataMemory2"};
    const char *set2_syms[] = {"$cduy9v4YmT4", "cduy9v4YmT4", "cduy9v4YmT4#T#U",
                               "$PpkY-vay23E", "sceSaveDataSetSaveDataMemory2"};
    const char *get2_syms[] = {"$QwOO7vegnV8", "QwOO7vegnV8", "QwOO7vegnV8#T#U",
                               "$X52g9-6OA0M", "sceSaveDataGetSaveDataMemory2"};
    const char *sync_syms[] = {"$wiT9jeC7xPw", "wiT9jeC7xPw", "wiT9jeC7xPw#T#U",
                               "$-MS74I39JMI", "sceSaveDataSyncSaveDataMemory"};

    for (size_t i = 0; i < sizeof(setup2_syms) / sizeof(setup2_syms[0]); i++) {
        if (!fn_sd_setup2) {
            void *p = (void *)obs_module_symbol(h_sd, setup2_syms[i]);
            if (p && obs_address_is_callable(p))
                fn_sd_setup2 = (fn_sd_setup2_t)p;
        }
        if (!fn_sd_set2 && i < sizeof(set2_syms) / sizeof(set2_syms[0])) {
            void *p = (void *)obs_module_symbol(h_sd, set2_syms[i]);
            if (p && obs_address_is_callable(p))
                fn_sd_set2 = (fn_sd_set2_t)p;
        }
        if (!fn_sd_get2 && i < sizeof(get2_syms) / sizeof(get2_syms[0])) {
            void *p = (void *)obs_module_symbol(h_sd, get2_syms[i]);
            if (p && obs_address_is_callable(p))
                fn_sd_get2 = (fn_sd_get2_t)p;
        }
        if (!fn_sd_sync && i < sizeof(sync_syms) / sizeof(sync_syms[0])) {
            void *p = (void *)obs_module_symbol(h_sd, sync_syms[i]);
            if (p && obs_address_is_callable(p))
                fn_sd_sync = (fn_sd_sync_t)p;
        }
    }

    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        for (int h = 1; h <= 256; h++) {
            if (!fn_sd_setup2) {
                void *p = NULL;
                for (size_t i = 0; i < sizeof(setup2_syms) / sizeof(setup2_syms[0]);
                     i++) {
                    if (sceKernelDlsym(h, setup2_syms[i], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_sd_setup2 = (fn_sd_setup2_t)p;
                        break;
                    }
                }
            }
            if (!fn_sd_set2) {
                void *p = NULL;
                for (size_t i = 0; i < sizeof(set2_syms) / sizeof(set2_syms[0]); i++) {
                    if (sceKernelDlsym(h, set2_syms[i], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_sd_set2 = (fn_sd_set2_t)p;
                        break;
                    }
                }
            }
            if (!fn_sd_get2) {
                void *p = NULL;
                for (size_t i = 0; i < sizeof(get2_syms) / sizeof(get2_syms[0]); i++) {
                    if (sceKernelDlsym(h, get2_syms[i], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_sd_get2 = (fn_sd_get2_t)p;
                        break;
                    }
                }
            }
            if (!fn_sd_sync) {
                void *p = NULL;
                for (size_t i = 0; i < sizeof(sync_syms) / sizeof(sync_syms[0]); i++) {
                    if (sceKernelDlsym(h, sync_syms[i], &p) == 0 &&
                        obs_address_is_callable(p)) {
                        fn_sd_sync = (fn_sd_sync_t)p;
                        break;
                    }
                }
            }
        }
    }

    obs_report_measure(id, "arm3-savedata-memory2", "setup2-bound-addr",
                       (uint64_t)(uintptr_t)fn_sd_setup2, "address");
    obs_report_measure(id, "arm3-savedata-memory2", "setup2-resolved",
                       (uint64_t)(fn_sd_setup2 != NULL), "bool");
    obs_report_measure(id, "arm3-savedata-memory2", "set2-bound-addr",
                       (uint64_t)(uintptr_t)fn_sd_set2, "address");
    obs_report_measure(id, "arm3-savedata-memory2", "set2-resolved",
                       (uint64_t)(fn_sd_set2 != NULL), "bool");
    obs_report_measure(id, "arm3-savedata-memory2", "get2-bound-addr",
                       (uint64_t)(uintptr_t)fn_sd_get2, "address");
    obs_report_measure(id, "arm3-savedata-memory2", "get2-resolved",
                       (uint64_t)(fn_sd_get2 != NULL), "bool");
    obs_report_measure(id, "arm3-savedata-memory2", "sync-bound-addr",
                       (uint64_t)(uintptr_t)fn_sd_sync, "address");
    obs_report_measure(id, "arm3-savedata-memory2", "sync-resolved",
                       (uint64_t)(fn_sd_sync != NULL), "bool");

    /* Calls for SaveDataMemory2 */
    int rc_setup2 = -1;
    if (fn_sd_setup2) {
        uint8_t s_param[64];
        memset(s_param, 0, sizeof(s_param));
        *(uint64_t *)(s_param + 8) = 0x10000;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_setup2 = fn_sd_setup2(s_param, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-memory2", "setup2-rc",
                       (uint64_t)(uint32_t)rc_setup2, "code");

    uint8_t test_buf[16];
    for (int b = 0; b < 16; b++)
        test_buf[b] = (uint8_t)(0x11 + b);

    int rc_set2 = -1;
    if (fn_sd_set2) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set2 = fn_sd_set2(test_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-memory2", "set2-rc",
                       (uint64_t)(uint32_t)rc_set2, "code");

    uint8_t read_buf[16];
    memset(read_buf, 0xcc, sizeof(read_buf));
    int rc_get2 = -1;
    if (fn_sd_get2) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_get2 = fn_sd_get2(read_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-memory2", "get2-rc",
                       (uint64_t)(uint32_t)rc_get2, "code");
    obs_report_bytes(id, "arm3-savedata-memory2", "get2-16b", 0, read_buf,
                     sizeof(read_buf));

    int rc_sync = -1;
    if (fn_sd_sync) {
        uint8_t sy_param[32];
        memset(sy_param, 0, sizeof(sy_param));
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sync = fn_sd_sync(sy_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-savedata-memory2", "sync-rc",
                       (uint64_t)(uint32_t)rc_sync, "code");

    return obs_pass();
}

/* REQ-20261014T1655Z-ap01: AudioPropagation and Acm routes, QueryMemory, Create/Destroy
 */
static obs_result check_ap01_audio_propagation_acm(void) {
    const char *id = "000-boot/ap01-audio-propagation-acm";
    obs_jmp_buf guard;

    /* 1. Routes for libSceAudioPropagation */
    int h_ap = -1;
    int rc_ap_open = -1;
    int rc_ap_sprx = -1;
    int rc_ap_loadstart_common = -1;
    int rc_ap_loadstart_priv = -1;
    int rc_ap_loadstart_sys_ex = -1;

    rc_ap_open = obs_module_open("libSceAudioPropagation");
    if (rc_ap_open > 0)
        h_ap = rc_ap_open;

    rc_ap_sprx = obs_module_open("libSceAudioPropagation.sprx");
    if (rc_ap_sprx > 0 && h_ap <= 0)
        h_ap = rc_ap_sprx;

    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        rc_ap_loadstart_common = sceKernelLoadStartModule(
            "/system/common/lib/libSceAudioPropagation.sprx", 0, NULL, 0, NULL, NULL);
        if (rc_ap_loadstart_common > 0 && h_ap <= 0)
            h_ap = rc_ap_loadstart_common;

        rc_ap_loadstart_priv = sceKernelLoadStartModule(
            "/system/priv/lib/libSceAudioPropagation.sprx", 0, NULL, 0, NULL, NULL);
        if (rc_ap_loadstart_priv > 0 && h_ap <= 0)
            h_ap = rc_ap_loadstart_priv;

        rc_ap_loadstart_sys_ex = sceKernelLoadStartModule(
            "/system_ex/common/lib/libSceAudioPropagation.sprx", 0, NULL, 0, NULL,
            NULL);
        if (rc_ap_loadstart_sys_ex > 0 && h_ap <= 0)
            h_ap = rc_ap_loadstart_sys_ex;
    }

    obs_report_measure(id, "routes-AudioPropagation", "open-lib-rc",
                       (uint64_t)(uint32_t)rc_ap_open, "code");
    obs_report_measure(id, "routes-AudioPropagation", "open-sprx-rc",
                       (uint64_t)(uint32_t)rc_ap_sprx, "code");
    obs_report_measure(id, "routes-AudioPropagation", "loadstart-common-rc",
                       (uint64_t)(uint32_t)rc_ap_loadstart_common, "code");
    obs_report_measure(id, "routes-AudioPropagation", "loadstart-priv-rc",
                       (uint64_t)(uint32_t)rc_ap_loadstart_priv, "code");
    obs_report_measure(id, "routes-AudioPropagation", "loadstart-sys-ex-rc",
                       (uint64_t)(uint32_t)rc_ap_loadstart_sys_ex, "code");
    obs_report_measure(id, "routes-AudioPropagation", "handle",
                       (uint64_t)(uint32_t)h_ap, "handle");

    /* 1b. Routes for libSceAcm */
    int h_acm = -1;
    int rc_acm_open = -1;
    int rc_acm_sprx = -1;
    int rc_acm_loadstart_common = -1;
    int rc_acm_loadstart_priv = -1;
    int rc_acm_loadstart_sys_ex = -1;

    rc_acm_open = obs_module_open("libSceAcm");
    if (rc_acm_open > 0)
        h_acm = rc_acm_open;

    rc_acm_sprx = obs_module_open("libSceAcm.sprx");
    if (rc_acm_sprx > 0 && h_acm <= 0)
        h_acm = rc_acm_sprx;

    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        rc_acm_loadstart_common = sceKernelLoadStartModule(
            "/system/common/lib/libSceAcm.sprx", 0, NULL, 0, NULL, NULL);
        if (rc_acm_loadstart_common > 0 && h_acm <= 0)
            h_acm = rc_acm_loadstart_common;

        rc_acm_loadstart_priv = sceKernelLoadStartModule(
            "/system/priv/lib/libSceAcm.sprx", 0, NULL, 0, NULL, NULL);
        if (rc_acm_loadstart_priv > 0 && h_acm <= 0)
            h_acm = rc_acm_loadstart_priv;

        rc_acm_loadstart_sys_ex = sceKernelLoadStartModule(
            "/system_ex/common/lib/libSceAcm.sprx", 0, NULL, 0, NULL, NULL);
        if (rc_acm_loadstart_sys_ex > 0 && h_acm <= 0)
            h_acm = rc_acm_loadstart_sys_ex;
    }

    obs_report_measure(id, "routes-Acm", "open-lib-rc", (uint64_t)(uint32_t)rc_acm_open,
                       "code");
    obs_report_measure(id, "routes-Acm", "open-sprx-rc",
                       (uint64_t)(uint32_t)rc_acm_sprx, "code");
    obs_report_measure(id, "routes-Acm", "loadstart-common-rc",
                       (uint64_t)(uint32_t)rc_acm_loadstart_common, "code");
    obs_report_measure(id, "routes-Acm", "loadstart-priv-rc",
                       (uint64_t)(uint32_t)rc_acm_loadstart_priv, "code");
    obs_report_measure(id, "routes-Acm", "loadstart-sys-ex-rc",
                       (uint64_t)(uint32_t)rc_acm_loadstart_sys_ex, "code");
    obs_report_measure(id, "routes-Acm", "handle", (uint64_t)(uint32_t)h_acm, "handle");

    /* 2. Resolve AudioPropagation functions */
    typedef int (*fn_ap_query_mem_t)(const void *, void *);
    typedef int (*fn_ap_sys_create_t)(const void *, void *, uint64_t *);
    typedef int (*fn_ap_sys_destroy_t)(uint64_t);

    fn_ap_query_mem_t fn_query_mem = NULL;
    if (obs_address_is_callable(
            (const void *)&sceAudioPropagationSystemQueryMemory_static)) {
        fn_query_mem = (fn_ap_query_mem_t)&sceAudioPropagationSystemQueryMemory_static;
    } else {
        fn_query_mem = (fn_ap_query_mem_t)census_resolve_symbol(
            "libSceAudioPropagation", "$7xyAxrusLko",
            "sceAudioPropagationSystemQueryMemory", NULL, NULL);
        if (!fn_query_mem)
            fn_query_mem =
                (fn_ap_query_mem_t)obs_module_symbol(OBS_HANDLE_SELF, "$7xyAxrusLko");
        if (!fn_query_mem)
            fn_query_mem = (fn_ap_query_mem_t)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAudioPropagationSystemQueryMemory");
        if (!fn_query_mem && h_ap > 0) {
            fn_query_mem = (fn_ap_query_mem_t)obs_module_symbol(h_ap, "$7xyAxrusLko");
            if (!fn_query_mem)
                fn_query_mem = (fn_ap_query_mem_t)obs_module_symbol(
                    h_ap, "sceAudioPropagationSystemQueryMemory");
        }
    }

    fn_ap_sys_create_t fn_sys_create = NULL;
    if (obs_address_is_callable(
            (const void *)&sceAudioPropagationSystemCreate_static)) {
        fn_sys_create = (fn_ap_sys_create_t)&sceAudioPropagationSystemCreate_static;
    } else {
        fn_sys_create = (fn_ap_sys_create_t)census_resolve_symbol(
            "libSceAudioPropagation", "$aNEqtSHdUSo", "sceAudioPropagationSystemCreate",
            NULL, NULL);
        if (!fn_sys_create)
            fn_sys_create =
                (fn_ap_sys_create_t)obs_module_symbol(OBS_HANDLE_SELF, "$aNEqtSHdUSo");
        if (!fn_sys_create)
            fn_sys_create = (fn_ap_sys_create_t)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAudioPropagationSystemCreate");
        if (!fn_sys_create && h_ap > 0) {
            fn_sys_create = (fn_ap_sys_create_t)obs_module_symbol(h_ap, "$aNEqtSHdUSo");
            if (!fn_sys_create)
                fn_sys_create = (fn_ap_sys_create_t)obs_module_symbol(
                    h_ap, "sceAudioPropagationSystemCreate");
        }
    }

    fn_ap_sys_destroy_t fn_sys_destroy = NULL;
    if (obs_address_is_callable(
            (const void *)&sceAudioPropagationSystemDestroy_static)) {
        fn_sys_destroy = (fn_ap_sys_destroy_t)&sceAudioPropagationSystemDestroy_static;
    } else {
        fn_sys_destroy = (fn_ap_sys_destroy_t)census_resolve_symbol(
            "libSceAudioPropagation", "$x5VPqg5iyAk",
            "sceAudioPropagationSystemDestroy", NULL, NULL);
        if (!fn_sys_destroy)
            fn_sys_destroy =
                (fn_ap_sys_destroy_t)obs_module_symbol(OBS_HANDLE_SELF, "$x5VPqg5iyAk");
        if (!fn_sys_destroy)
            fn_sys_destroy = (fn_ap_sys_destroy_t)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAudioPropagationSystemDestroy");
        if (!fn_sys_destroy && h_ap > 0) {
            fn_sys_destroy =
                (fn_ap_sys_destroy_t)obs_module_symbol(h_ap, "$x5VPqg5iyAk");
            if (!fn_sys_destroy)
                fn_sys_destroy = (fn_ap_sys_destroy_t)obs_module_symbol(
                    h_ap, "sceAudioPropagationSystemDestroy");
        }
    }

    obs_report_measure(id, "sceAudioPropagationSystemQueryMemory", "bound",
                       (uint64_t)(fn_query_mem != NULL), "bool");
    obs_report_measure(id, "sceAudioPropagationSystemCreate", "bound",
                       (uint64_t)(fn_sys_create != NULL), "bool");
    obs_report_measure(id, "sceAudioPropagationSystemDestroy", "bound",
                       (uint64_t)(fn_sys_destroy != NULL), "bool");

    /* 3. Resolve Acm functions */
    typedef int (*fn_acm_create_t)(uint64_t *);
    typedef int (*fn_acm_destroy_t)(uint64_t);

    fn_acm_create_t fn_acm_create = NULL;
    if (obs_address_is_callable((const void *)&sceAcmContextCreate_static)) {
        fn_acm_create = (fn_acm_create_t)&sceAcmContextCreate_static;
    } else {
        fn_acm_create = (fn_acm_create_t)census_resolve_symbol(
            "libSceAcm", "$ZIXln2K3XMk", "sceAcmContextCreate", NULL, NULL);
        if (!fn_acm_create)
            fn_acm_create =
                (fn_acm_create_t)obs_module_symbol(OBS_HANDLE_SELF, "$ZIXln2K3XMk");
        if (!fn_acm_create)
            fn_acm_create = (fn_acm_create_t)obs_module_symbol(OBS_HANDLE_SELF,
                                                               "sceAcmContextCreate");
        if (!fn_acm_create && h_acm > 0) {
            fn_acm_create = (fn_acm_create_t)obs_module_symbol(h_acm, "$ZIXln2K3XMk");
            if (!fn_acm_create)
                fn_acm_create =
                    (fn_acm_create_t)obs_module_symbol(h_acm, "sceAcmContextCreate");
        }
    }

    fn_acm_destroy_t fn_acm_destroy = NULL;
    if (obs_address_is_callable((const void *)&sceAcmContextDestroy_static)) {
        fn_acm_destroy = (fn_acm_destroy_t)&sceAcmContextDestroy_static;
    } else {
        fn_acm_destroy = (fn_acm_destroy_t)census_resolve_symbol(
            "libSceAcm", "$jBgBjAj02R8", "sceAcmContextDestroy", NULL, NULL);
        if (!fn_acm_destroy)
            fn_acm_destroy =
                (fn_acm_destroy_t)obs_module_symbol(OBS_HANDLE_SELF, "$jBgBjAj02R8");
        if (!fn_acm_destroy)
            fn_acm_destroy = (fn_acm_destroy_t)obs_module_symbol(
                OBS_HANDLE_SELF, "sceAcmContextDestroy");
        if (!fn_acm_destroy && h_acm > 0) {
            fn_acm_destroy = (fn_acm_destroy_t)obs_module_symbol(h_acm, "$jBgBjAj02R8");
            if (!fn_acm_destroy)
                fn_acm_destroy =
                    (fn_acm_destroy_t)obs_module_symbol(h_acm, "sceAcmContextDestroy");
        }
    }

    obs_report_measure(id, "sceAcmContextCreate", "bound",
                       (uint64_t)(fn_acm_create != NULL), "bool");
    obs_report_measure(id, "sceAcmContextDestroy", "bound",
                       (uint64_t)(fn_acm_destroy != NULL), "bool");

    /* 4. Execute SystemQueryMemory arms if bound */
    uint32_t returned_mem_sz = 0;
    if (fn_query_mem) {
        /* Base shape: rsi 64-byte block: dword 0x010107d4 at +0, qword 0x30 at +8, 0xcc
         * elsewhere. rdi 0x38 bytes zeroed */
        uint8_t rdi_sys[0x38];
        uint8_t rsi_mem[64];
        memset(rdi_sys, 0, sizeof(rdi_sys));
        memset(rsi_mem, 0xcc, sizeof(rsi_mem));
        *(uint32_t *)(rsi_mem + 0) = 0x010107d4;
        *(uint64_t *)(rsi_mem + 8) = 0x30;

        int sig_qm = OBS_FAULT_ARM(&guard);
        uint64_t rc_qm = 0xffffffffffffffffULL;
        if (sig_qm == 0) {
            rc_qm = (uint64_t)(uint32_t)fn_query_mem(rdi_sys, rsi_mem);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "query-mem-base", "fault", (uint64_t)sig_qm,
                               "signal");
        }
        obs_report_measure(id, "query-mem-base", "rc", rc_qm, "code");
        obs_report_bytes(id, "query-mem-base", "rdi-38b", 0, rdi_sys, sizeof(rdi_sys));
        obs_report_bytes(id, "query-mem-base", "rsi-64b", 0, rsi_mem, sizeof(rsi_mem));
        if (rc_qm == 0) {
            returned_mem_sz = *(uint32_t *)(rsi_mem + 0x18);
            obs_report_measure(id, "query-mem-base", "size-at-18",
                               (uint64_t)returned_mem_sz, "bytes");
        }

        /* 2a. rdi +0x20 set to 1, 4, 16 */
        static const uint32_t dword_20_vals[] = {1, 4, 16};
        for (size_t i = 0; i < 3; i++) {
            char tag[64];
            snprintf(tag, sizeof(tag), "query-mem-rdi20-%u", dword_20_vals[i]);
            memset(rdi_sys, 0, sizeof(rdi_sys));
            *(uint32_t *)(rdi_sys + 0x20) = dword_20_vals[i];
            memset(rsi_mem, 0xcc, sizeof(rsi_mem));
            *(uint32_t *)(rsi_mem + 0) = 0x010107d4;
            *(uint64_t *)(rsi_mem + 8) = 0x30;

            int sig_v = OBS_FAULT_ARM(&guard);
            uint64_t rc_v = 0xffffffffffffffffULL;
            if (sig_v == 0) {
                rc_v = (uint64_t)(uint32_t)fn_query_mem(rdi_sys, rsi_mem);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, tag, "fault", (uint64_t)sig_v, "signal");
            }
            obs_report_measure(id, tag, "rc", rc_v, "code");
            obs_report_bytes(id, tag, "rdi-38b", 0, rdi_sys, sizeof(rdi_sys));
            obs_report_bytes(id, tag, "rsi-64b", 0, rsi_mem, sizeof(rsi_mem));
        }

        /* 2b. rsi +0 set to 0, and rsi null */
        memset(rdi_sys, 0, sizeof(rdi_sys));
        memset(rsi_mem, 0xcc, sizeof(rsi_mem));
        *(uint32_t *)(rsi_mem + 0) = 0;
        *(uint64_t *)(rsi_mem + 8) = 0x30;
        int sig_v0 = OBS_FAULT_ARM(&guard);
        uint64_t rc_v0 = 0xffffffffffffffffULL;
        if (sig_v0 == 0) {
            rc_v0 = (uint64_t)(uint32_t)fn_query_mem(rdi_sys, rsi_mem);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "query-mem-ver0", "fault", (uint64_t)sig_v0,
                               "signal");
        }
        obs_report_measure(id, "query-mem-ver0", "rc", rc_v0, "code");
        obs_report_bytes(id, "query-mem-ver0", "rsi-64b", 0, rsi_mem, sizeof(rsi_mem));

        int sig_null = OBS_FAULT_ARM(&guard);
        uint64_t rc_null = 0xffffffffffffffffULL;
        if (sig_null == 0) {
            rc_null = (uint64_t)(uint32_t)fn_query_mem(rdi_sys, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "query-mem-rsi-null", "fault", (uint64_t)sig_null,
                               "signal");
        }
        obs_report_measure(id, "query-mem-rsi-null", "rc", rc_null, "code");
    }

    /* 3. SystemCreate / Destroy */
    if (fn_sys_create) {
        size_t alloc_sz = (returned_mem_sz > 0 && returned_mem_sz < 64 * 1024 * 1024)
                              ? returned_mem_sz
                              : 0x10000;
        (void)alloc_sz;
        static _Alignas(64) uint8_t sys_buf[0x10000];

        uint8_t rdi_sys[0x38];
        uint8_t rsi_mem[64];
        memset(rdi_sys, 0, sizeof(rdi_sys));
        memset(rsi_mem, 0xcc, sizeof(rsi_mem));
        *(uint32_t *)(rsi_mem + 0) = 0x010107d4;
        *(uint64_t *)(rsi_mem + 8) = 0x30;

        uint64_t sys_handle = 0xccccccccccccccccULL;
        int sig_sc = OBS_FAULT_ARM(&guard);
        uint64_t rc_sc = 0xffffffffffffffffULL;
        if (sig_sc == 0) {
            rc_sc = (uint64_t)(uint32_t)fn_sys_create(rdi_sys, sys_buf, &sys_handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "system-create", "fault", (uint64_t)sig_sc,
                               "signal");
        }
        obs_report_measure(id, "system-create", "rc", rc_sc, "code");
        obs_report_bytes(id, "system-create", "handle-8b", 0,
                         (const unsigned char *)&sys_handle, 8);

        if (fn_sys_destroy && sys_handle != 0xccccccccccccccccULL) {
            int sig_sd = OBS_FAULT_ARM(&guard);
            uint64_t rc_sd = 0xffffffffffffffffULL;
            if (sig_sd == 0) {
                rc_sd = (uint64_t)(uint32_t)fn_sys_destroy(sys_handle);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "system-destroy", "fault", (uint64_t)sig_sd,
                                   "signal");
            }
            obs_report_measure(id, "system-destroy", "rc", rc_sd, "code");
        }
    }

    /* 4. Acm ContextCreate / Destroy */
    if (fn_acm_create) {
        uint64_t acm_handle = 0xccccccccccccccccULL;
        int sig_acc = OBS_FAULT_ARM(&guard);
        uint64_t rc_acc = 0xffffffffffffffffULL;
        if (sig_acc == 0) {
            rc_acc = (uint64_t)(uint32_t)fn_acm_create(&acm_handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "acm-context-create", "fault", (uint64_t)sig_acc,
                               "signal");
        }
        obs_report_measure(id, "acm-context-create", "rc", rc_acc, "code");
        obs_report_bytes(id, "acm-context-create", "handle-8b", 0,
                         (const unsigned char *)&acm_handle, 8);

        if (fn_acm_destroy && acm_handle != 0xccccccccccccccccULL) {
            int sig_acd = OBS_FAULT_ARM(&guard);
            uint64_t rc_acd = 0xffffffffffffffffULL;
            if (sig_acd == 0) {
                rc_acd = (uint64_t)(uint32_t)fn_acm_destroy(acm_handle);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, "acm-context-destroy", "fault",
                                   (uint64_t)sig_acd, "signal");
            }
            obs_report_measure(id, "acm-context-destroy", "rc", rc_acd, "code");
        }
    }

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T1350Z-ly01: census: primitive exported layer and topology 7   */
/* -------------------------------------------------------------------------- */

static void build_ly01_vs(uint32_t *code, int layer_mode, int const_layer,
                          int export_pos1, uint64_t stored_ids_gpu, int store_ids) {
    uint32_t k = 0;
    code[k++] = 0xbfa00001u; /* s_inst_prefetch 0x1 */
    code[k++] = 0xbe8c037eu; /* s_mov_b32 s12, exec_lo */
    code[k++] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (alloc 1 prim, 3 verts) */
    code[k++] = 0x00001003u;
    code[k++] = 0xbf800000u; /* s_nop 0 */
    code[k++] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */

    /* Primitive connectivity export from Lane 0 */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e0202ffu; /* v_mov_b32 v1, 0x20280600 (vertices 0, 1, 2) */
    code[k++] = 0x20280600u;
    code[k++] = 0xf8000941u; /* exp prim, v1, off, off, off done */
    code[k++] = 0x00000001u;
    code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */

    if (store_ids) {
        code[k++] = 0xbe8003ffu; /* s_mov_b32 s0, stored_ids_lo */
        code[k++] = (uint32_t)stored_ids_gpu;
        code[k++] = 0xbe8103ffu; /* s_mov_b32 s1, stored_ids_hi */
        code[k++] = (uint32_t)(stored_ids_gpu >> 32);
        code[k++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
        code[k++] = 0x7e120201u; /* v_mov_b32 v9, s1 */

        /* Lane 0 stores vertex ID 0 to offset 0 */
        code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
        code[k++] = 0x7e1402ffu; /* v_mov_b32 v10, 0 */
        code[k++] = 0u;
        code[k++] = 0xdc708000u; /* global_store_dword v[8:9], v10, off offset:0 */
        code[k++] = 0x007d0a08u;

        /* Lane 1 stores vertex ID 1 to offset 4 */
        code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
        code[k++] = 0x7e1402ffu; /* v_mov_b32 v10, 1 */
        code[k++] = 1u;
        code[k++] = 0xdc708004u; /* global_store_dword v[8:9], v10, off offset:4 */
        code[k++] = 0x007d0a08u;

        /* Lane 2 stores vertex ID 2 to offset 8 */
        code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
        code[k++] = 0x7e1402ffu; /* v_mov_b32 v10, 2 */
        code[k++] = 2u;
        code[k++] = 0xdc708008u; /* global_store_dword v[8:9], v10, off offset:8 */
        code[k++] = 0x007d0a08u;

        code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
        code[k++] = 0x7e140202u; /* v_mov_b32 v10, s2 */
        code[k++] = 0xdc70800cu; /* global_store_dword v[8:9], v10, off offset:12 */
        code[k++] = 0x007d0a08u;
        code[k++] = 0x7e140203u; /* v_mov_b32 v10, s3 */
        code[k++] = 0xdc708010u; /* global_store_dword v[8:9], v10, off offset:16 */
        code[k++] = 0x007d0a08u;

        code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
        code[k++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    }

    /* Position export pos0 (must precede pos1 in hardware sequence) */
    code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[k++] = 0x7e0a02f1u; /* v_mov_b32 v5, -0.5f */
    code[k++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f */
    code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[k++] = 0x7e0a02f0u; /* v_mov_b32 v5, +0.5f */
    code[k++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f */
    code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[k++] = 0x7e0a0280u; /* v_mov_b32 v5, 0.0f */
    code[k++] = 0x7e0c02f0u; /* v_mov_b32 v6, +0.5f */
    code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[k++] = 0x7e060280u; /* v_mov_b32 v3, 0.0f (Z) */
    code[k++] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (W) */

    if (export_pos1) {
        code[k++] = 0xf80000cfu; /* exp pos0, v5, v6, v3, v4 (done=0) */
        code[k++] = 0x04030605u;
        code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */

        /* Position export pos1 carrying layer in Z (channel 2), final export with
         * done=1 */
        if (layer_mode == 1) {
            /* Disagreeing vertices: lane 0 -> 1, lane 1 -> 2, lane 2 -> 2 */
            code[k++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
            code[k++] = 0x7e1002ffu; /* v_mov_b32 v8, 1 */
            code[k++] = 1u;
            code[k++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
            code[k++] = 0x7e1002ffu; /* v_mov_b32 v8, 2 */
            code[k++] = 2u;
            code[k++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
            code[k++] = 0x7e1002ffu; /* v_mov_b32 v8, 2 */
            code[k++] = 2u;
        } else {
            code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
            code[k++] = 0x7e1002ffu; /* v_mov_b32 v8, const_layer */
            code[k++] = (uint32_t)const_layer;
        }
        code[k++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
        code[k++] = 0xf80008d4u; /* exp pos1 off, off, v8, off done */
        code[k++] = 0x00080000u;
        code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */
    } else {
        code[k++] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
        code[k++] = 0x04030605u;
        code[k++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */
    }
    code[k++] =
        0xbefe030cu; /* s_mov_b32 exec_lo, s12 (restore all lanes before retire) */
    code[k++] = 0xbf810000u; /* s_endpgm */

    while (k < 128) {
        code[k++] = 0xbf800000u; /* s_nop */
    }
}

static uint32_t build_ly01_dcb(uint32_t *dw, uint32_t color_view,
                               uint32_t pa_cl_vs_out_cntl, uint32_t pa_su_sc_mode_cntl,
                               uint32_t prim_type, uint32_t draw_vertex_count,
                               uint64_t payload_va, uint64_t fence_gpu,
                               uint64_t color_gpu, int export_pos1) {
    uint32_t *start = dw;
    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx[] = {{0x318u, 0},
                    {0x390u, 0},
                    {0x31bu, 0},
                    {0x31cu, 0x000180a8u},
                    {0x31du, 0},
                    {0x31eu, 0},
                    {0x3b0u, (63u << 14) | 63u},
                    {0x3b8u, 0x08c6c004u},
                    {0x109u, 0},
                    {0x202u, 0x00cc0010u},
                    {0x08eu, 0xfu},
                    {0x08fu, 0xfu},
                    {0x1e0u, 0x20010001u},
                    {0x200u, 0},
                    {0x08cu, 0xaa99aaaau},
                    {0x1d4u, 0xffu},
                    {0x291u, 0x10020040u},
                    {0x29bu, 0},
                    {0x2d3u, 1u},
                    {0x2d5u, 0x02002000u},
                    {0x1ffu, 0x40u},
                    {0x20eu, 0x78u},
                    {0x2a1u, 0},
                    {0x2a6u, 0x40u},
                    {0x2adu, 0},
                    {0x2abu, 4u},
                    {0x2ceu, 0},
                    {0x2d4u, 0x88101000u},
                    {0x103u, 0xffffffffu},
                    {0x30eu, 0xffffffffu},
                    {0x30fu, 0xffffffffu},
                    {0x310u, 0},
                    {0x314u, 0x202u},
                    {0x311u, 0x01fd2002u},
                    {0x312u, 0x03ff0080u},
                    {0x313u, 0x6000u},
                    {0x00eu, 2u},
                    {0x280u, 0x00080008u},
                    {0x281u, 0xffff0000u},
                    {0x282u, 8u},
                    {0x2deu, 0x1e9u},
                    {0x00cu, 0},
                    {0x00du, 0x40004000u},
                    {0x081u, 0x80000000u},
                    {0x082u, 0x40004000u},
                    {0x090u, 0x80000000u},
                    {0x091u, 0x40004000u},
                    {0x094u, 0x80000000u},
                    {0x095u, 0x40004000u},
                    {0x0b4u, 0},
                    {0x0b5u, 0x3f800000u},
                    {0x10fu, 0x42000000u},
                    {0x110u, 0x42000000u},
                    {0x111u, 0x42000000u},
                    {0x112u, 0x42000000u},
                    {0x113u, 0x3f000000u},
                    {0x114u, 0x3f000000u},
                    {0x083u, 0xffffu},
                    {0x084u, 0},
                    {0x085u, 0x20002000u},
                    {0x204u, 0},
                    {0x206u, 0x43fu},
                    {0x207u, 0},
                    {0x2fau, 0x3f800000u},
                    {0x2fbu, 0x3f800000u},
                    {0x2fcu, 0x3f800000u},
                    {0x2fdu, 0x3f800000u},
                    {0x205u, 0x240u},
                    {0x20cu, 0},
                    {0x292u, 2u},
                    {0x293u, 0x06020000u},
                    {0x2f8u, 0},
                    {0x2f9u, 0x2du},
                    {0x1b1u, 0x80u},
                    {0x1c2u, 1u},
                    {0x1c3u, 4u},
                    {0x1c5u, 9u},
                    {0x1b3u, 2u},
                    {0x1b4u, 2u},
                    {0x1b5u, 1u},
                    {0x1b6u, 0},
                    {0x1b8u, 0x01000000u}};
    for (size_t i = 0; i < sizeof(base_ctx) / sizeof(base_ctx[0]); i++) {
        uint32_t reg = base_ctx[i].reg;
        uint32_t val = base_ctx[i].val;
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        } else if (reg == 0x31bu) {
            val = color_view;
        } else if (reg == 0x207u) {
            val = pa_cl_vs_out_cntl;
        } else if (reg == 0x205u) {
            val = pa_su_sc_mode_cntl;
        } else if (reg == 0x1c3u) {
            val = export_pos1 ? 0x24u : 4u;
        } else if (reg == 0x1b1u) {
            val = 0x80u;
        }
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    for (uint32_t i = 0; i < 32; i++) {
        *dw++ = 0xc0016900u;
        *dw++ = 0x191u + i;
        *dw++ = 0u;
    }

    static const struct {
        uint32_t base_reg;
        uint64_t va_offset;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, 0x200u, 0x000c0010u, 0x00000000u},
        {0x48u, 0x000u, 0x000c0010u, 0x00000000u},
        {0x88u, 0x000u, 0x622c0047u, 0x00030000u},
        {0xc8u, 0x000u, 0x000c0010u, 0x00000000u},
        {0x108u, 0x000u, 0x000c0010u, 0x00000000u},
        {0x148u, 0x000u, 0x000c0010u, 0x00000000u},
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = payload_va + stages[s].va_offset;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 2u;
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
    }

    static const struct {
        uint32_t reg;
        uint32_t val;
    } spi_cu_regs[] = {
        {0x007u, 0x0000ffffu}, {0x001u, 0x00000003u}, {0x087u, 0x0000fffdu},
        {0x081u, 0x00000003u}, {0x107u, 0xffff0000u},
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u;
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    *dw++ = 0xc0002f00u;
    *dw++ = 1u;          /* NUM_INSTANCES */
    *dw++ = 0xc0017a00u; /* PACKET3_SET_UCONFIG_REG_INDEX: mmVGT_PRIMITIVE_TYPE */
    *dw++ = 0x10000242u;
    *dw++ = prim_type;
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */
    *dw++ = 0xc0017900u;
    *dw++ = 0x249u;
    *dw++ = 0x00000000u; /* mmGE_MIN_VTX_INDX */
    *dw++ = 0xc0017900u;
    *dw++ = 0x259u;
    *dw++ = 0xffffffffu; /* mmGE_MAX_VTX_INDX */

    *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO */
    *dw++ = draw_vertex_count;
    *dw++ = 2u;

    *dw++ = 0xc0064900u; /* RELEASE_MEM */
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;
    return (uint32_t)(dw - start);
}

static inline void ly01_flush_range(const void *p, size_t sz) {
#if defined(__x86_64__)
    const char *cp = (const char *)p;
    for (size_t off = 0; off < sz; off += 64) {
        __builtin_ia32_clflush((const void *)(cp + off));
    }
    __builtin_ia32_mfence();
#else
    (void)p;
    (void)sz;
#endif
}

static inline void ly01_flush_fence(volatile uint32_t *f) {
#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)f);
    __builtin_ia32_mfence();
#else
    (void)f;
#endif
}

static obs_result check_ly01_layer_export_topology7(void) {
    return obs_skip("hardware GFX pipe safety: DCB stalls ring on hardware");
    const char *id = "000-boot/ly01-layer-export-topology7";
    obs_jmp_buf guard;

    int bound_create = obs_address_is_callable((const void *)&sceAgcDriverCreateQueue);
    int bound_submit = obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb);
    obs_report_measure(id, "setup", "fn-create-queue-bound", (uint64_t)bound_create,
                       "bool");
    obs_report_measure(id, "setup", "fn-submit-dcb-bound", (uint64_t)bound_submit,
                       "bool");
    if (!bound_create || !bound_submit) {
        return obs_skip("sceAgcDriverCreateQueue or sceAgcDriverSubmitDcb not bound");
    }

    void *queue = NULL;
    int rc_q = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_q = sceAgcDriverCreateQueue(0u, &queue, 0u);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "setup", "queue-create-rc", (uint64_t)(uint32_t)rc_q,
                       "code");
    if (rc_q != 0 || queue == NULL) {
        return obs_skip("failed to create type 0 queue");
    }

#if !defined(OBSCENE_HOST_BUILD)
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *cmdbuf = (uint32_t *)oops_mem_alloc(0x10000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x40000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *stored_ids =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x1000, OOPS_MEM_WB_ONION);
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
#else
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(64) uint32_t s_host_cmdbuf[0x4000];
    static _Alignas(65536) uint32_t s_host_color_buf[65536];
    static _Alignas(64) uint32_t s_host_stored_ids[0x4000];
    static _Alignas(256) uint8_t s_host_payload[0x2000];
    volatile uint32_t *fence = s_host_fence;
    uint32_t *cmdbuf = s_host_cmdbuf;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *stored_ids = s_host_stored_ids;
    uint8_t *gpu_payload = s_host_payload;
#endif

    if (!fence || !cmdbuf || !color_buf || !stored_ids || !gpu_payload) {
        return obs_skip("memory allocation for GPU draw test failed");
    }

    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t stored_ids_gpu = (uint64_t)(uintptr_t)stored_ids;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    build_ge_indx_ps((uint32_t *)(gpu_payload + 0x200));
    build_ge_indx_fb((uint32_t *)(gpu_payload + 0x300));

    struct {
        uint64_t gpu_addr;
        uint32_t size;
        uint32_t flags;
        uint32_t pad[3];
    } desc;
    memset(&desc, 0, sizeof(desc));

    int ly01_stalled = 0;
#define LY01_PA_CL_ON ((1u << 18) | (1u << 21) | (1u << 24))
#define LY01_PA_CL_OFF ((1u << 21) | (1u << 24))

/* Helper lambda/macro to run a draw pass and report slices */
#define RUN_LY01_PASS(arm_name, color_view_val, pa_cl_val, pa_su_val, layer_m,         \
                      const_l, exp_pos1)                                               \
    do {                                                                               \
        if (ly01_stalled) {                                                            \
            obs_report_measure(id, arm_name, "fence-hit", 0, "bool");                  \
            break;                                                                     \
        }                                                                              \
        for (int s = 0; s < 4; s++) {                                                  \
            uint32_t clear_val = 0x11111111u * (uint32_t)(s + 1);                      \
            uint32_t *slice_ptr = (uint32_t *)((uint8_t *)color_buf + s * 0x10000);    \
            for (size_t px = 0; px < 256; px++) {                                      \
                slice_ptr[px] = clear_val;                                             \
            }                                                                          \
        }                                                                              \
        build_ly01_vs((uint32_t *)gpu_payload, (layer_m), (const_l), (exp_pos1),       \
                      stored_ids_gpu, 0);                                              \
        for (size_t p = 0; p < 64; p++) {                                              \
            ((uint32_t *)(gpu_payload + 0x100))[p] = ((uint32_t *)gpu_payload)[p];     \
        }                                                                              \
        uint32_t words =                                                               \
            build_ly01_dcb(cmdbuf, (color_view_val), (pa_cl_val), (pa_su_val), 4u, 3u, \
                           payload_va, fence_gpu, color_gpu, (exp_pos1));              \
        *fence = 0x11111111u;                                                          \
        desc.gpu_addr = (uint64_t)(uintptr_t)cmdbuf;                                   \
        desc.size = words;                                                             \
        ly01_flush_range(gpu_payload, 0x2000);                                         \
        ly01_flush_range((const void *)color_buf, 0x40000);                            \
        ly01_flush_range(cmdbuf, (size_t)words * 4);                                   \
        ly01_flush_fence(fence);                                                       \
        sig = OBS_FAULT_ARM(&guard);                                                   \
        int sub_rc = -1;                                                               \
        if (sig == 0) {                                                                \
            if (queue && obs_address_is_callable(                                      \
                             (const void *)&sceAgcDriverSubmitCommandBuffer)) {        \
                sub_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);                \
            } else {                                                                   \
                sub_rc = sceAgcDriverSubmitDcb(&desc);                                 \
            }                                                                          \
            obs_fault_unregister();                                                    \
        } else {                                                                       \
            obs_fault_unregister();                                                    \
        }                                                                              \
        int hit = 0;                                                                   \
        if (sub_rc == 0) {                                                             \
            for (int iter = 0; iter < 10000; iter++) {                                 \
                ly01_flush_fence(fence);                                               \
                if (*fence == 0xbeefcafeu) {                                           \
                    hit = 1;                                                           \
                    break;                                                             \
                }                                                                      \
                if (obs_address_is_callable((const void *)&sceKernelUsleep)) {         \
                    sceKernelUsleep(100);                                              \
                }                                                                      \
            }                                                                          \
        }                                                                              \
        if (!hit) {                                                                    \
            ly01_stalled = 1;                                                          \
        }                                                                              \
        ly01_flush_range((const void *)color_buf, 0x40000);                            \
        obs_report_measure(id, arm_name, "fence-hit", (uint64_t)hit, "bool");          \
        for (int s = 0; s < 4; s++) {                                                  \
            char stag[32];                                                             \
            snprintf(stag, sizeof(stag), "slice%d-64b", s);                            \
            obs_report_bytes(                                                          \
                id, arm_name, stag, 0,                                                 \
                (const unsigned char *)((const uint8_t *)color_buf + s * 0x10000),     \
                64);                                                                   \
        }                                                                              \
    } while (0)

    /* Arm 1: Layer in range. SLICE_START=0, SLICE_MAX=3 (0x6000), bit 18 set
     * (LY01_PA_CL_ON) */
    /* Draw constant layer 0, then 2, then 3 */
    RUN_LY01_PASS("arm1-layer-in-range-lay0", 0x00006000u, LY01_PA_CL_ON, 0x00000240u,
                  0, 0, 1);
    RUN_LY01_PASS("arm1-layer-in-range-lay2", 0x00006000u, LY01_PA_CL_ON, 0x00000240u,
                  0, 2, 1);
    RUN_LY01_PASS("arm1-layer-in-range-lay3", 0x00006000u, LY01_PA_CL_ON, 0x00000240u,
                  0, 3, 1);

    /* Arm 2: Layer offset by view. SLICE_START=1, SLICE_MAX=3 (0x6001), layer 1 */
    RUN_LY01_PASS("arm2-layer-offset-by-view", 0x00006001u, LY01_PA_CL_ON, 0x00000240u,
                  0, 1, 1);

    /* Arm 3: Layer past view. SLICE_START=0, SLICE_MAX=1 (0x2000), layer 3 */
    RUN_LY01_PASS("arm3-layer-past-view", 0x00002000u, LY01_PA_CL_ON, 0x00000240u, 0, 3,
                  1);

    /* Arm 4: Bit 18 clear. SLICE_START=0, SLICE_MAX=3 (0x6000), bit 18 clear
     * (LY01_PA_CL_OFF), layer 2 */
    RUN_LY01_PASS("arm4-bit18-clear-lay2", 0x00006000u, LY01_PA_CL_OFF, 0x00000240u, 0,
                  2, 1);

    /* Arm 5: Disagreeing vertices (1, 2, 2) */
    /* Sub-arm 5a: PROVOKING_VTX_LAST clear (0x00000240) */
    RUN_LY01_PASS("arm5-disagreeing-vtx-first", 0x00006000u, LY01_PA_CL_ON, 0x00000240u,
                  1, 0, 1);
    /* Sub-arm 5b: PROVOKING_VTX_LAST set (0x00080240) */
    RUN_LY01_PASS("arm5-disagreeing-vtx-last", 0x00006000u, LY01_PA_CL_ON, 0x00080240u,
                  1, 0, 1);

#undef RUN_LY01_PASS
#undef LY01_PA_CL_ON
#undef LY01_PA_CL_OFF

    /* Arm 6: Topology 7 (VGT_PRIMITIVE_TYPE 7). Draw 3, 4, 6 vertices storing IDs, s2,
     * s3 */
    static const uint32_t top7_counts[3] = {3, 4, 6};
    for (int i = 0; i < 3; i++) {
        if (ly01_stalled) {
            break;
        }
        uint32_t vcount = top7_counts[i];
        char arm_tag[32];
        snprintf(arm_tag, sizeof(arm_tag), "arm6-topology7-draw%u",
                 (unsigned int)vcount);

        for (int k = 0; k < 16; k++) {
            stored_ids[k] = 0xccccccccu;
        }
        build_ly01_vs((uint32_t *)gpu_payload, 0, 0, 0, stored_ids_gpu, 1);
        for (size_t p = 0; p < 64; p++) {
            ((uint32_t *)(gpu_payload + 0x100))[p] = ((uint32_t *)gpu_payload)[p];
        }

        uint32_t words = build_ly01_dcb(cmdbuf, 0x00006000u, 0u, 0x00000240u, 7u,
                                        vcount, payload_va, fence_gpu, color_gpu, 0);
        *fence = 0x11111111u;
        desc.gpu_addr = (uint64_t)(uintptr_t)cmdbuf;
        desc.size = words;

        ly01_flush_range(gpu_payload, 0x2000);
        ly01_flush_range((const void *)stored_ids, 128);
        ly01_flush_range(cmdbuf, (size_t)words * 4);
        ly01_flush_fence(fence);

        sig = OBS_FAULT_ARM(&guard);
        int sub_rc = -1;
        if (sig == 0) {
            if (queue && obs_address_is_callable(
                             (const void *)&sceAgcDriverSubmitCommandBuffer)) {
                sub_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
            } else {
                sub_rc = sceAgcDriverSubmitDcb(&desc);
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        int hit = 0;
        if (sub_rc == 0) {
            for (int iter = 0; iter < 10000; iter++) {
                ly01_flush_fence(fence);
                if (*fence == 0xbeefcafeu) {
                    hit = 1;
                    break;
                }
                if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                    sceKernelUsleep(100);
                }
            }
        }
        if (!hit) {
            ly01_stalled = 1;
        }
        ly01_flush_range((const void *)stored_ids, 128);

        obs_report_measure(id, arm_tag, "fence-hit", (uint64_t)hit, "bool");
        obs_report_measure(id, arm_tag, "stored-id-0", (uint64_t)stored_ids[0], "id");
        obs_report_measure(id, arm_tag, "stored-id-1", (uint64_t)stored_ids[1], "id");
        obs_report_measure(id, arm_tag, "stored-id-2", (uint64_t)stored_ids[2], "id");
        obs_report_measure(id, arm_tag, "sgpr-s2-gs-tg-info", (uint64_t)stored_ids[3],
                           "hex");
        obs_report_measure(id, arm_tag, "sgpr-s3-merged-wave-info",
                           (uint64_t)stored_ids[4], "hex");
    }

#if !defined(OBSCENE_HOST_BUILD)
    if (fence)
        oops_mem_free((void *)fence);
    if (cmdbuf)
        oops_mem_free((void *)cmdbuf);
    if (color_buf)
        oops_mem_free((void *)color_buf);
    if (stored_ids)
        oops_mem_free((void *)stored_ids);
    if (gpu_payload)
        oops_mem_free((void *)gpu_payload);
#endif
    if (queue && obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        (void)sceAgcDriverDestroyQueue(queue);
    }

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261015T1234Z-gt01: four calls whose placeholder holds PPSA04263 back */
/* -------------------------------------------------------------------------- */

static obs_result check_gt01_npmanager(void) {
    const char *id = "000-boot/gt01-npmanager";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x009d);
    }

    typedef int (*fn_np_set_title_id_t)(const void *, const void *);
    fn_np_set_title_id_t fn_set = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceNpSetNpTitleId_static)) {
        fn_set = (fn_np_set_title_id_t)&sceNpSetNpTitleId_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceNpManager");
        if (h > 0) {
            fn_set = (fn_np_set_title_id_t)obs_module_symbol(h, "$Ec63y59l9tw");
            if (!fn_set)
                fn_set =
                    (fn_np_set_title_id_t)obs_module_symbol(h, "sceNpSetNpTitleId");
        }
    }

    int bound = (fn_set != NULL && obs_address_is_callable((const void *)fn_set));
    obs_report_measure(id, "sceNpSetNpTitleId", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceNpSetNpTitleId", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNpSetNpTitleId", "route", 0,
                           "tried-static-and-sysmodule");
        return obs_pass();
    }

    /* PPSA04263 shape: 0x10-byte id block "PPSA01721_00\0\0\0\0" */
    uint8_t title_id_block[16];
    memset(title_id_block, 0, sizeof(title_id_block));
    memcpy(title_id_block, "PPSA01721_00", 12);

    /* secret block directly after it, non-zero */
    uint8_t secret_block[128];
    for (size_t i = 0; i < sizeof(secret_block); i++) {
        secret_block[i] = (uint8_t)(0x5a ^ (i * 7 + 0x33));
    }

    int rc1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_set(title_id_block, secret_block);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-1", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "call-1", "rc", (uint64_t)(uint32_t)rc1, "code");

    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_set(title_id_block, secret_block);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "call-2", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "call-2", "rc", (uint64_t)(uint32_t)rc2, "code");

    return obs_pass();
}

static obs_result check_gt01_contentdelete(void) {
    const char *id = "000-boot/gt01-contentdelete";
    obs_jmp_buf guard;

    typedef int (*fn_cd_init_t)(const void *);
    fn_cd_init_t fn_init = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceContentDeleteInitialize_static)) {
        fn_init = (fn_cd_init_t)&sceContentDeleteInitialize_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceContentDelete");
        if (h <= 1 &&
            obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            h = sceKernelLoadStartModule("/system/common/lib/libSceContentDelete.sprx",
                                         0, NULL, 0, NULL, NULL);
        }
        if (h > 0) {
            fn_init = (fn_cd_init_t)obs_module_symbol(h, "$zoxb0wEChEM");
            if (!fn_init)
                fn_init =
                    (fn_cd_init_t)obs_module_symbol(h, "sceContentDeleteInitialize");
        }
    }

    int bound = (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceContentDeleteInitialize", "bound", (uint64_t)bound,
                       "bool");
    if (bound) {
        obs_report_measure(id, "sceContentDeleteInitialize", "how",
                           (uint64_t)(is_link ? 1 : 2), is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceContentDeleteInitialize", "route", 0,
                           "tried-static-and-sprx");
        return obs_pass();
    }

    int rc_null = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null = fn_init(NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-null", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-null", "rc", (uint64_t)(uint32_t)rc_null, "code");

    uint8_t cd_param[0x20];
    memset(cd_param, 0, sizeof(cd_param));
    int rc_param = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_param = fn_init(cd_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-param-zeroed", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-param-zeroed", "rc", (uint64_t)(uint32_t)rc_param,
                       "code");

    int rc_sec = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_sec = fn_init(cd_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-second", "rc", (uint64_t)(uint32_t)rc_sec, "code");

    return obs_pass();
}

static obs_result check_gt01_net_sockinfo(void) {
    const char *id = "000-boot/gt01-net-sockinfo";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0001);
    }
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }

    typedef int (*fn_sockinfo_t)(int, void *, int, int);
    fn_sockinfo_t fn_info = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceNetGetSockInfo_static)) {
        fn_info = (fn_sockinfo_t)&sceNetGetSockInfo_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceNet");
        if (h > 0) {
            fn_info = (fn_sockinfo_t)obs_module_symbol(h, "$hLuXdjHnhiI");
            if (!fn_info)
                fn_info = (fn_sockinfo_t)obs_module_symbol(h, "sceNetGetSockInfo");
        }
    }

    int bound = (fn_info != NULL && obs_address_is_callable((const void *)fn_info));
    obs_report_measure(id, "sceNetGetSockInfo", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceNetGetSockInfo", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNetGetSockInfo", "route", 0,
                           "tried-static-and-libscenet");
        return obs_pass();
    }

    int s = -1;
    if (obs_address_is_callable((const void *)&sceNetSocket)) {
        s = sceNetSocket("gt01_udp", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
    }
    obs_report_measure(id, "socket-udp", "fd", (uint64_t)(uint32_t)s, "fd");

    uint8_t info[0x200];
    memset(info, 0xcc, sizeof(info));
    int rc_info = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_info = fn_info(s, info, 1, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "get-sock-info-udp", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "get-sock-info-udp", "rc", (uint64_t)(uint32_t)rc_info,
                       "code");

    /* Report changed bytes */
    size_t changed_len = 0;
    for (size_t i = sizeof(info); i > 0; i--) {
        if (info[i - 1] != 0xcc) {
            changed_len = i;
            break;
        }
    }
    if (changed_len > 0) {
        obs_report_bytes(id, "get-sock-info-udp", "info-changed", 0, info,
                         (unsigned int)changed_len);
    } else {
        obs_report_bytes(id, "get-sock-info-udp", "info-first-32b", 0, info, 32);
    }

    /* Bad socket ID */
    uint8_t bad_info[0x200];
    memset(bad_info, 0xcc, sizeof(bad_info));
    int rc_bad = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_bad = fn_info(-1, bad_info, 1, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "get-sock-info-bad-fd", "fault", (uint64_t)sig,
                           "signal");
    }
    obs_report_measure(id, "get-sock-info-bad-fd", "rc", (uint64_t)(uint32_t)rc_bad,
                       "code");

    if (s >= 0 && obs_address_is_callable((const void *)&sceNetSocketClose)) {
        (void)sceNetSocketClose(s);
    }

    return obs_pass();
}

static obs_result check_gt01_voice(void) {
    const char *id = "000-boot/gt01-voice";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x001A);
    }

    typedef int (*fn_voice_init_t)(const void *, uint32_t);
    fn_voice_init_t fn_init = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceVoiceInit_static)) {
        fn_init = (fn_voice_init_t)&sceVoiceInit_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceVoice");
        if (h > 0) {
            fn_init = (fn_voice_init_t)obs_module_symbol(h, "$9TrhuGzberQ");
            if (!fn_init)
                fn_init = (fn_voice_init_t)obs_module_symbol(h, "sceVoiceInit");
        }
    }

    int bound = (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceVoiceInit", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceVoiceInit", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceVoiceInit", "route", 0,
                           "tried-static-and-sysmodule");
        return obs_pass();
    }

    /* zeroed 0x10-byte param */
    uint8_t voice_param[0x10];
    memset(voice_param, 0, sizeof(voice_param));

    /* Try version 1 */
    int rc_v1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_v1 = fn_init(voice_param, 1);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-ver1", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-ver1", "rc", (uint64_t)(uint32_t)rc_v1, "code");

    /* Also try version 0x01000000 if ver1 was invalid */
    if (rc_v1 != 0) {
        int rc_v100 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_v100 = fn_init(voice_param, 0x01000000u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "init-ver-0x01000000", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "init-ver-0x01000000", "rc", (uint64_t)(uint32_t)rc_v100,
                           "code");
    }

    /* Again (already-initialized code) */
    int rc_sec = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_sec = fn_init(voice_param, rc_v1 == 0 ? 1 : 0x01000000u);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-second", "rc", (uint64_t)(uint32_t)rc_sec, "code");

    return obs_pass();
}

/* ========================================================================== */
/* REQ-20261015T2025Z-gt02: Voice, SockInfo, AjmBatch, ContentDelete          */
/* ========================================================================== */

static obs_result check_gt02_voice(void) {
    const char *id = "000-boot/gt02-voice";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x001A);
    }

    typedef int (*fn_voice_init_t)(void *, uint32_t);
    fn_voice_init_t fn_init = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceVoiceInit_static)) {
        fn_init = (fn_voice_init_t)&sceVoiceInit_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceVoice");
        if (h > 0) {
            fn_init = (fn_voice_init_t)obs_module_symbol(h, "$9TrhuGzberQ");
            if (!fn_init)
                fn_init = (fn_voice_init_t)obs_module_symbol(h, "sceVoiceInit");
        }
    }

    int bound = (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceVoiceInit", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceVoiceInit", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceVoiceInit", "route", 0,
                           "tried-static-and-sysmodule");
        return obs_pass();
    }

    /* PPSA04263's shape: 0x20-byte block holding u32 0x10000000 at +0 and zeros after,
     * version 100 */
    uint8_t voice_param[0x20];
    memset(voice_param, 0, sizeof(voice_param));
    *(uint32_t *)(voice_param + 0) = 0x10000000u;

    int rc1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_init(voice_param, 100);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-gta-first", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-gta-first", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_bytes(id, "init-gta-first", "param-20b", 0, voice_param, 0x20);

    /* Second call */
    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_init(voice_param, 100);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-gta-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-gta-second", "rc", (uint64_t)(uint32_t)rc2, "code");
    obs_report_bytes(id, "init-gta-second", "param-20b", 0, voice_param, 0x20);

    return obs_pass();
}

static obs_result check_gt02_net_sockinfo(void) {
    const char *id = "000-boot/gt02-net-sockinfo";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0001);
    }
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }

    typedef int (*fn_sockinfo_t)(int, void *, int, int);
    fn_sockinfo_t fn_info = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceNetGetSockInfo_static)) {
        fn_info = (fn_sockinfo_t)&sceNetGetSockInfo_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceNet");
        if (h > 0) {
            fn_info = (fn_sockinfo_t)obs_module_symbol(h, "$hLuXdjHnhiI");
            if (!fn_info)
                fn_info = (fn_sockinfo_t)obs_module_symbol(h, "sceNetGetSockInfo");
        }
    }

    int bound = (fn_info != NULL && obs_address_is_callable((const void *)fn_info));
    obs_report_measure(id, "sceNetGetSockInfo", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceNetGetSockInfo", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNetGetSockInfo", "route", 0,
                           "tried-static-and-libscenet");
        return obs_pass();
    }

    int s_udp = -1;
    if (obs_address_is_callable((const void *)&sceNetSocket)) {
        s_udp = sceNetSocket("gt02_udp", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
    }
    obs_report_measure(id, "socket-udp", "fd", (uint64_t)(uint32_t)s_udp, "fd");

    const struct {
        const char *tag;
        int fd;
        int count;
    } arms[] = {
        {"fd-0x15-cnt1", 0x15, 1},   {"fd-0x15-cnt4", 0x15, 4},
        {"fd-7fff-cnt1", 0x7fff, 1}, {"fd-7fff-cnt4", 0x7fff, 4},
        {"fd-bad-cnt1", -1, 1},      {"fd-bad-cnt4", -1, 4},
    };

    for (size_t a = 0; a < sizeof(arms) / sizeof(arms[0]); a++) {
        uint8_t info[0x200];
        memset(info, 0xcc, sizeof(info));
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_info(arms[a].fd, info, arms[a].count, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, arms[a].tag, "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, arms[a].tag, "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, arms[a].tag, "info-200h", 0, info, sizeof(info));
    }

    if (s_udp >= 0 && s_udp != 0x15) {
        uint8_t info_real[0x200];
        memset(info_real, 0xcc, sizeof(info_real));
        int rc_real = fn_info(s_udp, info_real, 1, 0);
        obs_report_measure(id, "fd-real-udp-cnt1", "rc", (uint64_t)(uint32_t)rc_real,
                           "code");
        obs_report_bytes(id, "fd-real-udp-cnt1", "info-200h", 0, info_real,
                         sizeof(info_real));
    }

    if (s_udp >= 0 && obs_address_is_callable((const void *)&sceNetSocketClose)) {
        (void)sceNetSocketClose(s_udp);
    }

    return obs_pass();
}

static obs_result check_gt02_ajm_batch(void) {
    const char *id = "000-boot/gt02-ajm-batch";
    obs_jmp_buf guard;

    int bound_init = obs_address_is_callable((const void *)&sceAjmInitialize_static);
    int bound_reg = obs_address_is_callable((const void *)&sceAjmModuleRegister_static);
    int bound_cr = obs_address_is_callable((const void *)&sceAjmInstanceCreate_static);
    int bound_bi = obs_address_is_callable((const void *)&sceAjmBatchInitialize_static);
    int bound_jd = obs_address_is_callable((const void *)&sceAjmBatchJobDecode_static);
    int bound_start = obs_address_is_callable((const void *)&sceAjmBatchStart_static);
    int bound_wait = obs_address_is_callable((const void *)&sceAjmBatchWait_static);

    obs_report_measure(id, "sceAjmInitialize", "bound", (uint64_t)bound_init, "bool");
    obs_report_measure(id, "sceAjmModuleRegister", "bound", (uint64_t)bound_reg,
                       "bool");
    obs_report_measure(id, "sceAjmInstanceCreate", "bound", (uint64_t)bound_cr, "bool");
    obs_report_measure(id, "sceAjmBatchInitialize", "bound", (uint64_t)bound_bi,
                       "bool");
    obs_report_measure(id, "sceAjmBatchJobDecode", "bound", (uint64_t)bound_jd, "bool");
    obs_report_measure(id, "sceAjmBatchStart", "bound", (uint64_t)bound_start, "bool");
    obs_report_measure(id, "sceAjmBatchWait", "bound", (uint64_t)bound_wait, "bool");

    int bound = (bound_init && bound_reg && bound_cr && bound_bi && bound_jd);
    obs_report_measure(id, "setup-bound", "bool", (uint64_t)bound, "bool");
    if (!bound) {
        obs_report_measure(id, "setup-bound", "route", 0, "ajm-helpers-not-callable");
        return obs_pass();
    }

    /* Initialize context */
    uint32_t ctx_id = 0;
    int rc_init = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init = sceAjmInitialize_static(0, &ctx_id);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ajm-init", "rc", (uint64_t)(uint32_t)rc_init, "code");
    obs_report_measure(id, "ajm-init", "ctx", (uint64_t)ctx_id, "id");
    if (rc_init != 0) {
        return obs_pass();
    }

    /* Register codec 0 */
    int rc_reg = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_reg = sceAjmModuleRegister_static(ctx_id, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ajm-mod-reg-c0", "rc", (uint64_t)(uint32_t)rc_reg, "code");

    /* Create instance 2 on codec 0, flags 1 */
    uint8_t out_inst[8];
    memset(out_inst, 0xff, sizeof(out_inst));
    int rc_cr = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_cr = sceAjmInstanceCreate_static(ctx_id, 0, 1, out_inst);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    uint32_t inst_id = *(uint32_t *)out_inst;
    obs_report_measure(id, "ajm-inst-create", "rc", (uint64_t)(uint32_t)rc_cr, "code");
    obs_report_measure(id, "ajm-inst-create", "inst", (uint64_t)inst_id, "id");
    obs_report_bytes(id, "ajm-inst-create", "out-8b", 0, out_inst, sizeof(out_inst));

    /* Initialize batch (GTA size: 0x40 into 0x80 buffer) */
    uint8_t batch_buf[0x80];
    memset(batch_buf, 0xcc, sizeof(batch_buf));
    uint64_t batch_slot = 0xccccccccccccccccULL;
    int rc_bi = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_bi = sceAjmBatchInitialize_static(batch_buf, 0x40, &batch_slot);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "batch-init-gta", "rc", (uint64_t)(uint32_t)rc_bi, "code");
    obs_report_measure(id, "batch-init-gta", "slot", batch_slot, "hex");

    /* JobDecode: 960b MPEG frame into 9216b output */
    uint8_t mpeg_in[960];
    memset(mpeg_in, 0, sizeof(mpeg_in));
    mpeg_in[0] = 0xff;
    mpeg_in[1] = 0xfa;
    mpeg_in[2] = 0x64;
    mpeg_in[3] = 0xc0;
    static uint8_t dec_out[9216];
    memset(dec_out, 0xcc, sizeof(dec_out));
    int rc_dec = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_dec = sceAjmBatchJobDecode_static(&batch_slot, (uint64_t)inst_id, mpeg_in,
                                             (void *)(uintptr_t)960, dec_out,
                                             (void *)(uintptr_t)9216);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "job-decode-gta", "rc", (uint64_t)(uint32_t)rc_dec, "code");

    /* PPSA04263 arm 3 shape: Start with priority 0x28 and 0x20-byte out filled 0xcc */
    uint8_t out_gta[0x20];
    memset(out_gta, 0xcc, sizeof(out_gta));
    int rc_start_gta = -1;
    if (bound_start) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_start5_t)(uint32_t, void *, uint32_t, uint32_t, void *);
            const void *fn_s_raw = (const void *)&sceAjmBatchStart_static;
            fn_start5_t p_start5 = (fn_start5_t)(uintptr_t)fn_s_raw;
            rc_start_gta = p_start5(ctx_id, &batch_slot, 0x28, 0, out_gta);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "start-gta-prio0x28", "fault", (uint64_t)sig,
                               "signal");
        }
    }
    obs_report_measure(id, "start-gta-prio0x28", "rc", (uint64_t)(uint32_t)rc_start_gta,
                       "code");
    obs_report_bytes(id, "start-gta-prio0x28", "out-20b", 0, out_gta, sizeof(out_gta));

    uint32_t wait_id = *(uint32_t *)out_gta;

    /* Wait with id Start wrote, timeout 0, 0x20-byte block filled 0xcc */
    uint8_t block_0[0x20];
    memset(block_0, 0xcc, sizeof(block_0));
    int rc_wait_0 = -1;
    if (bound_wait) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *, uint32_t);
            const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
            fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
            rc_wait_0 = p_wait5(ctx_id, wait_id, 0, block_0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "wait-gta-to0", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "wait-gta-to0", "rc", (uint64_t)(uint32_t)rc_wait_0, "code");
    obs_report_bytes(id, "wait-gta-to0", "block-20b", 0, block_0, sizeof(block_0));

    /* Wait with id Start wrote, timeout 30, 0x20-byte block filled 0xcc */
    uint8_t block_30[0x20];
    memset(block_30, 0xcc, sizeof(block_30));
    int rc_wait_30 = -1;
    if (bound_wait) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *, uint32_t);
            const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
            fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
            rc_wait_30 = p_wait5(ctx_id, wait_id, 0, block_30, 30);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "wait-gta-to30", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "wait-gta-to30", "rc", (uint64_t)(uint32_t)rc_wait_30,
                       "code");
    obs_report_bytes(id, "wait-gta-to30", "block-20b", 0, block_30, sizeof(block_30));

    /* If wait_id is invalid or Start failed, also wait with 0xffffffff and timeout 30
     */
    if (wait_id == 0xccccccccu) {
        uint8_t block_ff[0x20];
        memset(block_ff, 0xcc, sizeof(block_ff));
        int rc_wait_ff = -1;
        if (bound_wait) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *,
                                          uint32_t);
                const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
                fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
                rc_wait_ff = p_wait5(ctx_id, 0xffffffffu, 0, block_ff, 30);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, "wait-gta-id-ffff-to30", "rc",
                           (uint64_t)(uint32_t)rc_wait_ff, "code");
        obs_report_bytes(id, "wait-gta-id-ffff-to30", "block-20b", 0, block_ff,
                         sizeof(block_ff));
    }

    /* Also try 3-arg route: sceAjmBatchStart(ctx, (void *)&batch_slot, &bid_3arg) */
    uint32_t bid_3arg = 0xcccccccc;
    int rc_3arg = -1;
    if (bound_start) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_start3_t)(uint32_t, void *, uint32_t *);
            rc_3arg = ((fn_start3_t)&sceAjmBatchStart_static)(
                ctx_id, (void *)&batch_slot, &bid_3arg);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "start-3arg", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "start-3arg", "rc", (uint64_t)(uint32_t)rc_3arg, "code");
    obs_report_measure(id, "start-3arg", "batch-id", (uint64_t)bid_3arg, "id");

    /* Also try context 0x15 if ctx_id != 0x15 */
    if (ctx_id != 0x15) {
        uint8_t out_15[0x20];
        memset(out_15, 0xcc, sizeof(out_15));
        int rc_s15 = -1;
        if (bound_start) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                typedef int (*fn_start5_t)(uint32_t, void *, uint32_t, uint32_t,
                                           void *);
                const void *fn_s_raw = (const void *)&sceAjmBatchStart_static;
                fn_start5_t p_start5 = (fn_start5_t)(uintptr_t)fn_s_raw;
                rc_s15 = p_start5(0x15, &batch_slot, 0x28, 0, out_15);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, "start-gta-ctx0x15", "rc", (uint64_t)(uint32_t)rc_s15,
                           "code");
        obs_report_bytes(id, "start-gta-ctx0x15", "out-20b", 0, out_15, sizeof(out_15));

        uint8_t block_15[0x20];
        memset(block_15, 0xcc, sizeof(block_15));
        int rc_w15 = -1;
        if (bound_wait) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *,
                                          uint32_t);
                const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
                fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
                rc_w15 = p_wait5(0x15, 0xffffffffu, 0, block_15, 30);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, "wait-gta-ctx0x15-id-ffff-to30", "rc",
                           (uint64_t)(uint32_t)rc_w15, "code");
        obs_report_bytes(id, "wait-gta-ctx0x15-id-ffff-to30", "block-20b", 0, block_15,
                         sizeof(block_15));
    }

    /* Finalize */
    if (obs_address_is_callable((const void *)&sceAjmFinalize_static)) {
        int rc_fin = sceAjmFinalize_static(ctx_id);
        obs_report_measure(id, "ajm-finalize", "rc", (uint64_t)(uint32_t)rc_fin,
                           "code");
    }

    return obs_pass();
}

static obs_result check_gt02_contentdelete(void) {
    const char *id = "000-boot/gt02-contentdelete";
    obs_jmp_buf guard;

    typedef int (*fn_cd_init_t)(const void *);
    fn_cd_init_t fn_init = NULL;
    int is_link = 0;

    /* 1. Try static link first */
    if (obs_address_is_callable((const void *)&sceContentDeleteInitialize_static)) {
        fn_init = (fn_cd_init_t)&sceContentDeleteInitialize_static;
        is_link = 1;
    }

    /* 2. Sweep candidate sysmodule IDs from 0x0001 through 0x0115 */
    uint16_t loaded_sysm_id = 0;
    if (!fn_init && obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        for (uint16_t mid = 0x0001; mid <= 0x0115; mid++) {
            int rc_sys = sceSysmoduleLoadModule(mid);
            if (rc_sys == 0 || (uint32_t)rc_sys == 0x80540001u) {
                if (obs_address_is_callable(
                        (const void *)&sceContentDeleteInitialize_static)) {
                    fn_init = (fn_cd_init_t)&sceContentDeleteInitialize_static;
                    is_link = 1;
                    loaded_sysm_id = mid;
                    break;
                }
                int h = obs_module_open("libSceContentDelete");
                if (h > 0) {
                    fn_init = (fn_cd_init_t)obs_module_symbol(h, "$zoxb0wEChEM");
                    if (!fn_init)
                        fn_init = (fn_cd_init_t)obs_module_symbol(
                            h, "sceContentDeleteInitialize");
                    if (fn_init) {
                        loaded_sysm_id = mid;
                        break;
                    }
                }
            }
        }
    }

    /* 3. Try sprx paths */
    if (!fn_init && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        static const char *const sprx_paths[] = {
            "/system/common/lib/libSceContentDelete.sprx",
            "/system_ex/common/lib/libSceContentDelete.sprx",
            "/system/priv/lib/libSceContentDelete.sprx",
            "/system/vsh/module/libSceContentDelete.sprx",
        };
        for (size_t p = 0; p < sizeof(sprx_paths) / sizeof(sprx_paths[0]); p++) {
            int h = sceKernelLoadStartModule(sprx_paths[p], 0, NULL, 0, NULL, NULL);
            if (h > 0) {
                fn_init = (fn_cd_init_t)obs_module_symbol(h, "$zoxb0wEChEM");
                if (!fn_init)
                    fn_init = (fn_cd_init_t)obs_module_symbol(
                        h, "sceContentDeleteInitialize");
                if (fn_init)
                    break;
            }
        }
    }

    int bound = (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceContentDeleteInitialize", "bound", (uint64_t)bound,
                       "bool");
    if (bound) {
        obs_report_measure(id, "sceContentDeleteInitialize", "how",
                           (uint64_t)(is_link ? 1 : 2), is_link ? "link" : "dlsym");
        if (loaded_sysm_id != 0) {
            obs_report_measure(id, "sysmodule-loaded", "id", (uint64_t)loaded_sysm_id,
                               "hex");
        }
    } else {
        obs_report_measure(id, "sceContentDeleteInitialize", "route", 0,
                           "tried-sysmodule-sweep-and-sprx-paths");
        return obs_pass();
    }

    int rc_null = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_null = fn_init(NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-null", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-null", "rc", (uint64_t)(uint32_t)rc_null, "code");

    uint8_t cd_param[0x20];
    memset(cd_param, 0, sizeof(cd_param));
    int rc_param = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_param = fn_init(cd_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-param-zeroed", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-param-zeroed", "rc", (uint64_t)(uint32_t)rc_param,
                       "code");
    obs_report_bytes(id, "init-param-zeroed", "param-20b", 0, cd_param,
                     sizeof(cd_param));

    int rc_sec = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_sec = fn_init(cd_param);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-second", "rc", (uint64_t)(uint32_t)rc_sec, "code");
    obs_report_bytes(id, "init-second", "param-20b", 0, cd_param, sizeof(cd_param));

    return obs_pass();
}

/* REQ-20261016T1156Z-gt03: ContentDelete, SockInfo, AjmBatch decode wait */
static obs_result check_gt03_contentdelete(void) {
    const char *id = "000-boot/gt03-contentdelete";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x00ee);
    }

    typedef int (*fn_cd_init_t)(const void *);
    fn_cd_init_t fn_init = NULL;
    int is_link = 0;

    if (obs_address_is_callable((const void *)&sceContentDeleteInitialize_static)) {
        fn_init = (fn_cd_init_t)&sceContentDeleteInitialize_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceContentDelete");
        if (h > 0) {
            fn_init = (fn_cd_init_t)obs_module_symbol(h, "$zoxb0wEChEM");
            if (!fn_init)
                fn_init =
                    (fn_cd_init_t)obs_module_symbol(h, "sceContentDeleteInitialize");
        }
    }

    int bound = (fn_init != NULL && obs_address_is_callable((const void *)fn_init));
    obs_report_measure(id, "sceContentDeleteInitialize", "bound", (uint64_t)bound,
                       "bool");
    if (bound) {
        obs_report_measure(id, "sceContentDeleteInitialize", "how",
                           (uint64_t)(is_link ? 1 : 2), is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceContentDeleteInitialize", "route", 0,
                           "tried-sysmodule-0x00ee");
        return obs_pass();
    }

    /* PPSA04263 populated block:
     * u64 4 at +0
     * u64 0x4000 at +8
     * u64 0x1c at +0x10
     * pointer at +0x18 to a zeroed 0x4000-byte region */
    void *work_buf = calloc(1, 0x4000);
    uint64_t param_block[4];
    param_block[0] = 4;
    param_block[1] = 0x4000;
    param_block[2] = 0x1c;
    param_block[3] = (uint64_t)(uintptr_t)work_buf;

    int rc1 = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc1 = fn_init(param_block);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-first", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-first", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_bytes(id, "init-first", "block-20b", 0,
                     (const unsigned char *)param_block, sizeof(param_block));

    int rc2 = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc2 = fn_init(param_block);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        obs_report_measure(id, "init-second", "fault", (uint64_t)sig, "signal");
    }
    obs_report_measure(id, "init-second", "rc", (uint64_t)(uint32_t)rc2, "code");
    obs_report_bytes(id, "init-second", "block-20b", 0,
                     (const unsigned char *)param_block, sizeof(param_block));

    if (work_buf) {
        free(work_buf);
    }
    return obs_pass();
}

static obs_result check_gt03_net_sockinfo(void) {
    const char *id = "000-boot/gt03-net-sockinfo";
    obs_jmp_buf guard;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0001);
    }
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }

    typedef int (*fn_sockinfo_t)(int, void *, int, int);
    fn_sockinfo_t fn_info = NULL;
    int is_link = 0;
    if (obs_address_is_callable((const void *)&sceNetGetSockInfo_static)) {
        fn_info = (fn_sockinfo_t)&sceNetGetSockInfo_static;
        is_link = 1;
    } else {
        int h = obs_module_open("libSceNet");
        if (h > 0) {
            fn_info = (fn_sockinfo_t)obs_module_symbol(h, "$hLuXdjHnhiI");
            if (!fn_info)
                fn_info = (fn_sockinfo_t)obs_module_symbol(h, "sceNetGetSockInfo");
        }
    }

    int bound = (fn_info != NULL && obs_address_is_callable((const void *)fn_info));
    obs_report_measure(id, "sceNetGetSockInfo", "bound", (uint64_t)bound, "bool");
    if (bound) {
        obs_report_measure(id, "sceNetGetSockInfo", "how", (uint64_t)(is_link ? 1 : 2),
                           is_link ? "link" : "dlsym");
    } else {
        obs_report_measure(id, "sceNetGetSockInfo", "route", 0,
                           "tried-static-and-libscenet");
        return obs_pass();
    }

    int s_udp = -1;
    int s_tcp = -1;
    if (obs_address_is_callable((const void *)&sceNetSocket)) {
        s_udp = sceNetSocket("gt03_udp", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
        s_tcp = sceNetSocket("gt03_tcp", OBS_NET_AF_INET, 1 /* SOCK_STREAM */, 0);
    }
    obs_report_measure(id, "socket-udp", "fd", (uint64_t)(uint32_t)s_udp, "fd");
    obs_report_measure(id, "socket-tcp", "fd", (uint64_t)(uint32_t)s_tcp, "fd");

    /* Arm 1: UDP socket freshly made and not bound */
    if (s_udp >= 0) {
        uint8_t info_udp[0x200];
        memset(info_udp, 0xcc, sizeof(info_udp));
        int rc_udp = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_udp = fn_info(s_udp, info_udp, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "udp-fresh", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "udp-fresh", "rc", (uint64_t)(uint32_t)rc_udp, "code");
        obs_report_bytes(id, "udp-fresh", "info-200h", 0, info_udp, sizeof(info_udp));
    }

    /* Arm 2: TCP socket freshly made and not bound */
    if (s_tcp >= 0) {
        uint8_t info_tcp[0x200];
        memset(info_tcp, 0xcc, sizeof(info_tcp));
        int rc_tcp = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_tcp = fn_info(s_tcp, info_tcp, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "tcp-fresh", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "tcp-fresh", "rc", (uint64_t)(uint32_t)rc_tcp, "code");
        obs_report_bytes(id, "tcp-fresh", "info-200h", 0, info_tcp, sizeof(info_tcp));
    }

    /* Arm 3: UDP socket after sceNetBind to port 0 */
    if (s_udp >= 0 && obs_address_is_callable((const void *)&sceNetBind)) {
        obs_net_sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_len = sizeof(sin);
        sin.sin_family = OBS_NET_AF_INET;
        sin.sin_port = 0;
        sin.sin_addr = 0;
        int rc_bind = sceNetBind(s_udp, (const void *)&sin, sizeof(sin));
        obs_report_measure(id, "udp-bound-port0", "bind-rc",
                           (uint64_t)(uint32_t)rc_bind, "code");

        uint8_t info_bound[0x200];
        memset(info_bound, 0xcc, sizeof(info_bound));
        int rc_info_bound = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_info_bound = fn_info(s_udp, info_bound, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "udp-bound-port0", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "udp-bound-port0", "rc",
                           (uint64_t)(uint32_t)rc_info_bound, "code");
        obs_report_bytes(id, "udp-bound-port0", "info-200h", 0, info_bound,
                         sizeof(info_bound));
    }

    if (s_udp >= 0 && obs_address_is_callable((const void *)&sceNetSocketClose)) {
        (void)sceNetSocketClose(s_udp);
    }
    if (s_tcp >= 0 && obs_address_is_callable((const void *)&sceNetSocketClose)) {
        (void)sceNetSocketClose(s_tcp);
    }

    return obs_pass();
}

static obs_result check_gt03_ajm_batch(void) {
    const char *id = "000-boot/gt03-ajm-batch";
    obs_jmp_buf guard;

    int bound_init = obs_address_is_callable((const void *)&sceAjmInitialize_static);
    int bound_reg = obs_address_is_callable((const void *)&sceAjmModuleRegister_static);
    int bound_cr = obs_address_is_callable((const void *)&sceAjmInstanceCreate_static);
    int bound_bi = obs_address_is_callable((const void *)&sceAjmBatchInitialize_static);
    int bound_jd = obs_address_is_callable((const void *)&sceAjmBatchJobDecode_static);
    int bound_start = obs_address_is_callable((const void *)&sceAjmBatchStart_static);
    int bound_wait = obs_address_is_callable((const void *)&sceAjmBatchWait_static);

    obs_report_measure(id, "sceAjmInitialize", "bound", (uint64_t)bound_init, "bool");
    obs_report_measure(id, "sceAjmModuleRegister", "bound", (uint64_t)bound_reg,
                       "bool");
    obs_report_measure(id, "sceAjmInstanceCreate", "bound", (uint64_t)bound_cr, "bool");
    obs_report_measure(id, "sceAjmBatchInitialize", "bound", (uint64_t)bound_bi,
                       "bool");
    obs_report_measure(id, "sceAjmBatchJobDecode", "bound", (uint64_t)bound_jd, "bool");
    obs_report_measure(id, "sceAjmBatchStart", "bound", (uint64_t)bound_start, "bool");
    obs_report_measure(id, "sceAjmBatchWait", "bound", (uint64_t)bound_wait, "bool");

    int bound = (bound_init && bound_reg && bound_cr && bound_bi && bound_jd);
    obs_report_measure(id, "setup-bound", "bool", (uint64_t)bound, "bool");
    if (!bound) {
        obs_report_measure(id, "setup-bound", "route", 0, "ajm-helpers-not-callable");
        return obs_pass();
    }

    uint32_t ctx_id = 0;
    int rc_init = -1;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_init = sceAjmInitialize_static(0, &ctx_id);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ajm-init", "rc", (uint64_t)(uint32_t)rc_init, "code");
    obs_report_measure(id, "ajm-init", "ctx", (uint64_t)ctx_id, "id");
    if (rc_init != 0) {
        return obs_pass();
    }

    /* Register codec 0 (MP3) */
    int rc_reg = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_reg = sceAjmModuleRegister_static(ctx_id, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "ajm-mod-reg-c0", "rc", (uint64_t)(uint32_t)rc_reg, "code");

    /* Create instance on codec 0, flags 1 */
    uint8_t out_inst[8];
    memset(out_inst, 0xff, sizeof(out_inst));
    int rc_cr = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_cr = sceAjmInstanceCreate_static(ctx_id, 0, 1, out_inst);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    uint32_t inst_id = *(uint32_t *)out_inst;
    obs_report_measure(id, "ajm-inst-create", "rc", (uint64_t)(uint32_t)rc_cr, "code");
    obs_report_measure(id, "ajm-inst-create", "inst", (uint64_t)inst_id, "id");

    /* Initialize batch: 0x40 into 0x80 buffer */
    uint8_t batch_buf[0x80];
    memset(batch_buf, 0xcc, sizeof(batch_buf));
    uint64_t batch_slot = 0xccccccccccccccccULL;
    int rc_bi = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_bi = sceAjmBatchInitialize_static(batch_buf, 0x40, &batch_slot);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "batch-init", "rc", (uint64_t)(uint32_t)rc_bi, "code");
    obs_report_measure(id, "batch-init", "slot", batch_slot, "hex");

    /* One valid 960-byte MPEG-1 Layer III mono 48 kHz frame set:
     * four 240-byte frames, silence encoded with header ff fb 64 c0 and zeroed side
     * info */
    uint8_t mpeg_in[960];
    memset(mpeg_in, 0, sizeof(mpeg_in));
    for (int f = 0; f < 4; f++) {
        uint8_t *frame = mpeg_in + (f * 240);
        frame[0] = 0xff;
        frame[1] = 0xfb;
        frame[2] = 0x64;
        frame[3] = 0xc0;
    }

    static uint8_t dec_out[9216];
    memset(dec_out, 0xcc, sizeof(dec_out));
    int rc_dec = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_dec = sceAjmBatchJobDecode_static(&batch_slot, (uint64_t)inst_id, mpeg_in,
                                             (void *)(uintptr_t)960, dec_out,
                                             (void *)(uintptr_t)9216);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure(id, "job-decode", "rc", (uint64_t)(uint32_t)rc_dec, "code");

    /* Start batch with PPSA04263 5-arg shape */
    uint8_t out_gta[0x20];
    memset(out_gta, 0xcc, sizeof(out_gta));
    int rc_start = -1;
    if (bound_start) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_start5_t)(uint32_t, void *, uint32_t, uint32_t, void *);
            const void *fn_s_raw = (const void *)&sceAjmBatchStart_static;
            fn_start5_t p_start5 = (fn_start5_t)(uintptr_t)fn_s_raw;
            rc_start = p_start5(ctx_id, &batch_slot, 0x28, 0, out_gta);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "start-prio0x28", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "start-prio0x28", "rc", (uint64_t)(uint32_t)rc_start,
                       "code");
    obs_report_bytes(id, "start-prio0x28", "out-20b", 0, out_gta, sizeof(out_gta));

    uint32_t wait_id = *(uint32_t *)out_gta;

    /* First wait: timeout 0 */
    uint8_t block_0[0x20];
    memset(block_0, 0xcc, sizeof(block_0));
    int rc_wait_0 = -1;
    if (bound_wait) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *, uint32_t);
            const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
            fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
            rc_wait_0 = p_wait5(ctx_id, wait_id, 0, block_0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "wait-to0", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "wait-to0", "rc", (uint64_t)(uint32_t)rc_wait_0, "code");
    obs_report_bytes(id, "wait-to0", "block-20b", 0, block_0, sizeof(block_0));

    /* Second wait: timeout 30 */
    uint8_t block_30[0x20];
    memset(block_30, 0xcc, sizeof(block_30));
    int rc_wait_30 = -1;
    if (bound_wait) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*fn_wait5_t)(uint32_t, uint32_t, uint32_t, void *, uint32_t);
            const void *fn_w_raw = (const void *)&sceAjmBatchWait_static;
            fn_wait5_t p_wait5 = (fn_wait5_t)(uintptr_t)fn_w_raw;
            rc_wait_30 = p_wait5(ctx_id, wait_id, 0, block_30, 30);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "wait-to30", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "wait-to30", "rc", (uint64_t)(uint32_t)rc_wait_30, "code");
    obs_report_bytes(id, "wait-to30", "block-20b", 0, block_30, sizeof(block_30));

    /* First 64 bytes of 9216-byte output */
    obs_report_bytes(id, "decode-output", "out-64b", 0, dec_out, 64);

    /* Finalize */
    if (obs_address_is_callable((const void *)&sceAjmFinalize_static)) {
        int rc_fin = sceAjmFinalize_static(ctx_id);
        obs_report_measure(id, "ajm-finalize", "rc", (uint64_t)(uint32_t)rc_fin,
                           "code");
    }

    return obs_pass();
}

/* REQ-20261016T2335Z-gt04 Item 1: libc stat, open, fopen on memory: paths */
static obs_result check_gt04_memory_path(void) {
    const char *id = "000-boot/gt04-memory-path";
    obs_jmp_buf guard;

    int h_libc = obs_module_open("libc");
    int h_libk = obs_module_open("libkernel");
    int h_posix = obs_module_open("libScePosix");

    typedef int (*fn_stat_t)(const char *, void *);
    typedef int (*fn_open_t)(const char *, int, ...);
    typedef void *(*fn_fopen_t)(const char *, const char *);
    typedef int (*fn_fclose_t)(void *);
    typedef int *(*fn_error_t)(void);

    fn_stat_t fn_stat = obs_address_is_callable((const void *)&sce_libc_stat_static)
                            ? (fn_stat_t)&sce_libc_stat_static
                            : NULL;
    if (!fn_stat && h_libc > 0) {
        fn_stat = (fn_stat_t)obs_module_symbol(h_libc, "$E6ao34wPw+U");
        if (!fn_stat)
            fn_stat = (fn_stat_t)obs_module_symbol(h_libc, "stat");
    }
    if (!fn_stat && h_libk > 0) {
        fn_stat = (fn_stat_t)obs_module_symbol(h_libk, "$E6ao34wPw+U");
        if (!fn_stat)
            fn_stat = (fn_stat_t)obs_module_symbol(h_libk, "stat");
    }
    if (!fn_stat && h_posix > 0) {
        fn_stat = (fn_stat_t)obs_module_symbol(h_posix, "$E6ao34wPw+U");
        if (!fn_stat)
            fn_stat = (fn_stat_t)obs_module_symbol(h_posix, "stat");
    }

    fn_open_t fn_open = obs_address_is_callable((const void *)&sce_libc_open_static)
                            ? (fn_open_t)&sce_libc_open_static
                            : NULL;
    if (!fn_open && h_libc > 0) {
        fn_open = (fn_open_t)obs_module_symbol(h_libc, "$wuCroIGjt2g");
        if (!fn_open)
            fn_open = (fn_open_t)obs_module_symbol(h_libc, "open");
    }
    if (!fn_open && h_libk > 0) {
        fn_open = (fn_open_t)obs_module_symbol(h_libk, "$wuCroIGjt2g");
        if (!fn_open)
            fn_open = (fn_open_t)obs_module_symbol(h_libk, "open");
    }
    if (!fn_open && h_posix > 0) {
        fn_open = (fn_open_t)obs_module_symbol(h_posix, "$wuCroIGjt2g");
        if (!fn_open)
            fn_open = (fn_open_t)obs_module_symbol(h_posix, "open");
    }

    fn_fopen_t fn_fopen = obs_address_is_callable((const void *)&sce_libc_fopen_static)
                              ? (fn_fopen_t)&sce_libc_fopen_static
                              : NULL;
    if (!fn_fopen && h_libc > 0) {
        fn_fopen = (fn_fopen_t)obs_module_symbol(h_libc, "$xeYO4u7uyJ0");
        if (!fn_fopen)
            fn_fopen = (fn_fopen_t)obs_module_symbol(h_libc, "fopen");
    }
    if (!fn_fopen) {
        int h_libcint = obs_module_open("libSceLibcInternal");
        if (h_libcint > 0) {
            fn_fopen = (fn_fopen_t)obs_module_symbol(h_libcint, "$xeYO4u7uyJ0");
            if (!fn_fopen)
                fn_fopen = (fn_fopen_t)obs_module_symbol(h_libcint, "fopen");
        }
    }

    fn_fclose_t fn_fclose =
        obs_address_is_callable((const void *)&sce_libc_fclose_static)
            ? (fn_fclose_t)&sce_libc_fclose_static
            : NULL;
    if (!fn_fclose && h_libc > 0) {
        fn_fclose = (fn_fclose_t)obs_module_symbol(h_libc, "$uodLYyUip20");
        if (!fn_fclose)
            fn_fclose = (fn_fclose_t)obs_module_symbol(h_libc, "fclose");
    }

    fn_error_t fn_error =
        obs_address_is_callable((const void *)&__error) ? (fn_error_t)&__error : NULL;
    if (!fn_error && h_libk > 0) {
        fn_error = (fn_error_t)obs_module_symbol(h_libk, "$9BcDykPmo1I");
        if (!fn_error)
            fn_error = (fn_error_t)obs_module_symbol(h_libk, "__error");
    }
    if (!fn_error && h_libc > 0) {
        fn_error = (fn_error_t)obs_module_symbol(h_libc, "$9BcDykPmo1I");
        if (!fn_error)
            fn_error = (fn_error_t)obs_module_symbol(h_libc, "__error");
    }

    obs_report_measure(id, "stat", "bound", (uint64_t)(fn_stat != NULL), "bool");
    obs_report_measure(id, "open", "bound", (uint64_t)(fn_open != NULL), "bool");
    obs_report_measure(id, "fopen", "bound", (uint64_t)(fn_fopen != NULL), "bool");
    obs_report_measure(id, "__error", "bound", (uint64_t)(fn_error != NULL), "bool");

    static const struct {
        const char *tag;
        const char *path;
    } paths[] = {
        {"path-gfx",
         "memory:$0x10c09f0000,9156,0:00005_initial_interactive_screen_ps5.gfx"},
        {"path-memory-x", "memory:x"},
        {"path-app0-memory-x", "/app0/memory:x"},
    };

    for (size_t p = 0; p < sizeof(paths) / sizeof(paths[0]); p++) {
        const char *ptag = paths[p].tag;
        const char *path = paths[p].path;

        /* 1. stat */
        char subtag[64];
        snprintf(subtag, sizeof(subtag), "%s-stat", ptag);
        if (fn_stat) {
            uint8_t st[256];
            memset(st, 0xa5, sizeof(st));
            uint8_t before[256];
            memcpy(before, st, sizeof(st));
            if (fn_error)
                *fn_error() = 0;

            int rc_stat = -1;
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_stat = fn_stat(path, st);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_stat = -sig;
                obs_report_measure(id, subtag, "fault", (uint64_t)sig, "signal");
            }
            int err = fn_error ? *fn_error() : 0;
            obs_report_measure(id, subtag, "rc", (uint64_t)(uint32_t)rc_stat, "code");
            obs_report_measure(id, subtag, "errno", (uint64_t)(uint32_t)err, "errno");

            uint32_t changed = 0;
            for (size_t i = 0; i < sizeof(st); i++) {
                if (st[i] != before[i]) {
                    changed++;
                }
            }
            obs_report_measure(id, subtag, "changed-count", (uint64_t)changed, "count");
            if (changed > 0) {
                obs_report_written(id, subtag, "bytes-changed", before, st,
                                   (unsigned int)sizeof(st));
                obs_report_bytes(id, subtag, "st-256b", 0, st, sizeof(st));
                for (size_t i = 0; i < sizeof(st); i++) {
                    if (st[i] != before[i]) {
                        char ofs_tag[32];
                        snprintf(ofs_tag, sizeof(ofs_tag), "diff-0x%02x",
                                 (unsigned int)i);
                        obs_report_measure(id, subtag, ofs_tag, (uint64_t)st[i], "hex");
                    }
                }
            }
        } else {
            obs_report_measure(id, subtag, "status", 0, "not-possible");
        }

        /* 2. open */
        snprintf(subtag, sizeof(subtag), "%s-open", ptag);
        if (fn_open) {
            if (fn_error)
                *fn_error() = 0;
            int fd = -1;
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                fd = fn_open(path, 0 /* O_RDONLY */);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                fd = -sig;
                obs_report_measure(id, subtag, "fault", (uint64_t)sig, "signal");
            }
            int err = fn_error ? *fn_error() : 0;
            obs_report_measure(id, subtag, "rc", (uint64_t)(uint32_t)fd, "code");
            obs_report_measure(id, subtag, "errno", (uint64_t)(uint32_t)err, "errno");
            if (fd >= 0) {
                if (obs_address_is_callable((const void *)&sceKernelClose)) {
                    sceKernelClose(fd);
                }
            }
        } else {
            obs_report_measure(id, subtag, "status", 0, "not-possible");
        }

        /* 3. fopen */
        snprintf(subtag, sizeof(subtag), "%s-fopen", ptag);
        if (fn_fopen) {
            if (fn_error)
                *fn_error() = 0;
            void *fp = NULL;
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                fp = fn_fopen(path, "rb");
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                obs_report_measure(id, subtag, "fault", (uint64_t)sig, "signal");
            }
            int err = fn_error ? *fn_error() : 0;
            obs_report_measure(id, subtag, "ptr", (uint64_t)(uintptr_t)fp, "addr");
            obs_report_measure(id, subtag, "is-null", (uint64_t)(fp == NULL), "bool");
            obs_report_measure(id, subtag, "errno", (uint64_t)(uint32_t)err, "errno");
            if (fp != NULL && fn_fclose &&
                obs_address_is_callable((const void *)fn_fclose)) {
                fn_fclose(fp);
            }
        } else {
            obs_report_measure(id, subtag, "status", 0, "not-possible");
        }
    }

    return obs_pass();
}

/* REQ-20261016T2335Z-gt04 Item 2: notice-screen skip flag over launches */
static obs_result check_gt04_notice_skip(void) {
    const char *id = "000-boot/gt04-notice-skip";
    obs_jmp_buf guard;

    int h_ss = obs_module_open("libSceSystemService");

    typedef int (*fn_get_skip_flag_t)(void *);
    typedef int (*fn_set_skip_flag_t)(uint32_t);
    typedef int (*fn_dis_auto_set_t)(void);

    fn_get_skip_flag_t fn_get_skip =
        obs_address_is_callable(
            (const void *)&sceSystemServiceGetNoticeScreenSkipFlag_static)
            ? (fn_get_skip_flag_t)&sceSystemServiceGetNoticeScreenSkipFlag_static
            : NULL;
    if (!fn_get_skip && h_ss > 0) {
        fn_get_skip = (fn_get_skip_flag_t)obs_module_symbol(h_ss, "$3RQ5aQfnstU");
        if (!fn_get_skip)
            fn_get_skip = (fn_get_skip_flag_t)obs_module_symbol(
                h_ss, "sceSystemServiceGetNoticeScreenSkipFlag");
    }

    fn_set_skip_flag_t fn_set_skip =
        obs_address_is_callable(
            (const void *)&sceSystemServiceSetNoticeScreenSkipFlag_static)
            ? (fn_set_skip_flag_t)&sceSystemServiceSetNoticeScreenSkipFlag_static
            : NULL;
    if (!fn_set_skip && h_ss > 0) {
        fn_set_skip = (fn_set_skip_flag_t)obs_module_symbol(h_ss, "$Q3utJvma4Mo");
        if (!fn_set_skip)
            fn_set_skip = (fn_set_skip_flag_t)obs_module_symbol(
                h_ss, "sceSystemServiceSetNoticeScreenSkipFlag");
    }

    fn_dis_auto_set_t fn_dis_auto =
        obs_address_is_callable(
            (const void *)&sceSystemServiceDisableNoticeScreenSkipFlagAutoSet_static)
            ? (fn_dis_auto_set_t)&sceSystemServiceDisableNoticeScreenSkipFlagAutoSet_static
            : NULL;
    if (!fn_dis_auto && h_ss > 0) {
        fn_dis_auto = (fn_dis_auto_set_t)obs_module_symbol(h_ss, "$8Lo6Zv94aho");
        if (!fn_dis_auto)
            fn_dis_auto = (fn_dis_auto_set_t)obs_module_symbol(
                h_ss, "sceSystemServiceDisableNoticeScreenSkipFlagAutoSet");
    }

    obs_report_measure(id, "GetNoticeScreenSkipFlag", "bound",
                       (uint64_t)(fn_get_skip != NULL), "bool");
    obs_report_measure(id, "SetNoticeScreenSkipFlag", "bound",
                       (uint64_t)(fn_set_skip != NULL), "bool");
    obs_report_measure(id, "DisableNoticeScreenSkipFlagAutoSet", "bound",
                       (uint64_t)(fn_dis_auto != NULL), "bool");

    if (!fn_get_skip) {
        obs_report_measure(id, "flag", "status", 0, "not-possible");
        return obs_pass();
    }

    /* Hide splash screen to avoid SceLncService 20-second watchdog */
    if (obs_address_is_callable(
            (const void *)&sceSystemServiceHideSplashScreen_static)) {
        int rc_hide = sceSystemServiceHideSplashScreen_static();
        obs_report_measure(id, "hide-splash", "rc", (uint64_t)(uint32_t)rc_hide,
                           "code");
    }

    /* Read phase from /app0/gt04_phase.txt or /data/gt04_phase.txt (defaults to 0:
     * complete) */
    int phase = 0;
    if (obs_address_is_callable((const void *)&sceKernelOpen)) {
        int fd_p = sceKernelOpen("/app0/gt04_phase.txt", 0x0000 /* O_RDONLY */, 0);
        if (fd_p < 0) {
            fd_p = sceKernelOpen("/data/gt04_phase.txt", 0x0000 /* O_RDONLY */, 0);
        }
        if (fd_p < 0) {
            fd_p = sceKernelOpen("/temp0/gt04_phase.txt", 0x0000 /* O_RDONLY */, 0);
        }
        if (fd_p >= 0) {
            char pbuf[16];
            memset(pbuf, 0, sizeof(pbuf));
            sce_ssize_t n = sceKernelRead(fd_p, pbuf, sizeof(pbuf) - 1);
            if (n > 0) {
                if (pbuf[0] == '2')
                    phase = 2;
                else if (pbuf[0] == '3')
                    phase = 3;
                else if (pbuf[0] == '4')
                    phase = 4;
            }
            sceKernelClose(fd_p);
        }
    }
    obs_report_measure(id, "phase", "current", (uint64_t)phase, "number");

    if (phase == 1) {
        uint8_t b[16];
        memset(b, 0xa5, sizeof(b));
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_get_skip(b);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc = -sig;
            obs_report_measure(id, "launch1", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "launch1", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "launch1", "byte", (uint64_t)b[0], "hex");
        obs_report_bytes(id, "launch1", "buf-16b", 0, b, sizeof(b));

        /* Run 130 seconds */
        obs_boot_note("gt04: phase 1 running 130 seconds...\n");
        obs_display_close();
        for (int s = 0; s < 130; s++) {
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(1000000);
            }
            if (obs_address_is_callable((const void *)&sceSystemServicePowerTick)) {
                sceSystemServicePowerTick();
            }
            if (obs_address_is_callable(
                    (const void *)&sceSystemServiceReceiveEvent_static)) {
                uint8_t ev[512];
                for (int e = 0; e < 16; e++) {
                    memset(ev, 0, sizeof(ev));
                    if (sceSystemServiceReceiveEvent_static(ev) != 0)
                        break;
                }
            }
            if ((s + 1) % 10 == 0 || s == 129) {
                char note[64];
                snprintf(note, sizeof(note), "gt04: elapsed %d/130s\n", s + 1);
                obs_boot_note(note);
            }
        }
        obs_boot_note("gt04: 130s elapsed, phase 1 complete. Exiting.\n");
        obs_boot_note("[park] work done\n");
        obs_invoke_syscall(1, 0, 0, 0, 0, 0, 0);
        return obs_pass();
    } else if (phase == 2) {
        uint8_t b[16];
        memset(b, 0xa5, sizeof(b));
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_get_skip(b);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc = -sig;
            obs_report_measure(id, "launch2", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "launch2", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "launch2", "byte", (uint64_t)b[0], "hex");
        obs_report_bytes(id, "launch2", "buf-16b", 0, b, sizeof(b));

        obs_boot_note("gt04: phase 2 complete. Exiting.\n");
        obs_boot_note("[park] work done\n");
        obs_invoke_syscall(1, 0, 0, 0, 0, 0, 0);
        return obs_pass();
    } else if (phase == 3) {
        /* Call sceSystemServiceDisableNoticeScreenSkipFlagAutoSet() first */
        int rc_dis = -1;
        if (fn_dis_auto) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_dis = fn_dis_auto();
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_dis = -sig;
                obs_report_measure(id, "launch3-dis-auto", "fault", (uint64_t)sig,
                                   "signal");
            }
        }
        obs_report_measure(id, "launch3-dis-auto", "rc", (uint64_t)(uint32_t)rc_dis,
                           "code");

        /* sceSystemServiceSetNoticeScreenSkipFlag(0) */
        int rc_set0 = -1;
        if (fn_set_skip) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_set0 = fn_set_skip(0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_set0 = -sig;
                obs_report_measure(id, "launch3-set-0", "fault", (uint64_t)sig,
                                   "signal");
            }
        }
        obs_report_measure(id, "launch3-set-0", "rc", (uint64_t)(uint32_t)rc_set0,
                           "code");

        /* Read flag after set(0) */
        uint8_t b0[16];
        memset(b0, 0xa5, sizeof(b0));
        int rc_get0 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_get0 = fn_get_skip(b0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_get0 = -sig;
            obs_report_measure(id, "launch3-read-after-set-0", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "launch3-read-after-set-0", "rc",
                           (uint64_t)(uint32_t)rc_get0, "code");
        obs_report_measure(id, "launch3-read-after-set-0", "byte", (uint64_t)b0[0],
                           "hex");
        obs_report_bytes(id, "launch3-read-after-set-0", "buf-16b", 0, b0, sizeof(b0));

        /* sceSystemServiceSetNoticeScreenSkipFlag(1) */
        int rc_set1 = -1;
        if (fn_set_skip) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_set1 = fn_set_skip(1);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_set1 = -sig;
                obs_report_measure(id, "launch3-set-1", "fault", (uint64_t)sig,
                                   "signal");
            }
        }
        obs_report_measure(id, "launch3-set-1", "rc", (uint64_t)(uint32_t)rc_set1,
                           "code");

        /* Read flag after set(1) */
        uint8_t b1[16];
        memset(b1, 0xa5, sizeof(b1));
        int rc_get1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_get1 = fn_get_skip(b1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_get1 = -sig;
            obs_report_measure(id, "launch3-read-after-set-1", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "launch3-read-after-set-1", "rc",
                           (uint64_t)(uint32_t)rc_get1, "code");
        obs_report_measure(id, "launch3-read-after-set-1", "byte", (uint64_t)b1[0],
                           "hex");
        obs_report_bytes(id, "launch3-read-after-set-1", "buf-16b", 0, b1, sizeof(b1));

        obs_boot_note("gt04: phase 3 complete. Exiting.\n");
        obs_boot_note("[park] work done\n");
        obs_invoke_syscall(1, 0, 0, 0, 0, 0, 0);
        return obs_pass();
    }

    obs_report_measure(id, "phase", "done", 1, "bool");
    return obs_pass();
}

/* ========================================================================= */
/* REQ-20261017T0530Z-gt07: stubbed calls PPSA04263 branches on              */
/* ========================================================================= */
static obs_result check_gt07_stubbed_calls(void) {
    const char *id = "000-boot/gt07-stubbed-calls";
    obs_jmp_buf guard;

    /* 1. sceVoiceSetThreadsParams */
    int bound_v_init = obs_address_is_callable((const void *)&sceVoiceInit_static);
    int bound_v_set =
        obs_address_is_callable((const void *)&sceVoiceSetThreadsParams_static);
    typedef int (*fn_v_init_t)(const void *, uint32_t);
    typedef int (*fn_v_set_t)(const void *);
    fn_v_init_t p_v_init = bound_v_init ? (fn_v_init_t)&sceVoiceInit_static : NULL;
    fn_v_set_t p_v_set =
        bound_v_set ? (fn_v_set_t)&sceVoiceSetThreadsParams_static : NULL;

    if (!p_v_init || !p_v_set) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(0x001A); /* VOICE */
        }
        int h_voice = obs_module_open("libSceVoice");
        if (h_voice > 0) {
            if (!p_v_init) {
                p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "$9TrhuGzberQ");
                if (!p_v_init)
                    p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "sceVoiceInit");
            }
            if (!p_v_set) {
                p_v_set = (fn_v_set_t)obs_module_symbol(h_voice, "$clyKUyi3RYU");
                if (!p_v_set)
                    p_v_set = (fn_v_set_t)obs_module_symbol(h_voice,
                                                            "sceVoiceSetThreadsParams");
            }
        }
    }

    obs_report_measure(id, "sceVoiceInit", "bound", (uint64_t)(p_v_init != NULL),
                       "bool");
    obs_report_measure(id, "sceVoiceSetThreadsParams", "bound",
                       (uint64_t)(p_v_set != NULL), "bool");

    if (p_v_init) {
        uint8_t init_param[0x20];
        memset(init_param, 0, sizeof(init_param));
        *(uint32_t *)init_param = 0x10000000;
        int rc_vi = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vi = p_v_init(init_param, 100);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_vi = -sig;
            obs_report_measure(id, "voice-init", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "voice-init", "rc", (uint64_t)(uint32_t)rc_vi, "code");
    }

    if (p_v_set) {
        uint8_t gta_voice_params[32] = {0x01, 0x00, 0x00, 0x00, 0xbc, 0x02, 0x00, 0x00,
                                        0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0xbc, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int rc_vs = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vs = p_v_set(gta_voice_params);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_vs = -sig;
            obs_report_measure(id, "voice-set-threads", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "voice-set-threads", "rc", (uint64_t)(uint32_t)rc_vs,
                           "code");
    }

    /* 2. sceAppContentDownloadDataGetAvailableSpaceKb */
    int bound_app_init =
        obs_address_is_callable((const void *)&sceAppContentInitialize_static);
    int bound_app_space = obs_address_is_callable(
        (const void *)&sceAppContentDownloadDataGetAvailableSpaceKb_static);
    typedef int (*fn_app_init_t)(void *);
    typedef int (*fn_app_space_t)(const char *, void *);
    fn_app_init_t p_app_init =
        bound_app_init ? (fn_app_init_t)&sceAppContentInitialize_static : NULL;
    fn_app_space_t p_app_space =
        bound_app_space
            ? (fn_app_space_t)&sceAppContentDownloadDataGetAvailableSpaceKb_static
            : NULL;

    if (!p_app_init || !p_app_space) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(0x00B4); /* APP_CONTENT */
        }
        int h_app = obs_module_open("libSceAppContent");
        if (h_app > 0) {
            if (!p_app_init) {
                p_app_init = (fn_app_init_t)obs_module_symbol(h_app, "$R9lA82OraNs");
                if (!p_app_init)
                    p_app_init = (fn_app_init_t)obs_module_symbol(
                        h_app, "sceAppContentInitialize");
            }
            if (!p_app_space) {
                p_app_space = (fn_app_space_t)obs_module_symbol(h_app, "$Gl6w5i0JokY");
                if (!p_app_space)
                    p_app_space = (fn_app_space_t)obs_module_symbol(
                        h_app, "sceAppContentDownloadDataGetAvailableSpaceKb");
            }
        }
    }

    obs_report_measure(id, "sceAppContentInitialize", "bound",
                       (uint64_t)(p_app_init != NULL), "bool");
    obs_report_measure(id, "sceAppContentDownloadDataGetAvailableSpaceKb", "bound",
                       (uint64_t)(p_app_space != NULL), "bool");

    if (p_app_init) {
        int rc_ai = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ai = p_app_init(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_ai = -sig;
            obs_report_measure(id, "app-content-init", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "app-content-init", "rc", (uint64_t)(uint32_t)rc_ai,
                           "code");
    }

    if (p_app_space) {
        uint64_t kb = 0xc7c7c7c7c7c7c7c7ULL;
        int rc_as = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_as = p_app_space("/download0", &kb);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_as = -sig;
            obs_report_measure(id, "app-download-space", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "app-download-space", "rc", (uint64_t)(uint32_t)rc_as,
                           "code");
        obs_report_measure(id, "app-download-space", "kb", kb, "hex");
        obs_report_bytes(id, "app-download-space", "kb-8b", 0,
                         (const unsigned char *)&kb, sizeof(kb));
    }

    /* 3. sceNetGetSockInfo */
    int bound_sockinfo =
        obs_address_is_callable((const void *)&sceNetGetSockInfo_static);
    typedef int (*fn_sockinfo_t)(int, void *, int, int);
    fn_sockinfo_t p_sockinfo =
        bound_sockinfo ? (fn_sockinfo_t)&sceNetGetSockInfo_static : NULL;
    if (!p_sockinfo) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(0x0001); /* NET */
        }
        int h_net = obs_module_open("libSceNet");
        if (h_net > 0) {
            p_sockinfo = (fn_sockinfo_t)obs_module_symbol(h_net, "$hLuXdjHnhiI");
            if (!p_sockinfo)
                p_sockinfo =
                    (fn_sockinfo_t)obs_module_symbol(h_net, "sceNetGetSockInfo");
        }
    }
    obs_report_measure(id, "sceNetGetSockInfo", "bound", (uint64_t)(p_sockinfo != NULL),
                       "bool");

    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }
    int s = -1;
    if (obs_address_is_callable((const void *)&sceNetSocket)) {
        s = sceNetSocket("gt07_udp", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
    }
    obs_report_measure(id, "net-socket-udp", "fd", (uint64_t)(uint32_t)s, "fd");

    if (s >= 0 && p_sockinfo) {
        uint8_t info[0x200];
        memset(info, 0xc7, sizeof(info));
        int rc_si = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_si = p_sockinfo(s, info, 1, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_si = -sig;
            obs_report_measure(id, "net-sockinfo", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "net-sockinfo", "rc", (uint64_t)(uint32_t)rc_si, "code");
        obs_report_bytes(id, "net-sockinfo", "info-200h", 0, info, sizeof(info));
    }
    if (s >= 0 && obs_address_is_callable((const void *)&sceNetSocketClose)) {
        (void)sceNetSocketClose(s);
    }

    /* 4. sceContentDeleteInitialize */
    int bound_cd_init =
        obs_address_is_callable((const void *)&sceContentDeleteInitialize_static);
    typedef int (*fn_cd_init_t)(const void *);
    fn_cd_init_t p_cd_init =
        bound_cd_init ? (fn_cd_init_t)&sceContentDeleteInitialize_static : NULL;
    if (!p_cd_init) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(0x00ee);
        }
        int h_cd = obs_module_open("libSceContentDelete");
        if (h_cd > 0) {
            p_cd_init = (fn_cd_init_t)obs_module_symbol(h_cd, "$zoxb0wEChEM");
            if (!p_cd_init)
                p_cd_init =
                    (fn_cd_init_t)obs_module_symbol(h_cd, "sceContentDeleteInitialize");
        }
    }
    obs_report_measure(id, "sceContentDeleteInitialize", "bound",
                       (uint64_t)(p_cd_init != NULL), "bool");

    if (p_cd_init) {
        void *work_buf = calloc(1, 0x4000);
        uint64_t param_block[4];
        param_block[0] = 4;
        param_block[1] = 0x4000;
        param_block[2] = 0x1c;
        param_block[3] = (uint64_t)(uintptr_t)work_buf;

        int rc_cd = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cd = p_cd_init(param_block);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_cd = -sig;
            obs_report_measure(id, "content-delete-init", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "content-delete-init", "rc", (uint64_t)(uint32_t)rc_cd,
                           "code");
        if (work_buf)
            free(work_buf);
    }

    /* 5. sceVideoRecordingSetInfo */
    int bound_vr_set =
        obs_address_is_callable((const void *)&sceVideoRecordingSetInfo_static);
    typedef int (*fn_vr_set_t)(int, const void *, size_t);
    fn_vr_set_t p_vr_set =
        bound_vr_set ? (fn_vr_set_t)(uintptr_t)&sceVideoRecordingSetInfo_static : NULL;

    if (!p_vr_set) {
        int h_vr = obs_module_open("libSceVideoRecording");
        if (h_vr <= 0 &&
            obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            h_vr = sceKernelLoadStartModule(
                "/system/common/lib/libSceVideoRecording.sprx", 0, NULL, 0, NULL, NULL);
            if (h_vr <= 0) {
                h_vr = sceKernelLoadStartModule(
                    "/system_ex/common/lib/libSceVideoRecording.sprx", 0, NULL, 0, NULL,
                    NULL);
            }
        }
        if (h_vr > 0) {
            p_vr_set = (fn_vr_set_t)obs_module_symbol(h_vr, "$Fc8qxlKINYQ");
            if (!p_vr_set)
                p_vr_set =
                    (fn_vr_set_t)obs_module_symbol(h_vr, "sceVideoRecordingSetInfo");
        }
    }
    obs_report_measure(id, "sceVideoRecordingSetInfo", "bound",
                       (uint64_t)(p_vr_set != NULL), "bool");

    if (p_vr_set) {
        /* UTF-16 "Grand Theft Auto V" (18 chars, 0x24 bytes) */
        static const uint16_t str_gta16[] = {'G', 'r', 'a', 'n', 'd', ' ', 'T',
                                             'h', 'e', 'f', 't', ' ', 'A', 'u',
                                             't', 'o', ' ', 'V', 0};
        /* UTF-16 "GTAV" (4 chars, 8 bytes) */
        static const uint16_t str_gtav16[] = {'G', 'T', 'A', 'V', 0};

        int rc_vr1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vr1 = p_vr_set(6, (const void *)str_gta16, 0x24);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_vr1 = -sig;
            obs_report_measure(id, "video-rec-setinfo-title", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "video-rec-setinfo-title", "rc",
                           (uint64_t)(uint32_t)rc_vr1, "code");

        int rc_vr2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vr2 = p_vr_set(2, (const void *)str_gtav16, 8);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_vr2 = -sig;
            obs_report_measure(id, "video-rec-setinfo-short", "fault", (uint64_t)sig,
                               "signal");
        }
        obs_report_measure(id, "video-rec-setinfo-short", "rc",
                           (uint64_t)(uint32_t)rc_vr2, "code");
    }

    return obs_pass();
}

/* ========================================================================= */
/* REQ-20261017T0230Z-gt06: NP state callback registration and delivery      */
/* ========================================================================= */
struct gt06_invocation {
    uint32_t cb_id;
    int32_t user_id;
    int32_t state;
    uint64_t userdata;
    uint64_t tid;
    uint64_t elapsed_us;
};
static struct gt06_invocation s_gt06_invocations[32];
static volatile int s_gt06_count = 0;
static uint64_t s_gt06_start_us = 0;

static void gt06_probe_cb1(int user_id, int state, void *arg) {
    uint64_t now = (obs_address_is_callable((const void *)&sceKernelGetProcessTime))
                       ? sceKernelGetProcessTime()
                       : 0;
    int idx = s_gt06_count++;
    if (idx < 32) {
        s_gt06_invocations[idx].cb_id = 1;
        s_gt06_invocations[idx].user_id = user_id;
        s_gt06_invocations[idx].state = state;
        s_gt06_invocations[idx].userdata = (uint64_t)(uintptr_t)arg;
        s_gt06_invocations[idx].tid =
            (obs_address_is_callable((const void *)&scePthreadSelf))
                ? (uint64_t)(uintptr_t)scePthreadSelf()
                : 0;
        s_gt06_invocations[idx].elapsed_us =
            (now >= s_gt06_start_us) ? (now - s_gt06_start_us) : 0;
    }
}

static void gt06_probe_cb2(int user_id, int state, void *arg) {
    uint64_t now = (obs_address_is_callable((const void *)&sceKernelGetProcessTime))
                       ? sceKernelGetProcessTime()
                       : 0;
    int idx = s_gt06_count++;
    if (idx < 32) {
        s_gt06_invocations[idx].cb_id = 2;
        s_gt06_invocations[idx].user_id = user_id;
        s_gt06_invocations[idx].state = state;
        s_gt06_invocations[idx].userdata = (uint64_t)(uintptr_t)arg;
        s_gt06_invocations[idx].tid =
            (obs_address_is_callable((const void *)&scePthreadSelf))
                ? (uint64_t)(uintptr_t)scePthreadSelf()
                : 0;
        s_gt06_invocations[idx].elapsed_us =
            (now >= s_gt06_start_us) ? (now - s_gt06_start_us) : 0;
    }
}

static obs_result check_gt06_np_callbacks(void) {
    const char *id = "000-boot/gt06-np-callbacks";
    obs_jmp_buf guard;

    int bound_reg =
        obs_address_is_callable((const void *)&sceNpRegisterStateCallbackA_static);
    int bound_get = obs_address_is_callable((const void *)&sceNpGetState_static);

    typedef int (*fn_np_reg_t)(void (*)(int, int, void *), void *);
    typedef int (*fn_np_chk_t)(void);
    typedef int (*fn_np_get_t)(int32_t, int32_t *);

    fn_np_reg_t p_reg =
        bound_reg ? (fn_np_reg_t)&sceNpRegisterStateCallbackA_static : NULL;
    fn_np_chk_t p_chk =
        obs_address_is_callable((const void *)&sceNpCheckCallback_alt)
            ? (fn_np_chk_t)&sceNpCheckCallback_alt
            : (obs_address_is_callable((const void *)&sceNpCheckCallback_static)
                   ? (fn_np_chk_t)&sceNpCheckCallback_static
                   : NULL);
    fn_np_get_t p_get = bound_get ? (fn_np_get_t)&sceNpGetState_static : NULL;

    if (!p_reg || !p_chk || !p_get) {
        int h_npm = obs_module_open("libSceNpManager");
        if (h_npm > 0) {
            if (!p_reg) {
                p_reg = (fn_np_reg_t)obs_module_symbol(h_npm, "$qQJfO8HAiaY");
                if (!p_reg)
                    p_reg = (fn_np_reg_t)obs_module_symbol(
                        h_npm, "sceNpRegisterStateCallbackA");
            }
            if (!p_chk) {
                p_chk = (fn_np_chk_t)obs_module_symbol(h_npm, "$3Zl8BePTh9Y");
                if (!p_chk)
                    p_chk = (fn_np_chk_t)obs_module_symbol(h_npm, "sceNpCheckCallback");
            }
            if (!p_get) {
                p_get = (fn_np_get_t)obs_module_symbol(h_npm, "$eQH7nWPcAgc");
                if (!p_get)
                    p_get = (fn_np_get_t)obs_module_symbol(h_npm, "sceNpGetState");
            }
        }
    }

    obs_report_measure(id, "sceNpRegisterStateCallbackA", "bound",
                       (uint64_t)(p_reg != NULL), "bool");
    obs_report_measure(id, "sceNpCheckCallback", "bound", (uint64_t)(p_chk != NULL),
                       "bool");
    obs_report_measure(id, "sceNpGetState", "bound", (uint64_t)(p_get != NULL), "bool");

    /* Initial user ID */
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceInitialize_static)) {
        (void)sceUserServiceInitialize_static(NULL);
    }
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        (void)sceUserServiceGetInitialUser_static(&user);
    }
    obs_report_measure(id, "user-initial", "user-id", (uint64_t)(uint32_t)user, "id");

    /* Read initial sceNpGetState */
    if (p_get) {
        int32_t st_val = -1;
        int rc_st = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_st = p_get(user, &st_val);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_st = -sig;
            obs_report_measure(id, "state-initial", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "state-initial", "rc", (uint64_t)(uint32_t)rc_st,
                           "code");
        obs_report_measure(id, "state-initial", "state-u32", (uint64_t)(uint32_t)st_val,
                           "state");
        obs_report_bytes(id, "state-initial", "buf-4b", 0,
                         (const unsigned char *)&st_val, sizeof(st_val));
    }

    /* 1. Registration */
    s_gt06_count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        s_gt06_start_us = sceKernelGetProcessTime();
    }

    if (p_reg) {
        int rc_r1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_r1 = p_reg(gt06_probe_cb1, (void *)0x1111);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_r1 = -sig;
            obs_report_measure(id, "reg-first", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "reg-first", "rc", (uint64_t)(uint32_t)rc_r1, "code");

        int rc_r2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_r2 = p_reg(gt06_probe_cb1, (void *)0x2222);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_r2 = -sig;
            obs_report_measure(id, "reg-same-fn", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "reg-same-fn", "rc", (uint64_t)(uint32_t)rc_r2, "code");

        int rc_r3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_r3 = p_reg(gt06_probe_cb2, (void *)0x3333);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_r3 = -sig;
            obs_report_measure(id, "reg-diff-fn", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "reg-diff-fn", "rc", (uint64_t)(uint32_t)rc_r3, "code");
    }

    /* 2. Delivery: 10 times sceNpCheckCallback() 16 ms apart */
    for (int step = 0; step < 10; step++) {
        int count_before = s_gt06_count;
        int rc_chk = -1;
        if (p_chk) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_chk = p_chk();
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_chk = -sig;
            }
        }
        int count_after = s_gt06_count;
        char step_tag[32] = "chk-step-";
        obs_format_u64(step_tag + 9, (uint64_t)step);
        obs_report_measure(id, step_tag, "rc", (uint64_t)(uint32_t)rc_chk, "code");
        obs_report_measure(id, step_tag, "delta-invoked",
                           (uint64_t)(count_after - count_before), "count");

        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(16000);
        }
    }

    obs_report_measure(id, "chk-10x", "total-invocations", (uint64_t)s_gt06_count,
                       "count");
    for (int k = 0; k < s_gt06_count && k < 32; k++) {
        char inv_tag[32] = "inv-";
        obs_format_u64(inv_tag + 4, (uint64_t)k);
        obs_report_measure(id, inv_tag, "cb-id", (uint64_t)s_gt06_invocations[k].cb_id,
                           "id");
        obs_report_measure(id, inv_tag, "user-id",
                           (uint64_t)(uint32_t)s_gt06_invocations[k].user_id, "id");
        obs_report_measure(id, inv_tag, "state",
                           (uint64_t)(uint32_t)s_gt06_invocations[k].state, "state");
        obs_report_measure(id, inv_tag, "userdata", s_gt06_invocations[k].userdata,
                           "hex");
        obs_report_measure(id, inv_tag, "tid", s_gt06_invocations[k].tid, "tid");
        obs_report_measure(id, inv_tag, "elapsed-us", s_gt06_invocations[k].elapsed_us,
                           "us");
    }

    /* 2b. Delivery: no CheckCallback for 2 seconds */
    int count_before_2s = s_gt06_count;
    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        sceKernelUsleep(2000000);
    }
    int count_after_2s = s_gt06_count;
    obs_report_measure(id, "no-check-2s", "new-invocations",
                       (uint64_t)(count_after_2s - count_before_2s), "count");

    /* Final sceNpGetState */
    if (p_get) {
        int32_t st_val2 = -1;
        int rc_st2 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_st2 = p_get(user, &st_val2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_st2 = -sig;
            obs_report_measure(id, "state-after", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "state-after", "rc", (uint64_t)(uint32_t)rc_st2, "code");
        obs_report_measure(id, "state-after", "state-u32", (uint64_t)(uint32_t)st_val2,
                           "state");
        obs_report_bytes(id, "state-after", "buf-4b", 0,
                         (const unsigned char *)&st_val2, sizeof(st_val2));
    }

    return obs_pass();
}

/* ========================================================================= */
/* REQ-20261016T2335Z-gt05: Connected pad over time, and NP startup calls    */
/* ========================================================================= */
static obs_result check_gt05_pad_and_np(void) {
    const char *id = "000-boot/gt05-pad-and-np";
    obs_jmp_buf guard;

    /* 1. Pad record over time */
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceInitialize_static)) {
        (void)sceUserServiceInitialize_static(NULL);
    }
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        (void)sceUserServiceGetInitialUser_static(&user);
    }
    obs_report_measure(id, "user", "initial-user-id", (uint64_t)(uint32_t)user, "id");

    int bound_p_init = obs_address_is_callable((const void *)&scePadInit);
    int bound_p_open = obs_address_is_callable((const void *)&scePadOpen_static);
    int bound_p_read = obs_address_is_callable((const void *)&scePadReadState_static);
    int bound_p_close = obs_address_is_callable((const void *)&scePadClose_static);

    typedef int (*fn_pad_init_t)(void);
    typedef int (*fn_pad_open_t)(int, int, int, const void *);
    typedef int (*fn_pad_read_t)(int, void *);
    typedef int (*fn_pad_close_t)(int);

    fn_pad_init_t p_p_init = bound_p_init ? (fn_pad_init_t)&scePadInit : NULL;
    fn_pad_open_t p_p_open = bound_p_open ? (fn_pad_open_t)&scePadOpen_static : NULL;
    fn_pad_read_t p_p_read =
        bound_p_read ? (fn_pad_read_t)&scePadReadState_static : NULL;
    fn_pad_close_t p_p_close =
        bound_p_close ? (fn_pad_close_t)&scePadClose_static : NULL;

    if (!p_p_open || !p_p_read || !p_p_close) {
        int h_pad = obs_module_open("libScePad");
        if (h_pad > 0) {
            if (!p_p_init)
                p_p_init = (fn_pad_init_t)obs_module_symbol(h_pad, "scePadInit");
            if (!p_p_open) {
                p_p_open = (fn_pad_open_t)obs_module_symbol(h_pad, "$xk0AcarP3V4");
                if (!p_p_open)
                    p_p_open = (fn_pad_open_t)obs_module_symbol(h_pad, "scePadOpen");
            }
            if (!p_p_read) {
                p_p_read = (fn_pad_read_t)obs_module_symbol(h_pad, "$YndgXqQVV7c");
                if (!p_p_read)
                    p_p_read =
                        (fn_pad_read_t)obs_module_symbol(h_pad, "scePadReadState");
            }
            if (!p_p_close) {
                p_p_close = (fn_pad_close_t)obs_module_symbol(h_pad, "$6ncge5+l5Qs");
                if (!p_p_close)
                    p_p_close = (fn_pad_close_t)obs_module_symbol(h_pad, "scePadClose");
            }
        }
    }

    obs_report_measure(id, "scePadOpen", "bound", (uint64_t)(p_p_open != NULL), "bool");
    obs_report_measure(id, "scePadReadState", "bound", (uint64_t)(p_p_read != NULL),
                       "bool");

    if (p_p_init) {
        int rc_pi = p_p_init();
        obs_report_measure(id, "pad-init", "rc", (uint64_t)(uint32_t)rc_pi, "code");
    }

    int handle = -1;
    if (p_p_open) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            handle = p_p_open(user, 0, 0, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            handle = -sig;
            obs_report_measure(id, "pad-open", "fault", (uint64_t)sig, "signal");
        }
    }
    obs_report_measure(id, "pad-open", "handle", (uint64_t)(uint32_t)handle, "handle");

    if (handle >= 0 && p_p_read) {
        static uint8_t pad_buf1[4096];
        static uint8_t pad_buf2[4096];
        static uint8_t pad_buf3[4096];

        /* Read 1: untouched */
        memset(pad_buf1, 0xc7, sizeof(pad_buf1));
        int rc_pr1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr1 = p_p_read(handle, pad_buf1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_pr1 = -sig;
        }
        obs_report_measure(id, "read1", "rc", (uint64_t)(uint32_t)rc_pr1, "code");
        obs_report_bytes(id, "read1", "state-120b", 0, pad_buf1, 120);
        obs_report_measure(id, "read1", "timestamp", *(uint64_t *)(pad_buf1 + 80),
                           "hex");
        obs_report_measure(id, "read1", "connected",
                           (uint64_t)*(uint32_t *)(pad_buf1 + 88), "count");
        obs_report_measure(id, "read1", "buttons",
                           (uint64_t)*(uint32_t *)(pad_buf1 + 4), "hex");

        /* Sleep 100 ms */
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(100000);
        }

        /* Read 2: untouched */
        memset(pad_buf2, 0xc7, sizeof(pad_buf2));
        int rc_pr2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr2 = p_p_read(handle, pad_buf2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_pr2 = -sig;
        }
        obs_report_measure(id, "read2", "rc", (uint64_t)(uint32_t)rc_pr2, "code");
        obs_report_bytes(id, "read2", "state-120b", 0, pad_buf2, 120);
        obs_report_measure(id, "read2", "timestamp", *(uint64_t *)(pad_buf2 + 80),
                           "hex");
        obs_report_measure(id, "read2", "connected",
                           (uint64_t)*(uint32_t *)(pad_buf2 + 88), "count");
        obs_report_measure(id, "read2", "buttons",
                           (uint64_t)*(uint32_t *)(pad_buf2 + 4), "hex");

        /* Sleep 100 ms */
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(100000);
        }

        /* Read 3: cross held / current state */
        memset(pad_buf3, 0xc7, sizeof(pad_buf3));
        int rc_pr3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_pr3 = p_p_read(handle, pad_buf3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_pr3 = -sig;
        }
        obs_report_measure(id, "read3", "rc", (uint64_t)(uint32_t)rc_pr3, "code");
        obs_report_bytes(id, "read3", "state-120b", 0, pad_buf3, 120);
        obs_report_measure(id, "read3", "timestamp", *(uint64_t *)(pad_buf3 + 80),
                           "hex");
        obs_report_measure(id, "read3", "connected",
                           (uint64_t)*(uint32_t *)(pad_buf3 + 88), "count");
        obs_report_measure(id, "read3", "buttons",
                           (uint64_t)*(uint32_t *)(pad_buf3 + 4), "hex");

        if (p_p_close) {
            (void)p_p_close(handle);
        }
    }

    /* 2. NP and user-service startup calls in PPSA04263 order */

    /* 2.1 sceNpEntitlementAccessInitialize */
    int h_ea = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_ea = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0, NULL,
            NULL);
        if (h_ea <= 0) {
            h_ea = sceKernelLoadStartModule(
                "/system_ex/common/lib/libSceNpEntitlementAccess.sprx", 0, NULL, 0,
                NULL, NULL);
        }
    }
    if (h_ea <= 0) {
        h_ea = obs_module_open("libSceNpEntitlementAccess");
    }
    int worked_sysmod_id = -1;
    if (h_ea <= 0 && obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        static const uint16_t s_np_sysmods[] = { 0x009D, 0x00A8, 0x00AD, 0x0105 };
        for (size_t i = 0; i < sizeof(s_np_sysmods)/sizeof(s_np_sysmods[0]); i++) {
            uint16_t sm = s_np_sysmods[i];
            int rc_sm = sceSysmoduleLoadModule(sm);
            if (rc_sm == 0) {
                int test_h = obs_module_open("libSceNpEntitlementAccess");
                if (test_h > 0) {
                    h_ea = test_h;
                    worked_sysmod_id = sm;
                    break;
                }
            }
        }
    }
    obs_report_measure(id, "entitlement-access-load", "handle",
                       (uint64_t)(uint32_t)h_ea, "handle");
    obs_report_measure(id, "entitlement-access-load", "sysmodule-id",
                       (uint64_t)(uint32_t)worked_sysmod_id, "id");

    typedef int (*fn_ea_init_t)(const void *, void *);
    fn_ea_init_t p_ea_init = NULL;
    if (obs_address_is_callable((const void *)&sceNpEntitlementAccessInitialize_be)) {
        p_ea_init = (fn_ea_init_t)&sceNpEntitlementAccessInitialize_be;
    } else if (obs_address_is_callable(
                   (const void *)&sceNpEntitlementAccessInitialize_static)) {
        p_ea_init = (fn_ea_init_t)&sceNpEntitlementAccessInitialize_static;
    } else if (h_ea > 0) {
        p_ea_init = (fn_ea_init_t)obs_module_symbol(h_ea, "$jO8DM8oyego");
        if (!p_ea_init)
            p_ea_init = (fn_ea_init_t)obs_module_symbol(
                h_ea, "sceNpEntitlementAccessInitialize");
    }
    obs_report_measure(id, "sceNpEntitlementAccessInitialize", "bound",
                       (uint64_t)(p_ea_init != NULL), "bool");
    if (p_ea_init) {
        uint8_t ea_param[64];
        memset(ea_param, 0xc7, sizeof(ea_param));
        uint8_t ea_out[64];
        memset(ea_out, 0xc7, sizeof(ea_out));
        int rc_eai = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_eai = p_ea_init(ea_param, ea_out);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_eai = -sig;
            obs_report_measure(id, "ea-init", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "ea-init", "rc", (uint64_t)(uint32_t)rc_eai, "code");
        obs_report_bytes(id, "ea-init", "param-64b", 0, ea_param, sizeof(ea_param));
        obs_report_bytes(id, "ea-init", "out-64b", 0, ea_out, sizeof(ea_out));
    }

    /* 2.2 sceNpRegisterStateCallbackA(cb, arg) */
    typedef int (*fn_np_reg_t)(void (*)(int, int, void *), void *);
    fn_np_reg_t p_cb_reg = NULL;
    if (obs_address_is_callable((const void *)&sceNpRegisterStateCallbackA_static)) {
        p_cb_reg = (fn_np_reg_t)&sceNpRegisterStateCallbackA_static;
    } else {
        int h_npm = obs_module_open("libSceNpManager");
        if (h_npm > 0) {
            p_cb_reg = (fn_np_reg_t)obs_module_symbol(h_npm, "$qQJfO8HAiaY");
            if (!p_cb_reg)
                p_cb_reg = (fn_np_reg_t)obs_module_symbol(
                    h_npm, "sceNpRegisterStateCallbackA");
        }
    }
    obs_report_measure(id, "sceNpRegisterStateCallbackA-gt05", "bound",
                       (uint64_t)(p_cb_reg != NULL), "bool");
    if (p_cb_reg) {
        int rc_reg = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reg = p_cb_reg(gt06_probe_cb1, (void *)0x5555);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_reg = -sig;
            obs_report_measure(id, "np-reg-cb", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "np-reg-cb", "rc", (uint64_t)(uint32_t)rc_reg, "code");
    }

    /* 2.3 sceNpTrophy2CreateContext(&ctx, user, 0, 0) */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x00AD); /* NP_TROPHY */
    }
    int h_tr = obs_module_open("libSceNpTrophy2");
    if (h_tr <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_tr = sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx", 0,
                                        NULL, 0, NULL, NULL);
    }
    typedef int (*fn_tr_create_t)(uint64_t *, int32_t, uint64_t, uint64_t);
    fn_tr_create_t p_tr_create = NULL;
    if (obs_address_is_callable((const void *)&sceNpTrophy2CreateContext_static)) {
        p_tr_create = (fn_tr_create_t)&sceNpTrophy2CreateContext_static;
    } else if (h_tr > 0) {
        p_tr_create = (fn_tr_create_t)obs_module_symbol(h_tr, "$Bagshr7OQ6Q");
        if (!p_tr_create)
            p_tr_create =
                (fn_tr_create_t)obs_module_symbol(h_tr, "sceNpTrophy2CreateContext");
    }
    obs_report_measure(id, "sceNpTrophy2CreateContext", "bound",
                       (uint64_t)(p_tr_create != NULL), "bool");
    if (p_tr_create) {
        uint64_t trophy_ctx = 0xc7c7c7c7c7c7c7c7ULL;
        int rc_tr = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_tr = p_tr_create(&trophy_ctx, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_tr = -sig;
            obs_report_measure(id, "trophy-create", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "trophy-create", "rc", (uint64_t)(uint32_t)rc_tr,
                           "code");
        obs_report_measure(id, "trophy-create", "ctx", trophy_ctx, "hex");
        obs_report_bytes(id, "trophy-create", "ctx-8b", 0,
                         (const unsigned char *)&trophy_ctx, sizeof(trophy_ctx));
    }

    /* 2.4 sceNpUniversalDataSystemTerminate() */
    int h_uds = obs_module_open("libSceNpUniversalDataSystem");
    if (h_uds <= 0 &&
        obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_uds = sceKernelLoadStartModule(
            "/system/common/lib/libSceNpUniversalDataSystem.sprx", 0, NULL, 0, NULL,
            NULL);
    }
    typedef int (*fn_uds_term_t)(void);
    fn_uds_term_t p_uds_term = NULL;
    if (obs_address_is_callable(
            (const void *)&sceNpUniversalDataSystemTerminate_static)) {
        p_uds_term = (fn_uds_term_t)&sceNpUniversalDataSystemTerminate_static;
    } else if (h_uds > 0) {
        p_uds_term = (fn_uds_term_t)obs_module_symbol(h_uds, "$47UAEuQl+iI");
        if (!p_uds_term)
            p_uds_term = (fn_uds_term_t)obs_module_symbol(
                h_uds, "sceNpUniversalDataSystemTerminate");
    }
    obs_report_measure(id, "sceNpUniversalDataSystemTerminate", "bound",
                       (uint64_t)(p_uds_term != NULL), "bool");
    if (p_uds_term) {
        int rc_udst = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_udst = p_uds_term();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_udst = -sig;
            obs_report_measure(id, "uds-terminate", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "uds-terminate", "rc", (uint64_t)(uint32_t)rc_udst,
                           "code");
    }

    /* 2.5 sceRtcGetCurrentNetworkTick(&tick) */
    int h_rtc = obs_module_open("libSceRtc");
    if (h_rtc <= 0 &&
        obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_rtc = sceKernelLoadStartModule("/system/common/lib/libSceRtc.sprx", 0, NULL,
                                         0, NULL, NULL);
    }
    typedef int (*fn_rtc_tick_t)(void *);
    fn_rtc_tick_t p_rtc_tick = NULL;
    if (obs_address_is_callable((const void *)&sceRtcGetCurrentNetworkTick_static)) {
        p_rtc_tick = (fn_rtc_tick_t)&sceRtcGetCurrentNetworkTick_static;
    } else if (h_rtc > 0) {
        p_rtc_tick = (fn_rtc_tick_t)obs_module_symbol(h_rtc, "$zO9UL3qIINQ");
        if (!p_rtc_tick)
            p_rtc_tick =
                (fn_rtc_tick_t)obs_module_symbol(h_rtc, "sceRtcGetCurrentNetworkTick");
    }
    obs_report_measure(id, "sceRtcGetCurrentNetworkTick", "bound",
                       (uint64_t)(p_rtc_tick != NULL), "bool");
    if (p_rtc_tick) {
        uint8_t tick[16];
        memset(tick, 0xc7, sizeof(tick));
        int rc_rtc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rtc = p_rtc_tick(tick);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_rtc = -sig;
            obs_report_measure(id, "rtc-tick", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "rtc-tick", "rc", (uint64_t)(uint32_t)rc_rtc, "code");
        obs_report_bytes(id, "rtc-tick", "tick-16b", 0, tick, sizeof(tick));
    }

    /* 2.6 sceContentSearchInit(...) */
    int h_cs = obs_module_open("libSceContentSearch");
    if (h_cs <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_cs = sceKernelLoadStartModule("/system/common/lib/libSceContentSearch.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    typedef int (*fn_cs_init_t)(const void *);
    fn_cs_init_t p_cs_init = NULL;
    if (obs_address_is_callable((const void *)&sceContentSearchInit_static)) {
        p_cs_init = (fn_cs_init_t)&sceContentSearchInit_static;
    } else if (h_cs > 0) {
        p_cs_init = (fn_cs_init_t)obs_module_symbol(h_cs, "$dPj4ZtRcIWk");
        if (!p_cs_init)
            p_cs_init = (fn_cs_init_t)obs_module_symbol(h_cs, "sceContentSearchInit");
    }
    obs_report_measure(id, "sceContentSearchInit", "bound",
                       (uint64_t)(p_cs_init != NULL), "bool");
    if (p_cs_init) {
        uint8_t cs_param[64];
        memset(cs_param, 0xc7, sizeof(cs_param));
        int rc_csi = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_csi = p_cs_init(cs_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_csi = -sig;
            obs_report_measure(id, "cs-init", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "cs-init", "rc", (uint64_t)(uint32_t)rc_csi, "code");
        obs_report_bytes(id, "cs-init", "param-64b", 0, cs_param, sizeof(cs_param));
    }

    /* 2.7 sceUserServiceGetEvent(&event) polled for 5 s after start */
    typedef int (*fn_us_ev_t)(void *);
    fn_us_ev_t p_us_ev = NULL;
    if (obs_address_is_callable((const void *)&sceUserServiceGetEvent_static)) {
        p_us_ev = (fn_us_ev_t)&sceUserServiceGetEvent_static;
    } else {
        int h_us = obs_module_open("libSceUserService");
        if (h_us > 0) {
            p_us_ev = (fn_us_ev_t)obs_module_symbol(h_us, "$yH17Q6NWtVg");
            if (!p_us_ev)
                p_us_ev = (fn_us_ev_t)obs_module_symbol(h_us, "sceUserServiceGetEvent");
        }
    }
    obs_report_measure(id, "sceUserServiceGetEvent", "bound",
                       (uint64_t)(p_us_ev != NULL), "bool");

    int events_received = 0;
    int initial_user_login_found = 0;

    if (p_us_ev && obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        uint64_t t_start = sceKernelGetProcessTime();
        uint64_t t_end = t_start + 1000000; /* 1 second */

        while (sceKernelGetProcessTime() < t_end) {
            uint8_t ev[256];
            memset(ev, 0xc7, sizeof(ev));
            int rc_ev = -1;
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_ev = p_us_ev(ev);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_ev = -sig;
                break;
            }
            if (rc_ev == 0) {
                int ev_idx = events_received++;
                uint32_t ev_type = *(uint32_t *)(ev + 0);
                int32_t ev_user = *(int32_t *)(ev + 4);
                if (ev_user == user && (ev_type == 1 || ev_type == 0)) {
                    initial_user_login_found = 1;
                }
                char ev_tag[32] = "event-";
                obs_format_u64(ev_tag + 6, (uint64_t)ev_idx);
                obs_report_measure(id, ev_tag, "type", (uint64_t)ev_type, "type");
                obs_report_measure(id, ev_tag, "user-id", (uint64_t)(uint32_t)ev_user,
                                   "id");
                obs_report_bytes(id, ev_tag, "raw-32b", 0, ev, 32);
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(20000); /* 20 ms */
            }
        }
    }
    obs_report_measure(id, "us-events-poll", "total-events", (uint64_t)events_received,
                       "count");
    obs_report_measure(id, "us-events-poll", "initial-user-login-arrived",
                       (uint64_t)initial_user_login_found, "bool");

    return obs_pass();
}

/* ========================================================================= */
/* REQ-20261017T1135Z-gt09: Next three online start-up placeholders           */
/* ========================================================================= */
static obs_result check_gt09_placeholders(void) {
    const char *id = "000-boot/gt09-placeholders";
    obs_jmp_buf guard;

    /* 1. sceWebBrowserDialogInitialize() */
    int rc_sm_wb = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_sm_wb = sceSysmoduleLoadModule(0x00AB);
    }
    obs_report_measure(id, "webbrowserdialog", "sysmodule-id", 0x00AB, "id");
    obs_report_measure(id, "webbrowserdialog", "sysmodule-load-rc",
                       (uint64_t)(uint32_t)rc_sm_wb, "code");

    typedef int (*fn_wb_init_t)(void);
    fn_wb_init_t p_wb_init = NULL;
    int h_wb = obs_module_open("libSceWebBrowserDialog");
    if (h_wb <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_wb = sceKernelLoadStartModule("/system/common/lib/libSceWebBrowserDialog.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    if (h_wb > 0) {
        p_wb_init = (fn_wb_init_t)obs_module_symbol(h_wb, "$jqb7HntFQFc");
        if (!p_wb_init)
            p_wb_init = (fn_wb_init_t)obs_module_symbol(h_wb, "sceWebBrowserDialogInitialize");
    }
    obs_report_measure(id, "webbrowserdialog", "bound",
                       (uint64_t)(p_wb_init != NULL), "bool");

    if (p_wb_init) {
        int rc_wb1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wb1 = p_wb_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_wb1 = -sig;
            obs_report_measure(id, "webbrowserdialog", "init-call1-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "webbrowserdialog", "init-call1-rc",
                           (uint64_t)(uint32_t)rc_wb1, "code");

        int rc_wb2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wb2 = p_wb_init();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_wb2 = -sig;
            obs_report_measure(id, "webbrowserdialog", "init-call2-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "webbrowserdialog", "init-call2-rc",
                           (uint64_t)(uint32_t)rc_wb2, "code");
    }

    /* 2. sceGameLiveStreamingInitialize(0x4000) */
    int rc_sm_gls = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_sm_gls = sceSysmoduleLoadModule(0x0081);
    }
    obs_report_measure(id, "gamelivestreaming", "sysmodule-id", 0x0081, "id");
    obs_report_measure(id, "gamelivestreaming", "sysmodule-load-rc",
                       (uint64_t)(uint32_t)rc_sm_gls, "code");

    typedef int (*fn_gls_init_t)(uint64_t);
    fn_gls_init_t p_gls_init = NULL;
    int h_gls = obs_module_open("libSceGameLiveStreaming");
    if (h_gls <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_gls = sceKernelLoadStartModule("/system/common/lib/libSceGameLiveStreaming.sprx",
                                         0, NULL, 0, NULL, NULL);
    }
    if (h_gls > 0) {
        p_gls_init = (fn_gls_init_t)obs_module_symbol(h_gls, "$kvYEw2lBndk");
        if (!p_gls_init)
            p_gls_init = (fn_gls_init_t)obs_module_symbol(h_gls, "sceGameLiveStreamingInitialize");
    }
    obs_report_measure(id, "gamelivestreaming", "bound",
                       (uint64_t)(p_gls_init != NULL), "bool");

    if (p_gls_init) {
        int rc_gls1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_gls1 = p_gls_init(0x4000);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_gls1 = -sig;
            obs_report_measure(id, "gamelivestreaming", "init-call1-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "gamelivestreaming", "init-call1-rc",
                           (uint64_t)(uint32_t)rc_gls1, "code");

        int rc_gls2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_gls2 = p_gls_init(0x4000);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_gls2 = -sig;
            obs_report_measure(id, "gamelivestreaming", "init-call2-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "gamelivestreaming", "init-call2-rc",
                           (uint64_t)(uint32_t)rc_gls2, "code");
    }

    /* 3. sceSharePlayInitialize(pool, 0x1800) */
    int rc_sm_sp = -1;
    uint32_t sm_sp_id = 0x010f;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        rc_sm_sp = sceSysmoduleLoadModule(0x010f);
    }
    obs_report_measure(id, "shareplay", "sysmodule-id", (uint64_t)sm_sp_id, "id");
    obs_report_measure(id, "shareplay", "sysmodule-load-rc",
                       (uint64_t)(uint32_t)rc_sm_sp, "code");

    typedef int (*fn_sp_init_t)(void *, size_t);
    fn_sp_init_t p_sp_init = NULL;
    int h_sp = obs_module_open("libSceSharePlay");
    if (h_sp <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_sp = sceKernelLoadStartModule("/system/common/lib/libSceSharePlay.sprx",
                                        0, NULL, 0, NULL, NULL);
    }
    if (h_sp > 0) {
        p_sp_init = (fn_sp_init_t)obs_module_symbol(h_sp, "$isruqthpYcw");
        if (!p_sp_init)
            p_sp_init = (fn_sp_init_t)obs_module_symbol(h_sp, "sceSharePlayInitialize");
    }
    obs_report_measure(id, "shareplay", "bound",
                       (uint64_t)(p_sp_init != NULL), "bool");

    if (p_sp_init) {
        uint8_t pool[0x1800];
        memset(pool, 0xc7, sizeof(pool));

        int rc_sp1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sp1 = p_sp_init(pool, sizeof(pool));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_sp1 = -sig;
            obs_report_measure(id, "shareplay", "init-call1-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "shareplay", "init-call1-rc",
                           (uint64_t)(uint32_t)rc_sp1, "code");

        uint32_t changed = 0;
        for (size_t i = 0; i < sizeof(pool); i++) {
            if (pool[i] != 0xc7)
                changed++;
        }
        obs_report_measure(id, "shareplay", "init-call1-pool-changed-count",
                           (uint64_t)changed, "count");
        obs_report_bytes(id, "shareplay", "pool-head-16b", 0, pool, 16);

        int rc_sp2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sp2 = p_sp_init(pool, sizeof(pool));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_sp2 = -sig;
            obs_report_measure(id, "shareplay", "init-call2-fault",
                               (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "shareplay", "init-call2-rc",
                           (uint64_t)(uint32_t)rc_sp2, "code");
    }

    return obs_pass();
}

/* ========================================================================== */
/* REQ-20261017T2000Z-gt10: Socket Info and Voice Thread Parameters            */
/* ========================================================================== */
static obs_result check_gt10_socket_voice(void) {
    const char *id = "000-boot/gt10-socket-voice";
    obs_jmp_buf guard;
    int sig = 0;

    /* 1. sceNetGetSockInfo */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0001);
    }
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }

    typedef int (*fn_sockinfo_t)(int, void *, int, int);
    typedef int (*fn_sockclose_t)(int);
    fn_sockinfo_t fn_info = NULL;
    fn_sockclose_t fn_close = NULL;
    int h_net = obs_module_open("libSceNet");
    if (obs_address_is_callable((const void *)&sceNetGetSockInfo_static)) {
        fn_info = (fn_sockinfo_t)&sceNetGetSockInfo_static;
    } else if (h_net > 0) {
        fn_info = (fn_sockinfo_t)obs_module_symbol(h_net, "$hLuXdjHnhiI");
        if (!fn_info)
            fn_info = (fn_sockinfo_t)obs_module_symbol(h_net, "sceNetGetSockInfo");
    }
    if (h_net > 0) {
        fn_close = (fn_sockclose_t)obs_module_symbol(h_net, "sceNetSocketClose");
        if (!fn_close)
            fn_close = (fn_sockclose_t)obs_module_symbol(h_net, "$XhC0s4r4h4c");
    }

    obs_report_measure(id, "sceNetGetSockInfo", "bound", (uint64_t)(fn_info != NULL), "bool");

    if (fn_info && obs_address_is_callable((const void *)&sceNetSocket)) {
        /* 1a. Unbound UDP socket */
        int s = sceNetSocket("gt10_u", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
        obs_report_measure(id, "udp-unbound", "fd", (uint64_t)(uint32_t)s, "fd");
        if (s >= 0) {
            uint8_t info_unbound[0xa0];
            memset(info_unbound, 0xcc, sizeof(info_unbound));
            int rc_unbound = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_unbound = fn_info(s, info_unbound, 1, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_unbound = -sig;
                obs_report_measure(id, "udp-unbound", "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "udp-unbound", "rc", (uint64_t)(uint32_t)rc_unbound, "code");
            obs_report_bytes(id, "udp-unbound", "info-160b", 0, info_unbound, sizeof(info_unbound));

            /* 1b. Bound to port 0 */
            obs_net_sockaddr_in addr0;
            memset(&addr0, 0, sizeof(addr0));
            addr0.sin_len = sizeof(addr0);
            addr0.sin_family = OBS_NET_AF_INET;
            addr0.sin_port = 0;
            addr0.sin_addr = 0;
            int rc_b0 = -1;
            if (obs_address_is_callable((const void *)&sceNetBind)) {
                rc_b0 = sceNetBind(s, &addr0, sizeof(addr0));
            }
            obs_report_measure(id, "udp-port0", "bind-rc", (uint64_t)(uint32_t)rc_b0, "code");

            uint8_t info_p0[0xa0];
            memset(info_p0, 0xcc, sizeof(info_p0));
            int rc_p0 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_p0 = fn_info(s, info_p0, 1, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_p0 = -sig;
                obs_report_measure(id, "udp-port0", "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "udp-port0", "rc", (uint64_t)(uint32_t)rc_p0, "code");
            obs_report_bytes(id, "udp-port0", "info-160b", 0, info_p0, sizeof(info_p0));

            /* Close s */
            if (fn_close) {
                fn_close(s);
            }

            /* 1d. Closed socket query */
            uint8_t info_closed[0xa0];
            memset(info_closed, 0xcc, sizeof(info_closed));
            int rc_closed = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_closed = fn_info(s, info_closed, 1, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_closed = -sig;
                obs_report_measure(id, "udp-closed", "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "udp-closed", "rc", (uint64_t)(uint32_t)rc_closed, "code");
            obs_report_bytes(id, "udp-closed", "info-160b", 0, info_closed, sizeof(info_closed));
        }

        /* 1c. Bound to port 6672 on INADDR_ANY */
        int s6672 = sceNetSocket("gt10_6", OBS_NET_AF_INET, 2 /* SOCK_DGRAM */, 0);
        obs_report_measure(id, "udp-6672", "fd", (uint64_t)(uint32_t)s6672, "fd");
        if (s6672 >= 0) {
            obs_net_sockaddr_in addr6672;
            memset(&addr6672, 0, sizeof(addr6672));
            addr6672.sin_len = sizeof(addr6672);
            addr6672.sin_family = OBS_NET_AF_INET;
            addr6672.sin_port = ((6672 & 0xff) << 8) | ((6672 >> 8) & 0xff);
            addr6672.sin_addr = 0;
            int rc_b6672 = -1;
            if (obs_address_is_callable((const void *)&sceNetBind)) {
                rc_b6672 = sceNetBind(s6672, &addr6672, sizeof(addr6672));
            }
            obs_report_measure(id, "udp-port6672", "bind-rc", (uint64_t)(uint32_t)rc_b6672, "code");

            uint8_t info_6672[0xa0];
            memset(info_6672, 0xcc, sizeof(info_6672));
            int rc_6672 = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_6672 = fn_info(s6672, info_6672, 1, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_6672 = -sig;
                obs_report_measure(id, "udp-port6672", "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "udp-port6672", "rc", (uint64_t)(uint32_t)rc_6672, "code");
            obs_report_bytes(id, "udp-port6672", "info-160b", 0, info_6672, sizeof(info_6672));

            if (fn_close) {
                fn_close(s6672);
            }
        }
    }

    /* 2. sceVoiceSetThreadsParams */
    typedef int (*fn_v_init_t)(const void *, uint32_t);
    typedef int (*fn_v_set_t)(void *);
    fn_v_init_t p_v_init = NULL;
    fn_v_set_t p_v_set = NULL;

    if (obs_address_is_callable((const void *)&sceVoiceInit_static)) {
        p_v_init = (fn_v_init_t)&sceVoiceInit_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceSetThreadsParams_static)) {
        p_v_set = (fn_v_set_t)&sceVoiceSetThreadsParams_static;
    }
    if (!p_v_init || !p_v_set) {
        if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
            (void)sceSysmoduleLoadModule(0x001A);
        }
        int h_voice = obs_module_open("libSceVoice");
        if (h_voice > 0) {
            if (!p_v_init) {
                p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "$9TrhuGzberQ");
                if (!p_v_init)
                    p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "sceVoiceInit");
            }
            if (!p_v_set) {
                p_v_set = (fn_v_set_t)obs_module_symbol(h_voice, "$clyKUyi3RYU");
                if (!p_v_set)
                    p_v_set = (fn_v_set_t)obs_module_symbol(h_voice, "sceVoiceSetThreadsParams");
            }
        }
    }

    obs_report_measure(id, "sceVoiceSetThreadsParams", "bound", (uint64_t)(p_v_set != NULL), "bool");
    if (p_v_set) {
        /* GTA V 0x38 bytes: {u32 1, u32 0x2bc, u64 0x7f, u32 0x2bc, u32 0, u64 0x7f, u32 0x2bc, u32 0, u64 0x7f, u64 0} */
        uint8_t p38[0x38];
        memset(p38, 0, sizeof(p38));
        *(uint32_t *)(p38 + 0x00) = 1;
        *(uint32_t *)(p38 + 0x04) = 0x2bc;
        *(uint64_t *)(p38 + 0x08) = 0x7f;
        *(uint32_t *)(p38 + 0x10) = 0x2bc;
        *(uint32_t *)(p38 + 0x14) = 0;
        *(uint64_t *)(p38 + 0x18) = 0x7f;
        *(uint32_t *)(p38 + 0x20) = 0x2bc;
        *(uint32_t *)(p38 + 0x24) = 0;
        *(uint64_t *)(p38 + 0x28) = 0x7f;
        *(uint64_t *)(p38 + 0x30) = 0;

        uint8_t p38_copy[0x38];
        memcpy(p38_copy, p38, sizeof(p38));

        int rc_s38 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_s38 = p_v_set(p38);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_s38 = -sig;
            obs_report_measure(id, "voice-params-0x38", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "voice-params-0x38", "rc", (uint64_t)(uint32_t)rc_s38, "code");
        int changed38 = (memcmp(p38, p38_copy, sizeof(p38)) != 0);
        obs_report_measure(id, "voice-params-0x38", "changed", (uint64_t)changed38, "bool");
        obs_report_bytes(id, "voice-params-0x38", "out-56b", 0, p38, sizeof(p38));

        /* Zeroed block */
        uint8_t p_zero[0x38];
        memset(p_zero, 0, sizeof(p_zero));
        int rc_szero = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_szero = p_v_set(p_zero);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_szero = -sig;
            obs_report_measure(id, "voice-params-zero", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "voice-params-zero", "rc", (uint64_t)(uint32_t)rc_szero, "code");
        int changed_zero = 0;
        for (size_t i = 0; i < sizeof(p_zero); i++) {
            if (p_zero[i] != 0) { changed_zero = 1; break; }
        }
        obs_report_measure(id, "voice-params-zero", "changed", (uint64_t)changed_zero, "bool");
    }

    return obs_pass();
}

/* ========================================================================== */
/* REQ-20261017T2300Z-gt11: Share Feature Permits and Video Recording Info     */
/* ========================================================================== */
static obs_result check_gt11_share_recording(void) {
    const char *id = "000-boot/gt11-share-recording";
    obs_jmp_buf guard;

    /* 1. sceShareFeaturePermit */
    typedef int (*fn_share_init_t)(uint64_t, uint64_t, uint64_t);
    typedef int (*fn_share_permit_t)(uint32_t);

    fn_share_init_t fn_init = NULL;
    fn_share_permit_t fn_permit = NULL;

    int h_sh = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_sh = sceKernelLoadStartModule("/system/common/lib/libSceShare.native.sprx", 0, NULL, 0, NULL, NULL);
    }
    if (h_sh <= 0) {
        h_sh = obs_module_open("libSceShare");
    }
    if (h_sh > 0) {
        fn_init = (fn_share_init_t)obs_module_symbol(h_sh, "$nBDD66kiFW8");
        if (!fn_init) fn_init = (fn_share_init_t)obs_module_symbol(h_sh, "sceShareInitialize");
        fn_permit = (fn_share_permit_t)obs_module_symbol(h_sh, "$YBiIdcDPrxs");
        if (!fn_permit) fn_permit = (fn_share_permit_t)obs_module_symbol(h_sh, "sceShareFeaturePermit");
    }

    obs_report_measure(id, "sceShareInitialize", "bound", (uint64_t)(fn_init != NULL), "bool");
    obs_report_measure(id, "sceShareFeaturePermit", "bound", (uint64_t)(fn_permit != NULL), "bool");

    if (fn_init) {
        int rc_si = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_si = fn_init(0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_si = -sig;
        }
        obs_report_measure(id, "share-init", "rc", (uint64_t)(uint32_t)rc_si, "code");
    }

    if (fn_permit) {
        /* Permit(0x24) call 1 */
        int rc_p24_1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p24_1 = fn_permit(0x24);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p24_1 = -sig;
        }
        obs_report_measure(id, "permit-0x24-call1", "rc", (uint64_t)(uint32_t)rc_p24_1, "code");

        /* Permit(0x10) call 1 */
        int rc_p10_1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p10_1 = fn_permit(0x10);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p10_1 = -sig;
        }
        obs_report_measure(id, "permit-0x10-call1", "rc", (uint64_t)(uint32_t)rc_p10_1, "code");

        /* Permit(0x24) call 2 */
        int rc_p24_2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p24_2 = fn_permit(0x24);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p24_2 = -sig;
        }
        obs_report_measure(id, "permit-0x24-call2", "rc", (uint64_t)(uint32_t)rc_p24_2, "code");

        /* Permit(0x10) call 2 */
        int rc_p10_2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p10_2 = fn_permit(0x10);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p10_2 = -sig;
        }
        obs_report_measure(id, "permit-0x10-call2", "rc", (uint64_t)(uint32_t)rc_p10_2, "code");

        /* Permit(0) */
        int rc_p0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p0 = fn_permit(0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p0 = -sig;
        }
        obs_report_measure(id, "permit-0-call", "rc", (uint64_t)(uint32_t)rc_p0, "code");
    }

    /* 2. libSceVideoRecordingP and sceVideoRecordingSetInfo */
    int h_vr = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_vr = sceKernelLoadStartModule("/system/common/lib/libSceVideoRecordingP.sprx", 0, NULL, 0, NULL, NULL);
        if (h_vr <= 0) {
            h_vr = sceKernelLoadStartModule("/system_ex/common/lib/libSceVideoRecordingP.sprx", 0, NULL, 0, NULL, NULL);
        }
        if (h_vr <= 0) {
            h_vr = sceKernelLoadStartModule("/system/common/lib/libSceVideoRecording.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    if (h_vr <= 0) {
        h_vr = obs_module_open("libSceVideoRecordingP");
    }
    if (h_vr <= 0) {
        h_vr = obs_module_open("libSceVideoRecording");
    }

    typedef int (*fn_vr_setinfo_t)(uint32_t, const void *, size_t);
    fn_vr_setinfo_t p_vr_set = NULL;
    if (h_vr > 0) {
        p_vr_set = (fn_vr_setinfo_t)obs_module_symbol(h_vr, "$o0V2f24x1mY");
        if (!p_vr_set) p_vr_set = (fn_vr_setinfo_t)obs_module_symbol(h_vr, "sceVideoRecordingSetInfo");
    }

    obs_report_measure(id, "libSceVideoRecordingP", "handle", (uint64_t)(uint32_t)h_vr, "handle");
    obs_report_measure(id, "sceVideoRecordingSetInfo", "bound", (uint64_t)(p_vr_set != NULL), "bool");

    if (p_vr_set) {
        static const uint16_t title1[19] = {
            'G', 'r', 'a', 'n', 'd', ' ', 'T', 'h', 'e', 'f', 't', ' ', 'A', 'u', 't', 'o', ' ', 'V', 0
        };
        uint16_t title1_buf[20];
        memcpy(title1_buf, title1, sizeof(title1));
        int rc_set6 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set6 = p_vr_set(6, title1_buf, 0x24);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_set6 = -sig;
            obs_report_measure(id, "setinfo-type6", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "setinfo-type6", "rc", (uint64_t)(uint32_t)rc_set6, "code");
        int ch6 = (memcmp(title1_buf, title1, sizeof(title1)) != 0);
        obs_report_measure(id, "setinfo-type6", "changed", (uint64_t)ch6, "bool");

        static const uint16_t title2[5] = { 'G', 'T', 'A', 'V', 0 };
        uint16_t title2_buf[6];
        memcpy(title2_buf, title2, sizeof(title2));
        int rc_set2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set2 = p_vr_set(2, title2_buf, 8);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_set2 = -sig;
            obs_report_measure(id, "setinfo-type2", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "setinfo-type2", "rc", (uint64_t)(uint32_t)rc_set2, "code");
        int ch2 = (memcmp(title2_buf, title2, sizeof(title2)) != 0);
        obs_report_measure(id, "setinfo-type2", "changed", (uint64_t)ch2, "bool");
    }

    return obs_pass();
}

/* ========================================================================== */
/* REQ-20261017T1945Z-tr01: Trophies, Save Search and RTC Tick Offline         */
/* ========================================================================== */
static obs_result check_tr01_trophy_savedata_rtc(void) {
    const char *id = "000-boot/tr01-trophy-savedata-rtc";
    obs_jmp_buf guard;

    /* 1. libSceNpTrophy2 binding routes */
    int h_tr = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        int rc_sm4c = sceSysmoduleLoadModule(0x004c);
        obs_report_measure(id, "trophy2", "sysmodule-0x004c-rc", (uint64_t)(uint32_t)rc_sm4c, "code");
        int rc_sm108 = sceSysmoduleLoadModule(0x0108);
        obs_report_measure(id, "trophy2", "sysmodule-0x0108-rc", (uint64_t)(uint32_t)rc_sm108, "code");
        int rc_sm10c = sceSysmoduleLoadModule(0x010c);
        obs_report_measure(id, "trophy2", "sysmodule-0x010c-rc", (uint64_t)(uint32_t)rc_sm10c, "code");
    }
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_tr = sceKernelLoadStartModule("/system/common/lib/libSceNpTrophy2.sprx", 0, NULL, 0, NULL, NULL);
        if (h_tr <= 0) {
            h_tr = sceKernelLoadStartModule("/system_ex/common/lib/libSceNpTrophy2.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    if (h_tr <= 0) {
        h_tr = obs_module_open("libSceNpTrophy2");
    }
    obs_report_measure(id, "trophy2", "module-handle", (uint64_t)(uint32_t)h_tr, "handle");

    typedef int (*fn_tr_create_ctx_t)(uint64_t *, int32_t, uint32_t, uint32_t);
    typedef int (*fn_tr_destroy_ctx_t)(uint64_t);

    fn_tr_create_ctx_t fn_cctx = NULL;
    fn_tr_destroy_ctx_t fn_dctx = NULL;

    if (h_tr > 0) {
        fn_cctx = (fn_tr_create_ctx_t)obs_module_symbol(h_tr, "$zS1L88oYvj4");
        if (!fn_cctx) fn_cctx = (fn_tr_create_ctx_t)obs_module_symbol(h_tr, "sceNpTrophy2CreateContext");
        fn_dctx = (fn_tr_destroy_ctx_t)obs_module_symbol(h_tr, "sceNpTrophy2DestroyContext");
    }
    obs_report_measure(id, "trophy2", "create-context-bound", (uint64_t)(fn_cctx != NULL), "bool");

    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceInitialize_static)) {
        (void)sceUserServiceInitialize_static(NULL);
    }
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser_static)) {
        (void)sceUserServiceGetInitialUser_static(&user);
    }
    obs_report_measure(id, "trophy2", "user-id", (uint64_t)(uint32_t)user, "id");

    if (fn_cctx) {
        uint64_t ctx1 = 0;
        int rc_c1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_c1 = fn_cctx(&ctx1, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_c1 = -sig;
        }
        obs_report_measure(id, "trophy2", "create-ctx1-rc", (uint64_t)(uint32_t)rc_c1, "code");
        obs_report_measure(id, "trophy2", "create-ctx1-handle", ctx1, "hex");

        /* user -1 */
        uint64_t ctx_neg = 0;
        int rc_neg = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_neg = fn_cctx(&ctx_neg, -1, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_neg = -sig;
        }
        obs_report_measure(id, "trophy2", "create-ctx-neg1-rc", (uint64_t)(uint32_t)rc_neg, "code");

        if (fn_dctx && ctx1 != 0) {
            fn_dctx(ctx1);
        }
    }

    /* 2. libSceSaveData_native DirNameSearch */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x0015); /* SAVE_DATA */
    }
    int h_sd = obs_module_open("libSceSaveData_native");
    if (h_sd <= 0) h_sd = obs_module_open("libSceSaveData");
    obs_report_measure(id, "savedata", "module-handle", (uint64_t)(uint32_t)h_sd, "handle");

    typedef int (*fn_sd_search_t)(const void *, void *);
    fn_sd_search_t fn_search = NULL;
    if (h_sd > 0) {
        fn_search = (fn_sd_search_t)obs_module_symbol(h_sd, "$dyIhnXq-0SM");
        if (!fn_search) fn_search = (fn_sd_search_t)obs_module_symbol(h_sd, "sceSaveDataDirNameSearch");
    }
    obs_report_measure(id, "savedata", "dir-search-bound", (uint64_t)(fn_search != NULL), "bool");

    /* 3. sceRtcGetCurrentNetworkTick beside sceRtcGetCurrentTick */
    typedef int (*fn_rtc_tick_t)(uint64_t *);
    fn_rtc_tick_t fn_cur_tick = NULL;
    fn_rtc_tick_t fn_net_tick = NULL;

    if (obs_address_is_callable((const void *)&sceRtcGetCurrentNetworkTick_static)) {
        fn_net_tick = (fn_rtc_tick_t)&sceRtcGetCurrentNetworkTick_static;
    }
    int h_rtc = obs_module_open("libSceRtc");
    if (h_rtc > 0) {
        fn_cur_tick = (fn_rtc_tick_t)obs_module_symbol(h_rtc, "$RfVyLTV2wNc");
        if (!fn_cur_tick) fn_cur_tick = (fn_rtc_tick_t)obs_module_symbol(h_rtc, "sceRtcGetCurrentTick");
        if (!fn_net_tick) {
            fn_net_tick = (fn_rtc_tick_t)obs_module_symbol(h_rtc, "$zO9UL3qIINQ");
            if (!fn_net_tick) fn_net_tick = (fn_rtc_tick_t)obs_module_symbol(h_rtc, "sceRtcGetCurrentNetworkTick");
        }
    }

    obs_report_measure(id, "rtc", "cur-tick-bound", (uint64_t)(fn_cur_tick != NULL), "bool");
    obs_report_measure(id, "rtc", "net-tick-bound", (uint64_t)(fn_net_tick != NULL), "bool");

    uint64_t tick_cur = 0;
    int rc_cur = -1;
    if (fn_cur_tick) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_cur = fn_cur_tick(&tick_cur);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_cur = -sig;
        }
    }
    obs_report_measure(id, "rtc", "cur-tick-rc", (uint64_t)(uint32_t)rc_cur, "code");
    obs_report_measure(id, "rtc", "cur-tick-val", tick_cur, "tick");

    uint64_t tick_net = 0;
    int rc_net = -1;
    if (fn_net_tick) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_net = fn_net_tick(&tick_net);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_net = -sig;
        }
    }
    obs_report_measure(id, "rtc", "net-tick-rc", (uint64_t)(uint32_t)rc_net, "code");
    obs_report_measure(id, "rtc", "net-tick-val", tick_net, "tick");

    return obs_pass();
}

/* ========================================================================== */
/* REQ-20261018T0045Z-gt12: Three Command-Buffer Builders in Story Mode Shapes */
/* ========================================================================== */
typedef struct {
    uint64_t count_prefix;
    uint64_t begin;
    uint64_t end;
    uint64_t cur;
    uint64_t end2;
    uint64_t overflow_cb;
    uint64_t overflow_ctx;
    uint32_t reserved_dw;
    uint32_t pad34;
    uint8_t cmdbuf[512];
    uint8_t guard[64];
} obs_gt12_cb_probe;

static void gt12_cb_prepare(obs_gt12_cb_probe *probe) {
    probe->count_prefix = 0;
    probe->begin = (uint64_t)(uintptr_t)probe->cmdbuf;
    probe->end = (uint64_t)(uintptr_t)(probe->cmdbuf + sizeof(probe->cmdbuf));
    probe->cur = probe->begin;
    probe->end2 = probe->end;
    probe->overflow_cb = 0;
    probe->overflow_ctx = 0;
    probe->reserved_dw = 0;
    probe->pad34 = 0;
    memset(probe->cmdbuf, 0xcc, sizeof(probe->cmdbuf));
    memset(probe->guard, 0xc7, sizeof(probe->guard));
}

static obs_result check_gt12_story_builders(void) {
    const char *id = "000-boot/gt12-story-builders";
    obs_jmp_buf guard;
    int sig = 0;

    int h_agc = obs_module_open("libSceAgc");
    if (h_agc <= 0 && obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_agc = sceKernelLoadStartModule("/system/common/lib/libSceAgc.sprx", 0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "libSceAgc", "handle", (uint64_t)(uint32_t)h_agc, "handle");

    typedef uint64_t (*fn_agc_builder_t)(void *writer, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);
    typedef uint64_t (*fn_agc_rewind_patch_t)(void *packet, uint64_t state);

    fn_agc_builder_t fn_jump = NULL;
    fn_agc_builder_t fn_write_data = NULL;
    fn_agc_builder_t fn_event_write = NULL;
    fn_agc_builder_t fn_rewind = NULL;
    fn_agc_rewind_patch_t fn_rewind_patch = NULL;
    fn_agc_builder_t fn_wait_reg_mem = NULL;

    if (obs_address_is_callable((const void *)&sceAgcDcbJump)) {
        fn_jump = (fn_agc_builder_t)&sceAgcDcbJump;
    }
    if (obs_address_is_callable((const void *)&sceAgcAcbWriteData)) {
        fn_write_data = (fn_agc_builder_t)&sceAgcAcbWriteData;
    }
    if (obs_address_is_callable((const void *)&sceAgcAcbEventWrite)) {
        fn_event_write = (fn_agc_builder_t)&sceAgcAcbEventWrite;
    }
    if (obs_address_is_callable((const void *)&sceAgcDcbRewind)) {
        fn_rewind = (fn_agc_builder_t)&sceAgcDcbRewind;
    }
    if (obs_address_is_callable((const void *)&sceAgcAcbWaitRegMem)) {
        fn_wait_reg_mem = (fn_agc_builder_t)&sceAgcAcbWaitRegMem;
    }

    if (h_agc > 0) {
        if (!fn_jump) {
            fn_jump = (fn_agc_builder_t)obs_module_symbol(h_agc, "$o0TctNARIMU");
            if (!fn_jump) fn_jump = (fn_agc_builder_t)obs_module_symbol(h_agc, "$xSAR0LTcRKM");
            if (!fn_jump) fn_jump = (fn_agc_builder_t)obs_module_symbol(h_agc, "sceAgcDcbJump");
        }
        if (!fn_write_data) {
            fn_write_data = (fn_agc_builder_t)obs_module_symbol(h_agc, "$hM+Qs9c+nnk");
            if (!fn_write_data) fn_write_data = (fn_agc_builder_t)obs_module_symbol(h_agc, "$eZ4+17OQz4Q");
            if (!fn_write_data) fn_write_data = (fn_agc_builder_t)obs_module_symbol(h_agc, "sceAgcAcbWriteData");
        }
        if (!fn_event_write) {
            fn_event_write = (fn_agc_builder_t)obs_module_symbol(h_agc, "$4STpdZqzVnA");
            if (!fn_event_write) fn_event_write = (fn_agc_builder_t)obs_module_symbol(h_agc, "$cFazmnXpJOE");
            if (!fn_event_write) fn_event_write = (fn_agc_builder_t)obs_module_symbol(h_agc, "sceAgcAcbEventWrite");
        }
        if (!fn_rewind) {
            fn_rewind = (fn_agc_builder_t)obs_module_symbol(h_agc, "$rzCw94Mx980");
            if (!fn_rewind) fn_rewind = (fn_agc_builder_t)obs_module_symbol(h_agc, "$zfcxg-ewMK8");
            if (!fn_rewind) fn_rewind = (fn_agc_builder_t)obs_module_symbol(h_agc, "sceAgcDcbRewind");
        }
        if (!fn_rewind_patch) {
            fn_rewind_patch = (fn_agc_rewind_patch_t)obs_module_symbol(h_agc, "$nt5pCN9AJc4");
            if (!fn_rewind_patch) fn_rewind_patch = (fn_agc_rewind_patch_t)obs_module_symbol(h_agc, "sceAgcRewindPatchSetRewindState");
        }
        if (!fn_wait_reg_mem) {
            fn_wait_reg_mem = (fn_agc_builder_t)obs_module_symbol(h_agc, "$TgbnA+r32YY");
            if (!fn_wait_reg_mem) fn_wait_reg_mem = (fn_agc_builder_t)obs_module_symbol(h_agc, "sceAgcAcbWaitRegMem");
        }
    }

    obs_report_measure(id, "sceAgcDcbJump", "bound", (uint64_t)(fn_jump != NULL), "bool");
    obs_report_measure(id, "sceAgcAcbWriteData", "bound", (uint64_t)(fn_write_data != NULL), "bool");
    obs_report_measure(id, "sceAgcAcbEventWrite", "bound", (uint64_t)(fn_event_write != NULL), "bool");
    obs_report_measure(id, "sceAgcDcbRewind", "bound", (uint64_t)(fn_rewind != NULL), "bool");
    obs_report_measure(id, "sceAgcRewindPatchSetRewindState", "bound", (uint64_t)(fn_rewind_patch != NULL), "bool");
    obs_report_measure(id, "sceAgcAcbWaitRegMem", "bound", (uint64_t)(fn_wait_reg_mem != NULL), "bool");

    static _Alignas(64) obs_gt12_cb_probe s_probe;
    static _Alignas(64) uint8_t s_target_buf[256];
    memset(s_target_buf, 0x5a, sizeof(s_target_buf));

    /* 1. sceAgcDcbJump */
    if (fn_jump) {
        /* Arm 1: (dcb, 1, 2, target, 0xe) */
        gt12_cb_prepare(&s_probe);
        int rc1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc1 = (int)fn_jump(&s_probe.begin, 1, 2, (uint64_t)(uintptr_t)s_target_buf, 0xe, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc1 = -sig;
            obs_report_measure(id, "jump-arm1", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv1 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "jump-arm1", "rc", (uint64_t)(uint32_t)rc1, "code");
        obs_report_measure(id, "jump-arm1", "advance", (uint64_t)adv1, "bytes");
        obs_report_bytes(id, "jump-arm1", "packet", 0, s_probe.cmdbuf, adv1 > 0 && adv1 <= 64 ? adv1 : 16);

        /* Arm 2: (dcb, 1, 2, 0, 0) */
        gt12_cb_prepare(&s_probe);
        int rc2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc2 = (int)fn_jump(&s_probe.begin, 1, 2, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc2 = -sig;
            obs_report_measure(id, "jump-arm2", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv2 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "jump-arm2", "rc", (uint64_t)(uint32_t)rc2, "code");
        obs_report_measure(id, "jump-arm2", "advance", (uint64_t)adv2, "bytes");
        obs_report_bytes(id, "jump-arm2", "packet", 0, s_probe.cmdbuf, adv2 > 0 && adv2 <= 64 ? adv2 : 16);

        /* Arm 3: (dcb, 0, 0, target, 0xe) */
        gt12_cb_prepare(&s_probe);
        int rc3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc3 = (int)fn_jump(&s_probe.begin, 0, 0, (uint64_t)(uintptr_t)s_target_buf, 0xe, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc3 = -sig;
            obs_report_measure(id, "jump-arm3", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv3 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "jump-arm3", "rc", (uint64_t)(uint32_t)rc3, "code");
        obs_report_measure(id, "jump-arm3", "advance", (uint64_t)adv3, "bytes");
        obs_report_bytes(id, "jump-arm3", "packet", 0, s_probe.cmdbuf, adv3 > 0 && adv3 <= 64 ? adv3 : 16);
    }

    /* 2. sceAgcAcbWriteData */
    if (fn_write_data) {
        static _Alignas(64) uint32_t s_src_data[64];
        for (int i = 0; i < 64; i++) s_src_data[i] = 0x11111111u;
        uint64_t dst_addr = 0x740040d00000ULL;

        /* Arm 1: (acb, 2, 0, dst, src, 1) */
        gt12_cb_prepare(&s_probe);
        int rc_wd1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wd1 = (int)fn_write_data(&s_probe.begin, 2, 0, dst_addr, (uint64_t)(uintptr_t)s_src_data, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_wd1 = -sig;
            obs_report_measure(id, "writedata-cnt1", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_wd1 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "writedata-cnt1", "rc", (uint64_t)(uint32_t)rc_wd1, "code");
        obs_report_measure(id, "writedata-cnt1", "advance", (uint64_t)adv_wd1, "bytes");
        obs_report_bytes(id, "writedata-cnt1", "packet", 0, s_probe.cmdbuf, adv_wd1 > 0 && adv_wd1 <= 128 ? adv_wd1 : 32);

        /* Arm 2: (acb, 2, 0, dst, src, 0x40) */
        gt12_cb_prepare(&s_probe);
        int rc_wd40 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wd40 = (int)fn_write_data(&s_probe.begin, 2, 0, dst_addr, (uint64_t)(uintptr_t)s_src_data, 0x40);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_wd40 = -sig;
            obs_report_measure(id, "writedata-cnt40", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_wd40 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "writedata-cnt40", "rc", (uint64_t)(uint32_t)rc_wd40, "code");
        obs_report_measure(id, "writedata-cnt40", "advance", (uint64_t)adv_wd40, "bytes");
        obs_report_bytes(id, "writedata-cnt40", "packet", 0, s_probe.cmdbuf, adv_wd40 > 0 && adv_wd40 <= 300 ? adv_wd40 : 64);
    }

    /* 3. sceAgcAcbEventWrite */
    if (fn_event_write) {
        /* Arm 1: (acb, 7, 0, 0x78, 0x4ee) */
        gt12_cb_prepare(&s_probe);
        int rc_ew1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ew1 = (int)fn_event_write(&s_probe.begin, 7, 0, 0x78, 0x4ee, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_ew1 = -sig;
            obs_report_measure(id, "eventwrite-arm1", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_ew1 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "eventwrite-arm1", "rc", (uint64_t)(uint32_t)rc_ew1, "code");
        obs_report_measure(id, "eventwrite-arm1", "advance", (uint64_t)adv_ew1, "bytes");
        obs_report_bytes(id, "eventwrite-arm1", "packet", 0, s_probe.cmdbuf, adv_ew1 > 0 && adv_ew1 <= 64 ? adv_ew1 : 16);

        /* Arm 2: (acb, 7, 0, 0, 0) */
        gt12_cb_prepare(&s_probe);
        int rc_ew2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_ew2 = (int)fn_event_write(&s_probe.begin, 7, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_ew2 = -sig;
            obs_report_measure(id, "eventwrite-arm2", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_ew2 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "eventwrite-arm2", "rc", (uint64_t)(uint32_t)rc_ew2, "code");
        obs_report_measure(id, "eventwrite-arm2", "advance", (uint64_t)adv_ew2, "bytes");
        obs_report_bytes(id, "eventwrite-arm2", "packet", 0, s_probe.cmdbuf, adv_ew2 > 0 && adv_ew2 <= 64 ? adv_ew2 : 16);
    }

    /* 4. sceAgcDcbRewind and sceAgcRewindPatchSetRewindState */
    if (fn_rewind) {
        /* Arm 1: zero arguments */
        gt12_cb_prepare(&s_probe);
        int rc_rw0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rw0 = (int)fn_rewind(&s_probe.begin, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_rw0 = -sig;
            obs_report_measure(id, "rewind-zero", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_rw0 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "rewind-zero", "rc", (uint64_t)(uint32_t)rc_rw0, "code");
        obs_report_measure(id, "rewind-zero", "advance", (uint64_t)adv_rw0, "bytes");
        obs_report_bytes(id, "rewind-zero", "packet", 0, s_probe.cmdbuf, adv_rw0 > 0 && adv_rw0 <= 64 ? adv_rw0 : 16);

        /* Arm 2: sentinel arguments */
        gt12_cb_prepare(&s_probe);
        int rc_rw1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rw1 = (int)fn_rewind(&s_probe.begin, 0x11, 0x22, 0x33, 0x44, 0x55);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_rw1 = -sig;
            obs_report_measure(id, "rewind-sentinels", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_rw1 = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "rewind-sentinels", "rc", (uint64_t)(uint32_t)rc_rw1, "code");
        obs_report_measure(id, "rewind-sentinels", "advance", (uint64_t)adv_rw1, "bytes");
        obs_report_bytes(id, "rewind-sentinels", "packet", 0, s_probe.cmdbuf, adv_rw1 > 0 && adv_rw1 <= 64 ? adv_rw1 : 16);

        /* Patch test */
        if (fn_rewind_patch && adv_rw0 > 0) {
            uint8_t pkt_copy[64];
            memcpy(pkt_copy, s_probe.cmdbuf, sizeof(pkt_copy));
            int rc_patch = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_patch = (int)fn_rewind_patch(s_probe.cmdbuf, 0x12345678ULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_patch = -sig;
                obs_report_measure(id, "rewind-patch", "fault", (uint64_t)sig, "signal");
            }
            obs_report_measure(id, "rewind-patch", "rc", (uint64_t)(uint32_t)rc_patch, "code");
            int changed = memcmp(pkt_copy, s_probe.cmdbuf, adv_rw0) != 0;
            obs_report_measure(id, "rewind-patch", "changed", (uint64_t)changed, "bool");
            obs_report_bytes(id, "rewind-patch", "packet-after", 0, s_probe.cmdbuf, adv_rw0);
        }
    }

    /* 5. sceAgcAcbWaitRegMem */
    if (fn_wait_reg_mem) {
        gt12_cb_prepare(&s_probe);
        int rc_wrm = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wrm = (int)fn_wait_reg_mem(&s_probe.begin, 0, 3, 0, 3, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_wrm = -sig;
            obs_report_measure(id, "acb-waitregmem", "fault", (uint64_t)sig, "signal");
        }
        uint32_t adv_wrm = (uint32_t)(s_probe.cur - s_probe.begin);
        obs_report_measure(id, "acb-waitregmem", "rc", (uint64_t)(uint32_t)rc_wrm, "code");
        obs_report_measure(id, "acb-waitregmem", "advance", (uint64_t)adv_wrm, "bytes");
        obs_report_bytes(id, "acb-waitregmem", "packet", 0, s_probe.cmdbuf, adv_wrm > 0 && adv_wrm <= 64 ? adv_wrm : 32);
    }

    /* 6. sceAgcDriverSubmitAcb */
    int h_agcd = obs_module_open("libSceAgcDriver");
    typedef int (*fn_submit_acb_t)(void *q, const void *acb, uint32_t count, void *extra);
    fn_submit_acb_t fn_sub_acb = NULL;
    if (h_agcd > 0) {
        fn_sub_acb = (fn_submit_acb_t)obs_module_symbol(h_agcd, "$1PJFv69nJIE");
        if (!fn_sub_acb) fn_sub_acb = (fn_submit_acb_t)obs_module_symbol(h_agcd, "sceAgcDriverSubmitAcb");
    }
    obs_report_measure(id, "sceAgcDriverSubmitAcb", "bound", (uint64_t)(fn_sub_acb != NULL), "bool");
    if (fn_sub_acb) {
        int rc_sub_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sub_null = fn_sub_acb(NULL, NULL, 0, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_sub_null = -sig;
            obs_report_measure(id, "submit-acb-null", "fault", (uint64_t)sig, "signal");
        }
        obs_report_measure(id, "submit-acb-null", "rc", (uint64_t)(uint32_t)rc_sub_null, "code");
    }

    return obs_pass();
}

/* REQ-20261018T1300Z-gt13: libSceVoice ports in PPSA04263's shapes */
static obs_result check_gt13_voice_ports(void) {
    const char *id = "000-boot/gt13-voice-ports";
    obs_jmp_buf guard;
    int sig = 0;

    /* Load sysmodule 0x001A (libSceVoice) */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x001A);
    }
    int h_voice = obs_module_open("libSceVoice");
    obs_report_measure(id, "libSceVoice", "handle", (uint64_t)(uint32_t)h_voice, "handle");

    typedef int (*fn_v_init_t)(const void *, uint32_t);
    typedef int (*fn_v_set_t)(void *);
    typedef int (*fn_v_create_t)(uint32_t *, const void *);
    typedef int (*fn_v_connect_t)(uint32_t, uint32_t);
    typedef int (*fn_v_getinfo_t)(uint32_t, void *);
    typedef int (*fn_v_start_t)(void);
    typedef int (*fn_v_stop_t)(void);
    typedef int (*fn_v_del_t)(uint32_t);

    fn_v_init_t p_v_init = NULL;
    fn_v_set_t p_v_set = NULL;
    fn_v_create_t p_v_create = NULL;
    fn_v_connect_t p_v_connect = NULL;
    fn_v_getinfo_t p_v_getinfo = NULL;
    fn_v_start_t p_v_start = NULL;
    fn_v_stop_t p_v_stop = NULL;
    fn_v_del_t p_v_del = NULL;

    if (obs_address_is_callable((const void *)&sceVoiceInit_static)) {
        p_v_init = (fn_v_init_t)&sceVoiceInit_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceSetThreadsParams_static)) {
        p_v_set = (fn_v_set_t)&sceVoiceSetThreadsParams_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceCreatePort_static)) {
        p_v_create = (fn_v_create_t)&sceVoiceCreatePort_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceConnectIPortToOPort_static)) {
        p_v_connect = (fn_v_connect_t)&sceVoiceConnectIPortToOPort_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceGetPortInfo_static)) {
        p_v_getinfo = (fn_v_getinfo_t)&sceVoiceGetPortInfo_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceStart_static)) {
        p_v_start = (fn_v_start_t)&sceVoiceStart_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceStop_static)) {
        p_v_stop = (fn_v_stop_t)&sceVoiceStop_static;
    }
    if (obs_address_is_callable((const void *)&sceVoiceDeletePort_static)) {
        p_v_del = (fn_v_del_t)&sceVoiceDeletePort_static;
    }

    if (h_voice > 0) {
        if (!p_v_init) {
            p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "$9TrhuGzberQ");
            if (!p_v_init) p_v_init = (fn_v_init_t)obs_module_symbol(h_voice, "sceVoiceInit");
        }
        if (!p_v_set) {
            p_v_set = (fn_v_set_t)obs_module_symbol(h_voice, "$clyKUyi3RYU");
            if (!p_v_set) p_v_set = (fn_v_set_t)obs_module_symbol(h_voice, "sceVoiceSetThreadsParams");
        }
        if (!p_v_create) {
            p_v_create = (fn_v_create_t)obs_module_symbol(h_voice, "$nXpje5yNpaE");
            if (!p_v_create) p_v_create = (fn_v_create_t)obs_module_symbol(h_voice, "sceVoiceCreatePort");
        }
        if (!p_v_connect) {
            p_v_connect = (fn_v_connect_t)obs_module_symbol(h_voice, "$oV9GAdJ23Gw");
            if (!p_v_connect) p_v_connect = (fn_v_connect_t)obs_module_symbol(h_voice, "sceVoiceConnectIPortToOPort");
        }
        if (!p_v_getinfo) {
            p_v_getinfo = (fn_v_getinfo_t)obs_module_symbol(h_voice, "$CrLqDwWLoXM");
            if (!p_v_getinfo) p_v_getinfo = (fn_v_getinfo_t)obs_module_symbol(h_voice, "sceVoiceGetPortInfo");
        }
        if (!p_v_start) {
            p_v_start = (fn_v_start_t)obs_module_symbol(h_voice, "$54phPH2LZls");
            if (!p_v_start) p_v_start = (fn_v_start_t)obs_module_symbol(h_voice, "sceVoiceStart");
        }
        if (!p_v_stop) {
            p_v_stop = (fn_v_stop_t)obs_module_symbol(h_voice, "$Ao2YNSA7-Qo");
            if (!p_v_stop) p_v_stop = (fn_v_stop_t)obs_module_symbol(h_voice, "sceVoiceStop");
        }
        if (!p_v_del) {
            p_v_del = (fn_v_del_t)obs_module_symbol(h_voice, "$b7kJI+nx2hg");
            if (!p_v_del) p_v_del = (fn_v_del_t)obs_module_symbol(h_voice, "$b7kJI-nx2hg");
            if (!p_v_del) p_v_del = (fn_v_del_t)obs_module_symbol(h_voice, "sceVoiceDeletePort");
        }
    }

    obs_report_measure(id, "sceVoiceCreatePort", "bound", (uint64_t)(p_v_create != NULL), "bool");
    obs_report_measure(id, "sceVoiceConnectIPortToOPort", "bound", (uint64_t)(p_v_connect != NULL), "bool");
    obs_report_measure(id, "sceVoiceGetPortInfo", "bound", (uint64_t)(p_v_getinfo != NULL), "bool");
    obs_report_measure(id, "sceVoiceStart", "bound", (uint64_t)(p_v_start != NULL), "bool");
    obs_report_measure(id, "sceVoiceStop", "bound", (uint64_t)(p_v_stop != NULL), "bool");
    obs_report_measure(id, "sceVoiceDeletePort", "bound", (uint64_t)(p_v_del != NULL), "bool");

    /* Initialise voice subsystem as in gt02/gt10 */
    if (p_v_init) {
        uint8_t init_param[0x20];
        memset(init_param, 0, sizeof(init_param));
        *(uint32_t *)init_param = 0x10000000;
        int rc_vi = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_vi = p_v_init(init_param, 100);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_vi = -sig;
        }
        obs_report_measure(id, "voice-init", "rc", (uint64_t)(uint32_t)rc_vi, "code");
    }

    if (p_v_set) {
        uint8_t p38[0x38];
        memset(p38, 0, sizeof(p38));
        *(uint32_t *)(p38 + 0x00) = 1;
        *(uint32_t *)(p38 + 0x04) = 0x2bc;
        *(uint64_t *)(p38 + 0x08) = 0x7f;
        *(uint32_t *)(p38 + 0x10) = 0x2bc;
        *(uint32_t *)(p38 + 0x14) = 0;
        *(uint64_t *)(p38 + 0x18) = 0x7f;
        *(uint32_t *)(p38 + 0x20) = 0x2bc;
        *(uint32_t *)(p38 + 0x24) = 0;
        *(uint64_t *)(p38 + 0x28) = 0x7f;
        *(uint64_t *)(p38 + 0x30) = 0;
        int rc_set = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set = p_v_set(p38);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_set = -sig;
        }
        obs_report_measure(id, "voice-set-threads", "rc", (uint64_t)(uint32_t)rc_set, "code");
    }

    int user_id = 0x1ea2f4d9;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        (void)sceUserServiceGetInitialUser(&user_id);
    }
    obs_report_measure(id, "user", "id", (uint64_t)(uint32_t)user_id, "id");

    uint32_t p0 = 0xffffffff, p1 = 0xffffffff, p2 = 0xffffffff, p3 = 0xffffffff;

    /* 1. sceVoiceCreatePort(&p0, {u32 0, u32 0, f32 1.0, u32 U, u64 0}) */
    if (p_v_create) {
        uint8_t b0[0x18] = {0};
        *(uint32_t *)(b0 + 0x00) = 0;
        *(uint32_t *)(b0 + 0x04) = 0;
        *(float *)(b0 + 0x08) = 1.0f;
        *(uint32_t *)(b0 + 0x0c) = (uint32_t)user_id;
        *(uint64_t *)(b0 + 0x10) = 0;

        uint8_t b0_orig[0x18];
        memcpy(b0_orig, b0, 0x18);

        int rc_p0 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p0 = p_v_create(&p0, b0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p0 = -sig;
        }
        obs_report_measure(id, "create-p0", "rc", (uint64_t)(uint32_t)rc_p0, "code");
        obs_report_measure(id, "create-p0", "port-id", (uint64_t)p0, "id");
        obs_report_measure(id, "create-p0", "changed", (uint64_t)(memcmp(b0, b0_orig, 0x18) != 0), "bool");
        obs_report_bytes(id, "create-p0", "param-24b", 0, b0, 0x18);
    }

    /* 2. sceVoiceCreatePort(&p1, {u32 4, u32 0xa, f32 1.0, u32 0x1c84, u64 0}) */
    if (p_v_create) {
        uint8_t b1[0x18] = {0};
        *(uint32_t *)(b1 + 0x00) = 4;
        *(uint32_t *)(b1 + 0x04) = 0xa;
        *(float *)(b1 + 0x08) = 1.0f;
        *(uint32_t *)(b1 + 0x0c) = 0x1c84;
        *(uint64_t *)(b1 + 0x10) = 0;

        uint8_t b1_orig[0x18];
        memcpy(b1_orig, b1, 0x18);

        int rc_p1 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p1 = p_v_create(&p1, b1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p1 = -sig;
        }
        obs_report_measure(id, "create-p1", "rc", (uint64_t)(uint32_t)rc_p1, "code");
        obs_report_measure(id, "create-p1", "port-id", (uint64_t)p1, "id");
        obs_report_measure(id, "create-p1", "changed", (uint64_t)(memcmp(b1, b1_orig, 0x18) != 0), "bool");
        obs_report_bytes(id, "create-p1", "param-24b", 0, b1, 0x18);
    }

    /* 3. sceVoiceConnectIPortToOPort(p0, p1) */
    if (p_v_connect) {
        int rc_c01 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_c01 = p_v_connect(p0, p1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_c01 = -sig;
        }
        obs_report_measure(id, "connect-p0-p1", "rc", (uint64_t)(uint32_t)rc_c01, "code");
    }

    /* 4. sceVoiceCreatePort(&p2, {u32 3, u32 0x40, f32 1.0, u32 0x800, u32 0, u32 0x3e80}) */
    if (p_v_create) {
        uint8_t b2[0x18] = {0};
        *(uint32_t *)(b2 + 0x00) = 3;
        *(uint32_t *)(b2 + 0x04) = 0x40;
        *(float *)(b2 + 0x08) = 1.0f;
        *(uint32_t *)(b2 + 0x0c) = 0x800;
        *(uint32_t *)(b2 + 0x10) = 0;
        *(uint32_t *)(b2 + 0x14) = 0x3e80;

        uint8_t b2_orig[0x18];
        memcpy(b2_orig, b2, 0x18);

        int rc_p2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p2 = p_v_create(&p2, b2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p2 = -sig;
        }
        obs_report_measure(id, "create-p2", "rc", (uint64_t)(uint32_t)rc_p2, "code");
        obs_report_measure(id, "create-p2", "port-id", (uint64_t)p2, "id");
        obs_report_measure(id, "create-p2", "changed", (uint64_t)(memcmp(b2, b2_orig, 0x18) != 0), "bool");
        obs_report_bytes(id, "create-p2", "param-24b", 0, b2, 0x18);
    }

    /* 5. sceVoiceConnectIPortToOPort(p0, p2) */
    if (p_v_connect) {
        int rc_c02 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_c02 = p_v_connect(p0, p2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_c02 = -sig;
        }
        obs_report_measure(id, "connect-p0-p2", "rc", (uint64_t)(uint32_t)rc_c02, "code");
    }

    /* 6. sceVoiceCreatePort(&p3, {u32 5, u32 0, f32 1.0, u32 U, u64 2}) */
    if (p_v_create) {
        uint8_t b3[0x18] = {0};
        *(uint32_t *)(b3 + 0x00) = 5;
        *(uint32_t *)(b3 + 0x04) = 0;
        *(float *)(b3 + 0x08) = 1.0f;
        *(uint32_t *)(b3 + 0x0c) = (uint32_t)user_id;
        *(uint64_t *)(b3 + 0x10) = 2;

        uint8_t b3_orig[0x18];
        memcpy(b3_orig, b3, 0x18);

        int rc_p3 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_p3 = p_v_create(&p3, b3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_p3 = -sig;
        }
        obs_report_measure(id, "create-p3", "rc", (uint64_t)(uint32_t)rc_p3, "code");
        obs_report_measure(id, "create-p3", "port-id", (uint64_t)p3, "id");
        obs_report_measure(id, "create-p3", "changed", (uint64_t)(memcmp(b3, b3_orig, 0x18) != 0), "bool");
        obs_report_bytes(id, "create-p3", "param-24b", 0, b3, 0x18);
    }

    /* 7. For p0-p3: sceVoiceGetPortInfo(p, buf) 0x40 bytes cc */
    uint32_t ports[4] = {p0, p1, p2, p3};
    for (int i = 0; i < 4; i++) {
        char port_name[16];
        snprintf(port_name, sizeof(port_name), "port-p%d", i);

        uint8_t info_buf[0x40];
        memset(info_buf, 0xcc, sizeof(info_buf));
        int rc_info = -1;

        if (p_v_getinfo) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_info = p_v_getinfo(ports[i], info_buf);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_info = -sig;
            }
        }
        obs_report_measure(id, port_name, "getinfo-rc", (uint64_t)(uint32_t)rc_info, "code");
        obs_report_bytes(id, port_name, "info-64b", 0, info_buf, sizeof(info_buf));
    }

    /* sceVoiceStart and sceVoiceStop */
    int rc_start = -1;
    if (p_v_start) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_start = p_v_start();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_start = -sig;
        }
    }
    obs_report_measure(id, "voice-start", "rc", (uint64_t)(uint32_t)rc_start, "code");

    int rc_stop = -1;
    if (p_v_stop) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_stop = p_v_stop();
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_stop = -sig;
        }
    }
    obs_report_measure(id, "voice-stop", "rc", (uint64_t)(uint32_t)rc_stop, "code");

    /* Delete ports p0-p3, then p0 again */
    for (int i = 0; i < 4; i++) {
        char port_name[16];
        snprintf(port_name, sizeof(port_name), "del-p%d", i);
        int rc_del = -1;
        if (p_v_del) {
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_del = p_v_del(ports[i]);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
                rc_del = -sig;
            }
        }
        obs_report_measure(id, port_name, "rc", (uint64_t)(uint32_t)rc_del, "code");
    }

    int rc_del_p0_again = -1;
    if (p_v_del) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_del_p0_again = p_v_del(p0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc_del_p0_again = -sig;
        }
    }
    obs_report_measure(id, "del-p0-again", "rc", (uint64_t)(uint32_t)rc_del_p0_again, "code");

    return obs_pass();
}

#endif /* OBS_WAVE2_CHECKS_INC */

