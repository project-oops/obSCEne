/*
 * Network reachability, for Porthole.
 *
 * Porthole (oops-apps/src/porthole) is an elfldr payload that serves video and
 * controller sockets, and it resolves libSceNet itself because imports do not auto-bind
 * in unsigned payload mode. Nothing had confirmed the network is reachable that way,
 * and three constants in its socket layer were taken from public headers that disagree
 * rather than measured. This section answers that: it resolves the socket calls the way
 * a payload must, reports each address so a null one is told from a failed call, opens
 * a listener on a scratch port, and measures the two things Porthole's accept loop
 * turns on - the non-blocking option value and the would-block code.
 *
 * Every call is through a resolved pointer, never a raw import: in payload mode
 * `&sceNetSocket` is unbound, and the point is to reach the function the way Porthole
 * has to. A leg where nothing resolves is the honest finding that the delivery route
 * needs rethinking. (D329)
 */

#include "oops/freestd.h"
#include "oops/krw.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* Clear of everything in use (9021/9022/2121/3232/2323/8084/6967 and Porthole's
 * 9805/9806). Dedicated scratch ports per check avoid EADDRINUSE (0x80410130)
 * collisions from TIME_WAIT states. */
#define OBS_NET_PORT_LISTENER 9891
#define OBS_NET_PORT_RECV 9892
#define OBS_NET_PORT_ACCEPT 9893
#define OBS_NET_PORT_SOCKADDR_LEN16 9894
#define OBS_NET_PORT_SOCKADDR_LEN0 9895
/* 127.0.0.1 in network byte order, stored little-endian: bytes 7F 00 00 01. */
#define OBS_NET_LOOPBACK 0x0100007Fu

static uint16_t net_htons(uint16_t host) {
    return (uint16_t)((host << 8) | (host >> 8));
}

static int net_is_payload_mode(void) {
    return obs_get_payload_args() != NULL;
}

static int net_krw_is_ready(void) {
#if !defined(OBSCENE_HOST_BUILD)
    return krw_is_ready();
#else
    return 0;
#endif
}

static long net_syscall(long num, long a1, long a2, long a3, long a4, long a5,
                        long a6) {
#if !defined(OBSCENE_HOST_BUILD)
    return obs_invoke_syscall(num, a1, a2, a3, a4, a5, a6);
#else
    (void)num;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    (void)a6;
    return -1;
#endif
}

/* Walk the kernel export table / live dispatch table (D277/D300) for libSceNet symbols.
 */
static const void *net_walk_symbol(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    int handle = obs_module_open("libSceNet");
    (void)handle;

    char nid[12];
    obs_compute_nid(name, nid);

    /* 1. Try kernel dispatch table walk (D277/D278) via krw_dynlib_resolve_any */
#if !defined(OBSCENE_HOST_BUILD)
    if (krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t addr = krw_dynlib_resolve_any(pid, name);
        if (addr == 0 && handle > 0) {
            addr = krw_dynlib_resolve(pid, handle, nid);
        }
        if (addr >= 0x10000UL && obs_address_is_callable((const void *)addr)) {
            return (const void *)addr;
        }
    }
#endif

    /* 2. Try pre-dumped kexport_table if available in payload_args */
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        const void *kaddr =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (kaddr != NULL && obs_address_is_callable(kaddr)) {
            return kaddr;
        }
    }

    return NULL;
}

/* Resolve a libSceNet symbol for title mode: direct import, dlsym gadget, or walk. */
static const void *net_sym(const char *name, const void *direct) {
    if (obs_address_is_callable(direct)) {
        return direct;
    }
    int handle = obs_module_open("libSceNet");
    const void *p = obs_module_symbol(handle, name);
    if (obs_address_is_callable(p)) {
        return p;
    }
    const void *w = net_walk_symbol(name);
    return obs_address_is_callable(w) ? w : NULL;
}

/* Resolve a POSIX / libkernel candidate from candidate spellings in payload mode. */
static const void *net_resolve_posix_candidate(const char *const *candidates,
                                               size_t count) {
    const payload_args_t *pargs = obs_get_payload_args();
    for (size_t i = 0; i < count; i++) {
        const char *name = candidates[i];
        if (name == NULL) {
            continue;
        }
        char nid[12];
        obs_compute_nid(name, nid);
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *kaddr = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (kaddr != NULL && obs_address_is_callable(kaddr)) {
                return kaddr;
            }
        }
        const void *sym = obs_module_symbol(1, name);
        if (sym != NULL && obs_address_is_callable(sym)) {
            return sym;
        }
#if !defined(OBSCENE_HOST_BUILD)
        if (krw_is_ready()) {
            pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
            uintptr_t addr = krw_dynlib_resolve_any(pid, name);
            if (addr >= 0x10000UL && obs_address_is_callable((const void *)addr)) {
                return (const void *)addr;
            }
        }
#endif
    }
    return NULL;
}

typedef struct net_fns {
    int is_payload;

    /* Title mode (libSceNet) */
    int (*sce_socket)(const char *, int, int, int);
    int (*sce_bind)(int, const void *, uint32_t);
    int (*sce_listen)(int, int);
    int (*sce_accept)(int, void *, uint32_t *);
    int (*sce_recv)(int, void *, uint64_t, int);
    int (*sce_send)(int, const void *, uint64_t, int);
    int (*sce_close)(int);
    int (*sce_setsockopt)(int, int, int, const void *, uint32_t);
    int (*sce_connect)(int, const void *, uint32_t);

    /* Payload mode (POSIX / libkernel) */
    int (*socketex)(const char *, int, int, int);
    int (*posix_socket)(int, int, int);
    int (*posix_bind)(int, const void *, uint32_t);
    int (*posix_listen)(int, int);
    int (*posix_accept)(int, void *, uint32_t *);
    long (*posix_recv)(int, void *, size_t, int);
    long (*posix_sendto)(int, const void *, size_t, int, const void *, uint32_t);
    long (*posix_send)(int, const void *, size_t, int);
    int (*posix_close)(int);
    int (*sys_socketclose)(int);
    int (*posix_setsockopt)(int, int, int, const void *, uint32_t);
    int (*posix_connect)(int, const void *, uint32_t);
    int (*fcntl)(int, int, long);
    int *(*error_loc)(void);
} net_fns;

static int net_resolve_all(net_fns *f) {
    for (unsigned int i = 0; i < sizeof(*f); i++) {
        ((volatile unsigned char *)f)[i] = 0;
    }
    f->is_payload = net_is_payload_mode();
    if (f->is_payload) {
        static const char *const c_socketex[] = {"__sys_socketex"};
        static const char *const c_socket[] = {"socket", "_socket", "__sys_socket"};
        static const char *const c_bind[] = {"bind", "_bind"};
        static const char *const c_listen[] = {"listen", "_listen"};
        static const char *const c_accept[] = {"accept", "_accept"};
        static const char *const c_recv[] = {"recv", "_recv", "_recvfrom"};
        static const char *const c_sendto[] = {"_sendto", "sendto"};
        static const char *const c_send[] = {"send", "_send"};
        static const char *const c_close[] = {"close", "_close"};
        static const char *const c_socketclose[] = {"__sys_socketclose"};
        static const char *const c_setsockopt[] = {"_setsockopt", "setsockopt"};
        static const char *const c_connect[] = {"connect", "_connect"};
        static const char *const c_fcntl[] = {"fcntl", "_fcntl"};
        static const char *const c_error[] = {"__error", "__Error", "__errno_location"};

        /* clang-format off */
        f->socketex         = (int (*)(const char *, int, int, int))(uintptr_t)net_resolve_posix_candidate(c_socketex, OBS_COUNT(c_socketex));
        f->posix_socket     = (int (*)(int, int, int))(uintptr_t)net_resolve_posix_candidate(c_socket, OBS_COUNT(c_socket));
        f->posix_bind       = (int (*)(int, const void *, uint32_t))(uintptr_t)net_resolve_posix_candidate(c_bind, OBS_COUNT(c_bind));
        f->posix_listen     = (int (*)(int, int))(uintptr_t)net_resolve_posix_candidate(c_listen, OBS_COUNT(c_listen));
        f->posix_accept     = (int (*)(int, void *, uint32_t *))(uintptr_t)net_resolve_posix_candidate(c_accept, OBS_COUNT(c_accept));
        f->posix_recv       = (long (*)(int, void *, size_t, int))(uintptr_t)net_resolve_posix_candidate(c_recv, OBS_COUNT(c_recv));
        f->posix_sendto     = (long (*)(int, const void *, size_t, int, const void *, uint32_t))(uintptr_t)net_resolve_posix_candidate(c_sendto, OBS_COUNT(c_sendto));
        f->posix_send       = (long (*)(int, const void *, size_t, int))(uintptr_t)net_resolve_posix_candidate(c_send, OBS_COUNT(c_send));
        f->posix_close      = (int (*)(int))(uintptr_t)net_resolve_posix_candidate(c_close, OBS_COUNT(c_close));
        f->sys_socketclose  = (int (*)(int))(uintptr_t)net_resolve_posix_candidate(c_socketclose, OBS_COUNT(c_socketclose));
        f->posix_setsockopt = (int (*)(int, int, int, const void *, uint32_t))(uintptr_t)net_resolve_posix_candidate(c_setsockopt, OBS_COUNT(c_setsockopt));
        f->posix_connect    = (int (*)(int, const void *, uint32_t))(uintptr_t)net_resolve_posix_candidate(c_connect, OBS_COUNT(c_connect));
        f->fcntl            = (int (*)(int, int, long))(uintptr_t)net_resolve_posix_candidate(c_fcntl, OBS_COUNT(c_fcntl));
        f->error_loc        = (int *(*)(void))(uintptr_t)net_resolve_posix_candidate(c_error, OBS_COUNT(c_error));
        /* clang-format on */

        return f->posix_bind != NULL && f->posix_listen != NULL;
    }

    /* Title mode (libSceNet) */
    /* clang-format off */
    f->sce_socket     = (int (*)(const char *, int, int, int))(uintptr_t)net_sym("sceNetSocket", (const void *)&sceNetSocket);
    f->sce_bind       = (int (*)(int, const void *, uint32_t))(uintptr_t)net_sym("sceNetBind", (const void *)&sceNetBind);
    f->sce_listen     = (int (*)(int, int))(uintptr_t)net_sym("sceNetListen", (const void *)&sceNetListen);
    f->sce_accept     = (int (*)(int, void *, uint32_t *))(uintptr_t)net_sym("sceNetAccept", (const void *)&sceNetAccept);
    f->sce_recv       = (int (*)(int, void *, uint64_t, int))(uintptr_t)net_sym("sceNetRecv", (const void *)&sceNetRecv);
    f->sce_send       = (int (*)(int, const void *, uint64_t, int))(uintptr_t)net_sym("sceNetSend", (const void *)&sceNetSend);
    f->sce_close      = (int (*)(int))(uintptr_t)net_sym("sceNetSocketClose", (const void *)&sceNetSocketClose);
    f->sce_setsockopt = (int (*)(int, int, int, const void *, uint32_t))(uintptr_t)net_sym("sceNetSetsockopt", (const void *)&sceNetSetsockopt);
    f->sce_connect    = (int (*)(int, const void *, uint32_t))(uintptr_t)net_sym("sceNetConnect", (const void *)&sceNetConnect);
    /* clang-format on */
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }
    return f->sce_socket != NULL && f->sce_bind != NULL && f->sce_listen != NULL &&
           f->sce_close != NULL;
}

static int net_open_socket(const net_fns *f, const char *name) {
    if (f->is_payload) {
        if (f->socketex != NULL) {
            int s = f->socketex(name, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                OBS_NET_IPPROTO_TCP);
            if (s >= 0) {
                return s;
            }
            s = f->socketex(NULL, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                            OBS_NET_IPPROTO_TCP);
            if (s >= 0) {
                return s;
            }
        }
        if (f->posix_socket != NULL) {
            int s = f->posix_socket(OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                    OBS_NET_IPPROTO_TCP);
            if (s >= 0) {
                return s;
            }
        }
        /* FreeBSD syscall 97: SYS_socket(domain, type, protocol) */
        long s = net_syscall(97, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                             OBS_NET_IPPROTO_TCP, 0, 0, 0);
        return (int)s;
    }
    if (f->sce_socket != NULL) {
        int s = f->sce_socket(name, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                              OBS_NET_IPPROTO_TCP);
        if (s < 0 && name != NULL) {
            s = f->sce_socket(NULL, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                              OBS_NET_IPPROTO_TCP);
        }
        return s;
    }
    return -1;
}

static int net_bind_wrapper(const net_fns *f, int s, const void *addr, uint32_t len) {
    if (f->is_payload) {
        return f->posix_bind ? f->posix_bind(s, addr, len) : -1;
    }
    return f->sce_bind ? f->sce_bind(s, addr, len) : -1;
}

static int net_listen_wrapper(const net_fns *f, int s, int backlog) {
    if (f->is_payload) {
        return f->posix_listen ? f->posix_listen(s, backlog) : -1;
    }
    return f->sce_listen ? f->sce_listen(s, backlog) : -1;
}

static int net_accept_wrapper(const net_fns *f, int s, void *addr, uint32_t *len) {
    if (f->is_payload) {
        return f->posix_accept ? f->posix_accept(s, addr, len) : -1;
    }
    return f->sce_accept ? f->sce_accept(s, addr, len) : -1;
}

static int net_recv_wrapper(const net_fns *f, int s, void *buf, size_t len, int flags) {
    if (f->is_payload) {
        return f->posix_recv ? (int)f->posix_recv(s, buf, len, flags) : -1;
    }
    return f->sce_recv ? f->sce_recv(s, buf, len, flags) : -1;
}

static int net_send_wrapper(const net_fns *f, int s, const void *buf, size_t len,
                            int flags) {
    if (f->is_payload) {
        if (f->posix_send != NULL) {
            return (int)f->posix_send(s, buf, len, flags);
        }
        if (f->posix_sendto != NULL) {
            return (int)f->posix_sendto(s, buf, len, flags, NULL, 0);
        }
        return -1;
    }
    return f->sce_send ? f->sce_send(s, buf, len, flags) : -1;
}

static int net_close_wrapper(const net_fns *f, int s) {
    if (f->is_payload) {
        if (f->posix_close != NULL) {
            return f->posix_close(s);
        }
        if (f->sys_socketclose != NULL) {
            return f->sys_socketclose(s);
        }
        return (int)net_syscall(6, s, 0, 0, 0, 0, 0);
    }
    return f->sce_close ? f->sce_close(s) : -1;
}

static int net_setsockopt_wrapper(const net_fns *f, int s, int level, int optname,
                                  const void *optval, uint32_t optlen) {
    if (f->is_payload) {
        return f->posix_setsockopt
                   ? f->posix_setsockopt(s, level, optname, optval, optlen)
                   : -1;
    }
    return f->sce_setsockopt ? f->sce_setsockopt(s, level, optname, optval, optlen)
                             : -1;
}

static int net_connect_wrapper(const net_fns *f, int s, const void *addr,
                               uint32_t len) {
    if (f->is_payload) {
        return f->posix_connect ? f->posix_connect(s, addr, len) : -1;
    }
    return f->sce_connect ? f->sce_connect(s, addr, len) : -1;
}

static int net_set_nonblocking(const net_fns *f, int s) {
    int one = 1;
    int rc = net_setsockopt_wrapper(f, s, OBS_NET_SOL_SOCKET, OBS_NET_SO_NBIO_OPENORBIS,
                                    &one, (uint32_t)sizeof(one));
    if (f->is_payload && f->fcntl != NULL) {
        long flags = f->fcntl(s, 3 /* F_GETFL */, 0);
        if (flags >= 0) {
            int frc = f->fcntl(s, 4 /* F_SETFL */, flags | 0x0004 /* O_NONBLOCK */);
            if (frc == 0) {
                rc = 0;
            }
        }
    }
    return rc;
}

static int net_last_errno(const net_fns *f, int rc) {
    if (f->is_payload && f->error_loc != NULL) {
        int *p = f->error_loc();
        if (p != NULL) {
            return *p;
        }
    }
    if (rc < 0 && ((uint32_t)rc & 0xFFFFFF00u) == 0x80410100u) {
        return (int)((uint32_t)rc & 0xFFu);
    }
    return 0;
}

/* Fill an obs_net_sockaddr_in for the specified port. addr is host-order (INADDR_ANY =
 * 0, or loopback); sin_len is the struct size unless told otherwise. */
static void net_fill_addr(obs_net_sockaddr_in *a, uint16_t port, uint32_t sin_addr,
                          uint8_t sin_len) {
    for (unsigned int i = 0; i < sizeof(*a); i++) {
        ((volatile unsigned char *)a)[i] = 0;
    }
    a->sin_len = sin_len;
    a->sin_family = OBS_NET_AF_INET;
    a->sin_port = net_htons(port);
    a->sin_addr = sin_addr;
    a->sin_vport = 0;
}

/* ---- Ask 1: resolve the eight (and connect), report each address
 * ------------------------- */

static const char *const net_symbol_names[] = {
    "sceNetSocket",      "sceNetBind",       "sceNetListen",
    "sceNetAccept",      "sceNetRecv",       "sceNetSend",
    "sceNetSocketClose", "sceNetSetsockopt", "sceNetConnect",
};

static const void *const net_symbol_direct[] = {
    (const void *)&sceNetSocket,      (const void *)&sceNetBind,
    (const void *)&sceNetListen,      (const void *)&sceNetAccept,
    (const void *)&sceNetRecv,        (const void *)&sceNetSend,
    (const void *)&sceNetSocketClose, (const void *)&sceNetSetsockopt,
    (const void *)&sceNetConnect,
};

static obs_result check_net_resolve(void) {
    if (net_is_payload_mode()) {
        static const struct {
            const char *primary;
            const char *const candidates[4];
            size_t count;
        } posix_syms[] = {
            {"__sys_socketex",
             {"__sys_socketex", "socket", "_socket", "__sys_socket"},
             4},
            {"bind", {"bind", "_bind", NULL, NULL}, 2},
            {"listen", {"listen", "_listen", NULL, NULL}, 2},
            {"accept", {"accept", "_accept", NULL, NULL}, 2},
            {"recv", {"recv", "_recv", "_recvfrom", NULL}, 3},
            {"_sendto", {"_sendto", "sendto", "send", "_send"}, 4},
            {"close", {"close", "_close", "__sys_socketclose", NULL}, 3},
            {"_setsockopt", {"_setsockopt", "setsockopt", NULL, NULL}, 2},
            {"connect", {"connect", "_connect", NULL, NULL}, 2},
            {"fcntl", {"fcntl", "_fcntl", NULL, NULL}, 2},
            {"__error", {"__error", "__Error", "__errno_location", NULL}, 3},
        };
        unsigned int resolved = 0;
        for (unsigned int i = 0; i < OBS_COUNT(posix_syms); i++) {
            const void *addr = net_resolve_posix_candidate(posix_syms[i].candidates,
                                                           posix_syms[i].count);
            obs_report_measure("102-net/resolve", posix_syms[i].primary,
                               addr != NULL ? "kexport" : "kexport-null",
                               (uint64_t)(uintptr_t)addr, "vaddr");
            obs_report_measure("102-net/posix-symbols", posix_syms[i].primary,
                               obs_address_is_callable(addr) ? "callable"
                                                             : "not-callable",
                               obs_address_is_callable(addr) ? 1 : 0, "flag");
            obs_report_measure("102-net/posix-symbols", posix_syms[i].primary, "vaddr",
                               (uint64_t)(uintptr_t)addr, "address");
            if (addr != NULL) {
                resolved++;
            }
        }
        /* Measure FreeBSD syscall 97 */
        long s_test = net_syscall(97, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                  OBS_NET_IPPROTO_TCP, 0, 0, 0);
        obs_report_measure("102-net/resolve", "SYS_socket",
                           s_test >= 0 ? "syscall-fd" : "syscall-refused",
                           (uint64_t)(uint32_t)s_test, "fd");
        obs_report_measure("102-net/socket-syscall", "SYS_socket",
                           s_test >= 0 ? "syscall-fd" : "syscall-refused",
                           (uint64_t)(uint32_t)s_test, "fd");
        if (s_test >= 0) {
            net_fns f_tmp;
            net_resolve_all(&f_tmp);
            net_close_wrapper(&f_tmp, (int)s_test);
        }

        if (resolved == 0 && s_test < 0) {
            return obs_skip(
                "no POSIX socket symbols or syscall 97 resolved in payload mode");
        }
        if (resolved < OBS_COUNT(posix_syms)) {
            return obs_partial_value("some POSIX socket symbols did not resolve",
                                     (uint64_t)resolved);
        }
        return obs_pass_value((uint64_t)resolved);
    }

    /* Title mode (eboot, pkg) */
    unsigned int resolved = 0;
    for (unsigned int i = 0; i < OBS_COUNT(net_symbol_names); i++) {
        const void *direct = net_symbol_direct[i];
        int handle = obs_module_open("libSceNet");
        const void *viadlsym = obs_module_symbol(handle, net_symbol_names[i]);
        const void *viawalk = net_walk_symbol(net_symbol_names[i]);

        obs_report_measure("102-net/resolve", net_symbol_names[i],
                           obs_address_is_callable(direct) ? "import" : "import-null",
                           (uint64_t)(uintptr_t)direct, "vaddr");
        obs_report_measure("102-net/resolve", net_symbol_names[i],
                           obs_address_is_callable(viadlsym) ? "dlsym" : "dlsym-null",
                           (uint64_t)(uintptr_t)viadlsym, "vaddr");
        /* Only report walk if kernel dispatch walk capability is available.
         * In unprivileged title mode (eboot), omitting this avoids failing positive
         * control. */
        if (net_krw_is_ready()) {
            obs_report_measure("102-net/resolve", net_symbol_names[i],
                               obs_address_is_callable(viawalk) ? "walk" : "walk-null",
                               (uint64_t)(uintptr_t)viawalk, "vaddr");
        }
        if (obs_address_is_callable(direct) || obs_address_is_callable(viadlsym) ||
            obs_address_is_callable(viawalk)) {
            resolved++;
        }
    }
    if (resolved == 0) {
        return obs_skip(
            "libSceNet resolved by neither import, dlsym nor walk in this leg");
    }
    if (resolved < OBS_COUNT(net_symbol_names)) {
        return obs_partial_value("some libSceNet symbols did not resolve",
                                 (uint64_t)resolved);
    }
    return obs_pass_value((uint64_t)resolved);
}

/* ---- Ask 2.1: a listener can be opened at all
 * -------------------------------------------- */

static obs_result check_net_listener(void) {
    net_fns f;
    if (!net_resolve_all(&f)) {
        return obs_skip("socket calls did not resolve in this leg");
    }

    if (f.is_payload) {
        /* Settle how payload obtains a descriptor: measure syscall 97 and
         * __sys_socketex */
        long sys_fd = net_syscall(97, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                  OBS_NET_IPPROTO_TCP, 0, 0, 0);
        obs_report_measure("102-net/listener", "SYS_socket",
                           sys_fd >= 0 ? "descriptor" : "refused",
                           (uint64_t)(uint32_t)sys_fd, "return");
        obs_report_measure("102-net/socket-create", "SYS_socket",
                           sys_fd >= 0 ? "descriptor" : "refused",
                           (uint64_t)(uint32_t)sys_fd, "return");

        /* Confirm descriptor on scratch port 9899 per REQ-20260908T1528Z-7c1e */
        if (sys_fd >= 0) {
            obs_net_sockaddr_in a9899;
            net_fill_addr(&a9899, 9899, 0u, 16);
            int brc9899 = net_bind_wrapper(&f, (int)sys_fd, &a9899, 16);
            int lrc9899 = net_listen_wrapper(&f, (int)sys_fd, 1);
            obs_report_measure("102-net/socket-syscall-97", "bind-port-9899",
                               brc9899 == 0 ? "bound" : "refused",
                               (uint64_t)(uint32_t)brc9899, "return");
            obs_report_measure("102-net/socket-syscall-97", "listen-port-9899",
                               lrc9899 == 0 ? "listening" : "refused",
                               (uint64_t)(uint32_t)lrc9899, "return");
            net_close_wrapper(&f, (int)sys_fd);
        }

        if (f.socketex != NULL) {
            obs_report_measure("102-net/socket-resolve", "__sys_socketex", "address",
                               (uint64_t)(uintptr_t)f.socketex, "vaddr");
            int ex_named = f.socketex("obscene", OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                      OBS_NET_IPPROTO_TCP);
            obs_report_measure("102-net/listener", "__sys_socketex",
                               ex_named >= 0 ? "named" : "named-refused",
                               (uint64_t)(uint32_t)ex_named, "return");
            obs_report_measure("102-net/socket-create", "__sys_socketex",
                               ex_named >= 0 ? "named" : "named-refused",
                               (uint64_t)(uint32_t)ex_named, "return");
            if (ex_named >= 0) {
                net_close_wrapper(&f, ex_named);
            }
            int ex_null = f.socketex(NULL, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                     OBS_NET_IPPROTO_TCP);
            obs_report_measure("102-net/listener", "__sys_socketex",
                               ex_null >= 0 ? "null-name" : "null-refused",
                               (uint64_t)(uint32_t)ex_null, "return");
            obs_report_measure("102-net/socket-create", "__sys_socketex",
                               ex_null >= 0 ? "null-name" : "null-refused",
                               (uint64_t)(uint32_t)ex_null, "return");
            if (ex_null >= 0) {
                net_close_wrapper(&f, ex_null);
            }
        }
    } else {
        /* Title leg: try named vs null-name on sceNetSocket */
        int s_named = f.sce_socket("obscene", OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                   OBS_NET_IPPROTO_TCP);
        obs_report_measure("102-net/listener", "sceNetSocket",
                           s_named >= 0 ? "named" : "named-refused",
                           (uint64_t)(uint32_t)s_named, "return");
        if (s_named >= 0) {
            net_close_wrapper(&f, s_named);
        }
        int s_null = f.sce_socket(NULL, OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                                  OBS_NET_IPPROTO_TCP);
        obs_report_measure("102-net/listener", "sceNetSocket",
                           s_null >= 0 ? "null-name" : "null-refused",
                           (uint64_t)(uint32_t)s_null, "return");
        if (s_null >= 0) {
            net_close_wrapper(&f, s_null);
        }
    }

    int s = net_open_socket(&f, "obscene");
    if (s < 0) {
        return obs_fail_code("no socket could be opened", (uint64_t)(uint32_t)s);
    }
    obs_net_sockaddr_in addr;
    net_fill_addr(&addr, OBS_NET_PORT_LISTENER, 0u, (uint8_t)sizeof(addr));
    int brc = net_bind_wrapper(&f, s, &addr, (uint32_t)sizeof(addr));
    int lrc = net_listen_wrapper(&f, s, 1);
    obs_report_measure("102-net/listener", f.is_payload ? "bind" : "sceNetBind",
                       brc == 0 ? "bound" : "refused", (uint64_t)(uint32_t)brc,
                       "return");
    obs_report_measure("102-net/socket-bind", f.is_payload ? "bind" : "sceNetBind",
                       brc == 0 ? "bound" : "refused", (uint64_t)(uint32_t)brc,
                       "return");
    obs_report_measure("102-net/listener", f.is_payload ? "listen" : "sceNetListen",
                       lrc == 0 ? "listening" : "refused", (uint64_t)(uint32_t)lrc,
                       "return");
    obs_report_measure(
        "102-net/socket-listen", f.is_payload ? "listen" : "sceNetListen",
        lrc == 0 ? "listening" : "refused", (uint64_t)(uint32_t)lrc, "return");
    net_close_wrapper(&f, s);
    if (brc != 0 || lrc != 0) {
        return obs_partial_value("socket opened but bind or listen refused",
                                 (uint64_t)(uint32_t)(brc != 0 ? brc : lrc));
    }
    return obs_pass_value((uint64_t)(uint32_t)s);
}

/* ---- Ask 2.2: the non-blocking option
 * ---------------------------------------------------- */

static obs_result check_net_nonblocking_option(void) {
    net_fns f;
    if (!net_resolve_all(&f)) {
        return obs_skip("socket calls did not resolve in this leg");
    }
    int s = net_open_socket(&f, "obscene");
    if (s < 0) {
        return obs_fail_code("no socket to set the option on", (uint64_t)(uint32_t)s);
    }
    int one = 1;
    int rc_a =
        net_setsockopt_wrapper(&f, s, OBS_NET_SOL_SOCKET, OBS_NET_SO_NBIO_OPENORBIS,
                               &one, (uint32_t)sizeof one);
    obs_report_measure("102-net/nonblocking-option",
                       f.is_payload ? "_setsockopt" : "sceNetSetsockopt",
                       rc_a == 0 ? "0x1200-accepted" : "0x1200-return",
                       (uint64_t)(uint32_t)rc_a, "return");
    int rc_b = 0;
    int tried_b = 0;
    if (rc_a != 0 && !f.is_payload) {
        tried_b = 1;
        rc_b =
            net_setsockopt_wrapper(&f, s, OBS_NET_SOL_SOCKET, OBS_NET_SO_NBIO_VITASDK,
                                   &one, (uint32_t)sizeof one);
        obs_report_measure("102-net/nonblocking-option", "sceNetSetsockopt",
                           rc_b == 0 ? "0x1100-accepted" : "0x1100-return",
                           (uint64_t)(uint32_t)rc_b, "return");
    }
    int fcntl_rc = -1;
    if (f.is_payload && f.fcntl != NULL) {
        long flags = f.fcntl(s, 3 /* F_GETFL */, 0);
        if (flags >= 0) {
            fcntl_rc = f.fcntl(s, 4 /* F_SETFL */, flags | 0x0004 /* O_NONBLOCK */);
            obs_report_measure("102-net/nonblocking-option", "fcntl",
                               fcntl_rc == 0 ? "O_NONBLOCK-accepted"
                                             : "O_NONBLOCK-return",
                               (uint64_t)(uint32_t)fcntl_rc, "return");
            obs_report_measure("102-net/posix-fcntl", "fcntl", "F_SETFL",
                               (uint64_t)(uint32_t)fcntl_rc, "return");
        }
    }
    net_close_wrapper(&f, s);
    if (rc_a == 0) {
        return obs_pass_value(OBS_NET_SO_NBIO_OPENORBIS);
    }
    if (f.is_payload && fcntl_rc == 0) {
        return obs_pass_value(0x0004 /* O_NONBLOCK */);
    }
    if (tried_b && rc_b == 0) {
        return obs_partial_value("0x1200 refused; 0x1100 accepted",
                                 OBS_NET_SO_NBIO_VITASDK);
    }
    return obs_fail_code("neither non-blocking option value was accepted",
                         (uint64_t)(uint32_t)rc_a);
}

/* ---- Ask 2.3: the do-not-wait flag and the would-block code
 * ------------------------------ */

static uint64_t net_get_process_time(void) {
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        return sceKernelGetProcessTime();
    }
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        char nid[12];
        obs_compute_nid("sceKernelGetProcessTime", nid);
        const void *fn =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (fn != NULL && obs_address_is_callable(fn)) {
            return ((uint64_t (*)(void))fn)();
        }
    }
    return 0;
}

static int net_has_clock(void) {
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        return 1;
    }
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        char nid[12];
        obs_compute_nid("sceKernelGetProcessTime", nid);
        const void *fn =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (fn != NULL && obs_address_is_callable(fn)) {
            return 1;
        }
    }
    return 0;
}

static void net_usleep(unsigned int usec) {
    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        sceKernelUsleep(usec);
        return;
    }
    const payload_args_t *pargs = obs_get_payload_args();
    if (pargs != NULL && pargs->kexport_table != NULL) {
        char nid[12];
        obs_compute_nid("sceKernelUsleep", nid);
        const void *fn =
            obs_kexport_lookup((const obs_kexport_table_t *)pargs->kexport_table, nid);
        if (fn != NULL && obs_address_is_callable(fn)) {
            ((int (*)(unsigned int))fn)(usec);
            return;
        }
    }
    volatile uint64_t spin = 0;
    for (uint64_t i = 0; i < (uint64_t)usec * 100u; i++) {
        spin += i;
    }
    (void)spin;
}

static obs_result check_net_recv_would_block(void) {
    net_fns f;
    if (!net_resolve_all(&f) || !net_has_clock()) {
        return obs_skip("socket recv or the process clock did not resolve in this leg");
    }
    static unsigned char buf[64];
    int lst = net_open_socket(&f, "obscene");
    if (lst < 0) {
        return obs_fail_code("no socket for the would-block read",
                             (uint64_t)(uint32_t)lst);
    }
    obs_net_sockaddr_in addr;
    net_fill_addr(&addr, OBS_NET_PORT_RECV, 0u, (uint8_t)sizeof(addr));
    (void)net_bind_wrapper(&f, lst, &addr, (uint32_t)sizeof(addr));
    (void)net_listen_wrapper(&f, lst, 1);

    /* Part 1: MSG_DONTWAIT recv on idle listener */
    uint64_t t0 = net_get_process_time();
    int lrc = net_recv_wrapper(&f, lst, buf, sizeof buf, OBS_NET_MSG_DONTWAIT);
    uint64_t lel = net_get_process_time() - t0;
    obs_report_measure("102-net/recv-would-block", f.is_payload ? "recv" : "sceNetRecv",
                       "listener-return", (uint64_t)(uint32_t)lrc, "raw-code");
    obs_report_measure("102-net/recv-would-block", f.is_payload ? "recv" : "sceNetRecv",
                       lel < 10000u ? "listener-prompt" : "listener-blocked", lel,
                       "microseconds");
    if (f.is_payload) {
        int lerr = net_last_errno(&f, lrc);
        obs_report_measure("102-net/recv-would-block", "errno", "listener-errno",
                           (uint64_t)(uint32_t)lerr, "errno");
    }

    /* Part 2: connected non-blocking recv with nothing pending */
    int connected_rc = 0;
    int have_connected = 0;
    int connected_err = 0;
    int cli = net_open_socket(&f, "obscene");
    if (cli >= 0) {
        obs_net_sockaddr_in dst;
        net_fill_addr(&dst, OBS_NET_PORT_RECV, OBS_NET_LOOPBACK, (uint8_t)sizeof(dst));
        (void)net_connect_wrapper(&f, cli, &dst, (uint32_t)sizeof(dst));
        int acc = -1;
        for (int i = 0; i < 20 && acc < 0; i++) {
            acc = net_accept_wrapper(&f, lst, 0, 0);
            if (acc < 0) {
                net_usleep(50000);
            }
        }
        if (acc >= 0) {
            uint64_t t1 = net_get_process_time();
            connected_rc =
                net_recv_wrapper(&f, acc, buf, sizeof buf, OBS_NET_MSG_DONTWAIT);
            uint64_t cel = net_get_process_time() - t1;
            have_connected = 1;
            obs_report_measure("102-net/recv-would-block",
                               f.is_payload ? "recv" : "sceNetRecv", "connected-return",
                               (uint64_t)(uint32_t)connected_rc, "raw-code");
            obs_report_measure("102-net/recv-would-block",
                               f.is_payload ? "recv" : "sceNetRecv",
                               cel < 10000u ? "connected-prompt" : "connected-blocked",
                               cel, "microseconds");
            if (f.is_payload) {
                connected_err = net_last_errno(&f, connected_rc);
                obs_report_measure("102-net/recv-would-block", "errno",
                                   "connected-errno", (uint64_t)(uint32_t)connected_err,
                                   "errno");
                obs_report_measure("102-net/posix-would-block", "__error", "errno",
                                   (uint64_t)(uint32_t)connected_err, "errno");
            }
            net_close_wrapper(&f, acc);
        }
        net_close_wrapper(&f, cli);
    }
    net_close_wrapper(&f, lst);

    if (have_connected) {
        if (connected_rc >= 0) {
            return obs_partial_value(
                "connected recv returned data or zero, not would-block",
                (uint64_t)(uint32_t)connected_rc);
        }
        if (f.is_payload && connected_err == 35 /* EAGAIN */) {
            return obs_pass_value((uint64_t)(uint32_t)connected_err);
        }
        return obs_pass_value((uint64_t)(uint32_t)connected_rc);
    }
    if (lrc >= 0) {
        return obs_partial_value("listener recv returned data or zero",
                                 (uint64_t)(uint32_t)lrc);
    }
    return obs_pass_value((uint64_t)(uint32_t)lrc);
}

/* ---- Ask 2.4: does an accepted socket inherit the listener's non-blocking mode?
 * ---------- */

typedef struct {
    volatile int done;
    const net_fns *f;
    volatile int listener;
    volatile int client;
    volatile int accepted;
} obs_net_watchdog_t;

static void *obs_net_accept_watchdog(void *arg) {
    obs_net_watchdog_t *w = (obs_net_watchdog_t *)arg;
    for (int i = 0; i < 20; i++) {
        if (w->done) {
            return NULL;
        }
        if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(50000); /* 50ms * 20 = 1000ms max */
        }
    }
    if (!w->done) {
        int a = w->accepted;
        w->accepted = -1;
        if (a >= 0) {
            net_close_wrapper(w->f, a);
        }
        int c = w->client;
        w->client = -1;
        if (c >= 0) {
            net_close_wrapper(w->f, c);
        }
        int l = w->listener;
        w->listener = -1;
        if (l >= 0) {
            net_close_wrapper(w->f, l);
        }
    }
    return NULL;
}

static obs_result check_net_accept_inherits(void) {
    net_fns f;
    if (!net_resolve_all(&f)) {
        return obs_skip("socket calls did not resolve in this leg");
    }
    int listener = net_open_socket(&f, "obscene");
    if (listener < 0) {
        return obs_fail_code("no listener socket", (uint64_t)(uint32_t)listener);
    }
    obs_net_sockaddr_in addr;
    net_fill_addr(&addr, OBS_NET_PORT_ACCEPT, 0u, (uint8_t)sizeof(addr));
    if (net_bind_wrapper(&f, listener, &addr, (uint32_t)sizeof(addr)) != 0 ||
        net_listen_wrapper(&f, listener, 1) != 0) {
        net_close_wrapper(&f, listener);
        return obs_fail("the listener would not bind or listen");
    }
    net_set_nonblocking(&f, listener);

    /* Timeout guard on listener so accept cannot block indefinitely */
    struct {
        int64_t tv_sec;
        int64_t tv_usec;
    } rcvtimeo16 = {0, 200000}; /* 200ms */
    int rrc16 = net_setsockopt_wrapper(&f, listener, OBS_NET_SOL_SOCKET,
                                       0x1006 /* SO_RCVTIMEO */, &rcvtimeo16,
                                       (uint32_t)sizeof(rcvtimeo16));
    if (rrc16 != 0) {
        struct {
            int32_t tv_sec;
            int32_t tv_usec;
        } rcvtimeo8 = {0, 200000};
        (void)net_setsockopt_wrapper(&f, listener, OBS_NET_SOL_SOCKET,
                                     0x1006 /* SO_RCVTIMEO */, &rcvtimeo8,
                                     (uint32_t)sizeof(rcvtimeo8));
    }

    obs_net_watchdog_t w;
    w.done = 0;
    w.f = &f;
    w.listener = listener;
    w.client = -1;
    w.accepted = -1;

    ScePthread watchdog_th;
    int have_watchdog = 0;
    if (obs_address_is_callable((const void *)&scePthreadCreate)) {
        have_watchdog = (scePthreadCreate(&watchdog_th, NULL, obs_net_accept_watchdog,
                                          &w, "obs-net-wd") == 0);
    }

    /* Loopback self-connect */
    int client = net_open_socket(&f, "obscene");
    if (client < 0) {
        w.done = 1;
        if (have_watchdog && obs_address_is_callable((const void *)&scePthreadJoin)) {
            scePthreadJoin(watchdog_th, NULL);
        }
        net_close_wrapper(&f, listener);
        return obs_pending("no client socket for the self-connect; re-run");
    }
    w.client = client;

    obs_net_sockaddr_in dst;
    net_fill_addr(&dst, OBS_NET_PORT_ACCEPT, OBS_NET_LOOPBACK, (uint8_t)sizeof(dst));
    int conn_rc = net_connect_wrapper(&f, client, &dst, (uint32_t)sizeof(dst));
    obs_report_measure("102-net/accept-inherits", "connect", "rc",
                       (uint64_t)(uint32_t)conn_rc, "rc");
    if (conn_rc != 0) {
        w.done = 1;
        if (have_watchdog && obs_address_is_callable((const void *)&scePthreadJoin)) {
            scePthreadJoin(watchdog_th, NULL);
        }
        if (w.client >= 0) {
            net_close_wrapper(&f, client);
            w.client = -1;
        }
        if (w.listener >= 0) {
            net_close_wrapper(&f, listener);
            w.listener = -1;
        }
        return obs_fail_code("the client could not connect to the listener",
                             (uint64_t)(uint32_t)conn_rc);
    }

    int accepted = -1;
    for (int i = 0; i < 20 && accepted < 0; i++) {
        accepted = net_accept_wrapper(&f, listener, 0, 0);
        if (accepted < 0 && obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(50000);
        }
    }
    if (accepted < 0) {
        w.done = 1;
        if (have_watchdog && obs_address_is_callable((const void *)&scePthreadJoin)) {
            scePthreadJoin(watchdog_th, NULL);
        }
        if (w.client >= 0) {
            net_close_wrapper(&f, client);
            w.client = -1;
        }
        if (w.listener >= 0) {
            net_close_wrapper(&f, listener);
            w.listener = -1;
        }
        return obs_pending(
            "the self-connect did not complete; loopback may be closed here");
    }
    w.accepted = accepted;
    obs_report_measure("102-net/accept-inherits", "accept", "accepted-fd",
                       (uint64_t)(uint32_t)accepted, "fd");

    if (f.is_payload && f.fcntl != NULL) {
        long aflags = f.fcntl(accepted, 3 /* F_GETFL */, 0);
        obs_report_measure("102-net/posix-accept", "fcntl", "F_GETFL",
                           (uint64_t)(uint32_t)aflags, "flags");
    }

    /* Guard against indefinite blocking if accepted socket did not inherit non-blocking
     * mode. Standard 64-bit FreeBSD/PS5 struct timeval is 16 bytes (int64_t tv_sec,
     * tv_usec). Try 16-byte first, fallback to 8-byte if rejected. */
    struct {
        int64_t tv_sec;
        int64_t tv_usec;
    } sndtimeo16 = {0, 100000}; /* 100ms timeout */
    int so_rc16 = net_setsockopt_wrapper(&f, accepted, OBS_NET_SOL_SOCKET,
                                         0x1005 /* SO_SNDTIMEO */, &sndtimeo16,
                                         (uint32_t)sizeof(sndtimeo16));
    if (so_rc16 != 0) {
        struct {
            int32_t tv_sec;
            int32_t tv_usec;
        } sndtimeo8 = {0, 100000}; /* 100ms timeout */
        (void)net_setsockopt_wrapper(&f, accepted, OBS_NET_SOL_SOCKET,
                                     0x1005 /* SO_SNDTIMEO */, &sndtimeo8,
                                     (uint32_t)sizeof(sndtimeo8));
    }
    obs_report_measure("102-net/accept-inherits", "setsockopt", "SO_SNDTIMEO-rc",
                       (uint64_t)(uint32_t)so_rc16, "rc");

    /* Send until write buffer saturation to definitively test non-blocking inheritance
     * vs blocking */
    static unsigned char big[65536];
    int total_sent = 0;
    int last_rc = 0;
    uint64_t t0 = 0;
    uint64_t elapsed_us = 0;
    int sends_count = 0;

    for (int iter = 0; iter < 50; iter++) {
        if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
            t0 = sceKernelGetProcessTime();
        }
        last_rc = net_send_wrapper(&f, accepted, big, sizeof(big), 0);
        if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
            elapsed_us = sceKernelGetProcessTime() - t0;
        }
        sends_count++;
        if (last_rc <= 0) {
            break;
        }
        total_sent += last_rc;
    }

    w.done = 1;
    if (have_watchdog && obs_address_is_callable((const void *)&scePthreadJoin)) {
        scePthreadJoin(watchdog_th, NULL);
    }

    obs_report_measure("102-net/accept-inherits", f.is_payload ? "send" : "sceNetSend",
                       last_rc < 0 ? "would-block-code" : "sent",
                       (uint64_t)(uint32_t)last_rc, "raw-code");
    obs_report_measure("102-net/accept-inherits", "send", "total-sent",
                       (uint64_t)total_sent, "bytes");
    obs_report_measure("102-net/accept-inherits", "send", "sends-until-saturated",
                       (uint64_t)sends_count, "count");
    obs_report_measure("102-net/accept-inherits", "send",
                       elapsed_us < 10000u ? "saturation-prompt" : "saturation-waited",
                       elapsed_us, "microseconds");

    if (f.is_payload && last_rc < 0) {
        int err = net_last_errno(&f, last_rc);
        obs_report_measure("102-net/accept-inherits", "errno", "saturation-errno",
                           (uint64_t)(uint32_t)err, "errno");
    }

    if (w.accepted >= 0) {
        net_close_wrapper(&f, accepted);
        w.accepted = -1;
    }
    if (w.client >= 0) {
        net_close_wrapper(&f, client);
        w.client = -1;
    }
    if (w.listener >= 0) {
        net_close_wrapper(&f, listener);
        w.listener = -1;
    }

    if (last_rc < 0) {
        return obs_pass_value((uint64_t)(uint32_t)last_rc);
    }
    return obs_partial_value("buffer saturated without would-block",
                             (uint64_t)total_sent);
}

/* ---- Ask 3: the sockaddr layout
 * ---------------------------------------------------------- */

static obs_result check_net_sockaddr_bind(void) {
    net_fns f;
    if (!net_resolve_all(&f)) {
        return obs_skip("socket calls did not resolve in this leg");
    }
    int with_len = -1;
    int without_len = -1;
    int s1 = net_open_socket(&f, "obscene");
    if (s1 >= 0) {
        obs_net_sockaddr_in a;
        net_fill_addr(&a, OBS_NET_PORT_SOCKADDR_LEN16, 0u, (uint8_t)sizeof(a));
        with_len = net_bind_wrapper(&f, s1, &a, (uint32_t)sizeof(a));
        net_close_wrapper(&f, s1);
    }
    int s2 = net_open_socket(&f, "obscene");
    if (s2 >= 0) {
        obs_net_sockaddr_in a;
        net_fill_addr(&a, OBS_NET_PORT_SOCKADDR_LEN0, 0u, 0u); /* sin_len = 0 */
        without_len = net_bind_wrapper(&f, s2, &a, (uint32_t)sizeof(a));
        net_close_wrapper(&f, s2);
    }
    obs_report_measure("102-net/sockaddr-bind", f.is_payload ? "bind" : "sceNetBind",
                       with_len == 0 ? "sin_len-16-bound" : "sin_len-16-refused",
                       (uint64_t)(uint32_t)with_len, "return");
    obs_report_measure("102-net/sockaddr-bind", f.is_payload ? "bind" : "sceNetBind",
                       without_len == 0 ? "sin_len-0-bound" : "sin_len-0-refused",
                       (uint64_t)(uint32_t)without_len, "return");
    if (with_len != 0 && without_len != 0) {
        return obs_fail_code("bind refused both sockaddr lengths",
                             (uint64_t)(uint32_t)with_len);
    }
    if (with_len != 0) {
        return obs_partial_value("bind refused sin_len=16 but accepted sin_len=0",
                                 (uint64_t)(uint32_t)with_len);
    }
    if (without_len != 0) {
        return obs_pass_value(16);
    }
    return obs_pass_value(0);
}

/* ---- Ask 4: weak POSIX socket symbols binding at load (REQ-20260909T1315Z-ed26)
 * --------- */

static obs_result check_net_weak_bind(void) {
    if (!net_is_payload_mode()) {
        return obs_skip(
            "payload leg only: measures elfldr dynamic relocation of weak references");
    }

    size_t count = 0;
    const obs_loader_weak_entry_t *entries = obs_get_loader_weak_entries(&count);
    if (entries == NULL || count == 0) {
        return obs_skip("loader weak entry table not available");
    }

    unsigned int bound_count = 0;
    for (size_t i = 0; i < count; i++) {
        const obs_loader_weak_entry_t *e = &entries[i];
        obs_report_measure("102-net/weak-bind", e->name,
                           e->is_bound ? "bound" : "unresolved", e->is_bound ? 1 : 0,
                           "flag");
        obs_report_measure("102-net/weak-bind", e->name, "loader-got", e->initial_got,
                           "vaddr");
        if (e->is_bound) {
            bound_count++;
        }
    }

    if (bound_count == count) {
        return obs_pass_value((uint64_t)bound_count);
    }
    return obs_partial_value(
        "some weak undefined libkernel symbols remained unresolved at load",
        (uint64_t)bound_count);
}

/* ---- Ask 5: payload socket serving end-to-end on port 9899 (REQ-20260909T1315Z-04b6)
 * ----- */

static obs_result check_net_payload_serve(void) {
    if (!net_is_payload_mode()) {
        return obs_skip(
            "payload leg only: tests unsigned payload socket serve on port 9899");
    }

    /* 1. Report resolution route for each socket symbol: loader-bound weak ref vs
     * export-table */
    size_t weak_count = 0;
    const obs_loader_weak_entry_t *weak_entries =
        obs_get_loader_weak_entries(&weak_count);
    const payload_args_t *pargs = obs_get_payload_args();

    for (size_t i = 0; i < weak_count; i++) {
        const char *name = weak_entries[i].name;
        const char *route = "unresolved";
        uint64_t route_addr = 0;
        if (weak_entries[i].is_bound && weak_entries[i].initial_got != 0) {
            route = "loader-bound";
            route_addr = weak_entries[i].initial_got;
        } else if (pargs != NULL && pargs->kexport_table != NULL) {
            char nid[12];
            obs_compute_nid(name, nid);
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                route = "export-table";
                route_addr = (uint64_t)(uintptr_t)ka;
            }
        }
        obs_report_measure("102-net/symbol-route", name, route, route_addr, "vaddr");
    }

    /* 2. Open listener on scratch port 9899, accept, recv, echo, close */
    net_fns f;
    if (!net_resolve_all(&f)) {
        return obs_fail("POSIX socket calls did not resolve in payload mode");
    }

    int lst = net_open_socket(&f, "obscene-9899");
    obs_report_measure("102-net/payload-serve", "socket",
                       lst >= 0 ? "opened" : "refused", (uint64_t)(uint32_t)lst, "fd");
    if (lst < 0) {
        return obs_fail_code("could not open socket for port 9899",
                             (uint64_t)(uint32_t)lst);
    }

    obs_net_sockaddr_in a;
    net_fill_addr(&a, 9899, 0u, (uint8_t)sizeof(a));
    int brc = net_bind_wrapper(&f, lst, &a, (uint32_t)sizeof(a));
    obs_report_measure("102-net/payload-serve", "bind", brc == 0 ? "bound" : "refused",
                       (uint64_t)(uint32_t)brc, "rc");

    int lrc = net_listen_wrapper(&f, lst, 5);
    obs_report_measure("102-net/payload-serve", "listen",
                       lrc == 0 ? "listening" : "refused", (uint64_t)(uint32_t)lrc,
                       "rc");

    if (brc != 0 || lrc != 0) {
        net_close_wrapper(&f, lst);
        return obs_fail("bind or listen refused on port 9899");
    }

    net_set_nonblocking(&f, lst);

    /* Accept connection from host sweep runner (wait up to 10 seconds) */
    int acc = -1;
    for (int i = 0; i < 100 && acc < 0; i++) {
        acc = net_accept_wrapper(&f, lst, NULL, NULL);
        if (acc < 0 && obs_address_is_callable((const void *)&sceKernelUsleep)) {
            sceKernelUsleep(100000); /* 100ms */
        }
    }
    obs_report_measure("102-net/payload-serve", "accept",
                       acc >= 0 ? "accepted" : "refused", (uint64_t)(uint32_t)acc,
                       "rc");

    long nrecv = -1;
    long nsent = -1;
    if (acc >= 0) {
        char echo_buf[512];
        for (unsigned int b = 0; b < sizeof(echo_buf); b++) {
            echo_buf[b] = 0;
        }
        for (int r = 0; r < 50 && nrecv <= 0; r++) {
            nrecv = net_recv_wrapper(&f, acc, echo_buf, sizeof(echo_buf), 0);
            if (nrecv < 0 && obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(50000);
            }
        }
        obs_report_measure("102-net/payload-serve", "recv", "bytes-read",
                           (uint64_t)(nrecv >= 0 ? nrecv : 0), "bytes");

        if (nrecv > 0) {
            nsent = net_send_wrapper(&f, acc, echo_buf, (size_t)nrecv, 0);
        }
        obs_report_measure("102-net/payload-serve", "send", "bytes-echoed",
                           (uint64_t)(nsent >= 0 ? nsent : 0), "bytes");

        int c_acc = net_close_wrapper(&f, acc);
        obs_report_measure("102-net/payload-serve", "close-client", "rc",
                           (uint64_t)(uint32_t)c_acc, "rc");
    }

    int c_lst = net_close_wrapper(&f, lst);
    obs_report_measure("102-net/payload-serve", "close-listener", "rc",
                       (uint64_t)(uint32_t)c_lst, "rc");

    if (acc >= 0 && nrecv > 0 && nsent > 0) {
        return obs_pass_value((uint64_t)nsent);
    }
    return obs_partial_value("port 9899 listened; incoming connection not completed",
                             (uint64_t)(uint32_t)acc);
}

/* The address is the check function, not a libSceNet symbol, so the harness runs each
 * check even where the direct import is unbound - which in payload mode is every one of
 * them. The check then resolves through direct import, dlsym gadget, or kernel export
 * table. */
static const obs_check net_checks[] = {
    {"102-net/resolve", "libSceNet", "sceNetSocket", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_resolve, check_net_resolve, OBS_FROM_ASSUMED},
    {"102-net/listener", "libSceNet", "sceNetListen", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_listener, check_net_listener, OBS_FROM_ASSUMED},
    {"102-net/nonblocking-option", "libSceNet", "sceNetSetsockopt", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_net_nonblocking_option,
     check_net_nonblocking_option, OBS_FROM_ASSUMED},
    {"102-net/recv-would-block", "libSceNet", "sceNetRecv", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_recv_would_block, check_net_recv_would_block,
     OBS_FROM_ASSUMED},
    {"102-net/accept-inherits", "libSceNet", "sceNetAccept", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_accept_inherits, check_net_accept_inherits,
     OBS_FROM_ASSUMED},
    {"102-net/sockaddr-bind", "libSceNet", "sceNetBind", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_sockaddr_bind, check_net_sockaddr_bind, OBS_FROM_ASSUMED},
    {"102-net/weak-bind", "libkernel", "bind", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_weak_bind, check_net_weak_bind, OBS_FROM_ASSUMED},
    {"102-net/payload-serve", "libkernel", "listen", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)check_net_payload_serve, check_net_payload_serve, OBS_FROM_ASSUMED},
};

const obs_section obs_section_net = {
    "102-net",
    "Network reachability",
    "Resolves libSceNet for title mode and POSIX socket calls for payload mode, "
    "reports each "
    "address, measures socket creation (__sys_socketex and syscall 97), the "
    "non-blocking option, "
    "the would-block code, and accept inheritance for Porthole.",
    net_checks,
    OBS_COUNT(net_checks),
};
