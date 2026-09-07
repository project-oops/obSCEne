/*
 * Presentation: video, audio, input.
 *
 * Last, and reached only once everything beneath works. These are the subsystems
 * everyone wants to write first and the ones least worth writing first - an audio
 * shim built before the address space works cannot be exercised, so it cannot be
 * trusted.
 *
 * # Why so much of this section is negative
 *
 * Opening a video output or reading a controller means passing structures whose
 * layouts this project is not confident about. Guessing at one corrupts the stack,
 * and the crash lands nowhere near the mistake. Checking from the failure side needs
 * no layout at all and still proves the function exists, is reachable, validates its
 * arguments and returns a plausible error - which is exactly the set of things a
 * stub returning a constant gets wrong.
 */

#include "obscene/harness.h"
#include "obscene/display.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* The main video bus. Additional buses exist for auxiliary outputs. */

/* Resolved once per run rather than carried from the user section, so this file has
 * no ordering dependency beyond the one the harness already enforces. */
static int32_t initial_user(void) {
    int32_t user = 0;
    if (sceUserServiceGetInitialUser(&user) != 0) {
        return -1;
    }
    return user;
}

/* ---- 080-video ------------------------------------------------------------- */

static obs_result check_video_close_rejects_bad_handle(void) {
    if (obs_display_holds_output()) {
        /* Closing anything on this output risks the display's registration, and an
         * invalid handle is not obviously safe to hand to a platform that may not
         * validate it before touching shared state. */
        return obs_skip("the probe is drawing its report on this output");
    }
    int rc = sceVideoOutClose(OBS_HANDLE_INVALID);
    if (rc == 0) {
        return obs_partial("closing an invalid video handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_video_open(void) {
    OBS_REQUIRE(&sceVideoOutClose);
    if (obs_display_holds_output()) {
        /* The probe is drawing its report on this output. Opening it again and handing
         * it straight back tears the registration down underneath the display on at
         * least one platform, and the screen is worth more than this check: opening the
         * display already proved the output opens. */
        return obs_skip("the probe is drawing its report on this output");
    }
    int32_t user = initial_user();
    if (user < 0) {
        return obs_skip("no initial user, so there is nobody to open an output for");
    }
    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, NULL);
    if (handle <= 0) {
        handle = sceVideoOutOpen(0xFF, OBS_VIDEO_BUS_MAIN, 0, NULL);
    }
    if (handle <= 0) {
        return obs_fail_code("the main video output would not open",
                             (uint64_t)(uint32_t)handle);
    }
    /* Handed straight back. This program must leave the platform as it found it, or
     * a later run in the same process sees a different machine. */
    int rc = sceVideoOutClose(handle);
    if (rc != 0) {
        return obs_partial_value("opened, but the handle would not close",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)(uint32_t)handle);
}

static obs_result check_video_flip_rate_rejects_bad_handle(void) {
    int rc = sceVideoOutSetFlipRate(OBS_HANDLE_INVALID, 0);
    if (rc == 0) {
        return obs_partial("setting a flip rate on an invalid handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static const obs_check video_checks[] = {
    {"080-video/close-rejects-bad-handle", "libSceVideoOut", "sceVideoOutClose",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoOutClose,
     check_video_close_rejects_bad_handle, OBS_FROM_ASSUMED},
    {"080-video/open", "libSceVideoOut", "sceVideoOutOpen", OBS_CAP_MEMORY,
     OBS_CAP_VIDEO, (const void *)&sceVideoOutOpen, check_video_open, OBS_FROM_ASSUMED},
    {"080-video/flip-rate-rejects-bad-handle", "libSceVideoOut",
     "sceVideoOutSetFlipRate", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceVideoOutSetFlipRate, check_video_flip_rate_rejects_bad_handle,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_video = {
    "080-video",
    "Video output",
    "Acquiring and releasing the main display output.",
    video_checks,
    OBS_COUNT(video_checks),
};

/* ---- 090-audio ------------------------------------------------------------- */

static obs_result check_audio_init(void) {
    int rc = sceAudioOutInit();
    /* Initialising twice is legitimate and reports an already-initialised code
     * rather than failing, so a non-zero result here is not automatically a fault.
     * It is reported as amber with the code, which is the honest position: the value
     * is the finding, and this program does not know every code yet. */
    if (rc != 0) {
        return obs_partial_value("initialisation returned a non-zero code",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_audio_close_rejects_bad_handle(void) {
    int rc = sceAudioOutClose(OBS_HANDLE_INVALID);
    if (rc == 0) {
        return obs_partial("closing an invalid audio handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

#if !defined(OBSCENE_HOST_BUILD)
#include "oops/audio.h"

static obs_result check_oops_audio(void) {
    oops_audio_port_t *port = oops_audio_open(48000, 2, 512);
    if (!port) {
        int err = oops_audio_get_last_error();
        return obs_fail_code("oops_audio_open returned NULL", (uint64_t)(uint32_t)err);
    }
    /* Generate a tiny burst of silence (512 frames of stereo PCM) */
    int16_t silence[512 * 2];
    for (int i = 0; i < 512 * 2; i++) silence[i] = 0;

    int v_rc = oops_audio_set_volume(port, 1.0f, 1.0f);
    int w_rc = oops_audio_write(port, silence, 512);
    oops_audio_close(port);

    if (w_rc < 0) {
        return obs_fail_code("oops_audio_write failed", (uint64_t)(uint32_t)w_rc);
    }
    return obs_pass_value((uint64_t)(uint32_t)v_rc);
}
#endif

static const obs_check audio_checks[] = {
    {"090-audio/initialise", "libSceAudioOut", "sceAudioOutInit", OBS_CAP_NONE,
     OBS_CAP_AUDIO, (const void *)&sceAudioOutInit, check_audio_init, OBS_FROM_ASSUMED},
    {"090-audio/close-rejects-bad-handle", "libSceAudioOut", "sceAudioOutClose",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAudioOutClose,
     check_audio_close_rejects_bad_handle, OBS_FROM_ASSUMED},
#if !defined(OBSCENE_HOST_BUILD)
    {"090-audio/oops-sdk-pcm", "libSceAudioOut", "sceAudioOutOpen", OBS_CAP_NONE,
     OBS_CAP_AUDIO, (const void *)&sceAudioOutOpen, check_oops_audio, OBS_FROM_ASSUMED},
#endif
};

const obs_section obs_section_audio = {
    "090-audio",
    "Audio output",
    "Bringing up the audio subsystem and rejecting invalid handles.",
    audio_checks,
    OBS_COUNT(audio_checks),
};

/* ---- 100-input ------------------------------------------------------------- */

static obs_result check_pad_init(void) {
    int rc = scePadInit();
    if (rc != 0) {
        return obs_partial_value("initialisation returned a non-zero code",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_pad_open(void) {
    OBS_REQUIRE(&scePadClose);
    int32_t user = initial_user();
    if (user < 0) {
        return obs_skip("no initial user, so there is no controller to open");
    }
    int handle = scePadOpen(user, 0, 0, NULL);
    if (handle <= 0) {
        return obs_fail_code("no controller could be opened for the initial user",
                             (uint64_t)(uint32_t)handle);
    }
    int rc = scePadClose(handle);
    if (rc != 0) {
        return obs_partial_value("opened, but the handle would not close",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)(uint32_t)handle);
}

static obs_result check_pad_close_rejects_bad_handle(void) {
    int rc = scePadClose(OBS_HANDLE_INVALID);
    if (rc == 0) {
        return obs_partial("closing an invalid controller handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

#if !defined(OBSCENE_HOST_BUILD)
#include "oops/input.h"

static obs_result check_oops_input(void) {
    if (oops_input_init() != 0) {
        return obs_skip("oops_input_init could not open pad (no user or disconnected)");
    }
    oops_pad_state_t pad;
    int rc = oops_input_poll(0, &pad);
    if (rc != 0) {
        return obs_fail_code("oops_input_poll returned non-zero", (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)pad.buttons);
}
#endif

static const char *const pad_dualsense_symbols[] = {
    "scePadSetTriggerEffect",
    "scePadGetTriggerEffectState",
    "scePadSetVibrationMode",
    "scePadSetVibrationForce",
    "scePadGetControllerInformation",
    "scePadDeviceClassGetExtendedInformation",
    "scePadDeviceClassParseData",
};

static obs_result check_pad_dualsense_symbols(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("libScePad did not load");
    }

    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(pad_dualsense_symbols); i++) {
        const char *name = pad_dualsense_symbols[i];
        const void *addr = obs_module_symbol(handle, name);
        if (addr != NULL) {
            resolved++;
            obs_report_measure("100-input/dualsense-symbols", name, "vaddr",
                               (uint64_t)(uintptr_t)addr, "offset");
            /* The prologue is dumped only where the text is readable. A library's text is
             * execute-only on the console (xotext) - callable but not readable - so the old
             * `obs_address_is_callable` guard passed and the read faulted inside libScePad.
             * `obs_linkmap_readable` refuses xotext, so this dumps on a loader that maps text
             * readable (emulators) and skips it on hardware rather than crashing. (D325) */
            if (obs_strcmp(name, "scePadSetTriggerEffect") == 0 &&
                obs_linkmap_readable((uintptr_t)addr)) {
                obs_report_buffer("100-input/trigger-prologue", name, "prologue",
                                  (const unsigned char *)addr, 256);
            }
        } else {
            obs_report_measure("100-input/dualsense-symbols", name, "unresolved", 0, "status");
        }
    }

    if (resolved > 0) {
        return obs_pass_value((uint64_t)resolved);
    }
    return obs_skip("no DualSense extended symbols resolved (likely ps4_mode)");
}

static obs_result check_pad_trigger_state_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("libScePad did not load");
    }
    int (*fn_get)(int, void *) =
        (int (*)(int, void *))obs_module_symbol(handle, "scePadGetTriggerEffectState");
    if (fn_get == NULL || !obs_address_is_callable((const void *)fn_get)) {
        return obs_skip("scePadGetTriggerEffectState is not resolved");
    }

    int32_t user = initial_user();
    int pad_handle = -1;
    if (user >= 0 && obs_address_is_callable((const void *)&scePadOpen)) {
        pad_handle = scePadOpen(user, 0, 0, NULL);
    }

#define OBS_PAD_BUF_SIZE 4096u
    static uint8_t buf[OBS_PAD_BUF_SIZE];
    static uint8_t before[OBS_PAD_BUF_SIZE];
    for (size_t i = 0; i < OBS_PAD_BUF_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_get(pad_handle >= 0 ? pad_handle : 0, buf);

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_PAD_BUF_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (pad_handle >= 0 && obs_address_is_callable((const void *)&scePadClose)) {
        scePadClose(pad_handle);
    }

    if (written > 0) {
        obs_report_written("100-input/trigger-state", "scePadGetTriggerEffectState", "out-param",
                           before, buf, OBS_PAD_BUF_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("100-input/trigger-state", "scePadGetTriggerEffectState", "untouched",
                       before, buf, OBS_PAD_BUF_SIZE);
    if (rc != 0) {
        return obs_partial_value("call returned error code and wrote nothing", (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_PAD_BUF_SIZE
}

static obs_result check_pad_controller_info_outparam(void) {
    if (!obs_module_resolution_works()) {
        return obs_skip("run-time module resolution is unavailable in this process");
    }
    int handle = obs_module_open("libScePad");
    if (handle < 0) {
        return obs_skip("libScePad did not load");
    }
    int (*fn_info)(int, void *) =
        (int (*)(int, void *))obs_module_symbol(handle, "scePadGetControllerInformation");
    if (fn_info == NULL || !obs_address_is_callable((const void *)fn_info)) {
        return obs_skip("scePadGetControllerInformation is not resolved");
    }

    int32_t user = initial_user();
    int pad_handle = -1;
    if (user >= 0 && obs_address_is_callable((const void *)&scePadOpen)) {
        pad_handle = scePadOpen(user, 0, 0, NULL);
    }

#define OBS_PAD_INFO_SIZE 4096u
    static uint8_t buf[OBS_PAD_INFO_SIZE];
    static uint8_t before[OBS_PAD_INFO_SIZE];
    for (size_t i = 0; i < OBS_PAD_INFO_SIZE; i++) {
        buf[i] = 0xC7u;
        before[i] = 0xC7u;
    }

    int rc = fn_info(pad_handle >= 0 ? pad_handle : 0, buf);

    unsigned int written = 0;
    for (size_t i = 0; i < OBS_PAD_INFO_SIZE; i++) {
        if (buf[i] != before[i]) {
            written = (unsigned int)(i + 1u);
        }
    }

    if (pad_handle >= 0 && obs_address_is_callable((const void *)&scePadClose)) {
        scePadClose(pad_handle);
    }

    if (written > 0) {
        obs_report_written("100-input/controller-info", "scePadGetControllerInformation", "out-param",
                           before, buf, OBS_PAD_INFO_SIZE);
        return obs_pass_value((uint64_t)written);
    }

    obs_report_written("100-input/controller-info", "scePadGetControllerInformation", "untouched",
                       before, buf, OBS_PAD_INFO_SIZE);
    if (rc != 0) {
        return obs_partial_value("call returned error code and wrote nothing", (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
#undef OBS_PAD_INFO_SIZE
}

static const obs_check input_checks[] = {
    {"100-input/initialise", "libScePad", "scePadInit", OBS_CAP_NONE, OBS_CAP_INPUT,
     (const void *)&scePadInit, check_pad_init, OBS_FROM_ASSUMED},
    {"100-input/open", "libScePad", "scePadOpen", OBS_CAP_INPUT, OBS_CAP_NONE,
     (const void *)&scePadOpen, check_pad_open, OBS_FROM_ASSUMED},
    {"100-input/close-rejects-bad-handle", "libScePad", "scePadClose", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePadClose, check_pad_close_rejects_bad_handle,
     OBS_FROM_ASSUMED},
#if !defined(OBSCENE_HOST_BUILD)
    {"100-input/oops-sdk-poll", "libScePad", "scePadReadState", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePadReadState, check_oops_input, OBS_FROM_ASSUMED},
#endif
    {"100-input/dualsense-symbols", "libScePad", "scePadSetTriggerEffect", OBS_CAP_INPUT,
     OBS_CAP_NONE, (const void *)&scePadSetTriggerEffect, check_pad_dualsense_symbols,
     OBS_FROM_ASSUMED},
    {"100-input/trigger-state", "libScePad", "scePadGetTriggerEffectState", OBS_CAP_INPUT,
     OBS_CAP_NONE, (const void *)&scePadGetTriggerEffectState, check_pad_trigger_state_outparam,
     OBS_FROM_ASSUMED},
    {"100-input/controller-info", "libScePad", "scePadGetControllerInformation", OBS_CAP_INPUT,
     OBS_CAP_NONE, (const void *)&scePadGetControllerInformation, check_pad_controller_info_outparam,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_input = {
    "100-input",
    "Controller input",
    "Bringing up the controller subsystem, acquiring a pad, and DualSense features.",
    input_checks,
    OBS_COUNT(input_checks),
};
