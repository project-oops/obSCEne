/*
 * Which functions are actually doing anything.
 *
 * # The problem this exists to solve
 *
 * A platform that has not implemented a function commonly resolves it to a stub that
 * returns zero. A platform that has implemented it badly returns something wrong. Both
 * produce the same record - `wcslen` returned 0, `atol` returned 0, `strncat` returned
 * 0 - and they need completely different work from whoever is fixing the platform. One
 * is "write this"; the other is "you have a bug".
 *
 * Twenty-five failures in one run looked identical for exactly this reason, and the
 * honest thing that could be said about them was "probably stubs". That is the sort of
 * "probably" this suite exists to remove.
 *
 * # How it is told apart
 *
 * Call the same function twice with inputs whose answers *must* differ, and compare the
 * two results to each other rather than to an expected value.
 *
 *   strlen("a") and strlen("abcdef")   must be 1 and 6
 *   toupper('a') and toupper('z')      must be 'A' and 'Z'
 *   abs(-5) and abs(-9)                must be 5 and 9
 *
 * A stub returns the same value both times, whatever that value is. An implementation
 * that is merely wrong still varies with its input. So two equal answers means "not
 * responding", and that verdict needs no knowledge of what the right answer is - which
 * is what makes it robust where an expectation might be argued with.
 *
 * # What it does not claim
 *
 * Responding is not the same as correct, and this section never says it is. A function
 * that answers differently to different inputs has only proven it is looking at them.
 * Whether it is right is what every other section is for.
 *
 * The converse is not quite airtight either: an implementation could vary and still be
 * a stub of a cleverer kind. Nothing here can see that, and it is not worth pretending
 * otherwise - the two-value test catches the stub that exists in practice.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* Each probe calls its function twice and hands back both answers. The caller compares
 * them; a probe never decides anything, so the rule stays in one place. */
typedef struct probe {
    const char *library;
    const char *symbol;
    const void *address;
    void (*run)(uint64_t *first, uint64_t *second);
} probe;

static void probe_strlen(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)strlen("a");
    *b = (uint64_t)strlen("abcdef");
}

static void probe_strcmp(uint64_t *a, uint64_t *b) {
    /* Opposite orders must give opposite signs, so the two cannot be equal unless the
     * function is ignoring its arguments. */
    *a = (uint64_t)(int64_t)strcmp("a", "b");
    *b = (uint64_t)(int64_t)strcmp("b", "a");
}

static void probe_toupper(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)toupper('a');
    *b = (uint64_t)toupper('z');
}

static void probe_tolower(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)tolower('A');
    *b = (uint64_t)tolower('Z');
}

static void probe_abs(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)abs(-5);
    *b = (uint64_t)abs(-9);
}

static void probe_atoi(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)atoi("7");
    *b = (uint64_t)atoi("42");
}

static void probe_atol(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)atol("7");
    *b = (uint64_t)atol("42");
}

static void probe_isdigit(uint64_t *a, uint64_t *b) {
    /* Only the zero/non-zero distinction is specified, so a platform is free to return
     * any non-zero value - which is fine here, because equality is all that is tested. */
    *a = (uint64_t)(isdigit('5') != 0);
    *b = (uint64_t)(isdigit('x') != 0);
}

static void probe_isalpha(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)(isalpha('q') != 0);
    *b = (uint64_t)(isalpha('4') != 0);
}

static void probe_strspn(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)strspn("aab", "a");
    *b = (uint64_t)strspn("aaaab", "a");
}

static void probe_wcslen(uint64_t *a, uint64_t *b) {
    static const obs_wchar two[] = {'a', 'b', 0};
    static const obs_wchar four[] = {'a', 'b', 'c', 'd', 0};
    *a = (uint64_t)wcslen(two);
    *b = (uint64_t)wcslen(four);
}

static void probe_labs(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)labs(-5L);
    *b = (uint64_t)labs(-9L);
}

static void probe_memcmp(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)(int64_t)memcmp("ab", "ac", 2);
    *b = (uint64_t)(int64_t)memcmp("ac", "ab", 2);
}

static void probe_strncmp(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)(int64_t)strncmp("abc", "abd", 3);
    *b = (uint64_t)(int64_t)strncmp("abd", "abc", 3);
}

/* Pointer-returning functions are compared by the offset they land on, not the pointer
 * itself. A stub returning null gives the same answer twice; a working one gives two
 * different offsets into the same string. */
static void probe_strchr(uint64_t *a, uint64_t *b) {
    static const char text[] = "abcdef";
    const char *first = strchr(text, 'b');
    const char *second = strchr(text, 'e');
    *a = (uint64_t)(first == NULL ? 0 : (size_t)(first - text) + 1u);
    *b = (uint64_t)(second == NULL ? 0 : (size_t)(second - text) + 1u);
}

static void probe_strstr(uint64_t *a, uint64_t *b) {
    static const char text[] = "the quick brown";
    const char *first = strstr(text, "quick");
    const char *second = strstr(text, "brown");
    *a = (uint64_t)(first == NULL ? 0 : (size_t)(first - text) + 1u);
    *b = (uint64_t)(second == NULL ? 0 : (size_t)(second - text) + 1u);
}

static void probe_strcspn(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)strcspn("abcx", "x");
    *b = (uint64_t)strcspn("abcdefx", "x");
}

static void probe_strtol(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)strtol("7", NULL, 10);
    *b = (uint64_t)strtol("42", NULL, 10);
}

static void probe_strtoul(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)strtoul("f", NULL, 16);
    *b = (uint64_t)strtoul("ff", NULL, 16);
}

/* ---- the maths library -----------------------------------------------------
 *
 * Compared by bit pattern rather than by value, for the same reason 037-math reports
 * them that way: this program has no float formatter, and bits distinguish a negative
 * zero from a zero, which is exactly the sort of difference a stub erases.
 */
static uint64_t bits_of(double value) {
    union {
        double d;
        uint64_t u;
    } pun;
    pun.d = value;
    return pun.u;
}

static uint64_t bits_of_f(float value) {
    union {
        float f;
        uint32_t u;
    } pun;
    pun.f = value;
    return pun.u;
}

static void probe_sqrt(uint64_t *a, uint64_t *b) {
    *a = bits_of(sqrt(4.0));
    *b = bits_of(sqrt(9.0));
}

static void probe_fabs(uint64_t *a, uint64_t *b) {
    *a = bits_of(fabs(-2.5));
    *b = bits_of(fabs(-7.5));
}

static void probe_floor(uint64_t *a, uint64_t *b) {
    *a = bits_of(floor(2.7));
    *b = bits_of(floor(9.7));
}

static void probe_ceil(uint64_t *a, uint64_t *b) {
    *a = bits_of(ceil(2.1));
    *b = bits_of(ceil(9.1));
}

static void probe_fmod(uint64_t *a, uint64_t *b) {
    /* 7 mod 4 is 3 and 11 mod 4 is also 3, which is how the first version of this probe
     * reported a working fmod as silent. The inputs to a responsiveness probe have to be
     * checked against a real implementation, or the probe invents stubs - and the host
     * build caught exactly that before this ran anywhere else. */
    *a = bits_of(fmod(7.0, 4.0));
    *b = bits_of(fmod(9.0, 4.0));
}

static void probe_pow(uint64_t *a, uint64_t *b) {
    *a = bits_of(pow(2.0, 3.0));
    *b = bits_of(pow(2.0, 5.0));
}

static void probe_sin(uint64_t *a, uint64_t *b) {
    *a = bits_of(sin(0.0));
    *b = bits_of(sin(1.0));
}

static void probe_cos(uint64_t *a, uint64_t *b) {
    *a = bits_of(cos(0.0));
    *b = bits_of(cos(1.0));
}


/* ---- the surface promoted in D051 -------------------------------------------
 *
 * Each of these gained a value check in 035-libc or 037-math. A value check alone
 * cannot say whether a wrong answer means "implemented incorrectly" or "not
 * implemented", and those need opposite work from whoever reads the report. */

static void probe_round(uint64_t *a, uint64_t *b) {
    *a = bits_of(round(2.5));
    *b = bits_of(round(3.5));
}

static void probe_trunc(uint64_t *a, uint64_t *b) {
    *a = bits_of(trunc(2.7));
    *b = bits_of(trunc(9.7));
}

static void probe_exp(uint64_t *a, uint64_t *b) {
    *a = bits_of(exp(0.0));
    *b = bits_of(exp(1.0));
}

static void probe_log(uint64_t *a, uint64_t *b) {
    *a = bits_of(log(1.0));
    *b = bits_of(log(2.0));
}

static void probe_log2(uint64_t *a, uint64_t *b) {
    *a = bits_of(log2(8.0));
    *b = bits_of(log2(4.0));
}

static void probe_log10(uint64_t *a, uint64_t *b) {
    *a = bits_of(log10(1000.0));
    *b = bits_of(log10(100.0));
}

static void probe_tan(uint64_t *a, uint64_t *b) {
    *a = bits_of(tan(1.0));
    *b = bits_of(tan(2.0));
}

static void probe_asin(uint64_t *a, uint64_t *b) {
    *a = bits_of(asin(0.5));
    *b = bits_of(asin(0.25));
}

static void probe_acos(uint64_t *a, uint64_t *b) {
    *a = bits_of(acos(0.5));
    *b = bits_of(acos(0.25));
}

static void probe_atan(uint64_t *a, uint64_t *b) {
    *a = bits_of(atan(1.0));
    *b = bits_of(atan(2.0));
}

static void probe_atan2(uint64_t *a, uint64_t *b) {
    /* Swapped rather than merely different: an implementation that ignores one argument
     * answers the same for both, which is the specific way this one goes wrong. */
    *a = bits_of(atan2(1.0, 2.0));
    *b = bits_of(atan2(2.0, 1.0));
}

static void probe_floorf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(floorf(2.7f));
    *b = bits_of_f(floorf(9.7f));
}

static void probe_ceilf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(ceilf(2.1f));
    *b = bits_of_f(ceilf(9.1f));
}

static void probe_fmodf(uint64_t *a, uint64_t *b) {
    /* 9 and 7 against 4, not 7 and 11: those both give 3. See probe_fmod. */
    *a = bits_of_f(fmodf(9.0f, 4.0f));
    *b = bits_of_f(fmodf(7.0f, 4.0f));
}

static void probe_powf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(powf(2.0f, 3.0f));
    *b = bits_of_f(powf(2.0f, 5.0f));
}

static void probe_expf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(expf(0.0f));
    *b = bits_of_f(expf(1.0f));
}

static void probe_logf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(logf(1.0f));
    *b = bits_of_f(logf(2.0f));
}

static void probe_sinf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(sinf(0.0f));
    *b = bits_of_f(sinf(1.0f));
}

static void probe_cosf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(cosf(0.0f));
    *b = bits_of_f(cosf(1.0f));
}

static void probe_tanf(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(tanf(1.0f));
    *b = bits_of_f(tanf(2.0f));
}

static void probe_strtod(uint64_t *a, uint64_t *b) {
    *a = bits_of(strtod("2.5", 0));
    *b = bits_of(strtod("7.5", 0));
}

static void probe_strtof(uint64_t *a, uint64_t *b) {
    *a = bits_of_f(strtof("2.5", 0));
    *b = bits_of_f(strtof("7.5", 0));
}

static void probe_atoll(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)atoll("1");
    *b = (uint64_t)atoll("2");
}

static void probe_strtoull(uint64_t *a, uint64_t *b) {
    *a = strtoull("10", 0, 10);
    *b = strtoull("20", 0, 10);
}

static void probe_llabs(uint64_t *a, uint64_t *b) {
    *a = (uint64_t)llabs(-1LL);
    *b = (uint64_t)llabs(-2LL);
}

static void probe_strncasecmp(uint64_t *a, uint64_t *b) {
    /* Opposite orders, so the two answers have opposite signs. Equal here means the
     * comparison is not looking at its arguments. */
    *a = (uint64_t)(int64_t)strncasecmp("a", "b", 1);
    *b = (uint64_t)(int64_t)strncasecmp("b", "a", 1);
}

static void probe_sprintf(uint64_t *a, uint64_t *b) {
    char one[16];
    char two[16];
    *a = (uint64_t)(int64_t)sprintf(one, "%d", 1);
    *b = (uint64_t)(int64_t)sprintf(two, "%d", 22);
}

static const probe probes[] = {
    {"libSceLibcInternal", "strlen", (const void *)&strlen, probe_strlen},
    {"libSceLibcInternal", "strcmp", (const void *)&strcmp, probe_strcmp},
    {"libSceLibcInternal", "strspn", (const void *)&strspn, probe_strspn},
    {"libSceLibcInternal", "toupper", (const void *)&toupper, probe_toupper},
    {"libSceLibcInternal", "tolower", (const void *)&tolower, probe_tolower},
    {"libSceLibcInternal", "isdigit", (const void *)&isdigit, probe_isdigit},
    {"libSceLibcInternal", "isalpha", (const void *)&isalpha, probe_isalpha},
    {"libSceLibcInternal", "abs", (const void *)&abs, probe_abs},
    {"libSceLibcInternal", "labs", (const void *)&labs, probe_labs},
    {"libSceLibcInternal", "atoi", (const void *)&atoi, probe_atoi},
    {"libSceLibcInternal", "atol", (const void *)&atol, probe_atol},
    {"libSceLibcInternal", "wcslen", (const void *)&wcslen, probe_wcslen},
    {"libSceLibcInternal", "memcmp", (const void *)&memcmp, probe_memcmp},
    {"libSceLibcInternal", "strncmp", (const void *)&strncmp, probe_strncmp},
    {"libSceLibcInternal", "strchr", (const void *)&strchr, probe_strchr},
    {"libSceLibcInternal", "strstr", (const void *)&strstr, probe_strstr},
    {"libSceLibcInternal", "strcspn", (const void *)&strcspn, probe_strcspn},
    {"libSceLibcInternal", "strtol", (const void *)&strtol, probe_strtol},
    {"libSceLibcInternal", "strtoul", (const void *)&strtoul, probe_strtoul},
    {"libSceLibcInternal", "atoll", (const void *)&atoll, probe_atoll},
    {"libSceLibcInternal", "strtoull", (const void *)&strtoull, probe_strtoull},
    {"libSceLibcInternal", "llabs", (const void *)&llabs, probe_llabs},
    {"libSceLibcInternal", "strncasecmp", (const void *)&strncasecmp, probe_strncasecmp},
    {"libSceLibcInternal", "sprintf", (const void *)&sprintf, probe_sprintf},
};

/* The maths library, kept separate so its verdict is its own. An emulator that has
 * written the string functions and not the maths ones is a different state of affairs
 * from one that has done neither, and a single mixed count would hide that. */
static const probe math_probes[] = {
    {"libSceLibcInternal", "sqrt", (const void *)&sqrt, probe_sqrt},
    {"libSceLibcInternal", "fabs", (const void *)&fabs, probe_fabs},
    {"libSceLibcInternal", "floor", (const void *)&floor, probe_floor},
    {"libSceLibcInternal", "ceil", (const void *)&ceil, probe_ceil},
    {"libSceLibcInternal", "fmod", (const void *)&fmod, probe_fmod},
    {"libSceLibcInternal", "pow", (const void *)&pow, probe_pow},
    {"libSceLibcInternal", "sin", (const void *)&sin, probe_sin},
    {"libSceLibcInternal", "cos", (const void *)&cos, probe_cos},
    {"libSceLibcInternal", "round", (const void *)&round, probe_round},
    {"libSceLibcInternal", "trunc", (const void *)&trunc, probe_trunc},
    {"libSceLibcInternal", "exp", (const void *)&exp, probe_exp},
    {"libSceLibcInternal", "log", (const void *)&log, probe_log},
    {"libSceLibcInternal", "log2", (const void *)&log2, probe_log2},
    {"libSceLibcInternal", "log10", (const void *)&log10, probe_log10},
    {"libSceLibcInternal", "tan", (const void *)&tan, probe_tan},
    {"libSceLibcInternal", "asin", (const void *)&asin, probe_asin},
    {"libSceLibcInternal", "acos", (const void *)&acos, probe_acos},
    {"libSceLibcInternal", "atan", (const void *)&atan, probe_atan},
    {"libSceLibcInternal", "atan2", (const void *)&atan2, probe_atan2},
    {"libSceLibcInternal", "floorf", (const void *)&floorf, probe_floorf},
    {"libSceLibcInternal", "ceilf", (const void *)&ceilf, probe_ceilf},
    {"libSceLibcInternal", "fmodf", (const void *)&fmodf, probe_fmodf},
    {"libSceLibcInternal", "powf", (const void *)&powf, probe_powf},
    {"libSceLibcInternal", "expf", (const void *)&expf, probe_expf},
    {"libSceLibcInternal", "logf", (const void *)&logf, probe_logf},
    {"libSceLibcInternal", "sinf", (const void *)&sinf, probe_sinf},
    {"libSceLibcInternal", "cosf", (const void *)&cosf, probe_cosf},
    {"libSceLibcInternal", "tanf", (const void *)&tanf, probe_tanf},
    {"libSceLibcInternal", "strtod", (const void *)&strtod, probe_strtod},
    {"libSceLibcInternal", "strtof", (const void *)&strtof, probe_strtof},
};

/* Totals across every probe in this section, for 900-surface to read.
 *
 * The census cannot tell an implemented platform from one that resolves everything to a
 * stub - presence is not behaviour, and a platform stubbing every symbol scores full
 * marks on a census (BACKLOG §7). This section is the only thing in the program that
 * knows the difference, so it publishes what it found and the census section states the
 * gap where somebody reading a coverage number will see it. */
unsigned int obs_responsive_responding = 0;
unsigned int obs_responsive_silent = 0;

static obs_result walk(const probe *table, unsigned int count) {
    unsigned int responding = 0;
    unsigned int silent = 0;
    unsigned int absent = 0;

    for (unsigned int i = 0; i < count; i++) {
        const probe *p = &table[i];
        if (!obs_address_is_callable(p->address)) {
            absent++;
            obs_report_responsive(p->library, p->symbol, "absent", 0);
            continue;
        }
        uint64_t first = 0;
        uint64_t second = 0;
        p->run(&first, &second);
        if (first == second) {
            /* Two inputs with necessarily different answers produced the same one. The
             * function is not reading its arguments. */
            silent++;
            obs_report_responsive(p->library, p->symbol, "silent", first);
        } else {
            responding++;
            obs_report_responsive(p->library, p->symbol, "responds", second);
        }
    }

    /* Accumulated across both verdicts, since a caller wants the platform's behaviour
     * rather than one library's. */
    obs_responsive_responding += responding;
    obs_responsive_silent += silent;

    if (responding == 0 && silent > 0) {
        return obs_fail_code("nothing responds; this library is stubbed rather than "
                             "implemented",
                             (uint64_t)silent);
    }
    if (silent > 0) {
        return obs_partial_value("some of the library is stubbed rather than implemented",
                                 (uint64_t)silent);
    }
    if (responding == 0) {
        return obs_skip("none of these symbols is present to probe");
    }
    if (absent > 0) {
        /* Everything present responds, and some were not there to ask. Worth saying:
         * "all of what exists works" and "all of it exists and works" are different
         * claims, and only the report can tell them apart. */
        return obs_partial_value("everything present responds; some were absent",
                                 (uint64_t)absent);
    }
    return obs_pass_value((uint64_t)responding);
}

static obs_result check_libc_responsiveness(void) {
    return walk(probes, OBS_COUNT(probes));
}

static obs_result check_math_responsiveness(void) {
    return walk(math_probes, OBS_COUNT(math_probes));
}

static const obs_check responsive_checks[] = {
    {"007-responsive/libc", "libSceLibcInternal", "strlen", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strlen, check_libc_responsiveness, OBS_FROM_SPEC},
    {"007-responsive/math", "libSceLibcInternal", "sqrt", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sqrt, check_math_responsiveness, OBS_FROM_SPEC},
};

const obs_section obs_section_responsive = {
    "007-responsive",
    "Stub, or implemented",
    "Whether a function reads its arguments at all, tested by giving it two whose "
    "answers must differ. Read before the failures below: a function that answers the "
    "same thing to everything is unimplemented, not incorrect, and the two need "
    "different work.",
    responsive_checks,
    OBS_COUNT(responsive_checks),
};
