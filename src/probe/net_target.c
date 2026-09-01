/*
 * The command socket's backend on the console, over libSceNet.
 *
 * # This was a refusal, and now it is not
 *
 * It began as a deliberate stub (D126): the vendor's networking signatures were
 * unconfirmed, and D008 forbids calling a function whose arity is uncertain because a
 * wrong one corrupts the stack and surfaces far from the cause. That reasoning was
 * sound and the conclusion has since changed, because the premise did: the signatures
 * are now confirmed from two independent public sources that agree exactly - the
 * OpenOrbis toolchain headers and shadPS4's own libSceNet - and are declared in
 * platform.h. See D107.
 *
 * # Why this can be tested with no console
 *
 * An emulator whose net layer maps guest sockets onto host sockets makes a guest listen
 * open a real host port. shadPS4 does exactly this - `PosixSocket::bind` calls the host
 * `::bind`, `::listen` calls `::listen` - so obSCEne running inside shadPS4 and calling
 * `sceNetListen` opens a port the driver connects to from outside, no hardware in the
 * loop. That is what turns "networking needs a Steam Deck" into "networking needs the
 * emulator we already run in".
 *
 * # Guarded like everything else
 *
 * Every symbol is weak. `obs_net_backend_available` reports the transport present only
 * when the calls it needs actually resolve, so a build on a loader without libSceNet
 * announces no socket rather than faulting - the same honesty the rest of the program
 * keeps.
 */

#include "obscene/harness.h"
#include "obscene/net.h"
#include "obscene/platform.h"

int obs_net_backend_available(void) {
    /* Every call the transport makes, tested before any is made. Announcing a socket
     * this build cannot actually open would put a capability on the wire that then
     * fails, which is the confusion the whole protocol is arranged to avoid. */
    return obs_address_is_callable((const void *)&sceNetSocket) &&
           obs_address_is_callable((const void *)&sceNetBind) &&
           obs_address_is_callable((const void *)&sceNetListen) &&
           obs_address_is_callable((const void *)&sceNetAccept) &&
           obs_address_is_callable((const void *)&sceNetRecv) &&
           obs_address_is_callable((const void *)&sceNetSend) &&
           obs_address_is_callable((const void *)&sceNetSocketClose);
}

const char *obs_net_backend_name(void) {
    return "scenet";
}

/* Host byte order to network, by hand.
 *
 * `sceNetHtons` exists and could do this, but it is one more symbol that must resolve
 * for a listen to succeed, and the swap is two lines. Freestanding and self-contained
 * beats one more dependency that can be null. The target is little-endian x86, so the
 * swap is unconditional. */
static uint16_t obs_net_htons(uint16_t host) {
    return (uint16_t)((host << 8) | (host >> 8));
}

int obs_net_backend_listen(unsigned short port) {
    /* Best-effort, and its result is deliberately ignored. On a live platform the
     * network stack is already up and this returns "already initialised"; on one where
     * it matters it does the work. Either way a failure here shows up as the socket
     * call below failing, which is where it is handled. */
    if (obs_address_is_callable((const void *)&sceNetInit)) {
        (void)sceNetInit();
    }

    /* The name argument is the platform's, for its own debugging - not a host concept,
     * and confirmed present by both sources. */
    int s = sceNetSocket("obscene", OBS_NET_AF_INET, OBS_NET_SOCK_STREAM,
                         OBS_NET_IPPROTO_TCP);
    if (s < 0) {
        return -1;
    }

    obs_net_sockaddr_in addr;
    for (unsigned int i = 0; i < sizeof(addr); i++) {
        ((volatile unsigned char *)&addr)[i] = 0;
    }
    addr.sin_len = (uint8_t)sizeof(addr);
    addr.sin_family = OBS_NET_AF_INET;
    addr.sin_port = obs_net_htons((uint16_t)port);
    addr.sin_addr =
        0; /* INADDR_ANY: every interface, so the host reaches it on loopback */

    if (sceNetBind(s, &addr, (uint32_t)sizeof(addr)) < 0 || sceNetListen(s, 1) < 0) {
        (void)sceNetSocketClose(s);
        return -1;
    }
    return s;
}

int obs_net_backend_accept(int listener) {
    /* The peer address is not wanted - the driver already knows who it is - so both
     * out-parameters are null, which the confirmed signature permits. */
    return sceNetAccept(listener, 0, 0);
}

long obs_net_backend_recv(int connection, char *bytes, size_t len) {
    return (long)sceNetRecv(connection, bytes, (uint64_t)len, 0);
}

long obs_net_backend_send(int connection, const char *bytes, size_t len) {
    /* All of it or a failure. A partial send reported as success would truncate a
     * record mid-line, and a truncated record parses - worse than a missing one. */
    size_t sent = 0;
    while (sent < len) {
        int n = sceNetSend(connection, bytes + sent, (uint64_t)(len - sent), 0);
        if (n <= 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return (long)len;
}

void obs_net_backend_close(int handle) {
    (void)sceNetSocketClose(handle);
}

#if defined(OBS_NET_ESCAPE)
/* The console cannot honour this yet, and says so rather than guessing.
 *
 * Executing a blob needs a page mapped both writable-then-executable, and the vendor's
 * memory calls that could do it (sceKernelMmap and the JIT allocators) have unconfirmed
 * arities and flag constants - exactly what D008 forbids calling on a guess, because a
 * wrong one corrupts state far from here. So `run` returns `unsupported` on the
 * console, which is the honest answer and more useful than a map that half-works. When
 * the signatures are confirmed the way libSceNet's were (D107), this becomes real;
 * until then it refuses. */
int obs_net_backend_exec(const unsigned char *code, size_t len, const uint64_t *args,
                         unsigned int argc, uint64_t *result) {
    (void)code;
    (void)len;
    (void)args;
    (void)argc;
    (void)result;
    return -1;
}
#endif

/* Entropy, best-effort, and the "best" is doing a lot of work in that sentence.
 *
 * # What is actually available here
 *
 * A freestanding console module has no `/dev/urandom`, no RDRAND this program is
 * willing to assume, and no key storage. What it has is a timestamp counter and an
 * address space. So this mixes three sources, none of which is sufficient alone:
 *
 * - **Timing jitter**, which is the only real entropy in the list. The absolute value
 * of the TSC at startup is roughly uptime times frequency and an attacker can estimate
 * it; the *differences* between successive reads across a variable amount of work
 * cannot be estimated, because they depend on cache state, interrupts and clock
 * behaviour. The low bits of many such differences are the substance here.
 * - **A stack address**, which carries whatever ASLR the platform applies.
 * - **Process time**, as a third independent counter.
 *
 * # Say plainly what this is not
 *
 * It is not a cryptographic random number generator and nothing here should be
 * described as one. A realistic estimate is tens of bits, not 128, and the value it
 * produces is unique per startup rather than unpredictable to an adversary with precise
 * timing information.
 *
 * That is proportionate to the threat it exists for: another device on the same network
 * connecting and issuing commands. It is **not** proportionate to an adversary who can
 * observe the link, and the socket is cleartext anyway, so such an adversary reads the
 * secret off the wire and never has to guess it. See docs/PROTOCOL.md.
 */
static uint64_t obs_net_mix(uint64_t x) {
    /* splitmix64's finaliser. Chosen because it is short, has no state, needs no
     * library and is well studied as an avalanche function - a weak mixer would let the
     * structure in the inputs (a counter that mostly counts) survive into the output.
     */
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

int obs_net_backend_entropy(unsigned char *out, unsigned int len) {
    if (out == NULL || len == 0) {
        return -1;
    }
    /* The address of a local: whatever the platform's layout randomisation gives. */
    volatile unsigned char anchor = 0;
    uint64_t pool = (uint64_t)(uintptr_t)&anchor;

    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        pool = obs_net_mix(pool ^ sceKernelGetProcessTime());
    }

    if (obs_address_is_callable((const void *)&sceKernelReadTsc)) {
        /* Sixty-four rounds, each folding in the *difference* since the last read
         * rather than the reading itself. The spin length varies with the pool, so the
         * work between samples is not the same twice and the differences do not settle
         * into a pattern. */
        uint64_t previous = sceKernelReadTsc();
        for (unsigned int round = 0; round < 64u; round++) {
            volatile uint64_t spin = 0;
            unsigned int work = (unsigned int)(pool & 0x3Fu) + 16u;
            for (unsigned int i = 0; i < work; i++) {
                spin = spin + i;
            }
            uint64_t now = sceKernelReadTsc();
            pool = obs_net_mix(pool ^ (now - previous));
            previous = now;
        }
    } else {
        /* No counter at all. Refused rather than filled with the anchor address alone:
         * a secret derived from one predictable value is worse than none, because it
         * looks like protection. The caller reports that it could not be generated. */
        return -1;
    }

    for (unsigned int i = 0; i < len; i++) {
        if ((i & 7u) == 0u) {
            pool = obs_net_mix(pool);
        }
        out[i] = (unsigned char)((pool >> ((i & 7u) * 8u)) & 0xFFu);
    }
    return 0;
}
