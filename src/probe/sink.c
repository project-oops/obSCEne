/*
 * The report, on disk.
 *
 * # Why a fifth channel, when four already work
 *
 * The four in `runtime.c` are **alternatives** - the first that moves a byte is chosen
 * and the rest are never tried again. This is not one of them. It is a second
 * *destination*, written to in addition to whichever text channel was picked.
 *
 * The distinction matters because the two failure modes are opposite. A text channel
 * answers "can this system talk to a terminal I am watching", and on a console the
 * answer is often no. A file answers "can this system leave something behind", and that
 * is the question that matters when the run is over and the console is across the room.
 *
 * The failure mode this exists to prevent is easy to picture: the hardware arrives, a
 * hundred probes run, everything works - and the findings exist as a photograph of a
 * television. Every other capability on this page is worth more once the output is
 * ingestible and much less before.
 *
 * # Written as it goes, never buffered
 *
 * Each record reaches the file at the moment it is produced. Buffering would be faster
 * and would break the one property this program is built on: a report that stops
 * mid-record names the call that ended the run (CLAUDE.md, principle 1). A buffered
 * file loses exactly the records that matter most, because a crash discards the buffer
 * and the crash is the finding.
 *
 * So: no accumulation, no flush-at-exit, no "write it all at the end". The cost is a
 * system call per record and it is worth paying.
 *
 * # The path is discovered, not assumed
 *
 * Which directory a module may write to differs by platform, by generation, and by how
 * the module was launched. Rather than hard-code one and fail silently when it is
 * wrong, the candidates are tried in order and **the one that worked is reported**.
 *
 * That last part is the point. A run whose file sink silently failed and a run with no
 * file sink configured look identical afterwards, and the difference is whether the
 * findings exist. The `sink` record removes the ambiguity in the direction that
 * matters: a reader of the text report can see whether a file was also written, and
 * where.
 */

#include "obscene/harness.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sink.h"

/* Where the report is written, in order of preference.
 *
 * `/data` first because it is the conventional writable location for an unsigned
 * module, and a bare filename last because the directory a module was launched from is
 * usually read-only.
 *
 * None of these is asserted to exist. The list is a set of guesses and the `sink`
 * record says which one was right, which turns the guessing into a measurement. */
static const char *const obs_sink_paths[] = {
    "/mnt/usb0/obscene/report.txt",  "/mnt/usb0/obscene-report.txt",
    "/mnt/usb1/obscene/report.txt",  "/mnt/usb1/obscene-report.txt",
    "/data/obscene/report.txt",      "/data/obscene-report.txt",
    "/download0/obscene-report.txt", "obscene-report.txt",
};

/* Sinks: primary snapshot (for resume/tooling) and timestamped archive (for
 * non-clobbering runs). */
static int obs_sink_fd = -1;
static int obs_sink_ts_fd = -1;
static int obs_sink_tried = 0;
static char obs_sink_reported_path[128];

static void obs_format_ts_path(char *dest, size_t max_len, const char *prefix,
                               uint64_t ts, const char *suffix) {
    size_t p = 0;
    while (prefix[p] != '\0' && p + 1 < max_len) {
        dest[p] = prefix[p];
        p++;
    }
    if (p + OBS_NUM_MAX < max_len) {
        p += obs_format_u64(dest + p, ts);
    }
    size_t s = 0;
    while (suffix[s] != '\0' && p + 1 < max_len) {
        dest[p++] = suffix[s++];
    }
    dest[p] = '\0';
}

const char *obs_sink_open(void) {
    if (obs_sink_tried) {
        return obs_sink_reported_path[0] != '\0' ? obs_sink_reported_path : NULL;
    }
    obs_sink_tried = 1;

    /* Ensure dedicated persistent directories exist if supported */
    (void)obs_sink_backend_mkdir("/mnt/usb0/obscene");
    (void)obs_sink_backend_mkdir("/mnt/usb1/obscene");
    (void)obs_sink_backend_mkdir("/data/obscene");

    /* Try to open a timestamped archive sink first so this run never clobbers previous
     * ones */
    uint64_t ts = obs_sink_backend_time();
    if (ts > 0) {
        static const char *const ts_prefixes[] = {
            "/mnt/usb0/obscene/report-", "/mnt/usb0/obscene-report-",
            "/mnt/usb1/obscene/report-", "/mnt/usb1/obscene-report-",
            "/data/obscene/report-",     "/data/obscene-report-",
            "obscene-report-",
        };
        char ts_candidate[128];
        for (unsigned int i = 0; i < OBS_COUNT(ts_prefixes); i++) {
            obs_format_ts_path(ts_candidate, sizeof ts_candidate, ts_prefixes[i], ts,
                               ".txt");
            int fd = obs_sink_backend_open(ts_candidate);
            if (fd >= 0) {
                obs_sink_ts_fd = fd;
                break;
            }
        }
    }

    /* Open the primary/latest snapshot sink */
    for (unsigned int i = 0; i < OBS_COUNT(obs_sink_paths); i++) {
        int fd = obs_sink_backend_open(obs_sink_paths[i]);
        if (fd >= 0) {
            obs_sink_fd = fd;
            size_t n = 0;
            while (obs_sink_paths[i][n] != '\0' &&
                   n + 1 < sizeof obs_sink_reported_path) {
                obs_sink_reported_path[n] = obs_sink_paths[i][n];
                n++;
            }
            obs_sink_reported_path[n] = '\0';
            return obs_sink_reported_path;
        }
    }

    /* If primary failed but timestamped succeeded, report the timestamped path */
    if (obs_sink_ts_fd >= 0) {
        return "timestamped-archive";
    }

    return NULL;
}

void obs_sink_write(const char *bytes, size_t len) {
    if (obs_sink_fd < 0 && obs_sink_ts_fd < 0) {
        return;
    }
    if (obs_sink_fd >= 0) {
        size_t sent = 0;
        while (sent < len) {
            long n = obs_sink_backend_write(obs_sink_fd, bytes + sent, len - sent);
            if (n <= 0) {
                obs_sink_backend_close(obs_sink_fd);
                obs_sink_fd = -1;
                break;
            }
            sent += (size_t)n;
        }
    }
    if (obs_sink_ts_fd >= 0) {
        size_t sent = 0;
        while (sent < len) {
            long n = obs_sink_backend_write(obs_sink_ts_fd, bytes + sent, len - sent);
            if (n <= 0) {
                obs_sink_backend_close(obs_sink_ts_fd);
                obs_sink_ts_fd = -1;
                break;
            }
            sent += (size_t)n;
        }
    }
}

void obs_sink_close(void) {
    if (obs_sink_fd >= 0) {
        obs_sink_backend_close(obs_sink_fd);
        obs_sink_fd = -1;
    }
    if (obs_sink_ts_fd >= 0) {
        obs_sink_backend_close(obs_sink_ts_fd);
        obs_sink_ts_fd = -1;
    }
}

int obs_sink_is_open(void) {
    return obs_sink_fd >= 0 || obs_sink_ts_fd >= 0;
}

/* ---- what the last run did not finish
 * ----------------------------------------------------
 *
 * # Why a compile-time exclusion list was the wrong shape
 *
 * A check that ends the process takes every check behind it with it, so a loader that
 * hangs at `strlen` reports six results and stops. The answer was `-DOBSCENE_EXCLUDE`,
 * a list of ids baked in at build time, and it works: fpPS4 needs 44 and then runs to
 * the end.
 *
 * What it costs is the thing this program is for. **Each loader needed a different
 * module**, so "the same binary behaves differently on shadPS4 and fpPS4" stopped being
 * a measurement and became a build difference, and a reader comparing two reports was
 * comparing two programs. The lists drift, too: they live beside the module, and one
 * loader's list silently became another's more than once.
 *
 * # The report is already the state file
 *
 * `obs_sink_write` puts every record on disk *before* the risky call that follows it -
 * the same durable-write-ahead-of-risky ordering as announce-before-attempting. So a
 * report from a run that died already says which check was in flight: a `try` with no
 * `res`.
 *
 * That is the whole mechanism. Read the previous report, find the dangling
 * announcement, skip that one check, run everything else. Two runs of **one binary**
 * get past one crash; N runs get past N. No rebuild, no list, nothing loader-specific
 * in the module.
 *
 * # Read before the sink truncates
 *
 * `obs_sink_backend_open` opens `O_TRUNC`, so the previous report is gone the moment
 * the sink opens. This runs first, and `start.c` calls it before `obs_sink_open` for
 * that reason alone.
 *
 * # What it deliberately does not do
 *
 * It does not accumulate a list. One dangling id per run, because that is what the
 * evidence supports - a report shows one check in flight when the process ended.
 * Successive runs converge anyway: each skips the one it learned and discovers the
 * next.
 *
 * It does not carry across a rebuild. The build id is in the report, and a run whose
 * build differs is not evidence about this one; a changed check is exactly when a stale
 * skip would be worst. (D172)
 */

/* Sized to the longest check id with room over. A longer one is refused rather than
 * truncated: a truncated id matches by prefix, and would skip a check nobody excluded.
 */
#define OBS_RESUME_ID_MAX 96

/* How many checks can be skipped across a series of runs.
 *
 * **A single id is not enough, and assuming it was cost a wrong claim.** The first
 * version remembered only the dangling announcement and its comment said successive
 * runs would "converge anyway". They do not - they oscillate. Against shadPS4: run one
 * died at `040-file/open-rejects-null`, run two skipped it and died at
 * `080-video/flip-rate-rejects-bad-handle`, and run three forgot the first and died
 * there again. Each run discovered one thing and lost the last one.
 *
 * So the set accumulates, and it accumulates **through the report** rather than through
 * a second file: a run records every check it skipped for this reason, so the next run
 * reads its own predecessor's decisions back out and adds the new discovery to them.
 *
 * Sized for the worst loader measured - fpPS4 needs 44 - with room over. A run needing
 * more stops adding rather than overwriting, and says so, because a silently dropped
 * entry would bring back the oscillation this exists to fix. */
#define OBS_RESUME_MAX 96
static char obs_resume_ids[OBS_RESUME_MAX][OBS_RESUME_ID_MAX];
static unsigned int obs_resume_count;
static int obs_resume_full;
static int obs_resume_build_matched;
static unsigned int obs_resume_checks_seen;
/* The most recent announcement, promoted to a real entry only if the file ends without
 * its answer. */
static char obs_resume_pending[OBS_RESUME_ID_MAX];

/* Checks that failed to return **once**, which is not yet enough to skip one.
 *
 * # Why one observation is not evidence
 *
 * A dangling `try` means "did not return", and two very different things produce it: a
 * check that hangs, and a check that was still running when something killed the
 * process. The report cannot tell them apart, so acting on the first sighting treats an
 * accident as a fact.
 *
 * Measured, on shadPS4, eight consecutive runs of one binary: two completed with 36,341
 * records, the third crashed inside `900-surface/videoout`, and every run after it
 * reported
 * **`complete`** with 20,342. One intermittent crash had skipped two corpus groups, and
 * those two carry sixteen thousand symbols between them. The tally, the check count and
 * the verdict were identical before and after; only the raw record count moved, which
 * nobody compares. Deleting the resume file restored it exactly. (D181)
 *
 * So a check has to fail to return on **two consecutive runs** before it is skipped. A
 * genuine hang does that every time and is skipped on the second run; a one-off crash
 * or a timeout does not, and is forgotten the moment the check answers again.
 *
 * The cost is one extra run per blocker, paid in machine time. The alternative is a
 * report that says `complete` while missing half its measurements, which is the failure
 * this whole program exists to make impossible. */
static char obs_resume_watch[OBS_RESUME_MAX][OBS_RESUME_ID_MAX];
static unsigned int obs_resume_watch_count;

unsigned int obs_resume_watched_count(void) {
    return obs_resume_watch_count;
}

const char *obs_resume_watched(unsigned int index) {
    if (index >= obs_resume_watch_count) {
        return 0;
    }
    return obs_resume_watch[index];
}

/* Whether `id` already failed to return on the run before this one. */
static int obs_resume_is_watched(const char *id) {
    for (unsigned int s = 0; s < obs_resume_watch_count; s++) {
        const char *held = obs_resume_watch[s];
        size_t i = 0;
        while (held[i] != '\0' && id[i] != '\0' && held[i] == id[i]) {
            i++;
        }
        if (held[i] == '\0' && id[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

static void obs_resume_watch_add(const char *from, size_t n) {
    if (n == 0 || n >= OBS_RESUME_ID_MAX || obs_resume_watch_count >= OBS_RESUME_MAX) {
        return;
    }
    char candidate[OBS_RESUME_ID_MAX];
    for (size_t i = 0; i < n; i++) {
        candidate[i] = from[i];
    }
    candidate[n] = '\0';
    if (obs_resume_is_watched(candidate)) {
        return;
    }
    for (size_t i = 0; i <= n; i++) {
        obs_resume_watch[obs_resume_watch_count][i] = candidate[i];
    }
    obs_resume_watch_count++;
}

/* Answered, so whatever happened last time was not a hang. Dropped rather than kept,
 * which is what makes the rule *two consecutive* rather than *two ever*: an
 * intermittent fault has to recur immediately to count. */
static void obs_resume_watch_drop(const char *from, size_t n) {
    if (n == 0 || n >= OBS_RESUME_ID_MAX) {
        return;
    }
    for (unsigned int s = 0; s < obs_resume_watch_count; s++) {
        const char *held = obs_resume_watch[s];
        size_t i = 0;
        while (i < n && held[i] != '\0' && held[i] == from[i]) {
            i++;
        }
        if (i == n && held[i] == '\0') {
            for (unsigned int m = s + 1; m < obs_resume_watch_count; m++) {
                for (size_t k = 0; k < OBS_RESUME_ID_MAX; k++) {
                    obs_resume_watch[m - 1][k] = obs_resume_watch[m][k];
                }
            }
            obs_resume_watch_count--;
            return;
        }
    }
}

unsigned int obs_resume_skipped_count(void) {
    return obs_resume_count;
}

int obs_resume_overflowed(void) {
    return obs_resume_full;
}

int obs_resume_is_skipped(const char *id) {
    for (unsigned int s = 0; s < obs_resume_count; s++) {
        const char *held = obs_resume_ids[s];
        size_t i = 0;
        while (held[i] != '\0' && id[i] != '\0' && held[i] == id[i]) {
            i++;
        }
        if (held[i] == '\0' && id[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

/* Add `n` bytes of `from` to the set, ignoring a duplicate or an id that will not fit.
 */
static void obs_resume_remember(const char *from, size_t n) {
    if (n == 0 || n >= OBS_RESUME_ID_MAX) {
        return;
    }
    if (obs_resume_count >= OBS_RESUME_MAX) {
        obs_resume_full = 1;
        return;
    }
    char candidate[OBS_RESUME_ID_MAX];
    for (size_t i = 0; i < n; i++) {
        candidate[i] = from[i];
    }
    candidate[n] = '\0';
    if (obs_resume_is_skipped(candidate)) {
        return;
    }
    for (size_t i = 0; i <= n; i++) {
        obs_resume_ids[obs_resume_count][i] = candidate[i];
    }
    obs_resume_count++;
}

/* One record from the previous report.
 *
 * `try` remembers, `res` clears, `meta` confirms the build. Reading the record kind by
 * its first two fields rather than parsing the whole line: this runs before anything
 * else and on a platform that may be about to crash, so it does the least it can.
 */
static void obs_resume_consider(const char *line, const char *build_id) {
    /* `OBS|try|<id>|...` and `OBS|res|<id>|...` */
    if (line[0] != 'O' || line[1] != 'B' || line[2] != 'S' || line[3] != '|') {
        return;
    }
    const char *kind = line + 4;
    const char *rest = kind;
    while (*rest != '\0' && *rest != '|') {
        rest++;
    }
    if (*rest != '|') {
        return;
    }
    size_t kind_len = (size_t)(rest - kind);
    const char *id = rest + 1;
    size_t id_len = 0;
    while (id[id_len] != '\0' && id[id_len] != '|') {
        id_len++;
    }

    /* `try` is a candidate: it becomes a real entry only if no `res` follows. Held
     * aside rather than added, because most announcements are answered a moment later.
     */
    if (kind_len == 3 && kind[0] == 't' && kind[1] == 'r' && kind[2] == 'y') {
        if (id_len < OBS_RESUME_ID_MAX) {
            for (size_t i = 0; i < id_len; i++) {
                obs_resume_pending[i] = id[i];
            }
            obs_resume_pending[id_len] = '\0';
        } else {
            obs_resume_pending[0] = '\0';
        }
        return;
    }
    if (kind_len == 3 && kind[0] == 'r' && kind[1] == 'e' && kind[2] == 's') {
        /* Answered, so the announcement was not the end of the run. */
        obs_resume_pending[0] = '\0';
        /* And it answered, so it is not hanging. A watched id that returns is dropped,
         * which is what makes the rule two *consecutive* failures. */
        obs_resume_watch_drop(id, id_len);
        /* But a check the *previous* run skipped for this reason stays skipped. This is
         * how the set accumulates without a second file: each report carries its
         * predecessor's decisions, and reading them back is what stops the oscillation
         * a single remembered id produced. Matched on the detail text the harness
         * writes, which is the same coupling `compat` uses and for the same reason -
         * the format already carries the distinction. */
        const char *scan = id;
        while (*scan != '\0') {
            if (scan[0] == 'd' && scan[1] == 'i' && scan[2] == 'd' && scan[3] == ' ' &&
                scan[4] == 'n' && scan[5] == 'o' && scan[6] == 't' && scan[7] == ' ' &&
                scan[8] == 'r' && scan[9] == 'e' && scan[10] == 't') {
                obs_resume_remember(id, id_len);
                return;
            }
            scan++;
        }
        return;
    }
    /* `OBS|resume|<skipped>|<ok|full>|<watched id>...` - the watch list the previous
     * run left behind. Everything from the third field on is an id that failed to
     * return once and was retried rather than skipped; if it fails again this run, that
     * is the second consecutive sighting and it gets skipped. (D181) */
    if (kind_len == 6 && kind[0] == 'r' && kind[1] == 'e' && kind[2] == 's' &&
        kind[3] == 'u' && kind[4] == 'm' && kind[5] == 'e') {
        /* `id` is the count field here; the ids begin after the status that follows it.
         */
        const char *scan = id;
        unsigned int field = 0;
        while (*scan != '\0') {
            if (*scan == '|') {
                field++;
                scan++;
                if (field >= 2) {
                    size_t len = 0;
                    while (scan[len] != '\0' && scan[len] != '|') {
                        len++;
                    }
                    obs_resume_watch_add(scan, len);
                    scan += len;
                    continue;
                }
                continue;
            }
            scan++;
        }
        return;
    }

    /* `OBS|build|<build-id>|<target>` - **not** `meta`, whose third field is the format
     * version. Reading `meta` there was the first attempt and it silently never
     * matched, so every resume was discarded and the mechanism looked like it did
     * nothing.
     *
     * Two fields, not one, and the reason is that the default build id is the literal
     * string `dev`. It distinguishes nothing on its own: two different builds both say
     * `dev`, so a guard resting on it alone would be decorative. The target (`host` or
     * `module`) is a real discriminator and free, and the check count from `meta` is a
     * third - a report with a different number of checks is a different program
     * whatever it calls itself. */
    if (kind_len == 5 && kind[0] == 'b' && kind[1] == 'u' && kind[2] == 'i' &&
        kind[3] == 'l' && kind[4] == 'd') {
        size_t i = 0;
        while (i < id_len && build_id[i] != '\0' && id[i] == build_id[i]) {
            i++;
        }
        if (i == id_len && build_id[i] == '\0') {
            obs_resume_build_matched = 1;
        }
        return;
    }
    /* `OBS|meta|<version>|<sections>|<checks>`. The check count is the last field. */
    if (kind_len == 4 && kind[0] == 'm' && kind[1] == 'e' && kind[2] == 't' &&
        kind[3] == 'a') {
        const char *scan = id;
        const char *last = id;
        while (*scan != '\0') {
            if (*scan == '|') {
                last = scan + 1;
            }
            scan++;
        }
        unsigned int seen = 0;
        for (const char *d = last; *d >= '0' && *d <= '9'; d++) {
            seen = seen * 10u + (unsigned int)(*d - '0');
        }
        obs_resume_checks_seen = seen;
    }
}

void obs_resume_load(const char *build_id, unsigned int checks) {
    obs_resume_count = 0;
    obs_resume_full = 0;
    obs_resume_watch_count = 0;
    obs_resume_pending[0] = '\0';
    obs_resume_build_matched = 0;
    obs_resume_checks_seen = 0;
    if (build_id == NULL) {
        return;
    }

    for (unsigned int p = 0; p < OBS_COUNT(obs_sink_paths); p++) {
        int fd = obs_sink_backend_open_read(obs_sink_paths[p]);
        if (fd < 0) {
            continue;
        }
        char block[512];
        char line[OBS_RESUME_ID_MAX * 4];
        size_t held = 0;
        long got;
        /* Lines are reassembled across block boundaries. A record split by a read is
         * still a record, and dropping it would lose exactly the last one written -
         * which is the one this exists to find. */
        while ((got = obs_sink_backend_read(fd, block, sizeof block)) > 0) {
            for (long i = 0; i < got; i++) {
                char c = block[i];
                if (c != '\n') {
                    if (held + 1 < sizeof line) {
                        line[held] = c;
                        held++;
                    }
                    continue;
                }
                line[held] = '\0';
                held = 0;
                obs_resume_consider(line, build_id);
            }
        }
        if (held != 0) {
            line[held] = '\0';
            obs_resume_consider(line, build_id);
        }
        obs_sink_backend_close(fd);
        break;
    }

    /* A report from a different build is not evidence about this one, and a changed
     * check is exactly when acting on last run's word would be worst. Both signals have
     * to agree: the build id (weak alone - it defaults to the literal `dev`) and the
     * number of checks, which cannot match across a build that added or removed one. */
    /* The last announcement had no answer, so that is where the run ended - but once is
     * not evidence. Seen before, it is a hang and gets skipped; seen for the first
     * time, it goes on the watch list and this run tries it again. (D181) */
    if (obs_resume_pending[0] != '\0') {
        size_t n = 0;
        while (obs_resume_pending[n] != '\0') {
            n++;
        }
        if (obs_resume_is_watched(obs_resume_pending)) {
            obs_resume_remember(obs_resume_pending, n);
            obs_resume_watch_drop(obs_resume_pending, n);
        } else {
            obs_resume_watch_add(obs_resume_pending, n);
        }
    }

    if (!obs_resume_build_matched || obs_resume_checks_seen != checks) {
        obs_resume_count = 0;
        obs_resume_full = 0;
        obs_resume_watch_count = 0;
    }
}
