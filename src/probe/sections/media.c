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

#include "oops/freestd.h"
#include "oops/krw.h"
#include "obscene/fault.h"
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
    int32_t user = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0 &&
            obs_address_is_callable((const void *)&sceUserServiceInitialize)) {
            sceUserServiceInitialize(NULL);
            (void)sceUserServiceGetInitialUser(&user);
        }
    }
    return user;
}

/* One shared pattern buffer pair for the behavioural probes below (D328); checks run one at a
 * time, so sharing it is safe and saves .bss over a buffer per check. */
static uint8_t s_probe_buf[4096];
static uint8_t s_probe_before[4096];

static void obs_probe_fill(void) {
    for (size_t i = 0; i < sizeof s_probe_buf; i++) {
        s_probe_buf[i] = 0xC7u;
        s_probe_before[i] = 0xC7u;
    }
}

/* 1 + the last index the call changed; 0 if it changed nothing. The write extent. */
static unsigned int obs_probe_extent(void) {
    unsigned int extent = 0;
    for (size_t i = 0; i < sizeof s_probe_buf; i++) {
        if (s_probe_buf[i] != s_probe_before[i]) {
            extent = (unsigned int)(i + 1u);
        }
    }
    return extent;
}

/* The 32-bit word at an offset, assembled from bytes so no alignment or aliasing is assumed. */
static uint32_t obs_probe_word(size_t offset) {
    return (uint32_t)s_probe_buf[offset] | ((uint32_t)s_probe_buf[offset + 1] << 8) |
           ((uint32_t)s_probe_buf[offset + 2] << 16) |
           ((uint32_t)s_probe_buf[offset + 3] << 24);
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

/* ---- display behavioural probes (D328) --------------------------------------------------- */

/* The attribute block: dump the 256 bytes sceVideoOutSetBufferAttribute2 writes with tiling
 * mode 0 and again with 1. The two dumps show where the tiling field lands and confirm which
 * value is the tiled mode - which the SDK currently asserts only on the strength of the picture
 * appearing. No handle and no display needed: the call only fills a caller struct. */
static obs_result check_video_attribute_block(void) {
    if (!obs_address_is_callable((const void *)&sceVideoOutSetBufferAttribute2)) {
        return obs_skip("sceVideoOutSetBufferAttribute2 is not callable");
    }
    /* 0x80000000 is B8G8R8A8_SRGB in the OpenOrbis toolchain; the exact value does not affect
     * where the tiling field lands, which is what the two dumps are for. */
    for (uint32_t tiling = 0; tiling <= 1; tiling++) {
        obs_probe_fill();
        sceVideoOutSetBufferAttribute2(s_probe_buf, 0x80000000ULL, tiling, 1920u, 1080u, 0ULL, 0u,
                                       0ULL);
        obs_report_measure("080-video/attribute-block", "sceVideoOutSetBufferAttribute2",
                           "tiling-mode", (uint64_t)tiling, "index");
        obs_report_buffer("080-video/attribute-block", "sceVideoOutSetBufferAttribute2",
                          tiling == 0 ? "tiling0" : "tiling1", s_probe_buf, 256);
    }
    return obs_pass_value(256);
}

/* Flip status and pending: dump the flip-status record and read the pending query. Needs a
 * video output; when the probe is drawing its own report on the only output, or none can be
 * opened, that is PENDING rather than a risk to the display. */
static obs_result check_video_flip_status(void) {
    if (!obs_address_is_callable((const void *)&sceVideoOutGetFlipStatus)) {
        return obs_skip("sceVideoOutGetFlipStatus is not callable");
    }
    if (obs_display_holds_output()) {
        return obs_pending("the probe is drawing its report on the only output; run headless");
    }
    int32_t user = initial_user();
    int handle = -1;
    if (user >= 0 && obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, NULL);
    }
    if (handle <= 0) {
        return obs_pending("no video output could be opened for a flip-status read");
    }
    obs_probe_fill();
    int rc = sceVideoOutGetFlipStatus(handle, s_probe_buf);
    unsigned int extent = obs_probe_extent();
    if (obs_address_is_callable((const void *)&sceVideoOutClose)) {
        sceVideoOutClose(handle);
    }
    obs_report_written("080-video/flip-status", "sceVideoOutGetFlipStatus",
                       extent > 0 ? "status" : "untouched", s_probe_before, s_probe_buf,
                       sizeof s_probe_buf);
    if (extent == 0) {
        return obs_partial_value("the flip-status read wrote nothing", (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)extent);
}

/* The visual flip probe: burst submits and pacing measurement on the display's own output. */
static obs_result check_video_visual_flip(void) {
    if (!obs_address_is_callable((const void *)&sceVideoOutSubmitFlip)) {
        return obs_skip("sceVideoOutSubmitFlip is not callable");
    }
    int handle = obs_display_get_video_handle();
    if (handle < 0) {
        int32_t user = initial_user();
        if (user >= 0 && obs_address_is_callable((const void *)&sceVideoOutOpen)) {
            handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, NULL);
        }
    }
    if (handle < 0) {
        return obs_pending("needs the display's own output and buffers; run in the eboot leg with the display handed over");
    }

    /* Submit 32 flips back-to-back with flipMode 1 alternating the two buffers */
    int first_refused = 0;
    for (int i = 0; i < 32; i++) {
        uint64_t t0 = 0;
        if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
            t0 = (uint64_t)sceKernelGetProcessTime();
        }
        int buf_idx = i % 2;
        int rc = sceVideoOutSubmitFlip(handle, buf_idx, 1, 0);
        uint64_t t1 = 0;
        if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
            t1 = (uint64_t)sceKernelGetProcessTime();
        }
        char rc_name[16];
        rc_name[0] = 'r'; rc_name[1] = 'c'; rc_name[2] = '-';
        if (i + 1 < 10) {
            rc_name[3] = (char)('0' + (i + 1));
            rc_name[4] = '\0';
        } else {
            rc_name[3] = (char)('0' + (i + 1) / 10);
            rc_name[4] = (char)('0' + (i + 1) % 10);
            rc_name[5] = '\0';
        }
        obs_report_measure("080-video/visual-flip", "sceVideoOutSubmitFlip", rc_name,
                           (uint64_t)(uint32_t)rc, "code");
        char el_name[24];
        el_name[0] = 'e'; el_name[1] = 'l'; el_name[2] = 'a'; el_name[3] = 'p';
        el_name[4] = 's'; el_name[5] = 'e'; el_name[6] = 'd'; el_name[7] = '-';
        if (i + 1 < 10) {
            el_name[8] = (char)('0' + (i + 1));
            el_name[9] = '\0';
        } else {
            el_name[8] = (char)('0' + (i + 1) / 10);
            el_name[9] = (char)('0' + (i + 1) % 10);
            el_name[10] = '\0';
        }
        obs_report_measure("080-video/visual-flip", "sceVideoOutSubmitFlip", el_name,
                           t1 >= t0 ? (t1 - t0) : 0, "us");
        if (rc != 0 && first_refused == 0) {
            first_refused = i + 1;
            break;
        }
    }
    obs_report_measure("080-video/visual-flip", "sceVideoOutSubmitFlip", "first-refused",
                       (uint64_t)first_refused, "index");

    /* Read flip status immediately after the burst */
    if (obs_address_is_callable((const void *)&sceVideoOutGetFlipStatus)) {
        obs_probe_fill();
        sceVideoOutGetFlipStatus(handle, (void *)s_probe_buf);
        obs_report_buffer("080-video/visual-flip", "sceVideoOutGetFlipStatus", "after-burst",
                          s_probe_buf, 64);

        /* 600 ms later */
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(600000);
        } else {
            uint64_t start = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                                 ? (uint64_t)sceKernelGetProcessTime()
                                 : 0;
            while (obs_address_is_callable((const void *)&sceKernelGetProcessTime) &&
                   ((uint64_t)sceKernelGetProcessTime() - start) < 600000) {
            }
        }
        obs_probe_fill();
        sceVideoOutGetFlipStatus(handle, (void *)s_probe_buf);
        obs_report_buffer("080-video/visual-flip", "sceVideoOutGetFlipStatus", "after-600ms",
                          s_probe_buf, 64);
    }

    /* Event queue wait probe if symbols resolve */
    typedef int sce_equeue_t;
    int (*fn_create_eq)(sce_equeue_t *, const char *) =
        (int (*)(sce_equeue_t *, const char *))obs_module_symbol(1, "sceKernelCreateEqueue");
    int (*fn_delete_eq)(sce_equeue_t) =
        (int (*)(sce_equeue_t))obs_module_symbol(1, "sceKernelDeleteEqueue");
    int (*fn_wait_eq)(sce_equeue_t, void *, int, int *, void *) =
        (int (*)(sce_equeue_t, void *, int, int *, void *))obs_module_symbol(1, "sceKernelWaitEqueue");
    int (*fn_add_flip_ev)(sce_equeue_t, int, void *) =
        (int (*)(sce_equeue_t, int, void *))obs_module_symbol(1, "sceVideoOutAddFlipEvent");
    int (*fn_del_flip_ev)(sce_equeue_t, int) =
        (int (*)(sce_equeue_t, int))obs_module_symbol(1, "sceVideoOutDeleteFlipEvent");

    if (fn_create_eq != NULL && fn_add_flip_ev != NULL && fn_wait_eq != NULL) {
        sce_equeue_t eq = 0;
        int eq_rc = fn_create_eq(&eq, "visual_flip_eq");
        if (eq_rc == 0 && eq != 0) {
            (void)fn_add_flip_ev(eq, handle, (void *)0x1234);
            int out_events = 0;
            uint8_t ev_buf[256];
            for (size_t j = 0; j < sizeof(ev_buf); j++) ev_buf[j] = 0xC7;

            /* 32-bit timeout = 50000 us */
            uint32_t to32 = 50000;
            uint64_t t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                              ? (uint64_t)sceKernelGetProcessTime()
                              : 0;
            int w_rc32 = fn_wait_eq(eq, ev_buf, 1, &out_events, (void *)&to32);
            uint64_t t1 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                              ? (uint64_t)sceKernelGetProcessTime()
                              : 0;
            obs_report_measure("080-video/visual-flip", "sceKernelWaitEqueue", "timeout-32",
                               (uint64_t)(uint32_t)w_rc32, "rc");
            obs_report_measure("080-video/visual-flip", "sceKernelWaitEqueue", "elapsed-32",
                               t1 >= t0 ? (t1 - t0) : 0, "us");

            /* 64-bit timeout = 50000 us */
            uint64_t to64 = 50000;
            t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                     ? (uint64_t)sceKernelGetProcessTime()
                     : 0;
            int w_rc64 = fn_wait_eq(eq, ev_buf, 1, &out_events, (void *)&to64);
            t1 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                     ? (uint64_t)sceKernelGetProcessTime()
                     : 0;
            obs_report_measure("080-video/visual-flip", "sceKernelWaitEqueue", "timeout-64",
                               (uint64_t)(uint32_t)w_rc64, "rc");
            obs_report_measure("080-video/visual-flip", "sceKernelWaitEqueue", "elapsed-64",
                               t1 >= t0 ? (t1 - t0) : 0, "us");

            if (fn_del_flip_ev != NULL) {
                fn_del_flip_ev(eq, handle);
            }
            if (fn_delete_eq != NULL) {
                fn_delete_eq(eq);
            }
        }
    }

    return obs_pass();
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
    {"080-video/attribute-block", "libSceVideoOut", "sceVideoOutSetBufferAttribute2",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoOutSetBufferAttribute2,
     check_video_attribute_block, OBS_FROM_ASSUMED},
    {"080-video/flip-status", "libSceVideoOut", "sceVideoOutGetFlipStatus", OBS_CAP_VIDEO,
     OBS_CAP_NONE, (const void *)&sceVideoOutGetFlipStatus, check_video_flip_status,
     OBS_FROM_ASSUMED},
    {"080-video/visual-flip", "libSceVideoOut", "sceVideoOutSubmitFlip", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceVideoOutSubmitFlip, check_video_visual_flip,
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
    /* Initialising twice is legitimate (harness startup also calls sceAudioOutInit)
     * and reports 0x8026000E (already initialised) rather than failing. Both 0 and
     * 0x8026000E establish that audio is ready, granting OBS_CAP_AUDIO. */
    if (rc == 0 || (uint32_t)rc == 0x8026000Eu) {
        return obs_pass_value((uint64_t)(uint32_t)rc);
    }
    return obs_partial_value("initialisation returned a non-zero code",
                             (uint64_t)(uint32_t)rc);
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

/* ---- audio behavioural probes, called directly (D328) -----------------------------------
 *
 * Audio needs no peripheral, so these do not go PENDING: they open a port and measure what a
 * header states without evidence - the channel count a selector opens, the chunk sizes an open
 * accepts, whether output blocks, and the volume flags. A port that will not open is a FAIL or
 * SKIP with the code, not a PENDING. */

/* Open an audio port, returning handle (>= 0) or the negative return code (< 0).
 * length is frames per chunk, param the format selector. */
static int obs_audio_open(uint32_t length, uint32_t frequency, uint32_t param) {
    if (obs_address_is_callable((const void *)&sceAudioOutInit)) {
        sceAudioOutInit();
    }
    if (!obs_address_is_callable((const void *)&sceAudioOutOpen)) {
        return -1;
    }
    int32_t user = initial_user();
    int handle = -1;
    if (user >= 0) {
        handle = sceAudioOutOpen(user, 0, 0, length, frequency, param);
    }
    if (handle < 0) {
        handle = sceAudioOutOpen(0xFF, 0, 0, length, frequency, param);
    }
    return handle;
}

static void obs_audio_close(int handle) {
    if (handle >= 0 && obs_address_is_callable((const void *)&sceAudioOutClose)) {
        sceAudioOutClose(handle);
    }
}

/* Format selector: open at selector 0, 1 and 2 and dump the port-state record after each. The
 * state carries the channel count, which decides whether 0 is stereo or mono objectively. */
static obs_result check_audio_format_selector(void) {
    if (!obs_address_is_callable((const void *)&sceAudioOutGetPortState)) {
        return obs_skip("sceAudioOutGetPortState is not callable");
    }
    unsigned int opened = 0;
    static const char *const sel_labels[3] = {"state-sel0", "state-sel1", "state-sel2"};
    for (uint32_t sel = 0; sel <= 2; sel++) {
        int handle = obs_audio_open(512, 48000, sel);
        if (handle < 0) {
            obs_report_measure("090-audio/format-selector", "sceAudioOutOpen", "would-not-open",
                               (uint64_t)(uint32_t)handle, "return");
            continue;
        }
        opened++;
        obs_probe_fill();
        int rc = sceAudioOutGetPortState(handle, s_probe_buf);
        unsigned int extent = obs_probe_extent();
        obs_audio_close(handle);
        obs_report_measure("090-audio/format-selector", "sceAudioOutGetPortState", "selector",
                           (uint64_t)sel, "index");
        if (extent >= 3) {
            obs_report_measure("090-audio/format-selector", "sceAudioOutGetPortState", "channels",
                               (uint64_t)s_probe_buf[2], "count");
        }
        const char *label = (sel < 3) ? sel_labels[sel] : "state";
        obs_report_written("090-audio/format-selector", "sceAudioOutGetPortState",
                           extent > 0 ? label : "untouched", s_probe_before, s_probe_buf,
                           sizeof s_probe_buf);
        (void)rc;
    }
    if (opened == 0) {
        return obs_fail("no selector opened a port");
    }
    return obs_pass_value((uint64_t)opened);
}

/* Accepted open shapes: chunk sizes 256..2048 at 48000 and 44100, each return code reported.
 * The SDK's rounding to multiples of 256 in that range is an assumption this settles. */
static obs_result check_audio_open_shapes(void) {
    if (!obs_address_is_callable((const void *)&sceAudioOutOpen)) {
        return obs_skip("sceAudioOutOpen is not callable");
    }
    static const uint32_t freqs[] = {48000u, 44100u};
    static const uint32_t chunks[] = {256u, 512u, 1024u, 2048u};
    unsigned int accepted = 0;
    for (size_t f = 0; f < OBS_COUNT(freqs); f++) {
        for (size_t c = 0; c < OBS_COUNT(chunks); c++) {
            int handle = obs_audio_open(chunks[c], freqs[f], 0);
            /* value packs freq and chunk so a reader sees which shape each code belongs to. */
            uint64_t shape = ((uint64_t)freqs[f] << 16) | (uint64_t)chunks[c];
            if (handle >= 0) {
                accepted++;
                obs_report_measure("090-audio/open-shapes", "sceAudioOutOpen",
                                   "accepted", shape, "freq<<16|chunk");
                obs_audio_close(handle);
            } else {
                obs_report_measure("090-audio/open-shapes", "sceAudioOutOpen",
                                   "rejected-rc", (uint64_t)(uint32_t)handle, "return");
                obs_report_measure("090-audio/open-shapes", "sceAudioOutOpen",
                                   "rejected", shape, "freq<<16|chunk");
            }
        }
    }
    if (accepted == 0) {
        return obs_fail("no open shape was accepted");
    }
    return obs_pass_value((uint64_t)accepted);
}

/* Blocking behaviour: time eight consecutive 512-frame outputs. At 48 kHz a blocking call takes
 * about 85 ms for the set; a non-blocking one returns at once. The header promises blocking. */
static obs_result check_audio_blocking(void) {
    if (!obs_address_is_callable((const void *)&sceAudioOutOutput) ||
        !obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        return obs_skip("sceAudioOutOutput or the process clock is not callable");
    }
    int handle = obs_audio_open(512, 48000, 0);
    if (handle < 0) {
        obs_report_measure("090-audio/blocking", "sceAudioOutOpen", "rejected-rc",
                           (uint64_t)(uint32_t)handle, "return");
        return obs_fail_code("a port would not open for the timing run", (uint64_t)(uint32_t)handle);
    }
    static int16_t silence[512 * 2]; /* 512 frames, stereo, zero-filled (.bss) */
    uint64_t start = sceKernelGetProcessTime();
    for (int i = 0; i < 8; i++) {
        (void)sceAudioOutOutput(handle, silence);
    }
    uint64_t elapsed = sceKernelGetProcessTime() - start;
    obs_audio_close(handle);
    obs_report_measure("090-audio/blocking", "sceAudioOutOutput", "eight-outputs-512", elapsed,
                       "microseconds");
    /* ~85 ms means it blocked; near zero means it did not. Either is a pass - the number is the
     * finding - but a near-instant set is amber, because the header promises blocking. */
    if (elapsed < 40000u) {
        return obs_partial_value("eight outputs returned far faster than real time", elapsed);
    }
    return obs_pass_value(elapsed);
}

/* Volume flag: set-volume with flags 1, 2 and 3, each return code reported. */
static obs_result check_audio_volume_flag(void) {
    if (!obs_address_is_callable((const void *)&sceAudioOutSetVolume)) {
        return obs_skip("sceAudioOutSetVolume is not callable");
    }
    int handle = obs_audio_open(512, 48000, 0);
    if (handle < 0) {
        obs_report_measure("090-audio/volume-flag", "sceAudioOutOpen", "rejected-rc",
                           (uint64_t)(uint32_t)handle, "return");
        return obs_fail_code("a port would not open for the volume run", (uint64_t)(uint32_t)handle);
    }
    int vol[8];
    for (size_t i = 0; i < 8; i++) {
        vol[i] = 32768; /* 0 dB, per the OpenOrbis constant */
    }
    unsigned int ok = 0;
    for (int flag = 1; flag <= 3; flag++) {
        int rc = sceAudioOutSetVolume(handle, flag, vol);
        obs_report_measure("090-audio/volume-flag", "sceAudioOutSetVolume",
                           rc == 0 ? "accepted" : "code", (uint64_t)(uint32_t)rc, "flag-return");
        if (rc == 0) {
            ok++;
        }
    }
    obs_audio_close(handle);
    if (ok == 0) {
        return obs_partial("no volume flag returned success");
    }
    return obs_pass_value((uint64_t)ok);
}

static obs_result check_audio_drain(void) {
    if (!obs_address_is_callable((const void *)&sceAudioOutOutput) ||
        !obs_address_is_callable((const void *)&sceAudioOutOpen) ||
        !obs_address_is_callable((const void *)&sceAudioOutClose)) {
        return obs_skip("sceAudioOut functions not callable");
    }
    int handle = obs_audio_open(512, 48000, 1);
    if (handle < 0) {
        return obs_fail_code("a port would not open for audio drain probe",
                             (uint64_t)(uint32_t)handle);
    }
    static int16_t silence[512 * 2]; /* 512 frames, stereo */
    for (size_t i = 0; i < sizeof(silence) / sizeof(silence[0]); i++) {
        silence[i] = 0;
    }
    /* Queue four chunks of silence */
    for (int i = 0; i < 4; i++) {
        (void)sceAudioOutOutput(handle, silence);
    }
    /* Call sceAudioOutOutput(handle, NULL) */
    uint64_t t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                      ? (uint64_t)sceKernelGetProcessTime()
                      : 0;
    int rc_null = sceAudioOutOutput(handle, NULL);
    uint64_t t1 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                      ? (uint64_t)sceKernelGetProcessTime()
                      : 0;
    uint64_t el_null = t1 >= t0 ? (t1 - t0) : 0;
    obs_report_measure("090-audio/drain", "sceAudioOutOutput", "null-rc",
                       (uint64_t)(uint32_t)rc_null, "code");
    obs_report_measure("090-audio/drain", "sceAudioOutOutput", "null-elapsed",
                       el_null, "us");

    /* Call once more on the now-empty queue */
    t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
             ? (uint64_t)sceKernelGetProcessTime()
             : 0;
    int rc_empty = sceAudioOutOutput(handle, NULL);
    t1 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
             ? (uint64_t)sceKernelGetProcessTime()
             : 0;
    uint64_t el_empty = t1 >= t0 ? (t1 - t0) : 0;
    obs_report_measure("090-audio/drain", "sceAudioOutOutput", "null-empty-rc",
                       (uint64_t)(uint32_t)rc_empty, "code");
    obs_report_measure("090-audio/drain", "sceAudioOutOutput", "null-empty-elapsed",
                       el_empty, "us");

    obs_audio_close(handle);

    /* Open port again with selector 1 at 48000, 512 frames */
    handle = obs_audio_open(512, 48000, 1);
    if (handle >= 0) {
        /* Queue four chunks */
        for (int i = 0; i < 4; i++) {
            (void)sceAudioOutOutput(handle, silence);
        }
        /* Close immediately */
        t0 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                 ? (uint64_t)sceKernelGetProcessTime()
                 : 0;
        int rc_close = sceAudioOutClose(handle);
        t1 = obs_address_is_callable((const void *)&sceKernelGetProcessTime)
                 ? (uint64_t)sceKernelGetProcessTime()
                 : 0;
        uint64_t el_close = t1 >= t0 ? (t1 - t0) : 0;
        obs_report_measure("090-audio/drain", "sceAudioOutClose", "queued-close-rc",
                           (uint64_t)(uint32_t)rc_close, "code");
        obs_report_measure("090-audio/drain", "sceAudioOutClose", "queued-close-elapsed",
                           el_close, "us");
    }

    return obs_pass();
}

static obs_result check_audio_sysmodules(void) {
    typedef int (*fn_load_t)(uint16_t id);
    fn_load_t fn_load = (fn_load_t)obs_module_symbol(1, "sceSysmoduleLoadModule");
    if (fn_load == NULL) {
        fn_load = (fn_load_t)obs_module_symbol(OBS_HANDLE_SELF, "sceSysmoduleLoadModule");
    }
    if (fn_load == NULL && obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        fn_load = &sceSysmoduleLoadModule;
    }

    obs_report_measure("090-audio/sysmodules", "sceSysmoduleLoadModule", "callable",
                       fn_load != NULL ? 1 : 0, "flag");

    if (fn_load == NULL || !obs_address_is_callable((const void *)fn_load)) {
        return obs_skip("sceSysmoduleLoadModule not available");
    }

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    int rc_dec = -1;
    int rc_3d = -1;
    if (sig == 0) {
        rc_dec = fn_load(0x0088); /* OOPS_SYSMODULE_AUDIO_DEC */
        rc_3d  = fn_load(0x00A7); /* OOPS_SYSMODULE_AUDIO_3D */
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail_code("sceSysmoduleLoadModule faulted", (uint64_t)(uint32_t)sig);
    }

    obs_report_measure("090-audio/sysmodules", "libSceAudioDec", "module_id", 0x0088, "id");
    obs_report_measure("090-audio/sysmodules", "libSceAudioDec", "rc",
                       (uint64_t)(uint32_t)rc_dec, "code");
    obs_report_measure("090-audio/sysmodules", "libSceAudio3d", "module_id", 0x00A7, "id");
    obs_report_measure("090-audio/sysmodules", "libSceAudio3d", "rc",
                       (uint64_t)(uint32_t)rc_3d, "code");

    return obs_pass();
}

static obs_result check_audio_audioout2_ports(void) {
    static const char *const syms[] = {
        "sceAudioOut2Initialize",
        "sceAudioOut2PortCreate",
        "sceAudioOut2PortDestroy",
        "sceAudioOut2PortGetState",
        "sceAudioOut2PortSetAttributes",
    };

    int handle = obs_module_open("libSceAudioOut2");
    unsigned int resolved = 0;
    const void *fn_init = NULL;
    const void *fn_destroy = NULL;

    for (size_t i = 0; i < OBS_COUNT(syms); i++) {
        const char *name = syms[i];
        const void *addr = NULL;
        if (handle >= 0) {
            addr = obs_module_symbol(handle, name);
        }
        if (addr == NULL) {
            addr = obs_module_symbol(1, name);
        }
        if (addr == NULL) {
            addr = obs_module_symbol(OBS_HANDLE_SELF, name);
        }

        obs_report_measure("090-audio/audioout2-ports", name, "vaddr",
                           (uint64_t)(uintptr_t)addr, "vaddr");
        obs_report_measure("090-audio/audioout2-ports", name, "resolved",
                           addr != NULL ? 1 : 0, "bool");
        if (addr != NULL) {
            resolved++;
            if (obs_strcmp(name, "sceAudioOut2Initialize") == 0) {
                fn_init = addr;
            } else if (obs_strcmp(name, "sceAudioOut2PortDestroy") == 0) {
                fn_destroy = addr;
            }
        }
    }

    if (resolved == 0) {
        return obs_skip("libSceAudioOut2 symbols not resolved");
    }

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("libSceAudioOut2 probe faulted", (uint64_t)(uint32_t)sig);
    }

    /* Probe sceAudioOut2Initialize if callable */
    if (fn_init != NULL && obs_address_is_callable(fn_init)) {
        int (*init_func)(const void *) = (int (*)(const void *))fn_init;
        int rc_init = init_func(NULL);
        obs_report_measure("090-audio/audioout2-ports", "sceAudioOut2Initialize", "rc",
                           (uint64_t)(uint32_t)rc_init, "code");
    }

    /* Probe sceAudioOut2PortDestroy with invalid handle */
    if (fn_destroy != NULL && obs_address_is_callable(fn_destroy)) {
        int (*destroy_func)(int) = (int (*)(int))fn_destroy;
        int rc_destroy = destroy_func(OBS_HANDLE_INVALID);
        obs_report_measure("090-audio/audioout2-ports", "sceAudioOut2PortDestroy", "bad-handle-rc",
                           (uint64_t)(uint32_t)rc_destroy, "code");
    }

    obs_fault_unregister();
    return obs_pass_value((uint64_t)resolved);
}

static const obs_check audio_checks[] = {
    {"090-audio/initialise", "libSceAudioOut", "sceAudioOutInit", OBS_CAP_NONE,
     OBS_CAP_AUDIO, (const void *)&sceAudioOutInit, check_audio_init, OBS_FROM_ASSUMED},
    {"090-audio/sysmodules", "libSceSysmodule", "sceSysmoduleLoadModule", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audio_sysmodules, check_audio_sysmodules,
     OBS_FROM_ASSUMED},
    {"090-audio/audioout2-ports", "libSceAudioOut2", "sceAudioOut2PortCreate", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_audio_audioout2_ports, check_audio_audioout2_ports,
     OBS_FROM_ASSUMED},
    {"090-audio/drain", "libSceAudioOut", "sceAudioOutOutput", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAudioOutOutput, check_audio_drain,
     OBS_FROM_ASSUMED},
    {"090-audio/format-selector", "libSceAudioOut", "sceAudioOutGetPortState", OBS_CAP_AUDIO,
     OBS_CAP_NONE, (const void *)&sceAudioOutGetPortState, check_audio_format_selector,
     OBS_FROM_ASSUMED},
    {"090-audio/open-shapes", "libSceAudioOut", "sceAudioOutOpen", OBS_CAP_AUDIO, OBS_CAP_NONE,
     (const void *)&sceAudioOutOpen, check_audio_open_shapes, OBS_FROM_ASSUMED},
    {"090-audio/blocking", "libSceAudioOut", "sceAudioOutOutput", OBS_CAP_AUDIO, OBS_CAP_NONE,
     (const void *)&sceAudioOutOutput, check_audio_blocking, OBS_FROM_ASSUMED},
    {"090-audio/volume-flag", "libSceAudioOut", "sceAudioOutSetVolume", OBS_CAP_AUDIO,
     OBS_CAP_NONE, (const void *)&sceAudioOutSetVolume, check_audio_volume_flag,
     OBS_FROM_ASSUMED},
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
    oops_input_close();
    if (rc != 0) {
        return obs_fail_code("oops_input_poll returned non-zero", (uint64_t)(uint32_t)rc);
    }
    /* The raw record, not only the button word: a pass here should carry the bytes a later
     * layout question will need, rather than reducing the poll to one field. (D328) */
    obs_report_buffer("100-input/oops-sdk-poll", "scePadReadState", "raw",
                      (const unsigned char *)&pad, sizeof pad);
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

/* ---- controller behavioural probes, called directly so they run in every leg (D328) ----
 *
 * The out-param probes above resolve through dlsym and skip on a native eboot, whose dlsym
 * sees only imported symbols. These call the pad functions as the imports they now are, so a
 * pad-attached run reports in all three legs. Where no controller is attached - or none of the
 * inputs a probe is watching for arrive in its window - the result is PENDING, not a fault and
 * not a skip: plug in / press, re-run, and the same probe answers. */

/* Open a pad for the initial user, or -1. Caller closes with scePadClose. */
static int obs_pad_open(void) {
    if (obs_address_is_callable((const void *)&scePadInit)) {
        scePadInit();
    }
    int32_t user = initial_user();
    if (user < 0) {
        user = 1;
    }
    if (!obs_address_is_callable((const void *)&scePadOpen)) {
        return -1;
    }
    int handle = scePadOpen(user, 0, 0, NULL);
    if (handle <= 0 && user != 1) {
        handle = scePadOpen(1, 0, 0, NULL);
    }
    return handle > 0 ? handle : -1;
}

static void obs_pad_close(int handle) {
    if (handle >= 0 && obs_address_is_callable((const void *)&scePadClose)) {
        scePadClose(handle);
    }
}

/* Write extent of one scePadReadState - the record size a batched read strides by. */
static obs_result check_pad_read_extent(void) {
    if (!obs_address_is_callable((const void *)&scePadReadState)) {
        return obs_skip("scePadReadState is not callable");
    }
    int handle = obs_pad_open();
    obs_probe_fill();
    int rc = -1;
    if (handle >= 0) {
        rc = scePadReadState(handle, s_probe_buf);
        obs_pad_close(handle);
    } else {
        rc = scePadReadState(0, s_probe_buf);
    }
    unsigned int extent = obs_probe_extent();
    obs_report_written("100-input/read-extent", "scePadReadState",
                       "extent", s_probe_before, s_probe_buf,
                       sizeof s_probe_buf);
    obs_report_written("100-input/read-extent", "scePadReadState",
                       "out-param", s_probe_before, s_probe_buf,
                       sizeof s_probe_buf);
    obs_report_measure("100-input/read-extent", "scePadReadState", "extent",
                       (uint64_t)extent, "bytes");
    obs_report_measure("100-input/read-extent", "scePadReadState", "rc",
                       (uint64_t)(uint32_t)rc, "code");
    if (extent == 0) {
        return obs_partial_value("disconnected: read completed", (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)extent);
}

/* The batched read: ask for four records, report the return value and the extent. Extent over
 * count is the stride; the return value confirms it is a count. */
static obs_result check_pad_batched_read(void) {
    if (!obs_address_is_callable((const void *)&scePadRead)) {
        return obs_skip("scePadRead is not callable");
    }
    int handle = obs_pad_open();
    obs_probe_fill();
    int rc = -1;
    if (handle >= 0) {
        rc = scePadRead(handle, s_probe_buf, 4);
        obs_pad_close(handle);
    } else {
        rc = scePadRead(0, s_probe_buf, 4);
    }
    unsigned int extent = obs_probe_extent();
    obs_report_written("100-input/batched-read", "scePadRead",
                       "extent", s_probe_before, s_probe_buf,
                       sizeof s_probe_buf);
    obs_report_written("100-input/batched-read", "scePadRead",
                       "out-param", s_probe_before, s_probe_buf,
                       sizeof s_probe_buf);
    obs_report_measure("100-input/batched-read", "scePadRead", "returned",
                       (uint64_t)(uint32_t)rc, "count");
    obs_report_measure("100-input/batched-read", "scePadRead", "extent",
                       (uint64_t)extent, "bytes");
    if (extent == 0) {
        return obs_partial_value("disconnected: batched read completed", (uint64_t)(uint32_t)rc);
    }
    return obs_pass_value((uint64_t)extent);
}

/* Button bits: sample for a few seconds while Create, PS, touchpad-click and mic are pressed in
 * turn, and report the OR of every button word (offset 0) seen. Settles whether Create is bit 16
 * and whether the others arrive at all. */
static obs_result check_pad_button_bits(void) {
    if (!obs_address_is_callable((const void *)&scePadReadState)) {
        return obs_skip("scePadReadState is not callable");
    }
    int handle = obs_pad_open();
    if (handle < 0) {
        return obs_pending("no controller attached: connect one and re-run");
    }
    obs_report_measure("100-input/button-bits", "scePadReadState",
                       "press-create-ps-touchpad-mic-now", 4, "prompt-seconds");
    uint32_t seen = 0;
    /* ~4s at 50ms: bounded, non-blocking, and long enough to press four buttons in turn. */
    for (int i = 0; i < 80; i++) {
        obs_probe_fill();
        if (scePadReadState(handle, s_probe_buf) == 0 || obs_probe_extent() >= 4u) {
            seen |= obs_probe_word(0);
        }
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(50000);
        }
    }
    obs_pad_close(handle);
    if (seen == 0u) {
        return obs_pending("controller attached, but no button was seen in the window");
    }
    obs_report_measure("100-input/button-bits", "scePadReadState", "button-or", (uint64_t)seen,
                       "bits");
    return obs_pass_value((uint64_t)seen);
}

/* Stick and trigger raw range: sample while the sticks are swept and the triggers pulled, and
 * report the per-byte minimum and maximum over the first sixteen bytes of the record. The
 * mapper assumes sticks at bytes 4-7 and triggers at 8-9, 0..255 with 128 at centre (OpenOrbis);
 * the min/max pair shows the real rest-and-deflection range without this asserting the offsets. */
static obs_result check_pad_stick_trigger_range(void) {
    if (!obs_address_is_callable((const void *)&scePadReadState)) {
        return obs_skip("scePadReadState is not callable");
    }
    int handle = obs_pad_open();
    if (handle < 0) {
        return obs_pending("no controller attached: connect one and re-run");
    }
    obs_report_measure("100-input/stick-trigger-range", "scePadReadState",
                       "sweep-sticks-and-pull-triggers-now", 4, "prompt-seconds");
    uint8_t lo[16], hi[16];
    for (size_t i = 0; i < 16; i++) {
        lo[i] = 0xFFu;
        hi[i] = 0x00u;
    }
    int samples = 0;
    for (int i = 0; i < 80; i++) {
        obs_probe_fill();
        if (scePadReadState(handle, s_probe_buf) == 0 || obs_probe_extent() >= 16u) {
            samples++;
            for (size_t b = 0; b < 16; b++) {
                if (s_probe_buf[b] < lo[b]) {
                    lo[b] = s_probe_buf[b];
                }
                if (s_probe_buf[b] > hi[b]) {
                    hi[b] = s_probe_buf[b];
                }
            }
        }
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(50000);
        }
    }
    obs_pad_close(handle);
    if (samples == 0) {
        return obs_pending("controller attached, but no state was read in the window");
    }
    obs_report_buffer("100-input/stick-trigger-range", "scePadReadState", "min", lo, 16);
    obs_report_buffer("100-input/stick-trigger-range", "scePadReadState", "max", hi, 16);
    /* Any byte whose range opened up means something moved; a flat window is still waiting. */
    unsigned int moved = 0;
    for (size_t b = 0; b < 16; b++) {
        if (hi[b] > lo[b]) {
            moved++;
        }
    }
    if (moved == 0) {
        return obs_pending("controller read, but nothing moved: sweep the sticks and re-run");
    }
    return obs_pass_value((uint64_t)moved);
}

static const char *const pad_injection_symbols[] = {
    "scePadInit",
    "scePadOpen",
    "scePadClose",
    "scePadReadState",
    "scePadVirtualDeviceAddDevice",
    "scePadVirtualDeviceDeleteDevice",
    "scePadVirtualDeviceInsertData",
    "scePadSetParticularMode",
};

static obs_result check_input_payload_injection(void) {
    OBS_REQUIRE(&strcmp, &strncmp);

    obs_jmp_buf buf;
    int sig = OBS_FAULT_ARM(&buf);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("input payload injection lookup faulted", (uint64_t)(uint32_t)sig);
    }

    /* 1. Resolve sceSysmoduleLoadModule */
    int (*fn_load_module)(uint16_t) = NULL;
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        char nid[12];
        obs_compute_nid("sceSysmoduleLoadModule", nid);
        const void *kaddr = obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            fn_load_module = (int (*)(uint16_t))kaddr;
        }
    }
    if (fn_load_module == NULL && pargs == NULL && obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        fn_load_module = &sceSysmoduleLoadModule;
    }
    if (fn_load_module == NULL) {
        const void *msym = obs_module_symbol(1, "sceSysmoduleLoadModule");
        if (msym != NULL && obs_address_is_callable(msym)) {
            fn_load_module = (int (*)(uint16_t))msym;
        }
    }

    obs_report_measure("100-input/sysmodule-callable", "sceSysmoduleLoadModule", "callable",
                       fn_load_module != NULL ? 1 : 0, "flag");
    if (fn_load_module != NULL) {
        obs_report_measure("100-input/sysmodule-callable", "sceSysmoduleLoadModule", "vaddr",
                           (uint64_t)(uintptr_t)fn_load_module, "vaddr");
    }

    int load_rc = -1;
    if (fn_load_module != NULL) {
        load_rc = fn_load_module(0x0027); /* libScePad sysmodule ID */
    }
    obs_report_measure("100-input/sysmodule-load", "libScePad", "module_id", 0x0027, "id");
    obs_report_measure("100-input/sysmodule-load", "sceSysmoduleLoadModule", "rc",
                       (uint64_t)(uint32_t)load_rc, "code");

    /* 2. Re-attempt resolution of the eight symbols across 3 routes */
    int handles[128];
    size_t handle_count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &handle_count);
    }

#if !defined(OBSCENE_HOST_BUILD)
    pid_t pid = 0;
    if (krw_is_ready()) {
        pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    }
#endif

    unsigned int any_resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(pad_injection_symbols); i++) {
        const char *name = pad_injection_symbols[i];
        char nid[12];
        obs_compute_nid(name, nid);

        /* Route 1: dlsym */
        void *dlsym_addr = NULL;
        if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (size_t h = 0; h < handle_count && h < 128; h++) {
                if (handles[h] <= 0) continue;
                void *a = NULL;
                if (sceKernelDlsym(handles[h], nid, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
                if (sceKernelDlsym(handles[h], name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
            }
            if (dlsym_addr == NULL) {
                void *a = NULL;
                if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                } else if (sceKernelDlsym(0x2001, name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                } else {
                    const void *sym_self = obs_module_symbol(OBS_HANDLE_SELF, name);
                    if (sym_self != NULL) {
                        dlsym_addr = (void *)sym_self;
                    }
                }
            }
        }

        /* Route 2: kexport */
        void *kexport_addr = NULL;
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *ka = obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                kexport_addr = (void *)ka;
            }
        }

        /* Route 3: dynlib */
        uintptr_t dyn_addr = 0;
#if !defined(OBSCENE_HOST_BUILD)
        if (pid > 0 && krw_is_ready()) {
            dyn_addr = krw_dynlib_resolve_any(pid, name);
            if (dyn_addr < 0x10000UL || !obs_address_is_callable((const void *)dyn_addr)) {
                dyn_addr = 0;
            }
        }
#endif

        obs_report_measure("100-input/resolve", name, "dlsym", (uint64_t)(uintptr_t)dlsym_addr, "address");
        obs_report_measure("100-input/resolve", name, "kexport", (uint64_t)(uintptr_t)kexport_addr, "address");
        obs_report_measure("100-input/resolve", name, "dynlib", (uint64_t)dyn_addr, "address");

        void *best_addr = dlsym_addr != NULL ? dlsym_addr : (kexport_addr != NULL ? kexport_addr : (void *)dyn_addr);
        if (best_addr != NULL) {
            any_resolved++;
        }
        if (strncmp(name, "scePadVirtualDevice", 19) == 0) {
            obs_report_measure("100-input/virtual-device", name, "address",
                               (uint64_t)(uintptr_t)best_addr, "address");
            if (strcmp(name, "scePadVirtualDeviceAddDevice") == 0 && best_addr != NULL &&
                obs_address_is_callable(best_addr)) {
                int (*fn_add_dev)(void *) = (int (*)(void *))best_addr;
                uint8_t dev_param[256];
                for (size_t b = 0; b < sizeof(dev_param); b++) dev_param[b] = 0;
                obs_jmp_buf vbuf;
                int vsig = OBS_FAULT_ARM(&vbuf);
                int add_rc = -1;
                if (vsig == 0) {
                    add_rc = fn_add_dev(dev_param);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure("100-input/virtual-device", "scePadVirtualDeviceAddDevice", "rc",
                                   (uint64_t)(uint32_t)add_rc, "code");
                if (add_rc >= 0) {
                    int dev_handle = add_rc;
                    obs_report_measure("100-input/virtual-device", "scePadVirtualDeviceAddDevice", "handle",
                                       (uint64_t)(uint32_t)dev_handle, "handle");
                    const void *ins_addr = obs_module_symbol(OBS_HANDLE_SELF, "scePadVirtualDeviceInsertData");
                    if (ins_addr == NULL) ins_addr = obs_module_symbol(1, "scePadVirtualDeviceInsertData");
                    if (ins_addr != NULL && obs_address_is_callable(ins_addr)) {
                        uint8_t zero_buf[16];
                        for (size_t b = 0; b < sizeof(zero_buf); b++) zero_buf[b] = 0;
                        int (*fn_insert)(int, const void *, size_t) = (int (*)(int, const void *, size_t))ins_addr;
                        int ins_rc = -1;
                        vsig = OBS_FAULT_ARM(&vbuf);
                        if (vsig == 0) {
                            ins_rc = fn_insert(dev_handle, zero_buf, sizeof(zero_buf));
                            obs_fault_unregister();
                        } else {
                            obs_fault_unregister();
                        }
                        obs_report_measure("100-input/virtual-device", "scePadVirtualDeviceInsertData", "rc",
                                           (uint64_t)(uint32_t)ins_rc, "code");
                    }
                }
            }
        }
    }

    obs_fault_unregister();
    if (any_resolved > 0) {
        return obs_pass_value((uint64_t)any_resolved);
    }
    return obs_partial_value("libScePad sysmodule loaded; injection symbols not reached",
                             (uint64_t)(uint32_t)load_rc);
}

/* Mock Pad Data structure representing DualSense controller state */
typedef struct {
    uint32_t buttons;       /* Digital buttons bitmask */
    int8_t   left_stick_x;  /* -128 .. 127 */
    int8_t   left_stick_y;  /* -128 .. 127 */
    int8_t   right_stick_x; /* -128 .. 127 */
    int8_t   right_stick_y; /* -128 .. 127 */
    uint8_t  trigger_l2;    /* 0 .. 255 */
    uint8_t  trigger_r2;    /* 0 .. 255 */
    uint8_t  padding[2];
    uint32_t touch_id;
    uint16_t touch_x;
    uint16_t touch_y;
    uint8_t  reserved[32];
} obs_mock_pad_state_t;

static volatile int s_enable_input_mocking = 0;

static obs_result check_input_mocking_harness(void) {
    obs_report_measure("100-input/mocking-harness", "mocking", "enabled",
                       (uint64_t)s_enable_input_mocking, "bool");

    if (s_enable_input_mocking) {
        obs_mock_pad_state_t mock_pad;
        for (size_t i = 0; i < sizeof(mock_pad); i++) {
            ((uint8_t *)&mock_pad)[i] = 0;
        }

        /* Populate synthetic controller state: Cross + R1, sticks deflected, R2 half-trigger */
        mock_pad.buttons = (1u << 0) | (1u << 9);
        mock_pad.left_stick_x = 64;
        mock_pad.left_stick_y = -64;
        mock_pad.right_stick_x = -32;
        mock_pad.right_stick_y = 32;
        mock_pad.trigger_l2 = 0;
        mock_pad.trigger_r2 = 128;
        mock_pad.touch_id = 1;
        mock_pad.touch_x = 960;
        mock_pad.touch_y = 540;

        obs_report_buffer("100-input/mocking-harness", "mock_pad", "state",
                          (const unsigned char *)&mock_pad, sizeof(mock_pad));

        obs_jmp_buf buf;
        int sig = OBS_FAULT_ARM(&buf);
        if (sig != 0) {
            obs_fault_unregister();
            return obs_fail_code("input mocking faulted", (uint64_t)(uint32_t)sig);
        }

        const void *fn_insert_sym = obs_module_symbol(OBS_HANDLE_SELF, "scePadVirtualDeviceInsertData");
        if (fn_insert_sym == NULL) {
            fn_insert_sym = obs_module_symbol(1, "scePadVirtualDeviceInsertData");
        }
        if (fn_insert_sym != NULL && obs_address_is_callable(fn_insert_sym)) {
            int (*fn_insert)(int, const void *, size_t) = (int (*)(int, const void *, size_t))fn_insert_sym;
            int rc = fn_insert(0, &mock_pad, sizeof(mock_pad));
            obs_report_measure("100-input/mocking-harness", "scePadVirtualDeviceInsertData", "rc",
                               (uint64_t)(uint32_t)rc, "code");
        }

        obs_fault_unregister();
        return obs_pass();
    }

    return obs_pass();
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
    {"100-input/read-extent", "libScePad", "scePadReadState", OBS_CAP_INPUT, OBS_CAP_NONE,
     (const void *)&scePadReadState, check_pad_read_extent, OBS_FROM_ASSUMED},
    {"100-input/batched-read", "libScePad", "scePadRead", OBS_CAP_INPUT, OBS_CAP_NONE,
     (const void *)&scePadRead, check_pad_batched_read, OBS_FROM_ASSUMED},
    {"100-input/button-bits", "libScePad", "scePadReadState", OBS_CAP_INPUT, OBS_CAP_NONE,
     (const void *)&scePadReadState, check_pad_button_bits, OBS_FROM_ASSUMED},
    {"100-input/stick-trigger-range", "libScePad", "scePadReadState", OBS_CAP_INPUT,
     OBS_CAP_NONE, (const void *)&scePadReadState, check_pad_stick_trigger_range,
     OBS_FROM_ASSUMED},
    {"100-input/payload-injection", "libScePad", "scePadVirtualDeviceAddDevice",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_input_payload_injection,
     check_input_payload_injection, OBS_FROM_DERIVED},
    {"100-input/mocking-harness", "libScePad", "scePadVirtualDeviceInsertData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_input_mocking_harness,
     check_input_mocking_harness, OBS_FROM_DERIVED},
};

const obs_section obs_section_input = {
    "100-input",
    "Controller input",
    "Bringing up the controller subsystem, acquiring a pad, and DualSense features.",
    input_checks,
    OBS_COUNT(input_checks),
};
