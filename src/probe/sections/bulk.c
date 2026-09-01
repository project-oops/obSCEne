/*
 * The blind prober: every censused symbol, called with nothing.
 *
 * # What this is for
 *
 * The goal for this program is to touch every interface the platform offers. The
 * behavioural sections cost a confident signature each, which is the right price for a
 * check that makes a claim - and at that price the suite will never cover four hundred
 * functions, let alone the thousands a console actually exposes.
 *
 * The census solved the same problem one level down by asking a cheaper question: not
 * "does this work?" but "is this here?", which costs only a name. This asks the
 * question in between, and it is the one nothing else in the suite can answer:
 *
 *   **Is there an implementation behind this symbol, or is the address a stub?**
 *
 * That distinction is the single largest hole in what obSCEne currently measures. An
 * emulator that resolves every import to a shared do-nothing stub scores a perfect
 * census
 * - 383 of 383 present - while implementing none of it. `docs/COMPATIBILITY.md` says
 * this outright and has had no instrument for it.
 *
 * # How a call with no arguments answers that
 *
 * Every function is called with zero in all six argument registers, and the only thing
 * recorded is what came back. Nothing is asserted, because nothing can be: this program
 * does not know what these functions take.
 *
 * What makes it informative is that **the three outcomes are produced by different
 * things**:
 *
 * | outcome | what produces it |
 * |---|---|
 * | a vendor error code | argument validation ran, so something real is behind the
 * symbol | | zero | success on null input, or a stub that returns zero - the two are
 * not separable | | did not return | the callee dereferenced an argument without
 * checking it, or blocked |
 *
 * The first is the useful one, and it is *unforgeable in the direction that matters*: a
 * generic stub cannot return `0x8002_0016` for `sceKernelOpen` and `0x8002_0009` for
 * `sceKernelClose` unless somebody wrote both. A census counts addresses; this counts
 * implementations.
 *
 * The third is a finding rather than an accident - "this function faults on a null
 * argument" is a real property, and on hardware it is one worth knowing before a title
 * finds it.
 *
 * # How this differs from 007-responsive
 *
 * That section answers the same question far better, for a hundredth as many symbols.
 * It calls a function twice with inputs whose answers must differ and compares the two
 * - so it proves the function reads its arguments, which is real evidence. It costs a
 * confident signature and a hand-written probe pair per symbol, which is why it
 * covers 54.
 *
 * This costs nothing per symbol and covers all of them, and its evidence is weaker in
 * exactly one direction: `zero` cannot distinguish a stub from a success. `rejected`
 * cannot be produced by a stub at all, so where the two sections overlap they should
 * agree, and where they disagree one of them is wrong - which is worth having.
 *
 * The division is deliberate: **`007-responsive` proves behaviour for the few, this
 * detects implementation across the many.** Neither replaces the other.
 *
 * # Why this does not violate D008
 *
 * D008 forbids calling a function whose arity or struct layout is uncertain, and every
 * function here is uncertain. The rule survives because of what it is actually about.
 *
 * **D008 governs expectations, not calls.** Its two named costs are a wrong arity
 * corrupting the stack, and a wrong constant making a call succeed and do the wrong
 * thing silently. Neither applies:
 *
 * *The stack cannot be corrupted by an arity mismatch here.* On this ABI the first six
 * integer arguments travel in registers and **the caller cleans up**, so handing six
 * registers to a function that reads two leaves the stack exactly as it found it. The
 * callee ignores what it does not read. This is the same property that lets `printf`
 * work.
 *
 * *Nothing can succeed and do the wrong thing silently,* because nothing here has an
 * expectation to be wrong about. `130-layout` and `140-oracle` route around D008 the
 * same way and for the same reason: a record is not a claim.
 *
 * What is genuinely given up is that **the return value is only meaningful for
 * functions returning an integer**. One returning a float leaves the answer in a vector
 * register and this reads the integer one, which holds whatever was there. One
 * returning a large struct by value is handed a null result pointer and will fault.
 * Both are recorded as what they are and neither is corrected, because correcting them
 * needs the signature this section exists to avoid needing.
 *
 * # The variadic prototype is deliberate
 *
 * A variadic call must set the vector-argument count register; a normal call leaves it
 * holding whatever was there. Calling through a variadic prototype makes the compiler
 * set it to zero, which is correct for a variadic callee and ignored by every other
 * kind - so it is strictly safer than the obvious prototype, at no cost.
 *
 * # Off unless asked for
 *
 * Compiled in only under `OBS_BULK`. It is slow, it is the one part of the program
 * expected to end the process, and the default suite has to stay something that runs to
 * completion. See `scripts/bulk-sweep.sh` for the harness that drives it.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/corpus.h"
#include "obscene/surface.h"

/* Where to start. The harness raises this past whatever ended the last run.
 *
 * A crash takes the process with it, so a single pass can only ever reach the first
 * function that faults. Recompiling with the index after it resumes immediately past
 * the crash instead of paying for the whole sweep again - the same shape as the
 * exclusion list in `sweep-build.sh`, and for the same reason. */
#ifndef OBS_BULK_START
#define OBS_BULK_START 0u
#endif

/* How many to attempt in one run, counted from the start index. Zero means all of them.
 * Useful for bisecting a fault that the announcement alone does not explain. */
#ifndef OBS_BULK_LIMIT
#define OBS_BULK_LIMIT 0u
#endif

/* Everything below is compiled only when asked for.
 *
 * The section itself always exists and always reports - one skip, with the reason - so
 * a default run says "this was not done" rather than saying nothing. A capability that
 * disappears from the report when it is off is one nobody remembers is there. */
#if defined(OBS_BULK)

/* Declared as data, exactly as the census declares them.
 *
 * This is the point in the program where that convention is deliberately stepped
 * around, and doing it by casting an address rather than by redeclaring the name keeps
 * the step local to one expression. Every other translation unit still gets a `const
 * char` and still cannot call these by accident - which is the whole value of the
 * convention, and would be lost by declaring them as functions here. */
#define OBS_DECLARE_TARGET(name) extern OBS_WEAK const char name;
#define OBS_TARGET_ROW(name) {#name, (const void *)&name},

/* The call, through a prototype that commits to nothing beyond the calling convention.
 */
typedef uint64_t (*bulk_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                            ...);

typedef struct bulk_target {
    const char *name;
    const void *address;
} bulk_target;

typedef struct bulk_group {
    const char *library;
    const bulk_target *targets;
    unsigned int count;
} bulk_group;

/* One table per library, generated from the same list the census walks.
 *
 * The library name sits on the group rather than on every row, which is what makes the
 * nested list expansion work at all - the inner macro is expanded during the outer one
 * and cannot see its parameters. The census reached the same arrangement for the same
 * reason, and matching it means adding a library stays one line in surface.h. */
#define OBS_DEFINE_BULK_GROUP(tag, library, availability, LIST)                        \
    LIST(OBS_DECLARE_TARGET)                                                           \
    static const bulk_target tag##_targets[] = {LIST(OBS_TARGET_ROW)};

OBS_SURFACE_LIBRARIES(OBS_DEFINE_BULK_GROUP)

/* And the mined census, which is where nearly all of the surface now is.
 *
 * This walked `surface.h` alone: 383 targets against a census of 39,555, so the one
 * section that actually *calls* anything was covering **one per cent** of what the
 * program knows about. The tables were already generated in the shape this needs;
 * nothing pointed at them.
 *
 * Expanding to the corpus is the difference between "we can name a lot of symbols"
 * and "we have asked a lot of symbols whether anything is behind them", which is the
 * whole reason this section exists.
 *
 * **The callable list, not the census list.** 5,048 of the corpus are `Object` or `TLS`
 * - data, not functions - and calling one jumps into a variable. The first run of this
 * expansion did exactly that and died at `libSceImageUtil|__dso_handle`. Counting a
 * data symbol is fine; calling it is not, so the generator emits two lists and this
 * takes the narrower one (D114). */
OBS_CORPUS_CALLABLE_LIBRARIES(OBS_DEFINE_BULK_GROUP)

#define OBS_BULK_GROUP_ROW(tag, library, availability, LIST)                           \
    {library, tag##_targets, OBS_COUNT(tag##_targets)},

static const bulk_group groups[] = {OBS_SURFACE_LIBRARIES(
    OBS_BULK_GROUP_ROW) OBS_CORPUS_CALLABLE_LIBRARIES(OBS_BULK_GROUP_ROW)};

static const char *classify(uint64_t returned) {
    if (returned == 0) {
        /* Indistinguishable from a stub returning zero, and said so rather than
         * guessed at. A function that genuinely succeeds on six null arguments and one
         * that does nothing at all produce the same record; nothing here can separate
         * them, and pretending otherwise would be the one lie this section could tell.
         */
        return "zero";
    }
    if ((returned & 0xFFFF0000ull) == 0x80020000ull) {
        return "rejected";
    }
    if ((returned >> 63) != 0 || (returned & 0x80000000ull) != 0) {
        /* Some other negative-looking answer: a different facility, or an errno
         * returned directly. Worth separating from an ordinary value without claiming
         * to know which. */
        return "error-shaped";
    }
    return "value";
}

static obs_result run_bulk(void) {
    /* `position`, not `index`.
     *
     * The corpus declares 35,000 names as globals, and one of them is `index` - the BSD
     * string function. A local of that name now shadows it, which `-Wshadow` refuses.
     *
     * Worth noticing rather than just renaming: a census this size occupies the global
     * namespace, so ordinary local names can collide with the platform's. The compiler
     * catches it, which is the argument for keeping the warning at error. */
    unsigned int position = 0;
    unsigned int called = 0;
    unsigned int absent = 0;
    unsigned int skipped = 0;

    /* How long the list is, announced before walking it.
     *
     * Every other record here carries a bare position, and a position with no
     * denominator beside it invites the reader to supply one - which they will do from
     * whatever count they last saw. Two different counts in this repository are both
     * called "the census": `surface.h` holds 371 curated symbols and gets printed by
     * `verify.sh`, and this section walks the callable corpus, which is roughly a
     * hundred times larger and was printed nowhere. A sweep that had covered 1.1% of
     * the corpus was read, recorded and reported as 95% of the census on exactly that
     * basis. (D163)
     *
     * Emitted from the group table rather than from a constant, so it cannot disagree
     * with what the loop below actually walks. */
    unsigned int total = 0;
    for (unsigned int g = 0; g < OBS_COUNT(groups); g++) {
        total += groups[g].count;
    }
    obs_report_measure("910-bulk/probe", "(every censused symbol)", "list-length",
                       (uint64_t)total, "symbols");

    /* One flat position across every group, so the resume point is a single number.
     *
     * It counts *every* symbol including the ones not resolved, rather than counting
     * only the ones called. Two runs of the same build must agree on what position 291
     * means, and a count that skips absences does not - it would shift the moment a
     * loader resolved one more symbol, which is exactly the situation where a resumed
     * sweep is being compared against the run it resumed from. */
    for (unsigned int g = 0; g < OBS_COUNT(groups); g++) {
        for (unsigned int t = 0; t < groups[g].count; t++, position++) {
            if (position < (unsigned int)OBS_BULK_START) {
                skipped++;
                continue;
            }
            if (OBS_BULK_LIMIT != 0u && called >= (unsigned int)OBS_BULK_LIMIT) {
                goto done;
            }

            /* Not `!= NULL`, for the reason the census gives: a loader resolving
             * unknown symbols to a small non-null value would have every one of them
             * called. */
            if (!obs_address_is_callable(groups[g].targets[t].address)) {
                absent++;
                continue;
            }

            /* Announced before the call, and this is the only reason the section is
             * survivable. A record with an position and no answer after it names the
             * exact function that ended the process - which is both the finding and the
             * input the harness needs to resume. Principle 1, at the one place in the
             * program where it is load-bearing rather than precautionary. */
            obs_report_progress("910-bulk/probe", (uint64_t)position);
            obs_report_call(groups[g].library, groups[g].targets[t].name,
                            (uint64_t)position, "attempt", 0);

            bulk_fn fn = (bulk_fn)(uintptr_t)groups[g].targets[t].address;
            uint64_t returned = fn(0, 0, 0, 0, 0, 0);

            obs_report_call(groups[g].library, groups[g].targets[t].name,
                            (uint64_t)position, classify(returned), returned);
            called++;
        }
    }

done:
    if (called == 0) {
        if (skipped != 0 && absent == 0) {
            return obs_skip("the start position is past everything resolvable; "
                            "the sweep is done");
        }
        return obs_fail_code("nothing in the list resolved, so nothing could be probed",
                             (uint64_t)absent);
    }
    /* A count, not a verdict. Every call returning is the expected outcome and says
     * nothing on its own - the records are the result, and reading them is the tool's
     * job rather than this function's. */
    return obs_pass_value((uint64_t)called);
}

#else /* !OBS_BULK */

static obs_result run_bulk(void) {
    return obs_skip("built without OBS_BULK; see scripts/bulk-sweep.sh");
}

#endif

static const obs_check checks[] = {
    {"910-bulk/probe", "obscene", "(every censused symbol)", OBS_CAP_NONE, OBS_CAP_NONE,
     /* Its own address: the check is always callable, and what it probes is guarded
      * per-symbol inside the loop rather than by the harness. */
     (const void *)&run_bulk, run_bulk, OBS_FROM_DERIVED},
};

const obs_section obs_section_bulk = {
    "910-bulk",
    "Blind probe of the whole surface",
    "Calls every censused symbol with nothing and records what came back, to separate "
    "a resolved address from an implementation behind it.",
    checks,
    OBS_COUNT(checks),
};
