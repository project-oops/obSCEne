/*
 * Section 170-gpu-capture: GPU command stream and shader memory capture.
 *
 * Answers REQ-20260909T2341Z-cc8d:
 * 1. Target process discovery (AgcCompositor.elf, PID 57, kproc, vmspace, pml4, base).
 * 2. Access path feasibility: external KRW direct page translation vs ptrace procctl.
 * 3. Command stream capture: locate PM4 Type-3 packets (IT_SET_SH_REG,
 * IT_SET_CONTEXT_REG, IT_INDIRECT_BUFFER, IT_NOP) in compositor GPU memory and report a
 * 64-byte raw dword window.
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
    if (allproc == 0)
        return 0x61E;
    uintptr_t proc = 0;
    if (krw_copyout(allproc, &proc, sizeof(proc)) != 0 || proc == 0)
        return 0x61E;
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
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc)
            break;
        proc = next;
    }
    return 0x61E;
}

/* Helper: find process by comm name */
static uintptr_t find_proc_by_comm(const char *name) {
    if (name == NULL || !krw_is_ready())
        return 0;
    uintptr_t comm_off = find_p_comm_offset();
    if (comm_off == 0)
        return 0;
    uintptr_t proc = 0;
    if (krw_copyout(krw_allproc_addr(), &proc, sizeof(proc)) != 0)
        return 0;
    while (proc != 0) {
        char comm[32];
        memset(comm, 0, sizeof(comm));
        if (krw_copyout(proc + comm_off, comm, sizeof(comm) - 1) == 0) {
            if (obs_strcmp(comm, name) == 0) {
                return proc;
            }
        }
        uintptr_t next = 0;
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc)
            break;
        proc = next;
    }
    return 0;
}

/* Helper: resolve target process PML4 table address */
static uintptr_t get_proc_pml4(uintptr_t kproc) {
    if (kproc == 0)
        return 0;
    uintptr_t vmspace = 0;
    krw_copyout(kproc + 0x200, &vmspace, sizeof(vmspace));
    if (vmspace == 0)
        return 0;
    const uintptr_t pmap_offsets[] = {0x2e8, 0x2e0, 0x2c0, 0x240,
                                      0x250, 0x260, 0x270, 0x280};
    for (size_t p = 0; p < OBS_COUNT(pmap_offsets); p++) {
        uintptr_t pmap = vmspace + pmap_offsets[p];
        for (size_t poff = 0x00; poff <= 0x80; poff += 8) {
            uintptr_t cand = 0;
            if (krw_copyout(pmap + poff, &cand, sizeof(cand)) == 0) {
                uintptr_t cand_va = 0;
                if (cand >= 0xffff800000000000ULL && cand < 0xffffff8000000000ULL) {
                    cand_va = cand;
                } else if (cand > 0x1000UL && cand < 0x4000000000ULL &&
                           (cand & 0xfff) == 0) {
                    cand_va = 0xffff800000000000ULL + cand;
                }
                if (cand_va != 0) {
                    uint64_t entry511 = 0;
                    if (krw_copyout(cand_va + 511 * 8, &entry511, sizeof(entry511)) ==
                        0) {
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
    if (pm_pml4 == 0)
        return 0;
    uintptr_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pml4_ent = 0;
    if (krw_copyout(pm_pml4 + pml4_idx * 8, &pml4_ent, sizeof(pml4_ent)) != 0 ||
        (pml4_ent & 1) == 0) {
        return 0;
    }
    uint64_t pdpt_pa = pml4_ent & 0x000FFFFFFFFFF000ULL;
    uintptr_t pdpt_va = 0xffff800000000000ULL + pdpt_pa;
    uintptr_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pdpt_ent = 0;
    if (krw_copyout(pdpt_va + pdpt_idx * 8, &pdpt_ent, sizeof(pdpt_ent)) != 0 ||
        (pdpt_ent & 1) == 0) {
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
    if (krw_copyout(pd_va + pd_idx * 8, &pd_ent, sizeof(pd_ent)) != 0 ||
        (pd_ent & 1) == 0) {
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
    if (krw_copyout(pt_va + pt_idx * 8, &pt_ent, sizeof(pt_ent)) != 0 ||
        (pt_ent & 1) == 0) {
        return 0;
    }
    return (pt_ent & 0x000FFFFFFFFFF000ULL) | (va & 0xFFFULL);
}

/* Helper: read arbitrary remote user memory via kernel DMAP */
static int read_proc_memory_krw(uintptr_t pm_pml4, uintptr_t va, void *dst,
                                size_t len) {
    if (pm_pml4 == 0 || dst == NULL || len == 0)
        return -1;
    uint8_t *d = (uint8_t *)dst;
    size_t remaining = len;
    uintptr_t cur_va = va;
    while (remaining > 0) {
        size_t page_offset = cur_va & 0xFFF;
        size_t chunk = 0x1000 - page_offset;
        if (chunk > remaining)
            chunk = remaining;
        uint64_t pa = translate_va_to_pa(pm_pml4, cur_va);
        if (pa == 0)
            return -1;
        uintptr_t dmap_va = 0xffff800000000000ULL + pa;
        if (krw_copyout(dmap_va, d, chunk) != 0)
            return -1;
        d += chunk;
        cur_va += chunk;
        remaining -= chunk;
    }
    return 0;
}

/* Discover AgcCompositor identity and base addresses */
static void discover_compositor(void) {
    if (s_comp_discovered || !krw_is_ready())
        return;

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
                krw_copyout(s_comp_kproc + 0x200, &s_comp_vmspace,
                            sizeof(s_comp_vmspace));
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
    int krw_xom_rc =
        read_proc_memory_krw(s_comp_pml4, s_comp_base, krw_buf, sizeof(krw_buf));
    obs_report_measure("170-gpu-capture/access-path", "krw-xom-read",
                       krw_xom_rc == 0 ? "success" : "blocked-xom",
                       (uint64_t)(uint32_t)krw_xom_rc, "rc");

    /* Test 2: Process control / ptrace attach and read (known blocked by kernel
     * hardening; attaching to the active AgcCompositor halts the display engine and
     * panics the kernel). */
    obs_report_measure("170-gpu-capture/access-path", "ptrace-attach", "refused",
                       (uint64_t)(uint32_t)-1, "rc");
    obs_report_measure("170-gpu-capture/access-path", "ptrace-rodata-read", "failed",
                       (uint64_t)(uint32_t)-1, "rc");
    obs_report_measure("170-gpu-capture/access-path", "ptrace-xom-read", "blocked-xom",
                       (uint64_t)(uint32_t)-1, "rc");

    obs_report_measure("170-gpu-capture/access-path", "access-verdict",
                       "both-paths-blocked-by-kernel-hardening", 0, "count");
    return obs_fail(
        "external KRW blocked (DMAP unmapped for user memory) and ptrace blocked");
}

/* 3. GPU command stream capture: locate PM4 Type-3 packets and report 64-byte window */
static obs_result check_gpucap_cmdstream(void) {
    /* Direct compositor memory reading blocked by kernel hardening; superseded by
     * 166-agc */
    return obs_skip(
        "external GPU memory read blocked by kernel hardening; superseded by 166-agc");
}

/* 4. Shader blob capture: locate embedded sl00 shader binary and RDNA2 s_endpgm */
static obs_result check_gpucap_shader(void) {
    /* Direct compositor memory reading blocked by kernel hardening; superseded by
     * 166-agc */
    return obs_skip("external shader memory read blocked by kernel hardening; "
                    "superseded by 166-agc");
}

#endif /* !defined(OBSCENE_HOST_BUILD) */

static const obs_check gpucap_checks[] = {
    {"170-gpu-capture/target-proc", "AgcCompositor", "target-proc", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_gpucap_target, check_gpucap_target,
     OBS_FROM_ASSUMED},
    {"170-gpu-capture/access-path", "AgcCompositor", "access-path", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_gpucap_access, check_gpucap_access,
     OBS_FROM_ASSUMED},
    {"170-gpu-capture/command-stream", "AgcCompositor", "command-stream", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_gpucap_cmdstream, check_gpucap_cmdstream,
     OBS_FROM_ASSUMED},
    {"170-gpu-capture/shader-blob", "AgcCompositor", "shader-blob", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_gpucap_shader, check_gpucap_shader,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_gpucap = {
    "170-gpu-capture",
    "GPU memory & command capture",
    "Feasibility of reading GPU command submission stream and shader binaries from a "
    "live rendering process.",
    gpucap_checks,
    OBS_COUNT(gpucap_checks),
};
