/*
 * Extended input: keyboard and mouse presence, entry-point layout capture, and
 * out-param buffers.
 *
 * # Why this section exists
 *
 * The SDK provides input bindings for keyboard and mouse (oops/keyboard.h,
 * oops/mouse.h), but their data records (documented at 96 bytes for keyboard, 40 bytes
 * for mouse) remain unconfirmed on hardware. This section performs the presence census
 * for libSceKeyboard and libSceMouse, captures 256-byte function prologues, and records
 * out-param write extents.
 */

#include "oops/freestd.h"
#include "oops/krw.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

static int32_t inputext_initial_user(void) {
    int32_t user = -1;
    const void *fn_get = (const void *)&sceUserServiceGetInitialUser;
    const void *fn_init = (const void *)&sceUserServiceInitialize;
    if (!obs_address_is_callable(fn_get)) {
        int h = obs_module_open("libSceUserService");
        if (h >= 0) {
            fn_get = obs_module_symbol(h, "sceUserServiceGetInitialUser");
            fn_init = obs_module_symbol(h, "sceUserServiceInitialize");
        }
    }
    if (obs_address_is_callable(fn_get)) {
        int (*get_user)(int32_t *) = (int (*)(int32_t *))fn_get;
        if (get_user(&user) != 0 && obs_address_is_callable(fn_init)) {
            ((int (*)(void *))fn_init)(NULL);
            (void)get_user(&user);
        }
    }
    if (user < 0) {
        user = 0xFF; /* Fallback to system user (SCE_USER_SERVICE_USER_ID_SYSTEM) */
    }
    return user;
}

/* Resolve a peripheral symbol via direct import, obs_module_symbol, or kernel dispatch
 * table. */
static const void *inputext_sym(const char *name, const void *direct) {
    if (obs_address_is_callable(direct)) {
        return direct;
    }
    int handle = obs_module_open(strncmp(name, "sceMouse", 8) == 0 ? "libSceMouse"
                                                                   : "libSceKeyboard");
    const void *p = obs_module_symbol(handle, name);
    if (obs_address_is_callable(p)) {
        return p;
    }
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t kaddr = krw_dynlib_resolve_any(pid, name);
        if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
            return (const void *)kaddr;
        }
    }
#endif
    return NULL;
}

static const char *const keyboard_symbols[] = {
    "sceKeyboardInit",
    "sceKeyboardOpen",
    "sceKeyboardClose",
    "sceKeyboardReadState",
};

static const char *const mouse_symbols[] = {
    "sceMouseInit",
    "sceMouseOpen",
    "sceMouseClose",
    "sceMouseRead",
};

/* Presence census and prologue dump for libSceKeyboard */
static obs_result check_keyboard_presence(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceKeyboard");
    int has_direct = obs_address_is_callable((const void *)&sceKeyboardInit) ||
                     obs_address_is_callable((const void *)&sceKeyboardOpen) ||
                     obs_address_is_callable((const void *)&sceKeyboardClose) ||
                     obs_address_is_callable((const void *)&sceKeyboardReadState);
    if (handle < 0 && !has_direct) {
        return obs_fail("libSceKeyboard did not load in this context");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(keyboard_symbols); i++) {
        const char *name = keyboard_symbols[i];
        const void *direct = NULL;
        if (obs_strcmp(name, "sceKeyboardInit") == 0) {
            direct = (const void *)&sceKeyboardInit;
        } else if (obs_strcmp(name, "sceKeyboardOpen") == 0) {
            direct = (const void *)&sceKeyboardOpen;
        } else if (obs_strcmp(name, "sceKeyboardClose") == 0) {
            direct = (const void *)&sceKeyboardClose;
        } else if (obs_strcmp(name, "sceKeyboardReadState") == 0) {
            direct = (const void *)&sceKeyboardReadState;
        }

        const void *addr = NULL;
        if (handle >= 0) {
            addr = obs_module_symbol(handle, name);
        }
        if (addr == NULL && obs_address_is_callable(direct)) {
            addr = direct;
        }
#if !defined(OBSCENE_HOST_BUILD)
        if (addr == NULL && krw_is_ready()) {
            pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
            uintptr_t kaddr = krw_dynlib_resolve_any(pid, name);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                addr = (const void *)kaddr;
            }
        }
#endif
        if (addr != NULL) {
            resolved++;
            obs_report_measure("101-input-ext/keyboard-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            /* Readable, not merely callable: library text is execute-only (xotext) on
             * hardware, so dump the prologue only where it can be read (emulators),
             * never crashing on a console. (D325) */
            int readable = obs_linkmap_readable((uintptr_t)addr);
            obs_report_measure("101-input-ext/keyboard-symbols", name,
                               readable ? "readable-text" : "xotext",
                               (uint64_t)readable, "flag");
            if (readable) {
                obs_report_buffer("101-input-ext/kbd-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("101-input-ext/keyboard-symbols", name, "unresolved", 0,
                               "status");
        }
    }

    if (resolved == OBS_COUNT(keyboard_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceKeyboard symbols resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("libSceKeyboard loaded but no symbols resolved");
}

/* Presence census and prologue dump for libSceMouse */
static obs_result check_mouse_presence(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceMouse");
    int has_direct = obs_address_is_callable((const void *)&sceMouseInit) ||
                     obs_address_is_callable((const void *)&sceMouseOpen) ||
                     obs_address_is_callable((const void *)&sceMouseClose) ||
                     obs_address_is_callable((const void *)&sceMouseRead);
    if (handle < 0 && !has_direct) {
        return obs_fail("libSceMouse did not load in this context");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(mouse_symbols); i++) {
        const char *name = mouse_symbols[i];
        const void *direct = NULL;
        if (obs_strcmp(name, "sceMouseInit") == 0) {
            direct = (const void *)&sceMouseInit;
        } else if (obs_strcmp(name, "sceMouseOpen") == 0) {
            direct = (const void *)&sceMouseOpen;
        } else if (obs_strcmp(name, "sceMouseClose") == 0) {
            direct = (const void *)&sceMouseClose;
        } else if (obs_strcmp(name, "sceMouseRead") == 0) {
            direct = (const void *)&sceMouseRead;
        }

        const void *addr = NULL;
        if (handle >= 0) {
            addr = obs_module_symbol(handle, name);
        }
        if (addr == NULL && obs_address_is_callable(direct)) {
            addr = direct;
        }
#if !defined(OBSCENE_HOST_BUILD)
        if (addr == NULL && krw_is_ready()) {
            pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
            uintptr_t kaddr = krw_dynlib_resolve_any(pid, name);
            if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
                addr = (const void *)kaddr;
            }
        }
#endif
        if (addr != NULL) {
            resolved++;
            obs_report_measure("101-input-ext/mouse-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            int readable = obs_linkmap_readable((uintptr_t)addr);
            obs_report_measure("101-input-ext/mouse-symbols", name,
                               readable ? "readable-text" : "xotext",
                               (uint64_t)readable, "flag");
            if (readable) {
                obs_report_buffer("101-input-ext/mouse-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("101-input-ext/mouse-symbols", name, "unresolved", 0,
                               "status");
        }
    }

    if (resolved == OBS_COUNT(mouse_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceMouse symbols resolved",
                                 (uint64_t)resolved);
    }
    return obs_fail("libSceMouse loaded but no symbols resolved");
}

/* Out-param write extent capture for sceKeyboardReadState */
static obs_result check_keyboard_read_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    const void *p_init =
        inputext_sym("sceKeyboardInit", (const void *)&sceKeyboardInit);
    const void *p_open =
        inputext_sym("sceKeyboardOpen", (const void *)&sceKeyboardOpen);
    const void *p_close =
        inputext_sym("sceKeyboardClose", (const void *)&sceKeyboardClose);
    const void *p_read =
        inputext_sym("sceKeyboardReadState", (const void *)&sceKeyboardReadState);

    if (p_read == NULL) {
        return obs_skip("sceKeyboardReadState did not resolve");
    }

    int is_callable = obs_address_is_callable(p_read);
    int is_readable = obs_linkmap_readable((uintptr_t)p_read);
    obs_report_measure("101-input-ext/kbd-read", "sceKeyboardReadState", "vaddr",
                       (uint64_t)(uintptr_t)p_read, "vaddr");
    obs_report_measure("101-input-ext/kbd-read", "sceKeyboardReadState",
                       is_readable ? "readable-text" : "xotext", (uint64_t)is_readable,
                       "flag");
    obs_report_measure("101-input-ext/kbd-read", "sceKeyboardReadState",
                       is_callable ? "callable" : "not-callable", (uint64_t)is_callable,
                       "flag");

    if (!is_callable) {
        return obs_skip("sceKeyboardReadState is not callable");
    }

    int32_t user = inputext_initial_user();
    if (user < 0) {
        return obs_skip("no initial user for keyboard");
    }

    int (*fn_init)(void) = (int (*)(void))p_init;
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))p_open;
    int (*fn_close)(int) = (int (*)(int))p_close;
    int (*fn_read)(int, void *) = (int (*)(int, void *))p_read;

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }

    int handle = -1;
    if (fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }
    obs_report_measure("101-input-ext/kbd-read", "sceKeyboardOpen",
                       handle >= 0 ? "handle" : "refused", (uint64_t)(uint32_t)handle,
                       "handle");

#define OBS_KBD_BUF_SIZE 4096u
    static uint8_t buf[OBS_KBD_BUF_SIZE];
    static uint8_t before[OBS_KBD_BUF_SIZE];
    for (size_t i = 0; i < OBS_KBD_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_read(handle >= 0 ? handle : 0, buf);
    obs_report_measure("101-input-ext/kbd-read", "sceKeyboardReadState", "rc",
                       (uint64_t)(uint32_t)rc, "code");

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_KBD_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (handle >= 0 && fn_close != NULL &&
        obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }

    if (written > 0) {
        obs_report_written("101-input-ext/kbd-read", "sceKeyboardReadState",
                           "out-param", before, buf, OBS_KBD_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("101-input-ext/kbd-read", "sceKeyboardReadState", "untouched",
                       before, buf, OBS_KBD_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("read returned error and wrote nothing",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_KBD_BUF_SIZE
}

/* Out-param write extent capture for sceMouseRead */
static obs_result check_mouse_read_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    const void *p_init = inputext_sym("sceMouseInit", (const void *)&sceMouseInit);
    const void *p_open = inputext_sym("sceMouseOpen", (const void *)&sceMouseOpen);
    const void *p_close = inputext_sym("sceMouseClose", (const void *)&sceMouseClose);
    const void *p_read = inputext_sym("sceMouseRead", (const void *)&sceMouseRead);

    if (p_read == NULL) {
        return obs_skip("sceMouseRead did not resolve");
    }

    int is_callable = obs_address_is_callable(p_read);
    int is_readable = obs_linkmap_readable((uintptr_t)p_read);
    obs_report_measure("101-input-ext/mouse-read", "sceMouseRead", "vaddr",
                       (uint64_t)(uintptr_t)p_read, "vaddr");
    obs_report_measure("101-input-ext/mouse-read", "sceMouseRead",
                       is_readable ? "readable-text" : "xotext", (uint64_t)is_readable,
                       "flag");
    obs_report_measure("101-input-ext/mouse-read", "sceMouseRead",
                       is_callable ? "callable" : "not-callable", (uint64_t)is_callable,
                       "flag");

    if (!is_callable) {
        return obs_skip("sceMouseRead is not callable");
    }

    int32_t user = inputext_initial_user();
    if (user < 0) {
        return obs_skip("no initial user for mouse");
    }

    if (obs_get_payload_args() != NULL) {
        obs_report_measure("101-input-ext/mouse-read", "sceMouseInit", "unlinked-stub",
                           (uint64_t)(uintptr_t)p_init, "vaddr");
        return obs_skip("sceMouse symbols in libkernel are unlinked stubs in payload "
                        "mode (calling triggers signo 0xa0020101)");
    }

    int (*fn_init)(void) = (int (*)(void))p_init;
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))p_open;
    int (*fn_close)(int) = (int (*)(int))p_close;
    int (*fn_read)(int, void *, int) = (int (*)(int, void *, int))p_read;

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }

    int handle = -1;
    if (fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }
    obs_report_measure("101-input-ext/mouse-read", "sceMouseOpen",
                       handle >= 0 ? "handle" : "refused", (uint64_t)(uint32_t)handle,
                       "handle");

#define OBS_MOUSE_BUF_SIZE 4096u
    static uint8_t buf[OBS_MOUSE_BUF_SIZE];
    static uint8_t before[OBS_MOUSE_BUF_SIZE];
    for (size_t i = 0; i < OBS_MOUSE_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_read(handle >= 0 ? handle : 0, buf, 1);
    obs_report_measure("101-input-ext/mouse-read", "sceMouseRead", "rc",
                       (uint64_t)(uint32_t)rc, "code");

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_MOUSE_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (handle >= 0 && fn_close != NULL &&
        obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }

    if (written > 0) {
        obs_report_written("101-input-ext/mouse-read", "sceMouseRead", "out-param",
                           before, buf, OBS_MOUSE_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("101-input-ext/mouse-read", "sceMouseRead", "untouched", before,
                       buf, OBS_MOUSE_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("read returned error and wrote nothing",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_MOUSE_BUF_SIZE
}

/* ---- keyboard and mouse behavioural probes, called directly (D328)
 * ----------------------
 *
 * These call the read functions as imports so they run in every leg, and report PENDING
 * - not skip, not fault - when no peripheral is attached or the key/movement they watch
 * for never arrives. A re-run with the input answers the same probe. */

static uint8_t s_ie_buf[4096];
static uint8_t s_ie_before[4096];

static void obs_ie_fill(void) {
    for (size_t i = 0; i < sizeof s_ie_buf; i++) {
        s_ie_buf[i] = 0xC7u;
        s_ie_before[i] = 0xC7u;
    }
}

static unsigned int obs_ie_extent(void) {
    unsigned int extent = 0;
    for (size_t i = 0; i < sizeof s_ie_buf; i++) {
        if (s_ie_buf[i] != s_ie_before[i]) {
            extent = (unsigned int)(i + 1u);
        }
    }
    return extent;
}

static void obs_ie_nap(void) {
    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        sceKernelUsleep(50000);
    }
}

/* A keyboard with a key held: read once idle, then sample while shift and a letter are
 * held, and dump both. The two records locate the modifier and key-code fields within
 * the 96-byte layout; a run with no keyboard, or with no key pressed, is PENDING. */
static obs_result check_keyboard_held(void) {
    const void *p_read =
        inputext_sym("sceKeyboardReadState", (const void *)&sceKeyboardReadState);
    const void *p_init =
        inputext_sym("sceKeyboardInit", (const void *)&sceKeyboardInit);
    const void *p_open =
        inputext_sym("sceKeyboardOpen", (const void *)&sceKeyboardOpen);
    const void *p_close =
        inputext_sym("sceKeyboardClose", (const void *)&sceKeyboardClose);

    if (!obs_address_is_callable(p_read)) {
        return obs_skip("sceKeyboardReadState is not callable");
    }
    int (*fn_read)(int, void *) = (int (*)(int, void *))p_read;
    int (*fn_init)(void) = (int (*)(void))p_init;
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))p_open;
    int (*fn_close)(int) = (int (*)(int))p_close;

    int32_t user = inputext_initial_user();
    if (user < 0) {
        return obs_skip("no initial user for keyboard");
    }
    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }
    int handle = -1;
    if (fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }
    if (handle < 0) {
        return obs_pending("no keyboard attached: connect one and re-run");
    }
    /* Idle snapshot. */
    obs_ie_fill();
    (void)fn_read(handle, s_ie_buf);
    unsigned int idle_extent = obs_ie_extent();
    static uint8_t idle[96];
    for (size_t i = 0; i < 96; i++) {
        idle[i] = s_ie_buf[i];
    }
    obs_report_buffer("101-input-ext/keyboard-held", "sceKeyboardReadState", "idle",
                      idle, 96);

    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "hold-shift-and-a-letter-now", 4, "prompt-seconds");
    unsigned int best_diff = 0;
    static uint8_t held[96];
    uint32_t modifiers_seen_or = 0;
    for (int i = 0; i < 80; i++) {
        obs_ie_fill();
        if (fn_read(handle, s_ie_buf) == 0 || obs_ie_extent() >= 16u) {
            unsigned int diff = 0;
            for (size_t b = 0; b < 96; b++) {
                if (s_ie_buf[b] != idle[b]) {
                    diff++;
                }
            }
            uint32_t sample_mod = *(const uint32_t *)&s_ie_buf[0x1c];
            modifiers_seen_or |= sample_mod;
            if (diff > best_diff) {
                best_diff = diff;
                for (size_t b = 0; b < 96; b++) {
                    held[b] = s_ie_buf[b];
                }
            }
        }
        obs_ie_nap();
    }
    if (fn_close != NULL && obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }
    if (best_diff == 0) {
        return obs_pending("keyboard open, but no key was held in the window");
    }
    obs_report_buffer("101-input-ext/keyboard-held", "sceKeyboardReadState", "held",
                      held, 96);
    (void)idle_extent;

    /* REQ-20260922T1905Z-9c31: confirm modifiers at 0x1c, length at 0x14, connected/intercepted */
    uint32_t idle_mod = *(const uint32_t *)&idle[0x1c];
    uint32_t held_mod = *(const uint32_t *)&held[0x1c];
    int32_t held_len = *(const int32_t *)&held[0x14];
    uint16_t key0 = *(const uint16_t *)&held[0x20];
    uint8_t connected = held[0x10];
    uint8_t intercepted = held[0x08];

    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "idle-modifiers", (uint64_t)idle_mod, "hex");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "held-modifiers", (uint64_t)held_mod, "hex");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "modifiers-or", (uint64_t)modifiers_seen_or, "hex");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "mod-shift", (uint64_t)((held_mod & 0x22u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "mod-ctrl", (uint64_t)((held_mod & 0x11u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "mod-alt", (uint64_t)((held_mod & 0x44u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "mod-gui", (uint64_t)((held_mod & 0x88u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "length", (uint64_t)(uint32_t)held_len, "count");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "connected", (uint64_t)connected, "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "intercepted", (uint64_t)intercepted, "bool");
    obs_report_measure("101-input-ext/keyboard-held", "sceKeyboardReadState",
                       "keycode0", (uint64_t)key0, "hex");

    return obs_pass_value((uint64_t)best_diff);
}

/* A mouse attached and moving: one record for the extent, four for the stride, and a
 * sampled button word. No mouse, or no movement, is PENDING. */
static obs_result check_mouse_moving(void) {
    const void *p_read = inputext_sym("sceMouseRead", (const void *)&sceMouseRead);
    const void *p_init = inputext_sym("sceMouseInit", (const void *)&sceMouseInit);
    const void *p_open = inputext_sym("sceMouseOpen", (const void *)&sceMouseOpen);
    const void *p_close = inputext_sym("sceMouseClose", (const void *)&sceMouseClose);

    if (!obs_address_is_callable(p_read)) {
        return obs_skip("sceMouseRead is not callable");
    }
    int (*fn_read)(int, void *, int) = (int (*)(int, void *, int))p_read;
    int (*fn_init)(void) = (int (*)(void))p_init;
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))p_open;
    int (*fn_close)(int) = (int (*)(int))p_close;

    int32_t user = inputext_initial_user();
    if (user < 0) {
        return obs_skip("no initial user for mouse");
    }
    if (obs_get_payload_args() != NULL) {
        return obs_skip(
            "sceMouse symbols in libkernel are unlinked stubs in payload mode");
    }
    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }
    int handle = -1;
    if (fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }
    if (handle < 0) {
        return obs_pending("no mouse attached: connect one and re-run");
    }
    obs_ie_fill();
    (void)fn_read(handle, s_ie_buf, 1);
    unsigned int extent_one = obs_ie_extent();
    obs_report_written("101-input-ext/mouse-moving", "sceMouseRead",
                       extent_one > 0 ? "one-record" : "untouched", s_ie_before,
                       s_ie_buf, sizeof s_ie_buf);

    obs_ie_fill();
    (void)fn_read(handle, s_ie_buf, 4);
    unsigned int extent_four = obs_ie_extent();
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "extent-one",
                       (uint64_t)extent_one, "bytes");
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "extent-four",
                       (uint64_t)extent_four, "bytes");

    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead",
                       "move-and-hold-a-button-now", 3, "prompt-seconds");
    uint32_t button_or = 0;
    static uint8_t mouse_sample[64];
    int sample_captured = 0;
    for (int i = 0; i < 60; i++) {
        obs_ie_fill();
        if (fn_read(handle, s_ie_buf, 1) >= 0 || obs_ie_extent() >= 4u) {
            /* Mouse record: buttons in the first word (OpenOrbis). OR across the
             * window. */
            uint32_t bword = (uint32_t)s_ie_buf[0] | ((uint32_t)s_ie_buf[1] << 8) |
                             ((uint32_t)s_ie_buf[2] << 16) | ((uint32_t)s_ie_buf[3] << 24);
            button_or |= bword;
            if (!sample_captured && (obs_ie_extent() >= 16u || bword != 0)) {
                for (size_t b = 0; b < sizeof mouse_sample; b++) {
                    mouse_sample[b] = s_ie_buf[b];
                }
                sample_captured = 1;
            }
        }
        obs_ie_nap();
    }
    if (fn_close != NULL && obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }
    if (extent_one == 0 && !sample_captured) {
        return obs_pending("mouse open, but no record was read: move it and re-run");
    }
    uint32_t rec_len = extent_one > 0 ? extent_one : (extent_four > 0 ? extent_four / 4 : 40u);
    uint32_t stride = (extent_four > extent_one) ? (extent_four - extent_one) / 3u : rec_len;
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "record-stride",
                       (uint64_t)stride, "bytes");
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "button-or",
                       (uint64_t)button_or, "bits");
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "button-left",
                       (uint64_t)((button_or & 1u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "button-right",
                       (uint64_t)((button_or & 2u) ? 1 : 0), "bool");
    obs_report_measure("101-input-ext/mouse-moving", "sceMouseRead", "button-middle",
                       (uint64_t)((button_or & 4u) ? 1 : 0), "bool");
    if (sample_captured || extent_one > 0) {
        obs_report_buffer("101-input-ext/mouse-moving", "sceMouseRead", "record",
                          sample_captured ? mouse_sample : s_ie_buf,
                          rec_len > 64 ? 64 : rec_len);
    }
    return obs_pass_value((uint64_t)rec_len);
}

/* Reachability from a payload: try to bring libSceKeyboard and libSceMouse up through
 * sysmodule and see whether their read symbols resolve afterwards. Today they resolve
 * only in the app context; the eboot answer is the finding, and it is different per leg
 * by design. */
static obs_result check_periph_reachability(void) {
    unsigned int reachable = 0;
    /* Keyboard. */
    int kbd = obs_module_open("libSceKeyboard");
    const void *kbd_read =
        kbd >= 0 ? obs_module_symbol(kbd, "sceKeyboardReadState") : NULL;
#if !defined(OBSCENE_HOST_BUILD)
    if (kbd_read == NULL && krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t addr = krw_dynlib_resolve_any(pid, "sceKeyboardReadState");
        if (addr >= 0x10000UL && obs_address_is_callable((const void *)addr)) {
            kbd_read = (const void *)addr;
        }
    }
#endif
    obs_report_measure("101-input-ext/reachability", "libSceKeyboard",
                       kbd >= 0 ? "loaded" : "unavailable", (uint64_t)(uint32_t)kbd,
                       "handle");
    if (obs_address_is_callable(kbd_read)) {
        reachable++;
        obs_report_measure("101-input-ext/reachability", "sceKeyboardReadState",
                           "resolved", (uint64_t)(uintptr_t)kbd_read, "vaddr");
    }
    /* Mouse. */
    int mouse = obs_module_open("libSceMouse");
    const void *mouse_read =
        mouse >= 0 ? obs_module_symbol(mouse, "sceMouseRead") : NULL;
#if !defined(OBSCENE_HOST_BUILD)
    if (mouse_read == NULL && krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t addr = krw_dynlib_resolve_any(pid, "sceMouseRead");
        if (addr >= 0x10000UL && obs_address_is_callable((const void *)addr)) {
            mouse_read = (const void *)addr;
        }
    }
#endif
    obs_report_measure("101-input-ext/reachability", "libSceMouse",
                       mouse >= 0 ? "loaded" : "unavailable", (uint64_t)(uint32_t)mouse,
                       "handle");
    if (obs_address_is_callable(mouse_read)) {
        reachable++;
        obs_report_measure("101-input-ext/reachability", "sceMouseRead", "resolved",
                           (uint64_t)(uintptr_t)mouse_read, "vaddr");
    }
    if (reachable == 0) {
        return obs_partial(
            "neither libSceKeyboard nor libSceMouse could be reached here");
    }
    return obs_pass_value((uint64_t)reachable);
}

static const obs_check input_ext_checks[] = {
    {"101-input-ext/keyboard-presence", "libSceKeyboard", "sceKeyboardReadState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_keyboard_presence,
     check_keyboard_presence, OBS_FROM_ASSUMED},
    {"101-input-ext/mouse-presence", "libSceMouse", "sceMouseRead", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_mouse_presence, check_mouse_presence,
     OBS_FROM_ASSUMED},
    {"101-input-ext/kbd-read", "libSceKeyboard", "sceKeyboardReadState", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_keyboard_read_outparam,
     check_keyboard_read_outparam, OBS_FROM_ASSUMED},
    {"101-input-ext/mouse-read", "libSceMouse", "sceMouseRead", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_mouse_read_outparam, check_mouse_read_outparam,
     OBS_FROM_ASSUMED},
    {"101-input-ext/keyboard-held", "libSceKeyboard", "sceKeyboardReadState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_keyboard_held, check_keyboard_held,
     OBS_FROM_ASSUMED},
    {"101-input-ext/mouse-moving", "libSceMouse", "sceMouseRead", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_mouse_moving, check_mouse_moving,
     OBS_FROM_ASSUMED},
    {"101-input-ext/reachability", "libSceKeyboard", "sceKeyboardReadState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_periph_reachability,
     check_periph_reachability, OBS_FROM_ASSUMED},
};

const obs_section obs_section_input_ext = {
    "101-input-ext",
    "Keyboard and mouse input extensions",
    "Resolves libSceKeyboard and libSceMouse symbols, dumps entry-point machine code "
    "prologues for layout extraction, and tests out-param state read buffers.",
    input_ext_checks,
    OBS_COUNT(input_ext_checks),
};
