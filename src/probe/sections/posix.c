/*
 * The platform under its POSIX names.
 *
 * `libScePosix` exports POSIX with a `posix_` prefix, and it is worth a section of its
 * own for two reasons that no other library here has.
 *
 * # The expectations are settled
 *
 * POSIX says what these do. Every check in this file is OBS_FROM_SPEC - answerable from
 * a document anyone can consult, rather than from this project's own reasoning.
 *
 * That distinction is not academic. Most of this suite is OBS_FROM_ASSUMED, and an
 * emulator implemented to make an assumed check pass has only been made to agree with
 * us. Both can be wrong together, and the report would say otherwise. A spec check
 * cannot fail that way, because the authority is outside both projects.
 *
 * # They are a second spelling of functions already checked
 *
 * `scePthreadRwlockTryrdlock` and `posix_pthread_rwlock_tryrdlock` should be one
 * implementation behind two names. So should the rest of the family. That makes a
 * comparison possible that nothing else in this program can do: call both, and report
 * when they disagree.
 *
 * A divergence there is a real fault and an invisible one. Each path passes its own
 * checks; only holding them against each other shows it. `017-posix/spellings-agree`
 * exists for exactly that, and it is the only check in this suite whose expected value
 * comes from the platform rather than from a document - it asks for consistency, not
 * for a particular answer.
 *
 * # What is deliberately absent
 *
 * Everything needing a struct layout. `posix_nanosleep` and `posix_clock_gettime` take
 * a `timespec`; `posix_mmap` and the whole `sys_*` socket family take more. A wrong
 * layout produces a call that succeeds and does the wrong thing, which is worse than no
 * check at all (D008). Those are censused.
 *
 * `posix_raise` is absent for a different reason: it works. Raising a signal in a probe
 * whose value is that it keeps running to the end is not a trade worth making.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/* Large enough for any `sigset_t` this could meet - 16 bytes on the BSD the target
 * derives from, 128 on Linux where the host build runs. Nothing here assumes which,
 * or what is inside it: the set is only ever read back through the platform's own
 * `ismember`, so the layout stays the platform's business.
 *
 * Aligned as strictly as anything it might contain. A signal set is words, and handing
 * a library a misaligned one is a fault that would look like the library's bug. */
typedef union {
    uint64_t alignment;
    unsigned char bytes[128];
} obs_sigset;

/* 1 and 2 are SIGHUP and SIGINT on every system in this family, and neither is ever
 * raised here - the set is a data structure and these are just indices into it. Two are
 * needed rather than one so that adding a signal can be shown not to add every signal. */
#define OBS_SIGNAL_A 1
#define OBS_SIGNAL_B 2

static obs_result check_signal_sets(void) {
    OBS_REQUIRE(&posix_sigaddset, &posix_sigdelset, &posix_sigfillset, &posix_sigismember);
    obs_sigset set;
    for (size_t i = 0; i < sizeof(set.bytes); i++) {
        set.bytes[i] = 0xA5;
    }

    if (posix_sigemptyset(&set) != 0) {
        return obs_fail("an empty signal set could not be made");
    }
    /* The buffer was filled with a pattern first, so this also catches an
     * implementation that reports success and writes nothing: the stale bytes would
     * still read as members. */
    if (posix_sigismember(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal was already in a set said to be empty");
    }

    if (posix_sigaddset(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal could not be added to a set");
    }
    if (posix_sigismember(&set, OBS_SIGNAL_A) != 1) {
        return obs_fail("a signal that was added is not in the set");
    }
    /* Adding one must not add the others. An implementation that fills the set on any
     * add passes every check above. */
    if (posix_sigismember(&set, OBS_SIGNAL_B) != 0) {
        return obs_fail("adding one signal added another");
    }

    if (posix_sigdelset(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal could not be removed from a set");
    }
    if (posix_sigismember(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal that was removed is still in the set");
    }

    if (posix_sigfillset(&set) != 0) {
        return obs_fail("a full signal set could not be made");
    }
    if (posix_sigismember(&set, OBS_SIGNAL_A) != 1 ||
        posix_sigismember(&set, OBS_SIGNAL_B) != 1) {
        return obs_fail("a set said to be full is missing a signal");
    }
    /* Empty after full, so the last call cannot be the one that happens to work on a
     * freshly zeroed buffer. */
    if (posix_sigemptyset(&set) != 0) {
        return obs_fail("a full set could not be emptied");
    }
    if (posix_sigismember(&set, OBS_SIGNAL_B) != 0) {
        return obs_fail("emptying a full set left a signal in it");
    }
    return obs_pass();
}

static obs_result check_page_size(void) {
    int size = posix_getpagesize();
    if (size <= 0) {
        return obs_fail_code("the page size is not positive", (uint64_t)(uint32_t)size);
    }
    /* A power of two. Not a guess at which one - this target is documented as 16KiB
     * where the architecture's minimum is 4KiB, and asserting either would be
     * inventing a specification. That it is a power of two is not an assumption; a
     * page size that is not one cannot be used to page anything. */
    if ((size & (size - 1)) != 0) {
        return obs_fail_code("the page size is not a power of two",
                             (uint64_t)(uint32_t)size);
    }
    return obs_pass_value((uint64_t)(uint32_t)size);
}

static obs_result check_short_sleep(void) {
    /* A millisecond. Short enough that a suite of several hundred checks does not
     * notice it, long enough to be a real request rather than a rounding error.
     *
     * Blocking on purpose is allowed here, as it is in 050-time/usleep, because the
     * duration is ours rather than the platform's: nothing it waits for can fail to
     * arrive. A lock or a read has no such bound, which is why those are written as the
     * try form or not at all.
     *
     * `sceKernelUsleep` is the vendor spelling of this same call, so the two are
     * another candidate for the comparison in `spellings-agree`. Not done here: that
     * check compares outcomes, and two sleeps both returning zero would agree without
     * either having slept. */
    if (posix_usleep(1000u) != 0) {
        return obs_fail("a one-millisecond sleep reported failure");
    }
    /* # What a pass here does not establish
     *
     * That the sleep happened. `usleep` returns zero for any valid request, so a stub
     * returning zero to everything passes this, and the check cannot tell them apart.
     * That is the same defect D056 found in the inverse-trigonometry check, and it is
     * stated rather than fixed because the two available fixes are both worse.
     *
     * A responsiveness probe would compare the return values of two sleeps - which are
     * both zero when the function is *correct*, so it would report a working
     * implementation as silent. That is the `fmod(7, 4)` mistake with the inputs chosen
     * deliberately instead of by accident.
     *
     * A clock comparison would need a time source this check has not established. The
     * one the vendor section uses counts process time, which a sleeping thread may
     * legitimately not accrue - see 050-time/usleep, which for that reason only checks
     * that the clock did not go backwards.
     *
     * So a pass here means "the call accepted a valid request and reported success",
     * which is weaker than a pass elsewhere in this section and is worth knowing when
     * reading the sheet. The value is the fail: an implementation that refuses a
     * one-millisecond sleep is broken in a way worth seeing. */
    return obs_pass();
}

static obs_result check_rwlock(void) {
    OBS_REQUIRE(&posix_pthread_rwlock_destroy,
                &posix_pthread_rwlock_tryrdlock,
                &posix_pthread_rwlock_trywrlock,
                &posix_pthread_rwlock_unlock);
    ObsPosixRwlock lock = 0;
    if (posix_pthread_rwlock_init(&lock, 0) != 0) {
        return obs_fail("a POSIX read/write lock could not be created");
    }
    /* The same shape as 015-sync's check of the vendor spelling, deliberately: two
     * readers admitted at once, a writer refused while they hold it, and admitted once
     * they are gone. A platform that fails the second reader has built a mutex. */
    if (posix_pthread_rwlock_tryrdlock(&lock) != 0) {
        (void)posix_pthread_rwlock_destroy(&lock);
        return obs_fail("a fresh lock could not be taken for reading");
    }
    if (posix_pthread_rwlock_tryrdlock(&lock) != 0) {
        (void)posix_pthread_rwlock_unlock(&lock);
        (void)posix_pthread_rwlock_destroy(&lock);
        return obs_fail("a second reader was refused");
    }
    if (posix_pthread_rwlock_trywrlock(&lock) == 0) {
        (void)posix_pthread_rwlock_unlock(&lock);
        (void)posix_pthread_rwlock_unlock(&lock);
        (void)posix_pthread_rwlock_unlock(&lock);
        (void)posix_pthread_rwlock_destroy(&lock);
        return obs_fail("a writer was let in while readers held the lock");
    }
    (void)posix_pthread_rwlock_unlock(&lock);
    (void)posix_pthread_rwlock_unlock(&lock);

    if (posix_pthread_rwlock_trywrlock(&lock) != 0) {
        (void)posix_pthread_rwlock_destroy(&lock);
        return obs_fail("a writer was refused an unheld lock");
    }
    (void)posix_pthread_rwlock_unlock(&lock);
    if (posix_pthread_rwlock_destroy(&lock) != 0) {
        return obs_fail("a lock could not be destroyed");
    }
    return obs_pass();
}

static obs_result check_spellings_agree(void) {
    OBS_REQUIRE(&posix_pthread_rwlock_destroy,
                &posix_pthread_rwlock_init,
                &posix_pthread_rwlock_trywrlock,
                &posix_pthread_rwlock_unlock);
    /* The check this whole section exists for.
     *
     * These are two names for one thing. Nothing here asks for a particular answer -
     * it asks the two paths the same question and reports when they differ, which is a
     * fault whatever the right answer turns out to be.
     *
     * It cannot be written the usual way. An expected value would make it a third
     * opinion; the point is that the platform must agree with itself. */
    /* Both symbols have to be checked, and only one of them is this check's own.
     *
     * The harness skips a check whose declared symbol is null, because jumping to zero
     * takes the process down and loses everything behind it. A check that calls a
     * second library gets no such protection - the guard covers the symbol in the
     * table, not whatever else the body reaches for.
     *
     * This is not hypothetical. The first version of this check went straight into a
     * null `scePthreadRwlockInit` on the host build and took the process with it; the
     * `try` record with no `res` named it, which is what that invariant is for. Every
     * platform declaration is weak, so the address is the test. */
    if ((const void *)&scePthreadRwlockInit == 0 ||
        (const void *)&scePthreadRwlockTryrdlock == 0 ||
        (const void *)&scePthreadRwlockTrywrlock == 0 ||
        (const void *)&scePthreadRwlockUnlock == 0 ||
        (const void *)&scePthreadRwlockDestroy == 0) {
        return obs_skip("the vendor spelling is absent, so there is nothing to compare");
    }

    ScePthreadRwlock vendor = 0;
    ObsPosixRwlock posix = 0;
    int vendor_rc = scePthreadRwlockInit(&vendor, NULL, "obscene-compare");
    int posix_rc = posix_pthread_rwlock_init(&posix, 0);
    if ((vendor_rc == 0) != (posix_rc == 0)) {
        if (vendor_rc == 0) {
            (void)scePthreadRwlockDestroy(&vendor);
        }
        if (posix_rc == 0) {
            (void)posix_pthread_rwlock_destroy(&posix);
        }
        return obs_fail("one spelling created a lock and the other refused");
    }
    if (vendor_rc != 0) {
        return obs_skip("neither spelling could create a lock to compare");
    }

    /* Second reader on both. Whether it is admitted is the platform's business; that
     * the two answers match is not. */
    (void)scePthreadRwlockTryrdlock(&vendor);
    (void)posix_pthread_rwlock_tryrdlock(&posix);
    int vendor_second = scePthreadRwlockTryrdlock(&vendor);
    int posix_second = posix_pthread_rwlock_tryrdlock(&posix);

    /* Compared as outcomes rather than as codes: the two libraries are entitled to
     * report the same refusal with different numbers, and calling that a disagreement
     * would be this check inventing a requirement. */
    int disagree_reader = (vendor_second == 0) != (posix_second == 0);

    if (vendor_second == 0) {
        (void)scePthreadRwlockUnlock(&vendor);
    }
    if (posix_second == 0) {
        (void)posix_pthread_rwlock_unlock(&posix);
    }

    int vendor_writer = scePthreadRwlockTrywrlock(&vendor);
    int posix_writer = posix_pthread_rwlock_trywrlock(&posix);
    int disagree_writer = (vendor_writer == 0) != (posix_writer == 0);
    if (vendor_writer == 0) {
        (void)scePthreadRwlockUnlock(&vendor);
    }
    if (posix_writer == 0) {
        (void)posix_pthread_rwlock_unlock(&posix);
    }

    (void)scePthreadRwlockUnlock(&vendor);
    (void)posix_pthread_rwlock_unlock(&posix);
    (void)scePthreadRwlockDestroy(&vendor);
    (void)posix_pthread_rwlock_destroy(&posix);

    if (disagree_reader) {
        return obs_fail("the two spellings disagree about a second reader");
    }
    if (disagree_writer) {
        return obs_fail("the two spellings disagree about admitting a writer");
    }
    return obs_pass();
}

static const obs_check posix_checks[] = {
    {"017-posix/page-size", "libScePosix", "posix_getpagesize", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&posix_getpagesize, check_page_size, OBS_FROM_SPEC},
    {"017-posix/signal-sets", "libScePosix", "posix_sigemptyset", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&posix_sigemptyset, check_signal_sets, OBS_FROM_SPEC},
    {"017-posix/short-sleep", "libScePosix", "posix_usleep", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&posix_usleep, check_short_sleep, OBS_FROM_SPEC},
    {"017-posix/rwlock", "libScePosix", "posix_pthread_rwlock_init", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&posix_pthread_rwlock_init, check_rwlock,
     OBS_FROM_SPEC},
    /* Assumed, not spec: no document says the two libraries must be one
     * implementation. It is a strong expectation and it is still this project's. */
    {"017-posix/spellings-agree", "libScePosix", "posix_pthread_rwlock_tryrdlock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&posix_pthread_rwlock_tryrdlock,
     check_spellings_agree, OBS_FROM_ASSUMED},
};

const obs_section obs_section_posix = {
    "017-posix",
    "The platform under its POSIX names",
    "libScePosix exports POSIX with a prefix. Settled by a public document, and a "
    "second spelling of functions checked elsewhere - so the two can be held against "
    "each other.",
    posix_checks,
    OBS_COUNT(posix_checks),
};
