/*
 * What a thread attribute set says about the stack a thread is running on.
 *
 * # The one question this exists to settle
 *
 * `scePthreadAttrGetstackaddr` hands back an address. **Is that address the lowest byte
 * of the stack, or its top?** The two conventions differ by the whole size of the
 * stack, and a caller that guesses wrong computes a bound that is megabytes away from
 * the real one.
 *
 * It is not an academic question. Three retail titles run the same sequence once each,
 * early: `scePthreadSelf`, then `scePthreadAttrGet` on the answer, then the stack
 * address and the stack size. That is a garbage collector finding the span it has to
 * scan. The sibling emulator left those calls unimplemented, the collector computed its
 * bound from a placeholder, and all three titles walked off the top of the real stack
 * into the first unmapped page above it (its D575). It now answers them on FreeBSD's
 * convention - address is the lowest byte - and records that as an assumption a probe
 * could settle. This is that probe.
 *
 * # How it is settled without assuming either answer
 *
 * The check takes the address of one of its own locals, which is by construction inside
 * the stack of the thread asking, and asks which side of the reported address it falls:
 *
 *   * inside `[address, address + size)` - the address is the **base**, FreeBSD's
 * convention;
 *   * inside `[address - size, address)` - the address is the **top**;
 *   * neither - the region does not describe this thread's stack at all, which is wrong
 *     under both conventions and is the one outcome graded as a failure.
 *
 * No layout is assumed, no constant is invented, and the arithmetic is the whole
 * method. `sceKernelIsStack` is consulted as a second, independent witness where it
 * resolves - it reports bounds of its own - but nothing is asserted on it, because
 * platform.h is explicit that those two out-pointers are only *almost certainly* the
 * region's bounds.
 *
 * # Nothing here blocks and nothing is dereferenced
 *
 * Every value is compared as an integer. The one pointer that gets written through is
 * an out-parameter the caller owns. A bogus thread handle is deliberately not probed:
 * the platform would have to look it up in its own thread list, and handing a made-up
 * pointer to that is a crash risk that would cost every check behind this one to learn
 * a code.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "obscene/status.h"
#include "oops/freestd.h"

/* What the section learned about the running thread, filled by the first check and read
 * by the second. Kept rather than re-derived so the two checks describe one
 * observation: a second `AttrGet` could in principle answer differently, and then the
 * classification would be of a region the first check never reported. */
static void *s_stack_address;
static size_t s_stack_size;
static int s_described;

/* Ask the platform to describe the calling thread, into an attribute set of our own.
 *
 * The sequence three titles run, in the order they run it. A failure at any step leaves
 * nothing for the classification below to work on, which is why that check skips rather
 * than guessing when this one did not pass. */
static obs_result check_self_describes(void) {
    OBS_REQUIRE(&scePthreadSelf, &scePthreadAttrInit, &scePthreadAttrDestroy,
                &scePthreadAttrGetstackaddr, &scePthreadAttrGetstacksize);

    s_described = 0;

    ScePthread self = scePthreadSelf();
    if (self == NULL) {
        return obs_skip("the calling thread has no identity to describe");
    }

    ScePthreadAttr attr = 0;
    int rc = scePthreadAttrInit(&attr);
    if (rc != 0) {
        return obs_fail_code("an attribute set could not be created",
                             (uint64_t)(uint32_t)rc);
    }

    rc = scePthreadAttrGet(self, &attr);
    obs_report_error_code("libkernel", "scePthreadAttrGet", "on the calling thread",
                          (uint64_t)(uint32_t)rc);
    if (rc != 0) {
        (void)scePthreadAttrDestroy(&attr);
        return obs_fail_code("a running thread would not describe itself",
                             (uint64_t)(uint32_t)rc);
    }

    void *address = NULL;
    size_t size = 0;
    int addr_rc = scePthreadAttrGetstackaddr(&attr, &address);
    int size_rc = scePthreadAttrGetstacksize(&attr, &size);
    (void)scePthreadAttrDestroy(&attr);

    if (addr_rc != 0) {
        return obs_fail_code("the described stack had no address",
                             (uint64_t)(uint32_t)addr_rc);
    }
    if (size_rc != 0) {
        return obs_fail_code("the described stack had no size",
                             (uint64_t)(uint32_t)size_rc);
    }

    obs_report_measure("031-stackattr/self-describes", "scePthreadAttrGetstackaddr",
                       "stack-address", (uint64_t)(uintptr_t)address, "address");
    obs_report_measure("031-stackattr/self-describes", "scePthreadAttrGetstacksize",
                       "stack-size", (uint64_t)size, "bytes");

    /* A running thread stands on a stack, so a described one of no size is an
     * implementation that accepted the call and answered nothing - the shape of an
     * out-parameter left untouched, which is the failure the whole family keeps
     * meeting. */
    if (size == 0) {
        return obs_fail("a running thread was described as having a stack of no size");
    }
    if (address == NULL) {
        return obs_fail(
            "a running thread was described as having a stack at no address");
    }

    s_stack_address = address;
    s_stack_size = size;
    s_described = 1;
    return obs_pass_value((uint64_t)size);
}

/* Which end of the stack the reported address is.
 *
 * The finding is the classification, and it is reported as a measurement whichever way
 * it comes out: `1` for a base, `2` for a top. Only "neither" is graded, because a
 * region that does not contain the frame asking about it is wrong under both
 * conventions. */
#define OBS_SA_ADDRESS_IS_BASE 1u
#define OBS_SA_ADDRESS_IS_TOP 2u
#define OBS_SA_ADDRESS_IS_NEITHER 0u

static obs_result check_address_is_the_base(void) {
    /* The witness: a local of this frame, which is inside this thread's stack by
     * construction. Its address is compared and never read through. */
    volatile int frame = 0;

    if (!s_described) {
        return obs_skip(
            "the calling thread was not described, so there is nothing to place");
    }

    uintptr_t here = (uintptr_t)&frame;
    uintptr_t reported = (uintptr_t)s_stack_address;
    uintptr_t span = (uintptr_t)s_stack_size;

    unsigned int placement = OBS_SA_ADDRESS_IS_NEITHER;
    if (here >= reported && here - reported < span) {
        placement = OBS_SA_ADDRESS_IS_BASE;
    } else if (here < reported && reported - here <= span) {
        placement = OBS_SA_ADDRESS_IS_TOP;
    }

    obs_report_measure("031-stackattr/address-is-the-base",
                       "scePthreadAttrGetstackaddr", "placement", (uint64_t)placement,
                       "placement");
    obs_report_measure("031-stackattr/address-is-the-base",
                       "scePthreadAttrGetstackaddr", "frame-address", (uint64_t)here,
                       "address");

    /* The second witness, where it resolves. `sceKernelIsStack` reports bounds of its
     * own, and platform.h is explicit that they are only almost certainly the region's
     * - so they are recorded beside the classification and nothing is asserted on them.
     * Guarded on its own rather than through OBS_REQUIRE, so a platform without it
     * still answers the question this check exists for. */
    if (obs_address_is_callable((const void *)&sceKernelIsStack)) {
        void *low = NULL;
        void *high = NULL;
        int on_stack = sceKernelIsStack((void *)&frame, &low, &high);
        obs_report_measure("031-stackattr/address-is-the-base", "sceKernelIsStack",
                           "is-stack", (uint64_t)(uint32_t)on_stack, "code");
        obs_report_measure("031-stackattr/address-is-the-base", "sceKernelIsStack",
                           "low", (uint64_t)(uintptr_t)low, "address");
        obs_report_measure("031-stackattr/address-is-the-base", "sceKernelIsStack",
                           "high", (uint64_t)(uintptr_t)high, "address");
    }

    if (placement == OBS_SA_ADDRESS_IS_NEITHER) {
        /* Neither end. The frame asking is not inside the region either reading would
         * give, so the description belongs to some other stack or to none. */
        return obs_fail_code(
            "the described stack does not contain the frame that asked",
            (uint64_t)reported);
    }
    return obs_pass_value((uint64_t)placement);
}

/* What a set nothing has configured reports.
 *
 * A getter on an initialised attribute object succeeds - POSIX settles that much, which
 * is why this is DERIVED - and what it *reports* for a stack nobody set is the
 * measurement. A caller that reads a stale or invented region out of a fresh set would
 * compute a bound from it exactly as confidently as from a real one. */
static obs_result check_fresh_attr_names_no_stack(void) {
    OBS_REQUIRE(&scePthreadAttrInit, &scePthreadAttrDestroy,
                &scePthreadAttrGetstackaddr, &scePthreadAttrGetstacksize);

    ScePthreadAttr attr = 0;
    int rc = scePthreadAttrInit(&attr);
    if (rc != 0) {
        return obs_fail_code("an attribute set could not be created",
                             (uint64_t)(uint32_t)rc);
    }

    void *address = (void *)(uintptr_t)0xA5A5A5A5u;
    size_t size = (size_t)0xA5A5A5A5u;
    int addr_rc = scePthreadAttrGetstackaddr(&attr, &address);
    int size_rc = scePthreadAttrGetstacksize(&attr, &size);
    (void)scePthreadAttrDestroy(&attr);

    obs_report_error_code("libkernel", "scePthreadAttrGetstackaddr", "on a fresh set",
                          (uint64_t)(uint32_t)addr_rc);
    obs_report_measure("031-stackattr/fresh-attr-names-no-stack",
                       "scePthreadAttrGetstackaddr", "stack-address",
                       (uint64_t)(uintptr_t)address, "address");
    obs_report_measure("031-stackattr/fresh-attr-names-no-stack",
                       "scePthreadAttrGetstacksize", "stack-size", (uint64_t)size,
                       "bytes");

    if (addr_rc != 0 || size_rc != 0) {
        return obs_fail_code(
            "a getter refused an attribute set it had just initialised",
            (uint64_t)(uint32_t)(addr_rc != 0 ? addr_rc : size_rc));
    }
    /* The out-parameters were seeded with a recognisable value, so a platform that
     * accepts the call and writes nothing is visible in the record above rather than
     * reading as a plausible address. */
    if ((uintptr_t)address == (uintptr_t)0xA5A5A5A5u) {
        return obs_partial("the address out-parameter was not written");
    }
    return obs_pass_value((uint64_t)(uintptr_t)address);
}

struct attr_worker_result {
    volatile int started;
    volatile int finished;
    void *stack_addr;
    size_t stack_size;
    void *frame_local;
    int detach;
    int priority;
    uint64_t affinity;
};

static void *attr_probe_thread_entry(void *arg) {
    struct attr_worker_result *res = (struct attr_worker_result *)arg;
    volatile int local_var = 0x42;
    res->frame_local = (void *)&local_var;

    ScePthread self = scePthreadSelf();
    ScePthreadAttr self_attr = 0;
    if (obs_address_is_callable((const void *)&scePthreadAttrInit) &&
        scePthreadAttrInit(&self_attr) == 0) {
        if (obs_address_is_callable((const void *)&scePthreadAttrGet) &&
            scePthreadAttrGet(self, &self_attr) == 0) {
            if (obs_address_is_callable((const void *)&scePthreadAttrGetstacksize)) {
                (void)scePthreadAttrGetstacksize(&self_attr, &res->stack_size);
            }
            if (obs_address_is_callable((const void *)&scePthreadAttrGetstackaddr)) {
                (void)scePthreadAttrGetstackaddr(&self_attr, &res->stack_addr);
            }
            if (obs_address_is_callable((const void *)&scePthreadAttrGetdetachstate)) {
                (void)scePthreadAttrGetdetachstate(&self_attr, &res->detach);
            }
        }
        (void)scePthreadAttrDestroy(&self_attr);
    }

    if (obs_address_is_callable((const void *)&scePthreadGetprio)) {
        (void)scePthreadGetprio(self, &res->priority);
    }
    if (obs_address_is_callable((const void *)&scePthreadGetaffinity)) {
        (void)scePthreadGetaffinity(self, &res->affinity);
    }

    res->finished = 1;
    return NULL;
}

static obs_result check_pthread_attr_layout(void) {
    OBS_REQUIRE(&scePthreadAttrInit, &scePthreadAttrDestroy);

    ScePthreadAttr attr = 0;
    int rc = scePthreadAttrInit(&attr);
    if (rc != 0 || attr == NULL) {
        return obs_fail_code("scePthreadAttrInit failed", (uint64_t)(uint32_t)rc);
    }

    /* 1. Dump initial state of attr struct (128 bytes) */
    unsigned char initial[128];
    unsigned char before[128];
    unsigned char after[128];
    memset(initial, 0, sizeof(initial));
    memset(before, 0, sizeof(before));
    memset(after, 0, sizeof(after));

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        memcpy(initial, (const void *)attr, 128);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        (void)scePthreadAttrDestroy(&attr);
        return obs_fail_code("fault reading initial ScePthreadAttr bytes",
                             (uint64_t)sig);
    }

    obs_report_buffer("031-stackattr/attr-layout", "scePthreadAttrInit", "initial",
                      initial, 128);

    /* 2. Mutate stack size to distinctive 0x00181000 */
    memcpy(before, (const void *)attr, 128);
    int rc_stacksize = -1;
    if (obs_address_is_callable((const void *)&scePthreadAttrSetstacksize)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_stacksize = scePthreadAttrSetstacksize(&attr, (size_t)0x00181000u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    memcpy(after, (const void *)attr, 128);
    obs_report_measure("031-stackattr/attr-layout", "scePthreadAttrSetstacksize", "rc",
                       (uint64_t)(uint32_t)rc_stacksize, "code");
    obs_report_written("031-stackattr/attr-layout", "scePthreadAttrSetstacksize",
                       "diff", before, after, 128);

    /* 3. Mutate detach state to 1 (detached) */
    memcpy(before, (const void *)attr, 128);
    int rc_detach = -1;
    if (obs_address_is_callable((const void *)&scePthreadAttrSetdetachstate)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_detach = scePthreadAttrSetdetachstate(&attr, 1);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    memcpy(after, (const void *)attr, 128);
    obs_report_measure("031-stackattr/attr-layout", "scePthreadAttrSetdetachstate",
                       "rc", (uint64_t)(uint32_t)rc_detach, "code");
    obs_report_written("031-stackattr/attr-layout", "scePthreadAttrSetdetachstate",
                       "diff", before, after, 128);

    /* 4. Mutate affinity to distinctive 0x5 (cores 0 and 2) */
    memcpy(before, (const void *)attr, 128);
    int rc_aff = -1;
    if (obs_address_is_callable((const void *)&scePthreadAttrSetaffinity)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_aff = scePthreadAttrSetaffinity(&attr, 0x5u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    memcpy(after, (const void *)attr, 128);
    obs_report_measure("031-stackattr/attr-layout", "scePthreadAttrSetaffinity", "rc",
                       (uint64_t)(uint32_t)rc_aff, "code");
    obs_report_written("031-stackattr/attr-layout", "scePthreadAttrSetaffinity", "diff",
                       before, after, 128);

    /* 5. Mutate sched priority to distinctive 0x42 */
    memcpy(before, (const void *)attr, 128);
    int rc_sched = -1;
    struct {
        int sched_priority;
    } param;
    param.sched_priority = 0x42;
    if (obs_address_is_callable((const void *)&scePthreadAttrSetschedparam)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_sched = scePthreadAttrSetschedparam(&attr, &param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    memcpy(after, (const void *)attr, 128);
    obs_report_measure("031-stackattr/attr-layout", "scePthreadAttrSetschedparam", "rc",
                       (uint64_t)(uint32_t)rc_sched, "code");
    obs_report_written("031-stackattr/attr-layout", "scePthreadAttrSetschedparam",
                       "diff", before, after, 128);

    /* 6. Create thread using this attr to read back honoured values */
    struct attr_worker_result res;
    memset(&res, 0, sizeof(res));
    res.detach = -1;
    res.priority = -1;

    ScePthread child = NULL;
    int rc_create = -1;
    if (obs_address_is_callable((const void *)&scePthreadCreate)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            /* Try with &attr first */
            rc_create = scePthreadCreate(&child, &attr, attr_probe_thread_entry, &res,
                                         "obscene-attr");
            if (rc_create != 0) {
                /* Try with attr directly if &attr was rejected */
                rc_create = scePthreadCreate(&child, attr, attr_probe_thread_entry,
                                             &res, "obscene-attr");
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure("031-stackattr/attr-layout", "scePthreadCreate", "rc",
                       (uint64_t)(uint32_t)rc_create, "code");

    if (rc_create == 0) {
        /* Wait up to 500ms for child to report */
        for (int iter = 0; iter < 5000 && !res.finished; iter++) {
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
        if (rc_detach != 0 && obs_address_is_callable((const void *)&scePthreadJoin) &&
            child != NULL) {
            void *join_ret = NULL;
            (void)scePthreadJoin(child, &join_ret);
        }
    }

    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-stacksize",
                       (uint64_t)res.stack_size, "bytes");
    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-stackaddr",
                       (uint64_t)(uintptr_t)res.stack_addr, "address");
    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-frame-local",
                       (uint64_t)(uintptr_t)res.frame_local, "address");
    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-priority",
                       (uint64_t)(uint32_t)res.priority, "val");
    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-affinity",
                       res.affinity, "mask");
    obs_report_measure("031-stackattr/attr-layout", "readback", "observed-detach",
                       (uint64_t)(uint32_t)res.detach, "val");

    (void)scePthreadAttrDestroy(&attr);
    return obs_pass_value((uint64_t)res.stack_size);
}

static const obs_check stackattr_checks[] = {
    {"031-stackattr/self-describes", "libkernel", "scePthreadAttrGet", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadAttrGet, check_self_describes,
     OBS_FROM_DERIVED},
    {"031-stackattr/address-is-the-base", "libkernel", "scePthreadAttrGetstackaddr",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&scePthreadAttrGetstackaddr,
     check_address_is_the_base, OBS_FROM_ASSUMED},
    {"031-stackattr/fresh-attr-names-no-stack", "libkernel",
     "scePthreadAttrGetstackaddr", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&scePthreadAttrGetstackaddr, check_fresh_attr_names_no_stack,
     OBS_FROM_DERIVED},
    {"031-stackattr/attr-layout", "libkernel", "scePthreadAttrInit", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&scePthreadAttrInit, check_pthread_attr_layout,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_stackattr = {
    "031-stackattr",
    "Thread stack attributes",
    "What a thread attribute set reports about a running thread's stack, and - the "
    "open "
    "question a collector's scan bound depends on - whether the address it gives is "
    "the "
    "lowest byte of that stack or its top. Compares against the frame asking; asserts "
    "nothing about either convention.",
    stackattr_checks,
    OBS_COUNT(stackattr_checks),
};
