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

/* Stable lowercase name, used in the machine-readable line. */
const char *obs_status_name(obs_status status);

#endif /* OBSCENE_STATUS_H */
