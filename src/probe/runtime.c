/*
 * The freestanding runtime.
 *
 * Two build shapes share this file. The target build has no libc at all and reaches
 * the platform's write through its own system library. The host build links libc so
 * the harness itself can be run and tested on an ordinary machine, with the platform
 * calls stubbed - which is what makes the framework verifiable before any emulator
 * can load it.
 */

#include "oops/freestd.h"
#include "oops/target.h"
#include "oops/krw.h"
#include "obscene/runtime.h"
#include "obscene/harness.h"
#include "obscene/sink.h"

#if defined(OBSCENE_HOST_BUILD)
#include <unistd.h>
#include <time.h>
#include "obscene/platform.h"
#else
#include "obscene/platform.h"

/* Declared here rather than in `platform.h`, which is where it belongs.
 *
 * The census in `corpus.h` declares this name as `const char` so the type system
 * forbids calling it, and `bulk.c` and `surface.c` include both that and `platform.h` -
 * so a function declaration in the shared header is a conflict in those two translation
 * units. Moving it properly means excluding it from the generated census, which is the
 * documented five-step process in `CLAUDE.md` and is worth doing.
 *
 * Until then this is the third local copy of one signature, after `start.c` and
 * `min.c`, and three copies of a judgement is the thing this project says not to do. It
 * is written down here rather than left to be noticed.
 *
 * The signature is the one `start.c` uses and `min.c` proved on hardware. (D225) */
OBS_WEAK int sceKernelDebugOutText(int channel, const char *text);
#endif

/* The census control. Defined here, in a different translation unit from the census,
 * and deliberately never referenced by anything else - its only job is to be a symbol
 * that genuinely resolves, so a census reporting it absent proves the census itself is
 * broken. See the control check in src/sections/surface.c. */
const char obs_census_control_present = 0;

/*
 * Which way out the report goes.
 *
 * `OBS_CHANNEL_UNTRIED` until the first write, then whichever channel got a byte
 * through. Chosen once rather than per call: a channel that works for the first line
 * works for the rest, and re-probing would put the failed attempts of two other
 * channels between every pair of records.
 */
typedef enum obs_channel {
    OBS_CHANNEL_UNTRIED,
    OBS_CHANNEL_KERNEL_WRITE,
    OBS_CHANNEL_PUTS,
    OBS_CHANNEL_POSIX_WRITE,
    OBS_CHANNEL_PUTCHAR,
    /* Every channel tried and none of them moved a byte. Recorded so the code stops
     * trying: without it every write re-probes three dead functions, which on a
     * hundred-odd records is a lot of calls into nothing. */
    OBS_CHANNEL_NONE
} obs_channel;

/* libkernel's runtime base, once a payload entry established it, or zero.
 *
 * Defined outside the payload-only block so a section can read it on any build: it is
 * zero on the host and on an eboot - neither is loaded by elfldr - which the reader
 * treats as "no base, skip". Only the setter (in the payload block below) is
 * payload-specific. */
static unsigned long obs_libkernel_base_value;
static void (*obs_write_tee)(void *ctx, const char *bytes, size_t len);
static void *obs_write_tee_ctx;

unsigned long obs_libkernel_base(void) {
    return obs_libkernel_base_value;
}

static obs_loader_weak_entry_t s_loader_weak_entries[OBS_LOADER_WEAK_COUNT] = {
    {"__sys_socketex", 0, 0}, {"bind", 0, 0},    {"_sendto", 0, 0},
    {"_setsockopt", 0, 0},    {"recv", 0, 0},    {"accept", 0, 0},
    {"listen", 0, 0},         {"connect", 0, 0}, {"close", 0, 0},
    {"__error", 0, 0},
};

const obs_loader_weak_entry_t *obs_get_loader_weak_entries(size_t *count) {
    if (count != NULL) {
        *count = OBS_LOADER_WEAK_COUNT;
    }
    return s_loader_weak_entries;
}

#if defined(OBSCENE_HOST_BUILD)
int obs_has_syscall_route(void) {
    return 0;
}

uintptr_t obs_syscall_gadget_address(void) {
    return 0;
}

long obs_invoke_syscall(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    (void)num; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return -1;
}

uint64_t obs_time_now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000);
    }
    return 0;
}
#endif

#if !defined(OBSCENE_HOST_BUILD)
typedef struct {
    int64_t d_tag;
    uint64_t d_val;
} obs_elf64_dyn;

extern const obs_elf64_dyn _DYNAMIC[];

/* One record to the system log, and nothing inferred from the result.
 *
 * Not one of the channels above, deliberately - see the call site in `obs_write`. The
 * call returns a status rather than a byte count, so it cannot answer "did this work";
 * asked to, it answers yes on a loader that stub-resolves it and the report is lost
 * behind it.
 *
 * NUL-terminated into a bounded buffer because this takes a string rather than a
 * length, and a record longer than the buffer is refused rather than truncated: half a
 * record still parses, which is worse than none. (D233) */
static long s_libkernel_syscall_gadget = 0;

int obs_has_syscall_route(void) {
    if (s_libkernel_syscall_gadget != 0) {
        return 1;
    }
    if (obs_get_payload_args() != NULL) {
        return 1;
    }
    return 0;
}

uintptr_t obs_syscall_gadget_address(void) {
    return (uintptr_t)s_libkernel_syscall_gadget;
}

long obs_invoke_syscall(long num, long a1, long a2, long a3, long a4, long a5,
                        long a6) {
    long ret = -1;
    register long r10_arg __asm__("r10") = a4;
    register long r8_arg __asm__("r8") = a5;
    register long r9_arg __asm__("r9") = a6;

    /* clang-format off */
    if (s_libkernel_syscall_gadget != 0) {
        __asm__ volatile("movq %7, %%rax\n"
                         "movq %8, %%r10\n"
                         "callq *%9\n"
                         : "=a"(ret)
                         : "D"(a1), "S"(a2), "d"(a3), "r"(r10_arg), "r"(r8_arg),
                           "r"(r9_arg), "r"(num), "r"(a4), "r"(s_libkernel_syscall_gadget)
                         : "rcx", "r11", "memory");
        return ret;
    }

    /* In a native title/eboot process, direct syscall instructions outside libkernel
     * trigger an unhandled kernel exception (SIGSYS / crash-candidate). Avoid them if
     * dynamically linked symbols are present or no syscall gadget is known. */
    if (obs_get_payload_args() == NULL && s_libkernel_syscall_gadget == 0) {
        return -1;
    }

    __asm__ volatile("movq %5, %%rax\n"
                     "movq %6, %%r10\n"
                     "syscall\n"
                     "jnc 1f\n"
                     "movq $-1, %0\n"
                     "jmp 2f\n"
                     "1:\n"
                     "movq %%rax, %0\n"
                     "2:\n"
                     : "=r"(ret)
                     : "D"(a1), "S"(a2), "d"(a3), "r"(r8_arg), "r"(num),
                       "r"(r10_arg), "r"(r9_arg)
                     : "rax", "rcx", "r11", "memory");
    /* clang-format on */
    return ret;
}

typedef void (*fn_debug_out_t)(int, const char *);
typedef sce_ssize_t (*fn_write_t)(int, const void *, size_t);
typedef int (*fn_open_t)(const char *, int, uint16_t);
typedef int (*fn_close_t)(int);
typedef sce_ssize_t (*fn_read_t)(int, void *, size_t);
typedef int (*fn_usleep_t)(unsigned int);
typedef int (*fn_dlsym_t)(int, const char *, void **);
typedef uint64_t (*fn_get_process_time_t)(void);

static int obs_payload_output_bootstrapped;
static fn_debug_out_t s_fn_debug_out;
static fn_write_t s_fn_write;
static fn_open_t s_fn_open;
static fn_close_t s_fn_close;
static fn_read_t s_fn_read;
static fn_usleep_t s_fn_usleep;
static fn_dlsym_t s_fn_dlsym;
static fn_get_process_time_t s_fn_get_process_time;

static void obs_debug_out_write(const char *bytes, size_t len) {
    static char scratch[512];
    if (len == 0 || len >= sizeof scratch) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        scratch[i] = bytes[i];
    }
    scratch[len] = '\0';

    if (s_libkernel_syscall_gadget != 0) {
        obs_invoke_syscall(601, 7, (long)scratch, 0, 0, 0, 0);
    } else if (s_fn_debug_out != NULL) {
        s_fn_debug_out(0, scratch);
    } else if (obs_address_is_callable((const void *)&sceKernelDebugOutText)) {
        (void)sceKernelDebugOutText(0, scratch);
    }
}

static obs_channel obs_output_channel = OBS_CHANNEL_UNTRIED;

static void *obs_payload_resolve(const char *name) {
    if (name == NULL)
        return NULL;
    char nid[12];
    obs_compute_nid(name, nid);

    /* 0. Try staged kexport_table first if available (bypasses retail game DRM / uninitialized dlsym) */
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *kaddr = obs_kexport_lookup(
            (const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr))
            return (void *)(uintptr_t)kaddr;
    }

    if (s_fn_dlsym == NULL || !obs_address_is_callable((const void *)s_fn_dlsym))
        return NULL;

    void *addr = NULL;
    if (s_fn_dlsym(0x2001, nid, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    if (s_fn_dlsym(0x2001, name, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    if (s_fn_dlsym(0x2, nid, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    if (s_fn_dlsym(0x2, name, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    if (s_fn_dlsym(0x1, nid, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    if (s_fn_dlsym(0x1, name, &addr) == 0 && obs_address_is_callable(addr))
        return addr;
    return NULL;
}

void obs_bootstrap_payload_output(unsigned long payload_args_word0) {
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *gptr = obs_kexport_lookup(
            (const obs_kexport_table_t *)pargs->kexport_table, "W0xkN0+ZkCE");
        if (gptr != NULL && obs_address_is_callable(gptr)) {
            s_libkernel_syscall_gadget = (long)(uintptr_t)gptr + 0xa;
        }
    }

    if (payload_args_word0 >= 0x10000UL && payload_args_word0 < 0x0000800000000000UL &&
        (payload_args_word0 & 0x7UL) == 0) {
        if (obs_address_is_callable((const void *)payload_args_word0)) {
            s_fn_dlsym = (fn_dlsym_t)payload_args_word0;
            if (s_libkernel_syscall_gadget == 0) {
                s_libkernel_syscall_gadget = (long)payload_args_word0 + 0xa;
            }
        }
    }

    if (s_libkernel_syscall_gadget != 0 && s_fn_dlsym == NULL &&
        (pargs == NULL || pargs->kexport_table == NULL)) {
        obs_payload_output_bootstrapped = 1;
        return;
    }

    s_fn_debug_out = (fn_debug_out_t)obs_payload_resolve("sceKernelDebugOutText");
    s_fn_write = (fn_write_t)obs_payload_resolve("sceKernelWrite");
    s_fn_open = (fn_open_t)obs_payload_resolve("sceKernelOpen");
    s_fn_close = (fn_close_t)obs_payload_resolve("sceKernelClose");
    s_fn_read = (fn_read_t)obs_payload_resolve("sceKernelRead");
    s_fn_usleep = (fn_usleep_t)obs_payload_resolve("sceKernelUsleep");
    s_fn_get_process_time = (fn_get_process_time_t)obs_payload_resolve("sceKernelGetProcessTime");
    void *getpid_ptr = obs_payload_resolve("getpid");
    if (getpid_ptr != NULL) {
        s_libkernel_syscall_gadget = (long)(uintptr_t)getpid_ptr + 0xa;
        obs_libkernel_base_value = (unsigned long)(uintptr_t)getpid_ptr - 0x5b0UL;
    }
    if (obs_libkernel_base_value == 0 && payload_args_word0 != 0) {
        if ((payload_args_word0 & 0xffffffff00000000UL) == 0x800000000UL) {
            obs_libkernel_base_value = 0x800000000UL;
        } else {
            obs_libkernel_base_value = payload_args_word0 & ~0xfffffUL;
        }
    }
    obs_write_tee = NULL;
    obs_write_tee_ctx = NULL;
    obs_payload_output_bootstrapped = 1;
}

/* Resolve the output functions by name for a native title.
 *
 * A payload bootstraps these from its dlsym gadget above; a title has no payload args,
 * so it resolves them the way every section resolves a platform call - by name through
 * the loader's own sceKernelDlsym (obs_module_symbol). Without it the title's sink
 * falls through to a raw import, and the sink's imports split across two relocations:
 * the guard reads `&fn` (a GLOB_DAT slot) while the call goes through a separate
 * JUMP_SLOT, so a loader that binds the data slot but leaves the linkage slot at its
 * unresolved sentinel (0x2) passes the guard and jumps to 0x2 on the call. The first
 * boot note - written before obs_bind_dynamic_symbols could touch the tables - is where
 * that lands. Resolving a plain data pointer here and calling through it (s_fn_write /
 * s_fn_debug_out) sidesteps the split entirely: it is null-checkable and never a raw
 * linkage slot.
 *
 * Idempotent and guarded: a no-op once the payload path has bootstrapped, it fills only
 * a pointer still null, rejects anything obs_address_is_callable refuses (the 0x2
 * sentinel among them), and does nothing at all where module resolution is unavailable
 * - an emulator that stubs dlsym - leaving the raw-import channels to carry that case
 * as before. */
void obs_bootstrap_title_output(void) {
    if (obs_payload_output_bootstrapped) {
        return;
    }
    int handle = obs_module_open("libkernel");
    if (handle < 0) {
        return;
    }
    if (s_fn_debug_out == NULL) {
        const void *p = obs_module_symbol(handle, "sceKernelDebugOutText");
        if (obs_address_is_callable(p)) {
            s_fn_debug_out = (fn_debug_out_t)(uintptr_t)p;
        }
    }
    if (s_fn_write == NULL) {
        const void *p = obs_module_symbol(handle, "sceKernelWrite");
        if (obs_address_is_callable(p)) {
            s_fn_write = (fn_write_t)(uintptr_t)p;
        }
    }
    if (s_fn_get_process_time == NULL) {
        const void *p = obs_module_symbol(handle, "sceKernelGetProcessTime");
        if (obs_address_is_callable(p)) {
            s_fn_get_process_time = (fn_get_process_time_t)(uintptr_t)p;
        }
    }
    if (s_libkernel_syscall_gadget == 0) {
        const void *getpid_ptr = obs_module_symbol(handle, "getpid");
        if (getpid_ptr != NULL && obs_address_is_callable(getpid_ptr)) {
            s_libkernel_syscall_gadget = (long)(uintptr_t)getpid_ptr + 0xa;
            obs_libkernel_base_value = (unsigned long)(uintptr_t)getpid_ptr - 0x5b0UL;
        }
    }
}

#if !defined(OBSCENE_HOST_BUILD)
uint64_t obs_time_now_us(void) {
    if (s_fn_get_process_time != NULL) {
        return s_fn_get_process_time();
    }
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        return sceKernelGetProcessTime();
    }
#if defined(__x86_64__)
    return __builtin_ia32_rdtsc();
#else
    return 0;
#endif
}
#endif

/* Whether the raw-import output channels (direct sceKernelWrite/write/puts/putchar) may
 * be attempted at all.
 *
 * A module (emulator) keeps them: a loader that stubs dlsym but binds direct imports
 * has no other way out, so they run whenever module resolution is unavailable. A native
 * eboot never does. Its imports split GLOB_DAT (what `&fn`, and so the guard, reads)
 * from JUMP_SLOT (what the call goes through), and the loader can bind the data slot
 * while leaving the linkage slot at its unresolved sentinel (0x2) - so a guard that
 * passed still faults on the call. That is the fault this fix exists for. The eboot's
 * output is the dlsym-resolved s_fn_* pointers alone (obs_bootstrap_title_output fills
 * them before the first write); where those cannot be resolved it emits nothing rather
 * than jumping to 0x2, which is the honest failure. (D323) */
#if defined(OBSCENE_TARGET_EBOOT)
#define OBS_RAW_IMPORT_CHANNELS_OK() 0
#else
#define OBS_RAW_IMPORT_CHANNELS_OK() (!obs_module_resolution_works())
#endif

/* Sends what it can through one channel. Returns bytes accepted, zero if the channel
 * is absent or refused. */
static size_t obs_send(obs_channel channel, const char *bytes, size_t len) {
    /* One record. Sized to match the report's own line buffer: nothing longer than
     * this is ever produced, and refusing anything longer is safer than truncating a
     * record into something that still parses. */
    static char scratch[512];

    switch (channel) {
    case OBS_CHANNEL_KERNEL_WRITE: {
        if (s_fn_write != NULL) {
            long n = (long)s_fn_write(OBS_FD_STDOUT, bytes, len);
            if (n > 0)
                return (size_t)n;
        }
        if (s_fn_debug_out != NULL) {
            if (len >= sizeof(scratch))
                len = sizeof(scratch) - 1;
            for (size_t i = 0; i < len; i++)
                scratch[i] = bytes[i];
            scratch[len] = '\0';
            s_fn_debug_out(0, scratch);
            return len;
        }
        /* The raw import is a last resort, and only where the loader binds it. Where
         * module resolution works - every console, any emulator with a real dlsym - the
         * resolved s_fn_write above carries this channel, and the raw call is skipped:
         * its JUMP_SLOT is what a native title leaves at 0x2, and `&sceKernelWrite` (a
         * GLOB_DAT read) does not see that. Confined to a dlsym-less emulator, where
         * the slot is genuinely bound. (D323) */
        /* The raw import is a last resort, and only where the loader binds it - a
         * dlsym-less emulator (OBS_RAW_IMPORT_CHANNELS_OK). On a console the resolved
         * s_fn_write above carries this channel and this is skipped: the raw call goes
         * through the JUMP_SLOT a native title leaves at 0x2, which `&sceKernelWrite`
         * (a GLOB_DAT read) cannot see. (D323) */
        if (s_libkernel_syscall_gadget != 0) {
            if (len >= sizeof(scratch))
                len = sizeof(scratch) - 1;
            for (size_t i = 0; i < len; i++)
                scratch[i] = bytes[i];
            scratch[len] = '\0';
            long n = obs_invoke_syscall(601, 7, (long)scratch, 0, 0, 0, 0);
            if (n >= 0)
                return len;
        }
        if (OBS_RAW_IMPORT_CHANNELS_OK() &&
            obs_address_is_callable((const void *)&sceKernelWrite)) {
            long n = (long)sceKernelWrite(OBS_FD_STDOUT, bytes, len);
            return n > 0 ? (size_t)n : 0;
        }
        return 0;
    }
    case OBS_CHANNEL_PUTS: {
        /* Confined to a dlsym-less emulator (OBS_RAW_IMPORT_CHANNELS_OK); `puts` splits
         * GLOB_DAT (the guard reads it via `&puts`) from JUMP_SLOT (the call goes
         * through it), so a title that leaves the linkage slot unbound would fault on
         * the call the guard just approved. `obs_address_is_callable`, not `!= 0`: a
         * loader that resolves an unrecognised import to a small non-null value passes
         * a null check and faults on the call - how a title died at rip 0x2 with the
         * report unwritten. */
        if (OBS_RAW_IMPORT_CHANNELS_OK() &&
            obs_address_is_callable((const void *)&puts)) {
            /* Only a whole record. `puts` supplies a newline, so handing it a partial
             * line would break the record in two - something that parses and is wrong,
             * which is worse than no output. Every caller writes one complete line, so
             * refusing anything else costs nothing and cannot be got wrong later. */
            if (len == 0 || len > sizeof(scratch) || bytes[len - 1] != '\n') {
                return 0;
            }
            for (size_t i = 0; i + 1 < len; i++) {
                scratch[i] = bytes[i];
            }
            scratch[len - 1] = '\0';
            /* Non-negative on success, EOF on failure. A stub returning zero counts as
             * success, which is why this is tried after the channels that report a
             * count. */
            if (puts(scratch) < 0) {
                return 0;
            }
            return len;
        }
        return 0;
    }
    case OBS_CHANNEL_POSIX_WRITE: {
        /* Confined to a dlsym-less emulator (OBS_RAW_IMPORT_CHANNELS_OK). On a console
         * the resolved s_fn_write carries the report and this raw `write` import stays
         * off; it faulted here as `write(1, ...)` through a `0x2` slot. Callable, not
         * merely non-null - see the note on the puts channel. */
        if (OBS_RAW_IMPORT_CHANNELS_OK() &&
            obs_address_is_callable((const void *)&write)) {
            long n = (long)write(OBS_FD_STDOUT, bytes, len);
            return n > 0 ? (size_t)n : 0;
        }
        return 0;
    }
    case OBS_CHANNEL_PUTCHAR: {
        /* Confined to a dlsym-less emulator (OBS_RAW_IMPORT_CHANNELS_OK). Returns the
         * character written; anything else is a failure, and checking for it is what
         * stops a stub that returns zero reading as success - how the first version
         * lost the whole report. */
        if (OBS_RAW_IMPORT_CHANNELS_OK() &&
            obs_address_is_callable((const void *)&putchar)) {
            int c = (int)(unsigned char)bytes[0];
            if (putchar(c) == c) {
                return 1;
            }
        }
        return 0;
    }
    case OBS_CHANNEL_UNTRIED:
    case OBS_CHANNEL_NONE:
    default:
        return 0;
    }
}

/* The name of the chosen channel, for the report to state. Which way the output got
 * out is itself a result: a run that had to fall back to one character at a time has
 * told you something about the platform before any check has run. */
const char *obs_output_channel_name(void) {
    switch (obs_output_channel) {
    case OBS_CHANNEL_KERNEL_WRITE:
        return "sceKernelWrite";
    case OBS_CHANNEL_PUTS:
        return "puts";
    case OBS_CHANNEL_POSIX_WRITE:
        return "write";
    case OBS_CHANNEL_PUTCHAR:
        return "putchar";
    case OBS_CHANNEL_NONE:
        return "none";
    case OBS_CHANNEL_UNTRIED:
    default:
        return "untried";
    }
}
#else
const char *obs_output_channel_name(void) {
    return "host";
}

void obs_bootstrap_payload_output(unsigned long payload_args_word0) {
    (void)payload_args_word0;
}

/* Nothing to do: the host has no system log, and its report goes to standard output
 * through libc. Defined rather than the call site being conditional, so `obs_write`
 * reads the same in both builds. */
static void obs_debug_out_write(const char *bytes, size_t len) {
    (void)bytes;
    (void)len;
}
#endif

/* An extra destination for every record, set while a command wants the report on its
 * own channel rather than the probe's.
 *
 * The `report` verb runs the suite, and its records belong to the driver that asked -
 * so during that command this tee points at the session socket, and the
 * section/try/res/sym records arrive between the `ack` and the `done`, exactly as
 * docs/PROTOCOL.md promises. Null the rest of the time, so an ordinary run pays nothing
 * for it.
 *
 * A single function pointer rather than a channel in the enum: the enum picks *one*
 * text channel, and this is deliberately additive - the report still goes to stdout and
 * the file sink while a copy goes down the socket. */

void obs_set_write_tee(void (*fn)(void *ctx, const char *bytes, size_t len),
                       void *ctx) {
    obs_write_tee = fn;
    obs_write_tee_ctx = ctx;
}

void obs_write(const char *bytes, size_t len) {
    /* The tee first, so a record reaches the driver that asked for it even if a text
     * channel below hangs or the process then dies - same durable-write-ahead-of-risky
     * ordering as the sink. */
    if (obs_address_is_callable((const void *)obs_write_tee)) {
        obs_write_tee(obs_write_tee_ctx, bytes, len);
    }

    /* The file next, and unconditionally.
     *
     * Not a fallback and not part of the channel selection below - those are
     * alternatives and exactly one of them is chosen. This is a second destination, and
     * it gets the bytes whether or not a text channel works, because the case it exists
     * for is the one where none of them does.
     *
     * Before the channel loop rather than after, so a text channel that hangs or ends
     * the process cannot cost the record on disk. The ordering is the same reasoning as
     * announce-before-attempting: put the durable write ahead of the risky one. */
    obs_sink_write(bytes, len);

    /* The system log next, and also unconditionally - a second destination, not a
     * candidate.
     *
     * # Why it cannot be one of the channels below
     *
     * The selection works by asking each candidate to move bytes and believing the
     * first that says it did. `sceKernelDebugOutText` returns a status, not a count, so
     * it cannot answer that question honestly - and a channel that always claims
     * success is selected on any loader that stub-resolves the symbol, after which
     * every record goes nowhere.
     *
     * That is not hypothetical either. Put first in the candidate list, it did exactly
     * that under Kyty, which patches unresolved imports to a stub that returns:
     *
     *     Unresolved import stub called [15]: symbol=9JYNqN6jAKI[libkernel_v1]
     *
     * `9JYNqN6jAKI` is this function, and the whole report was lost behind it. The
     * comment on the candidate order below already warned about exactly this shape - a
     * channel that reports success and prints nothing - and the warning was
     * reintroduced above it. (D233)
     *
     * As a second destination the question never arises: nothing is inferred from the
     * call, so nothing can be inferred wrongly. It costs a duplicate on a loader that
     * implements both, which is what the file sink above already costs and for the same
     * reason - the case it exists for is the one where the channels below produce
     * nothing. */
    obs_debug_out_write(bytes, len);

    size_t sent = 0;
    while (sent < len) {
#if defined(OBSCENE_HOST_BUILD)
        long n = (long)write(1, bytes + sent, len - sent);
        if (n <= 0) {
            return;
        }
        sent += (size_t)n;
#else
        if (obs_output_channel == OBS_CHANNEL_NONE) {
            return;
        }
        if (obs_output_channel == OBS_CHANNEL_UNTRIED) {
            /* In order, cheapest and most faithful first. Each is tried with the real
             * bytes rather than a test message: a probe that announces itself before
             * it can be read would put noise at the head of every report, and a
             * channel that works has already done useful work. */
            //
            // `puts` comes before `write` deliberately. One emulator implements
            // `write` by returning the byte count and discarding the bytes - a channel
            // that reports success and prints nothing, which is the one failure this
            // selection cannot detect. It implements `puts` properly, so trying that
            // first gets a report out of it.
            static const obs_channel candidates[] = {
                OBS_CHANNEL_KERNEL_WRITE, OBS_CHANNEL_PUTS, OBS_CHANNEL_POSIX_WRITE,
                OBS_CHANNEL_PUTCHAR};
            size_t moved = 0;
            for (unsigned int i = 0; i < sizeof(candidates) / sizeof(candidates[0]);
                 i++) {
                moved = obs_send(candidates[i], bytes + sent, len - sent);
                if (moved > 0) {
                    obs_output_channel = candidates[i];
                    break;
                }
            }
            if (moved == 0) {
                obs_output_channel = OBS_CHANNEL_NONE;
                return;
            }
            sent += moved;
            continue;
        }
        size_t moved = obs_send(obs_output_channel, bytes + sent, len - sent);
        /* A channel that worked and has stopped means the stream is gone. Spinning
         * would hang the report rather than end it, and a hung run tells nobody
         * anything. */
        if (moved == 0) {
            return;
        }
        sent += moved;
#endif
    }
}

void obs_puts(const char *s) {
    obs_write(s, obs_strlen(s));
}

/* ---- link-map walk and run-context ----------------------------------------
 *
 * Enumerate the runtime linker's loaded objects by walking its own link-map, and from
 * that - plus the build and the payload anchor - name the execution context a run
 * measures in. Both read only memory the loader already wrote, with no syscall, so they
 * work where the platform refuses sceKernelGetModuleInfo (measured: the compatibility
 * host does).
 *
 * The layout is standard and cited, nothing here is invented or vendor-derived: the ELF
 * dynamic array (d_tag then d_un, eight bytes each) and DT_DEBUG = 21 are the ELF ABI;
 * r_debug (r_version, then r_map at offset 8) is FreeBSD <sys/link_elf.h>; link_map
 * (l_addr at 0, l_name at 8, l_next at 0x18) is FreeBSD <link.h>. The console is
 * FreeBSD-derived - the same citation the directory walk uses for dirent.
 * src/sections/modlink.c reports the full inventory through this same walk. */

/* Whether an address can be dereferenced, verified via direct kernel virtual query
 * probe. */
int obs_linkmap_readable(uintptr_t p) {
    if (p < 0x10000u || p >= 0x0000800000000000UL) {
        return 0;
    }
#if defined(OBSCENE_HOST_BUILD)
    return 0;
#else
    /* Never attempt to read from eboot text segment (xotext), which is execute-only on
     * PS5 */
    if (p >= 0x400000UL && p < 0x500000UL) {
        return 0;
    }
    char info[96];
    for (size_t i = 0; i < sizeof(info); i++) {
        info[i] = 0;
    }
    int ret = sceKernelVirtualQuery((const void *)p, 0, info, sizeof(info));
    if (ret == 0) {
        int prot = *(const int *)(info + 0x20);
        if ((prot & 1) != 0) {
            return 1;
        }
        return 0;
    }
    return 0;
#endif
}

/* Locate the runtime dynamic section carrying DT_DEBUG. Checks payload _DYNAMIC, then
 * eboot and libkernel. */
static const unsigned char *obs_linkmap_own_dynamic(const char **reason) {
#if !defined(OBSCENE_HOST_BUILD)
    /* 1. Check payload's own _DYNAMIC */
    if (obs_linkmap_readable((uintptr_t)_DYNAMIC)) {
        for (unsigned int i = 0; i < 4096; i++) {
            if (_DYNAMIC[i].d_tag == 0)
                break;
            if (_DYNAMIC[i].d_tag == 21 && _DYNAMIC[i].d_val != 0) {
                return (const unsigned char *)_DYNAMIC;
            }
        }
    }

#if !defined(OBSCENE_TARGET_MODULE)
    /* 2. Check main eboot text / data segments by querying virtual memory (payload
     * only) */
    uintptr_t addr = 0x400000UL;
    for (int step = 0; step < 32 && addr < 0x80000000UL;) {
        char vq_buf[96];
        for (size_t k = 0; k < sizeof(vq_buf); k++)
            vq_buf[k] = 0;
        int ret = sceKernelVirtualQuery((const void *)addr, 0, vq_buf, sizeof(vq_buf));
        if (ret != 0) {
            addr += 0x4000u;
            step++;
            continue;
        }
        uintptr_t seg_start = *(const uintptr_t *)(vq_buf + 0);
        size_t seg_size = *(const size_t *)(vq_buf + 8);
        if (seg_size == 0)
            seg_size = 0x4000u;

        if (obs_linkmap_readable(seg_start)) {
            const unsigned char *elf = (const unsigned char *)seg_start;
            if (elf[0] == 0x7f && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F') {
                uint64_t e_phoff = *(const uint64_t *)(elf + 0x20);
                uint16_t e_phentsize = *(const uint16_t *)(elf + 0x36);
                uint16_t e_phnum = *(const uint16_t *)(elf + 0x38);
                if (e_phoff != 0 && e_phentsize >= 0x38 && e_phnum > 0 &&
                    e_phnum <= 64) {
                    for (uint16_t i = 0; i < e_phnum; i++) {
                        const unsigned char *ph =
                            elf + e_phoff + ((size_t)i * e_phentsize);
                        uint32_t p_type = *(const uint32_t *)(ph + 0x00);
                        uint64_t p_vaddr = *(const uint64_t *)(ph + 0x10);
                        uintptr_t dyn_addr = (p_vaddr >= seg_start)
                                                 ? (uintptr_t)p_vaddr
                                                 : (seg_start + (uintptr_t)p_vaddr);
                        if (p_type == 2 &&
                            obs_linkmap_readable(dyn_addr)) { /* PT_DYNAMIC */
                            return (const unsigned char *)dyn_addr;
                        }
                    }
                }
            }
        }
        addr = seg_start + seg_size;
        step++;
    }
#endif /* !defined(OBSCENE_TARGET_MODULE) */

    /* 3. Check libkernel data segment */
    unsigned long lk_base = obs_libkernel_base();
    if (lk_base != 0) {
        uintptr_t lk_addr = lk_base;
        for (int step = 0; step < 16; step++) {
            char vq_buf[96];
            for (size_t k = 0; k < sizeof(vq_buf); k++)
                vq_buf[k] = 0;
            int ret =
                sceKernelVirtualQuery((const void *)lk_addr, 0, vq_buf, sizeof(vq_buf));
            if (ret != 0)
                break;
            uintptr_t seg_start = *(const uintptr_t *)(vq_buf + 0);
            size_t seg_size = *(const size_t *)(vq_buf + 8);
            if (seg_size == 0)
                seg_size = 0x4000u;
            if (obs_linkmap_readable(seg_start)) {
                for (size_t off = 0; off + 16 <= seg_size && off < 0x20000; off += 16) {
                    uint64_t tag = *(const uint64_t *)(seg_start + off);
                    uint64_t val = *(const uint64_t *)(seg_start + off + 8);
                    if (tag == 21 && val != 0 && obs_linkmap_readable((uintptr_t)val)) {
                        return (const unsigned char *)(seg_start + off);
                    }
                }
            }
            lk_addr = seg_start + seg_size;
        }
    }
#endif
    *reason = "dynamic section with DT_DEBUG unavailable";
    return (const unsigned char *)0;
}

unsigned int obs_linkmap_walk(int (*cb)(const char *name, unsigned long base,
                                        void *user),
                              void *user, const char **reason) {
    const char *local = "ok";
    if (reason == (const char **)0) {
        reason = &local;
    }
    *reason = "ok";

    const unsigned char *dyn = obs_linkmap_own_dynamic(reason);
    unsigned int count = 0;
    if (dyn != (const unsigned char *)0) {
        uintptr_t r_debug = 0;
        for (unsigned int i = 0; i < 4096; i++) {
            if (!obs_linkmap_readable((uintptr_t)(dyn + (size_t)i * 16u))) {
                break;
            }
            uint64_t tag = *(const uint64_t *)(dyn + (size_t)i * 16u);
            uint64_t val = *(const uint64_t *)(dyn + (size_t)i * 16u + 8u);
            if (tag == 0) { /* DT_NULL */
                break;
            }
            if (tag == 21) { /* DT_DEBUG */
                r_debug = (uintptr_t)val;
                break;
            }
        }
        if (r_debug != 0 && obs_linkmap_readable(r_debug) &&
            obs_linkmap_readable(r_debug + 8u)) {
            uintptr_t node = *(const uintptr_t *)(r_debug + 8u); /* r_map */
            for (unsigned int i = 0; i < 512u && node != 0; i++) {
                if ((node & 0x7u) != 0 || !obs_linkmap_readable(node) ||
                    !obs_linkmap_readable(node + 0x18u)) {
                    *reason = "link-map chain left mapped memory";
                    break;
                }
                unsigned long l_addr =
                    (unsigned long)*(const uintptr_t *)(node + 0x00u);
                uintptr_t name_ptr = *(const uintptr_t *)(node + 0x08u);
                const char *l_name = "";
                if (name_ptr != 0 && (name_ptr & 0x7u) == 0 &&
                    obs_linkmap_readable(name_ptr)) {
                    l_name = (const char *)name_ptr;
                }
                count++;
                if (cb && cb(l_name, l_addr, user)) {
                    break;
                }
                node = *(const uintptr_t *)(node + 0x18u); /* l_next */
            }
        }
    }

    /* Query platform system module list if available */
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList) &&
        obs_address_is_callable((const void *)&sceKernelGetModuleInfo)) {
        int handles[64];
        size_t written = 0;
        int rc = sceKernelGetModuleList(handles, 64, &written);
        if (rc == 0 && written > 0) {
            for (size_t i = 0; i < written; i++) {
                char info_buf[512];
                for (size_t k = 0; k < sizeof(info_buf); k++)
                    info_buf[k] = 0;
                *(size_t *)info_buf = 0x160;
                if (sceKernelGetModuleInfo(handles[i], info_buf) == 0) {
                    const char *mod_name = (const char *)(info_buf + 8);
                    unsigned long mod_base = *(const unsigned long *)(info_buf + 0x28);
                    count++;
                    if (cb && cb(mod_name, mod_base, user)) {
                        break;
                    }
                }
            }
        }
    }

    if (count == 0 && dyn == (const unsigned char *)0) {
        const payload_args_t *pargs = (const payload_args_t *)obs_get_payload_args();
        if (pargs != NULL && pargs->kexport_table != NULL) {
            *reason = "ok";
            if (cb) {
                const obs_kexport_table_t *kt =
                    (const obs_kexport_table_t *)pargs->kexport_table;
                char nid_buf[12];
                obs_compute_nid("sceAgcCreateShader", nid_buf);
                if (obs_kexport_lookup(kt, nid_buf) != NULL) {
                    cb("libSceAgc", 0, user);
                    count++;
                }
                obs_compute_nid("sceGnmSubmitCommandBuffers", nid_buf);
                if (obs_kexport_lookup(kt, nid_buf) != NULL) {
                    cb("libSceGnmDriver", 0, user);
                    count++;
                }
                if (count == 0) {
                    cb("payload", 0, user);
                    count++;
                }
            }
            return count;
        }
        return 0;
    }
    return count;
}

/* A bounded append of src into dst at pos, NUL-terminated; returns the new length. */
static size_t obs_ctx_append(char *dst, size_t pos, size_t cap, const char *src) {
    if (src == NULL) {
        return pos;
    }
    while (*src != '\0' && pos + 1 < cap) {
        dst[pos++] = *src++;
    }
    dst[pos] = '\0';
    return pos;
}

/* Freestanding substring search for the context classifier. */
static int obs_ctx_contains(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) {
        return 0;
    }
    for (size_t i = 0; hay[i] != '\0'; i++) {
        size_t j = 0;
        while (needle[j] != '\0' && hay[i + j] == needle[j]) {
            j++;
        }
        if (needle[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

struct obs_ctx_gpu {
    unsigned int walked;
    int agc;
    int gnm;
};

static int obs_ctx_gpu_cb(const char *name, unsigned long base, void *user) {
    struct obs_ctx_gpu *g = (struct obs_ctx_gpu *)user;
    (void)base;
    g->walked++;
    if (obs_ctx_contains(name, "libSceAgc") || obs_ctx_contains(name, "AgcDriver") ||
        obs_ctx_contains(name, "libSceAgcDriver")) {
        g->agc = 1;
    }
    if (obs_ctx_contains(name, "libSceGnm") || obs_ctx_contains(name, "GnmDriver") ||
        obs_ctx_contains(name, "libSceGnmDriver")) {
        g->gnm = 1;
    }
    return 0;
}

void obs_run_context(char *name, size_t name_cap, char *basis, size_t basis_cap) {
    const char *delivery;
    const char *delivery_detail;
    int is_host = 0;
#if defined(OBSCENE_HOST_BUILD)
    is_host = 1;
    delivery = "host";
    delivery_detail = "host build";
#else
    /* Delivery discriminator: an explicit check for payload bootstrap / payload args
     * rather than inferring from libkernel base != 0. A title running in an environment
     * where dlsym resolves getpid may still have a known libkernel base without being a
     * payload (REQ-20260910T0410Z-e5d9). */
    if (obs_payload_output_bootstrapped || obs_get_payload_args() != NULL) {
        delivery = "payload";
        delivery_detail = "elfldr payload";
    } else {
        delivery = "title";
        delivery_detail = "title eboot";
    }
#endif

    const char *generation;
    const char *gpu_detail;
    if (is_host) {
        generation = "na";
        gpu_detail = "no console libraries";
    } else {
        struct obs_ctx_gpu g = {0, 0, 0};
        const char *why = "ok";
        obs_linkmap_walk(obs_ctx_gpu_cb, &g, &why);
        if (g.walked == 0) {
            generation = "unknown-gpu";
            gpu_detail = why;
        } else if (g.agc) {
            generation = "prospero-native";
            gpu_detail = "libSceAgc mapped";
        } else if (g.gnm) {
            generation = "orbis-compat";
            gpu_detail = "libSceGnm mapped, libSceAgc absent";
        } else {
            generation = "unknown-gpu";
            gpu_detail = "no GPU library among loaded modules";
        }
    }

    size_t p = 0;
    p = obs_ctx_append(name, p, name_cap, delivery);
    p = obs_ctx_append(name, p, name_cap, "/");
    (void)obs_ctx_append(name, p, name_cap, generation);

    size_t q = 0;
    q = obs_ctx_append(basis, q, basis_cap, delivery_detail);
    q = obs_ctx_append(basis, q, basis_cap, "; ");
    (void)obs_ctx_append(basis, q, basis_cap, gpu_detail);
}

#if !defined(OBSCENE_HOST_BUILD)
typedef struct {
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} obs_elf64_sym;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
} obs_elf64_rela;

#if !defined(OBSCENE_TARGET_EBOOT)
static uintptr_t obs_find_own_base(void) {
    uintptr_t addr = (uintptr_t)&obs_bind_dynamic_symbols;
    addr &= ~0x3fffUL;
    for (int i = 0; i < 4096; i++) {
        if (addr < 0x10000UL) {
            break;
        }
        if (obs_linkmap_readable(addr) &&
            *(const uint32_t *)addr == 0x464c457f) {
            return addr;
        }
        addr -= 0x4000UL;
    }
    return 0;
}

#if !defined(OBSCENE_HOST_BUILD)
OBS_WEAK int __sys_socketex(const char *name, int domain, int type, int protocol);
OBS_WEAK int bind(int s, const void *addr, uint32_t addrlen);
OBS_WEAK long _sendto(int s, const void *msg, size_t len, int flags, const void *to,
                      uint32_t tolen);
OBS_WEAK int _setsockopt(int s, int level, int optname, const void *optval,
                         uint32_t optlen);
OBS_WEAK long recv(int s, void *buf, size_t len, int flags);
OBS_WEAK int accept(int s, void *addr, uint32_t *addrlen);
OBS_WEAK int listen(int s, int backlog);
OBS_WEAK int connect(int s, const void *name, uint32_t namelen);
OBS_WEAK int close(int fd);
OBS_WEAK int *__error(void);

static const void *const s_weak_posix_refs[] = {
    (const void *)&__sys_socketex, (const void *)&bind,    (const void *)&_sendto,
    (const void *)&_setsockopt,    (const void *)&recv,    (const void *)&accept,
    (const void *)&listen,         (const void *)&connect, (const void *)&close,
    (const void *)&__error,
};
#endif
#endif

static void obs_relocate_payload_got(void) {
#if defined(OBSCENE_TARGET_EBOOT)
    return;
#else
    if (obs_get_payload_args() == NULL) {
        return;
    }
#if !defined(OBSCENE_HOST_BUILD)
    (void)s_weak_posix_refs;
#endif
    uintptr_t base = obs_find_own_base();
    if (base == 0) {
        return;
    }
    const obs_elf64_dyn *dyn = _DYNAMIC;
    if (dyn == NULL) {
        return;
    }

    uintptr_t jmprel = 0;
    size_t pltrelsz = 0;
    uintptr_t symtab = 0;
    uintptr_t strtab = 0;

    for (size_t i = 0; dyn[i].d_tag != 0; i++) {
        switch (dyn[i].d_tag) {
        case 0x17: /* DT_JMPREL */
            jmprel = (uintptr_t)dyn[i].d_val;
            break;
        case 0x02: /* DT_PLTRELSZ */
            pltrelsz = (size_t)dyn[i].d_val;
            break;
        case 0x06: /* DT_SYMTAB */
            symtab = (uintptr_t)dyn[i].d_val;
            break;
        case 0x05: /* DT_STRTAB */
            strtab = (uintptr_t)dyn[i].d_val;
            break;
        }
    }

    if (jmprel == 0 || pltrelsz == 0 || symtab == 0 || strtab == 0) {
        return;
    }

    if (jmprel < base)
        jmprel += base;
    if (symtab < base)
        symtab += base;
    if (strtab < base)
        strtab += base;

    const payload_args_t *pargs = obs_get_payload_args();
    const obs_elf64_rela *r = (const obs_elf64_rela *)jmprel;
    size_t count = pltrelsz / sizeof(obs_elf64_rela);
    uintptr_t plt_start = 0;
    uintptr_t plt_end = 0;

    if (count > 0) {
        uint64_t slot0_val = *(const uint64_t *)(base + r[0].r_offset);
        uintptr_t first_stub = 0;
        if (slot0_val >= base && slot0_val < base + 0x1000000UL) {
            first_stub = (uintptr_t)slot0_val - 6;
        } else if (slot0_val > 0 && slot0_val < 0x1000000UL) {
            first_stub = base + (uintptr_t)slot0_val - 6;
        }
        if (first_stub >= base && first_stub < base + 0x1000000UL && first_stub >= 0x10) {
            plt_start = first_stub - 0x10;
            plt_end = plt_start + 0x10 + count * 0x10;
            obs_set_plt_bounds(plt_start, plt_end);
        }
    }

    for (size_t i = 0; i < count; i++) {
        uint32_t sym_idx = (uint32_t)(r[i].r_info >> 32);
        uint32_t r_type = (uint32_t)(r[i].r_info & 0xffffffff);
        if (r_type == 7 || r_type == 6) { /* R_X86_64_JUMP_SLOT or GLOB_DAT */
            const obs_elf64_sym *sym =
                (const obs_elf64_sym *)(symtab +
                                        (size_t)sym_idx * sizeof(obs_elf64_sym));
            const char *sym_name = (const char *)(strtab + sym->st_name);
            uint64_t *got_slot = (uint64_t *)(base + r[i].r_offset);

            /* Snapshot loader initial value before patching */
            uint64_t initial_val = *got_slot;
            for (size_t w = 0; w < OBS_LOADER_WEAK_COUNT; w++) {
                if (obs_strcmp(sym_name, s_loader_weak_entries[w].name) == 0) {
                    s_loader_weak_entries[w].initial_got = initial_val;
                    if (initial_val != 0 &&
                        (plt_start == 0 || initial_val < plt_start ||
                         initial_val >= plt_end) &&
                        (initial_val < base || initial_val >= (base + 0x2000000UL))) {
                        s_loader_weak_entries[w].is_bound = 1;
                    } else {
                        s_loader_weak_entries[w].is_bound = 0;
                    }
                    break;
                }
            }

            const void *resolved = NULL;
            if (pargs != NULL && pargs->kexport_table != NULL) {
                char nid[12];
                obs_compute_nid(sym_name, nid);
                resolved = obs_kexport_lookup(
                    (const obs_kexport_table_t *)pargs->kexport_table, nid);
            }
            if (resolved == NULL) {
                int h = obs_module_open("libkernel");
                if (h >= 0) {
                    resolved = obs_module_symbol(h, sym_name);
                }
            }
            if (resolved == NULL) {
                for (unsigned int s = 0; s < obs_section_count; s++) {
                    const obs_section *sec = obs_sections[s];
                    if (sec == NULL)
                        continue;
                    for (unsigned int c = 0; c < sec->check_count; c++) {
                        const obs_check *chk = &sec->checks[c];
                        if (chk->symbol != NULL &&
                            obs_strcmp(chk->symbol, sym_name) == 0 &&
                            chk->library != NULL &&
                            obs_strcmp(chk->library, "libkernel") != 0 &&
                            obs_strcmp(chk->library, "obscene") != 0) {
                            int mod = obs_module_open(chk->library);
                            if (mod >= 0) {
                                resolved = obs_module_symbol(mod, sym_name);
                                if (resolved != NULL)
                                    break;
                            }
                        }
                    }
                    if (resolved != NULL)
                        break;
                }
            }
            if (resolved != NULL && obs_address_is_callable(resolved)) {
                *got_slot = (uint64_t)(uintptr_t)resolved;
            } else if (initial_val != 0 &&
                       (plt_start == 0 || initial_val < plt_start ||
                        initial_val >= plt_end) &&
                       (initial_val < base || initial_val >= (base + 0x2000000UL))) {
                *got_slot = initial_val;
            } else {
                *got_slot = 0;
            }
        }
    }
#endif
}

void obs_bind_dynamic_symbols(void) {
    obs_relocate_payload_got();
    for (unsigned int s = 0; s < obs_section_count; s++) {
        const obs_section *section = obs_sections[s];
        if (section == NULL)
            continue;
        for (unsigned int c = 0; c < section->check_count; c++) {
            obs_check *check = (obs_check *)&section->checks[c];
            if (check->library != NULL && check->symbol != NULL &&
                !obs_address_is_callable(check->address)) {
                int handle = obs_module_open(check->library);
                if (handle >= 0) {
                    const void *addr = obs_module_symbol(handle, check->symbol);
                    if (addr != NULL) {
                        check->address = addr;
                    }
                }
            }
        }
    }
}

#if !defined(OBSCENE_TARGET_MODULE)
static int obs_is_prospero(void) {
    static int s_is_prospero = -1;
    if (s_is_prospero != -1) {
        return s_is_prospero;
    }
#if OOPS_TARGET_IS_PROSPERO || (defined(OBSCENE_GEN) && (OBSCENE_GEN >= 5))
    s_is_prospero = 1;
    return 1;
#else
    if (obs_detected_generation() == OBS_GENERATION_PROSPERO) {
        s_is_prospero = 1;
        return 1;
    }
    s_is_prospero = 0;
    return 0;
#endif
}

int sceKernelAllocateDirectMemory(sce_off_t search_start, sce_off_t search_end,
                                  size_t length, size_t alignment, int memory_type,
                                  sce_off_t *physical_address) {
    long ret =
        obs_invoke_syscall(572, (long)search_start, (long)search_end, (long)length,
                           (long)alignment, (long)memory_type, (long)physical_address);
    return (int)ret;
}

int sceKernelMapDirectMemory(void **virtual_address, size_t length, int protection,
                             int flags, sce_off_t physical_address, size_t alignment) {
    long num = obs_is_prospero() ? 585 : 573;
    long ret =
        obs_invoke_syscall(num, (long)virtual_address, (long)length, (long)protection,
                           (long)flags, (long)physical_address, (long)alignment);
    return (int)ret;
}

int sceKernelReleaseDirectMemory(sce_off_t physical_address, size_t length) {
    long num = obs_is_prospero() ? 586 : 574;
    long ret =
        obs_invoke_syscall(num, (long)physical_address, (long)length, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelMunmap(void *address, size_t length) {
    long ret = obs_invoke_syscall(73, (long)address, (long)length, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelVirtualQuery(const void *address, int flags, void *info,
                          size_t info_size) {
    long ret = obs_invoke_syscall(603, (long)address, (long)flags, (long)info,
                                  (long)info_size, 0, 0);
    return (int)ret;
}

int sceKernelUsleep(unsigned int microseconds) {
    if (s_fn_usleep != NULL) {
        return s_fn_usleep(microseconds);
    }
    struct {
        long sec;
        long nsec;
    } req;
    req.sec = (long)(microseconds / 1000000u);
    req.nsec = (long)((microseconds % 1000000u) * 1000u);
    return (int)obs_invoke_syscall(240, (long)&req, 0, 0, 0, 0, 0);
}

int sceKernelOpen(const char *path, int flags, uint16_t mode) {
    if (s_fn_open != NULL) {
        return s_fn_open(path, flags, mode);
    }
    return (int)obs_invoke_syscall(5, (long)path, (long)flags, (long)mode, 0, 0, 0);
}

int sceKernelClose(int fd) {
    if (s_fn_close != NULL) {
        return s_fn_close(fd);
    }
    return (int)obs_invoke_syscall(6, (long)fd, 0, 0, 0, 0, 0);
}

sce_ssize_t sceKernelRead(int fd, void *buf, size_t count) {
    if (s_fn_read != NULL) {
        return s_fn_read(fd, buf, count);
    }
    return (sce_ssize_t)obs_invoke_syscall(3, (long)fd, (long)buf, (long)count, 0, 0,
                                           0);
}

sce_ssize_t sceKernelWrite(int fd, const void *buf, size_t count) {
    if (s_fn_write != NULL) {
        return s_fn_write(fd, buf, count);
    }
    return (sce_ssize_t)obs_invoke_syscall(4, (long)fd, (long)buf, (long)count, 0, 0,
                                           0);
}

sce_ssize_t sceKernelGetdents(int fd, char *buf, int nbytes) {
    long ret = obs_invoke_syscall(272, (long)fd, (long)buf, (long)nbytes, 0, 0, 0);
    return (sce_ssize_t)ret;
}
#endif /* !defined(OBSCENE_TARGET_MODULE) */
#endif
