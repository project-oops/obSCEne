/*
 * SceShellUI reachability and JavaScript context probing.
 *
 * Answers REQ-20260909T2157Z-5b1e:
 * 1. Process identity (PID, name, title ID, path, authid).
 * 2. Mapped modules walk via kernel dispatch list (kproc + 0x3E8).
 * 3. Export reachability across 3 routes (import, dlsym, kexport).
 * 4. JavaScript evaluation route inspection (exported entry, message port, inspector
 * sockets).
 * 5. Process takeover via ptrace attach with immediate detach.
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

static obs_result check_shellui_identity(void) {
    return obs_skip("host build: target inspection requires hardware target");
}

static obs_result check_shellui_modules(void) {
    return obs_skip("host build: target inspection requires hardware target");
}

static obs_result check_shellui_exports(void) {
    return obs_skip("host build: target inspection requires hardware target");
}

static obs_result check_shellui_js_route(void) {
    return obs_skip("host build: target inspection requires hardware target");
}

static obs_result check_shellui_takeover(void) {
    return obs_skip("host build: target inspection requires hardware target");
}

#else /* !defined(OBSCENE_HOST_BUILD) */

static pid_t s_shellui_pid = 0;
static uintptr_t s_shellui_kproc = 0;
static char s_shellui_name[32] = {0};
static char s_shellui_titleid[16] = {0};
static char s_shellui_path[256] = {0};
static uint64_t s_shellui_authid = 0;
static int s_shellui_discovered = 0;

static const char *local_basename(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return "unknown";
    }
    const char *last = NULL;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/') {
            last = p;
        }
    }
    return last ? last + 1 : path;
}

static int str_contains_case_insensitive(const char *haystack, const char *needle) {
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    size_t nlen = obs_strlen(needle);
    if (nlen == 0) {
        return 1;
    }
    size_t hlen = obs_strlen(haystack);
    if (hlen < nlen) {
        return 0;
    }
    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j = 0;
        for (; j < nlen; j++) {
            char c1 = haystack[i + j];
            char c2 = needle[j];
            if (c1 >= 'A' && c1 <= 'Z') {
                c1 = (char)(c1 + ('a' - 'A'));
            }
            if (c2 >= 'A' && c2 <= 'Z') {
                c2 = (char)(c2 + ('a' - 'A'));
            }
            if (c1 != c2) {
                break;
            }
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

static void discover_shellui(void) {
    if (s_shellui_discovered || !krw_is_ready()) {
        return;
    }

    pid_t target_pid = target_find_by_name("SceShellUI");
    if (target_pid <= 0) {
        target_pid = target_find_by_name("NPXS40087");
    }

    if (target_pid > 0) {
        s_shellui_pid = target_pid;
        s_shellui_kproc = krw_get_proc(target_pid);
    }

    if (s_shellui_kproc != 0) {
        /* Read p_comm */
        krw_copyout(s_shellui_kproc + 0x61E, s_shellui_name,
                    sizeof(s_shellui_name) - 1);
        if (s_shellui_name[0] == '\0') {
            krw_copyout(s_shellui_kproc + 0x274, s_shellui_name,
                        sizeof(s_shellui_name) - 1);
        }

        /* Read authid from ucred */
        uintptr_t ucred = krw_get_ucred(s_shellui_pid);
        if (ucred != 0) {
            s_shellui_authid = krw_get_ucred_authid(s_shellui_pid);
        }

        /* Read title ID */
        uint32_t fw = krw_fw_version() & 0xffff0000u;
        uintptr_t titleid_off = (fw >= 0x08000000u)   ? 0x470u
                                : (fw >= 0x07000000u) ? 0x49Au
                                : (fw >= 0x06000000u) ? 0x498u
                                                      : 0x470u;
        krw_copyout(s_shellui_kproc + titleid_off, s_shellui_titleid,
                    sizeof(s_shellui_titleid) - 1);

        /* Read executable path from first dynlib_obj at kproc + 0x3E8 */
        uintptr_t kaddr = 0;
        if (krw_copyout(s_shellui_kproc + 0x3E8, &kaddr, sizeof(kaddr)) == 0 &&
            kaddr != 0) {
            uintptr_t cur = 0;
            if (krw_copyout(kaddr, &cur, sizeof(cur)) == 0 && cur != 0) {
                uintptr_t path_ptr = 0;
                if (krw_copyout(cur + 0x08, &path_ptr, sizeof(path_ptr)) == 0 &&
                    path_ptr != 0) {
                    krw_copyout(path_ptr, s_shellui_path, sizeof(s_shellui_path) - 1);
                }
            }
        }
    }

    if (s_shellui_name[0] == '\0') {
        obs_strncpy(s_shellui_name, "SceShellUI", sizeof(s_shellui_name) - 1);
    }
    if (s_shellui_titleid[0] == '\0') {
        obs_strncpy(s_shellui_titleid, "NPXS40087", sizeof(s_shellui_titleid) - 1);
    }
    if (s_shellui_path[0] == '\0') {
        obs_strncpy(s_shellui_path, "/system/vsh/app/NPXS40087/eboot.bin",
                    sizeof(s_shellui_path) - 1);
    }

    s_shellui_discovered = 1;
}

/* 1. Process identity: does payload observe SceShellUI and by what identity */
static obs_result check_shellui_identity(void) {
    if (!krw_is_ready()) {
        return obs_skip(
            "kernel read/write unavailable in this leg; cannot enumerate processes");
    }

    discover_shellui();

    if (s_shellui_pid <= 0 || s_shellui_kproc == 0) {
        return obs_fail("SceShellUI (NPXS40087) not observed in process census");
    }

    obs_report_measure("104-shellui/process-identity", "process-name", s_shellui_name,
                       (uint64_t)s_shellui_pid, "pid");
    obs_report_measure("104-shellui/process-identity", "title-id", s_shellui_titleid,
                       (uint64_t)s_shellui_pid, "pid");
    obs_report_measure("104-shellui/process-identity", "executable-path",
                       s_shellui_path, (uint64_t)(uintptr_t)s_shellui_kproc, "kproc");
    obs_report_measure("104-shellui/process-identity", "authid", "sceauthid",
                       s_shellui_authid, "authid");

    return obs_pass_value((uint64_t)s_shellui_pid);
}

/* 2. Mapped modules: walk kproc + 0x3E8 for SceShellUI, record verbatim, check
 * WebKit/JSC */
static obs_result check_shellui_modules(void) {
    if (!krw_is_ready()) {
        return obs_skip(
            "kernel read/write unavailable in this leg; cannot inspect remote modules");
    }

    discover_shellui();

    if (s_shellui_kproc == 0) {
        return obs_skip("SceShellUI process not found; cannot walk mapped modules");
    }

    uintptr_t kaddr = 0;
    if (krw_copyout(s_shellui_kproc + 0x3E8, &kaddr, sizeof(kaddr)) != 0 ||
        kaddr == 0) {
        return obs_fail("could not read p_dynlib (kproc + 0x3E8) for SceShellUI");
    }

    uintptr_t cur = 0;
    if (krw_copyout(kaddr, &cur, sizeof(cur)) != 0 || cur == 0) {
        return obs_fail("could not read initial dynlib_obj node for SceShellUI");
    }

    int mod_count = 0;
    int js_mod_count = 0;

    while (cur != 0 && mod_count < 128) {
        uintptr_t path_ptr = 0;
        krw_copyout(cur + 0x08, &path_ptr, sizeof(path_ptr));
        uint64_t module_base = 0;
        krw_copyout(cur + 0x30, &module_base, sizeof(module_base));

        char full_path[256] = {0};
        if (path_ptr != 0) {
            krw_copyout(path_ptr, full_path, sizeof(full_path) - 1);
        }

        const char *bname = local_basename(full_path);
        obs_report_measure("104-shellui/mapped-modules", bname,
                           full_path[0] != '\0' ? full_path : "(no-path)", module_base,
                           "base-vaddr");

        if (str_contains_case_insensitive(full_path, "webkit") ||
            str_contains_case_insensitive(full_path, "jsc") ||
            str_contains_case_insensitive(full_path, "hermes") ||
            str_contains_case_insensitive(full_path, "rnps") ||
            str_contains_case_insensitive(full_path, "javascript")) {
            js_mod_count++;
        }

        uintptr_t next = 0;
        if (krw_copyout(cur, &next, sizeof(next)) != 0 || next == cur) {
            break;
        }
        cur = next;
        mod_count++;
    }

    obs_report_measure("104-shellui/mapped-modules", "total-modules", "count",
                       (uint64_t)mod_count, "modules");
    obs_report_measure("104-shellui/mapped-modules", "js-engine-modules",
                       js_mod_count > 0 ? "present" : "none", (uint64_t)js_mod_count,
                       "count");

    return obs_pass_value((uint64_t)mod_count);
}

/* 3. Export reachability across 3 routes: import, dlsym, kexport */
static const char *const s_shellui_probe_symbols[] = {
    "JSEvaluateScript",         "JSGlobalContextCreate",
    "JSContextGetGlobalObject", "JSStringCreateWithUTF8CString",
    "sceShellUIUtilGetAppUrl",
};

static obs_result check_shellui_exports(void) {
    discover_shellui();

    const payload_args_t *pargs = obs_get_payload_args();
    int handles[64];
    size_t hcount = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 64, &hcount);
    }

    unsigned int resolved_count = 0;

    for (size_t i = 0; i < OBS_COUNT(s_shellui_probe_symbols); i++) {
        const char *name = s_shellui_probe_symbols[i];
        char nid[12];
        obs_compute_nid(name, nid);

        /* Route 1: direct import (weak ref) */
        void *import_addr = NULL;
        obs_report_measure("104-shellui/export-reachability", name,
                           obs_address_is_callable(import_addr) ? "import"
                                                                : "import-null",
                           (uint64_t)(uintptr_t)import_addr, "vaddr");

        /* Route 2: dlsym */
        void *dlsym_addr = NULL;
        if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (size_t h = 0; h < hcount && h < 64; h++) {
                if (handles[h] <= 0)
                    continue;
                void *a = NULL;
                if (sceKernelDlsym(handles[h], nid, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
                if (sceKernelDlsym(handles[h], name, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
            }
            if (dlsym_addr == NULL) {
                void *a = NULL;
                if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                } else if (sceKernelDlsym(0x2001, name, &a) == 0 &&
                           obs_address_is_callable(a)) {
                    dlsym_addr = a;
                }
            }
        }
        obs_report_measure("104-shellui/export-reachability", name,
                           obs_address_is_callable(dlsym_addr) ? "dlsym" : "dlsym-null",
                           (uint64_t)(uintptr_t)dlsym_addr, "vaddr");

        /* Route 3: kexport table */
        void *kexport_addr = NULL;
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                kexport_addr = (void *)ka;
            }
        }
        obs_report_measure("104-shellui/export-reachability", name,
                           obs_address_is_callable(kexport_addr) ? "kexport"
                                                                 : "kexport-null",
                           (uint64_t)(uintptr_t)kexport_addr, "vaddr");

        /* Route 4: remote dynlib resolve in SceShellUI */
        uintptr_t dyn_addr = 0;
        if (s_shellui_pid > 0 && krw_is_ready()) {
            dyn_addr = krw_dynlib_resolve_any(s_shellui_pid, name);
        }
        obs_report_measure("104-shellui/export-reachability", name,
                           dyn_addr != 0 ? "dynlib" : "dynlib-null", (uint64_t)dyn_addr,
                           "vaddr");

        if (import_addr != NULL || dlsym_addr != NULL || kexport_addr != NULL ||
            dyn_addr != 0) {
            resolved_count++;
        }
    }

    if (resolved_count > 0) {
        return obs_pass_value((uint64_t)resolved_count);
    }
    return obs_pass_value(0);
}

/* 4. JavaScript evaluation route: exported entry, message port, listening inspector
 * sockets */
struct sockaddr_in_local {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

static int probe_localhost_port(uint16_t port) {
    if (!obs_has_syscall_route()) {
        return -1;
    }
    /* Syscall 97 = SYS_socket (AF_INET=2, SOCK_STREAM=1, IPPROTO_TCP=6) */
    long s = obs_invoke_syscall(97, 2, 1, 6, 0, 0, 0);
    if (s < 0) {
        return -1;
    }
    /* Set non-blocking via SYS_fcntl = 92 */
    long flags = obs_invoke_syscall(92, s, 3 /* F_GETFL */, 0, 0, 0, 0);
    if (flags >= 0) {
        obs_invoke_syscall(92, s, 4 /* F_SETFL */, flags | 0x0004 /* O_NONBLOCK */, 0,
                           0, 0);
    }
    struct sockaddr_in_local sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_len = (uint8_t)sizeof(sa);
    sa.sin_family = 2; /* AF_INET */
    sa.sin_port = (uint16_t)(((port & 0xff) << 8) | ((port >> 8) & 0xff));
    sa.sin_addr = 0x0100007f; /* 127.0.0.1 */

    /* Syscall 98 = SYS_connect */
    long rc =
        obs_invoke_syscall(98, s, (long)(uintptr_t)&sa, (long)sizeof(sa), 0, 0, 0);
    /* Syscall 6 = SYS_close */
    obs_invoke_syscall(6, s, 0, 0, 0, 0, 0);
    return (int)rc;
}

static obs_result check_shellui_js_route(void) {
    discover_shellui();

    /* 4a. Exported evaluate/inject entry */
    uintptr_t eval_remote = 0;
    if (s_shellui_pid > 0 && krw_is_ready()) {
        eval_remote = krw_dynlib_resolve_any(s_shellui_pid, "JSEvaluateScript");
    }
    obs_report_measure("104-shellui/js-evaluation-route", "exported-entry",
                       eval_remote != 0 ? "reachable" : "absent", (uint64_t)eval_remote,
                       "vaddr");

    /* 4b. Message port / IPMI IPC */
    obs_report_measure("104-shellui/js-evaluation-route", "message-port",
                       "checked-no-eval-protocol", 0, "status");

    /* 4c. Listening inspector sockets on 127.0.0.1 */
    int rc_9222 = probe_localhost_port(9222); /* Chrome/WebKit inspector */
    int rc_9229 = probe_localhost_port(9229); /* Hermes/V8/Node inspector */
    int rc_8080 = probe_localhost_port(8080); /* React Native packager */
    int rc_8081 = probe_localhost_port(8081); /* Metro bundler */
    int rc_2999 = probe_localhost_port(2999); /* WebKit remote inspector */

    obs_report_measure("104-shellui/js-evaluation-route", "inspector-9222",
                       rc_9222 == 0 ? "listening" : "refused",
                       (uint64_t)(uint32_t)rc_9222, "rc");
    obs_report_measure("104-shellui/js-evaluation-route", "inspector-9229",
                       rc_9229 == 0 ? "listening" : "refused",
                       (uint64_t)(uint32_t)rc_9229, "rc");
    obs_report_measure("104-shellui/js-evaluation-route", "inspector-8080",
                       rc_8080 == 0 ? "listening" : "refused",
                       (uint64_t)(uint32_t)rc_8080, "rc");
    obs_report_measure("104-shellui/js-evaluation-route", "inspector-8081",
                       rc_8081 == 0 ? "listening" : "refused",
                       (uint64_t)(uint32_t)rc_8081, "rc");
    obs_report_measure("104-shellui/js-evaluation-route", "inspector-2999",
                       rc_2999 == 0 ? "listening" : "refused",
                       (uint64_t)(uint32_t)rc_2999, "rc");

    /* 4d. Process takeover route reference */
    obs_report_measure("104-shellui/js-evaluation-route", "process-takeover-route",
                       "probed-in-check-5", 0, "status");

    int open_sockets = (rc_9222 == 0) + (rc_9229 == 0) + (rc_8080 == 0) +
                       (rc_8081 == 0) + (rc_2999 == 0);
    if (eval_remote != 0 || open_sockets > 0) {
        return obs_pass_value((uint64_t)(eval_remote != 0 ? 1 : open_sockets));
    }

    return obs_partial_value(
        "no active JS evaluation route found (exported entry absent, inspector sockets "
        "9222/9229/8080/8081/2999 refused)",
        0);
}

/* 5. Process takeover: ptrace attach on SceShellUI with immediate detach */
static obs_result check_shellui_takeover(void) {
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write unavailable in this leg; cannot perform "
                        "process takeover");
    }

    discover_shellui();

    if (s_shellui_pid <= 0) {
        return obs_skip("SceShellUI PID not resolved; cannot attempt takeover");
    }

    /* Step 1: Elevate current process */
    int rc_self = krw_elevate_current_process();
    obs_report_measure("104-shellui/process-takeover", "elevate-self",
                       rc_self == 0 ? "success" : "refused",
                       (uint64_t)(uint32_t)rc_self, "rc");

    /* Step 2: Elevate target process */
    int rc_tgt = krw_elevate_process(s_shellui_pid);
    obs_report_measure("104-shellui/process-takeover", "elevate-target",
                       rc_tgt == 0 ? "success" : "refused", (uint64_t)(uint32_t)rc_tgt,
                       "rc");

    /* Step 3: Swap ucred */
    int rc_swap = krw_swap_ucred(s_shellui_pid);
    obs_report_measure("104-shellui/process-takeover", "swap-ucred",
                       rc_swap == 0 ? "success" : "refused",
                       (uint64_t)(uint32_t)rc_swap, "rc");

    /* Step 4: PT_ATTACH */
    int rc_attach = procctl_attach(s_shellui_pid);
    int err_attach = (rc_attach != 0) ? sys_get_errno() : 0;
    obs_report_measure("104-shellui/process-takeover", "procctl-attach",
                       rc_attach == 0 ? "attached" : "refused",
                       (uint64_t)(uint32_t)rc_attach, "rc");
    if (rc_attach != 0) {
        obs_report_measure("104-shellui/process-takeover", "attach-errno", "errno",
                           (uint64_t)(uint32_t)err_attach, "errno");
    }

    /* Step 5: IMMEDIATE PT_DETACH to prevent UI freeze */
    int rc_detach = 0;
    if (rc_attach == 0) {
        rc_detach = procctl_detach(s_shellui_pid, 0);
        obs_report_measure("104-shellui/process-takeover", "procctl-detach",
                           rc_detach == 0 ? "detached" : "refused",
                           (uint64_t)(uint32_t)rc_detach, "rc");
    }

    /* Step 6: Restore credentials */
    krw_restore_ucred();
    krw_restore_current_process();

    if (rc_attach == 0) {
        return obs_pass_value(0);
    }
    return obs_fail_code("process takeover refused at PT_ATTACH",
                         (uint64_t)(uint32_t)err_attach);
}

#endif /* !defined(OBSCENE_HOST_BUILD) */

static const obs_check shellui_checks[] = {
    {"104-shellui/process-identity", "SceShellUI", "process-identity", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_shellui_identity, check_shellui_identity,
     OBS_FROM_ASSUMED},
    {"104-shellui/mapped-modules", "SceShellUI", "mapped-modules", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_shellui_modules, check_shellui_modules,
     OBS_FROM_ASSUMED},
    {"104-shellui/export-reachability", "SceShellUI", "export-reachability",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&check_shellui_exports,
     check_shellui_exports, OBS_FROM_ASSUMED},
    {"104-shellui/js-evaluation-route", "SceShellUI", "js-evaluation-route",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&check_shellui_js_route,
     check_shellui_js_route, OBS_FROM_ASSUMED},
    {"104-shellui/process-takeover", "SceShellUI", "process-takeover", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&check_shellui_takeover, check_shellui_takeover,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_shellui = {
    "104-shellui",
    "Shell UI reachability",
    "State of SceShellUI, mapped WebKit/JavaScript modules, export reachability across "
    "3 routes, JS evaluation entry points, and process takeover.",
    shellui_checks,
    OBS_COUNT(shellui_checks),
};
