/*
 * Numbers the platform produced, recorded rather than only judged.
 *
 * # Why measuring is not the same as checking
 *
 * Every other section asks a yes-or-no question, and answering one needs an
 * expectation. Where a document supplies it that is `spec`; where nobody does, this
 * project supplies it and marks it `assumed` - sensible, unconfirmed, and correctable.
 *
 * There is a third thing available, and until now this program did not do it: **record
 * what happened and assert nothing about it**. A duration is a fact. It needs no
 * expectation to be worth having, and it does not go stale when a guess turns out
 * wrong.
 *
 * That matters most for the questions where an expectation is hardest. Nobody documents
 * how long a one-millisecond sleep takes. A threshold here would be this project
 * inventing a specification, and the check would then pass or fail on the threshold
 * rather than on the platform.
 *
 * # The measurement is the calibration
 *
 * Fifty-odd checks in this suite are `assumed` and none is `hardware`. Correcting one
 * today means re-deriving what it should have said. Correcting it from a measurement
 * means reading the number a console produced and editing a constant.
 *
 * So the assertions here are deliberately loose and marked `assumed`, and the numbers
 * are recorded beside them. When the numbers arrive from real hardware, the assertions
 * tighten to match and become `hardware` - and the recorded values from every emulator
 * become a fidelity ranking against a known answer, which no yes-or-no result can give.
 *
 * # Which clock is which, established rather than assumed
 *
 * The platform offers several time sources and does not say what they count. Process
 * time counts CPU consumed, so a sleeping thread does not accrue it; a wall clock
 * advances regardless. Guessing wrong makes every duration here wrong.
 *
 * It does not need guessing. Sleep for a known interval and read them all: the sources
 * that advanced by roughly the sleep are wall clocks, the ones that barely moved are
 * counting CPU. `120-measure/identify-clocks` does exactly that, and it is the closest
 * thing in this program to an experiment.
 *
 * # What a single run is worth
 *
 * Less than a comparison. "The sleep took 1043 microseconds" says little on its own;
 * the same figure from three emulators and a console says which are faithful. These
 * records are built to be diffed across platforms, which is why they carry their units
 * and the value that was asked for alongside the one observed.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Every time source that can be read without a struct. `sceKernelClockGettime` wants a
 * `timespec` and stays in the census for it (D008). */
typedef struct {
    const char *name;
    uint64_t (*read)(void);
} clock_source;

static uint64_t read_process_time(void) {
    return sceKernelGetProcessTime();
}

static uint64_t read_process_counter(void) {
    return sceKernelGetProcessTimeCounter();
}

static uint64_t read_tsc(void) {
    return sceKernelReadTsc();
}

static const clock_source clocks[] = {
    {"sceKernelGetProcessTime", read_process_time},
    {"sceKernelGetProcessTimeCounter", read_process_counter},
    {"sceKernelReadTsc", read_tsc},
};

/* Whether a source can be read at all. Every platform declaration is weak, so a symbol
 * the loader could not resolve is a null address - and this section calls several
 * symbols that are not its own table row, which is exactly the case D058 was written
 * about. */
static int clock_present(const clock_source *source) {
    if (source->read == read_process_time) {
        return (const void *)&sceKernelGetProcessTime != 0;
    }
    if (source->read == read_process_counter) {
        return (const void *)&sceKernelGetProcessTimeCounter != 0;
    }
    return (const void *)&sceKernelReadTsc != 0;
}

static obs_result check_frequencies(void) {
    /* The divisor that turns a counter into a duration. Recorded rather than checked
     * against a value: the figure differs between console generations and asserting one
     * would be inventing a specification.
     *
     * It is also the first number worth comparing across emulators. One reporting a
     * frequency that does not match the counter it hands out has a self-consistency
     * problem no single reading can show. */
    int found = 0;
    if ((const void *)&sceKernelGetTscFrequency != 0) {
        uint64_t hz = sceKernelGetTscFrequency();
        obs_report_measure("120-measure/frequencies", "sceKernelGetTscFrequency",
                           "frequency", hz, "hz");
        if (hz != 0) {
            found++;
        }
    }
    if ((const void *)&sceKernelGetProcessTimeCounterFrequency != 0) {
        uint64_t hz = sceKernelGetProcessTimeCounterFrequency();
        obs_report_measure("120-measure/frequencies",
                           "sceKernelGetProcessTimeCounterFrequency", "frequency", hz,
                           "hz");
        if (hz != 0) {
            found++;
        }
    }
    if (found == 0) {
        return obs_fail("no counter frequency is available, so ticks cannot be timed");
    }
    return obs_pass_value((uint64_t)found);
}

static obs_result check_clocks_advance(void) {
    /* Do they move at all. A stub returning a constant is the common failure and it is
     * invisible in a single reading.
     *
     * Busy work rather than a sleep, so this stays independent of whether sleeping
     * works - which is measured separately and might not. */
    int advancing = 0;
    int present = 0;
    for (size_t i = 0; i < OBS_COUNT(clocks); i++) {
        if (!clock_present(&clocks[i])) {
            continue;
        }
        present++;
        uint64_t before = clocks[i].read();
        /* The absolute reading, not just the delta. A delta says the clock advances but
         * is blind to where zero is: a small absolute value means it counts from
         * process start, a huge one means an epoch, and a title comparing this against
         * a stored timestamp behaves completely differently under the two. This is
         * early in the run, so "small" is unambiguous. */
        obs_report_measure("120-measure/clocks-advance", clocks[i].name, "absolute",
                           before, "ticks");
        volatile uint64_t spin = 0;
        for (uint64_t j = 0; j < 2000000u; j++) {
            spin = spin + j;
        }
        uint64_t after = clocks[i].read();
        /* Unsigned subtraction, so a counter that wrapped reads as a large delta rather
         * than as going backwards. A backwards clock is worth seeing, so it is reported
         * as its own quantity rather than folded into the delta. */
        obs_report_measure("120-measure/clocks-advance", clocks[i].name, "delta-busy",
                           after - before, "ticks");
        if (after > before) {
            advancing++;
        } else if (after < before) {
            obs_report_measure("120-measure/clocks-advance", clocks[i].name,
                               "went-backwards", before - after, "ticks");
        }
    }
    if (present == 0) {
        return obs_skip("no time source is present");
    }
    if (advancing == 0) {
        return obs_fail("no time source advanced across measurable work");
    }
    if (advancing < present) {
        return obs_partial_value("some time sources are constant", (uint64_t)advancing);
    }
    return obs_pass_value((uint64_t)advancing);
}

static obs_result check_identify_clocks(void) {
    /* The experiment.
     *
     * Sleep a known interval and read every source across it. One counting wall time
     * advances by roughly the sleep; one counting CPU time barely moves, because the
     * thread was not running. That identifies each source from its own behaviour rather
     * than from an assumption about its name.
     *
     * Nothing is asserted about which is which - the platform is entitled to whatever
     * answer it has. What is recorded is each delta beside the sleep that was asked
     * for, which makes the classification obvious to a reader and comparable across
     * platforms. */
    if ((const void *)&sceKernelUsleep == 0) {
        return obs_skip("nothing to sleep with, so the sources cannot be told apart");
    }

    const unsigned int requested = 20000u; /* 20ms: long enough to dwarf the noise. */
    uint64_t before[OBS_COUNT(clocks)];
    uint64_t after[OBS_COUNT(clocks)];
    int present[OBS_COUNT(clocks)];

    for (size_t i = 0; i < OBS_COUNT(clocks); i++) {
        present[i] = clock_present(&clocks[i]);
        before[i] = present[i] ? clocks[i].read() : 0;
    }
    int slept = sceKernelUsleep(requested);
    for (size_t i = 0; i < OBS_COUNT(clocks); i++) {
        after[i] = present[i] ? clocks[i].read() : 0;
    }

    obs_report_measure("120-measure/identify-clocks", "sceKernelUsleep", "requested",
                       requested, "us");
    int any = 0;
    for (size_t i = 0; i < OBS_COUNT(clocks); i++) {
        if (!present[i]) {
            continue;
        }
        any++;
        obs_report_measure("120-measure/identify-clocks", clocks[i].name,
                           "delta-across-sleep", after[i] - before[i], "ticks");
    }
    if (slept != 0) {
        return obs_fail("the sleep used to identify the clocks was refused");
    }
    if (any == 0) {
        return obs_skip("no time source is present");
    }
    return obs_pass_value((uint64_t)any);
}

static obs_result check_sleep_fidelity(void) {
    /* How long a sleep actually takes, in microseconds, for three requested durations.
     *
     * # The assertion here is a guess, and says so
     *
     * A sleep must take *some* time and must not take absurdly longer than asked. Both
     * bounds are this project's, not a document's, which is why this check is
     * OBS_FROM_ASSUMED - and why the numbers are recorded beside the verdict. When a
     * console supplies real figures the bounds tighten to match and the provenance
     * becomes OBS_FROM_HARDWARE.
     *
     * The lower bound is what a stub fails: `usleep` returns zero for any valid
     * request, so a stub passes a check that only reads the return value. It cannot
     * pass one that watches the clock. That is the gap 017-posix/short-sleep documents
     * and cannot close, and this is where it closes. */
    if ((const void *)&sceKernelUsleep == 0 || (const void *)&sceKernelReadTsc == 0 ||
        (const void *)&sceKernelGetTscFrequency == 0) {
        return obs_skip("no clock and sleep pair is present to measure with");
    }
    uint64_t hz = sceKernelGetTscFrequency();
    if (hz == 0) {
        return obs_skip("the counter frequency is zero, so ticks cannot be converted");
    }

    static const unsigned int durations[] = {1000u, 5000u, 20000u};
    int too_short = 0;
    int wildly_long = 0;
    int returned_early = 0;
    for (size_t i = 0; i < OBS_COUNT(durations); i++) {
        uint64_t before = sceKernelReadTsc();
        (void)sceKernelUsleep(durations[i]);
        uint64_t elapsed = sceKernelReadTsc() - before;
        /* Multiply before dividing, or a short interval rounds to zero. Twenty
         * milliseconds at a few gigahertz is around 1e8 ticks, so 1e8 * 1e6 is 1e14 and
         * nowhere near overflowing sixty-four bits. */
        uint64_t microseconds = (elapsed * 1000000u) / hz;
        obs_report_measure("120-measure/sleep-fidelity", "sceKernelUsleep", "requested",
                           durations[i], "us");
        obs_report_measure("120-measure/sleep-fidelity", "sceKernelUsleep", "observed",
                           microseconds, "us");

        /* Half the request. Loose on purpose: scheduling granularity is real and a
         * console is entitled to round. What this catches is a sleep that did not
         * happen at all. */
        if (microseconds < durations[i] / 2u) {
            too_short++;
        }
        /* Returning even slightly early is its own signal, and the first version of
         * this check could not see it: shadPS4 answered a twenty-millisecond request in
         * 19,757 microseconds, which is 243 short and nowhere near the half-request
         * floor above.
         *
         * A sleep is a lower bound. Code that sleeps before reading a register expects
         * the interval to have passed, and an early return is exactly the kind of fault
         * that produces a rare, unreproducible failure much later.
         *
         * Reported as partial rather than as a failure, and separately from the
         * measurement above, because the size of the shortfall matters and 243
         * microseconds is not the same fault as returning immediately. The threshold is
         * this project's - a console may well do the same, and if it does this becomes
         * the documented behaviour rather than a complaint. */
        if (microseconds < durations[i]) {
            returned_early++;
            obs_report_measure("120-measure/sleep-fidelity", "sceKernelUsleep",
                               "short-by", (uint64_t)durations[i] - microseconds, "us");
        }
        /* Ten times the request, plus a millisecond so the shortest duration is not
         * judged on scheduling noise alone. */
        if (microseconds > (uint64_t)durations[i] * 10u + 1000u) {
            wildly_long++;
        }
    }
    if (too_short > 0) {
        return obs_fail_code("a sleep returned far sooner than asked",
                             (uint64_t)too_short);
    }
    if (wildly_long > 0) {
        return obs_partial_value("a sleep took much longer than asked",
                                 (uint64_t)wildly_long);
    }
    if (returned_early > 0) {
        return obs_partial_value("a sleep returned before the interval had passed",
                                 (uint64_t)returned_early);
    }
    return obs_pass();
}

static inline void obs_cpuid_leaf(uint32_t leaf, uint32_t subleaf, uint32_t *eax,
                                  uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ __volatile__("cpuid"
                         : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                         : "a"(leaf), "c"(subleaf));
}

static obs_result check_cpuid_topology(void) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

    /* Leaf 0: Vendor String */
    obs_cpuid_leaf(0, 0, &eax, &ebx, &ecx, &edx);
    uint32_t vendor[4] = {ebx, edx, ecx, 0};
    obs_report_measure("120-measure/cpuid", "cpuid", "max_basic_leaf", (uint64_t)eax,
                       "leaf");
    obs_report_bytes("120-measure/cpuid", "cpuid", "vendor_string", 0,
                     (const unsigned char *)vendor, 12);

    /* Leaf 1: Family, Model, Stepping, Features */
    obs_cpuid_leaf(1, 0, &eax, &ebx, &ecx, &edx);
    obs_report_measure("120-measure/cpuid", "cpuid", "signature_eax", (uint64_t)eax,
                       "raw");
    obs_report_measure("120-measure/cpuid", "cpuid", "feature_ecx", (uint64_t)ecx,
                       "flags");
    obs_report_measure("120-measure/cpuid", "cpuid", "feature_edx", (uint64_t)edx,
                       "flags");
    obs_report_measure("120-measure/cpuid", "cpuid", "stepping", (uint64_t)(eax & 0xFu),
                       "val");
    obs_report_measure("120-measure/cpuid", "cpuid", "model",
                       (uint64_t)((eax >> 4) & 0xFu), "val");
    obs_report_measure("120-measure/cpuid", "cpuid", "family",
                       (uint64_t)((eax >> 8) & 0xFu), "val");

    /* Leaf 0x80000001: Extended AMD Features */
    obs_cpuid_leaf(0x80000001u, 0, &eax, &ebx, &ecx, &edx);
    obs_report_measure("120-measure/cpuid", "cpuid_ext", "feature_ecx", (uint64_t)ecx,
                       "flags");
    obs_report_measure("120-measure/cpuid", "cpuid_ext", "feature_edx", (uint64_t)edx,
                       "flags");

    /* Leaf 0x8000001D: AMD Zen 2 Cache Topology */
    for (uint32_t subleaf = 0; subleaf < 4; subleaf++) {
        obs_cpuid_leaf(0x8000001Du, subleaf, &eax, &ebx, &ecx, &edx);
        uint32_t cache_type = eax & 0x1Fu;
        if (cache_type == 0)
            break;
        uint32_t cache_level = (eax >> 5) & 0x7u;
        uint32_t line_size = (ebx & 0xFFFu) + 1u;
        uint32_t ways = ((ebx >> 22) & 0x3FFu) + 1u;
        uint32_t num_sets = ecx + 1u;
        uint64_t total_size = (uint64_t)line_size * ways * num_sets;

        obs_report_measure("120-measure/cache-topology", "cache", "subleaf",
                           (uint64_t)subleaf, "idx");
        obs_report_measure("120-measure/cache-topology", "cache", "level",
                           (uint64_t)cache_level, "level");
        obs_report_measure("120-measure/cache-topology", "cache", "type",
                           (uint64_t)cache_type, "type");
        obs_report_measure("120-measure/cache-topology", "cache", "line_size_bytes",
                           (uint64_t)line_size, "bytes");
        obs_report_measure("120-measure/cache-topology", "cache", "associativity_ways",
                           (uint64_t)ways, "ways");
        obs_report_measure("120-measure/cache-topology", "cache", "total_bytes",
                           total_size, "bytes");
    }

    /* RDTSCP IA32_TSC_AUX register */
    uint32_t aux = 0;
    uint32_t tsc_lo = 0, tsc_hi = 0;
    __asm__ __volatile__("rdtscp" : "=a"(tsc_lo), "=d"(tsc_hi), "=c"(aux));
    obs_report_measure("120-measure/cpuid", "rdtscp", "tsc_aux_raw", (uint64_t)aux,
                       "raw");
    obs_report_measure("120-measure/cpuid", "rdtscp", "core_id",
                       (uint64_t)(aux & 0xFFFu), "id");
    obs_report_measure("120-measure/cpuid", "rdtscp", "numa_node_id",
                       (uint64_t)((aux >> 12) & 0xFFu), "id");

    return obs_pass_value((uint64_t)aux);
}

static obs_result check_timer_ratio(void) {
    /* Three of the four symbols this check calls are not its table row, so each is
     * tested here rather than through `clock_present()`. The gate reads the body of the
     * check itself and cannot follow into a helper - and neither can a reader deciding
     * whether this check is safe on a platform missing one of them. `sceKernelUsleep`
     * was not covered by that helper at all. (D058) */
    if (!obs_address_is_callable((const void *)&sceKernelGetProcessTime) ||
        !obs_address_is_callable((const void *)&sceKernelGetProcessTimeCounter) ||
        !obs_address_is_callable((const void *)&sceKernelUsleep)) {
        return obs_skip("a clock or sleep entry point is not callable");
    }
    if (!clock_present(&clocks[0]) || !clock_present(&clocks[1]) ||
        !clock_present(&clocks[2])) {
        return obs_skip("required clock sources unavailable");
    }

    uint64_t tsc_0 = sceKernelReadTsc();
    uint64_t proc_0 = sceKernelGetProcessTime();
    uint64_t count_0 = sceKernelGetProcessTimeCounter();

    (void)sceKernelUsleep(10000); /* 10 ms */

    uint64_t tsc_1 = sceKernelReadTsc();
    uint64_t proc_1 = sceKernelGetProcessTime();
    uint64_t count_1 = sceKernelGetProcessTimeCounter();

    uint64_t dtsc = tsc_1 - tsc_0;
    uint64_t dproc = proc_1 - proc_0;
    uint64_t dcount = count_1 - count_0;

    obs_report_measure("120-measure/timer-ratio", "tsc_delta", "ticks", dtsc, "ticks");
    obs_report_measure("120-measure/timer-ratio", "process_time_delta", "us", dproc,
                       "us");
    obs_report_measure("120-measure/timer-ratio", "counter_delta", "ticks", dcount,
                       "ticks");

    if (dproc > 0) {
        uint64_t hz_estimate = (dtsc * 1000000u) / dproc;
        obs_report_measure("120-measure/timer-ratio", "tsc_hz_calibrated", "hz",
                           hz_estimate, "hz");
    }

    return obs_pass_value(dtsc);
}

static const obs_check measure_checks[] = {
    {"120-measure/frequencies", "libkernel", "sceKernelGetTscFrequency", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetTscFrequency, check_frequencies,
     OBS_FROM_ASSUMED},
    {"120-measure/clocks-advance", "libkernel", "sceKernelGetProcessTime", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetProcessTime, check_clocks_advance,
     OBS_FROM_ASSUMED},
    {"120-measure/identify-clocks", "libkernel", "sceKernelReadTsc", OBS_CAP_TIME,
     OBS_CAP_NONE, (const void *)&sceKernelReadTsc, check_identify_clocks,
     OBS_FROM_ASSUMED},
    {"120-measure/sleep-fidelity", "libkernel", "sceKernelUsleep", OBS_CAP_TIME,
     OBS_CAP_NONE, (const void *)&sceKernelUsleep, check_sleep_fidelity,
     OBS_FROM_ASSUMED},
    {"120-measure/cpuid-topology", "libkernel", "cpuid", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_cpuid_topology, check_cpuid_topology, OBS_FROM_ASSUMED},
    {"120-measure/timer-ratio", "libkernel", "sceKernelReadTsc", OBS_CAP_TIME,
     OBS_CAP_NONE, (const void *)check_timer_ratio, check_timer_ratio,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_measure = {
    "120-measure",
    "Measurements, not verdicts",
    "Numbers the platform produced, recorded so they can be compared across emulators "
    "and against a console. The assertions are loose guesses and marked assumed; the "
    "figures beside them are what makes correcting those guesses a one-line edit.",
    measure_checks,
    OBS_COUNT(measure_checks),
};
