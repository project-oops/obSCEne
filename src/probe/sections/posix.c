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
#include "obscene/report.h"
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
    if (obs_strcmp(name, "posix_getpagesize") == 0)
        return (void *)&posix_getpagesize;
    if (obs_strcmp(name, "posix_sigemptyset") == 0)
        return (void *)&posix_sigemptyset;
    if (obs_strcmp(name, "posix_sigfillset") == 0)
        return (void *)&posix_sigfillset;
    if (obs_strcmp(name, "posix_sigaddset") == 0)
        return (void *)&posix_sigaddset;
    if (obs_strcmp(name, "posix_sigdelset") == 0)
        return (void *)&posix_sigdelset;
    if (obs_strcmp(name, "posix_sigismember") == 0)
        return (void *)&posix_sigismember;
    if (obs_strcmp(name, "posix_usleep") == 0)
        return (void *)&posix_usleep;
    if (obs_strcmp(name, "posix_pthread_rwlock_init") == 0)
        return (void *)&posix_pthread_rwlock_init;
    if (obs_strcmp(name, "posix_pthread_rwlock_destroy") == 0)
        return (void *)&posix_pthread_rwlock_destroy;
    if (obs_strcmp(name, "posix_pthread_rwlock_tryrdlock") == 0)
        return (void *)&posix_pthread_rwlock_tryrdlock;
    if (obs_strcmp(name, "posix_pthread_rwlock_trywrlock") == 0)
        return (void *)&posix_pthread_rwlock_trywrlock;
    if (obs_strcmp(name, "posix_pthread_rwlock_unlock") == 0)
        return (void *)&posix_pthread_rwlock_unlock;
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
    fn_posix_sigemptyset_t fn_empty =
        (fn_posix_sigemptyset_t)obs_posix_symbol("posix_sigemptyset");
    fn_posix_sigfillset_t fn_fill =
        (fn_posix_sigfillset_t)obs_posix_symbol("posix_sigfillset");
    fn_posix_sigaddset_t fn_add =
        (fn_posix_sigaddset_t)obs_posix_symbol("posix_sigaddset");
    fn_posix_sigdelset_t fn_del =
        (fn_posix_sigdelset_t)obs_posix_symbol("posix_sigdelset");
    fn_posix_sigismember_t fn_member =
        (fn_posix_sigismember_t)obs_posix_symbol("posix_sigismember");
    if (fn_empty == NULL || fn_fill == NULL || fn_add == NULL || fn_del == NULL ||
        fn_member == NULL) {
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
    if (fn_member(&set, OBS_SIGNAL_A) != 1 || fn_member(&set, OBS_SIGNAL_B) != 1) {
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
    fn_posix_getpagesize_t fn =
        (fn_posix_getpagesize_t)obs_posix_symbol("posix_getpagesize");
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
    fn_posix_rwlock_init_t fn_init =
        (fn_posix_rwlock_init_t)obs_posix_symbol("posix_pthread_rwlock_init");
    fn_posix_rwlock_destroy_t fn_destroy =
        (fn_posix_rwlock_destroy_t)obs_posix_symbol("posix_pthread_rwlock_destroy");
    fn_posix_rwlock_tryrdlock_t fn_tryrd =
        (fn_posix_rwlock_tryrdlock_t)obs_posix_symbol("posix_pthread_rwlock_tryrdlock");
    fn_posix_rwlock_trywrlock_t fn_trywr =
        (fn_posix_rwlock_trywrlock_t)obs_posix_symbol("posix_pthread_rwlock_trywrlock");
    fn_posix_rwlock_unlock_t fn_unlock =
        (fn_posix_rwlock_unlock_t)obs_posix_symbol("posix_pthread_rwlock_unlock");
    if (fn_init == NULL || fn_destroy == NULL || fn_tryrd == NULL || fn_trywr == NULL ||
        fn_unlock == NULL) {
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
    fn_posix_rwlock_init_t fn_posix_init =
        (fn_posix_rwlock_init_t)obs_posix_symbol("posix_pthread_rwlock_init");
    fn_posix_rwlock_destroy_t fn_posix_destroy =
        (fn_posix_rwlock_destroy_t)obs_posix_symbol("posix_pthread_rwlock_destroy");
    fn_posix_rwlock_tryrdlock_t fn_posix_tryrd =
        (fn_posix_rwlock_tryrdlock_t)obs_posix_symbol("posix_pthread_rwlock_tryrdlock");
    fn_posix_rwlock_trywrlock_t fn_posix_trywr =
        (fn_posix_rwlock_trywrlock_t)obs_posix_symbol("posix_pthread_rwlock_trywrlock");
    fn_posix_rwlock_unlock_t fn_posix_unlock =
        (fn_posix_rwlock_unlock_t)obs_posix_symbol("posix_pthread_rwlock_unlock");
    if (fn_posix_init == NULL || fn_posix_destroy == NULL || fn_posix_tryrd == NULL ||
        fn_posix_trywr == NULL || fn_posix_unlock == NULL) {
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

/* REQ-20260914T1443Z-3ea7: Test whether libkernel exports standard POSIX pthread
 * symbols directly in the app sandbox (where libScePosix is blocked). */
static obs_result check_libkernel_pthread_symbols(void) {
    static const char *const pthread_syms[] = {
        "pthread_create", "pthread_join", "pthread_detach", "pthread_exit",
        "pthread_self", "pthread_equal", "pthread_mutex_init", "pthread_mutex_lock",
        "pthread_mutex_trylock", "pthread_mutex_unlock", "pthread_mutex_destroy",
        "pthread_cond_init", "pthread_cond_wait", "pthread_cond_timedwait",
        "pthread_cond_signal", "pthread_cond_broadcast", "pthread_cond_destroy",
        "pthread_rwlock_init", "pthread_rwlock_rdlock", "pthread_rwlock_wrlock",
        "pthread_rwlock_unlock", "pthread_rwlock_destroy", "pthread_once",
        "pthread_key_create", "pthread_key_delete", "pthread_getspecific",
        "pthread_setspecific"
    };

    int lk_handle = obs_module_open("libkernel");
    if (lk_handle < 0) {
        lk_handle = obs_module_open("libkernel.sprx");
    }

    uint64_t resolved_count = 0;
    for (size_t i = 0; i < sizeof(pthread_syms) / sizeof(pthread_syms[0]); i++) {
        const char *name = pthread_syms[i];
        const void *fn = NULL;
        if (lk_handle >= 0) {
            fn = obs_module_symbol(lk_handle, name);
        }
        if (fn == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(1, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn = addr;
            }
        }
        int callable = obs_address_is_callable(fn);
        if (callable) {
            resolved_count++;
        }
        obs_report_measure("017-posix/libkernel-pthread-symbols", name, "resolved",
                           (uint64_t)callable, "bool");
    }

    obs_report_measure("017-posix/libkernel-pthread-symbols", "libkernel", "total-resolved",
                       resolved_count, "count");

    /* Pass value records total resolved count (0 confirms libkernel exports no direct pthread
     * aliases, validating oops-mesa's vendor mapping requirement; >0 indicates direct aliases exist). */
    return obs_pass_value(resolved_count);
}

static obs_result check_posix_clock_symbols(void) {
    static const char *const clock_syms[] = {
        "clock_gettime",
        "clock_getres",
        "clock_settime",
        "nanosleep",
        "sched_yield",
        "gettimeofday",
        "sceKernelClockGettime",
        "sceKernelClockGetres",
        "sceKernelNanosleep",
        "sceKernelSchedYield",
        "sceKernelUsleep",
        "sceKernelSleep",
    };

    int lk_handle = obs_module_open("libkernel");
    if (lk_handle < 0) {
        lk_handle = obs_module_open("libkernel.sprx");
    }
    int libc_handle = obs_module_open("libSceLibcInternal");
    if (libc_handle < 0) {
        libc_handle = obs_module_open("libSceLibcInternal.sprx");
    }

    uint64_t lk_resolved = 0;
    uint64_t libc_resolved = 0;
    for (size_t i = 0; i < sizeof(clock_syms) / sizeof(clock_syms[0]); i++) {
        const char *name = clock_syms[i];

        /* Check in libkernel */
        const void *fn_lk = NULL;
        if (lk_handle >= 0) {
            fn_lk = obs_module_symbol(lk_handle, name);
        }
        if (fn_lk == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(1, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_lk = addr;
            }
        }
        int lk_call = obs_address_is_callable(fn_lk);
        if (lk_call) lk_resolved++;
        obs_report_measure("017-posix/clock-symbols", name, "libkernel",
                           (uint64_t)lk_call, "bool");

        /* Check in libc */
        const void *fn_libc = NULL;
        if (libc_handle >= 0) {
            fn_libc = obs_module_symbol(libc_handle, name);
        }
        if (fn_libc == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(0x2001, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_libc = addr;
            }
        }
        int libc_call = obs_address_is_callable(fn_libc);
        if (libc_call) libc_resolved++;
        obs_report_measure("017-posix/clock-symbols", name, "libc",
                           (uint64_t)libc_call, "bool");
    }

    obs_report_measure("017-posix/clock-symbols", "libkernel", "total-resolved",
                       lk_resolved, "count");
    obs_report_measure("017-posix/clock-symbols", "libSceLibcInternal", "total-resolved",
                       libc_resolved, "count");

    return obs_pass_value(lk_resolved + libc_resolved);
}

/*
 * Where nine libc names actually resolve from, for oops-mesa's import manifest.
 *
 * A title linking Mesa must name, for every symbol it imports, the library that exports it.
 * obSCEne's mined corpus answers that for 504 of them and carries these nine with `-` in the
 * library column: the mining saw the name and its NID but never learned the module. Until
 * 2026-09-16 oops-mesa's generator wrote that `-` through as though it were a placement, and the
 * console answered the way it should - the title loaded, then died with
 * PRX_NOT_RESOLVED_FUNCTION on the first call, which was `__assert` inside Mesa's option cache.
 * The generator now refuses instead, so nothing can be built from them until this says where
 * they live.
 *
 * Reported per symbol per library rather than as a verdict, because "absent from all three" is a
 * real answer and the useful one to have written down: it would mean the platform does not export
 * that name at all and Mesa's use of it has to be compiled out rather than bound.
 */
static obs_result check_unplaced_libc_imports(void) {
    static const char *const unplaced_syms[] = {
        "__assert", "__xuname", "getline", "localtime_r", "mknod",
        "mkstemps", "open_memstream", "openlog", "regcomp", "regexec", "regfree",
    };

    int lk_handle = obs_module_open("libkernel");
    if (lk_handle < 0) {
        lk_handle = obs_module_open("libkernel.sprx");
    }
    int libc_handle = obs_module_open("libSceLibcInternal");
    if (libc_handle < 0) {
        libc_handle = obs_module_open("libSceLibcInternal.sprx");
    }
    int posix_handle = obs_module_open("libScePosix");
    if (posix_handle < 0) {
        posix_handle = obs_module_open("libScePosix.sprx");
    }

    obs_report_measure("017-posix/unplaced-libc-imports", "handle-opened", "libkernel",
                       (uint64_t)(lk_handle >= 0 ? 1 : 0), "bool");
    obs_report_measure("017-posix/unplaced-libc-imports", "handle-opened", "libSceLibcInternal",
                       (uint64_t)(libc_handle >= 0 ? 1 : 0), "bool");
    obs_report_measure("017-posix/unplaced-libc-imports", "handle-opened", "libScePosix",
                       (uint64_t)(posix_handle >= 0 ? 1 : 0), "bool");

    uint64_t placed = 0;
    for (size_t i = 0; i < sizeof(unplaced_syms) / sizeof(unplaced_syms[0]); i++) {
        const char *name = unplaced_syms[i];
        int any = 0;

        const void *fn_lk = (lk_handle >= 0) ? obs_module_symbol(lk_handle, name) : NULL;
        if (fn_lk == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(1, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_lk = addr;
            }
        }
        int lk_call = obs_address_is_callable(fn_lk);
        obs_report_measure("017-posix/unplaced-libc-imports", name, "libkernel",
                           (uint64_t)lk_call, "bool");
        any |= lk_call;

        const void *fn_libc = (libc_handle >= 0) ? obs_module_symbol(libc_handle, name) : NULL;
        if (fn_libc == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(0x2001, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_libc = addr;
            }
        }
        int libc_call = obs_address_is_callable(fn_libc);
        obs_report_measure("017-posix/unplaced-libc-imports", name, "libSceLibcInternal",
                           (uint64_t)libc_call, "bool");
        any |= libc_call;

        const void *fn_px = (posix_handle >= 0) ? obs_module_symbol(posix_handle, name) : NULL;
        int px_call = obs_address_is_callable(fn_px);
        obs_report_measure("017-posix/unplaced-libc-imports", name, "libScePosix",
                           (uint64_t)px_call, "bool");
        any |= px_call;

        if (any) {
            placed++;
        }
    }

    obs_report_measure("017-posix/unplaced-libc-imports", "(symbols)", "placed-somewhere",
                       placed, "count");
    obs_report_measure("017-posix/unplaced-libc-imports", "(symbols)", "asked",
                       (uint64_t)(sizeof(unplaced_syms) / sizeof(unplaced_syms[0])), "count");

    return obs_pass_value(placed);
}

static obs_result check_mesa_candidate_imports(void) {
    /* Characterization for REQ-20260917T0025Z-1f6d:
     * Check which of eleven libc and kernel names the platform actually exports:
     * libSceLibcInternal: getenv, __stderrp, abort, fprintf, free, malloc, realloc
     * libkernel: __error, close, fstat, open, read
     */
    static const struct {
        const char *name;
        int is_control;
    } libc_syms[] = {
        {"getenv", 0},
        {"__stderrp", 0},
        {"abort", 0},
        {"fprintf", 0},
        {"free", 1},
        {"malloc", 1},
        {"realloc", 1},
    };

    static const struct {
        const char *name;
        int is_control;
    } lk_syms[] = {
        {"__error", 0},
        {"close", 1},
        {"fstat", 0},
        {"open", 1},
        {"read", 1},
    };

    int lk_handle = obs_module_open("libkernel");
    if (lk_handle < 0) {
        lk_handle = obs_module_open("libkernel.sprx");
    }
    int libc_handle = obs_module_open("libSceLibcInternal");
    if (libc_handle < 0) {
        libc_handle = obs_module_open("libSceLibcInternal.sprx");
    }

    obs_report_measure("017-posix/mesa-candidate-imports", "handle-opened", "libkernel",
                       (uint64_t)(lk_handle >= 0 ? 1 : 0), "bool");
    obs_report_measure("017-posix/mesa-candidate-imports", "handle-opened", "libSceLibcInternal",
                       (uint64_t)(libc_handle >= 0 ? 1 : 0), "bool");

    uint64_t libc_controls_resolved = 0;
    uint64_t libc_candidates_resolved = 0;
    for (size_t i = 0; i < sizeof(libc_syms) / sizeof(libc_syms[0]); i++) {
        const char *name = libc_syms[i].name;
        const void *fn_libc = (libc_handle >= 0) ? obs_module_symbol(libc_handle, name) : NULL;
        if (fn_libc == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(0x2001, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_libc = addr;
            }
        }
        int libc_call = obs_address_is_callable(fn_libc);
        obs_report_measure("017-posix/mesa-candidate-imports", name, "libSceLibcInternal",
                           (uint64_t)libc_call, "bool");
        if (libc_call) {
            if (libc_syms[i].is_control) {
                libc_controls_resolved++;
            } else {
                libc_candidates_resolved++;
            }
        }
    }

    uint64_t lk_controls_resolved = 0;
    uint64_t lk_candidates_resolved = 0;
    for (size_t i = 0; i < sizeof(lk_syms) / sizeof(lk_syms[0]); i++) {
        const char *name = lk_syms[i].name;
        const void *fn_lk = (lk_handle >= 0) ? obs_module_symbol(lk_handle, name) : NULL;
        if (fn_lk == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(1, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn_lk = addr;
            }
        }
        int lk_call = obs_address_is_callable(fn_lk);
        obs_report_measure("017-posix/mesa-candidate-imports", name, "libkernel",
                           (uint64_t)lk_call, "bool");
        if (lk_call) {
            if (lk_syms[i].is_control) {
                lk_controls_resolved++;
            } else {
                lk_candidates_resolved++;
            }
        }
    }

    obs_report_measure("017-posix/mesa-candidate-imports", "getenv", "eboot-linked",
                       (uint64_t)obs_address_is_callable((const void *)&getenv), "bool");
    obs_report_measure("017-posix/mesa-candidate-imports", "__error", "eboot-linked",
                       (uint64_t)obs_address_is_callable((const void *)&__error), "bool");

    obs_report_measure("017-posix/mesa-candidate-imports", "libc-controls", "resolved",
                       libc_controls_resolved, "count");
    obs_report_measure("017-posix/mesa-candidate-imports", "kernel-controls", "resolved",
                       lk_controls_resolved, "count");
    obs_report_measure("017-posix/mesa-candidate-imports", "candidates", "resolved",
                       libc_candidates_resolved + lk_candidates_resolved, "count");

    return obs_pass_value(libc_candidates_resolved + lk_candidates_resolved);
}

static obs_result check_posix_descriptors_output(void) {
    /* Characterization for REQ-20260917T0233Z-5c9d:
     * Determine whether fd 1, fd 2 (stderr) and SYS_klog output surface in the captured log,
     * and whether dup2(1, 2) makes fd 2 visible if it is initially dead.
     */
    static const char marker_klog[] = "OBS-KLOG-MARKER-7a8b9c\n";
    static const char marker_fd1[]  = "OBS-FD1-MARKER-7a8b9c\n";
    static const char marker_fd2[]  = "OBS-FD2-MARKER-7a8b9c\n";
    static const char marker_dup2[] = "OBS-FD2-DUP2-MARKER-7a8b9c\n";

    int lk_handle = obs_module_open("libkernel");
    if (lk_handle < 0) {
        lk_handle = obs_module_open("libkernel.sprx");
    }

    typedef sce_ssize_t (*fn_write_t)(int, const void *, size_t);
    typedef int (*fn_dup2_t)(int, int);

    fn_write_t p_write = NULL;
    fn_dup2_t p_dup2 = NULL;

    if (lk_handle >= 0) {
        p_write = (fn_write_t)obs_module_symbol(lk_handle, "write");
        p_dup2 = (fn_dup2_t)obs_module_symbol(lk_handle, "dup2");
        if (p_dup2 == NULL) {
            p_dup2 = (fn_dup2_t)obs_module_symbol(lk_handle, "sceKernelDup2");
        }
    }
    if (p_write == NULL && obs_address_is_callable((const void *)&sceKernelWrite)) {
        p_write = (fn_write_t)&sceKernelWrite;
    }

    int *err_ptr = obs_address_is_callable((const void *)&__error) ? __error() : NULL;

    /* 1. write(1, marker_fd1, len) */
    if (err_ptr) *err_ptr = 0;
    long rc_fd1 = -1;
    if (p_write != NULL) {
        rc_fd1 = (long)p_write(1, marker_fd1, sizeof(marker_fd1) - 1);
    } else {
        rc_fd1 = obs_invoke_syscall(4, 1, (long)marker_fd1, (long)(sizeof(marker_fd1) - 1), 0, 0, 0);
    }
    int err_fd1 = err_ptr ? *err_ptr : 0;

    /* 2. write(2, marker_fd2, len) */
    if (err_ptr) *err_ptr = 0;
    long rc_fd2 = -1;
    if (p_write != NULL) {
        rc_fd2 = (long)p_write(2, marker_fd2, sizeof(marker_fd2) - 1);
    } else {
        rc_fd2 = obs_invoke_syscall(4, 2, (long)marker_fd2, (long)(sizeof(marker_fd2) - 1), 0, 0, 0);
    }
    int err_fd2 = err_ptr ? *err_ptr : 0;

    /* 3. sys_call(SYS_klog, 7, marker_klog, 0, ...) */
    long rc_klog = obs_invoke_syscall(601, 7, (long)marker_klog, 0, 0, 0, 0);

    /* 4. dup2(1, 2) */
    if (err_ptr) *err_ptr = 0;
    long rc_dup2 = -1;
    if (p_dup2 != NULL) {
        rc_dup2 = (long)p_dup2(1, 2);
    } else {
        rc_dup2 = obs_invoke_syscall(90, 1, 2, 0, 0, 0, 0);
    }
    int err_dup2 = err_ptr ? *err_ptr : 0;

    /* 5. Repeat write(2, marker_dup2, len) after dup2 */
    if (err_ptr) *err_ptr = 0;
    long rc_fd2_dup2 = -1;
    if (p_write != NULL) {
        rc_fd2_dup2 = (long)p_write(2, marker_dup2, sizeof(marker_dup2) - 1);
    } else {
        rc_fd2_dup2 = obs_invoke_syscall(4, 2, (long)marker_dup2, (long)(sizeof(marker_dup2) - 1), 0, 0, 0);
    }
    int err_fd2_dup2 = err_ptr ? *err_ptr : 0;

    obs_report_measure("017-posix/descriptors-output", "rc-fd1", "bytes", (uint64_t)rc_fd1, "code");
    obs_report_measure("017-posix/descriptors-output", "errno-fd1", "errno", (uint64_t)(uint32_t)err_fd1, "code");
    obs_report_measure("017-posix/descriptors-output", "rc-fd2", "bytes", (uint64_t)rc_fd2, "code");
    obs_report_measure("017-posix/descriptors-output", "errno-fd2", "errno", (uint64_t)(uint32_t)err_fd2, "code");
    obs_report_measure("017-posix/descriptors-output", "rc-klog", "code", (uint64_t)rc_klog, "code");
    obs_report_measure("017-posix/descriptors-output", "rc-dup2", "code", (uint64_t)rc_dup2, "code");
    obs_report_measure("017-posix/descriptors-output", "errno-dup2", "errno", (uint64_t)(uint32_t)err_dup2, "code");
    obs_report_measure("017-posix/descriptors-output", "rc-fd2-after-dup2", "bytes", (uint64_t)rc_fd2_dup2, "code");
    obs_report_measure("017-posix/descriptors-output", "errno-fd2-after-dup2", "errno", (uint64_t)(uint32_t)err_fd2_dup2, "code");

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
     OBS_CAP_NONE, (const void *)check_rwlock, check_rwlock, OBS_FROM_SPEC},
    /* Assumed, not spec: no document says the two libraries must be one
     * implementation. It is a strong expectation and it is still this project's. */
    {"017-posix/spellings-agree", "libScePosix", "posix_pthread_rwlock_tryrdlock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_spellings_agree,
     check_spellings_agree, OBS_FROM_ASSUMED},
    {"017-posix/libkernel-pthread-symbols", "libkernel", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_libkernel_pthread_symbols, OBS_FROM_SPEC},
    {"017-posix/clock-symbols", "libkernel", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_posix_clock_symbols, OBS_FROM_SPEC},
    /* Assumed, not spec: nothing documents which module exports these, which is the
     * whole reason to ask the hardware. */
    {"017-posix/unplaced-libc-imports", "libSceLibcInternal", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_unplaced_libc_imports, OBS_FROM_ASSUMED},
    {"017-posix/mesa-candidate-imports", "libSceLibcInternal", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_mesa_candidate_imports, OBS_FROM_ASSUMED},
    {"017-posix/descriptors-output", "libkernel", "write", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_posix_descriptors_output, OBS_FROM_ASSUMED},
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
