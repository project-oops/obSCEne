/*
 * Recording: the path a console already uses to encode its own output.
 *
 * # Why this section exists
 *
 * The console encodes video continuously for its share feature, in hardware, and
 * `libSceVideoRecording` is the interface that drives it. Fourteen symbols, all of them
 * named in the corpus, none of them exercised until now.
 *
 * That makes this the cheapest route to encoded frames off a target: not a capture
 * pipeline written from nothing, but the one already running.
 *
 * # Why every check here is a refusal
 *
 * The same reason the video section gives, and more strongly. These functions take
 * structures whose layouts nobody here is confident about, and **their arities are
 * assumed** - the corpus names a symbol, it does not say how many arguments it takes.
 * Guessing wrong puts garbage in an argument register, and if the callee dereferences it
 * the crash lands nowhere near the mistake.
 *
 * So this section calls only the functions whose arguments are plausibly integers, with
 * values that are obviously invalid, and reads the refusal. That establishes the function
 * exists, is reachable, validates its input and returns a code - the exact set a stub
 * returning a constant gets wrong - and it does so without knowing one field of one struct.
 *
 * # What is deliberately not here
 *
 * `sceVideoRecordingOpen`, `Open2` and `SetInfo` take pointers. Calling those with a guess
 * is worth doing and does not belong in a suite: **a check that kills the probe takes the
 * whole report with it**, including every check that had already passed.
 *
 * That work belongs on the protocol, where `ack` is flushed before the call and a fault is
 * recorded as `died` rather than lost - one question, one recorded answer, and a re-send
 * afterwards. When a sequence has been established that way, it can come back here as a
 * check, which is what this suite is for: keeping what is known, not finding it.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/*
 * Declared here rather than in platform.h, and that is not a style choice.
 *
 * These names are in the mined corpus, and the census declares every corpus name as an
 * opaque `extern const char` so it can take an address without calling anything. A second
 * declaration as a function in platform.h is a redefinition in the one translation unit
 * that sees both, and the build says so.
 *
 * So a corpus name that a section wants to *call* is declared where only that section sees
 * it. `src/imports.c` already names this as one of the three kinds of import and is where
 * the library mapping goes.
 *
 * **None of these arities is confirmed.** A corpus names a symbol; it does not say how many
 * arguments the symbol takes. Declaring too few leaves the extra registers holding whatever
 * was there, which is harmless until the callee reads one as a pointer - so only the ones
 * plausibly taking integers are here, and the pointer-taking half of the library is absent
 * on purpose.
 */

OBS_WEAK int sceVideoRecordingQueryMemSize(int mode);
OBS_WEAK int sceVideoRecordingClose(int handle);
OBS_WEAK int sceVideoRecordingStop(int handle);
OBS_WEAK int sceVideoRecordingGetStatus(int handle);

/* ---- 105-record ------------------------------------------------------------ */

/*
 * A handle no recording session can have.
 *
 * The video section uses OBS_HANDLE_INVALID for the same purpose against a different
 * subsystem; reusing it keeps one idea of "obviously not a handle" across the program.
 */

static obs_result check_record_close_rejects_bad_handle(void) {
    int rc = sceVideoRecordingClose(OBS_HANDLE_INVALID);
    if (rc == 0) {
        /* Partial rather than a failure: it answered, and what it did with the invalid
         * handle is unknown. A zero here means either it validates nothing or it treats
         * the value as meaningful, and this check cannot tell which. */
        return obs_partial("closing an invalid recording handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_record_stop_rejects_bad_handle(void) {
    int rc = sceVideoRecordingStop(OBS_HANDLE_INVALID);
    if (rc == 0) {
        return obs_partial("stopping an invalid recording handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static obs_result check_record_status_rejects_bad_handle(void) {
    int rc = sceVideoRecordingGetStatus(OBS_HANDLE_INVALID);
    if (rc == 0) {
        return obs_partial("a status for an invalid recording handle reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/*
 * The one call here that is expected to succeed.
 *
 * A memory-size query is the safest question in the whole subsystem: it takes a mode and
 * returns a size, touching nothing. **It is also the first thing that leaks a structure** -
 * a size is how much room a caller has to provide, which is a fact about a layout obtained
 * without dereferencing anything.
 *
 * A negative answer is not a failure. It means the mode was not one this platform offers,
 * which is itself the sort of thing worth having written down.
 */
static obs_result check_record_query_mem_size(void) {
    OBS_REQUIRE(&sceVideoRecordingQueryMemSize);
    int rc = sceVideoRecordingQueryMemSize(0);
    if (rc < 0) {
        return obs_partial_value("the mode this asked about was refused",
                                 (uint64_t)(uint32_t)rc);
    }
    if (rc == 0) {
        return obs_partial("a recording needs no memory, which is not a plausible answer");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

static const obs_check record_checks[] = {
    {"105-record/query-mem-size", "libSceVideoRecording", "sceVideoRecordingQueryMemSize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoRecordingQueryMemSize,
     check_record_query_mem_size, OBS_FROM_ASSUMED},
    {"105-record/close-rejects-bad-handle", "libSceVideoRecording",
     "sceVideoRecordingClose", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceVideoRecordingClose, check_record_close_rejects_bad_handle,
     OBS_FROM_ASSUMED},
    {"105-record/stop-rejects-bad-handle", "libSceVideoRecording", "sceVideoRecordingStop",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoRecordingStop,
     check_record_stop_rejects_bad_handle, OBS_FROM_ASSUMED},
    {"105-record/status-rejects-bad-handle", "libSceVideoRecording",
     "sceVideoRecordingGetStatus", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceVideoRecordingGetStatus, check_record_status_rejects_bad_handle,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_record = {
    "105-record",
    "Video recording",
    "The encoder the console already drives for its own recordings.",
    record_checks,
    OBS_COUNT(record_checks),
};
