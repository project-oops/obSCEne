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
OBS_WEAK int sceKernelConvertUtcToLocaltime(int64_t utc, int64_t *local, int *timesec,
                                            int *dst);

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
OBS_WEAK int sceKernelReserveVirtualRange(void **addr_out, size_t len, int flags,
                                          size_t alignment);
OBS_WEAK int sceKernelGetSystemSwVersion(void *version);
OBS_WEAK int *__error(void);

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

/* The stack an attribute set describes: where it starts, and how big it is.
 *
 * # Where the shapes came from
 *
 * `scePthreadAttrGetstacksize` was censused for presence until now. Both getters take
 * an attribute object and one out-pointer, which is FreeBSD's shape for
 * `pthread_attr_getstackaddr(3)` and `pthread_attr_getstacksize(3)` and is what three
 * retail titles pass - a Unity shim calls Get, then these two, once each, on itself.
 *
 * **The out-parameters are pointer-wide, not `int`.** An address written as four bytes
 * leaves the caller's neighbouring variable holding the top half, which is the bug the
 * sibling project's D272 records for the `int`-shaped getters beside these.
 *
 * What the address *means* - the lowest byte of the stack, as FreeBSD's is, or its top
 * - is the open question `031-stackattr` exists to settle, and nothing here assumes it.
 */
OBS_WEAK int scePthreadAttrGetstackaddr(const ScePthreadAttr *attr, void **address);
OBS_WEAK int scePthreadAttrGetstacksize(const ScePthreadAttr *attr, size_t *size);

/* Fill an already-initialised attribute set from a *running* thread.
 *
 * FreeBSD's `pthread_attr_get_np(3)` is the reference for the shape - `(thread, attr)`,
 * the object initialised beforehand, and it comes back describing the thread as it is
 * rather than as it was asked for. The argument order is read off three retail titles:
 * the first register holds the handle `scePthreadSelf` answered a moment earlier.
 *
 * This is the call the sibling emulator left unimplemented while three titles used it
 * to find the bottom of the stack their garbage collector scans; each then scanned off
 * the top of the real stack (its D575). What it reports is worth measuring rather than
 * inferring. */
OBS_WEAK int scePthreadAttrGet(ScePthread thread, ScePthreadAttr *attr);
OBS_WEAK int scePthreadAttrSetstacksize(ScePthreadAttr *attr, size_t size);
OBS_WEAK int scePthreadAttrSetaffinity(ScePthreadAttr *attr, uint64_t mask);
OBS_WEAK int scePthreadAttrGetaffinity(const ScePthreadAttr *attr, uint64_t *mask);
OBS_WEAK int scePthreadAttrSetschedparam(ScePthreadAttr *attr, const void *param);
OBS_WEAK int scePthreadAttrGetschedparam(const ScePthreadAttr *attr, void *param);
OBS_WEAK int scePthreadGetprio(ScePthread thread, int *prio);
OBS_WEAK int scePthreadSetprio(ScePthread thread, int prio);
OBS_WEAK int scePthreadGetaffinity(ScePthread thread, uint64_t *mask);
OBS_WEAK int scePthreadSetaffinity(ScePthread thread, uint64_t mask);

/* ---- libkernel_sync_on_address: waiting on a word --------------------------
 *
 * The platform's futex is NOT declared here as a linked import, and that is the whole
 * point. Its export library `libkernel_sync_on_address` is a namespace inside
 * `libkernel.sprx`, not a loadable module of its own - so a linked import made the
 * title module declare a `needed_module` for a `.sprx` that does not exist, and the
 * title died on load. `032-syncaddr` resolves `sceKernelSyncOnAddressWait`/`Wake` by
 * name through `libkernel` at run time, the way `017-posix` and `019-posixerr` resolve
 * theirs, so the module carries no dependency on a phantom library. (Reverts D322's
 * linked import.)
 *
 * # The shape, for the host declaration below and the runtime cast in the section
 *
 * FreeBSD `_umtx_op(2)` is the citable analogue: the wait is `UMTX_OP_WAIT`, the wake
 * `UMTX_OP_WAKE`. Two arguments each - an address and a value to compare, an address
 * and a count to wake - which is what three retail titles pass, every observed call
 * leaving the third register zero. A timeout is deliberately not modelled (its unit is
 * unestablished; `docs/backlog/024`), and both values are 64-bit so the whole register
 * is defined whichever width `032-syncaddr` finds the comparison reads. */
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
/* The sanctioned way past W^X for a recompiler: create a shared object, then map it
 * writable and executable separately. Signatures are documented (ps4libdoc /
 * OpenOrbis). Used by 155-jit, the go/no-go for native emulation. */
OBS_WEAK int sceKernelJitCreateSharedMemory(const char *name, size_t len, int max_prot,
                                            int *handle_out);
OBS_WEAK int sceKernelJitMapSharedMemory(int handle, int prot, void **addr_out);
OBS_WEAK int sceKernelJitCreateAliasOfSharedMemory(int handle, int prot,
                                                   void **addr_out);
OBS_WEAK int sceKernelVirtualQuery(const void *addr, int flags, void *info,
                                   size_t info_size);
OBS_WEAK int sceKernelMunmap(void *addr, size_t len);
OBS_WEAK int sceKernelProtectDirectMemory(sce_off_t paddr, size_t len,
                                          unsigned int flags);
OBS_WEAK int sceKernelProtectDirectMemoryForPID(sce_off_t paddr, size_t len, int prot,
                                                int pid);
OBS_WEAK int sceKernelBatchMap(const void *entries, int count, int *completed);
OBS_WEAK int sceKernelMemoryPoolExpand(sce_off_t searchStart, sce_off_t searchEnd,
                                       size_t len, size_t align, sce_off_t *physOut);
OBS_WEAK int sceKernelMemoryPoolReserve(void *addrIn, size_t len, size_t align,
                                        uint32_t flags, void **addrOut);
OBS_WEAK int sceKernelMemoryPoolCommit(void *addr, size_t len, uint32_t type, int prot,
                                       uint32_t flags);
OBS_WEAK int sceKernelMemoryPoolDecommit(void *addr, size_t len, uint32_t flags);
OBS_WEAK int sceKernelSetPrtAperture(int slot, void *base, size_t len);
OBS_WEAK int sceKernelGetPrtAperture(int slot, void **baseOut, size_t *lenOut);
OBS_WEAK int sceKernelQueryMemoryProtection(const void *addr, uint64_t *start,
                                            uint64_t *end, uint32_t *prot);

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
OBS_WEAK void scePthreadExit(void *value);

/* ---- libkernel: fault recovery --------------------------------------------
 *
 * The signal primitives the fault guard installs its handler with (fault.c, D325/D326).
 * They are declared imports, not resolved by dlsym alone, because a native title's
 * dlsym resolves only the symbols the process already imports - so the guard went
 * unarmed on the eboot until these were bound like every other call. All are real
 * libkernel exports (data/hardware/libkernel-vaddrs.txt: _sigaction 0xd100,
 * _sigprocmask 0xcf70; scePthreadExit above, in data/hardware/ps5-full.txt). `struct
 * sigaction` and `sigset_t` cross as raw bytes
 * - the guard builds the FreeBSD amd64 layout itself rather than borrow a vendor header
 * (Principle 6). setjmp/longjmp are not imported: the guard carries its own (fault.c).
 */
OBS_WEAK int _sigaction(int sig, const void *act, void *oact);
OBS_WEAK int _sigprocmask(int how, const void *set, void *oset);

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

#if defined(OBSCENE_HOST_BUILD)
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

/* The futex, host side only. On the target `032-syncaddr` resolves these by name; on
 * the host it takes their address directly, and the stubs live in host_stubs.c. */
OBS_WEAK int sceKernelSyncOnAddressWait(void *address, uint64_t value);
OBS_WEAK int sceKernelSyncOnAddressWake(void *address, uint64_t count);
#endif

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

/* ---- libkernel: event queues ----------------------------------------------- */
OBS_WEAK int sceKernelCreateEqueue(void *eq, const char *name);
OBS_WEAK int sceKernelDeleteEqueue(uint64_t eq);
OBS_WEAK int sceKernelWaitEqueue(uint64_t eq, void *event, int num_events,
                                 int *out_events, void *timeout);

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
OBS_WEAK int sceSysmoduleUnloadModule(uint16_t id);
OBS_WEAK int sceSysmoduleIsLoaded(uint16_t id);
OBS_WEAK int sceSysmoduleLoadModuleInternal(uint32_t id);
OBS_WEAK int sceSysmoduleLoadModuleByNameInternal(const char *name, uint64_t a2,
                                                  uint64_t a3, uint64_t a4, uint64_t a5,
                                                  uint64_t a6);
OBS_WEAK int sceSysmoduleGetModuleHandleInternal(uint32_t id, int *handle);

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
OBS_WEAK int sceUserServiceGetAgeLevel(int32_t user_id, int32_t *age_level_out);
OBS_WEAK int sceUserServiceGetUserName(int32_t user_id, char *name_out, size_t size);
OBS_WEAK int sceUserServiceTerminate(void);

/* ---- libSceVideoOut -------------------------------------------------------- */

OBS_WEAK int sceVideoOutOpen(int user_id, int type, int index, const void *param);
OBS_WEAK int sceVideoOutClose(int handle);
OBS_WEAK int sceVideoOutSetFlipRate(int handle, int rate);
OBS_WEAK int sceVideoOutIsOutputSupported(int port, int bus);

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
/* Unregisters a buffer set previously registered on the video handle. */
OBS_WEAK int sceVideoOutUnregisterBuffers(int handle, int set_index);
/* Puts a registered buffer on the screen. */
OBS_WEAK int sceVideoOutSubmitFlip(int handle, int buffer_index, uint32_t flip_mode,
                                   int64_t flip_argument);

/* ---- libSceAudioOut -------------------------------------------------------- */

OBS_WEAK int sceAudioOutInit(void);
OBS_WEAK int sceAudioOutOpen(int user_id, int type, int index, uint32_t length,
                             uint32_t frequency, uint32_t param);
OBS_WEAK int sceAudioOutClose(int handle);
/* The behavioural half of the audio surface, called rather than censused so the SDK can
 * settle what a header states without evidence: whether a selector of 0 opens stereo or
 * mono (the port-state record carries the channel count), which chunk sizes an open
 * accepts, whether output blocks, and what the volume flags mean. `sceAudioOutOutput`
 * takes a `const void *` sample buffer; `sceAudioOutGetPortState` writes a state record
 * read as bytes (its layout is what the probe is measuring, so none is declared);
 * `sceAudioOutSetVolume` takes a flag and a volume array. Arities from the OpenOrbis
 * toolchain, a permitted source. */
OBS_WEAK int sceAudioOutOutput(int handle, const void *ptr);
OBS_WEAK int sceAudioOutGetPortState(int handle, void *state);
OBS_WEAK int sceAudioOutSetVolume(int handle, int flag, int *vol);

/* ---- libScePad ------------------------------------------------------------- */

OBS_WEAK int scePadInit(void);
OBS_WEAK int scePadOpen(int user_id, int type, int index, const void *param);
OBS_WEAK int scePadClose(int handle);
OBS_WEAK int scePadResetOrientation(int handle);
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
/* The batched read, called rather than censused so the SDK can confirm the record
 * stride: `scePadRead` returns how many records it filled and writes that many into the
 * buffer, so the write extent divided by the count is the stride a batched reader must
 * use. Same `void *` reasoning as `scePadReadState` - only the extent and the button
 * word are read. (num is the count requested.) Arity from the OpenOrbis toolchain, a
 * permitted source. */
OBS_WEAK int scePadRead(int handle, void *data, int num);

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
OBS_WEAK int sceKeyboardClose(int handle);
OBS_WEAK int sceKeyboardRead(int handle, void *data, int num);
OBS_WEAK int sceKeyboardReadState(int handle, void *data);
OBS_WEAK int sceKeyboardSetProcessPrivilege(int privilege);
OBS_WEAK int sceKeyboardSetProcessFocus(int focus);

/* ---- libSceMouse ----------------------------------------------------------- */
OBS_WEAK int sceMouseInit(void);
OBS_WEAK int sceMouseOpen(int user_id, int type, int index, void *param);
OBS_WEAK int sceMouseClose(int handle);
OBS_WEAK int sceMouseRead(int handle, void *data, int num);

/* ---- libSceVideodec2: video decode ----------------------------------------- */
OBS_WEAK int sceVideodec2CreateDecoder(const void *config, void *queue,
                                       void *decoder_out);
OBS_WEAK int sceVideodec2DeleteDecoder(void *decoder);
OBS_WEAK int sceVideodec2Decode(void *decoder, const void *input, void *frame_out,
                                void *picture_out);
OBS_WEAK int sceVideodec2Flush(void *decoder, void *frame_out, void *picture_out);
OBS_WEAK int sceVideodec2Reset(void *decoder);
OBS_WEAK int sceVideodec2QueryComputeMemoryInfo(void *info);
OBS_WEAK int sceVideodec2QueryDecoderMemoryInfo(const void *config, void *info);
OBS_WEAK int sceVideodec2AllocateComputeQueue(const void *queue_info, void *queue_out);
OBS_WEAK int sceVideodec2ReleaseComputeQueue(void *queue);
OBS_WEAK int sceVideodec2MapDirectMemory(void *addr, size_t size);
OBS_WEAK int sceVideodec2GetPictureInfo(const void *picture_out, void *p1, void *p2);
OBS_WEAK int sceVideodec2GetAvcPictureInfo(const void *picture_out, void *p1, void *p2);

/* ---- libSceAudiodec: audio decode ------------------------------------------ */
OBS_WEAK int sceAudiodecInitLibrary(uint32_t codec_type);
OBS_WEAK int sceAudiodecTermLibrary(uint32_t codec_type);
OBS_WEAK int sceAudiodecCreateDecoder(const void *ctrl, int codec_type);
OBS_WEAK int sceAudiodecDeleteDecoder(int handle);
OBS_WEAK int sceAudiodecDecode(int handle, void *au_info, void *pcm_item);
OBS_WEAK int sceAudiodecDecode2(int handle, void *au_info, void *pcm_item);
OBS_WEAK int sceAudiodecDecodeWithPriority(int handle, void *au_info, void *pcm_item,
                                           int prio);
OBS_WEAK int sceAudiodecDecode2WithPriority(int handle, void *au_info, void *pcm_item,
                                            int prio);
OBS_WEAK int sceAudiodecClearContext(int handle);

/* ---- Extended DualSense (libScePad) ---------------------------------------- */
OBS_WEAK int scePadSetTriggerEffect(int handle, const void *param);
OBS_WEAK int scePadGetTriggerEffectState(int handle, void *param);
OBS_WEAK int scePadSetVibrationMode(int handle, int mode);
OBS_WEAK int scePadSetVibrationForce(int handle, const void *param);
OBS_WEAK int scePadGetControllerInformation(int handle, void *info);

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

/* The main audio-out port type. 0 is `SCE_AUDIO_OUT_PORT_TYPE_MAIN` in the OpenOrbis
 * toolchain, an open-source and so permitted source; a named constant rather than a
 * bare 0 in the audio probes. */
#define OBS_AUDIO_OUT_PORT_TYPE_MAIN 0

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
OBS_WEAK int sceNetCtlTerm(void);
OBS_WEAK int sceNetCtlGetInfo(int code, void *info);
OBS_WEAK int sceNetCtlGetState(int *state);
OBS_WEAK int sceNetCtlGetResult(int eventType, int *errorCode);
OBS_WEAK int sceNetCtlGetNatInfo(void *natInfo);
OBS_WEAK int sceNetCtlRegisterCallback(void (*handler)(int eventType, void *arg),
                                       void *arg, int *cid);
OBS_WEAK int sceNetCtlUnregisterCallback(int cid);
OBS_WEAK int sceNetCtlCheckCallback(void);
OBS_WEAK int sceNetSocket(const char *name, int family, int type, int protocol);
OBS_WEAK int sceNetBind(int s, const void *addr, uint32_t addrlen);
OBS_WEAK int sceNetListen(int s, int backlog);
OBS_WEAK int sceNetAccept(int s, void *addr, uint32_t *paddrlen);
OBS_WEAK int sceNetRecv(int s, void *buf, uint64_t len, int flags);
OBS_WEAK int sceNetSend(int s, const void *buf, uint64_t len, int flags);
OBS_WEAK int sceNetSendto(int s, const void *msg, size_t len, int flags, const void *to,
                          uint32_t tolen);
OBS_WEAK int sceNetRecvfrom(int s, void *buf, size_t len, int flags, void *from,
                            uint32_t *fromlen);
OBS_WEAK int sceNetGetsockname(int s, void *name, uint32_t *namelen);
OBS_WEAK int sceNetSocketClose(int s);
OBS_WEAK int sceHttpUriParse(void *out, const char *uri, void *pool, uint64_t *required,
                             uint64_t poolSize);
/* The socket-option and connect calls Porthole's accept loop depends on, called rather
 * than censused so the probe can settle what two public sources disagree on: the
 * non-blocking option value (0x1200 in the OpenOrbis/CTurt headers, 0x1100 in vitasdk)
 * and the would-block code a non-blocking recv returns. `sceNetConnect` is here for the
 * accept-inherits probe's loopback self-connect. Arities from the OpenOrbis toolchain,
 * a permitted source. */
OBS_WEAK int sceNetSetsockopt(int s, int level, int optname, const void *optval,
                              uint32_t optlen);
OBS_WEAK int sceNetConnect(int s, const void *addr, uint32_t addrlen);

/* Socket-option and flag constants the net probes name rather than pass as bare
 * numbers. The two SO_NBIO values are the disagreeing public sources (D329); the probe
 * tries 0x1200 first. */
#define OBS_NET_SOL_SOCKET 0xFFFF
#define OBS_NET_SO_NBIO_OPENORBIS 0x1200
#define OBS_NET_SO_NBIO_VITASDK 0x1100
#define OBS_NET_MSG_DONTWAIT 0x80

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

/* ---- libSceAgc: current-generation GPU command-building and shaders --------
 *
 * Command builders that write into a caller-supplied command buffer preceded by
 * a small writer struct {begin, end} sitting 0x38 in front of the buffer.
 * In System V AMD64, passing up to 6 register arguments avoids stack corruption
 * for any arity <= 6.
 */
OBS_WEAK uint64_t sceAgcCbNop(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                              uint64_t arg4, uint64_t arg5) __asm__("$LtTouSCZjHM");
OBS_WEAK uint64_t sceAgcCbReleaseMem(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$wr23dPKyWc0");
OBS_WEAK uint64_t sceAgcDcbDmaData(void *arg0, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, uint64_t arg4,
                                   uint64_t arg5) __asm__("$WmAc2MEj6Io");
OBS_WEAK uint64_t sceAgcDcbWaitRegMem(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$VmW0Tdpy420");
OBS_WEAK uint64_t sceAgcDcbResetQueue(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$TRO721eVt4g");
/* The `*GetSize` siblings.
 *
 * What the library says a builder will consume, asked *before* the builder is called.
 * The guest reserves that many bytes and the builder then writes into the reservation,
 * so the two are one contract and neither number is knowable here in advance - see
 * `166-agc/<builder>-getsize` and D331.
 *
 * Declared with the generic six-register signature for the same reason the builders
 * above are: the arity is not established, and System V AMD64 makes a call with six
 * register arguments safe for any arity <= 6. A narrower declaration would assert an
 * argument count nobody here has measured.
 *
 * The labels are the identifiers the loader resolves. Each was derived from the name
 * with `obscene-tool nid` and then checked against `data/mined-names.txt`, which agrees
 * on all four from five independent sources.
 */
OBS_WEAK uint64_t sceAgcCbNopGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$t7PlZ9nt5Lc");
OBS_WEAK uint64_t sceAgcDcbDmaDataGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                          uint64_t arg3, uint64_t arg4,
                                          uint64_t arg5) __asm__("$2ccJz9LQI+w");
OBS_WEAK uint64_t sceAgcDcbSetIndexCountGetSize(uint64_t arg0, uint64_t arg1,
                                                uint64_t arg2, uint64_t arg3,
                                                uint64_t arg4,
                                                uint64_t arg5) __asm__("$mljzuGDZRQ4");
OBS_WEAK uint64_t sceAgcDcbSetUcRegisterDirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$aP1Ki9G3++4");
/* The jump pair. Two independent reimplementations disagree on this builder's dword
 * count, and one of them explains its extra dword as a predication slot another call
 * writes into the packet afterwards. That makes it the cheapest live disagreement the
 * reservation-versus-write invariant can settle, so it is the first pair added after
 * the original four (D331). */
OBS_WEAK uint64_t sceAgcDcbJump(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5) __asm__("$xSAR0LTcRKM");
OBS_WEAK uint64_t sceAgcDcbJumpGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                       uint64_t arg3, uint64_t arg4,
                                       uint64_t arg5) __asm__("$VEGu4dixjUg");
/* The ten builders whose reservation is measured against their write, and the eleven
 * `*GetSize` siblings. Generic six-register declarations: the arity of none of these is
 * established, and System V AMD64 makes a six-register call safe for any arity <= 6.
 * Every label was derived with `obscene-tool nid` and agrees with data/mined-names.txt.
 * `sceAgcCbQueueEndOfPipeAction` has no attested builder, so only its size is read.
 * (D331) */
OBS_WEAK uint64_t sceAgcAcbAcquireMem(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$KT-hTp-Ch14");
OBS_WEAK uint64_t sceAgcAcbDmaData(void *arg0, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, uint64_t arg4,
                                   uint64_t arg5) __asm__("$-RnpfpxIhec");
OBS_WEAK uint64_t sceAgcAcbJump(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5) __asm__("$e1DFTg+Sd8U");
OBS_WEAK uint64_t sceAgcCbBranch(void *arg0, uint64_t arg1, uint64_t arg2,
                                 uint64_t arg3, uint64_t arg4,
                                 uint64_t arg5) __asm__("$w1KFAHVqpaU");
OBS_WEAK uint64_t sceAgcDcbAcquireMem(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$57labkp+rSQ");
OBS_WEAK uint64_t sceAgcDcbDrawIndexIndirect(void *arg0, uint64_t arg1, uint64_t arg2,
                                             uint64_t arg3, uint64_t arg4,
                                             uint64_t arg5) __asm__("$t1vNu082-jM");
OBS_WEAK uint64_t
sceAgcDcbDrawIndexIndirectMulti(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5) __asm__("$ypVBz4uPKcQ");
OBS_WEAK uint64_t sceAgcDcbGetLodStats(void *arg0, uint64_t arg1, uint64_t arg2,
                                       uint64_t arg3, uint64_t arg4,
                                       uint64_t arg5) __asm__("$vuSXe69VILM");
OBS_WEAK uint64_t sceAgcDcbRewind(void *arg0, uint64_t arg1, uint64_t arg2,
                                  uint64_t arg3, uint64_t arg4,
                                  uint64_t arg5) __asm__("$zfcxg-ewMK8");
OBS_WEAK uint64_t sceAgcDcbStallCommandBufferParser(
    void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$u2T2DiA5hRI");
OBS_WEAK uint64_t sceAgcAcbAcquireMemGetSize(uint64_t arg0, uint64_t arg1,
                                             uint64_t arg2, uint64_t arg3,
                                             uint64_t arg4,
                                             uint64_t arg5) __asm__("$ewobAQeMo5k");
OBS_WEAK uint64_t sceAgcAcbDmaDataGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                          uint64_t arg3, uint64_t arg4,
                                          uint64_t arg5) __asm__("$M0ttm8h7SKA");
OBS_WEAK uint64_t sceAgcAcbJumpGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                       uint64_t arg3, uint64_t arg4,
                                       uint64_t arg5) __asm__("$b-oySn+G2tE");
OBS_WEAK uint64_t sceAgcCbBranchGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                        uint64_t arg3, uint64_t arg4,
                                        uint64_t arg5) __asm__("$uZW-mqsxkrM");
OBS_WEAK uint64_t sceAgcCbQueueEndOfPipeActionGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$hL7C0IRpWZI");
OBS_WEAK uint64_t sceAgcDcbAcquireMemGetSize(uint64_t arg0, uint64_t arg1,
                                             uint64_t arg2, uint64_t arg3,
                                             uint64_t arg4,
                                             uint64_t arg5) __asm__("$-vnlTPPXPrw");
OBS_WEAK uint64_t sceAgcDcbDrawIndexIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$mStuvI0zOtc");
OBS_WEAK uint64_t sceAgcDcbDrawIndexIndirectMultiGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$r98I08t+LOg");
OBS_WEAK uint64_t sceAgcDcbGetLodStatsGetSize(uint64_t arg0, uint64_t arg1,
                                              uint64_t arg2, uint64_t arg3,
                                              uint64_t arg4,
                                              uint64_t arg5) __asm__("$rUuVjyR+Rd4");
OBS_WEAK uint64_t sceAgcDcbRewindGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                         uint64_t arg3, uint64_t arg4,
                                         uint64_t arg5) __asm__("$QIXCsbipds0");
OBS_WEAK uint64_t sceAgcDcbStallCommandBufferParserGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$+u6dKSLWM2o");
OBS_WEAK int sceAgcInit(void *state, uint32_t version) __asm__("$23LRUSvYu1M");
OBS_WEAK int sceAgcInit_alias(void *state, uint32_t version) __asm__("$23LRUSvYu1M");
OBS_WEAK int sceAgcInit_named(void *state, uint32_t version) __asm__("sceAgcInit");
OBS_WEAK int sceAgcInit_nid_916d(void *state, uint32_t version) __asm__("$kW3GLb7QfPg");
OBS_WEAK const void *
sceAgcGetRegisterDefaults2_static(uint32_t version) __asm__("$2JtWUUiYBXs");
OBS_WEAK const void *
sceAgcGetRegisterDefaults2Internal_static(uint32_t version) __asm__("$wRbq6ZjNop4");
OBS_WEAK int sceKeyboardInit_static(void) __asm__("$wadT3QBCGY0");
OBS_WEAK int sceKeyboardOpen_static(int32_t user, int32_t mode, int32_t index,
                                    const void *opt) __asm__("$HJ+KnEHcaxI");
OBS_WEAK int sceKeyboardClose_static(int32_t handle) __asm__("$0LWei+c7RNc");
OBS_WEAK int sceKeyboardReadState_static(int32_t handle,
                                         void *state) __asm__("$6HpE68bzX6M");
OBS_WEAK int
sceKeyboardSetProcessPrivilege_static(int privilege) __asm__("$h8V5miA7UCs");
OBS_WEAK int sceKeyboardSetProcessPrivilege_alt(int privilege) __asm__("$K1A7IJp5xYc");
OBS_WEAK int sceKeyboardSetProcessFocus_static(int focus) __asm__("$4S5m1jZcpI4");
OBS_WEAK int sceKeyboardSetProcessFocus_alt(int focus) __asm__("$jqRcNtZmLuE");
OBS_WEAK int sceAudioOut2Initialize_static(void) __asm__("$g2tViFIohHE");
OBS_WEAK int sceAudioOut2UserCreate_static(int32_t user,
                                           void **handle_out) __asm__("$xywYcRB7nbQ");
OBS_WEAK int sceAudioOut2UserDestroy_static(void *handle) __asm__("$IaZXJ9M79uo");
OBS_WEAK int sceAudioOut2UserDestroy_alt(void *handle) __asm__("$6vY70ydXpiE");
OBS_WEAK int sceAudioOut2ContextResetParam_static(void *param) __asm__("$t5YrizufpQc");
OBS_WEAK int
sceAudioOut2ContextQueryMemory_static(void *param,
                                      uint64_t *size) __asm__("$pDmme7Bgm6E");
OBS_WEAK int sceAudioOut2ContextCreate_static(void *param, void *mem, uint64_t size,
                                              void **context) __asm__("$0x6o1VVAYSY");
OBS_WEAK int sceAudioOut2PortCreate_static(void *context, const void *param,
                                           void **handle) __asm__("$A89PZmqwrSQ");
OBS_WEAK int sceAudioOut2PortCreate_alt(void *context, const void *param,
                                        void **handle) __asm__("$JK2wamZPzwM");
OBS_WEAK int sceAudioOut2PortDestroy_static(void *handle) __asm__("$cd+Rtw+D1x8");
OBS_WEAK int sceAudioOut2PortDestroy_alt(void *handle) __asm__("$H9eDD7eR33E");
OBS_WEAK int sceAudioOut2PortGetState_static(void *handle,
                                             void *state) __asm__("$gatEUKG+Ea4");
OBS_WEAK int sceAudioOut2PortGetState_alt(void *handle,
                                          void *state) __asm__("$rhG+oVBEq4E");
OBS_WEAK int sceAudioOut2PortSetAttributes_static(void *handle, const void *attr,
                                                  uint32_t n) __asm__("$8XTArSPyWHk");
OBS_WEAK int sceAudioOut2PortSetAttributes_alt(void *handle, const void *attr,
                                               uint32_t n) __asm__("$eVjyI63AdPE");
OBS_WEAK int
sceAudioOut2ContextGetQueueLevel_static(void *context,
                                        uint32_t *level) __asm__("$R7d0F1g2qsU");
OBS_WEAK int
sceAudioOut2ContextGetQueueLevel_alt(void *context,
                                     uint32_t *level) __asm__("$xao2WBd0t0c");
OBS_WEAK int sceAudioOut2ContextPush_static(void *context, const void *buf,
                                            uint32_t n) __asm__("$aII9h5nli9U");
OBS_WEAK int sceAudioOut2ContextPush_alt(void *context, const void *buf,
                                         uint32_t n) __asm__("$1YvlmYc9gmg");
OBS_WEAK int sceAudioOut2ContextAdvance_static(void *context) __asm__("$PE2zHMqLSHs");
OBS_WEAK int sceAudioOut2ContextAdvance_alt(void *context) __asm__("$e0iLyhyzTTw");
OBS_WEAK int sceAudioOut2ContextDestroy_static(void *context) __asm__("$on6ZH7Abo10");
OBS_WEAK int sceAudioOut2ContextDestroy_alt(void *context) __asm__("$XaMbsB+ZfqI");
OBS_WEAK uint64_t sceAudioOut2GetSpeakerArrayMemorySize_static(
    uint32_t a, uint32_t b, uint32_t c) __asm__("$G1YOKDJYX2Y");
OBS_WEAK int sceAudioOut2SpeakerArrayCreate_static(void *a, void *b,
                                                   void *c) __asm__("$+k91hoTuoA8");
OBS_WEAK int sceAudioOut2SpeakerArrayDestroy_static(void *a) __asm__("$erCWQR5eKiQ");
OBS_WEAK int
sceAudioOut2GetSpeakerArrayCoefficients_static(void *a,
                                               void *b) __asm__("$4BlZurolOAo");
OBS_WEAK int sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients_static(
    void *a, void *b) __asm__("$28QqMnuuJ9Y");
OBS_WEAK int sceAudioOut2GetSpeakerInfo_static(void *a,
                                               uint32_t b) __asm__("$DImz2Ft9E2g");
OBS_WEAK int sceCommonDialogInitialize_static(void) __asm__("$uoUpLGNkygk");
OBS_WEAK int sceMsgDialogInitialize_static(void) __asm__("$lDqxaY1UbEo");
OBS_WEAK int sceMsgDialogOpen_static(const void *param) __asm__("$b06Hh0DPEaE");
OBS_WEAK int sceMsgDialogUpdateStatus_static(void) __asm__("$6fIC3XKt2k0");
OBS_WEAK int sceMsgDialogGetStatus_static(void) __asm__("$CWVW78Qc3fI");
OBS_WEAK int sceMsgDialogGetResult_static(void *result) __asm__("$Lr8ovHH9l6A");
OBS_WEAK int sceMsgDialogClose_static(void) __asm__("$HTrcDKlFKuM");
OBS_WEAK int sceMsgDialogTerminate_static(void) __asm__("$ePw-kqZmelo");
OBS_WEAK int
sceVideoOutUnknown_86721036bde0bf79_static(uint32_t handle, void *arg1, uint32_t arg2,
                                           uint32_t arg3, void *arg4,
                                           uint32_t arg5) __asm__("$eb-gvTYQcoY");
OBS_WEAK int sceJson2ValueC1_static(void *obj) __asm__("$9yDWMxEFdJU");
OBS_WEAK int sceJson2ValueD1_static(void *obj) __asm__("$WkkeywLJcgU");
OBS_WEAK int sceJson2ArrayC1_static(void *obj) __asm__("$-kU6bB4M-+k");
OBS_WEAK int sceJson2ArrayD1_static(void *obj) __asm__("$fLcU5G6Qrjs");
OBS_WEAK int sceJson2ObjectC1_static(void *obj) __asm__("$q0F6yS-rCms");
OBS_WEAK int sceJson2ObjectD1_static(void *obj) __asm__("$5OqszGpy7Mg");
OBS_WEAK int sceJson2StringC1_static(void *obj) __asm__("$oVkZ8W8-Q8A");
OBS_WEAK int sceJson2StringD1_static(void *obj) __asm__("$rHRr+131ATY");
OBS_WEAK int sceShareInitialize_static(uint64_t size, uint64_t prio,
                                       uint64_t cpumask) __asm__("$nBDD66kiFW8");
OBS_WEAK int sceShareInitialize_alt(uint64_t size, uint64_t prio,
                                    uint64_t cpumask) __asm__("$bxUiqevDEJw");
OBS_WEAK int sceShareTerminate_static(void) __asm__("$0IL1keINExQ");
OBS_WEAK int sceShareTerminate_alt(void) __asm__("$FBMN4pH1gtA");
OBS_WEAK int sceNpCheckCallback_static(void) __asm__("$1ofT4wV8md0");
OBS_WEAK int sceNpCheckCallback_alt(void) __asm__("$3Zl8BePTh9Y");
OBS_WEAK int sceSaveDataMount3_static(void *param,
                                      void *result) __asm__("$ZP4e7rlzOUk");
OBS_WEAK int sceSaveDataMount3_alt(void *param, void *result) __asm__("$STlzue4e-mQ");
OBS_WEAK int sceSaveDataUmount2_static(const void *mountPoint) __asm__("$uW4vfTwMQVo");
OBS_WEAK int sceSaveDataUmount2_alt(const void *mountPoint) __asm__("$WkEMPH0vbrk");
OBS_WEAK uint64_t sceAgcWriteDataPatchSetAddressOrOffset_static(
    void *packet, uint64_t address) __asm__("$fPSCdQxgpSw");
OBS_WEAK uint64_t sceAgcWriteDataPatchSetAddressOrOffset_alt(
    void *packet, uint64_t address) __asm__("$LKVgDHWC9Hw");
OBS_WEAK int sceSystemServiceParamGetInt_static(int32_t id,
                                                int32_t *out) __asm__("$fZo48un7LK4");
OBS_WEAK int sceSystemServiceParamGetInt_le(int32_t id,
                                            int32_t *out) __asm__("$riz76fI4mn0");
OBS_WEAK int sceNpEntitlementAccessInitialize_static(const void *param,
                                                     void *out) __asm__("$CnoyyjMD74w");
OBS_WEAK int sceNpEntitlementAccessInitialize_be(const void *param,
                                                 void *out) __asm__("$jO8DM8oyego");
OBS_WEAK int
sceNpGameIntentInitialize_static(const void *param) __asm__("$rR9-Gx-Bzps");
OBS_WEAK int sceNpGameIntentInitialize_be(const void *param) __asm__("$m87BHxt-H60");
OBS_WEAK int
sceNpSessionSignalingInitialize_static(const void *param, const void *arg1, uint64_t a2,
                                       uint64_t a3) __asm__("$CfDPn-iwyco");
OBS_WEAK int sceNpSessionSignalingInitialize_be(const void *param, const void *arg1,
                                                uint64_t a2,
                                                uint64_t a3) __asm__("$ysmw6J-P8Ak");
OBS_WEAK int
sceNpUniversalDataSystemInitialize_static(const void *param, const void *arg1,
                                          uint64_t a2,
                                          uint64_t a3) __asm__("$HngqGGyoNrI");
OBS_WEAK int sceNpUniversalDataSystemInitialize_be(const void *param, const void *arg1,
                                                   uint64_t a2,
                                                   uint64_t a3) __asm__("$sjaobBgqeB4");
OBS_WEAK int
sceNpTrophy2RegisterUnlockCallback_static(uint64_t ctx, uint64_t zero, void *cb,
                                          void *userdata) __asm__("$ipQa03zGRbE");
OBS_WEAK int
sceNpTrophy2RegisterUnlockCallback_be(uint64_t ctx, uint64_t zero, void *cb,
                                      void *userdata) __asm__("$sUXGfNMalIo");
OBS_WEAK int sceVideoRecordingInit_static(void) __asm__("$RXXBl6-iimo");
OBS_WEAK int sceVideoRecordingOpen_static(uint64_t arg0, uint64_t arg1,
                                          uint64_t arg2) __asm__("$7QoTYzVuzto");
OBS_WEAK int sceVideoRecordingClose_static(uint64_t arg0) __asm__("$KHvkPQJDMLk");
OBS_WEAK uint64_t sceVideoRecordingQueryMemSize_static(
    uint64_t arg0, uint64_t arg1) __asm__("$ZvWzS2wTIMc");
OBS_WEAK int sceVideoRecordingSetInfo_static(uint64_t arg0,
                                             const void *info) __asm__("$Fc8qxlKINYQ");
OBS_WEAK int sceAgcGetIsTrinityMode(uint8_t *out_is_trinity) __asm__("$BfBDZGbti7A");
#if defined(OBSCENE_HOST_BUILD)
OBS_WEAK
#endif
uint64_t sceAgc_nid_7d86501b8094ef57(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$V++UgBtQhn0");
OBS_WEAK int sceUserServiceGetGamePresets_static(int32_t user_id,
                                                 void *presets) __asm__("$-sD02mFDBh4");
OBS_WEAK int sceKernelAprResolveFilepathsToIdsAndFileSizes_static(
    const char **paths, uint32_t count, uint32_t *out_ids, uint64_t *out_sizes,
    int32_t *out_status, void *options) __asm__("$gEpBkcwxUjw");
OBS_WEAK int sceKernelAprSubmitCommandBufferAndGetResult_static(
    void *cb, uint32_t priority, uint64_t *out_result,
    uint32_t *out_wait_id) __asm__("$ASoW5WE-UPo");
OBS_WEAK int
sceKernelAprWaitCommandBuffer_static(uint32_t wait_id) __asm__("$rqwFKI4PAiM");
OBS_WEAK int sceKernelAprGetFileStat_static(uint32_t file_id,
                                            void *out_stat) __asm__("$ApkYaHb8Sek");
OBS_WEAK int sceKernelAprResolveFilepathsWithPrefixToIdsAndFileSizes_static(
    const char *prefix, const char **rel_paths, uint32_t count, uint32_t *out_ids,
    uint64_t *out_sizes, int32_t *out_status) __asm__("$w5fcCG+t31g");
OBS_WEAK void sceAmprCommandBufferConstructor_static(void *cb) __asm__("$8aI7R7WaOlc");
OBS_WEAK void sceAmprCommandBufferReset_static(void *cb) __asm__("$baQO9ez2gL4");
OBS_WEAK int sceAmprCommandBufferSetBuffer_static(void *cb, void *buf, uint32_t size,
                                                  uint64_t a3, uint64_t a4,
                                                  uint64_t a5) __asm__("$N-FSPA4S3nI");
OBS_WEAK int sceAmprCommandBufferSetBuffer(void *cb, void *buf, uint32_t size,
                                           uint64_t a3, uint64_t a4, uint64_t a5);
OBS_WEAK int
sceAmprAprCommandBufferConstructor_static(void *cb, void *ptr1, void *ptr2, void *buf,
                                          uint64_t zero,
                                          void *buf2) __asm__("$a8uLzYY--tM");
OBS_WEAK int
sceAmprAprCommandBufferReadFile_static(void *cb, uint32_t file_id, void *dest,
                                       uint64_t size,
                                       uint64_t offset) __asm__("$mQ16-QdKv7k");
OBS_WEAK int
sceAmprCommandBufferWriteAddress_04_00_static(void *cb, void *addr, uint32_t val,
                                              uint32_t a3) __asm__("$j0+3uJMxYJY");
OBS_WEAK int
sceAmprCommandBufferWriteAddress_04_00_url(void *cb, void *addr, uint32_t val,
                                           uint32_t a3) __asm__("$j0-3uJMxYJY");
OBS_WEAK int
sceAmprCommandBufferWriteAddress_04_00_le(void *cb, void *addr, uint32_t val,
                                          uint32_t a3) __asm__("$lmAxk7i3T48");
OBS_WEAK int sceAmprCommandBufferWriteAddress_04_00_named(
    void *cb, void *addr, uint32_t val,
    uint32_t a3) __asm__("sceAmprCommandBufferWriteAddress_04_00");
OBS_WEAK int
sceAmprMeasureCommandSizeWriteAddress_04_00_static(void *addr, uint32_t val,
                                                   uint32_t a3) __asm__("$4fgtGfXDrFc");
OBS_WEAK int
sceAmprMeasureCommandSizeWriteAddress_04_00_le(void *addr, uint32_t val,
                                               uint32_t a3) __asm__("$V6zD9Rkt+OE");
OBS_WEAK int sceAmprMeasureCommandSizeWriteAddress_04_00_named(
    void *addr, uint32_t val,
    uint32_t a3) __asm__("sceAmprMeasureCommandSizeWriteAddress_04_00");
OBS_WEAK int sceSaveDataInitialize3_static(void *param) __asm__("$TywrFKCoLGY");
OBS_WEAK int
sceSaveDataInitialize3_named(void *param) __asm__("sceSaveDataInitialize3");
OBS_WEAK int sceSaveDataTerminate_static(void) __asm__("$yKDy8S5yLA0");
OBS_WEAK int sceSaveDataTerminate_named(void) __asm__("sceSaveDataTerminate");
OBS_WEAK int
sceSaveDataSetupSaveDataMemory2_static(const void *param,
                                       void *result) __asm__("$oQySEUfgXRA");
OBS_WEAK int sceSaveDataSetupSaveDataMemory2_named(
    const void *param, void *result) __asm__("sceSaveDataSetupSaveDataMemory2");
OBS_WEAK int
sceSaveDataSetSaveDataMemory2_static(const void *param) __asm__("$cduy9v4YmT4");
OBS_WEAK int sceSaveDataSetSaveDataMemory2_named(const void *param) __asm__(
    "sceSaveDataSetSaveDataMemory2");
OBS_WEAK int
sceSaveDataGetSaveDataMemory2_static(const void *param) __asm__("$QwOO7vegnV8");
OBS_WEAK int sceSaveDataGetSaveDataMemory2_named(const void *param) __asm__(
    "sceSaveDataGetSaveDataMemory2");
OBS_WEAK int
sceSaveDataSyncSaveDataMemory_static(const void *param) __asm__("$wiT9jeC7xPw");
OBS_WEAK int sceSaveDataSyncSaveDataMemory_named(const void *param) __asm__(
    "sceSaveDataSyncSaveDataMemory");
OBS_WEAK void *reallocalign_static(void *ptr, size_t size,
                                   size_t align) __asm__("$BoTA41abbDg");
OBS_WEAK size_t
sceLibcMspaceMallocUsableSize_static(const void *ptr) __asm__("$3j5sEugWSnw");
OBS_WEAK size_t malloc_usable_size_static(const void *ptr) __asm__("$L0QZxn0SNzQ");
OBS_WEAK void *sceLibcMspaceCreate_static(const char *name, void *base, size_t size,
                                          uint64_t flags) __asm__("$-hn1tcVHq5Q");
OBS_WEAK void *sceLibcMspaceCreate(const char *name, void *base, size_t size,
                                   uint64_t flags);
OBS_WEAK int sceLibcMspaceDestroy_static(void *msp) __asm__("$W6SiVSiCDtI");
OBS_WEAK int sceLibcMspaceDestroy(void *msp);
OBS_WEAK int sceFontMemoryInit_static(void *obj, void *base, uint32_t size, void *memIf,
                                      void *msp, void *destroyCb,
                                      uint64_t zero) __asm__("$whrS4oksXc4");
OBS_WEAK int sceFontMemoryInit(void *obj, void *base, uint32_t size, void *memIf,
                               void *msp, void *destroyCb, uint64_t zero);
OBS_WEAK int sceFontMemoryTerm_static(void *obj) __asm__("$h6hIgxXEiEc");
OBS_WEAK int sceFontMemoryTerm(void *obj);
/* libSceJson2 (REQ-20261006T1505Z-js01, REQ-20261006T2220Z-js02) */
OBS_WEAK void sceJsonMemAllocatorCtor_static(void *obj) __asm__("$-hJRce8wn1U");
OBS_WEAK void
sceJsonMemAllocatorCtor_named(void *obj) __asm__("_ZN3sce4Json12MemAllocatorC2Ev");
OBS_WEAK void sceJsonMemAllocatorDtor_static(void *obj) __asm__("$OcAgPxcq5Vk");
OBS_WEAK void
sceJsonMemAllocatorDtor_named(void *obj) __asm__("_ZN3sce4Json12MemAllocatorD2Ev");
OBS_WEAK void sceJsonInitializerCtor_static(void *obj) __asm__("$cK6bYHf-Q5E");
OBS_WEAK void
sceJsonInitializerCtor_named(void *obj) __asm__("_ZN3sce4Json11InitializerC1Ev");
OBS_WEAK void sceJsonInitializerDtor_static(void *obj) __asm__("$RujUxbr3haM");
OBS_WEAK void
sceJsonInitializerDtor_named(void *obj) __asm__("_ZN3sce4Json11InitializerD1Ev");
OBS_WEAK int
sceJsonInitializerInitialize_static(void *obj,
                                    const void *param) __asm__("$Cxwy7wHq4J0");
OBS_WEAK int sceJsonInitializerInitialize_named(void *obj, const void *param) __asm__(
    "_ZN3sce4Json11Initializer10initializeEPKNS0_13InitParameterE");
OBS_WEAK int sceJsonInitializerTerminate_static(void *obj) __asm__("$PR5k1penBLM");
OBS_WEAK int sceJsonInitializerTerminate_named(void *obj) __asm__(
    "_ZN3sce4Json11Initializer9terminateEv");
OBS_WEAK void sceJsonInitParameter2Ctor_static(void *obj) __asm__("$WSOuge5IsCg");
OBS_WEAK void
sceJsonInitParameter2Ctor_named(void *obj) __asm__("_ZN3sce4Json14InitParameter2C1Ev");
OBS_WEAK void
sceJsonInitParameter2SetAllocator_static(void *param, void *alloc,
                                         void *user) __asm__("$I2QC8PYhJWY");
OBS_WEAK void
sceJsonInitParameter2SetAllocator_named(void *param, void *alloc, void *user) __asm__(
    "_ZN3sce4Json14InitParameter212setAllocatorEPNS0_12MemAllocatorEPv");

/* libSceFontFt (REQ-20261007T0850Z-fm02, REQ-20261013T1730Z-fr03) */
OBS_WEAK void *sceFontSelectLibraryFt_static(uint64_t mode) __asm__("$oM+XCzVG3oM");
OBS_WEAK void *sceFontSelectLibraryFt(uint64_t mode);
OBS_WEAK void *sceFontSelectRendererFt_static(uint64_t mode) __asm__("$Xx974EW-QFY");
OBS_WEAK void *sceFontSelectRendererFt_alt(uint64_t mode) __asm__("$Xx974EW+QFY");
OBS_WEAK void *sceFontSelectRendererFt(uint64_t mode);
OBS_WEAK uint64_t sceSigninDialogGetStatus_static(void) __asm__("$2m077aeC+PA");
OBS_WEAK uint64_t sceSigninDialogUpdateStatus_static(void) __asm__("$Bw31liTFT3A");

/* libSceFont (REQ-20261007T0850Z-fm02) */
OBS_WEAK int
sceFontCreateLibraryWithEdition_static(void *memory, void *selection, uint64_t edition,
                                       void **library) __asm__("$n590hj5Oe-k");
OBS_WEAK int sceFontCreateLibraryWithEdition(void *memory, void *selection,
                                             uint64_t edition, void **library);
OBS_WEAK int sceFontDestroyLibrary_static(void **library) __asm__("$FXP359ygujs");
OBS_WEAK int sceFontDestroyLibrary(void **library);

/* libSceNet (REQ-20261007T0915Z-ht01) */
OBS_WEAK int sceNetPoolCreate_static(const char *name, int size,
                                     int flags) __asm__("$dgJBaeJnGpo");
OBS_WEAK int sceNetPoolCreate(const char *name, int size, int flags);

/* libSceSsl (REQ-20261007T0915Z-ht01) */
OBS_WEAK int sceSslInit_static(size_t poolsize) __asm__("$hdpVEUDFW3s");
OBS_WEAK int sceSslInit(size_t poolsize);
OBS_WEAK int sceSslTerm_static(int ssl_ctx) __asm__("$0K1yQ6Lv-Yc");
OBS_WEAK int sceSslTerm(int ssl_ctx);

/* libSceHttp2 (REQ-20261007T0915Z-ht01) */
OBS_WEAK int sceHttp2Init_static(int pool, int ssl, size_t pool_size,
                                 int mode) __asm__("$3JCe3lCbQ8A");
OBS_WEAK int sceHttp2Init(int pool, int ssl, size_t pool_size, int mode);
OBS_WEAK int sceHttp2Term_static(int id) __asm__("$YiBUtz-pGkc");
OBS_WEAK int sceHttp2Term(int id);

OBS_WEAK uint64_t sceAgcCreateShader(void *out_slot, const void *header,
                                     const void *payload,
                                     uint64_t arg3) __asm__("$f3dg2CSgRKY");
OBS_WEAK int sceAgcGetFusedShaderSize(uint64_t *out_size, const void *half1,
                                      const void *half2) __asm__("$nQT5kYLv0cg");
OBS_WEAK int sceAgcFuseShaderHalves(void *fused_hdr, const void *half1,
                                    const void *half2,
                                    void *arg3) __asm__("$nApJjpKNBl4");
OBS_WEAK void *sceAgcDcbDrawIndexAuto(void *dcb, uint32_t count,
                                      uint32_t initiator) __asm__("$Yw0jKSqop+E");
OBS_WEAK void *
sceAgcDcbSetCxRegisterDirect(void *dcb, uint64_t reg_and_val) __asm__("$LHFXRrlTPD8");
OBS_WEAK void *
sceAgcDcbSetUcRegisterDirect(void *dcb, uint64_t reg_and_val) __asm__("$w4-d0n60hdo");
OBS_WEAK void *sceAgcCbSetShRegisterRangeDirect(void *dcb, uint32_t start_reg,
                                                const void *values,
                                                uint32_t count) __asm__("$n2fD4A+pb+g");
OBS_WEAK int sceAgcCreatePrimState(void *prim_state, void *sec_state,
                                   const void *shader_gs, const void *shader_vs,
                                   uint32_t prim_type) __asm__("$D9sr1xGUriE");
OBS_WEAK int
sceAgcCreateInterpolantMapping(void *mapping, const void *shader_vs,
                               const void *shader_ps) __asm__("$pdEV7bI6COI");
OBS_WEAK void *
sceAgcDcbSetCfRegisterDirect(void *dcb, uint64_t reg_and_val) __asm__("$73ZZdojLIgs");
OBS_WEAK void *
sceAgcDcbSetShRegisterDirect(void *dcb, uint64_t reg_and_val) __asm__("$pFLArOT53+w");
OBS_WEAK int
sceAgcUpdateInterpolantMapping(void *mapping, const void *shader_vs,
                               const void *shader_ps) __asm__("$SbuY2jN+axQ");
OBS_WEAK int sceAgcUpdatePrimState(void *prim_state, void *sec_state,
                                   uint32_t prim_type) __asm__("$Y3ymLfZ1384");
OBS_WEAK int sceAgcLinkShaders(void *link_state, void *sec_state, const void *shader_hs,
                               const void *shader_vs, const void *shader_ps,
                               uint32_t prim_type) __asm__("$MqAdbRMdNz4");
OBS_WEAK void *sceAgcDcbSetIndexBuffer(void *dcb,
                                       uint64_t gpu_addr) __asm__("$l4fM9K-Lyks");
OBS_WEAK void *sceAgcDcbSetIndexSize(void *dcb, uint32_t type,
                                     uint32_t flags) __asm__("$GIIW2J37e70");
OBS_WEAK void *sceAgcDcbSetIndexCount(void *dcb,
                                      uint32_t count) __asm__("$8N2tmT3jmC8");
OBS_WEAK void *sceAgcDcbDrawIndex(void *dcb, uint32_t count, uint64_t index_gpu_addr,
                                  uint32_t flags) __asm__("$q88lQ+GP5Yk");
OBS_WEAK void *sceAgcDcbEventWrite(void *dcb, uint32_t event_type,
                                   uint32_t event_index) __asm__("$aJf+j5yntiU");
OBS_WEAK void *sceAgcDcbSetNumInstances(void *dcb,
                                        uint32_t num_instances) __asm__("$tSBxhAPyytQ");

/* Outstanding AGC Command Builders and GetSize siblings (REQ-20260913T2355Z-a6aa) */
OBS_WEAK uint64_t sceAgcAcbDispatchIndirect(void *arg0, uint64_t arg1, uint64_t arg2,
                                            uint64_t arg3, uint64_t arg4,
                                            uint64_t arg5) __asm__("$j3EtxFkSIhQ");
OBS_WEAK uint64_t sceAgcAcbDispatchIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$PxKWV2fVAps");
OBS_WEAK uint64_t sceAgcAcbEventWrite(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$cFazmnXpJOE");
OBS_WEAK uint64_t sceAgcAcbEventWriteGetSize(uint64_t arg0, uint64_t arg1,
                                             uint64_t arg2, uint64_t arg3,
                                             uint64_t arg4,
                                             uint64_t arg5) __asm__("$Y-5vneiBtzk");
OBS_WEAK uint64_t sceAgcAcbPopMarker(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$6mFxkVqdmbQ");
OBS_WEAK uint64_t sceAgcAcbPushMarker(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$cpCILPya5Zk");
OBS_WEAK uint64_t sceAgcAcbResetQueue(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$JrtiDtKeS38");
OBS_WEAK uint64_t sceAgcAcbWaitRegMem(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$htn36gPnBk4");
OBS_WEAK uint64_t sceAgcAcbWriteData(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$eZ4+17OQz4Q");
OBS_WEAK uint64_t sceAgcCbDispatch(void *arg0, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, uint64_t arg4,
                                   uint64_t arg5) __asm__("$k3GhuSNmBLU");
OBS_WEAK uint64_t sceAgcCbDispatchGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                          uint64_t arg3, uint64_t arg4,
                                          uint64_t arg5) __asm__("$Abendgtz+3o");
OBS_WEAK uint64_t sceAgcCbSetShRegistersDirect(void *arg0, uint64_t arg1, uint64_t arg2,
                                               uint64_t arg3, uint64_t arg4,
                                               uint64_t arg5) __asm__("$UZbQjYAwwXM");
OBS_WEAK uint64_t sceAgcCbSetShRegistersDirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$yUBESvCCJ4I");
OBS_WEAK uint64_t sceAgcDcbCondExec(void *arg0, uint64_t arg1, uint64_t arg2,
                                    uint64_t arg3, uint64_t arg4,
                                    uint64_t arg5) __asm__("$BIPexNBSGog");
OBS_WEAK uint64_t sceAgcDcbCondExecGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                           uint64_t arg3, uint64_t arg4,
                                           uint64_t arg5) __asm__("$ou16V5hh5sg");
OBS_WEAK uint64_t sceAgcDcbDispatchIndirect(void *arg0, uint64_t arg1, uint64_t arg2,
                                            uint64_t arg3, uint64_t arg4,
                                            uint64_t arg5) __asm__("$CtB+A9-VxO0");
OBS_WEAK uint64_t sceAgcDcbDispatchIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$w8HVkEeXPv8");
OBS_WEAK uint64_t sceAgcDcbDrawIndexOffset(void *arg0, uint64_t arg1, uint64_t arg2,
                                           uint64_t arg3, uint64_t arg4,
                                           uint64_t arg5) __asm__("$B+aG9DUnTKA");
OBS_WEAK uint64_t sceAgcDcbDrawIndexOffsetGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$qMlfB1ZhMDc");
OBS_WEAK uint64_t sceAgcDcbDrawIndirect(void *arg0, uint64_t arg1, uint64_t arg2,
                                        uint64_t arg3, uint64_t arg4,
                                        uint64_t arg5) __asm__("$1q1titRBL6o");
OBS_WEAK uint64_t sceAgcDcbDrawIndirectGetSize(uint64_t arg0, uint64_t arg1,
                                               uint64_t arg2, uint64_t arg3,
                                               uint64_t arg4,
                                               uint64_t arg5) __asm__("$cxPZ4Wgvdj8");
OBS_WEAK uint64_t sceAgcDcbPopMarker(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$H7uZqCoNuWk");
OBS_WEAK uint64_t sceAgcDcbPushMarker(void *arg0, uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5) __asm__("$+kSrjIVxKFE");
OBS_WEAK uint64_t sceAgcDcbSetBaseIndirectArgs(void *arg0, uint64_t arg1, uint64_t arg2,
                                               uint64_t arg3, uint64_t arg4,
                                               uint64_t arg5) __asm__("$RmaJwLtc8rY");
OBS_WEAK uint64_t sceAgcDcbSetCfRegisterRangeDirect(
    void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$BVFg3CWU6Eo");
OBS_WEAK void *
sceAgcDcbSetCxRegistersIndirect(void *dcb, uint32_t count,
                                const void *reg_and_vals) __asm__("$ZvwO9euwYzc");
OBS_WEAK uint64_t sceAgcDcbSetCxRegistersIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$GBCh3zCihoU");
OBS_WEAK uint64_t sceAgcDcbSetFlip(void *arg0, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, uint64_t arg4,
                                   uint64_t arg5) __asm__("$YUeqkyT7mEQ");
OBS_WEAK uint64_t sceAgcDcbSetPredication(void *arg0, uint64_t arg1, uint64_t arg2,
                                          uint64_t arg3, uint64_t arg4,
                                          uint64_t arg5) __asm__("$bbFueFP+J4k");
OBS_WEAK uint64_t
sceAgcDcbSetShRegistersIndirect(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5) __asm__("$-HOOCn0JY48");
OBS_WEAK uint64_t sceAgcDcbSetShRegistersIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$nNlUtdDDvZ0");
OBS_WEAK uint64_t
sceAgcDcbSetUcRegistersIndirect(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                uint64_t arg4, uint64_t arg5) __asm__("$hvUfkUIQcOE");
OBS_WEAK uint64_t sceAgcDcbSetUcRegistersIndirectGetSize(
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$UQGTw4xRlcM");
OBS_WEAK uint64_t sceAgcDcbWaitUntilSafeForRendering(
    void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) __asm__("$MWiElSNE8j8");
OBS_WEAK uint64_t sceAgcDcbWriteData(void *arg0, uint64_t arg1, uint64_t arg2,
                                     uint64_t arg3, uint64_t arg4,
                                     uint64_t arg5) __asm__("$i1jyy49AjXU");
OBS_WEAK uint64_t sceAgcDcbWriteDataGetSize(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                            uint64_t arg3, uint64_t arg4,
                                            uint64_t arg5) __asm__("$p9tI+yTvx68");

OBS_WEAK int
sceAgcSetCxRegIndirectPatchAddRegisters(void *packet_ptr, uint32_t count,
                                        const void *regs) __asm__("$d-6uF9sZDIU");
OBS_WEAK int
sceAgcSetCxRegIndirectPatchSetAddress(void *packet_ptr,
                                      uint64_t addr) __asm__("$vcmNN+AAXnY");
OBS_WEAK int
sceAgcSetShRegIndirectPatchAddRegisters(void *packet_ptr, uint32_t count,
                                        const void *regs) __asm__("$z2duB-hHQSM");
OBS_WEAK int
sceAgcSetShRegIndirectPatchSetAddress(void *packet_ptr,
                                      uint64_t addr) __asm__("$Qrj4c+61z4A");
OBS_WEAK int
sceAgcSetUcRegIndirectPatchAddRegisters(void *packet_ptr, uint32_t count,
                                        const void *regs) __asm__("$vRoArM9zaIk");
OBS_WEAK int
sceAgcSetUcRegIndirectPatchSetAddress(void *packet_ptr,
                                      uint64_t addr) __asm__("$6lNcCp+fxi4");
OBS_WEAK int
sceAgcDmaDataPatchSetDstAddressOrOffset(void *packet_ptr,
                                        uint64_t dst_addr) __asm__("$IxYiarKlXxM");
OBS_WEAK int sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate(
    void *packet_ptr, uint64_t src_addr) __asm__("$cdDRpqcFGbU");
OBS_WEAK int sceAgcWaitRegMemPatchAddress(void *packet_ptr,
                                          uint64_t addr) __asm__("$3KDcnM3lrcU");
OBS_WEAK int
sceAgcQueueEndOfPipeActionPatchAddress(void *packet_ptr,
                                       uint64_t addr) __asm__("$0fWWK5uG9rQ");
OBS_WEAK int sceAgcGetDeviceInfo(void *info_buf) __asm__("$Tasc5SLczww");
OBS_WEAK int sceAgcSuspendPoint(void) __asm__("$h9z6+0hEydk");
OBS_WEAK int sceAgcSuspendPointAndCheckStatus(void *status) __asm__("$b+fis+WZ3Ig");
OBS_WEAK int sceAgcDriverGetDeviceInfo(void *info_buf) __asm__("$ZGMnhAlcv9Y");
OBS_WEAK int sceGnmGetGpuInfo(void *info_buf) __asm__("$GKIlegek0JQ");
OBS_WEAK int sceGnmGetGpuCoreClockFrequency(uint32_t *out_freq) __asm__("$Fwvh++m9IQI");

/* ---- libSceAgcDriver: current-generation GPU command submission ------------ */
OBS_WEAK int sceAgcDriverCreateQueue(uint32_t type, void **out_queue, uint64_t flags);
OBS_WEAK int sceAgcDriverDestroyQueue(void *queue);
OBS_WEAK int sceAgcDriverSubmitDcb(const void *dcb);
OBS_WEAK int sceAgcDriverSubmitCommandBuffer(void *queue, const void *dcb);
OBS_WEAK int sceAgcDriverSuspendPointSubmitDirect(void) __asm__("$ZV04pRl7cWU");
OBS_WEAK int sceAgcDriverSuspendPointSubmit(void *queue,
                                            uint32_t flags) __asm__("$QcmHLO2n7mk");
OBS_WEAK int
sceAgcDriverSysEnableSubmitDone45Exception(uint32_t enable) __asm__("$x3K61sY5m8Q");
OBS_WEAK int sceAgcDriverAddEqEvent(uint64_t queue, int arg1, int arg2);
OBS_WEAK int sceAgcDriverDeleteEqEvent(uint64_t queue, int arg1);
OBS_WEAK int sceAgcDriverGetEqEventType(const void *event);
OBS_WEAK int sceAgcDriverGetEqContextId(const void *event);
OBS_WEAK int sceAgcDriverQueryResourceRegistrationUserMemoryRequirements(
    uint64_t *out_size) __asm__("$AOLcoIkQDgM");
OBS_WEAK int
sceAgcDriverInitResourceRegistration(void *state,
                                     uint64_t size) __asm__("$F0Y42t-3e18");
OBS_WEAK int sceAgcDriverRegisterOwner(void *owner_desc) __asm__("$X-Nm5KLREeg");
OBS_WEAK int sceAgcDriverRegisterResource(void *arg0, void *arg1, const void *code,
                                          void *arg3,
                                          const char *name) __asm__("$W5z4eZrjEas");
OBS_WEAK int sceAgcDriverRegisterDefaultOwner(void *owner_desc) __asm__("$U9ueyEhSkF4");
OBS_WEAK int sceAgcDriverGetDefaultOwner(void *out_owner) __asm__("$F0ZXt5q0ZTA");
OBS_WEAK int sceAgcDriverSetTFRing(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                                   uint64_t arg3, void *h1,
                                   void *h2) __asm__("$XlNp7jzGiPo");

/* REQ-20261011T1430Z-sh02: Share content event callbacks */
OBS_WEAK int
sceShareRegisterContentEventCallback_static(const void *block) __asm__("$Sygnk9dr5WQ");
OBS_WEAK int
sceShareUnregisterContentEventCallback_static(void) __asm__("$KnsfHKmZqFA");

/* REQ-20261011T1530Z-ve01: VideoOut event id / event data / vblank events */
OBS_WEAK int sceVideoOutGetEventId_static(const void *event) __asm__("$U2JJtSqNKZI");
OBS_WEAK int sceVideoOutGetEventData_static(const void *event,
                                            void *data) __asm__("$rWUTcKdkUzQ");
OBS_WEAK int sceVideoOutAddVblankEvent_static(uint64_t eq, int handle,
                                              void *udata) __asm__("$Ek+VR4lcJQI");
OBS_WEAK int sceVideoOutAddVblankEvent_alt(uint64_t eq, int handle,
                                           void *udata) __asm__("$Xru92wHJRmg");
OBS_WEAK int sceVideoOutDeleteVblankEvent_static(uint64_t eq,
                                                 int handle) __asm__("$oNOQn3knW6s");
OBS_WEAK int sceVideoOutAddFlipEvent_static(uint64_t eq, int handle,
                                            void *udata) __asm__("$HXzjK9yI30k");
OBS_WEAK int sceVideoOutAddFlipEvent_alt(uint64_t eq, int handle,
                                         void *udata) __asm__("$Sd+I3CvjfB0");
OBS_WEAK int sceVideoOutDeleteFlipEvent_static(uint64_t eq,
                                               int handle) __asm__("$-Ozn0F1AFRg");
OBS_WEAK int sceVideoOutDeleteFlipEvent_alt(uint64_t eq,
                                            int handle) __asm__("$GBVAXdDn7Pw");

/* REQ-20261011T1640Z-us01: Unity User Service getters */
OBS_WEAK int
sceUserServiceInitialize_static(const void *params) __asm__("$j3YMu1MVNNo");
OBS_WEAK int
sceUserServiceGetInitialUser_static(int32_t *user_id_out) __asm__("$CdWp0oHWGr0");
OBS_WEAK int sceUserServiceGetUserName_static(int32_t user_id, char *name_out,
                                              size_t size) __asm__("$1xxcMiGu2fo");
OBS_WEAK int
sceUserServiceGetAgeLevel_static(int32_t user_id,
                                 int32_t *age_level_out) __asm__("$woNpu+45RLk");
OBS_WEAK int sceUserServiceGetAccessibilityChatTranscription_static(
    int32_t user_id, void *out) __asm__("$rnEhHqG-4xo");
OBS_WEAK int sceUserServiceGetAccessibilityPressAndHoldDelay_static(
    int32_t user_id, void *out) __asm__("$ZKJtxdgvzwg");
OBS_WEAK int
sceUserServiceGetAccessibilityTriggerEffect_static(int32_t user_id,
                                                   void *out) __asm__("$-3Y5GO+-i78");
OBS_WEAK int
sceUserServiceGetAccessibilityVibration_static(int32_t user_id,
                                               void *out) __asm__("$qWYHOFwqCxY");

/* REQ-20261011T1745Z-np03: Np / AppContent / Json startup calls */
OBS_WEAK int sceNpTrophy2CreateContext_static(uint64_t *ctx, int32_t user, uint64_t a2,
                                              uint64_t a3) __asm__("$Bagshr7OQ6Q");
OBS_WEAK int
sceNpUniversalDataSystemCreateContext_static(uint64_t *ctx, int32_t user, uint64_t a2,
                                             uint64_t a3) __asm__("$5zBnau1uIEo");
OBS_WEAK int sceNpGetAccountIdA_static(int32_t user, void *out) __asm__("$rbknaUjpqWo");
OBS_WEAK int sceAppContentInitialize_static(void *param) __asm__("$R9lA82OraNs");
OBS_WEAK int
sceAppContentDownloadDataGetAvailableSpaceKb_static(const char *mount,
                                                    void *kb) __asm__("$Gl6w5i0JokY");
OBS_WEAK void
sceJsonInitParameter2SetFileBufferSize_static(void *param,
                                              size_t size) __asm__("$Eu95jmqn5Rw");
OBS_WEAK void sceJsonInitParameter2SetFileBufferSize_named(
    void *param,
    size_t size) __asm__("_ZN3sce4Json14InitParameter217setFileBufferSizeEm");

/* REQ-20261012T0020Z-ce01: ContentExport init2 & term */
OBS_WEAK int sceContentExportInit2_static(void *param) __asm__("$0GnN4QCgIfs");
OBS_WEAK int sceContentExportTerm_static(void) __asm__("$+KDWny9Y-6k");

/* REQ-20261012T0445Z-av01: AvPlayer symbols */
OBS_WEAK int sceAvPlayerInitEx_static(const void *init,
                                      void *handle) __asm__("$o9eWRkSL+M4");
OBS_WEAK int sceAvPlayerPostInit_static(void *handle,
                                        const void *post) __asm__("$HD1YKVU26-M");
OBS_WEAK int sceAvPlayerIsActive_static(void *handle) __asm__("$UbQoYawOsfY");
OBS_WEAK int sceAvPlayerStreamCount_static(void *handle) __asm__("$hdTyRzCXQeQ");
OBS_WEAK int sceAvPlayerAddSourceEx_static(void *handle, int type,
                                           const char *src) __asm__("$x8uvuFOPZhU");
OBS_WEAK int sceAvPlayerClose_static(void *handle) __asm__("$NkJwDzKmIlw");
OBS_WEAK int sceAvPlayerSetLogCallback_static(void *fn,
                                              void *obj) __asm__("$eBTreZ84JFY");

/* REQ-20261012T0537Z-so01: Remoteplay */
OBS_WEAK int sceRemoteplayInitialize_static(void *heap,
                                            size_t size) __asm__("$k1SwgkMSOM8");
OBS_WEAK int
sceRemoteplayGetConnectionStatus_static(int32_t user,
                                        void *status) __asm__("$g3PNjYKWqnQ");
OBS_WEAK int sceRemoteplayTerminate_static(void) __asm__("$BOwybKVa3Do");

/* REQ-20261012T0635Z-ag01: AgcDriver NID 0xbc95ee5351ee0e67 and aux calls */
OBS_WEAK uint64_t
sceAgcDriverNidBc95ee5351ee0e67_static(const void *ev) __asm__("$Zw7uUVPulbw");
OBS_WEAK int
sceUserServiceGetAccessibilityZoomEnabled_static(int32_t user,
                                                 void *out) __asm__("$hD-H81EN9Vg");
OBS_WEAK int sceVideoOutIsOutputSupported_static(int port,
                                                 int bus) __asm__("$Nv8c-Kb+DUM");
OBS_WEAK int
sceSaveDataSetSaveDataMemory2_static(const void *param) __asm__("$cduy9v4YmT4");
OBS_WEAK int
sceSaveDataSyncSaveDataMemory_static(const void *param) __asm__("$wiT9jeC7xPw");

/* REQ-20261012T0710Z-gu01: GameUpdate */
OBS_WEAK uint64_t sceGameUpdateInitialize_static(uint64_t a1,
                                                 uint64_t a2) __asm__("$YJtKLttI9fM");
OBS_WEAK int sceGameUpdateTerminate_static(void) __asm__("$NSH-C-OmoNI");
OBS_WEAK int sceGameUpdateAbortRequest_static(void *req) __asm__("$d1CNGEOaK28");
OBS_WEAK int sceGameUpdateCheck_static(void *param) __asm__("$LYVV9z8+owM");
OBS_WEAK int sceGameUpdateCreateRequest_static(void *req) __asm__("$UvcvKaFvupA");
OBS_WEAK int sceGameUpdateDeleteRequest_static(void *req) __asm__("$bcCyjHN5sn0");

/* REQ-20261012T0728Z-pu01: Pad, Ime, UDS */
OBS_WEAK int scePadGetControllerInformation_static(int handle,
                                                   void *info) __asm__("$gjP9-KQzoUk");
OBS_WEAK int scePadResetOrientation_static(int handle) __asm__("$rIZnR6eSpvk");
OBS_WEAK int sceImeKeyboardGetResourceId_static(uint32_t id,
                                                void *out) __asm__("$dKadqZFgKKQ");
OBS_WEAK int
sceNpUniversalDataSystemCreateHandle_static(uint64_t *handle) __asm__("$hT0IAEvN+M0");

/* REQ-20261012T1232Z-ux01: pad extended info, UDS NID 0x0996125e66b8a5b3 */
OBS_WEAK int
scePadDeviceClassGetExtendedInformation_static(int handle,
                                               void *out) __asm__("$AcslpN1jHR8");
OBS_WEAK int
scePadDeviceClassGetExtendedInformation_be(int handle,
                                           void *out) __asm__("$Hx1j3aQlywE");
OBS_WEAK uint64_t sceNpUniversalDataSystem_0996125e66b8a5b3_static(
    void *a0, void *a1, void *a2, void *a3, void *a4, void *a5) __asm__("$s6W4Zl4Slgk");
OBS_WEAK uint64_t sceNpUniversalDataSystem_0996125e66b8a5b3_be(
    void *a0, void *a1, void *a2, void *a3, void *a4, void *a5) __asm__("$CZYSXma4pbM");

/* REQ-20261014T0430Z-ue01 & np05: UDS Event Lifecycle & NP Calls */
OBS_WEAK int
sceNpUniversalDataSystemCreateEvent_static(const char *name, void *prop, void *out,
                                           uint32_t flags) __asm__("$p+GcLqwpL9M");
OBS_WEAK int
sceNpUniversalDataSystemPostEvent_static(uint64_t ctx, uint64_t handle, void *event,
                                         uint32_t flags) __asm__("$CzkKf7ahIyU");
OBS_WEAK int
sceNpUniversalDataSystemDestroyEvent_static(void *event) __asm__("$wG+84pnNIuo");
OBS_WEAK int sceNpUniversalDataSystemDestroyEventPropertyObject_static(
    void *prop) __asm__("$kKUH0Viib3c");
OBS_WEAK int
sceNpUniversalDataSystemRegisterContext_static(uint64_t ctx, uint64_t handle,
                                               uint32_t flags) __asm__("$tpFJ8LIKvPw");
OBS_WEAK int
sceNpTrophy2UnregisterUnlockCallback_static(uint32_t ctx) __asm__("$wVqxM58sIKs");
OBS_WEAK int sceNpGetState_static(int32_t user, int32_t *state) __asm__("$eQH7nWPcAgc");
OBS_WEAK int sceNpSetNpTitleId_static(const void *id,
                                      const void *secret) __asm__("$Ec63y59l9tw");
OBS_WEAK int sceNpWebApi2Initialize_static(const void *param) __asm__("$+o9816YQhqQ");

OBS_WEAK int
sceAppContentDownloadDataGetAvailableSpaceKb_be(const char *mount,
                                                void *kb) __asm__("$RqIJLeawXho");

/* REQ-20261012T1251Z-pg01: PlayGo startup */
OBS_WEAK int scePlayGoInitialize_static(const void *param) __asm__("$sUaKk5WGzrY");
OBS_WEAK int scePlayGoInitialize_be(const void *param) __asm__("$ts6GlZOKRrE");
OBS_WEAK int scePlayGoOpen_static(int32_t *handle,
                                  uint32_t flags) __asm__("$YawcWmumUTM");
OBS_WEAK int scePlayGoOpen_be(int32_t *handle, uint32_t flags) __asm__("$M1Gma1ocrGE");
OBS_WEAK int scePlayGoSetInstallSpeed_static(int32_t handle,
                                             uint32_t speed) __asm__("$c91RT00cAOA");
OBS_WEAK int scePlayGoSetInstallSpeed_be(int32_t handle,
                                         uint32_t speed) __asm__("$4AAcTU9R3XM");
OBS_WEAK int scePlayGoGetChunkId_static(int32_t handle, void *ids, uint32_t max_ids,
                                        uint32_t *count) __asm__("$EPJUwdTFd+8");
OBS_WEAK int scePlayGoGetChunkId_be(int32_t handle, void *ids, uint32_t max_ids,
                                    uint32_t *count) __asm__("$73fF1MFU8hA");
OBS_WEAK int scePlayGoGetToDoList_static(int32_t handle, void *list, uint32_t count,
                                         uint32_t *entries) __asm__("$rebACSvzfjY");
OBS_WEAK int scePlayGoGetToDoList_be(int32_t handle, void *list, uint32_t count,
                                     uint32_t *entries) __asm__("$Nn7zKwnA5q0");
OBS_WEAK int scePlayGoClose_static(int32_t handle) __asm__("$Lw5lRyM1ylE");
OBS_WEAK int scePlayGoClose_be(int32_t handle) __asm__("$Uco1I0dlDi8");
OBS_WEAK int scePlayGoTerminate_static(void) __asm__("$8TNG4BG09zA");
OBS_WEAK int scePlayGoTerminate_be(void) __asm__("$MPe0EeBGM-E");

/* REQ-20261012T1827Z-1a71: hardware queries and sched priority */
OBS_WEAK uint64_t sceKernelGetCpuFrequency(void);
OBS_WEAK int sceKernelGetCpuTemperature(int *temp);
OBS_WEAK int sceKernelGetSocSensorTemperature(int sensor, int *temp);
OBS_WEAK int sceKernelGetCurrentFanDuty(int *duty);
OBS_WEAK int sceKernelGetHwModelName(char *name);
OBS_WEAK int sceKernelGetHwSerialNumber(char *serial);
OBS_WEAK int sched_get_priority_max(int policy);
OBS_WEAK int sched_get_priority_min(int policy);

/* REQ-20261012T1822Z-8b13: Zoom follow focus */
OBS_WEAK int
sceUserServiceGetAccessibilityZoomFollowFocus(int32_t user,
                                              void *out) __asm__("$O6IW1-Dwm-w");

/* REQ-20261012T1816Z-7af9: Net InetPton and ErrnoLoc */
OBS_WEAK int sceNetInetPton(int af, const char *src, void *dst);
OBS_WEAK int *sceNetErrnoLoc(void);

/* REQ-20261012T1816Z-ffa8: Http URI parse */
OBS_WEAK int sceHttpUriParse(void *out, const char *uri, void *pool, uint64_t *required,
                             size_t pool_size);

/* REQ-20261012T1822Z-0028: CesCs */
OBS_WEAK const void *sceCesRefersUcsProfileCp1252(void);
OBS_WEAK const void *sceCesRefersUcsProfileCp1252_static(void) __asm__("$LPzYZ+FR0BI");
OBS_WEAK int sceCesSbcToUtf8(const void *profile, uint32_t sbc, void *out, size_t max,
                             size_t *len);
OBS_WEAK int sceCesSbcToUtf8_static(const void *profile, uint32_t sbc, void *out,
                                    size_t max, size_t *len) __asm__("$xTd54EEL1Ao");
OBS_WEAK int sceCesUtf8ToSbc(const void *src, size_t max, size_t *used,
                             const void *profile, uint32_t *sbc);
OBS_WEAK int sceCesUtf8ToSbc_static(const void *src, size_t max, size_t *used,
                                    const void *profile,
                                    uint32_t *sbc) __asm__("$3Q1gOWWarcw");

/* REQ-20261012T1822Z-b15b: AudioOut2 mastering, system state, port register */
OBS_WEAK int sceAudioOut2MasteringInit_static(void) __asm__("$XHl38ZNknbs");
OBS_WEAK int
sceAudioOut2MasteringSetParam_static(const void *param) __asm__("$v8iOE+j8a5o");
OBS_WEAK int sceAudioOut2MasteringGetState_static(void *state) __asm__("$VZidxi2cYh0");
OBS_WEAK int sceAudioOut2MasteringTerm_static(void) __asm__("$2bbBBOkH4CY");
OBS_WEAK int sceAudioOut2GetSystemState_static(void *state) __asm__("$bkBN+CMLwRc");
OBS_WEAK int sceAudioOut2PortRegister_static(void *ctx,
                                             void *port) __asm__("$Bph+nEAV4WY");
OBS_WEAK int sceAudioOut2PortUnregister_static(void *ctx,
                                               void *port) __asm__("$16Ob5ACgOYk");

/* REQ-20261012T1822Z-3100: libSceRtc */
OBS_WEAK int sceRtcGetCurrentClockLocalTime_static(void *dt) __asm__("$ZPD1YOKI+Kw");
OBS_WEAK int sceRtcGetCurrentClock_static(void *dt, int tz) __asm__("$8lfvnRMqwEM");
OBS_WEAK int sceRtcGetCurrentClockUtc_static(void *dt) __asm__("$qZ8tGzZ-LnQ");
OBS_WEAK int sceRtcGetCurrentNetworkTick_static(void *tick) __asm__("$zO9UL3qIINQ");
OBS_WEAK uint32_t sceRtcGetTickResolution_static(void) __asm__("$jMNwqYr4R-k");
OBS_WEAK int sceRtcGetTick_static(const void *dt, void *tick) __asm__("$8w-H19ip48I");
OBS_WEAK int sceRtcSetTick_static(void *dt, const void *tick) __asm__("$ueega6v3GUw");
OBS_WEAK int sceRtcCheckValid_static(const void *dt) __asm__("$lPEBYdVX0XQ");
OBS_WEAK int sceRtcIsLeapYear_static(int year) __asm__("$Ug8pCwQvh0c");
OBS_WEAK int sceRtcGetDaysInMonth_static(int year, int month) __asm__("$3O7Ln8AqJ1o");
OBS_WEAK int sceRtcGetDayOfWeek_static(int year, int month,
                                       int day) __asm__("$CyIK-i4XdgQ");
OBS_WEAK int sceRtcGetTime_t_static(const void *dt, void *t) __asm__("$BtqmpTRXHgk");
OBS_WEAK int sceRtcSetTime_t_static(void *dt, int64_t t) __asm__("$bDEVVP4bTjQ");
OBS_WEAK int sceRtcGetWin32FileTime_static(const void *dt,
                                           void *ft) __asm__("$jfRO0uTjtzA");
OBS_WEAK int sceRtcCompareTick_static(const void *t1,
                                      const void *t2) __asm__("$fNaZ4DbzHAE");
OBS_WEAK int sceRtcConvertUtcToLocalTime_static(const void *u,
                                                void *l) __asm__("$M1TvFst-jrM");
OBS_WEAK int sceRtcConvertLocalTimeToUtc_static(const void *l,
                                                void *u) __asm__("$8Yr143yEnRo");
OBS_WEAK int sceRtcTickAddMonths_static(void *out, const void *in,
                                        int m) __asm__("$CL6y9q-XbuQ");
OBS_WEAK int sceRtcTickAddYears_static(void *out, const void *in,
                                       int y) __asm__("$-5y2uJ62qS8");
OBS_WEAK int sceRtcFormatRFC3339_static(char *b, const void *t,
                                        int tz) __asm__("$WJ3rqFwymew");
OBS_WEAK int sceRtcParseRFC3339_static(void *t, const char *s) __asm__("$99bMGglFW3I");

/* Wave 1: PngDec, PngEnc, SaveDataDialog, System Dialogs, Ult */
/* libScePngDec */
OBS_WEAK int scePngDecCreate_static(const void *param, void *memory, size_t size,
                                    uint32_t *handle) __asm__("$m0uW+8pFyaw");
OBS_WEAK int scePngDecDecode_static(uint32_t handle, const void *param,
                                    void *info) __asm__("$WC216DD3El4");
OBS_WEAK int scePngDecDelete_static(uint32_t handle) __asm__("$QbD+eENEwo8");
OBS_WEAK int scePngDecParseHeader_static(const void *param,
                                         void *info) __asm__("$U6h4e5JRPaQ");
OBS_WEAK int scePngDecQueryMemorySize_static(const void *param,
                                             size_t *size) __asm__("$-6srIGbLTIU");

/* libScePngEnc */
OBS_WEAK int scePngEncCreate_static(const void *param, void *memory, size_t size,
                                    uint32_t *handle) __asm__("$7aGTPfrqT9s");
OBS_WEAK int scePngEncDelete_static(uint32_t handle) __asm__("$RUrWdwTWZy8");
OBS_WEAK int scePngEncEncode_static(uint32_t handle, const void *param,
                                    void *output) __asm__("$xgDjJKpcyHo");
OBS_WEAK int scePngEncQueryMemorySize_static(const void *param,
                                             size_t *size) __asm__("$9030RnBDoh4");

/* libSceSaveDataDialog */
OBS_WEAK int sceSaveDataDialogClose_static(void) __asm__("$fH46Lag88XY");
OBS_WEAK int sceSaveDataDialogGetResult_static(void *result) __asm__("$yEiJ-qqr6Cg");
OBS_WEAK int sceSaveDataDialogGetStatus_static(void) __asm__("$ERKzksauAJA");
OBS_WEAK int sceSaveDataDialogInitialize_static(void *param) __asm__("$s9e3+YpRnzw");
OBS_WEAK int sceSaveDataDialogIsReadyToDisplay_static(void) __asm__("$en7gNVnh878");
OBS_WEAK int sceSaveDataDialogOpen_static(const void *param) __asm__("$4tPhsP6FpDI");
OBS_WEAK int
sceSaveDataDialogProgressBarInc_static(uint32_t val) __asm__("$V-uEeFKARJU");
OBS_WEAK int
sceSaveDataDialogProgressBarSetValue_static(uint32_t val,
                                            uint32_t total) __asm__("$hay1CfTmLyA");
OBS_WEAK int sceSaveDataDialogTerminate_static(void) __asm__("$YuH2FA7azqQ");
OBS_WEAK int sceSaveDataDialogUpdateStatus_static(void) __asm__("$KK3Bdg1RWK0");

/* libSceCommonDialog */
OBS_WEAK int sceCommonDialogInitialize_static(void) __asm__("$uoUpLGNkygk");
OBS_WEAK int sceCommonDialogIsUsed_static(void) __asm__("$BQ3tey0JmQM");

/* libSceImeDialog */
OBS_WEAK int sceImeDialogAbort_static(void) __asm__("$oBmw4xrmfKs");
OBS_WEAK int
sceImeDialogGetPanelSizeExtended_static(uint32_t type,
                                        void *size) __asm__("$CRD+jSErEJQ");
OBS_WEAK int sceImeDialogGetResult_static(void *result) __asm__("$x01jxu+vxlc");
OBS_WEAK int sceImeDialogGetStatus_static(void) __asm__("$IADmD4tScBY");
OBS_WEAK int sceImeDialogInit_static(const void *param,
                                     void *work) __asm__("$NUeBrN7hzf0");
OBS_WEAK int sceImeDialogTerm_static(void) __asm__("$gyTyVn+bXMw");

/* libSceMsgDialog */
OBS_WEAK int sceMsgDialogClose_static(void) __asm__("$HTrcDKlFKuM");
OBS_WEAK int sceMsgDialogGetResult_static(void *result) __asm__("$Lr8ovHH9l6A");
OBS_WEAK int sceMsgDialogGetStatus_static(void) __asm__("$CWVW78Qc3fI");
OBS_WEAK int sceMsgDialogInitialize_static(void) __asm__("$lDqxaY1UbEo");
OBS_WEAK int sceMsgDialogOpen_static(const void *param) __asm__("$b06Hh0DPEaE");
OBS_WEAK int sceMsgDialogProgressBarInc_static(uint32_t val) __asm__("$Gc5k1qcK4fs");
OBS_WEAK int
sceMsgDialogProgressBarSetValue_static(uint32_t val,
                                       uint32_t total) __asm__("$wTpfglkmv34");
OBS_WEAK int
sceMsgDialogProgressBarSetMsg_static(uint32_t val,
                                     const char *msg) __asm__("$6H-71OdrpXM");
OBS_WEAK int sceMsgDialogTerminate_static(void) __asm__("$ePw-kqZmelo");
OBS_WEAK int sceMsgDialogUpdateStatus_static(void) __asm__("$6fIC3XKt2k0");

/* libSceSystemService */
OBS_WEAK int sceSystemServiceGetStatus(void *status);
OBS_WEAK int
sceSystemServiceGetStatus_static(void *status) __asm__("sceSystemServiceGetStatus");

/* libSceAppContent additional */
OBS_WEAK int
sceAppContentTemporaryDataFormat_static(const char *mount) __asm__("$a5N7lAG0y2Q");
OBS_WEAK int
sceAppContentTemporaryDataGetAvailableSpaceKb_static(const char *mount,
                                                     void *kb) __asm__("$SaKib2Ug0yI");
OBS_WEAK int
sceAppContentTemporaryDataMount2_static(int type, void *mount) __asm__("$buYbeLOGWmA");
OBS_WEAK int sceAppContentAddcontMount_static(uint32_t serviceLabel, const char *label,
                                              void *mount) __asm__("$VANhIWcqYak");
OBS_WEAK int
sceAppContentAddcontUnmount_static(const char *mount) __asm__("$3rHWaV-1KC4");

/* libScePad additional */
OBS_WEAK int scePadResetLightBar_static(int handle) __asm__("$DscD1i9HX1w");
OBS_WEAK int scePadSetTriggerEffect_static(int handle,
                                           const void *param) __asm__("$2JgFB2n9oUM");

/* libSceErrorDialog */
OBS_WEAK int sceErrorDialogClose_static(void) __asm__("$ekXHb1kDBl0");
OBS_WEAK int sceErrorDialogGetStatus_static(void) __asm__("$t2FvHRXzgqk");
OBS_WEAK int sceErrorDialogInitialize_static(void) __asm__("$I88KChlynSs");
OBS_WEAK int sceErrorDialogOpen_static(const void *param) __asm__("$M2ZF-ClLhgY");
OBS_WEAK int sceErrorDialogOpenDetail_static(const void *param) __asm__("$jrpnVQfJYgQ");
OBS_WEAK int sceErrorDialogTerminate_static(void) __asm__("$9XAxK2PMwk8");
OBS_WEAK int sceErrorDialogUpdateStatus_static(void) __asm__("$WWiGuh9XfgQ");

/* libSceWebBrowserDialog */
OBS_WEAK int sceWebBrowserDialogClose_static(void) __asm__("$PSK+Eik919Q");
OBS_WEAK int sceWebBrowserDialogGetResult_static(void *result) __asm__("$vCaW0fgVQmc");
OBS_WEAK int sceWebBrowserDialogGetStatus_static(void) __asm__("$CFTG6a8TjOU");
OBS_WEAK int sceWebBrowserDialogInitialize_static(void) __asm__("$jqb7HntFQFc");
OBS_WEAK int sceWebBrowserDialogOpen_static(const void *param) __asm__("$FraP7debcdg");
OBS_WEAK int sceWebBrowserDialogTerminate_static(void) __asm__("$ocHtyBwHfys");
OBS_WEAK int sceWebBrowserDialogUpdateStatus_static(void) __asm__("$h1dR-t5ISgg");

/* libSceUlt */
OBS_WEAK int _sceUltUlthreadCreate_static(void *thread, const char *name,
                                          void (*entry)(uint64_t), uint64_t arg,
                                          void *stack, size_t stack_size, void *rt,
                                          const void *opt) __asm__("$znI3q8S7KQ4");
OBS_WEAK int
_sceUltUlthreadRuntimeCreate_static(void *rt, const char *name, uint32_t max_threads,
                                    uint32_t workers, void *work, const void *opt,
                                    size_t work_sz) __asm__("$jw9FkZBXo-g");
OBS_WEAK int _sceUltUlthreadRuntimeOptParamInitialize_static(
    void *opt, uint32_t version) __asm__("$V2u3WLrwh64");
OBS_WEAK size_t sceUltUlthreadRuntimeGetWorkAreaSize_static(
    uint32_t max_threads, uint32_t workers) __asm__("$grs2pbc2awM");
OBS_WEAK int sceUltUlthreadRuntimeDestroy_static(void *rt) __asm__("$-gxcs521SvA");
OBS_WEAK int _sceUltWaitingQueueResourcePoolCreate_static(
    void *pool, const char *name, uint32_t max_resources, uint32_t num_pools,
    void *work, const void *opt) __asm__("$YiHujOG9vXY");
OBS_WEAK size_t sceUltWaitingQueueResourcePoolGetWorkAreaSize_static(
    uint32_t max_resources, const void *opt) __asm__("$WIWV1Qd7PFU");
OBS_WEAK int
sceUltWaitingQueueResourcePoolDestroy_static(void *pool) __asm__("$or55417wcDk");
OBS_WEAK void sceUltUlthreadExit_static(int status) __asm__("$kF--s3xZjco");
OBS_WEAK int sceUltUlthreadJoin_static(void *thread,
                                       int *status) __asm__("$gCeAI57LGgI");
OBS_WEAK void sceUltUlthreadYield_static(void) __asm__("$HFd-lpjGxJA");
OBS_WEAK int _sceUltMutexCreate_static(void *mutex, const char *name, void *pool,
                                       const void *opt) __asm__("$mmt8Sa6tL6c");
OBS_WEAK int sceUltMutexLock_static(void *mutex) __asm__("$8hEGkR1pfr8");
OBS_WEAK int sceUltMutexTryLock_static(void *mutex) __asm__("$jOsUG0BJI-Y");
OBS_WEAK int sceUltMutexUnlock_static(void *mutex) __asm__("$h0XebKiMBtk");
OBS_WEAK int sceUltMutexDestroy_static(void *mutex) __asm__("$jW+HnafeS3Y");

OBS_WEAK int sceNetResolverCreate_static(const char *name, int pool,
                                         int flags) __asm__("$3L3PcQwghQs");
OBS_WEAK int sceNetResolverStartNtoa_static(int rid, const char *hostname, void *addr,
                                            int timeout, int retry,
                                            int flags) __asm__("$bBumpVl13zU");
OBS_WEAK int sceNetResolverDestroy_static(int rid) __asm__("$YgGMmx9YmZA");

OBS_WEAK int
sceNpGameIntentInitialize_static(const void *param) __asm__("$rR9-Gx-Bzps");
OBS_WEAK int sceNpGameIntentTerminate_static(void) __asm__("$-ZkjgMVYcNA");
OBS_WEAK int
sceNpGameIntentReceiveIntent_static(void *buf, size_t buf_size) __asm__("$T1z9ClAXQow");
OBS_WEAK int
sceNpGameIntentGetPropertyValueString_static(const void *intent, const char *prop,
                                             char *val,
                                             size_t *val_len) __asm__("$z-xc0yB0+aw");

OBS_WEAK int sceHttpUriParse_static(void *out, const char *uri, void *pool,
                                    uint64_t *required,
                                    uint64_t opt) __asm__("$W7jgfwKlZiE");
OBS_WEAK int sceKernelStopUnloadModule_static(int modid, size_t args, const void *argp,
                                              int flags, const void *pOpt,
                                              int *pRes) __asm__("$60F8zqh0p0A");

OBS_WEAK int sceAmprCommandBufferClearBuffer_static(void *obj) __asm__("$ULvXMDz56po");
OBS_WEAK uint32_t
sceAmprCommandBufferGetCurrentOffset_static(void *obj) __asm__("$GnxKOHEawhk");
OBS_WEAK uint32_t
sceAmprCommandBufferGetNumCommands_static(void *obj) __asm__("$gzndltBEzWc");
OBS_WEAK uint32_t sceAmprCommandBufferGetSize_static(void *obj) __asm__("$tZDDEo2tE5k");
OBS_WEAK int sceAmprCommandBufferReset_alt(void *obj) __asm__("$voD27PUOpG0");
OBS_WEAK int sceAmprCommandBufferWriteKernelEventQueue_04_00_static(
    void *obj, uint64_t a1, uint64_t a2, uint64_t a3) __asm__("$H896Pt-yB4I");

OBS_WEAK int sceSaveDataMount2_static(void *param) __asm__("$0kj-gTw5PtM");
OBS_WEAK int sceSaveDataGetMountInfo_static(const void *param,
                                            void *info) __asm__("$q8-aBtFHles");

OBS_WEAK int scePadResetLightBar_alt(int handle) __asm__("$XF9HL9YDxw4");
OBS_WEAK int scePadSetTriggerEffect_alt(int handle,
                                        const void *param) __asm__("$Q6H9aQcFmNg");
OBS_WEAK int sceNpUniversalDataSystemNid0996125e66b8a5b3_static(
    void *, void *, void *, void *, void *, void *) __asm__("$s6W4Zl4Slgk");

OBS_WEAK int sceFontOpenFontSet_static(void *lib, uint32_t set_type, uint32_t weight,
                                       const void *opt,
                                       void **font) __asm__("$cKYtVmeSTcw");
OBS_WEAK int sceFontOpenFontMemory_static(void *lib, const void *data, size_t size,
                                          void **font) __asm__("$KXUpebrFk1U");
OBS_WEAK int sceFontCloseFont_static(void *font) __asm__("$vzHs3C8lWJk");
OBS_WEAK int
sceFontCreateRendererWithEdition_static(void *lib, void *sel, uint64_t ed,
                                        void **rend) __asm__("$WaSFJoRWXaI");
OBS_WEAK int sceFontDestroyRenderer_static(void *rend) __asm__("$exAxkyVLt0s");
OBS_WEAK int sceFontBindRenderer_static(void *font, void *rend) __asm__("$iHLOKJFR59w");
OBS_WEAK int sceFontSetScalePixel_static(void *font, float sx,
                                         float sy) __asm__("$N1EBMeGhf7E");
OBS_WEAK int sceFontRenderSurfaceInit_static(void *surf, void *buf, uint32_t stride,
                                             uint32_t format, uint32_t w,
                                             uint32_t h) __asm__("$gdUCnU0gHdI");

/* REQ-20261013T2030Z-pt01: Pad tilt correction & trigger effect state */
OBS_WEAK int scePadSetTiltCorrectionState_static(int handle,
                                                 int state) __asm__("$vDLMoJLde8I");
OBS_WEAK int scePadGetTriggerEffectState_static(int handle, void *state,
                                                size_t size) __asm__("$znaWI0gpuo8");

/* REQ-20261013T1716Z-td01: Thread destructor runner hook */
OBS_WEAK int
_sceKernelSetThreadDtors_static(void (*runner)(void *)) __asm__("$rNhWz+lvOMU");

/* REQ-20261013T1735Z-js03: WebApi2 PushEvent, SaveData Transaction, Json2 */
OBS_WEAK int sceNpWebApi2PushEventCreateHandle_static(void *a0, void *a1, void *a2,
                                                      void *a3, void *a4,
                                                      void *a5) __asm__("$WV1GwM32NgY");
OBS_WEAK int
sceSaveDataCreateTransactionResource_static(void *a0, void *a1, void *a2, void *a3,
                                            void *a4, void *a5) __asm__("$gjRZNnw0JPE");

/* libSceJson2 Value, String, MemAllocator */
OBS_WEAK void *sceJson2ValueCtor_static(void *val) __asm__("$qBMjqyBn3OM");
OBS_WEAK void sceJson2ValueDtor_static(void *val) __asm__("$WTtYf+cNnXI");
OBS_WEAK void *sceJson2ValueSetType_static(void *val, int type) __asm__("$IKQimvG9Wqs");
OBS_WEAK void *sceJson2ValueSetBool_static(void *val, int b) __asm__("$5yHuiWXo2gg");
OBS_WEAK void *sceJson2ValueSetDouble_static(void *val,
                                             double d) __asm__("$BSmWDIkV4w4");
OBS_WEAK void *sceJson2ValueSetLong_static(void *val, long l) __asm__("$QxVVYhP-mvg");
OBS_WEAK void *sceJson2ValueSetUlong_static(void *val,
                                            unsigned long ul) __asm__("$SIe1ZmW7e7s");
OBS_WEAK void *sceJson2ValueSetString_static(void *val,
                                             const void *str) __asm__("$6l3Bv2gysNc");
OBS_WEAK void *sceJson2StringCtor_static(void *str,
                                         const char *s) __asm__("$9KUZFjI1IxA");
OBS_WEAK void sceJson2StringDtor_static(void *str) __asm__("$cG1VE2HMl6c");
OBS_WEAK void sceJson2MemAllocatorDtor2_static(void *alloc) __asm__("$OcAgPxcq5Vk");

/* REQ-20261015T0503Z-av01: AvPlayer stop */
OBS_WEAK int sceAvPlayerStop_static(void *handle) __asm__("$ZC17w3vB5Lo");

/* REQ-20261015T0210Z-pd01: scePadDeviceClassParseData */
OBS_WEAK int scePadDeviceClassParseData_static(int handle, const void *data,
                                               void *out) __asm__("$IHPqcbc0zCA");

/* REQ-20261015T0653Z-ud01: UDS SetString and DestroyHandle */
OBS_WEAK int sceNpUniversalDataSystemCreateEventPropertyObject_static(
    uint64_t *out) __asm__("$s6W4Zl4Slgk");
OBS_WEAK int sceNpUniversalDataSystemEventPropertyObjectSetString_static(
    uint64_t obj, const char *key, const char *val) __asm__("$MfDb+4Nln64");
OBS_WEAK int
sceNpUniversalDataSystemDestroyHandle_static(uint64_t handle) __asm__("$AUIHb7jUX3I");

/* REQ-20261015T0017Z-ai01: sceAjmInstanceCreate / Destroy / Batch */
OBS_WEAK int sceAjmInstanceCreate_static(uint32_t ctx, uint32_t codec, uint32_t flags,
                                         void *out) __asm__("$AxoDrINp4J8");
OBS_WEAK int sceAjmInstanceDestroy_static(uint32_t ctx,
                                          uint64_t id) __asm__("$RbLbuKv8zho");
OBS_WEAK int sceAjmBatchInitialize_static(void *buffer, size_t size,
                                          void *batch) __asm__("$MmpF1XsQiHw");
OBS_WEAK int sceAjmBatchJobDecode_static(void *batch, uint64_t instance, void *a2,
                                         void *a3, void *a4,
                                         void *a5) __asm__("$39WxhR-ePew");

/* REQ-20261014T1655Z-ap01: AudioPropagation and Acm */
OBS_WEAK int
sceAudioPropagationSystemQueryMemory_static(const void *p1,
                                            void *p2) __asm__("$7xyAxrusLko");
OBS_WEAK int
sceAudioPropagationSystemCreate_static(const void *p1, void *p2,
                                       uint64_t *out) __asm__("$aNEqtSHdUSo");
OBS_WEAK int
sceAudioPropagationSystemDestroy_static(uint64_t handle) __asm__("$x5VPqg5iyAk");
OBS_WEAK int sceAcmContextCreate_static(uint64_t *out) __asm__("$ZIXln2K3XMk");
OBS_WEAK int sceAcmContextDestroy_static(uint64_t handle) __asm__("$jBgBjAj02R8");

/* REQ-20261015T1234Z-gt01: ContentDelete, Net SockInfo, Voice */
OBS_WEAK int
sceContentDeleteInitialize_static(const void *param) __asm__("$zoxb0wEChEM");
OBS_WEAK int sceNetGetSockInfo_static(int s, void *info, int n,
                                      int flags) __asm__("$hLuXdjHnhiI");
OBS_WEAK int sceVoiceInit_static(const void *param,
                                 uint32_t version) __asm__("$9TrhuGzberQ");

/* REQ-20261015T1522Z-ai02: Ajm batch start and wait */
OBS_WEAK int sceAjmBatchStartBuffer_static(uint32_t ctx, void *buffer, size_t size,
                                           uint32_t *batch_id) __asm__("$fFFkk0xfGWs");
OBS_WEAK int sceAjmBatchStart_static(uint32_t ctx, void *batch,
                                     uint32_t *batch_id) __asm__("$5tOfnaClcqM");
OBS_WEAK int sceAjmBatchWait_static(uint32_t ctx, uint32_t batch_id, uint32_t timeout,
                                    void *error_res) __asm__("$-qLsfDAywIY");

OBS_WEAK int sceSystemServicePowerTick(void) __asm__("$ky8+cQvJtl0");

#endif /* OBSCENE_PLATFORM_H */
