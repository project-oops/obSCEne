/*
 * The command socket.
 *
 * Implements `docs/PROTOCOL.md`, which is the contract and was written first. Where
 * this disagrees with that document, this is wrong.
 *
 * # The split, and why the target backend is empty for now
 *
 * Same shape as the file sink: the protocol - parsing, dispatch, sequencing, the rule
 * that an acknowledgement precedes the work - lives in `net.c` and is shared. The five
 * socket calls live in one file per target, and the build compiles exactly one.
 *
 * `net_posix.c` covers the host and a general-purpose handheld, whose socket interface
 * is ordinary POSIX and whose signatures are therefore certain. `net_target.c` covers
 * the console and currently refuses: the vendor's networking symbols are in the census
 * by name, and **their arities and structure layouts are not confirmed**. Calling them
 * anyway is precisely what D008 exists to forbid, and a wrong arity would corrupt the
 * stack and crash somewhere unrelated to networking.
 *
 * That is not a gap to be embarrassed about, it is the rule working. The listener
 * announces no capability it cannot honour, and a driver discovers the limitation
 * rather than assuming past it.
 */

#ifndef OBSCENE_NET_H
#define OBSCENE_NET_H

#include <stddef.h>
#include <stdint.h>

/* ---- the backend --------------------------------------------------------------------
 *
 * Six calls. A negative return is a failure in all of them, so the shared code above
 * never has to know what a platform's error convention is.
 */

/* Creates a listening socket bound to `port` on all interfaces. Returns a handle. */
int obs_net_backend_listen(unsigned short port);

/* Waits for one connection. Returns a handle for it. */
int obs_net_backend_accept(int listener);

/* Fills `out` with `len` bytes of platform entropy. Negative when it cannot.
 *
 * A backend call because the two targets differ completely in what they can honestly
 * offer. The POSIX one reads `/dev/urandom` and is a real CSPRNG; the console one has
 * no such thing and mixes timing jitter, so it is best-effort and its own comment is
 * explicit that it is not cryptographic. A failure is reported rather than papered over
 * with a constant: a secret derived from a predictable value is worse than no secret,
 * because it looks like protection.
 */
int obs_net_backend_entropy(unsigned char *out, unsigned int len);

/* Reads up to `len` bytes. Returns the count, 0 at end of stream, negative on failure.
 */
long obs_net_backend_recv(int connection, char *bytes, size_t len);

/* Writes exactly `len` bytes, or fails. Returns `len`, or negative. */
long obs_net_backend_send(int connection, const char *bytes, size_t len);

void obs_net_backend_close(int handle);

/* Whether this build has a real backend at all.
 *
 * Not a constant, because it is answered by whichever file was compiled - which is the
 * whole point of the split. A build with the refusing backend says so once, plainly,
 * rather than binding a socket that cannot work and failing later where it reads as a
 * network problem. */
int obs_net_backend_available(void);

/* The backend's name, for the record that says how a session was served. */
const char *obs_net_backend_name(void);

#if defined(OBS_NET_ESCAPE)
/* Executes a blob of position-independent machine code: maps it executable, calls it
 * with up to six integer arguments, unmaps it, and reports the return value in
 * `*result`.
 *
 * This is the teeth of the `blob`/`run` escape hatch, and the reason it is behind a
 * build flag and a capability that are both off by default: it runs code the socket
 * supplied, which only the owner driving their own probe on their own hardware should
 * ever do. A blob that faults when called ends the process, exactly as `call` does with
 * a bad address.
 *
 * Returns 0 on success; negative if this backend cannot execute a blob at all (the
 * console backend, which has no confirmed way to map executable memory) or the mapping
 * failed. The negative return is what `run` reports as `unsupported`. Declared only
 * when the escape hatch is compiled in, so a default build carries none of this. */
int obs_net_backend_exec(const unsigned char *code, size_t len, const uint64_t *args,
                         unsigned int argc, uint64_t *result);
#endif

/* ---- the server
 * ---------------------------------------------------------------------- */

/* Listens, serves one session to completion, and returns.
 *
 * One at a time, deliberately. A second driver interleaving commands would make the
 * ordering record meaningless, and ordering is part of the evidence - a result that
 * depended on what came before is only interpretable if what came before is written
 * down.
 *
 * Returns 0 when a session was served, negative when the socket could not be opened. */
int obs_net_serve(unsigned short port);

/* Generates the secret for this run. Negative when the platform could not supply
 * entropy, in which case the probe serves unauthenticated and says so. See the long
 * note in net.c. */
int obs_net_secret_generate(void);

/* The secret as hex text, or an empty string when there is none. */
const char *obs_net_secret_text(void);

/* The default port. Nothing else claims it. */
#define OBS_NET_PORT 9803

#endif /* OBSCENE_NET_H */
