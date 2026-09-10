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
#include "obscene/runtime.h"
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
static obs_result check_agc_init(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_queue_types(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_submit_nop(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_submit_batch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_driver_submit_fence(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_compute_dispatch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_graphics_submit(void) {
    return obs_skip("libSceAgc is current-generation; excluded from PS4 target");
}
static obs_result check_agc_shader_differential(void) {
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
    {"166-agc/cb-unnamed-ef57", "libSceAgc", "$fYZQG4CU71c", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_unnamed_ef57, OBS_FROM_ASSUMED},
    {"166-agc/dcb-reset-queue", "libSceAgc", "sceAgcDcbResetQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_reset_queue, OBS_FROM_ASSUMED},
    {"166-agc/init", "libSceAgc", "sceAgcInit", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_init, OBS_FROM_ASSUMED},
    {"166-agc/create-shader", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_create_shader, OBS_FROM_ASSUMED},
    {"166-agc/dcb-constructor-audit", "libSceAgc", "(census)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_constructor_audit, OBS_FROM_ASSUMED},
    {"166-agc/patch-exclusion-guard", "libSceAgc", "(guard)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_exclusion_guard, OBS_FROM_ASSUMED},
    {"166-agc/driver-symbols", "libSceAgcDriver", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_symbols, OBS_FROM_ASSUMED},
    {"166-agc/driver-create-queue", "libSceAgcDriver", "sceAgcDriverCreateQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_create_queue,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-queue-types", "libSceAgcDriver", "sceAgcDriverCreateQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_queue_types,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-nop", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_nop,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-batch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_batch,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-fence", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_fence,
     OBS_FROM_ASSUMED},
    {"166-agc/compute-dispatch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_compute_dispatch,
     OBS_FROM_ASSUMED},
    {"166-agc/graphics-submit", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_graphics_submit,
     OBS_FROM_ASSUMED},
    {"166-agc/shader-differential", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_shader_differential, OBS_FROM_ASSUMED},
};
#else

typedef uint64_t (*agc_cb_fn)(void *arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                              uint64_t arg4, uint64_t arg5, uint64_t arg6,
                              uint64_t arg7, uint64_t arg8, uint64_t arg9,
                              uint64_t arg10, uint64_t arg11);

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
    uint64_t count_prefix; /* arg0 - 8: count for NID 0x7d86501b8094ef57 */
    uint64_t begin;        /* arg0 + 0x00: pointer to cmdbuf */
    uint64_t end;          /* arg0 + 0x08: pointer to cmdbuf + 0x400 */
    uint64_t cur;          /* arg0 + 0x10: current writer pointer */
    uint64_t end2;         /* arg0 + 0x18: secondary limit */
    void *overflow_cb;     /* arg0 + 0x20: buffer-overflow callback (NULL) */
    void *overflow_ctx;    /* arg0 + 0x28: callback context (NULL) */
    uint32_t reserved_dw;  /* arg0 + 0x30: reserved/used dwords counter (0) */
    uint32_t pad34;        /* arg0 + 0x34: padding to 0x38 (0) */
    uint8_t cmdbuf[OBS_AGC_CMDBUF_SIZE]; /* arg0 + 0x38 .. 0x437 */
    uint8_t guard[OBS_AGC_GUARD_SIZE];   /* arg0 + 0x438 .. 0x477: overrun guard */
} obs_agc_cb_probe;

_Static_assert(offsetof(obs_agc_cb_probe, cmdbuf) - offsetof(obs_agc_cb_probe, begin) ==
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
        s_agc_probe =
            (obs_agc_cb_probe *)(aligned_begin - offsetof(obs_agc_cb_probe, begin));
        return s_agc_probe;
    }
#endif
    static _Alignas(64) uint8_t s_fallback_probe_buf[sizeof(obs_agc_cb_probe) + 128];
    uintptr_t base = (uintptr_t)s_fallback_probe_buf;
    uintptr_t aligned_begin = (base + 64 + 63) & ~63ULL;
    s_agc_probe =
        (obs_agc_cb_probe *)(aligned_begin - offsetof(obs_agc_cb_probe, begin));
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
    if (s_agc_handle != -2)
        return s_agc_handle;
    s_agc_handle = obs_module_open("libSceAgc");
    if (s_agc_handle < 0) {
        if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            s_agc_handle = sceKernelLoadStartModule("/system/common/lib/libSceAgc.sprx",
                                                    0, NULL, 0, NULL, NULL);
        }
    }
    return s_agc_handle;
}

static const void *agc_resolve(const char *name) {
    int h = agc_get_handle();
    if (h >= 0) {
        const void *addr = obs_module_symbol(h, name);
        if (obs_address_is_callable(addr))
            return addr;
    }
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *a = NULL;
        if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a))
            return a;
        if (sceKernelDlsym(0x2001, name, &a) == 0 && obs_address_is_callable(a))
            return a;
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
    return agc_cb_run_two_pass("166-agc/cb-release-mem", "sceAgcCbReleaseMem", 0,
                               (agc_cb_fn)fn);
}

static obs_result check_agc_dcb_dma_data(void) {
    const void *fn = agc_resolve("sceAgcDcbDmaData");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcDcbDmaData)) {
        fn = (const void *)&sceAgcDcbDmaData;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbDmaData not found");
    }
    return agc_cb_run_two_pass("166-agc/dcb-dma-data", "sceAgcDcbDmaData", 0,
                               (agc_cb_fn)fn);
}

static obs_result check_agc_dcb_wait_reg_mem(void) {
    const void *fn = agc_resolve("sceAgcDcbWaitRegMem");
    if (fn == NULL && obs_address_is_callable((const void *)&sceAgcDcbWaitRegMem)) {
        fn = (const void *)&sceAgcDcbWaitRegMem;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbWaitRegMem not found");
    }
    return agc_cb_run_two_pass("166-agc/dcb-wait-reg-mem", "sceAgcDcbWaitRegMem", 0,
                               (agc_cb_fn)fn);
}

/* Unnamed NID 0x7d86501b8094ef57: count 0x1fa sits at arg0 - 8. */
static obs_result check_agc_cb_unnamed_ef57(void) {
    const void *fn = agc_resolve("$fYZQG4CU71c");
    if (fn == NULL &&
        obs_address_is_callable((const void *)&sceAgc_nid_7d86501b8094ef57)) {
        fn = (const void *)&sceAgc_nid_7d86501b8094ef57;
    }
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or $fYZQG4CU71c not found");
    }
    return agc_cb_run_two_pass("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", 0x1fa,
                               (agc_cb_fn)fn);
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
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue",
                           "writer-struct-0", before, probe->cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass0",
                           rc0, "rc");
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
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue",
                           "writer-struct-0x400", before, probe->cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass1",
                           rc1, "rc");
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

static const uint8_t agc_retail_hdr_full_0[304] = {
    0x31u, 0x32u, 0x33u, 0x34u, 0x18u, 0x00u, 0x00u, 0x00u, 0xd8u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x70u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x38u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x30u, 0x01u, 0x00u, 0x00u, 0x90u, 0x04u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x0eu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x30u, 0x00u, 0x00u, 0x00u, 0x0au, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x09u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x62u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x0cu, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0du, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x2au, 0x02u, 0x00u, 0x00u, 0x60u, 0x04u, 0x00u, 0x28u,
    0x2au, 0x02u, 0x00u, 0x00u, 0xcdu, 0xe4u, 0xc9u, 0xcau, 0x12u, 0x02u, 0x00u, 0x00u,
    0x84u, 0x00u, 0x2cu, 0x40u, 0x13u, 0x02u, 0x00u, 0x00u, 0x92u, 0x09u, 0x00u, 0x00u,
    0x28u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x07u, 0x02u, 0x00u, 0x00u,
    0x08u, 0x00u, 0x00u, 0x00u, 0x08u, 0x02u, 0x00u, 0x00u, 0x08u, 0x00u, 0x00u, 0x00u,
    0x09u, 0x02u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x38u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x48u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x40u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x38u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x30u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x09u, 0x00u, 0x0bu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0xffu, 0xffu, 0x00u, 0x00u, 0xffu, 0xffu, 0xffu, 0xffu,
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
    0xffu, 0xffu, 0x00u, 0x00u,
};

__attribute__((aligned(256))) static const uint8_t agc_retail_payload_0[1160] = {
    0x03u, 0x00u, 0xa0u, 0xbfu, 0x0au, 0x00u, 0x46u, 0xd7u, 0x0au, 0x06u, 0x05u, 0x04u,
    0x04u, 0x00u, 0x46u, 0xd7u, 0x09u, 0x06u, 0x01u, 0x04u, 0x01u, 0x03u, 0x8cu, 0xbeu,
    0x02u, 0x03u, 0x8du, 0xbeu, 0x03u, 0x03u, 0x8eu, 0xbeu, 0x83u, 0x14u, 0x12u, 0x34u,
    0x00u, 0x00u, 0x48u, 0xd5u, 0x0au, 0x0du, 0x5du, 0x02u, 0x07u, 0x00u, 0x46u, 0xd7u,
    0x04u, 0x0du, 0x21u, 0x02u, 0x84u, 0x14u, 0x10u, 0x34u, 0x0du, 0x00u, 0x48u, 0xd5u,
    0x04u, 0x07u, 0x5du, 0x02u, 0x81u, 0x12u, 0x16u, 0x2cu, 0x82u, 0x12u, 0x04u, 0x2cu,
    0x0cu, 0x00u, 0x69u, 0xd5u, 0x00u, 0x00u, 0x02u, 0x00u, 0x85u, 0x14u, 0x06u, 0x34u,
    0x86u, 0x14u, 0x0cu, 0x34u, 0x88u, 0x16u, 0x1eu, 0x36u, 0xa0u, 0x04u, 0x1cu, 0x36u,
    0x8au, 0x08u, 0x00u, 0x34u, 0x88u, 0x14u, 0x02u, 0x34u, 0x89u, 0x0eu, 0x0au, 0x2cu,
    0x0fu, 0x00u, 0x71u, 0xd7u, 0x82u, 0x04u, 0x3eu, 0x04u, 0x0eu, 0x00u, 0x71u, 0xd7u,
    0x8au, 0x14u, 0x3au, 0x04u, 0x83u, 0x0eu, 0x14u, 0x2cu, 0x02u, 0x00u, 0x47u, 0xd7u,
    0x0du, 0x19u, 0x32u, 0x02u, 0x05u, 0x00u, 0x47u, 0xd7u, 0x05u, 0x19u, 0x32u, 0x02u,
    0x04u, 0x03u, 0x8fu, 0xbeu, 0x0du, 0x00u, 0x71u, 0xd7u, 0xffu, 0x16u, 0x3au, 0x04u,
    0x80u, 0x00u, 0x00u, 0x00u, 0x0eu, 0x00u, 0x71u, 0xd7u, 0xa0u, 0x12u, 0x3eu, 0x04u,
    0x84u, 0x0eu, 0x16u, 0x2cu, 0x09u, 0x00u, 0x71u, 0xd7u, 0x91u, 0x14u, 0xfeu, 0x03u,
    0x00u, 0x03u, 0x00u, 0x00u, 0x0cu, 0x00u, 0x71u, 0xd7u, 0x91u, 0x14u, 0xfeu, 0x03u,
    0x00u, 0x09u, 0x00u, 0x00u, 0x10u, 0x00u, 0x71u, 0xd7u, 0x91u, 0x14u, 0xfeu, 0x03u,
    0x00u, 0x01u, 0x00u, 0x00u, 0x82u, 0x0eu, 0x0eu, 0x2cu, 0x01u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x02u, 0x36u, 0x04u, 0x00u, 0x0fu, 0x00u, 0x00u, 0x0du, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x10u, 0x3au, 0x04u, 0x80u, 0x00u, 0x00u, 0x00u, 0x08u, 0x00u, 0x71u, 0xd7u,
    0x91u, 0x14u, 0xfeu, 0x03u, 0x00u, 0x0bu, 0x00u, 0x00u, 0x86u, 0x08u, 0x14u, 0x34u,
    0x09u, 0x00u, 0x71u, 0xd7u, 0x84u, 0x16u, 0x26u, 0x04u, 0x0eu, 0x00u, 0x71u, 0xd7u,
    0x84u, 0x16u, 0x32u, 0x04u, 0x0cu, 0x00u, 0x71u, 0xd7u, 0xffu, 0x06u, 0x36u, 0x04u,
    0x00u, 0x02u, 0x00u, 0x00u, 0x0fu, 0x00u, 0x71u, 0xd7u, 0x84u, 0x16u, 0x22u, 0x04u,
    0x83u, 0x14u, 0x1au, 0x2cu, 0x0bu, 0x00u, 0x71u, 0xd7u, 0x84u, 0x16u, 0x42u, 0x04u,
    0x10u, 0x00u, 0x71u, 0xd7u, 0xc0u, 0x0eu, 0x26u, 0x04u, 0x0eu, 0x00u, 0x71u, 0xd7u,
    0xc0u, 0x0eu, 0x3au, 0x04u, 0x0fu, 0x00u, 0x71u, 0xd7u, 0xc0u, 0x0eu, 0x3eu, 0x04u,
    0x08u, 0x00u, 0x71u, 0xd7u, 0xffu, 0x0cu, 0x32u, 0x04u, 0x00u, 0x08u, 0x00u, 0x00u,
    0x87u, 0x08u, 0x06u, 0x34u, 0x0cu, 0x00u, 0x71u, 0xd7u, 0x90u, 0x1au, 0xfeu, 0x03u,
    0x00u, 0x0au, 0x00u, 0x00u, 0x88u, 0x08u, 0x08u, 0x34u, 0x11u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x00u, 0x3eu, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x06u, 0x00u, 0x71u, 0xd7u,
    0xc0u, 0x0eu, 0x2eu, 0x04u, 0x84u, 0x14u, 0x0eu, 0x2cu, 0x0bu, 0x00u, 0x71u, 0xd7u,
    0x90u, 0x1au, 0xfeu, 0x03u, 0x00u, 0x08u, 0x00u, 0x00u, 0x05u, 0x03u, 0x80u, 0xbeu,
    0x06u, 0x03u, 0x81u, 0xbeu, 0x09u, 0x00u, 0x71u, 0xd7u, 0xffu, 0x00u, 0x1au, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x84u, 0x0eu, 0x1eu, 0x36u, 0x06u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x00u, 0x3au, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x0eu, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x00u, 0x42u, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x10u, 0x00u, 0x71u, 0xd7u,
    0x90u, 0x1au, 0xfeu, 0x03u, 0x00u, 0x02u, 0x00u, 0x00u, 0x09u, 0x00u, 0x45u, 0xd7u,
    0x01u, 0x13u, 0x16u, 0x04u, 0x12u, 0x00u, 0x71u, 0xd7u, 0x90u, 0x1au, 0x3eu, 0x04u,
    0x82u, 0x14u, 0x1au, 0x2cu, 0x07u, 0x00u, 0x45u, 0xd7u, 0x01u, 0x1du, 0x16u, 0x04u,
    0xc0u, 0x14u, 0x1cu, 0x36u, 0x0au, 0x00u, 0x71u, 0xd7u, 0xc0u, 0x14u, 0x22u, 0x04u,
    0x06u, 0x00u, 0x45u, 0xd7u, 0x01u, 0x0du, 0x16u, 0x04u, 0x12u, 0x00u, 0x71u, 0xd7u,
    0xc0u, 0x1au, 0x4au, 0x04u, 0x05u, 0x00u, 0x45u, 0xd7u, 0x01u, 0x23u, 0x16u, 0x04u,
    0xc0u, 0x1au, 0x22u, 0x36u, 0x0au, 0x00u, 0x71u, 0xd7u, 0xffu, 0x06u, 0x2au, 0x04u,
    0x00u, 0x01u, 0x00u, 0x00u, 0x0du, 0x00u, 0x72u, 0xd7u, 0x90u, 0x10u, 0x3au, 0x04u,
    0x07u, 0x03u, 0x82u, 0xbeu, 0x08u, 0x03u, 0x83u, 0xbeu, 0x13u, 0x00u, 0x72u, 0xd7u,
    0x10u, 0x1fu, 0x46u, 0x04u, 0x0bu, 0x00u, 0x72u, 0xd7u, 0x0bu, 0x1fu, 0x46u, 0x04u,
    0x0cu, 0x00u, 0x72u, 0xd7u, 0x0cu, 0x1fu, 0x46u, 0x04u, 0x0fu, 0x00u, 0x72u, 0xd7u,
    0x84u, 0x10u, 0x3au, 0x04u, 0x0au, 0x00u, 0x71u, 0xd7u, 0xffu, 0x08u, 0x2au, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x11u, 0x00u, 0x71u, 0xd7u, 0xffu, 0x00u, 0x4au, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x10u, 0x00u, 0x71u, 0xd7u, 0xffu, 0x00u, 0x2eu, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x0cu, 0x00u, 0x71u, 0xd7u, 0xffu, 0x00u, 0x32u, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x00u, 0x00u, 0x71u, 0xd7u, 0xffu, 0x00u, 0x4eu, 0x04u,
    0x00u, 0x04u, 0x00u, 0x00u, 0x12u, 0x00u, 0x72u, 0xd7u, 0x94u, 0x10u, 0x3au, 0x04u,
    0x0eu, 0x00u, 0x71u, 0xd7u, 0xffu, 0x06u, 0x3eu, 0x04u, 0x00u, 0x01u, 0x00u, 0x00u,
    0x0bu, 0x00u, 0x45u, 0xd7u, 0x01u, 0x23u, 0x0au, 0x04u, 0x08u, 0x00u, 0x45u, 0xd7u,
    0x01u, 0x21u, 0x0au, 0x04u, 0x00u, 0x00u, 0x45u, 0xd7u, 0x01u, 0x01u, 0x0au, 0x04u,
    0x0cu, 0x00u, 0x45u, 0xd7u, 0x01u, 0x19u, 0x0au, 0x04u, 0x01u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x06u, 0x36u, 0x04u, 0x00u, 0x01u, 0x00u, 0x00u, 0x0du, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x06u, 0x4au, 0x04u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x20u, 0x00u, 0xe0u,
    0x09u, 0x03u, 0x03u, 0x80u, 0x00u, 0x20u, 0x00u, 0xe0u, 0x00u, 0x00u, 0x03u, 0x80u,
    0x00u, 0x20u, 0x00u, 0xe0u, 0x05u, 0x05u, 0x03u, 0x80u, 0x01u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x08u, 0x06u, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x09u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x08u, 0x36u, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x04u, 0x00u, 0x71u, 0xd7u,
    0xffu, 0x08u, 0x3au, 0x04u, 0x00u, 0x04u, 0x00u, 0x00u, 0x02u, 0x03u, 0x1au, 0x4au,
    0x02u, 0x13u, 0x1cu, 0x4au, 0x02u, 0x09u, 0x12u, 0x4au, 0x00u, 0x20u, 0x00u, 0xe0u,
    0x07u, 0x01u, 0x03u, 0x80u, 0x00u, 0x20u, 0x00u, 0xe0u, 0x0cu, 0x04u, 0x03u, 0x80u,
    0x02u, 0x15u, 0x0eu, 0x4au, 0x81u, 0x1au, 0x14u, 0x2cu, 0x81u, 0x12u, 0x12u, 0x2cu,
    0x81u, 0x0eu, 0x18u, 0x2cu, 0x00u, 0x20u, 0x00u, 0xe0u, 0x0bu, 0x02u, 0x03u, 0x80u,
    0x00u, 0x20u, 0x00u, 0xe0u, 0x06u, 0x07u, 0x03u, 0x80u, 0x00u, 0x20u, 0x00u, 0xe0u,
    0x08u, 0x06u, 0x03u, 0x80u, 0x81u, 0x1cu, 0x16u, 0x2cu, 0x72u, 0x3fu, 0x8cu, 0xbfu,
    0x00u, 0x20u, 0x14u, 0xe0u, 0x0cu, 0x02u, 0x00u, 0x80u, 0x00u, 0x20u, 0x14u, 0xe0u,
    0x09u, 0x00u, 0x00u, 0x80u, 0x70u, 0x3fu, 0x8cu, 0xbfu, 0x00u, 0x20u, 0x14u, 0xe0u,
    0x0au, 0x06u, 0x00u, 0x80u, 0x00u, 0x20u, 0x14u, 0xe0u, 0x0bu, 0x04u, 0x00u, 0x80u,
    0x00u, 0x00u, 0x81u, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu,
    0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu,
    0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu,
    0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu,
    0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x9fu, 0xbfu, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x73u, 0x6cu, 0x30u, 0x30u,
    0x85u, 0x00u, 0x00u, 0x00u, 0xa0u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x57u, 0x25u, 0x10u, 0x00u, 0x00u, 0xa6u, 0x01u, 0x04u, 0x10u, 0x0au, 0x10u, 0x0bu,
    0x10u, 0x0cu, 0x10u, 0x40u, 0x40u, 0x42u, 0x20u, 0x4bu, 0x10u, 0x4cu, 0x10u, 0x4du,
    0x10u, 0x4eu, 0x10u, 0x6fu, 0x60u, 0x79u, 0x10u, 0x7au, 0x10u, 0x85u, 0x10u, 0x00u,
    0x00u, 0x6eu, 0x00u, 0x00u, 0x64u, 0x0fu, 0x20u, 0x21u, 0x10u, 0x24u, 0x40u, 0x25u,
    0x10u, 0x00u, 0x00u, 0x65u, 0x21u, 0x10u, 0x24u, 0x40u, 0x00u, 0x00u, 0x64u, 0x21u,
    0x10u, 0x24u, 0x40u, 0x00u, 0x00u, 0x61u, 0x29u, 0x10u, 0x00u, 0x00u, 0x60u, 0x28u,
    0x50u, 0x29u, 0x10u, 0x00u, 0x00u, 0x81u, 0xa0u, 0x1du, 0x00u, 0x01u, 0x20u, 0x80u,
    0xfdu, 0xacu, 0x17u, 0x21u, 0x00u, 0x01u, 0x56u, 0x00u, 0x51u, 0x6au, 0x00u, 0x4au,
    0x14u, 0x6au, 0x43u, 0x6fu, 0x04u, 0x6cu, 0x10u, 0x6cu, 0x72u, 0x24u, 0x38u, 0x0au,
    0x00u, 0x76u, 0x08u, 0x10u, 0x76u, 0x7au, 0x24u, 0x35u, 0x7fu, 0x15u, 0x83u, 0x01u,
    0x02u, 0x08u, 0x08u, 0x01u, 0x7fu, 0x01u, 0x0eu, 0xf8u, 0x55u, 0xe9u, 0x03u, 0x00u,
    0x5du, 0x0du, 0x80u, 0x80u, 0xe0u, 0x40u, 0x20u, 0xe1u, 0xb1u, 0xc2u, 0x87u, 0x05u,
    0x62u, 0x61u, 0x72u, 0x65u, 0x66u, 0x6fu, 0x6fu, 0x74u, 0xcdu, 0xe4u, 0xc9u, 0xcau,
    0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0xb4u, 0x03u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0xa0u, 0x00u, 0x00u, 0x00u, 0xc8u, 0x3bu, 0xc2u, 0x5eu,
    0xe1u, 0x98u, 0xf0u, 0x50u,
};

static obs_result check_agc_init(void) {
    if (!obs_address_is_callable((const void *)&sceAgcInit)) {
        return obs_skip("sceAgcInit not resolved");
    }

    uint64_t state_slot[8];
    for (size_t i = 0; i < 8; i++) {
        state_slot[i] = 0;
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during sceAgcInit", (uint64_t)sig);
    }

    int rc_init = sceAgcInit((void *)state_slot, 0xd);
    obs_fault_unregister();
    obs_report_measure("166-agc/init", "sceAgcInit", "rc-init",
                       (uint64_t)(uint32_t)rc_init, "code");
    obs_report_bytes("166-agc/init", "sceAgcInit", "state-out", 0,
                     (const unsigned char *)state_slot, sizeof(state_slot));

    if (obs_address_is_callable((const void *)&sceAgcGetIsTrinityMode)) {
        uint8_t is_trinity = 0xff;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_trinity = sceAgcGetIsTrinityMode(&is_trinity);
            obs_fault_unregister();
            obs_report_measure("166-agc/init", "sceAgcGetIsTrinityMode", "rc-trinity",
                               (uint64_t)(uint32_t)rc_trinity, "code");
            obs_report_measure("166-agc/init", "sceAgcGetIsTrinityMode", "is-trinity",
                               (uint64_t)is_trinity, "bool");
        } else {
            obs_fault_unregister();
        }
    }

    if (rc_init == 0) {
        return obs_pass();
    }
    return obs_partial_value("sceAgcInit returned non-zero",
                             (uint64_t)(uint32_t)rc_init);
}

/* sceAgcCreateShader: arity 4. Creates authentic shader object and audits structure
 * layout. */
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

    /* 1. Null argument call */
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc_null = fn_create(NULL, NULL, NULL, 0);
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-null",
                           rc_null, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-null",
                           (uint64_t)sig, "fault-sig");
    }

    /* 2. Authentic retail shader from AgcCompositor */
    uint8_t hdr_buf[512];
    for (size_t i = 0; i < sizeof(hdr_buf); i++) {
        hdr_buf[i] = 0xc7;
    }
    for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) {
        hdr_buf[i] = agc_retail_hdr_full_0[i];
    }

    void *shader_obj = (void *)(uintptr_t)0xdeadbeefbaadf00d;
    uint64_t rc_retail = 0xffffffff;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_retail = fn_create((void *)&shader_obj, (const void *)hdr_buf,
                              (const void *)agc_retail_payload_0, 0);
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-retail",
                           rc_retail, "code");
        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "obj-valid",
            (shader_obj != NULL && shader_obj != (void *)(uintptr_t)0xdeadbeefbaadf00d)
                ? 1u
                : 0u,
            "flag");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-retail",
                           (uint64_t)sig, "fault-sig");
    }

    if (rc_retail == 0 && shader_obj != NULL &&
        shader_obj != (void *)(uintptr_t)0xdeadbeefbaadf00d &&
        obs_address_is_callable(shader_obj)) {
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "arg0-addr",
                           (uint64_t)(uintptr_t)&shader_obj, "addr");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "arg1-addr",
                           (uint64_t)(uintptr_t)hdr_buf, "addr");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "arg2-addr",
                           (uint64_t)(uintptr_t)agc_retail_payload_0, "addr");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                           "shader-obj-ptr", (uint64_t)(uintptr_t)shader_obj, "addr");

        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "dist-from-arg0",
            (uint64_t)((intptr_t)shader_obj - (intptr_t)&shader_obj), "offset");
        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "dist-from-arg1",
            (uint64_t)((intptr_t)shader_obj - (intptr_t)hdr_buf), "offset");
        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "dist-from-arg2",
            (uint64_t)((intptr_t)shader_obj - (intptr_t)agc_retail_payload_0),
            "offset");

        uint32_t hdr_changed = 0;
        for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) {
            if (hdr_buf[i] != agc_retail_hdr_full_0[i]) {
                hdr_changed++;
            }
        }
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader",
                           "hdr-changed-bytes", (uint64_t)hdr_changed, "count");

        size_t highest_touched = 0;
        for (size_t i = sizeof(hdr_buf); i > 0; i--) {
            if (hdr_buf[i - 1] != 0xc7) {
                highest_touched = i;
                break;
            }
        }
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "hdr-extent",
                           (uint64_t)highest_touched, "bytes");

        obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader", "shader-obj", 0,
                         (const unsigned char *)shader_obj, 0x80u);

        uint64_t field_10 = *(const uint64_t *)((const char *)shader_obj + 0x10);
        uint64_t field_30 = *(const uint64_t *)((const char *)shader_obj + 0x30);
        uint32_t field_50 = *(const uint32_t *)((const char *)shader_obj + 0x50);
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "field-0x10",
                           field_10, "addr");
        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "f10-dist-from-arg2",
            (uint64_t)((intptr_t)field_10 - (intptr_t)agc_retail_payload_0), "offset");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "field-0x30",
                           field_30, "val");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "field-0x50",
                           (uint64_t)field_50, "val");
        return obs_pass();
    }

    if (rc_retail == 0) {
        return obs_pass();
    }
    return obs_partial_value("create shader returned non-zero code", rc_retail);
}

static obs_result check_agc_shader_differential(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("sceAgcCreateShader not callable");
    }

    /* Base template from retail header */
    uint8_t base_hdr[384];
    for (size_t i = 0; i < sizeof(base_hdr); i++) {
        base_hdr[i] = 0;
    }
    for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) {
        base_hdr[i] = agc_retail_hdr_full_0[i];
    }

    const void *payload = (const void *)agc_retail_payload_0;

    /* Perturbation testing matrix across format fields */
    struct {
        const char *name;
        size_t offset;
        size_t size;
        uint32_t val;
        int use_unaligned_payload;
    } cases[] = {
        {"baseline", 0, 0, 0, 0},
        {"magic-zero", 0x00, 4, 0x00000000u, 0},
        {"magic-deadbeef", 0x00, 4, 0xdeadbeefu, 0},
        {"version-zero", 0x04, 4, 0x00000000u, 0},
        {"version-17", 0x04, 4, 0x00000017u, 0},
        {"user-data-zero", 0x08, 8, 0x00000000u, 0},
        {"cx-reg-zero", 0x18, 8, 0x00000000u, 0},
        {"sh-reg-zero", 0x20, 8, 0x00000000u, 0},
        {"specials-zero", 0x28, 8, 0x00000000u, 0},
        {"hdr-size-zero", 0x40, 4, 0x00000000u, 0},
        {"hdr-size-60", 0x40, 4, 0x00000060u, 0},
        {"shader-size-zero", 0x44, 4, 0x00000000u, 0},
        {"target-zero", 0x4c, 4, 0x00000000u, 0},
        {"type-pixel", 0x5a, 1, 0x01u, 0},
        {"type-vertex", 0x5a, 1, 0x02u, 0},
        {"type-invalid", 0x5a, 1, 0x63u, 0},
        {"num-sh-zero", 0x5c, 1, 0x00u, 0},
        {"unaligned-code", 0, 0, 0, 1},
    };

    unsigned int passed_tests = 0;
    for (size_t c = 0; c < OBS_COUNT(cases); c++) {
        uint8_t cur_hdr[384];
        for (size_t i = 0; i < sizeof(cur_hdr); i++) {
            cur_hdr[i] = base_hdr[i];
        }

        if (cases[c].size == 1) {
            cur_hdr[cases[c].offset] = (uint8_t)cases[c].val;
        } else if (cases[c].size == 2) {
            *(uint16_t *)(cur_hdr + cases[c].offset) = (uint16_t)cases[c].val;
        } else if (cases[c].size == 4) {
            *(uint32_t *)(cur_hdr + cases[c].offset) = cases[c].val;
        } else if (cases[c].size == 8) {
            *(uint64_t *)(cur_hdr + cases[c].offset) = (uint64_t)cases[c].val;
        }

        const void *cur_payload = cases[c].use_unaligned_payload
                                      ? (const void *)((uintptr_t)payload + 4)
                                      : payload;
        void *out_obj = NULL;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint64_t rc = sceAgcCreateShader(&out_obj, cur_hdr, cur_payload, 0);
            obs_fault_unregister();
            obs_report_measure("166-agc/shader-differential", "sceAgcCreateShader",
                               cases[c].name, rc, "code");
            passed_tests++;
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/shader-differential", "sceAgcCreateShader",
                               cases[c].name, (uint64_t)sig, "fault-sig");
        }
    }

    if (passed_tests > 0) {
        return obs_pass_value((uint64_t)passed_tests);
    }
    return obs_fail("differential shader tests failed to run");
}

/* DCB Constructor audit: audit potential constructor candidates without fabricating
 * handles. */
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

/* Patch function exclusion guard: asserts that the 7 unsafe patch functions remain
 * uncalled. */
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
typedef struct {
    uint64_t gpu_addr;
    uint32_t size;
    uint8_t flags;
    uint8_t pad[3];
} obs_agc_dcb_desc;

static obs_result check_agc_driver_symbols(void) {
    int cq_ok = obs_address_is_callable((const void *)&sceAgcDriverCreateQueue);
    int dq_ok = obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue);
    int sd_ok = obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb);

    obs_report_symbol("libSceAgcDriver", "sceAgcDriverCreateQueue", cq_ok, OBS_CURRENT);
    obs_report_symbol("libSceAgcDriver", "sceAgcDriverDestroyQueue", dq_ok,
                      OBS_CURRENT);
    obs_report_symbol("libSceAgcDriver", "sceAgcDriverSubmitDcb", sd_ok, OBS_CURRENT);

    unsigned int found = (cq_ok ? 1u : 0u) + (dq_ok ? 1u : 0u) + (sd_ok ? 1u : 0u);
    if (found == 0) {
        return obs_fail("none of the libSceAgcDriver symbols resolved");
    }
    return obs_pass_value((uint64_t)found);
}

static obs_result check_agc_driver_create_queue(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue)) {
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

    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();

    obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    obs_report_measure("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                       "queue-valid", (queue != NULL) ? 1u : 0u, "flag");

    if (queue != NULL && obs_address_is_callable(queue)) {
        obs_report_bytes("166-agc/driver-create-queue", "sceAgcDriverCreateQueue",
                         "queue-header", 0, (const unsigned char *)queue, 32u);

        /* Measure whether active queue state changes sceAgcCreateShader response */
        if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
            uint8_t dummy_out[32];
            uint8_t hdr[32];
            for (size_t z = 0; z < sizeof(hdr); z++)
                hdr[z] = 0;
            hdr[0] = '1';
            hdr[1] = '2';
            hdr[2] = '3';
            hdr[3] = '4';
            hdr[4] = 0x18;
            *(uint32_t *)(hdr + 8) = 0xd8u;
            uint8_t dummy_payload[0x100];
            for (size_t z = 0; z < sizeof(dummy_payload); z++)
                dummy_payload[z] = 0;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                uint64_t rc_cs = sceAgcCreateShader(dummy_out, hdr, dummy_payload, 0);
                obs_fault_unregister();
                obs_report_measure("166-agc/driver-create-queue", "sceAgcCreateShader",
                                   "rc-shader-with-queue", rc_cs, "code");
            } else {
                obs_fault_unregister();
            }
        }
    }

    int rc_destroy = -1;
    if (queue != NULL &&
        obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_destroy = sceAgcDriverDestroyQueue(queue);
            obs_fault_unregister();
            obs_report_measure("166-agc/driver-create-queue",
                               "sceAgcDriverDestroyQueue", "rc-destroy",
                               (uint64_t)(uint32_t)rc_destroy, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/driver-create-queue",
                               "sceAgcDriverDestroyQueue", "rc-destroy", (uint64_t)sig,
                               "fault-sig");
        }
    }

    if (rc_create == 0 && queue != NULL) {
        return obs_pass();
    }
    return obs_partial_value("create queue returned non-zero code or null",
                             (uint64_t)(uint32_t)rc_create);
}

static obs_result check_agc_driver_queue_types(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue)) {
        return obs_skip("sceAgcDriverCreateQueue not resolved");
    }

    unsigned int created_count = 0;
    for (uint32_t qtype = 0; qtype <= 4; qtype++) {
        char qtype_str[16] = "type_";
        obs_format_u64(qtype_str + 5, qtype);

        void *queue = NULL;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        int rc = -1;
        if (sig == 0) {
            rc = sceAgcDriverCreateQueue(qtype, &queue, 0u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/driver-queue-types", qtype_str, "fault",
                               (uint64_t)sig, "sig");
            continue;
        }

        obs_report_measure("166-agc/driver-queue-types", qtype_str, "rc-create",
                           (uint64_t)(uint32_t)rc, "code");

        if (rc == 0 && queue != NULL) {
            created_count++;
            obs_report_measure("166-agc/driver-queue-types", qtype_str, "valid", 1,
                               "bool");

            /* Read 32-byte queue header */
            obs_report_bytes("166-agc/queue-header", qtype_str, "header", 0,
                             (const unsigned char *)queue, 32u);

            if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
                sig = OBS_FAULT_ARM(&guard);
                int rc_dest = -1;
                if (sig == 0) {
                    rc_dest = sceAgcDriverDestroyQueue(queue);
                    obs_fault_unregister();
                    obs_report_measure("166-agc/driver-queue-types", qtype_str,
                                       "rc-destroy", (uint64_t)(uint32_t)rc_dest,
                                       "code");
                } else {
                    obs_fault_unregister();
                }
            }
        } else {
            obs_report_measure("166-agc/driver-queue-types", qtype_str, "valid", 0,
                               "bool");
        }
    }

    if (created_count > 0) {
        return obs_pass_value((uint64_t)created_count);
    }
    return obs_fail("no queue types could be created");
}

static obs_result check_agc_driver_submit_nop(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("sceAgcDriverCreateQueue or SubmitDcb not resolved");
    }

    void *queue = NULL;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during driver queue setup");
    }

    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("queue creation failed; skipping submit");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    uint32_t *dw = (uint32_t *)probe->cmdbuf;
    for (int i = 0; i < 4; i++) {
        dw[i] = 0xffff1000u;
    }

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->cmdbuf;
    desc.size = 4u; /* 4 DWORDs */
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = sceAgcDriverSubmitDcb(&desc);
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-nop", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-nop", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sceAgcDriverDestroyQueue(queue);
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
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_driver_submit_batch(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("sceAgcDriverCreateQueue or SubmitDcb not resolved");
    }

    void *queue = NULL;
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during driver queue setup");
    }

    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("queue creation failed; skipping batch submit");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);

    /* Emit a multi-dword PM4 batch: 8 NOP packets */
    uint32_t *dw = (uint32_t *)probe->cur;
    for (int i = 0; i < 8; i++) {
        dw[i] = 0xffff1000u;
    }
    probe->cur += 8 * 4;
    for (int p = 0; p < 16; p++) {
        ((uint32_t *)probe->cur)[p] = 0xffff1000u;
    }

    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/driver-submit-batch", "sceAgcDriverSubmitDcb",
                       "bytes-written", (uint64_t)bytes_written, "size");

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u; /* PM4 size in DWORDs */
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = sceAgcDriverSubmitDcb(&desc);
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-batch", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-batch", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sceAgcDriverDestroyQueue(queue);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    if (sig != 0) {
        return obs_fail("fault during multi-dword submit");
    }
    if (submit_rc == 0) {
        return obs_pass();
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_driver_submit_fence(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before queue/fence creation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(64) uint32_t s_host_fence_single[16];
    volatile uint32_t *fence = s_host_fence_single;
#endif

    obs_fault_unregister();
    if (fence == NULL) {
        return obs_skip("failed to allocate Onion memory for fence");
    }
    *fence = 0x11111111u;

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("queue creation failed; skipping fence submit");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;

    /* Emit RELEASE_MEM: EOP event write to fence address */
    *dw++ = 0xc0064900u; /* DW0: PACKET3_RELEASE_MEM, count 6 */
    *dw++ = 0x06603514u; /* DW1: GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB |
                            CACHE_POLICY(3) | EVENT_TYPE(0x14) | EVENT_INDEX(5) */
    *dw++ = 0x20000000u; /* DW2: DATA_SEL(1) = write 32-bit int low */
    *dw++ = (uint32_t)fence_gpu;         /* DW3: address low */
    *dw++ = (uint32_t)(fence_gpu >> 32); /* DW4: address high */
    *dw++ = 0xbeefcafeu;                 /* DW5: fence value */
    *dw++ = 0u;                          /* DW6: high 32 bits of value */
    *dw++ = 0u;                          /* DW7: context_id / pad */

    /* Pad trailing area with PM4 NOPs to ensure prefetch safety */
    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "bytes-written", (uint64_t)bytes_written, "size");

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u; /* PM4 size in DWORDs (8 DWORDs = 32 bytes) */
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = sceAgcDriverSubmitDcb(&desc);
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    uint32_t fence_val = *fence;
    int fence_hit = 0;
    if (submit_rc == 0) {
        for (int iter = 0; iter < 10000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            fence_val = *fence;
            if (fence_val == 0xbeefcafeu) {
                fence_hit = 1;
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-val", (uint64_t)fence_val, "val");
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sceAgcDriverDestroyQueue(queue);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    if (sig != 0) {
        return obs_fail("fault during fence submit or poll");
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_pass();
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after submit", (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_compute_dispatch(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("AGC symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before queue/shader creation", (uint64_t)sig);
    }

    /* 1. Allocate GPU payload, fence buffer, and ALU output buffer in Onion memory */
#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *out_buf =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_payload[sizeof(agc_retail_payload_0)];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(64) uint32_t s_host_out[16];
    uint8_t *gpu_payload = s_host_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *out_buf = s_host_out;
#endif

    obs_fault_unregister();

    if (gpu_payload == NULL || fence == NULL || out_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for shader, fence or out_buf");
    }

    for (size_t i = 0; i < sizeof(agc_retail_payload_0); i++) {
        gpu_payload[i] = agc_retail_payload_0[i];
    }
    *fence = 0x11111111u;
    *out_buf = 0x55555555u;

    /* Custom RDNA2 compute bytecode at gpu_payload + 0x000:
     * 1. s_waitcnt vmcnt(0) expcnt(0) lgkmcnt(0)  [0xbf8c0000]
     * 2. s_mov_b32 s0, out_gpu_lo                 [0xbe8003ff, out_gpu_lo]
     * 3. s_mov_b32 s1, out_gpu_hi                 [0xbe8103ff, out_gpu_hi]
     * 4. v_mov_b32_e32 v0, s0                     [0x7e000200]
     * 5. v_mov_b32_e32 v1, s1                     [0x7e020201]
     * 6. v_mov_b32_e32 v2, 0x12345678             [0x7e0402ff, 0x12345678]
     * 7. v_add_nc_u32_e32 v2, 0x11111111, v2      [0x4a0404ff, 0x11111111] (0x12345678
     * + 0x11111111 = 0x23456789)
     * 8. global_store_dword v[0:1], v2, off       [0xdc708000, 0x007d0200]
     * 9. s_waitcnt vmcnt(0)                       [0xbf8c3f70]
     * 10. s_endpgm                                [0xbf810000]
     */
    uint64_t out_gpu = (uint64_t)(uintptr_t)out_buf;
    uint32_t out_gpu_lo = (uint32_t)out_gpu;
    uint32_t out_gpu_hi = (uint32_t)(out_gpu >> 32);

    uint32_t *code = (uint32_t *)gpu_payload;
    code[0] = 0xbf8c0000u;
    code[1] = 0xbe8003ffu;
    code[2] = out_gpu_lo;
    code[3] = 0xbe8103ffu;
    code[4] = out_gpu_hi;
    code[5] = 0x7e000200u;
    code[6] = 0x7e020201u;
    code[7] = 0x7e0402ffu;
    code[8] = 0x12345678u;
    code[9] = 0x4a0404ffu;
    code[10] = 0x11111111u;
    code[11] = 0xdc708000u;
    code[12] = 0x007d0200u;
    code[13] = 0xbf8c3f70u;
    code[14] = 0xbf810000u;
    for (size_t k = 15; k < 0x3b0u / 4u; k++) {
        code[k] = 0xbf9f0000u; /* s_nop 0 */
    }

    /* 2. Instantiate compute shader with GPU payload */
    uint8_t hdr_buf[384];
    for (size_t i = 0; i < sizeof(hdr_buf); i++) {
        hdr_buf[i] = 0;
    }
    for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) {
        hdr_buf[i] = agc_retail_hdr_full_0[i];
    }

    void *shader_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during shader instantiation");
    }
    uint64_t rc_shader =
        sceAgcCreateShader(&shader_obj, (void *)hdr_buf, gpu_payload, 0);
    obs_fault_unregister();

    obs_report_measure("166-agc/compute-dispatch", "sceAgcCreateShader", "rc-shader",
                       rc_shader, "code");
    if (rc_shader != 0 || shader_obj == NULL) {
        return obs_fail("failed to instantiate shader for dispatch");
    }

    /* 3. Create GPU queue */
    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("queue creation failed; skipping dispatch");
    }

    /* 4. Build DCB containing SET_SH_REG, DISPATCH_DIRECT, and RELEASE_MEM */
    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);

    uint32_t *dw = (uint32_t *)probe->cur;

    /* Extract resolved registers from hdr_buf (starts at hdr_buf + 0x88, 11 register
     * pairs) */
    const uint32_t *reg_table = (const uint32_t *)(hdr_buf + 0x88);
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;
    for (int i = 0; i < 11; i++) {
        uint32_t reg_idx = reg_table[i * 2];
        uint32_t reg_val = reg_table[i * 2 + 1];
        if (reg_idx == 0x20c) { /* COMPUTE_PGM_LO: bits 39:8 of shader VA */
            reg_val = (uint32_t)(payload_va >> 8);
        } else if (reg_idx == 0x20d) { /* COMPUTE_PGM_HI: bits 47:40 of shader VA */
            reg_val = (uint32_t)(payload_va >> 40);
        }
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = reg_idx;
        *dw++ = reg_val;
    }

    /* Also bind out_buf to COMPUTE_USER_DATA_0 and 1 */
    *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
    *dw++ = 0x240u;      /* COMPUTE_USER_DATA_0 */
    *dw++ = out_gpu_lo;
    *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
    *dw++ = 0x241u;      /* COMPUTE_USER_DATA_1 */
    *dw++ = out_gpu_hi;

    /* Emit DISPATCH_DIRECT: 1 threadgroup (8x8x1 = 64 threads) */
    *dw++ = 0xc0031500u; /* DISPATCH_DIRECT */
    *dw++ = 1u;          /* dim_x */
    *dw++ = 1u;          /* dim_y */
    *dw++ = 1u;          /* dim_z */
    *dw++ = 0x41u;       /* initiator */

    /* Emit RELEASE_MEM: EOP event write to fence address */
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    *dw++ = 0xc0064900u; /* DW0: PACKET3_RELEASE_MEM, count 6 */
    *dw++ = 0x06603514u; /* DW1: GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB |
                            CACHE_POLICY(3) | EVENT_TYPE(0x14) | EVENT_INDEX(5) */
    *dw++ = 0x20000000u; /* DW2: DATA_SEL(1) = write 32-bit int low */
    *dw++ = (uint32_t)fence_gpu;         /* DW3: address low */
    *dw++ = (uint32_t)(fence_gpu >> 32); /* DW4: address high */
    *dw++ = 0xbeefcafeu;                 /* DW5: fence value written on completion */
    *dw++ = 0u;                          /* DW6: high 32 bits of value */
    *dw++ = 0u;                          /* DW7: context_id / pad */

    /* Pad trailing area with PM4 NOPs to ensure prefetch safety */
    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb",
                       "bytes-written", (uint64_t)bytes_written, "size");

    /* 5. Submit DCB */
    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u; /* PM4 size in DWORDs */
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = sceAgcDriverSubmitDcb(&desc);
        obs_fault_unregister();
        obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    /* 6. Poll fence */
    uint32_t fence_val = *fence;
    int fence_hit = 0;
    if (submit_rc == 0) {
        for (int iter = 0; iter < 10000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            fence_val = *fence;
            if (fence_val == 0xbeefcafeu) {
                fence_hit = 1;
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb", "fence-val",
                       (uint64_t)fence_val, "val");
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb", "fence-hit",
                       (uint64_t)fence_hit, "bool");

    /* 7. Read back ALU output buffer */
#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)out_buf);
#endif
    uint32_t alu_val = *out_buf;
    int alu_hit = (alu_val == 0x23456789u);
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb", "alu-val",
                       (uint64_t)alu_val, "val");
    obs_report_measure("166-agc/compute-dispatch", "sceAgcDriverSubmitDcb", "alu-hit",
                       (uint64_t)alu_hit, "bool");

    /* 8. Destroy queue */
    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sceAgcDriverDestroyQueue(queue);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    if (sig != 0) {
        return obs_fail("fault during compute dispatch submit or poll");
    }
    if (submit_rc == 0 && fence_hit == 1 && alu_hit == 1) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("fence hit but ALU output mismatch",
                                 (uint64_t)alu_val);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after dispatch", (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_graphics_submit(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before queue/fence creation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(64) uint32_t s_host_gfx_fence[16];
    volatile uint32_t *fence = s_host_gfx_fence;
#endif

    obs_fault_unregister();
    if (fence == NULL) {
        return obs_skip("failed to allocate Onion memory for fence");
    }
    *fence = 0x11111111u;

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    /* Create Type 0 (Universal / Graphics) Queue */
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/graphics-submit", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping graphics submit");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;

    /* Emit SET_CONTEXT_REG: packet type 3, opcode 0x28, count 2 body DWORDs (count - 1
     * = 1) Sets CB_COLOR0_BASE (context register 0x200) to test graphics pipeline
     * register setup */
    *dw++ = 0xc0012800u;                /* DW0: PACKET3_SET_CONTEXT_REG, count 1 */
    *dw++ = 0x200u;                     /* DW1: reg offset 0x200 (CB_COLOR0_BASE) */
    *dw++ = (uint32_t)(fence_gpu >> 8); /* DW2: base address >> 8 */

    /* Emit RELEASE_MEM: EOP event write to fence address */
    *dw++ = 0xc0064900u; /* DW0: PACKET3_RELEASE_MEM, count 6 */
    *dw++ = 0x06603514u; /* DW1: GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB |
                            CACHE_POLICY(3) | EVENT_TYPE(0x14) | EVENT_INDEX(5) */
    *dw++ = 0x20000000u; /* DW2: DATA_SEL(1) = write 32-bit int low */
    *dw++ = (uint32_t)fence_gpu;         /* DW3: address low */
    *dw++ = (uint32_t)(fence_gpu >> 32); /* DW4: address high */
    *dw++ = 0xbeefcafeu;                 /* DW5: fence value */
    *dw++ = 0u;                          /* DW6: high 32 bits of value */
    *dw++ = 0u;                          /* DW7: context_id / pad */

    /* Pad trailing area with PM4 NOPs to ensure prefetch safety */
    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/graphics-submit", "sceAgcDriverSubmitDcb",
                       "bytes-written", (uint64_t)bytes_written, "size");

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u; /* PM4 size in DWORDs */
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
        obs_report_measure("166-agc/graphics-submit", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/graphics-submit", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)sig, "fault-sig");
    }

    uint32_t fence_val = *fence;
    int fence_hit = 0;
    if (submit_rc == 0) {
        for (int iter = 0; iter < 1000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            fence_val = *fence;
            if (fence_val == 0xbeefcafeu) {
                fence_hit = 1;
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }
    obs_report_measure("166-agc/graphics-submit", "sceAgcDriverSubmitDcb", "fence-val",
                       (uint64_t)fence_val, "val");
    obs_report_measure("166-agc/graphics-submit", "sceAgcDriverSubmitDcb", "fence-hit",
                       (uint64_t)fence_hit, "bool");

    if (fence_hit == 1 &&
        obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sceAgcDriverDestroyQueue(queue);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    if (sig != 0) {
        return obs_fail("fault during graphics submit or poll");
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_pass();
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after graphics submit",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
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
    {"166-agc/cb-unnamed-ef57", "libSceAgc", "$fYZQG4CU71c", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_unnamed_ef57, OBS_FROM_ASSUMED},
    {"166-agc/dcb-reset-queue", "libSceAgc", "sceAgcDcbResetQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_reset_queue, OBS_FROM_ASSUMED},
    {"166-agc/init", "libSceAgc", "sceAgcInit", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcInit, check_agc_init, OBS_FROM_ASSUMED},
    {"166-agc/create-shader", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreateShader, check_agc_create_shader,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-constructor-audit", "libSceAgc", "(census)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_constructor_audit, OBS_FROM_ASSUMED},
    {"166-agc/patch-exclusion-guard", "libSceAgc", "(guard)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_exclusion_guard, OBS_FROM_ASSUMED},
    {"166-agc/driver-symbols", "libSceAgcDriver", "(symbols)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_symbols, OBS_FROM_ASSUMED},
    {"166-agc/driver-create-queue", "libSceAgcDriver", "sceAgcDriverCreateQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverCreateQueue,
     check_agc_driver_create_queue, OBS_FROM_ASSUMED},
    {"166-agc/driver-queue-types", "libSceAgcDriver", "sceAgcDriverCreateQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverCreateQueue,
     check_agc_driver_queue_types, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-nop", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_driver_submit_nop, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-batch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_driver_submit_batch, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-fence", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_driver_submit_fence, OBS_FROM_ASSUMED},
    {"166-agc/compute-dispatch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_compute_dispatch, OBS_FROM_ASSUMED},
    {"166-agc/graphics-submit", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_graphics_submit, OBS_FROM_ASSUMED},
    {"166-agc/shader-differential", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreateShader, check_agc_shader_differential,
     OBS_FROM_ASSUMED},
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
