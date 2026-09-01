/*
 * Imports from the target platform's system libraries.
 *
 * # These names are ABI, not prose
 *
 * The symbol and library strings here are how the loader finds these functions -
 * the import hash is computed from the symbol name. Renaming them for tidiness
 * would stop this program testing anything, so they stay exactly as the platform
 * spells them. Comments and documentation elsewhere avoid vendor branding; these
 * cannot.
 *
 * # Nothing here is invented
 *
 * Every declaration below is a signature this project is confident about from
 * public interface documentation and open-source homebrew toolchains. Where an
 * arity or a struct layout is uncertain, the function is **left out** rather than
 * guessed at: a wrong arity corrupts the stack and the resulting crash points
 * nowhere near the mistake. Adding one is a matter of confirming the signature, and
 * the check tables are arranged so it costs three lines.
 *
 * # Opaque handles
 *
 * Where a type is a handle whose layout is not needed to call the function, it is
 * declared opaque. Guessing at a struct layout to pass one by value is the same
 * error as guessing at an arity, and this program exists to find that class of bug
 * rather than to contain it.
 */

#ifndef OBSCENE_PLATFORM_H
#define OBSCENE_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t sce_off_t;
typedef int64_t sce_ssize_t;

/* A thread handle. Opaque: only ever held and passed back. */
typedef void *ScePthread;

/* Every platform declaration is weak.
 *
 * An unresolved import then becomes a null address rather than a link failure, which
 * is what lets this program report "not present" for a function instead of dying at
 * load time. Taking the address of a weak undefined symbol is defined and yields
 * null; calling one jumps to zero, so the harness checks before it calls. */
/* Overridable, because a loader may refuse to resolve a weak symbol at all.
 *
 * On a real console, elfldr left every weak import null and the probe jumped to zero at
 * entry - the ELF specification says an unresolved weak symbol resolves to zero
 * *without error*, so a strict resolver leaving them alone is correct behaviour, not a
 * bug in it.
 * `-DOBS_WEAK=` builds the same sources with strong undefined symbols, which a resolver
 * must either satisfy or fail loudly over. Both are more useful than silence. (D205)
 *
 * The default stays weak: that is what lets this program report "not present" rather
 * than fail to load, and it is right everywhere the loader resolves what it can. */
#ifndef OBS_WEAK
#define OBS_WEAK __attribute__((weak))
#endif

/* Calls `fn` once per import declared outside the census, with the library it comes
 * from. Defined in imports.c; see the note there on why the association cannot be
 * read off the declarations themselves. */
void obs_platform_each_symbol(void (*fn)(const char *library, const char *symbol));

#if defined(__cplusplus)
extern "C" {
#endif

/* ---- libkernel: process and time ------------------------------------------ */

/* Microseconds of process CPU time. Monotonic within a process. */
OBS_WEAK uint64_t sceKernelGetProcessTime(void);
/* Raw timestamp counter ticks. */
OBS_WEAK uint64_t sceKernelGetProcessTimeCounter(void);
/* Frequency of the counter above, in hertz. */
OBS_WEAK uint64_t sceKernelGetTscFrequency(void);

/* The cycle counter and the divisor that turns it into time.
 *
 * Both take no arguments and return a plain integer, so they carry none of the struct
 * risk that keeps `sceKernelClockGettime` in the census. That is what makes them the
 * measuring instrument for 120-measure: a duration can be recorded in real units
 * without assuming a single layout.
 *
 * Signatures from the OpenOrbis toolchain headers (libkernel.h), which is published
 * interface documentation. Four independent emulators implement both. */
OBS_WEAK uint64_t sceKernelReadTsc(void);

/* ---- calls that fill a buffer, for 130-layout --------------------------------
 *
 * Declared with the buffer as `void *` and never as a named structure. That is the
 * whole point: 130-layout dumps what these write and does not interpret it, so no
 * layout is assumed and D008 is not being bent.
 *
 * Arities from the OpenOrbis toolchain headers, which is published interface
 * documentation. The arity is the part that must be right - a wrong one corrupts the
 * stack - and it is the only part being taken. */
OBS_WEAK int sceKernelDirectMemoryQuery(sce_off_t offset, int flags, void *info,
                                        size_t size);
OBS_WEAK int sceKernelGetSystemSwVersion(void *version);

/* The kernel answering questions about itself, by name.
 *
 * # Why this is worth a declaration of its own
 *
 * Nothing in this program has ever asked the platform a sysctl, and the sibling
 * emulator is blocked on exactly one answer: a payload calls `kern.osrelease` once,
 * does not like what an unimplemented call gives it, reports that firmware detection
 * failed and turns a feature off. That emulator refuses to invent the string,
 * correctly, so the value has to come from a machine - and this is the program that
 * runs on one.
 *
 * Arity and shape are POSIX, which the target kernel derives from; the names asked for
 * are this program's choice and every one of them is reported whether it answers or
 * not, so a refusal is data rather than a gap. */
OBS_WEAK int sysctlbyname(const char *name, void *oldp, size_t *oldlenp,
                          const void *newp, size_t newlen);

/* Free storage, for the HUD's DISK field. `statfs(path, buf)` fills a `struct statfs`;
 * only two fields are read, at FreeBSD's stable offsets (`f_bsize` at 0x10, `f_bavail`
 * at 0x30 - sys/mount.h), out of an over-sized buffer, so the layout past them is never
 * depended on. Arity and shape are POSIX/FreeBSD, which the target kernel derives from;
 * a `void *` buffer keeps this program from carrying the vendor struct it deliberately
 * does not have. (D272) */
OBS_WEAK int statfs(const char *path, void *buf);
OBS_WEAK int sceVideoOutGetResolutionStatus(int handle, void *status);

/* How many frames the display has actually put on screen.
 *
 * # Why this one is worth the risk of a struct
 *
 * Every other display call reports whether it was *accepted*. None of them reports
 * whether anything was *shown*, and those are different facts on a platform whose
 * display library is only partly implemented: obSCEne opened an output, registered a
 * framebuffer, submitted a flip, was told yes four times, and drew a black window for
 * the whole run - then reported `display|ready|1920x1080 framebuffer`, which was a
 * claim it had no evidence for.
 *
 * The counter is the evidence. Submit a flip, read it, and if it has not moved then the
 * platform accepted a frame and presented nothing. That is a behavioural test in the
 * same shape as the rest of this program - call it and look at what happened, rather
 * than trust the return code - and it needs no knowledge of which loader is running.
 *
 * # The layout, and why reading one field is defensible
 *
 * The status structure is large and the two implementations available disagree about
 * most of it. They agree about the **first field**, which is the only one read here:
 *
 *     Kyty       uint64_t count;   // first member
 *     PS5PCEM    count: u64,       // first member, in two separate files
 *
 * Two independent readings agreeing on the field being used is the same standard D111
 * set for the buffer descriptor. Everything after it is left alone. The caller passes a
 * buffer far larger than either implementation writes, so a third implementation
 * writing more cannot reach past it.
 *
 * `IMPLEMENTATIONS`. Hardware settles whether the first field is the frame count.
 * (D187)
 */
OBS_WEAK int sceVideoOutGetFlipStatus(int handle, void *status);

/* ---- flexible memory ---------------------------------------------------------
 *
 * A second allocation path, and one obSCEne had no coverage of at all.
 *
 * Direct memory - which `020-memory` exercises - is physical memory reserved by offset
 * and mapped explicitly. Flexible memory is the other kind: the system finds the pages,
 * and the caller asks for a size rather than a location. A title uses both, and an
 * emulator implementing one and not the other passes every memory check in this suite.
 *
 * Found by asking which functions the current-generation emulators implement that this
 * program does not know about (`scripts/ps5-gap.py`). Signatures from the OpenOrbis
 * toolchain headers.
 */
OBS_WEAK int sceKernelAvailableFlexibleMemorySize(size_t *out);
/* The configured total, distinct from the available figure above (available =
 * configured minus what is mapped). The vendor libkernel exports both; mined across
 * eight independent sources (PS5PCEM, SharpEMU, aerolib, fpPS4, ps4libdoc, shadPS4,
 * ...), and the signature mirrors its available sibling - one `size_t` out-parameter.
 */
OBS_WEAK int sceKernelConfiguredFlexibleMemorySize(size_t *out);
OBS_WEAK int sceKernelMapFlexibleMemory(void **address, size_t len, int prot,
                                        int flags);
OBS_WEAK int sceKernelReleaseFlexibleMemory(void *address, size_t len);

/* ---- more of libkernel, from the emulator gap analysis -----------------------
 *
 * Signatures from the OpenOrbis toolchain headers, each implemented by two or more
 * independent emulators. `reports/gap-checkable.txt` lists the rest.
 *
 * `sceKernelGetCompiledSdkVersion` is deliberately absent: the header declares it with
 * no arguments and a void return, which is the toolchain saying it does not know
 * either. Calling it on that basis is what D008 forbids. */

/* Whether an address is on the calling thread's stack, and the region it lies in.
 *
 * The two out-pointers are almost certainly the bounds of that region. `almost
 * certainly` is why 130-layout would be the place to establish it and this is not - the
 * check below uses only the return value, which is the part the name settles. */
OBS_WEAK int sceKernelIsStack(void *address, void **low, void **high);

/* Thread attributes. Opaque handle, as every other pthread object here is. */
typedef void *ScePthreadAttr;
OBS_WEAK int scePthreadAttrInit(ScePthreadAttr *attr);
OBS_WEAK int scePthreadAttrDestroy(ScePthreadAttr *attr);
OBS_WEAK int scePthreadAttrSetdetachstate(ScePthreadAttr *attr, int state);
OBS_WEAK int scePthreadAttrGetdetachstate(const ScePthreadAttr *attr, int *state);
OBS_WEAK uint64_t sceKernelGetProcessTimeCounterFrequency(void);

/* ---- libkernel: descriptors ------------------------------------------------ */

OBS_WEAK sce_ssize_t sceKernelWrite(int fd, const void *buf, size_t nbytes);
OBS_WEAK sce_ssize_t sceKernelRead(int fd, void *buf, size_t nbytes);

/* The POSIX spelling of the same call.
 *
 * Declared as well as sceKernelWrite because they are different symbols and an
 * implementation may have one and not the other - which is not a hypothetical: an
 * emulator was found stubbing sceKernelWrite, so every line this program produced went
 * into a function that returned zero and discarded it. A probe whose report depends on
 * one unimplemented function reports nothing, anywhere, including the fact that the
 * function is unimplemented.
 *
 * Signature is POSIX. See obs_write in runtime.c for how one is chosen.
 *
 * Target build only. On the host these are the real libc, already declared by its own
 * headers, and a second declaration with a weak attribute is an error rather than a
 * redundancy. The host build does not use them: it writes through libc directly. */
#if !defined(OBSCENE_HOST_BUILD)
OBS_WEAK sce_ssize_t write(int fd, const void *buf, size_t nbytes);
#endif
OBS_WEAK int sceKernelOpen(const char *path, int flags, uint16_t mode);
OBS_WEAK int sceKernelClose(int fd);
OBS_WEAK sce_off_t sceKernelLseek(int fd, sce_off_t offset, int whence);

/* Read directory entries from an open directory descriptor into `buf`, returning the
 * number of bytes filled, 0 at end of directory, or negative on error. This is how a
 * probe discovers what is on the filesystem rather than being told - 048-selfaudit
 * walks the application directories to find a real container instead of naming one.
 *
 * Signature and the dirent layout below are FreeBSD's, from `freebsd-src`
 * `sys/sys/dirent.h` (the FreeBSD-11-and-earlier `struct dirent`, which is what this
 * generation's kernel derives from - the newer 12+ layout widens d_fileno and inserts
 * d_off). The vendor rename is confirmed by open-source aerolib symbol tables. It is a
 * hypothesis until the walk returns readable names; a wrong layout yields rubbish
 * rather than a crash, because the parser is bounded by d_reclen. (selfish#D087) */
OBS_WEAK sce_ssize_t sceKernelGetdents(int fd, char *buf, int nbytes);

/* The dirent packed into that buffer, one per entry, each `d_reclen` bytes long.
 *
 *   0x00  d_fileno   u32   the entry's file number
 *   0x04  d_reclen   u16   how many bytes this record occupies - advance by this
 *   0x06  d_type     u8    what kind of entry it is (see below)
 *   0x07  d_namlen   u8    how long the name is
 *   0x08  d_name     ...   the name, `d_namlen` bytes, not necessarily NUL-terminated
 */
#define OBS_DIRENT_RECLEN 0x04
#define OBS_DIRENT_TYPE 0x06
#define OBS_DIRENT_NAMLEN 0x07
#define OBS_DIRENT_NAME 0x08
/* d_type values, FreeBSD's. A directory is what the walk descends into. */
#define OBS_DT_DIR 4
#define OBS_DT_REG 8

/* ---- libkernel: direct memory ---------------------------------------------- */

/* Total physical memory available to a title, in bytes. */
OBS_WEAK size_t sceKernelGetDirectMemorySize(void);
OBS_WEAK int sceKernelAllocateDirectMemory(sce_off_t search_start, sce_off_t search_end,
                                           size_t len, size_t alignment,
                                           int memory_type,
                                           sce_off_t *physical_address_out);
OBS_WEAK int sceKernelAllocateMainDirectMemory(size_t len, size_t alignment,
                                               int memory_type,
                                               sce_off_t *physical_address_out);
OBS_WEAK int sceKernelReleaseDirectMemory(sce_off_t start, size_t len);
OBS_WEAK int sceKernelMapDirectMemory(void **addr, size_t len, int prot, int flags,
                                      sce_off_t direct_memory_start,
                                      size_t max_page_size);
OBS_WEAK int sceKernelVirtualQuery(const void *addr, int flags, void *info,
                                   size_t info_size);
OBS_WEAK int sceKernelMunmap(void *addr, size_t len);

/* ---- libkernel: scheduling ------------------------------------------------- */

OBS_WEAK int sceKernelUsleep(unsigned int microseconds);

/* Previous-generation only, and takes no arguments - which is why it is safe to call
 * as a second, independent signal for 005-generation. */
OBS_WEAK int sceKernelIsNeoMode(void);

/* ---- libkernel: threads ---------------------------------------------------- */

OBS_WEAK ScePthread scePthreadSelf(void);
OBS_WEAK int scePthreadCreate(ScePthread *thread, const void *attr,
                              void *(*entry)(void *), void *arg, const char *name);
OBS_WEAK int scePthreadJoin(ScePthread thread, void **value_out);

/* ---- libkernel: POSIX synchronisation --------------------------------------
 *
 * Mutexes, read/write locks and semaphores. POSIX specifies what these do, so the
 * expectations are `spec` rather than beliefs - a platform that gets them wrong is
 * wrong against a document anyone can read.
 *
 * # Try, never block
 *
 * Only the non-blocking forms are declared. `scePthreadMutexLock` on a mutex whose
 * implementation is broken does not fail, it hangs, and a probe that hangs loses every
 * check behind it. `Trylock` answers the same question - is this lock free - and comes
 * back either way.
 *
 * That costs nothing in coverage. "A fresh mutex can be taken" and "a held mutex cannot
 * be taken again" together pin the semantics, and both are `try` calls. The blocking
 * forms stay in the census, where being present is all that is claimed of them.
 */

/* Opaque handles. The platform makes them and takes them back; nothing here reads one.
 */
typedef void *ScePthreadMutex;
typedef void *ScePthreadRwlock;

/* Mutex attributes, and the one question in this header that a probe exists to settle.
 *
 * A mutex's recursion policy is decided here and nowhere else, and getting it wrong is
 * not a wrong answer - it is a deadlock. A guest that takes a lock it already holds
 * either proceeds (recursive) or stops forever (not), and an implementation that
 * guesses the default has a whole-process hang waiting on the guess.
 *
 * **The `Get` counterpart is what makes this checkable.** `Settype` alone can only be
 * probed by its return code, which says the call was accepted and nothing about whether
 * anything was stored. `Gettype` turns it into a round trip: set a value, read it back,
 * and compare. That is a positive check in the sense of principle 7 - it proves the
 * attribute object carries state, where a return code proves argument validation.
 *
 * The type constants are deliberately **not** declared. POSIX names three and fixes
 * none of their values, so writing one here would be inventing the thing the check is
 * for (D008). `015-sync/mutexattr-round-trip` sweeps candidates and records which the
 * platform accepts, which costs nothing and answers the question the constant would
 * have assumed.
 */
typedef void *ScePthreadMutexattr;

OBS_WEAK int scePthreadMutexattrInit(ScePthreadMutexattr *attr);
OBS_WEAK int scePthreadMutexattrDestroy(ScePthreadMutexattr *attr);
OBS_WEAK int scePthreadMutexattrSettype(ScePthreadMutexattr *attr, int type);
OBS_WEAK int scePthreadMutexattrGettype(ScePthreadMutexattr *attr, int *type_out);

OBS_WEAK int scePthreadMutexInit(ScePthreadMutex *mutex, const void *attr,
                                 const char *name);
OBS_WEAK int scePthreadMutexDestroy(ScePthreadMutex *mutex);
OBS_WEAK int scePthreadMutexTrylock(ScePthreadMutex *mutex);
OBS_WEAK int scePthreadMutexUnlock(ScePthreadMutex *mutex);

OBS_WEAK int scePthreadRwlockInit(ScePthreadRwlock *lock, const void *attr,
                                  const char *name);
OBS_WEAK int scePthreadRwlockDestroy(ScePthreadRwlock *lock);
OBS_WEAK int scePthreadRwlockTryrdlock(ScePthreadRwlock *lock);
OBS_WEAK int scePthreadRwlockTrywrlock(ScePthreadRwlock *lock);
OBS_WEAK int scePthreadRwlockUnlock(ScePthreadRwlock *lock);

/* ---- condition variables and barriers ----------------------------------------
 *
 * # Why these were deferred, and what changed
 *
 * Testing either *meaningfully* needs a second thread that waits, and a waiter needs a
 * timeout or a broken implementation hangs the run - the failure the try-only rule in
 * 015-sync exists to prevent. That is still true, and `scePthreadCondTimedwait` is
 * still out of reach because it takes a `timespec` (D008).
 *
 * What was missed is that two operations here **cannot block at all**:
 *
 *   * a barrier initialised with a count of one is satisfied by the calling thread, so
 *     `Wait` returns immediately and no second thread is involved;
 *   * `Signal` and `Broadcast` on a condition variable with no waiters are defined to
 *     have no effect and return.
 *
 * So the subsystem is testable without the timeout mechanism, without a second thread,
 * and without faking anything. Not fully - nothing here proves a waiter is ever woken -
 * and that limit is stated in the checks rather than papered over.
 *
 * # Signatures
 *
 * From the OpenOrbis toolchain headers, which are published interface documentation,
 * and the condition-variable half is confirmed independently by an emulator's own
 * declaration. The arity of `BarrierInit` in particular - handle, attributes, count,
 * name
 * - is read rather than inferred from the POSIX shape, because inferring an arity is
 * what D008 forbids.
 *
 * Opaque handles, as every other lock in this file is. */
typedef void *ScePthreadCond;
typedef void *ScePthreadBarrier;

OBS_WEAK int scePthreadCondInit(ScePthreadCond *cond, const void *attr,
                                const char *name);
OBS_WEAK int scePthreadCondDestroy(ScePthreadCond *cond);
OBS_WEAK int scePthreadCondSignal(ScePthreadCond *cond);
OBS_WEAK int scePthreadCondBroadcast(ScePthreadCond *cond);

/* The blocking one, and the only unbounded call this program makes.
 *
 * Called on a spawned thread that nothing joins, so a wakeup that never arrives strands
 * that thread and not the run. See `015-sync/condvar-wakes-a-waiter`, which is built
 * entirely around keeping the main thread out of here. */
OBS_WEAK int scePthreadCondWait(ScePthreadCond *cond, ScePthreadMutex *mutex);

OBS_WEAK int scePthreadBarrierInit(ScePthreadBarrier *barrier, const void *attr,
                                   unsigned int count, const char *name);
OBS_WEAK int scePthreadBarrierDestroy(ScePthreadBarrier *barrier);
OBS_WEAK int scePthreadBarrierWait(ScePthreadBarrier *barrier);

/* ---- libScePosix: the same platform under its POSIX names --------------------
 *
 * A separate library exporting POSIX with a `posix_` prefix. It matters here for a
 * reason none of the other libraries have: **these are a second spelling of functions
 * this program already calls.** `scePthreadRwlockTryrdlock` and
 * `posix_pthread_rwlock_tryrdlock` should be one implementation behind two names, and
 * an emulator where they disagree has a bug no single-path check can see.
 *
 * They are also the best provenance available without a console. POSIX settles what
 * these do, so a check written against them is OBS_FROM_SPEC - a document anyone can
 * consult, rather than this project's own reasoning. Most of this suite is still
 * OBS_FROM_ASSUMED, and an emulator implemented to match an assumption is only
 * agreeing with us.
 *
 * # Nothing here needs a struct layout, and that is why this list is short
 *
 * `posix_nanosleep`, `posix_clock_gettime` and the timed lock calls all take a
 * `timespec`, and `posix_mmap` and the socket calls take more. A layout is exactly what
 * D008 says to leave out: wrong, it produces a call that succeeds and does the wrong
 * thing. They are censused instead.
 *
 * The signal-set calls are the exception that proves it. A `sigset_t` is opaque and its
 * size differs between systems, so the pointer is declared as `void *` - identical in
 * the ABI, since every pointer passes the same way - and the checks pass a buffer large
 * enough for any of them and read the answer back through the library's own
 * `ismember`. Nothing about the layout is assumed, only that it is smaller than the
 * buffer.
 */

/* An opaque handle, exactly as the vendor spelling of the same lock is. */
typedef void *ObsPosixRwlock;
OBS_WEAK int posix_pthread_rwlock_init(ObsPosixRwlock *lock, const void *attr);
OBS_WEAK int posix_pthread_rwlock_destroy(ObsPosixRwlock *lock);
OBS_WEAK int posix_pthread_rwlock_tryrdlock(ObsPosixRwlock *lock);
OBS_WEAK int posix_pthread_rwlock_trywrlock(ObsPosixRwlock *lock);
OBS_WEAK int posix_pthread_rwlock_unlock(ObsPosixRwlock *lock);

/* Signal sets. The pointer is untyped on purpose - see above. */
OBS_WEAK int posix_sigemptyset(void *set);
OBS_WEAK int posix_sigfillset(void *set);
OBS_WEAK int posix_sigaddset(void *set, int signal);
OBS_WEAK int posix_sigdelset(void *set, int signal);
OBS_WEAK int posix_sigismember(const void *set, int signal);

/* Page size and sleeping. Both return a value that can be checked without knowing
 * anything the platform has not already told us. */
OBS_WEAK int posix_getpagesize(void);
OBS_WEAK int posix_usleep(unsigned int microseconds);

/* Counting semaphores. `Poll` rather than `Wait`, for the reason above. */
OBS_WEAK int sceKernelCreateSema(int *out, const char *name, uint32_t attr, int init,
                                 int max, const void *opt);
OBS_WEAK int sceKernelDeleteSema(int sema);
OBS_WEAK int sceKernelSignalSema(int sema, int count);
OBS_WEAK int sceKernelPollSema(int sema, int need);

/* ---- libkernel: event flags ------------------------------------------------
 *
 * A synchronisation primitive with real semantics to check, rather than an existence
 * test: set a bit, poll for it, clear it, poll again. Each step has one correct answer
 * and none of them requires a struct layout.
 *
 * # Poll rather than wait, deliberately
 *
 * `sceKernelWaitEventFlag` blocks. A probe that blocks on a platform whose event flags
 * do not work never returns, and takes every check behind it with it - the worst
 * outcome available, and one this suite has already paid for twice.
 * `sceKernelPollEventFlag` asks the same question and comes back either way.
 */

/* Opaque. The platform hands one back and takes it again; nothing here reads it. */
typedef void *SceKernelEventFlag;

OBS_WEAK int sceKernelCreateEventFlag(SceKernelEventFlag *out, const char *name,
                                      uint32_t attr, uint64_t init_pattern,
                                      const void *param);
OBS_WEAK int sceKernelDeleteEventFlag(SceKernelEventFlag flag);
OBS_WEAK int sceKernelSetEventFlag(SceKernelEventFlag flag, uint64_t bits);
OBS_WEAK int sceKernelClearEventFlag(SceKernelEventFlag flag, uint64_t bits);
OBS_WEAK int sceKernelPollEventFlag(SceKernelEventFlag flag, uint64_t bits,
                                    uint32_t mode, uint64_t *out_pattern);

/* Single-waiter, first-in-first-out. The simplest attribute combination, so a failure
 * is a failure of event flags rather than of a particular queuing policy. */
#define OBS_EVF_ATTR_FIFO 0x01u
#define OBS_EVF_ATTR_SINGLE 0x10u
/* Wait until every named bit is set. */
#define OBS_EVF_WAITMODE_AND 0x01u

/* ---- libkernel: which machine this is ---------------------------------------
 *
 * Both were missing from this program entirely until an emulator's release notes named
 * them as newly implemented - not reported absent, never asked about. They are here
 * because that gap is the one this suite is least able to see on its own. */
OBS_WEAK int sceKernelIsDevkit(void);
OBS_WEAK int sceKernelIsCex(void);

/* ---- libkernel: dynamic linking -------------------------------------------- */

OBS_WEAK int sceKernelLoadStartModule(const char *name, size_t argc, const void *argv,
                                      uint32_t flags, void *opt, int *result_out);
OBS_WEAK int sceKernelDlsym(int handle, const char *symbol, void **address_out);

/* Enumerating what is actually loaded.
 *
 * The difference between testing a list somebody wrote and testing what the platform
 * has. A list is only ever as complete as whoever maintained it; this asks the machine.
 *
 * # The info structure is read carefully, and not declared
 *
 * `sceKernelGetModuleList` deals only in handles, so it needs no layout at all and is
 * safe outright. `sceKernelGetModuleInfo` fills a structure whose full layout this
 * program does not know - so it is passed a generously sized, zeroed buffer with its
 * leading size field set, and only the two fields at the front are read back. Reading a
 * field at a guessed offset would produce confident nonsense, which is worse than
 * reading nothing (D008). See src/sections/modules.c. */
OBS_WEAK int sceKernelGetModuleList(int *out_handles, size_t max, size_t *out_count);
OBS_WEAK int sceKernelGetModuleInfo(int handle, void *info);

/* ---- libSceSysmodule: system module control ------------------------------- */
OBS_WEAK int sceSysmoduleLoadModule(uint16_t id);
OBS_WEAK int sceSysmoduleIsLoaded(uint16_t id);

/* ---- libSceLibcInternal: the C runtime -------------------------------------
 *
 * The largest omission in the first pass of this program, and the easiest to get
 * right. A title imports far more of the C library than it does of any single vendor
 * subsystem, and every signature here comes from ISO C rather than from anything that
 * needed reverse engineering - so this is the one area where **positive** checks are
 * cheap, which is exactly what D007 says the suite is short of.
 *
 * The library exports plain C names; the import hash is computed from `malloc` and
 * `strlen` just as it is from `sceKernelOpen`.
 *
 * # memcpy, memset and memmove are deliberately absent
 *
 * This program defines its own, because the compiler emits calls to them regardless
 * of `-ffreestanding` and a freestanding binary with no definition fails to link.
 * A local definition wins over a weak import, so declaring them here would mean the
 * checks silently measured our implementations rather than the platform's - a test
 * that passes by testing itself. They are excluded rather than quietly wrong.
 */

OBS_WEAK size_t strlen(const char *s);
OBS_WEAK int strcmp(const char *a, const char *b);
OBS_WEAK int strncmp(const char *a, const char *b, size_t n);
OBS_WEAK char *strchr(const char *s, int c);
OBS_WEAK char *strrchr(const char *s, int c);
OBS_WEAK char *strncpy(char *dest, const char *src, size_t n);
OBS_WEAK char *strcat(char *dest, const char *src);
OBS_WEAK char *strstr(const char *haystack, const char *needle);
OBS_WEAK int memcmp(const void *a, const void *b, size_t n);
OBS_WEAK void *memchr(const void *s, int c, size_t n);

OBS_WEAK void *malloc(size_t size);
OBS_WEAK void *calloc(size_t count, size_t size);
OBS_WEAK void *realloc(void *ptr, size_t size);
OBS_WEAK void free(void *ptr);

OBS_WEAK char *strcpy(char *dest, const char *src);
OBS_WEAK size_t strspn(const char *s, const char *accept);
OBS_WEAK size_t strcspn(const char *s, const char *reject);
OBS_WEAK char *strtok(char *s, const char *delimiters);

OBS_WEAK int snprintf(char *dest, size_t size, const char *format, ...);
OBS_WEAK int atoi(const char *s);
OBS_WEAK long strtol(const char *s, char **end, int base);
OBS_WEAK unsigned long strtoul(const char *s, char **end, int base);
OBS_WEAK int abs(int value);
OBS_WEAK void qsort(void *base, size_t count, size_t size,
                    int (*compare)(const void *, const void *));
OBS_WEAK void *bsearch(const void *key, const void *base, size_t count, size_t size,
                       int (*compare)(const void *, const void *));

OBS_WEAK int rand(void);
OBS_WEAK void srand(unsigned int seed);

OBS_WEAK int toupper(int c);
OBS_WEAK int tolower(int c);
OBS_WEAK int isdigit(int c);
OBS_WEAK int isalpha(int c);
OBS_WEAK int isspace(int c);
OBS_WEAK int isupper(int c);

/* ---- more of the C runtime, made callable ----------------------------------
 *
 * Every one of these was censused - reported present, never invoked. ISO C says exactly
 * what each does, so the expectations are `spec`: a platform that gets them wrong is
 * wrong against a document anyone can read, and the failure is not negotiable.
 *
 * Chosen for having one unambiguous answer that is easy to get subtly wrong. Case
 * conversion, span-finding and integer division are all places an implementation can be
 * plausible and incorrect, which is exactly what an existence test cannot see.
 */
OBS_WEAK char *strncat(char *dest, const char *src, size_t n);
OBS_WEAK char *strpbrk(const char *s, const char *accept);
OBS_WEAK int strcasecmp(const char *a, const char *b);
OBS_WEAK long atol(const char *s);
OBS_WEAK long long strtoll(const char *s, char **end, int base);
OBS_WEAK long labs(long value);
OBS_WEAK int islower(int c);
OBS_WEAK int isalnum(int c);
OBS_WEAK int isprint(int c);
OBS_WEAK int ispunct(int c);

/* ---- why `div` and `ldiv` are counted and not called -------------------------
 *
 * They return a struct by value, which is an ABI path nothing else here exercises -
 * an eight-byte struct comes back in one register, a sixteen-byte one in two - so
 * they looked worth a check.
 *
 * **ISO C does not specify the member order.** It fixes that the type contains `quot`
 * and `rem` and leaves their order to the implementation, so any declaration here
 * would be inventing a layout, which is what D008 forbids.
 *
 * An order-independent check was written and then withdrawn (D052). It worked, but it
 * needed the host build to defer to the real `div_t`, and reaching that type means
 * including <stdlib.h> - which then collides with every other declaration in this
 * file, because glibc defines several of them inline. A check that cannot run under
 * `make host` cannot be told apart from a bug in itself, and that is worth more than
 * the check.
 *
 * They stay in the census. Presence is what can be honestly claimed about them.
 */

/* The rest of the settled C library surface. Every one of these has an answer ISO C
 * or POSIX fixes, which is what makes them worth checking rather than counting. */
OBS_WEAK long long atoll(const char *s);
OBS_WEAK unsigned long long strtoull(const char *s, char **end, int base);
OBS_WEAK long long llabs(long long value);
OBS_WEAK int strncasecmp(const char *a, const char *b, size_t n);
OBS_WEAK char *strdup(const char *s);
OBS_WEAK int sprintf(char *dest, const char *format, ...);
/* Wide characters are four bytes on this target's ABI. Declared here rather than
 * pulled from a header, because a freestanding build has none - and the census never
 * needed the type at all, since it only ever took addresses. */
typedef int obs_wchar;
OBS_WEAK size_t wcslen(const obs_wchar *s);
OBS_WEAK char *getenv(const char *name);

/* ---- the maths half of the C library ---------------------------------------
 *
 * Checked separately in 037-math. Every value these are checked against is exactly
 * representable in binary floating point, so the checks compare exactly and need no
 * tolerance - a tolerance is a place for a wrong answer to hide, and picking one
 * would be inventing a specification nobody wrote.
 */

OBS_WEAK double sqrt(double x);
OBS_WEAK double pow(double base, double exponent);
OBS_WEAK double fabs(double x);
OBS_WEAK double floor(double x);
OBS_WEAK double ceil(double x);
OBS_WEAK double fmod(double numerator, double denominator);
OBS_WEAK double sin(double x);
OBS_WEAK double cos(double x);
OBS_WEAK float sqrtf(float x);
OBS_WEAK float fabsf(float x);

/* Rounding, where implementations diverge in a way the standard does not permit.
 *
 * `round` is away from zero at the halfway point: round(2.5) is 3, round(-2.5) is -3.
 * The nearest-even convention used by the hardware rounding mode would give 2 and -2,
 * and a `round` written as "add a half and truncate" gets negatives wrong. Both are
 * common enough to be worth catching. */
OBS_WEAK double round(double x);
OBS_WEAK double trunc(double x);

/* Exponentials and logarithms. Checked only at points whose results are exact:
 * exp(0) is 1, log(1) is 0, log2(8) is 3, log10(1000) is 3. */
OBS_WEAK double exp(double x);
OBS_WEAK double log(double x);
OBS_WEAK double log2(double x);
OBS_WEAK double log10(double x);

/* The remaining trigonometry, checked at zero and one where the answers are exact.
 *
 * asin(1) is pi/2, which is not exactly representable, so it is not checked. A
 * tolerance there would be inventing a specification. */
OBS_WEAK double tan(double x);
OBS_WEAK double asin(double x);
OBS_WEAK double acos(double x);
OBS_WEAK double atan(double x);
OBS_WEAK double atan2(double y, double x);

/* The single-precision family.
 *
 * Worth checking separately rather than assuming they follow the double versions: an
 * implementation that forwards to the double version and returns the wide result is
 * numerically right and returns in the wrong register class, which corrupts the caller
 * rather than the answer. */
OBS_WEAK float floorf(float x);
OBS_WEAK float ceilf(float x);
OBS_WEAK float fmodf(float numerator, float denominator);
OBS_WEAK float powf(float base, float exponent);
OBS_WEAK float expf(float x);
OBS_WEAK float logf(float x);
OBS_WEAK float sinf(float x);
OBS_WEAK float cosf(float x);
OBS_WEAK float tanf(float x);

/* Text to floating point. "2.5" and "-0.25" are exactly representable, so these are
 * exact comparisons like the rest of 037-math. */
OBS_WEAK double strtod(const char *text, char **end);
OBS_WEAK float strtof(const char *text, char **end);

/* Writes one character and returns it, or EOF.
 *
 * The last-resort output channel: a byte at a time is slow and it is the only one of
 * the three that cannot mangle the report. `puts` would append a newline to every
 * chunk, and the report is line-oriented - so a channel that inserts lines produces
 * something that parses and is wrong, which is worse than nothing.
 *
 * Moved here from the census in surface.h. A name the program calls cannot also be
 * declared there: the census declares its names as `const char` precisely so the type
 * system forbids calling them. */
#if !defined(OBSCENE_HOST_BUILD)
OBS_WEAK int putchar(int c);

/* Writes a NUL-terminated string and a newline.
 *
 * An output channel, and the newline is why it is usable rather than a hazard: every
 * record this program emits is written in one call and ends in one, so the channel
 * drops ours and lets `puts` supply it. A channel that appended a newline to a partial
 * write would produce a report that parses and is wrong.
 *
 * Worth having because an emulator was found implementing `puts` and `printf` while
 * `write` succeeded and discarded the bytes. */
OBS_WEAK int puts(const char *s);

/* Ends the process.
 *
 * Called once, at the end of the run. The entry point is where the process starts,
 * not a function a loader calls and returns from - see src/start.c for what returning
 * actually did.
 *
 * Moved here from the census for the same reason as putchar: a name this program
 * calls cannot also be declared there, where names are `const char` precisely so the
 * type system forbids calling them. */
OBS_WEAK void exit(int status);
#endif

/* ---- libSceSysmodule ------------------------------------------------------- */

OBS_WEAK int sceSysmoduleLoadModule(uint16_t id);
OBS_WEAK int sceSysmoduleIsLoaded(uint16_t id);

/* ---- libSceUserService ----------------------------------------------------- */

OBS_WEAK int sceUserServiceInitialize(const void *params);
OBS_WEAK int sceUserServiceGetInitialUser(int32_t *user_id_out);
OBS_WEAK int sceUserServiceTerminate(void);

/* ---- libSceVideoOut -------------------------------------------------------- */

OBS_WEAK int sceVideoOutOpen(int user_id, int type, int index, const void *param);
OBS_WEAK int sceVideoOutClose(int handle);
OBS_WEAK int sceVideoOutSetFlipRate(int handle, int rate);

/* ---- libSceVideoOut: putting pixels on the screen ---------------------------
 *
 * Enough of the display path to draw the report. See src/display.c for why a probe
 * draws anything at all.
 *
 * The attribute structure's layout is deliberately not declared.
 * `sceVideoOutSetBufferAttribute` fills it, so this program never has to know what is
 * in it - which is the difference between a confident signature and an invented struct
 * (D008). It is passed as an opaque buffer, generously sized.
 */

/* Fills an attribute structure. The one call here that writes through its first
 * argument, and the reason no layout is declared. */
OBS_WEAK void sceVideoOutSetBufferAttribute(void *attribute, uint32_t pixel_format,
                                            uint32_t tiling_mode, uint32_t aspect_ratio,
                                            uint32_t width, uint32_t height,
                                            uint32_t pitch_in_pixels);
/* Hands the display a set of framebuffers to flip between. */
OBS_WEAK int sceVideoOutRegisterBuffers(int handle, int start_index,
                                        void *const *addresses, int buffer_count,
                                        const void *attribute);

/*
 * The current generation's forms of the two calls above.
 *
 * # Why both, rather than detecting the generation
 *
 * The two generations do not merely prefer different entry points - they expose
 * different ones, and neither exposes the other's:
 *
 *   shadPS4 (previous)   sceVideoOutRegisterBuffers        and not the `2` form
 *   PS5PCEM (current)    sceVideoOutRegisterBuffers2       and not the plain form
 *
 * So a module built for one generation and run on the other finds nothing, which is
 * exactly what `display|absent` meant on a current-generation loader (D127, D110).
 *
 * The obvious repair is to ask `005-generation` which console this is and branch. That
 * is the wrong instrument: it infers the generation from *other* symbols, when the
 * question "can I call this" is answered directly and without inference by whether this
 * symbol resolved. A weak import that came back null has already said no. So
 * `display.c` takes whichever pair is present, and a platform offering both - SharpEMU
 * tags both forms `Gen4 | Gen5` - simply gets the newer one.
 *
 * # These signatures are corroborated, not inferred
 *
 * D008 forbids calling a function whose argument shape is uncertain, and a framebuffer
 * descriptor is the worst place to be wrong. Two independent implementations agree:
 *
 *   PS5PCEM  a Zig signature naming every parameter in order
 *   SharpEMU register-by-register - rdi, rsi, rdx, rcx, r8, r9, then the stack
 *
 * Both give the same eight arguments in the same order for the attribute call, and the
 * same eight for the register call. They also agree on where each field lands inside
 * the structure, which is the part a wrong guess would corrupt.
 *
 * The structure is still passed as an opaque buffer for the same reason the previous
 * generation's is: the platform fills it, so this program never needs its layout - only
 * that the buffer is large enough. Knowing the layout and declining to declare it is
 * the stronger position, because the agreement above is about two emulators rather than
 * about hardware.
 */
OBS_WEAK void sceVideoOutSetBufferAttribute2(void *attribute, uint64_t pixel_format,
                                             uint32_t tiling_mode, uint32_t width,
                                             uint32_t height, uint64_t option,
                                             uint32_t dcc_control,
                                             uint64_t dcc_clear_color);
/* The current generation's register call. Two arguments more than the previous form: a
 * set index before the buffer index, and a category and option after the attribute. */
OBS_WEAK int sceVideoOutRegisterBuffers2(int handle, int set_index, int start_index,
                                         const void *buffers, int buffer_count,
                                         const void *attribute, int category,
                                         void *option);
/* Puts a registered buffer on the screen. */
OBS_WEAK int sceVideoOutSubmitFlip(int handle, int buffer_index, uint32_t flip_mode,
                                   int64_t flip_argument);

/* ---- libSceAudioOut -------------------------------------------------------- */

OBS_WEAK int sceAudioOutInit(void);
OBS_WEAK int sceAudioOutOpen(int user_id, int type, int index, uint32_t length,
                             uint32_t frequency, uint32_t param);
OBS_WEAK int sceAudioOutClose(int handle);

/* ---- libScePad ------------------------------------------------------------- */

OBS_WEAK int scePadInit(void);
OBS_WEAK int scePadOpen(int user_id, int type, int index, const void *param);
OBS_WEAK int scePadClose(int handle);
/* Reads the controller's current state into a caller-provided buffer.
 *
 * The buffer is `void *`, not a struct, on purpose: this program reads one field from
 * it - the button bitfield at offset 0 - and declaring the whole `ScePadData` layout
 * would be inventing the parts it does not use (D008). The caller passes a buffer
 * comfortably larger than the real structure and reads the first word; over-sizing a
 * stack buffer is safe, and under-sizing it is what the size guess would risk. The
 * button offset and masks come from the OpenOrbis SDK, an open-source toolchain, which
 * is a permitted provenance source. */
OBS_WEAK int scePadReadState(int handle, void *data);

/* ---- libSceKeyboard -------------------------------------------------------- */

/* A USB keyboard, so the report can be paged from something other than a DualSense - a
 * KVM sends keys, not pad input, which is how this gets driven and verified without a
 * controller.
 *
 * `sceKeyboardReadState` takes a `void *` for the same reason `scePadReadState` does:
 * only the keycode array is read, at the offset the OpenOrbis sample validates by using
 * it, and the fields that header marks uncertain (`XXX: is it 64-bit?`) are never
 * touched. Arities and the keycode layout are from the OpenOrbis toolchain and its
 * working keyboard sample. */
OBS_WEAK int sceKeyboardInit(void);
OBS_WEAK int sceKeyboardOpen(int user_id, int type, int index, void *param);
OBS_WEAK int sceKeyboardReadState(int handle, void *data);

#if defined(__cplusplus)
}
#endif

/* ---- ABI constants ---------------------------------------------------------
 *
 * Same rule as the declarations above: these are interface facts, and a wrong one
 * produces a call that succeeds and does the wrong thing. Only values this project
 * is confident about appear here.
 */

/* Direct-memory types.
 *
 * # What "onion" and "garlic" are, since the names explain nothing on their own
 *
 * They are **AMD's own names for the two buses on an APU**, not console terminology,
 * and they are described in AMD's published APU architecture material and in
 * open-source graphics driver work. Anything with an AMD APU has both; this console is
 * one.
 *
 * On an APU the CPU and GPU share physical memory, and there are two paths between the
 * GPU and that memory:
 *
 *   **Onion** - the *coherent* path. It runs through the CPU's cache-coherency fabric,
 * so GPU accesses snoop CPU caches and the two always agree about what is in memory.
 * Lower bandwidth for the GPU, and nothing has to be flushed.
 *
 *   **Garlic** - the *fast* path. It goes straight to memory, bypassing the CPU cache
 *   hierarchy. Full bandwidth for the GPU, and **not coherent**: what the CPU has
 * written is not necessarily visible until the CPU's write buffers drain. CPU *reads*
 * over it are famously slow, because nothing is cached.
 *
 * The `WB`/`WC` half of each name is the CPU-side caching policy - write-back (cached)
 * or write-combining (uncached, buffered until a buffer fills or is fenced).
 *
 * # Which one a framebuffer this program draws wants
 *
 * `WC_GARLIC` is the conventional choice for a framebuffer, and that convention assumes
 * the *GPU* is filling it. This program fills it with the CPU, one pixel at a time, and
 * then submits a flip - so the display can scan out write-combined data that has not
 * finished draining. `WB_ONION` is cached and coherent, which is what a CPU-drawn
 * buffer wants.
 *
 * Measured here: **both are accepted** by `sceVideoOutRegisterBuffers` once the buffer
 * is aligned to `0x10000` (D253), so the choice is about what the display *sees* rather
 * than about whether it accepts the buffer at all.
 *
 * # Provenance
 *
 * The bus semantics above are public AMD architecture, independent of any console. The
 * three *numbers* below are this platform's own encoding and are ABI constants like
 * every other value in this section - only ones this project is confident about appear
 * here. (D254)
 */
#define OBS_MEM_TYPE_WB_ONION 0
#define OBS_MEM_TYPE_WC_GARLIC 3
#define OBS_MEM_TYPE_WB_GARLIC 10

/* Mapping protection bits. The CPU and GPU halves are separate ranges, which is why
 * a value that looks like a POSIX PROT_ constant is not one. */
#define OBS_PROT_CPU_READ 0x01
#define OBS_PROT_CPU_WRITE 0x02
#define OBS_PROT_CPU_RW (OBS_PROT_CPU_READ | OBS_PROT_CPU_WRITE)
#define OBS_PROT_GPU_READ 0x10
#define OBS_PROT_GPU_WRITE 0x20

/* Open flags, which follow POSIX rather than anything vendor-specific. */
/* Seek origins. POSIX fixes these values, so they are settled rather than assumed:
 * SEEK_SET is 0, SEEK_CUR is 1, SEEK_END is 2, and every system in this family agrees.
 *
 * Only SEEK_CUR is used so far, by the relation that asks whether a descriptor is still
 * live after its sibling was closed - seeking zero from the current position moves
 * nothing and needs no buffer, which makes it the least invasive way to ask. */
/* The main video output. Zero, and the only bus any of these calls is given.
 *
 * Was defined separately in `src/display.c` and `src/sections/media.c`, and a third
 * copy was about to be written. An ABI constant with two definitions is two things to
 * get wrong; it lives here with the rest of them now. */
#define OBS_VIDEO_BUS_MAIN 0

#define OBS_SEEK_SET 0
#define OBS_SEEK_CUR 1
#define OBS_SEEK_END 2

#define OBS_O_RDONLY 0x0000
#define OBS_O_WRONLY 0x0001
#define OBS_O_RDWR 0x0002

/* Descriptor numbers, fixed by POSIX rather than by the vendor. */
#define OBS_FD_STDOUT 1

/* An invalid descriptor, used for the negative checks. -1 is never a valid
 * descriptor on any POSIX-derived system, so a function that accepts it has failed
 * to validate its argument. */
#define OBS_FD_INVALID (-1)

/* An invalid handle for the subsystem APIs, used the same way. */
#define OBS_HANDLE_INVALID (-1)
/* A handle representing the running process itself. */
#define OBS_HANDLE_SELF 0

/* ---- networking (libSceNet) ---------------------------------------------------------
 *
 * The first platform functions obSCEne *calls* over the network rather than merely
 * censuses, so the signatures are held to D008: a wrong arity here corrupts the stack
 * and surfaces somewhere unrelated to networking, which is the worst kind of wrong to
 * debug on a target that took effort to reach.
 *
 * They are confirmed from two independent public sources that agree exactly - the
 * OpenOrbis toolchain headers (`include/orbis/Net.h`) and shadPS4's own `libSceNet`
 * implementation - including the unusual first argument to `sceNetSocket`: a name
 * string the platform keeps for its own debugging. Two sources rather than one, the
 * same control discipline D097 applies to loaders.
 *
 * These back the console socket transport in `src/net_target.c`. It works against an
 * emulator whose net layer maps guest sockets onto host sockets - shadPS4 does, its
 * `PosixSocket::bind` calls the host `::bind` - so a guest `sceNetListen` opens a real
 * host port and the driver connects from outside with no console present. */
#define OBS_NET_AF_INET 2
#define OBS_NET_SOCK_STREAM 1
#define OBS_NET_IPPROTO_TCP 6

/* The platform's sockaddr_in, and **not** the host's layout.
 *
 * A leading length byte, a one-byte family, and a `sin_vport` the host form does not
 * carry. Copied field-for-field from the confirmed definition: guessing it matches the
 * host would misread the port and address silently, which is a bound socket on the
 * wrong endpoint rather than an error. */
typedef struct obs_net_sockaddr_in {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port; /* network byte order */
    uint32_t sin_addr; /* network byte order */
    uint16_t sin_vport;
    char sin_zero[6];
} obs_net_sockaddr_in;

OBS_WEAK int sceNetInit(void);
OBS_WEAK int sceNetTerm(void);

/* ---- libSceNetCtl: the network configuration ------------------------------
 *
 * For the console's own IP address. `sceNetCtlGetInfo` takes a selector code and fills
 * a `void *` info union; only code 14 (the IP address) is used and only the string at
 * offset 0 of what it writes is read, so the union's full layout is not depended on -
 * the buffer is over-sized past the union's largest member so the call cannot overrun
 * it. Codes and the union from the OpenOrbis toolchain headers, an open-source source.
 * `sceNetCtlInit` is idempotent: calling it when it is already initialised returns an
 * already-done code, which this ignores. */
OBS_WEAK int sceNetCtlInit(void);
OBS_WEAK int sceNetCtlGetInfo(int code, void *info);
OBS_WEAK int sceNetSocket(const char *name, int family, int type, int protocol);
OBS_WEAK int sceNetBind(int s, const void *addr, uint32_t addrlen);
OBS_WEAK int sceNetListen(int s, int backlog);
OBS_WEAK int sceNetAccept(int s, void *addr, uint32_t *paddrlen);
OBS_WEAK int sceNetRecv(int s, void *buf, uint64_t len, int flags);
OBS_WEAK int sceNetSend(int s, const void *buf, uint64_t len, int flags);
OBS_WEAK int sceNetSocketClose(int s);

/* The GPU command-building half of libSceGnmDriver.
 *
 * These two build PM4 command packets into a caller-supplied buffer and return - they
 * touch no GPU and submit nothing, so they are safe to call, and their output is the
 * PM4 encoding a command-processor emulator must parse. Only these two: their arities
 * are confirmed by two independent open reimplementations that agree exactly (shadPS4
 * and GPCS4). `sceGnmSetCsShader` is deliberately absent - those two sources disagree
 * on whether it takes three arguments or four (a trailing modifier), and D008 forbids
 * calling a function whose arity is uncertain. The submitting and shader-binding calls
 * stay in the census, uncalled, for the same reason.
 *
 * The provenance is OBS_FROM_ASSUMED, not a spec: the vendor documents none of this,
 * and the confirmation is two emulators agreeing, which is strong evidence but not a
 * document. */
OBS_WEAK uint32_t sceGnmDispatchInitDefaultHardwareState(uint32_t *cmdbuf,
                                                         uint32_t size);
OBS_WEAK int32_t sceGnmDispatchDirect(uint32_t *cmdbuf, uint32_t size,
                                      uint32_t threads_x, uint32_t threads_y,
                                      uint32_t threads_z, uint32_t flags);

#endif /* OBSCENE_PLATFORM_H */
