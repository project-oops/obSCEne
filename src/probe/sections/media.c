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
     (const void *)&sceVideoOutSetFlipRate, check_video_flip_rate_rejects_bad_handle, OBS_FROM_ASSUMED},
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

static const obs_check audio_checks[] = {
    {"090-audio/initialise", "libSceAudioOut", "sceAudioOutInit", OBS_CAP_NONE,
     OBS_CAP_AUDIO, (const void *)&sceAudioOutInit, check_audio_init, OBS_FROM_ASSUMED},
    {"090-audio/close-rejects-bad-handle", "libSceAudioOut", "sceAudioOutClose",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAudioOutClose,
     check_audio_close_rejects_bad_handle, OBS_FROM_ASSUMED},
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

static const obs_check input_checks[] = {
    {"100-input/initialise", "libScePad", "scePadInit", OBS_CAP_NONE, OBS_CAP_INPUT,
     (const void *)&scePadInit, check_pad_init, OBS_FROM_ASSUMED},
    {"100-input/open", "libScePad", "scePadOpen", OBS_CAP_INPUT, OBS_CAP_NONE,
     (const void *)&scePadOpen, check_pad_open, OBS_FROM_ASSUMED},
    {"100-input/close-rejects-bad-handle", "libScePad", "scePadClose", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePadClose, check_pad_close_rejects_bad_handle, OBS_FROM_ASSUMED},
};

const obs_section obs_section_input = {
    "100-input",
    "Controller input",
    "Bringing up the controller subsystem and acquiring a pad.",
    input_checks,
    OBS_COUNT(input_checks),
};
