/*
 * Base layer: the report stream, then the process it runs in.
 *
 * Nothing above this section means anything if these fail. If the number formatter
 * is wrong, every value in the report is wrong; if the write path is wrong, there is
 * no report at all.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* ---- 000-boot -------------------------------------------------------------- */

/* Compares against a literal without libc. */
static int same_text(const char *a, const char *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static obs_result check_formatting(void) {
    /* Self-check before trusting any number this program prints. A formatter that
     * reverses its digits produces a report that looks entirely plausible and is
     * quietly useless, and no platform failure would ever reveal it. */
    char buf[OBS_NUM_MAX];
    size_t n = obs_format_u64(buf, 1234567890u);
    if (n != 10 || !same_text(buf, "1234567890", 10)) {
        return obs_fail("decimal formatting is wrong");
    }
    n = obs_format_hex(buf, 0xdeadbeefu);
    if (n != 10 || !same_text(buf, "0xdeadbeef", 10)) {
        return obs_fail("hexadecimal formatting is wrong");
    }
    n = obs_format_i64(buf, -42);
    if (n != 3 || !same_text(buf, "-42", 3)) {
        return obs_fail("signed formatting is wrong");
    }
    /* The most negative value has no positive counterpart; negating it in signed
     * arithmetic is undefined rather than merely wrong, so it is worth pinning. */
    n = obs_format_i64(buf, (-9223372036854775807LL - 1));
    if (n != 20 || !same_text(buf, "-9223372036854775808", 20)) {
        return obs_fail("the most negative value does not format");
    }
    return obs_pass();
}

static obs_result check_write_returns_count(void) {
    /* A write must report how many bytes it took. Returning zero, or the buffer
     * length regardless of what happened, are both common stub shortcuts and both
     * produce a report stream that silently truncates under load. */
    static const char probe[] = "";
    sce_ssize_t n = sceKernelWrite(OBS_FD_STDOUT, probe, 0);
    if (n < 0) {
        return obs_fail_code("a zero-length write reported an error", (uint64_t)n);
    }
    if (n != 0) {
        return obs_partial_value("a zero-length write claimed to write bytes",
                                 (uint64_t)n);
    }
    return obs_pass();
}

static obs_result check_write_rejects_bad_descriptor(void) {
    /* The first negative check, and the shape most of this program uses: an
     * implementation that returns success for an obviously invalid argument has not
     * implemented the function, it has implemented a constant. */
    static const char probe[] = "x";
    sce_ssize_t n = sceKernelWrite(OBS_FD_INVALID, probe, 1);
    if (n >= 0) {
        return obs_partial_value("writing to an invalid descriptor reported success",
                                 (uint64_t)n);
    }
    return obs_pass_value((uint64_t)n);
}

/* Reads the stack pointer as the caller of this function saw it.
 *
 * `__builtin_frame_address(0)` is the frame base of this function. On x86-64 SysV that
 * is a known distance from the stack pointer at entry, but the distance depends on what
 * the compiler did with the frame - so the value is not used absolutely, only for its
 * alignment, which the ABI fixes regardless. */
static uintptr_t obs_frame_alignment(void) {
    return (uintptr_t)__builtin_frame_address(0) & 0x0Fu;
}

static obs_result check_stack_alignment(void) {
    /* # Why this is worth a check of its own
     *
     * x86-64 SysV requires the stack to be sixteen-byte aligned at a `call`, which
     * leaves `rsp % 16 == 8` on entry to the callee once the return address is pushed.
     * Compiled code assumes it, and uses the assumption to place aligned spills - so a
     * violation does not fail immediately. It fails the first time something spills a
     * vector register, which may be in a completely different function, long
     * afterwards, and only once the implementation is large enough to want one.
     *
     * That makes it invisible until it is expensive. The orbistoun side found exactly
     * this shape: 370 of 372 guest calls arriving misaligned for the entire life of the
     * project, undetected until an implementation grew big enough to hit it.
     *
     * **No loader passes this by accident**, and a loader that jumps to an entry point
     * rather than calling one gets it wrong without any other symptom.
     *
     * # What it can and cannot see
     *
     * It reads its own frame, so it measures the alignment this program is running with
     * - which is what a platform function called from here will inherit. It cannot see
     * how a *callback* is entered, so a loader that gets ordinary calls right and
     * callbacks wrong would pass. Stated rather than implied. */
    uintptr_t offset = obs_frame_alignment();

    /* A frame base is sixteen-byte aligned in a correctly entered function, whatever
     * the compiler chose to do above it. Anything else means the stack was already
     * skewed before this program started running. */
    if (offset != 0) {
        return obs_fail_code("the stack is not sixteen-byte aligned", (uint64_t)offset);
    }
    return obs_pass();
}

static const obs_check boot_checks[] = {
    {"000-boot/number-formatting", "obscene", "obs_format_u64", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_formatting, OBS_FROM_ASSUMED},
    {"000-boot/write-returns-count", "libkernel", "sceKernelWrite", OBS_CAP_NONE,
     OBS_CAP_OUTPUT, (const void *)&sceKernelWrite, check_write_returns_count,
     OBS_FROM_DERIVED},
    {"000-boot/write-rejects-bad-fd", "libkernel", "sceKernelWrite", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelWrite, check_write_rejects_bad_descriptor,
     OBS_FROM_DERIVED},
    {"000-boot/stack-alignment", "obscene", "(self-check)", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_stack_alignment, OBS_FROM_SPEC},
};

const obs_section obs_section_boot = {
    "000-boot",
    "Boot and report stream",
    "Establishes that the report itself can be trusted before anything is reported.",
    boot_checks,
    OBS_COUNT(boot_checks),
};

/* ---- 010-kernel ------------------------------------------------------------ */

static obs_result check_process_time(void) {
    uint64_t first = sceKernelGetProcessTime();
    /* Burn a little time without sleeping, so this stays independent of the timer
     * section running later. */
    volatile uint64_t spin = 0;
    for (uint64_t i = 0; i < 200000u; i++) {
        spin += i;
    }
    uint64_t second = sceKernelGetProcessTime();
    if (second < first) {
        return obs_fail_code("process time went backwards", second);
    }
    if (second == first) {
        /* A constant clock is the classic stub, and it deadlocks anything that waits
         * on elapsed time. Amber rather than red: a coarse real clock can legitimately
         * not have ticked. */
        return obs_partial_value("process time did not advance", second);
    }
    return obs_pass_value(second - first);
}

static obs_result check_process_time_counter(void) {
    uint64_t first = sceKernelGetProcessTimeCounter();
    uint64_t second = sceKernelGetProcessTimeCounter();
    if (second < first) {
        return obs_fail_code("the counter went backwards", second);
    }
    if (first == 0 && second == 0) {
        return obs_partial("the counter reads zero");
    }
    return obs_pass_value(second);
}

static obs_result check_tsc_frequency(void) {
    uint64_t hz = sceKernelGetTscFrequency();
    if (hz == 0) {
        /* Everything that converts counter ticks to seconds divides by this. Zero is
         * not a slow clock, it is a fault waiting to happen somewhere unrelated. */
        return obs_fail("the counter frequency is zero");
    }
    /* Anything below a megahertz or above a terahertz is not a plausible timestamp
     * counter, and a wrong frequency makes every derived duration wrong by a factor
     * nobody will think to question. */
    if (hz < 1000000u || hz > 1000000000000u) {
        return obs_partial_value("the counter frequency is implausible", hz);
    }
    return obs_pass_value(hz);
}

static const obs_check kernel_checks[] = {
    {"010-kernel/process-time", "libkernel", "sceKernelGetProcessTime", OBS_CAP_NONE,
     OBS_CAP_TIME, (const void *)&sceKernelGetProcessTime, check_process_time,
     OBS_FROM_ASSUMED},
    {"010-kernel/process-time-counter", "libkernel", "sceKernelGetProcessTimeCounter",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelGetProcessTimeCounter,
     check_process_time_counter, OBS_FROM_ASSUMED},
    {"010-kernel/tsc-frequency", "libkernel", "sceKernelGetTscFrequency", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelGetTscFrequency, check_tsc_frequency,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_kernel = {
    "010-kernel",
    "Kernel core",
    "Process identity and the clocks everything else measures itself against.",
    kernel_checks,
    OBS_COUNT(kernel_checks),
};
