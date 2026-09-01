/*
 * Synchronisation primitives, and thread churn.
 *
 * # Why these checks exist
 *
 * They were chosen from an emulator's release notes rather than invented. That release
 * rewrote its event flags to fix a title that hung, and fixed a thread pool that
 * destroyed threads twice once more than a cached number were in flight. Both were
 * present in this program's census - reported as existing, and never called - so the
 * suite said nothing about either while both were broken.
 *
 * That is the gap this section closes, and it is the argument for the whole approach:
 * an existence test cannot see a behaviour bug, and behaviour bugs are what emulators
 * actually have. Checking these against a release that fixed them, and a release that
 * did not, is how the method gets proved rather than asserted.
 *
 * # Poll rather than wait
 *
 * `sceKernelWaitEventFlag` blocks. A probe that blocks on a platform whose event flags
 * do not work never comes back, and takes every check behind it with it - an outcome
 * this suite has already paid for twice. `sceKernelPollEventFlag` asks the same
 * question and returns either way, which is the only version safe to run against
 * something suspected of being broken.
 *
 * # Nothing is left behind
 *
 * Every flag created is deleted and every thread joined, on every path including the
 * failures. A probe that leaks a kernel object changes the machine for the checks after
 * it, and a report where check N depends on whether check M leaked is not a report.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Two arbitrary bits far enough apart that a platform confusing a pattern with a count,
 * or setting one bit and reporting another, produces a visibly wrong number rather than
 * an off-by-one that reads as success. */
#define OBS_EVF_BIT_A 0x0000000000000002ull
#define OBS_EVF_BIT_B 0x0000000100000000ull

/* More than a thread cache is likely to hold. The bug this mirrors appeared only once
 * more than a cached number of threads had been through the pool, so a check that
 * creates two would never have seen it. */
#ifndef OBS_THREAD_CHURN
#define OBS_THREAD_CHURN 40
#endif

/* Often enough to locate a crash, rarely enough not to bury the report. */
#define OBS_CHURN_REPORT_EVERY 4

static obs_result check_event_flag_round_trip(void) {
    OBS_REQUIRE(&sceKernelClearEventFlag,
                &sceKernelCreateEventFlag,
                &sceKernelDeleteEventFlag,
                &sceKernelSetEventFlag);
    SceKernelEventFlag flag = 0;
    int rc = sceKernelCreateEventFlag(&flag, "obscene-evf",
                                      OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0, NULL);
    if (rc != 0) {
        return obs_fail_code("an event flag could not be created", (uint64_t)(uint32_t)rc);
    }
    if (flag == 0) {
        return obs_fail("creation reported success and handed back nothing");
    }

    /* A flag created empty must not report a bit nobody set. Checked before setting
     * anything, because a platform that reports every bit set would otherwise pass the
     * whole sequence below. */
    uint64_t pattern = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    if (rc == 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail("a freshly created flag reports a bit that was never set");
    }

    rc = sceKernelSetEventFlag(flag, OBS_EVF_BIT_A);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a bit could not be set", (uint64_t)(uint32_t)rc);
    }

    pattern = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a bit that was set does not poll as set",
                             (uint64_t)(uint32_t)rc);
    }
    if ((pattern & OBS_EVF_BIT_A) == 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("the poll succeeded but returned the wrong pattern", pattern);
    }

    /* A bit nobody set must still not be reported, now that another one is. */
    uint64_t other = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_B, OBS_EVF_WAITMODE_AND, &other);
    if (rc == 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("polling for an unset bit succeeded", other);
    }

    /* Clear, and the argument is a mask of what to **keep**.
     *
     * This check asserted the opposite for a long time - `clear(BIT_A)` and then a poll for
     * BIT_A expected to fail - and shadPS4 and PS5PCEM both failed it with "a cleared bit
     * still polls as set". They were right and this was wrong. shadPS4's `Clear` is
     * `m_bits &= bits`; PS5PCEM's is the same with a comment saying "The PS5 ABI supplies the
     * bits to retain, not the bits to remove."
     *
     * It sat wrong while carrying `OBS_FROM_DOCUMENTED`, the highest rung short of hardware,
     * and the host build agreed with it because the host stub had been written to the same
     * misreading. Two implementations disagreeing with us was the only thing that surfaced
     * it. (D166)
     *
     * So the sequence below sets **two** bits and keeps one. That distinguishes the two
     * readings in a way clearing a single bit cannot: under keep-mask semantics B goes and A
     * stays; under clear-mask semantics A goes and B stays. Either way one bit changes, so a
     * check watching only one bit sees a plausible answer whichever contract is true - which
     * is precisely how this went unnoticed. */
    rc = sceKernelSetEventFlag(flag, OBS_EVF_BIT_B);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a second bit could not be set", (uint64_t)(uint32_t)rc);
    }

    rc = sceKernelClearEventFlag(flag, OBS_EVF_BIT_A);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a bit could not be cleared", (uint64_t)(uint32_t)rc);
    }

    /* The kept bit is still there. */
    pattern = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("the retained bit was cleared: the mask was read as bits to "
                             "remove rather than bits to keep",
                             (uint64_t)(uint32_t)rc);
    }

    /* The unnamed one is gone. */
    uint64_t remaining = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_B, OBS_EVF_WAITMODE_AND, &remaining);
    if (rc == 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("a bit outside the retain mask survived the clear", remaining);
    }

    /* An empty mask keeps nothing, which is the only way to clear everything and the
     * strongest single statement of what the argument means. */
    rc = sceKernelClearEventFlag(flag, 0);
    if (rc != 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("an empty retain mask was refused", (uint64_t)(uint32_t)rc);
    }
    pattern = 0;
    rc = sceKernelPollEventFlag(flag, OBS_EVF_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    if (rc == 0) {
        (void)sceKernelDeleteEventFlag(flag);
        return obs_fail_code("an empty retain mask left a bit set", pattern);
    }

    rc = sceKernelDeleteEventFlag(flag);
    if (rc != 0) {
        return obs_partial_value("the flag worked but would not delete",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_event_flag_rejects_bad_handle(void) {
    uint64_t pattern = 0;
    int rc = sceKernelPollEventFlag(0, OBS_EVF_BIT_A, OBS_EVF_WAITMODE_AND, &pattern);
    if (rc == 0) {
        return obs_partial("polling a null event flag reported success");
    }
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* Counted by the threads themselves, so a platform that reports success without
 * running a body is caught. Not atomic: the threads are joined one at a time below, so
 * only one is ever running. */
static unsigned int obs_churn_ran = 0;

static void *churn_body(void *arg) {
    (void)arg;
    obs_churn_ran++;
    return NULL;
}

static obs_result check_thread_churn(void) {
    OBS_REQUIRE(&scePthreadJoin);
    obs_churn_ran = 0;

    for (unsigned int i = 0; i < OBS_THREAD_CHURN; i++) {
        /* Sparsely, so a crash inside the loop is pinned to within a few iterations.
         * A bug that appears only once a thread cache has filled is invisible to a
         * check that reports nothing until it finishes. */
        if (i % OBS_CHURN_REPORT_EVERY == 0) {
            obs_report_progress("015-sync/thread-churn", (uint64_t)i);
        }
        ScePthread thread = 0;
        int rc = scePthreadCreate(&thread, NULL, churn_body, NULL, "obscene-churn");
        if (rc != 0) {
            return obs_fail_code("a thread could not be created part-way through",
                                 (uint64_t)i);
        }
        rc = scePthreadJoin(thread, NULL);
        if (rc != 0) {
            return obs_fail_code("a thread could not be joined part-way through",
                                 (uint64_t)i);
        }
    }

    if (obs_churn_ran != OBS_THREAD_CHURN) {
        /* Every create and join succeeded and some body did not run. That is worse than
         * an outright failure, because a caller has no way to notice it. */
        return obs_fail_code("fewer thread bodies ran than threads were created",
                             (uint64_t)obs_churn_ran);
    }
    return obs_pass_value((uint64_t)obs_churn_ran);
}

static obs_result check_machine_kind(void) {
    if (!obs_address_is_callable((const void *)&sceKernelIsDevkit) ||
        !obs_address_is_callable((const void *)&sceKernelIsCex)) {
        return obs_skip("the platform does not say which kind of machine it is");
    }
    int devkit = sceKernelIsDevkit();
    int cex = sceKernelIsCex();
    if (devkit != 0 && cex != 0) {
        /* A machine cannot be both a retail unit and a development kit. Either answer
         * alone is fine and this program has no opinion on which; both is a bug. */
        return obs_fail("the platform reports being both a devkit and a retail unit");
    }
    if (devkit == 0 && cex == 0) {
        /* Neither is not an answer either. Reported as partial rather than failed
         * because this program is not certain the pair is exhaustive - there may be a
         * third kind of machine it has never heard of, which is exactly the sort of
         * thing hardware settles and documentation does not. */
        return obs_partial("the platform says it is neither a devkit nor a retail unit");
    }
    /* The value is the interesting part, not the verdict: it says which machine a
     * report came from, which matters when comparing two of them. */
    return obs_pass_value((uint64_t)((devkit != 0 ? 2u : 0u) | (cex != 0 ? 1u : 0u)));
}

/* ---- POSIX primitives ------------------------------------------------------
 *
 * Every one of these is `try`, never a blocking call. A lock whose implementation is
 * broken does not refuse, it hangs, and a probe that hangs loses everything behind it.
 * The pair "a fresh lock can be taken" and "a held lock cannot be taken again" pins the
 * semantics without ever waiting on anything.
 */

static obs_result check_mutex_semantics(void) {
    OBS_REQUIRE(&scePthreadMutexDestroy, &scePthreadMutexInit, &scePthreadMutexUnlock);
    ScePthreadMutex mutex = 0;
    int rc = scePthreadMutexInit(&mutex, NULL, "obscene-mutex");
    if (rc != 0) {
        return obs_fail_code("a mutex could not be created", (uint64_t)(uint32_t)rc);
    }

    rc = scePthreadMutexTrylock(&mutex);
    if (rc != 0) {
        (void)scePthreadMutexDestroy(&mutex);
        return obs_fail_code("a fresh mutex could not be taken", (uint64_t)(uint32_t)rc);
    }

    /* Taking a mutex this thread already holds must fail. A platform that allows it has
     * a lock that does not lock, which every caller above it will believe. */
    rc = scePthreadMutexTrylock(&mutex);
    if (rc == 0) {
        (void)scePthreadMutexUnlock(&mutex);
        (void)scePthreadMutexUnlock(&mutex);
        (void)scePthreadMutexDestroy(&mutex);
        return obs_fail("a mutex already held was taken a second time");
    }

    rc = scePthreadMutexUnlock(&mutex);
    if (rc != 0) {
        (void)scePthreadMutexDestroy(&mutex);
        return obs_fail_code("a held mutex could not be released",
                             (uint64_t)(uint32_t)rc);
    }

    /* And once released it must be takeable again, which is what separates a working
     * unlock from one that merely returned success. */
    rc = scePthreadMutexTrylock(&mutex);
    if (rc != 0) {
        (void)scePthreadMutexDestroy(&mutex);
        return obs_fail_code("a released mutex could not be taken again",
                             (uint64_t)(uint32_t)rc);
    }
    (void)scePthreadMutexUnlock(&mutex);

    rc = scePthreadMutexDestroy(&mutex);
    if (rc != 0) {
        return obs_partial_value("the mutex worked but would not be destroyed",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_rwlock_semantics(void) {
    OBS_REQUIRE(&scePthreadRwlockDestroy,
                &scePthreadRwlockInit,
                &scePthreadRwlockTrywrlock,
                &scePthreadRwlockUnlock);
    ScePthreadRwlock lock = 0;
    int rc = scePthreadRwlockInit(&lock, NULL, "obscene-rwlock");
    if (rc != 0) {
        return obs_fail_code("a read/write lock could not be created",
                             (uint64_t)(uint32_t)rc);
    }

    /* Two readers at once is the whole point of the type. A platform where the second
     * read lock fails has built a mutex and called it a read/write lock. */
    rc = scePthreadRwlockTryrdlock(&lock);
    if (rc != 0) {
        (void)scePthreadRwlockDestroy(&lock);
        return obs_fail_code("a fresh lock could not be taken for reading",
                             (uint64_t)(uint32_t)rc);
    }
    rc = scePthreadRwlockTryrdlock(&lock);
    if (rc != 0) {
        (void)scePthreadRwlockUnlock(&lock);
        (void)scePthreadRwlockDestroy(&lock);
        return obs_fail_code("a second reader was refused", (uint64_t)(uint32_t)rc);
    }

    /* A writer must not get in while readers hold it. */
    rc = scePthreadRwlockTrywrlock(&lock);
    if (rc == 0) {
        (void)scePthreadRwlockUnlock(&lock);
        (void)scePthreadRwlockUnlock(&lock);
        (void)scePthreadRwlockUnlock(&lock);
        (void)scePthreadRwlockDestroy(&lock);
        return obs_fail("a writer was let in while readers held the lock");
    }

    (void)scePthreadRwlockUnlock(&lock);
    (void)scePthreadRwlockUnlock(&lock);

    /* With the readers gone, a writer must get in. */
    rc = scePthreadRwlockTrywrlock(&lock);
    if (rc != 0) {
        (void)scePthreadRwlockDestroy(&lock);
        return obs_fail_code("a writer was refused an unheld lock",
                             (uint64_t)(uint32_t)rc);
    }
    (void)scePthreadRwlockUnlock(&lock);

    rc = scePthreadRwlockDestroy(&lock);
    if (rc != 0) {
        return obs_partial_value("the lock worked but would not be destroyed",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_semaphore_counts(void) {
    OBS_REQUIRE(&sceKernelCreateSema, &sceKernelDeleteSema, &sceKernelSignalSema);
    int sema = 0;
    /* Created empty, so the first poll must fail. Starting it full would let a platform
     * that ignores the count pass the first step by accident. */
    int rc = sceKernelCreateSema(&sema, "obscene-sema", 0, 0, 8, NULL);
    if (rc != 0) {
        return obs_fail_code("a semaphore could not be created", (uint64_t)(uint32_t)rc);
    }

    rc = sceKernelPollSema(sema, 1);
    if (rc == 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail("an empty semaphore handed out a token");
    }

    rc = sceKernelSignalSema(sema, 2);
    if (rc != 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail_code("a semaphore could not be signalled",
                             (uint64_t)(uint32_t)rc);
    }

    /* Two were put in, so two come out and a third does not. That is the count being
     * kept, rather than a flag being set. */
    if (sceKernelPollSema(sema, 1) != 0 || sceKernelPollSema(sema, 1) != 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail("a semaphore signalled twice would not yield two tokens");
    }
    if (sceKernelPollSema(sema, 1) == 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail("a semaphore yielded more tokens than were put in");
    }

    rc = sceKernelDeleteSema(sema);
    if (rc != 0) {
        return obs_partial_value("the semaphore worked but would not be deleted",
                                 (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

/* ---- condition variables and barriers ----------------------------------------
 *
 * Both were deferred because testing them meaningfully needs a second thread that waits,
 * and a waiter needs a timeout or a broken implementation hangs the run.
 *
 * That reasoning held for the *interesting* cases and was applied to the whole subsystem.
 * Two operations here cannot block at all, and they were available the whole time:
 *
 *   * a barrier of one is satisfied by the calling thread;
 *   * signalling a condition variable nobody is waiting on has no effect and returns.
 *
 * # What these do not establish
 *
 * That a waiter is ever woken. Nothing below blocks, so nothing below proves the wakeup
 * path works at all - which is the half that actually matters to a title. Stated here
 * rather than left for a reader to assume, because a green subsystem that has never been
 * waited on is exactly the kind of false confidence this project exists to avoid.
 *
 * What they do establish is that the objects exist, that creating and destroying them
 * works repeatedly, and that the non-blocking operations behave. A platform failing any
 * of these cannot possibly get the blocking ones right.
 */

static obs_result check_condvar_lifecycle(void) {
    OBS_REQUIRE(&scePthreadCondDestroy, &scePthreadCondSignal, &scePthreadCondBroadcast);

    ScePthreadCond cond = 0;
    int rc = scePthreadCondInit(&cond, NULL, "obscene-cond");
    if (rc != 0) {
        return obs_fail_code("a condition variable could not be created",
                             (uint64_t)(uint32_t)rc);
    }
    if (cond == 0) {
        return obs_fail("creation reported success and handed back nothing");
    }

    /* Signalling with nobody waiting is defined to have no effect, and to succeed. It
     * cannot block, because there is no thread to hand the lock to.
     *
     * This is the whole trick that made the subsystem testable: the operation with no
     * waiter is the operation with no wait. */
    rc = scePthreadCondSignal(&cond);
    if (rc != 0) {
        (void)scePthreadCondDestroy(&cond);
        return obs_fail_code("signalling an unwaited condition variable failed",
                             (uint64_t)(uint32_t)rc);
    }
    rc = scePthreadCondBroadcast(&cond);
    if (rc != 0) {
        (void)scePthreadCondDestroy(&cond);
        return obs_fail_code("broadcasting to an unwaited condition variable failed",
                             (uint64_t)(uint32_t)rc);
    }
    /* Twice, because an implementation that consumes something on the first signal and
     * has nothing to consume on the second is a real shape of bug and invisible to one
     * call. */
    if (scePthreadCondSignal(&cond) != 0) {
        (void)scePthreadCondDestroy(&cond);
        return obs_fail("a second signal failed where the first succeeded");
    }

    rc = scePthreadCondDestroy(&cond);
    if (rc != 0) {
        return obs_fail_code("a condition variable could not be destroyed",
                             (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_barrier_of_one_releases(void) {
    OBS_REQUIRE(&scePthreadBarrierDestroy, &scePthreadBarrierWait);

    /* A barrier releases when its count of threads have arrived. Set the count to one and
     * the calling thread is that count, so `Wait` returns immediately - no second thread,
     * no timeout, and no way for a correct implementation to block.
     *
     * A platform that hangs here has a barrier that does not understand its own count,
     * which is worth finding. The risk is real and bounded: it is the only call in this
     * suite that could block, and it can only do so on an implementation that is wrong in
     * a way nothing else would reveal. */
    ScePthreadBarrier barrier = 0;
    int rc = scePthreadBarrierInit(&barrier, NULL, 1u, "obscene-barrier");
    if (rc != 0) {
        return obs_fail_code("a barrier could not be created", (uint64_t)(uint32_t)rc);
    }
    if (barrier == 0) {
        return obs_fail("creation reported success and handed back nothing");
    }

    rc = scePthreadBarrierWait(&barrier);
    /* POSIX returns a distinguished value to exactly one thread of the group and zero to
     * the rest, so a non-zero return here is not necessarily a failure. What matters is
     * that it *returned*. Asserting which value would be inventing a specification for a
     * renamed call - see D074. */
    (void)rc;

    /* Again, because a barrier is reusable: after releasing, it must serve another round.
     * An implementation that latches open or stays closed fails here and passes above. */
    rc = scePthreadBarrierWait(&barrier);
    (void)rc;

    rc = scePthreadBarrierDestroy(&barrier);
    if (rc != 0) {
        return obs_fail_code("a barrier could not be destroyed", (uint64_t)(uint32_t)rc);
    }
    return obs_pass();
}

static obs_result check_condvar_handles_distinct(void) {
    OBS_REQUIRE(&scePthreadCondDestroy);

    /* The handle-table relation, on the last synchronisation type that lacked it. */
    enum { COUNT = 6 };
    ScePthreadCond conds[COUNT] = {0};
    unsigned int made = 0;
    for (unsigned int i = 0; i < COUNT; i++) {
        if (scePthreadCondInit(&conds[i], NULL, "obscene-distinct") != 0) {
            break;
        }
        made++;
    }
    int duplicate = 0;
    for (unsigned int i = 0; i < made; i++) {
        for (unsigned int j = i + 1u; j < made; j++) {
            if (conds[i] == conds[j]) {
                duplicate = 1;
            }
        }
    }
    for (unsigned int i = 0; i < made; i++) {
        (void)scePthreadCondDestroy(&conds[i]);
    }
    if (made < 2) {
        return obs_skip("fewer than two condition variables could be created");
    }
    if (duplicate) {
        return obs_fail_code("two live condition variables share a handle",
                             (uint64_t)made);
    }
    return obs_pass_value((uint64_t)made);
}

/* ---- is a waiter ever woken? -------------------------------------------------
 *
 * The half the checks above cannot reach. Nothing that does not block can prove a wakeup
 * works, and it is the half a title depends on.
 *
 * # Why this needs no timeout
 *
 * The deferral assumed a waiter must be given a deadline, or a broken implementation
 * hangs the run. That is true of a thread that waits - and **the run is not that thread**.
 *
 * The main thread never enters a blocking primitive. It spawns a waiter, sleeps a fixed
 * interval, signals, sleeps again, and reads a flag. Every step has a bound this program
 * chose, so a wakeup that never arrives leaves the *waiter* stuck while the run carries on
 * and reports exactly that.
 *
 * The stuck thread is not joined, deliberately: joining it would import the hang the whole
 * design avoids. It is left where it is, and `exit` takes it down with the process.
 *
 * # The telemetry
 *
 * A flag the waiter advances through known states, read from the main thread. It says more
 * than "did it come back": `WAITING` and never `WOKEN` is a wakeup that never arrived,
 * while never reaching `WAITING` means the waiter died before it got there - a different
 * fault with a different cause, and one a boolean would have merged with the first.
 *
 * `volatile`, not atomic: this is freestanding and `stdatomic.h` is not guaranteed. In
 * practice the sleeps between writes and reads involve the kernel, which is enough on
 * this architecture. Stated because it is a real limitation and not a rigorous one.
 */

#define OBS_WAKE_START 0u
#define OBS_WAKE_LOCKED 1u
#define OBS_WAKE_WAITING 2u
#define OBS_WAKE_WOKEN 3u

static volatile unsigned int obs_wake_state = OBS_WAKE_START;
static ScePthreadCond obs_wake_cond = 0;
static ScePthreadMutex obs_wake_mutex = 0;

static void *obs_wake_waiter(void *arg) {
    (void)arg;
    if (scePthreadMutexTrylock(&obs_wake_mutex) != 0) {
        return NULL;
    }
    obs_wake_state = OBS_WAKE_LOCKED;
    obs_wake_state = OBS_WAKE_WAITING;
    /* The one genuinely unbounded call in this program, and it is on a thread nobody
     * waits for. If it never returns, this thread stays here and the run does not. */
    (void)scePthreadCondWait(&obs_wake_cond, &obs_wake_mutex);
    obs_wake_state = OBS_WAKE_WOKEN;
    (void)scePthreadMutexUnlock(&obs_wake_mutex);
    return NULL;
}

static obs_result check_condvar_wakes_a_waiter(void) {
    OBS_REQUIRE(&scePthreadCondInit, &scePthreadCondDestroy, &scePthreadCondSignal,
                &scePthreadCondWait, &scePthreadMutexInit, &scePthreadMutexDestroy,
                &scePthreadMutexTrylock, &scePthreadMutexUnlock, &scePthreadCreate,
                &sceKernelUsleep);

    obs_wake_state = OBS_WAKE_START;
    if (scePthreadMutexInit(&obs_wake_mutex, NULL, "obscene-wake") != 0) {
        return obs_skip("no mutex to wait against");
    }
    if (scePthreadCondInit(&obs_wake_cond, NULL, "obscene-wake") != 0) {
        (void)scePthreadMutexDestroy(&obs_wake_mutex);
        return obs_skip("no condition variable to wait on");
    }

    ScePthread waiter = NULL;
    int rc = scePthreadCreate(&waiter, NULL, obs_wake_waiter, NULL, "obscene-waiter");
    if (rc != 0) {
        (void)scePthreadCondDestroy(&obs_wake_cond);
        (void)scePthreadMutexDestroy(&obs_wake_mutex);
        return obs_skip("no thread to wait with");
    }

    /* Long enough for the waiter to reach the wait on any plausible scheduler. Too short
     * and the signal arrives before anybody is listening, which looks identical to a
     * wakeup that does not work - so this is generous on purpose. */
    (void)sceKernelUsleep(50000u);

    if (obs_wake_state < OBS_WAKE_WAITING) {
        /* It never got as far as waiting, so nothing has been learned about wakeups. A
         * failure here belongs to the mutex or the thread, not to the condition
         * variable. */
        return obs_partial_value("the waiter never reached the wait",
                                 (uint64_t)obs_wake_state);
    }

    (void)scePthreadCondSignal(&obs_wake_cond);
    (void)sceKernelUsleep(50000u);

    unsigned int state = obs_wake_state;
    /* Nothing is destroyed and nothing is joined when the waiter is still inside the
     * wait: destroying a condition variable somebody is blocked on is undefined, and
     * joining is the hang this design exists to avoid. Leaking two handles on a platform
     * that is already broken is the cheaper mistake. */
    if (state == OBS_WAKE_WOKEN) {
        (void)scePthreadCondDestroy(&obs_wake_cond);
        (void)scePthreadMutexDestroy(&obs_wake_mutex);
        return obs_pass();
    }
    return obs_fail_code("a signalled waiter was never woken", (uint64_t)state);
}


/* ---- mutex attributes: the recursion policy, settled rather than assumed ----
 *
 * Requested by the sibling project, whose threading layer picks non-recursive "because it
 * is POSIX's" and has no way to check. The consequence of guessing wrong is not a wrong
 * answer, it is a whole-process deadlock the emulator caused: a guest that re-takes a lock
 * it already holds proceeds under one policy and stops forever under the other.
 *
 * # Trylock, never Lock - and here that is not a style rule
 *
 * The obvious probe is `Settype(RECURSIVE)`, then lock twice, and see whether the second
 * call returns. It hangs **exactly when the answer is "not recursive"** - which is the case
 * being tested for - taking the rest of the suite with it. `scePthreadMutexLock` is not
 * declared in this program for that reason, and is not declared for this either.
 *
 * `Trylock` answers the same question and always comes back. It also answers it in more
 * detail: success means the second acquisition was allowed, and the failure code
 * distinguishes "already held" from "deadlock refused" where the platform bothers to.
 *
 * # No type constant is invented
 *
 * POSIX names three mutex types and fixes none of their values, so hardcoding one would be
 * the invention D008 forbids, and the check would then be measuring a guess. The probe
 * sweeps a small range of candidates and records what the platform said about each. Which
 * values are accepted is itself a finding, and one no header states.
 *
 * # Why the range runs to 4 and starts at 0
 *
 * It ran to 3 first, chosen for no reason beyond "three named types, so a little wider". That
 * missed a real value. Kyty's `PthreadMutexattrSettype` maps the argument explicitly:
 *
 *     case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
 *     case 2: ptype = PTHREAD_MUTEX_RECURSIVE;  break;
 *     case 3:
 *     case 4: ptype = PTHREAD_MUTEX_NORMAL;     break;
 *     default: EXIT("invalid type: %d", type);
 *
 * So the accepted set is one-based `{1, 2, 3, 4}`, and *not* the POSIX values, where NORMAL is
 * 0. A sweep of 0..3 therefore spent a quarter of its range on a value that implementation
 * rejects outright, while never trying one it accepts. `IMPLEMENTATIONS`, not `SPEC`: it is
 * one emulator's reading, which is why this still sweeps rather than encoding the mapping.
 *
 * 0 is kept deliberately. If it really is invalid, a platform refusing it is a *result* - and
 * it is the value a POSIX-shaped guess reaches for first, so knowing it is refused earns a
 * slot.
 */
#define OBS_MUTEX_TYPE_CANDIDATES 5

/* One quantity name per candidate, because the loop emits a record per type and a report
 * that says "second-acquisition" four times cannot say which type was recursive - which is
 * the single fact this exists to produce. Built as a table rather than formatted, since the
 * runtime has no string formatting and should not grow any for this. */
static const char *const obs_type_quantity[OBS_MUTEX_TYPE_CANDIDATES] = {
    "type-0-second-acquisition",
    "type-1-second-acquisition",
    "type-2-second-acquisition",
    "type-3-second-acquisition",
    "type-4-second-acquisition",
};

/* One quantity name per candidate for the round-trip, for the same reason: the count of types that
 * round-trip does not say WHICH one does not, and matching an emulator to hardware needs exactly
 * that. Each records what `Gettype` reads back after `Settype(type)`, or -1 where `Settype` refused
 * the type or `Gettype` failed - so `read-back == type` is a clean round-trip, a differing value is a
 * normalisation, and -1 is a refusal. */
static const char *const obs_type_readback[OBS_MUTEX_TYPE_CANDIDATES] = {
    "type-0-read-back",
    "type-1-read-back",
    "type-2-read-back",
    "type-3-read-back",
    "type-4-read-back",
};

static obs_result check_mutexattr_round_trip(void) {
    OBS_REQUIRE(&scePthreadMutexattrInit, &scePthreadMutexattrDestroy,
                &scePthreadMutexattrSettype, &scePthreadMutexattrGettype);

    ScePthreadMutexattr attr = NULL;
    int rc = scePthreadMutexattrInit(&attr);
    if (rc != 0) {
        return obs_fail_code("the attribute object could not be initialised", (uint64_t)rc);
    }

    /* What a fresh attribute reports before anything is set. The default policy is the
     * thing an implementation has to choose, and this is the platform stating it. */
    int initial = -1;
    int got_initial = scePthreadMutexattrGettype(&attr, &initial);
    obs_report_measure("015-sync/mutexattr-round-trip", "scePthreadMutexattrGettype",
                       "default-type",
                       got_initial == 0 ? (uint64_t)(int64_t)initial : (uint64_t)0,
                       "type");

    unsigned int accepted = 0;
    unsigned int stored = 0;
    for (int type = 0; type < OBS_MUTEX_TYPE_CANDIDATES; type++) {
        int set_rc = scePthreadMutexattrSettype(&attr, type);
        int read_back = -1;
        int get_rc = -1;
        if (set_rc == 0) {
            accepted++;
            get_rc = scePthreadMutexattrGettype(&attr, &read_back);
            if (get_rc == 0 && read_back == type) {
                stored++;
            }
        }
        /* What this type actually did, per type, so a diff names the one that does not round-trip
         * (the count says four of five do, not which). `read_back` when the pair succeeded, -1 when
         * `Settype` refused the type or `Gettype` failed. */
        obs_report_measure("015-sync/mutexattr-round-trip", "scePthreadMutexattrGettype",
                           obs_type_readback[type],
                           (set_rc == 0 && get_rc == 0) ? (uint64_t)(int64_t)read_back
                                                        : (uint64_t)(int64_t)(-1),
                           "type");
    }
    (void)scePthreadMutexattrDestroy(&attr);

    if (accepted == 0) {
        return obs_fail("no mutex type value was accepted, so the policy cannot be set");
    }
    /* The question the sibling asked: does the attribute object carry state at all? An
     * implementation that accepts a type and stores nothing looks identical from the
     * return code alone, and a Get on it reads whatever the caller's stack held. */
    if (stored == 0) {
        return obs_fail_code("types are accepted but nothing is stored; the attribute "
                             "object is inert",
                             (uint64_t)accepted);
    }
    if (stored < accepted) {
        return obs_partial_value("some accepted types did not read back", (uint64_t)stored);
    }
    return obs_pass_value((uint64_t)accepted);
}

/* Does a mutex configured with each accepted type let its owner take it twice?
 *
 * Recorded per type rather than reduced to one verdict, because "which value means
 * recursive" is exactly what nobody can state - and a report saying "type 2 allowed a
 * second acquisition, types 0, 1, 3 and 4 refused it" answers that without naming a
 * constant.
 */
static obs_result check_mutex_recursion(void) {
    OBS_REQUIRE(&scePthreadMutexattrInit, &scePthreadMutexattrDestroy,
                &scePthreadMutexattrSettype, &scePthreadMutexInit, &scePthreadMutexDestroy,
                &scePthreadMutexTrylock, &scePthreadMutexUnlock);

    unsigned int recursive = 0;
    unsigned int refused = 0;
    for (int type = 0; type < OBS_MUTEX_TYPE_CANDIDATES; type++) {
        ScePthreadMutexattr attr = NULL;
        if (scePthreadMutexattrInit(&attr) != 0) {
            continue;
        }
        if (scePthreadMutexattrSettype(&attr, type) != 0) {
            (void)scePthreadMutexattrDestroy(&attr);
            continue;
        }
        ScePthreadMutex mutex = NULL;
        if (scePthreadMutexInit(&mutex, &attr, "obscene-recursion") != 0) {
            (void)scePthreadMutexattrDestroy(&attr);
            continue;
        }
        if (scePthreadMutexTrylock(&mutex) == 0) {
            int second = scePthreadMutexTrylock(&mutex);
            obs_report_measure("015-sync/mutex-recursion", "scePthreadMutexTrylock",
                               obs_type_quantity[type], (uint64_t)(int64_t)second,
                               "code");
            if (second == 0) {
                recursive++;
                /* Taken twice, so released twice. Leaving a recursive mutex held once over
                 * would leak a lock into every check behind this one. */
                (void)scePthreadMutexUnlock(&mutex);
            } else {
                refused++;
            }
            (void)scePthreadMutexUnlock(&mutex);
        }
        (void)scePthreadMutexDestroy(&mutex);
        (void)scePthreadMutexattrDestroy(&attr);
    }

    if (recursive == 0 && refused == 0) {
        return obs_fail("no mutex could be created from any attribute type");
    }
    /* Zero is a real answer, not a failure: it says no candidate type made this platform
     * recursive, which is what an implementation choosing a default needs to know. */
    return obs_pass_value((uint64_t)recursive);
}

/* What does unlocking a mutex nobody holds return?
 *
 * The sibling refuses the operation and guesses the code. This asks. An unlock never waits,
 * so it is safe in a way the lock probes are not, and it is the cheapest of the set.
 *
 * Deliberately *unheld* rather than held-by-another-thread. Both are worth knowing and only
 * one is deterministic without a rendezvous: the other needs a second thread to be holding
 * the lock at the moment of the call, which is an interleaving, and a probe whose answer
 * depends on scheduling is worse than no probe.
 */
static obs_result check_mutex_unlock_unheld(void) {
    OBS_REQUIRE(&scePthreadMutexInit, &scePthreadMutexDestroy, &scePthreadMutexUnlock);

    ScePthreadMutex mutex = NULL;
    int rc = scePthreadMutexInit(&mutex, NULL, "obscene-unheld");
    if (rc != 0) {
        return obs_fail_code("a default mutex could not be created", (uint64_t)rc);
    }
    int unlocked = scePthreadMutexUnlock(&mutex);
    obs_report_measure("015-sync/mutex-unlock-unheld", "scePthreadMutexUnlock",
                       "unheld-unlock", (uint64_t)(int64_t)unlocked, "code");
    (void)scePthreadMutexDestroy(&mutex);

    if (unlocked == 0) {
        /* Permitted by POSIX for a plain mutex, and worth flagging rather than passing
         * silently: an implementation that allows it puts two holders in one critical
         * section the moment a guest relies on it. */
        return obs_partial("unlocking an unheld mutex reported success");
    }
    return obs_pass_value((uint64_t)(int64_t)unlocked);
}


static const obs_check sync_checks[] = {
    {"015-sync/mutexattr-round-trip", "libkernel", "scePthreadMutexattrSettype",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexattrSettype,
     check_mutexattr_round_trip, OBS_FROM_DERIVED},
    {"015-sync/mutex-recursion", "libkernel", "scePthreadMutexattrSettype",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexattrSettype,
     check_mutex_recursion, OBS_FROM_ASSUMED},
    {"015-sync/mutex-unlock-unheld", "libkernel", "scePthreadMutexUnlock", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadMutexUnlock, check_mutex_unlock_unheld,
     OBS_FROM_SPEC},

    {"015-sync/mutex", "libkernel", "scePthreadMutexTrylock", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadMutexTrylock, check_mutex_semantics, OBS_FROM_SPEC},
    {"015-sync/rwlock", "libkernel", "scePthreadRwlockTryrdlock", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadRwlockTryrdlock, check_rwlock_semantics,
     OBS_FROM_SPEC},
    {"015-sync/semaphore", "libkernel", "sceKernelPollSema", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelPollSema, check_semaphore_counts, OBS_FROM_SPEC},
    /* `IMPLEMENTATIONS`, and this check is why that rung exists.
     *
     * It was the only check in the suite claiming `DOCUMENTED` - a vendor document describes
     * this behaviour specifically - and it had the behaviour backwards. What supports it is
     * shadPS4's `Clear` (`m_bits &= bits`, C++) and PS5PCEM's `clearEventFlag`
     * (`object.bits &= mask`, Zig, with the comment "The PS5 ABI supplies the bits to retain,
     * not the bits to remove"). Two implementations, two languages, no shared codebase.
     *
     * It sat at `ASSUMED` for as long as it took to conclude the ladder was missing a rung
     * rather than that this was a guess: ASSUMED says the project reasoned it out, which
     * discards the fact that somebody's working code says so. (D166, D169)
     *
     * This comment belongs **above** the row, not inside its braces. Put between the
     * capability fields and the runner it made the row invisible to every tool that parses
     * these tables - `guards` stopped checking it, `caps` stopped ordering it and `counts`
     * under-reported - because the parser requires the field before `OBS_FROM_*` to be an
     * identifier, and a comment is not one. The check still ran; nothing said otherwise.
     * (D168) */
    {"015-sync/event-flag-round-trip", "libkernel", "sceKernelPollEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelPollEventFlag,
     check_event_flag_round_trip, OBS_FROM_IMPLEMENTATIONS},
    {"015-sync/event-flag-rejects-bad-handle", "libkernel", "sceKernelPollEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelPollEventFlag,
     check_event_flag_rejects_bad_handle, OBS_FROM_ASSUMED},
    {"015-sync/thread-churn", "libkernel", "scePthreadCreate", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadCreate, check_thread_churn, OBS_FROM_SPEC},
    {"015-sync/machine-kind", "libkernel", "sceKernelIsCex", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelIsCex, check_machine_kind, OBS_FROM_ASSUMED},
    {"015-sync/condvar-lifecycle", "libkernel", "scePthreadCondInit", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadCondInit, check_condvar_lifecycle,
     OBS_FROM_DERIVED},
    {"015-sync/condvar-handles-distinct", "libkernel", "scePthreadCondInit",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadCondInit,
     check_condvar_handles_distinct, OBS_FROM_SPEC},
    {"015-sync/barrier-of-one-releases", "libkernel", "scePthreadBarrierInit",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadBarrierInit,
     check_barrier_of_one_releases, OBS_FROM_DERIVED},
    {"015-sync/condvar-wakes-a-waiter", "libkernel", "scePthreadCondWait", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadCondWait, check_condvar_wakes_a_waiter,
     OBS_FROM_DERIVED},
};

const obs_section obs_section_sync = {
    "015-sync",
    "Synchronisation and thread churn",
    "Primitives with real semantics rather than existence tests, chosen from what an "
    "emulator's release notes said it had just fixed. Every one of these was censused "
    "and never called while it was broken.",
    sync_checks,
    OBS_COUNT(sync_checks),
};
