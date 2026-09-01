/*
 * The check and section model.
 *
 * # Announce before attempting
 *
 * The harness prints a check's identity *before* calling into the platform, and
 * flushes it. On real hardware that costs a line of output; under an emulator it is
 * the difference between a useful report and none at all, because the normal outcome
 * of an unimplemented function is a hard crash that takes the process with it. The
 * last line printed then names the exact call that died.
 *
 * This is the single most important property of this program and everything else is
 * arranged around it.
 */

#ifndef OBSCENE_HARNESS_H
#define OBSCENE_HARNESS_H

#include "obscene/status.h"

/* A capability a check can require or provide.
 *
 * Bit flags rather than names so the dependency test is a mask compare with no
 * allocation and no string handling in a freestanding binary. */
typedef enum obs_capability {
    OBS_CAP_NONE = 0,
    OBS_CAP_OUTPUT = 1u << 0,
    OBS_CAP_MEMORY = 1u << 1,
    OBS_CAP_THREAD = 1u << 2,
    OBS_CAP_FILE = 1u << 3,
    OBS_CAP_TIME = 1u << 4,
    OBS_CAP_MODULE = 1u << 5,
    OBS_CAP_VIDEO = 1u << 6,
    OBS_CAP_AUDIO = 1u << 7,
    OBS_CAP_INPUT = 1u << 8,
    OBS_CAP_GPU = 1u << 9,
    /* The C runtime is usable. */
    OBS_CAP_LIBC = 1u << 10,
    /* The heap specifically - separated from OBS_CAP_LIBC because string handling
     * can work perfectly while allocation does not, and the checks that need memory
     * should not be skipped for the sake of a broken strlen. */
    OBS_CAP_HEAP = 1u << 11,
} obs_capability;

/* Marker for a check that exercises no platform symbol - the self-checks in the boot
 * section. Distinct from NULL, which means "the symbol is genuinely absent". */
extern const char obs_no_symbol_marker;
#define OBS_NO_SYMBOL ((const void *)&obs_no_symbol_marker)

/* Which console generation a symbol belongs to.
 *
 * Without this, "absent" means two incompatible things: the function does not exist on
 * this generation, which is expected and not a gap; or it exists and the platform has
 * not implemented it, which is work to do. Counting them together makes a coverage
 * figure meaningless - the same mistake the census control exists to prevent, one
 * level up. */
typedef enum obs_availability {
    /* Present on both generations. */
    OBS_SHARED = 0,
    /* Previous generation only. Absence on current hardware is correct. */
    OBS_PREVIOUS = 1,
    /* Current generation only. Absence on previous hardware is correct. */
    OBS_CURRENT = 2,
    /* Not established. Reported honestly rather than assumed shared - a wrong
     * assumption here silently reclassifies a real gap as an expected absence. */
    OBS_AVAILABILITY_UNKNOWN = 3,
} obs_availability;

/* Stable lowercase name for the report. */
const char *obs_availability_name(obs_availability availability);

/* Which console the probe believes it is running on.
 *
 * Detected rather than asked for: there is no portable call that answers honestly,
 * and an emulator would answer whatever it was configured to say. Inferred instead
 * from which generation-exclusive symbols resolve, which is a fact about the platform
 * actually present. */
typedef enum obs_generation {
    OBS_GENERATION_UNKNOWN = 0,
    OBS_GENERATION_PREVIOUS = 1,
    OBS_GENERATION_CURRENT = 2,
    /* Both resolve. Real hardware runs the previous generation through a
     * compatibility path, so this is possible and means absences cannot be
     * attributed to either. */
    OBS_GENERATION_BOTH = 3,
} obs_generation;

/* What 005-generation concluded. OBS_GENERATION_UNKNOWN before it has run. */
obs_generation obs_detected_generation(void);
const char *obs_generation_name(obs_generation generation);

/* The graphics driver(s) that resolve here, for the HUD's GPU field: "gnm", "agc", "gnm+agc",
 * or NULL when neither does. Deliberately NOT the generation verdict: `obs_detected_generation`
 * refuses to name a console when the current generation's driver cannot even be looked at (a
 * ps4_game is refused it, D255) - but the driver that IS resolving is a separate, honest fact,
 * and on a ps4_game it is exactly the one that verdict has to drop. `gnm` names a driver, not a
 * console: a current-generation console in compatibility mode exposes it too. (D272) */
const char *obs_gpu_drivers(void);

/* Where a check's expectation came from.
 *
 * # Why this is in the report
 *
 * A conformance suite is only worth as much as its expectations, and not all of them
 * are worth the same. "strlen returns the number of characters" is settled by a
 * standard anyone can read. "closing an invalid handle returns non-zero" is a
 * reasonable belief this program holds and nobody has confirmed.
 *
 * Both produce a FAIL, and until now the report presented them identically. An emulator
 * author reading one cannot act on it without knowing which kind it is: the first is
 * their bug, the second might be ours.
 *
 * # Most of this suite is currently ASSUMED, and that is the honest answer
 *
 * The ISO C and POSIX halves are settled. Almost everything vendor-specific is not: the
 * functions are documented, their exact error codes for invalid input generally are
 * not, so an expectation about them is an assumption however sensible.
 *
 * That is what makes a run on real hardware worth so much. It does not merely check the
 * suite - it *upgrades* it, turning ASSUMED into HARDWARE one check at a time, and only
 * then is a green run something an emulator can be held to. Nothing carries HARDWARE
 * yet, and that absence is the accurate picture of where this project stands.
 */
typedef enum obs_provenance {
    /* This program's own reasoning. Sensible, unconfirmed. */
    OBS_FROM_ASSUMED = 0,
    /* Two or more independent implementations were read, and they agree.
     *
     * # Why this rung had to exist
     *
     * `015-sync/event-flag-round-trip` asserted the opposite of what
     * `sceKernelClearEventFlag` does - the argument is a mask of what to **keep** - and it
     * carried `OBS_FROM_DOCUMENTED` while doing so. Two emulators failed it, they were
     * right, and the correction had nowhere honest to go (D166): `ASSUMED` says "this
     * project guessed", which throws away that somebody's working code says otherwise, and
     * `DOCUMENTED` claims a citation nobody here can produce, which is the mistake that
     * caused it.
     *
     * The sibling project raised the same shape on the measurement axis - a value for
     * "measured, but not on the target" - and it was declined on the argument that the
     * origin field carries that as data. There is no origin field on the documentation
     * axis, so the argument does not transfer, and declining twice would be a position
     * held for symmetry rather than for a reason.
     *
     * # Below SPEC, and that placement is the whole caveat
     *
     * Stronger than this project's own guess. Weaker than a document anyone can check,
     * because **implementations are not independent witnesses**: these projects read each
     * other's source, and `obscene-tool consensus` says so in its own output - "agreement
     * is evidence, not four witnesses". Two implementations sharing an ancestor agree about
     * their ancestor.
     *
     * # The line to hold
     *
     * At least two implementations that do not share a codebase, read directly, and named
     * in the check's comment. One implementation is not this - it is a single opinion, and
     * `ASSUMED` describes a single opinion accurately whoever holds it. */
    OBS_FROM_IMPLEMENTATIONS,
    /* ISO C or POSIX. Settled by a document anyone can check. */
    OBS_FROM_SPEC,
    /* Vendor interface documentation describes this behaviour specifically. */
    OBS_FROM_DOCUMENTED,
    /* The platform's kernel derives from a documented system, and that system's
     * specification settles this.
     *
     * # Why this is not SPEC
     *
     * `sceKernelClose` is not `close`. The name says it is a rename of a POSIX call and
     * the target kernel is FreeBSD-derived, so POSIX almost certainly settles what it
     * does - but "almost certainly" is an inference about the renaming, not a document
     * about this function. Calling it SPEC would claim a document that does not mention
     * the symbol.
     *
     * # And why it is not ASSUMED
     *
     * Because it is much better than a guess, and flattening the two loses the
     * distinction that matters when deciding what a console visit needs to answer. An
     * ASSUMED check is this project's reasoning and could be wrong in any direction. A
     * DERIVED one is wrong only if the vendor changed a behaviour while keeping the
     * name, which is a narrow and checkable claim.
     *
     * # The line to hold
     *
     * Only where the source specification settles the *specific case being checked*.
     * POSIX says `open` on a missing path fails, so that is DERIVED. POSIX says passing
     * a null path is undefined, so a check that a null path is rejected stays ASSUMED -
     * the analogue is just as exact and the specification declines to answer. */
    OBS_FROM_DERIVED,
    /* Observed on real hardware and recorded. The only kind an emulator can be held
     * to without argument. */
    OBS_FROM_HARDWARE
} obs_provenance;

/* Short name for the report, and for anything reading it. */
const char *obs_provenance_name(obs_provenance from);

typedef struct obs_check {
    /* Stable identifier, unique across the whole program. Used as the key when
     * diffing two runs, so it must never be renamed casually. */
    const char *id;
    /* The library the symbol is imported from, exactly as the ABI names it. */
    const char *library;
    /* The symbol, exactly as the ABI names it. Not prose - renaming it would stop
     * this testing anything. */
    const char *symbol;
    /* What must already work for this to mean anything. */
    unsigned int requires_caps;
    /* What a PASS proves works, unlocking dependents. */
    unsigned int provides_caps;
    /* The symbol's address, or NULL if the loader could not resolve it.
     *
     * Every platform declaration is weak, so an unresolved import is a null address
     * rather than a link failure. The harness skips a check whose address is null
     * instead of calling it - jumping to zero would take the process down and lose
     * every check after it, to establish something the address already said. */
    const void *address;
    obs_result (*run)(void);
    /* Where this check's expectation came from. Last so it reads as what it is: a
     * property of the expectation rather than of the call. */
    obs_provenance from;
} obs_check;

typedef struct obs_section {
    /* Two-digit ordering prefix plus a name, e.g. "010-kernel". Sections run in
     * array order, base layers first, so a failure high up is read against a
     * foundation already known to be sound. */
    const char *id;
    const char *title;
    /* One line on what this section establishes. */
    const char *purpose;
    const obs_check *checks;
    unsigned int check_count;
} obs_section;

/* Tallies across a run. */
typedef struct obs_tally {
    unsigned int pass;
    unsigned int partial;
    unsigned int fail;
    unsigned int skip;
} obs_tally;

/* Runs every registered section in order. Returns the tally. */
obs_tally obs_run_all(void);

/* Whether an address can plausibly be called.
 *
 * Null is the documented answer for an unresolved weak import, and an emulator was
 * found resolving unrecognised symbols to small non-null values instead. A guard that
 * only rejects null accepts those, calls them, and loses the rest of the run. See
 * harness.c for why the first page is the boundary.
 *
 * Used by the census as well, so a symbol reported present is one that could actually
 * be called. */
int obs_address_is_callable(const void *address);

/* Load a library by name and hand back a handle, or a negative value if it will not load.
 *
 * # Asking, rather than requiring
 *
 * The other way to find out whether a library is there is to import a symbol from it, which
 * puts it in `DT_NEEDED` and makes a system loader load it **before this program runs**. That
 * is not a question, it is a demand, and a demand a title cannot meet is a console that dies
 * with nothing on record - there is no instruction of ours left to report from. (D226)
 *
 * This returns a value instead. A library that will not load becomes a finding, which is what
 * a probe is for. Guard both platform calls before using it: an emulator that implements
 * neither should report a skip rather than a wall of absences.
 *
 * The path is tried in several forms and the one that worked is not reported, because the
 * caller reports the library. See `obs_module_open` in `harness.c` for why no absolute path
 * can be written down.
 */
int obs_module_open(const char *library);

/* Resolve one name through a handle from [`obs_module_open`], or null.
 *
 * Null both when the resolver refuses and when it returns an address that is not callable:
 * a resolver answering success with a small non-null value would otherwise count as present,
 * which is the trap `obs_address_is_callable` exists for. */
const void *obs_module_symbol(int handle, const char *name);

/* Whether run-time module resolution works on this platform at all.
 *
 * # Without this, every answer it gives is unreadable
 *
 * `obs_module_open` failing has two possible causes and they are opposite findings: the library
 * is genuinely not there, or **this loader does not implement module loading** and no library
 * would ever be found. A census built on the first reading, run on a platform that is the
 * second, reports every library as absent - including the one it is currently making calls
 * into.
 *
 * That is not hypothetical. It is what PS5PCEM produced the first time the run-time census met
 * it: `900-surface/kernel fail - this library could not be loaded`, from a process running on
 * `libkernel` at the time.
 *
 * So there is a control, and it is the same argument `check_control` makes one level down: an
 * absence has to mean one thing before a count of absences means anything. `libkernel` is the
 * subject because this program is executing calls into it - if *that* cannot be resolved, the
 * mechanism is what is missing, not the library.
 *
 * Decided once and remembered, because it cannot change during a run. (D232)
 */
int obs_module_resolution_works(void);

/* Whether a check was named at build time as one that ends the process here.
 *
 * Reported as a skip with a reason rather than omitted, so it stays visible and a diff
 * still sees it stop being run. See harness.c for why this exists and why the default
 * list is empty. */
/* Nonzero when a check must not run. The value says which mechanism decided:
 * `OBS_EXCLUDED_AT_BUILD` is an operator list compiled in, `OBS_EXCLUDED_BY_PREVIOUS_RUN`
 * is this program noticing that the last run of the same binary announced it and never
 * finished. Only the second is a finding. */
#define OBS_EXCLUDED_AT_BUILD 1
#define OBS_EXCLUDED_BY_PREVIOUS_RUN 2
int obs_check_is_excluded(const char *id);

/* The ordered section list. Defined in registry.c - one explicit list, so the
 * order is greppable rather than emergent from link order. */
extern const obs_section *const obs_sections[];
extern const unsigned int obs_section_count;

#define OBS_COUNT(array) ((unsigned int)(sizeof(array) / sizeof((array)[0])))

/* Skips the check unless every listed symbol resolved.
 *
 * # Why a check needs this at all
 *
 * The harness guards **one** symbol: the one in the check's table row. Every platform
 * declaration is weak, so anything else the body reaches for can be null, and calling
 * through a null pointer ends the process and loses every check behind it.
 *
 * # And why that is worse than a crash
 *
 * A check announces its table-row symbol before running. `015-sync/event-flag-round-trip`
 * announced `sceKernelPollEventFlag` and called `sceKernelCreateEventFlag` as its first
 * statement; had Create been the null one, the last line of the report would have named a
 * function that was never reached.
 *
 * Announce-before-attempting is the property this whole program is arranged around. An
 * unguarded call does not just risk the run, it makes the report lie about where it died.
 *
 * # Use
 *
 *     static obs_result check_malloc_free(void) {
 *         OBS_REQUIRE(&free);
 *         ...
 *
 * First statement, before any call. Takes addresses because that is what weak linkage
 * makes null, and `scripts/guards.py` reads it the same way - it treats a symbol whose
 * address the body mentions as guarded, however that address is spelled.
 *
 * The rule was written down in D058 and enforced nowhere, and thirty-eight checks
 * violated it. `scripts/guards.py` runs in `verify.sh` now, so it cannot drift again. */
#define OBS_REQUIRE(...)                                                                 \
    do {                                                                                 \
        const void *const obs_required_[] = {__VA_ARGS__};                               \
        for (unsigned int obs_i_ = 0; obs_i_ < OBS_COUNT(obs_required_); obs_i_++) {     \
            if (obs_required_[obs_i_] == 0) {                                            \
                return obs_skip("a symbol this check also calls is not present");        \
            }                                                                            \
        }                                                                                \
    } while (0)

#endif /* OBSCENE_HARNESS_H */
