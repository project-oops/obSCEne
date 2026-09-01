/*
 * The smallest module that still goes through the whole vendor path.
 *
 * # Why this exists
 *
 * The full probe imports four hundred symbols from fourteen libraries and faults
 * inside the loader with nothing in its log. That failure has too many possible causes
 * to reason about: a wrong tag, a wrong id, one bad symbol out of four hundred, a
 * relocation the loader applies differently, or something about scale.
 *
 * This has one import and one library. It is built by the same linker script, through
 * the same `mkmodule`, with the same tags - so anything structural is still present,
 * and everything about scale is gone.
 *
 * # Three outcomes, and they are distinguishable
 *
 * That is the whole point of writing it this way rather than as a spin loop:
 *
 * - **Output appears.** The loader resolved an import and the guest called it. Nothing
 *   structural is wrong, and the full probe fails for a reason to do with its size or
 *   with one particular symbol.
 * - **It hangs with no output.** Guest instructions are executing - there is no other
 *   way to reach the loop - and the import resolved, but the call produced nothing.
 *   That points at the emulator discarding the bytes rather than at this module.
 * - **It faults at address zero.** The import did not resolve. Deliberate: see below.
 * - **It faults anywhere else.** Something structural, and this is now a small enough
 *   module to reason about directly.
 *
 * A `ret` would collapse the last two: returning from an entry with nothing meaningful
 * on the stack faults, and that looks identical to never having run.
 *
 * # OBSCENE_MIN_NO_IMPORT
 *
 * Built with that define, this imports nothing at all - no symbol table entries for a
 * loader to resolve, no relocation against an undefined symbol. It still carries the
 * vendor segment, the process parameters and the same three loadable segments.
 *
 * That splits the remaining question in half. If the importless build runs and the
 * one-import build does not, the fault is in resolving imports. If both fail, nothing
 * to do with imports is responsible and the cause is in loading or in the entry
 * itself - which are a much smaller pair of things to be wrong about.
 */

#ifdef OBSCENE_MIN_DEBUG_OUT
/* A candidate output channel, tested here before being trusted anywhere else.
 *
 * The signature comes from public interface documentation. It is the least confident
 * declaration in this repository, which is exactly why it is being tried in a
 * throwaway module with one import rather than added to the probe: if the arity is
 * wrong the stack is corrupted and the crash says nothing useful, and here that costs
 * one build.
 *
 * What the run establishes is not whether it works but whether the emulator has an
 * implementation: an unimplemented function is logged as a stub by name, so its
 * absence from the stub list is the answer. */
__attribute__((weak)) int sceKernelDebugOutText(int channel, const char *text);
#endif

#ifndef OBSCENE_MIN_NO_IMPORT
/* Declared here rather than included from platform.h: this must not acquire the
 * probe's four hundred declarations by the back door, and the signature is the one
 * already confirmed for the full build. */
__attribute__((weak)) long sceKernelWrite(int fd, const void *buf, unsigned long len);

#ifdef OBSCENE_MIN_FILE
/* The file route, for a console.
 *
 * Descriptors 1 and 2 are an emulator convenience: the host process catches them. A
 * console has no parent listening, so the same run that proves everything works
 * produces no evidence that it did - which is indistinguishable from failing. Writing a
 * file the FTP server can hand back closes that gap.
 *
 * `/data` is the first entry in the full build's sink candidate list and was confirmed
 * writable on hardware before this was written, so the two agree about where a report
 * goes.
 *
 * Same rule as sceKernelWrite above: declared here, not included, and both signatures
 * are the ones already confirmed for the full build rather than new guesses. (D008) */
__attribute__((weak)) int sceKernelOpen(const char *path, int flags, int mode);
__attribute__((weak)) int sceKernelClose(int fd);
#define OBS_MIN_O_WRONLY 0x0001
#define OBS_MIN_O_CREAT 0x0200
#define OBS_MIN_O_TRUNC 0x0400
#endif
#endif

void obscene_start(void);

void obscene_start(void) {
#ifdef OBSCENE_MIN_NO_IMPORT
    /* Nothing to import, nothing to resolve, nothing to relocate against an undefined
     * symbol. Reaching the loop below is the entire result. */
#else
    static const char message[] = "obscene-min: the guest is running\n";

    /* The address test, and a deliberate crash when it fails.
     *
     * There is no output channel to report with - that is the whole problem being
     * diagnosed - so the only way this can say "the import did not resolve" is to do
     * something an emulator will report on its own. A fault at address zero is
     * unmistakable and cannot be confused with the spin below.
     *
     * Without it, an unresolved import and a resolved one that printed nothing both
     * look like a process that hangs, and those need completely different fixes. */
    if (&sceKernelWrite == 0) {
        /* Volatile, or the compiler deletes it: an indirection through a plain null
         * constant is undefined behaviour it is entitled to assume never happens, and
         * clang says so rather than emitting a trap. Through a volatile pointer it has
         * to make the call. */
        void (*volatile unreachable)(void) = 0;
        unreachable();
    }

#ifdef OBSCENE_MIN_DEBUG_OUT
    if (&sceKernelDebugOutText != 0) {
        (void)sceKernelDebugOutText(0, message);
    }
#endif
    /* Descriptor 1 is standard output. One emulator routes it to the host process's
     * own stdout, which is where a report from it will arrive.
     *
     * The return value is checked, not discarded. An import can resolve to a stub that
     * returns zero instead of to the real function, and that is indistinguishable from
     * a working call except by what comes back - the address is non-null either way, so
     * the test above cannot see it. A short write is the signal. */
    {
        /* Both standard streams. One emulator's stdout is not the handle a parent
         * process redirects - the write succeeds and the bytes are not reachable - so
         * the same line goes to stderr as well. Cheap, and it is the difference between
         * "the probe cannot report" and "the report went somewhere I did not look". */
#ifdef OBSCENE_MIN_FILE
        /* Attempted before the descriptor writes, not after: on a console this is the
         * only channel that leaves anything behind, so it must not sit behind a call
         * that might not return. Same ordering principle as the sink in the full build.
         */
        if (&sceKernelOpen != 0 && &sceKernelClose != 0) {
            const int fd = sceKernelOpen(
                "/data/obscene-report.txt",
                OBS_MIN_O_WRONLY | OBS_MIN_O_CREAT | OBS_MIN_O_TRUNC, 0666);
            if (fd >= 0) {
                (void)sceKernelWrite(fd, message, sizeof message - 1);
                (void)sceKernelClose(fd);
            }
        }
#endif
        (void)sceKernelWrite(2, message, sizeof message - 1);
        const long wrote = sceKernelWrite(1, message, sizeof message - 1);
        if (wrote != (long)(sizeof message - 1)) {
            /* Bound to something that is not the real write. Same trick as above: with
             * no output channel, a deliberate fault is the only way to say so. */
            void (*volatile bound_to_a_stub)(void) = 0;
            bound_to_a_stub();
        }
    }
#endif

#ifdef OBSCENE_MIN_FILE
    /* On a console the loop below is the wrong ending.
     *
     * It exists because on an emulator "it hangs here" is the *result*: reaching the
     * loop proves guest instructions executed, and returning would fault in a way that
     * reads as never having started. That reasoning depends on somebody watching the
     * emulator.
     *
     * A console has nobody watching, and the evidence is already on disk by this point
     * - so spinning buys nothing and costs a core pegged at 100% until someone kills
     * the process. Falling out of the entry faults instead, which under a homebrew
     * loader is an ordinary process death and leaves the file behind. A clean end beats
     * an observable one when the observation has already been made. */
    return;
#else
    /* Deliberate: an emulator that hangs here has executed guest instructions, and
     * there is no other way for it to reach this loop. Returning instead would fault
     * and read as never having started. */
    for (;;) {
    }
#endif
}
