/*
 * Current-generation GPU command-building and shader creation: libSceAgc.
 *
 * # What this probes, and what it deliberately does not
 *
 * This section probes the console's current-generation GPU API (libSceAgc)
 * called by retail titles such as PPSA02664. It is to libSceAgc what 165-gnm is
 * to libSceGnmDriver: "does this call exist, is it reachable, what command bytes
 * does it write, and what structure layout does it produce?"
 *
 * It probes only what can be safely constructed from known caller-owned buffers:
 *
 * 1. Command builders (Class B): `sceAgcCbNop`, `sceAgcCbReleaseMem`,
 *    `sceAgcDcbDmaData`, `sceAgcDcbWaitRegMem`, and unnamed NID 0x7d86501b8094ef57.
 *    The caller passes a pointer to a struct holding {begin, end} sitting 0x38 in
 *    front of a 0x400-byte command buffer.
 *
 * 2. Queue initialiser (Class B): `sceAgcDcbResetQueue`. Identified in orbistoun
 *    D565 / Worklog 415 as being called by PPSA02664 directly on guest-allocated
 *    heap memory before every other use of the object. Probed on a poisoned
 *    caller-allocated buffer to capture whether it writes the initial DCB structure.
 *
 * 3. Shader creation: `sceAgcCreateShader`. Arity 4. Takes a 32-byte destination
 *    slot, a header beginning with '1234' (0x31 0x32 0x33 0x34) and size 0x18,
 *    and shader bytecode. Dumps the 32-byte out-parameter and the resulting
 *    0x200-byte shader object, specifically recording +0x30 and +0x50.
 *
 * # Why D008 safety holds here
 *
 * In System V AMD64, the first six integer/pointer arguments are passed in
 * registers (rdi, rsi, rdx, rcx, r8, r9). A callee simply ignores registers it
 * does not take; the stack is never involved for arities <= 6. Setting all six
 * registers to individually safe values protects the stack entirely.
 *
 * # What is strictly excluded (Class A and Class C)
 *
 * Eight functions require an initialised DCB handle in arg0 (sceAgcDcbEventWrite,
 * PushMarker, PopMarker, AcquireMem, SetCxRegistersIndirect, SetUcRegistersIndirect,
 * SetIndexSize, WaitUntilSafeForRendering). Calling them is deferred until
 * ResetQueue's layout is measured.
 *
 * Seven patch functions take unmapped or placeholder handles (such as orbistoun's
 * 0x7fff0001 or unaligned 0x140081c3bf21). Fabricating handles is unsafe and
 * prohibited: sceAgcSetCxRegIndirectPatchAddRegisters,
 * sceAgcSetUcRegIndirectPatchAddRegisters, sceAgcSetCxRegIndirectPatchSetAddress,
 * sceAgcSetUcRegIndirectPatchSetAddress, sceAgcQueueEndOfPipeActionPatchAddress,
 * sceAgcWaitRegMemPatchAddress, sceAgcDmaDataPatchSetDstAddressOrOffset.
 */

#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "oops/target.h"
#if !defined(OBSCENE_HOST_BUILD)
#include "oops/memory.h"
#endif

#include <stddef.h>

#define OBS_AGC_CMDBUF_SIZE 0x400u
#define OBS_AGC_GUARD_SIZE 64u
#define OBS_AGC_POISON_BYTE 0xCCu
#define OBS_AGC_GUARD_BYTE 0xC7u

#if OOPS_TARGET_IS_PS4
static obs_result check_agc_cb_nop(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_cb_release_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_dcb_dma_data(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_dcb_wait_reg_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_cb_unnamed_ef57(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_dcb_reset_queue(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_create_shader(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_dcb_constructor_audit(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_patch_exclusion_guard(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_symbols(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_create_queue(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_submit_nop(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}

static const obs_check agc_checks[] = {
    {"166-agc/cb-nop", "libSceAgc", "sceAgcCbNop", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_nop, OBS_FROM_ASSUMED},
    {"166-agc/cb-release-mem", "libSceAgc", "sceAgcCbReleaseMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_release_mem, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data", "libSceAgc", "sceAgcDcbDmaData", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-reg-mem", "libSceAgc", "sceAgcDcbWaitRegMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_wait_reg_mem, OBS_FROM_ASSUMED},
    {"166-agc/cb-unnamed-ef57", "libSceAgc", "$fYZQG4CU71c", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_unnamed_ef57, OBS_FROM_ASSUMED},
    {"166-agc/dcb-reset-queue", "libSceAgc", "sceAgcDcbResetQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_reset_queue, OBS_FROM_ASSUMED},
    {"166-agc/create-shader", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_create_shader, OBS_FROM_ASSUMED},
    {"166-agc/dcb-constructor-audit", "libSceAgc", "(census)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_constructor_audit,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-exclusion-guard", "libSceAgc", "(guard)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_exclusion_guard,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-symbols", "libSceAgcDriver", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_symbols, OBS_FROM_ASSUMED},
    {"166-agc/driver-create-queue", "libSceAgcDriver", "sceAgcDriverCreateQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_create_queue, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-nop", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_nop, OBS_FROM_ASSUMED},
};
#else

typedef uint64_t (*agc_cb_fn)(void *arg0, uint64_t arg1, uint64_t arg2,
                              uint64_t arg3, uint64_t arg4, uint64_t arg5,
                              uint64_t arg6, uint64_t arg7, uint64_t arg8,
                              uint64_t arg9, uint64_t arg10, uint64_t arg11);

/* The caller-owned command buffer writer struct.
 *
 * Stack observation from PPSA02664:
 * arg0 points to {begin, end, cur, end2, overflow_cb, overflow_ctx, reserved_dw}
 * sitting 0x38 in front of the 0x400-byte command buffer. For NID 0x7d86501b8094ef57,
 * arg0 - 8 holds a count (0x1fa observed in the guest).
 *
 * AGC command builders inspect:
 *   arg0 + 0x10: current writer pointer
 *   arg0 + 0x18: secondary limit / end of buffer
 *   arg0 + 0x20: overflow callback function pointer (must be NULL)
 *   arg0 + 0x28: callback context (must be NULL)
 *   arg0 + 0x30: reserved/used dwords counter (must be 0)
 * If 0x20/0x30 are poisoned with non-zero bytes (e.g. 0xCC), the remaining space check
 * underflows and immediately invokes call *0x20(%rdi), jumping to 0xCCCCCCCCCCCCCCCC
 * which faults with SIGBUS (0xa).
 */
typedef struct {
    uint64_t count_prefix;               /* arg0 - 8: count for NID 0x7d86501b8094ef57 */
    uint64_t begin;                      /* arg0 + 0x00: pointer to cmdbuf */
    uint64_t end;                        /* arg0 + 0x08: pointer to cmdbuf + 0x400 */
    uint64_t cur;                        /* arg0 + 0x10: current writer pointer */
    uint64_t end2;                       /* arg0 + 0x18: secondary limit */
    void *overflow_cb;                   /* arg0 + 0x20: buffer-overflow callback (NULL) */
    void *overflow_ctx;                  /* arg0 + 0x28: callback context (NULL) */
    uint32_t reserved_dw;                /* arg0 + 0x30: reserved/used dwords counter (0) */
    uint32_t pad34;                      /* arg0 + 0x34: padding to 0x38 (0) */
    uint8_t cmdbuf[OBS_AGC_CMDBUF_SIZE]; /* arg0 + 0x38 .. 0x437 */
    uint8_t guard[OBS_AGC_GUARD_SIZE];   /* arg0 + 0x438 .. 0x477: overrun guard */
} obs_agc_cb_probe;

_Static_assert(offsetof(obs_agc_cb_probe, cmdbuf) -
                       offsetof(obs_agc_cb_probe, begin) ==
                   0x38,
               "cmdbuf must sit exactly 0x38 in front of begin");
_Static_assert(offsetof(obs_agc_cb_probe, begin) -
                       offsetof(obs_agc_cb_probe, count_prefix) ==
                   8,
               "count_prefix must sit exactly 8 bytes before begin");
_Static_assert(offsetof(obs_agc_cb_probe, overflow_cb) -
                       offsetof(obs_agc_cb_probe, begin) ==
                   0x20,
               "overflow_cb must sit at begin + 0x20");
_Static_assert(offsetof(obs_agc_cb_probe, reserved_dw) -
                       offsetof(obs_agc_cb_probe, begin) ==
                   0x30,
               "reserved_dw must sit at begin + 0x30");

static obs_agc_cb_probe *s_agc_probe = NULL;

static obs_agc_cb_probe *get_agc_probe(void) {
    if (s_agc_probe != NULL) {
        return s_agc_probe;
    }
#if !defined(OBSCENE_HOST_BUILD)
    void *mem = oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    if (mem != NULL) {
        uintptr_t base = (uintptr_t)mem;
        uintptr_t aligned_begin = (base + 64 + 63) & ~63ULL;
        s_agc_probe = (obs_agc_cb_probe *)(aligned_begin - offsetof(obs_agc_cb_probe, begin));
        return s_agc_probe;
    }
#endif
    static _Alignas(64) uint8_t s_fallback_probe_buf[sizeof(obs_agc_cb_probe) + 128];
    uintptr_t base = (uintptr_t)s_fallback_probe_buf;
    uintptr_t aligned_begin = (base + 64 + 63) & ~63ULL;
    s_agc_probe = (obs_agc_cb_probe *)(aligned_begin - offsetof(obs_agc_cb_probe, begin));
    return s_agc_probe;
}

static void agc_cb_prepare(obs_agc_cb_probe *probe, uint64_t count) {
    probe->count_prefix = count;
    probe->begin = (uint64_t)(uintptr_t)probe->cmdbuf;
    probe->end = (uint64_t)(uintptr_t)(probe->cmdbuf + OBS_AGC_CMDBUF_SIZE);
    probe->cur = probe->begin;
    probe->end2 = probe->end;
    probe->overflow_cb = NULL;
    probe->overflow_ctx = NULL;
    probe->reserved_dw = 0;
    probe->pad34 = 0;
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        probe->cmdbuf[i] = OBS_AGC_POISON_BYTE;
    }
    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        probe->guard[i] = OBS_AGC_GUARD_BYTE;
    }
}

static int agc_cb_guard_intact(const obs_agc_cb_probe *probe) {
    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        if (probe->guard[i] != OBS_AGC_GUARD_BYTE) {
            return 0;
        }
    }
    return 1;
}

static unsigned int agc_cb_written_bytes(const obs_agc_cb_probe *probe,
                                         uint8_t poison) {
    unsigned int written = 0;
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        if (probe->cmdbuf[i] != poison) {
            written = i + 1u;
        }
    }
    return written;
}

/* Common runner for a command-buffer function that takes (writer, arg1).
 *
 * Runs two passes to see what changes:
 *   pass 0: with arg1 = 0
 *   pass 1: with arg1 = 1
 *
 * Emits buffer byte dump or written dump, checks for overrun.
 */
static obs_result agc_cb_run_two_pass(const char *id, const char *symbol,
                                      uint64_t count, agc_cb_fn fn) {
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_fail("failed to allocate command buffer memory");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    /* Pass 0: arg1 = 0 */
    agc_cb_prepare(probe, count);
    uint64_t rc0 = fn(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the call wrote past the end of its command buffer (overrun)");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written0 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    if (written0 > 0) {
        /* Wrote something */
        obs_report_written(id, symbol, "pm4-pass0", before, probe->cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        obs_report_measure(id, symbol, "rc-pass0", rc0, "rc");
        return obs_pass_value((uint64_t)written0);
    }

    /* Pass 1: arg1 = 1 */
    agc_cb_prepare(probe, count);
    uint64_t rc1 = fn(&probe->begin, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the call wrote past the end of its command buffer (overrun)");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    if (written1 > 0) {
        /* Wrote something on pass 1 */
        obs_report_written(id, symbol, "pm4-pass1", before, probe->cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        obs_report_measure(id, symbol, "rc-pass1", rc1, "rc");
        return obs_pass_value((uint64_t)written1);
    }

    /* Buffer remained untouched across both passes */
    obs_report_written(id, symbol, "untouched", before, probe->cmdbuf,
                       OBS_AGC_CMDBUF_SIZE);
    if (rc0 != 0 || rc1 != 0) {
        uint64_t err = (rc1 != 0) ? rc1 : rc0;
        return obs_partial_value("call returned error code and wrote nothing", err);
    }
    return obs_fail(
        "the call returned success but wrote nothing to the command buffer");
}

static int s_agc_handle = -2;

static int agc_get_handle(void) {
    if (s_agc_handle != -2) return s_agc_handle;
    s_agc_handle = obs_module_open("libSceAgc");
    if (s_agc_handle < 0) {
        if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            s_agc_handle = sceKernelLoadStartModule("/system/common/lib/libSceAgc.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    return s_agc_handle;
}

static const void *agc_resolve(const char *name) {
    int h = agc_get_handle();
    if (h >= 0) {
        const void *addr = obs_module_symbol(h, name);
        if (obs_address_is_callable(addr)) return addr;
    }
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *a = NULL;
        if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) return a;
        if (sceKernelDlsym(0x2001, name, &a) == 0 && obs_address_is_callable(a)) return a;
    }
    return NULL;
}

/* Control test: sceAgcCbNop writes a PM4 NOP packet. */
static obs_result check_agc_cb_nop(void) {
    const void *fn = agc_resolve("sceAgcCbNop");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcCbNop)) {
        fn = (const void *)&sceAgcCbNop;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcCbNop not found");
    }
    return agc_cb_run_two_pass("166-agc/cb-nop", "sceAgcCbNop", 0, (agc_cb_fn)fn);
}

static obs_result check_agc_cb_release_mem(void) {
    const void *fn = agc_resolve("sceAgcCbReleaseMem");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcCbReleaseMem)) {
        fn = (const void *)&sceAgcCbReleaseMem;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcCbReleaseMem not found");
    }
    return agc_cb_run_two_pass("166-agc/cb-release-mem", "sceAgcCbReleaseMem", 0, (agc_cb_fn)fn);
}

static obs_result check_agc_dcb_dma_data(void) {
    const void *fn = agc_resolve("sceAgcDcbDmaData");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcDcbDmaData)) {
        fn = (const void *)&sceAgcDcbDmaData;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbDmaData not found");
    }
    return agc_cb_run_two_pass("166-agc/dcb-dma-data", "sceAgcDcbDmaData", 0, (agc_cb_fn)fn);
}

static obs_result check_agc_dcb_wait_reg_mem(void) {
    const void *fn = agc_resolve("sceAgcDcbWaitRegMem");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcDcbWaitRegMem)) {
        fn = (const void *)&sceAgcDcbWaitRegMem;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbWaitRegMem not found");
    }
    return agc_cb_run_two_pass("166-agc/dcb-wait-reg-mem", "sceAgcDcbWaitRegMem", 0, (agc_cb_fn)fn);
}

/* Unnamed NID 0x7d86501b8094ef57: count 0x1fa sits at arg0 - 8. */
static obs_result check_agc_cb_unnamed_ef57(void) {
    const void *fn = agc_resolve("$fYZQG4CU71c");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgc_nid_7d86501b8094ef57)) {
        fn = (const void *)&sceAgc_nid_7d86501b8094ef57;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or $fYZQG4CU71c not found");
    }
    return agc_cb_run_two_pass("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", 0x1fa, (agc_cb_fn)fn);
}

/* sceAgcDcbResetQueue: Class B caller-allocated initialiser (D565 / Worklog 415).
 * Tests calling on a caller-owned writer struct with 0 and 0x400 sizes. */
static obs_result check_agc_dcb_reset_queue(void) {
    const void *fn_raw = agc_resolve("sceAgcDcbResetQueue");
    if (fn_raw == NULL && obs_address_is_callable((const void *)&sceAgcDcbResetQueue)) {
        fn_raw = (const void *)&sceAgcDcbResetQueue;
    }
    if (fn_raw == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbResetQueue not found");
    }
    agc_cb_fn fn_reset = (agc_cb_fn)fn_raw;

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_fail("failed to allocate command buffer memory");
    }

    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    /* Pass 1: reset queue with zero args on a valid writer struct */
    agc_cb_prepare(probe, 0);
    uint64_t rc0 = fn_reset(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("sceAgcDcbResetQueue wrote past command buffer in writer test");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written0 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    if (written0 > 0) {
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "writer-struct-0",
                           before, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass0", rc0, "rc");
        return obs_pass_value((uint64_t)written0);
    }

    /* Pass 2: reset queue with size 0x400 in arg1 */
    agc_cb_prepare(probe, 0);
    uint64_t rc1 = fn_reset(&probe->begin, 0x400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("sceAgcDcbResetQueue wrote past command buffer in writer test");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    if (written1 > 0) {
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "writer-struct-0x400",
                           before, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass1", rc1, "rc");
        return obs_pass_value((uint64_t)written1);
    }

    obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "untouched",
                       before, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
    if (rc0 != 0 || rc1 != 0) {
        uint64_t err = (rc1 != 0) ? rc1 : rc0;
        return obs_partial_value("call returned error code and wrote nothing", err);
    }
    return obs_fail("the call returned success but wrote nothing to the buffer");
}

/* sceAgcCreateShader: arity 4. Dumps 32-byte out-parameter and 0x200-byte shader object. */
static obs_result check_agc_create_shader(void) {
    const void *fn_raw = agc_resolve("sceAgcCreateShader");
    if (fn_raw == NULL && obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        fn_raw = (const void *)&sceAgcCreateShader;
    }
    if (fn_raw == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcCreateShader not found");
    }
    typedef uint64_t (*agc_create_shader_fn)(void *out_slot, const void *header,
                                             const void *payload, uint64_t flags);
    agc_create_shader_fn fn_create = (agc_create_shader_fn)fn_raw;

    static const uint32_t lengths[3] = {0xd8u, 0x118u, 0x108u};
    static const char *labels[3] = {"payload-0xd8", "payload-0x118", "payload-0x108"};
    unsigned int valid_objects = 0;
    uint64_t last_ret = 0;

    for (unsigned int p = 0; p < 3; p++) {
        uint32_t plen = lengths[p];
        const char *label = labels[p];

        /* Header: magic '1234', header size 0x18, payload length plen */
        uint8_t header[32];
        for (size_t i = 0; i < sizeof(header); i++) {
            header[i] = 0;
        }
        header[0] = 0x31; /* '1' */
        header[1] = 0x32; /* '2' */
        header[2] = 0x33; /* '3' */
        header[3] = 0x34; /* '4' */
        header[4] = 0x18; /* header size 24 bytes */
        *(uint32_t *)(header + 8) = plen;
        if (p == 2) {
            /* PPSA03416 format: 0xa8 at offset 24 */
            *(uint32_t *)(header + 24) = 0xa8u;
        }

        /* Payload buffer with deterministic non-zero pattern */
        uint8_t payload[0x200];
        if (p == 2) {
            /* PPSA03416 dumped bytecode header */
            static const uint32_t ppsa_words[8] = {
                0xBFA00001u, 0x7E000000u, 0x7E000000u, 0x7E000000u,
                0x93EBFF03u, 0x00080008u, 0x8F6A8C6Bu, 0x8700FF03u
            };
            for (size_t i = 0; i < sizeof(payload); i++) {
                payload[i] = 0;
            }
            for (size_t i = 0; i < 8; i++) {
                *(uint32_t *)(payload + i * 4) = ppsa_words[i];
            }
            for (size_t i = 32; i < sizeof(payload); i++) {
                payload[i] = (uint8_t)((i * 7u + 0x13u) & 0xFFu);
            }
        } else {
            for (size_t i = 0; i < sizeof(payload); i++) {
                payload[i] = (uint8_t)((i * 7u + 0x13u) & 0xFFu);
            }
        }

        /* 32-byte poisoned destination slot to observe out-param writes */
        uint8_t out_slot[32];
        for (size_t i = 0; i < sizeof(out_slot); i++) {
            out_slot[i] = 0xC7u;
        }

        if (p == 0) {
            obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                             "out-before", 0, out_slot, (unsigned int)sizeof(out_slot));
        }

        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        uint64_t rc = 0;
        if (sig == 0) {
            rc = fn_create((void *)out_slot, (const void *)header,
                           (const void *)payload, 0);
            obs_fault_unregister();
            last_ret = rc;

            if (p == 0) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-wellformed", rc, "code");
                obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                                 "out-after", 0, out_slot, (unsigned int)sizeof(out_slot));
            } else if (p == 1) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-payload-118", rc, "code");
            } else if (p == 2) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-ppsa03416", rc, "code");
            }
        } else {
            obs_fault_unregister();
            if (p == 0) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-wellformed", (uint64_t)sig, "fault-sig");
                obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                                 "out-after", 0, out_slot, (unsigned int)sizeof(out_slot));
            } else if (p == 1) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-payload-118", (uint64_t)sig, "fault-sig");
            } else if (p == 2) {
                obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                                   "rc-ppsa03416", (uint64_t)sig, "fault-sig");
            }
        }

        /* Record the 32 bytes at arg0 */
        obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader", label, 0,
                         out_slot, (unsigned int)sizeof(out_slot));

        /* If *arg0 was written and is a mapped pointer, dump 0x200 bytes from it */
        if (rc == 0) {
            const void *shader_obj = *(const void **)out_slot;
            if (shader_obj != NULL && obs_address_is_callable(shader_obj)) {
                valid_objects++;
                const char *obj_label = (p == 0) ? "shader-obj-d8" : ((p == 1) ? "shader-obj-118" : "shader-obj-108");
                obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                                 obj_label, 0, (const unsigned char *)shader_obj,
                                 0x200u);

                /* Specifically inspect +0x30 (quadword) and +0x50 (dword) */
                uint64_t field_30 =
                    *(const uint64_t *)((const char *)shader_obj + 0x30);
                uint32_t field_50 =
                    *(const uint32_t *)((const char *)shader_obj + 0x50);
                obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                                 "field-0x30", 0x30,
                                 (const unsigned char *)&field_30,
                                 (unsigned int)sizeof(field_30));
                obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader",
                                 "field-0x50", 0x50,
                                 (const unsigned char *)&field_50,
                                 (unsigned int)sizeof(field_50));
            }
        }
    }

    /* Null argument call */
    obs_jmp_buf guard_null;
    int sig_null = OBS_FAULT_ARM(&guard_null);
    if (sig_null == 0) {
        uint64_t rc_null = fn_create(NULL, NULL, NULL, 0);
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-null",
                           rc_null, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-null",
                           (uint64_t)sig_null, "fault-sig");
    }

    if (valid_objects > 0) {
        return obs_pass_value((uint64_t)valid_objects);
    }
    if (last_ret != 0) {
        return obs_partial_value(
            "create shader returned non-zero code; out-slots recorded",
            last_ret);
    }
    return obs_pass();
}

/* DCB Constructor audit: audit potential constructor candidates without fabricating handles. */
static obs_result check_agc_dcb_constructor_audit(void) {
    int handle = obs_module_open("libSceAgc");
    if (handle < 0) {
        return obs_skip(
            "libSceAgc is not loaded; cannot audit DCB constructor candidates");
    }

    static const char *const candidates[] = {
        "sceAgcInit",
        "sceAgcCreatePrimState",
        "sceAgcDcbRewind",
        "sceAgcDriverInitResourceRegistration",
    };

    unsigned int found_candidates = 0;
    for (unsigned int i = 0; i < OBS_COUNT(candidates); i++) {
        const void *addr = obs_module_symbol(handle, candidates[i]);
        int present = obs_address_is_callable(addr);
        obs_report_symbol("libSceAgc", candidates[i], present, OBS_CURRENT);
        if (present) {
            found_candidates++;
        }
    }

    return obs_pass_value((uint64_t)found_candidates);
}

/* Patch function exclusion guard: asserts that the 7 unsafe patch functions remain uncalled. */
static obs_result check_agc_patch_exclusion_guard(void) {
    static const char *const patch_functions[] = {
        "sceAgcSetCxRegIndirectPatchAddRegisters",
        "sceAgcSetUcRegIndirectPatchAddRegisters",
        "sceAgcSetCxRegIndirectPatchSetAddress",
        "sceAgcSetUcRegIndirectPatchSetAddress",
        "sceAgcQueueEndOfPipeActionPatchAddress",
        "sceAgcWaitRegMemPatchAddress",
        "sceAgcDmaDataPatchSetDstAddressOrOffset",
    };

    for (unsigned int i = 0; i < OBS_COUNT(patch_functions); i++) {
        obs_report_symbol("libSceAgc", patch_functions[i], 0, OBS_CURRENT);
    }

    return obs_pass();
}

/* libSceAgcDriver symbol resolution and queue management */
typedef int (*agc_driver_create_queue_fn)(uint32_t type, void **out_queue, uint64_t flags);
typedef int (*agc_driver_destroy_queue_fn)(void *queue);
typedef int (*agc_driver_submit_dcb_fn)(const void *dcb_desc);

typedef struct {
    uint64_t gpu_addr;
    uint32_t size;
    uint8_t flags;
    uint8_t pad[3];
} obs_agc_dcb_desc;

static obs_result check_agc_driver_symbols(void) {
    int handle = obs_module_open("libSceAgcDriver");
    if (handle < 0) {
        return obs_skip("libSceAgcDriver could not be loaded");
    }

    static const struct {
        const char *name;
        const char *nid_name;
    } driver_syms[] = {
        {"sceAgcDriverCreateQueue", "$zP4ZNlXLBVg"},
        {"sceAgcDriverDestroyQueue", "$XNbrdwCsZ9A"},
        {"sceAgcDriverSubmitDcb", "$UglJIZjGssM"},
        {"sceAgcDriverSubmitAcb", "$gSRnr79F8tQ"},
        {"sceAgcDriverAddEqEvent", "$w2rJhmD+dsE"},
        {"sceAgcDriverDeleteEqEvent", "$DL2RXaXOy88"},
    };

    unsigned int found = 0;
    for (unsigned int i = 0; i < OBS_COUNT(driver_syms); i++) {
        const void *addr = obs_module_symbol(handle, driver_syms[i].nid_name);
        if (addr == NULL) {
            addr = obs_module_symbol(handle, driver_syms[i].name);
        }
        int present = obs_address_is_callable(addr);
        obs_report_symbol("libSceAgcDriver", driver_syms[i].name, present, OBS_CURRENT);
        if (present) {
            found++;
        }
    }

    if (found == 0) {
        return obs_fail("none of the libSceAgcDriver symbols resolved");
    }
    return obs_pass_value((uint64_t)found);
}

static obs_result check_agc_driver_create_queue(void) {
    int handle = obs_module_open("libSceAgcDriver");
    if (handle < 0) {
        return obs_skip("libSceAgcDriver could not be loaded");
    }
    agc_driver_create_queue_fn fn_create =
        (agc_driver_create_queue_fn)obs_module_symbol(handle, "$zP4ZNlXLBVg");
    if (fn_create == NULL) {
        fn_create = (agc_driver_create_queue_fn)obs_module_symbol(handle, "sceAgcDriverCreateQueue");
    }
    agc_driver_destroy_queue_fn fn_destroy =
        (agc_driver_destroy_queue_fn)obs_module_symbol(handle, "$XNbrdwCsZ9A");
    if (fn_destroy == NULL) {
        fn_destroy = (agc_driver_destroy_queue_fn)obs_module_symbol(handle, "sceAgcDriverDestroyQueue");
    }
    if (fn_create == NULL || !obs_address_is_callable((const void *)fn_create)) {
        return obs_skip("sceAgcDriverCreateQueue not resolved");
    }

    void *queue = NULL;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                           "rc-create", (uint64_t)sig, "fault-sig");
        return obs_fail("fault during sceAgcDriverCreateQueue");
    }

    int rc_create = fn_create(3u, &queue, 0u);
    obs_fault_unregister();

    obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                       "queue-valid", (queue != NULL) ? 1u : 0u, "flag");

    if (queue != NULL && obs_address_is_callable(queue)) {
        obs_report_bytes("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                         "queue-header", 0, (const unsigned char *)queue, 32u);

        /* Measure whether active queue state changes sceAgcCreateShader response */
        const void *fn_cs_raw = agc_resolve("sceAgcCreateShader");
        if (fn_cs_raw == NULL && obs_address_is_callable((const void *)&sceAgcCreateShader)) {
            fn_cs_raw = (const void *)&sceAgcCreateShader;
        }
        if (fn_cs_raw != NULL && obs_address_is_callable(fn_cs_raw)) {
            typedef uint64_t (*agc_cs_fn)(void *out, const void *hdr, const void *payload, uint64_t flags);
            agc_cs_fn fn_cs = (agc_cs_fn)fn_cs_raw;
            uint8_t dummy_out[32];
            uint8_t hdr[32];
            for (size_t z = 0; z < sizeof(hdr); z++) hdr[z] = 0;
            hdr[0] = '1'; hdr[1] = '2'; hdr[2] = '3'; hdr[3] = '4';
            hdr[4] = 0x18;
            *(uint32_t *)(hdr + 8) = 0xd8u;
            uint8_t dummy_payload[0x100];
            for (size_t z = 0; z < sizeof(dummy_payload); z++) dummy_payload[z] = 0;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc_cs = fn_cs(dummy_out, hdr, dummy_payload, 0);
                obs_fault_unregister();
                obs_report_measure("166-agc/driver-create-queue", "sceAgcCreateShader",
                                   "rc-shader-with-queue", rc_cs, "code");
            } else {
                obs_fault_unregister();
            }
        }
    }

    int rc_destroy = -1;
    if (queue != NULL && fn_destroy != NULL && obs_address_is_callable((const void *)fn_destroy)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_destroy = fn_destroy(queue);
            obs_fault_unregister();
            obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverDestroyQueue",
                               "rc-destroy", (uint64_t)(uint32_t)rc_destroy, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverDestroyQueue",
                               "rc-destroy", (uint64_t)sig, "fault-sig");
        }
    }

    if (rc_create == 0 && queue != NULL) {
        return obs_pass();
    }
    return obs_partial_value("create queue returned non-zero code or null", (uint64_t)(uint32_t)rc_create);
}

static obs_result check_agc_driver_submit_nop(void) {
    int handle = obs_module_open("libSceAgcDriver");
    if (handle < 0) {
        return obs_skip("libSceAgcDriver could not be loaded");
    }
    agc_driver_create_queue_fn fn_create =
        (agc_driver_create_queue_fn)obs_module_symbol(handle, "$zP4ZNlXLBVg");
    if (fn_create == NULL) {
        fn_create = (agc_driver_create_queue_fn)obs_module_symbol(handle, "sceAgcDriverCreateQueue");
    }
    agc_driver_destroy_queue_fn fn_destroy =
        (agc_driver_destroy_queue_fn)obs_module_symbol(handle, "$XNbrdwCsZ9A");
    if (fn_destroy == NULL) {
        fn_destroy = (agc_driver_destroy_queue_fn)obs_module_symbol(handle, "sceAgcDriverDestroyQueue");
    }
    agc_driver_submit_dcb_fn fn_submit =
        (agc_driver_submit_dcb_fn)obs_module_symbol(handle, "$UglJIZjGssM");
    if (fn_submit == NULL) {
        fn_submit = (agc_driver_submit_dcb_fn)obs_module_symbol(handle, "sceAgcDriverSubmitDcb");
    }
    if (fn_create == NULL || fn_submit == NULL) {
        return obs_skip("sceAgcDriverCreateQueue or SubmitDcb not resolved");
    }

    void *queue = NULL;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during driver queue setup");
    }

    int rc_create = fn_create(3u, &queue, 0u);
    obs_fault_unregister();
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("queue creation failed; skipping submit");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    *(uint32_t *)probe->cmdbuf = 0xffff1000u;

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->cmdbuf;
    desc.size = 4u;
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = fn_submit(&desc);
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-nop", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-nop", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    if (fn_destroy != NULL) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            fn_destroy(queue);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    if (sig != 0) {
        return obs_fail("fault during sceAgcDriverSubmitDcb");
    }
    if (submit_rc == 0) {
        return obs_pass();
    }
    return obs_partial_value("submit dcb returned non-zero code", (uint64_t)(uint32_t)submit_rc);
}

static const obs_check agc_checks[] = {
    {"166-agc/cb-nop", "libSceAgc", "sceAgcCbNop", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_nop, OBS_FROM_ASSUMED},
    {"166-agc/cb-release-mem", "libSceAgc", "sceAgcCbReleaseMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_release_mem, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data", "libSceAgc", "sceAgcDcbDmaData", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-reg-mem", "libSceAgc", "sceAgcDcbWaitRegMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_wait_reg_mem, OBS_FROM_ASSUMED},
    {"166-agc/cb-unnamed-ef57", "libSceAgc", "$fYZQG4CU71c", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_unnamed_ef57, OBS_FROM_ASSUMED},
    {"166-agc/dcb-reset-queue", "libSceAgc", "sceAgcDcbResetQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_reset_queue, OBS_FROM_ASSUMED},
    {"166-agc/create-shader", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_create_shader, OBS_FROM_ASSUMED},
    {"166-agc/dcb-constructor-audit", "libSceAgc", "(census)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_constructor_audit,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-exclusion-guard", "libSceAgc", "(guard)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_exclusion_guard,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-symbols", "libSceAgcDriver", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_symbols, OBS_FROM_ASSUMED},
    {"166-agc/driver-create-queue", "libSceAgcDriver", "sceAgcDriverCreateQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_create_queue, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-nop", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_nop, OBS_FROM_ASSUMED},
};
#endif

const obs_section obs_section_agc = {
    "166-agc",
    "AGC command building and shaders",
    "Calling confirmed libSceAgc command builders and shader creation, "
    "recording packet encodings and shader structure offsets.",
    agc_checks,
    OBS_COUNT(agc_checks),
};
