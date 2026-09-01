/*
 * obscene-injector - Native PS5 Process Injector Entry Point.
 *
 * Consumes the session kernel R/W to hijack a target PS5 process,
 * map obSCEne's ELF segments, and jump to its entry point.
 */

#include "common/krw.h"
#include "common/freestd.h"
#include "common/syscall.h"
#include "injector/injector.h"
#include "injector/procctl.h"
#include "injector/target.h"
#include "injector/loader.h"

#define O_RDONLY 0

/* Embedded payload symbols (embedded directly in .rodata) */
extern const uint8_t __obscene_payload_start[] __attribute__((visibility("hidden")));
extern const uint8_t __obscene_payload_end[] __attribute__((visibility("hidden")));

void klog_write(const char *msg) {
    if (msg == NULL) return;
    size_t len = obs_strlen(msg);
    if (len == 0) return;

    char buf[256];
    static const char prefix[] = "<118>[injector] ";
    size_t prefix_len = sizeof(prefix) - 1;
    memcpy(buf, prefix, prefix_len);
    size_t copy_len = len;
    if (copy_len > sizeof(buf) - prefix_len - 2) {
        copy_len = sizeof(buf) - prefix_len - 2;
    }
    memcpy(buf + prefix_len, msg, copy_len);
    buf[prefix_len + copy_len] = '\n';
    buf[prefix_len + copy_len + 1] = '\0';
    sys_call(SYS_klog, 7, (long)buf, 0, 0, 0, 0);
    sys_call(SYS_write, 1, (long)buf, (long)(prefix_len + copy_len + 1), 0, 0, 0);
}

static void injector_exit(int code) __attribute__((noreturn));

static void injector_exit(int code) {
    sys_call(SYS_exit, (long)code, 0, 0, 0, 0, 0);
    for (;;) {
        __asm__ volatile("pause");
    }
}

void klog_write_num(const char *prefix, int64_t num) {
    char buf[128];
    size_t plen = obs_strlen(prefix);
    if (plen > sizeof(buf) - OBS_NUM_MAX - 1) {
        plen = sizeof(buf) - OBS_NUM_MAX - 1;
    }
    memcpy(buf, prefix, plen);
    size_t nlen = obs_format_i64(buf + plen, num);
    buf[plen + nlen] = '\0';
    klog_write(buf);
}

void klog_write_hex(const char *prefix, uint64_t hex) {
    char buf[128];
    size_t plen = obs_strlen(prefix);
    if (plen > sizeof(buf) - OBS_NUM_MAX - 1) {
        plen = sizeof(buf) - OBS_NUM_MAX - 1;
    }
    memcpy(buf, prefix, plen);
    size_t nlen = obs_format_hex(buf + plen, hex);
    buf[plen + nlen] = '\0';
    klog_write(buf);
}

static int read_file_from_disk(const char *path, uint8_t *buffer, size_t max_size, size_t *out_size) {
    long fd = sys_call(SYS_open, (long)path, O_RDONLY, 0, 0, 0, 0);
    if (fd < 0) {
        return -1;
    }

    size_t total = 0;
    while (total < max_size) {
        long n = sys_call(SYS_read, fd, (long)(buffer + total), (long)(max_size - total), 0, 0, 0);
        if (n <= 0) {
            break;
        }
        total += (size_t)n;
    }
    sys_call(SYS_close, fd, 0, 0, 0, 0, 0);

    if (out_size != NULL) {
        *out_size = total;
    }
    return (total > 0) ? 0 : -1;
}

static void build_restore_trampoline(uint8_t *code, size_t *code_len, const struct reg *r) {
    size_t idx = 0;
    #define EMIT_MOVABS(reg_prefix, opcode, val) do { \
        code[idx++] = (uint8_t)(reg_prefix); \
        code[idx++] = (uint8_t)(opcode); \
        uint64_t v = (uint64_t)(val); \
        memcpy(&code[idx], &v, 8); \
        idx += 8; \
    } while (0)

    EMIT_MOVABS(0x48, 0xb8, r->r_rax);
    EMIT_MOVABS(0x48, 0xbb, r->r_rbx);
    EMIT_MOVABS(0x48, 0xb9, r->r_rcx);
    EMIT_MOVABS(0x48, 0xba, r->r_rdx);
    EMIT_MOVABS(0x48, 0xbe, r->r_rsi);
    EMIT_MOVABS(0x48, 0xbf, r->r_rdi);
    EMIT_MOVABS(0x48, 0xbd, r->r_rbp);
    EMIT_MOVABS(0x49, 0xb8, r->r_r8);
    EMIT_MOVABS(0x49, 0xb9, r->r_r9);
    EMIT_MOVABS(0x49, 0xba, r->r_r10);
    EMIT_MOVABS(0x49, 0xbb, r->r_r11);
    EMIT_MOVABS(0x49, 0xbc, r->r_r12);
    EMIT_MOVABS(0x49, 0xbd, r->r_r13);
    EMIT_MOVABS(0x49, 0xbe, r->r_r14);
    EMIT_MOVABS(0x49, 0xbf, r->r_r15);
    EMIT_MOVABS(0x48, 0xbc, r->r_rsp);

    /* pushq $imm32_low : 68 [4 bytes] */
    code[idx++] = 0x68;
    uint32_t rip_low = (uint32_t)(r->r_rip & 0xFFFFFFFF);
    memcpy(&code[idx], &rip_low, 4);
    idx += 4;

    /* movl $imm32_high, 0x4(%rsp) : c7 44 24 04 [4 bytes] */
    code[idx++] = 0xc7;
    code[idx++] = 0x44;
    code[idx++] = 0x24;
    code[idx++] = 0x04;
    uint32_t rip_high = (uint32_t)((r->r_rip >> 32) & 0xFFFFFFFF);
    memcpy(&code[idx], &rip_high, 4);
    idx += 4;

    /* ret : c3 */
    code[idx++] = 0xc3;

    #undef EMIT_MOVABS
    *code_len = idx;
}

/**
 * Main injector entry point.
 */
int injector_start(payload_args_t *args) {
    if (args == NULL) {
        injector_exit(-1);
    }

    /* 0. Initialize Syscall Trampoline (avoids PPRBUG direct-syscall traps) */
    sys_call_init(args);

    klog_write("starting obscene-injector payload...");
    klog_write_hex("payload_args kpipe_addr=", (uint64_t)args->kpipe_addr);
    klog_write_hex("payload_args kdata_base=", (uint64_t)args->kdata_base_addr);

    /* 1. Initialize Kernel R/W */
    if (krw_init(args) != 0) {
        klog_write("ERROR: krw_init failed");
        injector_exit(-2);
    }

    klog_write_hex("krw initialized, fw=", krw_fw_version());
    klog_write_hex("kernel data base=", krw_kdata_base());

    /* 2. Elevate credentials to grant ptrace privileges and lift syscall restriction */
    if (krw_elevate_current_process() != 0) {
        klog_write("ERROR: krw_elevate_current_process failed");
        injector_exit(-3);
    }
    klog_write("process credentials elevated (authid=0x4800000000010003)");

    /* 3. Resolve target process */
    klog_write("resolving target process...");
    pid_t target_pid = target_resolve(NULL);
    if (target_pid <= 0) {
        klog_write("ERROR: no running game process found (launch a retail game first)");
        krw_restore_current_process();
        injector_exit(-4);
    }
    klog_write_num("resolved target pid: ", (int64_t)target_pid);

    /* 4. Elevate target credentials and synchronize ucred before ptrace attach */
    klog_write("elevating target process credentials...");
    if (krw_elevate_process(target_pid) != 0) {
        klog_write("WARNING: krw_elevate_process failed");
    }
    if (krw_swap_ucred(target_pid) != 0) {
        klog_write("WARNING: krw_swap_ucred failed");
    } else {
        klog_write("ucred synchronized with target process");
    }

    /* 5. Determine payload data source */
    const uint8_t *payload_data = NULL;
    size_t payload_size = 0;

    static uint8_t disk_buffer[0x200000]; /* 2 MiB staging buffer */

    size_t embedded_size = (size_t)((uintptr_t)__obscene_payload_end - (uintptr_t)__obscene_payload_start);
    if (embedded_size > 0) {
        payload_data = __obscene_payload_start;
        payload_size = embedded_size;
        klog_write_num("using embedded payload blob, size: ", (int64_t)payload_size);
    } else {
        /* Try reading from /data/obscene-payload.elf, /data/obscene.elf, or /data/payload.elf */
        klog_write("embedded blob absent, trying /data/ paths...");
        if (read_file_from_disk("/data/obscene-payload.elf", disk_buffer, sizeof(disk_buffer), &payload_size) == 0 ||
            read_file_from_disk("/data/obscene.elf", disk_buffer, sizeof(disk_buffer), &payload_size) == 0 ||
            read_file_from_disk("/data/payload.elf", disk_buffer, sizeof(disk_buffer), &payload_size) == 0) {
            payload_data = disk_buffer;
            klog_write_num("loaded payload from disk, size: ", (int64_t)payload_size);
        }
    }

    if (payload_data == NULL || payload_size == 0) {
        klog_write("ERROR: no payload binary found");
        krw_restore_ucred();
        krw_restore_current_process();
        injector_exit(-5);
    }

    /* 6. Attach to target process */
    klog_write_num("attaching ptrace to pid ", (int64_t)target_pid);
    if (procctl_attach(target_pid) != 0) {
        klog_write("ERROR: procctl_attach failed");
        krw_restore_ucred();
        krw_restore_current_process();
        injector_exit(-6);
    }
    klog_write("ptrace attach success (target stopped)");

    /* 6. Read target thread registers */
    struct reg bak_reg;
    if (procctl_getregs(target_pid, &bak_reg) != 0) {
        klog_write("ERROR: procctl_getregs failed");
        procctl_detach(target_pid, 0);
        krw_restore_current_process();
        injector_exit(-7);
    }
    klog_write_hex("target thread RIP=", bak_reg.r_rip);
    klog_write_hex("target thread RSP=", bak_reg.r_rsp);
    klog_write_hex("target thread RBP=", bak_reg.r_rbp);
    klog_write_hex("target thread RAX=", bak_reg.r_rax);
    klog_write_hex("target thread RFLAGS=", bak_reg.r_rflags);

    /* 7. Resolve target libkernel base and setup remote syscall gadget */
    uintptr_t target_kproc = krw_get_proc(target_pid);
    uintptr_t target_libkernel_base = krw_find_target_libkernel_base(target_kproc, (uintptr_t)bak_reg.r_rip);
    klog_write_hex("target libkernel_base=", target_libkernel_base);
    procctl_find_syscall_gadget(target_pid, target_libkernel_base);
    /* 8. Load ELF into target process address space */
    klog_write("mapping ELF segments into target process...");
    uintptr_t target_base = 0;
    size_t target_size = 0;
    uintptr_t entry_addr = loader_load_into_proc(target_pid, payload_data, payload_size,
                                                 target_libkernel_base,
                                                 &target_base, &target_size);
    if (entry_addr == 0) {
        klog_write("ERROR: loader_load_into_proc failed");
        procctl_detach(target_pid, 0);
        krw_restore_current_process();
        injector_exit(-8);
    }
    klog_write_hex("payload mapped, base=", target_base);
    klog_write_hex("payload mapped, entry=", entry_addr);



    /* 9. Setup dedicated stack + payload_args in target process memory */
    uintptr_t alloc_remote = procctl_remote_mmap(
        target_pid, 0, 0x40000,
        PROC_PROT_READ | PROC_PROT_WRITE | PROC_PROT_EXEC,
        PROC_MAP_ANONYMOUS | PROC_MAP_PRIVATE,
        -1, 0
    );

    if (alloc_remote == 0) {
        klog_write("ERROR: remote_mmap for payload stack failed");
        procctl_detach(target_pid, 0);
        krw_restore_current_process();
        injector_exit(-8);
    }

    uintptr_t args_remote = alloc_remote;
    payload_args_t target_args;
    memset(&target_args, 0, sizeof(target_args));
    if (target_libkernel_base != 0) {
        target_args.sys_dynlib_dlsym = (int (*)(int, const char *, void *))(target_libkernel_base + 0x5b0);
    }
    target_args.kpipe_addr = args->kpipe_addr;
    target_args.kdata_base_addr = args->kdata_base_addr;
    procctl_copyin(target_pid, &target_args, args_remote, sizeof(target_args));
    klog_write_hex("payload_args staged at remote ", args_remote);
    klog_write_hex("staged getpid at ", (uintptr_t)target_args.sys_dynlib_dlsym);

    /* Dedicated stack top (16-byte aligned) */
    uintptr_t stack_top = (alloc_remote + 0x40000 - 0x200) & ~0xFULL;
    uintptr_t tramp_remote = stack_top + 0x80;

    /* Build and stage the register-restoration trampoline */
    uint8_t tramp_code[256];
    size_t tramp_len = 0;
    build_restore_trampoline(tramp_code, &tramp_len, &bak_reg);
    procctl_copyin(target_pid, tramp_code, tramp_remote, tramp_len);
    klog_write_hex("restore trampoline staged at ", tramp_remote);

    /* 10. Hijack target thread */
    struct reg jmp_reg;
    memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));

    /* Push trampoline address onto dedicated stack as return address */
    jmp_reg.r_rsp = stack_top - 8;
    procctl_setlong(target_pid, jmp_reg.r_rsp, tramp_remote);

    jmp_reg.r_rip = entry_addr;
    jmp_reg.r_rdi = args_remote; /* First argument (SysV ABI): payload_args_t *args */

    klog_write_hex("hijacking thread: new RIP=", jmp_reg.r_rip);
    klog_write_hex("hijacking thread: new RSP=", jmp_reg.r_rsp);
    klog_write_hex("hijacking thread: new RDI=", jmp_reg.r_rdi);

    if (procctl_setregs(target_pid, &jmp_reg) != 0) {
        klog_write("ERROR: procctl_setregs failed");
        procctl_detach(target_pid, 0);
        krw_restore_current_process();
        injector_exit(-9);
    }

    /* Verify hijacked registers were written */
    struct reg verify_reg;
    if (procctl_getregs(target_pid, &verify_reg) == 0) {
        klog_write_hex("verified target RIP=", verify_reg.r_rip);
        klog_write_hex("verified target RSP=", verify_reg.r_rsp);
        klog_write_hex("verified target RDI=", verify_reg.r_rdi);
    }

    klog_write("registers set, resuming target...");

    /* 10. Detach while STILL ELEVATED to resume target */
    int detach_ret = procctl_detach(target_pid, 0);
    klog_write_num("detach return=", (int64_t)detach_ret);
    klog_write("injection complete! target process running obSCEne.");

    /* 11. Now restore injector credentials */
    krw_restore_current_process();

    injector_exit(0);
}
