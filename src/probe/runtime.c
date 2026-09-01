/*
 * The freestanding runtime.
 *
 * Two build shapes share this file. The target build has no libc at all and reaches
 * the platform's write through its own system library. The host build links libc so
 * the harness itself can be run and tested on an ordinary machine, with the platform
 * calls stubbed - which is what makes the framework verifiable before any emulator
 * can load it.
 */

#include "common/freestd.h"
#include "obscene/runtime.h"
#include "obscene/sink.h"

#if defined(OBSCENE_HOST_BUILD)
#include <unistd.h>
#include "obscene/platform.h"
#else
#include "obscene/harness.h"
#include "obscene/platform.h"

/* Declared here rather than in `platform.h`, which is where it belongs.
 *
 * The census in `corpus.h` declares this name as `const char` so the type system forbids
 * calling it, and `bulk.c` and `surface.c` include both that and `platform.h` - so a function
 * declaration in the shared header is a conflict in those two translation units. Moving it
 * properly means excluding it from the generated census, which is the documented five-step
 * process in `CLAUDE.md` and is worth doing.
 *
 * Until then this is the third local copy of one signature, after `start.c` and `min.c`, and
 * three copies of a judgement is the thing this project says not to do. It is written down
 * here rather than left to be noticed.
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
 * Defined outside the payload-only block so a section can read it on any build: it is zero on
 * the host and on an eboot - neither is loaded by elfldr - which the reader treats as "no base,
 * skip". Only the setter (in the payload block below) is payload-specific. */
static unsigned long obs_libkernel_base_value;

unsigned long obs_libkernel_base(void) {
    return obs_libkernel_base_value;
}

#if !defined(OBSCENE_HOST_BUILD)
typedef struct {
    int64_t  d_tag;
    uint64_t d_val;
} obs_elf64_dyn;

extern const obs_elf64_dyn _DYNAMIC[];

/* One record to the system log, and nothing inferred from the result.
 *
 * Not one of the channels above, deliberately - see the call site in `obs_write`. The call
 * returns a status rather than a byte count, so it cannot answer "did this work"; asked to,
 * it answers yes on a loader that stub-resolves it and the report is lost behind it.
 *
 * NUL-terminated into a bounded buffer because this takes a string rather than a length, and
 * a record longer than the buffer is refused rather than truncated: half a record still
 * parses, which is worse than none. (D233) */
long obs_invoke_syscall(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    unsigned long base = obs_libkernel_base();
    uintptr_t gadget = (base != 0) ? (base + 0x6aaUL) : 0;

    long ret;
    register long r10_arg __asm__("r10") = a4;
    register long r8_arg  __asm__("r8")  = a5;
    register long r9_arg  __asm__("r9")  = a6;

    if (gadget != 0) {
        __asm__ volatile(
            "movq %5, %%rax\n"
            "movq %6, %%r10\n"
            "callq *%7\n"
            "jnc 1f\n"
            "movq $-1, %0\n"
            "jmp 2f\n"
            "1:\n"
            "movq %%rax, %0\n"
            "2:\n"
            : "=r"(ret)
            : "D"(a1), "S"(a2), "d"(a3), "r"(r8_arg), "r"(num), "r"(r10_arg), "r"(gadget), "r"(r9_arg)
            : "rax", "rcx", "r11", "memory"
        );
    } else {
        __asm__ volatile(
            "movq %5, %%rax\n"
            "movq %6, %%r10\n"
            "syscall\n"
            "jnc 1f\n"
            "movq $-1, %0\n"
            "jmp 2f\n"
            "1:\n"
            "movq %%rax, %0\n"
            "2:\n"
            : "=r"(ret)
            : "D"(a1), "S"(a2), "d"(a3), "r"(r8_arg), "r"(num), "r"(r10_arg), "r"(r9_arg)
            : "rax", "rcx", "r11", "memory"
        );
    }
    return ret;
}

static void obs_debug_out_write(const char *bytes, size_t len) {
    static char scratch[512];
    if (len == 0 || len >= sizeof scratch) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        scratch[i] = bytes[i];
    }
    scratch[len] = '\0';

    unsigned long base = obs_libkernel_base();
    if (base != 0) {
        (void)obs_invoke_syscall(601, 7, (long)scratch, 0, 0, 0, 0);
    } else if (obs_address_is_callable((const void *)&sceKernelDebugOutText)) {
        (void)sceKernelDebugOutText(0, scratch);
    }
}

static obs_channel obs_output_channel = OBS_CHANNEL_UNTRIED;

/* A write bootstrapped from payload_args, for the raw-payload context.
 *
 * # The bug this fixes
 *
 * Loaded by elfldr, a payload has *no imports resolved* - the loader applies only relocations
 * and resolves nothing (D209). So the weak `sceKernelWrite` the channel below calls is null,
 * every text channel is absent, and the whole report goes nowhere. The minimal `boot.c` avoids
 * this by computing a write from `payload_args[0]` and calling it directly; the full suite never
 * did, so a suite run as a payload produced silence.
 *
 * This is that same computation, made available to the channel: set once at entry, from
 * `getpid` (which elfldr hands as `payload_args[0]`) plus the vaddrs selfish read off the real
 * 12.40 `libkernel_sys.sprx` - `getpid` at `0x5b0`, `sceKernelWrite` at `0x16e00` (D209). Null
 * on every other build, where the ordinary channel selection stands untouched.
 */
static int obs_payload_output_bootstrapped;

void obs_bootstrap_payload_output(unsigned long payload_args_word0) {
    /* Only from something shaped like a libkernel export address. A build that is not a payload
     * has argc or a stack pointer in word zero, and computing a call target from that and jumping
     * to it is the one thing this must never do - so it is refused unless the value is a
     * canonical, sixteen-aligned low-half address, which getpid is and neither of those is. */
    if (payload_args_word0 < 0x10000UL || payload_args_word0 >= 0x0000800000000000UL
        || (payload_args_word0 & 0xfUL) != 0) {
        return;
    }
    unsigned long base = payload_args_word0 - 0x5b0UL;
    obs_libkernel_base_value = base;
    obs_payload_output_bootstrapped = 1;
}

/* Sends what it can through one channel. Returns bytes accepted, zero if the channel
 * is absent or refused. */
static size_t obs_send(obs_channel channel, const char *bytes, size_t len) {
    /* One record. Sized to match the report's own line buffer: nothing longer than
     * this is ever produced, and refusing anything longer is safer than truncating a
     * record into something that still parses. */
    static char scratch[512];

    switch (channel) {
    case OBS_CHANNEL_KERNEL_WRITE: {
        /* The bootstrapped write first, when a payload set one: in that context the weak symbol
         * is null and this is the only working write there is (see obs_bootstrap_payload_output). */
        if (obs_payload_output_bootstrapped) {
            if (len >= sizeof(scratch)) len = sizeof(scratch) - 1;
            for (size_t i = 0; i < len; i++) {
                scratch[i] = bytes[i];
            }
            scratch[len] = '\0';
            long n = obs_invoke_syscall(601, 7, (long)scratch, 0, 0, 0, 0);
            return n >= 0 ? len : 0;
        }
        if (&sceKernelWrite == 0) {
            return 0;
        }
        long n = (long)sceKernelWrite(OBS_FD_STDOUT, bytes, len);
        return n > 0 ? (size_t)n : 0;
    }
    case OBS_CHANNEL_PUTS: {
        if (&puts == 0) {
            return 0;
        }
        /* Only a whole record. `puts` supplies a newline, so handing it a partial line
         * would break the record in two - something that parses and is wrong, which is
         * worse than no output. Every caller writes one complete line, so refusing
         * anything else costs nothing and cannot be got wrong later. */
        if (len == 0 || len > sizeof(scratch) || bytes[len - 1] != '\n') {
            return 0;
        }
        for (size_t i = 0; i + 1 < len; i++) {
            scratch[i] = bytes[i];
        }
        scratch[len - 1] = '\0';
        /* Returns a non-negative value on success, EOF on failure. A stub returning
         * zero counts as success here, which is why this is tried after the channels
         * that report a byte count. */
        if (puts(scratch) < 0) {
            return 0;
        }
        return len;
    }
    case OBS_CHANNEL_POSIX_WRITE: {
        if (&write == 0) {
            return 0;
        }
        long n = (long)write(OBS_FD_STDOUT, bytes, len);
        return n > 0 ? (size_t)n : 0;
    }
    case OBS_CHANNEL_PUTCHAR: {
        if (&putchar == 0) {
            return 0;
        }
        /* Returns the character written. Anything else is a failure, and checking for
         * it is what stops a stub that returns zero reading as success - which is
         * exactly how the first version of this lost the whole report. */
        int c = (int)(unsigned char)bytes[0];
        if (putchar(c) != c) {
            return 0;
        }
        return 1;
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

/* Nothing to do: the host has no system log, and its report goes to standard output through
 * libc. Defined rather than the call site being conditional, so `obs_write` reads the same in
 * both builds. */
static void obs_debug_out_write(const char *bytes, size_t len) {
    (void)bytes;
    (void)len;
}
#endif

/* An extra destination for every record, set while a command wants the report on its own
 * channel rather than the probe's.
 *
 * The `report` verb runs the suite, and its records belong to the driver that asked - so
 * during that command this tee points at the session socket, and the section/try/res/sym
 * records arrive between the `ack` and the `done`, exactly as docs/PROTOCOL.md promises.
 * Null the rest of the time, so an ordinary run pays nothing for it.
 *
 * A single function pointer rather than a channel in the enum: the enum picks *one* text
 * channel, and this is deliberately additive - the report still goes to stdout and the file
 * sink while a copy goes down the socket. */
static void (*obs_write_tee)(void *ctx, const char *bytes, size_t len);
static void *obs_write_tee_ctx;

void obs_set_write_tee(void (*fn)(void *ctx, const char *bytes, size_t len), void *ctx) {
    obs_write_tee = fn;
    obs_write_tee_ctx = ctx;
}

void obs_write(const char *bytes, size_t len) {
    /* The tee first, so a record reaches the driver that asked for it even if a text
     * channel below hangs or the process then dies - same durable-write-ahead-of-risky
     * ordering as the sink. */
    if (obs_write_tee != NULL) {
        obs_write_tee(obs_write_tee_ctx, bytes, len);
    }

    /* The file next, and unconditionally.
     *
     * Not a fallback and not part of the channel selection below - those are alternatives
     * and exactly one of them is chosen. This is a second destination, and it gets the
     * bytes whether or not a text channel works, because the case it exists for is the one
     * where none of them does.
     *
     * Before the channel loop rather than after, so a text channel that hangs or ends the
     * process cannot cost the record on disk. The ordering is the same reasoning as
     * announce-before-attempting: put the durable write ahead of the risky one. */
    obs_sink_write(bytes, len);

    /* The system log next, and also unconditionally - a second destination, not a candidate.
     *
     * # Why it cannot be one of the channels below
     *
     * The selection works by asking each candidate to move bytes and believing the first that
     * says it did. `sceKernelDebugOutText` returns a status, not a count, so it cannot answer
     * that question honestly - and a channel that always claims success is selected on any
     * loader that stub-resolves the symbol, after which every record goes nowhere.
     *
     * That is not hypothetical either. Put first in the candidate list, it did exactly that
     * under Kyty, which patches unresolved imports to a stub that returns:
     *
     *     Unresolved import stub called [15]: symbol=9JYNqN6jAKI[libkernel_v1]
     *
     * `9JYNqN6jAKI` is this function, and the whole report was lost behind it. The comment on
     * the candidate order below already warned about exactly this shape - a channel that
     * reports success and prints nothing - and the warning was reintroduced above it. (D233)
     *
     * As a second destination the question never arises: nothing is inferred from the call, so
     * nothing can be inferred wrongly. It costs a duplicate on a loader that implements both,
     * which is what the file sink above already costs and for the same reason - the case it
     * exists for is the one where the channels below produce nothing. */
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
            for (unsigned int i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
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
 * Enumerate the runtime linker's loaded objects by walking its own link-map, and from that -
 * plus the build and the payload anchor - name the execution context a run measures in. Both
 * read only memory the loader already wrote, with no syscall, so they work where the platform
 * refuses sceKernelGetModuleInfo (measured: the compatibility host does).
 *
 * The layout is standard and cited, nothing here is invented or vendor-derived: the ELF
 * dynamic array (d_tag then d_un, eight bytes each) and DT_DEBUG = 21 are the ELF ABI; r_debug
 * (r_version, then r_map at offset 8) is FreeBSD <sys/link_elf.h>; link_map (l_addr at 0,
 * l_name at 8, l_next at 0x18) is FreeBSD <link.h>. The console is FreeBSD-derived - the same
 * citation the directory walk uses for dirent. src/sections/modlink.c reports the full
 * inventory through this same walk. */



/* Whether an address can be dereferenced, verified via direct kernel virtual query probe. */
static int obs_linkmap_readable(uintptr_t p) {
    if (p < 0x10000u || p >= 0x0000800000000000UL) {
        return 0;
    }
#if defined(OBSCENE_HOST_BUILD)
    return 0;
#else
    char info[96];
    for (size_t i = 0; i < sizeof(info); i++) {
        info[i] = 0;
    }
    int ret = sceKernelVirtualQuery((const void *)p, 0, info, sizeof(info));
    if (ret == 0) {
        return 1;
    }
    return 0;
#endif
}

/* Locate the runtime dynamic section carrying DT_DEBUG. Checks payload _DYNAMIC, then eboot and libkernel. */
static const unsigned char *obs_linkmap_own_dynamic(const char **reason) {
#if !defined(OBSCENE_HOST_BUILD)
    /* 1. Check payload's own _DYNAMIC */
    if (obs_linkmap_readable((uintptr_t)_DYNAMIC)) {
        for (unsigned int i = 0; i < 4096; i++) {
            if (_DYNAMIC[i].d_tag == 0) break;
            if (_DYNAMIC[i].d_tag == 21 && _DYNAMIC[i].d_val != 0) {
                return (const unsigned char *)_DYNAMIC;
            }
        }
    }

    /* 2. Check main eboot text / data segments by querying virtual memory */
    uintptr_t addr = 0x400000UL;
    for (int step = 0; step < 32 && addr < 0x80000000UL; ) {
        char vq_buf[96];
        for (size_t k = 0; k < sizeof(vq_buf); k++) vq_buf[k] = 0;
        int ret = sceKernelVirtualQuery((const void *)addr, 0, vq_buf, sizeof(vq_buf));
        if (ret != 0) {
            addr += 0x4000u;
            step++;
            continue;
        }
        uintptr_t seg_start = *(const uintptr_t *)(vq_buf + 0);
        size_t seg_size = *(const size_t *)(vq_buf + 8);
        if (seg_size == 0) seg_size = 0x4000u;

        if (obs_linkmap_readable(seg_start)) {
            const unsigned char *elf = (const unsigned char *)seg_start;
            if (elf[0] == 0x7f && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F') {
                uint64_t e_phoff = *(const uint64_t *)(elf + 0x20);
                uint16_t e_phentsize = *(const uint16_t *)(elf + 0x36);
                uint16_t e_phnum = *(const uint16_t *)(elf + 0x38);
                if (e_phoff != 0 && e_phentsize >= 0x38 && e_phnum > 0 && e_phnum <= 64) {
                    for (uint16_t i = 0; i < e_phnum; i++) {
                        const unsigned char *ph = elf + e_phoff + ((size_t)i * e_phentsize);
                        uint32_t p_type = *(const uint32_t *)(ph + 0x00);
                        uint64_t p_vaddr = *(const uint64_t *)(ph + 0x10);
                        uintptr_t dyn_addr = (p_vaddr >= seg_start) ? (uintptr_t)p_vaddr : (seg_start + (uintptr_t)p_vaddr);
                        if (p_type == 2 && obs_linkmap_readable(dyn_addr)) { /* PT_DYNAMIC */
                            return (const unsigned char *)dyn_addr;
                        }
                    }
                }
            }
        }
        addr = seg_start + seg_size;
        step++;
    }

    /* 3. Check libkernel data segment */
    unsigned long lk_base = obs_libkernel_base();
    if (lk_base != 0) {
        uintptr_t lk_addr = lk_base;
        for (int step = 0; step < 16; step++) {
            char vq_buf[96];
            for (size_t k = 0; k < sizeof(vq_buf); k++) vq_buf[k] = 0;
            int ret = sceKernelVirtualQuery((const void *)lk_addr, 0, vq_buf, sizeof(vq_buf));
            if (ret != 0) break;
            uintptr_t seg_start = *(const uintptr_t *)(vq_buf + 0);
            size_t seg_size = *(const size_t *)(vq_buf + 8);
            if (seg_size == 0) seg_size = 0x4000u;
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

unsigned int obs_linkmap_walk(int (*cb)(const char *name, unsigned long base, void *user),
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
        if (r_debug != 0 && obs_linkmap_readable(r_debug) && obs_linkmap_readable(r_debug + 8u)) {
            uintptr_t node = *(const uintptr_t *)(r_debug + 8u); /* r_map */
            for (unsigned int i = 0; i < 512u && node != 0; i++) {
                if ((node & 0x7u) != 0 || !obs_linkmap_readable(node) || !obs_linkmap_readable(node + 0x18u)) {
                    *reason = "link-map chain left mapped memory";
                    break;
                }
                unsigned long l_addr = (unsigned long)*(const uintptr_t *)(node + 0x00u);
                uintptr_t name_ptr = *(const uintptr_t *)(node + 0x08u);
                const char *l_name = "";
                if (name_ptr != 0 && (name_ptr & 0x7u) == 0 && obs_linkmap_readable(name_ptr)) {
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

    /* Fallback: if DT_DEBUG was not present or yielded no nodes, query platform module list */
    if (count == 0) {
        int handles[64];
        size_t written = 0;
        int rc = sceKernelGetModuleList(handles, 64, &written);
        if (rc == 0 && written > 0) {
            for (size_t i = 0; i < written; i++) {
                char info_buf[512];
                for (size_t k = 0; k < sizeof(info_buf); k++) info_buf[k] = 0;
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
        dst[pos] = *src;
        pos++;
        src++;
    }
    if (cap > 0) {
        dst[pos] = '\0';
    }
    return pos;
}

/* Does hay contain needle? */
static int obs_ctx_contains(const char *hay, const char *needle) {
    if (hay == NULL || needle == NULL) {
        return 0;
    }
    for (unsigned int i = 0; hay[i] != '\0'; i++) {
        unsigned int j = 0;
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
    if (obs_ctx_contains(name, "libSceAgc")) {
        g->agc = 1;
    }
    if (obs_ctx_contains(name, "libSceGnm")) {
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
    if (obs_libkernel_base() != 0) {
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
            generation = "ps5-native";
            gpu_detail = "libSceAgc mapped";
        } else if (g.gnm) {
            generation = "ps4-bc";
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
    int64_t  r_addend;
} obs_elf64_rela;

void obs_bind_dynamic_symbols(void) {
    for (unsigned int s = 0; s < obs_section_count; s++) {
        const obs_section *section = obs_sections[s];
        if (section == NULL) continue;
        for (unsigned int c = 0; c < section->check_count; c++) {
            obs_check *check = (obs_check *)&section->checks[c];
            if (check->library != NULL && check->symbol != NULL && !obs_address_is_callable(check->address)) {
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

int sceKernelDlsym(int handle, const char *symbol, void **address_out) {
    long ret = obs_invoke_syscall(591, (long)handle, (long)symbol, (long)address_out, 0, 0, 0);
    return (int)ret;
}

int sceKernelGetModuleList(int *handles, size_t max, size_t *written) {
    size_t count = 0;
    long ret = obs_invoke_syscall(599, (long)handles, (long)max, (long)&count, 0, 0, 0);
    if (ret >= 0) {
        if (written != NULL) {
            *written = (count > 0) ? count : (size_t)ret;
        }
        return 0;
    }
    return (int)ret;
}

int sceKernelGetModuleInfo(int handle, void *info) {
    if (info != NULL) {
        size_t *sz = (size_t *)info;
        if (*sz == 0) {
            *sz = 0x160;
        }
    }
    long ret = obs_invoke_syscall(593, (long)handle, (long)info, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelLoadStartModule(const char *name, size_t argc, const void *argv, unsigned int flags, void *p5, int *p6) {
    (void)argc; (void)argv; (void)p5; (void)p6;
    int handle = -1;
    long ret = obs_invoke_syscall(594, (long)name, (long)flags, (long)&handle, 0, 0, 0);
    if (ret == 0 && handle >= 0) {
        return handle;
    }
    return (int)ret;
}

int sceKernelAllocateDirectMemory(sce_off_t search_start, sce_off_t search_end, size_t length, size_t alignment, int memory_type, sce_off_t *physical_address) {
    long ret = obs_invoke_syscall(572, (long)search_start, (long)search_end, (long)length, (long)alignment, (long)memory_type, (long)physical_address);
    return (int)ret;
}

int sceKernelMapDirectMemory(void **virtual_address, size_t length, int protection, int flags, sce_off_t physical_address, size_t alignment) {
    long ret = obs_invoke_syscall(573, (long)virtual_address, (long)length, (long)protection, (long)flags, (long)physical_address, (long)alignment);
    return (int)ret;
}

int sceKernelReleaseDirectMemory(sce_off_t physical_address, size_t length) {
    long ret = obs_invoke_syscall(574, (long)physical_address, (long)length, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelMunmap(void *address, size_t length) {
    long ret = obs_invoke_syscall(73, (long)address, (long)length, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelVirtualQuery(const void *address, int flags, void *info, size_t info_size) {
    long ret = obs_invoke_syscall(603, (long)address, (long)flags, (long)info, (long)info_size, 0, 0);
    return (int)ret;
}

int sceKernelUsleep(unsigned int microseconds) {
    long ret = obs_invoke_syscall(240, (long)microseconds, 0, 0, 0, 0, 0);
    return (int)ret;
}

int sceKernelOpen(const char *path, int flags, uint16_t mode) {
    long ret = obs_invoke_syscall(5, (long)path, (long)flags, (long)mode, 0, 0, 0);
    return (int)ret;
}

int sceKernelClose(int fd) {
    long ret = obs_invoke_syscall(6, (long)fd, 0, 0, 0, 0, 0);
    return (int)ret;
}

sce_ssize_t sceKernelRead(int fd, void *buf, size_t count) {
    long ret = obs_invoke_syscall(3, (long)fd, (long)buf, (long)count, 0, 0, 0);
    return (sce_ssize_t)ret;
}

sce_ssize_t sceKernelWrite(int fd, const void *buf, size_t count) {
    long ret = obs_invoke_syscall(4, (long)fd, (long)buf, (long)count, 0, 0, 0);
    return (sce_ssize_t)ret;
}

sce_ssize_t sceKernelGetdents(int fd, char *buf, int nbytes) {
    long ret = obs_invoke_syscall(272, (long)fd, (long)buf, (long)nbytes, 0, 0, 0);
    return (sce_ssize_t)ret;
}
#else
void obs_bind_dynamic_symbols(void) {}
#endif

