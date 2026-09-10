/*
 * Check outcomes.
 *
 * The user-facing model is red / amber / green. A fourth value exists because
 * without it a single failed prerequisite cascades into a wall of false reds: if
 * allocation fails, every check that needs memory "fails" too, and the report stops
 * naming the one thing actually broken. SKIP keeps the signal where it belongs.
 */

#ifndef OBSCENE_STATUS_H
#define OBSCENE_STATUS_H

#include <stddef.h>
#include <stdint.h>

typedef enum obs_status {
    /* Green. The call succeeded and every postcondition held. */
    OBS_PASS = 0,
    /* Amber. It returned, but something was off - a success code with a
     * nonsensical value, or a documented "not supported" answered gracefully.
     * Distinct from PASS because an emulator stub returning zero for everything
     * would otherwise look perfect. */
    OBS_PARTIAL = 1,
    /* Red. It returned an error where success was expected. */
    OBS_FAIL = 2,
    /* Grey. A prerequisite did not hold, so this was never attempted. Not a
     * verdict on the function - it says nothing was learned. */
    OBS_SKIP = 3,
    /* Black. The call did not return - it faulted (SIGSEGV and its kin) and the fault
     * guard recovered the run. Distinct from FAIL, which is a call that returned an
     * error: a crash is the strongest finding a probe can make, and folding it into
     * FAIL would hide it in the counts and read as an ordinary bad result. Principle
     * 1's "a `try` with no `res` means the call did not return" now has one exception,
     * and this is it - the `res` names the crash rather than the record being absent.
     * (D325) */
    OBS_CRASH = 4,
    /* Blue. The check can run, but has not been given the input it needs - a controller
     * attached, a button pressed, a stick deflected. Distinct from SKIP, which says the
     * check does not apply here: PENDING says it does apply and is waiting. It is never
     * blocking and never fatal - the check samples its window, finds nothing, and
     * reports that it is still waiting, so a re-run once the input is provided produces
     * the real result. A `try` with no `res` still means the call did not return; a
     * PENDING `res` means the call returned but the peripheral did not. (D328) */
    OBS_PENDING = 5,
} obs_status;

/* What one check observed. */
typedef struct obs_result {
    obs_status status;
    /* Short explanation. Static storage only - the harness does not copy it. */
    const char *detail;
    /* The value or error code observed, reported when has_value is set. This is
     * what makes a run diffable: a return code that changes between builds is the
     * signal, and prose describing it is not. */
    uint64_t value;
    int has_value;
} obs_result;

obs_result obs_pass(void);
obs_result obs_pass_value(uint64_t value);
obs_result obs_partial(const char *detail);
obs_result obs_partial_value(const char *detail, uint64_t value);
obs_result obs_fail(const char *detail);
obs_result obs_fail_code(const char *detail, uint64_t code);
obs_result obs_skip(const char *detail);
/* The call faulted and the guard recovered. `signal` is the fault signal number,
 * reported as the value so a run is diffable on which signal a call raised. Only the
 * fault guard constructs this; a check never returns it directly. */
obs_result obs_crash(int signal);
/* The check needs an input it has not been given (a peripheral, a button press).
 * Non-blocking and non-fatal: it reports what it is waiting for and nothing more, and a
 * re-run with the input produces the real result. (D328) */
obs_result obs_pending(const char *detail);

/* Stable lowercase name, used in the machine-readable line. */
const char *obs_status_name(obs_status status);

#endif /* OBSCENE_STATUS_H */
