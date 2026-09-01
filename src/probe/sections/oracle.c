/*
 * Asking the platform what it knows, rather than guessing.
 *
 * # Why this is potentially the most valuable section here
 *
 * An identifier is a truncated hash of a name, and hashing is one way. The established
 * route from an identifier back to a name is to guess: generate candidates, hash each,
 * and hope for a match against identifiers observed in the wild. It works, it is
 * enormously expensive - one project reports 249,506 candidates for a single confirmed
 * name - and it can only ever recover names that something happens to import.
 *
 * If the platform resolves symbols **by name**, that collapses to one question per
 * candidate with a yes-or-no answer, and it reaches functions no title imports at all.
 *
 * `docs/HARDWARE-PROBE.md` calls this "the one that changes the most", and it is the
 * cheapest thing on that page to attempt: the call is already declared, and the check
 * is a loop.
 *
 * # It is written to be useful when the answer is no
 *
 * A platform that cannot resolve by name is the likely outcome, and this section is
 * built so that outcome is stated rather than merely absent. The control pair below is
 * the whole design: **a name that must resolve and a name that cannot**. An
 * implementation answering yes to both is useless in the opposite direction to one
 * answering no to both, and neither is distinguishable from a working oracle by any
 * single query.
 *
 * That is the same reasoning as the census control (`900-surface/control`), and for the
 * same reason: an instrument that cannot be shown to work is not evidence.
 */

#include "common/freestd.h"
#include "common/krw.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* A handle meaning "the running process" rather than a loaded module.
 *
 * Zero is the conventional value for that in every dynamic-linking interface this one
 * resembles, and it is what a caller with no module handle has to try. It may well be
 * refused, which is a reportable answer rather than a failure of the check. */
#define OBS_HANDLE_SELF 0

/* Names the oracle is asked about.
 *
 * Two of them are the control and the rest are ordinary. The candidates are
 * deliberately things this program already imports, so a yes is checkable against a
 * symbol whose address the loader has already resolved - which is what makes a positive
 * answer trustworthy rather than merely encouraging. */
typedef struct {
    const char *library;
    const char *name;
    /* 1 must resolve, 0 must not, -1 unknown and only reported. */
    int expected;
} candidate;

static const candidate candidates[] = {
    /* The control pair. Neither is a real query - they exist to establish whether the
     * answers mean anything at all. */
    {"libkernel", "sceKernelWrite", 1},
    {"libkernel", "obscene_certainly_not_a_symbol", 0},
    /* Ordinary queries, all imported by this module, so a resolved address can be
     * compared against one the loader already produced. */
    {"libkernel", "sceKernelGetProcessTime", -1},
    {"libkernel", "scePthreadSelf", -1},
    {"libSceLibcInternal", "strlen", -1},
    /* And one that is real, documented and deliberately *not* imported here. If the
     * oracle answers for this, it is answering about the platform rather than about
     * this module's import table - which is the difference between a name oracle and an
     * expensive way to read our own symbols back. */
    {"libkernel", "sceKernelGetCurrentCpu", -1},
};

static obs_result check_resolve_by_name(void) {
    unsigned int resolved = 0;
    unsigned int asked = 0;
    int control_present = -1;
    int control_absent = -1;

    for (unsigned int i = 0; i < OBS_COUNT(candidates); i++) {
        void *address = 0;
        int rc = sceKernelDlsym(OBS_HANDLE_SELF, candidates[i].name, &address);
        int present = (rc == 0 && address != 0);
        asked++;
        if (present) {
            resolved++;
        }
        obs_report_resolve(candidates[i].library, candidates[i].name, present,
                           (uint64_t)(uintptr_t)address);
        if (candidates[i].expected == 1) {
            control_present = present;
        } else if (candidates[i].expected == 0) {
            control_absent = present;
        }
    }

    /* The control decides what any of this is worth.
     *
     * A platform saying yes to a name that cannot exist is not an oracle, it is a
     * function that returns success - and its yes for every other name means nothing.
     * That is the more dangerous failure of the two, because it looks like success. */
    if (control_absent == 1) {
        return obs_fail("a name that cannot exist resolved; the answers are worthless");
    }
    if (control_present == 0) {
        /* Saying no to everything is the expected outcome on a platform without a
         * by-name interface, and it is honest. Reported as a skip rather than a
         * failure: the platform is not obliged to have one. */
        return obs_skip(
            "nothing resolves by name, including a symbol known to be present");
    }
    if (control_present == 1 && control_absent == 0) {
        /* Both controls right, so the answers can be believed - which makes this the
         * most consequential pass in the suite. */
        return obs_pass_value((uint64_t)resolved);
    }
    return obs_partial_value("the controls did not both answer, so treat the rest as "
                             "unconfirmed",
                             (uint64_t)asked);
}

static obs_result check_error_codes(void) {
    OBS_REQUIRE(&sceKernelClose, &sceKernelRead, &sceKernelLseek);

    /* Deliberately wrong arguments, and the *code* is the finding.
     *
     * Every negative check in this suite asserts that a bad argument is refused and
     * throws the returned value away. That is right on an emulator, where the code is
     * invented. On hardware the code is the whole point: an implementation returning
     * something no caller recognises makes guests retry forever, and a table of real
     * codes turns "unimplemented" into "fails the way the caller expects".
     *
     * No expectation is asserted here at all. It records what came back. */
    unsigned int recorded = 0;

    obs_report_error_code("libkernel", "sceKernelClose", "descriptor -1",
                          (uint64_t)(uint32_t)sceKernelClose(OBS_HANDLE_INVALID));
    recorded++;

    unsigned char scratch[16];
    obs_report_error_code(
        "libkernel", "sceKernelRead", "descriptor -1",
        (uint64_t)sceKernelRead(OBS_HANDLE_INVALID, scratch, sizeof scratch));
    recorded++;

    obs_report_error_code(
        "libkernel", "sceKernelLseek", "descriptor -1",
        (uint64_t)sceKernelLseek(OBS_HANDLE_INVALID, 0, OBS_SEEK_CUR));
    recorded++;

    if (obs_address_is_callable((const void *)&sceKernelOpen)) {
        obs_report_error_code("libkernel", "sceKernelOpen", "path that does not exist",
                              (uint64_t)(uint32_t)sceKernelOpen(
                                  "/obscene/definitely/not/here", OBS_O_RDONLY, 0));
        recorded++;
    }
    if (obs_address_is_callable((const void *)&sceKernelDeleteEventFlag)) {
        obs_report_error_code("libkernel", "sceKernelDeleteEventFlag", "null handle",
                              (uint64_t)(uint32_t)sceKernelDeleteEventFlag(NULL));
        recorded++;
    }
    if (obs_address_is_callable((const void *)&sceKernelPollEventFlag)) {
        obs_report_error_code(
            "libkernel", "sceKernelPollEventFlag", "null handle",
            (uint64_t)(uint32_t)sceKernelPollEventFlag(NULL, 1, 0, NULL));
        recorded++;
    }
    if (obs_address_is_callable((const void *)&sceKernelSetEventFlag)) {
        obs_report_error_code("libkernel", "sceKernelSetEventFlag", "null handle",
                              (uint64_t)(uint32_t)sceKernelSetEventFlag(NULL, 1));
        recorded++;
    }

    /* Never a failure. Nothing here has an expectation to violate - the section exists
     * to fill a table, and an empty table would be the only bad outcome. */
    return obs_pass_value((uint64_t)recorded);
}

static unsigned int oracle_census_asked;
static unsigned int oracle_census_resolved;

static void oracle_ask_one(const char *library, const char *symbol) {
    void *address = 0;
    int rc = sceKernelDlsym(OBS_HANDLE_SELF, symbol, &address);
    int present = (rc == 0 && address != 0);
    oracle_census_asked++;
    if (present) {
        oracle_census_resolved++;
    }
    obs_report_resolve(library, symbol, present, (uint64_t)(uintptr_t)address);
    if ((oracle_census_asked & 31u) == 0u) {
        obs_report_progress("140-oracle/resolve-census", (uint64_t)oracle_census_asked);
    }
}

static obs_result check_resolve_census(void) {
    OBS_REQUIRE(&sceKernelDlsym);

    void *present_address = 0;
    void *absent_address = 0;
    int present_rc =
        sceKernelDlsym(OBS_HANDLE_SELF, "sceKernelWrite", &present_address);
    int absent_rc = sceKernelDlsym(OBS_HANDLE_SELF, "obscene_no_such_symbol_exists",
                                   &absent_address);
    if (absent_rc == 0 && absent_address != 0) {
        return obs_fail(
            "a name that cannot exist resolved; the walk would be worthless");
    }
    if (!(present_rc == 0 && present_address != 0)) {
        return obs_skip("the oracle does not answer, so there is nothing to walk");
    }

    oracle_census_asked = 0;
    oracle_census_resolved = 0;
    obs_surface_each_symbol(oracle_ask_one);

    obs_report_measure("140-oracle/resolve-census", "sceKernelDlsym", "asked",
                       (uint64_t)oracle_census_asked, "names");
    obs_report_measure("140-oracle/resolve-census", "sceKernelDlsym", "resolved",
                       (uint64_t)oracle_census_resolved, "names");
    if (oracle_census_asked == 0) {
        return obs_skip("the census yielded no names to ask about");
    }
    return obs_pass_value((uint64_t)oracle_census_resolved);
}

static obs_result check_kernel_exports_stream(void) {
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs == NULL || pargs->kexport_table == NULL) {
        return obs_skip("no kernel export table available in this execution context");
    }
    const obs_kexport_table_t *table =
        (const obs_kexport_table_t *)pargs->kexport_table;
    for (uint32_t i = 0; i < table->count; i++) {
        const obs_kexport_entry_t *entry = &table->entries[i];
        obs_report_measure("140-oracle/kexport-table", entry->nid, "vaddr",
                           entry->vaddr, "offset");
    }
    return obs_pass_value((uint64_t)table->count);
}

static const obs_check oracle_checks[] = {
    {"140-oracle/resolve-by-name", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_resolve_by_name,
     OBS_FROM_ASSUMED},
    {"140-oracle/resolve-census", "libkernel", "sceKernelDlsym", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelDlsym, check_resolve_census,
     OBS_FROM_ASSUMED},
    {"140-oracle/error-codes", "libkernel", "sceKernelClose", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceKernelClose, check_error_codes, OBS_FROM_ASSUMED},
    {"140-oracle/kexport-stream", "obscene", "kexport_table", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&obs_get_payload_args, check_kernel_exports_stream,
     OBS_FROM_HARDWARE},
};

const obs_section obs_section_oracle = {
    "140-oracle",
    "Asking, not guessing",
    "Whether the platform will resolve a symbol by name - which would replace the "
    "whole "
    "guess-and-hash approach to naming - and what its real error codes are. Both "
    "record "
    "answers rather than testing them.",
    oracle_checks,
    OBS_COUNT(oracle_checks),
};
