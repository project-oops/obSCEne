/*
 * The report's second destination.
 *
 * Not one of the four channels in `runtime.c`. Those are alternatives and only one is
 * chosen; this is written to *as well*, because "can this system talk to a terminal" and
 * "can this system leave something behind" are different questions and a console usually
 * answers them differently.
 *
 * See src/sink.c for why it is never buffered.
 */

#ifndef OBSCENE_SINK_H
#define OBSCENE_SINK_H

#include <stddef.h>
#include <stdint.h>

/* Opens the first writable candidate path, once.
 *
 * Returns the path that worked, or NULL if none did or the platform has no file support.
 * The caller reports it, so that a run with a working sink and a run with none are
 * distinguishable afterwards - which they are not if this fails quietly. */
const char *obs_sink_open(void);

/* Writes bytes to the sink if it is open, and does nothing if it is not.
 *
 * Deliberately silent about failure. This sits underneath the reporting layer, so it has
 * no way to report anything without recursion, and a sink that has stopped closes itself
 * rather than interrupting a run that is still producing useful records elsewhere. */
void obs_sink_write(const char *bytes, size_t len);

void obs_sink_close(void);
int obs_sink_is_open(void);

/* ---- the backend -------------------------------------------------------------------
 *
 * Three calls, implemented once per target, and **selected by which file the build
 * compiles** rather than by a preprocessor branch inside a shared one.
 *
 * That is a deliberate structure and not a style preference. What differs between targets
 * here is small and total: the same path list, the same write loop, the same
 * never-buffer rule, reaching a different set of five functions. A `#if` in the middle of
 * `sink.c` would put two targets' code in one file where only one of them is ever
 * compiled, so the untaken branch is never even parsed and rots quietly.
 *
 * A list in the Makefile says which backend a target gets, in one place, visibly. Adding a
 * target means adding a file and a line - not threading another condition through code
 * that already works.
 *
 * A descriptor is an `int` because every backend so far has one. A backend whose handle is
 * not an int would need this widened, and that is a change worth making deliberately
 * rather than hiding behind a typedef nothing else uses. */

/* Opens `path` for writing, creating and truncating. Returns a descriptor, or negative. */
int obs_sink_backend_open(const char *path);

/* Writes up to `len` bytes. Returns the count accepted, or negative on failure. */
long obs_sink_backend_write(int fd, const void *bytes, size_t len);

/* Opens `path` for reading, without creating it. Returns a descriptor, or negative.
 *
 * The read side exists for one caller: the previous run's report. `obs_sink_backend_open`
 * truncates, so by the time the sink is writing there is nothing left to learn from - which
 * is why this is separate rather than a mode flag, and why `start.c` calls the reader first.
 * See `obs_resume_load`. */
int obs_sink_backend_open_read(const char *path);

/* Reads up to `len` bytes. Returns the count, 0 at end of file, negative on failure. */
long obs_sink_backend_read(int fd, void *bytes, size_t len);

void obs_sink_backend_close(int fd);

/* Attempts to create directory `path`. Returns 0 on success or negative on failure. */
int obs_sink_backend_mkdir(const char *path);

/* Returns current wall clock time or process time in seconds/ticks, or 0. */
uint64_t obs_sink_backend_time(void);

#endif /* OBSCENE_SINK_H */

/* ---- resuming past a check that did not return -------------------------------------------
 *
 * Reads the previous run's report and remembers the one check it announced and never
 * finished, so this run can skip it. That turns "the loader dies at check six" from a
 * build-time exclusion list into a property of running the same binary twice.
 *
 * Must be called **before** `obs_sink_open`, which truncates.
 */
void obs_resume_load(const char *build_id, unsigned int checks);

/* Whether this id was skipped by a previous run of the same build. */
int obs_resume_is_skipped(const char *id);

/* How many ids are carried, and whether the set filled up (which would reintroduce the
 * oscillation the set exists to prevent, so it is reported rather than silent). */
unsigned int obs_resume_skipped_count(void);

/* Checks that failed to return once and are being tried again rather than skipped.
 *
 * Written onto the `resume` record so the next run inherits them: one dangling `try` is an
 * accident as often as a hang, and skipping on the first sighting cost sixteen thousand
 * measurements behind a `complete` verdict. See `obs_resume_load`. (D181) */
unsigned int obs_resume_watched_count(void);

/* The watched id at `index`, or null past the end. */
const char *obs_resume_watched(unsigned int index);
int obs_resume_overflowed(void);
