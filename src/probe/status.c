/* Result constructors. Trivial by design: a check should read as what it observed,
 * not as struct assembly. */

#include "obscene/harness.h"
#include "obscene/status.h"

obs_result obs_pass(void) {
    obs_result r = {OBS_PASS, NULL, 0, 0};
    return r;
}

obs_result obs_pass_value(uint64_t value) {
    obs_result r = {OBS_PASS, NULL, value, 1};
    return r;
}

obs_result obs_partial(const char *detail) {
    obs_result r = {OBS_PARTIAL, detail, 0, 0};
    return r;
}

obs_result obs_partial_value(const char *detail, uint64_t value) {
    obs_result r = {OBS_PARTIAL, detail, value, 1};
    return r;
}

obs_result obs_fail(const char *detail) {
    obs_result r = {OBS_FAIL, detail, 0, 0};
    return r;
}

obs_result obs_fail_code(const char *detail, uint64_t code) {
    obs_result r = {OBS_FAIL, detail, code, 1};
    return r;
}

obs_result obs_skip(const char *detail) {
    obs_result r = {OBS_SKIP, detail, 0, 0};
    return r;
}

const char *obs_status_name(obs_status status) {
    switch (status) {
    case OBS_PASS:
        return "pass";
    case OBS_PARTIAL:
        return "partial";
    case OBS_FAIL:
        return "fail";
    case OBS_SKIP:
        return "skip";
    }
    /* Not reachable through the enum, but a corrupted value should say so rather
     * than read as a pass. */
    return "unknown";
}

const char *obs_availability_name(obs_availability availability) {
    switch (availability) {
    case OBS_SHARED:
        return "shared";
    case OBS_PREVIOUS:
        return "previous";
    case OBS_CURRENT:
        return "current";
    case OBS_AVAILABILITY_UNKNOWN:
        return "unknown";
    }
    return "unknown";
}

const char *obs_generation_name(obs_generation generation) {
    switch (generation) {
    case OBS_GENERATION_PREVIOUS:
        return "previous";
    case OBS_GENERATION_CURRENT:
        return "current";
    case OBS_GENERATION_BOTH:
        return "both";
    case OBS_GENERATION_UNKNOWN:
        return "unknown";
    }
    return "unknown";
}
