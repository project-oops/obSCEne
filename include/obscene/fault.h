/*
 * The fault guard: a check that would end the process is caught, recorded as a crash, and
 * the suite continues.
 *
 * A pre-call guard (obs_address_is_callable) stops a jump to a bad address, but it cannot
 * stop a fault that happens *inside* a resolved function - the platform's futex faulted
 * inside libkernel with valid-looking arguments, taking the whole run down before a single
 * record after it. This is the freestanding equivalent of exception handling: install a
 * handler for the fault signals, arm a landing pad around each risky call, and on a fault
 * longjmp back to it. (D325)
 *
 * # How it is armed
 *
 * `OBS_FAULT_ARM` must be a macro, because `sigsetjmp` captures the frame it is called in;
 * wrapped in a function it would capture a frame that has already returned. So the arm
 * happens at the call site:
 *
 *     obs_jmp_buf buf;
 *     int sig = OBS_FAULT_ARM(&buf);   // 0 on the first pass, the signal number on a fault
 *     if (sig == 0) {
 *         result = risky();
 *         obs_fault_unregister();
 *     } else {
 *         obs_fault_unregister();
 *         result = obs_crash(sig);
 *     }
 *
 * The landing pad is keyed by thread, so a check that runs its risky call on a worker
 * (the futex does, to survive a hang) arms its own pad and the handler lands on whichever
 * thread faulted.
 *
 * # What it does not do
 *
 * It catches faults, not hangs: a call that never returns raises no signal, and is still
 * handled the way it always was - written as a `try` form, or run on a thread nobody joins.
 * (The scope was chosen deliberately; see D325.)
 */

#ifndef OBSCENE_FAULT_H
#define OBSCENE_FAULT_H

/* A landing pad. Sized well past a FreeBSD or a host sigjmp_buf (~200 bytes) so the same
 * byte buffer holds either without this header knowing which, and 16-aligned because the
 * saved register/FPU state a sigjmp_buf holds is accessed at that alignment. */
typedef struct obs_jmp_buf {
    _Alignas(16) unsigned char jb[512];
} obs_jmp_buf;

/* Install the fault handlers. Idempotent; a no-op where the primitives cannot be resolved
 * (the guard is then simply absent and a faulting check ends the run as before). Call once,
 * after module resolution is available and before the suite runs. */
void obs_fault_init(void);

/* Whether the handlers are installed. Reported once so a run says whether it was guarded. */
int obs_fault_available(void);

/* A short account of what init resolved and how the install went, for the report - so a run
 * that is not guarded says why rather than looking the same as one that is. */
const char *obs_fault_detail(void);

/* Record `buf` as the calling thread's landing pad. Paired with OBS_FAULT_ARM, which calls
 * this and then sigsetjmp; never call it alone. */
void obs_fault_register(obs_jmp_buf *buf);

/* Clear the calling thread's landing pad. Call on both the normal and the faulted path. */
void obs_fault_unregister(void);

#if defined(OBSCENE_HOST_BUILD)
#include <setjmp.h>
#define OBS_FAULT_ARM(pbuf)                                                                \
    (obs_fault_register(pbuf), sigsetjmp(*(sigjmp_buf *)(void *)(pbuf)->jb, 1))
#else
/* The platform's setjmp, resolved by name at init - `sigsetjmp` (two args, saves the mask)
 * where it exports, else plain `setjmp` (one arg) with the handler restoring the mask. Both
 * null until init / where neither resolves, in which case the arm is a plain 0 (no guard)
 * and the risky call runs unguarded, exactly as it did before this existed. Exactly one of
 * the two is non-null after a successful init, so the arm and the handler's jump agree. */
extern int (*obs_sigsetjmp_fn)(void *env, int savemask);
extern int (*obs_setjmp_fn)(void *env);
#define OBS_FAULT_ARM(pbuf)                                                                \
    (obs_fault_register(pbuf),                                                             \
     obs_sigsetjmp_fn != 0   ? obs_sigsetjmp_fn((pbuf)->jb, 1)                             \
     : obs_setjmp_fn != 0    ? obs_setjmp_fn((pbuf)->jb)                                   \
                             : 0)
#endif

#endif /* OBSCENE_FAULT_H */
