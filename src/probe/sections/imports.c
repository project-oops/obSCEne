/*
 * This program's own imports, on the two axes that decide whether a check can run.
 *
 * # The question this exists to answer
 *
 * Twenty-four checks skipped on real hardware with "the loader did not resolve this
 * symbol for this build". That message is honest (D235) and it is not a diagnosis: a
 * null import has at least two causes and they need opposite repairs.
 *
 *   - The platform does not offer the symbol. Nothing to fix; the skip is the finding.
 *   - The platform offers it and *this module's declaration of the import did not bind*.
 *     A defect here, fixable here, and twenty-four checks come back.
 *
 * Those look identical from inside a check. They look different from here, because a
 * run-time lookup asks the same question by a different route.
 *
 * # Why it walks the check registry rather than `imports.c`
 *
 * `imports.c` lists library and symbol and no address, and some imports are declared ad
 * hoc inside a section file rather than in `platform.h`, so a table with addresses in it
 * could not be written there without moving those declarations first.
 *
 * The registry already carries all three - every check row is `(library, symbol, address)`
 * and the harness already tests that address to decide whether to run. So the set this
 * walks is exactly the set that gates a check, which is the set the question is about.
 * Imports that no check depends on are not measured here, and that is the right scope:
 * an unbound import nothing calls costs nothing.
 *
 * # Why it is one check and not one per symbol
 *
 * The per-symbol detail is in the `import` records, which is where an inventory belongs.
 * The check's own verdict answers the only question a summary can: is there anything here
 * that a repair could recover? A PASS means every unbound import is unbound because the
 * platform does not have it, and no amount of work on this module changes any of them.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Whether two names are the same. `strcmp` is a thing this program measures rather than
 * uses (see `035-libc`), and the runtime brings in no libc. */
static int same(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* How many distinct libraries a single walk may hold open.
 *
 * Bounded because there is no allocation here, and sized from the manifest: the module
 * declares twelve. Overflowing this is not silent - `libraries_overflowed` is reported -
 * because a cache that quietly stops caching turns into a reopen per check, and a reopen
 * per check on a platform where opening is expensive reads as a hang.
 *
 * It *did* overflow, on the first hardware run, because the walk was not restricted to this
 * module's own imports and the census names three hundred and fifty libraries. The report
 * said so, which is the only reason the run was readable at all. */
#define OBS_IMPORT_LIBRARIES 24

/* Whether `library` is one this module declares an import from.
 *
 * `obs_platform_each_symbol` takes a callback and carries no context, so the answer is
 * accumulated through file scope. Single-threaded and used only from the walk below. */
static const char *obs_wanted_library;
static int obs_wanted_found;

static void obs_note_library(const char *library, const char *symbol) {
    (void)symbol;
    if (!obs_wanted_found && same(library, obs_wanted_library)) {
        obs_wanted_found = 1;
    }
}

/* Whether a check's symbol field holds a symbol rather than a description of one. */
static int obs_is_symbol_name(const char *symbol) {
    if (*symbol == '\0') {
        return 0;
    }
    for (const char *at = symbol; *at != '\0'; at++) {
        if (*at == '(' || *at == ')' || *at == ' ') {
            return 0;
        }
    }
    return 1;
}

static int obs_library_is_imported(const char *library) {

    obs_wanted_library = library;
    obs_wanted_found = 0;
    obs_platform_each_symbol(obs_note_library);
    return obs_wanted_found;
}

/* Libraries a sweep proved end the process when loaded, as a space-separated list.
 *
 * Asking the resolver about a symbol means loading its library, and ten of them do not come
 * back. This section walked into one on its first full run - the report stops one library
 * after `libScePad`, and the next in registry order is `libSceVideoRecording`.
 *
 * The build's `EXCLUDE` could not cover it: that matches check ids, and this is one check
 * opening many libraries. Which is precisely the shape of D235 - a library loaded as a side
 * effect of a check that was not about loading it - committed here in the section written to
 * explain D235. Coming a second time in a place written by somebody who had just read about
 * it is worth recording.
 *
 * The names are derived in the Makefile from `data/hardware/crashers.txt`, so the record that
 * cost a hardware sweep is the one thing that has to be kept correct. (D245) */
#ifndef OBS_UNSAFE_LIBRARIES
#define OBS_UNSAFE_LIBRARIES ""
#endif

static int obs_library_is_unsafe(const char *library) {
    const char *at = OBS_UNSAFE_LIBRARIES;
    while (*at != '\0') {
        while (*at == ' ') {
            at++;
        }
        const char *name = library;
        const char *scan = at;
        while (*scan != '\0' && *scan != ' ' && *name != '\0' && *scan == *name) {
            scan++;
            name++;
        }
        /* Both ended together, so the whole entry matched the whole name. Comparing only the
         * prefix would make `libSceOpusDec` in the list silence `libSceOpusDecSomethingElse`,
         * which is a different library. */
        if (*name == '\0' && (*scan == '\0' || *scan == ' ')) {
            return 1;
        }
        while (*at != '\0' && *at != ' ') {
            at++;
        }
    }
    return 0;
}

static obs_result check_unbound_imports_are_absent_symbols(void) {
    if (!obs_module_resolution_works()) {
        /* Without a working resolver every symbol answers "unresolvable" and the two
         * causes collapse back into one - which is the exact conflation this section
         * exists to undo. Reporting a sheet of `unresolvable` here would be D232 again,
         * in a section written because of D232. */
        return obs_skip("this platform does not resolve modules by name, so the second "
                        "axis cannot be measured and one axis answers nothing");
    }

    /* Opened once per library rather than once per check: forty-odd checks name
     * `libkernel`, and opening a library forty times to ask forty questions is how a
     * measurement becomes a load test. */
    const char *names[OBS_IMPORT_LIBRARIES];
    int handles[OBS_IMPORT_LIBRARIES];
    unsigned int known = 0;
    int libraries_overflowed = 0;

    unsigned int unlinked_but_resolvable = 0;
    unsigned int unlinked_and_unresolvable = 0;
    unsigned int linked_but_unresolvable = 0;
    unsigned int examined = 0;
    unsigned int unasked = 0;

    for (unsigned int s = 0; s < obs_section_count; s++) {
        const obs_section *section = obs_sections[s];
        for (unsigned int c = 0; c < section->check_count; c++) {
            const obs_check *check = &section->checks[c];
            if (check->library == NULL || check->symbol == NULL) {
                continue;
            }
            /* Only libraries this module actually imports from.
             *
             * **This filter was `check->symbol[0] == '\0'` and that was wrong**, in a way
             * that cost a run: the census names a library per check and three hundred and
             * fifty libraries overall, all with symbols, so the walk sailed past the filter
             * and opened every one of them - before the census itself ran. The tally went
             * from 218 pass / 190 fail to 115 / 362, and the section that exists to explain
             * a measurement had quietly replaced it.
             *
             * The scope this section claims is "this program's own imports". That set is
             * `src/imports.c`, and asking it directly is both correct and cheap. A check
             * naming a library this module never imports from has no import to be unbound,
             * so there is nothing here to say about it. */
            if (!obs_library_is_imported(check->library)) {
                continue;
            }
            /* A check whose "symbol" is a placeholder, not a symbol.
             *
             * The census names a library per check and writes `(census)` where a symbol would
             * go, because the check is about the library rather than about any one name. Those
             * rows came through and read as `linked`, since the address in the row is the
             * census check's own guard - so every library appeared to bind two imports, and
             * "libScePad binds some and not others" was read off five rows of which three were
             * real. It changed a per-library finding into a per-symbol one, which is a
             * different search.
             *
             * Told apart by the ABI's own rule rather than by a section id: a symbol name is a
             * C identifier, so a bracket or a space in it means the field is prose. */
            if (!obs_is_symbol_name(check->symbol)) {
                continue;
            }

            int handle = -1;
            unsigned int i = 0;
            for (; i < known; i++) {
                if (same(names[i], check->library)) {
                    handle = handles[i];
                    break;
                }
            }
            if (i == known) {
                /* Named before it is opened, so a library that does not return names itself.
                 * The same property every check in this program has, and this walk needed its
                 * own because one check opens many libraries. */
                obs_report_module(check->library, 0);
                handle = obs_library_is_unsafe(check->library)
                             ? -1
                             : obs_module_open(check->library);
                if (known < OBS_IMPORT_LIBRARIES) {
                    names[known] = check->library;
                    handles[known] = handle;
                    known++;
                } else {
                    libraries_overflowed = 1;
                }
            }

            /* A library that was never asked about emits no record.
             *
             * The alternative is a row saying `unresolvable`, and that would be a lie of the
             * exact kind this section exists to stop: "the platform does not have it" recorded
             * where the truth is "we did not ask, because asking ends the process". A missing
             * row says nothing, which is what was learnt. The count is in the verdict so the
             * silence is accounted for rather than merely quiet. */
            if (obs_library_is_unsafe(check->library)) {
                unasked++;
                continue;
            }

            int linked = obs_address_is_callable(check->address);
            int resolvable = handle >= 0
                             && obs_module_symbol(handle, check->symbol) != NULL;
            obs_report_import(check->library, check->symbol, linked, resolvable);
            examined++;

            if (!linked && resolvable) {
                unlinked_but_resolvable++;
            } else if (!linked && !resolvable) {
                unlinked_and_unresolvable++;
            } else if (linked && !resolvable) {
                linked_but_unresolvable++;
            }
        }
    }

    if (libraries_overflowed) {
        return obs_partial_value("more libraries are named than this walk can hold open, "
                                 "so some rows were measured against a reopened handle",
                                 examined);
    }
    (void)linked_but_unresolvable;
    (void)unlinked_and_unresolvable;

    if (unasked > 0 && unlinked_but_resolvable == 0) {
        /* Reported before the pass, because a pass here means "every unbound import is
         * unbound because the platform lacks the symbol" and that claim cannot be made about
         * imports nobody asked about. Partial is the honest shape: what was measured is
         * clean, and some of it was not measured. */
        return obs_partial_value("some libraries were not asked about, because loading them "
                                 "ends the process on this platform",
                                 unasked);
    }

    if (unlinked_but_resolvable > 0) {
        /* **The repairable case, and the reason for the section.** The platform has the
         * symbol under the name and library this module declares, and the import still
         * did not bind. Every check behind one of these is skipping for a reason that is
         * ours. */
        return obs_fail_code("some imports did not bind to symbols this platform does have",
                             unlinked_but_resolvable);
    }
    return obs_pass_value(examined);
}

/*
 * Whether an import that reads as unbound can be called anyway.
 *
 * # Why this is not the same question as the one above
 *
 * Everything this program uses to decide "is this symbol available" reads a **data slot**.
 * `check->address` is a relocated entry in a `static const` table (an `R_X86_64_64` against an
 * undefined symbol); `&scePadClose` written in code is a GOT load (`R_GLOB_DAT`). A *call*
 * goes somewhere else entirely - through the PLT, an `R_JUMP_SLOT`, of which this module has
 * 198.
 *
 * Nothing establishes that a loader fills all three the same way. If it binds the PLT and
 * leaves the data slots null, then every one of the fourteen imports this section reports as
 * unbound is **callable**, fourteen checks are skipping for no reason, and the entire gating
 * mechanism of this program is reading the wrong thing.
 *
 * That is worth one call to find out, and no amount of reading the file can answer it: the
 * question is what the loader did, not what the module asked for.
 *
 * # Why it deliberately does what D058 forbids
 *
 * D058 says a check reaching for a symbol other than its own must test that address first,
 * because jumping to zero ends the run. **This does not test it, and that is the measurement.**
 * The rule exists so a run is not lost to an unguarded jump; here the jump is the experiment,
 * and the cost of losing the run is bounded by two properties this program already has: the
 * `try` record is written before the call, so a death names this check and nothing else, and
 * the resume record skips it on the next run of the same build. Both were exercised getting
 * past a library that ends the process (D245).
 *
 * The call is from the failure side, like the rest of the suite: an invalid handle, which a
 * working implementation must reject. It asks nothing of the pad and opens nothing.
 */
/* Off unless asked for, because it has already been answered and the answer ends the process.
 *
 * **Measured on hardware, 2026-08-30: the call does not return.** `scePadClose` with a null
 * address slot dies, so the PLT is not bound either and the three relocation forms are all
 * unfilled together. The address test was telling the truth and this program's gating is
 * reading the right thing.
 *
 * That is a genuine result and it eliminates a whole class of repair - there is no "the import
 * works, we are just testing it wrong". It is also a one-shot: leaving it on costs two runs
 * every time (D181 retries once before skipping) to re-learn something already known.
 *
 * It stays in the tree rather than being deleted because it is the only thing that would
 * notice a firmware where the answer changed, and it is one flag to run again:
 *
 *     make pkg CALL_UNBOUND=1
 */
#ifndef OBS_CALL_UNBOUND
#define OBS_CALL_UNBOUND 0
#endif

static obs_result check_unbound_import_is_callable(void) {
    if (obs_address_is_callable((const void *)&scePadClose)) {
        return obs_skip("this import bound, so there is no unbound one here to try calling");
    }
#if OBS_CALL_UNBOUND
    /* Unguarded on purpose. See above. */
    int rc = scePadClose(OBS_FD_INVALID);
    /* It returned. The address slot is null and the call worked, so the two are filled
     * separately and this program has been gating on the wrong one. */
    return obs_pass_value((uint64_t)(uint32_t)rc);
#else
    return obs_skip("calling an unbound import ended the process when this was last asked, so "
                    "it is not asked again unless the build says to - CALL_UNBOUND=1");
#endif
}

static const obs_check import_checks[] = {

    /* Guarded on `sceKernelClose`, which binds, rather than on the symbol it calls. The
     * harness skips a check whose own address is null, and a check written to call a null
     * address would therefore never run. */
    {"061-imports/unbound-import-is-callable", "libkernel", "sceKernelClose",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelClose,
     check_unbound_import_is_callable, OBS_FROM_ASSUMED},
    {"061-imports/unbound-are-absent", "libkernel", "sceKernelLoadStartModule",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelLoadStartModule,
     check_unbound_imports_are_absent_symbols, OBS_FROM_ASSUMED},
};

const obs_section obs_section_imports = {
    "061-imports",
    "Imports",
    "Whether an import that did not bind is a symbol the platform lacks, or one this "
    "module failed to ask for correctly.",
    import_checks,
    OBS_COUNT(import_checks),
};
