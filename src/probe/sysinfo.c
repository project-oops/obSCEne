/*
 * Platform facts for the HUD. See obscene/sysinfo.h for the philosophy: `unknown` is a
 * first-class, honest answer, and a value appears only when read through a confirmed
 * signature.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sysinfo.h"

static unsigned int obs_listening_port;
static const char *obs_session_secret;

void obs_sysinfo_set_listening(unsigned int port) {
    obs_listening_port = port;
}

/* Held by pointer, not copied. The string lives in `net.c` for the life of the process and
 * copying it would put a second one in a runtime that has no allocator and no need. */
void obs_sysinfo_set_secret(const char *secret) {
    obs_session_secret = secret;
}

const char *obs_sysinfo_label(obs_sys_field field) {
    switch (field) {
    case OBS_SYS_LISTENING:
        return "NET";
    case OBS_SYS_SECRET:
        return "KEY";
    case OBS_SYS_IP:
        return "IP";
    case OBS_SYS_FIRMWARE:
        return "FW";
    case OBS_SYS_GENERATION:
        return "GEN";
    case OBS_SYS_GPU:
        return "GPU";
    case OBS_SYS_MEMORY:
        return "MEM";
    case OBS_SYS_VRAM:
        return "VRAM";
    case OBS_SYS_TEMP:
        return "TEMP";
    case OBS_SYS_STORAGE:
        return "DISK";
#if OBSCENE_GEN == 4
    case OBS_SYS_PS4_FIRMWARE:
        return "PS4FW";
#endif
    case OBS_SYS_COUNT:
    default:
        return "?";
    }
}

/* Copies a string into buf, bounded, NUL-terminated. Returns the length written. */
static size_t obs_put(char *buf, size_t n, const char *text) {
    size_t i = 0;
    while (text[i] != '\0' && i + 1 < n) {
        buf[i] = text[i];
        i++;
    }
    buf[i] = '\0';
    return i;
}

/* Appends "<value>M" for a byte count, in mebibytes, integer-only (no float in the
 * runtime, CLAUDE.md principle 8). Whole mebibytes are enough to read a pool size by. */
static void obs_put_mib(char *buf, size_t n, uint64_t bytes) {
    size_t at = 0;
    if (n >= OBS_NUM_MAX + 2) {
        at += obs_format_u64(buf + at, bytes >> 20);
    }
    at += obs_put(buf + at, n - at, "M");
    buf[at] = '\0';
}

obs_sys_state obs_sysinfo_value(obs_sys_field field, char *buf, size_t n) {
    /* The default every field falls back to. A getter that cannot answer leaves this in
     * place, so "unknown" is never something a caller has to remember to write. */
    obs_put(buf, n, "unknown");

    switch (field) {
    case OBS_SYS_LISTENING:
        if (obs_listening_port == 0) {
            return OBS_SYS_ABSENT;
        } else {
            /* The bind is on every interface; the port is the fact a driver needs, and on
             * an emulator that maps to the host the connect address is host loopback. The
             * machine's own address is the separate IP field. */
            size_t at = obs_put(buf, n, "0.0.0.0:");
            at += obs_format_u64(buf + at, obs_listening_port);
            buf[at] = '\0';
            return OBS_SYS_KNOWN;
        }

    case OBS_SYS_SECRET:
        if (obs_session_secret == NULL || obs_session_secret[0] == '\0') {
            /* Not serving, or the platform could not supply entropy. `ABSENT` rather than an
             * empty value, because "there is no secret" is something an operator needs to see
             * on the screen: it means anything on the network can drive this. */
            return OBS_SYS_ABSENT;
        } else {
            size_t at = obs_put(buf, n, obs_session_secret);
            buf[at] = '\0';
            return OBS_SYS_KNOWN;
        }

    case OBS_SYS_IP: {
        /* The console's own address, from the network configuration.
         *
         * `sceNetCtlGetInfo(14, info)` writes the IP address string at offset 0 of the info
         * union (code 14 = IP_ADDRESS, OpenOrbis). The buffer is over-sized well past the
         * union's largest member so the call cannot overrun it, and only the string is read.
         * `sceNetCtlInit` is called first and its result ignored - it is idempotent, and this
         * program already brings the net stack up for its report server. */
        if (!obs_address_is_callable((const void *)&sceNetCtlGetInfo)) {
            return OBS_SYS_ABSENT;
        }
        if (obs_address_is_callable((const void *)&sceNetCtlInit)) {
            (void)sceNetCtlInit();
        }
        unsigned char info[320];
        for (size_t i = 0; i < sizeof(info); i++) {
            info[i] = 0u;
        }
        if (sceNetCtlGetInfo(14, info) != 0) {
            /* Present and called, and it declined - unknown rather than absent. Happens when
             * the console is offline, which is itself a fact and not a fault. */
            return OBS_SYS_UNCONFIRMED;
        }
        const char *ip = (const char *)info;
        if (ip[0] == '\0') {
            return OBS_SYS_UNCONFIRMED;
        }
        obs_put(buf, n, ip);
        return OBS_SYS_KNOWN;
    }

    case OBS_SYS_FIRMWARE: {
        /* The console's own firmware, read from `kern.version` rather than from
         * `sceKernelGetSystemSwVersion`.
         *
         * That call answers 13.090.001 here, and it is not wrong - it is the version of the
         * **PS4-compatibility environment** this title runs inside, because the title is a
         * `ps4_game`. The console's actual system software is in the kernel's own version
         * string, which this hardware reported as:
         *
         *     r226974/releases/12.40 Nov 27 2025 02:23:38
         *
         * so the firmware is the token after "releases/", up to the first space - `12.40`. The
         * whole string is reported raw by `135-sysctl`; the header shows the distilled version.
         * `kern.osrelease` is not used: on this platform it is the placeholder "0.0-prototype".
         *
         * Reading the token out of the string depends on the kernel keeping that ident format,
         * which is more fragile than a struct field - so a string that does not carry the
         * marker reports unconfirmed rather than guessing. (D261) */
        if (!obs_address_is_callable((const void *)&sysctlbyname)) {
            return OBS_SYS_ABSENT;
        }
        char version[128];
        size_t len = sizeof(version) - 1u;
        if (sysctlbyname("kern.version", version, &len, (void *)0, 0) != 0) {
            return OBS_SYS_UNCONFIRMED;
        }
        version[sizeof(version) - 1u] = '\0';

        /* Find "releases/" without libc: the token after it is the firmware. */
        static const char marker[] = "releases/";
        const char *at = 0;
        for (size_t i = 0; version[i] != '\0'; i++) {
            size_t j = 0;
            while (marker[j] != '\0' && version[i + j] == marker[j]) {
                j++;
            }
            if (marker[j] == '\0') {
                at = version + i + j;
                break;
            }
        }
        if (at == 0) {
            return OBS_SYS_UNCONFIRMED;
        }
        size_t k = 0;
        while (at[k] != '\0' && at[k] != ' ' && k + 1u < n) {
            buf[k] = at[k];
            k++;
        }
        buf[k] = '\0';
        return (k == 0u) ? OBS_SYS_UNCONFIRMED : OBS_SYS_KNOWN;
    }

    case OBS_SYS_GENERATION:
        /* Asked, not inferred again.
         *
         * This used to run its own inference from `sceAgcAcbAcquireMem` and
         * `sceGnmSubmitDone` - different markers from the ones `005-generation` uses, so
         * the header and the section could disagree about the one fact the header states.
         * `obs_detected_generation()` existed to prevent exactly that and had no callers
         * (D110). It now owns the question and computes on demand, so this is correct even
         * though the header is drawn before any section runs. */
        /* Named by the driver that resolved, not by where the console sits in time.
         *
         * This read "5 (current)" and "4 (previous)", which is a fact with an expiry date:
         * the day a sixth generation ships, every report this project has ever produced
         * starts claiming the wrong thing, and the archived ones cannot be corrected. A
         * report is supposed to still be true when it is read.
         *
         * The number is the generation and the parenthetical is the *evidence* - `agc` and
         * `gnm` are the graphics drivers the inference actually keys on, so the label now
         * says what was observed rather than how recent it was when this was written.
         * Neither ages. */
        switch (obs_detected_generation()) {
        case OBS_GENERATION_CURRENT:
            obs_put(buf, n, "5 (agc)");
            return OBS_SYS_KNOWN;
        case OBS_GENERATION_PREVIOUS:
            obs_put(buf, n, "4 (gnm)");
            return OBS_SYS_KNOWN;
        case OBS_GENERATION_BOTH:
            /* Both driver families resolve. This does *not* name a console - claiming one from
             * a stub was the mistake this program exists to expose, and a loader that
             * stub-resolves every unresolved import answers "yes" to the current-generation
             * driver with no implementation behind it. But it is a positive observation and a
             * different fact from "neither resolves": something IS on the other end, and "both
             * present" is itself the fingerprint of a stub-everything loader as much as of real
             * back-compat. So it is reported as a known value of its own - `both` - rather than
             * collapsed into the `unknown` a genuine absence produces. The console stays
             * unnamed; the situation does not. `neither` falls through to absent/`unknown`. */
            obs_put(buf, n, "both");
            return OBS_SYS_KNOWN;
        case OBS_GENERATION_UNKNOWN:
        default:
#if OBSCENE_GEN == 4
            /* The *console's* generation could not be identified - a `ps4_game` is refused the
             * current generation's driver, so the probe cannot look (D255). But the *mode this
             * title runs in* is not unknown: it was built `gen4` and it is running, so it is
             * running as a `ps4_game` - PS4 compatibility mode on whatever console this is.
             * "ps4_mode" is the platform's own name for that, and it is a more useful and
             * equally honest answer than "unknown": a build fact this program is certain of,
             * not an inference about the machine. */
            obs_put(buf, n, "ps4_mode");
            return OBS_SYS_KNOWN;
#else
            return OBS_SYS_ABSENT;
#endif
        }

    case OBS_SYS_GPU: {
        /* The graphics driver present, which the GEN field drops when it falls to `ps4_mode`.
         * `obs_gpu_drivers` asks the driver question rather than the generation one, so gnm - here
         * and rendering - is reported instead of discarded. NULL means neither driver resolved. */
        const char *driver = obs_gpu_drivers();
        if (driver == (const char *)0) {
            return OBS_SYS_ABSENT;
        }
        obs_put(buf, n, driver);
        return OBS_SYS_KNOWN;
    }

    case OBS_SYS_MEMORY:
        if (obs_address_is_callable((const void *)&sceKernelAvailableFlexibleMemorySize)) {
            size_t available = 0;
            if (sceKernelAvailableFlexibleMemorySize(&available) == 0) {
                obs_put_mib(buf, n, (uint64_t)available);
                return OBS_SYS_KNOWN;
            }
            /* Present, called, and it declined - a real "unknown" rather than absence. */
            return OBS_SYS_UNCONFIRMED;
        }
        return OBS_SYS_ABSENT;

    case OBS_SYS_VRAM:
        /* The console's memory is shared, so the direct-memory pool is the closest honest
         * figure for "graphics memory" - it is the physical RAM a title carves its GPU
         * allocations out of. Total, not free; free would need a query whose layout is not
         * confirmed. */
        if (obs_address_is_callable((const void *)&sceKernelGetDirectMemorySize)) {
            obs_put_mib(buf, n, (uint64_t)sceKernelGetDirectMemorySize());
            return OBS_SYS_KNOWN;
        }
        return OBS_SYS_ABSENT;

    case OBS_SYS_TEMP:
        /* No confirmed temperature query - none is even in the mined corpus, and inventing an
         * arity or a struct for a thermal call is the sin this program exists to expose (D008).
         * `unknown` here is the visible reminder that a SoC-temperature query is wanted and not
         * yet found, on a real console and an emulator alike. The forcing function, not an
         * oversight - and the honest state until a provenanced signature surfaces. */
        return OBS_SYS_ABSENT;

    case OBS_SYS_STORAGE: {
        /* Free storage, via `statfs` on the title's own writable mount.
         *
         * `statfs(path, buf)` fills a `struct statfs` whose layout is FreeBSD's - which this
         * kernel derives from - and the two fields needed sit at offsets stable across every
         * FreeBSD it could descend from: the fragment size `f_bsize` (u64) at 0x10 and the
         * non-superuser available block count `f_bavail` (i64) at 0x30 (sys/mount.h). Free
         * bytes = f_bavail * f_bsize. The buffer is over-sized well past the structure and only
         * those two words are read, so a layout that differs after them cannot be misread and
         * the call cannot overrun - the same discipline the version getters use. `/download0`
         * is a title's own writable mount; a path the sandbox refuses reports unconfirmed
         * rather than a wrong number. (D272) */
        if (!obs_address_is_callable((const void *)&statfs)) {
            return OBS_SYS_ABSENT;
        }
        unsigned char sb[512]; /* >> sizeof(struct statfs) */
        for (size_t i = 0; i < sizeof(sb); i++) {
            sb[i] = 0u;
        }
        if (statfs("/download0", sb) != 0) {
            return OBS_SYS_UNCONFIRMED;
        }
        uint64_t bsize = 0;
        uint64_t bavail = 0;
        for (unsigned int i = 0; i < 8u; i++) {
            bsize |= (uint64_t)sb[0x10u + i] << (i * 8u);
            bavail |= (uint64_t)sb[0x30u + i] << (i * 8u);
        }
        obs_put_mib(buf, n, bavail * bsize);
        return OBS_SYS_KNOWN;
    }

#if OBSCENE_GEN == 4
    case OBS_SYS_PS4_FIRMWARE: {
        /* The PS4-compatibility environment's own version - the one `sceKernelGetSystemSwVersion`
         * reports. This is the value that is *not* the console's firmware (that is the FW field,
         * from kern.version); it is the version a `ps4_game` is told it runs on, and it is worth
         * surfacing precisely because the two differ and the difference is the fingerprint of
         * compatibility mode.
         *
         * `OrbisKernelSwVersion`: `size_t` Size at 0x00 (caller-set), a 0x1C-byte version string
         * at 0x08, packed version at 0x24, total 0x28 - from the OpenOrbis toolchain headers.
         * Only the string is read, and the buffer is over-sized past 0x28 so the call cannot
         * overrun it. (D263) */
        if (!obs_address_is_callable((const void *)&sceKernelGetSystemSwVersion)) {
            return OBS_SYS_ABSENT;
        }
        unsigned char version[0x40];
        for (size_t i = 0; i < sizeof(version); i++) {
            version[i] = 0u;
        }
        version[0] = (unsigned char)0x28; /* Size, little-endian; 0x28 fits one byte. */
        if (sceKernelGetSystemSwVersion(version) != 0) {
            return OBS_SYS_UNCONFIRMED;
        }
        const char *at = (const char *)(version + 0x08);
        while (*at == ' ') {
            at++;
        }
        if (*at == '\0') {
            return OBS_SYS_UNCONFIRMED;
        }
        obs_put(buf, n, at);
        return OBS_SYS_KNOWN;
    }
#endif

    case OBS_SYS_COUNT:
    default:
        return OBS_SYS_ABSENT;
    }
}

/* The record's tier word. The report carries the tier as a name because a consumer diffing
 * two platforms needs "unwired here, missing there" to read as different, not both as
 * `unknown`. */
__attribute__((unused)) static const char *obs_state_word(obs_sys_state state) {
    switch (state) {
    case OBS_SYS_KNOWN:
        return "known";
    case OBS_SYS_UNCONFIRMED:
        return "unconfirmed";
    case OBS_SYS_ABSENT:
    default:
        return "absent";
    }
}

/* A stable diff key per field, deliberately not the abbreviated HUD label. `FW`/`DISK` read
 * well on a status line; `firmware`/`storage` are what a report reader keys on, and an
 * abbreviation is a presentation choice that should not be a field name in an interface. */
__attribute__((unused)) static const char *obs_field_key(obs_sys_field field) {
    switch (field) {
    case OBS_SYS_LISTENING:
        return "listening";
    case OBS_SYS_SECRET:
        return "secret";
    case OBS_SYS_IP:
        return "ip";
    case OBS_SYS_FIRMWARE:
        return "firmware";
    case OBS_SYS_GENERATION:
        return "generation";
    case OBS_SYS_GPU:
        return "gpu";
    case OBS_SYS_MEMORY:
        return "memory";
    case OBS_SYS_VRAM:
        return "vram";
    case OBS_SYS_TEMP:
        return "temp";
    case OBS_SYS_STORAGE:
        return "storage";
#if OBSCENE_GEN == 4
    case OBS_SYS_PS4_FIRMWARE:
        return "ps4_firmware";
#endif
    case OBS_SYS_COUNT:
    default:
        return "unknown";
    }
}

void obs_sysinfo_report(void) {
#if defined(OBS_NO_UI)
    /* A build with no status bar does not gather what would fill one.
     *
     * The status fields exist to be drawn in the header, and a headless build - the elfldr
     * payload - draws no header. Gathering them there is not just wasted work: some of the
     * getters touch subsystems that a title has set up and a bare payload has not. The IP
     * field calls `sceNetCtlInit`/`sceNetCtlGetInfo`, and running that in an elfldr payload,
     * where the network is not brought up the way it is for a title, is a plausible way to
     * wedge the process - so the build that has no use for the answer does not ask. (D262)
     *
     * The function still exists and is still called; it simply reports nothing here. */
    return;
#else
    /* Indexed through an int rather than incrementing the enum: `field++` on an enum is an
     * int-to-enum conversion the strict build treats as an error (CLAUDE.md build flags). */
    for (int i = 0; i < OBS_SYS_COUNT; i++) {
        obs_sys_field field = (obs_sys_field)i;
        /* 32 bytes is the buffer size obs_sysinfo_value documents as its minimum, and it
         * seeds `unknown` itself, so a non-known field carries `unknown` here without this
         * loop having to remember to. */
        char value[32];
        obs_sys_state state = obs_sysinfo_value(field, value, sizeof(value));
        obs_report_sysinfo(obs_field_key(field), obs_state_word(state), value);
    }
#endif
}
