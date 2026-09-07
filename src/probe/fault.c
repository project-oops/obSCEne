/*
 * The fault guard. See include/obscene/fault.h for the model and D325 for why it exists.
 *
 * Two builds share this. The host installs POSIX handlers directly against libc, which is
 * what lets the mechanism be proven on an ordinary machine before a console runs it. The
 * target resolves the same primitives by name through the loader - `sigsetjmp`,
 * `siglongjmp`, `_sigaction` and `scePthreadSelf`, all exported by libraries a title loads
 * - because it links no libc.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"

#if defined(OBSCENE_HOST_BUILD)
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#else
#include "oops/freestd.h"
#include "obscene/platform.h"
#endif

/* The fault signals. Same numbers on the FreeBSD-derived console and on the host. */
#define OBS_SIGILL 4
#define OBS_SIGFPE 8
#define OBS_SIGBUS 10
#define OBS_SIGSEGV 11

/* One landing pad per armed thread. Two is the most ever live at once (the suite's main
 * thread and one futex worker); the headroom is for a section that arms more. */
#define OBS_FAULT_SLOTS 16
static struct {
    unsigned long tid;
    obs_jmp_buf *buf;
} s_slots[OBS_FAULT_SLOTS];

static int s_available = 0;
static int s_inited = 0;
/* What init resolved and how the install went, stated in the report so a run that is not
 * guarded says why rather than looking the same as one that is. */
static char s_detail[80] = "not initialised";

#if !defined(OBSCENE_HOST_BUILD)
/* The platform primitives, resolved once at init. The two setjmp pointers are declared in
 * the header because the arm macro calls whichever resolved at the check's own call site.
 * `sigsetjmp` (two args, saves the signal mask) is preferred; `setjmp` (one arg) is the
 * fallback, and then the handler restores the mask itself with sigprocmask so a second
 * fault is still caught. */
int (*obs_sigsetjmp_fn)(void *, int) = 0;
int (*obs_setjmp_fn)(void *) = 0;
static void (*s_siglongjmp_fn)(void *, int) = 0;
static void (*s_longjmp_fn)(void *, int) = 0;
static int (*s_sigaction_fn)(int, const void *, void *) = 0;
static int (*s_sigprocmask_fn)(int, const void *, void *) = 0;
static unsigned long (*s_pthread_self_fn)(void) = 0;
static void (*s_pthread_exit_fn)(void *) = 0;
#endif

static unsigned long obs_fault_tid(void) {
#if defined(OBSCENE_HOST_BUILD)
    return (unsigned long)pthread_self();
#else
    if (s_pthread_self_fn != 0) {
        return s_pthread_self_fn();
    }
    return 1; /* single-thread fallback: everything shares one slot */
#endif
}

void obs_fault_register(obs_jmp_buf *buf) {
    unsigned long tid = obs_fault_tid();
    int empty = -1;
    for (int i = 0; i < OBS_FAULT_SLOTS; i++) {
        if (s_slots[i].tid == tid) {
            s_slots[i].buf = buf;
            return;
        }
        if (empty < 0 && s_slots[i].buf == 0) {
            empty = i;
        }
    }
    if (empty >= 0) {
        s_slots[empty].tid = tid;
        s_slots[empty].buf = buf;
    }
}

void obs_fault_unregister(void) {
    unsigned long tid = obs_fault_tid();
    for (int i = 0; i < OBS_FAULT_SLOTS; i++) {
        if (s_slots[i].tid == tid) {
            s_slots[i].buf = 0;
            return;
        }
    }
}

static obs_jmp_buf *obs_fault_current(void) {
    unsigned long tid = obs_fault_tid();
    for (int i = 0; i < OBS_FAULT_SLOTS; i++) {
        if (s_slots[i].tid == tid && s_slots[i].buf != 0) {
            return s_slots[i].buf;
        }
    }
    return 0;
}

/* A bounded copy of a status literal into s_detail, for the report to state. */
static void obs_fault_set_detail(const char *s) {
    unsigned int i = 0;
    while (s[i] != '\0' && i + 1u < sizeof s_detail) {
        s_detail[i] = s[i];
        i++;
    }
    s_detail[i] = '\0';
}

#if defined(OBSCENE_HOST_BUILD)

static void obs_fault_handler(int sig) {
    obs_jmp_buf *b = obs_fault_current();
    if (b != 0) {
        obs_fault_unregister();
        siglongjmp(*(sigjmp_buf *)(void *)b->jb, sig);
    }
    /* No pad on this thread: fall back to the default action so the process still dies
     * rather than spinning on a re-faulting instruction. */
    signal(sig, SIG_DFL);
}

void obs_fault_init(void) {
    if (s_inited) {
        return;
    }
    s_inited = 1;
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = obs_fault_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    int ok = 1;
    ok &= (sigaction(OBS_SIGSEGV, &act, 0) == 0);
    ok &= (sigaction(OBS_SIGBUS, &act, 0) == 0);
    ok &= (sigaction(OBS_SIGILL, &act, 0) == 0);
    ok &= (sigaction(OBS_SIGFPE, &act, 0) == 0);
    s_available = ok;
    obs_fault_set_detail(ok ? "installed (host libc)" : "host sigaction failed");
}

#else /* target */

static void obs_fault_handler(int sig) {
    obs_jmp_buf *b = obs_fault_current();
    if (b != 0) {
        obs_fault_unregister();
        /* Unblock this signal before leaving the handler, so a later fault is caught
         * again. `siglongjmp` restores the mask itself (savemask=1); the plain-longjmp
         * fallback does not, so this does it - unblocking a signal siglongjmp is about to
         * unblock too is harmless. sigset_t is four 32-bit words; the signal's bit is
         * (sig-1) in the first. */
        if (s_sigprocmask_fn != 0 && sig >= 1 && sig <= 32) {
            unsigned char set[16];
            for (int i = 0; i < 16; i++) {
                set[i] = 0;
            }
            ((unsigned int *)(void *)set)[0] = 1u << (unsigned int)(sig - 1);
            s_sigprocmask_fn(2 /* SIG_UNBLOCK */, set, 0);
        }
        if (s_siglongjmp_fn != 0) {
            s_siglongjmp_fn(b->jb, sig);
        } else if (s_longjmp_fn != 0) {
            s_longjmp_fn(b->jb, sig);
        }
    }
    /* No pad on this thread. On the console that is a platform-spawned service thread
     * (libScePad, video, audio) faulting outside any armed check - the suite's own threads
     * are always armed while they run risky code. Terminate just this thread rather than
     * restoring the default disposition: sigaction is process-global, so restoring it would
     * both end the whole process AND leave the guard disarmed for every thread after,
     * which is how a single service-thread fault was disarming the guard before a later
     * check crashed uncaught. The main thread keeps its guard and the suite runs on. Only
     * if the thread cannot be exited is the default restored, as a last resort against an
     * infinite re-fault. (D325) */
    if (s_pthread_exit_fn != 0) {
        s_pthread_exit_fn(0);
    }
    if (s_sigaction_fn != 0) {
        unsigned char dfl[32];
        for (int i = 0; i < 32; i++) {
            dfl[i] = 0;
        }
        s_sigaction_fn(sig, dfl, 0);
    }
}

/* Resolve one symbol from a handle, callable-checked. */
static const void *obs_fault_sym(int handle, const char *name) {
    const void *p = obs_module_symbol(handle, name);
    return obs_address_is_callable(p) ? p : 0;
}

/* Resolve a name from libkernel, then libSceLibcInternal. The signal entry points are the
 * former's, the standard-C ones (`setjmp`/`longjmp`) the latter's, and either might carry a
 * given spelling - so both are tried rather than assuming which library owns it. */
static const void *obs_fault_resolve(const char *name) {
    int h = obs_module_open("libkernel");
    const void *p = (h >= 0) ? obs_fault_sym(h, name) : 0;
    if (p == 0) {
        h = obs_module_open("libSceLibcInternal");
        if (h >= 0) {
            p = obs_fault_sym(h, name);
        }
    }
    return p;
}

/* A freestanding setjmp/longjmp, so the guard does not depend on libc exporting them.
 *
 * The pkg loader resolves setjmp/longjmp from libSceLibcInternal through sceKernelDlsym, so
 * the guard arms and catches there. A native eboot cannot: its libSceLibcInternal reads base
 * 0x0 and sceKernelDlsym returns ESRCH for that module's exports - the modules that DO
 * resolve are libkernel's, which is why the eboot's klog output (sceKernelDebugOutText) works
 * while its guard did not, so one late uncaught crash truncated the whole native run before
 * OBS|end. Rather than leave the guard off on the delivery shape a late crash most hurts,
 * save and restore the state a non-local jump needs directly. This is our own code
 * implementing the x86-64 SysV ABI, not a vendor declaration - the same footing as any
 * runtime.c helper (Principle 8). It is also our own defined symbol, not an import, so it
 * carries none of the GLOB_DAT/JUMP_SLOT split that leaves a native title's imports at 0x2
 * (D323): the call is a link-time PC-relative branch. The signal mask is not saved here; the
 * handler unblocks the fault signal with sigprocmask before jumping, exactly as it does for
 * the resolved plain-setjmp fallback. (D326)
 *
 * jb 8-byte slots: 0 rbx, 1 rbp, 2 r12, 3 r13, 4 r14, 5 r15, 6 rsp (as after ret), 7 rip. */
__asm__(".text\n"
        ".p2align 4\n"
        ".global obs_local_setjmp\n"
        ".hidden obs_local_setjmp\n"
        "obs_local_setjmp:\n"
        "\tmovq %rbx,    (%rdi)\n"
        "\tmovq %rbp,   8(%rdi)\n"
        "\tmovq %r12,  16(%rdi)\n"
        "\tmovq %r13,  24(%rdi)\n"
        "\tmovq %r14,  32(%rdi)\n"
        "\tmovq %r15,  40(%rdi)\n"
        "\tleaq 8(%rsp), %rax\n"
        "\tmovq %rax,  48(%rdi)\n"
        "\tmovq (%rsp), %rax\n"
        "\tmovq %rax,  56(%rdi)\n"
        "\txorl %eax, %eax\n"
        "\tret\n"
        ".p2align 4\n"
        ".global obs_local_longjmp\n"
        ".hidden obs_local_longjmp\n"
        "obs_local_longjmp:\n"
        "\tmovq    (%rdi), %rbx\n"
        "\tmovq   8(%rdi), %rbp\n"
        "\tmovq  16(%rdi), %r12\n"
        "\tmovq  24(%rdi), %r13\n"
        "\tmovq  32(%rdi), %r14\n"
        "\tmovq  40(%rdi), %r15\n"
        "\tmovq  48(%rdi), %rsp\n"
        "\tmovl %esi, %eax\n"
        "\ttestl %eax, %eax\n"
        "\tjnz 1f\n"
        "\tmovl $1, %eax\n"
        "1:\n"
        "\tjmp *56(%rdi)\n");
extern int obs_local_setjmp(void *env);
extern void obs_local_longjmp(void *env, int val);

/* Append "tag" then a single '0'/'1' to s_detail, bounded. Lets the guard record state which
 * primitives resolved, so a run that is guarded but still dies (or is not guarded at all)
 * shows the exact gap rather than only pass/fail - the difference between "no signal
 * primitive resolved" and "resolved but the platform did not deliver the signal". */
static void obs_fault_flag(const char *tag, int v) {
    unsigned int i = 0;
    while (i + 1u < sizeof s_detail && s_detail[i] != '\0') {
        i++;
    }
    unsigned int j = 0;
    while (tag[j] != '\0' && i + 1u < sizeof s_detail) {
        s_detail[i++] = tag[j++];
    }
    if (i + 1u < sizeof s_detail) {
        s_detail[i++] = v ? '1' : '0';
    }
    s_detail[i] = '\0';
}

/* Prefer a bound import over dlsym. A native title's dlsym resolves only the symbols the
 * process already imports, so a primitive the eboot links directly (imports.c) is found this
 * way when dlsym cannot see it - the gap that left the guard unarmed on the eboot while its
 * imported output worked. The callable check rejects a weak-unbound 0 and the 0x2 unresolved
 * sentinel, falling through to dlsym, which is what a module or emulator with a real dlsym
 * uses. (D326) */
static const void *obs_fault_pick(const void *import_addr, const char *name) {
    if (obs_address_is_callable(import_addr)) {
        return import_addr;
    }
    return obs_fault_resolve(name);
}

void obs_fault_init(void) {
    if (s_inited) {
        return;
    }
    s_inited = 1;

    const void *ssj = obs_fault_resolve("sigsetjmp");
    const void *slj = obs_fault_resolve("siglongjmp");
    const void *sj = obs_fault_resolve("setjmp");
    const void *lj = obs_fault_resolve("longjmp");
    /* The signal primitives and the thread calls are imports (imports.c), so their bound
     * address is tried before dlsym - the native title resolves them no other way. The POSIX
     * spellings stay as dlsym fallbacks for a loader that exports them under those names. */
    const void *sa = obs_fault_pick((const void *)&_sigaction, "_sigaction");
    if (sa == 0) {
        sa = obs_fault_resolve("sigaction");
    }
    if (sa == 0) {
        sa = obs_fault_resolve("posix_sigaction");
    }
    const void *spm = obs_fault_pick((const void *)&_sigprocmask, "_sigprocmask");
    if (spm == 0) {
        spm = obs_fault_resolve("sigprocmask");
    }
    if (spm == 0) {
        spm = obs_fault_resolve("posix_sigprocmask");
    }
    const void *self = obs_fault_pick((const void *)&scePthreadSelf, "scePthreadSelf");
    const void *pex = obs_fault_pick((const void *)&scePthreadExit, "scePthreadExit");

    /* clang-format off */
    obs_sigsetjmp_fn  = (int (*)(void *, int))(uintptr_t)ssj;
    s_siglongjmp_fn   = (void (*)(void *, int))(uintptr_t)slj;
    obs_setjmp_fn     = (int (*)(void *))(uintptr_t)sj;
    s_longjmp_fn      = (void (*)(void *, int))(uintptr_t)lj;
    s_sigaction_fn    = (int (*)(int, const void *, void *))(uintptr_t)sa;
    s_sigprocmask_fn  = (int (*)(int, const void *, void *))(uintptr_t)spm;
    s_pthread_self_fn = (unsigned long (*)(void))(uintptr_t)self;
    s_pthread_exit_fn = (void (*)(void *))(uintptr_t)pex;
    /* clang-format on */

    /* Whether the fault-recovery primitives resolved, captured before the pair-selection
     * clears one, so the report's bitmap reflects what the loader actually offered. */
    int b_sa = (sa != 0);
    int b_spm = (spm != 0);
    int b_pex = (pex != 0);
    int used_local = 0;

    /* The arm and the jump must use the same pair, or a sigsetjmp buffer meets a plain
     * longjmp. Prefer the sig variants (they save the mask); then the loader's plain
     * setjmp/longjmp; then our own freestanding pair, so a loader that resolves neither (a
     * native eboot, whose libSceLibcInternal dlsym does not work) still arms the guard rather
     * than running every check unguarded. Clear whichever pair is not used so the macro and
     * the handler cannot disagree. (D326) */
    if (obs_sigsetjmp_fn != 0 && s_siglongjmp_fn != 0) {
        obs_setjmp_fn = 0;
        s_longjmp_fn = 0;
    } else if (obs_setjmp_fn != 0 && s_longjmp_fn != 0) {
        obs_sigsetjmp_fn = 0;
        s_siglongjmp_fn = 0;
    } else {
        obs_sigsetjmp_fn = 0;
        s_siglongjmp_fn = 0;
        obs_setjmp_fn = &obs_local_setjmp;
        s_longjmp_fn = &obs_local_longjmp;
        used_local = 1;
    }
    if (s_sigaction_fn == 0) {
        obs_fault_set_detail("no sigaction export");
        obs_fault_flag(" sa=", b_sa);
        obs_fault_flag(" spm=", b_spm);
        obs_fault_flag(" pex=", b_pex);
        return;
    }

    /* struct sigaction (FreeBSD amd64): handler at 0, sa_flags int at 8, sigset_t[4] at 12.
     * Built as bytes so no vendor header is needed - the layout is the FreeBSD ABI, cited
     * like every other struct this program reads. */
    unsigned char act[32];
    for (int i = 0; i < 32; i++) {
        act[i] = 0;
    }
    *(void **)(void *)(act + 0) = (void *)(uintptr_t)&obs_fault_handler;

    int ok = 1;
    ok &= (s_sigaction_fn(OBS_SIGSEGV, act, 0) == 0);
    ok &= (s_sigaction_fn(OBS_SIGBUS, act, 0) == 0);
    ok &= (s_sigaction_fn(OBS_SIGILL, act, 0) == 0);
    ok &= (s_sigaction_fn(OBS_SIGFPE, act, 0) == 0);
    s_available = ok;
    if (!ok) {
        obs_fault_set_detail("sigaction call failed");
    } else if (obs_sigsetjmp_fn != 0) {
        obs_fault_set_detail("installed (sigsetjmp)");
    } else if (used_local) {
        obs_fault_set_detail("installed (local setjmp)");
    } else {
        obs_fault_set_detail("installed (setjmp)");
    }
    obs_fault_flag(" sa=", b_sa);
    obs_fault_flag(" spm=", b_spm);
    obs_fault_flag(" pex=", b_pex);
}

#endif

int obs_fault_available(void) {
    return s_available;
}

const char *obs_fault_detail(void) {
    return s_detail;
}
