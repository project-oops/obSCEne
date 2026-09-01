#include "obscene/report.h"
#include "obscene/runtime.h"
/* For the watch list the `resume` record carries. (D181) */
#include "obscene/sink.h"

/* Long enough for any line this program produces: the longest is a result line
 * carrying an identifier, a status, a number and a detail string. */
#define OBS_LINE_MAX 512

typedef struct line {
    char buf[OBS_LINE_MAX];
    size_t len;
} line;

static void line_start(line *l, const char *kind) {
    l->len = 0;
    for (const char *p = OBS_PREFIX; *p != '\0'; p++) {
        l->buf[l->len++] = *p;
    }
    l->buf[l->len++] = OBS_SEP;
    for (const char *p = kind; *p != '\0' && l->len < OBS_LINE_MAX - 1; p++) {
        l->buf[l->len++] = *p;
    }
}

static void line_field(line *l, const char *text) {
    if (l->len < OBS_LINE_MAX - 1) {
        l->buf[l->len++] = OBS_SEP;
    }
    if (text == NULL) {
        return;
    }
    for (const char *p = text; *p != '\0' && l->len < OBS_LINE_MAX - 1; p++) {
        /* A separator inside a field would silently shift every field after it, so
         * it is replaced rather than escaped. Escaping would need a matching
         * unescape in every parser; substitution needs none. */
        l->buf[l->len++] = (*p == OBS_SEP || *p == '\n') ? ' ' : *p;
    }
}

static void line_field_u64(line *l, uint64_t value) {
    if (l->len < OBS_LINE_MAX - 1) {
        l->buf[l->len++] = OBS_SEP;
    }
    if (l->len + OBS_NUM_MAX < OBS_LINE_MAX) {
        l->len += obs_format_u64(l->buf + l->len, value);
    }
}

static void line_field_hex(line *l, uint64_t value) {
    if (l->len < OBS_LINE_MAX - 1) {
        l->buf[l->len++] = OBS_SEP;
    }
    if (l->len + OBS_NUM_MAX < OBS_LINE_MAX) {
        l->len += obs_format_hex(l->buf + l->len, value);
    }
}

static void line_end(line *l) {
    l->buf[l->len++] = '\n';
    obs_write(l->buf, l->len);
}

void obs_report_meta(unsigned int sections, unsigned int checks) {
    line l;
    line_start(&l, "meta");
    line_field_u64(&l, OBS_FORMAT_VERSION);
    line_field_u64(&l, sections);
    line_field_u64(&l, checks);
    line_end(&l);
}

void obs_report_build(void) {
    line l;
    line_start(&l, "build");
    line_field(&l, OBSCENE_BUILD_ID);
    line_field(&l, OBSCENE_TARGET);
    line_end(&l);
}

void obs_report_context(const char *name, const char *basis) {
    /* The execution context a run measured in - "<delivery>/<generation>" - emitted next to
     * `build` because it is the other half of "where did this come from": `build` says how the
     * binary was made, `context` says the environment it ran in. Two payload runs a reader
     * cannot otherwise tell apart - one in the ps4 compatibility host, one injected into a
     * native process - differ only here, because they are the same binary. Distinct from a
     * check's OBS_FROM_* tag, which is the provenance of the expectation, not of the
     * measurement. Derived by obs_run_context from the build, the payload anchor and the
     * link-map. */
    line l;
    line_start(&l, "context");
    line_field(&l, name);
    line_field(&l, basis);
    line_end(&l);
}

void obs_report_gpu_device(const char *backend, const char *device, const char *type) {
    line l;
    line_start(&l, "gpudev");
    line_field(&l, backend);
    line_field(&l, device);
    line_field(&l, type);
    line_end(&l);
}

void obs_report_gpu(const char *kernel, unsigned int lane, uint32_t input,
                    uint32_t output) {
    line l;
    line_start(&l, "gpu");
    line_field(&l, kernel);
    line_field_u64(&l, lane);
    line_field_hex(&l, input);
    line_field_hex(&l, output);
    line_end(&l);
}

void obs_report_gpu_op(const char *kernel, unsigned int lane, uint32_t output,
                       const uint32_t *inputs, unsigned int input_count) {
    line l;
    line_start(&l, "gpuop");
    line_field(&l, kernel);
    line_field_u64(&l, lane);
    line_field_hex(&l, output);
    /* The inputs trail the fixed fields, one hex field each. A reader that knows the arity
     * takes that many; one that does not reads to end of line. */
    for (unsigned int i = 0; i < input_count; i++) {
        line_field_hex(&l, inputs[i]);
    }
    line_end(&l);
}

void obs_report_net(const char *state, unsigned int port) {
    line l;
    line_start(&l, "net");
    line_field(&l, state);
    line_field_u64(&l, port);
    line_end(&l);
}

void obs_report_sysinfo(const char *field, const char *state, const char *value) {
    line l;
    line_start(&l, "sysinfo");
    line_field(&l, field);
    line_field(&l, state);
    line_field(&l, value);
    line_end(&l);
}

void obs_report_sink(const char *path) {
    /* "none" rather than an empty field when there is no sink.
     *
     * An empty field reads as "this build does not emit that column", which is what a
     * reader of an older report would correctly conclude. A word says the sink was
     * attempted and did not open, and those are different runs. */
    line l;
    line_start(&l, "sink");
    line_field(&l, path != NULL ? path : "none");
    line_end(&l);
}

void obs_report_resume(unsigned int skipped, int overflowed) {
    /* Emitted whether or not anything was carried, because "nothing was skipped" and "this
     * build cannot skip" are different runs and a reader cannot tell them apart from silence.
     *
     * The overflow flag is the important half. The skip set is a fixed table, and a run that
     * filled it would quietly stop learning - which is exactly the oscillation the set exists
     * to prevent, returning without a symptom. Reported so it cannot happen unnoticed. */
    line l;
    line_start(&l, "resume");
    line_field_u64(&l, skipped);
    line_field(&l, overflowed ? "full" : "ok");
    /* Then every check under watch, by id.
     *
     * These are the ones that failed to return once and are being tried again rather than
     * skipped, and this record is the only place the set can live: the report *is* the state
     * file, there is no second one, and a check that is being retried emits an ordinary result
     * rather than a skip - so nothing else in the stream would carry it forward. (D181)
     *
     * Appended to the end of the line, which is what the format permits without a version
     * bump: a reader that does not know about these stops at the two fields it does. */
    for (unsigned int w = 0; w < obs_resume_watched_count(); w++) {
        const char *id = obs_resume_watched(w);
        if (id != 0) {
            line_field(&l, id);
        }
    }
    line_end(&l);
}

void obs_report_display(const char *state, const char *detail, uint64_t code) {
    /* Whether the screen can be believed.
     *
     * The drawn report is a second, independent report, and a reader looking at a
     * photograph of a screen has no way to tell "these are the results" from "the
     * display never came up and this is a stale frame". The stream says which.
     *
     * The code is the platform's own account of a refusal, appended after the existing
     * fields so a parser written against the earlier shape keeps working. `detail` names the
     * step that refused and is this program's sentence; the code is the only part a reader
     * can look up or compare between two consoles, and the display path threw it away at
     * seven separate sites. Zero where no call reported one. (D249) */
    line l;
    line_start(&l, "display");
    line_field(&l, state);
    line_field(&l, detail);
    line_field_hex(&l, code);
    line_end(&l);
}

void obs_report_responsive(const char *library, const char *symbol, const char *verdict,
                           uint64_t observed) {
    /* Whether one function reads its arguments.
     *
     * A record per symbol rather than a verdict per section, for the same reason the
     * census emits one: this is an inventory, and a tool comparing two platforms wants
     * to diff the lists. The observed value is carried because a stub's constant answer
     * is worth seeing - a library that returns zero to everything and one that returns
     * the same wrong number to everything are different kinds of unfinished. */
    line l;
    line_start(&l, "responsive");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field(&l, verdict);
    line_field_hex(&l, observed);
    line_end(&l);
}


void obs_report_call(const char *library, const char *symbol, uint64_t index,
                     const char *outcome, uint64_t returned) {
    /* Library and symbol on every record rather than only on the attempt.
     *
     * The two records are separated by the call, and the call is the thing that may not
     * return - so a run's output can end between them, and every field the second record
     * would have carried has to be recoverable from the first alone. Repeating two strings
     * is what makes the truncated case readable. */
    line l;
    line_start(&l, "call");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field_hex(&l, index);
    line_field(&l, outcome);
    line_field_hex(&l, returned);
    line_end(&l);
}

void obs_report_measure(const char *id, const char *symbol, const char *quantity,
                        uint64_t value, const char *unit) {
    /* One record per number, rather than one per measurement with several columns.
     *
     * A measurement grows: today a sleep records what was asked and what elapsed, and
     * on hardware it will want the same from a second clock. Appending a column would
     * change the shape of every existing record; appending a row does not, and the
     * report is a contract (docs/OUTPUT.md). */
    line l;
    line_start(&l, "measure");
    line_field(&l, id);
    line_field(&l, symbol);
    line_field(&l, quantity);
    line_field_hex(&l, value);
    line_field(&l, unit);
    line_end(&l);
}

void obs_report_progress(const char *id, uint64_t reached) {
    /* How far a looping check has got.
     *
     * Announce-before-attempting names the call that did not return, which is enough
     * when a check makes one call. A check that makes forty says only "this one", and
     * the difference between failing on the first iteration and the thirty-ninth is the
     * difference between "broken" and "breaks under churn" - which is the entire point
     * of a churn check.
     *
     * Emitted sparsely: this is a position, not a trace. */
    line l;
    line_start(&l, "progress");
    line_field(&l, id);
    line_field_u64(&l, reached);
    line_end(&l);
}

void obs_report_module(const char *name, uint64_t handle) {
    /* One loaded module, as the platform names it.
     *
     * A record of its own rather than a check result, for the same reason `sym` is:
     * these are an inventory, not a verdict, and there may be dozens. A tool comparing
     * two platforms wants to diff the lists, which needs them as data. */
    line l;
    line_start(&l, "module");
    line_field(&l, name);
    line_field_hex(&l, handle);
    line_end(&l);
}

void obs_report_module_word(unsigned int offset, uint64_t value) {
    /* One word from the front of a structure whose layout is not known.
     *
     * Emitted only when the assumed layout fails, so a normal run never carries these.
     * The point is to derive an offset from what a platform actually wrote rather than
     * to try guesses until one produces something plausible - a guess that lands on
     * printable bytes reads as a name and is not one. */
    line l;
    line_start(&l, "moduleword");
    line_field_u64(&l, offset);
    line_field_hex(&l, value);
    line_end(&l);
}

void obs_report_section(const obs_section *section) {
    line l;
    line_start(&l, "section");
    line_field(&l, section->id);
    line_field(&l, section->title);
    line_field(&l, section->purpose);
    line_end(&l);
}

void obs_report_attempt(const obs_check *check) {
    /* Emitted before the call, and written straight through with no buffering. If
     * the platform function takes the process down, this line is the last thing on
     * the stream and it names the exact call that did it. That is the whole reason
     * this function exists separately from obs_report_result. */
    line l;
    line_start(&l, "try");
    line_field(&l, check->id);
    line_field(&l, check->library);
    line_field(&l, check->symbol);
    line_end(&l);
}

void obs_report_result(const obs_check *check, obs_result result) {
    line l;
    line_start(&l, "res");
    line_field(&l, check->id);
    line_field(&l, obs_status_name(result.status));
    if (result.has_value) {
        line_field_hex(&l, result.value);
    } else {
        line_field(&l, "");
    }
    line_field(&l, result.detail);
    /* Appended after the fact, which the format allows: new fields go on the end of a
     * line and an older reader stops before this one. It says how much the verdict is
     * worth - a FAIL against a standard is the platform's problem, a FAIL against an
     * assumption might be ours. */
    line_field(&l, obs_provenance_name(check->from));
    line_end(&l);
}

void obs_report_symbol(const char *library, const char *symbol, int present,
                       obs_availability availability) {
    line l;
    line_start(&l, "sym");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field(&l, present ? "present" : "absent");
    /* Appended after the existing fields, so a parser written against the earlier
     * shape keeps working - the format allows growth only at the end of a line. */
    line_field(&l, obs_availability_name(availability));
    line_end(&l);
}

void obs_report_section_tally(const obs_section *section, obs_tally tally) {
    line l;
    line_start(&l, "sectiontally");
    line_field(&l, section->id);
    line_field_u64(&l, tally.pass);
    line_field_u64(&l, tally.partial);
    line_field_u64(&l, tally.fail);
    line_field_u64(&l, tally.skip);
    line_end(&l);
}

/* Sixteen bytes per record. Short enough that a line stays readable on the drawn screen
 * and in a terminal, and a round number so offsets line up when several are read
 * together. */
#define OBS_BYTES_PER_RECORD 16u

void obs_report_bytes(const char *id, const char *symbol, const char *what,
                      unsigned int offset, const unsigned char *bytes,
                      unsigned int len) {
    static const char digits[] = "0123456789abcdef";
    char hex[OBS_BYTES_PER_RECORD * 2u + 1u];
    unsigned int n = 0;
    for (unsigned int i = 0; i < len && i < OBS_BYTES_PER_RECORD; i++) {
        hex[n++] = digits[(bytes[i] >> 4) & 0x0Fu];
        hex[n++] = digits[bytes[i] & 0x0Fu];
    }
    hex[n] = '\0';

    line l;
    line_start(&l, "bytes");
    line_field(&l, id);
    line_field(&l, symbol);
    line_field(&l, what);
    line_field_u64(&l, offset);
    line_field(&l, hex);
    line_end(&l);
}

void obs_report_buffer(const char *id, const char *symbol, const char *what,
                       const unsigned char *bytes, unsigned int len) {
    /* The extent first, because it is the single most useful number here: a caller who
     * reads nothing else learns how many bytes the platform touched. Zero means the call
     * wrote nothing, which is a finding rather than an empty result. */
    unsigned int extent = 0;
    for (unsigned int i = 0; i < len; i++) {
        if (bytes[i] != 0) {
            extent = i + 1u;
        }
    }
    obs_report_bytes(id, symbol, "extent", extent, (const unsigned char *)"", 0);
    for (unsigned int off = 0; off < extent; off += OBS_BYTES_PER_RECORD) {
        unsigned int remaining = extent - off;
        obs_report_bytes(id, symbol, what, off, &bytes[off],
                         remaining < OBS_BYTES_PER_RECORD ? remaining
                                                          : OBS_BYTES_PER_RECORD);
    }
}

void obs_report_written(const char *id, const char *symbol, const char *what,
                        const unsigned char *before, const unsigned char *after,
                        unsigned int len) {
    /* The extent is the last byte that *changed*, not the last that is non-zero.
     *
     * That is the whole difference from `obs_report_buffer`, and it is worth a second
     * function: under the other rule a structure whose final field is a zeroed reserved word
     * reads as shorter than it is, and a call that writes nothing but zeroes reads as a call
     * that writes nothing at all - which is a finding, and the wrong one. */
    unsigned int extent = 0;
    unsigned int changed = 0;
    for (unsigned int i = 0; i < len; i++) {
        if (before[i] != after[i]) {
            extent = i + 1u;
            changed++;
        }
    }
    obs_report_bytes(id, symbol, "extent", extent, (const unsigned char *)"", 0);
    obs_report_bytes(id, symbol, "changed", changed, (const unsigned char *)"", 0);
    /* Untouched runs inside the extent are reported, because a hole is a fact a hexdump
     * cannot show. A structure the call leaves alone at offsets 8..15 says something about
     * alignment or about a member it does not own, and the values alone look continuous. */
    unsigned int run = 0;
    for (unsigned int i = 0; i < extent; i++) {
        if (before[i] == after[i]) {
            run++;
        } else if (run != 0) {
            obs_report_bytes(id, symbol, "untouched", i - run, (const unsigned char *)"", run);
            run = 0;
        }
    }
    for (unsigned int off = 0; off < extent; off += OBS_BYTES_PER_RECORD) {
        unsigned int remaining = extent - off;
        obs_report_bytes(id, symbol, what, off, &after[off],
                         remaining < OBS_BYTES_PER_RECORD ? remaining
                                                          : OBS_BYTES_PER_RECORD);
    }
}

void obs_report_size(const char *library, const char *symbol, unsigned int argument,
                     unsigned int size, int accepted, uint64_t code) {
    line l;
    line_start(&l, "size");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field_u64(&l, argument);
    line_field_u64(&l, size);
    line_field(&l, accepted ? "accepted" : "rejected");
    line_field_hex(&l, code);
    line_end(&l);
}

void obs_report_resolve(const char *library, const char *symbol, int present,
                        uint64_t address) {
    line l;
    line_start(&l, "resolve");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field(&l, present ? "present" : "absent");
    line_field_hex(&l, address);
    line_end(&l);
}

void obs_report_error_code(const char *library, const char *symbol, const char *argument,
                           uint64_t returned) {
    line l;
    line_start(&l, "err");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field(&l, argument);
    line_field_hex(&l, returned);
    line_end(&l);
}

void obs_report_region(unsigned int index, uint64_t first, uint64_t second,
                       int progressing) {
    line l;
    line_start(&l, "region");
    line_field_u64(&l, index);
    line_field_hex(&l, first);
    line_field_hex(&l, second);
    line_field(&l, progressing ? "advanced" : "stalled");
    line_end(&l);
}

void obs_report_frontier(unsigned int established, unsigned int blocked,
                         unsigned int deepest_section) {
    /* Three numbers, and the middle one is the point: checks that never ran because a
     * capability they needed was never established. That is the count of the suite
     * sitting behind the floor, and it is the number an emulator author can move. */
    line l;
    line_start(&l, "frontier");
    line_field_u64(&l, established);
    line_field_u64(&l, blocked);
    line_field_u64(&l, deepest_section);
    line_end(&l);
}

void obs_report_tally(obs_tally tally) {
    line l;
    line_start(&l, "tally");
    line_field_u64(&l, tally.pass);
    line_field_u64(&l, tally.partial);
    line_field_u64(&l, tally.fail);
    line_field_u64(&l, tally.skip);
    line_end(&l);
}

void obs_report_end(void) {
    line l;
    line_start(&l, "end");
    /* Which way the report got out.
     *
     * Here rather than in the build record because it is not known until something has
     * been written, and the build record is the first thing written. By the last line
     * it is settled.
     *
     * Worth a field of its own: a run that fell back to one character at a time has
     * said that the platform does not implement its ordinary write, which is a result
     * about the platform and not a detail of this program. A run reporting "none"
     * cannot be read at all, so that value only ever appears on a host build. */
    line_field(&l, obs_output_channel_name());
    line_end(&l);
}

void obs_report_import(const char *library, const char *symbol, int linked, int resolvable) {
    /* One of this program's own imports, on two axes that have to be separate.
     *
     * `linked` is whether the loader bound the import slot when the module was loaded.
     * `resolvable` is whether the same name, in the same library, comes back from a
     * run-time lookup. **Neither answers the other**, and the whole reason this record
     * exists is that they disagreed:
     *
     *   linked, resolvable          ordinary - the call works
     *   not linked, resolvable      the symbol is there and OUR import did not bind.
     *                               A defect in what this module declares, and fixable
     *                               here. Twenty-four checks sat behind this.
     *   not linked, not resolvable  the platform does not offer it under that name in
     *                               that library. A finding about the platform, or a
     *                               wrong library in `imports.c`.
     *   linked, not resolvable      the run-time resolver is weaker than the loader,
     *                               which says something about the resolver only.
     *
     * The census (`sym`) cannot carry this. A censused name is declared as data so it
     * can never be called, and this program's own imports are declared as functions in
     * `platform.h` - a name cannot be in both places, so the symbols whose status matters
     * most were exactly the ones the census could not see. (D240) */
    line l;
    line_start(&l, "import");
    line_field(&l, library);
    line_field(&l, symbol);
    line_field(&l, linked ? "linked" : "unlinked");
    line_field(&l, resolvable ? "resolvable" : "unresolvable");
    line_end(&l);
}
