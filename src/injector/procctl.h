/*
 * Process Control Interface.
 *
 * ptrace-style process control (attach, get/set regs, single-step, continue,
 * memory inspection/modification, remote syscalls).
 */

#ifndef OBSCENE_INJECTOR_PROCCTL_H
#define OBSCENE_INJECTOR_PROCCTL_H

#include <stddef.h>
#include <stdint.h>
#include "common/freestd.h"

/**
 * FreeBSD x86_64 register layout (pinned to <machine/reg.h>).
 */
struct reg {
    uint64_t r_r15;
    uint64_t r_r14;
    uint64_t r_r13;
    uint64_t r_r12;
    uint64_t r_r11;
    uint64_t r_r10;
    uint64_t r_r9;
    uint64_t r_r8;
    uint64_t r_rdi;
    uint64_t r_rsi;
    uint64_t r_rbp;
    uint64_t r_rbx;
    uint64_t r_rdx;
    uint64_t r_rcx;
    uint64_t r_rax;
    uint32_t r_trapno;
    uint16_t r_fs;
    uint16_t r_gs;
    uint32_t r_err;
    uint16_t r_es;
    uint16_t r_ds;
    uint64_t r_rip;
    uint64_t r_cs;
    uint64_t r_rflags;
    uint64_t r_rsp;
    uint64_t r_ss;
};

/**
 * Standard mmap / protection flags
 */
#define PROC_PROT_NONE 0x00
#define PROC_PROT_READ 0x01
#define PROC_PROT_WRITE 0x02
#define PROC_PROT_EXEC 0x04

#define PROC_MAP_SHARED 0x0001
#define PROC_MAP_PRIVATE 0x0002
#define PROC_MAP_FIXED 0x0010
#define PROC_MAP_ANONYMOUS 0x1000

/**
 * Attach / detach / control execution flow.
 */
int procctl_attach(pid_t pid);
int procctl_detach(pid_t pid, int sig);
int procctl_step(pid_t pid);
int procctl_continue(pid_t pid, int sig);

/**
 * Register state.
 */
int procctl_getregs(pid_t pid, struct reg *r);
int procctl_setregs(pid_t pid, const struct reg *r);

/**
 * Remote memory read/write.
 */
int procctl_copyin(pid_t pid, const void *src, uintptr_t dst_addr, size_t len);
int procctl_copyout(pid_t pid, uintptr_t src_addr, void *dst, size_t len);
int procctl_setlong(pid_t pid, uintptr_t addr, uint64_t val);
uint64_t procctl_getlong(pid_t pid, uintptr_t addr);
int procctl_setint(pid_t pid, uintptr_t addr, uint32_t val);
uint32_t procctl_getint(pid_t pid, uintptr_t addr);

/**
 * Remote address space management.
 */
uintptr_t procctl_remote_mmap(pid_t pid, uintptr_t addr, size_t len, int prot,
                              int flags, int fd, off_t off);
int procctl_remote_munmap(pid_t pid, uintptr_t addr, size_t len);
int procctl_remote_mprotect(pid_t pid, uintptr_t addr, size_t len, int prot);

/**
 * Remote syscall execution.
 */
long procctl_remote_syscall(pid_t pid, int sysno, uint64_t a1, uint64_t a2, uint64_t a3,
                            uint64_t a4, uint64_t a5, uint64_t a6);
void procctl_set_syscall_gadget(uintptr_t gadget);
uintptr_t procctl_find_syscall_gadget(pid_t pid, uintptr_t libkernel_base);

#endif /* OBSCENE_INJECTOR_PROCCTL_H */
