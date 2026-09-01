/*
 * The file sink's backend on an ordinary machine.
 *
 * The other half of the pair described in `obscene/sink.h`. It exists so that the sink
 * is
 * **exercised by `make host`**, which CLAUDE.md requires of anything before its output
 * is believed - a rule that has caught a wrong check twice.
 *
 * Without it the sink would be untestable off hardware. The target's `sceKernelOpen` is
 * stubbed on the host as not-implemented, which is the right answer for the `040-file`
 * checks that measure it and the wrong one for a sink that needs to actually write
 * something. Giving the host build a real backend separates those two concerns instead
 * of compromising one for the other: the checks still see a stub, and the sink still
 * gets tested.
 *
 * What this does *not* test is the target's three platform calls, which is the same
 * position every other platform call is in and is exactly what the emulators and the
 * hardware are for. What it does test is everything else - path discovery, the write
 * loop, the close-on-failure path, and that a record reaches disk at the moment it is
 * produced.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "obscene/sink.h"

int obs_sink_backend_open(const char *path) {
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
}

long obs_sink_backend_write(int fd, const void *bytes, size_t len) {
    return (long)write(fd, bytes, len);
}

void obs_sink_backend_close(int fd) {
    (void)close(fd);
}

int obs_sink_backend_open_read(const char *path) {
    return open(path, O_RDONLY);
}

long obs_sink_backend_read(int fd, void *bytes, size_t len) {
    return (long)read(fd, bytes, len);
}

int obs_sink_backend_mkdir(const char *path) {
    return mkdir(path, 0755);
}

uint64_t obs_sink_backend_time(void) {
    time_t now = time(NULL);
    return now > 0 ? (uint64_t)now : 0;
}
