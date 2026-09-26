/*
 * The suspend/resume lifecycle surface a big-app must satisfy to be put to sleep.
 *
 * Answers REQ-20260922T2226Z-5e8c: a real title (neverball, an SDL2 game on oops-gl)
 * does not quiesce when the dashboard's Close or rest-mode asks the system to suspend
 * it - it dies with `0xa0d0c00f CPU_FAULT_SUSPENDPOINT_TIMEOUT_IN_SUSPEND_ASYNC`,
 * "No suspendPoint for 100sec". That is the CPU/suspend sibling of the GPU fault this
 * worklog already records from a wedged pipe (`0xa0d0c00c
 * GPU_FAULT_SUSPENDPOINT_TIMEOUT_IN_RUN_ASYNC`, see 166-agc). The request asks what a
 * title has to *do* so the kernel can reach a suspend point in it.
 *
 * This section is the surface, not the behaviour. It resolves the four families of
 * entry point a cooperating title would call and reports which bind and are callable:
 *
 *   1. libSceSystemServiceSuspend - the explicit cooperation calls. The corpus census
 *      names exactly three, all callable: `sceSystemServiceDeclareReadyForSuspend` is
 *      the one a title invokes to say it has reached a quiescent point and may be
 *      frozen; the Enable/Disable pair governs whether a notification is delivered
 *      first. This is the prime candidate for "what reaches the suspend point".
 *   2. libSceSystemService - the event pump. `sceSystemServiceReceiveEvent` +
 *      `sceSystemServiceGetStatus` are how a title learns a suspend is coming (the
 *      event it must receive, and per the request may have to acknowledge).
 *   3. libSceSysCore - the sceApplication lifecycle: IsSuspendable / Suspend / Resume /
 *      SystemSuspend, the higher-level surface the same freeze is driven through.
 *   4. libSceAgc - `sceAgcSuspendPoint`, the GPU command-stream suspend point. Its
 *      presence is what a driver or title reaches so an in-flight pipe does not hold
 *      the process out of suspend (the request's arm 1: does outstanding GPU state
 *      block the suspend point).
 *
 * **No suspend/lifecycle entry point is ever called (D008).** Two reasons beyond the
 * usual: the arities are unconfirmed, and `sceSystemServiceDeclareReadyForSuspend`
 * would, if it does what its name says, declare *this* probe ready to be frozen
 * mid-run. The `load-on-demand` check (5) does call `sceKernelLoadStartModule` - a
 * confirmed-signature loader used across the suite - to answer whether the absent
 * libraries can be brought in, but it loads only; it still calls none of their symbols.
 * The one question this surface cannot answer is the behavioural half of arm 1 -
 * whether a specific unretired fence or bound context is what times the suspend out -
 * because an inert probe cannot observe its own suspension. That needs a live close on
 * a title that drains vs. one that does not, and it is called out as such in the
 * verdict.
 *
 * First hardware run (2026-09-23, FW 12.40) settled the surface: the event pump (2) and
 * the AGC suspend point (4) are present and reachable in a homebrew title; the
 * declare-ready library (1) and the sceApplication lifecycle (3) are not - which is
 * what check 5 then interrogates.
 */

#include "oops/freestd.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Resolve `name` from `library`, taking the .sprx spelling if the base name will not
 * load, and report resolved/callable/vaddr under `id`. Returns 1 when the symbol
 * resolved to a callable address, 0 otherwise. Degrades cleanly on the host build,
 * where `obs_module_open` refuses every library and this reports each symbol absent. */
static int suspend_resolve(const char *id, const char *library, const char *name) {
    int handle = obs_module_open(library);
    if (handle < 0) {
        /* A vendor library is commonly present only under its .sprx name in the app
         * sandbox; try that before concluding absence. */
        char alt[64];
        size_t n = obs_strlen(library);
        if (n > 0 && n + 5 < sizeof alt) {
            for (size_t i = 0; i < n; i++) {
                alt[i] = library[i];
            }
            alt[n + 0] = '.';
            alt[n + 1] = 's';
            alt[n + 2] = 'p';
            alt[n + 3] = 'r';
            alt[n + 4] = 'x';
            alt[n + 5] = '\0';
            handle = obs_module_open(alt);
        }
    }

    const void *addr = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
    int callable = (addr != NULL && obs_address_is_callable(addr)) ? 1 : 0;

    obs_report_measure(id, name, "resolved", addr != NULL ? 1u : 0u, "bool");
    obs_report_measure(id, name, "callable", (uint64_t)callable, "bool");
    if (addr != NULL) {
        obs_report_measure(id, name, "vaddr", (uint64_t)(uintptr_t)addr, "vaddr");
    }
    return callable;
}

/* 1. The explicit cooperation calls: libSceSystemServiceSuspend. The census names
 * exactly these three and marks all three callable, so a title that never resolves
 * `sceSystemServiceDeclareReadyForSuspend` is missing the call that reaches the
 * suspend point - which is the shape of neverball's timeout. */
static obs_result check_suspend_declare_ready(void) {
    static const char *const syms[] = {
        "sceSystemServiceDeclareReadyForSuspend",
        "sceSystemServiceEnableSuspendNotification",
        "sceSystemServiceDisableSuspendNotification",
    };
    unsigned int resolved = 0;
    int declare_ready = 0;
    for (size_t i = 0; i < OBS_COUNT(syms); i++) {
        int ok = suspend_resolve("141-suspend/declare-ready",
                                 "libSceSystemServiceSuspend", syms[i]);
        resolved += (unsigned int)ok;
        if (i == 0) {
            declare_ready = ok;
        }
    }
    obs_report_measure("141-suspend/declare-ready", "libSceSystemServiceSuspend",
                       "resolved-of-3", (uint64_t)resolved, "count");
    if (declare_ready) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value(
            "suspend library present but DeclareReadyForSuspend did not resolve",
            (uint64_t)resolved);
    }
    return obs_skip("libSceSystemServiceSuspend did not load");
}

/* 2. The event pump: how a title learns a suspend is imminent, and (per arm 2) the
 * event it may be required to acknowledge. */
static obs_result check_suspend_receive_event(void) {
    static const char *const syms[] = {
        "sceSystemServiceReceiveEvent",      "sceSystemServiceGetStatus",
        "sceSystemServiceGetEventForDaemon", "sceSystemServiceGetPSButtonEvent",
        "sceSystemServiceIsAppSuspended",
    };
    unsigned int resolved = 0;
    int receive = 0;
    for (size_t i = 0; i < OBS_COUNT(syms); i++) {
        int ok = suspend_resolve("141-suspend/receive-event", "libSceSystemService",
                                 syms[i]);
        resolved += (unsigned int)ok;
        if (i == 0) {
            receive = ok;
        }
    }
    obs_report_measure("141-suspend/receive-event", "libSceSystemService",
                       "resolved-of-5", (uint64_t)resolved, "count");
    if (receive) {
        return obs_pass_value((uint64_t)resolved);
    }
    if (resolved > 0) {
        return obs_partial_value(
            "SystemService present but ReceiveEvent did not resolve",
            (uint64_t)resolved);
    }
    return obs_skip("libSceSystemService did not load");
}

/* 3. The higher-level lifecycle: libSceSysCore's sceApplication suspend/resume. The
 * same freeze, driven through the application object rather than the raw service. */
static obs_result check_suspend_application_lifecycle(void) {
    static const char *const syms[] = {
        "sceApplicationIsSuspendable",
        "sceApplicationSuspend",
        "sceApplicationResume",
        "sceApplicationSystemSuspend",
        "sceApplicationLocalProcessSuspend",
        "sceApplicationLocalProcessResume",
    };
    unsigned int resolved = 0;
    for (size_t i = 0; i < OBS_COUNT(syms); i++) {
        resolved += (unsigned int)suspend_resolve("141-suspend/application-lifecycle",
                                                  "libSceSysCore", syms[i]);
    }
    obs_report_measure("141-suspend/application-lifecycle", "libSceSysCore",
                       "resolved-of-6", (uint64_t)resolved, "count");
    if (resolved > 0) {
        return obs_pass_value((uint64_t)resolved);
    }
    return obs_skip("no sceApplication lifecycle symbols resolved");
}

/* 4. The GPU-side suspend point: libSceAgc's sceAgcSuspendPoint. Its presence is the
 * evidence for the request's arm 1 - a GPU pipe has an explicit point the stream must
 * reach for the process to be suspendable, and an in-flight submit that never reaches
 * one is a candidate for holding the process out of suspend. This resolves the
 * primitive; whether a specific unretired fence times the suspend out is behavioural
 * and stated in the verdict. */
static obs_result check_suspend_agc_point(void) {
    int ok = suspend_resolve("141-suspend/agc-suspend-point", "libSceAgc",
                             "sceAgcSuspendPoint");

    /* The suspend-timeout fault this section exists for, recorded so a reader diffing
     * runs sees the code beside the surface that would prevent it. */
    obs_report_measure("141-suspend/agc-suspend-point", "kernel",
                       "cpu-suspend-timeout-fault", 0xa0d0c00fULL, "fault");
    obs_report_measure("141-suspend/agc-suspend-point", "kernel",
                       "gpu-suspend-timeout-fault", 0xa0d0c00cULL, "fault");
    /* Behavioural, not resolvable inert: whether draining the GPU (retiring fences,
     * unbinding) is required before a suspend point can be reached. Needs a live close
     * comparing a title that drains against one that does not. */
    obs_report_measure("141-suspend/agc-suspend-point", "gpu-drain-required",
                       "needs-live-suspend", 1u, "bool");

    if (ok) {
        return obs_pass_value(1);
    }
    return obs_skip("sceAgcSuspendPoint did not resolve");
}

/* Attempt to load one `.sprx` by path and report it. `res` is poisoned so an untouched
 * out-param is a visible answer (D303, the same argument 106-encoder makes): a bare 0
 * cannot be told from "never written". Returns the module handle, or the negative rc if
 * the load refused. */
static int suspend_try_load(const char *id, const char *path) {
    int res = (int)0xC7C7C7C7u;
    int handle = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, &res);
    obs_report_measure(id, path, "handle", (uint64_t)(uint32_t)handle, "handle");
    obs_report_measure(id, path, "res", (uint64_t)(uint32_t)res, "code");
    return handle;
}

/* 5. Loadable on demand? The first hardware run (2026-09-23, FW 12.40) found
 * libSceSystemServiceSuspend and the libSceSysCore lifecycle absent from a homebrew
 * title's address space - `declare-ready` and `application-lifecycle` both skipped,
 * zero resolved - while the event pump and the AGC suspend point were present. Neither
 * library is in the sysmodule id table, so `obs_module_open` only ever tried
 * `LoadStartModule` over its path prefixes and swallowed the codes. This asks the same
 * question out loud: it calls `sceKernelLoadStartModule` on each candidate `.sprx` with
 * a poisoned result word, reporting the handle and code per path so "not found" is told
 * apart from "refused", then re-resolves the two entry points to report whether a load
 * made them appear. No id is guessed - the paths are built from the census library
 * names, which are authoritative. */
static obs_result check_suspend_load_on_demand(void) {
    if (!obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        return obs_skip("sceKernelLoadStartModule is not callable");
    }

    static const char *const suspend_paths[] = {
        "/system/common/lib/libSceSystemServiceSuspend.sprx",
        "/system/priv/lib/libSceSystemServiceSuspend.sprx",
        "/system_ex/common_ex/lib/libSceSystemServiceSuspend.sprx",
        "/system_ex/priv/lib/libSceSystemServiceSuspend.sprx",
    };
    static const char *const syscore_paths[] = {
        "/system/common/lib/libSceSysCore.sprx",
        "/system/priv/lib/libSceSysCore.sprx",
        "/system_ex/common_ex/lib/libSceSysCore.sprx",
        "/system_ex/priv/lib/libSceSysCore.sprx",
    };

    int suspend_loaded = 0;
    for (size_t i = 0; i < OBS_COUNT(suspend_paths); i++) {
        if (suspend_try_load("141-suspend/load-on-demand", suspend_paths[i]) > 0) {
            suspend_loaded = 1;
        }
    }
    int syscore_loaded = 0;
    for (size_t i = 0; i < OBS_COUNT(syscore_paths); i++) {
        if (suspend_try_load("141-suspend/load-on-demand", syscore_paths[i]) > 0) {
            syscore_loaded = 1;
        }
    }
    obs_report_measure("141-suspend/load-on-demand", "libSceSystemServiceSuspend",
                       "loaded", (uint64_t)suspend_loaded, "bool");
    obs_report_measure("141-suspend/load-on-demand", "libSceSysCore", "loaded",
                       (uint64_t)syscore_loaded, "bool");

    /* Re-resolve now that a load has been attempted - if the load took, obs_module_open
     * finds the module in the list this time and the symbol appears. */
    int declare_after =
        suspend_resolve("141-suspend/load-on-demand", "libSceSystemServiceSuspend",
                        "sceSystemServiceDeclareReadyForSuspend");
    int suspendable_after = suspend_resolve(
        "141-suspend/load-on-demand", "libSceSysCore", "sceApplicationIsSuspendable");

    if (declare_after || suspendable_after) {
        return obs_pass_value(
            (uint64_t)((declare_after ? 1u : 0u) + (suspendable_after ? 1u : 0u)));
    }
    if (suspend_loaded || syscore_loaded) {
        return obs_partial_value(
            "a library loaded on demand but its entry point still did not resolve",
            (uint64_t)((suspend_loaded ? 1u : 0u) + (syscore_loaded ? 1u : 0u)));
    }
    return obs_skip("neither library loaded on demand from any candidate path");
}

/* 6. Call it honestly (REQ-20260923T0055Z-9b41). Checks 1-5 resolve the surface and
 * never call it (D008); this one call is the exception the request asks for, and a
 * probe can afford it where the SDK cannot: it runs under a fault guard that recovers
 * the whole run, in the same one-argument shape oops-sdk's pump already uses on
 * hardware without crashing (so the arity is empirically safe - a size-typed second
 * argument would be the danger, and passing a pointer for it is worse, not better). It
 * measures the two constants the pump assumes: the return value when the queue is
 * empty, and how many bytes the call writes. The buffer is poisoned so the write extent
 * needs no layout, and the queue is drained first so the reported "no-event" return is
 * the value after the events obscene's own launch delivered are gone. */
static obs_result check_suspend_receive_event_call(void) {
    const char *id = "141-suspend/receive-event-call";
    int handle = obs_module_open("libSceSystemService");
    if (handle < 0) {
        handle = obs_module_open("libSceSystemService.sprx");
    }
    const void *fn = (handle >= 0)
                         ? obs_module_symbol(handle, "sceSystemServiceReceiveEvent")
                         : NULL;
    if (fn == NULL || !obs_address_is_callable(fn)) {
        return obs_skip("sceSystemServiceReceiveEvent did not resolve");
    }

    /* One argument, exactly as oops-sdk's pump calls it. 512 is the size the pump hands
     * it; 1024 here so an over-write is caught rather than hidden. */
    static unsigned char event[1024];
    static unsigned char first_before[1024];
    static unsigned char first_after[1024];
    typedef int (*recv_fn_t)(void *);
    recv_fn_t call = (recv_fn_t)fn;

    int first_rc = 0;
    unsigned int first_written = 0;
    unsigned int events_drained = 0;
    int got_empty = 0;
    int no_event_rc = 0;

    /* Drain: call until one call leaves the poison untouched (an empty queue), bounded.
     */
    for (int iter = 0; iter < 16 && !got_empty; iter++) {
        for (size_t i = 0; i < sizeof event; i++) {
            event[i] = 0xC7u;
        }
        obs_jmp_buf jb;
        int sig = OBS_FAULT_ARM(&jb);
        int rc = 0;
        if (sig == 0) {
            rc = call(event);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure(id, "sceSystemServiceReceiveEvent", "faulted", 1u,
                               "bool");
            obs_report_measure(id, "sceSystemServiceReceiveEvent", "fault-signal",
                               (uint64_t)(uint32_t)sig, "signal");
            return obs_partial_value("call faulted under guard; arity is above one or "
                                     "the call needs setup first",
                                     (uint64_t)(uint32_t)sig);
        }
        unsigned int written = 0;
        for (size_t i = 0; i < sizeof event; i++) {
            if (event[i] != 0xC7u) {
                written = (unsigned int)(i + 1);
            }
        }
        if (iter == 0) {
            first_rc = rc;
            first_written = written;
            for (size_t i = 0; i < sizeof event; i++) {
                first_before[i] = 0xC7u;
                first_after[i] = event[i];
            }
        }
        if (written == 0u) {
            got_empty = 1;
            no_event_rc = rc;
        } else {
            events_drained++;
        }
    }

    obs_report_measure(id, "sceSystemServiceReceiveEvent", "first-return",
                       (uint64_t)(uint32_t)first_rc, "code");
    obs_report_measure(id, "sceSystemServiceReceiveEvent", "first-bytes-written",
                       (uint64_t)first_written, "bytes");
    obs_report_measure(id, "sceSystemServiceReceiveEvent", "events-drained",
                       (uint64_t)events_drained, "count");
    obs_report_measure(id, "sceSystemServiceReceiveEvent", "queue-emptied",
                       (uint64_t)got_empty, "bool");
    if (got_empty) {
        obs_report_measure(id, "sceSystemServiceReceiveEvent", "no-event-return",
                           (uint64_t)(uint32_t)no_event_rc, "code");
    }
    if (first_written > 0u) {
        obs_report_written(id, "sceSystemServiceReceiveEvent", "first-event-image",
                           first_before, first_after, 64u);
    }
    return got_empty ? obs_pass_value((uint64_t)(uint32_t)no_event_rc)
                     : obs_partial_value("queue did not empty within 16 calls",
                                         (uint64_t)events_drained);
}

static const obs_check suspend_checks[] = {
    {"141-suspend/declare-ready", "libSceSystemServiceSuspend",
     "sceSystemServiceDeclareReadyForSuspend", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_suspend_declare_ready, check_suspend_declare_ready,
     OBS_FROM_DERIVED},
    {"141-suspend/receive-event", "libSceSystemService", "sceSystemServiceReceiveEvent",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_suspend_receive_event,
     check_suspend_receive_event, OBS_FROM_DERIVED},
    {"141-suspend/application-lifecycle", "libSceSysCore",
     "sceApplicationIsSuspendable", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_suspend_application_lifecycle,
     check_suspend_application_lifecycle, OBS_FROM_DERIVED},
    {"141-suspend/agc-suspend-point", "libSceAgc", "sceAgcSuspendPoint", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_suspend_agc_point, check_suspend_agc_point,
     OBS_FROM_DERIVED},
    {"141-suspend/load-on-demand", "libSceSystemServiceSuspend",
     "sceSystemServiceDeclareReadyForSuspend", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_suspend_load_on_demand, check_suspend_load_on_demand,
     OBS_FROM_DERIVED},
    {"141-suspend/receive-event-call", "libSceSystemService",
     "sceSystemServiceReceiveEvent", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_suspend_receive_event_call, check_suspend_receive_event_call,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_suspend = {
    "141-suspend",
    "The suspend/resume lifecycle surface",
    "Which cooperation, event-pump, application-lifecycle and GPU suspend-point entry "
    "points a big-app must call to be suspended cleanly - resolved but not called, "
    "except "
    "the event pump, which is called once under a fault guard to measure what it "
    "returns "
    "and writes.",
    suspend_checks,
    OBS_COUNT(suspend_checks),
};
