/*
 * The command socket's backend on an ordinary POSIX system.
 *
 * Covers two targets with one file: the host, where the protocol is developed and tested
 * without any hardware, and a general-purpose handheld, whose networking is ordinary Linux.
 *
 * That overlap is the reason the networking can be built and proven long before either
 * console is available - which is the point of having a stand-in at all. What the stand-in
 * cannot do is answer anything about the vendor's libraries, and nothing here pretends
 * otherwise.
 */

#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(OBS_NET_ESCAPE)
#include <stdint.h>
#include <sys/mman.h>
#endif

#include "obscene/net.h"

int obs_net_backend_available(void) {
    return 1;
}

const char *obs_net_backend_name(void) {
    return "posix";
}

int obs_net_backend_listen(unsigned short port) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        return -1;
    }

    /* Without this, a probe that has just crashed cannot rebind for a minute or so while
     * the old socket lingers - and crashing is the normal case here, not the exceptional
     * one. A supervisor restarting the probe would spend that minute failing to listen,
     * which reads as the probe being broken. */
    int one = 1;
    (void)setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0
        || listen(listener, 1) < 0) {
        (void)close(listener);
        return -1;
    }
    return listener;
}

int obs_net_backend_accept(int listener) {
    for (;;) {
        int connection = accept(listener, NULL, NULL);
        if (connection >= 0) {
            return connection;
        }
        /* A signal interrupting the wait is not a failure. Returning here would end the
         * session for a reason that has nothing to do with the driver. */
        if (errno != EINTR) {
            return -1;
        }
    }
}

long obs_net_backend_recv(int connection, char *bytes, size_t len) {
    for (;;) {
        long n = (long)recv(connection, bytes, len, 0);
        if (n >= 0 || errno != EINTR) {
            return n;
        }
    }
}

long obs_net_backend_send(int connection, const char *bytes, size_t len) {
    /* All of it, or a failure.
     *
     * A partial send reported as success would truncate a record mid-line, and a truncated
     * record is worse than a missing one: it parses. The loop is what makes the caller's
     * "one record, written whole" guarantee true rather than usually true. */
    size_t sent = 0;
    while (sent < len) {
        long n = (long)send(connection, bytes + sent, len - sent, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            return -1;
        }
        sent += (size_t)n;
    }
    return (long)len;
}

void obs_net_backend_close(int handle) {
    (void)close(handle);
}

#if defined(OBS_NET_ESCAPE)
/* Maps the blob writable, copies it in, flips it to read-execute, calls it, and unmaps it.
 *
 * Two mappings' worth of protection changes rather than one PROT_WRITE|PROT_EXEC region: a
 * page that is writable and executable at the same moment is the thing every hardening guide
 * says not to make, and there is no reason to here - the code is written once and then only
 * run. mprotect between the two is the whole difference.
 *
 * A blob that faults when called takes the process down, which is the ordinary outcome of
 * running supplied code and is recorded as `died` by the driver, exactly as a bad `call` is. */
int obs_net_backend_exec(const unsigned char *code, size_t len, const uint64_t *args,
                         unsigned int argc, uint64_t *result) {
    if (len == 0) {
        return -1;
    }
    void *mem = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return -1;
    }
    memcpy(mem, code, len);
    if (mprotect(mem, len, PROT_READ | PROT_EXEC) != 0) {
        (void)munmap(mem, len);
        return -1;
    }
    uint64_t a[6] = {0, 0, 0, 0, 0, 0};
    for (unsigned int i = 0; i < argc && i < 6; i++) {
        a[i] = args[i];
    }
    typedef uint64_t (*obs_blob_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
    obs_blob_fn fn = (obs_blob_fn)(uintptr_t)mem;
    uint64_t r = fn(a[0], a[1], a[2], a[3], a[4], a[5]);
    (void)munmap(mem, len);
    *result = r;
    return 0;
}
#endif

/* Entropy from the operating system, because this build has one.
 *
 * The console backend beside this has to make do with timing jitter and says so at length.
 * Here there is a kernel CSPRNG and using anything else would be inventing a weakness. Read
 * rather than `getrandom(2)` so the file stays plain POSIX and the same source builds on the
 * Deck and the build VM without a feature test. */
int obs_net_backend_entropy(unsigned char *out, unsigned int len) {
    if (out == NULL || len == 0) {
        return -1;
    }
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    unsigned int filled = 0;
    while (filled < len) {
        ssize_t got = read(fd, out + filled, len - filled);
        if (got <= 0) {
            (void)close(fd);
            return -1;
        }
        filled += (unsigned int)got;
    }
    (void)close(fd);
    return 0;
}
