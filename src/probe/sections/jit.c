/*
 * Executable memory: can this process JIT.
 *
 * # Why this section exists
 *
 * A recompiler - the heart of any fast emulator, and the whole reason a PS2 emulator is
 * even conceivable on this hardware - writes machine code at run time and then executes
 * it. The platform enforces W^X: a page is writable or executable, never both, and code
 * that is not signed does not run. The one sanctioned way through is the JIT interface:
 * create a shared memory object, then map it twice - a writable alias to emit code
 * into, an executable alias to run it from. `sceKernelJitCreateSharedMemory` is the
 * door.
 *
 * The census places these symbols in `libkernel_ps2emu` and `libkernel_jvm` as well as
 * plain `libkernel` - the platform's own PS2 emulator and Java VM use exactly this
 * path. So the question is not whether the interface exists but whether *this* process,
 * a homebrew payload, is permitted to walk through it. That is a single, cheap
 * measurement, and it is the go/no-go for the entire idea of native emulation here: if
 * create is refused, no recompiler runs and the CPU's speed is irrelevant.
 *
 * # What it measures, and what it does not
 *
 * It calls create and reports the result faithfully - a valid handle means the process
 * may make executable memory, a negative code means it was refused and which code. If
 * create succeeds it goes one step further and asks for the two mappings a recompiler
 * needs, because "created" and "dual-mapped" are different permissions and a reader
 * deciding whether to port an emulator needs both answered.
 *
 * It stops short of writing code and jumping to it. Executing bytes this program wrote
 * is a larger claim than a conformance probe should make on its own, and the mapping
 * result already tells the story the port needs. The execute step is the confirmation a
 * first payload makes, not this.
 *
 * # Provenance
 *
 * The signatures are documented (ps4libdoc, the OpenOrbis toolchain), which is why this
 * calls them rather than leaving them out under section-2 of the conventions. The
 * maximum-protection value is the documented CPU read/write/execute; a wrong value
 * would make create refuse with EINVAL rather than corrupt anything, and the refusal
 * code is reported, so even that is a legible result rather than a crash.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Documented CPU protections for the JIT object: read, write, execute. The create call
 * takes the *maximum* protection the object may ever be mapped with; the individual
 * maps below take a subset each. (ps4libdoc / OpenOrbis) */
#define OBS_JIT_PROT_READ 0x01
#define OBS_JIT_PROT_WRITE 0x02
#define OBS_JIT_PROT_EXEC 0x04
#define OBS_JIT_PROT_RWX (OBS_JIT_PROT_READ | OBS_JIT_PROT_WRITE | OBS_JIT_PROT_EXEC)

/* One page is enough to answer the permission question. */
#define OBS_JIT_SIZE 0x4000u

typedef int (*fn_jit_create_t)(const char *name, size_t size, int flags, int *handle);
typedef int (*fn_jit_map_t)(int handle, int flags, void **addr);
typedef int (*fn_jit_alias_t)(int handle, int flags, void **addr);

static fn_jit_create_t resolve_jit_create(void) {
    if (obs_address_is_callable((const void *)&sceKernelJitCreateSharedMemory)) {
        return sceKernelJitCreateSharedMemory;
    }
    int h = obs_module_open("libkernel");
    if (h >= 0) {
        return (fn_jit_create_t)obs_module_symbol(h, "sceKernelJitCreateSharedMemory");
    }
    return NULL;
}

static fn_jit_map_t resolve_jit_map(void) {
    if (obs_address_is_callable((const void *)&sceKernelJitMapSharedMemory)) {
        return sceKernelJitMapSharedMemory;
    }
    int h = obs_module_open("libkernel");
    if (h >= 0) {
        return (fn_jit_map_t)obs_module_symbol(h, "sceKernelJitMapSharedMemory");
    }
    return NULL;
}

static fn_jit_alias_t resolve_jit_alias(void) {
    if (obs_address_is_callable((const void *)&sceKernelJitCreateAliasOfSharedMemory)) {
        return sceKernelJitCreateAliasOfSharedMemory;
    }
    int h = obs_module_open("libkernel");
    if (h >= 0) {
        return (fn_jit_alias_t)obs_module_symbol(
            h, "sceKernelJitCreateAliasOfSharedMemory");
    }
    return NULL;
}

/*
 * Can the process create JIT-backed executable memory at all.
 *
 * This is the measurement the whole section is for. A handle back means yes; a negative
 * return means no, and the code says why (a permission refusal reads differently from a
 * bad argument).
 */
static obs_result check_jit_create(void) {
    fn_jit_create_t fn_create = resolve_jit_create();
    if (!obs_address_is_callable((const void *)fn_create)) {
        return obs_skip(
            "sceKernelJitCreateSharedMemory does not resolve in this process");
    }

    int handle = -1;
    int rc = fn_create("obscene-jit", OBS_JIT_SIZE, OBS_JIT_PROT_RWX, &handle);
    if (rc != 0) {
        return obs_fail_code("the process was refused JIT shared memory",
                             (uint64_t)(unsigned int)rc);
    }
    if (handle < 0) {
        return obs_fail("create reported success but handed back no handle");
    }
    /* The handle is left open. There is no confirmed release call for it, and the probe
     * process is about to end, so closing on a guess would be inventing an interface to
     * tidy up one. */
    return obs_pass_value((uint64_t)(unsigned int)handle);
}

/*
 * Given create works, can the object be mapped the two ways a recompiler needs.
 *
 * Executable via `Map`, writable via `CreateAlias`. A recompiler emits through the
 * writable alias and runs through the executable one; if either mapping is refused, the
 * dual-mapping a JIT depends on is not available even though creation was.
 */
static obs_result check_jit_dual_map(void) {
    fn_jit_create_t fn_create = resolve_jit_create();
    fn_jit_map_t fn_map = resolve_jit_map();
    fn_jit_alias_t fn_alias = resolve_jit_alias();

    if (!obs_address_is_callable((const void *)fn_create) ||
        !obs_address_is_callable((const void *)fn_map) ||
        !obs_address_is_callable((const void *)fn_alias)) {
        return obs_skip("a JIT mapping entry point does not resolve in this process");
    }

    int handle = -1;
    int rc = fn_create("obscene-jit-map", OBS_JIT_SIZE, OBS_JIT_PROT_RWX, &handle);
    if (rc != 0 || handle < 0) {
        return obs_skip("JIT create was refused, so mapping cannot be reached");
    }

    void *exec_view = 0;
    int mrc = fn_map(handle, OBS_JIT_PROT_READ | OBS_JIT_PROT_EXEC, &exec_view);
    if (mrc != 0 || exec_view == 0) {
        return obs_fail_code("the executable mapping was refused",
                             (uint64_t)(unsigned int)mrc);
    }

    void *write_view = 0;
    int arc = fn_alias(handle, OBS_JIT_PROT_READ | OBS_JIT_PROT_WRITE, &write_view);
    if (arc != 0 || write_view == 0) {
        return obs_partial_value("execute mapped, but the writable alias was refused",
                                 (uint64_t)(unsigned int)arc);
    }

    /* Both mappings exist and are different addresses for the same object - the shape a
     * recompiler writes through one and runs through the other. Not executed here (see
     * the header): the confirmation belongs to a first payload, not a probe. */
    return obs_pass();
}

static const obs_check jit_checks[] = {
    {"155-jit/create", "libkernel", "sceKernelJitCreateSharedMemory", OBS_CAP_MEMORY,
     OBS_CAP_NONE, (const void *)&sceKernelJitCreateSharedMemory, check_jit_create,
     OBS_FROM_DOCUMENTED},
    {"155-jit/dual-map", "libkernel", "sceKernelJitMapSharedMemory", OBS_CAP_MEMORY,
     OBS_CAP_NONE, (const void *)&sceKernelJitMapSharedMemory, check_jit_dual_map,
     OBS_FROM_DOCUMENTED},
};

const obs_section obs_section_jit = {
    "155-jit",
    "Executable memory (JIT)",
    "Whether a homebrew process may create and dual-map executable memory - the "
    "sanctioned "
    "path around W^X that a recompiler needs, and the one the platform's own PS2 "
    "emulator "
    "takes. The go/no-go for native emulation: if create is refused, no JIT runs and "
    "the "
    "CPU's speed does not matter.",
    jit_checks,
    OBS_COUNT(jit_checks),
};
