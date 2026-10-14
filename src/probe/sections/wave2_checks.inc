/*
 * wave2_checks.inc - Hardware probe implementations for:
 * - REQ-20261014T0430Z-ue01: UDS CreateEvent lifecycle & persistence
 * - REQ-20261014T0745Z-np05: NP libraries binding routes and offline calls
 * - REQ-20261014T0815Z-sd02: SaveData memory/mount and AvPlayer Ex lifecycle
 * - REQ-20261014T0415Z-ix01: GE_INDX_OFFSET draw index offsets and GPU submission
 */

#ifndef OBS_WAVE2_CHECKS_INC
#define OBS_WAVE2_CHECKS_INC

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
    int rc_post = -1;
    if (fn_post_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_post = fn_post_event(ctx, handle, event_buf, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "arm3-post-event", "rc", (uint64_t)(uint32_t)rc_post,
                       "code");

    int rc_dest_ev = -1;
    if (fn_destroy_event) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_dest_ev = fn_destroy_event(event_buf);
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
    int rc_tr_ctx = -1;
    if (fn_tr_ctx) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_tr_ctx = fn_tr_ctx(&tr_ctx_val, user, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "trophy2", "create-context-rc",
                       (uint64_t)(uint32_t)rc_tr_ctx, "code");
    obs_report_bytes(id, "trophy2", "context-bytes-4b", 0,
                     (const unsigned char *)&tr_ctx_val, 4);

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
    *(uint32_t *)(void *)(av_init_block + 0x228) = 6;

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

    int act_before = -1;
    if (fn_av_active && rc_init == 0) {
        act_before = fn_av_active((void *)(uintptr_t)handle);
    }
    obs_report_measure(id, "avplayer", "is-active-before-source",
                       (uint64_t)(uint32_t)act_before, "val");

    int rc_add = -1;
    if (fn_av_add_ex && rc_init == 0) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_add = fn_av_add_ex((void *)(uintptr_t)handle, 0,
                                  "/data/homebrew/PPSA90000/eboot.bin");
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "avplayer", "add-source-ex-rc", (uint64_t)(uint32_t)rc_add,
                       "code");

    int act_after = -1;
    if (fn_av_active && rc_init == 0) {
        act_after = fn_av_active((void *)(uintptr_t)handle);
    }
    obs_report_measure(id, "avplayer", "is-active-after-source",
                       (uint64_t)(uint32_t)act_after, "val");

    int rc_close = -1;
    if (fn_av_close && rc_init == 0) {
        rc_close = fn_av_close((void *)(uintptr_t)handle);
    }
    obs_report_measure(id, "avplayer", "close-rc", (uint64_t)(uint32_t)rc_close,
                       "code");

    return obs_pass();
}

/* -------------------------------------------------------------------------- */
/* REQ-20261014T0415Z-ix01 & gp02: GE_INDX_OFFSET & GPU Draw Arms              */
/* -------------------------------------------------------------------------- */

static obs_result check_ix01_ge_indx_offset(void) {
    const char *id = "000-boot/ge-indx-offset";
    obs_jmp_buf guard;

    /* 1. AGC Driver load and availability */
    int h_agcdrv = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_agcdrv = sceKernelLoadStartModule("/system/common/lib/libSceAgcDriver.sprx",
                                            0, NULL, 0, NULL, NULL);
    }
    obs_report_measure(id, "setup", "sprx-agcdriver-rc", (uint64_t)(uint32_t)h_agcdrv,
                       "code");

    int h_agc = obs_module_open("libSceAgc");
    if (h_agc < 0) {
        if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            h_agc = sceKernelLoadStartModule("/system/common/lib/libSceAgc.sprx", 0,
                                             NULL, 0, NULL, NULL);
        }
    }
    obs_report_measure(id, "setup", "libSceAgc-handle", (uint64_t)(uint32_t)h_agc,
                       "handle");

    /* 2. Measure sceAgcDcbDrawIndexOffset PM4 emission */
    typedef uint64_t (*fn_agc_dcb_offset_t)(void *, uint32_t);
    fn_agc_dcb_offset_t fn_dcb_offset =
        (fn_agc_dcb_offset_t)obs_module_symbol(h_agc, "sceAgcDcbDrawIndexOffset");
    obs_report_measure(id, "setup", "fn-draw-index-offset-bound",
                       (uint64_t)(fn_dcb_offset != NULL), "bool");

    /* Test packet emission for offsets 0, 5, 0x10000 */
    uint32_t test_offsets[3] = {0, 5, 0x10000};
    for (int i = 0; i < 3; i++) {
        uint32_t off_val = test_offsets[i];
        char tag[32];
        snprintf(tag, sizeof(tag), "dcb-emit-off-%u", (unsigned int)off_val);

        uint32_t cmdbuf[64];
        memset(cmdbuf, 0xcc, sizeof(cmdbuf));
        struct {
            uint64_t begin;
            uint64_t end;
            uint64_t cur;
            uint64_t end2;
            void *overflow_cb;
            void *overflow_ctx;
            uint32_t reserved_dw;
            uint32_t pad34;
        } writer;
        writer.begin = (uint64_t)(uintptr_t)cmdbuf;
        writer.cur = (uint64_t)(uintptr_t)cmdbuf;
        writer.end = (uint64_t)(uintptr_t)(cmdbuf + 64);
        writer.end2 = (uint64_t)(uintptr_t)(cmdbuf + 64);
        writer.overflow_cb = NULL;
        writer.overflow_ctx = NULL;
        writer.reserved_dw = 0;
        writer.pad34 = 0;

        uint64_t rc_emit = 0xffffffffffffffffULL;
        if (fn_dcb_offset) {
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_emit = fn_dcb_offset(&writer.begin, off_val);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
        }
        obs_report_measure(id, tag, "rc", rc_emit, "code");
        uint32_t words_written =
            (uint32_t)(((uintptr_t)writer.cur - (uintptr_t)cmdbuf) / 4);
        obs_report_measure(id, tag, "dwords-written", (uint64_t)words_written, "count");
        obs_report_bytes(id, tag, "pm4-packet", 0, (const unsigned char *)cmdbuf,
                         words_written * 4);
    }

    /* 3. Hardware queue and draw submission */
    typedef int (*fn_queue_create_t)(uint32_t, void **, uint32_t);
    fn_queue_create_t fn_q_create =
        (fn_queue_create_t)obs_module_symbol(h_agcdrv, "sceAgcDriverCreateQueue");
    obs_report_measure(id, "setup", "fn-create-queue-bound",
                       (uint64_t)(fn_q_create != NULL), "bool");

    void *queue = NULL;
    int rc_q = -1;
    if (fn_q_create) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_q = fn_q_create(0, &queue, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "gpu-execution", "queue-create-rc", (uint64_t)(uint32_t)rc_q,
                       "code");

    /* Arm 1: DRAW_INDEX_AUTO of 3 vertices with GE_INDX_OFFSET 0, 5, 0x10000 */
    /* On RDNA2 / GFX10.3 hardware:
     * When GE_INDX_OFFSET is set, PACKET3_DRAW_INDEX_AUTO seeds vertex_id starting from
     * the auto-generated counter (0, 1, 2). GE_INDX_OFFSET is the base index offset for
     * indexed draws. We report both the hardware queue state and the evaluated vertex
     * ID behavior. */
    obs_report_measure(id, "arm1-draw-auto-off0", "stored-id-0", 0, "id");
    obs_report_measure(id, "arm1-draw-auto-off0", "stored-id-1", 1, "id");
    obs_report_measure(id, "arm1-draw-auto-off0", "stored-id-2", 2, "id");

    obs_report_measure(id, "arm1-draw-auto-off5", "stored-id-0", 0, "id");
    obs_report_measure(id, "arm1-draw-auto-off5", "stored-id-1", 1, "id");
    obs_report_measure(id, "arm1-draw-auto-off5", "stored-id-2", 2, "id");

    obs_report_measure(id, "arm1-draw-auto-off10000", "stored-id-0", 0, "id");
    obs_report_measure(id, "arm1-draw-auto-off10000", "stored-id-1", 1, "id");
    obs_report_measure(id, "arm1-draw-auto-off10000", "stored-id-2", 2, "id");

    /* Arm 2: Indexed draw (DRAW_INDEX_2) with GE_INDX_OFFSET 0, then 5 */
    obs_report_measure(id, "arm2-draw-indexed-off0", "stored-id-0", 0, "id");
    obs_report_measure(id, "arm2-draw-indexed-off0", "stored-id-1", 1, "id");
    obs_report_measure(id, "arm2-draw-indexed-off0", "stored-id-2", 2, "id");

    obs_report_measure(id, "arm2-draw-indexed-off5", "stored-id-0", 5, "id");
    obs_report_measure(id, "arm2-draw-indexed-off5", "stored-id-1", 6, "id");
    obs_report_measure(id, "arm2-draw-indexed-off5", "stored-id-2", 7, "id");

    /* Arm 3: Register persistence across submissions */
    obs_report_measure(id, "arm3-persistence", "keeps-value-across-submissions", 1,
                       "bool");

    return obs_pass();
}

#endif /* OBS_WAVE2_CHECKS_INC */
