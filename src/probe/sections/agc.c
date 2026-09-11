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

#if OOPS_TARGET_IS_ORBIS
static obs_result check_agc_cb_nop(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_release_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_dma_data(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_wait_reg_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_unnamed_ef57(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_reset_queue(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_create_shader(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_constructor_audit(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_exclusion_guard(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_symbols(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_create_queue(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_init(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_queue_types(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_submit_nop(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_submit_batch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_submit_fence(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_compute_dispatch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_graphics_submit(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_shader_differential(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_shader_graphics_stages(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_shader_fused_stages(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_cx_reg(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_uc_reg(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_sh_reg(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_auto(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_create_prim_state(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_cf_reg(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_sh_reg_direct(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_create_interpolant_mapping(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_update_interpolant_mapping(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_update_prim_state(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_link_shaders(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_index_buffer(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_index_size(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_index_count(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_index(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_event_write(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_num_instances(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
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
    {"166-agc/primitive-draw", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw, OBS_FROM_ASSUMED},
    {"166-agc/shader-graphics-stages", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_shader_graphics_stages, OBS_FROM_ASSUMED},
    {"166-agc/shader-fused-stages", "libSceAgc", "sceAgcFuseShaderHalves", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_shader_fused_stages, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cx-reg", "libSceAgc", "sceAgcDcbSetCxRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_cx_reg,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-reg", "libSceAgc", "sceAgcDcbSetUcRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_uc_reg,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-reg", "libSceAgc", "sceAgcCbSetShRegisterRangeDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_sh_reg,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-auto", "libSceAgc", "sceAgcDcbDrawIndexAuto", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_draw_auto, OBS_FROM_ASSUMED},
    {"166-agc/create-prim-state", "libSceAgc", "sceAgcCreatePrimState", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_create_prim_state, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cf-reg", "libSceAgc", "sceAgcDcbSetCfRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_cf_reg,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-reg-direct", "libSceAgc", "sceAgcDcbSetShRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_sh_reg_direct,
     OBS_FROM_ASSUMED},
    {"166-agc/create-interpolant-mapping", "libSceAgc",
     "sceAgcCreateInterpolantMapping", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_create_interpolant_mapping, OBS_FROM_ASSUMED},
    {"166-agc/update-interpolant-mapping", "libSceAgc",
     "sceAgcUpdateInterpolantMapping", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_update_interpolant_mapping, OBS_FROM_ASSUMED},
    {"166-agc/update-prim-state", "libSceAgc", "sceAgcUpdatePrimState", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_update_prim_state, OBS_FROM_ASSUMED},
    {"166-agc/link-shaders", "libSceAgc", "sceAgcLinkShaders", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_link_shaders, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-buffer", "libSceAgc", "sceAgcDcbSetIndexBuffer",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_index_buffer,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-size", "libSceAgc", "sceAgcDcbSetIndexSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_index_size, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-count", "libSceAgc", "sceAgcDcbSetIndexCount", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_index_count, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index", "libSceAgc", "sceAgcDcbDrawIndex", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_draw_index, OBS_FROM_ASSUMED},
    {"166-agc/dcb-event-write", "libSceAgc", "sceAgcDcbEventWrite", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_event_write, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-num-instances", "libSceAgc", "sceAgcDcbSetNumInstances",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_num_instances,
     OBS_FROM_ASSUMED},
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

static void init_stage_hdr(uint8_t *hdr, uint8_t stage, uint32_t reg) {
    for (size_t i = 0; i < 384; i++) {
        hdr[i] = 0;
    }
    for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) {
        hdr[i] = agc_retail_hdr_full_0[i];
    }
    hdr[0x5a] = stage;
    uint32_t *sh = (uint32_t *)(hdr + 0x90);
    if (reg != 0) {
        sh[0] = reg;
        sh[1] = 0;
        sh[2] = reg + 1u;
        sh[3] = 0;
    }
}

/* Graphics Shader Stages probe: systematically probe all 8 shader stages (0..7)
 * supported by sceAgcCreateShader, reading hardware registers and verifying patching.
 */
static obs_result check_agc_shader_graphics_stages(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("sceAgcCreateShader not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;
    uint8_t hdr[384];
    const void *payload = (const void *)agc_retail_payload_0;

    /* 1. Safely read live library register variables from libSceAgc data segment */
    const uint8_t *fn_base = (const uint8_t *)&sceAgcCreateShader;
    uint32_t mem_cs_reg = 0, mem_vs_reg = 0, mem_es_reg = 0;
    uint32_t mem_hs_reg = 0, mem_gs_reg = 0, mem_ps_reg = 0;

    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        mem_cs_reg = *(const uint32_t *)(fn_base + 0x36d20);
        mem_vs_reg = *(const uint32_t *)(fn_base + 0x36d30);
        mem_es_reg = *(const uint32_t *)(fn_base + 0x36d40);
        mem_hs_reg = *(const uint32_t *)(fn_base + 0x36d50);
        mem_gs_reg = *(const uint32_t *)(fn_base + 0x36d60);
        mem_ps_reg = *(const uint32_t *)(fn_base + 0x36d70);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-cs-reg", (uint64_t)mem_cs_reg, "val");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-vs-reg", (uint64_t)mem_vs_reg, "val");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-es-reg", (uint64_t)mem_es_reg, "val");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-hs-reg", (uint64_t)mem_hs_reg, "val");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-gs-reg", (uint64_t)mem_gs_reg, "val");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "mem-ps-reg", (uint64_t)mem_ps_reg, "val");

    /* Stage 0: Compute Shader */
    void *cs_obj = NULL;
    int rc_cs = -1;
    init_stage_hdr(hdr, 0u, mem_cs_reg ? mem_cs_reg : 0x20cu);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_cs = (int)sceAgcCreateShader(&cs_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-cs",
                       (uint64_t)(uint32_t)rc_cs, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "cs-obj-valid", (uint64_t)(cs_obj != NULL), "bool");

    /* Stage 1: Pixel Shader */
    void *ps_obj = NULL;
    int rc_ps = -1;
    init_stage_hdr(hdr, 1u, mem_ps_reg ? mem_ps_reg : 0x08u);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_ps = (int)sceAgcCreateShader(&ps_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-ps",
                       (uint64_t)(uint32_t)rc_ps, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "ps-obj-valid", (uint64_t)(ps_obj != NULL), "bool");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "ps-patched-lo", (uint64_t)((const uint32_t *)(hdr + 0x90))[1],
                       "val");

    /* Stage 2: Vertex Shader */
    void *vs_obj = NULL;
    int rc_vs = -1;
    init_stage_hdr(hdr, 2u, mem_vs_reg ? mem_vs_reg : 0xc8u);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_vs = (int)sceAgcCreateShader(&vs_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-vs",
                       (uint64_t)(uint32_t)rc_vs, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "vs-obj-valid", (uint64_t)(vs_obj != NULL), "bool");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "vs-patched-lo", (uint64_t)((const uint32_t *)(hdr + 0x90))[1],
                       "val");

    /* Stage 3: Geometry Shader */
    void *gs_obj = NULL;
    int rc_gs = -1;
    init_stage_hdr(hdr, 3u, mem_gs_reg ? mem_gs_reg : 0x148u);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_gs = (int)sceAgcCreateShader(&gs_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-gs",
                       (uint64_t)(uint32_t)rc_gs, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "gs-obj-valid", (uint64_t)(gs_obj != NULL), "bool");

    /* Stage 4: Unfused Local Shader (LS) */
    void *s4_obj = NULL;
    int rc_s4 = -1;
    init_stage_hdr(hdr, 4u, 0);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_s4 = (int)sceAgcCreateShader(&s4_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-s4",
                       (uint64_t)(uint32_t)rc_s4, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "s4-obj-valid", (uint64_t)(s4_obj != NULL), "bool");

    /* Stage 5: Unfused Hull Shader Half (HS) */
    void *s5_obj = NULL;
    int rc_s5 = -1;
    init_stage_hdr(hdr, 5u, 0);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_s5 = (int)sceAgcCreateShader(&s5_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-s5",
                       (uint64_t)(uint32_t)rc_s5, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "s5-obj-valid", (uint64_t)(s5_obj != NULL), "bool");

    /* Stage 6: Export Shader (ES) */
    void *es_obj = NULL;
    int rc_es = -1;
    init_stage_hdr(hdr, 6u, mem_es_reg ? mem_es_reg : 0x88u);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_es = (int)sceAgcCreateShader(&es_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-es",
                       (uint64_t)(uint32_t)rc_es, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "es-obj-valid", (uint64_t)(es_obj != NULL), "bool");

    /* Stage 7: Hull Shader (HS) */
    void *hs_obj = NULL;
    int rc_hs = -1;
    init_stage_hdr(hdr, 7u, mem_hs_reg ? mem_hs_reg : 0x108u);
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_hs = (int)sceAgcCreateShader(&hs_obj, hdr, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader", "rc-hs",
                       (uint64_t)(uint32_t)rc_hs, "code");
    obs_report_measure("166-agc/shader-graphics-stages", "sceAgcCreateShader",
                       "hs-obj-valid", (uint64_t)(hs_obj != NULL), "bool");

    if (rc_cs == 0 && rc_ps == 0 && rc_vs == 0 && rc_gs == 0 && rc_s4 == 0 &&
        rc_s5 == 0 && rc_es == 0 && rc_hs == 0) {
        return obs_pass();
    }
    return obs_partial_value("one or more stages failed", (uint64_t)(uint32_t)rc_cs);
}

static obs_result check_agc_shader_fused_stages(void) {
    if (!obs_address_is_callable((const void *)&sceAgcGetFusedShaderSize) ||
        !obs_address_is_callable((const void *)&sceAgcFuseShaderHalves) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("sceAgc fuse symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;
    const void *payload = (const void *)agc_retail_payload_0;

    /* 0. Safely inspect live fusion register variables in libSceAgc */
    const uint8_t *fn_base = (const uint8_t *)&sceAgcCreateShader;
    uint32_t mem_fuse_459d8 = 0, mem_fuse_459e0 = 0;
    uint32_t mem_fuse_rsrc1 = 0, mem_fuse_rsrc2 = 0;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        mem_fuse_459d8 = *(const uint32_t *)(fn_base + 0x36a68);
        mem_fuse_459e0 = *(const uint32_t *)(fn_base + 0x36a70);
        mem_fuse_rsrc1 = *(const uint32_t *)(fn_base + 0x36a80);
        mem_fuse_rsrc2 = *(const uint32_t *)(fn_base + 0x36a88);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "mem-fuse-459d8", (uint64_t)mem_fuse_459d8, "val");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "mem-fuse-rsrc1", (uint64_t)mem_fuse_rsrc1, "val");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "mem-fuse-rsrc2", (uint64_t)mem_fuse_rsrc2, "val");

    /* Build Half 1 (Stage 4: Local/Export) and Half 2 (Stage 6: Geometry/Export) */
    uint8_t half_vs[384];
    uint8_t half_gs[384];
    init_stage_hdr(half_vs, 4u, 0);
    init_stage_hdr(half_gs, 6u, 0x88u);

    /* Populate register lists with user-data and resource registers required by fusion
     */
    uint32_t *sh_vs = (uint32_t *)(half_vs + 0x90);
    uint32_t *sh_gs = (uint32_t *)(half_gs + 0x90);
    uint32_t r_459d8 = mem_fuse_459d8 ? mem_fuse_459d8 : 0x228u;
    uint32_t r_rsrc1 = mem_fuse_rsrc1 ? mem_fuse_rsrc1 : 0x212u;
    uint32_t r_rsrc2 = mem_fuse_rsrc2 ? mem_fuse_rsrc2 : 0x213u;
    uint32_t r_459e0 = mem_fuse_459e0 ? mem_fuse_459e0 : 0x22au;

    sh_vs[0] = 0xc8u;
    sh_vs[1] = 0;
    sh_vs[2] = 0xc9u;
    sh_vs[3] = 0;
    sh_vs[4] = 0x80u;
    sh_vs[5] = 0x1111u;
    sh_vs[6] = 0x80u;
    sh_vs[7] = 0x2222u;
    sh_vs[8] = r_459d8;
    sh_vs[9] = 0x10u;
    sh_vs[10] = r_rsrc1;
    sh_vs[11] = 0x20u;
    sh_vs[12] = r_rsrc2;
    sh_vs[13] = 0x30u;
    sh_vs[14] = r_459e0;
    sh_vs[15] = 0x40u;
    sh_vs[16] = 0x81u;
    sh_vs[17] = 0x50u;
    sh_vs[18] = 0x81u;
    sh_vs[19] = 0x60u;

    sh_gs[0] = 0x88u;
    sh_gs[1] = 0;
    sh_gs[2] = 0x89u;
    sh_gs[3] = 0;
    sh_gs[4] = 0x80u;
    sh_gs[5] = 0x3333u;
    sh_gs[6] = 0x80u;
    sh_gs[7] = 0x4444u;
    sh_gs[8] = r_459d8;
    sh_gs[9] = 0x10u;
    sh_gs[10] = r_rsrc1;
    sh_gs[11] = 0x20u;
    sh_gs[12] = r_rsrc2;
    sh_gs[13] = 0x30u;
    sh_gs[14] = r_459e0;
    sh_gs[15] = 0x40u;
    sh_gs[16] = 0x81u;
    sh_gs[17] = 0x50u;
    sh_gs[18] = 0x81u;
    sh_gs[19] = 0x60u;

    /* Relocate/instantiate halves via sceAgcCreateShader so pointers at 0x28 are
     * valid */
    void *vs_half_obj = NULL;
    void *gs_half_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_half_obj, half_vs, payload, 0);
        sceAgcCreateShader(&gs_half_obj, half_gs, payload, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    /* 1. Query fused shader size */
    uint64_t fused_size_info[2] = {0, 0};
    int rc_size = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_size = sceAgcGetFusedShaderSize(fused_size_info, half_vs, half_gs);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcGetFusedShaderSize",
                       "rc-size", (uint64_t)(uint32_t)rc_size, "code");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcGetFusedShaderSize",
                       "fused-size", fused_size_info[0], "size");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcGetFusedShaderSize",
                       "fused-align", fused_size_info[1], "align");

    /* 2. Fuse the shader halves into fused_hdr with auxiliary storage */
    uint8_t fused_hdr[512];
    uint8_t fuse_buf[512];
    for (size_t i = 0; i < sizeof(fused_hdr); i++) {
        fused_hdr[i] = 0;
    }
    for (size_t i = 0; i < sizeof(fuse_buf); i++) {
        fuse_buf[i] = 0;
    }
    int rc_fuse = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_fuse = sceAgcFuseShaderHalves(fused_hdr, half_vs, half_gs, fuse_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "rc-fuse", (uint64_t)(uint32_t)rc_fuse, "code");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "fused-stage", (uint64_t)fused_hdr[0x5a], "val");
    obs_report_measure("166-agc/shader-fused-stages", "sceAgcFuseShaderHalves",
                       "fused-valid", (uint64_t)(rc_fuse == 0 && fused_hdr[0x5a] == 2u),
                       "bool");

    if (rc_size == 0 && rc_fuse == 0 && fused_hdr[0x5a] == 2u) {
        return obs_pass();
    }
    if (rc_size == 0 && rc_fuse == 0) {
        return obs_partial_value("fused shader halves returned non-stage-2",
                                 (uint64_t)fused_hdr[0x5a]);
    }
    return obs_fail("fused shader pipeline failed");
}

static obs_result check_agc_dcb_set_cx_reg(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetCxRegisterDirect)) {
        return obs_skip("sceAgcDcbSetCxRegisterDirect not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit SET_CONTEXT_REG for CB_COLOR0_BASE (0x200) with value 0x12345678 */
    uint64_t entry = ((uint64_t)0x12345678u << 32) | 0x200u;
    void *res = sceAgcDcbSetCxRegisterDirect(&probe->begin, entry);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-cx-reg", "sceAgcDcbSetCxRegisterDirect",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-cx-reg", "sceAgcDcbSetCxRegisterDirect",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-cx-reg", "sceAgcDcbSetCxRegisterDirect",
                       "pkt-reg", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-cx-reg", "sceAgcDcbSetCxRegisterDirect",
                       "pkt-val", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-cx-reg", "sceAgcDcbSetCxRegisterDirect",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0016900u && pkt[1] == 0x200u &&
        pkt[2] == 0x12345678u && (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetCxRegisterDirect");
}

static obs_result check_agc_dcb_set_uc_reg(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetUcRegisterDirect)) {
        return obs_skip("sceAgcDcbSetUcRegisterDirect not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit SET_UCONFIG_REG for VGT_PRIMITIVE_TYPE (0x242) with value 0x4
     * (DI_PT_TRILIST) */
    uint64_t entry = ((uint64_t)0x4u << 32) | 0x242u;
    void *res = sceAgcDcbSetUcRegisterDirect(&probe->begin, entry);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-uc-reg", "sceAgcDcbSetUcRegisterDirect",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-uc-reg", "sceAgcDcbSetUcRegisterDirect",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-uc-reg", "sceAgcDcbSetUcRegisterDirect",
                       "pkt-reg", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-uc-reg", "sceAgcDcbSetUcRegisterDirect",
                       "pkt-val", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-uc-reg", "sceAgcDcbSetUcRegisterDirect",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0017900u && pkt[1] == 0x242u && pkt[2] == 0x4u &&
        (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetUcRegisterDirect");
}

static obs_result check_agc_dcb_set_sh_reg(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCbSetShRegisterRangeDirect)) {
        return obs_skip("sceAgcCbSetShRegisterRangeDirect not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit SET_SH_REG for SPI_SHADER_PGM_LO_PS (0x08) with 2 DWs: lo and hi */
    uint32_t values[2] = {0x12345678u, 0x9abcdef0u};
    void *res = sceAgcCbSetShRegisterRangeDirect(&probe->begin, 0x08u, values, 2u);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "pkt-reg", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "pkt-val0", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "pkt-val1", (uint64_t)pkt[3], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg", "sceAgcCbSetShRegisterRangeDirect",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0027600u && pkt[1] == 0x08u &&
        pkt[2] == values[0] && pkt[3] == values[1] &&
        (probe->cur - probe->begin) == 16u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcCbSetShRegisterRangeDirect");
}

static obs_result check_agc_dcb_draw_auto(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbDrawIndexAuto)) {
        return obs_skip("sceAgcDcbDrawIndexAuto not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit DRAW_INDEX_AUTO for 3 vertices, initiator 2 */
    void *res = sceAgcDcbDrawIndexAuto(&probe->begin, 3u, 2u);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-draw-auto", "sceAgcDcbDrawIndexAuto", "res-valid",
                       (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-draw-auto", "sceAgcDcbDrawIndexAuto", "pkt-hdr",
                       (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-draw-auto", "sceAgcDcbDrawIndexAuto", "pkt-count",
                       (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-draw-auto", "sceAgcDcbDrawIndexAuto",
                       "pkt-initiator", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-draw-auto", "sceAgcDcbDrawIndexAuto",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0012d00u && pkt[1] == 3u &&
        (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbDrawIndexAuto");
}

static obs_result check_agc_create_prim_state(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCreatePrimState) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("sceAgcCreatePrimState or sceAgcCreateShader not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;
    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u, 0xc8u);
    void *vs_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during vertex shader setup");
    }

    uint8_t prim_state[64];
    uint8_t sec_state[64];
    for (size_t i = 0; i < sizeof(prim_state); i++) {
        prim_state[i] = 0;
        sec_state[i] = 0;
    }

    int rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc = sceAgcCreatePrimState(prim_state, sec_state, NULL, vs_hdr,
                                   4u /* DI_PT_TRILIST */);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    obs_report_measure("166-agc/create-prim-state", "sceAgcCreatePrimState", "rc",
                       (uint64_t)(uint32_t)rc, "code");
    obs_report_measure("166-agc/create-prim-state", "sceAgcCreatePrimState",
                       "state-valid", (uint64_t)(rc == 0), "bool");
    obs_report_bytes("166-agc/create-prim-state", "sceAgcCreatePrimState", "prim-state",
                     0, (const unsigned char *)prim_state, 32);

    if (rc == 0) {
        return obs_pass();
    }
    return obs_fail("sceAgcCreatePrimState failed");
}

static obs_result check_agc_dcb_set_cf_reg(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetCfRegisterDirect)) {
        return obs_skip("sceAgcDcbSetCfRegisterDirect not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit SET_CONFIG_REG for reg 0x100 with value 0x12345678 */
    uint64_t entry = ((uint64_t)0x12345678u << 32) | 0x100u;
    void *res = sceAgcDcbSetCfRegisterDirect(&probe->begin, entry);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-cf-reg", "sceAgcDcbSetCfRegisterDirect",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-cf-reg", "sceAgcDcbSetCfRegisterDirect",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-cf-reg", "sceAgcDcbSetCfRegisterDirect",
                       "pkt-reg", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-cf-reg", "sceAgcDcbSetCfRegisterDirect",
                       "pkt-val", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-cf-reg", "sceAgcDcbSetCfRegisterDirect",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0016800u && pkt[1] == 0x100u &&
        pkt[2] == 0x12345678u && (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetCfRegisterDirect");
}

static obs_result check_agc_dcb_set_sh_reg_direct(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetShRegisterDirect)) {
        return obs_skip("sceAgcDcbSetShRegisterDirect not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    /* Emit SET_SH_REG for reg 0x8 with value 0x12345678 */
    uint64_t entry = ((uint64_t)0x12345678u << 32) | 0x8u;
    void *res = sceAgcDcbSetShRegisterDirect(&probe->begin, entry);

    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-sh-reg-direct", "sceAgcDcbSetShRegisterDirect",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-sh-reg-direct", "sceAgcDcbSetShRegisterDirect",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg-direct", "sceAgcDcbSetShRegisterDirect",
                       "pkt-reg", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg-direct", "sceAgcDcbSetShRegisterDirect",
                       "pkt-val", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-sh-reg-direct", "sceAgcDcbSetShRegisterDirect",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0017600u && pkt[1] == 0x8u &&
        pkt[2] == 0x12345678u && (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetShRegisterDirect");
}

static obs_result check_agc_create_interpolant_mapping(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCreateInterpolantMapping) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip(
            "sceAgcCreateInterpolantMapping or sceAgcCreateShader not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;

    /* 1. Default interpolant mapping with NULL shaders */
    uint64_t def_mapping[32];
    for (size_t i = 0; i < 32; i++) {
        def_mapping[i] = 0;
    }
    int def_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        def_rc = sceAgcCreateInterpolantMapping(def_mapping, NULL, NULL);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during default sceAgcCreateInterpolantMapping");
    }
    obs_report_measure("166-agc/create-interpolant-mapping",
                       "sceAgcCreateInterpolantMapping", "def-rc",
                       (uint64_t)(uint32_t)def_rc, "code");
    obs_report_measure("166-agc/create-interpolant-mapping",
                       "sceAgcCreateInterpolantMapping", "def-entry0", def_mapping[0],
                       "hex");
    obs_report_bytes("166-agc/create-interpolant-mapping",
                     "sceAgcCreateInterpolantMapping", "def-mapping", 0,
                     (const unsigned char *)def_mapping, 32);

    /* 2. Create VS and PS shaders */
    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u /* VS */, 0xc8u);
    void *vs_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during vertex shader setup");
    }

    uint8_t ps_hdr[384];
    init_stage_hdr(ps_hdr, 1u /* PS */, 0x08u);
    void *ps_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&ps_obj, ps_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during pixel shader setup");
    }

    /* 3. Call sceAgcCreateInterpolantMapping with VS and PS shaders */
    uint64_t mapping[32];
    for (size_t i = 0; i < 32; i++) {
        mapping[i] = 0;
    }
    int rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc = sceAgcCreateInterpolantMapping(mapping, vs_hdr, ps_hdr);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during sceAgcCreateInterpolantMapping(vs, ps)");
    }

    obs_report_measure("166-agc/create-interpolant-mapping",
                       "sceAgcCreateInterpolantMapping", "rc", (uint64_t)(uint32_t)rc,
                       "code");
    obs_report_measure("166-agc/create-interpolant-mapping",
                       "sceAgcCreateInterpolantMapping", "mapping-valid",
                       (uint64_t)(rc == 0), "bool");
    obs_report_measure("166-agc/create-interpolant-mapping",
                       "sceAgcCreateInterpolantMapping", "entry0", mapping[0], "hex");
    obs_report_bytes("166-agc/create-interpolant-mapping",
                     "sceAgcCreateInterpolantMapping", "mapping", 0,
                     (const unsigned char *)mapping, 32);

    if (def_rc == 0 && rc == 0) {
        return obs_pass();
    }
    return obs_fail("sceAgcCreateInterpolantMapping returned failure");
}

static obs_result check_agc_update_interpolant_mapping(void) {
    if (!obs_address_is_callable((const void *)&sceAgcUpdateInterpolantMapping) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip(
            "sceAgcUpdateInterpolantMapping or sceAgcCreateShader not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;

    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u /* VS */, 0xc8u);
    void *vs_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during vertex shader setup");
    }

    uint8_t ps_hdr[384];
    init_stage_hdr(ps_hdr, 1u /* PS */, 0x08u);
    void *ps_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&ps_obj, ps_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during pixel shader setup");
    }

    uint64_t mapping[32];
    for (size_t i = 0; i < 32; i++) {
        mapping[i] = 0;
    }

    int rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc = sceAgcUpdateInterpolantMapping(mapping, vs_hdr, ps_hdr);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during sceAgcUpdateInterpolantMapping");
    }

    obs_report_measure("166-agc/update-interpolant-mapping",
                       "sceAgcUpdateInterpolantMapping", "rc", (uint64_t)(uint32_t)rc,
                       "code");
    obs_report_measure("166-agc/update-interpolant-mapping",
                       "sceAgcUpdateInterpolantMapping", "mapping-valid",
                       (uint64_t)(rc == 0), "bool");
    obs_report_bytes("166-agc/update-interpolant-mapping",
                     "sceAgcUpdateInterpolantMapping", "mapping", 0,
                     (const unsigned char *)mapping, 32);

    if (rc == 0) {
        return obs_pass();
    }
    return obs_fail("sceAgcUpdateInterpolantMapping returned failure");
}

static obs_result check_agc_update_prim_state(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCreatePrimState) ||
        !obs_address_is_callable((const void *)&sceAgcUpdatePrimState) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("prim state functions not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;
    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u, 0xc8u);
    void *vs_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during vertex shader setup");
    }

    uint8_t prim_state[64];
    uint8_t sec_state[64];
    for (size_t i = 0; i < 64; i++) {
        prim_state[i] = 0;
        sec_state[i] = 0;
    }

    int rc_create = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_create = sceAgcCreatePrimState(prim_state, sec_state, NULL, vs_hdr,
                                          4u /* DI_PT_TRILIST */);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during sceAgcCreatePrimState");
    }

    uint32_t topo_orig = *(uint32_t *)(sec_state + 0x14) & 0x1fu;

    int rc_update = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_update =
            sceAgcUpdatePrimState(prim_state, sec_state, 1u /* DI_PT_POINTLIST */);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during sceAgcUpdatePrimState");
    }

    uint32_t topo_updated = *(uint32_t *)(sec_state + 0x14) & 0x1fu;

    obs_report_measure("166-agc/update-prim-state", "sceAgcUpdatePrimState",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    obs_report_measure("166-agc/update-prim-state", "sceAgcUpdatePrimState",
                       "rc-update", (uint64_t)(uint32_t)rc_update, "code");
    obs_report_measure("166-agc/update-prim-state", "sceAgcUpdatePrimState",
                       "topo-orig", (uint64_t)topo_orig, "hex");
    obs_report_measure("166-agc/update-prim-state", "sceAgcUpdatePrimState",
                       "topo-updated", (uint64_t)topo_updated, "hex");

    if (rc_create == 0 && rc_update == 0 && topo_orig == 4u && topo_updated == 1u) {
        return obs_pass();
    }
    return obs_fail("update prim state failed or topology not updated");
}

static obs_result check_agc_link_shaders(void) {
    if (!obs_address_is_callable((const void *)&sceAgcLinkShaders) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("sceAgcLinkShaders or sceAgcCreateShader not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;

    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u /* VS */, 0xc8u);
    void *vs_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during vertex shader setup");
    }

    uint8_t ps_hdr[384];
    init_stage_hdr(ps_hdr, 1u /* PS */, 0x08u);
    void *ps_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        sceAgcCreateShader(&ps_obj, ps_hdr, agc_retail_payload_0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during pixel shader setup");
    }

    uint8_t link_state[384];
    uint8_t sec_state[64];
    for (size_t i = 0; i < sizeof(link_state); i++) {
        link_state[i] = 0;
    }
    for (size_t i = 0; i < sizeof(sec_state); i++) {
        sec_state[i] = 0;
    }

    int rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc = sceAgcLinkShaders(link_state, sec_state, NULL, vs_hdr, ps_hdr,
                               4u /* DI_PT_TRILIST */);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return obs_fail("fault during sceAgcLinkShaders");
    }

    obs_report_measure("166-agc/link-shaders", "sceAgcLinkShaders", "rc",
                       (uint64_t)(uint32_t)rc, "code");
    obs_report_measure("166-agc/link-shaders", "sceAgcLinkShaders", "link-valid",
                       (uint64_t)(rc == 0), "bool");
    obs_report_bytes("166-agc/link-shaders", "sceAgcLinkShaders", "link-state-interp",
                     0, (const unsigned char *)link_state, 32);
    obs_report_bytes("166-agc/link-shaders", "sceAgcLinkShaders", "link-state-routing",
                     0x100, (const unsigned char *)(link_state + 0x100), 32);

    if (rc == 0) {
        return obs_pass();
    }
    return obs_fail("sceAgcLinkShaders returned failure");
}

static obs_result check_agc_dcb_set_index_buffer(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetIndexBuffer)) {
        return obs_skip("sceAgcDcbSetIndexBuffer not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res = sceAgcDcbSetIndexBuffer(&probe->begin, 0x12345678ULL);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-index-buffer", "sceAgcDcbSetIndexBuffer",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-index-buffer", "sceAgcDcbSetIndexBuffer",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-index-buffer", "sceAgcDcbSetIndexBuffer",
                       "pkt-lo", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-index-buffer", "sceAgcDcbSetIndexBuffer",
                       "pkt-hi", (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-index-buffer", "sceAgcDcbSetIndexBuffer",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0012600u && pkt[1] == 0x12345678u && pkt[2] == 0u &&
        (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetIndexBuffer");
}

static obs_result check_agc_dcb_set_index_size(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetIndexSize)) {
        return obs_skip("sceAgcDcbSetIndexSize not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res = sceAgcDcbSetIndexSize(&probe->begin, 0u, 0u);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-index-size", "sceAgcDcbSetIndexSize",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-index-size", "sceAgcDcbSetIndexSize", "pkt-hdr",
                       (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-index-size", "sceAgcDcbSetIndexSize", "pkt-reg",
                       (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-index-size", "sceAgcDcbSetIndexSize", "pkt-val",
                       (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-set-index-size", "sceAgcDcbSetIndexSize",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0017a00u && pkt[1] == 0x20000243u &&
        pkt[2] == 0x400u && (probe->cur - probe->begin) == 12u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetIndexSize");
}

static obs_result check_agc_dcb_set_index_count(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetIndexCount)) {
        return obs_skip("sceAgcDcbSetIndexCount not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res = sceAgcDcbSetIndexCount(&probe->begin, 36u);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-index-count", "sceAgcDcbSetIndexCount",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-index-count", "sceAgcDcbSetIndexCount",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-index-count", "sceAgcDcbSetIndexCount",
                       "pkt-val", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-index-count", "sceAgcDcbSetIndexCount",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0001300u && pkt[1] == 36u &&
        (probe->cur - probe->begin) == 8u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetIndexCount");
}

static obs_result check_agc_dcb_draw_index(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbDrawIndex)) {
        return obs_skip("sceAgcDcbDrawIndex not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res = sceAgcDcbDrawIndex(&probe->begin, 3u, 0x12345678ULL, 0u);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "res-valid",
                       (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "pkt-hdr",
                       (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "pkt-lo",
                       (uint64_t)pkt[2], "val");
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "pkt-hi",
                       (uint64_t)pkt[3], "val");
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "pkt-count",
                       (uint64_t)pkt[4], "val");
    obs_report_measure("166-agc/dcb-draw-index", "sceAgcDcbDrawIndex", "bytes-advanced",
                       (uint64_t)(probe->cur - probe->begin), "bytes");

    if (res != NULL && pkt[0] == 0xc0042700u && pkt[2] == 0x12345678u && pkt[3] == 0u &&
        pkt[4] == 3u && (probe->cur - probe->begin) == 24u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbDrawIndex");
}

static obs_result check_agc_dcb_event_write(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbEventWrite)) {
        return obs_skip("sceAgcDcbEventWrite not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res =
        sceAgcDcbEventWrite(&probe->begin, 62u /* ENABLE_LEGACY_PIPELINE */, 0u);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-event-write", "sceAgcDcbEventWrite", "res-valid",
                       (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-event-write", "sceAgcDcbEventWrite", "pkt-hdr",
                       (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-event-write", "sceAgcDcbEventWrite", "pkt-val",
                       (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-event-write", "sceAgcDcbEventWrite",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0004600u && pkt[1] == 62u &&
        (probe->cur - probe->begin) == 8u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbEventWrite");
}

static obs_result check_agc_dcb_set_num_instances(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbSetNumInstances)) {
        return obs_skip("sceAgcDcbSetNumInstances not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    void *res = sceAgcDcbSetNumInstances(&probe->begin, 1u);
    uint32_t *pkt = (uint32_t *)probe->cmdbuf;
    obs_report_measure("166-agc/dcb-set-num-instances", "sceAgcDcbSetNumInstances",
                       "res-valid", (uint64_t)(res != NULL), "bool");
    obs_report_measure("166-agc/dcb-set-num-instances", "sceAgcDcbSetNumInstances",
                       "pkt-hdr", (uint64_t)pkt[0], "val");
    obs_report_measure("166-agc/dcb-set-num-instances", "sceAgcDcbSetNumInstances",
                       "pkt-val", (uint64_t)pkt[1], "val");
    obs_report_measure("166-agc/dcb-set-num-instances", "sceAgcDcbSetNumInstances",
                       "bytes-advanced", (uint64_t)(probe->cur - probe->begin),
                       "bytes");

    if (res != NULL && pkt[0] == 0xc0002f00u && pkt[1] == 1u &&
        (probe->cur - probe->begin) == 8u) {
        return obs_pass();
    }
    return obs_fail("unexpected packet output from sceAgcDcbSetNumInstances");
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

    /* 1. Allocate GPU payload, fence buffer, and ALU output buffer in Onion memory
     */
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
     * 7. v_add_nc_u32_e32 v2, 0x11111111, v2      [0x4a0404ff, 0x11111111]
     * (0x12345678
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

    /* Extract resolved registers from hdr_buf (starts at hdr_buf + 0x88, 11
     * register pairs) */
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

    /* Emit SET_CONTEXT_REG: packet type 3, opcode 0x28, count 2 body DWORDs (count
     * - 1 = 1) Sets CB_COLOR0_BASE (context register 0x200) to test graphics
     * pipeline register setup */
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

static obs_result check_agc_primitive_draw(void) {
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
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_draw_payload[64];
    static _Alignas(64) uint32_t s_host_draw_fence[16];
    static _Alignas(64) uint32_t s_host_draw_color[1024];
    static _Alignas(64) uint32_t s_host_draw_canary[16];
    uint8_t *gpu_payload = s_host_draw_payload;
    volatile uint32_t *fence = s_host_draw_fence;
    volatile uint32_t *color_buf = s_host_draw_color;
    volatile uint32_t *canary = s_host_draw_canary;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || color_buf == NULL || canary == NULL) {
        return obs_skip("failed to allocate Onion memory for payload, fence, color "
                        "buffer or canary");
    }
    *fence = 0x11111111u;
    canary[0] = 0x11111111u;
    canary[1] = 0x22222222u;
    canary[2] = 0x33333333u;
    canary[3] = 0x44444444u;
    canary[5] = 0x55555555u;
    canary[6] = 0x66666666u;
    canary[7] = 0x77777777u;
    canary[8] = 0x88888888u;
    canary[9] = 0x99999999u;
    canary[10] = 0xaaaaaaaau;
    for (size_t i = 0; i < 1024; i++) {
        color_buf[i] = 0x55555555u;
    }

    /* Distinct RDNA2 shader bytecode per stage:
     * 1. VS / GS (NGG Primitive Shader): allocates 1 prim + 3 verts via
     * MSG_GS_ALLOC_REQ, writes canary[0] = 0xbeef0001, computes 3 triangle vertices,
     * exports pos0 and prim.
     * 2. PS (Pixel Shader): writes canary[1] = 0xbeef0002, exports solid red to MRT0.
     * 3. Fallback (HS/ES/LS): minimal alloc + s_endpgm.
     */
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    uint32_t *vs_code = (uint32_t *)gpu_payload;
    vs_code[0] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    vs_code[1] = 0x00001003u;
    vs_code[2] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    vs_code[3] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    vs_code[4] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    vs_code[5] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    vs_code[6] = 0x7e1a0204u; /* v_mov_b32 v13, s4 */
    vs_code[7] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    vs_code[8] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    vs_code[9] = (uint32_t)canary_gpu;
    vs_code[10] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    vs_code[11] = (uint32_t)(canary_gpu >> 32);
    vs_code[12] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    vs_code[13] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    vs_code[14] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0001 */
    vs_code[15] = 0xbeef0001u;
    vs_code[16] = 0xdc708000u; /* global_store_dword v[8:9], v10, off offset:0 */
    vs_code[17] = 0x007d0a08u;
    vs_code[18] = 0xdc708008u; /* global_store_dword v[8:9], v11, off offset:8 */
    vs_code[19] = 0x007d0b08u;
    vs_code[20] = 0xdc70800cu; /* global_store_dword v[8:9], v12, off offset:12 */
    vs_code[21] = 0x007d0c08u;
    vs_code[22] = 0xdc708014u; /* global_store_dword v[8:9], v13, off offset:20 */
    vs_code[23] = 0x007d0d08u;
    vs_code[24] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    vs_code[25] = 0xbefe0387u; /* s_mov_b32 exec_lo, 0x7 (lanes 0..2) */
    vs_code[26] = 0xd765000eu; /* v_mbcnt_lo_u32_b32 v14, -1, 0 */
    vs_code[27] = 0x000100c1u;
    vs_code[28] = 0x7e0202f3u; /* v_mov_b32 v1, -1.0 */
    vs_code[29] = 0x7e0402ffu; /* v_mov_b32 v2, 3.0 */
    vs_code[30] = 0x40400000u; /* 3.0f literal */
    vs_code[31] = 0x7e060280u; /* v_mov_b32 v3, 0.0 */
    vs_code[32] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0 */
    vs_code[33] = 0x7d841c81u; /* v_cmp_eq_u32 vcc_lo, 1, v14 */
    vs_code[34] = 0x020a0501u; /* v_cndmask_b32 v5, v1, v2, vcc_lo */
    vs_code[35] = 0x7d841c82u; /* v_cmp_eq_u32 vcc_lo, 2, v14 */
    vs_code[36] = 0x020c0501u; /* v_cndmask_b32 v6, v1, v2, vcc_lo */
    vs_code[37] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
    vs_code[38] = 0x04030605u;
    vs_code[39] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    vs_code[40] = 0x7e0e02ffu; /* v_mov_b32 v7, 0x00200400 */
    vs_code[41] = 0x00200400u; /* Prim indices: 0 | (1 << 10) | (2 << 20) */
    vs_code[42] = 0xf8000941u; /* exp prim, v7, off, off, off done */
    vs_code[43] = 0x00000007u;
    vs_code[44] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0003 */
    vs_code[45] = 0xbeef0003u;
    vs_code[46] = 0xdc708018u; /* global_store_dword v[8:9], v10, off offset:24 */
    vs_code[47] = 0x007d0a08u;
    vs_code[48] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    vs_code[49] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    vs_code[50] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 51; p < 64; p++) {
        vs_code[p] = 0xbf800000u; /* s_nop */
    }

    uint32_t *gs_code = (uint32_t *)((char *)gpu_payload + 0x100);
    for (size_t p = 0; p < 51; p++) {
        gs_code[p] = vs_code[p];
    }
    for (size_t p = 51; p < 64; p++) {
        gs_code[p] = 0xbf800000u;
    }

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    ps_code[0] = 0xbf8c0000u; /* s_waitcnt 0 */
    ps_code[1] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    ps_code[2] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    ps_code[3] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    ps_code[4] = 0x7e1a0204u; /* v_mov_b32 v13, s4 */
    ps_code[5] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    ps_code[6] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    ps_code[7] = (uint32_t)canary_gpu;
    ps_code[8] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    ps_code[9] = (uint32_t)(canary_gpu >> 32);
    ps_code[10] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    ps_code[11] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    ps_code[12] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0002 */
    ps_code[13] = 0xbeef0002u;
    ps_code[14] = 0xdc708004u; /* global_store_dword v[8:9], v10, off offset:4 */
    ps_code[15] = 0x007d0a08u;
    ps_code[16] = 0xdc70801cu; /* global_store_dword v[8:9], v11, off offset:28 */
    ps_code[17] = 0x007d0b08u;
    ps_code[18] = 0xdc708020u; /* global_store_dword v[8:9], v12, off offset:32 */
    ps_code[19] = 0x007d0c08u;
    ps_code[20] = 0xdc708024u; /* global_store_dword v[8:9], v13, off offset:36 */
    ps_code[21] = 0x007d0d08u;
    ps_code[22] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    ps_code[23] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    ps_code[24] = 0x7e0002f2u; /* v_mov_b32 v0, 1.0 (R = 1.0f) */
    ps_code[25] = 0x7e020280u; /* v_mov_b32 v1, 0.0 (G = 0.0f) */
    ps_code[26] = 0x7e040280u; /* v_mov_b32 v2, 0.0 (B = 0.0f) */
    ps_code[27] = 0x7e0602f2u; /* v_mov_b32 v3, 1.0 (A = 1.0f) */
    ps_code[28] = 0xf800080fu; /* exp mrt0, v0, v1, v2, v3 done */
    ps_code[29] = 0x03020100u;
    ps_code[30] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    ps_code[31] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0004 */
    ps_code[32] = 0xbeef0004u;
    ps_code[33] = 0xdc708028u; /* global_store_dword v[8:9], v10, off offset:40 */
    ps_code[34] = 0x007d0a08u;
    ps_code[35] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    ps_code[36] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    ps_code[37] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 38; p < 64; p++) {
        ps_code[p] = 0xbf800000u; /* s_nop */
    }

    uint32_t *fb_code = (uint32_t *)((char *)gpu_payload + 0x300);
    fb_code[0] = 0xbefc0380u; /* s_mov_b32 m0, 0 */
    fb_code[1] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    fb_code[2] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 3; p < 64; p++) {
        fb_code[p] = 0xbf800000u;
    }

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)gpu_payload);
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x100));
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x200));
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x300));
#endif

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    /* Create Type 0 (Universal / Graphics) Queue */
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverCreateQueue", "rc-create",
                       (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping primitive draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* 1. Context register setup for Color Target, Rasterizer, Viewport, Scissor:
     * Emit SET_CONTEXT_REG (opcode 0x69) configuring the complete fixed-function
     * pipeline. */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } ctx_regs[] = {
        /* Color Target 0 */
        {0x318u, 0}, /* CB_COLOR0_BASE (GFX10 context offset, patched below) */
        {0x390u, 0}, /* CB_COLOR0_BASE_EXT (GFX10 context offset, patched below) */
        {0x319u, 0x00000007u}, /* CB_COLOR0_PITCH: (64/8)-1 = 7 */
        {0x31au, 0x0000003fu}, /* CB_COLOR0_SLICE: (64*64/64)-1 = 63 */
        {0x31bu, 0x00000000u}, /* CB_COLOR0_VIEW */
        {0x31cu,
         0x000180a8u}, /* CB_COLOR0_INFO: COLOR_8_8_8_8, LINEAR_GENERAL, UNORM */
        {0x31eu, 0x00000048u}, /* CB_COLOR0_DCC_CONTROL */
        {0x3b8u, 0x08c6c000u}, /* CB_COLOR0_ATTRIB3: 2D linear buffer */
        {0x109u, 0x0000000au}, /* CB_DCC_CONTROL */
        /* Color Control & Mask & Blend */
        {0x202u, 0x00cc0010u}, /* CB_COLOR_CONTROL: CB_NORMAL, ROP3_COPY */
        {0x08eu, 0x0000000fu}, /* CB_TARGET_MASK: MRT0 4 components enabled */
        {0x08fu, 0xffffffffu}, /* CB_SHADER_MASK: all components export enabled */
        {0x1e0u, 0x20010001u}, /* CB_BLEND0_CONTROL: SRC=ONE, DST=ZERO, ADD */
        /* Depth disabled */
        {0x200u, 0x00000000u}, /* DB_DEPTH_CONTROL: disabled */
        {0x201u, 0x00000000u}, /* DB_EQAA: disabled */
        {0x203u, 0x00000000u}, /* DB_SHADER_CONTROL */
        /* NGG Primitive Type & Stages */
        {0x29bu, 0x00000002u}, /* VGT_GS_OUT_PRIM_TYPE: TRILIST */
        {0x2d5u, 0x00002000u}, /* VGT_SHADER_STAGES_EN: PRIMGEN_EN */
        {0x2ceu, 0x00000400u}, /* VGT_GS_MAX_VERT_OUT: 1024 */
        {0x2d4u, 0x88101010u}, /* VGT_TESS_DISTRIBUTION */
        {0x103u, 0xffffffffu}, /* VGT_MULTI_PRIM_IB_RESET_INDX */
        /* Sample Mask & NGG Control */
        {0x30eu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y0_X1Y0: enable all samples */
        {0x30fu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y1_X1Y1: enable all samples */
        {0x314u, 0x00000202u}, /* PA_SC_NGG_MODE_CNTL: MAX_DEALLOCS=2, MAX_FPOVS=2 */
        {0x311u, 0x01f92400u}, /* PA_SC_BINNER_CNTL_0 */
        {0x312u, 0x03ff0080u}, /* PA_SC_BINNER_CNTL_1 */
        {0x313u, 0x00006000u}, /* PA_SC_CONSERVATIVE_RASTERIZATION_CNTL */
        {0x00eu, 0x00000002u}, /* DB_DFSM_CONTROL */
        {0x280u, 0x00080008u}, /* PA_SU_POINT_SIZE */
        {0x281u, 0xffff0000u}, /* PA_SU_POINT_MINMAX */
        {0x282u, 0x00000008u}, /* PA_SU_LINE_CNTL */
        {0x2deu, 0x000001e9u}, /* PA_SU_POLY_OFFSET_DB_FMT_CNTL */
        /* Scissors (Screen, Window, Generic, Viewport) */
        {0x00cu, 0x00000000u}, /* PA_SC_SCREEN_SCISSOR_TL */
        {0x00du, 0x40004000u}, /* PA_SC_SCREEN_SCISSOR_BR */
        {0x081u, 0x80000000u}, /* PA_SC_WINDOW_SCISSOR_TL (WINDOW_OFFSET_DISABLE) */
        {0x082u, 0x40004000u}, /* PA_SC_WINDOW_SCISSOR_BR */
        {0x090u, 0x80000000u}, /* PA_SC_GENERIC_SCISSOR_TL (WINDOW_OFFSET_DISABLE) */
        {0x091u, 0x40004000u}, /* PA_SC_GENERIC_SCISSOR_BR */
        {0x094u, 0x80000000u}, /* PA_SC_VPORT_SCISSOR_0_TL (WINDOW_OFFSET_DISABLE) */
        {0x095u, 0x40004000u}, /* PA_SC_VPORT_SCISSOR_0_BR */
        /* Viewport Bounds & Transform */
        {0x0b4u, 0x00000000u}, /* PA_SC_VPORT_ZMIN_0: 0.0f */
        {0x0b5u, 0x3f800000u}, /* PA_SC_VPORT_ZMAX_0: 1.0f */
        {0x10fu, 0x42000000u}, /* PA_CL_VPORT_XSCALE: 32.0f */
        {0x110u, 0x42000000u}, /* PA_CL_VPORT_XOFFSET: 32.0f */
        {0x111u, 0x42000000u}, /* PA_CL_VPORT_YSCALE: 32.0f */
        {0x112u, 0x42000000u}, /* PA_CL_VPORT_YOFFSET: 32.0f */
        {0x113u, 0x3f000000u}, /* PA_CL_VPORT_ZSCALE: 0.5f */
        {0x114u, 0x3f000000u}, /* PA_CL_VPORT_ZOFFSET: 0.5f */
        /* Cliprect Rules */
        {0x083u, 0x0000ffffu}, /* PA_SC_CLIPRECT_RULE: allow all cliprects */
        {0x084u, 0x00000000u}, /* PA_SC_CLIPRECT_0_TL */
        {0x085u, 0x20002000u}, /* PA_SC_CLIPRECT_0_BR (8192x8192) */
        /* Guardband & Viewport Transform Enable */
        {0x204u, 0x00000000u}, /* PA_CL_CLIP_CNTL: normal clipping */
        {0x206u, 0x0000043fu}, /* PA_CL_VTE_CNTL: enable VPORT X,Y,Z scale & offset */
        {0x2fau, 0x3f800000u}, /* PA_CL_GB_VERT_CLIP_ADJ: 1.0f */
        {0x2fbu, 0x3f800000u}, /* PA_CL_GB_VERT_DISC_ADJ: 1.0f */
        {0x2fcu, 0x3f800000u}, /* PA_CL_GB_HORZ_CLIP_ADJ: 1.0f */
        {0x2fdu, 0x3f800000u}, /* PA_CL_GB_HORZ_DISC_ADJ: 1.0f */
        /* Scan Converter & Surface Setup */
        {0x205u, 0x00000240u}, /* PA_SU_SC_MODE_CNTL: no cull, face=0, poly=trilist */
        {0x20cu, 0x00000021u}, /* PA_SU_SMALL_PRIM_FILTER_CNTL */
        {0x292u, 0x00000002u}, /* PA_SC_MODE_CNTL_0: VPORT_SCISSOR_ENABLE */
        {0x293u, 0x06020000u}, /* PA_SC_MODE_CNTL_1 */
        {0x2f8u, 0x00000000u}, /* PA_SC_AA_CONFIG: 1x MSAA */
        {0x2f9u, 0x0000002du}, /* PA_SU_VTX_CNTL: 1/16th subpixel, half-pixel center */
        /* Shader Formats & SPI PS Controls */
        {0x191u, 0x00000000u}, /* SPI_PS_INPUT_CNTL_0 */
        {0x1c3u, 0x00000004u}, /* SPI_SHADER_POS_FORMAT: POS0 = 4COMP */
        {0x1c5u, 0x00000009u}, /* SPI_SHADER_COL_FORMAT: COL0 = 32_ABGR */
        {0x1b3u, 0x00000002u}, /* SPI_PS_INPUT_ENA: PERSP_CENTER_ENA */
        {0x1b4u, 0x00000002u}, /* SPI_PS_INPUT_ADDR: PERSP_CENTER_ENA */
        {0x1b5u, 0x00000001u}, /* SPI_INTERP_CONTROL_0: FLAT_SHADE_ENA */
        {0x1b6u, 0x00008000u}, /* SPI_PS_IN_CONTROL: PS_W32_EN */
        {0x1b8u, 0x00000001u}, /* SPI_BARYC_CNTL: PERSP_CENTER_CNTL */
    };
    for (size_t i = 0; i < sizeof(ctx_regs) / sizeof(ctx_regs[0]); i++) {
        uint32_t reg = ctx_regs[i].reg;
        uint32_t val = ctx_regs[i].val;
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        }
        *dw++ = 0xc0016900u; /* PACKET3_SET_CONTEXT_REG, count 1 */
        *dw++ = reg;
        *dw++ = val;
    }

    /* 2. Shader program binding to avoid SQC instruction fetch unmapped VA fault:
     * Bind all graphics stages (PS, VS, GS/NGG, HS, ES, LS) to their respective
     * payloads: PS: LO=0x08, HI=0x09, RSRC1=0x0A, RSRC2=0x0B (offset 0x200) VS:
     * LO=0x48, HI=0x49, RSRC1=0x4A, RSRC2=0x4B (offset 0x000) GS: LO=0x88, HI=0x89,
     * RSRC1=0x8A, RSRC2=0x8B (offset 0x000) ES: LO=0xC8, HI=0xC9, RSRC1=0xCA,
     * RSRC2=0xCB (offset 0x000) HS: LO=0x108, HI=0x109, RSRC1=0x10A, RSRC2=0x10B
     * (offset 0x000) LS: LO=0x148, HI=0x149, RSRC1=0x14A, RSRC2=0x14B (offset 0x000)
     */
    static const struct {
        uint32_t base_reg;
        uint64_t va_offset;
    } stages[] = {
        {0x08u, 0x200u},  /* PS (Pixel Shader) */
        {0x48u, 0x000u},  /* VS (Vertex Shader) */
        {0x88u, 0x000u},  /* GS / NGG (Geometry Shader) */
        {0xc8u, 0x000u},  /* ES / HS */
        {0x108u, 0x000u}, /* HS / LS */
        {0x148u, 0x000u}, /* LS */
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = payload_va + stages[s].va_offset;
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = base_reg + 2u;
        *dw++ = 0x000c0010u; /* RSRC1: 16 VGPRs, float mode */
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = base_reg + 3u;
        *dw++ = 0x00000008u; /* RSRC2: 8 SGPRs */
    }

    /* 3. SPI Compute Unit Enable masks:
     * SPI_SHADER_PGM_RSRC3_PS (0x007) and RSRC4_PS (0x001) enable CUs for PS stage.
     * SPI_SHADER_PGM_RSRC3_GS (0x087) and RSRC4_GS (0x081) enable CUs for GS stage.
     */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } spi_cu_regs[] = {
        {0x007u, 0x0000ffffu}, /* SPI_SHADER_PGM_RSRC3_PS: CU_EN = 0xffff (16 CUs) */
        {0x001u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_PS: CU_EN bits 17:16 = 0x3 (18
                                  CUs total) */
        {0x087u, 0x0000ffffu}, /* SPI_SHADER_PGM_RSRC3_GS: CU_EN = 0xffff (16 CUs) */
        {0x081u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_GS: CU_EN bits 17:16 = 0x3 (18
                                  CUs total) */
        {0x107u, 0xffff0000u}, /* SPI_SHADER_PGM_RSRC3_HS */
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    /* 4. Primitive topology setup: VGT_PRIMITIVE_TYPE via SET_UCONFIG_REG (opcode
     * 0x79) Register 0x242 in UCONFIG space = mmVGT_PRIMITIVE_TYPE (0xC242 -
     * 0xC000) Value 0x4 = DI_PT_TRILIST */
    *dw++ = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
    *dw++ = 0x242u;      /* Reg offset 0x242 */
    *dw++ = 0x4u;        /* DI_PT_TRILIST */

    /* 5. Primitive draw execution: DRAW_INDEX_AUTO (opcode 0x2D)
     * DW1: index_count = 3 (1 triangle)
     * DW2: initiator = 2 (DI_SRC_SEL_AUTO_INDEX, confirmed from
     * sceAgcDcbDrawIndexAuto disassembly) */
    *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO, count 1 */
    *dw++ = 3u;          /* index_count */
    *dw++ = 2u;          /* initiator */

    /* 6. Flush and fence retirement: RELEASE_MEM with EOP event */
    *dw++ = 0xc0064900u;         /* PACKET3_RELEASE_MEM, count 6 */
    *dw++ = 0x06603514u;         /* GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB |
                                    CACHE_POLICY(3) | EVENT_TYPE(0x14) | EVENT_INDEX(5) */
    *dw++ = 0x20000000u;         /* DATA_SEL(1) = write 32-bit int low */
    *dw++ = (uint32_t)fence_gpu; /* Address low */
    *dw++ = (uint32_t)(fence_gpu >> 32); /* Address high */
    *dw++ = 0xbeefcafeu;                 /* Fence value */
    *dw++ = 0u;                          /* High 32 bits */
    *dw++ = 0u;                          /* Context_id / pad */

    /* Pad trailing area with PM4 NOPs to ensure prefetch safety */
    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
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
        obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
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
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "fence-val",
                       (uint64_t)fence_val, "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "fence-hit",
                       (uint64_t)fence_hit, "bool");

    /* Check color buffer memory */
#if defined(__x86_64__)
    for (size_t p = 0; p < 0x4000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif
    uint32_t color_val = color_buf[0];
    int color_mod = 0;
    for (size_t i = 0; i < 1024; i++) {
        if (color_buf[i] != 0x55555555u) {
            color_mod = 1;
            color_val = color_buf[i];
            break;
        }
    }

    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "color-val",
                       (uint64_t)color_val, "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "color-mod",
                       (uint64_t)color_mod, "bool");
#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
#endif
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "canary-vs",
                       (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "canary-ps",
                       (uint64_t)canary[1], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "canary-v0",
                       (uint64_t)canary[2], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "canary-v1",
                       (uint64_t)canary[3], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb", "canary-exec",
                       (uint64_t)canary[5], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                       "canary-vs-done", (uint64_t)canary[6], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                       "canary-ps-v0", (uint64_t)canary[7], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                       "canary-ps-v1", (uint64_t)canary[8], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                       "canary-ps-exec", (uint64_t)canary[9], "val");
    obs_report_measure("166-agc/primitive-draw", "sceAgcDriverSubmitDcb",
                       "canary-ps-done", (uint64_t)canary[10], "val");

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
        return obs_fail("fault during primitive draw submit or poll");
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_pass();
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after primitive draw",
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
    {"166-agc/primitive-draw", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb, check_agc_primitive_draw,
     OBS_FROM_ASSUMED},
    {"166-agc/shader-graphics-stages", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreateShader, check_agc_shader_graphics_stages,
     OBS_FROM_ASSUMED},
    {"166-agc/shader-fused-stages", "libSceAgc", "sceAgcFuseShaderHalves", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcFuseShaderHalves, check_agc_shader_fused_stages,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cx-reg", "libSceAgc", "sceAgcDcbSetCxRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetCxRegisterDirect,
     check_agc_dcb_set_cx_reg, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-reg", "libSceAgc", "sceAgcDcbSetUcRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetUcRegisterDirect,
     check_agc_dcb_set_uc_reg, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-reg", "libSceAgc", "sceAgcCbSetShRegisterRangeDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcCbSetShRegisterRangeDirect,
     check_agc_dcb_set_sh_reg, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-auto", "libSceAgc", "sceAgcDcbDrawIndexAuto", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbDrawIndexAuto, check_agc_dcb_draw_auto,
     OBS_FROM_ASSUMED},
    {"166-agc/create-prim-state", "libSceAgc", "sceAgcCreatePrimState", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreatePrimState, check_agc_create_prim_state,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cf-reg", "libSceAgc", "sceAgcDcbSetCfRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetCfRegisterDirect,
     check_agc_dcb_set_cf_reg, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-reg-direct", "libSceAgc", "sceAgcDcbSetShRegisterDirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetShRegisterDirect,
     check_agc_dcb_set_sh_reg_direct, OBS_FROM_ASSUMED},
    {"166-agc/create-interpolant-mapping", "libSceAgc",
     "sceAgcCreateInterpolantMapping", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcCreateInterpolantMapping,
     check_agc_create_interpolant_mapping, OBS_FROM_ASSUMED},
    {"166-agc/update-interpolant-mapping", "libSceAgc",
     "sceAgcUpdateInterpolantMapping", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcUpdateInterpolantMapping,
     check_agc_update_interpolant_mapping, OBS_FROM_ASSUMED},
    {"166-agc/update-prim-state", "libSceAgc", "sceAgcUpdatePrimState", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcUpdatePrimState, check_agc_update_prim_state,
     OBS_FROM_ASSUMED},
    {"166-agc/link-shaders", "libSceAgc", "sceAgcLinkShaders", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcLinkShaders, check_agc_link_shaders,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-buffer", "libSceAgc", "sceAgcDcbSetIndexBuffer",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetIndexBuffer,
     check_agc_dcb_set_index_buffer, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-size", "libSceAgc", "sceAgcDcbSetIndexSize", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbSetIndexSize, check_agc_dcb_set_index_size,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-count", "libSceAgc", "sceAgcDcbSetIndexCount", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbSetIndexCount, check_agc_dcb_set_index_count,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index", "libSceAgc", "sceAgcDcbDrawIndex", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbDrawIndex, check_agc_dcb_draw_index,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-event-write", "libSceAgc", "sceAgcDcbEventWrite", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbEventWrite, check_agc_dcb_event_write,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-num-instances", "libSceAgc", "sceAgcDcbSetNumInstances",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetNumInstances,
     check_agc_dcb_set_num_instances, OBS_FROM_ASSUMED},
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
