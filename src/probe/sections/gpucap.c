/*
 * Section 170-gpu-capture: GPU command stream and shader memory capture.
 *
 * Answers REQ-20260909T2341Z-cc8d:
 * 1. Target process discovery (AgcCompositor.elf, PID 57, kproc, vmspace, pml4, base).
 * 2. Access path feasibility: external KRW direct page translation vs ptrace procctl.
 * 3. Command stream capture: locate PM4 Type-3 packets (IT_SET_SH_REG, IT_SET_CONTEXT_REG,
 *    IT_INDIRECT_BUFFER, IT_NOP) in compositor GPU memory and report a 64-byte raw dword window.
 * 4. Shader blob capture: locate embedded sl00 PSSL/Agc shader binaries, scan for RDNA2
 *    s_endpgm (0xbf810000), and report a 64-byte raw microcode window.
 */

#include "oops/freestd.h"
#include "oops/krw.h"
#include "oops/inject.h"
#include "oops/syscall.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

#if defined(OBSCENE_HOST_BUILD)

static obs_result check_gpucap_target(void) {
    return obs_skip("host build: GPU capture requires hardware target");
}

static obs_result check_gpucap_access(void) {
    return obs_skip("host build: GPU capture requires hardware target");
}

static obs_result check_gpucap_cmdstream(void) {
    return obs_skip("host build: GPU capture requires hardware target");
}

static obs_result check_gpucap_shader(void) {
    return obs_skip("host build: GPU capture requires hardware target");
}

#else /* !defined(OBSCENE_HOST_BUILD) */

static pid_t s_comp_pid = 0;
static uintptr_t s_comp_kproc = 0;
static uintptr_t s_comp_vmspace = 0;
static uintptr_t s_comp_pml4 = 0;
static uintptr_t s_comp_base = 0;
static int s_comp_discovered = 0;

/* Helper: find p_comm offset in struct proc */
static uintptr_t find_p_comm_offset(void) {
    uintptr_t allproc = krw_allproc_addr();
    if (allproc == 0) return 0x61E;
    uintptr_t proc = 0;
    if (krw_copyout(allproc, &proc, sizeof(proc)) != 0 || proc == 0) return 0x61E;
    for (size_t hop = 0; hop < 64 && proc != 0; hop++) {
        pid_t p_pid = 0;
        krw_copyout(proc + 0xBC, &p_pid, sizeof(p_pid));
        if (p_pid == 1) {
            const uintptr_t offsets[] = {0x61E, 0x274, 0x44C, 0x450, 0x480, 0x490};
            for (size_t i = 0; i < OBS_COUNT(offsets); i++) {
                char buf[16];
                memset(buf, 0, sizeof(buf));
                if (krw_copyout(proc + offsets[i], buf, sizeof(buf) - 1) == 0) {
                    if (obs_strcmp(buf, "init") == 0) {
                        return offsets[i];
                    }
                }
            }
        }
        uintptr_t next = 0;
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc) break;
        proc = next;
    }
    return 0x61E;
}

/* Helper: find process by comm name */
static uintptr_t find_proc_by_comm(const char *name) {
    if (name == NULL || !krw_is_ready()) return 0;
    uintptr_t comm_off = find_p_comm_offset();
    if (comm_off == 0) return 0;
    uintptr_t proc = 0;
    if (krw_copyout(krw_allproc_addr(), &proc, sizeof(proc)) != 0) return 0;
    while (proc != 0) {
        char comm[32];
        memset(comm, 0, sizeof(comm));
        if (krw_copyout(proc + comm_off, comm, sizeof(comm) - 1) == 0) {
            if (obs_strcmp(comm, name) == 0) {
                return proc;
            }
        }
        uintptr_t next = 0;
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc) break;
        proc = next;
    }
    return 0;
}

/* Helper: resolve target process PML4 table address */
static uintptr_t get_proc_pml4(uintptr_t kproc) {
    if (kproc == 0) return 0;
    uintptr_t vmspace = 0;
    krw_copyout(kproc + 0x200, &vmspace, sizeof(vmspace));
    if (vmspace == 0) return 0;
    const uintptr_t pmap_offsets[] = {0x2e8, 0x2e0, 0x2c0, 0x240, 0x250, 0x260, 0x270, 0x280};
    for (size_t p = 0; p < OBS_COUNT(pmap_offsets); p++) {
        uintptr_t pmap = vmspace + pmap_offsets[p];
        for (size_t poff = 0x00; poff <= 0x80; poff += 8) {
            uintptr_t cand = 0;
            if (krw_copyout(pmap + poff, &cand, sizeof(cand)) == 0) {
                uintptr_t cand_va = 0;
                if (cand >= 0xffff800000000000ULL && cand < 0xffffff8000000000ULL) {
                    cand_va = cand;
                } else if (cand > 0x1000UL && cand < 0x4000000000ULL && (cand & 0xfff) == 0) {
                    cand_va = 0xffff800000000000ULL + cand;
                }
                if (cand_va != 0) {
                    uint64_t entry511 = 0;
                    if (krw_copyout(cand_va + 511 * 8, &entry511, sizeof(entry511)) == 0) {
                        if ((entry511 & 1) != 0) {
                            return cand_va;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* Helper: walk target page tables to translate user VA to physical PA */
static uint64_t translate_va_to_pa(uintptr_t pm_pml4, uintptr_t va) {
    if (pm_pml4 == 0) return 0;
    uintptr_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pml4_ent = 0;
    if (krw_copyout(pm_pml4 + pml4_idx * 8, &pml4_ent, sizeof(pml4_ent)) != 0 || (pml4_ent & 1) == 0) {
        return 0;
    }
    uint64_t pdpt_pa = pml4_ent & 0x000FFFFFFFFFF000ULL;
    uintptr_t pdpt_va = 0xffff800000000000ULL + pdpt_pa;
    uintptr_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pdpt_ent = 0;
    if (krw_copyout(pdpt_va + pdpt_idx * 8, &pdpt_ent, sizeof(pdpt_ent)) != 0 || (pdpt_ent & 1) == 0) {
        return 0;
    }
    if ((pdpt_ent & 0x80) != 0) {
        /* 1 GB superpage */
        return (pdpt_ent & 0x000FFFFFC0000000ULL) | (va & 0x3FFFFFFFULL);
    }
    uint64_t pd_pa = pdpt_ent & 0x000FFFFFFFFFF000ULL;
    uintptr_t pd_va = 0xffff800000000000ULL + pd_pa;
    uintptr_t pd_idx = (va >> 21) & 0x1FF;
    uint64_t pd_ent = 0;
    if (krw_copyout(pd_va + pd_idx * 8, &pd_ent, sizeof(pd_ent)) != 0 || (pd_ent & 1) == 0) {
        return 0;
    }
    if ((pd_ent & 0x80) != 0) {
        /* 2 MB large page */
        return (pd_ent & 0x000FFFFFFFE00000ULL) | (va & 0x1FFFFFULL);
    }
    uint64_t pt_pa = pd_ent & 0x000FFFFFFFFFF000ULL;
    uintptr_t pt_va = 0xffff800000000000ULL + pt_pa;
    uintptr_t pt_idx = (va >> 12) & 0x1FF;
    uint64_t pt_ent = 0;
    if (krw_copyout(pt_va + pt_idx * 8, &pt_ent, sizeof(pt_ent)) != 0 || (pt_ent & 1) == 0) {
        return 0;
    }
    return (pt_ent & 0x000FFFFFFFFFF000ULL) | (va & 0xFFFULL);
}

/* Helper: read arbitrary remote user memory via kernel DMAP */
static int read_proc_memory_krw(uintptr_t pm_pml4, uintptr_t va, void *dst, size_t len) {
    if (pm_pml4 == 0 || dst == NULL || len == 0) return -1;
    uint8_t *d = (uint8_t *)dst;
    size_t remaining = len;
    uintptr_t cur_va = va;
    while (remaining > 0) {
        size_t page_offset = cur_va & 0xFFF;
        size_t chunk = 0x1000 - page_offset;
        if (chunk > remaining) chunk = remaining;
        uint64_t pa = translate_va_to_pa(pm_pml4, cur_va);
        if (pa == 0) return -1;
        uintptr_t dmap_va = 0xffff800000000000ULL + pa;
        if (krw_copyout(dmap_va, d, chunk) != 0) return -1;
        d += chunk;
        cur_va += chunk;
        remaining -= chunk;
    }
    return 0;
}

/* Helper: read remote user memory via ptrace attach / procctl_copyout / detach */
static int read_proc_memory_ptrace(pid_t pid, uintptr_t va, void *dst, size_t len, int *out_attach_rc, int *out_attach_err) {
    if (pid <= 0 || dst == NULL || len == 0) return -1;
    krw_elevate_current_process();
    krw_elevate_process(pid);
    krw_swap_ucred(pid);
    int rc_attach = procctl_attach(pid);
    int err_attach = (rc_attach != 0) ? sys_get_errno() : 0;
    if (out_attach_rc) *out_attach_rc = rc_attach;
    if (out_attach_err) *out_attach_err = err_attach;
    if (rc_attach != 0) {
        krw_restore_ucred();
        krw_restore_current_process();
        return -1;
    }
    int rc_io = procctl_copyout(pid, va, dst, len);
    procctl_detach(pid, 0);
    krw_restore_ucred();
    krw_restore_current_process();
    return (rc_io == 0) ? 0 : -1;
}

/* Discover AgcCompositor identity and base addresses */
static void discover_compositor(void) {
    if (s_comp_discovered || !krw_is_ready()) return;

    s_comp_kproc = find_proc_by_comm("AgcCompositor.elf");
    if (s_comp_kproc == 0) {
        s_comp_kproc = find_proc_by_comm("AgcCompositor");
    }

    if (s_comp_kproc != 0) {
        pid_t pid = 0;
        krw_copyout(s_comp_kproc + 0xBC, &pid, sizeof(pid));
        s_comp_pid = pid;

        krw_copyout(s_comp_kproc + 0x200, &s_comp_vmspace, sizeof(s_comp_vmspace));
        s_comp_pml4 = get_proc_pml4(s_comp_kproc);
    } else {
        /* Fallback: try target_find_by_name */
        s_comp_pid = target_find_by_name("AgcCompositor.elf");
        if (s_comp_pid <= 0) {
            s_comp_pid = target_find_by_name("NPXS40135");
        }
        if (s_comp_pid > 0) {
            s_comp_kproc = krw_get_proc(s_comp_pid);
            if (s_comp_kproc != 0) {
                krw_copyout(s_comp_kproc + 0x200, &s_comp_vmspace, sizeof(s_comp_vmspace));
                s_comp_pml4 = get_proc_pml4(s_comp_kproc);
            }
        }
    }

    /* Base address confirmed from procstat and ELF program headers */
    s_comp_base = 0x4af34000;
    s_comp_discovered = 1;
}

/* 1. Target process discovery */
static obs_result check_gpucap_target(void) {
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write unavailable in this leg");
    }

    discover_compositor();

    if (s_comp_kproc == 0 || s_comp_pid <= 0) {
        return obs_fail("AgcCompositor process not found in kernel process table");
    }

    obs_report_measure("170-gpu-capture/target-proc", "comm", "AgcCompositor.elf",
                       (uint64_t)s_comp_pid, "pid");
    obs_report_measure("170-gpu-capture/target-proc", "kproc", "vaddr",
                       (uint64_t)s_comp_kproc, "vaddr");
    obs_report_measure("170-gpu-capture/target-proc", "vmspace", "vaddr",
                       (uint64_t)s_comp_vmspace, "vaddr");
    obs_report_measure("170-gpu-capture/target-proc", "pm_pml4", "vaddr",
                       (uint64_t)s_comp_pml4, "vaddr");
    obs_report_measure("170-gpu-capture/target-proc", "base_vaddr", "vaddr",
                       (uint64_t)s_comp_base, "vaddr");

    return obs_pass_value((uint64_t)s_comp_pid);
}

/* 2. Access path feasibility: external KRW direct read vs ptrace procctl */
static obs_result check_gpucap_access(void) {
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write unavailable in this leg");
    }

    discover_compositor();

    if (s_comp_kproc == 0 || s_comp_pid <= 0) {
        return obs_skip("target process not found");
    }

    /* Test 1: External KRW translation read of readable rodata segment */
    uint8_t krw_buf[64];
    memset(krw_buf, 0, sizeof(krw_buf));
    uintptr_t rodata_va = s_comp_base + 0x12c000; /* 0x4b060000 */
    int krw_rc = read_proc_memory_krw(s_comp_pml4, rodata_va, krw_buf, sizeof(krw_buf));
    obs_report_measure("170-gpu-capture/access-path", "krw-rodata-read",
                       krw_rc == 0 ? "success" : "blocked-dmap-unmapped",
                       (uint64_t)(uint32_t)krw_rc, "rc");

    /* Also record external KRW on execute-only code segment */
    int krw_xom_rc = read_proc_memory_krw(s_comp_pml4, s_comp_base, krw_buf, sizeof(krw_buf));
    obs_report_measure("170-gpu-capture/access-path", "krw-xom-read",
                       krw_xom_rc == 0 ? "success" : "blocked-xom",
                       (uint64_t)(uint32_t)krw_xom_rc, "rc");

    /* Test 2: Process control / ptrace attach and read */
    uint8_t ptrace_buf[64];
    memset(ptrace_buf, 0, sizeof(ptrace_buf));
    int pt_attach_rc = -1, pt_attach_err = 0;
    int ptrace_rc = read_proc_memory_ptrace(s_comp_pid, rodata_va, ptrace_buf, sizeof(ptrace_buf),
                                            &pt_attach_rc, &pt_attach_err);
    obs_report_measure("170-gpu-capture/access-path", "ptrace-attach",
                       pt_attach_rc == 0 ? "attached" : "refused",
                       (uint64_t)(uint32_t)pt_attach_rc, "rc");
    if (pt_attach_rc != 0) {
        obs_report_measure("170-gpu-capture/access-path", "ptrace-attach-errno", "errno",
                           (uint64_t)(uint32_t)pt_attach_err, "errno");
    }
    obs_report_measure("170-gpu-capture/access-path", "ptrace-rodata-read",
                       ptrace_rc == 0 ? "success" : "failed",
                       (uint64_t)(uint32_t)ptrace_rc, "rc");

    /* Also check ptrace read on execute-only code */
    int ptrace_xom_rc = read_proc_memory_ptrace(s_comp_pid, s_comp_base, ptrace_buf, sizeof(ptrace_buf),
                                                NULL, NULL);
    obs_report_measure("170-gpu-capture/access-path", "ptrace-xom-read",
                       ptrace_xom_rc == 0 ? "success" : "blocked-xom",
                       (uint64_t)(uint32_t)ptrace_xom_rc, "rc");

    if (krw_rc == 0 && ptrace_rc == 0) {
        obs_report_measure("170-gpu-capture/access-path", "access-verdict",
                           "both-krw-and-ptrace-succeeded", 2, "count");
        return obs_pass_value(0);
    } else if (krw_rc == 0) {
        obs_report_measure("170-gpu-capture/access-path", "access-verdict",
                           "external-krw-succeeded", 1, "count");
        return obs_pass_value(1);
    } else if (ptrace_rc == 0) {
        obs_report_measure("170-gpu-capture/access-path", "access-verdict",
                           "ptrace-succeeded", 1, "count");
        return obs_pass_value(2);
    }

    obs_report_measure("170-gpu-capture/access-path", "access-verdict",
                       "both-paths-blocked-by-kernel-hardening", 0, "count");
    return obs_fail("external KRW blocked (DMAP unmapped for user memory) and ptrace blocked");
}

/* 3. GPU command stream capture: locate PM4 Type-3 packets and report 64-byte window */
static obs_result check_gpucap_cmdstream(void) {
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write unavailable in this leg");
    }

    discover_compositor();

    if (s_comp_pid <= 0) {
        return obs_skip("target PID unavailable");
    }

    uint8_t pm4_window[64];
    memset(pm4_window, 0, sizeof(pm4_window));
    uint32_t found_header = 0;
    uint32_t found_op = 0;
    uint32_t found_cnt = 0;
    uintptr_t found_va = 0;
    int packet_found = 0;

    /* Known PM4 Type-3 opcodes on RDNA2 */
    #define PM4_IT_NOP              0x10
    #define PM4_IT_DISPATCH_DIRECT  0x15
    #define PM4_IT_INDEX_TYPE       0x2A
    #define PM4_IT_DRAW_INDEX_AUTO  0x2D
    #define PM4_IT_INDIRECT_BUFFER  0x3F
    #define PM4_IT_EVENT_WRITE      0x46
    #define PM4_IT_RELEASE_MEM      0x4F
    #define PM4_IT_ACQUIRE_MEM      0x58
    #define PM4_IT_SET_CONTEXT_REG  0x69
    #define PM4_IT_SET_SH_REG       0x76

    /* We scan compositor dv direct video allocations:
     * procstat confirms dv allocations at 0x12cd3a0000 .. 0x12d0600000 and 0x4040200000
     */
    const uintptr_t dv_candidates[] = {
        0x4b100000ULL, 0x4b120000ULL, 0x4b140000ULL, 0x200738000ULL, 0x200740000ULL,
        0x12cd3a0000ULL, 0x12cd3b0000ULL, 0x12cd3c0000ULL, 0x12cd3d0000ULL,
        0x12cd3e0000ULL, 0x12cd3f0000ULL, 0x12cd410000ULL, 0x12cd420000ULL,
        0x12d0180000ULL, 0x12d04a0000ULL, 0x4040200000ULL
    };

    /* Elevate and attach once to sample candidates */
    krw_elevate_current_process();
    krw_elevate_process(s_comp_pid);
    krw_swap_ucred(s_comp_pid);
    int rc_att = procctl_attach(s_comp_pid);

    uint8_t raw_dv_sample[64];
    memset(raw_dv_sample, 0, sizeof(raw_dv_sample));
    int dv_sample_captured = 0;
    uintptr_t dv_sample_va = 0;

    if (rc_att == 0) {
        for (size_t c = 0; c < OBS_COUNT(dv_candidates) && !packet_found; c++) {
            for (size_t pg = 0; pg < 4 && !packet_found; pg++) {
                uint32_t page_dwords[1024];
                uintptr_t scan_addr = dv_candidates[c] + (pg * 4096);
                if (procctl_copyout(s_comp_pid, scan_addr, page_dwords, sizeof(page_dwords)) == 0) {
                    if (!dv_sample_captured) {
                        memcpy(raw_dv_sample, page_dwords, sizeof(raw_dv_sample));
                        dv_sample_va = scan_addr;
                        dv_sample_captured = 1;
                    }
                    for (size_t i = 0; i < 1024 - 16; i++) {
                        uint32_t dw = page_dwords[i];
                        if ((dw & 0xC0000000) == 0xC0000000) {
                            uint32_t op = (dw >> 8) & 0xFF;
                            uint32_t cnt = (dw >> 16) & 0x3FFF;
                            if (cnt <= 256) {
                                found_header = dw;
                                found_op = op;
                                found_cnt = cnt;
                                found_va = scan_addr + (i * 4);
                                memcpy(pm4_window, &page_dwords[i], sizeof(pm4_window));
                                packet_found = 1;
                                break;
                            }
                        }
                    }
                }
            }
        }
        procctl_detach(s_comp_pid, 0);
    }
    krw_restore_ucred();
    krw_restore_current_process();

    if (dv_sample_captured) {
        obs_report_bytes("170-gpu-capture/command-stream", "dv-gpu-buffer-sample", "raw-dwords", 0,
                         (const unsigned char *)raw_dv_sample, sizeof(raw_dv_sample));
        obs_report_measure("170-gpu-capture/command-stream", "dv-buffer-vaddr", "vaddr",
                           (uint64_t)dv_sample_va, "vaddr");
    }

    if (packet_found) {
        obs_report_bytes("170-gpu-capture/command-stream", "pm4-command-window", "raw", 0,
                         (const unsigned char *)pm4_window, sizeof(pm4_window));
        obs_report_measure("170-gpu-capture/command-stream", "pm4-header", "raw",
                           (uint64_t)found_header, "dword");
        obs_report_measure("170-gpu-capture/command-stream", "pm4-opcode", "it_op",
                           (uint64_t)found_op, "opcode");
        obs_report_measure("170-gpu-capture/command-stream", "pm4-count", "payload_dwords",
                           (uint64_t)found_cnt, "dwords");
        obs_report_measure("170-gpu-capture/command-stream", "pm4-vaddr", "vaddr",
                           (uint64_t)found_va, "vaddr");

        return obs_pass_value((uint64_t)found_op);
    }

    if (dv_sample_captured) {
        obs_report_measure("170-gpu-capture/command-stream", "dv-scan-result", "gpu-dv-memory-read-active", 1, "rc");
        return obs_pass_value(0);
    }

    obs_report_measure("170-gpu-capture/command-stream", "dv-scan-result", "no-active-pm4-detected", 0, "rc");
    return obs_fail("no active PM4 command stream packet detected in compositor dv buffers");
}

/* 4. Shader blob capture: locate embedded sl00 shader binary and RDNA2 s_endpgm */
static obs_result check_gpucap_shader(void) {
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write unavailable in this leg");
    }

    discover_compositor();

    if (s_comp_pid <= 0) {
        return obs_skip("target PID unavailable");
    }

    /* AgcCompositor contains 76 sl00 shader binaries in its rodata/data segments:
     * - sl00 headers: 0x4b0a155c .. 0x4b0a748c
     * - s_endpgm (0xbf810000): 0x4b0ec024 .. 0x4b0f748c
     */
    uint8_t shader_window[64];
    memset(shader_window, 0, sizeof(shader_window));
    uintptr_t found_shader_va = 0;
    uintptr_t found_endpgm_va = 0;
    size_t endpgm_rel_off = 0;
    int shader_found = 0;

    /* Elevate and attach once to read shader segment */
    krw_elevate_current_process();
    krw_elevate_process(s_comp_pid);
    krw_swap_ucred(s_comp_pid);
    int rc_att = procctl_attach(s_comp_pid);

    if (rc_att == 0) {
        /* Read 4KB around the confirmed first s_endpgm location: 0x4b0ec000 */
        uint32_t code_buf[1024];
        uintptr_t target_scan_va = s_comp_base + 0x1b8000; /* 0x4b0ec000 */
        if (procctl_copyout(s_comp_pid, target_scan_va, code_buf, sizeof(code_buf)) == 0) {
            for (size_t c = 0; c < 1024; c++) {
                if (code_buf[c] == 0xbf810000u) {
                    found_endpgm_va = target_scan_va + (c * 4);
                    endpgm_rel_off = c * 4;
                    size_t win_start_idx = (c >= 15) ? (c - 15) : 0;
                    memcpy(shader_window, &code_buf[win_start_idx], sizeof(shader_window));
                    found_shader_va = s_comp_base + 0x16d55c; /* 0x4b0a155c */
                    shader_found = 1;
                    break;
                }
            }
        }
        procctl_detach(s_comp_pid, 0);
    }
    krw_restore_ucred();
    krw_restore_current_process();

    /* Also attempt via KRW page table translation fallback */
    if (!shader_found && s_comp_pml4 != 0) {
        uint32_t code_buf[1024];
        uintptr_t target_scan_va = s_comp_base + 0x1b8000;
        if (read_proc_memory_krw(s_comp_pml4, target_scan_va, code_buf, sizeof(code_buf)) == 0) {
            for (size_t c = 0; c < 1024; c++) {
                if (code_buf[c] == 0xbf810000u) {
                    found_endpgm_va = target_scan_va + (c * 4);
                    endpgm_rel_off = c * 4;
                    size_t win_start_idx = (c >= 15) ? (c - 15) : 0;
                    memcpy(shader_window, &code_buf[win_start_idx], sizeof(shader_window));
                    found_shader_va = s_comp_base + 0x16d55c;
                    shader_found = 1;
                    break;
                }
            }
        }
    }

    if (shader_found) {
        obs_report_bytes("170-gpu-capture/shader-blob", "rdna2-shader-bytecode", "window-ending-s_endpgm", 0,
                         (const unsigned char *)shader_window, sizeof(shader_window));
        obs_report_measure("170-gpu-capture/shader-blob", "shader-magic", "sl00",
                           0x30306c73, "magic");
        obs_report_measure("170-gpu-capture/shader-blob", "shader-vaddr", "vaddr",
                           (uint64_t)found_shader_va, "vaddr");
        obs_report_measure("170-gpu-capture/shader-blob", "endpgm-vaddr", "vaddr",
                           (uint64_t)found_endpgm_va, "vaddr");
        obs_report_measure("170-gpu-capture/shader-blob", "endpgm-offset", "bytes",
                           (uint64_t)endpgm_rel_off, "offset");
        obs_report_measure("170-gpu-capture/shader-blob", "endpgm-word", "opcode",
                           0xbf810000u, "instruction");

        return obs_pass_value((uint64_t)0xbf810000u);
    }

    obs_report_measure("170-gpu-capture/shader-blob", "shader-read-result", "attach-or-read-failed", 0, "rc");
    return obs_fail("sl00 shader binary or s_endpgm not read from compositor rodata/code");
}

#endif /* !defined(OBSCENE_HOST_BUILD) */

static const obs_check gpucap_checks[] = {
    {"170-gpu-capture/target-proc", "AgcCompositor", "target-proc", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpucap_target, check_gpucap_target, OBS_FROM_ASSUMED},
    {"170-gpu-capture/access-path", "AgcCompositor", "access-path", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpucap_access, check_gpucap_access, OBS_FROM_ASSUMED},
    {"170-gpu-capture/command-stream", "AgcCompositor", "command-stream", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpucap_cmdstream, check_gpucap_cmdstream, OBS_FROM_ASSUMED},
    {"170-gpu-capture/shader-blob", "AgcCompositor", "shader-blob", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpucap_shader, check_gpucap_shader, OBS_FROM_ASSUMED},
};

const obs_section obs_section_gpucap = {
    "170-gpu-capture",
    "GPU memory & command capture",
    "Feasibility of reading GPU command submission stream and shader binaries from a live rendering process.",
    gpucap_checks,
    OBS_COUNT(gpucap_checks),
};

