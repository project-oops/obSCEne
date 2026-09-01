/*
 * The symbol census.
 *
 * A different kind of work from every other section, and placed last because of it.
 * The behavioural sections ask whether a function *works*; this asks only whether it
 * *exists*, which costs a name rather than a confident signature and therefore scales
 * to the whole platform.
 *
 * The ratio it produces - symbols present against symbols known - is the honest
 * headline for an emulator's progress. Thirty-five behavioural checks cannot measure
 * that; several hundred presence probes can, at no risk.
 *
 * # Nothing here is ever called
 *
 * Every censused name is declared `const char` rather than as a function, so only its
 * address can be read. That is not a convention to remember: the type system rejects
 * a call outright, which is the point. Calling a function whose signature is unknown
 * is precisely the mistake D008 exists to prevent, and a census covering hundreds of
 * symbols would otherwise be hundreds of chances to make it.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "obscene/runtime.h"
#include "obscene/corpus.h"
#include "obscene/nids.h"
#include "obscene/surface.h"

typedef struct obs_symbol {
    const char *name;
    /* Null when the loader could not resolve it. Weak declarations are what make
     * that a reportable fact rather than a link failure. */
    const void *address;
} obs_symbol;

/* Whether this build takes the address of every censused name.
 *
 * # Taking an address is what imports a symbol, and importing is what requires a
 * library
 *
 * The census asks "does this exist?" by taking a name's address and testing it. That
 * makes the name an undefined import, which puts its library in `DT_NEEDED`, which
 * makes a loader
 * **load that library before this program runs**. Under a homebrew loader or an
 * emulator that costs nothing: one loads what it is told, the other stubs everything.
 *
 * A system loader is not either of those. The census names 339 libraries beyond the
 * thirteen the checks actually call, so an eboot required 352, a title is given six,
 * and the console went down inside the loader with no report - before any instruction
 * of ours could run, which is the one place this program's guards cannot reach. (D226,
 * D227)
 *
 * So an eboot is built with this off. No address is taken, nothing is imported, no
 * library is required, and the census reports itself skipped with a reason rather than
 * emitting thirty-five thousand absences that are true of the build rather than of the
 * platform.
 *
 * **The measurement is not lost, it moves.** With this off, `census` loads each library
 * through `sceKernelLoadStartModule` and resolves each name through `sceKernelDlsym` -
 * so the section asks the same questions, produces the same records, and additionally
 * reports a library that *cannot be loaded* as a finding rather than as a dead console.
 * That is strictly more than linking ever told us. (D229) */
#ifndef OBS_CENSUS_LINKED
#define OBS_CENSUS_LINKED 0
#endif

/* Declares one censused name as data. See the note above on why not as a function.
 *
 * Kept in both modes. A declaration imports nothing on its own - only taking the
 * address does - so leaving it costs nothing and keeps the two modes one file rather
 * than two. */
#define OBS_DECLARE_SYMBOL(name) extern OBS_WEAK const char name;
#if OBS_CENSUS_LINKED
/* One table row. The name appears once in the list and is stringified here, so the
 * reported string and the resolved symbol can never drift apart. */
#define OBS_SYMBOL_ROW(name) {#name, (const void *)&name},
#else
/* The same row without the address, which is the only part that imports anything.
 *
 * The **name is kept**, so the table, the checks and every record they produce are
 * unchanged - what changes is how presence is decided, not what is asked or how it is
 * reported. The stringify-once property above holds either way. */
#define OBS_SYMBOL_ROW(name) {#name, NULL},
#endif

static obs_result census(const char *library, obs_availability availability,
                         const obs_symbol *symbols, unsigned int count) {
    unsigned int present = 0;
#if !OBS_CENSUS_LINKED
    /* Resolved at run time, because this build imports none of these names. See
     * `OBS_CENSUS_LINKED`: the address in each row is null and the name is what is
     * left.
     *
     * Announced by the harness before this runs, one check per library, which is what
     * makes a library that takes the process down with it name itself. */
    /* The control first, or every result below is unreadable.
     *
     * A loader that does not implement module resolution answers "no" for every
     * library, including ones this process is executing calls into - which is a sheet
     * of failures that parses, counts, and is wrong throughout. Measured, on PS5PCEM:
     * `900-surface/kernel fail` from a process running on `libkernel`. (D232) */
    if (!obs_module_resolution_works()) {
        return obs_skip(
            "this build resolves the census at run time and this platform does not "
            "resolve modules by name, so no library here could be found - which is "
            "a fact about the loader rather than about any library");
    }

    int handle = obs_module_open(library);
    if (handle < 0) {
        /* **A finding, and the one this whole change exists to produce**, now that the
         * control above has established the mechanism works. Under the linked census
         * this same condition was a console that died in the loader with nothing on
         * record. */
        return obs_fail("this library could not be loaded");
    }

    for (unsigned int i = 0; i < count; i++) {
        /* `obs_module_symbol` returns null both when the resolver refuses and when it
         * answers with an address that is not callable - the trap the linked census
         * avoids by not testing `!= NULL`. */
        int found = obs_module_symbol(handle, symbols[i].name) != NULL;
        obs_report_symbol(library, symbols[i].name, found, availability);
        present += (unsigned int)(found ? 1 : 0);
    }
#else
    for (unsigned int i = 0; i < count; i++) {
        /* Not `!= NULL`. A loader that resolves an unrecognised symbol to a small
         * non-null value would have every one of those counted as present, which
         * inflates the one number this section exists to produce. */
        int found = obs_address_is_callable(symbols[i].address);
        obs_report_symbol(library, symbols[i].name, found, availability);
        present += (unsigned int)(found ? 1 : 0);
    }
#endif

    if (present == count) {
        return obs_pass_value(present);
    }

    /* A library belonging to the *other* generation is expected to be missing, and
     * scoring that red would bury the real gaps under absences that are correct.
     * This is the same conflation the control check guards against one level down:
     * an absence has to mean one thing before a count of absences means anything. */
    if (availability == OBS_PREVIOUS || availability == OBS_CURRENT) {
        if (present == 0) {
            return obs_skip("belongs to the other console generation, so absence is "
                            "expected rather than a gap");
        }
        return obs_partial_value("partly present, though it belongs to the other "
                                 "console generation",
                                 present);
    }

    if (present == 0) {
        /* None of the library is here. Reported red because a title needing any of
         * it cannot start, not because zero is worse than one. */
        return obs_fail_code("none of this library is present", present);
    }
    return obs_partial_value("only part of this library is present", present);
}

/*
 * The control.
 *
 * Every other number in this section is a count of absences, and an absence is what a
 * broken census produces too. Without a control, "none of this library is present"
 * and "the presence test does not work" are the same output - and on any host that
 * implements none of the platform, that is the output for everything.
 *
 * So: one symbol that must resolve, and one that must not. Both are probed exactly
 * the way real entries are. If this check does not pass, nothing else in the section
 * means anything.
 */

/* Declared weak here but defined in runtime.c, so this goes through the same
 * resolution path as a real platform symbol rather than short-circuiting. */
extern OBS_WEAK const char obs_census_control_present;
/* Never defined anywhere, by design. */
extern OBS_WEAK const char obs_census_control_absent;

static obs_result check_control(void) {
    int found_present = obs_address_is_callable(&obs_census_control_present);
    int found_absent = obs_address_is_callable(&obs_census_control_absent);
    obs_report_symbol("obscene", "obs_census_control_present", found_present,
                      OBS_SHARED);
    obs_report_symbol("obscene", "obs_census_control_absent", found_absent, OBS_SHARED);

    if (!found_present) {
        return obs_fail("a symbol that is definitely present reported absent; "
                        "every count in this section is meaningless");
    }
    if (found_absent) {
        return obs_fail("a symbol that does not exist reported present; "
                        "every count in this section is meaningless");
    }
    return obs_pass();
}

/* Generates, for each group: the declarations, the table, and the check that walks
 * it. Adding a library is one line in surface.h and nothing here. */
#define OBS_DEFINE_GROUP(tag, library, availability, LIST)                             \
    LIST(OBS_DECLARE_SYMBOL)                                                           \
    static const obs_symbol tag##_symbols[] = {LIST(OBS_SYMBOL_ROW)};                  \
    static obs_result check_##tag(void) {                                              \
        return census(library, availability, tag##_symbols, OBS_COUNT(tag##_symbols)); \
    }

OBS_SURFACE_LIBRARIES(OBS_DEFINE_GROUP)

/* And the mined corpus, through the same macro.
 *
 * `corpus.h` is generated and declares its groups in exactly the shape `surface.h`
 * uses, so nothing here needs to know which of the two a group came from. That is the
 * whole reason the generator emits that shape: a second walk, a second table type or a
 * second check function would all be places for the two censuses to drift apart.
 *
 * They stay separate files because they are different kinds of thing - one curated and
 * annotated, one mined and mechanical (D105) - not because they behave differently. */
OBS_CORPUS_LIBRARIES(OBS_DEFINE_GROUP)

/* And the nameless ones, which need their own macros because a row carries two things.
 *
 * A named symbol is its own C declaration; an identifier is not a C name at all, so
 * each gets a generated one plus an assembler label carrying the real symbol. The label
 * is what the linker emits, what the manifest records and what mkmodule sees - the C
 * name exists only because C requires one.
 *
 * The label keeps its `$` in the reported name deliberately.
 * `sym|libSceFont|$Xh3kd9sLpQw` says what is true: something is exported here and this
 * project cannot name it. */
#define OBS_DECLARE_NID(symbol, label) extern OBS_WEAK const char symbol __asm__(label);
#if OBS_CENSUS_LINKED
#define OBS_NID_ROW(symbol, label) {label, (const void *)&symbol},
#else
#define OBS_NID_ROW(symbol, label) {label, NULL},
#endif
#define OBS_DEFINE_NID_GROUP(tag, library, availability, LIST)                         \
    LIST(OBS_DECLARE_NID)                                                              \
    static const obs_symbol tag##_symbols[] = {LIST(OBS_NID_ROW)};                     \
    static obs_result check_##tag(void) {                                              \
        return census(library, availability, tag##_symbols, OBS_COUNT(tag##_symbols)); \
    }

OBS_NID_LIBRARIES(OBS_DEFINE_NID_GROUP)

/* The census probes addresses rather than calling anything, so it
 * has no symbol of its own to guard and no prerequisites - it is
 * safe even when everything else has failed, which is exactly
 * when its answer is most useful. */
#define OBS_GROUP_ROW(tag, library, availability, LIST)                                \
    {"900-surface/" #tag, library,       "(census)",  OBS_CAP_NONE,                    \
     OBS_CAP_NONE,        OBS_NO_SYMBOL, check_##tag, OBS_FROM_ASSUMED},

/* Published by 007-responsive, which is the only part of this program that knows the
 * difference between a symbol existing and a symbol working. */
extern unsigned int obs_responsive_responding;
extern unsigned int obs_responsive_silent;

/* How much of this census is worth anything.
 *
 * # The problem it exists to state
 *
 * Presence is not behaviour. A platform resolving every symbol to a stub that returns
 * success scores full marks on everything above and is useless, and a census cannot
 * tell the two apart - it reads addresses and never calls anything (BACKLOG §7).
 *
 * That is not hypothetical. A previous-generation emulator reports every one of the
 * current generation's 87 graphics symbols as present, through a generic stub, for an
 * interface it does not implement at all.
 *
 * # Why it lives here rather than in a document
 *
 * A caveat in a section's purpose line is read once. A number in the report is read
 * every time somebody quotes a coverage figure, which is the moment the caveat matters.
 *
 * The verdict is deliberately never `pass`. This check does not test the platform - it
 * qualifies the rest of the section - so a green line would be one more thing to add
 * up. It reports `partial` with the responding count, and says plainly when the sample
 * says the platform is mostly stubs. */
static obs_result check_presence_is_not_behaviour(void) {
    unsigned int tested = obs_responsive_responding + obs_responsive_silent;
    if (tested == 0) {
        return obs_partial("nothing was probed for behaviour, so this census says only "
                           "which symbols resolve");
    }
    /* Integer arithmetic: this is freestanding and there is no floating point in the
     * runtime (CLAUDE.md principle 8). Tenths are enough to read a proportion by. */
    unsigned int tenths = (obs_responsive_responding * 10u) / tested;
    if (obs_responsive_responding == 0) {
        return obs_fail_code("every probed function is a stub; this census counts "
                             "resolution, not implementation",
                             (uint64_t)obs_responsive_silent);
    }
    if (tenths < 5u) {
        return obs_partial_value("most probed functions are stubs; read this census as "
                                 "an upper bound, not a coverage figure",
                                 (uint64_t)obs_responsive_responding);
    }
    return obs_partial_value(
        "presence is not behaviour; this census counts symbols that "
        "resolve, and only the sections above call anything",
        (uint64_t)obs_responsive_responding);
}

static const obs_check surface_checks[] = {
    /* First, so it is read before the numbers it validates. */
    {"900-surface/control", "obscene", "(census control)", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_control, OBS_FROM_ASSUMED},
    /* Second, so it qualifies the counts before they are read rather than after. */
    {"900-surface/presence-is-not-behaviour", "obscene", "(qualifier)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_presence_is_not_behaviour, OBS_FROM_SPEC},
    OBS_SURFACE_LIBRARIES(OBS_GROUP_ROW) OBS_CORPUS_LIBRARIES(OBS_GROUP_ROW)
        OBS_NID_LIBRARIES(OBS_GROUP_ROW)};

/* Reports which library each censused name belongs to.
 *
 * The module build needs this: an encoded import carries a library id, and an id with
 * no declared library resolves to nothing. The association exists only in the tables
 * above, so it is published from here rather than written down a second time and kept
 * in step by hand.
 *
 * A callback rather than a returned array because the tables are per-group statics
 * generated by a macro - there is no single array to hand back, and building one
 * would mean allocating, which this program does not do.
 */
#define OBS_WALK_GROUP(tag, library, availability, LIST)                               \
    for (unsigned int i = 0; i < OBS_COUNT(tag##_symbols); i++) {                      \
        fn(library, tag##_symbols[i].name);                                            \
    }

void obs_surface_each_symbol(void (*fn)(const char *library, const char *symbol)) {
#if !OBS_CENSUS_LINKED
    /* Nothing to walk: the tables hold one placeholder row apiece. This is only ever
     * called by the host build, which always links the census - but the eboot compiles
     * this file too, so it has to be correct rather than merely unreached. */
    (void)fn;
#else
    OBS_SURFACE_LIBRARIES(OBS_WALK_GROUP)
    /* The corpus too, or the module build imports none of it: this walk is what emits
     * the manifest `mkmodule` links against, so a symbol missing here is censused by
     * the host build and absent from the module entirely. */
    OBS_CORPUS_LIBRARIES(OBS_WALK_GROUP)
    OBS_NID_LIBRARIES(OBS_WALK_GROUP)
#endif
}

const obs_section obs_section_surface = {
    "900-surface",
    "Symbol census",
    "How much of the known surface exists, by library. Nothing "
    "is called, and the "
    "control check validates the presence test itself - read it "
    "first.",
    surface_checks,
    OBS_COUNT(surface_checks),
};
