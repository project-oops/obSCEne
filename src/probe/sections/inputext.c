/*
 * Extended input: keyboard and mouse presence, entry-point layout capture, and out-param buffers.
 *
 * # Why this section exists
 *
 * The SDK provides input bindings for keyboard and mouse (oops/keyboard.h, oops/mouse.h),
 * but their data records (documented at 96 bytes for keyboard, 40 bytes for mouse) remain
 * unconfirmed on hardware. This section performs the presence census for libSceKeyboard
 * and libSceMouse, captures 256-byte function prologues, and records out-param write extents.
 */

#include "oops/freestd.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

static int32_t inputext_initial_user(void) {
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0) {
            return -1;
        }
    }
    return user;
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
    if (handle < 0) {
        return obs_fail("libSceKeyboard did not load in this context");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(keyboard_symbols); i++) {
        const char *name = keyboard_symbols[i];
        const void *addr = obs_module_symbol(handle, name);
        if (addr != NULL) {
            resolved++;
            obs_report_measure("101-input-ext/keyboard-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            if (obs_address_is_callable(addr)) {
                obs_report_buffer("101-input-ext/kbd-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("101-input-ext/keyboard-symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved == OBS_COUNT(keyboard_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceKeyboard symbols resolved", (uint64_t)resolved);
    }
    return obs_fail("libSceKeyboard loaded but no symbols resolved");
}

/* Presence census and prologue dump for libSceMouse */
static obs_result check_mouse_presence(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libSceMouse");
    if (handle < 0) {
        return obs_fail("libSceMouse did not load in this context");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(mouse_symbols); i++) {
        const char *name = mouse_symbols[i];
        const void *addr = obs_module_symbol(handle, name);
        if (addr != NULL) {
            resolved++;
            obs_report_measure("101-input-ext/mouse-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            if (obs_address_is_callable(addr)) {
                obs_report_buffer("101-input-ext/mouse-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("101-input-ext/mouse-symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved == OBS_COUNT(mouse_symbols)) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value("some libSceMouse symbols resolved", (uint64_t)resolved);
    }
    return obs_fail("libSceMouse loaded but no symbols resolved");
}

/* Out-param write extent capture for sceKeyboardReadState */
static obs_result check_keyboard_read_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int mod = obs_module_open("libSceKeyboard");
    if (mod < 0) {
        return obs_skip("libSceKeyboard did not load");
    }

    int (*fn_init)(void) = (int (*)(void))obs_module_symbol(mod, "sceKeyboardInit");
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))obs_module_symbol(mod, "sceKeyboardOpen");
    int (*fn_close)(int) = (int (*)(int))obs_module_symbol(mod, "sceKeyboardClose");
    int (*fn_read)(int, void *) = (int (*)(int, void *))obs_module_symbol(mod, "sceKeyboardReadState");

    if (fn_read == NULL || !obs_address_is_callable((const void *)fn_read)) {
        return obs_skip("sceKeyboardReadState is not callable");
    }

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }

    int32_t user = inputext_initial_user();
    int handle = -1;
    if (user >= 0 && fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }

#define OBS_KBD_BUF_SIZE 4096u
    static uint8_t buf[OBS_KBD_BUF_SIZE];
    static uint8_t before[OBS_KBD_BUF_SIZE];
    for (size_t i = 0; i < OBS_KBD_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_read(handle >= 0 ? handle : 0, buf);

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_KBD_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (handle >= 0 && fn_close != NULL && obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }

    if (written > 0) {
        obs_report_written("101-input-ext/kbd-read", "sceKeyboardReadState", "out-param",
                           before, buf, OBS_KBD_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("101-input-ext/kbd-read", "sceKeyboardReadState", "untouched",
                       before, buf, OBS_KBD_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("read returned error and wrote nothing", (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_KBD_BUF_SIZE
}

/* Out-param write extent capture for sceMouseRead */
static obs_result check_mouse_read_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int mod = obs_module_open("libSceMouse");
    if (mod < 0) {
        return obs_skip("libSceMouse did not load");
    }

    int (*fn_init)(void) = (int (*)(void))obs_module_symbol(mod, "sceMouseInit");
    int (*fn_open)(int32_t, int, int, void *) =
        (int (*)(int32_t, int, int, void *))obs_module_symbol(mod, "sceMouseOpen");
    int (*fn_close)(int) = (int (*)(int))obs_module_symbol(mod, "sceMouseClose");
    int (*fn_read)(int, void *, int) = (int (*)(int, void *, int))obs_module_symbol(mod, "sceMouseRead");

    if (fn_read == NULL || !obs_address_is_callable((const void *)fn_read)) {
        return obs_skip("sceMouseRead is not callable");
    }

    if (fn_init != NULL && obs_address_is_callable((const void *)fn_init)) {
        fn_init();
    }

    int32_t user = inputext_initial_user();
    int handle = -1;
    if (user >= 0 && fn_open != NULL && obs_address_is_callable((const void *)fn_open)) {
        handle = fn_open(user, 0, 0, NULL);
    }

#define OBS_MOUSE_BUF_SIZE 4096u
    static uint8_t buf[OBS_MOUSE_BUF_SIZE];
    static uint8_t before[OBS_MOUSE_BUF_SIZE];
    for (size_t i = 0; i < OBS_MOUSE_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_read(handle >= 0 ? handle : 0, buf, 1);

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_MOUSE_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (handle >= 0 && fn_close != NULL && obs_address_is_callable((const void *)fn_close)) {
        fn_close(handle);
    }

    if (written > 0) {
        obs_report_written("101-input-ext/mouse-read", "sceMouseRead", "out-param",
                           before, buf, OBS_MOUSE_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("101-input-ext/mouse-read", "sceMouseRead", "untouched",
                       before, buf, OBS_MOUSE_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("read returned error and wrote nothing", (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_MOUSE_BUF_SIZE
}

static const obs_check input_ext_checks[] = {
    {"101-input-ext/keyboard-presence", "libSceKeyboard", "sceKeyboardReadState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_keyboard_presence,
     check_keyboard_presence, OBS_FROM_ASSUMED},
    {"101-input-ext/mouse-presence", "libSceMouse", "sceMouseRead",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_mouse_presence,
     check_mouse_presence, OBS_FROM_ASSUMED},
    {"101-input-ext/kbd-read", "libSceKeyboard", "sceKeyboardReadState",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_keyboard_read_outparam,
     check_keyboard_read_outparam, OBS_FROM_ASSUMED},
    {"101-input-ext/mouse-read", "libSceMouse", "sceMouseRead",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_mouse_read_outparam,
     check_mouse_read_outparam, OBS_FROM_ASSUMED},
};

const obs_section obs_section_input_ext = {
    "101-input-ext",
    "Keyboard and mouse input extensions",
    "Resolves libSceKeyboard and libSceMouse symbols, dumps entry-point machine code "
    "prologues for layout extraction, and tests out-param state read buffers.",
    input_ext_checks,
    OBS_COUNT(input_ext_checks),
};
