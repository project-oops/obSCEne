/*
 * The maths half of the C library.
 *
 * Separate from 035-libc because it is a distinct concern and often a distinct
 * implementation, and because floating point fails in its own particular ways -
 * sign handling around zero, rounding direction on negatives, and single-precision
 * variants quietly forwarding to the double version.
 *
 * # Every comparison here is exact
 *
 * Each value checked against is exactly representable in binary floating point:
 * sqrt(2.25) is 1.5 and nothing else, pow(2, 10) is 1024 and nothing else. So the
 * checks compare with `!=` and need no tolerance.
 *
 * That is deliberate. Picking an epsilon would mean inventing a specification nobody
 * wrote, and a tolerance is exactly where a wrong answer hides - an implementation
 * off by one unit in the last place passes a loose check forever. Where a result
 * genuinely is not exact, the function is left out rather than checked loosely.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/* Reports a wrong result by its bit pattern rather than its value.
 *
 * The report carries integers, and a decimal rendering would need a float formatter
 * this program does not have. Bits are also the better answer: they distinguish
 * negative zero from zero, and one quiet NaN from another, which is precisely the
 * class of fault worth catching here. */
static uint64_t bits_of(double value) {
    union {
        double d;
        uint64_t u;
    } pun;
    pun.d = value;
    return pun.u;
}

static obs_result check_sqrt(void) {
    if (sqrt(4.0) != 2.0) {
        return obs_fail_code("sqrt(4) is wrong", bits_of(sqrt(4.0)));
    }
    if (sqrt(2.25) != 1.5) {
        return obs_fail_code("sqrt(2.25) is wrong", bits_of(sqrt(2.25)));
    }
    if (sqrt(0.0) != 0.0) {
        return obs_fail_code("sqrt(0) is wrong", bits_of(sqrt(0.0)));
    }
    if (sqrt(1.0) != 1.0) {
        return obs_fail_code("sqrt(1) is wrong", bits_of(sqrt(1.0)));
    }
    return obs_pass();
}

static obs_result check_pow(void) {
    if (pow(2.0, 10.0) != 1024.0) {
        return obs_fail_code("pow(2, 10) is wrong", bits_of(pow(2.0, 10.0)));
    }
    /* Anything to the zero is one, including zero. A conditional that special-cases
     * the base before the exponent gets this wrong. */
    if (pow(3.0, 0.0) != 1.0 || pow(0.0, 0.0) != 1.0) {
        return obs_fail("a zero exponent did not give one");
    }
    if (pow(2.0, -1.0) != 0.5) {
        return obs_fail_code("a negative exponent is wrong", bits_of(pow(2.0, -1.0)));
    }
    return obs_pass();
}

static obs_result check_fabs(void) {
    if (fabs(-3.5) != 3.5 || fabs(3.5) != 3.5) {
        return obs_fail("absolute value is wrong");
    }
    /* fabs(-0.0) must be +0.0. Comparing with == cannot tell them apart, so the sign
     * bit is checked directly - an implementation that returns its argument unchanged
     * for negative zero passes every value comparison and still produces -0.0. */
    if (bits_of(fabs(-0.0)) != bits_of(0.0)) {
        return obs_partial_value("fabs(-0.0) kept its sign bit", bits_of(fabs(-0.0)));
    }
    return obs_pass();
}

static obs_result check_floor_ceil(void) {
    OBS_REQUIRE(&ceil);
    if (floor(1.5) != 1.0 || ceil(1.5) != 2.0) {
        return obs_fail("rounding a positive value is wrong");
    }
    /* The negative cases are where implementations break: floor goes *down* to -2 and
     * ceil goes *up* to -1. Truncating toward zero gets both backwards and looks
     * right for every positive input anyone tests with. */
    if (floor(-1.5) != -2.0) {
        return obs_fail_code("floor(-1.5) did not round down", bits_of(floor(-1.5)));
    }
    if (ceil(-1.5) != -1.0) {
        return obs_fail_code("ceil(-1.5) did not round up", bits_of(ceil(-1.5)));
    }
    /* Exact integers must be left alone rather than nudged. */
    if (floor(3.0) != 3.0 || ceil(3.0) != 3.0) {
        return obs_fail("an exact integer was altered by rounding");
    }
    return obs_pass();
}

static obs_result check_fmod(void) {
    if (fmod(7.0, 3.0) != 1.0) {
        return obs_fail_code("fmod is wrong", bits_of(fmod(7.0, 3.0)));
    }
    /* The result takes the sign of the *numerator*, not the denominator. This is the
     * opposite of the modulo operator in several languages, and getting it wrong
     * silently mirrors anything that wraps an angle or an index. */
    if (fmod(-7.0, 3.0) != -1.0) {
        return obs_fail_code("a negative numerator gave the wrong sign",
                             bits_of(fmod(-7.0, 3.0)));
    }
    if (fmod(7.0, -3.0) != 1.0) {
        return obs_fail_code("a negative denominator changed the sign",
                             bits_of(fmod(7.0, -3.0)));
    }
    return obs_pass();
}

static obs_result check_trigonometry(void) {
    OBS_REQUIRE(&cos);
    /* Only the exact points are checked. sin(pi/6) is 0.5 in theory and not in binary
     * floating point, and checking it would need a tolerance this section refuses to
     * invent. */
    if (sin(0.0) != 0.0) {
        return obs_fail_code("sin(0) is wrong", bits_of(sin(0.0)));
    }
    if (cos(0.0) != 1.0) {
        return obs_fail_code("cos(0) is wrong", bits_of(cos(0.0)));
    }
    /* An implementation returning a constant zero for everything passes sin(0) alone,
     * so something non-zero has to be checked too - without asserting its value. */
    if (cos(1.0) == 0.0 && sin(1.0) == 0.0) {
        return obs_fail("both functions return zero for a non-zero angle");
    }
    return obs_pass();
}

static obs_result check_single_precision(void) {
    OBS_REQUIRE(&fabsf);
    if (sqrtf(4.0f) != 2.0f || sqrtf(2.25f) != 1.5f) {
        return obs_fail("single-precision square root is wrong");
    }
    if (fabsf(-2.5f) != 2.5f || fabsf(2.5f) != 2.5f) {
        return obs_fail("single-precision absolute value is wrong");
    }
    /* A single-precision function that forwards to the double version and returns the
     * wide result works numerically and returns in the wrong register class. That is
     * an ABI fault rather than a maths one, and it corrupts the caller instead of the
     * answer - which is why it is worth a check of its own. */
    if (sqrtf(1.0f) != 1.0f) {
        return obs_fail("single-precision square root of one is wrong");
    }
    return obs_pass();
}

static obs_result check_round(void) {
    /* The halfway cases are the whole point. ISO C says round is away from zero, so
     * 2.5 is 3 and -2.5 is -3. The hardware's default rounding mode is nearest-even
     * and would answer 2 and -2; an implementation written as "add a half, truncate"
     * answers 3 and -2. Both are wrong in a way no other input reveals. */
    if (round(2.5) != 3.0) {
        return obs_fail_code("round(2.5) is not away from zero", bits_of(round(2.5)));
    }
    if (round(-2.5) != -3.0) {
        return obs_fail_code("round(-2.5) is not away from zero", bits_of(round(-2.5)));
    }
    if (round(3.5) != 4.0) {
        return obs_fail_code("round(3.5) is wrong", bits_of(round(3.5)));
    }
    if (round(2.4) != 2.0 || round(2.6) != 3.0) {
        return obs_fail("round is wrong away from the halfway point");
    }
    if (round(-0.5) != -1.0) {
        return obs_fail_code("round(-0.5) is wrong", bits_of(round(-0.5)));
    }
    return obs_pass();
}

static obs_result check_trunc(void) {
    if (trunc(2.7) != 2.0) {
        return obs_fail_code("trunc(2.7) is wrong", bits_of(trunc(2.7)));
    }
    /* Toward zero, not downward. An implementation that forwards to floor answers -3
     * here and is right about every positive input. */
    if (trunc(-2.7) != -2.0) {
        return obs_fail_code("trunc(-2.7) rounds down, not toward zero",
                             bits_of(trunc(-2.7)));
    }
    if (trunc(2.0) != 2.0 || trunc(-2.0) != -2.0) {
        return obs_fail("trunc changes a value that is already whole");
    }
    return obs_pass();
}

static obs_result check_exp_log(void) {
    OBS_REQUIRE(&log, &log10, &log2);
    if (exp(0.0) != 1.0) {
        return obs_fail_code("exp(0) is wrong", bits_of(exp(0.0)));
    }
    if (log(1.0) != 0.0) {
        return obs_fail_code("log(1) is wrong", bits_of(log(1.0)));
    }
    /* Powers of two are exact in log2, which is why it is checked and log(e) is not. */
    if (log2(8.0) != 3.0) {
        return obs_fail_code("log2(8) is wrong", bits_of(log2(8.0)));
    }
    if (log2(1.0) != 0.0) {
        return obs_fail_code("log2(1) is wrong", bits_of(log2(1.0)));
    }
    if (log2(0.25) != -2.0) {
        return obs_fail_code("log2 of a fraction is wrong", bits_of(log2(0.25)));
    }
    if (log10(1000.0) != 3.0) {
        return obs_fail_code("log10(1000) is wrong", bits_of(log10(1000.0)));
    }
    if (log10(1.0) != 0.0) {
        return obs_fail_code("log10(1) is wrong", bits_of(log10(1.0)));
    }
    return obs_pass();
}

static obs_result check_inverse_trigonometry(void) {
    OBS_REQUIRE(&acos, &asin, &atan, &tan);
    /* Only the points with exact answers. asin(1) is pi/2, which no binary format
     * holds exactly, so checking it would mean picking a tolerance - and a tolerance
     * is where a wrong answer hides. */
    if (tan(0.0) != 0.0) {
        return obs_fail_code("tan(0) is wrong", bits_of(tan(0.0)));
    }
    if (asin(0.0) != 0.0) {
        return obs_fail_code("asin(0) is wrong", bits_of(asin(0.0)));
    }
    if (acos(1.0) != 0.0) {
        return obs_fail_code("acos(1) is wrong", bits_of(acos(1.0)));
    }
    if (atan(0.0) != 0.0) {
        return obs_fail_code("atan(0) is wrong", bits_of(atan(0.0)));
    }
    if (atan2(0.0, 1.0) != 0.0) {
        return obs_fail_code("atan2(0, 1) is wrong", bits_of(atan2(0.0, 1.0)));
    }
    /* Every value above is zero, because zero is the only exact answer these have. So a
     * function that returns zero to everything passes all five, and a pass here would
     * mean nothing at all - which is what happened the first time this ran under an
     * emulator whose maths library is stubbed.
     *
     * These pairs merely have to differ from each other. That is the 007-responsive
     * argument applied inside a value check: it separates "silent" from "wrong", and
     * without it this check cannot tell them apart. */
    if (bits_of(tan(1.0)) == bits_of(tan(2.0))) {
        return obs_fail("tan returns one value for two different inputs");
    }
    if (bits_of(asin(0.5)) == bits_of(asin(0.25))) {
        return obs_fail("asin returns one value for two different inputs");
    }
    if (bits_of(acos(0.5)) == bits_of(acos(0.25))) {
        return obs_fail("acos returns one value for two different inputs");
    }
    if (bits_of(atan(1.0)) == bits_of(atan(2.0))) {
        return obs_fail("atan returns one value for two different inputs");
    }
    if (bits_of(atan2(1.0, 2.0)) == bits_of(atan2(2.0, 1.0))) {
        return obs_fail("atan2 returns one value for two different inputs");
    }
    return obs_pass();
}

static obs_result check_single_precision_family(void) {
    OBS_REQUIRE(&ceilf, &cosf, &expf, &floorf, &fmodf, &logf, &sinf, &tanf);
    if (floorf(2.7f) != 2.0f || floorf(-2.1f) != -3.0f) {
        return obs_fail("floorf is wrong");
    }
    if (ceilf(2.1f) != 3.0f || ceilf(-2.7f) != -2.0f) {
        return obs_fail("ceilf is wrong");
    }
    /* 9 and 4, not 7 and 4: fmod(7, 4) and fmod(11, 4) are both 3, and a check built
     * on inputs whose answers happen to agree reports a working function as broken.
     * That mistake was made once already, in the responsiveness probes. */
    if (fmodf(9.0f, 4.0f) != 1.0f) {
        return obs_fail("fmodf is wrong");
    }
    if (powf(2.0f, 10.0f) != 1024.0f || powf(3.0f, 0.0f) != 1.0f) {
        return obs_fail("powf is wrong");
    }
    if (expf(0.0f) != 1.0f || logf(1.0f) != 0.0f) {
        return obs_fail("expf or logf is wrong at its exact point");
    }
    if (sinf(0.0f) != 0.0f || cosf(0.0f) != 1.0f || tanf(0.0f) != 0.0f) {
        return obs_fail("single-precision trigonometry is wrong at zero");
    }
    return obs_pass();
}

static obs_result check_text_to_float(void) {
    OBS_REQUIRE(&strtof);
    char *end = 0;
    /* 2.5 and 0.25 are sums of powers of two, so these are exact and no tolerance is
     * needed. A decimal like 0.1 would need one, which is why it is not here. */
    if (strtod("2.5", &end) != 2.5) {
        return obs_fail_code("strtod(\"2.5\") is wrong", bits_of(strtod("2.5", &end)));
    }
    if (end == 0 || *end != 0) {
        return obs_fail("strtod did not consume the whole number");
    }
    if (strtod("-0.25", 0) != -0.25) {
        return obs_fail("strtod is wrong on a negative fraction");
    }
    /* A number with something after it: the end pointer is the only way to tell
     * "parsed 2.5 and stopped" from "parsed something else". */
    if (strtod("2.5xyz", &end) != 2.5 || end == 0 || *end != 'x') {
        return obs_fail("strtod stopped in the wrong place");
    }
    /* Nothing numeric at all: ISO C says zero is returned and the end pointer is set
     * to the start, which is what distinguishes it from a parsed zero. */
    end = 0;
    if (strtod("zzz", &end) != 0.0) {
        return obs_fail("strtod invented a value from text that is not a number");
    }
    if (strtof("2.5", 0) != 2.5f) {
        return obs_fail("strtof is wrong");
    }
    return obs_pass();
}

static const obs_check math_checks[] = {
    {"037-math/sqrt", "libSceLibcInternal", "sqrt", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sqrt, check_sqrt, OBS_FROM_SPEC},
    {"037-math/pow", "libSceLibcInternal", "pow", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&pow, check_pow, OBS_FROM_SPEC},
    {"037-math/fabs", "libSceLibcInternal", "fabs", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&fabs, check_fabs, OBS_FROM_SPEC},
    {"037-math/floor-ceil", "libSceLibcInternal", "floor", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&floor, check_floor_ceil, OBS_FROM_SPEC},
    {"037-math/fmod", "libSceLibcInternal", "fmod", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&fmod, check_fmod, OBS_FROM_SPEC},
    {"037-math/trigonometry", "libSceLibcInternal", "sin", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sin, check_trigonometry, OBS_FROM_SPEC},
    {"037-math/single-precision", "libSceLibcInternal", "sqrtf", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sqrtf, check_single_precision, OBS_FROM_SPEC},
    {"037-math/round", "libSceLibcInternal", "round", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&round, check_round, OBS_FROM_SPEC},
    {"037-math/trunc", "libSceLibcInternal", "trunc", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&trunc, check_trunc, OBS_FROM_SPEC},
    {"037-math/exp-log", "libSceLibcInternal", "exp", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&exp, check_exp_log, OBS_FROM_SPEC},
    {"037-math/inverse-trigonometry", "libSceLibcInternal", "atan2", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&atan2, check_inverse_trigonometry, OBS_FROM_SPEC},
    {"037-math/single-precision-family", "libSceLibcInternal", "powf", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&powf, check_single_precision_family, OBS_FROM_SPEC},
    {"037-math/text-to-float", "libSceLibcInternal", "strtod", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&strtod, check_text_to_float, OBS_FROM_SPEC},
};

const obs_section obs_section_math = {
    "037-math",
    "Floating-point maths",
    "Exact-value checks on the maths library, where sign and rounding direction are "
    "what break.",
    math_checks,
    OBS_COUNT(math_checks),
};
