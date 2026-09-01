/*
 * Host-build stubs.
 *
 * Compiled only for the host build, never for the target. Every platform function
 * returns a synthetic "not implemented" code so the harness itself can be run and
 * checked on an ordinary machine, with no console and no emulator involved.
 *
 * # Why this is worth having
 *
 * Without it, the first time this program runs is inside an emulator that does not
 * work yet, and a bug in the harness is indistinguishable from a bug in the thing
 * being measured. The host build makes the framework verifiable on its own: the
 * expected result is a full sheet of red, arriving in the right order, with the
 * dependency skips in the right places.
 *
 * # The error code
 *
 * 0xDEADBEEF, returned as-is rather than negated. Real platform error codes begin
 * 0x80 or 0x81, so this can never be mistaken for something actually observed.
 *
 * The first attempt here defined a positive constant with its high bit clear and
 * returned the negation of it, reasoning that the high bit would stay clear. It does
 * not: negating 0x7F000001 produces 0x80FFFFFF, which is precisely the shape of a
 * real error code. The value a caller sees is what matters, not the value written in
 * the source.
 */

/* The build is -std=c11, which is strict ISO and asks glibc to hide everything POSIX -
 * `sigset_t`, `pthread_rwlock_t`, `getpagesize` and `usleep` all vanish. This asks for
 * them back.
 *
 * It has to come before any include, and it is confined to this file: nothing in the
 * freestanding build sees it, and the target build has no libc to ask. */
#define _DEFAULT_SOURCE 1

#include "obscene/platform.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define OBS_HOST_NOT_IMPLEMENTED ((int)0xDEADBEEF)

/* "This is implemented, and the answer is no."
 *
 * Distinct from the value above, and the distinction is load-bearing. A poll that finds the
 * bits unset and a poll that does not exist are opposite results, and returning the same
 * code for both would let a check pass its negative case against a stub that never ran -
 * which is the failure `900-surface/presence-is-not-behaviour` exists to name, reproduced
 * inside the known-good implementation everything else is validated against. */
#define OBS_HOST_NOT_SATISFIED ((int)0xDEADBEEE)

/* ---- libScePosix, forwarded to the real thing --------------------------------
 *
 * Not stubs. These are the one part of the platform the host genuinely has, so
 * forwarding gives the checks in 017-posix a known-good implementation to be validated
 * against - which is the whole argument for the host build (D001), and what caught a
 * responsiveness probe whose two "must differ" inputs did not differ.
 *
 * A stub returning "not implemented" here would make every check in that section fail
 * on the host, and a check that has only ever been seen to fail is not evidence that it
 * can pass.
 *
 * The signal-set functions take `void *` in this project's declarations, so that no
 * `sigset_t` layout is assumed. The cast is safe in the direction that matters: the
 * buffer passed is larger than any `sigset_t` and more strictly aligned.
 */
int posix_sigemptyset(void *set) {
    return sigemptyset((sigset_t *)set);
}

int posix_sigfillset(void *set) {
    return sigfillset((sigset_t *)set);
}

int posix_sigaddset(void *set, int signal) {
    return sigaddset((sigset_t *)set, signal);
}

int posix_sigdelset(void *set, int signal) {
    return sigdelset((sigset_t *)set, signal);
}

int posix_sigismember(const void *set, int signal) {
    return sigismember((const sigset_t *)set, signal);
}

int posix_getpagesize(void) {
    return getpagesize();
}

int posix_usleep(unsigned int microseconds) {
    return usleep(microseconds);
}

/* The lock is an opaque handle in this project's declarations, matching how the vendor
 * spelling is declared. The host's `pthread_rwlock_t` is a whole struct rather than a
 * pointer, so a single static one is used and the handle merely says which. That is
 * enough for the check, which only ever creates one lock at a time. */
static pthread_rwlock_t host_rwlock;

int posix_pthread_rwlock_init(ObsPosixRwlock *lock, const void *attr) {
    (void)attr;
    int rc = pthread_rwlock_init(&host_rwlock, 0);
    if (rc == 0) {
        *lock = &host_rwlock;
    }
    return rc;
}

int posix_pthread_rwlock_destroy(ObsPosixRwlock *lock) {
    int rc = pthread_rwlock_destroy(&host_rwlock);
    *lock = 0;
    return rc;
}

int posix_pthread_rwlock_tryrdlock(ObsPosixRwlock *lock) {
    (void)lock;
    return pthread_rwlock_tryrdlock(&host_rwlock);
}

int posix_pthread_rwlock_trywrlock(ObsPosixRwlock *lock) {
    (void)lock;
    return pthread_rwlock_trywrlock(&host_rwlock);
}

int posix_pthread_rwlock_unlock(ObsPosixRwlock *lock) {
    (void)lock;
    return pthread_rwlock_unlock(&host_rwlock);
}

/* Forwarded to the real thing so the report actually reaches the terminal, and so
 * the boot section measures a genuine write rather than a stub agreeing with
 * itself. */
sce_ssize_t sceKernelWrite(int fd, const void *buf, size_t nbytes) {
    return (sce_ssize_t)write(fd, buf, nbytes);
}

/* Forwarded, like the write above it. A real descriptor is what every file relation
 * needs, and returning bytes a caller can compare is the difference between validating
 * `018-relational/file-position-tracks-reads` and skipping it. */
sce_ssize_t sceKernelRead(int fd, void *buf, size_t nbytes) {
    return (sce_ssize_t)read(fd, buf, nbytes);
}

/* ---- the clocks, forwarded to real ones --------------------------------------
 *
 * Constants here would make 120-measure untestable off a console, and a section whose
 * logic has never run is not something to hand an emulator. Forwarding gives it the two
 * things it needs to be exercised properly: a genuine CPU-time clock and a genuine wall
 * clock, which are exactly what `identify-clocks` sets out to tell apart.
 *
 * So the host is not merely a place this compiles - it is a platform where the right
 * answer is known. `sceKernelGetProcessTime` must barely advance across a sleep and
 * `sceKernelReadTsc` must advance by about the sleep, and if the experiment cannot show
 * that here it will not show anything on a console.
 */
static uint64_t host_nanoseconds(clockid_t which) {
    struct timespec now;
    if (clock_gettime(which, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * 1000000000u + (uint64_t)now.tv_nsec;
}

/* Microseconds of CPU consumed, which is what the vendor name says it is. A sleeping
 * thread accrues almost none, which is the property under test. */
uint64_t sceKernelGetProcessTime(void) {
    return host_nanoseconds(CLOCK_PROCESS_CPUTIME_ID) / 1000u;
}

/* The same clock unconverted, so the two differ in unit as well as in name - a section
 * that accidentally assumed one scale would show it here rather than on hardware. */
uint64_t sceKernelGetProcessTimeCounter(void) {
    return host_nanoseconds(CLOCK_PROCESS_CPUTIME_ID);
}

/* Wall time. Monotonic rather than realtime: a counter that can be stepped backwards by
 * the system clock would make a duration meaningless, and this stands in for a cycle
 * counter, which cannot be. */
uint64_t sceKernelReadTsc(void) {
    return host_nanoseconds(CLOCK_MONOTONIC);
}

/* Nanoseconds per second, because that is the unit the two counters above are in. The
 * conversion in `sleep-fidelity` is therefore exercised for real rather than skipped on
 * a zero frequency. */
uint64_t sceKernelGetTscFrequency(void) {
    return 1000000000u;
}

uint64_t sceKernelGetProcessTimeCounterFrequency(void) {
    return 1000000000u;
}

/* ---- files, forwarded to real ones -------------------------------------------
 *
 * Not stubs, for the same reason the clocks are not.
 *
 * Five checks were skipping here - `040-file` reaches its negative cases without a real
 * descriptor, but every *relation* about files needs one. Two of them,
 * `018-relational/descriptors-distinct` and `close-is-not-idempotent`, had additionally
 * never run for an unrelated reason (D158), so between the two faults the file relations
 * had no known-good implementation to have been validated against at all.
 *
 * # The path a console has and a host does not
 *
 * The checks open `/app0/eboot.bin`, which is the running module and is present by
 * definition on anything executing this program. The host equivalent is the running
 * binary, so that one path is redirected to `/proc/self/exe` and everything else is passed
 * through. The redirect is deliberately narrow: a check that opens something else is
 * asking a different question and should get the real answer, including a real refusal.
 */
static const char *host_real_path(const char *path) {
    if (path == NULL) {
        return NULL;
    }
    if (strcmp(path, "/app0/eboot.bin") == 0) {
        return "/proc/self/exe";
    }
    /* A scan root under a test tree, so 048-selfaudit's directory walk can be exercised on the
     * host: set OBS_HOST_APPROOT to a directory of <title>/eboot.bin and the app roots resolve
     * inside it. On a console this env is unset and the real paths are used. */
    const char *root = getenv("OBS_HOST_APPROOT");
    if (root != NULL) {
        static char remapped[1024];
        const char *const roots[] = {"/system/vsh/app", "/user/app"};
        for (unsigned int i = 0; i < sizeof roots / sizeof roots[0]; i++) {
            size_t rl = strlen(roots[i]);
            if (strncmp(path, roots[i], rl) == 0) {
                size_t k = 0;
                for (const char *c = root; *c && k < sizeof remapped - 1; c++) {
                    remapped[k++] = *c;
                }
                for (const char *c = path + rl; *c && k < sizeof remapped - 1; c++) {
                    remapped[k++] = *c;
                }
                remapped[k] = '\0';
                return remapped;
            }
        }
    }
    return path;
}

int sceKernelOpen(const char *path, int flags, uint16_t mode) {
    const char *real = host_real_path(path);
    if (real == NULL) {
        /* A null path is the one case `040-file/open-rejects-null` asks about, and handing
         * it to `open` would be undefined rather than refused. */
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    /* Read-only whatever was asked. The vendor flag values are not the host's, and this
     * program's file checks never write - so translating the flags would mean inventing a
     * mapping to serve no check, while opening for write would let a wrong test damage the
     * binary it is running from. */
    (void)flags;
    int fd = open(real, O_RDONLY, (mode_t)mode);
    if (fd < 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return fd;
}

int sceKernelClose(int fd) {
    if (close(fd) != 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return 0;
}

/* Directory enumeration on the host, translated to the layout the target returns.
 *
 * Linux `getdents64` gives a different `dirent` (wide d_ino, a d_off, d_type at the end); the
 * probe parses the FreeBSD-11 one (d_fileno u32, d_reclen, d_type, d_namlen, name). This reads
 * the host's dirents and rewrites each into that layout, so the scan logic in 048-selfaudit is
 * exercised for real under `make host` rather than only on a console. The d_type values happen
 * to agree (DT_DIR=4, DT_REG=8), so only the offsets move. */
sce_ssize_t sceKernelGetdents(int fd, char *buf, int nbytes) {
    char tmp[8192];
    long n = syscall(SYS_getdents64, fd, tmp, sizeof tmp);
    if (n < 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    long out = 0;
    long pos = 0;
    while (pos < n) {
        /* linux_dirent64: d_ino(8) d_off(8) d_reclen@16(2) d_type@18(1) d_name@19 */
        unsigned short lrec;
        memcpy(&lrec, tmp + pos + 16, sizeof lrec);
        if (lrec == 0) {
            break;
        }
        unsigned char ltype = (unsigned char)tmp[pos + 18];
        const char *lname = tmp + pos + 19;
        size_t namelen = strlen(lname);
        size_t frec = (8 + namelen + 1 + 7) & ~(size_t)7; /* FreeBSD rounds records to 8 */
        if (out + (long)frec > (long)nbytes) {
            break;
        }
        char *o = buf + out;
        memset(o, 0, frec);
        unsigned short fr = (unsigned short)frec;
        memcpy(o + 4, &fr, sizeof fr);   /* d_reclen */
        o[6] = (char)ltype;              /* d_type */
        o[7] = (char)(namelen & 0xffu);  /* d_namlen */
        memcpy(o + 8, lname, namelen);
        out += (long)frec;
        pos += lrec;
    }
    return out;
}

sce_off_t sceKernelLseek(int fd, sce_off_t offset, int whence) {
    /* The vendor `SEEK_*` values are ISO C's and match the host's, which is worth stating
     * rather than relying on: they are the three checked constants in `platform.h`, and if
     * a platform ever disagreed the translation would belong here. */
    int host_whence;
    switch (whence) {
        case OBS_SEEK_SET: host_whence = SEEK_SET; break;
        case OBS_SEEK_CUR: host_whence = SEEK_CUR; break;
        case OBS_SEEK_END: host_whence = SEEK_END; break;
        default: return OBS_HOST_NOT_IMPLEMENTED;
    }
    off_t moved = lseek(fd, (off_t)offset, host_whence);
    if (moved < 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return (sce_off_t)moved;
}

/* ---- direct memory, a real allocator over a static arena ---------------------
 *
 * `018-relational/allocations-do-not-overlap` asks whether two allocations held at the
 * same time can name the same memory. A stub returning a fixed offset would fail it and a
 * stub returning not-implemented skips it, and neither is a validation - so the host needs
 * an allocator that genuinely tracks what it has handed out.
 *
 * A bump pointer with a free list of released extents. Not efficient and not general; it
 * is enough to make "two live allocations are disjoint" and "a released extent can be
 * allocated again" (`direct-memory-round-trip`) both mean something.
 *
 * # Why the arena is half a gigabyte
 *
 * Nothing here is ever mapped. These calls reserve a *range* - an offset and a length in a
 * table - and turning one into usable memory is `sceKernelMapDirectMemory`, which remains
 * unimplemented. So the span costs an integer, and the first choice of sixteen megabytes
 * made `020-memory/direct-size` report `partial: implausibly small` against its 256 MiB
 * threshold, leaving that check's pass branch unexercised on the one platform where the
 * right answer is known.
 *
 * Raising it is not the host telling a plausible lie, which is the hazard this project keeps
 * meeting. The number is true: the allocator really does hand out offsets across that range,
 * and really does refuse past the end of it.
 */
#define OBS_HOST_DIRECT_SIZE ((sce_off_t)0x20000000)
#define OBS_HOST_DIRECT_MAX 16

typedef struct host_extent {
    sce_off_t start;
    size_t len;
    int in_use;
} host_extent;
static host_extent host_direct[OBS_HOST_DIRECT_MAX];
static sce_off_t host_direct_next;

size_t sceKernelGetDirectMemorySize(void) {
    return (size_t)OBS_HOST_DIRECT_SIZE;
}

int sceKernelAllocateDirectMemory(sce_off_t search_start, sce_off_t search_end,
                                  size_t len, size_t alignment, int memory_type,
                                  sce_off_t *physical_address_out) {
    (void)memory_type;
    if (physical_address_out == NULL || len == 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    if (alignment == 0) {
        alignment = 1;
    }
    if (search_end <= search_start) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }

    /* A released extent of at least the size asked for, before taking new ground. This is
     * what makes `direct-memory-round-trip` measure something: an allocator that never
     * reuses would pass its first two calls and fail the third only once the arena ran
     * out, which is a different fault from the one that check is looking for. */
    for (int i = 0; i < OBS_HOST_DIRECT_MAX; i++) {
        if (!host_direct[i].in_use && host_direct[i].len >= len) {
            host_direct[i].in_use = 1;
            *physical_address_out = host_direct[i].start;
            return 0;
        }
    }

    sce_off_t start = host_direct_next;
    sce_off_t misaligned = start % (sce_off_t)alignment;
    if (misaligned != 0) {
        start += (sce_off_t)alignment - misaligned;
    }
    if (start < search_start) {
        start = search_start;
    }
    if (start + (sce_off_t)len > search_end || start + (sce_off_t)len > OBS_HOST_DIRECT_SIZE) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_DIRECT_MAX; i++) {
        if (host_direct[i].len == 0) {
            host_direct[i].start = start;
            host_direct[i].len = len;
            host_direct[i].in_use = 1;
            host_direct_next = start + (sce_off_t)len;
            *physical_address_out = start;
            return 0;
        }
    }
    /* The table is full. Reported as a refusal to allocate, which is a real outcome for an
     * allocator and one the relations are written to skip on rather than fail. */
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelReleaseDirectMemory(sce_off_t start, size_t len) {
    for (int i = 0; i < OBS_HOST_DIRECT_MAX; i++) {
        if (host_direct[i].in_use && host_direct[i].start == start
            && host_direct[i].len == len) {
            host_direct[i].in_use = 0;
            return 0;
        }
    }
    /* Releasing something never allocated, or releasing twice. Refused rather than
     * accepted, because a release that always succeeds is how a double-free check passes
     * against nothing. */
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* A map entry, and a deliberate difference between flag values.
 *
 * `130-layout/direct-memory-query` and `direct-memory-query-flags` both skipped here for
 * want of this, which means neither had been run against a known-good implementation - and
 * the second one exists to answer a question orbistoun cannot answer from its side, so
 * shipping it unvalidated would be shipping an instrument nobody has calibrated.
 *
 * # What this is and is not
 *
 * It is **not** a model of what a console returns. This program does not know that; finding
 * it out is the entire purpose of `130-layout`, and a host stub that guessed would be
 * putting a guess where the report should carry a measurement.
 *
 * It is a shape chosen to exercise the *instrument*: a start and a length in the first
 * sixteen bytes, a byte that varies with the flag, and a refusal for one flag value. Between
 * them they drive every branch of the check - baseline capture, difference detection, the
 * first-differing-byte report, and the refusal count - on a machine where the right answer
 * is known because this file wrote it.
 *
 * The varying byte sits at offset 16, past the two eight-byte fields, so a check that
 * compared only the first sixteen bytes would find nothing and say so. That is the mistake
 * worth catching here: a differencing pass that stops early reports "the flag changes
 * nothing", which is a conclusion rather than a silence.
 */
int sceKernelDirectMemoryQuery(sce_off_t offset, int flags, void *info, size_t size) {
    if (info == NULL || size < 24) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    /* One value refused, so the check's refusal branch is exercised somewhere. Four rather
     * than a value nothing passes: it is in the set the flag sweep tries, and a refusal it
     * never sees would not be a test of anything. */
    if (flags == 4) {
        return OBS_HOST_NOT_SATISFIED;
    }
    unsigned char *out = (unsigned char *)info;
    for (size_t i = 0; i < size; i++) {
        out[i] = 0;
    }
    /* Start and length, little-endian, as such a structure would carry them. The start is
     * the offset asked about, so a check that queried elsewhere would see it move. */
    for (unsigned int i = 0; i < 8; i++) {
        out[i] = (unsigned char)(((uint64_t)offset >> (i * 8)) & 0xFFu);
        out[8 + i] = (unsigned char)((0x00100000u >> (i * 8)) & 0xFFu);
    }
    /* The byte the flag reaches, past both fields. */
    out[16] = (unsigned char)(0xA0u + (unsigned int)flags);
    return 0;
}

int sceKernelMapDirectMemory(void **addr, size_t len, int prot, int flags,
                             sce_off_t direct_memory_start, size_t max_page_size) {
    (void)addr;
    (void)len;
    (void)prot;
    (void)flags;
    (void)direct_memory_start;
    (void)max_page_size;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelMunmap(void *addr, size_t len) {
    (void)addr;
    (void)len;
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* A real sleep, for the same reason as the clocks: a measurement of a sleep that did
 * not happen measures nothing, and the checks that watch the clock across one would
 * never have run. */
int sceKernelUsleep(unsigned int microseconds) {
    return usleep(microseconds);
}

/* ---- enough of the vendor surface to validate 018-relational ------------------
 *
 * The relational checks compare results to each other, so they can run without any
 * document saying what the platform should return - but they still need something that
 * *behaves* to be checked against. Constants here would leave the section unvalidated,
 * and CLAUDE.md is explicit that a check which has never passed a known-good
 * implementation is not evidence.
 *
 * So these are small correct implementations rather than stubs: real handles, a real
 * count, a real identity. Each is deliberately the simplest thing that satisfies the
 * relation honestly, which also means a mistake in a check shows up here as a failure
 * rather than being hidden by a stub that says yes to everything.
 */
ScePthread scePthreadSelf(void) {
    /* The real one. `018-relational/thread-identity-stable` asks that two calls agree and
     * that the answer is not nothing - both true here, and neither true of a stub
     * returning NULL, which is what this used to be. */
    return (ScePthread)pthread_self();
}

/* Event flags: a handle table with real allocation and release.
 *
 * `sceKernelCreateEventFlag` in the target build takes a `SceKernelEventFlag *`, which is
 * an opaque pointer. Here it points into a small static pool - enough for the eight the
 * distinctness check asks for, with room over, and released on delete so the reuse check
 * measures something. */
#define OBS_HOST_EVF_MAX 32
typedef struct host_event_flag {
    int in_use;
    uint64_t bits;
} host_event_flag;
static host_event_flag host_evf[OBS_HOST_EVF_MAX];

int sceKernelCreateEventFlag(SceKernelEventFlag *out, const char *name, uint32_t attr,
                             uint64_t init, const void *opt) {
    (void)name;
    (void)attr;
    (void)opt;
    if (out == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_EVF_MAX; i++) {
        if (!host_evf[i].in_use) {
            host_evf[i].in_use = 1;
            host_evf[i].bits = init;
            *out = (SceKernelEventFlag)&host_evf[i];
            return 0;
        }
    }
    /* Exhaustion reported as a failure to create, which is what the reuse check reads as
     * "it worked and then stopped" if release is not happening. */
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelDeleteEventFlag(SceKernelEventFlag flag) {
    if (flag == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    ((host_event_flag *)flag)->in_use = 0;
    return 0;
}

/* Set, clear and poll against the same table.
 *
 * Added when `018-relational/event-flag-state-is-per-object` skipped for want of them.
 * The check that matters here is whether a bit set on one flag is visible on another, and
 * a stub cannot answer that unless its state is genuinely per-object - which is the same
 * property the check exists to measure, so the two would be testing each other if the
 * state lived anywhere but in the entry the handle points at.
 *
 * `015-sync/event-flag-round-trip` had been skipping for the same reason and now runs. */
int sceKernelSetEventFlag(SceKernelEventFlag flag, uint64_t bits) {
    if (flag == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    ((host_event_flag *)flag)->bits |= bits;
    return 0;
}

int sceKernelClearEventFlag(SceKernelEventFlag flag, uint64_t bits) {
    if (flag == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    /* `&= bits`, and the argument is a mask of what to **keep** - the opposite of what the
     * name suggests. shadPS4 does `m_bits &= bits`; PS5PCEM does the same and says why in a
     * comment: "The PS5 ABI supplies the bits to retain, not the bits to remove."
     *
     * This was written the other way round for a day, as the complement, on the reasoning
     * that a host stub should implement the *obvious* semantics so that a console
     * disagreeing with it would be reporting a real difference. **That reasoning is
     * backwards** and it cost a wrong verdict: `015-sync/event-flag-round-trip` asserted the
     * obvious semantics too, so the stub agreed with the check, the check passed here, and a
     * check that has passed a known-good implementation is what this project calls evidence.
     *
     * It took a three-way consensus to see - both emulators failing while the host alone
     * passed. A known-good implementation of the *wrong contract* is worse than none,
     * because it manufactures exactly the confidence rule 5 exists to supply. (D166) */
    ((host_event_flag *)flag)->bits &= bits;
    return 0;
}

int sceKernelPollEventFlag(SceKernelEventFlag flag, uint64_t bits, uint32_t mode,
                           uint64_t *out_pattern) {
    if (flag == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    uint64_t held = ((host_event_flag *)flag)->bits;
    if (out_pattern != NULL) {
        *out_pattern = held;
    }
    /* AND is the only mode this program asks for, and the only one implemented. Anything
     * else returns not-implemented rather than being quietly treated as AND: a stub that
     * answers a question it was not asked is how a wrong assumption gets confirmed. */
    if (mode != OBS_EVF_WAITMODE_AND) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    if ((held & bits) == bits) {
        return 0;
    }
    /* Not set. A distinct code from the not-implemented one, so "the bits are not there"
     * and "this stub does not do that" never read the same in a report. */
    return OBS_HOST_NOT_SATISFIED;
}

/* One buffer-filling call, so 130-layout's dump mechanism is exercised somewhere the
 * right answer is known.
 *
 * The section reports bytes rather than verdicts, which means a bug in the *reporting* -
 * a wrong extent, a mangled hex nibble, a chunk boundary off by one - would produce a
 * plausible-looking hexdump of the wrong thing. On hardware that would be indetectable
 * and permanent, because the whole point is that nobody knows what the bytes should be.
 *
 * So this writes a pattern chosen to catch exactly those: a recognisable prefix, a byte
 * that is not its own nibble-swap, a zero in the middle to prove the extent is the *last*
 * non-zero byte and not the first zero, and a run that crosses the sixteen-byte record
 * boundary. */
int sceKernelGetSystemSwVersion(void *version) {
    if (version == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    static const unsigned char pattern[] = {
        0x28, 0x00, 0x00, 0x00,  /* a length, little-endian, as such structures carry */
        'H',  'O',  'S',  'T',
        0x12, 0x34, 0x56, 0x78,  /* nibble order is visible if it is wrong */
        0x00, 0x00, 0x00, 0x00,  /* an interior gap the extent must see past */
        0xDE, 0xAD, 0xBE, 0xEF,  /* and a run past the first record boundary */
    };
    unsigned char *out = (unsigned char *)version;
    for (unsigned int i = 0; i < sizeof pattern; i++) {
        out[i] = pattern[i];
    }
    return 0;
}

/* Condition variables and barriers: real ones, so 015-sync's three new checks are
 * exercised rather than skipped.
 *
 * The host genuinely has both, so nothing is simulated. That matters more here than
 * elsewhere: these checks were deferred for a long time on the grounds that the subsystem
 * could not be tested safely, and shipping them without ever seeing them pass a working
 * implementation would repeat the mistake in the other direction. */
#define OBS_HOST_COND_MAX 16
static struct {
    int in_use;
    pthread_cond_t cond;
} host_cond[OBS_HOST_COND_MAX];

int scePthreadCondInit(ScePthreadCond *cond, const void *attr, const char *name) {
    (void)attr;
    (void)name;
    if (cond == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_COND_MAX; i++) {
        if (!host_cond[i].in_use) {
            if (pthread_cond_init(&host_cond[i].cond, NULL) != 0) {
                return OBS_HOST_NOT_IMPLEMENTED;
            }
            host_cond[i].in_use = 1;
            *cond = (ScePthreadCond)&host_cond[i].cond;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadCondDestroy(ScePthreadCond *cond) {
    if (cond == NULL || *cond == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_COND_MAX; i++) {
        if (&host_cond[i].cond == (pthread_cond_t *)*cond) {
            host_cond[i].in_use = 0;
            (void)pthread_cond_destroy(&host_cond[i].cond);
            *cond = NULL;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadCondSignal(ScePthreadCond *cond) {
    if (cond == NULL || *cond == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_cond_signal((pthread_cond_t *)*cond);
}

int scePthreadCondBroadcast(ScePthreadCond *cond) {
    if (cond == NULL || *cond == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_cond_broadcast((pthread_cond_t *)*cond);
}

/* The blocking one. Forwarded for the same reason as the rest, and with more riding on
 * it: `015-sync/condvar-wakes-a-waiter` is the only check in this suite whose whole
 * design is about surviving a call that never returns, and a stub could not tell whether
 * that design works. Here a real signal really does wake a real waiter, so a pass means
 * the mechanism was exercised rather than merely not crashed. */
int scePthreadCondWait(ScePthreadCond *cond, ScePthreadMutex *mutex) {
    if (cond == NULL || *cond == NULL || mutex == NULL || *mutex == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_cond_wait((pthread_cond_t *)*cond, (pthread_mutex_t *)*mutex);
}

#define OBS_HOST_BARRIER_MAX 8
static struct {
    int in_use;
    pthread_barrier_t barrier;
} host_barrier[OBS_HOST_BARRIER_MAX];

int scePthreadBarrierInit(ScePthreadBarrier *barrier, const void *attr,
                          unsigned int count, const char *name) {
    (void)attr;
    (void)name;
    if (barrier == NULL || count == 0) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_BARRIER_MAX; i++) {
        if (!host_barrier[i].in_use) {
            if (pthread_barrier_init(&host_barrier[i].barrier, NULL, count) != 0) {
                return OBS_HOST_NOT_IMPLEMENTED;
            }
            host_barrier[i].in_use = 1;
            *barrier = (ScePthreadBarrier)&host_barrier[i].barrier;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadBarrierDestroy(ScePthreadBarrier *barrier) {
    if (barrier == NULL || *barrier == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_BARRIER_MAX; i++) {
        if (&host_barrier[i].barrier == (pthread_barrier_t *)*barrier) {
            host_barrier[i].in_use = 0;
            (void)pthread_barrier_destroy(&host_barrier[i].barrier);
            *barrier = NULL;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* The one that proves the technique: with a count of one this returns immediately, so
 * the check never blocks and needs no second thread. */
int scePthreadBarrierWait(ScePthreadBarrier *barrier) {
    if (barrier == NULL || *barrier == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_barrier_wait((pthread_barrier_t *)*barrier);
}

/* Mutexes: real ones, so 018-relational's two mutex relations are exercised.
 *
 * The handle is an opaque pointer in this project's declarations, which matches how the
 * platform spells it, so a pool of real pthread mutexes fits behind it directly. Distinct
 * handles fall out of using distinct slots, and non-recursiveness falls out of
 * PTHREAD_MUTEX_DEFAULT - neither is simulated, which is the point: a check that has only
 * ever met a fake is not evidence. */
#define OBS_HOST_MUTEX_MAX 16
static struct {
    int in_use;
    pthread_mutex_t mutex;
} host_mutex[OBS_HOST_MUTEX_MAX];

int scePthreadMutexInit(ScePthreadMutex *mutex, const void *attr, const char *name) {
    (void)name;
    if (mutex == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    /* The attribute is honoured rather than dropped, and that is the whole point of it.
     *
     * This used to pass NULL and ignore the argument, which is harmless for every check
     * that existed then and would have quietly broken the one that came next: a recursion
     * probe run against a mutex that never received the recursive attribute reports "not
     * recursive" on a host where it demonstrably is, and the check would have looked
     * correct while measuring nothing.
     *
     * `attr` points at the opaque handle, so it is dereferenced once to reach the real
     * attribute object - the same shape the platform's own signature has. */
    const pthread_mutexattr_t *chosen = NULL;
    if (attr != NULL) {
        chosen = (const pthread_mutexattr_t *)*(void *const *)attr;
    }
    for (int i = 0; i < OBS_HOST_MUTEX_MAX; i++) {
        if (!host_mutex[i].in_use) {
            if (pthread_mutex_init(&host_mutex[i].mutex, chosen) != 0) {
                return OBS_HOST_NOT_IMPLEMENTED;
            }
            host_mutex[i].in_use = 1;
            *mutex = (ScePthreadMutex)&host_mutex[i].mutex;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* Mutex attributes, on the real thing.
 *
 * These exist so the recursion checks can be run against an implementation that is known
 * to be correct before anything they say about a console is believed - step five of the
 * checklist, and the step that has caught a wrong check twice.
 *
 * Backed by a small table for the same reason the mutexes above are: the guest holds a
 * `void *` and the host needs somewhere real to put a `pthread_mutexattr_t`. */
#define OBS_HOST_MUTEXATTR_MAX 8
static struct {
    pthread_mutexattr_t attr;
    int in_use;
} host_mutexattr[OBS_HOST_MUTEXATTR_MAX];

int scePthreadMutexattrInit(ScePthreadMutexattr *attr) {
    if (attr == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_MUTEXATTR_MAX; i++) {
        if (!host_mutexattr[i].in_use) {
            if (pthread_mutexattr_init(&host_mutexattr[i].attr) != 0) {
                return OBS_HOST_NOT_IMPLEMENTED;
            }
            host_mutexattr[i].in_use = 1;
            *attr = &host_mutexattr[i].attr;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadMutexattrDestroy(ScePthreadMutexattr *attr) {
    if (attr == NULL || *attr == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_MUTEXATTR_MAX; i++) {
        if (&host_mutexattr[i].attr == (pthread_mutexattr_t *)*attr) {
            host_mutexattr[i].in_use = 0;
            (void)pthread_mutexattr_destroy(&host_mutexattr[i].attr);
            *attr = NULL;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadMutexattrSettype(ScePthreadMutexattr *attr, int type) {
    if (attr == NULL || *attr == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_mutexattr_settype((pthread_mutexattr_t *)*attr, type);
}

int scePthreadMutexattrGettype(ScePthreadMutexattr *attr, int *type_out) {
    if (attr == NULL || *attr == NULL || type_out == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_mutexattr_gettype((pthread_mutexattr_t *)*attr, type_out);
}

int scePthreadMutexDestroy(ScePthreadMutex *mutex) {
    if (mutex == NULL || *mutex == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_MUTEX_MAX; i++) {
        if (&host_mutex[i].mutex == (pthread_mutex_t *)*mutex) {
            host_mutex[i].in_use = 0;
            (void)pthread_mutex_destroy(&host_mutex[i].mutex);
            *mutex = NULL;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadMutexTrylock(ScePthreadMutex *mutex) {
    if (mutex == NULL || *mutex == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_mutex_trylock((pthread_mutex_t *)*mutex);
}

int scePthreadMutexUnlock(ScePthreadMutex *mutex) {
    if (mutex == NULL || *mutex == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    return pthread_mutex_unlock((pthread_mutex_t *)*mutex);
}

/* Semaphores: a real count, because a count is exactly what the relation tests. */
#define OBS_HOST_SEMA_MAX 16
static struct {
    int in_use;
    int count;
    int max;
} host_sema[OBS_HOST_SEMA_MAX];

int sceKernelCreateSema(int *out, const char *name, uint32_t attr, int init, int max,
                        const void *opt) {
    (void)name;
    (void)attr;
    (void)opt;
    if (out == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_SEMA_MAX; i++) {
        if (!host_sema[i].in_use) {
            host_sema[i].in_use = 1;
            host_sema[i].count = init;
            host_sema[i].max = max;
            /* One-based, so zero stays available as "no semaphore". */
            *out = i + 1;
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelDeleteSema(int sema) {
    if (sema < 1 || sema > OBS_HOST_SEMA_MAX) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    host_sema[sema - 1].in_use = 0;
    return 0;
}

int sceKernelSignalSema(int sema, int count) {
    if (sema < 1 || sema > OBS_HOST_SEMA_MAX || !host_sema[sema - 1].in_use) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    if (host_sema[sema - 1].count + count > host_sema[sema - 1].max) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    host_sema[sema - 1].count += count;
    return 0;
}

int sceKernelPollSema(int sema, int need) {
    if (sema < 1 || sema > OBS_HOST_SEMA_MAX || !host_sema[sema - 1].in_use) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    /* The whole point: refusing when the count is short is what makes a third poll
     * against two signals fail, and a stub cannot do it. */
    if (host_sema[sema - 1].count < need) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    host_sema[sema - 1].count -= need;
    return 0;
}

/* Real threads, because two checks are worthless without them.
 *
 * `015-sync/thread-churn` creates and joins in a loop, and `015-sync/condvar-wakes-a-waiter`
 * is built entirely around a waiter on another thread. Against a stub the first proves
 * nothing and the second cannot run at all - and the second is the one whose *design* is
 * the interesting part, so shipping it unexercised would be exactly the mistake its own
 * section comment warns about.
 *
 * The handle is an opaque pointer on the target, and `pthread_t` is an integer here, so
 * it goes through a small pool rather than being cast. Casting would work on this
 * platform and quietly stop working somewhere else. */
#define OBS_HOST_THREAD_MAX 64
static struct {
    int in_use;
    pthread_t thread;
} host_thread[OBS_HOST_THREAD_MAX];

int scePthreadCreate(ScePthread *thread, const void *attr, void *(*entry)(void *),
                     void *arg, const char *name) {
    (void)attr;
    (void)name;
    if (thread == NULL || entry == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_THREAD_MAX; i++) {
        if (!host_thread[i].in_use) {
            if (pthread_create(&host_thread[i].thread, NULL, entry, arg) != 0) {
                return OBS_HOST_NOT_IMPLEMENTED;
            }
            host_thread[i].in_use = 1;
            *thread = (ScePthread)&host_thread[i];
            return 0;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePthreadJoin(ScePthread thread, void **value_out) {
    if (thread == NULL) {
        return OBS_HOST_NOT_IMPLEMENTED;
    }
    for (int i = 0; i < OBS_HOST_THREAD_MAX; i++) {
        if (&host_thread[i] == (void *)thread && host_thread[i].in_use) {
            int rc = pthread_join(host_thread[i].thread, value_out);
            /* Released only on a successful join, so a failed join does not hand the
             * slot to somebody else while the thread is still using it. */
            if (rc == 0) {
                host_thread[i].in_use = 0;
            }
            return rc;
        }
    }
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelLoadStartModule(const char *name, size_t argc, const void *argv,
                             uint32_t flags, void *opt, int *result_out) {
    (void)name;
    (void)argc;
    (void)argv;
    (void)flags;
    (void)opt;
    (void)result_out;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelDlsym(int handle, const char *symbol, void **address_out) {
    (void)handle;
    (void)symbol;
    (void)address_out;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceSysmoduleLoadModule(uint16_t id) {
    (void)id;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceSysmoduleIsLoaded(uint16_t id) {
    (void)id;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceUserServiceInitialize(const void *params) {
    (void)params;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceUserServiceGetInitialUser(int32_t *user_id_out) {
    (void)user_id_out;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceUserServiceTerminate(void) {
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoOutOpen(int user_id, int type, int index, const void *param) {
    (void)user_id;
    (void)type;
    (void)index;
    (void)param;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoOutClose(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoOutSetFlipRate(int handle, int rate) {
    (void)handle;
    (void)rate;
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* Recording. A host has no hardware encoder to drive, so these refuse like the rest -
 * which is what the section reads as a skip rather than as an answer about a console.
 *
 * **Prototyped here rather than in platform.h.** These names are in the mined corpus, and
 * the census declares every corpus name as an opaque `extern const char` in order to take
 * its address; a function declaration in platform.h is a redefinition in the translation
 * unit that sees both. The section that calls them declares them the same way, for the
 * same reason. */

int sceVideoRecordingQueryMemSize(int mode);
int sceVideoRecordingClose(int handle);
int sceVideoRecordingStop(int handle);
int sceVideoRecordingGetStatus(int handle);

int sceVideoRecordingQueryMemSize(int mode) {
    (void)mode;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoRecordingClose(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoRecordingStop(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceVideoRecordingGetStatus(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceAudioOutInit(void) {
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceAudioOutOpen(int user_id, int type, int index, uint32_t length,
                    uint32_t frequency, uint32_t param) {
    (void)user_id;
    (void)type;
    (void)index;
    (void)length;
    (void)frequency;
    (void)param;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceAudioOutClose(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePadInit(void) {
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePadOpen(int user_id, int type, int index, const void *param) {
    (void)user_id;
    (void)type;
    (void)index;
    (void)param;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int scePadClose(int handle) {
    (void)handle;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelGetModuleList(int *out_handles, size_t max, size_t *out_count) {
    (void)out_handles;
    (void)max;
    (void)out_count;
    return OBS_HOST_NOT_IMPLEMENTED;
}

int sceKernelGetModuleInfo(int handle, void *info) {
    (void)handle;
    (void)info;
    return OBS_HOST_NOT_IMPLEMENTED;
}

/* Static dummy classification and case tables for host build validation */
const void *_Getpctype(void);
const void *_Getptolower(void);
const void *_Getptoupper(void);
const void *_Getwctype(void);
const void *_Getwctolower(void);
const void *_Getwctoupper(void);

static const unsigned short s_host_ctype_tab[512] = {0};
static const short s_host_tolower_tab[512] = {0};
static const short s_host_toupper_tab[512] = {0};
static const unsigned short s_host_wctype_tab[512] = {0};
static const short s_host_wtolower_tab[512] = {0};
static const short s_host_wtoupper_tab[512] = {0};

const void *_Getpctype(void) {
    return (const void *)&s_host_ctype_tab[16];
}

const void *_Getptolower(void) {
    return (const void *)&s_host_tolower_tab[16];
}

const void *_Getptoupper(void) {
    return (const void *)&s_host_toupper_tab[16];
}

const void *_Getwctype(void) {
    return (const void *)&s_host_wctype_tab[16];
}

const void *_Getwctolower(void) {
    return (const void *)&s_host_wtolower_tab[16];
}

const void *_Getwctoupper(void) {
    return (const void *)&s_host_wtoupper_tab[16];
}
