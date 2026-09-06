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
#include "obscene/runtime.h"
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
 * needed rather than one so that adding a signal can be shown not to add every signal.
 */
#define OBS_SIGNAL_A 1
#define OBS_SIGNAL_B 2

#if !defined(OBSCENE_HOST_BUILD)
typedef void *ObsPosixRwlock;
#endif

typedef int (*fn_posix_getpagesize_t)(void);
typedef int (*fn_posix_sigemptyset_t)(void *);
typedef int (*fn_posix_sigfillset_t)(void *);
typedef int (*fn_posix_sigaddset_t)(void *, int);
typedef int (*fn_posix_sigdelset_t)(void *, int);
typedef int (*fn_posix_sigismember_t)(const void *, int);
typedef int (*fn_posix_usleep_t)(unsigned int);
typedef int (*fn_posix_rwlock_init_t)(ObsPosixRwlock *, const void *);
typedef int (*fn_posix_rwlock_destroy_t)(ObsPosixRwlock *);
typedef int (*fn_posix_rwlock_tryrdlock_t)(ObsPosixRwlock *);
typedef int (*fn_posix_rwlock_trywrlock_t)(ObsPosixRwlock *);
typedef int (*fn_posix_rwlock_unlock_t)(ObsPosixRwlock *);

#if !defined(OBSCENE_HOST_BUILD)
static int s_posix_handle = -2;

static int obs_posix_handle(void) {
    if (s_posix_handle != -2) {
        return s_posix_handle;
    }
    s_posix_handle = obs_module_open("libScePosix");
    if (s_posix_handle < 0) {
        s_posix_handle = obs_module_open("libScePosixForWebKit");
    }
    return s_posix_handle;
}
#endif

static void *obs_posix_symbol(const char *name) {
#if defined(OBSCENE_HOST_BUILD)
    if (obs_strcmp(name, "posix_getpagesize") == 0) return (void *)&posix_getpagesize;
    if (obs_strcmp(name, "posix_sigemptyset") == 0) return (void *)&posix_sigemptyset;
    if (obs_strcmp(name, "posix_sigfillset") == 0) return (void *)&posix_sigfillset;
    if (obs_strcmp(name, "posix_sigaddset") == 0) return (void *)&posix_sigaddset;
    if (obs_strcmp(name, "posix_sigdelset") == 0) return (void *)&posix_sigdelset;
    if (obs_strcmp(name, "posix_sigismember") == 0) return (void *)&posix_sigismember;
    if (obs_strcmp(name, "posix_usleep") == 0) return (void *)&posix_usleep;
    if (obs_strcmp(name, "posix_pthread_rwlock_init") == 0) return (void *)&posix_pthread_rwlock_init;
    if (obs_strcmp(name, "posix_pthread_rwlock_destroy") == 0) return (void *)&posix_pthread_rwlock_destroy;
    if (obs_strcmp(name, "posix_pthread_rwlock_tryrdlock") == 0) return (void *)&posix_pthread_rwlock_tryrdlock;
    if (obs_strcmp(name, "posix_pthread_rwlock_trywrlock") == 0) return (void *)&posix_pthread_rwlock_trywrlock;
    if (obs_strcmp(name, "posix_pthread_rwlock_unlock") == 0) return (void *)&posix_pthread_rwlock_unlock;
    return NULL;
#else
    int h = obs_posix_handle();
    if (h < 0) {
        return NULL;
    }
    return (void *)obs_module_symbol(h, name);
#endif
}

static obs_result check_signal_sets(void) {
    fn_posix_sigemptyset_t fn_empty = (fn_posix_sigemptyset_t)obs_posix_symbol("posix_sigemptyset");
    fn_posix_sigfillset_t fn_fill = (fn_posix_sigfillset_t)obs_posix_symbol("posix_sigfillset");
    fn_posix_sigaddset_t fn_add = (fn_posix_sigaddset_t)obs_posix_symbol("posix_sigaddset");
    fn_posix_sigdelset_t fn_del = (fn_posix_sigdelset_t)obs_posix_symbol("posix_sigdelset");
    fn_posix_sigismember_t fn_member = (fn_posix_sigismember_t)obs_posix_symbol("posix_sigismember");
    if (fn_empty == NULL || fn_fill == NULL || fn_add == NULL || fn_del == NULL || fn_member == NULL) {
        return obs_skip("libScePosix is not available in this sandbox");
    }
    obs_sigset set;
    for (size_t i = 0; i < sizeof(set.bytes); i++) {
        set.bytes[i] = 0xA5;
    }

    if (fn_empty(&set) != 0) {
        return obs_fail("an empty signal set could not be made");
    }
    /* The buffer was filled with a pattern first, so this also catches an
     * implementation that reports success and writes nothing: the stale bytes would
     * still read as members. */
    if (fn_member(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal was already in a set said to be empty");
    }

    if (fn_add(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal could not be added to a set");
    }
    if (fn_member(&set, OBS_SIGNAL_A) != 1) {
        return obs_fail("a signal that was added is not in the set");
    }
    /* Adding one must not add the others. An implementation that fills the set on any
     * add passes every check above. */
    if (fn_member(&set, OBS_SIGNAL_B) != 0) {
        return obs_fail("adding one signal added another");
    }

    if (fn_del(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal could not be removed from a set");
    }
    if (fn_member(&set, OBS_SIGNAL_A) != 0) {
        return obs_fail("a signal that was removed is still in the set");
    }

    if (fn_fill(&set) != 0) {
        return obs_fail("a full signal set could not be made");
    }
    if (fn_member(&set, OBS_SIGNAL_A) != 1 ||
        fn_member(&set, OBS_SIGNAL_B) != 1) {
        return obs_fail("a set said to be full is missing a signal");
    }
    /* Empty after full, so the last call cannot be the one that happens to work on a
     * freshly zeroed buffer. */
    if (fn_empty(&set) != 0) {
        return obs_fail("a full set could not be emptied");
    }
    if (fn_member(&set, OBS_SIGNAL_B) != 0) {
        return obs_fail("emptying a full set left a signal in it");
    }
    return obs_pass();
}

static obs_result check_page_size(void) {
    fn_posix_getpagesize_t fn = (fn_posix_getpagesize_t)obs_posix_symbol("posix_getpagesize");
    if (fn == NULL) {
        return obs_skip("libScePosix is not available in this sandbox");
    }
    int size = fn();
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
    fn_posix_usleep_t fn = (fn_posix_usleep_t)obs_posix_symbol("posix_usleep");
    if (fn == NULL) {
        return obs_skip("libScePosix is not available in this sandbox");
    }
    /* A millisecond. Short enough that a suite of several hundred checks does not
     * notice it, long enough to be a real request rather than a rounding error. */
    if (fn(1000u) != 0) {
        return obs_fail("a one-millisecond sleep reported failure");
    }
    return obs_pass();
}

static obs_result check_rwlock(void) {
    fn_posix_rwlock_init_t fn_init = (fn_posix_rwlock_init_t)obs_posix_symbol("posix_pthread_rwlock_init");
    fn_posix_rwlock_destroy_t fn_destroy = (fn_posix_rwlock_destroy_t)obs_posix_symbol("posix_pthread_rwlock_destroy");
    fn_posix_rwlock_tryrdlock_t fn_tryrd = (fn_posix_rwlock_tryrdlock_t)obs_posix_symbol("posix_pthread_rwlock_tryrdlock");
    fn_posix_rwlock_trywrlock_t fn_trywr = (fn_posix_rwlock_trywrlock_t)obs_posix_symbol("posix_pthread_rwlock_trywrlock");
    fn_posix_rwlock_unlock_t fn_unlock = (fn_posix_rwlock_unlock_t)obs_posix_symbol("posix_pthread_rwlock_unlock");
    if (fn_init == NULL || fn_destroy == NULL || fn_tryrd == NULL || fn_trywr == NULL || fn_unlock == NULL) {
        return obs_skip("libScePosix is not available in this sandbox");
    }
    ObsPosixRwlock lock = 0;
    if (fn_init(&lock, 0) != 0) {
        return obs_fail("a POSIX read/write lock could not be created");
    }
    /* The same shape as 015-sync's check of the vendor spelling, deliberately: two
     * readers admitted at once, a writer refused while they hold it, and admitted once
     * they are gone. A platform that fails the second reader has built a mutex. */
    if (fn_tryrd(&lock) != 0) {
        (void)fn_destroy(&lock);
        return obs_fail("a fresh lock could not be taken for reading");
    }
    if (fn_tryrd(&lock) != 0) {
        (void)fn_unlock(&lock);
        (void)fn_destroy(&lock);
        return obs_fail("a second reader was refused");
    }
    if (fn_trywr(&lock) == 0) {
        (void)fn_unlock(&lock);
        (void)fn_unlock(&lock);
        (void)fn_unlock(&lock);
        (void)fn_destroy(&lock);
        return obs_fail("a writer was let in while readers held the lock");
    }
    (void)fn_unlock(&lock);
    (void)fn_unlock(&lock);

    if (fn_trywr(&lock) != 0) {
        (void)fn_destroy(&lock);
        return obs_fail("a writer was refused an unheld lock");
    }
    (void)fn_unlock(&lock);
    if (fn_destroy(&lock) != 0) {
        return obs_fail("a lock could not be destroyed");
    }
    return obs_pass();
}

static obs_result check_spellings_agree(void) {
    if ((const void *)&scePthreadRwlockInit == 0 ||
        (const void *)&scePthreadRwlockTryrdlock == 0 ||
        (const void *)&scePthreadRwlockTrywrlock == 0 ||
        (const void *)&scePthreadRwlockUnlock == 0 ||
        (const void *)&scePthreadRwlockDestroy == 0) {
        return obs_skip(
            "the vendor spelling is absent, so there is nothing to compare");
    }
    fn_posix_rwlock_init_t fn_posix_init = (fn_posix_rwlock_init_t)obs_posix_symbol("posix_pthread_rwlock_init");
    fn_posix_rwlock_destroy_t fn_posix_destroy = (fn_posix_rwlock_destroy_t)obs_posix_symbol("posix_pthread_rwlock_destroy");
    fn_posix_rwlock_tryrdlock_t fn_posix_tryrd = (fn_posix_rwlock_tryrdlock_t)obs_posix_symbol("posix_pthread_rwlock_tryrdlock");
    fn_posix_rwlock_trywrlock_t fn_posix_trywr = (fn_posix_rwlock_trywrlock_t)obs_posix_symbol("posix_pthread_rwlock_trywrlock");
    fn_posix_rwlock_unlock_t fn_posix_unlock = (fn_posix_rwlock_unlock_t)obs_posix_symbol("posix_pthread_rwlock_unlock");
    if (fn_posix_init == NULL || fn_posix_destroy == NULL || fn_posix_tryrd == NULL || fn_posix_trywr == NULL || fn_posix_unlock == NULL) {
        return obs_skip("libScePosix is not available in this sandbox");
    }

    ScePthreadRwlock vendor = 0;
    ObsPosixRwlock posix = 0;
    int vendor_rc = scePthreadRwlockInit(&vendor, NULL, "obscene-compare");
    int posix_rc = fn_posix_init(&posix, 0);
    if ((vendor_rc == 0) != (posix_rc == 0)) {
        if (vendor_rc == 0) {
            (void)scePthreadRwlockDestroy(&vendor);
        }
        if (posix_rc == 0) {
            (void)fn_posix_destroy(&posix);
        }
        return obs_fail("one spelling created a lock and the other refused");
    }
    if (vendor_rc != 0) {
        return obs_skip("neither spelling could create a lock to compare");
    }

    /* Second reader on both. Whether it is admitted is the platform's business; that
     * the two answers match is not. */
    (void)scePthreadRwlockTryrdlock(&vendor);
    (void)fn_posix_tryrd(&posix);
    int vendor_second = scePthreadRwlockTryrdlock(&vendor);
    int posix_second = fn_posix_tryrd(&posix);

    int disagree_reader = (vendor_second == 0) != (posix_second == 0);

    if (vendor_second == 0) {
        (void)scePthreadRwlockUnlock(&vendor);
    }
    if (posix_second == 0) {
        (void)fn_posix_unlock(&posix);
    }

    int vendor_writer = scePthreadRwlockTrywrlock(&vendor);
    int posix_writer = fn_posix_trywr(&posix);
    int disagree_writer = (vendor_writer == 0) != (posix_writer == 0);
    if (vendor_writer == 0) {
        (void)scePthreadRwlockUnlock(&vendor);
    }
    if (posix_writer == 0) {
        (void)fn_posix_unlock(&posix);
    }

    (void)scePthreadRwlockUnlock(&vendor);
    (void)fn_posix_unlock(&posix);
    (void)scePthreadRwlockDestroy(&vendor);
    (void)fn_posix_destroy(&posix);

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
     OBS_CAP_NONE, (const void *)check_page_size, check_page_size, OBS_FROM_SPEC},
    {"017-posix/signal-sets", "libScePosix", "posix_sigemptyset", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_signal_sets, check_signal_sets, OBS_FROM_SPEC},
    {"017-posix/short-sleep", "libScePosix", "posix_usleep", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_short_sleep, check_short_sleep, OBS_FROM_SPEC},
    {"017-posix/rwlock", "libScePosix", "posix_pthread_rwlock_init", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_rwlock, check_rwlock,
     OBS_FROM_SPEC},
    /* Assumed, not spec: no document says the two libraries must be one
     * implementation. It is a strong expectation and it is still this project's. */
    {"017-posix/spellings-agree", "libScePosix", "posix_pthread_rwlock_tryrdlock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_spellings_agree,
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
