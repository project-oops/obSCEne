/*
 * The failure convention of the platform's POSIX-named exports.
 *
 * # The one question this exists to settle
 *
 * `libScePosix` exports POSIX under a `posix_` prefix, and every one of those exports
 * is a rename of a call whose failure convention POSIX fixes: a file call returns -1
 * and sets errno, a pthread call returns the errno itself. The vendor's own
 * `sceKernel*` and `scePthread*` twins do neither - they return `0x8002_0000 | errno`,
 * a scheme measured across five families and seven provoked failures by the sibling
 * project (its D398).
 *
 * So which convention does a *POSIX-named* export use when it fails - POSIX's, or the
 * vendor's? No POSIX-named export has ever been measured, and 24 of orbistoun's open
 * questions and roughly 101k recorded calls rest on the answer (docs/backlog/022). It
 * is one call to find out, and this section makes it in two independent families so the
 * result is a convention rather than a single data point:
 *
 *   * the file family - `posix_read` and `posix_write` on descriptor -1;
 *   * the pthread family - `posix_pthread_rwlock_trywrlock` while a read lock is held.
 *
 * A single family could be a quirk of one call. Two families that disagree refute "it
 * is a convention" outright, which is the cheaper thing to look for; two that agree
 * make the convention credible for the rest of the 149 names nobody will call by hand.
 *
 * # Where these resolve, and why it is not only libScePosix
 *
 * The names are resolved by name, the way `017-posix` resolves them, from `libScePosix`
 * first and then `libkernel`. The fallback is not optional: the captured console runs
 * show `libScePosix` does not load in the PS5 app sandbox, which is why `017-posix`
 * skips all five of its checks there - and the same `posix_` names are exported by
 * `libkernel`, which this program is already running on. Resolving only libScePosix
 * would make this section skip on the one platform whose answer the premise is waiting
 * for. See `obs_posixerr_symbol`.
 *
 * # It records the encoding; it fails only on an accepted bad argument
 *
 * The encoding is the finding, and no expectation about it is asserted - a `-1`, a bare
 * errno and a `0x8002...` are all legitimate answers this section reports rather than
 * grades, the same stance `140-oracle/error-codes` takes for the vendor-named calls.
 * The one thing that *is* a failure is the platform accepting the bad argument: a read
 * on a closed descriptor that returns a byte count, or a write lock granted while a
 * reader holds it, is a broken implementation whatever encoding it would have used, and
 * POSIX settles that it must be refused. That postcondition is why these carry
 * OBS_FROM_DERIVED rather than ASSUMED: the refusal is POSIX's, the encoding is the
 * open measurement.
 *
 * # Why nothing here blocks, and nothing needs a struct layout
 *
 * A read or write on descriptor -1 fails before touching the buffer; a `trywrlock`
 * never waits. The rwlock handle is an opaque pointer-sized slot, exactly as
 * 017-posix's rwlock check treats it, so no layout is assumed.
 * `posix_pthread_mutex_lock` on an invalid handle - the other shape this question could
 * take - is deliberately avoided: on a real lock it blocks, and on the host oracle an
 * invalid handle is undefined behaviour, so it could neither run safely on hardware nor
 * be validated under `make host`. The provoked failures chosen here are deterministic
 * and safe in both places. (See docs/decisions/D321.)
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/runtime.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "obscene/status.h"

#if defined(OBSCENE_HOST_BUILD)
/* On the host the POSIX-named exports are the real thing: the file calls come straight
 * from the C library and the read/write locks from the same host stubs 017-posix uses.
 * That makes the host the known-good POSIX baseline - it returns -1 and a bare errno,
 * the convention the target is being measured against. */
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif

/* The rwlock handle, opaque and pointer-sized, matching how 017-posix and the vendor
 * spelling both declare it. Nothing here reads it; the library hands one back and takes
 * it again. */
#if !defined(OBSCENE_HOST_BUILD)
typedef void *ObsPosixRwlock;
#endif

typedef sce_ssize_t (*fn_read_t)(int fd, void *buf, size_t nbytes);
typedef sce_ssize_t (*fn_write_t)(int fd, const void *buf, size_t nbytes);
typedef int (*fn_rwlock_init_t)(ObsPosixRwlock *, const void *);
typedef int (*fn_rwlock_destroy_t)(ObsPosixRwlock *);
typedef int (*fn_rwlock_tryrdlock_t)(ObsPosixRwlock *);
typedef int (*fn_rwlock_trywrlock_t)(ObsPosixRwlock *);
typedef int (*fn_rwlock_unlock_t)(ObsPosixRwlock *);
typedef int (*fn_open_t)(const char *path, int flags, ...);
typedef int (*fn_close_t)(int fd);
typedef int (*fn_mutex_destroy_t)(void *mutex);

#if !defined(OBSCENE_HOST_BUILD)
static int s_posixerr_handle = -2;
static int s_libkernel_handle = -2;

static int obs_posixerr_handle(void) {
    if (s_posixerr_handle != -2) {
        return s_posixerr_handle;
    }
    s_posixerr_handle = obs_module_open("libScePosix");
    if (s_posixerr_handle < 0) {
        s_posixerr_handle = obs_module_open("libScePosixForWebKit");
    }
    return s_posixerr_handle;
}

/* libkernel, the fallback, and on real hardware the one that actually answers.
 *
 * `libScePosix` does not load in the PS5 app sandbox - the captured console runs report
 * `900-surface/corpus_..._libScePosix fail - this library could not be loaded`, which
 * is why `017-posix` skips all five of its checks there. The same `posix_`-prefixed
 * names are exported by `libkernel` (see `data/hardware/libkernel-vaddrs.txt`:
 * `posix_read`, `posix_write`, `posix_pthread_rwlock_*`), and `libkernel` is the
 * library this program is already running on, so it is present by definition. Without
 * this fallback the whole premise this section exists to settle would skip on the
 * console it most needs to run on. */
static int obs_libkernel_handle(void) {
    if (s_libkernel_handle != -2) {
        return s_libkernel_handle;
    }
    s_libkernel_handle = obs_module_open("libkernel");
    return s_libkernel_handle;
}
#endif

static void *obs_posixerr_symbol(const char *name) {
#if defined(OBSCENE_HOST_BUILD)
    if (obs_strcmp(name, "posix_read") == 0)
        return (void *)&read;
    if (obs_strcmp(name, "posix_write") == 0)
        return (void *)&write;
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
    if (obs_strcmp(name, "open") == 0 || obs_strcmp(name, "posix_open") == 0)
        return (void *)&open;
    if (obs_strcmp(name, "close") == 0 || obs_strcmp(name, "posix_close") == 0)
        return (void *)&close;
    if (obs_strcmp(name, "pthread_mutex_destroy") == 0 ||
        obs_strcmp(name, "posix_pthread_mutex_destroy") == 0)
        return (void *)&pthread_mutex_destroy;
    if (obs_strcmp(name, "pthread_create") == 0) return (void *)&pthread_create;
    if (obs_strcmp(name, "pthread_join") == 0) return (void *)&pthread_join;
    if (obs_strcmp(name, "pthread_detach") == 0) return (void *)&pthread_detach;
    if (obs_strcmp(name, "pthread_self") == 0) return (void *)&pthread_self;
    if (obs_strcmp(name, "pthread_equal") == 0) return (void *)&pthread_equal;
    if (obs_strcmp(name, "pthread_once") == 0) return (void *)&pthread_once;
    if (obs_strcmp(name, "pthread_key_create") == 0) return (void *)&pthread_key_create;
    if (obs_strcmp(name, "pthread_getspecific") == 0) return (void *)&pthread_getspecific;
    if (obs_strcmp(name, "pthread_setspecific") == 0) return (void *)&pthread_setspecific;
    if (obs_strcmp(name, "pthread_mutex_init") == 0) return (void *)&pthread_mutex_init;
    if (obs_strcmp(name, "pthread_mutex_lock") == 0) return (void *)&pthread_mutex_lock;
    if (obs_strcmp(name, "pthread_mutex_unlock") == 0) return (void *)&pthread_mutex_unlock;
    if (obs_strcmp(name, "pthread_cond_init") == 0) return (void *)&pthread_cond_init;
    if (obs_strcmp(name, "pthread_cond_wait") == 0) return (void *)&pthread_cond_wait;
    if (obs_strcmp(name, "pthread_cond_broadcast") == 0) return (void *)&pthread_cond_broadcast;
    if (obs_strcmp(name, "pthread_kill") == 0) return (void *)&pthread_kill;
    return NULL;
#else
    /* libScePosix first, so where it does resolve the answer is that library's own;
     * then libkernel, which exports the same names and is always present. A name absent
     * from a loaded libScePosix falls through too, not just the case where the library
     * is gone. */
    int h = obs_posixerr_handle();
    if (h >= 0) {
        const void *addr = obs_module_symbol(h, name);
        if (addr != NULL) {
            return (void *)addr;
        }
    }
    int k = obs_libkernel_handle();
    if (k >= 0) {
        return (void *)obs_module_symbol(k, name);
    }
    return NULL;
#endif
}

/* The convention a returned code belongs to, named rather than numbered so a report
 * reads without a key. The vendor scheme is `0x8002_0000 | errno`; a bare small value
 * is the pthread convention (the errno itself); an all-ones word is the file convention
 * (-1). Everything else is recorded as it is, because guessing is what this section
 * exists not to do. */
#define OBS_ENC_POSIX_MINUS1 0u /* -1, the file convention */
#define OBS_ENC_POSIX_ERRNO 1u  /* a bare small errno, the pthread convention */
#define OBS_ENC_VENDOR 2u       /* 0x8002_0000 | errno */
#define OBS_ENC_ACCEPTED 3u     /* the bad argument was not refused at all */
#define OBS_ENC_OTHER 4u        /* something none of the above describes */

/* Classify a return whose success is a non-negative count (the file calls).
 *
 * Order matters: the vendor scheme `0x8002_0000 | errno` is a large *positive* value
 * when a call that returns a signed count hands it back, so it has to be tested before
 * the "non-negative means the bad descriptor was accepted" rule - otherwise a
 * vendor-encoded refusal reads as a broken accept. `-1` is checked first because it is
 * the file convention and unambiguous. */
static unsigned int obs_encoding_of_count(sce_ssize_t ret) {
    if (ret == -1) {
        return OBS_ENC_POSIX_MINUS1;
    }
    uint32_t low = (uint32_t)(uint64_t)ret;
    if ((low & 0xFFFF0000u) == 0x80020000u) {
        return OBS_ENC_VENDOR;
    }
    if (ret >= 0) {
        /* A genuine non-negative count on a closed descriptor: the argument was not
         * refused at all. */
        return OBS_ENC_ACCEPTED;
    }
    /* Some other negative value - a bare negative errno, say. Recorded as it is rather
     * than forced into one of the named conventions. */
    return OBS_ENC_OTHER;
}

/* Classify a return whose success is zero (the pthread calls). */
static unsigned int obs_encoding_of_status(int ret) {
    if (ret == 0) {
        return OBS_ENC_ACCEPTED;
    }
    if (ret == -1) {
        return OBS_ENC_POSIX_MINUS1;
    }
    uint32_t u = (uint32_t)ret;
    if ((u & 0xFFFF0000u) == 0x80020000u) {
        return OBS_ENC_VENDOR;
    }
    if (u < 0x1000u) {
        return OBS_ENC_POSIX_ERRNO;
    }
    return OBS_ENC_OTHER;
}

/* The file family: a read and a write on descriptor -1.
 *
 * Both must fail - the descriptor is closed - and the failure's encoding is the
 * finding. A scratch buffer is passed so the call has somewhere to point; on a bad
 * descriptor it is never touched. */
static obs_result check_fd_encoding(void) {
    fn_read_t fn_read = (fn_read_t)obs_posixerr_symbol("posix_read");
    fn_write_t fn_write = (fn_write_t)obs_posixerr_symbol("posix_write");
    if (fn_read == NULL || fn_write == NULL) {
        return obs_skip(
            "the POSIX-named exports resolved in neither libScePosix nor libkernel");
    }

    unsigned char scratch[16];
    for (size_t i = 0; i < sizeof scratch; i++) {
        scratch[i] = 0;
    }

    sce_ssize_t r = fn_read(-1, scratch, sizeof scratch);
    unsigned int r_enc = obs_encoding_of_count(r);
    obs_report_error_code("libScePosix", "posix_read", "descriptor -1",
                          (uint64_t)(uint32_t)(int32_t)r);
    obs_report_measure("019-posixerr/fd-encoding", "posix_read", "encoding",
                       (uint64_t)r_enc, "encoding");

    sce_ssize_t w = fn_write(-1, scratch, sizeof scratch);
    unsigned int w_enc = obs_encoding_of_count(w);
    obs_report_error_code("libScePosix", "posix_write", "descriptor -1",
                          (uint64_t)(uint32_t)(int32_t)w);
    obs_report_measure("019-posixerr/fd-encoding", "posix_write", "encoding",
                       (uint64_t)w_enc, "encoding");

    if (r_enc == OBS_ENC_ACCEPTED || w_enc == OBS_ENC_ACCEPTED) {
        return obs_fail("a read or write on a closed descriptor was not refused");
    }
    if (r_enc != w_enc) {
        /* Two calls in one family that encode failure differently is itself a finding,
         * and a strange one - reported rather than smoothed over. */
        return obs_partial_value("read and write on a bad descriptor encode failure "
                                 "differently",
                                 (uint64_t)((r_enc << 4) | w_enc));
    }
    return obs_pass_value((uint64_t)r_enc);
}

/* The pthread family: a write lock refused while a reader holds it.
 *
 * Take a read lock, then try for a write lock. The writer must be refused - a reader
 * holds the lock - and the refusal's encoding is the finding. This is the same provoked
 * failure 017-posix/rwlock relies on, read for its code rather than its verdict. */
static obs_result check_pthread_encoding(void) {
    fn_rwlock_init_t fn_init =
        (fn_rwlock_init_t)obs_posixerr_symbol("posix_pthread_rwlock_init");
    fn_rwlock_destroy_t fn_destroy =
        (fn_rwlock_destroy_t)obs_posixerr_symbol("posix_pthread_rwlock_destroy");
    fn_rwlock_tryrdlock_t fn_tryrd =
        (fn_rwlock_tryrdlock_t)obs_posixerr_symbol("posix_pthread_rwlock_tryrdlock");
    fn_rwlock_trywrlock_t fn_trywr =
        (fn_rwlock_trywrlock_t)obs_posixerr_symbol("posix_pthread_rwlock_trywrlock");
    fn_rwlock_unlock_t fn_unlock =
        (fn_rwlock_unlock_t)obs_posixerr_symbol("posix_pthread_rwlock_unlock");
    if (fn_init == NULL || fn_destroy == NULL || fn_tryrd == NULL || fn_trywr == NULL ||
        fn_unlock == NULL) {
        return obs_skip(
            "the POSIX-named exports resolved in neither libScePosix nor libkernel");
    }

    ObsPosixRwlock lock = 0;
    if (fn_init(&lock, 0) != 0) {
        return obs_skip("a POSIX read/write lock could not be created to provoke");
    }
    if (fn_tryrd(&lock) != 0) {
        (void)fn_destroy(&lock);
        return obs_skip("a read lock could not be taken to hold against a writer");
    }

    int writer = fn_trywr(&lock);
    unsigned int enc = obs_encoding_of_status(writer);
    obs_report_error_code("libScePosix", "posix_pthread_rwlock_trywrlock",
                          "write lock while read-held", (uint64_t)(uint32_t)writer);
    obs_report_measure("019-posixerr/pthread-encoding",
                       "posix_pthread_rwlock_trywrlock", "encoding", (uint64_t)enc,
                       "encoding");

    if (writer == 0) {
        /* The writer was let in while a reader held the lock. Release it before tearing
         * down, and report the semantics failure - a broken lock, regardless of how it
         * would have encoded a refusal it never made. */
        (void)fn_unlock(&lock);
    }
    (void)fn_unlock(&lock);
    (void)fn_destroy(&lock);

    if (writer == 0) {
        return obs_fail("a write lock was granted while a reader held the lock");
    }
    return obs_pass_value((uint64_t)enc);
}

static obs_result check_provoked_errors(void) {
    /* Fulfills REQ-20260914T1110Z-9b12:
     * Deliberately provoke errors on POSIX-named exports:
     * - open("/nonexistent_path_to_provoke_posix_error", O_RDONLY)
     * - close(-1)
     * - pthread_mutex_destroy(NULL)
     * Record exact 32-bit return code and errno/convention.
     */
    fn_open_t fn_open = (fn_open_t)obs_posixerr_symbol("posix_open");
    if (fn_open == NULL) {
        fn_open = (fn_open_t)obs_posixerr_symbol("open");
    }
    fn_close_t fn_close = (fn_close_t)obs_posixerr_symbol("posix_close");
    if (fn_close == NULL) {
        fn_close = (fn_close_t)obs_posixerr_symbol("close");
    }
    fn_mutex_destroy_t fn_pmutex_destroy =
        (fn_mutex_destroy_t)obs_posixerr_symbol("posix_pthread_mutex_destroy");
    if (fn_pmutex_destroy == NULL) {
        fn_pmutex_destroy = (fn_mutex_destroy_t)obs_posixerr_symbol("pthread_mutex_destroy");
    }

    uint32_t open_rc = 0;
    int open_err = 0;
    if (fn_open != NULL) {
        open_rc = (uint32_t)fn_open("/nonexistent_path_to_provoke_posix_error", 0);
#if defined(OBSCENE_HOST_BUILD)
        open_err = errno;
#else
        if (&__error != NULL && __error() != NULL) {
            open_err = *__error();
        }
#endif
        obs_report_measure("019-posixerr/provoked-errors", "open", "rc", (uint64_t)open_rc,
                           "rc");
        obs_report_measure("019-posixerr/provoked-errors", "open", "errno",
                           (uint64_t)(uint32_t)open_err, "errno");
        obs_report_measure("019-posixerr/provoked-errors", "open", "encoding",
                           (uint64_t)obs_encoding_of_count((sce_ssize_t)(int32_t)open_rc),
                           "encoding");
    }

    uint32_t close_rc = 0;
    int close_err = 0;
    if (fn_close != NULL) {
        close_rc = (uint32_t)fn_close(-1);
#if defined(OBSCENE_HOST_BUILD)
        close_err = errno;
#else
        if (&__error != NULL && __error() != NULL) {
            close_err = *__error();
        }
#endif
        obs_report_measure("019-posixerr/provoked-errors", "close", "rc", (uint64_t)close_rc,
                           "rc");
        obs_report_measure("019-posixerr/provoked-errors", "close", "errno",
                           (uint64_t)(uint32_t)close_err, "errno");
        obs_report_measure("019-posixerr/provoked-errors", "close", "encoding",
                           (uint64_t)obs_encoding_of_count((sce_ssize_t)(int32_t)close_rc),
                           "encoding");
    }

    uint32_t mutex_rc = 0;
    if (fn_pmutex_destroy != NULL) {
        mutex_rc = (uint32_t)fn_pmutex_destroy(NULL);
        obs_report_measure("019-posixerr/provoked-errors", "pthread_mutex_destroy", "rc",
                           (uint64_t)mutex_rc, "rc");
        obs_report_measure("019-posixerr/provoked-errors", "pthread_mutex_destroy",
                           "encoding", (uint64_t)obs_encoding_of_status((int)mutex_rc),
                           "encoding");
    }

    if (fn_open == NULL && fn_close == NULL && fn_pmutex_destroy == NULL) {
        return obs_skip("none of open, close, or pthread_mutex_destroy could be resolved");
    }

    return obs_pass_value((uint64_t)close_rc);
}

static obs_result check_thread_symbols_libkernel(void) {
    static const struct {
        const char *unprefixed;
        const char *prefixed;
    } pairs[] = {
        {"pthread_create", "posix_pthread_create"},
        {"pthread_join", "posix_pthread_join"},
        {"pthread_detach", "posix_pthread_detach"},
        {"pthread_self", "posix_pthread_self"},
        {"pthread_equal", "posix_pthread_equal"},
        {"pthread_once", "posix_pthread_once"},
        {"pthread_key_create", "posix_pthread_key_create"},
        {"pthread_getspecific", "posix_pthread_getspecific"},
        {"pthread_setspecific", "posix_pthread_setspecific"},
        {"pthread_mutex_init", "posix_pthread_mutex_init"},
        {"pthread_mutex_lock", "posix_pthread_mutex_lock"},
        {"pthread_mutex_unlock", "posix_pthread_mutex_unlock"},
        {"pthread_cond_init", "posix_pthread_cond_init"},
        {"pthread_cond_wait", "posix_pthread_cond_wait"},
        {"pthread_cond_broadcast", "posix_pthread_cond_broadcast"},
    };
    static const char *const controls[] = {
        "pthread_kill", "pthread_timedjoin_np", "posix_read"
    };

    unsigned int bound_unprefixed = 0;
    unsigned int bound_prefixed = 0;

    for (unsigned int i = 0; i < OBS_COUNT(pairs); i++) {
        void *sym_u = obs_posixerr_symbol(pairs[i].unprefixed);
        void *sym_p = obs_posixerr_symbol(pairs[i].prefixed);

        if (sym_u != NULL) bound_unprefixed++;
        if (sym_p != NULL) bound_prefixed++;

        obs_report_measure("019-posixerr/thread-symbols-libkernel", pairs[i].unprefixed,
                           "bound", (uint64_t)(sym_u != NULL), "bool");
        obs_report_measure("019-posixerr/thread-symbols-libkernel", pairs[i].prefixed,
                           "bound", (uint64_t)(sym_p != NULL), "bool");
    }

    for (unsigned int i = 0; i < OBS_COUNT(controls); i++) {
        void *sym_c = obs_posixerr_symbol(controls[i]);
        obs_report_measure("019-posixerr/thread-symbols-libkernel", controls[i],
                           "control-bound", (uint64_t)(sym_c != NULL), "bool");
    }

    obs_report_measure("019-posixerr/thread-symbols-libkernel", "summary", "bound-unprefixed",
                       (uint64_t)bound_unprefixed, "count");
    obs_report_measure("019-posixerr/thread-symbols-libkernel", "summary", "bound-prefixed",
                       (uint64_t)bound_prefixed, "count");

    return obs_pass_value((uint64_t)(bound_unprefixed + bound_prefixed));
}

static const obs_check posixerr_checks[] = {
    {"019-posixerr/fd-encoding", "libScePosix", "posix_read", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_fd_encoding, check_fd_encoding,
     OBS_FROM_DERIVED},
    {"019-posixerr/pthread-encoding", "libScePosix", "posix_pthread_rwlock_trywrlock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_pthread_encoding,
     check_pthread_encoding, OBS_FROM_DERIVED},
    {"019-posixerr/provoked-errors", "libScePosix", "(provoked)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_provoked_errors, OBS_FROM_DERIVED},
    {"019-posixerr/thread-symbols-libkernel", "libkernel", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_thread_symbols_libkernel, OBS_FROM_DERIVED},
};

const obs_section obs_section_posixerr = {
    "019-posixerr",
    "POSIX-named error encoding",
    "Which failure convention the platform's POSIX-named exports use - POSIX's -1 and "
    "errno, or the vendor 0x8002 encoding its own twins use - measured in the file and "
    "pthread families by one provoked failure each. Records the encoding; fails only "
    "if "
    "a bad argument is accepted.",
    posixerr_checks,
    OBS_COUNT(posixerr_checks),
};
