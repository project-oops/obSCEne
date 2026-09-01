/*
 * The freestanding runtime.
 *
 * There is no libc here. Everything this program needs to format a number and put
 * bytes on an output stream is in this file, which is short on purpose: every
 * function added here is one more thing that can be wrong while diagnosing something
 * else.
 */

#ifndef OBSCENE_RUNTIME_H
#define OBSCENE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

/* Writes bytes to the report stream. Never partial: loops until the whole buffer
 * has gone, because a truncated report line is worse than a missing one - it looks
 * like data. */
void obs_write(const char *bytes, size_t len);

/* Bootstrap the output channel from payload_args[0] (getpid) for a raw-payload run, where
 * no import is resolved and the weak write symbol is null. A no-op unless word 0 is shaped
 * like a libkernel export. Call once at entry. See src/runtime.c. */
void obs_bootstrap_payload_output(unsigned long payload_args_word0);

/* libkernel's runtime base a payload entry established (payload_args[0] - getpid's vaddr), or
 * zero on any build not loaded as an elfldr payload. What 139-exports confirms exports against.
 * See src/runtime.c. */
unsigned long obs_libkernel_base(void);
long obs_invoke_syscall(long num, long a1, long a2, long a3, long a4, long a5, long a6);

/* Point every subsequent record at an extra destination, or clear it with NULL.
 *
 * Used by the `report` verb to copy the suite's records down the command socket while it
 * runs, so a driver gets the full stream between its `ack` and `done` rather than only the
 * summary. Additive: the report still reaches stdout and the file sink. See src/runtime.c. */
void obs_set_write_tee(void (*fn)(void *ctx, const char *bytes, size_t len), void *ctx);

/* Writes a NUL-terminated string. */
void obs_puts(const char *s);

/* Which output channel the report went out through, or "none".
 *
 * Reported rather than assumed: an emulator was found stubbing sceKernelWrite, and a
 * run that had to fall back has said something about the platform before a single
 * check has run. */
const char *obs_output_channel_name(void);

/* Appends a decimal or hexadecimal rendering of `value` to `dest`, returning the
 * number of bytes written. `dest` must hold at least OBS_NUM_MAX bytes.
 *
 * Split out from the writing so the formatting is exercised by the host-side unit
 * tests, which cannot call the platform at all. */
#define OBS_NUM_MAX 24
size_t obs_format_u64(char *dest, uint64_t value);
size_t obs_format_i64(char *dest, int64_t value);
size_t obs_format_hex(char *dest, uint64_t value);

size_t obs_strlen(const char *s);
int obs_strcmp(const char *a, const char *b);
void obs_compute_nid(const char *name, char out_nid[12]);

struct payload_args;
const struct payload_args *obs_get_payload_args(void);

/* Exists solely so the symbol census has something that must resolve. */
extern const char obs_census_control_present;

/* Walk the runtime linker's link-map, calling cb(name, base, user) for each loaded object;
 * returns the number walked (0 if the link-map could not be reached). cb returns nonzero to
 * stop early. Syscall-free - it reads only what the loader wrote - so it works where
 * sceKernelGetModuleInfo is refused. See src/runtime.c and src/sections/modlink.c. */
unsigned int obs_linkmap_walk(int (*cb)(const char *name, unsigned long base, void *user),
                              void *user, const char **reason);

/* Name the execution context this run measures in - "<delivery>/<generation>", e.g.
 * "payload/ps4-bc" or "payload/ps5-native" - into `name`, with a human-readable basis into
 * `basis`. Derived rather than declared: the build, the payload anchor, and which GPU library
 * the link-map shows mapped. Distinct from a check's OBS_FROM_* provenance, which is where the
 * *expectation* came from; this is where the *measurement* was taken. See src/runtime.c. */
void obs_run_context(char *name, size_t name_cap, char *basis, size_t basis_cap);

/* In-process dynamic symbol binder: walks the payload's .dynamic relocations (R_X86_64_64,
 * R_X86_64_GLOB_DAT, R_X86_64_JUMP_SLOT) and dynamically resolves weak import symbols against
 * all loaded module handles using sys_dynlib_dlsym. Enables full native PS5 execution. */
void obs_bind_dynamic_symbols(void);

#endif /* OBSCENE_RUNTIME_H */
