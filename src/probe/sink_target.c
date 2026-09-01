/*
 * The file sink's backend on the target platform.
 *
 * One of two files implementing the three calls in `obscene/sink.h`. The build compiles
 * exactly one of them; see `COMMON_SRC` and the per-target lists in the Makefile, and the
 * note in the header for why that is a source list rather than a preprocessor branch.
 *
 * Everything above this - which paths to try, writing a record at a time, closing a sink
 * that has stopped accepting bytes - is in `sink.c` and is shared. What is here is the
 * three platform calls and nothing else, which is the whole point of the split: a second
 * target adds a file this size, not a condition in every file.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sink.h"

struct sce_timeval {
    int64_t tv_sec;
    int64_t tv_usec;
};
OBS_WEAK int sceKernelGettimeofday(struct sce_timeval *tp, void *tzp);
OBS_WEAK int sceKernelMkdir(const char *path, uint16_t mode);

/* Create, truncate, write-only.
 *
 * Truncate rather than append: a file holding two runs concatenated parses as one run with
 * two `meta` records, and diffing that against a single run produces nonsense. One file,
 * one run. */
#define OBS_O_WRONLY 0x0001
#define OBS_O_CREAT 0x0200
#define OBS_O_TRUNC 0x0400
/* Readable and writable by anyone, and that is not laziness about permissions.
 *
 * The report is written by a title, inside its sandbox, as whatever user the platform runs
 * titles as. Everything that exists to *retrieve* it is a different process running as
 * somebody else: the shell server, the file-transfer server, a later payload. A mode of
 * 0600 makes all of them fail with `Permission denied` on a file they can see in a
 * listing, which is the worst available shape - the file is plainly there and no tool on
 * the machine can open it, so it reads as a broken retrieval path rather than a mode.
 *
 * A mode of zero fails the same way and more obviously. Both were arrived at by reasoning
 * about the writer alone; the reader is the one that matters, and it is never the writer.
 *
 * There is nothing to protect here. The file is a list of which system functions answered,
 * on a machine the operator owns, deleted and rewritten on every run. (D237)
 */
#define OBS_SINK_MODE ((uint16_t)0666)

int obs_sink_backend_open(const char *path) {
    /* Weak, like every platform declaration, so a platform without file support resolves
     * these to null. Tested before the call rather than after: jumping to zero would end
     * the run before a single record had been written anywhere, including to the text
     * channel that was about to work. */
    if (!obs_address_is_callable((const void *)&sceKernelOpen)
        || !obs_address_is_callable((const void *)&sceKernelWrite)) {
        return -1;
    }
    return sceKernelOpen(path, OBS_O_WRONLY | OBS_O_CREAT | OBS_O_TRUNC, OBS_SINK_MODE);
}

long obs_sink_backend_write(int fd, const void *bytes, size_t len) {
    if (!obs_address_is_callable((const void *)&sceKernelWrite)) {
        return -1;
    }
    return (long)sceKernelWrite(fd, bytes, len);
}

void obs_sink_backend_close(int fd) {
    if (obs_address_is_callable((const void *)&sceKernelClose)) {
        sceKernelClose(fd);
    }
}

int obs_sink_backend_open_read(const char *path) {
    /* Guarded like every other platform call here: a loader without file support resolves
     * these to null, and jumping to zero while looking for an *optional* previous report
     * would end a run that had nothing wrong with it. */
    if (!obs_address_is_callable((const void *)&sceKernelOpen)
        || !obs_address_is_callable((const void *)&sceKernelRead)) {
        return -1;
    }
    /* No `O_CREAT`: a report that is not there is the ordinary first-run case, and creating
     * an empty one would turn "nothing to learn" into "a file that says nothing", which the
     * caller cannot tell from a run that finished cleanly. */
    return sceKernelOpen(path, OBS_O_RDONLY, 0);
}

long obs_sink_backend_read(int fd, void *bytes, size_t len) {
    if (!obs_address_is_callable((const void *)&sceKernelRead)) {
        return -1;
    }
    return (long)sceKernelRead(fd, bytes, len);
}

int obs_sink_backend_mkdir(const char *path) {
    if (!obs_address_is_callable((const void *)&sceKernelMkdir)) {
        return -1;
    }
    return sceKernelMkdir(path, OBS_SINK_MODE);
}

uint64_t obs_sink_backend_time(void) {
    if (obs_address_is_callable((const void *)&sceKernelGettimeofday)) {
        struct sce_timeval tv;
        if (sceKernelGettimeofday(&tv, NULL) == 0 && tv.tv_sec > 0) {
            return (uint64_t)tv.tv_sec;
        }
    }
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        return sceKernelGetProcessTime();
    }
    return 0;
}
