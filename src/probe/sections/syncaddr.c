/*
 * Waiting on a word, and waking it: the platform's futex.
 *
 * # Why this matters more than its size suggests
 *
 * `libkernel_sync_on_address` has two exports and they were censused for presence only,
 * because the census loads a library by name and this one is a library *inside* the
 * kernel module rather than a module of its own - so it reported `this library could
 * not be loaded` and nothing has ever called either function.
 *
 * Meanwhile they are the busiest pair on the platform. Across every guest run the
 * sibling emulator has recorded, **78% of all calls went to the wait** - one function,
 * in one title, through a Unity shim that re-exports it (its D566). Answered with a
 * placeholder it spun 2.27 million times in twelve seconds; modelled on FreeBSD's
 * `_umtx_op(2)` it drops to 26 calls (its D573). That model rests on assumptions no run
 * can settle, and they are what this section measures:
 *
 *   * does a word that already differs return at once, and with what code;
 *   * does a wake actually release a blocked waiter, and what does it answer;
 *   * **is the comparison 32-bit or 64-bit** - the emulator assumed 64 from an export
 *     layout in which the wait shares an entry point with the `Wait64` spelling;
 *   * what does a wake with nobody waiting answer.
 *
 * # The futex is resolved by name, not linked - and that is what keeps the title alive
 *
 * `sceKernelSyncOnAddressWait`/`Wake` are exported by a library *namespace* inside
 * `libkernel.sprx` (`libkernel_sync_on_address`), not by a loadable module of that name
 * - the hardware dumps carry them in `libkernel`, and no
 * `libkernel_sync_on_address.sprx` exists. Declaring them as linked imports made the
 * title module declare a `needed_module` for a `.sprx` the loader cannot find, so the
 * title died before its first record while the payload - which resolves by address and
 * has no dependency table - ran fine. So they are resolved at run time through
 * `libkernel`, the way `017-posix` and `019-posixerr` resolve their names, and nothing
 * is added to the module's dependency table. (D321-adjacent; the linked-import approach
 * was D322, reverted here.)
 *
 * # Every wait happens on a thread nobody joins, and that is the whole safety argument
 *
 * A futex wait blocks. There is no `try` form of it, so the rule that a blocking call
 * is written as the `try` form or not at all cannot be satisfied the usual way - and a
 * probe that hangs loses every check behind it, which this suite has paid for twice.
 *
 * So the main thread never calls the wait. A worker runs a fixed script of waits and
 * advances a state counter as it goes; the main thread only ever sleeps, reads that
 * counter, and calls the *wake*, which cannot block. If the platform never returns from
 * a wait, the worker stays there and the run does not - exactly the arrangement
 * `015-sync/condvar-wakes-a-waiter` already uses for the one other unbounded call in
 * this program, and for the same reason.
 *
 * That also makes the width question safe, which it otherwise would not be. The
 * discriminating case is a word whose high half differs and whose low half matches:
 * under a 64-bit comparison it returns at once, and under a 32-bit one it blocks.
 * **Both outcomes are recoverable** - the blocked one is released by a wake the main
 * thread issues anyway - so the measurement costs nothing whichever way it comes out.
 *
 * # The state counter, and why it is not a boolean
 *
 * It only ever increases, so "the worker got at least this far" is one comparison. A
 * boolean would merge two different faults: a wait that never returned and a worker
 * that died before reaching it are different findings with different causes.
 *
 * `volatile`, not atomic: this is freestanding and `stdatomic.h` is not guaranteed. The
 * sleeps between the writes and the reads involve the kernel, which is enough on this
 * architecture. Stated because it is a real limitation and not a rigorous one - the
 * same caveat `015-sync` carries for the same reason.
 *
 * # No timeout is probed, deliberately
 *
 * `_umtx_op` has a timeout slot and the wrapper may well carry one, but every observed
 * guest call leaves that register zero and nothing establishes its unit. Declaring a
 * third parameter to probe it would be declaring an arity nothing settles, which
 * principle 2 forbids. It is recorded in `docs/backlog/024` as what a later session can
 * settle.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* The futex pair, resolved by name at run time. The platform spells the wait as taking
 * an address and a value to compare, the wake as an address and a count. */
typedef int (*fn_sa_wait_t)(void *address, uint64_t value);
typedef int (*fn_sa_wake_t)(void *address, uint64_t count);

#if !defined(OBSCENE_HOST_BUILD)
static int s_sa_handle = -2;

/* The futex lives in `libkernel` - the export library `libkernel_sync_on_address` is a
 * namespace inside it, not a loadable module - so open `libkernel` and resolve the
 * names against it. */
static int obs_sa_handle(void) {
    if (s_sa_handle != -2) {
        return s_sa_handle;
    }
    s_sa_handle = obs_module_open("libkernel");
    return s_sa_handle;
}
#endif

static void *obs_sa_symbol(const char *name) {
#if defined(OBSCENE_HOST_BUILD)
    /* The host stubs, declared in platform.h's host block and defined in host_stubs.c.
     */
    if (obs_strcmp(name, "sceKernelSyncOnAddressWait") == 0)
        return (void *)&sceKernelSyncOnAddressWait;
    if (obs_strcmp(name, "sceKernelSyncOnAddressWake") == 0)
        return (void *)&sceKernelSyncOnAddressWake;
    return NULL;
#else
    int h = obs_sa_handle();
    if (h < 0) {
        return NULL;
    }
    return (void *)obs_module_symbol(h, name);
#endif
}

/* Resolved once and cached: the worker and three checks all reach for the same pair. */
static fn_sa_wait_t s_sa_wait;
static fn_sa_wake_t s_sa_wake;
static int s_sa_resolved;

static void obs_sa_resolve(void) {
    if (s_sa_resolved) {
        return;
    }
    s_sa_resolved = 1;
    s_sa_wait = (fn_sa_wait_t)obs_sa_symbol("sceKernelSyncOnAddressWait");
    s_sa_wake = (fn_sa_wake_t)obs_sa_symbol("sceKernelSyncOnAddressWake");
}

/* The word the first two waits are made against. Zero, and left zero: the mismatch case
 * passes a different value rather than changing the word, so the blocking case that
 * follows it does not depend on a write landing first. */
static uint64_t s_sa_gate;

/* The word the width question turns on: high half set, low half clear.
 *
 * Waiting on it for a value of zero matches in the low 32 bits and differs in all 64.
 * So a platform comparing 32 bits blocks and one comparing 64 returns at once, and the
 * two are told apart by which happens. This is little-endian reasoning and the target
 * is x86-64: the low half is the one a 32-bit read at this address sees. */
static uint64_t s_sa_width_word = 0x0000000100000000ull;
#define OBS_SA_WIDTH_EXPECT 0x0000000000000000ull

/* An address nobody ever waits on, for the wake that must find no one. */
static uint64_t s_sa_lonely;

/* How far the worker got. Only ever increases. */
#define OBS_SA_START 0u
#define OBS_SA_ABSENT 1u
#define OBS_SA_MISMATCH_ENTER 2u
#define OBS_SA_MISMATCH_RETURNED 3u
#define OBS_SA_BLOCKED_ENTER 4u
#define OBS_SA_RELEASED 5u
#define OBS_SA_WIDTH_ENTER 6u
#define OBS_SA_WIDTH_RETURNED 7u

static volatile unsigned int s_sa_state = OBS_SA_START;
static volatile int s_sa_mismatch_rc;
static volatile int s_sa_blocked_rc;
static volatile int s_sa_width_rc;
static int s_sa_worker_started;
/* The fault signal a wait raised on the worker, or zero. The waits have faulted inside
 * libkernel on hardware; the worker catches that (D325) and records it here so the main
 * thread reports a crash rather than the run ending from a thread it cannot join. */
static volatile int s_sa_faulted;

/* Long enough for the worker to reach its next wait on any plausible scheduler.
 * Generous on purpose: a wake that arrives before anybody is listening looks exactly
 * like a wake that does not work, and the whole section would misreport on a short
 * sleep. */
#define OBS_SA_SETTLE_US 50000u

/* The script. Three waits, in order, each announced by the counter before it is made so
 * a wait that never returns is visible as the state it stopped in. */
static void *obs_sa_worker(void *arg) {
    (void)arg;
    /* The worker reaches for the resolved wait, not for one the harness guarded - so it
     * is checked again here. Calling through a null pointer would end the run to
     * establish something the pointer already said. */
    if (s_sa_wait == NULL) {
        s_sa_state = OBS_SA_ABSENT;
        return NULL;
    }

    /* The wait has faulted inside libkernel on hardware. Arm the fault guard on this
     * worker so a fault lands back here as a recorded crash (s_sa_faulted, read by the
     * main thread) rather than a SIGSEGV that ends a run nobody can join the worker to
     * rescue. One pad covers all three waits; the state counter already names which one
     * was in flight. (D325)
     */
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        s_sa_faulted = sig;
        return NULL;
    }

    /* 1. A word that already holds something else. A futex must not wait on this, and
     * the code it answers is the measurement. */
    s_sa_state = OBS_SA_MISMATCH_ENTER;
    s_sa_mismatch_rc = s_sa_wait(&s_sa_gate, s_sa_gate + 1ull);
    s_sa_state = OBS_SA_MISMATCH_RETURNED;

    /* 2. The same word, matching. This blocks, and the main thread's wake is what ends
     * it. */
    s_sa_state = OBS_SA_BLOCKED_ENTER;
    s_sa_blocked_rc = s_sa_wait(&s_sa_gate, s_sa_gate);
    s_sa_state = OBS_SA_RELEASED;

    /* 3. The width question: matches in 32 bits, differs in 64. Returning means the
     *    comparison read all 64; blocking means it read the low half, and the main
     * thread releases it either way. */
    s_sa_state = OBS_SA_WIDTH_ENTER;
    s_sa_width_rc = s_sa_wait(&s_sa_width_word, OBS_SA_WIDTH_EXPECT);
    s_sa_state = OBS_SA_WIDTH_RETURNED;
    obs_fault_unregister();
    return NULL;
}

/* What a wake answers when nothing is waiting.
 *
 * On the main thread, because a wake cannot block. It is also the question the
 * emulator's model turns on in the other direction: it deliberately does not remember
 * such a wake, on the reasoning that the word carries the state and the wake only ends
 * a sleep. What the platform *returns* here is the part a run can see, and whether it
 * is a status or a count of threads woken is exactly what the value distinguishes. */
static obs_result check_wake_with_no_waiter(void) {
    obs_sa_resolve();
    if (s_sa_wake == NULL) {
        return obs_skip("the wake was not resolved for this build");
    }
    int rc = s_sa_wake(&s_sa_lonely, 1ull);
    obs_report_error_code("libkernel_sync_on_address", "sceKernelSyncOnAddressWake",
                          "nobody waiting", (uint64_t)(uint32_t)rc);
    obs_report_measure("032-syncaddr/wake-with-no-waiter", "sceKernelSyncOnAddressWake",
                       "returned", (uint64_t)(int64_t)rc, "code");
    /* Nothing is graded. A zero could be success or a count of none, and a non-zero
     * could be an error or a count - all legitimate, and the value is the finding. */
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* A word that already differs must not be waited on.
 *
 * This is the defining property of a compare-and-wait, and the one a caller depends on:
 * the wake may have come first, so a wait that slept anyway would be the lost-wakeup
 * hang. It is also the check that starts the worker. */
static obs_result check_wait_returns_on_mismatch(void) {
    OBS_REQUIRE(&scePthreadCreate, &sceKernelUsleep);

    obs_sa_resolve();
    if (s_sa_wait == NULL) {
        return obs_skip("the wait was not resolved for this build");
    }

    s_sa_state = OBS_SA_START;
    ScePthread worker = NULL;
    int rc = scePthreadCreate(&worker, NULL, obs_sa_worker, NULL, "obscene-syncaddr");
    if (rc != 0) {
        return obs_skip("no thread to wait on");
    }
    s_sa_worker_started = 1;
    (void)sceKernelUsleep(OBS_SA_SETTLE_US);

    unsigned int state = s_sa_state;
    if (state == OBS_SA_ABSENT) {
        return obs_skip("the wait was not resolved for this build");
    }
    if (s_sa_faulted != 0) {
        /* The wait faulted inside libkernel and the guard caught it on the worker. That
         * is the finding - the futex, called as its exports describe it, does not
         * survive hardware - reported as a crash rather than a fail so the counts keep
         * them apart. */
        return obs_crash(s_sa_faulted);
    }
    if (state < OBS_SA_MISMATCH_RETURNED) {
        /* It went in and did not come back. The worker is left where it is - joining it
         * is the hang this design exists to avoid - and every check below skips. */
        return obs_fail_code("a wait on a word that already differed did not return",
                             (uint64_t)state);
    }

    obs_report_error_code("libkernel_sync_on_address", "sceKernelSyncOnAddressWait",
                          "value already differs",
                          (uint64_t)(uint32_t)s_sa_mismatch_rc);
    obs_report_measure("032-syncaddr/wait-returns-on-mismatch",
                       "sceKernelSyncOnAddressWait", "returned",
                       (uint64_t)(int64_t)s_sa_mismatch_rc, "code");
    return obs_pass_value((uint64_t)(uint32_t)s_sa_mismatch_rc);
}

/* A wake releases a waiter that is blocked on the same word.
 *
 * The round trip, and the only check here that proves the pair works rather than
 * measuring an edge of it. A platform that returns success from both calls while the
 * waiter stays blocked is the exact shape of a stubbed synchronisation layer, and it is
 * silent. */
static obs_result check_wake_releases_a_waiter(void) {
    OBS_REQUIRE(&sceKernelUsleep);

    obs_sa_resolve();
    if (s_sa_wake == NULL) {
        return obs_skip("the wake was not resolved for this build");
    }
    if (!s_sa_worker_started) {
        return obs_skip("no worker reached a wait");
    }
    if (s_sa_state < OBS_SA_BLOCKED_ENTER) {
        return obs_skip("the worker never reached the blocking wait");
    }
    /* It has announced the wait but may not be parked in it yet. A wake that arrives
     * first is indistinguishable from one that does not work, so this is generous. */
    (void)sceKernelUsleep(OBS_SA_SETTLE_US);

    int rc = s_sa_wake(&s_sa_gate, 1ull);
    obs_report_error_code("libkernel_sync_on_address", "sceKernelSyncOnAddressWake",
                          "one waiter blocked", (uint64_t)(uint32_t)rc);
    obs_report_measure("032-syncaddr/wake-releases-a-waiter",
                       "sceKernelSyncOnAddressWake", "returned", (uint64_t)(int64_t)rc,
                       "code");
    (void)sceKernelUsleep(OBS_SA_SETTLE_US);

    if (s_sa_state < OBS_SA_RELEASED) {
        /* One more, asking for every waiter rather than one, before calling it a
         * failure: a platform whose count argument means something other than "how
         * many" would refuse the first and honour this, and that difference is worth
         * recording rather than losing inside a verdict. */
        int all_rc = s_sa_wake(&s_sa_gate, 0x7FFFFFFFull);
        obs_report_measure("032-syncaddr/wake-releases-a-waiter",
                           "sceKernelSyncOnAddressWake", "retry-all",
                           (uint64_t)(int64_t)all_rc, "code");
        (void)sceKernelUsleep(OBS_SA_SETTLE_US);
    }

    if (s_sa_state < OBS_SA_RELEASED) {
        return obs_fail_code("a blocked waiter was not released by a wake on its word",
                             (uint64_t)s_sa_state);
    }
    obs_report_measure("032-syncaddr/wake-releases-a-waiter",
                       "sceKernelSyncOnAddressWait", "wait-returned",
                       (uint64_t)(int64_t)s_sa_blocked_rc, "code");
    return obs_pass_value((uint64_t)(uint32_t)rc);
}

/* Whether the comparison reads 32 bits or 64.
 *
 * The worker is waiting on a word whose high half differs from the value it passed and
 * whose low half matches. Returning means all 64 bits were compared; still being in the
 * wait means only the low 32 were. Both are answers, and the second is released here.
 */
#define OBS_SA_COMPARE_64 64u
#define OBS_SA_COMPARE_32 32u

static obs_result check_compare_width(void) {
    /* The wake is required as well as the sleep: the 32-bit answer leaves the worker
     * parked, and a width this check cannot release is one it should not measure. */
    OBS_REQUIRE(&sceKernelUsleep);

    obs_sa_resolve();
    if (s_sa_wake == NULL) {
        return obs_skip("the wake was not resolved for this build");
    }
    if (!s_sa_worker_started || s_sa_state < OBS_SA_WIDTH_ENTER) {
        return obs_skip("the worker never reached the width wait");
    }
    (void)sceKernelUsleep(OBS_SA_SETTLE_US);

    unsigned int width =
        (s_sa_state >= OBS_SA_WIDTH_RETURNED) ? OBS_SA_COMPARE_64 : OBS_SA_COMPARE_32;
    obs_report_measure("032-syncaddr/compare-width", "sceKernelSyncOnAddressWait",
                       "compare-width", (uint64_t)width, "bits");

    if (width == OBS_SA_COMPARE_32) {
        /* It is parked on a word whose low half matched. Release it, so the worker
         * leaves rather than being abandoned for a measurement that has already been
         * made. */
        (void)s_sa_wake(&s_sa_width_word, 0x7FFFFFFFull);
        (void)sceKernelUsleep(OBS_SA_SETTLE_US);
        obs_report_measure("032-syncaddr/compare-width", "sceKernelSyncOnAddressWait",
                           "released-after", (uint64_t)s_sa_state, "state");
        return obs_pass_value((uint64_t)OBS_SA_COMPARE_32);
    }

    obs_report_measure("032-syncaddr/compare-width", "sceKernelSyncOnAddressWait",
                       "returned", (uint64_t)(int64_t)s_sa_width_rc, "code");
    return obs_pass_value((uint64_t)OBS_SA_COMPARE_64);
}

static const obs_check syncaddr_checks[] = {
    {"032-syncaddr/wake-with-no-waiter", "libkernel_sync_on_address",
     "sceKernelSyncOnAddressWake", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_wake_with_no_waiter, check_wake_with_no_waiter,
     OBS_FROM_ASSUMED},
    {"032-syncaddr/wait-returns-on-mismatch", "libkernel_sync_on_address",
     "sceKernelSyncOnAddressWait", OBS_CAP_THREAD, OBS_CAP_NONE,
     (const void *)check_wait_returns_on_mismatch, check_wait_returns_on_mismatch,
     OBS_FROM_DERIVED},
    {"032-syncaddr/wake-releases-a-waiter", "libkernel_sync_on_address",
     "sceKernelSyncOnAddressWake", OBS_CAP_THREAD, OBS_CAP_NONE,
     (const void *)check_wake_releases_a_waiter, check_wake_releases_a_waiter,
     OBS_FROM_DERIVED},
    {"032-syncaddr/compare-width", "libkernel_sync_on_address",
     "sceKernelSyncOnAddressWait", OBS_CAP_THREAD, OBS_CAP_NONE,
     (const void *)check_compare_width, check_compare_width, OBS_FROM_ASSUMED},
};

const obs_section obs_section_syncaddr = {
    "032-syncaddr",
    "Waiting on a word",
    "The platform's futex, never called before: whether a word that already differs is "
    "waited on, whether a wake releases a blocked waiter, and whether the comparison "
    "reads "
    "32 bits or 64. Every wait runs on a worker nobody joins, so none of it can hang "
    "the "
    "suite.",
    syncaddr_checks,
    OBS_COUNT(syncaddr_checks),
};
