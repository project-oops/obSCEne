/*
 * Which console is this?
 *
 * Runs early, because every later result is read differently depending on the answer.
 * A previous-generation graphics symbol missing from current hardware is correct; the
 * same absence on previous-generation hardware is a broken platform. Without knowing
 * which, "absent" cannot be scored at all.
 *
 * # Detected, not assumed
 *
 * The program does not ask the platform what it is - there is no portable call that
 * answers honestly, and an emulator would answer whatever it was configured to say.
 * Instead it probes symbols that exist on exactly one generation and infers from
 * which of them resolve. That is a fact about the platform actually present rather
 * than a claim it makes about itself.
 *
 * # It can legitimately answer "both" or "neither"
 *
 * An emulator part-way through implementing either generation will resolve some of
 * both, or none of either. Both are reported honestly rather than forced into a
 * verdict, because a wrong generation silently reclassifies every expected absence as
 * a gap and every gap as expected.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/* Discriminators.
 *
 * Chosen for being unambiguous rather than important: the previous generation's
 * graphics driver has no counterpart on current hardware, and the current
 * generation's does not exist on the previous one. Declared as data so they can only
 * be probed, never called - the same rule the census follows, for the same reason. */
extern OBS_WEAK const char sceGnmSubmitCommandBuffers;
extern OBS_WEAK const char sceGnmDrawIndex;
/* The current generation has exactly one discriminator here, not two, because only
 * one has a source (D008). A name invented to round out the pair would resolve on
 * nothing and quietly misreport current-generation hardware as unidentifiable - the
 * failure would look like a missing driver rather than a bad guess. */

/* Two more markers, taken over from `sysinfo.c` when its duplicate inference was removed.
 * The previous generation's submit-done and the current generation's command-buffer
 * acquire: both unambiguous, both already declared this way somewhere, and having them
 * here means one file owns the question. */
extern OBS_WEAK const char sceGnmSubmitDone;

/* What the probe concluded. Read by later sections and by the report renderer, so an
 * other-generation absence can be shown as expected rather than as a fault. */
static obs_generation detected = OBS_GENERATION_UNKNOWN;

/* Whether each generation's graphics driver is here.
 *
 * Extracted so there is exactly one inference. `sysinfo.c` had its own, drawing the header
 * that reads `GEN 5 (CURRENT)` - and it used *different* marker symbols
 * (`sceGnmSubmitDone`, `sceAgcAcbAcquireMem`) than this section did. Two inferences that
 * can disagree, over the one fact the report puts in its header, while the accessor built
 * to prevent that had no callers at all (D110).
 *
 * More markers rather than fewer: a platform that implements one function of a driver and
 * not another still answers, and any of them resolving is evidence the driver is there. */
static int obs_previous_present(void) {
    return &sceGnmSubmitCommandBuffers != NULL || &sceGnmDrawIndex != NULL
           || &sceGnmSubmitDone != NULL;
}

/* The current generation's driver is asked for **at run time**, not linked.
 *
 * This read `&sceAgcCreateShader != NULL`, which is presence-by-linking: taking the address
 * imports the name, which puts `libSceAgcDriver` in `DT_NEEDED`, which makes a system loader
 * load it before this program runs. The two AGC libraries are current-generation graphics and
 * a package installs under a `ps4_game` category, so they are exactly the ones a title is
 * least likely to be given - and a `DT_NEEDED` a title cannot meet is a console that dies with
 * nothing on record, before any of this could report. (D226)
 *
 * That made the generation probe able to kill the thing it was probing. Asking instead returns
 * a value: the driver loads, or it does not, and either way the answer is legible.
 *
 * The markers stay the same two, for the reason above them - more markers rather than fewer -
 * and so does the meaning of the answer. (D230) */
static int obs_current_present(void) {
    /* Unanswerable, not absent.
     *
     * A loader that cannot resolve modules by name says "no" to every one, so without this the
     * probe reports "the current generation's driver is missing" on a platform where it simply
     * did not look. Measured on PS5PCEM, which implements the current generation's graphics
     * and was reported as neither generation. (D232) */
    if (!obs_module_resolution_works()) {
        return -1;
    }
    static const struct {
        const char *library;
        const char *symbol;
    } markers[] = {
        {"libSceAgcDriver", "sceAgcCreateShader"},
        {"libSceAgc", "sceAgcAcbAcquireMem"},
    };
    /* A marker library that will not open is "could not look", not "absent".
     *
     * **This reported the wrong generation for a real console.** Both markers live in
     * `libSceAgc` and `libSceAgcDriver`, and those are `EI_ABIVERSION 2` libraries. A title in
     * the previous generation's category is refused them by the loader, which says so in the
     * system log and not to us:
     *
     *     ### ERROR: ABIVERSION mismatch. /<sandbox>/common/lib/libSceAgcDriver.sprx
     *
     * So on a current-generation console this returned 0, `005-generation/detect` passed, and
     * the report and the on-screen header both said **GEN 4** about a machine that is not one.
     * A pass with a wrong answer is worse than a failure, because nothing about it invites a
     * second look.
     *
     * The distinction is the one this function already makes at the top for
     * `obs_module_resolution_works`, applied one level down: a library that did not open tells
     * you nothing about what is in it. A library that opened and did not have the symbol tells
     * you the symbol is not there, and that is still worth reporting as absence. (D255) */
    int opened_any = 0;
    for (unsigned int i = 0; i < OBS_COUNT(markers); i++) {
        int handle = obs_module_open(markers[i].library);
        if (handle < 0) {
            continue;
        }
        opened_any = 1;
        if (obs_module_symbol(handle, markers[i].symbol) != NULL || handle > 0) {
            return 1;
        }
    }
    if (!opened_any) {
        return -1;
    }
    return 0;
}

/* The detected generation, computed on demand.
 *
 * Lazy because the header is drawn before any section runs, so a value cached only by
 * `005-generation` would be unknown for the first part of every run - and the honest fix is
 * to compute it when asked rather than to keep a second copy of the reasoning somewhere it
 * can be answered earlier. */
obs_generation obs_detected_generation(void) {
    if (detected != OBS_GENERATION_UNKNOWN) {
        return detected;
    }
    int previous = obs_previous_present();
    int answer = obs_current_present();
    /* Compared against 1 rather than tested for truth: `obs_current_present` answers
     * `-1` for "could not look", which is not zero and is emphatically not yes. */
    int current = answer == 1;

    /* "Could not look" leaves the generation unknown, not previous.
     *
     * This computed `previous && !current` and called it *previous generation*, which reads
     * the same for "the current generation's markers are absent" and "the current generation's
     * markers could not be reached". On a real console it is always the second: the markers
     * are `EI_ABIVERSION 2` libraries and this module's category is refused them.
     *
     * The result was a console reporting **GEN 4** about itself, in the stream and across the
     * top of its own screen, with nothing anywhere saying the answer was a guess. Unknown is
     * the honest output and it is visibly unknown, which is the whole point of having the
     * state. (D255) */
    if (answer < 0) {
        return detected;
    }

    if (previous && current) {
        detected = OBS_GENERATION_BOTH;
    } else if (previous) {
        detected = OBS_GENERATION_PREVIOUS;
    } else if (current) {
        detected = OBS_GENERATION_CURRENT;
    }
    return detected;
}

/* The graphics driver(s) present, reported as a driver fact rather than a generation verdict.
 *
 * `obs_detected_generation` collapses to UNKNOWN the moment the current generation's driver
 * cannot be looked at, which on a ps4_game is always (D255) - and that discards the gnm driver
 * that is plainly here (it draws the report and `165-gnm` passes). This asks the same two
 * markers a different question - "which driver resolves", not "which console is this" - so the
 * gnm the verdict has to drop is still surfaced. `== 1` on the current side, so a "could not
 * look" (-1) is not read as present; gnm being present names no console, so nothing here ages. */
const char *obs_gpu_drivers(void) {
    int previous = obs_previous_present();
    int current = obs_current_present() == 1;
    if (previous && current) {
        return "gnm+agc";
    }
    if (current) {
        return "agc";
    }
    if (previous) {
        return "gnm";
    }
    return (const char *)0;
}

static obs_result check_generation(void) {
    int previous = obs_previous_present();
    int current_raw = obs_current_present();
    int current = current_raw == 1;

    /* The current-generation side could not be looked at, so no verdict is available even
     * though half the evidence is. Reported as its own outcome rather than folded into
     * "neither driver resolves", which would be a claim about the platform made from a
     * measurement that never happened. (D232) */
    if (current_raw < 0) {
        /* Unknown, even though the previous generation's driver resolves.
         *
         * This wrote `previous ? PREVIOUS : UNKNOWN`, which is the exact "GEN 4 about itself"
         * bug D255 fixed in `obs_detected_generation` - and missed here, in the check that
         * actually runs. The two share the cached `detected`, so the correct UNKNOWN from the
         * header's early call was overwritten with PREVIOUS the moment this section ran, and
         * every later redraw showed GEN 4 while the stream (captured before this) still said
         * unknown. The console cannot be identified when the current generation's markers
         * cannot even be loaded, so the cache must say so; the message below still notes that
         * the previous driver resolved, which is the informative part. (D255) */
        detected = OBS_GENERATION_UNKNOWN;
        return obs_partial(
            previous ? "the previous generation's driver resolves; the current generation's "
                       "could not be looked for, because this platform does not resolve "
                       "modules by name"
                     : "neither driver could be established: the previous generation's does "
                       "not resolve and the current generation's could not be looked for");
    }

    /* No sym records here. The census owns symbol reporting, and emitting them from
     * two places produces a symbol listed twice - which verify.py refuses, because a
     * duplicate makes a diff silently ambiguous. This check's output is the verdict. */

    if (previous && current) {
        /* Both drivers resolve. On real hardware that can be the back-compat path; on an
         * emulator it is more often the fingerprint of a loader that stub-resolves every
         * unresolved import, so the current-generation driver "answers" with no
         * implementation behind it (measured: one shadPS4 run stub-resolved tens of
         * thousands of this program's imports, hundreds of them the current-generation
         * graphics library). Either way neither set of absences can be attributed to a
         * generation, so this stays a non-verdict rather than picking one - the same reason
         * the header renders `GEN` as `unknown` here rather than a console number. */
        detected = OBS_GENERATION_BOTH;
        return obs_partial("both generations' drivers resolve (real back-compat, or a "
                           "stub-everything loader answering for free); absences cannot be "
                           "attributed to a generation");
    }
    if (previous) {
        detected = OBS_GENERATION_PREVIOUS;
        return obs_pass_value(OBS_GENERATION_PREVIOUS);
    }
    if (current) {
        detected = OBS_GENERATION_CURRENT;
        return obs_pass_value(OBS_GENERATION_CURRENT);
    }

    /* Neither. Almost certainly an emulator with no graphics driver implemented,
     * which is the normal state early on. Not a failure of this check - the check
     * worked and the answer is "cannot tell". */
    detected = OBS_GENERATION_UNKNOWN;
    return obs_partial("neither generation's graphics driver resolves, so the console "
                       "cannot be identified");
}

static obs_result check_neo_mode(void) {
    /* A previous-generation call with no current-generation counterpart. Reported for
     * its own sake and as a second, independent signal - if this resolves while the
     * graphics probe said current-generation, one of the two is wrong and that is
     * worth seeing. */
    int neo = sceKernelIsNeoMode();
    if (detected == OBS_GENERATION_CURRENT) {
        return obs_partial_value("a previous-generation call answered on hardware "
                                 "identified as current-generation",
                                 (uint64_t)(uint32_t)neo);
    }
    return obs_pass_value((uint64_t)(uint32_t)neo);
}

static const obs_check generation_checks[] = {
    {"005-generation/detect", "obscene", "(symbol probe)", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_generation, OBS_FROM_ASSUMED},
    {"005-generation/neo-mode", "libkernel", "sceKernelIsNeoMode", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelIsNeoMode, check_neo_mode, OBS_FROM_ASSUMED},
};

const obs_section obs_section_generation = {
    "005-generation",
    "Console generation",
    "Which console this is, inferred from which exclusive symbols resolve. Read this "
    "first: it decides whether an absence below is a gap or is expected.",
    generation_checks,
    OBS_COUNT(generation_checks),
};
