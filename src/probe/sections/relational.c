/*
 * Properties the platform must have, whatever its answers are.
 *
 * # Why this section exists
 *
 * Every other behavioural check compares a result against an expected value, and needs
 * something to supply that value. ISO C and POSIX supply it for the C library. For the
 * vendor surface nothing does - the functions are documented, their exact returns for a
 * given input generally are not - so those checks are `assumed`, and an emulator built
 * to satisfy an assumption has only been made to agree with us.
 *
 * There is a way to test without an oracle: compare results **to each other** rather than
 * to an expected value. Two calls that must differ, an operation and its inverse, a
 * sequence that must be repeatable. These hold whatever the platform's actual numbers
 * are, so they need no authority to check - and they fail loudly on the implementations
 * that matter, because a stub cannot satisfy a relation it does not track state for.
 *
 * `007-responsive` already does this, and does it well. It was aimed at libc and maths,
 * which is exactly where ISO C already gave a free oracle and the technique was needed
 * least. This section aims it where there is no oracle at all.
 *
 * # These need no struct layout and no documented error code
 *
 * That is what makes them available now. `docs/BACKLOG.md` §2 records struct-taking
 * functions as blocked on layouts, and §5 records the suite as leaning negative. A
 * relation routes around both: it takes handles and integers, and it asks about the
 * relationship between two results rather than about either one's value.
 *
 * # What a failure here means
 *
 * Something stronger than a wrong number. "Two opens of the same path returned the same
 * descriptor" is not a matter of interpretation - no console does that, and a caller
 * that closes one has closed the other. These are the faults that produce corruption
 * somewhere unrelated, much later, which is the hardest kind to find from a report.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Distinctness over a small set, without allocating or sorting.
 *
 * Quadratic, and that is the right trade at these sizes: the alternative needs either
 * ordering the handles, which assumes they are comparable in a way nothing promises, or
 * a hash, which is more code to be wrong in a program whose value is being trustworthy. */
static int all_distinct(const uint64_t *values, unsigned int count) {
    for (unsigned int i = 0; i < count; i++) {
        for (unsigned int j = i + 1u; j < count; j++) {
            if (values[i] == values[j]) {
                return 0;
            }
        }
    }
    return 1;
}

static obs_result check_event_flag_handles_distinct(void) {
    OBS_REQUIRE(&sceKernelDeleteEventFlag);

    /* Eight is enough to catch a handle table that hands out one slot repeatedly, and
     * few enough that a platform with a small limit is not being failed for its limit.
     * A create that refuses is reported as its own outcome rather than as a duplicate. */
    enum { COUNT = 8 };
    SceKernelEventFlag flags[COUNT] = {0};
    uint64_t handles[COUNT] = {0};
    unsigned int made = 0;

    for (unsigned int i = 0; i < COUNT; i++) {
        int rc = sceKernelCreateEventFlag(&flags[i], "obscene-distinct",
                                          OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0,
                                          NULL);
        if (rc != 0) {
            break;
        }
        handles[made] = (uint64_t)(uintptr_t)flags[i];
        made++;
    }

    for (unsigned int i = 0; i < made; i++) {
        (void)sceKernelDeleteEventFlag(flags[i]);
    }

    if (made < 2) {
        return obs_skip("fewer than two event flags could be created to compare");
    }
    /* The relation. Nothing here says what a handle should be - only that two live
     * objects cannot be the same one. An implementation returning a fixed handle passes
     * every existing check in 015-sync and fails this. */
    if (!all_distinct(handles, made)) {
        return obs_fail_code("two live event flags share a handle", (uint64_t)made);
    }
    return obs_pass_value((uint64_t)made);
}

static obs_result check_event_flag_handles_reusable(void) {
    OBS_REQUIRE(&sceKernelDeleteEventFlag);

    /* Create, delete, create again. A platform that leaks its handle table fails the
     * second round; one that works is unaffected. The count is deliberately larger than
     * the round above, because a leak of one slot per cycle needs a few cycles to show.
     *
     * This is the relational form of a resource-exhaustion test that does not need to
     * know the limit - which is what makes it safe to run. */
    enum { ROUNDS = 6 };
    for (unsigned int round = 0; round < ROUNDS; round++) {
        SceKernelEventFlag flag = 0;
        int rc = sceKernelCreateEventFlag(&flag, "obscene-reuse",
                                          OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0,
                                          NULL);
        if (rc != 0) {
            if (round == 0) {
                return obs_skip("no event flag could be created to cycle");
            }
            /* The interesting failure: it worked and then stopped working, which is a
             * leak rather than an absence. The round is the value, so a reader knows
             * how many it took. */
            return obs_fail_code("creation stopped working after repeated create/delete",
                                 (uint64_t)round);
        }
        (void)sceKernelDeleteEventFlag(flag);
    }
    return obs_pass_value((uint64_t)ROUNDS);
}

static obs_result check_direct_memory_round_trip(void) {
    OBS_REQUIRE(&sceKernelReleaseDirectMemory, &sceKernelGetDirectMemorySize);

    /* Allocate, release, allocate the same size again. The second allocation must
     * succeed: an allocator that does not return released memory to its pool fails
     * here and passes every value check in 020-memory, because each individual call
     * behaves correctly.
     *
     * The size is small and the alignment is the console's page granularity, so this is
     * a request any platform can satisfy if it can satisfy any. */
    const size_t size = 0x4000;
    sce_off_t search = 0;
    sce_off_t first = 0;
    int rc = sceKernelAllocateDirectMemory(search, (sce_off_t)sceKernelGetDirectMemorySize(),
                                           size, 0x4000, OBS_MEM_TYPE_WB_ONION, &first);
    if (rc != 0) {
        return obs_skip("no direct memory could be allocated to cycle");
    }
    rc = sceKernelReleaseDirectMemory(first, size);
    if (rc != 0) {
        return obs_fail_code("a fresh allocation could not be released",
                             (uint64_t)(uint32_t)rc);
    }
    sce_off_t second = 0;
    rc = sceKernelAllocateDirectMemory(search, (sce_off_t)sceKernelGetDirectMemorySize(),
                                       size, 0x4000, OBS_MEM_TYPE_WB_ONION, &second);
    if (rc != 0) {
        return obs_fail_code("the same allocation failed after being released",
                             (uint64_t)(uint32_t)rc);
    }
    (void)sceKernelReleaseDirectMemory(second, size);
    /* Deliberately not asserting the two offsets match. A platform is entitled to hand
     * back a different region, and requiring the same one would be inventing a policy.
     * That it succeeds at all is the property. */
    return obs_pass_value((uint64_t)second);
}

static obs_result check_semaphore_count_is_tracked(void) {
    OBS_REQUIRE(&sceKernelDeleteSema, &sceKernelSignalSema, &sceKernelPollSema);

    /* A counting semaphore is a count. Signal it twice, and two polls must succeed and
     * a third must not.
     *
     * No expected error code appears here: the third poll is required to *differ in
     * outcome* from the first two, not to return a particular value. That keeps it free
     * of the assumption every negative check in this suite carries, and it is the whole
     * reason this section can be `spec` rather than `assumed` - a counting semaphore
     * that does not count is not a matter of opinion. */
    int sema = 0;
    if (sceKernelCreateSema(&sema, "obscene-count", 0, 0, 4, NULL) != 0) {
        return obs_skip("no semaphore could be created");
    }
    int first_signal = sceKernelSignalSema(sema, 1);
    int second_signal = sceKernelSignalSema(sema, 1);
    if (first_signal != 0 || second_signal != 0) {
        (void)sceKernelDeleteSema(sema);
        return obs_fail("a semaphore below its maximum refused a signal");
    }
    int first_poll = sceKernelPollSema(sema, 1);
    int second_poll = sceKernelPollSema(sema, 1);
    int third_poll = sceKernelPollSema(sema, 1);
    (void)sceKernelDeleteSema(sema);

    if (first_poll != 0 || second_poll != 0) {
        return obs_fail("two signals did not yield two successful polls");
    }
    if (third_poll == 0) {
        /* The one that catches a stub returning success to everything - and the one an
         * existence test and a negative check both miss. */
        return obs_fail("a third poll succeeded against only two signals");
    }
    return obs_pass();
}

static obs_result check_thread_identity_is_stable(void) {
    OBS_REQUIRE(&scePthreadSelf);

    /* Asking twice from the same thread must give the same answer, and it must not be
     * nothing. Two relations, no expected value: what a thread identifier *is* is the
     * platform's business.
     *
     * A stub returning a fresh value each call passes any check that only tests for
     * non-null, and fails this. */
    ScePthread first = scePthreadSelf();
    ScePthread second = scePthreadSelf();
    if (first != second) {
        return obs_fail("the same thread reported two different identities");
    }
    if (first == NULL) {
        return obs_fail("the running thread has no identity");
    }
    return obs_pass_value((uint64_t)(uintptr_t)first);
}

static obs_result check_descriptors_distinct(void) {
    OBS_REQUIRE(&sceKernelClose, &sceKernelLseek);

    /* Two opens of the same path must give two descriptors, and closing one must not
     * close the other.
     *
     * This is the relation with the worst failure mode in the list. An implementation
     * handing back the same descriptor twice passes every check in 040-file - each open
     * succeeds, each read works - and then a caller that closes one has silently closed
     * the other's. What breaks is somewhere else, later, in code that did nothing wrong.
     *
     * The path is one every platform has: this program's own module. Reading is never
     * attempted, so no assumption is made about content. */
    static const char *const path = "/app0/eboot.bin";
    int first = sceKernelOpen(path, OBS_O_RDONLY, 0);
    if (first < 0) {
        return obs_skip("nothing could be opened to compare");
    }
    int second = sceKernelOpen(path, OBS_O_RDONLY, 0);
    if (second < 0) {
        (void)sceKernelClose(first);
        /* Not a failure: a platform is entitled to a one-open-per-file policy, and
         * calling that a bug would be inventing a rule. Recorded as a skip so the
         * distinction survives into the report. */
        return obs_skip("the same path could not be opened twice");
    }
    if (first == second) {
        (void)sceKernelClose(first);
        return obs_fail_code("two opens of one path returned the same descriptor",
                             (uint64_t)(uint32_t)first);
    }
    /* Closing one must leave the other usable. `lseek` to the current position is the
     * least invasive way to ask "is this still a live descriptor" - it moves nothing and
     * needs no buffer. */
    (void)sceKernelClose(first);
    sce_off_t still_open = sceKernelLseek(second, 0, OBS_SEEK_CUR);
    (void)sceKernelClose(second);
    if (still_open < 0) {
        return obs_fail("closing one descriptor invalidated the other");
    }
    return obs_pass();
}

static obs_result check_close_is_not_idempotent(void) {
    OBS_REQUIRE(&sceKernelOpen);

    /* Closing twice must not succeed twice.
     *
     * A close that reports success on an already-closed descriptor is how a
     * double-close bug hides: the first free happens, the second is silently accepted,
     * and the descriptor gets reused underneath somebody. No expected error code is
     * asserted - only that the second outcome differs from the first, which is the
     * relation and needs no document. */
    int fd = sceKernelOpen("/app0/eboot.bin", OBS_O_RDONLY, 0);
    if (fd < 0) {
        return obs_skip("nothing could be opened to close");
    }
    int first = sceKernelClose(fd);
    if (first != 0) {
        return obs_fail_code("a freshly opened descriptor could not be closed",
                             (uint64_t)(uint32_t)first);
    }
    int second = sceKernelClose(fd);
    if (second == 0) {
        return obs_fail("closing an already-closed descriptor succeeded");
    }
    return obs_pass();
}

static obs_result check_clock_never_goes_backwards(void) {
    OBS_REQUIRE(&sceKernelUsleep);

    /* Monotonicity. Sampled across real work rather than in a tight loop, so a coarse
     * clock is not failed for being coarse: equal readings are fine, decreasing ones are
     * not.
     *
     * Nothing here says how fast the clock should run or what it counts - 120-measure
     * asks that. This asks only that it never moves backwards, which is true of every
     * clock whatever it measures. */
    uint64_t previous = sceKernelGetProcessTime();
    for (unsigned int i = 0; i < 8u; i++) {
        (void)sceKernelUsleep(200u);
        uint64_t now = sceKernelGetProcessTime();
        if (now < previous) {
            return obs_fail_code("the clock went backwards", previous - now);
        }
        previous = now;
    }
    return obs_pass_value(previous);
}

static obs_result check_mutex_handles_distinct(void) {
    OBS_REQUIRE(&scePthreadMutexDestroy);

    /* The same relation as the event-flag check, on a different subsystem, because a
     * handle table is usually per-type and one being right says nothing about another. */
    enum { COUNT = 6 };
    ScePthreadMutex mutexes[COUNT] = {0};
    uint64_t handles[COUNT] = {0};
    unsigned int made = 0;
    for (unsigned int i = 0; i < COUNT; i++) {
        if (scePthreadMutexInit(&mutexes[i], NULL, "obscene-distinct") != 0) {
            break;
        }
        handles[made] = (uint64_t)(uintptr_t)mutexes[i];
        made++;
    }
    for (unsigned int i = 0; i < made; i++) {
        (void)scePthreadMutexDestroy(&mutexes[i]);
    }
    if (made < 2) {
        return obs_skip("fewer than two mutexes could be created to compare");
    }
    if (!all_distinct(handles, made)) {
        return obs_fail_code("two live mutexes share a handle", (uint64_t)made);
    }
    return obs_pass_value((uint64_t)made);
}

static obs_result check_mutex_is_not_recursive_by_default(void) {
    OBS_REQUIRE(&scePthreadMutexInit, &scePthreadMutexDestroy, &scePthreadMutexUnlock);

    /* A default mutex, held, must refuse a second lock from the same thread.
     *
     * The try form throughout, so a mutex that *is* recursive is reported rather than
     * deadlocking the run - which is the failure this relation would otherwise cause and
     * is exactly what CLAUDE.md forbids.
     *
     * Marked assumed rather than spec: POSIX makes the default type implementation-
     * defined, so "not recursive" is a strong expectation and still this project's. What
     * the check really establishes is that the lock *tracks state at all* - an
     * implementation returning success to every Trylock fails it. */
    ScePthreadMutex mutex = 0;
    if (scePthreadMutexInit(&mutex, NULL, "obscene-recursive") != 0) {
        return obs_skip("no mutex could be created");
    }
    int first = scePthreadMutexTrylock(&mutex);
    if (first != 0) {
        (void)scePthreadMutexDestroy(&mutex);
        return obs_fail("a fresh mutex could not be locked");
    }
    int second = scePthreadMutexTrylock(&mutex);
    if (second == 0) {
        /* Held twice, so release twice before destroying. */
        (void)scePthreadMutexUnlock(&mutex);
    }
    (void)scePthreadMutexUnlock(&mutex);
    (void)scePthreadMutexDestroy(&mutex);
    if (second == 0) {
        return obs_fail("a held mutex accepted a second lock from the same thread");
    }
    return obs_pass();
}

/* ---- State must belong to the object it was put in -------------------------
 *
 * The three checks below are one idea applied to three subsystems, and it is the idea the
 * rest of this section does not cover.
 *
 * Every existing relation asks about *one* object: does a handle stay distinct, does a
 * count count, does a clock advance. An implementation backing every event flag with a
 * single global word passes all of them - the count counts, the bits set, the handles are
 * distinct because they are allocated separately from the state they name. It is only
 * visible by asking a **second** object whether it can see the first one's state.
 *
 * That is not a hypothetical shape. It is what the simplest possible stand-in looks like:
 * one static variable and a function that reads it, which is how a subsystem gets stubbed
 * when the API is needed before the implementation is.
 */

static obs_result check_event_flag_state_is_per_object(void) {
    OBS_REQUIRE(&sceKernelDeleteEventFlag, &sceKernelPollEventFlag, &sceKernelCreateEventFlag);

    SceKernelEventFlag first = 0;
    SceKernelEventFlag second = 0;
    if (sceKernelCreateEventFlag(&first, "obscene-owned-a",
                                 OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0, NULL) != 0) {
        return obs_skip("no event flag could be created");
    }
    if (sceKernelCreateEventFlag(&second, "obscene-owned-b",
                                 OBS_EVF_ATTR_FIFO | OBS_EVF_ATTR_SINGLE, 0, NULL) != 0) {
        (void)sceKernelDeleteEventFlag(first);
        return obs_skip("a second event flag could not be created to compare");
    }

    /* One bit, set on the first flag only. Not bit zero: a zero pattern is what several
     * of these calls return on failure, and a bit that reads the same as "nothing" makes
     * the two outcomes indistinguishable in the record. */
    const uint64_t bit = 0x2u;
    int set = sceKernelSetEventFlag(first, bit);

    /* The other flag is asked first, deliberately. Polling the one that was set could
     * consume or clear the bit on an implementation that does more than it is asked, and
     * the interesting question would then be asked of a flag whose state we had already
     * disturbed. */
    uint64_t elsewhere = 0;
    int seen_elsewhere =
        sceKernelPollEventFlag(second, bit, OBS_EVF_WAITMODE_AND, &elsewhere);
    uint64_t own = 0;
    int seen_own = sceKernelPollEventFlag(first, bit, OBS_EVF_WAITMODE_AND, &own);

    (void)sceKernelDeleteEventFlag(first);
    (void)sceKernelDeleteEventFlag(second);

    if (set != 0) {
        return obs_fail_code("a fresh event flag refused a set", (uint64_t)(uint32_t)set);
    }
    if (seen_own != 0) {
        /* Ordered before the relation because it is the more basic failure: a flag that
         * cannot report its own bit makes the comparison meaningless rather than false. */
        return obs_fail_code("an event flag did not report the bit just set on it",
                             (uint64_t)(uint32_t)seen_own);
    }
    if (seen_elsewhere == 0) {
        return obs_fail_code("a bit set on one event flag was visible on another",
                             elsewhere);
    }
    return obs_pass();
}


/* Does an out-parameter stay inside the space it was given?
 *
 * # How this was found, which is the argument for it existing
 *
 * `semaphore-state-is-per-object` creates two semaphores into two adjacent `int`s and then
 * signals the first. On fpPS4 that failed with "a fresh semaphore refused a signal" and the
 * code `0x80020016` - EINVAL - which sent a reader looking at the *signal*. The signal was
 * fine. `sem_enter` returns EINVAL for a null handle, and the handle was null because
 * **creating the second semaphore had overwritten the first**.
 *
 * `sceKernelCreateSema` takes an `int *`. fpPS4's `SceKernelSema` is a pointer to a struct, so
 * it writes eight bytes through it; shadPS4's is a `u32` slot index and PS5PCEM's parameter is
 * `?*u32`, both four. Two implementations against one, and the declaration here follows the
 * two - which makes the third an overrun rather than a disagreement about arity.
 *
 * # Why it needs its own check
 *
 * The relation check found it by accident and described it wrongly, because a corrupted handle
 * fails at whatever touches it next. Asking the question directly turns "something later went
 * wrong" into "the call wrote past its argument", and that is the difference between a bug
 * report an implementer can act on and one they cannot reproduce.
 *
 * # The shape
 *
 * A guard word immediately after the handle, checked for disturbance. This is `130-layout`'s
 * trick applied to a four-byte destination: the buffer there is oversized and the guard proves
 * nothing ran past it. Same idea, smaller subject.
 *
 * Reported rather than judged where it is ambiguous. A platform whose handle is genuinely
 * pointer-sized is not committing an error by writing eight bytes - it is disagreeing with
 * this program's declaration, and the record says which happened rather than assuming. What is
 * never acceptable is the silent half: a caller who allocated four bytes lost whatever sat in
 * the next four, and nothing told them. (D171)
 */
static obs_result check_handle_fits_its_out_parameter(void) {
    OBS_REQUIRE(&sceKernelCreateSema, &sceKernelDeleteSema);

    /* Guard immediately after the handle, in one struct so the adjacency is the compiler's
     * contract rather than a hope about stack layout - which is exactly what made the
     * original failure depend on which order two locals happened to be placed in. */
    struct {
        int handle;
        uint32_t guard;
    } slot;
    slot.handle = 0;
    slot.guard = 0xA5A5A5A5u;

    int rc = sceKernelCreateSema(&slot.handle, "obscene-width", 0, 0, 4, NULL);
    uint32_t after = slot.guard;
    if (rc == 0 && slot.handle != 0) {
        (void)sceKernelDeleteSema(slot.handle);
    }
    if (rc != 0) {
        return obs_skip("no semaphore could be created to measure");
    }

    obs_report_measure("018-relational/handle-fits-its-out-parameter", "sceKernelCreateSema",
                       "guard-after-handle", (uint64_t)after, "word");
    if (after != 0xA5A5A5A5u) {
        /* The four bytes past the handle were written. The value is what landed there, which
         * for a pointer-sized handle is its upper half and says so at a glance. */
        return obs_fail_code("the call wrote past the end of the int it was given",
                             (uint64_t)after);
    }
    return obs_pass();
}

static obs_result check_semaphore_state_is_per_object(void) {
    OBS_REQUIRE(&sceKernelCreateSema, &sceKernelDeleteSema, &sceKernelSignalSema,
                &sceKernelPollSema);

    /* Guarded slots, and the guard is checked before anything is concluded.
     *
     * Two bare adjacent `int`s here is what found the fpPS4 out-parameter overrun (D171), and
     * it found it *badly*: creating the second handle overwrote the first, the first then
     * signalled as null, and this check reported "a fresh semaphore refused a signal" - which
     * is about the signal, which was fine. A corrupted handle fails at whatever touches it
     * next, and that is never where the fault is.
     *
     * So the relation is only asked once the handles are known intact. Where they are not,
     * this **skips and names the check that measures it**, because on such a platform the
     * question cannot be asked: a handle wider than its slot is unusable through that slot,
     * and failing the relation would blame per-object state for a width problem. */
    struct {
        int handle;
        uint32_t guard;
    } a, b;
    a.handle = 0;
    a.guard = 0xA5A5A5A5u;
    b.handle = 0;
    b.guard = 0xA5A5A5A5u;

    if (sceKernelCreateSema(&a.handle, "obscene-owned-a", 0, 0, 4, NULL) != 0) {
        return obs_skip("no semaphore could be created");
    }
    if (sceKernelCreateSema(&b.handle, "obscene-owned-b", 0, 0, 4, NULL) != 0) {
        (void)sceKernelDeleteSema(a.handle);
        return obs_skip("a second semaphore could not be created to compare");
    }
    if (a.guard != 0xA5A5A5A5u || b.guard != 0xA5A5A5A5u) {
        (void)sceKernelDeleteSema(a.handle);
        (void)sceKernelDeleteSema(b.handle);
        return obs_skip("the handle does not fit its out-parameter on this platform, so it "
                        "cannot be used through one; see 018-relational/"
                        "handle-fits-its-out-parameter");
    }
    int first = a.handle;
    int second = b.handle;

    int signalled = sceKernelSignalSema(first, 1);
    /* The untouched one first, for the same reason as above: a successful poll consumes a
     * count, so asking the signalled one first would change what the second question is
     * about. */
    int took_from_other = sceKernelPollSema(second, 1);
    int took_from_own = sceKernelPollSema(first, 1);

    (void)sceKernelDeleteSema(first);
    (void)sceKernelDeleteSema(second);

    if (signalled != 0) {
        return obs_fail_code("a fresh semaphore refused a signal",
                             (uint64_t)(uint32_t)signalled);
    }
    if (took_from_own != 0) {
        return obs_fail_code("a semaphore would not yield the count just signalled to it",
                             (uint64_t)(uint32_t)took_from_own);
    }
    if (took_from_other == 0) {
        return obs_fail("a count signalled to one semaphore was taken from another");
    }
    return obs_pass();
}

static obs_result check_mutex_state_is_per_object(void) {
    OBS_REQUIRE(&scePthreadMutexInit, &scePthreadMutexDestroy, &scePthreadMutexUnlock);

    ScePthreadMutex first = 0;
    ScePthreadMutex second = 0;
    if (scePthreadMutexInit(&first, NULL, "obscene-owned-a") != 0) {
        return obs_skip("no mutex could be created");
    }
    if (scePthreadMutexInit(&second, NULL, "obscene-owned-b") != 0) {
        (void)scePthreadMutexDestroy(&first);
        return obs_skip("a second mutex could not be created to compare");
    }

    /* Trylock throughout. The failing implementation this is aimed at - one lock behind
     * two handles - would make a blocking lock on the second mutex wait forever, taking
     * every check behind this one with it. The whole point is to survive finding it. */
    int held_first = scePthreadMutexTrylock(&first);
    int held_second = scePthreadMutexTrylock(&second);

    if (held_second == 0) {
        (void)scePthreadMutexUnlock(&second);
    }
    if (held_first == 0) {
        (void)scePthreadMutexUnlock(&first);
    }
    (void)scePthreadMutexDestroy(&first);
    (void)scePthreadMutexDestroy(&second);

    if (held_first != 0) {
        return obs_fail_code("a fresh mutex refused a trylock",
                             (uint64_t)(uint32_t)held_first);
    }
    if (held_second != 0) {
        return obs_fail_code("holding one mutex made a different mutex unavailable",
                             (uint64_t)(uint32_t)held_second);
    }
    return obs_pass();
}

/* ---- Relations that need a second thread -----------------------------------
 *
 * # Why these do not join
 *
 * `scePthreadJoin` blocks, and this section runs at 018 while `030-thread` is where join is
 * established. A join here on a platform whose threads do not finish would hang, and take
 * every check behind it - including the ones in `030-thread` that would have diagnosed it.
 * The section's own rule is that anything which can block is written as the `try` form or
 * not at all, and there is no `tryjoin`.
 *
 * So the child sets a flag last and the parent spins a bounded number of times waiting for
 * it. The parent gives up rather than waiting, and the give-up is reported as its own
 * outcome. The ordering matters: the child touches nothing shared after setting the flag,
 * so a parent that has seen it set may tear down what the child was using. (D159)
 */

/* Written by the child, read by the parent. Volatile because the compiler cannot see the
 * other thread's write and is entitled to fold the read to its initial value. */
static volatile uintptr_t obs_child_identity;
static volatile int obs_child_trylock_rc;
static volatile int obs_child_done;
static ScePthreadMutex obs_contended_mutex;

/* Roughly a second of spinning on any machine this runs on, and it does not matter if it
 * is ten. It bounds a wait whose alternative is not waiting at all. */
enum { OBS_CHILD_SPIN = 40000000 };

/* Wait for the child to say it finished, without blocking on it. Returns whether it did. */
static int obs_await_child(void) {
    for (unsigned int i = 0; i < (unsigned int)OBS_CHILD_SPIN; i++) {
        if (obs_child_done) {
            return 1;
        }
    }
    return 0;
}

static void *obs_identity_child(void *arg) {
    obs_child_identity = (uintptr_t)scePthreadSelf();
    obs_child_done = 1;
    return arg;
}

static obs_result check_thread_identities_differ(void) {
    OBS_REQUIRE(&scePthreadSelf, &scePthreadCreate);

    /* `030-thread/self` asks whether a thread has an identity and `thread-identity-stable`
     * above asks whether it keeps it. Both are satisfied by a function that returns the
     * same constant to everyone - which is a plausible stub, and which makes every mutex
     * and condition variable on the platform silently wrong, since they are all keyed on
     * exactly this value. Two threads is the only way to ask. */
    uintptr_t parent = (uintptr_t)scePthreadSelf();
    obs_child_identity = 0;
    obs_child_done = 0;

    ScePthread child = 0;
    int rc = scePthreadCreate(&child, NULL, obs_identity_child, NULL, "obscene-identity");
    if (rc != 0) {
        return obs_skip("no second thread could be created to compare");
    }
    if (!obs_await_child()) {
        return obs_partial("the second thread did not finish within the budget");
    }

    uintptr_t other = obs_child_identity;
    if (other == 0) {
        return obs_fail("a running thread reported no identity");
    }
    if (other == parent) {
        return obs_fail_code("two live threads reported the same identity",
                             (uint64_t)other);
    }
    return obs_pass_value((uint64_t)other);
}

static void *obs_exclusion_child(void *arg) {
    obs_child_trylock_rc = scePthreadMutexTrylock(&obs_contended_mutex);
    /* If it was handed the lock, give it straight back. Unlocking from a thread that is
     * not the owner is not defined behaviour anywhere - but we are already inside the
     * failure this exists to find, and leaving it held would make the parent's destroy the
     * thing that hangs. Recovering matters more than being tidy about a broken lock. */
    if (obs_child_trylock_rc == 0) {
        (void)scePthreadMutexUnlock(&obs_contended_mutex);
    }
    obs_child_done = 1;
    return arg;
}

static obs_result check_mutex_excludes_another_thread(void) {
    OBS_REQUIRE(&scePthreadMutexInit, &scePthreadMutexDestroy, &scePthreadMutexUnlock,
                &scePthreadCreate);

    /* Mutual exclusion, which is the entire purpose of a mutex and which nothing else in
     * this suite tests. `015-sync/mutex` locks and unlocks on one thread; `mutex-recursion`
     * asks what a second lock from the *same* thread does. Neither can tell a real lock
     * from a function that returns zero, because on one thread both look identical. */
    obs_contended_mutex = 0;
    if (scePthreadMutexInit(&obs_contended_mutex, NULL, "obscene-contended") != 0) {
        return obs_skip("no mutex could be created");
    }
    int held = scePthreadMutexTrylock(&obs_contended_mutex);
    if (held != 0) {
        (void)scePthreadMutexDestroy(&obs_contended_mutex);
        return obs_fail_code("a fresh mutex refused a trylock", (uint64_t)(uint32_t)held);
    }

    obs_child_trylock_rc = 0;
    obs_child_done = 0;
    ScePthread child = 0;
    int rc = scePthreadCreate(&child, NULL, obs_exclusion_child, NULL, "obscene-contend");
    if (rc != 0) {
        (void)scePthreadMutexUnlock(&obs_contended_mutex);
        (void)scePthreadMutexDestroy(&obs_contended_mutex);
        return obs_skip("no second thread could be created to contend");
    }
    int finished = obs_await_child();
    int child_rc = obs_child_trylock_rc;

    if (!finished) {
        /* Deliberately not unlocking or destroying: the child may still be inside
         * `trylock` on this mutex, and tearing it down underneath would turn an
         * inconclusive result into a crash somewhere unrelated. Leaking one mutex is the
         * cheaper outcome. */
        return obs_partial("the contending thread did not finish within the budget");
    }
    (void)scePthreadMutexUnlock(&obs_contended_mutex);
    (void)scePthreadMutexDestroy(&obs_contended_mutex);

    if (child_rc == 0) {
        return obs_fail("a mutex held by one thread was acquired by another");
    }
    return obs_pass_value((uint64_t)(uint32_t)child_rc);
}

/* ---- Two more, on memory and on files -------------------------------------- */

static obs_result check_allocations_do_not_overlap(void) {
    OBS_REQUIRE(&sceKernelReleaseDirectMemory, &sceKernelGetDirectMemorySize);

    /* Two allocations held at once must not name the same physical memory.
     *
     * `direct-memory-round-trip` above releases before allocating again, so it says nothing
     * about this: an allocator that ignores its own bookkeeping and returns the same offset
     * every time passes that check and every value check in `020-memory`. What it produces
     * is two callers writing over each other, and neither of them did anything wrong. */
    const size_t size = 0x4000;
    const sce_off_t end = (sce_off_t)sceKernelGetDirectMemorySize();
    sce_off_t first = 0;
    if (sceKernelAllocateDirectMemory(0, end, size, 0x4000, OBS_MEM_TYPE_WB_ONION,
                                      &first) != 0) {
        return obs_skip("no direct memory could be allocated");
    }
    sce_off_t second = 0;
    if (sceKernelAllocateDirectMemory(0, end, size, 0x4000, OBS_MEM_TYPE_WB_ONION,
                                      &second) != 0) {
        (void)sceKernelReleaseDirectMemory(first, size);
        /* Not a failure. A platform with very little direct memory left is entitled to
         * refuse the second, and calling that a bug would be inventing a policy. */
        return obs_skip("a second allocation could not be held at the same time");
    }

    obs_report_measure("018-relational/allocations-do-not-overlap",
                       "sceKernelAllocateDirectMemory", "first", (uint64_t)first, "offset");
    obs_report_measure("018-relational/allocations-do-not-overlap",
                       "sceKernelAllocateDirectMemory", "second", (uint64_t)second,
                       "offset");

    (void)sceKernelReleaseDirectMemory(first, size);
    (void)sceKernelReleaseDirectMemory(second, size);

    /* Disjoint means neither range starts inside the other. Written as two containment
     * tests rather than by ordering the offsets, because that needs no assumption about
     * which allocation comes first in memory. */
    uint64_t a = (uint64_t)first;
    uint64_t b = (uint64_t)second;
    uint64_t span = (uint64_t)size;
    if (a == b) {
        return obs_fail_code("two live allocations were given the same offset", a);
    }
    int overlaps = (a < b && b - a < span) || (b < a && a - b < span);
    if (overlaps) {
        return obs_fail_code("two live allocations overlap", a < b ? b - a : a - b);
    }
    return obs_pass_value(a < b ? b - a : a - b);
}

static obs_result check_file_position_tracks_reads(void) {
    OBS_REQUIRE(&sceKernelOpen, &sceKernelClose, &sceKernelLseek, &sceKernelRead);

    /* Three relations on one descriptor, none of which needs to know the file's contents.
     *
     * A fresh descriptor is at zero; reading N bytes advances the position by exactly the
     * number of bytes the call said it read; and seeking back to that position and reading
     * again yields the same bytes. Together they are the difference between a `read` that
     * works and one that returns a plausible count having done nothing - which every
     * negative check in `040-file` accepts, because they only ever ask it to fail.
     *
     * The file is this program's own module, which is present by definition on anything
     * that is running it. Nothing is assumed about what is in it. */
    int fd = sceKernelOpen("/app0/eboot.bin", OBS_O_RDONLY, 0);
    if (fd < 0) {
        return obs_skip("nothing could be opened to read");
    }
    sce_off_t start = sceKernelLseek(fd, 0, OBS_SEEK_CUR);
    if (start != 0) {
        (void)sceKernelClose(fd);
        return obs_fail_code("a freshly opened file was not positioned at zero",
                             (uint64_t)start);
    }

    unsigned char first[32];
    unsigned char again[32];
    for (unsigned int i = 0; i < sizeof first; i++) {
        first[i] = 0xA5u;
        again[i] = 0x5Au;
    }
    sce_ssize_t read_first = sceKernelRead(fd, first, sizeof first);
    if (read_first <= 0) {
        (void)sceKernelClose(fd);
        return obs_fail_code("a readable file yielded no bytes", (uint64_t)read_first);
    }
    sce_off_t after = sceKernelLseek(fd, 0, OBS_SEEK_CUR);
    if (after != (sce_off_t)read_first) {
        (void)sceKernelClose(fd);
        obs_report_measure("018-relational/file-position-tracks-reads", "sceKernelLseek",
                           "position-after-read", (uint64_t)after, "bytes");
        return obs_fail_code("the position did not advance by the number of bytes read",
                             (uint64_t)read_first);
    }

    sce_off_t rewound = sceKernelLseek(fd, 0, OBS_SEEK_SET);
    if (rewound != 0) {
        (void)sceKernelClose(fd);
        return obs_fail_code("a seek to the start did not report the start",
                             (uint64_t)rewound);
    }
    sce_ssize_t read_again = sceKernelRead(fd, again, (size_t)read_first);
    (void)sceKernelClose(fd);

    if (read_again != read_first) {
        return obs_fail_code("re-reading the same offset returned a different count",
                             (uint64_t)read_again);
    }
    for (sce_ssize_t i = 0; i < read_first; i++) {
        if (first[i] != again[i]) {
            /* The value is the offset that differed, which is what a reader needs to tell
             * "the second read did nothing" from "the second read got something else". */
            return obs_fail_code("re-reading the same offset returned different bytes",
                                 (uint64_t)i);
        }
    }
    return obs_pass_value((uint64_t)read_first);
}

static const obs_check relational_checks[] = {
    /* Spec, not assumed, and the distinction is the point of the section: no document
     * states these particular returns, but "two live objects do not share one handle"
     * and "a counting semaphore counts" are not this project's opinions. */
    {"018-relational/event-flag-handles-distinct", "libkernel", "sceKernelCreateEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelCreateEventFlag,
     check_event_flag_handles_distinct, OBS_FROM_SPEC},
    {"018-relational/event-flag-handles-reusable", "libkernel",
     "sceKernelCreateEventFlag", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelCreateEventFlag, check_event_flag_handles_reusable,
     OBS_FROM_SPEC},
    {"018-relational/direct-memory-round-trip", "libkernel",
     "sceKernelAllocateDirectMemory", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelAllocateDirectMemory, check_direct_memory_round_trip,
     OBS_FROM_SPEC},
    {"018-relational/semaphore-counts", "libkernel", "sceKernelCreateSema", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelCreateSema, check_semaphore_count_is_tracked,
     OBS_FROM_SPEC},
    {"018-relational/thread-identity-stable", "libkernel", "scePthreadSelf",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadSelf,
     check_thread_identity_is_stable, OBS_FROM_SPEC},
    /* `OBS_CAP_NONE`, and the two file checks here used to say `OBS_CAP_FILE`.
     *
     * Capabilities are granted in running order, this section runs at 018, and the check
     * that grants `OBS_CAP_FILE` is in `040-file`. So both of these were gated on something
     * that could not exist yet, and **neither has ever run** - on any target, host included.
     * Each reported `skip: a prerequisite capability was not established`, which reads as a
     * platform limitation rather than a build-order mistake, and that is why it survived
     * review in every report it appeared in.
     *
     * The requirement was redundant as well as fatal: each body opens the file itself and
     * skips with a better message when it cannot. `obscene-tool caps` gates the ordering
     * now, so it cannot come back silently. (D158) */
    {"018-relational/descriptors-distinct", "libkernel", "sceKernelOpen", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelOpen, check_descriptors_distinct,
     OBS_FROM_SPEC},
    {"018-relational/close-is-not-idempotent", "libkernel", "sceKernelClose",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelClose,
     check_close_is_not_idempotent, OBS_FROM_SPEC},
    {"018-relational/clock-never-goes-backwards", "libkernel",
     "sceKernelGetProcessTime", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelGetProcessTime, check_clock_never_goes_backwards,
     OBS_FROM_SPEC},
    {"018-relational/mutex-handles-distinct", "libkernel", "scePthreadMutexInit",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexInit,
     check_mutex_handles_distinct, OBS_FROM_SPEC},
    {"018-relational/mutex-not-recursive", "libkernel", "scePthreadMutexTrylock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexTrylock,
     check_mutex_is_not_recursive_by_default, OBS_FROM_ASSUMED},
    /* Per-object state. `spec` for the same reason as the rest: no document states these
     * returns, and "one object's state is not another's" is not this project's opinion. */
    {"018-relational/event-flag-state-is-per-object", "libkernel", "sceKernelSetEventFlag",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelSetEventFlag,
     check_event_flag_state_is_per_object, OBS_FROM_SPEC},
    {"018-relational/semaphore-state-is-per-object", "libkernel", "sceKernelSignalSema",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelSignalSema,
     check_semaphore_state_is_per_object, OBS_FROM_SPEC},
    /* `IMPLEMENTATIONS`: shadPS4's handle is a `u32` slot index and PS5PCEM's out-parameter
     * is `?*u32`; fpPS4's is a pointer. Two against one, and the declaration follows the two,
     * which is what makes the third an overrun. (D171) */
    {"018-relational/handle-fits-its-out-parameter", "libkernel", "sceKernelCreateSema",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelCreateSema,
     check_handle_fits_its_out_parameter, OBS_FROM_IMPLEMENTATIONS},
    {"018-relational/mutex-state-is-per-object", "libkernel", "scePthreadMutexTrylock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexTrylock,
     check_mutex_state_is_per_object, OBS_FROM_SPEC},
    /* `OBS_CAP_NONE` on both of the threading relations, and not because they need no
     * threads. `OBS_CAP_THREAD` is granted in `030-thread`, which runs after this section -
     * requiring it here is the exact mistake `obscene-tool caps` now gates (D158). Each
     * body creates its own thread and skips when it cannot, which is a better message
     * anyway. */
    {"018-relational/thread-identities-differ", "libkernel", "scePthreadCreate",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadCreate,
     check_thread_identities_differ, OBS_FROM_SPEC},
    {"018-relational/mutex-excludes-another-thread", "libkernel", "scePthreadMutexTrylock",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadMutexTrylock,
     check_mutex_excludes_another_thread, OBS_FROM_SPEC},
    {"018-relational/allocations-do-not-overlap", "libkernel",
     "sceKernelAllocateDirectMemory", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelAllocateDirectMemory, check_allocations_do_not_overlap,
     OBS_FROM_SPEC},
    {"018-relational/file-position-tracks-reads", "libkernel", "sceKernelRead",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelRead,
     check_file_position_tracks_reads, OBS_FROM_SPEC},
};

const obs_section obs_section_relational = {
    "018-relational",
    "Properties, not values",
    "Results compared to each other rather than to an expected value, so no authority is "
    "needed to check them. Aimed at the vendor surface, where no document supplies one.",
    relational_checks,
    OBS_COUNT(relational_checks),
};
