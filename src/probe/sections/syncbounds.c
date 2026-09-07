/*
 * Semaphore and event-flag bounds: what the parameters mean, and how a bad handle is
 * refused.
 *
 * # Why this is a section of its own, beside 015-sync
 *
 * `015-sync` proves the primitives *work*: a semaphore keeps a count, an event flag
 * sets and clears a bit. This asks the questions a working primitive leaves open, the
 * ones a sibling emulator project is blocked on and cannot decide from the captures it
 * has (docs/backlog/022):
 *
 *   * what the `need` argument to a semaphore poll asks for - a count, or a flag;
 *   * what a bad handle returns, and whether the semaphore and event-flag families
 *     answer it the same way;
 *   * which bits of an event flag's wait-mode word are understood, when only the
 *     "all bits present" mode (AND) has ever been modelled.
 *
 * Each is a fact the platform holds and no document states. The section records what it
 * observes rather than asserting a value it would have had to invent (D008).
 *
 * # Poll, never wait - and that is the whole reason this is safe
 *
 * The obvious way to probe the semaphore is `sceKernelWaitSema`. It blocks: called on a
 * semaphore with no tokens it does not return until one arrives, and the third argument
 * that would bound the wait is a timeout whose *unit* is precisely one of the things
 * orbistoun records as unestablished (D540). A probe that blocks on a platform whose
 * semaphores are broken never comes back and loses every check behind it - an outcome
 * this suite has already paid for twice.
 *
 * `sceKernelPollSema` asks the same question about the `need` count and the bad handle
 * and always returns. It is the safe counterpart, exactly as `sceKernelPollEventFlag`
 * is the safe counterpart of `sceKernelWaitEventFlag` in 015-sync. So `sceKernelWaitSema`
 * is deliberately *not* called here: its only measurable behaviour beyond Poll's is the
 * timeout, and the timeout cannot be measured without a bounded wait, which needs the
 * unit this probe would be trying to discover. That one question is left for a hardware
 * session that can approach it with a known unit; everything else about the semaphore's
 * bounds is settled here without ever risking the hang. (See docs/decisions/D321.)
 *
 * # Nothing is left behind
 *
 * Every semaphore and every flag created is deleted on every path, including the
 * failures, so a check that runs after this one meets the machine this one found.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Two bits far enough apart that a platform confusing a pattern with a count produces a
 * visibly wrong number rather than an off-by-one that reads as success. Same choice as
 * 015-sync, for the same reason. */
#define OBS_SB_BIT_A 0x0000000000000002ull
#define OBS_SB_BIT_B 0x0000000100000000ull

/* Invalid semaphore handles. The handle is an `int`; zero is "no semaphore" and -1 is
 * the conventional invalid descriptor. Neither can block - there is nothing to wait on -
 * so this is safe where a wait would not be. */
#define OBS_SB_SEMA_NULL 0
#define OBS_SB_SEMA_BAD (-1)

/* ---- semaphores ----------------------------------------------------------- */

/* What does a poll on a handle that names no semaphore return?
 *
 * orbistoun measured ESRCH (0x80020003) for the event-flag family and carries a
 * placeholder (0x7fff0003) for the semaphore family, never having measured it - so
 * whether the two families agree is unestablished (D540). This records the semaphore
 * side; `event-flag-bad-handle` below records the other, and the two `err` records
 * answer the agreement question by inspection, without either check having to reach
 * into the other family's symbols. */
static obs_result check_sema_bad_handle(void) {
    int null_rc = sceKernelPollSema(OBS_SB_SEMA_NULL, 1);
    obs_report_error_code("libkernel", "sceKernelPollSema", "handle 0",
                          (uint64_t)(uint32_t)null_rc);
    int bad_rc = sceKernelPollSema(OBS_SB_SEMA_BAD, 1);
    obs_report_error_code("libkernel", "sceKernelPollSema", "handle -1",
                          (uint64_t)(uint32_t)bad_rc);

    /* A poll on a semaphore that does not exist must not hand out a token. A platform
     * that returns success here would let a guest wait on nothing and proceed. */
    if (null_rc == 0 || bad_rc == 0) {
        return obs_fail("a poll on an invalid semaphore handle reported success");
    }
    /* The value is the finding, not the verdict: the code a bad handle returns is what
     * the sibling project needs, and comparing it against the event-flag family's is the
     * open question this feeds. */
    return obs_pass_value((uint64_t)(uint32_t)bad_rc);
}

/* Is `need` a count or a flag?
 *
 * orbistoun models the count as one-unit-and-block; what a `need` above one asks for is
 * unestablished (D540). Signal three tokens in, then poll for two and two again: under a
 * count, the first succeeds and leaves one, so the second fails; under a flag, both
 * behave the same. A `need` of zero is polled last, because "does asking for nothing
 * always succeed" is its own parameter-handling fact and one no document settles. */
static obs_result check_sema_count(void) {
    OBS_REQUIRE(&sceKernelCreateSema, &sceKernelDeleteSema, &sceKernelSignalSema);

    int sema = 0;
    int rc = sceKernelCreateSema(&sema, "obscene-bounds", 0, 0, 8, NULL);
    if (rc != 0) {
        return obs_fail_code("a semaphore could not be created",
                             (uint64_t)(uint32_t)rc);
    }

    rc = sceKernelSignalSema(sema, 3);
    if (rc != 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail_code("three tokens could not be signalled",
                             (uint64_t)(uint32_t)rc);
    }

    int take_two_a = sceKernelPollSema(sema, 2);
    obs_report_measure("016-syncbounds/sema-count", "sceKernelPollSema", "need-2-of-3",
                       (uint64_t)(int64_t)take_two_a, "code");
    int take_two_b = sceKernelPollSema(sema, 2);
    obs_report_measure("016-syncbounds/sema-count", "sceKernelPollSema",
                       "need-2-of-1-left", (uint64_t)(int64_t)take_two_b, "code");
    int take_one = sceKernelPollSema(sema, 1);
    obs_report_measure("016-syncbounds/sema-count", "sceKernelPollSema",
                       "need-1-of-1-left", (uint64_t)(int64_t)take_one, "code");
    int need_zero = sceKernelPollSema(sema, 0);
    obs_report_measure("016-syncbounds/sema-count", "sceKernelPollSema",
                       "need-0-of-empty", (uint64_t)(int64_t)need_zero, "code");

    (void)sceKernelDeleteSema(sema);

    /* The count contract: two came out of three, a second two did not (only one was
     * left), and the last one did. A platform treating `need` as a flag lets the second
     * poll for two succeed, because it never subtracted the first two. */
    if (take_two_a != 0) {
        return obs_fail_code("a poll for two of three tokens was refused",
                             (uint64_t)(int64_t)take_two_a);
    }
    if (take_two_b == 0) {
        return obs_fail("a poll for two tokens succeeded with only one left: need is "
                        "not a count");
    }
    if (take_one != 0) {
        return obs_fail_code("the last remaining token could not be taken",
                             (uint64_t)(int64_t)take_one);
    }
    return obs_pass_value((uint64_t)(int64_t)need_zero);
}

/* ---- event flags ---------------------------------------------------------- */

/* What does a poll on a null event flag return?
 *
 * The other half of the bad-handle question above. `015-sync/event-flag-rejects-bad-handle`
 * reports the verdict; this records the *code*, because the code is what settles whether
 * the two families agree. */
static obs_result check_event_flag_bad_handle(void) {
    uint64_t pattern = 0;
    int rc = sceKernelPollEventFlag(NULL, OBS_SB_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    obs_report_error_code("libkernel", "sceKernelPollEventFlag", "null handle",
                          (uint64_t)(uint32_t)rc);
    if (rc == 0) {
        return obs_partial("polling a null event flag reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* Which wait-mode bits does the platform understand?
 *
 * Only bit 0 - AND, "every named bit must be set" - has ever been modelled; what the
 * rest of the word selects is unestablished (D540). The discriminator is one flag with
 * one of two bits set: under AND, a poll for both bits fails; under an OR-shaped mode it
 * would succeed on the one that is present. So each candidate mode is asked exactly that
 * question, and the answer says whether the mode is AND-shaped, OR-shaped, or refused -
 * without this program having to name a constant it cannot confirm.
 *
 * A poll never blocks whatever the mode, so sweeping unknown mode values is safe. */
#define OBS_SB_MODE_CANDIDATES 4
static const uint32_t obs_sb_modes[OBS_SB_MODE_CANDIDATES] = {
    0x00u, /* no bits: does the platform default, reject, or treat as AND? */
    0x01u, /* AND, the one mode known to be modelled - the control */
    0x02u, /* the natural next bit, a candidate for OR */
    0x11u, /* AND with a high nibble set, to see whether unrelated bits are ignored */
};
static const char *const obs_sb_mode_names[OBS_SB_MODE_CANDIDATES] = {
    "mode-0x00-both-of-one",
    "mode-0x01-both-of-one",
    "mode-0x02-both-of-one",
    "mode-0x11-both-of-one",
};

static obs_result check_event_flag_waitmode(void) {
    OBS_REQUIRE(&sceKernelCreateEventFlag, &sceKernelDeleteEventFlag,
                &sceKernelSetEventFlag);

    SceKernelEventFlag flag = 0;
    int rc = sceKernelCreateEventFlag(&flag, "obscene-waitmode",
                                      OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0, NULL);
    if (rc != 0) {
        return obs_fail_code("an event flag could not be created",
                             (uint64_t)(uint32_t)rc);
    }
    rc = sceKernelSetEventFlag(flag, OBS_SB_BIT_A);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a bit could not be set", (uint64_t)(uint32_t)rc);
    }

    /* The control, under the one mode known to work: A is present, B is not, and both
     * together are not - which is the AND contract stated in three answers. Measured
     * before the sweep so a platform that fails it is caught before its mode answers are
     * trusted. */
    uint64_t pat = 0;
    int present = sceKernelPollEventFlag(flag, OBS_SB_BIT_A, OBS_EVF_WAITMODE_AND, &pat);
    int absent = sceKernelPollEventFlag(flag, OBS_SB_BIT_B, OBS_EVF_WAITMODE_AND, &pat);
    int both = sceKernelPollEventFlag(flag, OBS_SB_BIT_A | OBS_SB_BIT_B,
                                      OBS_EVF_WAITMODE_AND, &pat);

    /* The sweep: for each candidate mode, poll for both bits when only one is set. A
     * success means the mode is not AND (one present bit satisfied it); a distinct
     * refusal means it is AND-shaped or unknown. The raw code is recorded so the analysis
     * can tell "not satisfied" from "mode rejected". */
    for (int i = 0; i < OBS_SB_MODE_CANDIDATES; i++) {
        pat = 0;
        int mode_rc = sceKernelPollEventFlag(flag, OBS_SB_BIT_A | OBS_SB_BIT_B,
                                             obs_sb_modes[i], &pat);
        obs_report_measure("016-syncbounds/event-flag-waitmode",
                           "sceKernelPollEventFlag", obs_sb_mode_names[i],
                           (uint64_t)(int64_t)mode_rc, "code");
    }

    (void)sceKernelDeleteEventFlag(flag);

    if (present != 0) {
        return obs_fail_code("a bit that was set did not poll as present under AND",
                             (uint64_t)(uint32_t)present);
    }
    if (absent == 0) {
        return obs_fail("a bit that was never set polled as present under AND");
    }
    if (both == 0) {
        return obs_fail("AND was satisfied with only one of two bits set");
    }
    return obs_pass();
}

static const obs_check syncbounds_checks[] = {
    {"016-syncbounds/sema-bad-handle", "libkernel", "sceKernelPollSema", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelPollSema, check_sema_bad_handle,
     OBS_FROM_ASSUMED},
    {"016-syncbounds/sema-count", "libkernel", "sceKernelPollSema", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelPollSema, check_sema_count, OBS_FROM_ASSUMED},
    {"016-syncbounds/event-flag-bad-handle", "libkernel", "sceKernelPollEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelPollEventFlag,
     check_event_flag_bad_handle, OBS_FROM_ASSUMED},
    {"016-syncbounds/event-flag-waitmode", "libkernel", "sceKernelPollEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelPollEventFlag,
     check_event_flag_waitmode, OBS_FROM_ASSUMED},
};

const obs_section obs_section_syncbounds = {
    "016-syncbounds",
    "Semaphore and event-flag bounds",
    "What the semaphore poll's count argument means, what a bad handle returns in each "
    "family, and which event-flag wait-mode bits the platform understands. Records what "
    "it observes; never waits, so it is safe on a platform whose primitives are broken.",
    syncbounds_checks,
    OBS_COUNT(syncbounds_checks),
};
