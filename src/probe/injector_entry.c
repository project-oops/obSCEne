/*
 * obSCEne Injector Entry Point.
 *
 * Thin payload launcher that resolves the target process, embeds
 * obscene-probe-<target>.elf, and delegates process control, remote ELF mapping, and
 * thread hijacking to oops-sdk.
 */

#include "oops/inject.h"
#include "oops/freestd.h"
#include "oops/syscall.h"
#include "oops/krw.h"

extern const uint8_t __obscene_payload_start[] __attribute__((visibility("hidden")));
extern const uint8_t __obscene_payload_end[] __attribute__((visibility("hidden")));

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
} injector_elf64_rela;

extern const injector_elf64_rela __rela_dyn_start[]
    __attribute__((visibility("hidden")));
extern const injector_elf64_rela __rela_dyn_end[] __attribute__((visibility("hidden")));

int injector_start(payload_args_t *args);

static uintptr_t injector_find_own_base(void);
static uintptr_t injector_self_relocate(void);

static uintptr_t injector_find_own_base(void) {
    uintptr_t addr = (uintptr_t)&injector_start;
    addr &= ~0x3fffUL;
    for (int i = 0; i < 4096; i++) {
        if (addr < 0x10000UL) {
            break;
        }
        if (*(const uint32_t *)addr == 0x464c457fU) {
            return addr;
        }
        addr -= 0x4000UL;
    }
    return 0;
}

static uintptr_t injector_self_relocate(void) {
    uintptr_t base = injector_find_own_base();
    if (base == 0) {
        return 0;
    }
    const injector_elf64_rela *r = __rela_dyn_start;
    const injector_elf64_rela *end = __rela_dyn_end;
    while (r < end) {
        uint32_t r_type = (uint32_t)(r->r_info & 0xffffffffU);
        if (r_type == 8) { /* R_X86_64_RELATIVE */
            uint64_t *target = (uint64_t *)(base + r->r_offset);
            if (r->r_addend >= 0) {
                *target = (uint64_t)base + (uint64_t)r->r_addend;
            } else {
                *target = (uint64_t)base - (uint64_t)(-r->r_addend);
            }
        }
        r++;
    }
    return base;
}

static void injector_exit(int code) __attribute__((noreturn));
static void injector_exit(int code) {
    sys_call(SYS_exit, (long)code, 0, 0, 0, 0, 0);
    for (;;) {
        __asm__ volatile("pause");
    }
}

int injector_start(payload_args_t *args);

int injector_start(payload_args_t *args) {
    uintptr_t base = injector_self_relocate();

    if (args == NULL) {
        injector_exit(-1);
    }

    sys_call_init(args);

    klog_write("starting obscene-injector payload (backed by oops-sdk)...");
    if (base != 0) {
        klog_write_hex("injector: base=", base);
    }

    if (krw_init(args) != 0) {
        klog_write("ERROR: krw_init failed");
        injector_exit(-2);
    }

    if (krw_elevate_current_process() != 0) {
        klog_write("ERROR: krw_elevate_current_process failed");
        injector_exit(-3);
    }

#ifdef OBSCENE_INJECT_TARGET
    const char *target_spec = OBSCENE_INJECT_TARGET;
    if (target_spec[0] == '\0') {
        target_spec = NULL;
    }
#else
    const char *target_spec = NULL;
#endif
    pid_t target_pid = target_resolve(target_spec);
    if (target_pid <= 0) {
        klog_write("ERROR: no running game process found (launch a retail game first)");
        krw_restore_current_process();
        injector_exit(-4);
    }
    klog_write_num("resolved target pid: ", (int64_t)target_pid);

    size_t payload_size =
        (size_t)((uintptr_t)__obscene_payload_end - (uintptr_t)__obscene_payload_start);
    if (payload_size == 0) {
        klog_write("ERROR: embedded payload missing");
        krw_restore_current_process();
        injector_exit(-5);
    }
    klog_write_num("using embedded payload blob, size: ", (int64_t)payload_size);

    int ret = oops_inject_elf(target_pid, __obscene_payload_start, payload_size, args);
    krw_restore_current_process();
    injector_exit(ret);
}
