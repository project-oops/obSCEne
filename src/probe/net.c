/*
 * The command protocol, served over a socket.
 *
 * `docs/PROTOCOL.md` is the contract; this implements it. The document came first
 * deliberately, so that a consumer can be built against a grammar rather than against
 * whatever this file happens to do today (D102).
 *
 * # The one rule everything else arranges itself around
 *
 * **`ack` is written and flushed before the command runs.**
 *
 * Arbitrary calls with arbitrary arguments fault constantly - that is the normal case
 * here, not the exceptional one - and a probe cannot report its own death, because the
 * process is gone. So the protocol is arranged to make death legible from the *other*
 * end: an `ack` with no result, followed by a closed connection, means exactly one
 * thing.
 *
 * The driver records that as `died`. It never invents a value, and it never records a
 * null return, because a timeout written down as "returned 0" is fiction that is
 * indistinguishable from evidence - and it is the fiction that gets trusted later.
 *
 * This is the same principle the report already applies to itself (CLAUDE.md, principle
 * 1), and it is the reason the ordering below is not negotiable: acknowledge, flush,
 * then work.
 *
 * # Capabilities are what this build can actually do
 *
 * Not a wish list. Every token announced in `hello` must be honourable, and anything
 * else is refused - a responder that guesses at a command it does not implement
 * produces a record that looks like evidence and is not.
 *
 * The list is assembled from what is really present, so a build with the refusing
 * network backend, or a platform missing a function, announces less rather than failing
 * later.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/net.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#if defined(OBS_GPU)
#include "obscene/gpu.h"
#endif

/* A request line. The specification caps a line at 4096 bytes including the terminator;
 * anything longer is a protocol error rather than something to truncate quietly. */
#define OBS_NET_LINE_MAX 4096
/* Replies are records, which the report layer already bounds well below this. */
#define OBS_NET_REPLY_MAX 512

/* One session's state.
 *
 * `last_seq` is not bookkeeping: sequence numbers must strictly increase, and a driver
 * that repeats or rewinds one has lost track of which reply belongs to which request.
 * Catching it here is cheaper than discovering it in a corpus afterwards. */
typedef struct obs_session {
    int connection;
    unsigned long last_seq;
    int greeted;
    int finished;
    /* Printed in `hello` and on every `part`. The driver holds it, and a *different*
     * one where it expected the old one is how a restart becomes visible - which
     * matters because a faulting command ends the probe and something outside this
     * protocol brings it back. Everything before a new identifier belongs to a
     * different process. */
    char id[OBS_NUM_MAX + 1];
} obs_session;

/* Fills in a session identifier that differs from the last one.
 *
 * Derived from a clock where the platform has one, because the property that matters is
 * that a *restarted* probe produces a different value - and a counter starting from one
 * every time a process begins produces the same sequence on every run, which is
 * precisely the case restart detection exists to catch.
 *
 * Where there is no clock this falls back to a counter and **says so**, by prefixing
 * the value. A driver reading `c1` knows the identifier only distinguishes sessions
 * within one process and cannot see a restart; a driver reading a clock-derived one
 * knows it can. An identifier whose guarantees are unknowable would be worse than none.
 */
static unsigned long obs_net_session_counter;

static void obs_net_session_id(obs_session *session) {
    obs_net_session_counter++;
    uint64_t value = 0;
    const char *prefix = "c";
#if !defined(OBSCENE_HOST_BUILD)
    if (obs_address_is_callable((const void *)&sceKernelGetProcessTime)) {
        value = sceKernelGetProcessTime();
        prefix = "t";
    }
#endif
    if (value == 0) {
        value = (uint64_t)obs_net_session_counter;
    }
    size_t at = 0;
    session->id[at++] = prefix[0];
    at += obs_format_hex(session->id + at, value);
    session->id[at] = '\0';
}

/* ---- writing
 * --------------------------------------------------------------------------
 *
 * Records are built here rather than through `obs_report_*`, because those write to the
 * report's own channels and these have to go to one particular socket. The shapes are
 * identical on purpose - a session transcript and a report are the same kind of
 * artefact, and the same parsers read both.
 */

/* Appends a field's *contents*, with separators neutralised.
 *
 * A `|` inside a field would shift every field after it, so it is replaced rather than
 * escaped - no parser then needs a matching unescape step, which is the same rule the
 * report follows.
 *
 * **Only for field contents.** The first version used this to write the `OBS|` prefix
 * too, and the sanitiser duly turned that separator into a space: every record went out
 * as `OBS ack|1|hello`. The wire format was wrong from the very first byte and it took
 * running a real session to see it, because the code reads as though it is writing what
 * it means to write. Structure is emitted by `obs_net_raw`, contents by this. */
static size_t obs_net_append(char *out, size_t at, const char *text) {
    for (size_t i = 0; text[i] != '\0' && at < OBS_NET_REPLY_MAX - 2; i++) {
        char c = text[i];
        out[at++] = (c == '|' || c == '\n') ? ' ' : c;
    }
    return at;
}

/* Appends structure - prefixes and separators - exactly as given. */
static size_t obs_net_raw(char *out, size_t at, const char *text) {
    for (size_t i = 0; text[i] != '\0' && at < OBS_NET_REPLY_MAX - 2; i++) {
        out[at++] = text[i];
    }
    return at;
}

static size_t obs_net_append_u64(char *out, size_t at, uint64_t value) {
    char scratch[OBS_NUM_MAX];
    size_t n = obs_format_u64(scratch, value);
    for (size_t i = 0; i < n && at < OBS_NET_REPLY_MAX - 2; i++) {
        out[at++] = scratch[i];
    }
    return at;
}

static size_t obs_net_append_hex(char *out, size_t at, uint64_t value) {
    char scratch[OBS_NUM_MAX];
    size_t n = obs_format_hex(scratch, value);
    for (size_t i = 0; i < n && at < OBS_NET_REPLY_MAX - 2; i++) {
        out[at++] = scratch[i];
    }
    return at;
}

/* Sends one complete record. Returns zero when the connection has gone. */
static int obs_net_line(obs_session *session, const char *kind, const char *a,
                        const char *b, const char *c) {
    char out[OBS_NET_REPLY_MAX];
    size_t at = 0;
    at = obs_net_raw(out, at, "OBS|");
    at = obs_net_append(out, at, kind);
    if (a != NULL) {
        out[at++] = '|';
        at = obs_net_append(out, at, a);
    }
    if (b != NULL) {
        out[at++] = '|';
        at = obs_net_append(out, at, b);
    }
    if (c != NULL) {
        out[at++] = '|';
        at = obs_net_append(out, at, c);
    }
    out[at++] = '\n';
    return obs_net_backend_send(session->connection, out, at) >= 0;
}

/* `ack`, and the flush that makes it mean something.
 *
 * The backend's send writes all of it or fails, so by the time this returns the bytes
 * are on the wire. That is what lets the other end distinguish "did not answer" from
 * "was never asked", and it is the reason this is a separate function rather than a
 * line in each handler - a handler that forgot to acknowledge would silently produce a
 * command whose death is unattributable. */
static int obs_net_ack(obs_session *session, unsigned long seq, const char *verb) {
    char number[OBS_NUM_MAX + 1];
    size_t n = obs_format_u64(number, seq);
    number[n] = '\0';
    return obs_net_line(session, "ack", number, verb, NULL);
}

static int obs_net_done(obs_session *session, unsigned long seq, const char *outcome,
                        uint64_t value, const char *detail) {
    char out[OBS_NET_REPLY_MAX];
    size_t at = 0;
    at = obs_net_raw(out, at, "OBS|done|");
    at = obs_net_append_u64(out, at, seq);
    out[at++] = '|';
    at = obs_net_append(out, at, outcome);
    out[at++] = '|';
    /* An empty value field rather than `0x0` where there is nothing to report. Zero is
     * a legitimate answer from a call, and writing it for "no value" would make the two
     * indistinguishable - which is the whole failure this protocol is arranged to
     * avoid. */
    if (outcome[0] == 'r' && outcome[1] == 'e' && outcome[2] == 't') {
        at = obs_net_append_hex(out, at, value);
    }
    out[at++] = '|';
    if (detail != NULL) {
        at = obs_net_append(out, at, detail);
    }
    out[at++] = '\n';
    return obs_net_backend_send(session->connection, out, at) >= 0;
}

static int obs_net_refused(obs_session *session, unsigned long seq,
                           const char *reason) {
    char number[OBS_NUM_MAX + 1];
    size_t n = obs_format_u64(number, seq);
    number[n] = '\0';
    return obs_net_line(session, "refused", number, reason, NULL);
}

/* ---- reading
 * -------------------------------------------------------------------------- */

/* Reads one newline-terminated line. Returns its length, or negative when the
 * connection has closed or the line exceeded the specification's limit. */
static long obs_net_read_line(obs_session *session, char *out, size_t max) {
    size_t at = 0;
    for (;;) {
        char c;
        long n = obs_net_backend_recv(session->connection, &c, 1);
        if (n <= 0) {
            return -1;
        }
        if (c == '\n') {
            out[at] = '\0';
            return (long)at;
        }
        /* Carried over from a driver on a platform whose line endings differ. Dropped
         * rather than refused: it is not ambiguous, and refusing would fail a session
         * over something nobody would think to look for. */
        if (c == '\r') {
            continue;
        }
        if (at + 1 >= max) {
            return -2;
        }
        out[at++] = c;
    }
}

static int obs_net_equal(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return a[i] == b[i];
}

/* Splits a line on `|` in place, returning the field count. */
static unsigned int obs_net_split(char *line, char **fields, unsigned int max) {
    unsigned int count = 0;
    fields[count++] = line;
    for (size_t i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            line[i] = '\0';
            if (count < max) {
                fields[count++] = &line[i + 1];
            }
        }
    }
    return count;
}

static uint64_t obs_net_parse_u64(const char *text, int *ok) {
    uint64_t value = 0;
    *ok = 0;
    if (text[0] == '\0') {
        return 0;
    }
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        value = value * 10u + (uint64_t)(text[i] - '0');
    }
    *ok = 1;
    return value;
}

/* Hex, with or without the `0x` the protocol uses for addresses and arguments. Rejects
 * an empty string and any non-hex digit, so a malformed argument becomes a
 * `bad-argument` refusal rather than a call to a garbage address. */
static uint64_t obs_net_parse_hex(const char *text, int *ok) {
    *ok = 0;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }
    if (text[0] == '\0') {
        return 0;
    }
    uint64_t value = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        unsigned int d;
        if (c >= '0' && c <= '9') {
            d = (unsigned int)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            d = (unsigned int)(c - 'a') + 10u;
        } else if (c >= 'A' && c <= 'F') {
            d = (unsigned int)(c - 'A') + 10u;
        } else {
            return 0;
        }
        value = (value << 4) | d;
    }
    *ok = 1;
    return value;
}

/* One `bytes` record, straight to the socket, in the exact shape `obs_report_bytes`
 * uses so the same parser reads a session and a report (docs/OUTPUT.md).
 *
 * Sixteen bytes per record, matching OBS_BYTES_PER_RECORD, so no line approaches the
 * length cap however large the read. This is a separate emitter from the report's
 * because these bytes are the driver's answer and must go down *this* socket, not to
 * the probe's own output channels. */
static int obs_net_bytes(obs_session *session, const char *id, unsigned int offset,
                         const unsigned char *bytes, unsigned int len) {
    static const char digits[] = "0123456789abcdef";
    char out[OBS_NET_REPLY_MAX];
    size_t at = 0;
    at = obs_net_raw(out, at, "OBS|bytes|");
    at = obs_net_append(out, at, id);
    at = obs_net_raw(out, at, "|(memory)|contents|");
    at = obs_net_append_u64(out, at, offset);
    out[at++] = '|';
    for (unsigned int i = 0; i < len && at + 2 < OBS_NET_REPLY_MAX - 2; i++) {
        out[at++] = digits[(bytes[i] >> 4) & 0x0Fu];
        out[at++] = digits[bytes[i] & 0x0Fu];
    }
    out[at++] = '\n';
    return obs_net_backend_send(session->connection, out, at) >= 0;
}

/* The call primitive's function type. Committed to nothing but the calling convention -
 * the same variadic-prototype trick 910-bulk uses (D096), for the same reason: on this
 * ABI the caller cleans up, so handing six registers to a function that reads fewer
 * cannot corrupt the stack, and a variadic prototype zeroes the vector-count register a
 * variadic callee reads. D008 is about expectations, not calls, and a `call` asserts
 * nothing about what it invokes. */
typedef uint64_t (*obs_net_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                               uint64_t, ...);

/* ---- the session secret
 * -----------------------------------------------------------------
 *
 * # Why there is one
 *
 * `docs/PROTOCOL.md` describes this socket accurately: unauthenticated, and its `call`
 * verb invokes an arbitrary address with six arguments. On a development machine that
 * is fine, because the listener is on a machine with a firewall. On a console it is
 * not: there is no shell to tunnel through, the module binds every interface because
 * the driver is on another machine, and anything else on that network can connect and
 * issue commands.
 *
 * # Generated per startup, never built in
 *
 * A secret compiled into the module would be shared by everybody who has that module,
 * which is the opposite of a secret. This is generated once when the probe starts
 * listening and lasts for that run: every session in the accept loop uses the same one,
 * and restarting the probe replaces it.
 *
 * It is **displayed**, because that is the only channel a console has - the HUD already
 * draws the port for the same reason ("read the address off the screen", which the
 * protocol assumes) and the secret goes beside it. It is also emitted as a record, so a
 * host or emulator run can read it off stdout without a camera.
 *
 * # What it defends and what it does not
 *
 * It stops another device on the network connecting and driving the probe. That is the
 * threat it exists for and it is effective against it.
 *
 * It does **not** defend against anyone who can observe the link. The socket is
 * cleartext, so an adversary on the path reads the secret out of the `hello` and never
 * needs to guess it, and on the console the entropy behind it is timing jitter rather
 * than a CSPRNG. Both facts are in `docs/PROTOCOL.md` rather than left for someone to
 * discover.
 */
#define OBS_NET_SECRET_BYTES 16u
#define OBS_NET_SECRET_CHARS (OBS_NET_SECRET_BYTES * 2u)

/* The secret for this run, as hex text, and whether there is one. Empty means the probe
 * could not generate one and is serving unauthenticated - which is reported rather than
 * silently accepted. */
static char obs_net_secret[OBS_NET_SECRET_CHARS + 1u];
static int obs_net_secret_ready;

const char *obs_net_secret_text(void) {
    return obs_net_secret_ready ? obs_net_secret : "";
}

int obs_net_secret_generate(void) {
    unsigned char bytes[OBS_NET_SECRET_BYTES];
    obs_net_secret_ready = 0;
    obs_net_secret[0] = '\0';
    if (obs_net_backend_entropy(bytes, OBS_NET_SECRET_BYTES) < 0) {
        return -1;
    }
    static const char digits[] = "0123456789abcdef";
    for (unsigned int i = 0; i < OBS_NET_SECRET_BYTES; i++) {
        obs_net_secret[i * 2u] = digits[(bytes[i] >> 4) & 0x0Fu];
        obs_net_secret[i * 2u + 1u] = digits[bytes[i] & 0x0Fu];
    }
    obs_net_secret[OBS_NET_SECRET_CHARS] = '\0';
    obs_net_secret_ready = 1;
    return 0;
}

/* Compare without letting the time taken say how much was right.
 *
 * An ordinary comparison returns as soon as two bytes differ, so an attacker who can
 * time the response learns the secret one character at a time - a few hundred attempts
 * rather than 2^128. Every byte is compared here and the differences are accumulated.
 *
 * The offered string may be shorter than the secret, so the index into it stops
 * advancing at its terminator instead of running past the end of the buffer, and a
 * final byte catches an offered string that is longer. */
static int obs_net_secret_matches(const char *offered) {
    if (offered == NULL) {
        return 0;
    }
    unsigned char diff = 0;
    unsigned int j = 0;
    for (unsigned int i = 0; i < OBS_NET_SECRET_CHARS; i++) {
        char c = offered[j];
        diff |= (unsigned char)((unsigned char)c ^ (unsigned char)obs_net_secret[i]);
        if (c != '\0') {
            j++;
        }
    }
    diff |= (unsigned char)offered[j];
    return diff == 0;
}

/* ---- capabilities
 * ----------------------------------------------------------------------
 *
 * Assembled from what this build can honour, not from what the grammar defines.
 *
 * `report` is always available: the compiled-in suite is the one thing the probe can
 * always do. The rest are absent until they are implemented, and their absence is how a
 * driver learns what this target is - a stand-in with no vendor libraries announces no
 * `resolve`, and a driver discovers that rather than assuming past it.
 */
static const char *obs_net_capabilities(void) {
    /* `call` and `read` join `report`: both are always available - calling an address
     * and reading memory need no vendor library to resolve first. `write` stays off,
     * because a read or a call costs a crash at worst while a write costs a crash *and*
     * whatever state was being built (docs/PROTOCOL.md, security posture).
     *
     * `blob` and `reset` are announced only when the build opted into the escape hatch
     * with OBS_NET_ESCAPE. `blob`/`run` execute code the socket supplied, so they are
     * off unless a build deliberately turns them on - the doc's "blob is off unless
     * enabled" made literal. A driver against a default build learns they are absent
     * and does not assume past it. */
#if defined(OBS_GPU)
    /* `gpu` only when a backend is actually up - a GPU build on a target with no usable
     * device (the console's refusing stub) announces nothing it cannot honour, exactly
     * the rule the capability list exists to keep. */
    if (obs_gpu_backend_available()) {
#if defined(OBS_NET_ESCAPE)
        return "call,read,report,gpu,blob,reset";
#else
        return "call,read,report,gpu";
#endif
    }
#endif
#if defined(OBS_NET_ESCAPE)
    return "call,read,report,blob,reset";
#else
    return "call,read,report";
#endif
}

/* The call primitive: invoke an address with up to six integer arguments, report the
 * return.
 *
 * The announcement in the session loop is already on the wire before this runs, so a
 * call that faults leaves an `ack` with no `done` and the driver records `died` - which
 * is the normal, expected outcome of poking an arbitrary address, not an error path. */
static int obs_net_call(obs_session *session, unsigned long seq, char **fields,
                        unsigned int count) {
    int ok = 0;
    uint64_t addr = obs_net_parse_hex(fields[3], &ok);
    /* A malformed address is refused; a *valid but fatal* one (0, an unmapped page) is
     * not. `call` invokes what it is told to, and a call that faults is the normal,
     * designed outcome - the `ack` is already on the wire, so the death reads as a lone
     * `ack` and the driver records `died`. Special-casing 0 here would both contradict
     * the spec and hide the death path a consumer must handle anyway. */
    if (count < 4 || !ok) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    /* Up to six arguments, hex, defaulting to zero. A malformed one is refused rather
     * than silently read as zero - a wrong argument to an arbitrary call is not
     * something to paper over. */
    uint64_t a[6] = {0, 0, 0, 0, 0, 0};
    for (unsigned int i = 0; i < 6u && (unsigned int)(4 + i) < count; i++) {
        int arg_ok = 0;
        a[i] = obs_net_parse_hex(fields[4 + i], &arg_ok);
        if (!arg_ok) {
            return obs_net_refused(session, seq, "bad-argument");
        }
    }
    obs_net_fn fn = (obs_net_fn)(uintptr_t)addr;
    uint64_t returned = fn(a[0], a[1], a[2], a[3], a[4], a[5]);
    return obs_net_done(session, seq, "returned", returned, NULL);
}

/* The read primitive: dump guest memory as `bytes` records, then a `done` with the
 * length.
 *
 * An unmapped address faults, which is the `died` path (ack, then the connection
 * closes) - not a refusal. That is deliberate per the spec: "this address is not
 * readable" and "asking about this address killed the process" are different facts, and
 * this build reports the second by dying rather than pretending it could test the
 * address first. */
static int obs_net_read(obs_session *session, unsigned long seq, char **fields,
                        unsigned int count) {
    int addr_ok = 0;
    int len_ok = 0;
    uint64_t addr = obs_net_parse_hex(fields[3], &addr_ok);
    uint64_t len = count > 4 ? obs_net_parse_hex(fields[4], &len_ok) : 0;
    if (count < 5 || !addr_ok || !len_ok) {
        return obs_net_refused(session, seq, "bad-argument");
    }

    /* `read/0x…`, matching the id in docs/examples/protocol/06-read.txt so a consumer
     * built against that transcript reads a live session unchanged. `obs_format_hex`
     * already writes the `0x` prefix - adding one here produced `read/0x0x…`, which is
     * why the prefix is not written by hand. */
    char id[5 + 2 + 16 + 1];
    size_t at = 0;
    id[at++] = 'r';
    id[at++] = 'e';
    id[at++] = 'a';
    id[at++] = 'd';
    id[at++] = '/';
    at += obs_format_hex(id + at, addr);
    id[at] = '\0';

    const unsigned char *base = (const unsigned char *)(uintptr_t)addr;
    for (uint64_t off = 0; off < len; off += 16u) {
        unsigned int chunk = (len - off) < 16u ? (unsigned int)(len - off) : 16u;
        if (!obs_net_bytes(session, id, (unsigned int)off, base + off, chunk)) {
            return 0;
        }
    }
    return obs_net_done(session, seq, "returned", len, NULL);
}

/* ---- the session
 * ----------------------------------------------------------------------- */

static int obs_net_hello(obs_session *session, unsigned long seq, char **fields,
                         unsigned int count) {
    int ok = 0;
    uint64_t wanted = count > 3 ? obs_net_parse_u64(fields[3], &ok) : 0;
    if (!ok || wanted < 1) {
        return obs_net_refused(session, seq, "bad-argument");
    }

    /* The secret, checked before a single capability is disclosed.
     *
     * Placed here rather than beside `greeted` below because the reply to `hello` names
     * everything this build can do, and an unauthenticated peer should not learn that.
     * The existing gate does the rest of the work: every other verb is refused with
     * `not-negotiated` until `greeted` is set, and returning here never sets it.
     *
     * Field 4, appended after the version, which is what `docs/OUTPUT.md` permits - new
     * fields go on the end of a line. A driver built before this existed still works
     * against a probe that could not generate a secret, and gets a clear refusal
     * against one that did, rather than a parse error. */
    if (obs_net_secret_ready) {
        const char *offered = count > 4 ? fields[4] : "";
        if (!obs_net_secret_matches(offered)) {
            return obs_net_refused(session, seq, "unauthorised");
        }
    }

    /* At most what the driver asked for, and this build speaks exactly one version. A
     * driver asking for a later one is answered in the version both understand rather
     * than being refused, which is what makes the version field useful rather than a
     * gate. */
    if (!obs_net_line(session, "hello", "1", session->id, obs_net_capabilities())) {
        return 0;
    }

    /* What the probe can *observe* about itself, and nothing it cannot.
     *
     * `binary` is the build kind (module/host), not the machine - it used to be sent as
     * `target`, which squatted on the key a consumer grades by with a value that is not
     * a machine at all. The machine identity - `target`, `gpu`, `firmware`, above all
     * whether this is real hardware - a probe **cannot certify**: inside an emulator
     * every system call answers as the emulator chooses, so a self-reported `firmware`
     * would be an emulator's value wearing the hardware's badge. That identity is the
     * operator's to assert through the driver (`drive --part target=prospero`), never
     * the probe's to claim. See docs/OUTPUT.md, "The origin is stamped by the operator,
     * not claimed by the probe". */
    if (!obs_net_line(session, "part", session->id, "probe", OBSCENE_BUILD_ID) ||
        !obs_net_line(session, "part", session->id, "binary", OBSCENE_TARGET) ||
        !obs_net_line(session, "part", session->id, "transport",
                      obs_net_backend_name())) {
        return 0;
    }
    session->greeted = 1;
    return obs_net_done(session, seq, "ok", 0, NULL);
}

/* The report tee: copy one already-formatted record straight to the session socket.
 *
 * Installed via obs_set_write_tee only while a `report` command runs, so the suite's
 * records reach the driver between its `ack` and `done`. The bytes are a complete
 * `OBS|…\n` line already, so this forwards them verbatim - the same shape the driver
 * reads from a report file. A send failure is swallowed: the session is ending anyway,
 * and the `done` that follows will fail to send too and end the loop cleanly. */
static void obs_net_tee(void *ctx, const char *bytes, size_t len) {
    obs_session *session = (obs_session *)ctx;
    (void)obs_net_backend_send(session->connection, bytes, len);
}

#if defined(OBS_GPU)
/* The gpu primitive: dispatch a compiled-in kernel over caller-supplied operands.
 *
 * `CMD|seq|gpu|<kernel>|<op0>|<op1>|...` - the operands are 32-bit words (float bits)
 * fed to the named shader. Unlike `call` and `blob`, this runs only shaders the build
 * already contains, so it executes nothing arbitrary: the driver picks *which* known
 * kernel and *what* inputs, which is the interactive loop - a new question without a
 * rebuild.
 *
 * The operands follow the same lane layout the section uses, applied per command: a
 * unary kernel takes N operands as N lanes; an arity-k kernel takes a multiple of k,
 * each group a tuple laid out `[in0..in_{k-1}, out]`. Results come back as the same
 * `gpu`/`gpuop` records a report emits, teed to this socket, so a driver reads a live
 * dispatch exactly as it reads a captured one - and `gpudev` goes first, so the results
 * are never read without their provenance.
 *
 * A dispatch that fails without crashing (a Vulkan error, the probe still alive) is not
 * a death: it returns zero lanes and no records, which a driver tells from success by
 * the lane count in the `done`. A dispatch that *ends the process* is the ordinary
 * `ack`-with-no `done` path, handled by the driver as `died` like any other. */
static int obs_net_gpu(obs_session *session, unsigned long seq, char **fields,
                       unsigned int count) {
    /* Bounded, so one command cannot ask for an unbounded buffer. The line-length cap
     * already limits the operands; this is the belt to that braces. */
    enum { OBS_NET_GPU_MAX = 256u };
    static uint32_t operands[OBS_NET_GPU_MAX];
    static uint32_t buffer[OBS_NET_GPU_MAX * 4u];

    if (count < 5) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    const char *kernel = fields[3];
    unsigned int arity = obs_gpu_arity(kernel);
    if (arity == 0u) {
        /* Unknown kernel: refused, not guessed. The driver learns the names from a
         * report, where every kernel appears. */
        return obs_net_refused(session, seq, "bad-argument");
    }

    unsigned int nops = count - 4u;
    if (nops == 0u || nops > OBS_NET_GPU_MAX || (arity > 1u && nops % arity != 0u)) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    for (unsigned int i = 0; i < nops; i++) {
        int ok = 0;
        operands[i] = (uint32_t)obs_net_parse_hex(fields[4 + i], &ok);
        if (!ok) {
            return obs_net_refused(session, seq, "bad-argument");
        }
    }

    unsigned int stride =
        arity > 1u ? arity + 1u : 1u; /* unary is in place, stride 1 */
    unsigned int lanes = arity == 1u ? nops : nops / arity;
    unsigned int words = lanes * stride;
    for (unsigned int l = 0; l < lanes; l++) {
        for (unsigned int d = 0; d < arity; d++) {
            buffer[l * stride + d] = operands[l * arity + d];
        }
        if (arity > 1u) {
            buffer[l * stride + arity] = 0u; /* the output slot */
        }
    }

    /* Tee to this socket while the dispatch runs, exactly as `report` does, so the
     * gpu/gpuop lines arrive between the ack and the done. Provenance first. */
    obs_set_write_tee(obs_net_tee, session);
    obs_report_gpu_device(obs_gpu_backend_name(), obs_gpu_device_name(),
                          obs_gpu_device_type());
    int rc = obs_gpu_dispatch_named(kernel, buffer, words);
    if (rc == 0) {
        for (unsigned int l = 0; l < lanes; l++) {
            if (arity == 1u) {
                obs_report_gpu(kernel, l, operands[l], buffer[l]);
            } else {
                obs_report_gpu_op(kernel, l, buffer[l * stride + arity],
                                  &operands[l * arity], arity);
            }
        }
    }
    obs_set_write_tee(0, 0);

    /* The lane count on success, zero on a graceful failure - which the driver reads as
     * "ran but produced nothing", distinct from a death. */
    return obs_net_done(session, seq, "returned", rc == 0 ? lanes : 0u, NULL);
}
#endif /* OBS_GPU */

/* Serves commands until the driver leaves or the connection drops. */
#if defined(OBS_NET_ESCAPE)
/* ---- the escape hatch: blob, run and reset
 * ---------------------------------------------
 *
 * Compiled only when a build defines OBS_NET_ESCAPE, because `blob`/`run` execute code
 * the socket supplied - the doc's "blob is off unless enabled" made literal. Storage is
 * a small fixed table, not an allocation, in keeping with the freestanding rule: a
 * handful of blobs can be resident and named, and `reset` returns to the known state of
 * none.
 */
#define OBS_BLOB_COUNT 4
#define OBS_BLOB_MAX 4096u
#define OBS_BLOB_ID_MAX 15

typedef struct {
    char id[OBS_BLOB_ID_MAX + 1];
    unsigned char code[OBS_BLOB_MAX];
    size_t len;
    int used;
} obs_blob;

static obs_blob obs_blobs[OBS_BLOB_COUNT];

static void obs_blob_clear(void) {
    for (unsigned int i = 0; i < OBS_BLOB_COUNT; i++) {
        obs_blobs[i].used = 0;
        obs_blobs[i].len = 0;
        obs_blobs[i].id[0] = 0;
    }
}

/* The blob named `id`, or the first free slot when `create` and none matches. Null when
 * the id is empty or too long, or the table is full and none matched. */
static obs_blob *obs_blob_find(const char *id, int create) {
    size_t n = 0;
    while (id[n]) {
        n++;
    }
    if (n == 0 || n > OBS_BLOB_ID_MAX) {
        return 0;
    }
    obs_blob *free_slot = 0;
    for (unsigned int i = 0; i < OBS_BLOB_COUNT; i++) {
        if (obs_blobs[i].used && obs_net_equal(obs_blobs[i].id, id)) {
            return &obs_blobs[i];
        }
        if (!obs_blobs[i].used && !free_slot) {
            free_slot = &obs_blobs[i];
        }
    }
    if (!create || !free_slot) {
        return 0;
    }
    for (size_t i = 0; i <= n; i++) {
        free_slot->id[i] = id[i];
    }
    free_slot->used = 1;
    free_slot->len = 0;
    return free_slot;
}

/* One hex nibble, or -1 for a non-hex character. */
static int obs_net_nibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* CMD|seq|blob|id|offset|hex - uploads a chunk of machine code at an offset into a
 * named blob. Chunked so a line stays inside the length bound; the id lets several be
 * resident at once. */
static int obs_net_blob(obs_session *session, unsigned long seq, char **fields,
                        unsigned int count) {
    if (count < 6) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    int ok = 0;
    uint64_t offset = obs_net_parse_u64(fields[4], &ok);
    if (!ok || offset > OBS_BLOB_MAX) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    obs_blob *b = obs_blob_find(fields[3], 1);
    if (!b) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    const char *hex = fields[5];
    size_t hexlen = 0;
    while (hex[hexlen]) {
        hexlen++;
    }
    if (hexlen == 0 || hexlen % 2 != 0) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    size_t nbytes = hexlen / 2;
    if (nbytes > (size_t)OBS_BLOB_MAX - (size_t)offset) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    for (size_t i = 0; i < nbytes; i++) {
        int hi = obs_net_nibble(hex[i * 2]);
        int lo = obs_net_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return obs_net_refused(session, seq, "bad-argument");
        }
        b->code[(size_t)offset + i] = (unsigned char)((hi << 4) | lo);
    }
    if ((size_t)offset + nbytes > b->len) {
        b->len = (size_t)offset + nbytes;
    }
    /* The `done` value is the count of bytes this chunk carried, so a driver uploading
     * in chunks can check each landed whole - see
     * docs/examples/protocol/07-blob-run.txt. */
    return obs_net_done(session, seq, "returned", (uint64_t)nbytes, 0);
}

/* CMD|seq|run|id|arg0|... - calls an uploaded blob with up to six integer args and
 * reports the return. A blob that faults ends the process (ack with no done -> the
 * driver records died), exactly like `call`; a backend that cannot execute at all makes
 * this `unsupported`. */
static int obs_net_run(obs_session *session, unsigned long seq, char **fields,
                       unsigned int count) {
    if (count < 4) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    obs_blob *b = obs_blob_find(fields[3], 0);
    if (!b || b->len == 0) {
        return obs_net_refused(session, seq, "bad-argument");
    }
    uint64_t args[6] = {0, 0, 0, 0, 0, 0};
    unsigned int argc = 0;
    for (unsigned int i = 0; i + 4u < count && i < 6; i++) {
        int ok = 0;
        args[i] = obs_net_parse_hex(fields[4 + i], &ok);
        if (!ok) {
            return obs_net_refused(session, seq, "bad-argument");
        }
        argc++;
    }
    uint64_t result = 0;
    if (obs_net_backend_exec(b->code, b->len, args, argc, &result) < 0) {
        return obs_net_refused(session, seq, "unsupported");
    }
    return obs_net_done(session, seq, "returned", result, 0);
}

/* CMD|seq|reset - frees the resident blobs and returns to the known state of none
 * loaded. The detail says how many were freed, in the same shape as 08-reset.txt's
 * console reset - a reset that returns `ok` and reports nothing is the one the doc
 * warns reads as doing nothing. */
static int obs_net_reset(obs_session *session, unsigned long seq) {
    unsigned int freed = 0;
    for (unsigned int i = 0; i < OBS_BLOB_COUNT; i++) {
        if (obs_blobs[i].used) {
            freed++;
        }
    }
    obs_blob_clear();
    char detail[40];
    size_t at = obs_net_raw(detail, 0, "freed ");
    at = obs_net_append_u64(detail, at, freed);
    at = obs_net_raw(detail, at, " blob(s)");
    detail[at] = '\0';
    return obs_net_done(session, seq, "ok", 0, detail);
}
#endif /* OBS_NET_ESCAPE */

static void obs_net_session(obs_session *session) {
    char line[OBS_NET_LINE_MAX];
#if defined(OBS_NET_ESCAPE)
    /* A fresh session starts with no blobs resident, whatever a prior one left in the
     * static table. reset returns here mid-session; this guarantees it at the start of
     * one. */
    obs_blob_clear();
#endif
    while (!session->finished) {
        long length = obs_net_read_line(session, line, sizeof(line));
        if (length == -2) {
            /* Over the limit. Nothing is acknowledged, because nothing was understood -
             * an `ack` naming a verb we could not read would be a lie about what was
             * attempted. */
            (void)obs_net_line(session, "refused", "0", "bad-argument", NULL);
            return;
        }
        if (length < 0) {
            return;
        }
        if (length == 0) {
            continue;
        }

        /* Wide enough for the longest request: `CMD|seq|call|addr|a0|a1|a2|a3|a4|a5`,
         * ten fields. A narrower split would silently drop a call's later arguments
         * into the last field, which is a wrong call rather than a refused one. */
        char *fields[12];
        unsigned int count = obs_net_split(line, fields, 12);
        if (count < 3 || !obs_net_equal(fields[0], "CMD")) {
            (void)obs_net_line(session, "refused", "0", "bad-argument", NULL);
            continue;
        }

        int ok = 0;
        uint64_t seq = obs_net_parse_u64(fields[1], &ok);
        if (!ok || seq <= session->last_seq) {
            (void)obs_net_line(session, "refused", fields[1], "bad-argument", NULL);
            continue;
        }
        session->last_seq = (unsigned long)seq;
        const char *verb = fields[2];

        /* Acknowledged before anything else happens, including before the verb is known
         * to be one we implement. A refusal is still a command that was attempted, and
         * the transcript is easier to read - and easier to check - when every request
         * has an acknowledgement rather than only the ones that got somewhere. */
        if (!obs_net_ack(session, (unsigned long)seq, verb)) {
            return;
        }

        if (obs_net_equal(verb, "hello")) {
            if (!obs_net_hello(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }

        /* Everything else needs a session. A driver that skips the greeting has not
         * negotiated a version or learned what this build can do, so answering would be
         * answering a question that was never properly asked. */
        if (!session->greeted) {
            if (!obs_net_refused(session, (unsigned long)seq, "not-negotiated")) {
                return;
            }
            continue;
        }

        if (obs_net_equal(verb, "bye")) {
            (void)obs_net_done(session, (unsigned long)seq, "ok", 0, NULL);
            session->finished = 1;
            continue;
        }

        if (obs_net_equal(verb, "call")) {
            if (!obs_net_call(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }

        if (obs_net_equal(verb, "read")) {
            if (!obs_net_read(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }

#if defined(OBS_GPU)
        if (obs_net_equal(verb, "gpu")) {
            if (!obs_net_gpu(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }
#endif

#if defined(OBS_NET_ESCAPE)
        if (obs_net_equal(verb, "blob")) {
            if (!obs_net_blob(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }
        if (obs_net_equal(verb, "run")) {
            if (!obs_net_run(session, (unsigned long)seq, fields, count)) {
                return;
            }
            continue;
        }
        if (obs_net_equal(verb, "reset")) {
            if (!obs_net_reset(session, (unsigned long)seq)) {
                return;
            }
            continue;
        }
#else
        if (obs_net_equal(verb, "blob") || obs_net_equal(verb, "run") ||
            obs_net_equal(verb, "reset")) {
            /* Known verbs, but this build did not compile the escape hatch, so their
             * capability was never announced. Refused `not-negotiated` - the reason a
             * capability outside the negotiated set gets
             * (docs/examples/protocol/09-no-reset.txt), not unknown-verb, which would
             * deny they are part of the grammar at all. */
            if (!obs_net_refused(session, (unsigned long)seq, "not-negotiated")) {
                return;
            }
            continue;
        }
#endif

        if (obs_net_equal(verb, "report")) {
            /* The compiled-in suite, streamed down the socket as it runs.
             *
             * The tee copies every record to this session while the suite runs, so the
             * driver receives the section/try/res/sym/tally records between this `ack`
             * and the `done`, as docs/PROTOCOL.md describes - and the report still
             * reaches stdout and the file sink, because the tee is additive. Cleared
             * afterwards so nothing outside a `report` command goes to the socket.
             *
             * The `done` value stays the fail count, now as a summary line after the
             * full stream rather than instead of it. */
            obs_set_write_tee(obs_net_tee, session);
            obs_tally tally = obs_run_all();
            obs_set_write_tee(0, 0);
            if (!obs_net_done(session, (unsigned long)seq, "returned",
                              (uint64_t)tally.fail, NULL)) {
                return;
            }
            continue;
        }

        /* Never guessed at, never approximated. A responder that interprets a command
         * it does not know produces a record that looks like evidence and is not. */
        if (!obs_net_refused(session, (unsigned long)seq, "unknown-verb")) {
            return;
        }
    }
}

int obs_net_serve(unsigned short port) {
    if (!obs_net_backend_available()) {
        return -1;
    }
    int listener = obs_net_backend_listen(port);
    if (listener < 0) {
        return -1;
    }

    int connection = obs_net_backend_accept(listener);
    if (connection < 0) {
        obs_net_backend_close(listener);
        return -1;
    }

    obs_session session = {connection, 0, 0, 0, {0}};
    obs_net_session_id(&session);
    obs_net_session(&session);

    obs_net_backend_close(connection);
    obs_net_backend_close(listener);
    return 0;
}
