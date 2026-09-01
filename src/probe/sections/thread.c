/*
 * Threads.
 *
 * Placed above memory because a thread needs a stack, and below everything else
 * because the presentation subsystems all run work on threads they create
 * themselves. A platform that cannot start a thread cannot reach a frame.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
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

static const obs_check thread_checks[] = {
    {"030-thread/self", "libkernel", "scePthreadSelf", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadSelf, check_self, OBS_FROM_SPEC},
    {"030-thread/create", "libkernel", "scePthreadCreate", OBS_CAP_NONE, OBS_CAP_THREAD,
     (const void *)&scePthreadCreate, check_create, OBS_FROM_SPEC},
    {"030-thread/join", "libkernel", "scePthreadJoin", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadJoin, check_join, OBS_FROM_SPEC},
};

const obs_section obs_section_thread = {
    "030-thread",
    "Threads",
    "Creating a thread, proving its body actually ran, and joining it back.",
    thread_checks,
    OBS_COUNT(thread_checks),
};
