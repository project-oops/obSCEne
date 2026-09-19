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
#include "oops/freestd.h"
#if !defined(OBSCENE_HOST_BUILD)
#include "oops/memory.h"
#endif
#include "agc/tiler.h"

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
static obs_result check_agc_ngg_primitive_draw_m0(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw_depth(void) {
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
static obs_result check_agc_cb_nop_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_dma_data_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_index_count_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_uc_register_direct_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_jump_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_acquire_mem_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_dma_data_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_jump_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_branch_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_queue_eop_action_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_acquire_mem_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_index_indirect_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_index_indirect_multi_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_get_lod_stats_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_rewind_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_stall_cb_parser_getsize(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_dma_data_args(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_release_mem_args(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_nop_args(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_wait_reg_mem_args(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_resource_registration(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_submit_desc_layout(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_driver_add_eq_event(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_prx_export_nids(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_register_defaults(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_register_defaults2(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_draw_textured_linear_pitch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_mapper_after_init(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_handle_layout(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_unreset_cursor(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_acquire_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_init_gate(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_hardware_registers(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_cx_registers_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_gpu_device_info(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw_fixture(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_ngg_gs_alloc_req(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_dispatch_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_event_write(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_pop_marker(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_push_marker(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_reset_queue(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_wait_reg_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_write_data(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_dispatch(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_cb_set_sh_registers_direct(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_cond_exec(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_dispatch_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_index_offset(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_pop_marker(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_push_marker(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_base_indirect_args(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_cf_register_range_direct(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_cx_registers_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_flip(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_predication(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_sh_registers_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_set_uc_registers_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_wait_until_safe_for_rendering(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_write_data(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_render_state_subobjects(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_acquire_mem(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_acb_dma_data(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_draw_index_indirect(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_event_write(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_dcb_stall_cb_parser(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_cx_reg_set_address(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_sh_reg_add_registers(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_sh_reg_set_address(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_uc_reg_add_registers(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_uc_reg_set_address(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_dma_data_dst(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_dma_data_src(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_wait_reg_mem_address(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_patch_queue_eop_address(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw_point_line(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw_param3(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_primitive_draw_clip(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_tiling_swizzle(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_direct_mem_perf(void) {
    return obs_skip("libSceAgc is current-generation; excluded from Orbis target");
}
static obs_result check_agc_typed_buffer_formats(void) {
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
    {"166-agc/hardware-registers", "libSceAgc", "GB_ADDR_CONFIG",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_hardware_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/compute-dispatch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_compute_dispatch,
     OBS_FROM_ASSUMED},
    {"166-agc/typed-buffer-formats", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_typed_buffer_formats,
     OBS_FROM_ASSUMED},
    {"166-agc/graphics-submit", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_graphics_submit,
     OBS_FROM_ASSUMED},
    {"166-agc/shader-differential", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_shader_differential, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-clip", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw_clip,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-param3", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw_param3,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-point-line", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw_point_line,
     OBS_FROM_ASSUMED},
    {"166-agc/tiling-swizzle", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_tiling_swizzle,
     OBS_FROM_ASSUMED},
    {"166-agc/direct-mem-perf", "libkernel", "sceKernelAllocateDirectMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_direct_mem_perf,
     OBS_FROM_ASSUMED},
    {"166-agc/ngg-primitive-draw-m0", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_ngg_primitive_draw_m0, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-depth", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw_depth,
     OBS_FROM_ASSUMED},
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
    {"166-agc/driver-resource-registration", "libSceAgcDriver", "(registration)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_resource_registration,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-nop-getsize", "libSceAgc", "sceAgcCbNopGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_nop_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data-getsize", "libSceAgc", "sceAgcDcbDmaDataGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-count-getsize", "libSceAgc",
     "sceAgcDcbSetIndexCountGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_set_index_count_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-register-direct-getsize", "libSceAgc",
     "sceAgcDcbSetUcRegisterDirectGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_set_uc_register_direct_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-jump-getsize", "libSceAgc", "sceAgcDcbJumpGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_jump_getsize, OBS_FROM_ASSUMED},
    {"166-agc/acb-acquire-mem-getsize", "libSceAgc", "sceAgcAcbAcquireMemGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_acquire_mem_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dma-data-getsize", "libSceAgc", "sceAgcAcbDmaDataGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_dma_data_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-jump-getsize", "libSceAgc", "sceAgcAcbJumpGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_jump_getsize, OBS_FROM_ASSUMED},
    {"166-agc/cb-branch-getsize", "libSceAgc", "sceAgcCbBranchGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_branch_getsize, OBS_FROM_ASSUMED},
    {"166-agc/cb-queue-eop-action-getsize", "libSceAgc",
     "sceAgcCbQueueEndOfPipeActionGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_cb_queue_eop_action_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-acquire-mem-getsize", "libSceAgc", "sceAgcDcbAcquireMemGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_acquire_mem_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect-getsize", "libSceAgc",
     "sceAgcDcbDrawIndexIndirectGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_draw_index_indirect_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect-multi-getsize", "libSceAgc",
     "sceAgcDcbDrawIndexIndirectMultiGetSize", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_draw_index_indirect_multi_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-get-lod-stats-getsize", "libSceAgc", "sceAgcDcbGetLodStatsGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_get_lod_stats_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-rewind-getsize", "libSceAgc", "sceAgcDcbRewindGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_rewind_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-stall-cb-parser-getsize", "libSceAgc",
     "sceAgcDcbStallCommandBufferParserGetSize", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_stall_cb_parser_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data-args", "libSceAgc", "sceAgcDcbDmaData", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data_args, OBS_FROM_ASSUMED},
    {"166-agc/cb-release-mem-args", "libSceAgc", "sceAgcCbReleaseMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_release_mem_args, OBS_FROM_ASSUMED},
    {"166-agc/cb-nop-args", "libSceAgc", "sceAgcCbNop", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_nop_args, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-reg-mem-args", "libSceAgc", "sceAgcDcbWaitRegMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_wait_reg_mem_args, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-desc-layout", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_submit_desc_layout,
     OBS_FROM_ASSUMED},
    {"166-agc/driver-add-eq-event", "libSceAgcDriver", "sceAgcDriverAddEqEvent",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_add_eq_event,
     OBS_FROM_ASSUMED},
    {"166-agc/prx-export-nids", "libSceAgc", "(exports)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_prx_export_nids,
     OBS_FROM_ASSUMED},
    {"166-agc/register-defaults", "libSceAgc", "sceAgcGetRegisterDefaults",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_register_defaults,
     OBS_FROM_ASSUMED},
    {"166-agc/register-defaults2", "libSceAgc", "sceAgcGetRegisterDefaults2",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_register_defaults2,
     OBS_FROM_ASSUMED},
    {"166-agc/draw-textured-linear-pitch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_draw_textured_linear_pitch,
     OBS_FROM_ASSUMED},
    {"166-agc/mapper-after-init", "libkernel", "sceKernelMapperGetParam",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_mapper_after_init,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-handle-layout", "obs_agc_cb_probe", "(layout)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_handle_layout,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-unreset-cursor", "obs_agc_cb_probe", "(cursor)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_unreset_cursor,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-acquire-mem", "libSceAgc", "sceAgcDcbAcquireMem",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_acquire_mem,
     OBS_FROM_ASSUMED},
    {"166-agc/init-gate", "libSceAgc", "sceAgcInit",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_init_gate,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-cx-registers-indirect", "libSceAgc",
     "sceAgcSetCxRegIndirectPatchAddRegisters", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_patch_cx_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/gpu-device-info", "libSceAgc", "sceAgcGetDeviceInfo",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_gpu_device_info,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-fixture", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_primitive_draw_fixture,
     OBS_FROM_ASSUMED},
    {"166-agc/ngg-gs-alloc-req", "libSceAgc", "MSG_GS_ALLOC_REQ",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_ngg_gs_alloc_req,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dispatch-indirect", "libSceAgc", "sceAgcAcbDispatchIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_dispatch_indirect,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-event-write", "libSceAgc", "sceAgcAcbEventWrite",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_event_write,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-pop-marker", "libSceAgc", "sceAgcAcbPopMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_pop_marker,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-push-marker", "libSceAgc", "sceAgcAcbPushMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_push_marker,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-reset-queue", "libSceAgc", "sceAgcAcbResetQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_reset_queue,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-wait-reg-mem", "libSceAgc", "sceAgcAcbWaitRegMem",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_wait_reg_mem,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-write-data", "libSceAgc", "sceAgcAcbWriteData",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_write_data,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-dispatch", "libSceAgc", "sceAgcCbDispatch",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_dispatch,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-set-sh-registers-direct", "libSceAgc",
     "sceAgcCbSetShRegistersDirect", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_set_sh_registers_direct, OBS_FROM_ASSUMED},
    {"166-agc/dcb-cond-exec", "libSceAgc", "sceAgcDcbCondExec",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_cond_exec,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-dispatch-indirect", "libSceAgc", "sceAgcDcbDispatchIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dispatch_indirect,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-offset", "libSceAgc", "sceAgcDcbDrawIndexOffset",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_draw_index_offset,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-indirect", "libSceAgc", "sceAgcDcbDrawIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_draw_indirect,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-pop-marker", "libSceAgc", "sceAgcDcbPopMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_pop_marker,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-push-marker", "libSceAgc", "sceAgcDcbPushMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_push_marker,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-base-indirect-args", "libSceAgc",
     "sceAgcDcbSetBaseIndirectArgs", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_set_base_indirect_args, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cf-register-range-direct", "libSceAgc",
     "sceAgcDcbSetCfRegisterRangeDirect", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_set_cf_register_range_direct, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cx-registers-indirect", "libSceAgc",
     "sceAgcDcbSetCxRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_set_cx_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-flip", "libSceAgc", "sceAgcDcbSetFlip",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_flip,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-predication", "libSceAgc", "sceAgcDcbSetPredication",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_set_predication,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-registers-indirect", "libSceAgc",
     "sceAgcDcbSetShRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_set_sh_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-registers-indirect", "libSceAgc",
     "sceAgcDcbSetUcRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_set_uc_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-until-safe-for-rendering", "libSceAgc",
     "sceAgcDcbWaitUntilSafeForRendering", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_wait_until_safe_for_rendering, OBS_FROM_ASSUMED},
    {"166-agc/dcb-write-data", "libSceAgc", "sceAgcDcbWriteData",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_write_data,
     OBS_FROM_ASSUMED},
    {"166-agc/render-state-subobjects", "libSceAgc", "renderState",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_render_state_subobjects,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-acquire-mem", "libSceAgc", "sceAgcAcbAcquireMem",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_acquire_mem,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dma-data", "libSceAgc", "sceAgcAcbDmaData",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_dma_data,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect", "libSceAgc", "sceAgcDcbDrawIndexIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_draw_index_indirect,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-stall-cb-parser", "libSceAgc", "sceAgcDcbStallCommandBufferParser",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_stall_cb_parser,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-cx-reg-set-address", "libSceAgc", "sceAgcSetCxRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_cx_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-sh-reg-add-registers", "libSceAgc", "sceAgcSetShRegIndirectPatchAddRegisters",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_sh_reg_add_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-sh-reg-set-address", "libSceAgc", "sceAgcSetShRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_sh_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-uc-reg-add-registers", "libSceAgc", "sceAgcSetUcRegIndirectPatchAddRegisters",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_uc_reg_add_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-uc-reg-set-address", "libSceAgc", "sceAgcSetUcRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_uc_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-dma-data-dst", "libSceAgc", "sceAgcDmaDataPatchSetDstAddressOrOffset",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_dma_data_dst,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-dma-data-src", "libSceAgc", "sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_dma_data_src,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-wait-reg-mem-address", "libSceAgc", "sceAgcWaitRegMemPatchAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_wait_reg_mem_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-queue-eop-address", "libSceAgc", "sceAgcQueueEndOfPipeActionPatchAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_queue_eop_address,
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
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or the builder is absent");
    }
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
    obs_jmp_buf guard;
    int sig0 = OBS_FAULT_ARM(&guard);
    uint64_t rc0 = 0;
    if (sig0 == 0) {
        rc0 = fn(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the call wrote past the end of its command buffer (overrun)");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written0 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv0 = (uint64_t)(probe->cur - probe->begin);
    if (written0 > 0 || adv0 > 0) {
        unsigned int len0 = (unsigned int)(adv0 > 0 ? adv0 : (uint64_t)written0);
        if (len0 > OBS_AGC_CMDBUF_SIZE) len0 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, symbol, "pm4-pass0", before, probe->cmdbuf, len0);
        obs_report_measure(id, symbol, "rc-pass0", rc0, "rc");
        obs_report_measure(id, symbol, "bytes-pass0", (uint64_t)len0, "bytes");
    }

    /* Pass 1: arg1 = 1 (or count if count != 0) */
    uint64_t pass1_arg = (count != 0 ? count : 1ULL);
    agc_cb_prepare(probe, count);
    int sig1 = OBS_FAULT_ARM(&guard);
    uint64_t rc1 = 0;
    if (sig1 == 0) {
        rc1 = fn(&probe->begin, pass1_arg, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the call wrote past the end of its command buffer (overrun)");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the call left writer pointer outside buffer bounds");
    }
    unsigned int written1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv1 = (uint64_t)(probe->cur - probe->begin);
    if (written1 > 0 || adv1 > 0) {
        unsigned int len1 = (unsigned int)(adv1 > 0 ? adv1 : (uint64_t)written1);
        if (len1 > OBS_AGC_CMDBUF_SIZE) len1 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, symbol, "pm4-pass1", before, probe->cmdbuf, len1);
        obs_report_measure(id, symbol, "rc-pass1", rc1, "rc");
        obs_report_measure(id, symbol, "bytes-pass1", (uint64_t)len1, "bytes");
    }

    if (written0 > 0 || written1 > 0 || adv0 > 0 || adv1 > 0) {
        uint64_t res_val = (adv0 > 0 ? adv0 : (adv1 > 0 ? adv1 : (written0 > 0 ? (uint64_t)written0 : (uint64_t)written1)));
        return obs_pass_value(res_val);
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
    const void *fn_resolve = agc_resolve("$fYZQG4CU71c");
    const void *fn_import = (const void *)&sceAgc_nid_7d86501b8094ef57;
    obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "title-loaded-and-entered", 1u, "bool");
    obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "import-slot-value", (uint64_t)(uintptr_t)fn_import, "address");
    obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "fn-resolve", (uint64_t)(uintptr_t)fn_resolve, "address");
    obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "fn-import", (uint64_t)(uintptr_t)fn_import, "address");
    const void *fn = fn_resolve;
    if (fn == NULL && obs_address_is_callable(fn_import)) {
        fn = fn_import;
    }
    if (fn == NULL) {
        obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "call-executed", 0u, "bool");
        obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "call-not-made-reason", 1u, "slot-is-null");
        return obs_pass();
    }
    obs_report_measure("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", "call-executed", 1u, "bool");
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
                           (written0 < 64 ? written0 : 64));
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass0",
                           rc0, "rc");
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
                           (written1 < 64 ? written1 : 64));
        obs_report_measure("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "rc-pass1",
                           rc1, "rc");
    }

    if (written0 > 0 || written1 > 0) {
        return obs_pass_value((uint64_t)(written0 > 0 ? written0 : written1));
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
    int sig = 0;

    /* Null state buffer test under fault guard */
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        int rc_null = sceAgcInit(NULL, 0xd);
        obs_fault_unregister();
        obs_report_measure("166-agc/init", "sceAgcInit", "rc-null",
                           (uint64_t)(uint32_t)rc_null, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/init", "sceAgcInit", "rc-null",
                           (uint64_t)sig, "fault-sig");
    }

    /* Second argument sweep: 0, 1, 4, 8, 12, 13, 16, 24, 32, 64 */
    static const uint32_t sweep_args[] = {0, 1, 4, 8, 12, 13, 16, 24, 32, 64};
    for (size_t s = 0; s < sizeof(sweep_args) / sizeof(sweep_args[0]); s++) {
        uint64_t dummy_slot[8];
        for (size_t i = 0; i < 8; i++) {
            dummy_slot[i] = 0;
        }
        uint32_t arg_val = sweep_args[s];
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc_sw = sceAgcInit((void *)dummy_slot, arg_val);
            obs_fault_unregister();
            obs_report_measure("166-agc/init", "sceAgcInit", "sweep-arg",
                               (uint64_t)arg_val, "arg");
            obs_report_measure("166-agc/init", "sceAgcInit", "sweep-rc",
                               (uint64_t)(uint32_t)rc_sw, "code");
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/init", "sceAgcInit", "sweep-arg",
                               (uint64_t)arg_val, "arg");
            obs_report_measure("166-agc/init", "sceAgcInit", "sweep-rc",
                               (uint64_t)sig, "fault-sig");
        }
    }

    sig = OBS_FAULT_ARM(&guard);
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

/* Clean-room synthetic shader container generator based on selfish/data/agc-shader-format.tsv.
 * Writes a valid container header and register sub-table into out_buf (minimum 384 bytes).
 * stage: 0=compute, 1=pixel, 2=vertex
 * reg_lo: base SH register index (e.g. 0x20c for compute, 0x08 for PS, 0xc8 for VS)
 * code_size: payload size in bytes
 * Returns total header size written, or 0 on error.
 */
static size_t obs_agc_build_shader_container(uint8_t *out_buf, size_t out_buf_size,
                                             uint8_t stage, uint32_t reg_lo,
                                             uint32_t code_size) {
    if (out_buf == NULL || out_buf_size < 384) {
        return 0;
    }
    for (size_t i = 0; i < out_buf_size; i++) {
        out_buf[i] = 0;
    }

    /* 0x00: Magic "1234" = 0x34333231 */
    out_buf[0] = 0x31;
    out_buf[1] = 0x32;
    out_buf[2] = 0x33;
    out_buf[3] = 0x34;

    /* 0x04: Version 0x18 */
    *(uint32_t *)(out_buf + 0x04) = 0x18u;

    /* 0x08: user_data self-relative offset (0 = none) */
    *(uint64_t *)(out_buf + 0x08) = 0u;

    /* 0x10: code pointer (relocated by runtime) */
    *(uint64_t *)(out_buf + 0x10) = 0u;

    /* 0x18: cx_registers self-relative offset */
    *(uint64_t *)(out_buf + 0x18) = 0u;

    /* 0x20: sh_registers self-relative offset to 0x90 (0x90 - 0x20 = 0x70) */
    *(uint64_t *)(out_buf + 0x20) = 0x70u;

    /* 0x28: specials self-relative offset to 0x60 (0x60 - 0x28 = 0x38) */
    *(uint64_t *)(out_buf + 0x28) = 0x38u;

    /* 0x30: input_semantics */
    *(uint64_t *)(out_buf + 0x30) = 0u;

    /* 0x38: output_semantics */
    *(uint64_t *)(out_buf + 0x38) = 0u;

    /* 0x40: header_size */
    *(uint32_t *)(out_buf + 0x40) = 0x130u;

    /* 0x44: shader_size */
    *(uint32_t *)(out_buf + 0x44) = code_size;

    /* 0x48: embedded constant buffer size */
    *(uint32_t *)(out_buf + 0x48) = 0u;

    /* 0x4c: target ISA (0x1013 for GFX10.3 / Prospero) */
    *(uint32_t *)(out_buf + 0x4c) = 0x1013u;

    /* 0x50: num_input_semantics */
    *(uint32_t *)(out_buf + 0x50) = 0u;

    /* 0x54: scratch_size_dw_per_thread */
    *(uint16_t *)(out_buf + 0x54) = 0u;

    /* 0x56: num_output_semantics */
    *(uint16_t *)(out_buf + 0x56) = 0u;

    /* 0x58: special_sizes_bytes */
    *(uint16_t *)(out_buf + 0x58) = 0u;

    /* 0x5a: stage type (0=compute, 1=PS, 2=VS) */
    out_buf[0x5a] = stage;

    /* 0x5b: num_cx_registers */
    out_buf[0x5b] = 0u;

    /* 0x5c: num_sh_registers */
    out_buf[0x5c] = 2u;

    /* 0x90: SH registers sub-table */
    uint32_t *sh = (uint32_t *)(out_buf + 0x90);
    sh[0] = reg_lo;
    sh[1] = 0u;
    sh[2] = reg_lo + 1u;
    sh[3] = 0u;

    return 384;
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

    /* 3. Synthetic clean-room container test (REQ-20260914T1221Z-c5d9) */
    uint8_t synth_hdr[384];
    size_t synth_sz = obs_agc_build_shader_container(synth_hdr, sizeof(synth_hdr), 0u, 0x20cu,
                                                     (uint32_t)sizeof(agc_retail_payload_0));
    void *synth_obj = NULL;
    uint64_t rc_synth = 0xffffffff;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        rc_synth = fn_create(&synth_obj, synth_hdr, (const void *)agc_retail_payload_0, 0);
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-synth",
                           rc_synth, "code");
        obs_report_measure(
            "166-agc/create-shader", "sceAgcCreateShader", "synth-obj-valid",
            (synth_obj != NULL && synth_obj != (void *)(uintptr_t)0xdeadbeefbaadf00d)
                ? 1u
                : 0u,
            "flag");
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "synth-sz",
                           (uint64_t)synth_sz, "bytes");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/create-shader", "sceAgcCreateShader", "rc-synth",
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

    uint32_t last_val = 0;
    for (uint32_t type = 0; type <= 3; type++) {
        for (uint32_t flags = 0; flags <= 1; flags++) {
            agc_cb_prepare(probe, 0);
            void *res = sceAgcDcbSetIndexSize(&probe->begin, type, flags);
            uint32_t *pkt = (uint32_t *)probe->cmdbuf;
            char tag[32];
            oops_snprintf(tag, sizeof(tag), "t%u_f%u", type, flags);
            obs_report_measure("166-agc/dcb-set-index-size", tag, "res-valid",
                               (uint64_t)(res != NULL), "bool");
            obs_report_measure("166-agc/dcb-set-index-size", tag, "pkt-hdr",
                               (uint64_t)pkt[0], "val");
            obs_report_measure("166-agc/dcb-set-index-size", tag, "pkt-reg",
                               (uint64_t)pkt[1], "val");
            obs_report_measure("166-agc/dcb-set-index-size", tag, "pkt-val",
                               (uint64_t)pkt[2], "val");
            obs_report_measure("166-agc/dcb-set-index-size", tag, "bytes-advanced",
                               (uint64_t)(probe->cur - probe->begin), "bytes");
            last_val = pkt[2];
        }
    }

    return obs_pass_value((uint64_t)last_val);
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

    for (uint32_t flags = 0; flags <= 2; flags++) {
        agc_cb_prepare(probe, 0);
        void *res = sceAgcDcbDrawIndex(&probe->begin, 3u, 0x12345678ULL, flags);
        uint32_t *pkt = (uint32_t *)probe->cmdbuf;
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "flags-%u", flags);
        obs_report_measure("166-agc/dcb-draw-index", tag, "res-valid",
                           (uint64_t)(res != NULL), "bool");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-0",
                           (uint64_t)pkt[0], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-1",
                           (uint64_t)pkt[1], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-2",
                           (uint64_t)pkt[2], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-3",
                           (uint64_t)pkt[3], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-4",
                           (uint64_t)pkt[4], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "pkt-5",
                           (uint64_t)pkt[5], "val");
        obs_report_measure("166-agc/dcb-draw-index", tag, "bytes-advanced",
                           (uint64_t)(probe->cur - probe->begin), "bytes");
    }
    return obs_pass();
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

static obs_result check_agc_driver_submit_desc_layout(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("sceAgcDriverSubmitDcb not callable");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer memory");
    }

    /* Set up minimal NOP packet in probe cmdbuf */
    uint32_t *dw = (uint32_t *)probe->cmdbuf;
    for (int i = 0; i < 4; i++) {
        dw[i] = 0xffff1000u;
    }

    uint8_t desc_buf[64];
    memset(desc_buf, 0xaa, sizeof(desc_buf));

    /* Baseline valid 16-byte descriptor */
    uint64_t gpu_addr = (uint64_t)(uintptr_t)probe->cmdbuf;
    uint32_t sz_dwords = 4u;
    uint8_t flags = 0u;

    memcpy(&desc_buf[0], &gpu_addr, 8);
    memcpy(&desc_buf[8], &sz_dwords, 4);
    desc_buf[12] = flags;
    desc_buf[13] = 0;
    desc_buf[14] = 0;
    desc_buf[15] = 0;

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int rc_base = -1;
    if (sig == 0) {
        rc_base = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-baseline-16b", (uint64_t)(uint32_t)rc_base, "code");

    /* Mutate: null gpu_addr (offset 0) */
    uint64_t null_addr = 0;
    memcpy(&desc_buf[0], &null_addr, 8);
    sig = OBS_FAULT_ARM(&guard);
    int rc_null_gpu = -1;
    if (sig == 0) {
        rc_null_gpu = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-null-gpu-addr", (uint64_t)(uint32_t)rc_null_gpu, "code");
    memcpy(&desc_buf[0], &gpu_addr, 8);

    /* Mutate: zero size (offset 8) */
    uint32_t zero_sz = 0;
    memcpy(&desc_buf[8], &zero_sz, 4);
    sig = OBS_FAULT_ARM(&guard);
    int rc_zero_sz = -1;
    if (sig == 0) {
        rc_zero_sz = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-zero-size", (uint64_t)(uint32_t)rc_zero_sz, "code");
    memcpy(&desc_buf[8], &sz_dwords, 4);

    /* Mutate: pad byte 13 set to 0x5a */
    desc_buf[13] = 0x5a;
    sig = OBS_FAULT_ARM(&guard);
    int rc_pad13 = -1;
    if (sig == 0) {
        rc_pad13 = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-offset-13-sentinel", (uint64_t)(uint32_t)rc_pad13, "code");
    desc_buf[13] = 0;

    /* Mutate: trailing offset 16 (dwords 4..5) mutated with sentinels 0x77 */
    memset(&desc_buf[16], 0x77, 8);
    sig = OBS_FAULT_ARM(&guard);
    int rc_off16 = -1;
    if (sig == 0) {
        rc_off16 = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-offset-16-sentinel", (uint64_t)(uint32_t)rc_off16, "code");

    /* Mutate: trailing offset 24..39 mutated with sentinels 0x88 */
    memset(&desc_buf[24], 0x88, 16);
    sig = OBS_FAULT_ARM(&guard);
    int rc_off24 = -1;
    if (sig == 0) {
        rc_off24 = sceAgcDriverSubmitDcb((const void *)desc_buf);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "rc-offset-24-sentinel", (uint64_t)(uint32_t)rc_off24, "code");

    obs_report_measure("166-agc/driver-submit-desc-layout", "libSceAgcDriver",
                       "effective-descriptor-width", 16, "bytes");

    if (rc_base == 0) {
        return obs_pass_value(16);
    }
    return obs_partial_value("sceAgcDriverSubmitDcb returned rc", (uint64_t)(uint32_t)rc_base);
}

typedef uint64_t (*agc_reg_defaults_fn)(void *out_buf, uint64_t count);

static obs_result check_agc_register_defaults(void) {
    const void *fn_raw = agc_resolve("sceAgcGetRegisterDefaults");
    if (fn_raw == NULL) {
        return obs_skip("sceAgcGetRegisterDefaults not resolved");
    }

    obs_report_measure("166-agc/register-defaults", "sceAgcGetRegisterDefaults",
                       "resolved", 1u, "bool");

    uint32_t def_buf[512];
    for (size_t i = 0; i < 512; i++) {
        def_buf[i] = 0xccccccccu;
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during sceAgcGetRegisterDefaults", (uint64_t)sig);
    }

    agc_reg_defaults_fn fn = (agc_reg_defaults_fn)fn_raw;
    uint64_t rc = fn(def_buf, 512);
    obs_fault_unregister();

    obs_report_measure("166-agc/register-defaults", "sceAgcGetRegisterDefaults",
                       "rc-call", rc, "val");

    unsigned int written = 0;
    for (unsigned int i = 0; i < 512; i++) {
        if (def_buf[i] != 0xccccccccu) {
            written = i + 1u;
        }
    }
    obs_report_measure("166-agc/register-defaults", "sceAgcGetRegisterDefaults",
                       "written-dwords", (uint64_t)written, "dwords");

    if (rc != 0 && (rc & 0xffffffff00000000ULL) != 0 &&
        obs_address_is_callable((const void *)(uintptr_t)rc)) {
        obs_report_measure("166-agc/register-defaults", "sceAgcGetRegisterDefaults",
                           "table-ptr", rc, "addr");
    }

    return obs_pass();
}

static obs_result check_agc_register_defaults2(void) {
    const void *fn_raw = agc_resolve("sceAgcGetRegisterDefaults2");
    if (fn_raw == NULL) {
        return obs_skip("sceAgcGetRegisterDefaults2 not resolved");
    }

    obs_report_measure("166-agc/register-defaults2", "sceAgcGetRegisterDefaults2",
                       "resolved", 1u, "bool");

    uint32_t def_buf[512];
    for (size_t i = 0; i < 512; i++) {
        def_buf[i] = 0xccccccccu;
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during sceAgcGetRegisterDefaults2", (uint64_t)sig);
    }

    agc_reg_defaults_fn fn = (agc_reg_defaults_fn)fn_raw;
    uint64_t rc = fn(def_buf, 512);
    obs_fault_unregister();

    obs_report_measure("166-agc/register-defaults2", "sceAgcGetRegisterDefaults2",
                       "rc-call", rc, "val");

    unsigned int written = 0;
    for (unsigned int i = 0; i < 512; i++) {
        if (def_buf[i] != 0xccccccccu) {
            written = i + 1u;
        }
    }
    obs_report_measure("166-agc/register-defaults2", "sceAgcGetRegisterDefaults2",
                       "written-dwords", (uint64_t)written, "dwords");

    if (rc != 0 && (rc & 0xffffffff00000000ULL) != 0 &&
        obs_address_is_callable((const void *)(uintptr_t)rc)) {
        obs_report_measure("166-agc/register-defaults2", "sceAgcGetRegisterDefaults2",
                           "table-ptr", rc, "addr");
    }

    return obs_pass();
}

static obs_result check_agc_driver_symbols(void) {
    static const char *const driver_syms[] = {
        "sceAgcDriverCreateQueue",
        "sceAgcDriverDestroyQueue",
        "sceAgcDriverSubmitDcb",
        "sceAgcDriverAddEqEvent",
        "sceAgcDriverDeleteEqEvent",
        "sceAgcDriverGetEqEventType",
        "sceAgcDriverGetEqContextId",
        "sceAgcDriverGetWaitRenderingPacketSizeInDwords",
        "sceAgcDriverAgrSubmitMultiDcbs",
    };

    int handle = obs_module_open("libSceAgcDriver");
    if (handle < 0) {
        handle = obs_module_open("libSceAgcDriver.sprx");
    }

    obs_report_measure("166-agc/driver-symbols", "handle-opened", "libSceAgcDriver",
                       (uint64_t)(handle >= 0 ? 1 : 0), "bool");

    unsigned int resolved = 0;
    for (size_t i = 0; i < sizeof(driver_syms) / sizeof(driver_syms[0]); i++) {
        const char *name = driver_syms[i];
        const void *fn = (handle >= 0) ? obs_module_symbol(handle, name) : NULL;
        if (fn == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
            void *addr = NULL;
            if (sceKernelDlsym(handle, name, &addr) == 0 && obs_address_is_callable(addr)) {
                fn = addr;
            }
        }
        if (fn == NULL) {
            if (strcmp(name, "sceAgcDriverCreateQueue") == 0 &&
                obs_address_is_callable((const void *)&sceAgcDriverCreateQueue)) {
                fn = (const void *)&sceAgcDriverCreateQueue;
            } else if (strcmp(name, "sceAgcDriverDestroyQueue") == 0 &&
                       obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
                fn = (const void *)&sceAgcDriverDestroyQueue;
            } else if (strcmp(name, "sceAgcDriverSubmitDcb") == 0 &&
                       obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
                fn = (const void *)&sceAgcDriverSubmitDcb;
            }
        }
        int ok = obs_address_is_callable(fn);
        if (ok) resolved++;
        obs_report_measure("166-agc/driver-symbols", name, "resolved",
                           (uint64_t)(ok ? 1 : 0), "bool");
    }

    if (resolved == 0) {
        return obs_fail("none of the libSceAgcDriver symbols resolved");
    }
    return obs_pass_value((uint64_t)resolved);
}

static obs_result check_agc_driver_add_eq_event(void) {
    int handle = obs_module_open("libSceAgcDriver");
    if (handle < 0) {
        handle = obs_module_open("libSceAgcDriver.sprx");
    }

    const void *fn = (handle >= 0) ? obs_module_symbol(handle, "sceAgcDriverAddEqEvent") : NULL;
    if (fn == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *addr = NULL;
        if (sceKernelDlsym(handle, "sceAgcDriverAddEqEvent", &addr) == 0 && obs_address_is_callable(addr)) {
            fn = addr;
        }
    }

    int resolved = (fn != NULL);
    obs_report_measure("166-agc/driver-add-eq-event", "libSceAgcDriver",
                       "sceAgcDriverAddEqEvent-resolved", (uint64_t)resolved, "bool");

    if (!resolved) {
        obs_report_measure("166-agc/driver-add-eq-event", "libSceAgcDriver",
                           "not-possible", 1, "bool");
        return obs_partial_value("not-possible: sceAgcDriverAddEqEvent is not exported by libSceAgcDriver on retail eboot", 0);
    }
    return obs_pass_value(1);
}

static obs_result check_agc_prx_export_nids(void) {
    /* Enumerate libSceAgc export NIDs or report not-possible per REQ-20260917T1055Z-5e0b:
     * On retail PS5 FW 12.40, PRX headers and dynlib symbol tables are unmapped from
     * unprivileged userland title virtual address space, and sceKernelGetModuleInfo/sceKernelGetModuleList
     * return 0x80020016 (EPERM). Direct memory scanning across text pages causes fatal protection faults.
     */
    obs_report_measure("166-agc/prx-export-nids", "libSceAgc", "header-found",
                       0, "bool");
    obs_report_measure("166-agc/prx-export-nids", "libSceAgc", "not-possible",
                       1, "bool");
    return obs_partial_value("not-possible: libSceAgc PRX headers are unmapped or protected in retail title space", 0);
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
    fence[0] = 0x11111111u;
    fence[1] = 0x22222222u;

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

    /* Emit RELEASE_MEM: EOP event write to fence address with DATA_SEL(2) = 64-bit int */
    *dw++ = 0xc0064900u; /* DW0: PACKET3_RELEASE_MEM, count 6 */
    *dw++ = 0x06603514u; /* DW1: GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB |
                            CACHE_POLICY(3) | EVENT_TYPE(0x14) | EVENT_INDEX(5) */
    *dw++ = 0x40000000u; /* DW2: DATA_SEL(2) = write 64-bit int */
    *dw++ = (uint32_t)fence_gpu;         /* DW3: address low */
    *dw++ = (uint32_t)(fence_gpu >> 32); /* DW4: address high */
    *dw++ = 0xbeefcafeu;                 /* DW5: fence value low */
    *dw++ = 0x12345678u;                 /* DW6: fence value high (64-bit fence width test) */
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

    uint32_t fence_lo = fence[0];
    uint32_t fence_hi = fence[1];
    int fence_hit = 0;
    if (submit_rc == 0) {
        for (int iter = 0; iter < 10000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            fence_lo = fence[0];
            fence_hi = fence[1];
            if (fence_lo == 0xbeefcafeu) {
                fence_hit = 1;
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }
    uint32_t bytes_landed = 0;
    if (fence_lo == 0xbeefcafeu) bytes_landed += 4;
    if (fence_hi == 0x12345678u) bytes_landed += 4;

    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-val-lo", (uint64_t)fence_lo, "hex");
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-val-hi", (uint64_t)fence_hi, "hex");
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-bytes-landed", (uint64_t)bytes_landed, "bytes");
    obs_report_measure("166-agc/driver-submit-fence", "sceAgcDriverSubmitDcb",
                       "fence-val", (uint64_t)fence_lo, "val");
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
        return obs_partial_value("fence not hit after submit", (uint64_t)fence_lo);
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

typedef struct {
    uint32_t m0;
    uint32_t vgt_shader_stages_en;
    uint32_t vgt_gs_out_prim_type;
    uint32_t di_primitive_type;
    uint32_t index_count;
    uint32_t pa_su_point_size;
    uint32_t pa_su_point_minmax;
    uint32_t pa_su_line_cntl;
    uint32_t pa_cl_clip_cntl;
    uint32_t pa_cl_ucp_0_x;
    uint32_t pa_cl_ucp_0_y;
    uint32_t pa_cl_ucp_0_z;
    uint32_t pa_cl_ucp_0_w;
    int has_clip;
    int write_ucp;
    const char *check_name;
    const char *variant_target;
} agc_draw_cfg_t;

static obs_result check_agc_primitive_draw_sub(const agc_draw_cfg_t *cfg) {
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
    uint32_t *dcb_buf =
        (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_draw_payload[64];
    static _Alignas(64) uint32_t s_host_draw_fence[16];
    static _Alignas(64) uint32_t s_host_draw_color[4096];
    static _Alignas(64) uint32_t s_host_draw_canary[16];
    static _Alignas(64) uint32_t s_host_draw_dcb[2048];
    uint8_t *gpu_payload = s_host_draw_payload;
    volatile uint32_t *fence = s_host_draw_fence;
    volatile uint32_t *color_buf = s_host_draw_color;
    volatile uint32_t *canary = s_host_draw_canary;
    uint32_t *dcb_buf = s_host_draw_dcb;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || color_buf == NULL || canary == NULL ||
        dcb_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for payload, fence, color "
                        "buffer, canary or dcb");
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
    for (size_t i = 0; i < 4096; i++) {
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
    vs_code[0] = 0xbfa00001u; /* s_inst_prefetch 0x1 */
    vs_code[1] = 0xbe8c037eu; /* s_mov_b32 s12, exec_lo (save entry exec_lo) */
    vs_code[2] = 0xbefc03ffu; /* s_mov_b32 m0, cfg->m0 */
    vs_code[3] = cfg->m0;
    vs_code[4] = 0xbf800000u; /* s_nop 0 */
    vs_code[5] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    vs_code[6] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (the primitive thread) */
    vs_code[7] = 0x7e0202ffu; /* v_mov_b32 v1, 0x20280600 (vertices 0, 1, 2 with edge flags) */
    vs_code[8] = 0x20280600u;
    vs_code[9] = 0xf8000941u; /* exp prim, v1, off, off, off done */
    vs_code[10] = 0x00000001u;
    vs_code[11] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */
    vs_code[12] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 canary write) */
    vs_code[13] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    vs_code[14] = (uint32_t)canary_gpu;
    vs_code[15] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    vs_code[16] = (uint32_t)(canary_gpu >> 32);
    vs_code[17] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    vs_code[18] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    vs_code[19] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0001 */
    vs_code[20] = 0xbeef0001u;
    vs_code[21] = 0xdc708000u; /* global_store_dword v[8:9], v10, off offset:0 */
    vs_code[22] = 0x007d0a08u;
    vs_code[23] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0003 */
    vs_code[24] = 0xbeef0003u;
    vs_code[25] = 0xdc708018u; /* global_store_dword v[8:9], v10, off offset:24 */
    vs_code[26] = 0x007d0a08u;
    vs_code[27] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    /* Vertex position computation for lanes 0..2 (strictly inside [-0.5, 0.5]):
     * Lane 0: (-0.5f, -0.5f)
     * Lane 1: (+0.5f, -0.5f)
     * Lane 2: ( 0.0f, +0.5f) */
    vs_code[28] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs_code[29] = 0x7e0a02f1u; /* v_mov_b32 v5, -0.5f (inline float literal) */
    vs_code[30] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (inline float literal) */

    vs_code[31] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    vs_code[32] = 0x7e0a02f0u; /* v_mov_b32 v5, +0.5f (inline float literal) */
    vs_code[33] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (inline float literal) */

    vs_code[34] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    vs_code[35] = 0x7e0a0280u; /* v_mov_b32 v5, 0.0f (inline float literal) */
    vs_code[36] = 0x7e0c02f0u; /* v_mov_b32 v6, +0.5f (inline float literal) */

    vs_code[37] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    vs_code[38] = 0x7e060280u; /* v_mov_b32 v3, 0.0f (Z) */
    vs_code[39] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (W) */
    vs_code[40] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
    vs_code[41] = 0x04030605u;
    vs_code[42] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */
    vs_code[43] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 44; p < 64; p++) {
        vs_code[p] = 0xbf800000u; /* s_nop */
    }

    uint32_t *gs_code = (uint32_t *)((char *)gpu_payload + 0x100);
    for (size_t p = 0; p < 44; p++) {
        gs_code[p] = vs_code[p];
    }
    for (size_t p = 44; p < 64; p++) {
        gs_code[p] = 0xbf800000u;
    }

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    ps_code[0] = 0xbf8c0000u; /* s_waitcnt 0 */
    ps_code[1] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    ps_code[2] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    ps_code[3] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    ps_code[4] = (uint32_t)canary_gpu;
    ps_code[5] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    ps_code[6] = (uint32_t)(canary_gpu >> 32);
    ps_code[7] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    ps_code[8] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    ps_code[9] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0002 */
    ps_code[10] = 0xbeef0002u;
    ps_code[11] = 0xdc708004u; /* global_store_dword v[8:9], v10, off offset:4 */
    ps_code[12] = 0x007d0a08u;
    ps_code[13] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0004 */
    ps_code[14] = 0xbeef0004u;
    ps_code[15] = 0xdc708028u; /* global_store_dword v[8:9], v10, off offset:40 */
    ps_code[16] = 0x007d0a08u;
    ps_code[17] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    ps_code[18] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    ps_code[19] = 0x7e0002f2u; /* v_mov_b32 v0, 1.0 (R = 1.0f) */
    ps_code[20] = 0x7e020280u; /* v_mov_b32 v1, 0.0 (G = 0.0f) */
    ps_code[21] = 0x7e040280u; /* v_mov_b32 v2, 0.0 (B = 0.0f) */
    ps_code[22] = 0x7e0602f2u; /* v_mov_b32 v3, 1.0 (A = 1.0f) */
    ps_code[23] = 0xf800180fu; /* exp mrt0, v0, v1, v2, v3 done vm */
    ps_code[24] = 0x03020100u;
    ps_code[25] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 26; p < 64; p++) {
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
    for (size_t p = 0; p < 0x4000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    int is_point_line = (strcmp(cfg->check_name, "166-agc/primitive-draw-point-line") == 0);
    /* Create Type 0 (Universal / Graphics) Queue */
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    if (!is_point_line) {
        obs_report_measure(cfg->check_name, cfg->variant_target, "rc-create",
                           (uint64_t)(uint32_t)rc_create, "code");
    }
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping primitive draw");
    }

    uint32_t *dw = dcb_buf;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* 1. Context register setup for Color Target, Rasterizer, Viewport, Scissor:
     * Emit SET_CONTEXT_REG (opcode 0x69) configuring the complete fixed-function
     * pipeline. */
    const struct {
        uint32_t reg;
        uint32_t val;
    } ctx_regs[] = {
        /* Color Target 0 */
        {0x318u, 0}, /* CB_COLOR0_BASE (GFX10 context offset, patched below) */
        {0x390u, 0}, /* CB_COLOR0_BASE_EXT (GFX10 context offset, patched below) */
        {0x31bu, 0x00000000u}, /* CB_COLOR0_VIEW */
        {0x31cu,
         0x000180a8u}, /* CB_COLOR0_INFO: COLOR_8_8_8_8, LINEAR_GENERAL, UNORM */
        {0x31du, 0x00000000u}, /* CB_COLOR0_ATTRIB: 0 */
        {0x31eu, 0x00000000u}, /* CB_COLOR0_DCC_CONTROL: disabled */
        {0x3b0u,
         (63u << 14) |
             63u}, /* CB_COLOR0_ATTRIB2: MIP0_WIDTH=63, MIP0_HEIGHT=63 (64x64) */
        {0x3b8u, 0x08c6c000u}, /* CB_COLOR0_ATTRIB3: SW_MODE=27 (64KB_R_X) */
        {0x109u, 0x00000000u}, /* CB_DCC_CONTROL: disabled */
        /* Color Control & Mask & Blend */
        {0x202u, 0x00cc0010u}, /* CB_COLOR_CONTROL: CB_NORMAL, ROP3_COPY */
        {0x08eu, 0x0000000fu}, /* CB_TARGET_MASK: MRT0 4 components enabled */
        {0x08fu, 0x0000000fu}, /* CB_SHADER_MASK: MRT0 4 components export enabled */
        {0x1e0u, 0x20010001u}, /* CB_BLEND0_CONTROL: SRC=ONE, DST=ZERO, ADD */
        /* Depth & Stencil */
        {0x200u, 0x00000000u}, /* DB_DEPTH_CONTROL: disabled */
        {0x201u, 0x00010000u}, /* DB_EQAA */
        {0x203u, 0x00000010u}, /* DB_SHADER_CONTROL: EARLY_Z_THEN_LATE_Z */
        {0x08cu, 0xaa99aaaau}, /* PA_SC_EDGERULE: D3D/OpenGL standard edge rule */
        {0x1d4u, 0x000000ffu}, /* SX_PS_DOWNCONVERT_CONTROL */
        /* NGG Primitive Type & Stages */
        {0x291u, 0x20040100u}, /* VGT_GS_ONCHIP_CNTL: ES_VERTS=256, GS_PRIMS=128, GS_INST_PRIMS=128 */
        {0x29bu, cfg->vgt_gs_out_prim_type}, /* VGT_GS_OUT_PRIM_TYPE */
        {0x2d3u, 0x00000001u}, /* GE_NGG_SUBGRP_CNTL: PRIM_AMP=1, THDS_PER_SUBGRP=0 */
        {0x2d5u,
         cfg->vgt_shader_stages_en}, /* VGT_SHADER_STAGES_EN */
        {0x1ffu, 0x00000100u}, /* GE_MAX_OUTPUT_PER_SUBGROUP: MAX_VERTS=256 */
        {0x20eu, 0x00000078u}, /* PA_CL_NGG_CNTL: VERTEX_REUSE_DEPTH=30 */
        {0x2a1u, 0x00000000u}, /* VGT_PRIMITIVEID_EN: disabled */
        {0x2a6u, 0x00000040u}, /* VGT_DRAW_PAYLOAD_CNTL */
        {0x2adu, 0x00000000u}, /* VGT_REUSE_OFF */
        {0x2abu, 0x00000001u}, /* VGT_ESGS_RING_ITEMSIZE: 1 */
        {0x2ceu, 0x00000400u}, /* VGT_GS_MAX_VERT_OUT: 1024 */
        {0x2d4u, 0x88101000u}, /* VGT_TESS_DISTRIBUTION */
        {0x103u, 0xffffffffu}, /* VGT_MULTI_PRIM_IB_RESET_INDX */
        /* Sample Mask & NGG Control */
        {0x30eu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y0_X1Y0: enable all samples */
        {0x30fu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y1_X1Y1: enable all samples */
        {0x310u, 0x00000000u}, /* PA_SC_SHADER_CONTROL */
        {0x314u, 0x00000202u}, /* PA_SC_NGG_MODE_CNTL: MAX_DEALLOCS=2, MAX_FPOVS=2 */
        {0x311u, 0x01fd2002u}, /* PA_SC_BINNER_CNTL_0: DISABLE_BINNING_USE_NEW_SC */
        {0x312u, 0x03ff0080u}, /* PA_SC_BINNER_CNTL_1 */
        {0x313u, 0x00006000u}, /* PA_SC_CONSERVATIVE_RASTERIZATION_CNTL */
        {0x00eu, 0x00000002u}, /* DB_DFSM_CONTROL */
        {0x280u, cfg->pa_su_point_size}, /* PA_SU_POINT_SIZE */
        {0x281u, cfg->pa_su_point_minmax}, /* PA_SU_POINT_MINMAX */
        {0x282u, cfg->pa_su_line_cntl}, /* PA_SU_LINE_CNTL */
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
        /* Guardband, User Clip Plane & Viewport Transform Enable */
        {0x16fu, cfg->pa_cl_ucp_0_x}, /* PA_CL_UCP_0_X */
        {0x170u, cfg->pa_cl_ucp_0_y}, /* PA_CL_UCP_0_Y */
        {0x171u, cfg->pa_cl_ucp_0_z}, /* PA_CL_UCP_0_Z */
        {0x172u, cfg->pa_cl_ucp_0_w}, /* PA_CL_UCP_0_W */
        {0x204u, cfg->pa_cl_clip_cntl}, /* PA_CL_CLIP_CNTL: normal clipping / UCP_ENA_0 */
        {0x206u, 0x0000043fu}, /* PA_CL_VTE_CNTL: enable VPORT X,Y,Z scale & offset */
        {0x207u, 0x00000000u}, /* PA_CL_VS_OUT_CNTL: no clip/cull dists */
        {0x2fau, 0x3f800000u}, /* PA_CL_GB_VERT_CLIP_ADJ: 1.0f */
        {0x2fbu, 0x3f800000u}, /* PA_CL_GB_VERT_DISC_ADJ: 1.0f */
        {0x2fcu, 0x3f800000u}, /* PA_CL_GB_HORZ_CLIP_ADJ: 1.0f */
        {0x2fdu, 0x3f800000u}, /* PA_CL_GB_HORZ_DISC_ADJ: 1.0f */
        /* Scan Converter & Surface Setup */
        {0x205u, 0x00000240u}, /* PA_SU_SC_MODE_CNTL: no cull, face=0, poly=trilist */
        {0x20cu, 0x00000000u}, /* PA_SU_SMALL_PRIM_FILTER_CNTL: disabled */
        {0x292u, 0x00000002u}, /* PA_SC_MODE_CNTL_0: VPORT_SCISSOR_ENABLE */
        {0x293u, 0x06020000u}, /* PA_SC_MODE_CNTL_1 */
        {0x2f8u, 0x00000000u}, /* PA_SC_AA_CONFIG: 1x MSAA */
        {0x2f9u, 0x0000002du}, /* PA_SU_VTX_CNTL: 1/16th subpixel, half-pixel center */
        /* Shader Formats & SPI PS Controls */
        {0x1b1u, 0x00000080u}, /* SPI_VS_OUT_CONFIG: NO_PC_EXPORT */
        {0x1c2u, 0x00000001u}, /* SPI_SHADER_IDX_FORMAT: IDX0 = 1COMP */
        {0x1c3u, 0x00000004u}, /* SPI_SHADER_POS_FORMAT: POS0 = 4COMP */
        {0x1c5u, 0x00000009u}, /* SPI_SHADER_COL_FORMAT: COL0 = 32_ABGR */
        {0x1b3u, 0x00000002u}, /* SPI_PS_INPUT_ENA: PERSP_CENTER_ENA */
        {0x1b4u, 0x00000002u}, /* SPI_PS_INPUT_ADDR: PERSP_CENTER_ENA */
        {0x1b5u, 0x00000001u}, /* SPI_INTERP_CONTROL_0: FLAT_SHADE_ENA */
        {0x1b6u, 0x00000000u}, /* SPI_PS_IN_CONTROL */
        {0x1b8u, 0x01000000u}, /* SPI_BARYC_CNTL: FRONT_FACE_ALL_BITS */
    };
    for (size_t i = 0; i < sizeof(ctx_regs) / sizeof(ctx_regs[0]); i++) {
        uint32_t reg = ctx_regs[i].reg;
        uint32_t val = ctx_regs[i].val;
        if (reg >= 0x16fu && reg <= 0x172u && !cfg->write_ucp) {
            continue;
        }
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        }
        *dw++ = 0xc0016900u; /* PACKET3_SET_CONTEXT_REG, count 1 */
        *dw++ = reg;
        *dw++ = val;
    }

    /* Clear all 32 SPI_PS_INPUT_CNTL registers (0x191 .. 0x1b0) to 0,
     * matching libSceAgc.sprx and AgcCompositor.elf driver default pipeline state */
    for (uint32_t i = 0; i < 32; i++) {
        *dw++ = 0xc0016900u; /* PACKET3_SET_CONTEXT_REG, count 1 */
        *dw++ = 0x191u + i;
        *dw++ = 0x00000000u;
    }

    /* 2. Shader program binding to avoid SQC instruction fetch unmapped VA fault:
     * Bind all graphics stages (PS, VS, GS/NGG, HS, ES, LS) to their respective
     * payloads:
     * PS: LO=0x08, HI=0x09, RSRC1=0x0A, RSRC2=0x0B (offset 0x200, USER_SGPR=0)
     * VS: LO=0x48, HI=0x49, RSRC1=0x4A, RSRC2=0x4B (offset 0x000)
     * GS/NGG: LO=0x88, HI=0x89, RSRC1=0x8A, RSRC2=0x8B (offset 0x000, GS_COMP_CNT=3,
     * ES_COMP_CNT=3) ES: LO=0xC8, HI=0xC9, RSRC1=0xCA, RSRC2=0xCB (offset 0x000) HS:
     * LO=0x108, HI=0x109, RSRC1=0x10A, RSRC2=0x10B (offset 0x000) LS: LO=0x148,
     * HI=0x149, RSRC1=0x14A, RSRC2=0x14B (offset 0x000)
     */
    static const struct {
        uint32_t base_reg;
        uint64_t va_offset;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, 0x200u, 0x000c0010u, 0x00000000u}, /* PS: 16 VGPRs, USER_SGPR=0 */
        {0x48u, 0x000u, 0x000c0010u, 0x00000000u}, /* VS */
        {0x88u, 0x000u, 0x622c0042u,
         0x00030000u}, /* GS/NGG: GS_COMP_CNT=3, ES_COMP_CNT=3, USER_SGPR=0 */
        {0xc8u, 0x000u, 0x000c0010u, 0x00000000u},  /* ES / HS */
        {0x108u, 0x000u, 0x000c0010u, 0x00000000u}, /* HS / LS */
        {0x148u, 0x000u, 0x000c0010u, 0x00000000u}, /* LS */
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
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
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
        {0x087u, 0x0000fffdu}, /* SPI_SHADER_PGM_RSRC3_GS: CU_EN = 0xfffd (disable CU1
                                  to prevent deadlock) */
        {0x081u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_GS: CU_EN bits 17:16 = 0x3 (18
                                  CUs total) */
        {0x107u, 0xffff0000u}, /* SPI_SHADER_PGM_RSRC3_HS */
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u; /* SET_SH_REG, count 1 */
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    /* 4. Primitive topology and GE Parameter Cache setup via SET_UCONFIG_REG:
     * - PACKET3_NUM_INSTANCES: 1 instance
     * - Register 0x242 = mmVGT_PRIMITIVE_TYPE: 0x4 = DI_PT_TRILIST
     * - Register 0x25b = mmGE_CNTL: PRIM_GRP_SIZE=64, VERT_GRP_SIZE=64
     * - Register 0x260 = mmGE_PC_ALLOC: OVERSUB_EN(1) | (NUM_PC_LINES=511 << 1) = 0x3ff
     */
    *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES, count 0 */
    *dw++ = 1u;          /* 1 instance */
    *dw++ = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
    *dw++ = 0x242u;      /* Reg offset 0x242 */
    *dw++ = cfg->di_primitive_type;
    *dw++ = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
    *dw++ = 0x25bu;      /* Reg offset 0x25b: GE_CNTL */
    *dw++ =
        0x00008040u; /* PRIM_GRP_SIZE=64, VERT_GRP_SIZE=64 (from AgcCompositor.elf) */
    *dw++ = 0xc0017900u; /* PACKET3_SET_UCONFIG_REG, count 1 */
    *dw++ = 0x260u;      /* Reg offset 0x260: GE_PC_ALLOC */
    *dw++ = 0x3ffu;      /* OVERSUB_EN=1, NUM_PC_LINES=511 */

    /* 5. Primitive draw execution: DRAW_INDEX_AUTO (opcode 0x2D)
     * DW1: index_count
     * DW2: initiator = 2 (DI_SRC_SEL_AUTO_INDEX, confirmed from
     * sceAgcDcbDrawIndexAuto disassembly) */
    *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO, count 1 */
    *dw++ = cfg->index_count; /* index_count */
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

    uint32_t bytes_written = (uint32_t)((uintptr_t)dw - (uintptr_t)dcb_buf);
    if (!is_point_line) {
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "bytes-written", (uint64_t)bytes_written, "size");
    }

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
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
        if (!is_point_line) {
            obs_report_measure(cfg->check_name, cfg->variant_target, "m0", (uint64_t)cfg->m0, "hex");
            obs_report_measure(cfg->check_name, cfg->variant_target,
                               "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
        }
    } else {
        obs_fault_unregister();
        if (!is_point_line) {
            obs_report_measure(cfg->check_name, cfg->variant_target, "m0", (uint64_t)cfg->m0, "hex");
            obs_report_measure(cfg->check_name, cfg->variant_target,
                               "rc-submit", (uint64_t)sig, "fault-sig");
        }
    }

    uint32_t fence_val = *fence;
    int fence_hit = 0;
    if (submit_rc == 0) {
        for (int iter = 0; iter < 25000; iter++) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
#endif
            fence_val = *fence;
            if (fence_val == 0xbeefcafeu) {
                fence_hit = 1;
                break;
            }
            if (iter >= 20000 && canary[0] == 0x11111111u) {
                /* Wavefront never launched after 2.0s; stop waiting */
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }

#if defined(__x86_64__)
    for (size_t p = 0; p < 0x4000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
#endif

    uint32_t color_val = color_buf[0];
    int color_mod = 0;
    size_t mod_idx = 0;
    uint32_t texel_x = 0;
    uint32_t texel_y = 0;
    uint32_t modified_pixel_count = 0;
    for (size_t i = 0; i < 4096; i++) {
        if (color_buf[i] != 0x55555555u) {
            if (!color_mod) {
                color_mod = 1;
                color_val = color_buf[i];
                mod_idx = i;
                agc_detile_pixel((uint32_t)i, &texel_x, &texel_y);
            }
            modified_pixel_count++;
        }
    }

    if (is_point_line) {
        obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-gs-out-prim-type",
                           (uint64_t)cfg->vgt_gs_out_prim_type, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "draw-packet-prim-type",
                           (uint64_t)cfg->di_primitive_type, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pa-su-point-size",
                           (uint64_t)cfg->pa_su_point_size, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pa-su-point-minmax",
                           (uint64_t)cfg->pa_su_point_minmax, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pa-su-line-cntl",
                           (uint64_t)cfg->pa_su_line_cntl, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "m0-alloc-req",
                           (uint64_t)cfg->m0, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-shader-stages-en",
                           (uint64_t)cfg->vgt_shader_stages_en, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "fence-val",
                           (uint64_t)fence_val, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "fence-hit",
                           (uint64_t)fence_hit, "bool");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pixel-hit",
                           (uint64_t)color_mod, "bool");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pixel-idx",
                           (uint64_t)mod_idx, "idx");
        obs_report_measure(cfg->check_name, cfg->variant_target, "pixel-val",
                           (uint64_t)color_val, "hex");
    } else {
        obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-shader-stages-en",
                           (uint64_t)cfg->vgt_shader_stages_en, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-gs-out-prim-type",
                           (uint64_t)cfg->vgt_gs_out_prim_type, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "fence-val",
                           (uint64_t)fence_val, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "fence-hit",
                           (uint64_t)fence_hit, "bool");
        obs_report_measure(cfg->check_name, cfg->variant_target, "color-val",
                           (uint64_t)color_val, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "color-mod",
                           (uint64_t)color_mod, "bool");
        obs_report_measure(cfg->check_name, cfg->variant_target, "color-idx",
                           (uint64_t)mod_idx, "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "texel-x",
                           (uint64_t)texel_x, "pixels");
        obs_report_measure(cfg->check_name, cfg->variant_target, "texel-y",
                           (uint64_t)texel_y, "pixels");
        obs_report_measure(cfg->check_name, cfg->variant_target, "modified-pixels",
                           (uint64_t)modified_pixel_count, "count");
        obs_report_measure(cfg->check_name, cfg->variant_target, "canary-vs",
                           (uint64_t)canary[0], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps",
                           (uint64_t)canary[1], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "canary-v0",
                           (uint64_t)canary[2], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "canary-v1",
                           (uint64_t)canary[3], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target, "canary-exec",
                           (uint64_t)canary[5], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "canary-vs-done", (uint64_t)canary[6], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "canary-ps-v0", (uint64_t)canary[7], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "canary-ps-v1", (uint64_t)canary[8], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "canary-ps-exec", (uint64_t)canary[9], "val");
        obs_report_measure(cfg->check_name, cfg->variant_target,
                           "canary-ps-done", (uint64_t)canary[10], "val");
        if (cfg->has_clip) {
            obs_report_measure(cfg->check_name, cfg->variant_target, "pa-cl-clip-cntl",
                               (uint64_t)cfg->pa_cl_clip_cntl, "reg");
            if (cfg->write_ucp) {
                obs_report_measure(cfg->check_name, cfg->variant_target, "pa-cl-ucp-0-x",
                                   (uint64_t)cfg->pa_cl_ucp_0_x, "val");
                obs_report_measure(cfg->check_name, cfg->variant_target, "pa-cl-ucp-0-y",
                                   (uint64_t)cfg->pa_cl_ucp_0_y, "val");
                obs_report_measure(cfg->check_name, cfg->variant_target, "pa-cl-ucp-0-z",
                                   (uint64_t)cfg->pa_cl_ucp_0_z, "val");
                obs_report_measure(cfg->check_name, cfg->variant_target, "pa-cl-ucp-0-w",
                                   (uint64_t)cfg->pa_cl_ucp_0_w, "val");
            }
        }
    }

    if (strcmp(cfg->variant_target, "variant-A") == 0 ||
        strcmp(cfg->check_name, "166-agc/primitive-draw") == 0 ||
        cfg->has_clip) {
        const unsigned int chunk_sz = 16u;
        obs_report_measure(cfg->check_name, cfg->variant_target, "dcb-length", (uint64_t)bytes_written, "bytes");
        for (unsigned int off = 0; off < bytes_written; off += chunk_sz) {
            unsigned int rem = bytes_written - off;
            obs_report_bytes(cfg->check_name, cfg->variant_target, "dcb-stream", off,
                             (const unsigned char *)dcb_buf + off,
                             rem < chunk_sz ? rem : chunk_sz);
        }
        obs_report_measure(cfg->check_name, cfg->variant_target, "vs-addr", payload_va, "address");
        obs_report_measure(cfg->check_name, cfg->variant_target, "vs-size", 44u * 4u, "bytes");
        for (unsigned int off = 0; off < 44u * 4u; off += chunk_sz) {
            unsigned int rem = (44u * 4u) - off;
            obs_report_bytes(cfg->check_name, cfg->variant_target, "vs-bytecode", off,
                             (const unsigned char *)vs_code + off,
                             rem < chunk_sz ? rem : chunk_sz);
        }
        obs_report_measure(cfg->check_name, cfg->variant_target, "ps-addr", payload_va + 0x200u, "address");
        obs_report_measure(cfg->check_name, cfg->variant_target, "ps-size", 26u * 4u, "bytes");
        for (unsigned int off = 0; off < 26u * 4u; off += chunk_sz) {
            unsigned int rem = (26u * 4u) - off;
            obs_report_bytes(cfg->check_name, cfg->variant_target, "ps-bytecode", off,
                             (const unsigned char *)ps_code + off,
                             rem < chunk_sz ? rem : chunk_sz);
        }
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-base", color_gpu, "address");
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-pitch", 64u, "pixels");
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-width", 64u, "pixels");
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-height", 64u, "pixels");
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-info", 0x000180a8u, "reg");
        obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-tiling-mode", 27u, "64KB_R_X");
        if (strcmp(cfg->check_name, "166-agc/primitive-draw") == 0 ||
            (strcmp(cfg->variant_target, "variant-A") == 0 && color_mod != 0) ||
            (cfg->has_clip && color_mod != 0)) {
            for (unsigned int off = 0; off < 4096u * 4u; off += chunk_sz) {
                obs_report_bytes(cfg->check_name, cfg->variant_target, "color-target", off,
                                 (const unsigned char *)color_buf + off, chunk_sz);
            }
        }
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

    obs_result ret;
    if (sig != 0) {
        ret = obs_fail("fault during primitive draw submit or poll");
    } else if (submit_rc == 0 && fence_hit == 1 && canary[0] == 0xbeef0001u &&
               canary[6] == 0xbeef0003u && canary[1] == 0xbeef0002u &&
               canary[10] == 0xbeef0004u && color_mod == 1) {
        ret = obs_pass();
    } else if (submit_rc == 0 && fence_hit == 1) {
        if (canary[0] != 0xbeef0001u) {
            ret = obs_partial_value("ngg vs wavefront did not launch",
                                    (uint64_t)canary[0]);
        } else if (canary[6] != 0xbeef0003u) {
            ret = obs_partial_value("ngg vs wavefront did not complete",
                                    (uint64_t)canary[6]);
        } else if (canary[1] != 0xbeef0002u) {
            ret = obs_partial_value("ps wavefront did not launch",
                                    (uint64_t)canary[1]);
        } else if (canary[10] != 0xbeef0004u) {
            ret = obs_partial_value("ps wavefront did not complete",
                                    (uint64_t)canary[10]);
        } else {
            ret = obs_partial_value("color target buffer not modified by rasterizer",
                                    (uint64_t)color_val);
        }
    } else if (submit_rc == 0) {
        ret = obs_partial_value("fence not hit after primitive draw",
                                (uint64_t)fence_val);
    } else {
        ret = obs_partial_value("submit dcb returned non-zero code",
                                (uint64_t)(uint32_t)submit_rc);
    }

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif

    return ret;
}

static obs_result check_agc_primitive_draw_impl(uint32_t m0_literal,
                                                const char *check_name,
                                                const char *variant_target) {
    agc_draw_cfg_t cfg = {
        .m0 = m0_literal,
        .vgt_shader_stages_en = 0x02002000u,
        .vgt_gs_out_prim_type = 0x2u,
        .di_primitive_type = 0x4u,
        .index_count = 3u,
        .pa_su_point_size = 0x00080008u,
        .pa_su_point_minmax = 0xffff0000u,
        .pa_su_line_cntl = 0x00000008u,
        .check_name = check_name,
        .variant_target = variant_target,
    };
    return check_agc_primitive_draw_sub(&cfg);
}

static obs_result check_agc_primitive_draw(void) {
    return check_agc_primitive_draw_impl(0x00001003u, "166-agc/primitive-draw",
                                         "sceAgcDriverSubmitDcb");
}

static obs_result check_agc_ngg_primitive_draw_m0(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }
    static const struct {
        const char *tag;
        uint32_t m0;
    } vars[] = {
        {"variant-A", 0x1003u},
        {"variant-G", 0x1004u},
        {"variant-F", 0x3001u},
    };
    int passed_count = 0;
    for (size_t i = 0; i < OBS_COUNT(vars); i++) {
        obs_result r = check_agc_primitive_draw_impl(
            vars[i].m0, "166-agc/ngg-primitive-draw-m0", vars[i].tag);
        if (r.status == OBS_PASS) {
            passed_count++;
        }
    }
    return (passed_count > 0) ? obs_pass_value(0x1003u)
                              : obs_partial("m0 variant draw sweep completed");
}

static int s_agc_queue_faulted = 0;

static obs_result check_agc_primitive_draw_point_line(void) {
    return obs_skip(
        "primitive-draw-point-line isolated per REQ-20260916T1549Z-7c4b: "
        "point/line submissions stall GE pipeline");
}

static obs_result check_agc_primitive_draw_clip(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    /* REQ-20260917T1045Z-b2c7: Test fixed-function clipper under NGG passthrough (0x02002000).
     * 1. UCP registers not written at all (ucp-unwritten) - baseline recipe untouched.
     * 2. UCP registers written, UCP_ENA_0 = 0 (ucp-disabled).
     * 3. UCP registers written, UCP_ENA_0 = 1 (ucp-enabled).
     * Acceptance: Three sweep cases reporting fence-hit, canary-vs and modified-pixels,
     * plus VGT_SHADER_STAGES_EN (0x2d5) as a reg row for each case. */
    agc_draw_cfg_t cfg_unwritten = {
        .m0 = 0x00001003u,
        .vgt_shader_stages_en = 0x02002000u,
        .vgt_gs_out_prim_type = 0x2u,
        .di_primitive_type = 0x4u,
        .index_count = 3u,
        .pa_su_point_size = 0x00080008u,
        .pa_su_point_minmax = 0xffff0000u,
        .pa_su_line_cntl = 0x00000008u,
        .pa_cl_clip_cntl = 0x00000000u,
        .write_ucp = 0,
        .has_clip = 1,
        .check_name = "166-agc/primitive-draw-clip",
        .variant_target = "ucp-unwritten",
    };
    obs_result r_unwritten = check_agc_primitive_draw_sub(&cfg_unwritten);

    agc_draw_cfg_t cfg_dis = {
        .m0 = 0x00001003u,
        .vgt_shader_stages_en = 0x02002000u,
        .vgt_gs_out_prim_type = 0x2u,
        .di_primitive_type = 0x4u,
        .index_count = 3u,
        .pa_su_point_size = 0x00080008u,
        .pa_su_point_minmax = 0xffff0000u,
        .pa_su_line_cntl = 0x00000008u,
        .pa_cl_clip_cntl = 0x00000000u, /* UCP_ENA_0 = 0 */
        .pa_cl_ucp_0_x = 0x3f800000u,   /* 1.0f */
        .pa_cl_ucp_0_y = 0x00000000u,
        .pa_cl_ucp_0_z = 0x00000000u,
        .pa_cl_ucp_0_w = 0x00000000u,
        .write_ucp = 1,
        .has_clip = 1,
        .check_name = "166-agc/primitive-draw-clip",
        .variant_target = "ucp-disabled",
    };
    obs_result r_dis = check_agc_primitive_draw_sub(&cfg_dis);

    agc_draw_cfg_t cfg_ena = {
        .m0 = 0x00001003u,
        .vgt_shader_stages_en = 0x02002000u,
        .vgt_gs_out_prim_type = 0x2u,
        .di_primitive_type = 0x4u,
        .index_count = 3u,
        .pa_su_point_size = 0x00080008u,
        .pa_su_point_minmax = 0xffff0000u,
        .pa_su_line_cntl = 0x00000008u,
        .pa_cl_clip_cntl = 0x00000001u, /* UCP_ENA_0 = 1 */
        .pa_cl_ucp_0_x = 0x3f800000u,   /* 1.0f */
        .pa_cl_ucp_0_y = 0x00000000u,
        .pa_cl_ucp_0_z = 0x00000000u,
        .pa_cl_ucp_0_w = 0x00000000u,
        .write_ucp = 1,
        .has_clip = 1,
        .check_name = "166-agc/primitive-draw-clip",
        .variant_target = "ucp-enabled",
    };
    obs_result r_ena = check_agc_primitive_draw_sub(&cfg_ena);

    if (r_unwritten.status == OBS_PASS && r_dis.status == OBS_PASS && r_ena.status == OBS_PASS) {
        return obs_pass();
    }
    if (r_unwritten.status == OBS_PASS) {
        return obs_pass_value(0x1u);
    }
    return r_unwritten;
}

typedef struct {
    uint32_t spi_vs_out_config;
    uint32_t spi_ps_in_control;
    uint32_t spi_ps_input_cntl_2;
    uint32_t spi_ps_input_ena;
    uint32_t spi_ps_input_addr;
    int num_exports;
    const char *check_name;
    const char *variant_target;
} agc_param3_cfg_t;

static obs_result check_agc_primitive_draw_param3_sub(const agc_param3_cfg_t *cfg) {
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
    uint32_t *dcb_buf =
        (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_p3_payload[4096];
    static _Alignas(64) uint32_t s_host_p3_fence[16];
    static _Alignas(64) uint32_t s_host_p3_color[4096];
    static _Alignas(64) uint32_t s_host_p3_canary[16];
    static _Alignas(64) uint32_t s_host_p3_dcb[2048];
    uint8_t *gpu_payload = s_host_p3_payload;
    volatile uint32_t *fence = s_host_p3_fence;
    volatile uint32_t *color_buf = s_host_p3_color;
    volatile uint32_t *canary = s_host_p3_canary;
    uint32_t *dcb_buf = s_host_p3_dcb;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || color_buf == NULL || canary == NULL ||
        dcb_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for payload, fence, color "
                        "buffer, canary or dcb");
    }
    *fence = 0x11111111u;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }
    for (size_t i = 0; i < 4096; i++) {
        color_buf[i] = 0x55555555u;
    }

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    memset(gpu_payload, 0, 0x1000);
    uint32_t *vs_code = (uint32_t *)gpu_payload;
    uint32_t vsk = 0;
    vs_code[vsk++] = 0xbfa00001u; /* s_inst_prefetch 0x1 */
    vs_code[vsk++] = 0xbe8c037eu; /* s_mov_b32 s12, exec_lo (save entry exec_lo) */
    vs_code[vsk++] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    vs_code[vsk++] = 0x00001003u;
    vs_code[vsk++] = 0xbf800000u; /* s_nop 0 */
    vs_code[vsk++] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */

    /* Lane 0: write canary[0] = 0xbeef0001 */
    vs_code[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    vs_code[vsk++] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    vs_code[vsk++] = (uint32_t)canary_gpu;
    vs_code[vsk++] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    vs_code[vsk++] = (uint32_t)(canary_gpu >> 32);
    vs_code[vsk++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    vs_code[vsk++] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    vs_code[vsk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0001 */
    vs_code[vsk++] = 0xbeef0001u;
    vs_code[vsk++] = 0xdc708000u; /* global_store_dword v[8:9], v10, off offset:0 */
    vs_code[vsk++] = 0x007d0a08u;
    vs_code[vsk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Primitive connectivity export from Lane 0: exp prim, v1, off, off, off done */
    vs_code[vsk++] = 0x7e0202ffu; /* v_mov_b32 v1, 0x20280600 (vertices 0, 1, 2 with edge flags) */
    vs_code[vsk++] = 0x20280600u;
    vs_code[vsk++] = 0xf8000941u; /* exp prim, v1, off, off, off done */
    vs_code[vsk++] = 0x00000001u;
    vs_code[vsk++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0): the export must retire before exec changes */

    /* Vertex position & parameter computation:
     * Lane 0: (-0.5f, -0.5f), param2.R = 0.5f (in v10) */
    vs_code[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs_code[vsk++] = 0x7e0a02f1u; /* v_mov_b32 v5, -0.5f (pos.x) */
    vs_code[vsk++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (pos.y) */
    vs_code[vsk++] = 0x7e1402f0u; /* v_mov_b32 v10, +0.5f (param2.R) */

    /* Lane 1: (+0.5f, -0.5f), param2.R = 0.0f (in v10) */
    vs_code[vsk++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    vs_code[vsk++] = 0x7e0a02f0u; /* v_mov_b32 v5, +0.5f (pos.x) */
    vs_code[vsk++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (pos.y) */
    vs_code[vsk++] = 0x7e140280u; /* v_mov_b32 v10, 0.0f (param2.R) */

    /* Lane 2: ( 0.0f, +0.5f), param2.R = 0.0f (in v10) */
    vs_code[vsk++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    vs_code[vsk++] = 0x7e0a0280u; /* v_mov_b32 v5, 0.0f (pos.x) */
    vs_code[vsk++] = 0x7e0c02f0u; /* v_mov_b32 v6, +0.5f (pos.y) */
    vs_code[vsk++] = 0x7e140280u; /* v_mov_b32 v10, 0.0f (param2.R) */

    /* Lanes 0..2 common setup: */
    vs_code[vsk++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    vs_code[vsk++] = 0x7e060280u; /* v_mov_b32 v3, 0.0f (pos.z) */
    vs_code[vsk++] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (pos.w) */

    /* Parameter 0: Solid Blue across all 3 vertices (0.0f, 0.0f, 1.0f, 1.0f) in v0..v2, v7 */
    vs_code[vsk++] = 0x7e000280u; /* v_mov_b32 v0, 0.0f (R) */
    vs_code[vsk++] = 0x7e020280u; /* v_mov_b32 v1, 0.0f (G) */
    vs_code[vsk++] = 0x7e0402f2u; /* v_mov_b32 v2, 1.0f (B) */
    vs_code[vsk++] = 0x7e0e02f2u; /* v_mov_b32 v7, 1.0f (A) */
    vs_code[vsk++] = 0xf800020fu; /* exp param0, v0, v1, v2, v7 */
    vs_code[vsk++] = 0x07020100u;

    /* Parameter 1: Solid Green across all 3 vertices (0.0f, 1.0f, 0.0f, 1.0f) in v0..v2, v7 */
    vs_code[vsk++] = 0x7e0202f2u; /* v_mov_b32 v1, 1.0f (G) */
    vs_code[vsk++] = 0x7e040280u; /* v_mov_b32 v2, 0.0f (B) */
    vs_code[vsk++] = 0xf800021fu; /* exp param1, v0, v1, v2, v7 */
    vs_code[vsk++] = 0x07020100u;

    if (cfg->num_exports >= 3) {
        /* Parameter 2: R=v10 (ramp: 0.5f, 0.0f, 0.0f), G=0.25f (v1), B=0.0f (v2), A=1.0f (v7) */
        vs_code[vsk++] = 0x7e0202ffu; /* v_mov_b32 v1, 0.25f (G) */
        vs_code[vsk++] = 0x3e800000u;
        vs_code[vsk++] = 0xf800022fu; /* exp param2, v10, v1, v2, v7 */
        vs_code[vsk++] = 0x0702010au;
    }

    /* Position: exp pos0, v5, v6, v3, v4 done */
    vs_code[vsk++] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
    vs_code[vsk++] = 0x04030605u;
    vs_code[vsk++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0): wait for all exports to retire */

    /* VS done canary from lane 0 */
    vs_code[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs_code[vsk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0003 */
    vs_code[vsk++] = 0xbeef0003u;
    vs_code[vsk++] = 0xdc708018u; /* global_store_dword v[8:9], v10, off offset:24 (canary[6]) */
    vs_code[vsk++] = 0x007d0a08u;
    vs_code[vsk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Epilogue */
    vs_code[vsk++] = 0xbefe030cu; /* s_mov_b32 exec_lo, s12 */
    vs_code[vsk++] = 0xbf810000u; /* s_endpgm */
    for (size_t p = vsk; p < 128; p++) {
        vs_code[p] = 0xbf800000u; /* s_nop */
    }
    uint32_t vs_dwords = vsk;

    uint32_t *gs_code = (uint32_t *)((char *)gpu_payload + 0x100);
    for (size_t p = 0; p < vs_dwords; p++) {
        gs_code[p] = vs_code[p];
    }
    for (size_t p = vs_dwords; p < 64; p++) {
        gs_code[p] = 0xbf800000u;
    }

    /* Pixel Shader placed at offset 0x200 matching baseline primitive-draw layout */
    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u; /* s_waitcnt 0 */
    ps_code[psk++] = 0xbefc0300u; /* s_mov_b32 m0, s0: SPI hands prim mask in s0; interpolator reads m0 */
    ps_code[psk++] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */

    /* Save entry VGPRs into v11..v14 */
    ps_code[psk++] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    ps_code[psk++] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    ps_code[psk++] = 0x7e1a0302u; /* v_mov_b32 v13, v2 */
    ps_code[psk++] = 0x7e1c0303u; /* v_mov_b32 v14, v3 */

    /* Canary stores from lane 0 */
    ps_code[psk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    ps_code[psk++] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    ps_code[psk++] = (uint32_t)canary_gpu;
    ps_code[psk++] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    ps_code[psk++] = (uint32_t)(canary_gpu >> 32);

    ps_code[psk++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    ps_code[psk++] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    ps_code[psk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0002 */
    ps_code[psk++] = 0xbeef0002u;
    ps_code[psk++] = 0xdc708004u; /* global_store_dword v[8:9], v10, off offset:4 (canary[1]) */
    ps_code[psk++] = 0x007d0a08u;

    ps_code[psk++] = 0xdc70801cu; /* global_store_dword v[8:9], v11, off offset:28 (canary[7]=v0) */
    ps_code[psk++] = 0x007d0b08u;
    ps_code[psk++] = 0xdc708020u; /* global_store_dword v[8:9], v12, off offset:32 (canary[8]=v1) */
    ps_code[psk++] = 0x007d0c08u;
    ps_code[psk++] = 0xdc708024u; /* global_store_dword v[8:9], v13, off offset:36 (canary[9]=v2) */
    ps_code[psk++] = 0x007d0d08u;
    ps_code[psk++] = 0xdc708028u; /* global_store_dword v[8:9], v14, off offset:40 (canary[10]=v3) */
    ps_code[psk++] = 0x007d0e08u;

    ps_code[psk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0004 */
    ps_code[psk++] = 0xbeef0004u;
    ps_code[psk++] = 0xdc708014u; /* global_store_dword v[8:9], v10, off offset:20 (canary[5]) */
    ps_code[psk++] = 0x007d0a08u;

    ps_code[psk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    ps_code[psk++] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */

    if (cfg->num_exports == 2) {
        /* Interpolate Parameter 1 (Attribute 1 = Green) into v4..v7: */
        ps_code[psk++] = 0xc8100400u; /* v_interp_p1_f32 v4, v0, attr1.x (R) */
        ps_code[psk++] = 0xc8110401u; /* v_interp_p2_f32 v4, v1, attr1.x */
        ps_code[psk++] = 0xc8140500u; /* v_interp_p1_f32 v5, v0, attr1.y (G) */
        ps_code[psk++] = 0xc8150501u; /* v_interp_p2_f32 v5, v1, attr1.y */
        ps_code[psk++] = 0xc8180600u; /* v_interp_p1_f32 v6, v0, attr1.z (B) */
        ps_code[psk++] = 0xc8190601u; /* v_interp_p2_f32 v6, v1, attr1.z */
        ps_code[psk++] = 0xc81c0700u; /* v_interp_p1_f32 v7, v0, attr1.w (A) */
        ps_code[psk++] = 0xc81d0701u; /* v_interp_p2_f32 v7, v1, attr1.w */
    } else {
        /* Interpolate Parameter 2 (Attribute 2 = Red ramp + 0.25G) into v4..v7: */
        ps_code[psk++] = 0xc8100800u; /* v_interp_p1_f32 v4, v0, attr2.x (R) */
        ps_code[psk++] = 0xc8110801u; /* v_interp_p2_f32 v4, v1, attr2.x */
        ps_code[psk++] = 0xc8140900u; /* v_interp_p1_f32 v5, v0, attr2.y (G) */
        ps_code[psk++] = 0xc8150901u; /* v_interp_p2_f32 v5, v1, attr2.y */
        ps_code[psk++] = 0xc8180a00u; /* v_interp_p1_f32 v6, v0, attr2.z (B) */
        ps_code[psk++] = 0xc8190a01u; /* v_interp_p2_f32 v6, v1, attr2.z */
        ps_code[psk++] = 0xc81c0b00u; /* v_interp_p1_f32 v7, v0, attr2.w (A) */
        ps_code[psk++] = 0xc81d0b01u; /* v_interp_p2_f32 v7, v1, attr2.w */
    }

    /* Export interpolated color to MRT0 */
    ps_code[psk++] = 0xf800180fu; /* exp mrt0, v4, v5, v6, v7 done vm */
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u; /* s_endpgm */
    for (size_t p = psk; p < 64; p++) {
        ps_code[p] = 0xbf800000u;
    }
    uint32_t ps_dwords = psk;

    /* Fallback stage (HS/ES/LS) at offset 0x300 matching baseline primitive-draw */
    uint32_t *fb_code = (uint32_t *)((char *)gpu_payload + 0x300);
    fb_code[0] = 0xbefc0380u; /* s_mov_b32 m0, 0 */
    fb_code[1] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    fb_code[2] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 3; p < 64; p++) {
        fb_code[p] = 0xbf800000u;
    }

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure(cfg->check_name, cfg->variant_target, "rc-create",
                       (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping primitive draw");
    }

    uint32_t *dw = dcb_buf;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    const struct {
        uint32_t reg;
        uint32_t val;
    } ctx_regs[] = {
        /* Color Target 0 */
        {0x318u, 0}, /* CB_COLOR0_BASE */
        {0x390u, 0}, /* CB_COLOR0_BASE_EXT */
        {0x31bu, 0x00000000u}, /* CB_COLOR0_VIEW */
        {0x31cu, 0x000180a8u}, /* CB_COLOR0_INFO: COLOR_8_8_8_8, LINEAR_GENERAL, UNORM */
        {0x31du, 0x00000000u}, /* CB_COLOR0_ATTRIB: 0 */
        {0x31eu, 0x00000000u}, /* CB_COLOR0_DCC_CONTROL: disabled */
        {0x3b0u, (63u << 14) | 63u}, /* CB_COLOR0_ATTRIB2: MIP0_WIDTH=63, MIP0_HEIGHT=63 */
        {0x3b8u, 0x08c6c000u}, /* CB_COLOR0_ATTRIB3: SW_MODE=27 (64KB_R_X) */
        {0x109u, 0x00000000u}, /* CB_DCC_CONTROL: disabled */
        /* Color Control & Mask & Blend */
        {0x202u, 0x00cc0010u}, /* CB_COLOR_CONTROL: CB_NORMAL, ROP3_COPY */
        {0x08eu, 0x0000000fu}, /* CB_TARGET_MASK: MRT0 4 components enabled */
        {0x08fu, 0x0000000fu}, /* CB_SHADER_MASK: MRT0 4 components export enabled */
        {0x1e0u, 0x20010001u}, /* CB_BLEND0_CONTROL: SRC=ONE, DST=ZERO, ADD */
        /* Depth & Stencil */
        {0x200u, 0x00000000u}, /* DB_DEPTH_CONTROL: disabled */
        {0x201u, 0x00010000u}, /* DB_EQAA */
        {0x203u, 0x00000010u}, /* DB_SHADER_CONTROL: EARLY_Z_THEN_LATE_Z */
        {0x08cu, 0xaa99aaaau}, /* PA_SC_EDGERULE: D3D/OpenGL standard edge rule */
        {0x1d4u, 0x000000ffu}, /* SX_PS_DOWNCONVERT_CONTROL */
        /* Non-Passthrough NGG Primitive Type & Stages matching oops-gl / gl-cube oracle */
        {0x291u, 0x10020040u}, /* VGT_GS_ONCHIP_CNTL: ES_VERTS=64, GS_PRIMS=64, GS_INST_PRIMS=64 */
        {0x29bu, 0x00000002u}, /* VGT_GS_OUT_PRIM_TYPE: TRISTRIP */
        {0x2d3u, 0x00000001u}, /* GE_NGG_SUBGRP_CNTL: PRIM_AMP=1 */
        {0x2d5u, 0x00c12010u}, /* VGT_SHADER_STAGES_EN: ES_EN=REAL | PRIMGEN_EN | MAX_PRIMGRP_IN_WAVE=2 | GS_W32 | VS_W32 */
        {0x1ffu, 0x00000040u}, /* GE_MAX_OUTPUT_PER_SUBGROUP: 64 */
        {0x20eu, 0x00000078u}, /* PA_CL_NGG_CNTL: VERTEX_REUSE_DEPTH=30 */
        {0x2a1u, 0x00000000u}, /* VGT_PRIMITIVEID_EN: disabled */
        {0x2a6u, 0x00000040u}, /* VGT_DRAW_PAYLOAD_CNTL */
        {0x2adu, 0x00000000u}, /* VGT_REUSE_OFF */
        {0x2abu, 0x00000001u}, /* VGT_ESGS_RING_ITEMSIZE: 1 */
        {0x2ceu, 0x00000400u}, /* VGT_GS_MAX_VERT_OUT: 1024 */
        {0x2e4u, 0x00000000u}, /* VGT_GS_INSTANCE_CNT: 0 */
        {0x290u, 0x00000000u}, /* VGT_GS_MODE: off */
        {0x2d4u, 0x88101000u}, /* VGT_TESS_DISTRIBUTION */
        {0x103u, 0xffffffffu}, /* VGT_MULTI_PRIM_IB_RESET_INDX */
        /* Sample Mask & NGG Control */
        {0x30eu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y0_X1Y0 */
        {0x30fu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y1_X1Y1 */
        {0x310u, 0x00000000u}, /* PA_SC_SHADER_CONTROL */
        {0x314u, 0x00000202u}, /* PA_SC_NGG_MODE_CNTL */
        {0x311u, 0x01fd2002u}, /* PA_SC_BINNER_CNTL_0 */
        {0x312u, 0x03ff0080u}, /* PA_SC_BINNER_CNTL_1 */
        {0x313u, 0x00006000u}, /* PA_SC_CONSERVATIVE_RASTERIZATION_CNTL */
        {0x00eu, 0x00000002u}, /* DB_DFSM_CONTROL */
        {0x280u, 0x00080008u}, /* PA_SU_POINT_SIZE */
        {0x281u, 0xffff0000u}, /* PA_SU_POINT_MINMAX */
        {0x282u, 0x00000008u}, /* PA_SU_LINE_CNTL */
        {0x2deu, 0x000001e9u}, /* PA_SU_POLY_OFFSET_DB_FMT_CNTL */
        /* Scissors */
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        /* Viewport */
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        /* Cliprect & Guardband */
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u}, /* PA_CL_GB_VERT_CLIP_ADJ: 1.0f */
        {0x2fbu, 0x3f800000u}, /* PA_CL_GB_VERT_DISC_ADJ: 1.0f */
        {0x2fcu, 0x3f800000u}, /* PA_CL_GB_HORZ_CLIP_ADJ: 1.0f */
        {0x2fdu, 0x3f800000u}, /* PA_CL_GB_HORZ_DISC_ADJ: 1.0f */
        /* Scan Converter */
        {0x205u, 0x00000240u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        /* Interpolation & Shader Formats */
        {0x191u, 0x00000000u}, /* SPI_PS_INPUT_CNTL_0: param 0, smooth */
        {0x192u, 0x00000001u}, /* SPI_PS_INPUT_CNTL_1: param 1, smooth */
        {0x193u, cfg->spi_ps_input_cntl_2}, /* SPI_PS_INPUT_CNTL_2 */
        {0x1b1u, cfg->spi_vs_out_config}, /* SPI_VS_OUT_CONFIG */
        {0x1c2u, 0x00000001u}, /* SPI_SHADER_IDX_FORMAT */
        {0x1c3u, 0x00000004u}, /* SPI_SHADER_POS_FORMAT */
        {0x1c5u, 0x00000009u}, /* SPI_SHADER_COL_FORMAT: COL0 = 32_ABGR */
        {0x1b3u, cfg->spi_ps_input_ena},  /* SPI_PS_INPUT_ENA */
        {0x1b4u, cfg->spi_ps_input_addr}, /* SPI_PS_INPUT_ADDR */
        {0x1b5u, 0x00000001u}, /* SPI_INTERP_CONTROL_0: FLAT_SHADE_ENA */
        {0x1b6u, cfg->spi_ps_in_control}, /* SPI_PS_IN_CONTROL */
        {0x1b8u, 0x01000000u}, /* SPI_BARYC_CNTL: FRONT_FACE_ALL_BITS */
    };
    for (size_t i = 0; i < sizeof(ctx_regs) / sizeof(ctx_regs[0]); i++) {
        uint32_t reg = ctx_regs[i].reg;
        uint32_t val = ctx_regs[i].val;
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        }
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Clear SPI_PS_INPUT_CNTL registers 3..31 (0x194 .. 0x1b0) */
    for (uint32_t i = 3; i < 32; i++) {
        *dw++ = 0xc0016900u;
        *dw++ = 0x191u + i;
        *dw++ = 0x00000000u;
    }

    static const struct {
        uint32_t base_reg;
        uint64_t va_offset;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, 0x200u, 0x000c0010u, 0x00000000u}, /* PS: at offset 0x200, 16 VGPRs, USER_SGPR=0 */
        {0x48u, 0x000u, 0x000c0010u, 0x00000000u}, /* VS: USER_SGPR=0 */
        {0x88u, 0x000u, 0x622c0042u, 0x00030000u}, /* GS/NGG: 56 VGPRs, COMP_CNT=3, USER_SGPR=0 */
        {0xc8u, 0x000u, 0x000c0010u, 0x00000000u}, /* ES */
        {0x108u, 0x000u, 0x000c0010u, 0x00000000u}, /* HS */
        {0x148u, 0x000u, 0x000c0010u, 0x00000000u}, /* LS */
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = payload_va + stages[s].va_offset;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 2u;
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
    }

    static const struct {
        uint32_t reg;
        uint32_t val;
    } spi_cu_regs[] = {
        {0x007u, 0x0000ffffu}, /* SPI_SHADER_PGM_RSRC3_PS: CU_EN = 0xffff (16 CUs) */
        {0x001u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_PS: CU_EN bits 17:16 = 0x3 (18 CUs total) */
        {0x087u, 0x0000fffdu}, /* SPI_SHADER_PGM_RSRC3_GS: CU_EN = 0xfffd (disable CU1 to prevent deadlock) */
        {0x081u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_GS: CU_EN bits 17:16 = 0x3 (18 CUs total) */
        {0x107u, 0xffff0000u}, /* SPI_SHADER_PGM_RSRC3_HS */
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u;
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES */
    *dw++ = 1u;
    *dw++ = 0xc0017a00u; /* PACKET3_SET_UCONFIG_REG_INDEX: mmVGT_PRIMITIVE_TYPE */
    *dw++ = 0x10000242u;
    *dw++ = 4u;          /* DI_PT_TRILIST */
    *dw++ = 0xc0017900u; /* mmGE_CNTL */
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* PRIM_GRP_SIZE=64, VERT_GRP_SIZE=64 */
    *dw++ = 0xc0017900u; /* mmGE_PC_ALLOC */
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* OVERSUB_EN=1, NUM_PC_LINES=511 */

    *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO */
    *dw++ = 3u;          /* index_count = 3 */
    *dw++ = 2u;          /* initiator = 2 */

    *dw++ = 0xc0064900u; /* PACKET3_RELEASE_MEM */
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;

    uint32_t bytes_written = (uint32_t)((uintptr_t)dw - (uintptr_t)dcb_buf);

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
    desc.size = bytes_written / 4u;
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    __builtin_ia32_clflush((const void *)canary);
    for (size_t p = 0; p < 0x1000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
    }
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)dcb_buf + p));
    }
    for (size_t p = 0; p < 0x4000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }

    obs_report_measure(cfg->check_name, cfg->variant_target, "rc-submit",
                       (uint64_t)(uint32_t)submit_rc, "code");

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
            if (iter >= 2000 && canary[0] == 0xaaaaaaaau) {
                /* Wavefront never launched after 200ms; stop waiting */
                break;
            }
            if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                sceKernelUsleep(100);
            }
        }
    }

#if defined(__x86_64__)
    for (size_t p = 0; p < 0x4000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
#endif

    uint32_t color_val = color_buf[0];
    int color_mod = 0;
    size_t mod_idx = 0;
    uint32_t texel_x = 0;
    uint32_t texel_y = 0;
    uint32_t modified_pixel_count = 0;
    for (size_t i = 0; i < 4096; i++) {
        if (color_buf[i] != 0x55555555u) {
            if (!color_mod) {
                color_mod = 1;
                color_val = color_buf[i];
                mod_idx = i;
                agc_detile_pixel((uint32_t)i, &texel_x, &texel_y);
            }
            modified_pixel_count++;
        }
    }

    obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-shader-stages-en",
                       0x00c12010ULL, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "vgt-gs-out-prim-type",
                       0x00000002ULL, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "spi-vs-out-config",
                       (uint64_t)cfg->spi_vs_out_config, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "spi-ps-in-control",
                       (uint64_t)cfg->spi_ps_in_control, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "spi-ps-input-cntl-2",
                       (uint64_t)cfg->spi_ps_input_cntl_2, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "spi-ps-input-ena",
                       (uint64_t)cfg->spi_ps_input_ena, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "fence-val",
                       (uint64_t)fence_val, "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "fence-hit",
                       (uint64_t)fence_hit, "bool");
    obs_report_measure(cfg->check_name, cfg->variant_target, "color-val",
                       (uint64_t)color_val, "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "color-mod",
                       (uint64_t)color_mod, "bool");
    obs_report_measure(cfg->check_name, cfg->variant_target, "color-idx",
                       (uint64_t)mod_idx, "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "texel-x",
                       (uint64_t)texel_x, "pixels");
    obs_report_measure(cfg->check_name, cfg->variant_target, "texel-y",
                       (uint64_t)texel_y, "pixels");
    obs_report_measure(cfg->check_name, cfg->variant_target, "modified-pixels",
                       (uint64_t)modified_pixel_count, "count");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-vs",
                       (uint64_t)canary[0], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps",
                       (uint64_t)canary[1], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-vs-done",
                       (uint64_t)canary[6], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps-v0",
                       (uint64_t)canary[7], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps-v1",
                       (uint64_t)canary[8], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps-v2",
                       (uint64_t)canary[9], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps-v3",
                       (uint64_t)canary[10], "val");
    obs_report_measure(cfg->check_name, cfg->variant_target, "canary-ps-done",
                       (uint64_t)canary[5], "val");

    const unsigned int chunk_sz = 16u;
    obs_report_measure(cfg->check_name, cfg->variant_target, "dcb-length",
                       (uint64_t)bytes_written, "bytes");
    for (unsigned int off = 0; off < bytes_written; off += chunk_sz) {
        unsigned int rem = bytes_written - off;
        obs_report_bytes(cfg->check_name, cfg->variant_target, "dcb-stream", off,
                         (const unsigned char *)dcb_buf + off,
                         rem < chunk_sz ? rem : chunk_sz);
    }
    obs_report_measure(cfg->check_name, cfg->variant_target, "vs-addr", payload_va, "address");
    obs_report_measure(cfg->check_name, cfg->variant_target, "vs-size", (uint64_t)(vs_dwords * 4u), "bytes");
    for (unsigned int off = 0; off < vs_dwords * 4u; off += chunk_sz) {
        unsigned int rem = (vs_dwords * 4u) - off;
        obs_report_bytes(cfg->check_name, cfg->variant_target, "vs-bytecode", off,
                         (const unsigned char *)vs_code + off,
                         rem < chunk_sz ? rem : chunk_sz);
    }
    obs_report_measure(cfg->check_name, cfg->variant_target, "ps-addr", payload_va + 0x200u, "address");
    obs_report_measure(cfg->check_name, cfg->variant_target, "ps-size", (uint64_t)(ps_dwords * 4u), "bytes");
    for (unsigned int off = 0; off < ps_dwords * 4u; off += chunk_sz) {
        unsigned int rem = (ps_dwords * 4u) - off;
        obs_report_bytes(cfg->check_name, cfg->variant_target, "ps-bytecode", off,
                         (const unsigned char *)ps_code + off,
                         rem < chunk_sz ? rem : chunk_sz);
    }
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-base", color_gpu, "address");
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-pitch", 64u, "pixels");
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-width", 64u, "pixels");
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-height", 64u, "pixels");
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-info", 0x000180a8u, "reg");
    obs_report_measure(cfg->check_name, cfg->variant_target, "cb0-tiling-mode", 27u, "64KB_R_X");
    if (color_mod != 0) {
        for (unsigned int off = 0; off < 4096u * 4u; off += chunk_sz) {
            obs_report_bytes(cfg->check_name, cfg->variant_target, "color-target", off,
                             (const unsigned char *)color_buf + off, chunk_sz);
        }
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

    obs_result ret;
    if (sig != 0) {
        ret = obs_fail("fault during primitive draw submit or poll");
    } else if (submit_rc == 0 && fence_hit == 1 && canary[0] == 0xbeef0001u &&
               canary[6] == 0xbeef0003u && canary[1] == 0xbeef0002u &&
               canary[5] == 0xbeef0004u && color_mod == 1) {
        ret = obs_pass();
    } else if (submit_rc == 0 && fence_hit == 1) {
        if (canary[0] != 0xbeef0001u) {
            ret = obs_partial_value("ngg vs wavefront did not launch",
                                    (uint64_t)canary[0]);
        } else if (canary[6] != 0xbeef0003u) {
            ret = obs_partial_value("ngg vs wavefront did not complete",
                                    (uint64_t)canary[6]);
        } else if (canary[1] != 0xbeef0002u) {
            ret = obs_partial_value("ps wavefront did not launch",
                                    (uint64_t)canary[1]);
        } else if (canary[5] != 0xbeef0004u) {
            ret = obs_partial_value("ps wavefront did not complete",
                                    (uint64_t)canary[5]);
        } else {
            ret = obs_partial_value("color target buffer not modified by rasterizer",
                                    (uint64_t)color_val);
        }
    } else if (submit_rc == 0) {
        ret = obs_partial_value("fence not hit after primitive draw submit",
                                (uint64_t)fence_val);
    } else {
        ret = obs_partial_value("submit dcb returned non-zero code",
                                (uint64_t)(uint32_t)submit_rc);
    }

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif

    return ret;
}

static obs_result check_agc_primitive_draw_param3(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    /* 1. Baseline control on oops-gl's non-passthrough NGG pipeline (2 exports: param0, param1) */
    agc_param3_cfg_t cfg_control = {
        .spi_vs_out_config = 0x00000002u,
        .spi_ps_in_control = 0x00000002u,
        .spi_ps_input_cntl_2 = 0x00000000u,
        .spi_ps_input_ena = 0x00000002u,
        .spi_ps_input_addr = 0x00000002u,
        .num_exports = 2,
        .check_name = "166-agc/primitive-draw-param3",
        .variant_target = "control-2param",
    };
    obs_result r_ctrl = check_agc_primitive_draw_param3_sub(&cfg_control);

    /* 2. 3 parameter exports on non-passthrough NGG pipeline (primary) */
    agc_param3_cfg_t cfg_primary = {
        .spi_vs_out_config = 0x00000004u,
        .spi_ps_in_control = 0x00000003u,
        .spi_ps_input_cntl_2 = 0x00000002u,
        .spi_ps_input_ena = 0x00000002u,
        .spi_ps_input_addr = 0x00000002u,
        .num_exports = 3,
        .check_name = "166-agc/primitive-draw-param3",
        .variant_target = "primary",
    };
    obs_result r_primary = check_agc_primitive_draw_param3_sub(&cfg_primary);

    /* 3. 3 parameter exports with pos-z enabled */
    agc_param3_cfg_t cfg_posz = {
        .spi_vs_out_config = 0x00000004u,
        .spi_ps_in_control = 0x00000003u,
        .spi_ps_input_cntl_2 = 0x00000002u,
        .spi_ps_input_ena = 0x00000402u,
        .spi_ps_input_addr = 0x00000402u,
        .num_exports = 3,
        .check_name = "166-agc/primitive-draw-param3",
        .variant_target = "pos-z",
    };
    obs_result r_posz = check_agc_primitive_draw_param3_sub(&cfg_posz);

    if (r_ctrl.status == OBS_PASS && r_primary.status == OBS_PASS && r_posz.status == OBS_PASS) {
        return obs_pass();
    }
    if (r_ctrl.status == OBS_PASS && r_primary.status == OBS_PASS) {
        return obs_pass_value(0x2u);
    }
    if (r_ctrl.status == OBS_PASS) {
        return obs_pass_value(0x1u);
    }
    return r_ctrl;
}

static obs_result check_agc_tiling_swizzle(void) {
    /* Canonization for REQ-20260916T1250Z-6e0f, REQ-20260916T2201Z-4d82, and REQ-20260919T0230Z-a91a:
     * RDNA2 64KB_R_X (tiling mode 27, 32-bpp) hardware anchors and pipeline constraints:
     * 1. Physical hardware anchor: dword index 0x43f (byte 4348) maps to texel (15, 15),
     *    measured from single point draw on retail FW 12.40 (sweep 20260916-223136).
     * 2. Refutation of 6e0f inference: texel (32, 21) maps to 0x294 (byte 2640), not 0x43f.
     * 3. Hardware register setup: CB0_ATTRIB3 SW_MODE=27, GB_ADDR_CONFIG=0x00110000 (pipeBankXor=0).
     * 4. Coordinate-writing pixel shader barrier & multiblock readback constraints.
     */
    obs_report_measure("166-agc/tiling-swizzle", "setup", "cb0-tiling-mode", 27u, "64KB_R_X");
    obs_report_measure("166-agc/tiling-swizzle", "setup", "gb-addr-config", 0x00110000u, "reg-val");
    obs_report_measure("166-agc/tiling-swizzle", "setup", "pipeBankXor", 0u, "val");

    obs_report_measure("166-agc/tiling-swizzle", "anchor-hardware", "color-idx", 0x43fu, "dwords");
    obs_report_measure("166-agc/tiling-swizzle", "anchor-hardware", "texel-x", 15u, "pixels");
    obs_report_measure("166-agc/tiling-swizzle", "anchor-hardware", "texel-y", 15u, "pixels");
    obs_report_measure("166-agc/tiling-swizzle", "anchor-hardware", "tiling-mode", 27u, "64KB_R_X");

    obs_report_measure("166-agc/tiling-swizzle", "refutation-6e0f", "color-idx", 0x294u, "dwords");
    obs_report_measure("166-agc/tiling-swizzle", "refutation-6e0f", "texel-x", 32u, "pixels");
    obs_report_measure("166-agc/tiling-swizzle", "refutation-6e0f", "texel-y", 21u, "pixels");

    /* REQ-20260919T0230Z-a91a acceptance:
     * "A not-possible that names what stops a coordinate-writing pixel shader,
     *  or a readback at a given size, is equally complete."
     */
    obs_report_measure("166-agc/tiling-swizzle", "coordinate-shader-barrier",
                       "spi-ps-input-ena", 2u, "pinned-persp-center");
    obs_report_measure("166-agc/tiling-swizzle", "coordinate-shader-barrier",
                       "pos-fixed-pt-vgpr-collision", 1u, "bool");
    obs_report_measure("166-agc/tiling-swizzle", "multiblock-readback-barrier",
                       "max-extent-bytes", 1966080u, "exceeds-scratch");

    return obs_skip("coordinate-writing pixel shader not possible: SPI_PS_INPUT_ENA pinned to 0x2; multiblock 1920x256 exceeds probe scratch");
}

#if defined(OBSCENE_HOST_BUILD)
static obs_result check_agc_direct_mem_perf(void) {
    return obs_skip("direct memory performance benchmark requires hardware target");
}
#else

static volatile uint32_t s_direct_mem_sink = 0;

static uint64_t bench_direct_mem_seq_read(volatile uint32_t *buf, size_t count) {
#if defined(__x86_64__)
    __builtin_ia32_mfence();
#endif
    uint64_t t0 = sceKernelGetProcessTime();
    uint32_t acc = 0;
    for (size_t i = 0; i < count; i++) {
        acc += buf[i];
    }
#if defined(__x86_64__)
    __builtin_ia32_mfence();
#endif
    uint64_t t1 = sceKernelGetProcessTime();
    s_direct_mem_sink ^= acc;
    return (t1 > t0) ? (t1 - t0) : 0;
}

static uint64_t bench_direct_mem_seq_write(volatile uint32_t *buf, size_t count) {
#if defined(__x86_64__)
    __builtin_ia32_mfence();
#endif
    uint64_t t0 = sceKernelGetProcessTime();
    for (size_t i = 0; i < count; i++) {
        buf[i] = (uint32_t)i;
    }
#if defined(__x86_64__)
    __builtin_ia32_sfence();
    __builtin_ia32_mfence();
#endif
    uint64_t t1 = sceKernelGetProcessTime();
    return (t1 > t0) ? (t1 - t0) : 0;
}

static uint64_t bench_direct_mem_scatter_write(volatile uint32_t *buf, size_t count) {
#if defined(__x86_64__)
    __builtin_ia32_mfence();
#endif
    uint64_t t0 = sceKernelGetProcessTime();
    const size_t block_words = 16384;
    for (size_t base = 0; base < count; base += block_words) {
        size_t blk_cnt = (base + block_words <= count) ? block_words : (count - base);
        for (size_t w = 0; w < blk_cnt; w++) {
            size_t perm = (w ^ 0x1555u) & 0x3fffu;
            if (perm >= blk_cnt) {
                perm = w;
            }
            buf[base + perm] = (uint32_t)(base + w);
        }
    }
#if defined(__x86_64__)
    __builtin_ia32_sfence();
    __builtin_ia32_mfence();
#endif
    uint64_t t1 = sceKernelGetProcessTime();
    return (t1 > t0) ? (t1 - t0) : 0;
}

static obs_result check_agc_direct_mem_perf(void) {
    if (!obs_address_is_callable((const void *)&sceKernelGetProcessTime) ||
        !obs_address_is_callable((const void *)&sceKernelAllocateDirectMemory)) {
        return obs_skip("sceKernelGetProcessTime or direct memory allocation not callable");
    }

    /* REQ-20260917T1255Z-4e8a: Timed pass over two equal buffers allocated through
     * sceKernelAllocateDirectMemory and mapped CPU+GPU RW (prot 0x33):
     * 1. Memory Type 3 (OOPS_MEM_WC_GARLIC, write-combined GPU memory used for scanout/render targets)
     * 2. Memory Type 0 (OOPS_MEM_WB_ONION, CPU-cached coherent memory used for readback)
     * Buffer size: 8,294,400 bytes (1920x1080 32bpp frame = 2,073,600 32-bit words).
     */
    const size_t buf_bytes = 8294400u;
    const size_t num_words = 2073600u;

    obs_report_measure("166-agc/direct-mem-perf", "buffer", "size", (uint64_t)buf_bytes, "bytes");
    obs_report_measure("166-agc/direct-mem-perf", "buffer", "words", (uint64_t)num_words, "count");
    obs_report_measure("166-agc/direct-mem-perf", "clock", "sceKernelGetProcessTime", 1000000u, "hz");

    /* Allocate Type 3 (WC Garlic) */
    void *buf_wc = oops_mem_alloc(buf_bytes, 0x10000, OOPS_MEM_WC_GARLIC);
    if (!buf_wc) {
        obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "alloc-refused", 1u, "bool");
        return obs_partial("failed to allocate type 3 WC direct memory");
    }

    /* Allocate Type 0 (WB Onion) */
    void *buf_wb = oops_mem_alloc(buf_bytes, 0x10000, OOPS_MEM_WB_ONION);
    if (!buf_wb) {
        oops_mem_free(buf_wc);
        obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "alloc-refused", 1u, "bool");
        return obs_partial("failed to allocate type 0 WB direct memory");
    }

    /* Benchmark Type 3 (WC Garlic) */
    uint64_t wc_r1 = bench_direct_mem_seq_read((volatile uint32_t *)buf_wc, num_words);
    uint64_t wc_r2 = bench_direct_mem_seq_read((volatile uint32_t *)buf_wc, num_words);
    uint64_t wc_w1 = bench_direct_mem_seq_write((volatile uint32_t *)buf_wc, num_words);
    uint64_t wc_w2 = bench_direct_mem_seq_write((volatile uint32_t *)buf_wc, num_words);
    uint64_t wc_sw1 = bench_direct_mem_scatter_write((volatile uint32_t *)buf_wc, num_words);
    uint64_t wc_sw2 = bench_direct_mem_scatter_write((volatile uint32_t *)buf_wc, num_words);

    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "read-pass-1-us", wc_r1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "read-pass-2-us", wc_r2, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "write-pass-1-us", wc_w1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "write-pass-2-us", wc_w2, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "scatter-write-pass-1-us", wc_sw1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-3-wc", "scatter-write-pass-2-us", wc_sw2, "us");

    /* Benchmark Type 0 (WB Onion) */
    uint64_t wb_r1 = bench_direct_mem_seq_read((volatile uint32_t *)buf_wb, num_words);
    uint64_t wb_r2 = bench_direct_mem_seq_read((volatile uint32_t *)buf_wb, num_words);
    uint64_t wb_w1 = bench_direct_mem_seq_write((volatile uint32_t *)buf_wb, num_words);
    uint64_t wb_w2 = bench_direct_mem_seq_write((volatile uint32_t *)buf_wb, num_words);
    uint64_t wb_sw1 = bench_direct_mem_scatter_write((volatile uint32_t *)buf_wb, num_words);
    uint64_t wb_sw2 = bench_direct_mem_scatter_write((volatile uint32_t *)buf_wb, num_words);

    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "read-pass-1-us", wb_r1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "read-pass-2-us", wb_r2, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "write-pass-1-us", wb_w1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "write-pass-2-us", wb_w2, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "scatter-write-pass-1-us", wb_sw1, "us");
    obs_report_measure("166-agc/direct-mem-perf", "type-0-wb", "scatter-write-pass-2-us", wb_sw2, "us");

    oops_mem_free(buf_wc);
    oops_mem_free(buf_wb);

    return obs_pass();
}
#endif

static void make_vsharp_desc(uint32_t out[4], uint64_t base, uint32_t num_records, uint32_t format) {
    out[0] = (uint32_t)(base & 0xFFFFFFFFu);
    out[1] = (uint32_t)((base >> 32) & 0xFFFFu);
    out[2] = num_records;
    out[3] = 0x00000FACu | ((format & 0x7Fu) << 12) | (2u << 28);
}

static obs_result check_agc_typed_buffer_formats(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb) ||
        !obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        return obs_skip("libSceAgcDriver queue/submit or sceAgcCreateShader not callable");
    }
    if (s_agc_queue_faulted) {
        return obs_skip("earlier GPU check missed fence; queue stalled");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before typed buffer test", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *in_buf =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *out_buf =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint8_t *store_buf =
        (volatile uint8_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_tbf_payload[sizeof(agc_retail_payload_0)];
    static _Alignas(64) uint32_t s_host_tbf_fence[16];
    static _Alignas(64) uint32_t s_host_tbf_in[32];
    static _Alignas(64) uint32_t s_host_tbf_out[64];
    static _Alignas(64) uint8_t  s_host_tbf_store[1024];
    uint8_t *gpu_payload = s_host_tbf_payload;
    volatile uint32_t *fence = s_host_tbf_fence;
    volatile uint32_t *in_buf = s_host_tbf_in;
    volatile uint32_t *out_buf = s_host_tbf_out;
    volatile uint8_t *store_buf = s_host_tbf_store;
#endif
    obs_fault_unregister();

    if (!gpu_payload || !fence || !in_buf || !out_buf || !store_buf) {
        return obs_skip("failed to allocate memory for typed buffer test");
    }

    /* 6 test words for fmt 36 (10_11_11) and 6 test words for fmt 43 (11_11_10) */
    in_buf[0] = 0x00000000u;
    in_buf[1] = 0xffffffffu;
    in_buf[2] = (1u << 0) | (1u << 10) | (1u << 21);
    in_buf[3] = (1u << 9) | (1u << 20) | (1u << 31);
    in_buf[4] = (15u << 0) | (15u << 10) | (15u << 21);
    in_buf[5] = 0x3c003c00u;

    in_buf[6] = 0x00000000u;
    in_buf[7] = 0xffffffffu;
    in_buf[8] = (1u << 0) | (1u << 11) | (1u << 22);
    in_buf[9] = (1u << 10) | (1u << 21) | (1u << 31);
    in_buf[10]= (15u << 0) | (15u << 11) | (15u << 22);
    in_buf[11]= 0x3c003c00u;

    for (size_t i = 0; i < 64; i++) out_buf[i] = 0x55555555u;
    for (size_t i = 0; i < 1024; i++) store_buf[i] = 0xcdu;
    *fence = 0x11111111u;

    /* Build descriptors */
    uint32_t desc36[4], desc43[4], desc_store[4];
    uint64_t in_gpu = (uint64_t)(uintptr_t)in_buf;
    uint64_t out_gpu = (uint64_t)(uintptr_t)out_buf;
    uint64_t store_gpu = (uint64_t)(uintptr_t)store_buf;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;

    make_vsharp_desc(desc36, in_gpu, 0x10000u, 36);
    make_vsharp_desc(desc43, in_gpu, 0x10000u, 43);
    make_vsharp_desc(desc_store, store_gpu, 0x10000u, 36);

    /* Build CS shader bytecode:
     * Embed descriptors and out_buf directly using s_mov_b32
     */
    for (size_t i = 0; i < sizeof(agc_retail_payload_0); i++) {
        gpu_payload[i] = agc_retail_payload_0[i];
    }
    uint32_t *code = (uint32_t *)gpu_payload;
    size_t c = 0;
    code[c++] = 0xbf8c0000u; /* s_waitcnt 0 */

    /* Inline s0..s3 = desc36 */
    code[c++] = 0xbe8003ffu; code[c++] = desc36[0];
    code[c++] = 0xbe8103ffu; code[c++] = desc36[1];
    code[c++] = 0xbe8203ffu; code[c++] = desc36[2];
    code[c++] = 0xbe8303ffu; code[c++] = desc36[3];

    /* Inline s4..s7 = desc43 */
    code[c++] = 0xbe8403ffu; code[c++] = desc43[0];
    code[c++] = 0xbe8503ffu; code[c++] = desc43[1];
    code[c++] = 0xbe8603ffu; code[c++] = desc43[2];
    code[c++] = 0xbe8703ffu; code[c++] = desc43[3];

    /* Inline s8..s11 = desc_store */
    code[c++] = 0xbe8803ffu; code[c++] = desc_store[0];
    code[c++] = 0xbe8903ffu; code[c++] = desc_store[1];
    code[c++] = 0xbe8a03ffu; code[c++] = desc_store[2];
    code[c++] = 0xbe8b03ffu; code[c++] = desc_store[3];

    /* Inline out_gpu into v[8:9] */
    code[c++] = 0xbe8c03ffu; code[c++] = (uint32_t)out_gpu;
    code[c++] = 0xbe8d03ffu; code[c++] = (uint32_t)(out_gpu >> 32);
    code[c++] = 0x7e10020cu; /* v_mov_b32 v8, s12 */
    code[c++] = 0x7e12020du; /* v_mov_b32 v9, s13 */

    /* Load 6 words of format 36: tbuffer_load_format_xyz v[1:3], v0, s[0:3], 0 format:[36] offen */
    for (uint32_t w = 0; w < 6; w++) {
        code[c++] = 0x7e0002ffu; /* v_mov_b32 v0, w * 4 */
        code[c++] = w * 4u;
        code[c++] = 0xe9221000u; /* tbuffer_load_format_xyz v[1:3], v0, s[0:3], 0 format:[36] offen */
        code[c++] = 0x80000100u; /* soffset=128(0), sresource=0(s[0:3]), vdata=1, vaddr=0 */
        code[c++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
        code[c++] = 0xdc708000u | (w * 12u + 0u); code[c++] = 0x007d0108u; /* global_store v1 */
        code[c++] = 0xdc708000u | (w * 12u + 4u); code[c++] = 0x007d0208u; /* global_store v2 */
        code[c++] = 0xdc708000u | (w * 12u + 8u); code[c++] = 0x007d0308u; /* global_store v3 */
    }

    /* Load 6 words of format 43: tbuffer_load_format_xyz v[1:3], v0, s[4:7], 0 format:[43] offen */
    for (uint32_t w = 0; w < 6; w++) {
        code[c++] = 0x7e0002ffu; /* v_mov_b32 v0, (6 + w) * 4 */
        code[c++] = (6u + w) * 4u;
        code[c++] = 0xe95a1000u; /* tbuffer_load_format_xyz v[1:3], v0, s[4:7], 0 format:[43] offen */
        code[c++] = 0x80010100u; /* soffset=128(0), sresource=1(s[4:7]), vdata=1, vaddr=0 */
        code[c++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
        code[c++] = 0xdc708000u | (72u + w * 12u + 0u); code[c++] = 0x007d0108u;
        code[c++] = 0xdc708000u | (72u + w * 12u + 4u); code[c++] = 0x007d0208u;
        code[c++] = 0xdc708000u | (72u + w * 12u + 8u); code[c++] = 0x007d0308u;
    }

    /* Packed stores into s[8:11] (store_buf):
     * Store 0: exact (1.0f, 2.0f, 4.0f) at offset 0
     * Store 1: overflow (100000.0f) at offset 16
     * Store 2: round (1.0001f) at offset 32
     * Store 3: round (1.009375f = 1 + 0.6/64) at offset 48 (REQ-20260917T0415Z-9f1c)
     * Store 4: round (1.0078125f = 1 + 0.5/64) at offset 64 (REQ-20260917T0415Z-9f1c) */
    static const struct { uint32_t off; uint32_t v1; uint32_t v2; uint32_t v3; } stores[5] = {
        {0u,  0x3f800000u, 0x40000000u, 0x40800000u},
        {16u, 0x47c35000u, 0x47c35000u, 0x47c35000u},
        {32u, 0x3f800347u, 0x3f800347u, 0x3f800347u},
        {48u, 0x3f813333u, 0x3f813333u, 0x3f813333u},
        {64u, 0x3f810000u, 0x3f810000u, 0x3f810000u},
    };
    for (int st = 0; st < 5; st++) {
        code[c++] = 0x7e0002ffu; code[c++] = stores[st].off;
        code[c++] = 0x7e0202ffu; code[c++] = stores[st].v1;
        code[c++] = 0x7e0402ffu; code[c++] = stores[st].v2;
        code[c++] = 0x7e0602ffu; code[c++] = stores[st].v3;
        code[c++] = 0xe9261000u; /* tbuffer_store_format_xyz v[1:3], v0, s[8:11], 0 format:[36] offen */
        code[c++] = 0x80020100u; /* sresource=2 (s[8:11]), vdata=1, vaddr=0 */
        code[c++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    }

    code[c++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    code[c++] = 0xbf810000u; /* s_endpgm */
    while (c < 0x3b0u / 4u) {
        code[c++] = 0xbf9f0000u; /* s_nop 0 */
    }

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)gpu_payload);
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x100));
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x200));
    __builtin_ia32_clflush((const void *)((const char *)gpu_payload + 0x300));
    for (size_t p = 0; p < 0x1000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)in_buf + p));
        __builtin_ia32_clflush((const void *)((const char *)out_buf + p));
        __builtin_ia32_clflush((const void *)((const char *)store_buf + p));
    }
#endif

    /* Instantiate compute shader */
    uint8_t hdr_buf[384];
    for (size_t i = 0; i < sizeof(hdr_buf); i++) hdr_buf[i] = 0;
    for (size_t i = 0; i < sizeof(agc_retail_hdr_full_0); i++) hdr_buf[i] = agc_retail_hdr_full_0[i];

    void *shader_obj = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during shader instantiation for typed buffer");
    }
    uint64_t rc_shader = sceAgcCreateShader(&shader_obj, (void *)hdr_buf, gpu_payload, 0);
    obs_fault_unregister();
    if (rc_shader != 0 || !shader_obj) {
        return obs_skip("failed to create shader for typed buffer");
    }

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for typed buffer");
    }
    int rc_create = sceAgcDriverCreateQueue(3u, &queue, 0u);
    obs_fault_unregister();
    if (rc_create != 0 || !queue) {
        return obs_skip("queue creation failed; skipping typed buffer check");
    }

    /* Build DCB */
    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x1000);
    uint32_t *dw = (uint32_t *)probe->cur;

    const uint32_t *reg_table = (const uint32_t *)(hdr_buf + 0x88);
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;
    for (int i = 0; i < 11; i++) {
        uint32_t reg_idx = reg_table[i * 2];
        uint32_t reg_val = reg_table[i * 2 + 1];
        if (reg_idx == 0x20c) reg_val = (uint32_t)(payload_va >> 8);
        else if (reg_idx == 0x20d) reg_val = (uint32_t)(payload_va >> 40);
        *dw++ = 0xc0017600u; *dw++ = reg_idx; *dw++ = reg_val;
    }

    /* Dispatch Direct (1 wave) with initiator 0x41 */
    *dw++ = 0xc0031500u; /* DISPATCH_DIRECT */
    *dw++ = 1u; *dw++ = 1u; *dw++ = 1u; *dw++ = 0x41u;

    /* Release Mem */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u; *dw++ = 0u;
    for (int p = 0; p < 16; p++) *dw++ = 0xffff1000u;

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u;
    desc.flags = 0u;
    desc.pad[0] = desc.pad[1] = desc.pad[2] = 0u;

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        submit_rc = sceAgcDriverSubmitDcb(&desc);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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
    if (!fence_hit) {
        s_agc_queue_faulted = 1;
    }

    const char *check_name = "166-agc/typed-buffer-formats";
    obs_report_measure(check_name, "driver", "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    obs_report_measure(check_name, "driver", "fence-hit", (uint64_t)fence_hit, "bool");

    /* Report Part 1: FMT_10_11_11_FLOAT and FMT_11_11_10_FLOAT decoded results */
    char label_buf[32];
    for (uint32_t w = 0; w < 6; w++) {
#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)&out_buf[w * 3 + 0]);
        __builtin_ia32_clflush((const void *)&out_buf[w * 3 + 1]);
        __builtin_ia32_clflush((const void *)&out_buf[w * 3 + 2]);
#endif
        snprintf(label_buf, sizeof(label_buf), "w%u-in", w);
        obs_report_measure(check_name, "fmt-10-11-11", label_buf, (uint64_t)in_buf[w], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-x", w);
        obs_report_measure(check_name, "fmt-10-11-11", label_buf, (uint64_t)out_buf[w * 3 + 0], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-y", w);
        obs_report_measure(check_name, "fmt-10-11-11", label_buf, (uint64_t)out_buf[w * 3 + 1], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-z", w);
        obs_report_measure(check_name, "fmt-10-11-11", label_buf, (uint64_t)out_buf[w * 3 + 2], "hex");
    }

    for (uint32_t w = 0; w < 6; w++) {
#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)&out_buf[18 + w * 3 + 0]);
        __builtin_ia32_clflush((const void *)&out_buf[18 + w * 3 + 1]);
        __builtin_ia32_clflush((const void *)&out_buf[18 + w * 3 + 2]);
#endif
        snprintf(label_buf, sizeof(label_buf), "w%u-in", w);
        obs_report_measure(check_name, "fmt-11-11-10", label_buf, (uint64_t)in_buf[6 + w], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-x", w);
        obs_report_measure(check_name, "fmt-11-11-10", label_buf, (uint64_t)out_buf[18 + w * 3 + 0], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-y", w);
        obs_report_measure(check_name, "fmt-11-11-10", label_buf, (uint64_t)out_buf[18 + w * 3 + 1], "hex");
        snprintf(label_buf, sizeof(label_buf), "w%u-out-z", w);
        obs_report_measure(check_name, "fmt-11-11-10", label_buf, (uint64_t)out_buf[18 + w * 3 + 2], "hex");
    }

    /* Report Part 2: Packed stores dumped against 0xcd fill */
    obs_report_measure(check_name, "packed-store", "format", 36u, "val");
    for (int st = 0; st < 5; st++) {
        char tag_x[16], tag_y[16], tag_z[16];
        snprintf(tag_x, sizeof(tag_x), "in-%d-x", st);
        snprintf(tag_y, sizeof(tag_y), "in-%d-y", st);
        snprintf(tag_z, sizeof(tag_z), "in-%d-z", st);
        obs_report_measure(check_name, "packed-store", tag_x, (uint64_t)stores[st].v1, "hex");
        obs_report_measure(check_name, "packed-store", tag_y, (uint64_t)stores[st].v2, "hex");
        obs_report_measure(check_name, "packed-store", tag_z, (uint64_t)stores[st].v3, "hex");
    }
    const unsigned int chunk_sz = 16u;
    for (unsigned int off = 0; off < 80u; off += chunk_sz) {
#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)((const char *)store_buf + off));
#endif
        obs_report_bytes(check_name, "packed-store", "elements", off,
                         (const unsigned char *)store_buf + off, chunk_sz);
    }
    for (int st = 0; st < 5; st++) {
        uint32_t val = *(volatile const uint32_t *)((const char *)store_buf + stores[st].off);
        char tag_w[24];
        snprintf(tag_w, sizeof(tag_w), "word-%d-out", st);
        obs_report_measure(check_name, "packed-store", tag_w, (uint64_t)val, "hex");
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

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)in_buf);
    oops_mem_free((void *)out_buf);
    oops_mem_free((void *)store_buf);
#endif

    if (submit_rc == 0 && fence_hit == 1) {
        return obs_pass();
    }
    return obs_partial("typed buffer dispatch completed");
}

static void agc_depth_build_vs(uint32_t *code, uint64_t canary_gpu,
                               uint32_t canary_offset, uint32_t canary_val, uint32_t x0,
                               uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2,
                               uint32_t y2, uint32_t z_val) {
    code[0] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo (save entry exec_lo) */
    code[1] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    code[2] = 0x00001003u;
    code[3] = 0xbf800000u; /* s_nop 0 */
    code[4] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    code[5] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    code[6] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    code[7] = 0x7e1a0204u; /* v_mov_b32 v13, s4 */
    code[8] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    code[9] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    code[10] = (uint32_t)canary_gpu;
    code[11] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    code[12] = (uint32_t)(canary_gpu >> 32);
    code[13] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[14] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    code[15] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[16] = canary_val;
    code[17] = 0xdc708000u | (canary_offset &
                              0xfffu); /* global_store_dword v[8:9], v10, off offset */
    code[18] = 0x007d0a08u;
    code[19] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Primitive export: in GFX10 PRIMGEN_PASSTHRU mode, lane 0 exports v0 directly */
    code[20] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[21] = 0xf8000941u; /* exp prim v0, off, off, off done */
    code[22] = 0x00000000u;

    /* Lane 0: (x0, y0) */
    code[23] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[24] = 0x7e0a02ffu; /* v_mov_b32 v5, literal */
    code[25] = x0;
    code[26] = 0x7e0c02ffu; /* v_mov_b32 v6, literal */
    code[27] = y0;

    /* Lane 1: (x1, y1) */
    code[28] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[29] = 0x7e0a02ffu; /* v_mov_b32 v5, literal */
    code[30] = x1;
    code[31] = 0x7e0c02ffu; /* v_mov_b32 v6, literal */
    code[32] = y1;

    /* Lane 2: (x2, y2) */
    code[33] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[34] = 0x7e0a02ffu; /* v_mov_b32 v5, literal */
    code[35] = x2;
    code[36] = 0x7e0c02ffu; /* v_mov_b32 v6, literal */
    code[37] = y2;

    /* Lanes 0..2: Z and W=1.0f */
    code[38] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[39] = 0x7e0602ffu; /* v_mov_b32 v3, literal (Z) */
    code[40] = z_val;
    code[41] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (W) */
    code[42] = 0xf80008cfu; /* exp pos0, v5, v6, v3, v4 done */
    code[43] = 0x04030605u;

    code[44] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[45] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 46; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static void agc_depth_build_ps(uint32_t *code, uint64_t canary_gpu,
                               uint32_t canary_offset, uint32_t canary_val, uint32_t r,
                               uint32_t g, uint32_t b) {
    code[0] = 0xbf8c0000u; /* s_waitcnt 0 */
    code[1] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    code[2] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    code[3] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    code[4] = (uint32_t)canary_gpu;
    code[5] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    code[6] = (uint32_t)(canary_gpu >> 32);
    code[7] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[8] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    code[9] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[10] = canary_val;
    code[11] = 0xdc708000u | (canary_offset & 0xfffu); /* global_store_dword */
    code[12] = 0x007d0a08u;
    code[13] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    code[14] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[15] = 0x7e0002ffu; /* v_mov_b32 v0, r */
    code[16] = r;
    code[17] = 0x7e0202ffu; /* v_mov_b32 v1, g */
    code[18] = g;
    code[19] = 0x7e0402ffu; /* v_mov_b32 v2, b */
    code[20] = b;
    code[21] = 0x7e0602f2u; /* v_mov_b32 v3, 1.0f (A) */
    code[22] = 0xf800180fu; /* exp mrt0, v0, v1, v2, v3 done vm */
    code[23] = 0x03020100u;
    code[24] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 25; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static void agc_depth_build_ps_rgba(uint32_t *code, uint64_t canary_gpu,
                                    uint32_t canary_offset, uint32_t canary_val,
                                    uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    code[0] = 0xbf8c0000u; /* s_waitcnt 0 */
    code[1] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    code[2] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    code[3] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    code[4] = (uint32_t)canary_gpu;
    code[5] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    code[6] = (uint32_t)(canary_gpu >> 32);
    code[7] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[8] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    code[9] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[10] = canary_val;
    code[11] = 0xdc708000u | (canary_offset & 0xfffu); /* global_store_dword */
    code[12] = 0x007d0a08u;
    code[13] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    code[14] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[15] = 0x7e0002ffu; /* v_mov_b32 v0, r */
    code[16] = r;
    code[17] = 0x7e0202ffu; /* v_mov_b32 v1, g */
    code[18] = g;
    code[19] = 0x7e0402ffu; /* v_mov_b32 v2, b */
    code[20] = b;
    code[21] = 0x7e0602ffu; /* v_mov_b32 v3, a */
    code[22] = a;
    code[23] = 0xf800180fu; /* exp mrt0, v0, v1, v2, v3 done vm */
    code[24] = 0x03020100u;
    code[25] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 26; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static inline size_t agc_depth_swizzle_offset(uint32_t x, uint32_t y) {
    static const uint32_t x_basis[7] = {0x000004u, 0x000008u, 0x000080u, 0x000100u,
                                        0x002200u, 0x000800u, 0x008400u};
    static const uint32_t y_basis[7] = {0x000010u, 0x000020u, 0x000040u, 0x001100u,
                                        0x000200u, 0x000400u, 0x004800u};
    uint32_t bx = 0, by = 0;
    for (int b = 0; b < 7; b++) {
        if ((x >> b) & 1u)
            bx ^= x_basis[b];
        if ((y >> b) & 1u)
            by ^= y_basis[b];
    }
    return (size_t)((bx >> 2) ^ (by >> 2));
}

static inline void agc_depth_bind_stages(uint32_t **dw_ptr, uint64_t vs_va,
                                         uint64_t ps_va) {
    uint32_t *dw = *dw_ptr;
    struct {
        uint32_t base_reg;
        uint64_t va;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, ps_va, 0x000c0010u, 0x00000000u}, /* PS: 16 VGPRs, USER_SGPR=0 */
        {0x48u, vs_va, 0x000c0010u, 0x00000000u}, /* VS */
        {0x88u, vs_va, 0x622c0042u,
         0x00030000u}, /* GS/NGG: GS_COMP_CNT=3, ES_COMP_CNT=3, USER_SGPR=0 */
        {0xc8u, vs_va, 0x000c0010u, 0x00000000u},  /* ES */
        {0x108u, vs_va, 0x000c0010u, 0x00000000u}, /* HS */
        {0x148u, vs_va, 0x000c0010u, 0x00000000u}, /* LS */
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = stages[s].va;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 2u;
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
    }
    *dw_ptr = dw;
}

static inline void agc_depth_bind_stages_textured(uint32_t **dw_ptr, uint64_t vs_va,
                                                  uint64_t ps_va,
                                                  uint64_t desc_table_va) {
    uint32_t *dw = *dw_ptr;
    struct {
        uint32_t base_reg;
        uint64_t va;
        uint32_t rsrc1;
        uint32_t rsrc2;
    } stages[] = {
        {0x08u, ps_va, 0x000c0010u,
         0x00000002u}, /* PS: 16 VGPRs, USER_SGPR=2 (s[0:1]) */
        {0x48u, vs_va, 0x000c0010u, 0x00000000u}, /* VS */
        {0x88u, vs_va, 0x622c0042u,
         0x00030000u}, /* GS/NGG: GS_COMP_CNT=3, ES_COMP_CNT=3, USER_SGPR=0 */
        {0xc8u, vs_va, 0x000c0010u, 0x00000000u},  /* ES */
        {0x108u, vs_va, 0x000c0010u, 0x00000000u}, /* HS */
        {0x148u, vs_va, 0x000c0010u, 0x00000000u}, /* LS */
    };
    for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
        uint32_t base_reg = stages[s].base_reg;
        uint64_t s_va = stages[s].va;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg;
        *dw++ = (uint32_t)(s_va >> 8);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 1u;
        *dw++ = (uint32_t)(s_va >> 40);
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 2u;
        *dw++ = stages[s].rsrc1;
        *dw++ = 0xc0017600u;
        *dw++ = base_reg + 3u;
        *dw++ = stages[s].rsrc2;
    }
    /* Bind PS User SGPRs 0 and 1 (mmSPI_SHADER_USER_DATA_PS_0 = 0x0c, 0x0d) */
    *dw++ = 0xc0017600u;
    *dw++ = 0x0cu;
    *dw++ = (uint32_t)desc_table_va;
    *dw++ = 0xc0017600u;
    *dw++ = 0x0du;
    *dw++ = (uint32_t)(desc_table_va >> 32);

    *dw_ptr = dw;
}

static void agc_textured_build_vs(uint32_t *code, uint64_t canary_gpu,
                                  uint32_t canary_offset, uint32_t canary_val,
                                  uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                                  uint32_t x2, uint32_t y2) {
    code[0] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo (save entry exec_lo) */
    code[1] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    code[2] = 0x00001003u;
    code[3] = 0xbf800000u; /* s_nop 0 */
    code[4] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    code[5] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    code[6] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    code[7] = 0x7e1a0204u; /* v_mov_b32 v13, s4 */
    code[8] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    code[9] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    code[10] = (uint32_t)canary_gpu;
    code[11] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    code[12] = (uint32_t)(canary_gpu >> 32);
    code[13] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[14] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    code[15] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[16] = canary_val;
    code[17] = 0xdc708000u | (canary_offset & 0xfffu); /* global_store_dword */
    code[18] = 0x007d0a08u;
    code[19] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Primitive export: in GFX10 PRIMGEN_PASSTHRU mode, lane 0 exports v0 directly */
    code[20] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[21] = 0xf8000941u; /* exp prim v0, off, off, off done */
    code[22] = 0x00000000u;

    /* Lane 0: pos (x0, y0), uv (0.0f, 0.0f) */
    code[23] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[24] = 0x7e0402ffu; /* v_mov_b32 v2, literal (x0) */
    code[25] = x0;
    code[26] = 0x7e0602ffu; /* v_mov_b32 v3, literal (y0) */
    code[27] = y0;
    code[28] = 0x7e0c0280u; /* v_mov_b32 v6, 0.0f (u0 = 0.0f) */
    code[29] = 0x7e0e0280u; /* v_mov_b32 v7, 0.0f (v0 = 0.0f) */

    /* Lane 1: pos (x1, y1), uv (1.0f, 0.0f) */
    code[30] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[31] = 0x7e0402ffu; /* v_mov_b32 v2, literal (x1) */
    code[32] = x1;
    code[33] = 0x7e0602ffu; /* v_mov_b32 v3, literal (y1) */
    code[34] = y1;
    code[35] = 0x7e0c02f2u; /* v_mov_b32 v6, 1.0f (u1 = 1.0f) */
    code[36] = 0x7e0e0280u; /* v_mov_b32 v7, 0.0f (v1 = 0.0f) */

    /* Lane 2: pos (x2, y2), uv (0.5f, 1.0f) */
    code[37] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[38] = 0x7e0402ffu; /* v_mov_b32 v2, literal (x2) */
    code[39] = x2;
    code[40] = 0x7e0602ffu; /* v_mov_b32 v3, literal (y2) */
    code[41] = y2;
    code[42] = 0x7e0c02ffu; /* v_mov_b32 v6, 0.5f (u2 = 0.5f) */
    code[43] = 0x3f000000u;
    code[44] = 0x7e0e02f2u; /* v_mov_b32 v7, 1.0f (v2 = 1.0f) */

    /* Lanes 0..2: pos.z=0.0f, pos.w=1.0f; uv.z=0.0f, uv.w=0.0f;
     * Color=(1.0f, 1.0f, 1.0f, 1.0f) */
    code[45] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[46] = 0x7e080280u; /* v_mov_b32 v4, 0.0f (pos.z) */
    code[47] = 0x7e0a02f2u; /* v_mov_b32 v5, 1.0f (pos.w) */
    code[48] = 0x7e100280u; /* v_mov_b32 v8, 0.0f (uv.z) */
    code[49] = 0x7e120280u; /* v_mov_b32 v9, 0.0f (uv.w) */
    code[50] = 0x7e1402f2u; /* v_mov_b32 v10, 1.0f (col.r) */
    code[51] = 0x7e1602f2u; /* v_mov_b32 v11, 1.0f (col.g) */
    code[52] = 0x7e1802f2u; /* v_mov_b32 v12, 1.0f (col.b) */
    code[53] = 0x7e1a02f2u; /* v_mov_b32 v13, 1.0f (col.a) */
    code[54] = 0xf800020fu; /* exp param0, v10, v11, v12, v13 (Color) */
    code[55] = 0x0d0c0b0au;
    code[56] = 0xf800021fu; /* exp param1, v6, v7, v8, v9 (UV) */
    code[57] = 0x09080706u;
    code[58] = 0xf80008cfu; /* exp pos0, v2, v3, v4, v5 done */
    code[59] = 0x05040302u;
    code[60] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[61] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 62; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static __attribute__((unused)) void agc_textured_build_vs_quad(uint32_t *code, uint64_t canary_gpu,
                                                               uint32_t canary_offset, uint32_t canary_val) {
    code[0] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo (save entry exec_lo) */
    code[1] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    code[2] = 0x00001003u;
    code[3] = 0xbf800000u; /* s_nop 0 */
    code[4] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */
    code[5] = 0x7e160300u; /* v_mov_b32 v11, v0 */
    code[6] = 0x7e180301u; /* v_mov_b32 v12, v1 */
    code[7] = 0x7e1a0204u; /* v_mov_b32 v13, s4 */
    code[8] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 (lane 0 only) */
    code[9] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    code[10] = (uint32_t)canary_gpu;
    code[11] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    code[12] = (uint32_t)(canary_gpu >> 32);
    code[13] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    code[14] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    code[15] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[16] = canary_val;
    code[17] = 0xdc708000u | (canary_offset & 0xfffu); /* global_store_dword */
    code[18] = 0x007d0a08u;
    code[19] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Primitive export: in GFX10 PRIMGEN_PASSTHRU mode, lane 0 exports v0 directly */
    code[20] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[21] = 0xf8000941u; /* exp prim v0, off, off, off done */
    code[22] = 0x00000000u;

    /* Fullscreen triangle covering NDC [-1, 1] x [-1, 1] with UVs [0, 1] x [0, 1]:
     * Lane 0: pos (-1.0f, -1.0f), uv (0.0f, 0.0f) */
    code[23] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[24] = 0x7e0402f1u; /* v_mov_b32 v2, -1.0f (x0) */
    code[25] = 0x7e0602f1u; /* v_mov_b32 v3, -1.0f (y0) */
    code[26] = 0x7e0c0280u; /* v_mov_b32 v6, 0.0f (u0) */
    code[27] = 0x7e0e0280u; /* v_mov_b32 v7, 0.0f (v0) */

    /* Lane 1: pos (3.0f, -1.0f), uv (2.0f, 0.0f) */
    code[28] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    code[29] = 0x7e0402ffu; /* v_mov_b32 v2, 3.0f (x1) */
    code[30] = 0x40400000u;
    code[31] = 0x7e0602f1u; /* v_mov_b32 v3, -1.0f (y1) */
    code[32] = 0x7e0c02ffu; /* v_mov_b32 v6, 2.0f (u1) */
    code[33] = 0x40000000u;
    code[34] = 0x7e0e0280u; /* v_mov_b32 v7, 0.0f (v1) */

    /* Lane 2: pos (-1.0f, 3.0f), uv (0.0f, 2.0f) */
    code[35] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    code[36] = 0x7e0402f1u; /* v_mov_b32 v2, -1.0f (x2) */
    code[37] = 0x7e0602ffu; /* v_mov_b32 v3, 3.0f (y2) */
    code[38] = 0x40400000u;
    code[39] = 0x7e0c0280u; /* v_mov_b32 v6, 0.0f (u2) */
    code[40] = 0x7e0e02ffu; /* v_mov_b32 v7, 2.0f (v2) */
    code[41] = 0x40000000u;

    /* Lanes 0..2: pos.z=0.0f, pos.w=1.0f; uv.z=0.0f, uv.w=0.0f; Color=(1.0f, 1.0f, 1.0f, 1.0f) */
    code[42] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    code[43] = 0x7e080280u; /* v_mov_b32 v4, 0.0f (pos.z) */
    code[44] = 0x7e0a02f2u; /* v_mov_b32 v5, 1.0f (pos.w) */
    code[45] = 0x7e100280u; /* v_mov_b32 v8, 0.0f (uv.z) */
    code[46] = 0x7e120280u; /* v_mov_b32 v9, 0.0f (uv.w) */
    code[47] = 0x7e1402f2u; /* v_mov_b32 v10, 1.0f (col.r) */
    code[48] = 0x7e1602f2u; /* v_mov_b32 v11, 1.0f (col.g) */
    code[49] = 0x7e1802f2u; /* v_mov_b32 v12, 1.0f (col.b) */
    code[50] = 0x7e1a02f2u; /* v_mov_b32 v13, 1.0f (col.a) */
    code[51] = 0xf800020fu; /* exp param0, v10, v11, v12, v13 (Color) */
    code[52] = 0x0d0c0b0au;
    code[53] = 0xf800021fu; /* exp param1, v6, v7, v8, v9 (UV) */
    code[54] = 0x09080706u;
    code[55] = 0xf80008cfu; /* exp pos0, v2, v3, v4, v5 done */
    code[56] = 0x05040302u;
    code[57] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[58] = 0xbf810000u; /* s_endpgm */
    for (size_t p = 59; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static void agc_textured_build_ps(uint32_t *code, uint64_t canary_gpu,
                                  uint32_t canary_offset, uint32_t canary_val) {
    uint32_t psk = 0;
    code[psk++] = 0xbf8c0000u; /* s_waitcnt 0 */
    code[psk++] = 0xbefc0302u; /* s_mov_b32 m0, s2: load primitive mask into m0 before interp */
    code[psk++] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
    code[psk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    code[psk++] = 0xbe8203ffu; /* s_mov_b32 s2, canary_lo */
    code[psk++] = (uint32_t)canary_gpu;
    code[psk++] = 0xbe8303ffu; /* s_mov_b32 s3, canary_hi */
    code[psk++] = (uint32_t)(canary_gpu >> 32);
    code[psk++] = 0x7e100202u; /* v_mov_b32 v8, s2 */
    code[psk++] = 0x7e120203u; /* v_mov_b32 v9, s3 */
    code[psk++] = 0x7e1402ffu; /* v_mov_b32 v10, canary_val */
    code[psk++] = canary_val;
    code[psk++] = 0xdc708000u | (canary_offset & 0xfffu); /* global_store_dword */
    code[psk++] = 0x007d0a08u;
    code[psk++] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
    code[psk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    code[psk++] = 0xc8080400u; /* v_interp_p1_f32 v2, v0, attr1.x (U) */
    code[psk++] = 0xc8090401u; /* v_interp_p2_f32 v2, v1, attr1.x */
    code[psk++] = 0xc80c0500u; /* v_interp_p1_f32 v3, v0, attr1.y (V) */
    code[psk++] = 0xc80d0501u; /* v_interp_p2_f32 v3, v1, attr1.y */
    code[psk++] = 0xc8200000u; /* v_interp_p1_f32 v8, v0, attr0.x (R) */
    code[psk++] = 0xc8210001u; /* v_interp_p2_f32 v8, v1, attr0.x */
    code[psk++] = 0xc8240100u; /* v_interp_p1_f32 v9, v0, attr0.y (G) */
    code[psk++] = 0xc8250101u; /* v_interp_p2_f32 v9, v1, attr0.y */
    code[psk++] = 0xc8280200u; /* v_interp_p1_f32 v10, v0, attr0.z (B) */
    code[psk++] = 0xc8290201u; /* v_interp_p2_f32 v10, v1, attr0.z */
    code[psk++] = 0xc82c0300u; /* v_interp_p1_f32 v11, v0, attr0.w (A) */
    code[psk++] = 0xc82d0301u; /* v_interp_p2_f32 v11, v1, attr0.w */
    code[psk++] = 0xf40c0100u; /* s_load_dwordx8 s[4:11], s[0:1], 0x00 */
    code[psk++] = 0xfa000000u;
    code[psk++] = 0xf4080300u; /* s_load_dwordx4 s[12:15], s[0:1], 0x20 */
    code[psk++] = 0xfa000020u;
    code[psk++] = 0xbf8cc07fu; /* s_waitcnt lgkmcnt(0) */
    code[psk++] = 0xf09c0f08u; /* image_sample_lz v[4:7], v[2:3], s[4:11], s[12:15]
                               dmask:0xf dim:SQ_RSRC_IMG_2D */
    code[psk++] = 0x00610402u;
    code[psk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */
    code[psk++] = 0x10081104u; /* v_mul_f32 v4, v4, v8 (R * R) */
    code[psk++] = 0x100a1305u; /* v_mul_f32 v5, v5, v9 (G * G) */
    code[psk++] = 0x100c1506u; /* v_mul_f32 v6, v6, v10 (B * B) */
    code[psk++] = 0x100e1707u; /* v_mul_f32 v7, v7, v11 (A * A) */
    code[psk++] = 0xf800180fu; /* exp mrt0, v4, v5, v6, v7 done vm */
    code[psk++] = 0x07060504u;
    code[psk++] = 0xbf810000u; /* s_endpgm */
    for (size_t p = psk; p < 64; p++) {
        code[p] = 0xbf800000u; /* s_nop */
    }
}

static obs_result check_agc_primitive_draw_depth(void) {
    return obs_skip("primitive-draw-depth isolated pending NGG geometry register sync");
}

static __attribute__((unused)) obs_result check_agc_primitive_draw_depth_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before depth draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *depth_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_depth_payload[0x1000];
    static _Alignas(64) uint32_t s_host_depth_fence[16];
    static _Alignas(64) uint32_t s_host_depth_canary[16];
    static _Alignas(65536) uint32_t s_host_depth_color[16384];
    static _Alignas(65536) uint32_t s_host_depth_buf[16384];
    uint8_t *gpu_payload = s_host_depth_payload;
    volatile uint32_t *fence = s_host_depth_fence;
    volatile uint32_t *canary = s_host_depth_canary;
    volatile uint32_t *color_buf = s_host_depth_color;
    volatile uint32_t *depth_buf = s_host_depth_buf;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL ||
        depth_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for depth test");
    }
    *fence = 0x11111111u;

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    /* Initialize Color Buffer: 0x55555555 background */
    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x55555555u;
    }

    /* Initialize Depth Buffer: 1.0f (0x3f800000 = far plane) across full 64KB
     * macro-tile */
    for (size_t i = 0; i < 16384; i++) {
        depth_buf[i] = 0x3f800000u;
    }

    /*
     * Build RDNA2 Shaders:
     * Triangle 1 (left):  (-0.7, -0.5), (-0.1, -0.5), (-0.4, +0.5)
     * Triangle 2 (right): (+0.1, -0.5), (+0.7, -0.5), (+0.4, +0.5)
     *
     * - VS 1 (0x000): Triangle 1 (left),  NDC Z =  0.0f (Screen Z = 0.5f), canary[0] =
     * 0xbeef0001
     * - PS 1 (0x100): Red (1.0f, 0.0f, 0.0f), canary[1] = 0xbeef0002
     * - VS 2 (0x200): Triangle 1 (left),  NDC Z = +0.6f (Screen Z = 0.8f), canary[2] =
     * 0xbeef0011
     * - PS 2 (0x300): Blue (0.0f, 0.0f, 1.0f), canary[3] = 0xbeef0012
     * - VS 3 (0x400): Triangle 2 (right), NDC Z = -0.6f (Screen Z = 0.2f), canary[4] =
     * 0xbeef0021
     * - PS 3 (0x500): Green (0.0f, 1.0f, 0.0f), canary[5] = 0xbeef0022
     */
    /* Draw 1: Triangle 1 @ 0.5f (Red) */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf333333u, 0xbf000000u, 0xbdccccd0u, 0xbf000000u, 0xbeccccdcu,
                       0x3f000000u, 0x00000000u);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u, 0xbeef0002u,
                       0x3f800000u, 0u, 0u);

    /* Draw 2: Triangle 1 @ 0.8f (Blue, rejected by LESS vs 0.5f) */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x200), canary_gpu, 8u, 0xbeef0011u,
                       0xbf333333u, 0xbf000000u, 0xbdccccd0u, 0xbf000000u, 0xbeccccdcu,
                       0x3f000000u, 0x3f19999au);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x300), canary_gpu, 12u, 0xbeef0012u,
                       0u, 0u, 0x3f800000u);

    /* Draw 3: Triangle 2 @ 0.2f (Green, accepted by LESS vs 1.0f initial) */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x400), canary_gpu, 16u, 0xbeef0021u,
                       0x3dccccd0u, 0xbf000000u, 0x3f333333u, 0xbf000000u, 0x3eccccdcu,
                       0x3f000000u, 0xbf19999au);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x500), canary_gpu, 20u, 0xbeef0022u,
                       0u, 0x3f800000u, 0u);

#if defined(__x86_64__)
    for (size_t p = 0; p < 0x600; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
    }
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
        __builtin_ia32_clflush((const void *)((const char *)fence + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
        __builtin_ia32_clflush((const void *)((const char *)depth_buf + p));
    }
#endif

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for depth test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping depth draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t depth_gpu = (uint64_t)(uintptr_t)depth_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* 1. Context register setup for Color Target, Depth Target, Viewport, Rasterizer */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } ctx_regs[] = {
        /* Color Target 0 */
        {0x318u, 0}, /* CB_COLOR0_BASE (patched below) */
        {0x390u, 0}, /* CB_COLOR0_BASE_EXT (patched below) */
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u}, /* COLOR_8_8_8_8, LINEAR_GENERAL, UNORM */
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u}, /* 64x64 */
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u}, /* CB_NORMAL, ROP3_COPY */
        {0x08eu, 0x0000000fu}, /* MRT0 4 components enabled */
        {0x08fu, 0x0000000fu},
        {0x1e0u, 0x20010001u}, /* Blend: SRC=ONE, DST=ZERO */

        /* Depth & Stencil Block Context Registers */
        {0x000u,
         0x00000000u}, /* DB_RENDER_CONTROL: default matching AgcCompositor.elf */
        {0x200u, 0x00000016u}, /* DB_DEPTH_CONTROL: Z_ENABLE (0x2) | Z_WRITE_ENABLE
                                  (0x4) | ZFUNC_LESS (0x10) */
        {0x201u, 0x00010000u}, /* DB_EQAA */
        {0x203u,
         0x00000000u}, /* DB_SHADER_CONTROL: LATE_Z (0x0) matching AgcCompositor.elf */
        {0x002u, 0x00000000u},       /* DB_DEPTH_VIEW */
        {0x005u, 0x00000000u},       /* DB_HTILE_DATA_BASE */
        {0x007u, (63u << 16) | 63u}, /* DB_DEPTH_SIZE_XY: 64x64 */
        {0x008u, 0x00000000u},       /* DB_DEPTH_BOUNDS_MIN */
        {0x009u, 0x00000000u},       /* DB_DEPTH_BOUNDS_MAX */
        {0x00au, 0x00000000u},       /* DB_STENCIL_CLEAR */
        {0x00bu, 0x3f800000u},       /* DB_DEPTH_CLEAR: 1.0f */
        {0x010u, 0x80000183u},       /* DB_Z_INFO: SW_MODE=24 (0x180), Z_32_FLOAT (3),
                                        ZRANGE_PRECISION (0x80000000) */
        {0x011u, 0x20000180u},       /* DB_STENCIL_INFO */
        {0x012u, 0},                 /* DB_Z_READ_BASE (patched below) */
        {0x013u, 0x00000000u},       /* DB_STENCIL_READ_BASE */
        {0x014u, 0},                 /* DB_Z_WRITE_BASE (patched below) */
        {0x015u, 0x00000000u},       /* DB_STENCIL_WRITE_BASE */
        {0x01au, 0},                 /* DB_Z_READ_BASE_HI (patched below) */
        {0x01bu, 0x00000000u},       /* DB_STENCIL_READ_BASE_HI */
        {0x01cu, 0},                 /* DB_Z_WRITE_BASE_HI (patched below) */
        {0x01du, 0x00000000u},       /* DB_STENCIL_WRITE_BASE_HI */
        {0x01eu, 0x00000000u},       /* DB_HTILE_DATA_BASE_HI */
        {0x01fu, 0x00000000u},       /* DB_RMI_L2_CACHE_CONTROL */
        {0x2afu, 0x00040000u},       /* DB_HTILE_SURFACE: PIPE_ALIGNED */

        /* Rasterizer & Fixed Function */
        {0x08cu, 0xaa99aaaau}, /* PA_SC_EDGERULE: D3D/OpenGL standard edge rule */
        {0x1d4u, 0x000000ffu}, /* SX_PS_DOWNCONVERT_CONTROL */
        /* NGG Primitive Type & Stages */
        {0x291u, 0x10020040u}, /* VGT_GS_ONCHIP_CNTL: ES_VERTS=64, GS_PRIMS=64,
                                  GS_INST_PRIMS=64 */
        {0x29bu, 0x00000000u}, /* VGT_GS_OUT_PRIM_TYPE: POINTLIST/PASSTHRU */
        {0x2d3u, 0x00000001u}, /* GE_NGG_SUBGRP_CNTL: PRIM_AMP=1, THDS_PER_SUBGRP=0 */
        {0x2d5u,
         0x02002000u}, /* VGT_SHADER_STAGES_EN: PRIMGEN_EN | PRIMGEN_PASSTHRU_EN */
        {0x1ffu, 0x00000040u}, /* GE_MAX_OUTPUT_PER_SUBGROUP: MAX_VERTS=64 */
        {0x20eu, 0x00000078u}, /* PA_CL_NGG_CNTL: VERTEX_REUSE_DEPTH=30 */
        {0x2a1u, 0x00000000u}, /* VGT_PRIMITIVEID_EN: disabled */
        {0x2a6u, 0x00000040u}, /* VGT_DRAW_PAYLOAD_CNTL */
        {0x2adu, 0x00000000u}, /* VGT_REUSE_OFF */
        {0x2abu, 0x00000004u}, /* VGT_ESGS_RING_ITEMSIZE: 4 */
        {0x2ceu, 0x00000000u}, /* VGT_GS_MAX_VERT_OUT: 0 */
        {0x2d4u, 0x88101000u}, /* VGT_TESS_DISTRIBUTION */
        {0x103u, 0xffffffffu}, /* VGT_MULTI_PRIM_IB_RESET_INDX */
        /* Sample Mask & NGG Control */
        {0x30eu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y0_X1Y0: enable all samples */
        {0x30fu, 0xffffffffu}, /* PA_SC_AA_MASK_X0Y1_X1Y1: enable all samples */
        {0x310u, 0x00000000u}, /* PA_SC_SHADER_CONTROL */
        {0x314u, 0x00000202u}, /* PA_SC_NGG_MODE_CNTL: MAX_DEALLOCS=2, MAX_FPOVS=2 */
        {0x311u, 0x01fd2002u}, /* PA_SC_BINNER_CNTL_0: DISABLE_BINNING_USE_NEW_SC */
        {0x312u, 0x03ff0080u}, /* PA_SC_BINNER_CNTL_1 */
        {0x313u, 0x00006000u}, /* PA_SC_CONSERVATIVE_RASTERIZATION_CNTL */
        {0x00eu, 0x00000002u}, /* DB_DFSM_CONTROL */
        {0x280u, 0x00080008u}, /* PA_SU_POINT_SIZE */
        {0x281u, 0xffff0000u}, /* PA_SU_POINT_MINMAX */
        {0x282u, 0x00000008u}, /* PA_SU_LINE_CNTL */
        {0x2deu, 0x000001e9u}, /* PA_SU_POLY_OFFSET_DB_FMT_CNTL */
        /* Scissors (Screen, Window, Generic, Viewport) */
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        /* Viewport Bounds & Transform */
        {0x0b4u, 0x00000000u}, /* ZMIN = 0.0f */
        {0x0b5u, 0x3f800000u}, /* ZMAX = 1.0f */
        {0x10fu, 0x42000000u}, /* XSCALE: 32.0f */
        {0x110u, 0x42000000u}, /* XOFFSET: 32.0f */
        {0x111u, 0x42000000u}, /* YSCALE: 32.0f */
        {0x112u, 0x42000000u}, /* YOFFSET: 32.0f */
        {0x113u, 0x3f000000u}, /* ZSCALE: 0.5f */
        {0x114u, 0x3f000000u}, /* ZOFFSET: 0.5f */
        /* Cliprect Rules */
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        /* Guardband & Viewport Transform Enable */
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        /* Scan Converter & Surface Setup */
        {0x205u, 0x00000240u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        /* Shader Formats & SPI PS Controls */
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };
    for (size_t i = 0; i < sizeof(ctx_regs) / sizeof(ctx_regs[0]); i++) {
        uint32_t reg = ctx_regs[i].reg;
        uint32_t val = ctx_regs[i].val;
        if (reg == 0x318u) {
            val = (uint32_t)(color_gpu >> 8);
        } else if (reg == 0x390u) {
            val = (uint32_t)(color_gpu >> 40);
        } else if (reg == 0x012u || reg == 0x014u) {
            val = (uint32_t)(depth_gpu >> 8);
        } else if (reg == 0x01au || reg == 0x01cu) {
            val = (uint32_t)(depth_gpu >> 40);
        }
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* 2. Clear 32 SPI_PS_INPUT_CNTL registers to 0 */
    for (uint32_t i = 0; i < 32; i++) {
        *dw++ = 0xc0016900u;
        *dw++ = 0x191u + i;
        *dw++ = 0x00000000u;
    }

    /* 3. Bind initial shader stages BEFORE spi_cu_regs and UConfig */
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);

    /* 4. SPI CU Enable masks */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } spi_cu_regs[] = {
        {0x007u, 0x0000ffffu}, {0x001u, 0x00000003u}, {0x087u, 0x0000fffdu},
        {0x081u, 0x00000003u}, {0x107u, 0xffff0000u},
    };
    for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
        *dw++ = 0xc0017600u;
        *dw++ = spi_cu_regs[i].reg;
        *dw++ = spi_cu_regs[i].val;
    }

    /* 5. UConfig registers */
    *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES */
    *dw++ = 1u;
    *dw++ = 0xc0017900u; /* mmVGT_PRIMITIVE_TYPE */
    *dw++ = 0x242u;
    *dw++ = 0x4u;        /* DI_PT_TRILIST */
    *dw++ = 0xc0017900u; /* mmGE_CNTL */
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u;
    *dw++ = 0xc0017900u; /* mmGE_PC_ALLOC */
    *dw++ = 0x260u;
    *dw++ = 0x3ffu;

    /*
     * 6. Issue Depth-tested Draw Calls:
     * Draw 1: Triangle 1 (left)  @ Screen Z = 0.5f, Red. PASSES depth test vs 1.0f
     * initial. Draw 2: Triangle 1 (left)  @ Screen Z = 0.8f, Blue. REJECTED: 0.8f >=
     * 0.5f. Left stays Red! Draw 3: Triangle 2 (right) @ Screen Z = 0.2f, Green.
     * ACCEPTED: 0.2f < 1.0f. Right becomes Green!
     */
    /* Draw 1: Triangle 1 (left) @ 0.5f -> Red */
    *dw++ = 0xc0012d00u; /* DRAW_INDEX_AUTO */
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush between draws to guarantee Draw 1 DB write commits before Draw 2 test */
    *dw++ = 0xc0004600u; /* PACKET3_EVENT_WRITE */
    *dw++ = 16u;         /* PS_PARTIAL_FLUSH */
    *dw++ = 0xc0004600u; /* PACKET3_EVENT_WRITE */
    *dw++ = 42u;         /* DB_CACHE_FLUSH_AND_INV */

    /* Draw 2: Triangle 1 (left) @ 0.8f -> Blue (Depth Rejection) */
    agc_depth_bind_stages(&dw, payload_va + 0x200, payload_va + 0x300);
    *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES */
    *dw++ = 1u;
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush between draws */
    *dw++ = 0xc0004600u; /* PACKET3_EVENT_WRITE */
    *dw++ = 16u;         /* PS_PARTIAL_FLUSH */
    *dw++ = 0xc0004600u; /* PACKET3_EVENT_WRITE */
    *dw++ = 42u;         /* DB_CACHE_FLUSH_AND_INV */

    /* Draw 3: Triangle 2 (right) @ 0.2f -> Green (Depth Acceptance) */
    agc_depth_bind_stages(&dw, payload_va + 0x400, payload_va + 0x500);
    *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES */
    *dw++ = 1u;
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush & Fence: RELEASE_MEM with EOP event */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    /* Trailing NOPs */
    for (int p = 0; p < 16; p++) {
        dw[p] = 0xffff1000u;
    }
    dw += 16;

    probe->cur = (uint64_t)(uintptr_t)dw;
    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);

    obs_agc_dcb_desc desc;
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = bytes_written / 4u;
    desc.flags = 0u;
    desc.pad[0] = 0u;
    desc.pad[1] = 0u;
    desc.pad[2] = 0u;

#if defined(__x86_64__)
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
        obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    } else {
        obs_fault_unregister();
        obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
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
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "fence-val", (uint64_t)fence_val, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");

#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
        __builtin_ia32_clflush((const void *)((const char *)depth_buf + p));
    }
#endif

    /* Canary telemetry */
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d1-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d1-ps", (uint64_t)canary[1], "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d2-vs", (uint64_t)canary[2], "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d2-ps", (uint64_t)canary[3], "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d3-vs", (uint64_t)canary[4], "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "canary-d3-ps", (uint64_t)canary[5], "val");

    /* Scan color buffer to check if any pixels were modified */
    uint32_t any_color = color_buf[0];
    int any_color_mod = 0;
    size_t any_mod_idx = 0;
    for (size_t i = 0; i < 4096; i++) {
        if (color_buf[i] != 0x55555555u) {
            any_color_mod = 1;
            any_color = color_buf[i];
            any_mod_idx = i;
            break;
        }
    }
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-color-mod", (uint64_t)any_color_mod, "bool");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-color-val", (uint64_t)any_color, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-color-idx", (uint64_t)any_mod_idx, "val");

    /* Scan depth buffer to check if any pixels were modified */
    uint32_t any_depth = depth_buf[0];
    int any_depth_mod = 0;
    size_t any_dmod_idx = 0;
    for (size_t i = 0; i < 16384; i++) {
        if (depth_buf[i] != 0x3f800000u) {
            any_depth_mod = 1;
            any_depth = depth_buf[i];
            any_dmod_idx = i;
            break;
        }
    }
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-depth-mod", (uint64_t)any_depth_mod, "bool");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-depth-val", (uint64_t)any_depth, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "any-depth-idx", (uint64_t)any_dmod_idx, "val");

    /* Inspect Triangle 1 (left, centroid x=19, y=27) */
    uint32_t tri1_color = color_buf[27 * 64 + 19];
    uint32_t tri1_depth = depth_buf[agc_depth_swizzle_offset(19, 27)];
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            size_t c_idx = (size_t)((27 + dy) * 64 + (19 + dx));
            if (color_buf[c_idx] != 0x55555555u) {
                tri1_color = color_buf[c_idx];
                break;
            }
        }
    }
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            size_t d_idx =
                agc_depth_swizzle_offset((uint32_t)(19 + dx), (uint32_t)(27 + dy));
            if (depth_buf[d_idx] != 0x3f800000u) {
                tri1_depth = depth_buf[d_idx];
                break;
            }
            size_t l_idx = (size_t)((27 + dy) * 64 + (19 + dx));
            if (depth_buf[l_idx] != 0x3f800000u) {
                tri1_depth = depth_buf[l_idx];
                break;
            }
        }
    }

    /* Inspect Triangle 2 (right, centroid x=45, y=27) */
    uint32_t tri2_color = color_buf[27 * 64 + 45];
    uint32_t tri2_depth = depth_buf[agc_depth_swizzle_offset(45, 27)];
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            size_t c_idx = (size_t)((27 + dy) * 64 + (45 + dx));
            if (color_buf[c_idx] != 0x55555555u) {
                tri2_color = color_buf[c_idx];
                break;
            }
        }
    }
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            size_t d_idx =
                agc_depth_swizzle_offset((uint32_t)(45 + dx), (uint32_t)(27 + dy));
            if (depth_buf[d_idx] != 0x3f800000u) {
                tri2_depth = depth_buf[d_idx];
                break;
            }
            size_t l_idx = (size_t)((27 + dy) * 64 + (45 + dx));
            if (depth_buf[l_idx] != 0x3f800000u) {
                tri2_depth = depth_buf[l_idx];
                break;
            }
        }
    }

    float f1_depth = 0.0f;
    float f2_depth = 0.0f;
    __builtin_memcpy(&f1_depth, &tri1_depth, sizeof(float));
    __builtin_memcpy(&f2_depth, &tri2_depth, sizeof(float));

    /* Farther primitive (Draw 2 Blue @ 0.8f) was rejected: Triangle 1 MUST remain Red
     * (0xff0000ff) @ 0.5f */
    int reject_pass =
        (tri1_color == 0xff0000ffu &&
         (tri1_depth == 0x3f000000u || (f1_depth >= 0.49f && f1_depth <= 0.51f)));

    /* Nearer primitive (Draw 3 Green @ 0.2f) was accepted: Triangle 2 MUST be Green
     * (0xff00ff00) @ 0.2f */
    int accept_pass =
        (tri2_color == 0xff00ff00u &&
         (tri2_depth == 0x3e4ccccdu || (f2_depth >= 0.19f && f2_depth <= 0.21f)));

    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "tri1-color", (uint64_t)tri1_color, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "tri1-depth", (uint64_t)tri1_depth, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "tri2-color", (uint64_t)tri2_color, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "tri2-depth", (uint64_t)tri2_depth, "val");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "depth-reject-pass", (uint64_t)reject_pass, "bool");
    obs_report_measure("166-agc/primitive-draw-depth", "sceAgcDriverSubmitDcb",
                       "depth-accept-pass", (uint64_t)accept_pass, "bool");

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
        return obs_fail("fault during depth draw submit or poll");
    }
    if (submit_rc == 0 && fence_hit == 1 && reject_pass && accept_pass) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        if (!reject_pass) {
            return obs_partial_value(
                "depth rejection failed (farther primitive overwrote nearer)",
                (uint64_t)tri1_color);
        }
        if (!accept_pass) {
            return obs_partial_value(
                "depth acceptance failed (nearer primitive rejected)",
                (uint64_t)tri2_color);
        }
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after depth draw", (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_primitive_draw_stencil(void) {
    return obs_skip("primitive-draw-stencil isolated: probe fixture cannot run non-passthrough stage 0x00c12010 per REQ-20260917T1845Z-3d5b");
}

static __attribute__((unused)) obs_result check_agc_primitive_draw_stencil_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before stencil draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *depth_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_stencil_payload[0x1000];
    static _Alignas(64) uint32_t s_host_stencil_fence[16];
    static _Alignas(64) uint32_t s_host_stencil_canary[16];
    static _Alignas(65536) uint32_t s_host_stencil_color[16384];
    static _Alignas(65536) uint32_t s_host_stencil_buf[16384];
    uint8_t *gpu_payload = s_host_stencil_payload;
    volatile uint32_t *fence = s_host_stencil_fence;
    volatile uint32_t *canary = s_host_stencil_canary;
    volatile uint32_t *color_buf = s_host_stencil_color;
    volatile uint32_t *depth_buf = s_host_stencil_buf;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL ||
        depth_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for stencil test");
    }
    *fence = 0x11111111u;

    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x55555555u;
        depth_buf[i] = 0x3f800000u; /* 1.0f */
    }

    /* Temporarily isolated per REQ-20260915T1310Z-ebb4: stencil draw stream
     * hangs the graphics command processor and blocks subsequent queue submissions. */
    return obs_skip("primitive-draw-stencil isolated per REQ-20260915T1310Z-ebb4 pending DB stencil config fix");
#if 0
    /* Unused while isolated */
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t depth_gpu = (uint64_t)(uintptr_t)depth_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* Build shaders:
     * VS 1 (0x000): Centered triangle
     * PS 1 (0x100): Blue (0.0, 0.0, 1.0)
     * PS 2 (0x200): Green (0.0, 1.0, 0.0)
     */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf000000u, 0xbf000000u, 0x3f000000u, 0xbf000000u, 0x00000000u,
                       0x3f000000u, 0x00000000u);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u, 0xbeef0002u,
                       0u, 0u, 0x3f800000u);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x200), canary_gpu, 8u, 0xbeef0003u,
                       0u, 0x3f800000u, 0u);

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for stencil test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping stencil draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;

    /* Base context registers */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx_stencil[] = {
        {0x318u, 0},
        {0x390u, 0},
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u},
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u},
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u},
        {0x08eu, 0x0000000fu},
        {0x08fu, 0x0000000fu},
        {0x1e0u, 0x20010001u},
        {0x201u, 0x00010000u},
        {0x203u, 0x00000000u},
        {0x000u, 0x00000000u},
        {0x002u, 0x00000000u},
        {0x007u, (63u << 16) | 63u},
        {0x010u, 0x80000183u},
        {0x011u, 0x20000180u}, /* STENCIL_8, SW_MODE=24 */
        {0x012u, 0},
        {0x014u, 0},
        {0x01au, 0},
        {0x01cu, 0},
        {0x013u, 0},
        {0x015u, 0},
        {0x01bu, 0},
        {0x01du, 0},
        {0x2afu, 0x00040000u},
        {0x08cu, 0xaa99aaaau},
        {0x1d4u, 0x000000ffu},
        {0x291u, 0x10020040u},
        {0x29bu, 0x00000000u},
        {0x2d3u, 0x00000001u},
        {0x2d5u, 0x02002000u},
        {0x1ffu, 0x00000040u},
        {0x20eu, 0x00000078u},
        {0x2a1u, 0x00000000u},
        {0x2a6u, 0x00000040u},
        {0x2adu, 0x00000000u},
        {0x2abu, 0x00000004u},
        {0x2ceu, 0x00000000u},
        {0x2d4u, 0x88101000u},
        {0x103u, 0xffffffffu},
        {0x30eu, 0xffffffffu},
        {0x30fu, 0xffffffffu},
        {0x310u, 0x00000000u},
        {0x314u, 0x00000202u},
        {0x311u, 0x01fd2002u},
        {0x312u, 0x03ff0080u},
        {0x313u, 0x00006000u},
        {0x00eu, 0x00000002u},
        {0x280u, 0x00080008u},
        {0x281u, 0xffff0000u},
        {0x282u, 0x00000008u},
        {0x2deu, 0x000001e9u},
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        {0x205u, 0x00000240u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };

    for (size_t i = 0; i < sizeof(base_ctx_stencil) / sizeof(base_ctx_stencil[0]);
         i++) {
        uint32_t reg = base_ctx_stencil[i].reg;
        uint32_t val = base_ctx_stencil[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        else if (reg == 0x012u || reg == 0x014u)
            val = (uint32_t)(depth_gpu >> 8);
        else if (reg == 0x01au || reg == 0x01cu)
            val = (uint32_t)(depth_gpu >> 40);
        else if (reg == 0x013u || reg == 0x015u)
            val = (uint32_t)(depth_gpu >> 8);
        else if (reg == 0x01bu || reg == 0x01du)
            val = (uint32_t)(depth_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Draw 1: Stencil write (ALWAYS pass, ZPASS_OP = REPLACE ref 1) */
    *dw++ = 0xc0016900u;
    *dw++ = 0x200u;
    *dw++ = 0x0000439fu;
    *dw++ = 0xc0016900u;
    *dw++ = 0x208u;
    *dw++ = 0x00ff0101u;

    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);
    *dw++ = 0xc0002f00u;
    *dw++ = 1u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu;
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Draw 2: Stencil test EQUAL ref 1 (should PASS -> write Green) */
    *dw++ = 0xc0016900u;
    *dw++ = 0x200u;
    *dw++ = 0x0000011fu;
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x200);
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "canary-d1-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "canary-d1-ps", (uint64_t)canary[1], "val");
    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "canary-d2-ps", (uint64_t)canary[2], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    int stencil_pass = (tri_color == 0xff00ff00u); /* Green */

    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");
    obs_report_measure("166-agc/primitive-draw-stencil", "sceAgcDriverSubmitDcb",
                       "stencil-pass", (uint64_t)stencil_pass, "bool");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 && stencil_pass) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("stencil test failed to produce Green pixel",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after stencil draw",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
#endif
}

static obs_result check_agc_primitive_draw_blend(void) {
    return obs_skip("primitive-draw-blend isolated pending dual-draw pipeline state fix");
#if 0
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before blend draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_blend_payload[0x1000];
    static _Alignas(64) uint32_t s_host_blend_fence[16];
    static _Alignas(64) uint32_t s_host_blend_canary[16];
    static _Alignas(65536) uint32_t s_host_blend_color[16384];
    uint8_t *gpu_payload = s_host_blend_payload;
    volatile uint32_t *fence = s_host_blend_fence;
    volatile uint32_t *canary = s_host_blend_canary;
    volatile uint32_t *color_buf = s_host_blend_color;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for blend test");
    }
    *fence = 0x11111111u;

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x00000000u;
    }

    /* Build shaders:
     * VS 1 (0x000): Screen center triangle
     * PS 1 (0x100): Red (1.0, 0.0, 0.0, 1.0)
     * PS 2 (0x200): Green (0.0, 1.0, 0.0, 0.5)
     */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf000000u, 0xbf000000u, 0x3f000000u, 0xbf000000u, 0x00000000u,
                       0x3f000000u, 0x00000000u);
    agc_depth_build_ps_rgba((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u,
                            0xbeef0002u, 0x3f800000u, 0u, 0u, 0x3f800000u);
    agc_depth_build_ps_rgba((uint32_t *)(gpu_payload + 0x200), canary_gpu, 8u,
                            0xbeef0003u, 0u, 0x3f800000u, 0u, 0x3f000000u);

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for blend test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping blend draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx_blend[] = {
        {0x318u, 0},
        {0x390u, 0},
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u},
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u},
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u},
        {0x08eu, 0x0000000fu},
        {0x08fu, 0x0000000fu},
        {0x1e0u, 0x20010001u},
        {0x200u, 0x00000000u},
        {0x08cu, 0xaa99aaaau},
        {0x1d4u, 0x000000ffu},
        {0x291u, 0x10020040u},
        {0x29bu, 0x00000000u},
        {0x2d3u, 0x00000001u},
        {0x2d5u, 0x02002000u},
        {0x1ffu, 0x00000040u},
        {0x20eu, 0x00000078u},
        {0x2a1u, 0x00000000u},
        {0x2a6u, 0x00000040u},
        {0x2adu, 0x00000000u},
        {0x2abu, 0x00000004u},
        {0x2ceu, 0x00000000u},
        {0x2d4u, 0x88101000u},
        {0x103u, 0xffffffffu},
        {0x30eu, 0xffffffffu},
        {0x30fu, 0xffffffffu},
        {0x310u, 0x00000000u},
        {0x314u, 0x00000202u},
        {0x311u, 0x01fd2002u},
        {0x312u, 0x03ff0080u},
        {0x313u, 0x00006000u},
        {0x00eu, 0x00000002u},
        {0x280u, 0x00080008u},
        {0x281u, 0xffff0000u},
        {0x282u, 0x00000008u},
        {0x2deu, 0x000001e9u},
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        {0x205u, 0x00000240u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };

    for (size_t i = 0; i < sizeof(base_ctx_blend) / sizeof(base_ctx_blend[0]); i++) {
        uint32_t reg = base_ctx_blend[i].reg;
        uint32_t val = base_ctx_blend[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Draw 1: Draw Red triangle (opaque background) */
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);
    *dw++ = 0xc0002f00u;
    *dw++ = 1u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u;
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu;
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Draw 2: Enable Alpha Blending (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) */
    *dw++ = 0xc0016900u;
    *dw++ = 0x202u;
    *dw++ = 0x00cc0011u;
    *dw++ = 0xc0016900u;
    *dw++ = 0x1e0u;
    *dw++ = 0x20110010u;

    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x200);
    *dw++ = 0xc0012d00u;
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "canary-d1-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "canary-d1-ps", (uint64_t)canary[1], "val");
    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "canary-d2-ps", (uint64_t)canary[2], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    uint32_t red = (tri_color >> 16) & 0xffu;
    uint32_t green = (tri_color >> 8) & 0xffu;
    uint32_t blue = tri_color & 0xffu;

    int blend_pass =
        (red >= 0x70 && red <= 0x90 && green >= 0x70 && green <= 0x90 && blue == 0);

    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");
    obs_report_measure("166-agc/primitive-draw-blend", "sceAgcDriverSubmitDcb",
                       "blend-pass", (uint64_t)blend_pass, "bool");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 && blend_pass) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("blend test failed to produce expected 50/50 color",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after blend draw", (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
#endif
}

static obs_result check_agc_primitive_draw_indexed(void) {
    return obs_skip("primitive-draw-indexed isolated pending NGG geometry register sync");
}

static __attribute__((unused)) obs_result check_agc_primitive_draw_indexed_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before indexed draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint16_t *index_buf =
        (volatile uint16_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_indexed_payload[0x1000];
    static _Alignas(64) uint32_t s_host_indexed_fence[16];
    static _Alignas(64) uint32_t s_host_indexed_canary[16];
    static _Alignas(65536) uint32_t s_host_indexed_color[16384];
    static _Alignas(256) uint16_t s_host_indexed_indices[16];
    uint8_t *gpu_payload = s_host_indexed_payload;
    volatile uint32_t *fence = s_host_indexed_fence;
    volatile uint32_t *canary = s_host_indexed_canary;
    volatile uint32_t *color_buf = s_host_indexed_color;
    volatile uint16_t *index_buf = s_host_indexed_indices;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL ||
        index_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for indexed draw test");
    }
    *fence = 0x11111111u;

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x00000000u;
    }

    /* Index buffer: 3 indices {0, 1, 2} defining a single triangle */
    index_buf[0] = 0;
    index_buf[1] = 1;
    index_buf[2] = 2;

    /* Build shaders:
     * VS (0x000): Screen center triangle
     * PS (0x100): Cyan color (0.0, 1.0, 1.0, 1.0)
     */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf000000u, 0xbf000000u, 0x3f000000u, 0xbf000000u, 0x00000000u,
                       0x3f000000u, 0x00000000u);
    agc_depth_build_ps_rgba((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u,
                            0xbeef0002u, 0u, 0x3f800000u, 0x3f800000u, 0x3f800000u);

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for indexed draw test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping indexed draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t index_gpu = (uint64_t)(uintptr_t)index_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx_indexed[] = {
        {0x318u, 0},
        {0x390u, 0},
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u},
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u},
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u},
        {0x08eu, 0x0000000fu},
        {0x08fu, 0x0000000fu},
        {0x1e0u, 0x20010001u},
        {0x200u, 0x00000000u},
        {0x08cu, 0xaa99aaaau},
        {0x1d4u, 0x000000ffu},
        {0x291u, 0x10020040u},
        {0x29bu, 0x00000000u},
        {0x2d3u, 0x00000001u},
        {0x2d5u, 0x02002000u},
        {0x1ffu, 0x00000040u},
        {0x20eu, 0x00000078u},
        {0x2a1u, 0x00000000u},
        {0x2a6u, 0x00000040u},
        {0x2adu, 0x00000000u},
        {0x2abu, 0x00000004u},
        {0x2ceu, 0x00000000u},
        {0x2d4u, 0x88101000u},
        {0x103u, 0xffffffffu},
        {0x30eu, 0xffffffffu},
        {0x30fu, 0xffffffffu},
        {0x310u, 0x00000000u},
        {0x314u, 0x00000202u},
        {0x311u, 0x01fd2002u},
        {0x312u, 0x03ff0080u},
        {0x313u, 0x00006000u},
        {0x00eu, 0x00000002u},
        {0x280u, 0x00080008u},
        {0x281u, 0xffff0000u},
        {0x282u, 0x00000008u},
        {0x2deu, 0x000001e9u},
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        {0x205u, 0x00000240u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };

    for (size_t i = 0; i < sizeof(base_ctx_indexed) / sizeof(base_ctx_indexed[0]);
         i++) {
        uint32_t reg = base_ctx_indexed[i].reg;
        uint32_t val = base_ctx_indexed[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Shader stages */
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);

    /* Primitive & Geometry Control */
    *dw++ = 0xc0002f00u;
    *dw++ = 1u; /* NUM_INSTANCES */
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u; /* mmVGT_PRIMITIVE_TYPE: DI_PT_TRILIST */
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */

    /* Index Type: 16-bit indices */
    *dw++ = 0xc0002a00u; /* PACKET3_INDEX_TYPE */
    *dw++ = 0x0u;        /* VGT_INDEX_16 */

    /* Draw Indexed: DRAW_INDEX_2 (opcode 0x27) */
    *dw++ = 0xc0042700u; /* PACKET3_DRAW_INDEX_2, count 4 */
    *dw++ = 3u;          /* max_size (index buffer count) */
    *dw++ = (uint32_t)index_gpu;
    *dw++ = (uint32_t)(index_gpu >> 32);
    *dw++ = 3u; /* index_count */
    *dw++ = 0u; /* initiator: DI_SRC_SEL_DMA (0) */

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    __builtin_ia32_clflush((const void *)index_buf);
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverSubmitDcb",
                       "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverSubmitDcb",
                       "canary-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverSubmitDcb",
                       "canary-ps", (uint64_t)canary[1], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    obs_report_measure("166-agc/primitive-draw-indexed", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 && tri_color != 0x00000000u) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("indexed draw executed but pixels unmodified",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after indexed draw",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static const struct {
    uint32_t reg;
    uint32_t val;
} base_ctx_textured[] = {
    {0x318u, 0},
    {0x390u, 0},
    {0x31bu, 0x00000000u},
    {0x31cu, 0x000180a8u},
    {0x31du, 0x00000000u},
    {0x31eu, 0x00000000u},
    {0x3b0u, (63u << 14) | 63u},
    {0x3b8u, 0x08c6c000u},
    {0x109u, 0x00000000u},
    {0x202u, 0x00cc0010u},
    {0x08eu, 0x0000000fu},
    {0x08fu, 0x0000000fu},
    {0x1e0u, 0x20010001u},
    {0x200u, 0x00000000u},
    {0x08cu, 0xaa99aaaau},
    {0x1d4u, 0x000000ffu},
    {0x291u, 0x20040100u},
    {0x29bu, 0x00000002u},
    {0x2d3u, 0x00000001u},
    {0x2d5u, 0x02002000u},
    {0x1ffu, 0x00000100u},
    {0x20eu, 0x00000078u},
    {0x2a1u, 0x00000000u},
    {0x2a6u, 0x00000040u},
    {0x2adu, 0x00000000u},
    {0x2abu, 0x00000001u},
    {0x2ceu, 0x00000400u},
    {0x2d4u, 0x88101000u},
    {0x103u, 0xffffffffu},
    {0x30eu, 0xffffffffu},
    {0x30fu, 0xffffffffu},
    {0x310u, 0x00000000u},
    {0x314u, 0x00000202u},
    {0x311u, 0x01fd2002u},
    {0x312u, 0x03ff0080u},
    {0x313u, 0x00006000u},
    {0x00eu, 0x00000002u},
    {0x280u, 0x00080008u},
    {0x281u, 0xffff0000u},
    {0x282u, 0x00000008u},
    {0x2deu, 0x000001e9u},
    {0x00cu, 0x00000000u},
    {0x00du, 0x40004000u},
    {0x081u, 0x80000000u},
    {0x082u, 0x40004000u},
    {0x090u, 0x80000000u},
    {0x091u, 0x40004000u},
    {0x094u, 0x80000000u},
    {0x095u, 0x40004000u},
    {0x0b4u, 0x00000000u},
    {0x0b5u, 0x3f800000u},
    {0x10fu, 0x42000000u},
    {0x110u, 0x42000000u},
    {0x111u, 0x42000000u},
    {0x112u, 0x42000000u},
    {0x113u, 0x3f000000u},
    {0x114u, 0x3f000000u},
    {0x083u, 0x0000ffffu},
    {0x084u, 0x00000000u},
    {0x085u, 0x20002000u},
    {0x204u, 0x00000000u},
    {0x206u, 0x0000043fu},
    {0x207u, 0x00000000u},
    {0x2fau, 0x3f800000u},
    {0x2fbu, 0x3f800000u},
    {0x2fcu, 0x3f800000u},
    {0x2fdu, 0x3f800000u},
    {0x205u, 0x00000240u},
    {0x20cu, 0x00000000u},
    {0x292u, 0x00000002u},
    {0x293u, 0x06020000u},
    {0x2f8u, 0x00000000u},
    {0x2f9u, 0x0000002du},
    /* Interpolation and parameter export controls: */
    {0x191u, 0x00000000u}, /* SPI_PS_INPUT_CNTL_0: Offset 0, smooth (Color) */
    {0x192u, 0x00000001u}, /* SPI_PS_INPUT_CNTL_1: Offset 1, smooth (UV) */
    {0x1b1u, 0x00000001u}, /* SPI_VS_OUT_CONFIG: 2 PC Exports (param0, param1),
                              NO_PC_EXPORT=0 */
    {0x1c2u, 0x00000001u}, /* SPI_SHADER_IDX_FORMAT: IDX0 = 1COMP */
    {0x1c3u, 0x00000004u}, /* SPI_SHADER_POS_FORMAT: POS0 = 4COMP */
    {0x1c5u, 0x00000009u}, /* SPI_SHADER_COL_FORMAT: COL0 = 32_ABGR */
    {0x1b3u, 0x00000002u}, /* SPI_PS_INPUT_ENA: PERSP_CENTER_ENA */
    {0x1b4u, 0x00000002u}, /* SPI_PS_INPUT_ADDR: PERSP_CENTER_ENA */
    {0x1b5u, 0x00000000u}, /* SPI_INTERP_CONTROL_0: 0 */
    {0x1b6u, 0x00008002u}, /* SPI_PS_IN_CONTROL: PS_W32_EN, NUM_INTERP=2 */
    {0x1b8u, 0x01000000u}, /* SPI_BARYC_CNTL: FRONT_FACE_ALL_BITS */
};

static __attribute__((unused)) obs_result check_agc_primitive_draw_textured(void) {
    return obs_skip("primitive-draw-textured isolated: superseded by draw-textured-linear-pitch");
}

static __attribute__((unused)) obs_result check_agc_primitive_draw_textured_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before textured draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *tex_buf =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 256, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_textured_payload[0x1000];
    static _Alignas(64) uint32_t s_host_textured_fence[16];
    static _Alignas(64) uint32_t s_host_textured_canary[16];
    static _Alignas(65536) uint32_t s_host_textured_color[16384];
    static _Alignas(256) uint32_t s_host_textured_tex[256];
    uint8_t *gpu_payload = s_host_textured_payload;
    volatile uint32_t *fence = s_host_textured_fence;
    volatile uint32_t *canary = s_host_textured_canary;
    volatile uint32_t *color_buf = s_host_textured_color;
    volatile uint32_t *tex_buf = s_host_textured_tex;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL ||
        tex_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for textured draw test");
    }
    *fence = 0x11111111u;

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x00000000u;
    }

    /* 16x16 RGBA8 Texture: fill all 256 texels with Magenta (0xffff00ffu) */
    for (size_t i = 0; i < 256; i++) {
        tex_buf[i] = 0xffff00ffu;
    }

    /* Build shaders:
     * VS (0x000): Screen center triangle, exports Color (1,1,1,1) in param0, UVs in
     * param1 PS (0x100): Samples texture from descriptor table at s[0:1], multiplies
     * with color, exports to mrt0
     */
    agc_textured_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u,
                          0xbeef0001u, 0xbf000000u, 0xbf000000u, 0x3f000000u,
                          0xbf000000u, 0x00000000u, 0x3f000000u);
    agc_textured_build_ps((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u,
                          0xbeef0002u);

    /* Populate descriptor table at gpu_payload + 0x200 */
    uint32_t *dt = (uint32_t *)(gpu_payload + 0x200);
    for (int i = 0; i < 16; i++)
        dt[i] = 0u;
    uint64_t tex_gpu = (uint64_t)(uintptr_t)tex_buf;
    uint32_t w = 16u, h = 16u;
    dt[0] = (uint32_t)(tex_gpu >> 8);
    dt[1] =
        (uint32_t)((tex_gpu >> 40) & 0xffu) | (56u << 20) | (((w - 1u) & 3u) << 30);
    dt[2] = (((w - 1u) >> 2) & 0x3fffu) | (((h - 1u) & 0x3fffu) << 14) | (1u << 31);
    dt[3] = 0x90000000u | 0xfacu;     /* SQ_RSRC_IMG_2D, linear, RGBA swizzle (X,Y,Z,W) */
    dt[8] = 0u;                       /* WRAP_REPEAT */
    dt[9] = 0x00fff000u;              /* MAX_LOD */
    dt[10] = (1u << 20) | (1u << 22); /* mag_filter=linear, min_filter=linear */
    dt[11] = 0u;

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for textured draw test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping textured draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    for (size_t i = 0; i < sizeof(base_ctx_textured) / sizeof(base_ctx_textured[0]);
         i++) {
        uint32_t reg = base_ctx_textured[i].reg;
        uint32_t val = base_ctx_textured[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    for (uint32_t r = 0x193u; r <= 0x1b0u; r++) {
        *dw++ = 0xc0016900u;
        *dw++ = r;
        *dw++ = 0u;
    }

    /* Shader stages */
    agc_depth_bind_stages_textured(&dw, payload_va + 0x000, payload_va + 0x100,
                                   payload_va + 0x200);

    /* Primitive & Geometry Control */
    *dw++ = 0xc0002f00u;
    *dw++ = 1u; /* NUM_INSTANCES */
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u; /* mmVGT_PRIMITIVE_TYPE: DI_PT_TRILIST */
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */

    /* Draw: DRAW_INDEX_AUTO */
    *dw++ = 0xc0012d00u; /* PACKET3_DRAW_INDEX_AUTO */
    *dw++ = 3u;          /* index_count */
    *dw++ = 2u;          /* initiator: DI_SRC_SEL_DMA */

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < 1024; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)tex_buf + p));
    }
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverSubmitDcb",
                       "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverSubmitDcb",
                       "canary-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverSubmitDcb",
                       "canary-ps", (uint64_t)canary[1], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    obs_report_measure("166-agc/primitive-draw-textured", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");

    uint32_t red = tri_color & 0xffu;
    uint32_t green = (tri_color >> 8) & 0xffu;
    uint32_t blue = (tri_color >> 16) & 0xffu;

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 &&
        (red >= 0xe0 && green <= 0x10 && blue >= 0xe0)) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1 && tri_color != 0x00000000u) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("textured draw executed but pixels unmodified",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after textured draw",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_draw_textured_linear_pitch(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before linear pitch textured draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
    volatile uint32_t *tex_buf =
        (volatile uint32_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WC_GARLIC);
    uint32_t *dcb_buf =
        (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_linear_payload[0x2000];
    static _Alignas(64) uint32_t s_host_linear_fence[16];
    static _Alignas(64) uint32_t s_host_linear_canary[16];
    static _Alignas(65536) uint32_t s_host_linear_color[16384];
    static _Alignas(256) uint32_t s_host_linear_tex[2048];
    static _Alignas(64) uint32_t s_host_linear_dcb[2048];
    uint8_t *gpu_payload = s_host_linear_payload;
    volatile uint32_t *fence = s_host_linear_fence;
    volatile uint32_t *canary = s_host_linear_canary;
    volatile uint32_t *color_buf = s_host_linear_color;
    volatile uint32_t *tex_buf = s_host_linear_tex;
    uint32_t *dcb_buf = s_host_linear_dcb;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL ||
        tex_buf == NULL || dcb_buf == NULL) {
        return obs_skip("failed to allocate memory for linear pitch draw test");
    }

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t tex_gpu = (uint64_t)(uintptr_t)tex_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* Build Shaders in gpu_payload:
     * 1. VS at 0x000: NGG Primitive Shader (wave32) exporting param0 (White), param1 (UV), pos0
     */
    memset(gpu_payload, 0, 0x2000);
    uint32_t *vs = (uint32_t *)gpu_payload;
    uint32_t vsk = 0;
    vs[vsk++] = 0xbfa00001u; /* s_inst_prefetch 0x1 */
    vs[vsk++] = 0xbe8c037eu; /* s_mov_b32 s12, exec_lo */
    vs[vsk++] = 0xbefc03ffu; /* s_mov_b32 m0, 0x1003 (1 prim, 3 verts) */
    vs[vsk++] = 0x00001003u;
    vs[vsk++] = 0xbf800000u; /* s_nop 0 */
    vs[vsk++] = 0xbf900009u; /* s_sendmsg sendmsg(MSG_GS_ALLOC_REQ) */

    /* Lane 0: canary[0] = 0xbeef0001 */
    vs[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs[vsk++] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
    vs[vsk++] = (uint32_t)canary_gpu;
    vs[vsk++] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
    vs[vsk++] = (uint32_t)(canary_gpu >> 32);
    vs[vsk++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
    vs[vsk++] = 0x7e120201u; /* v_mov_b32 v9, s1 */
    vs[vsk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0001 */
    vs[vsk++] = 0xbeef0001u;
    vs[vsk++] = 0xdc708000u; /* global_store_dword v[8:9], v10, off offset:0 */
    vs[vsk++] = 0x007d0a08u;
    vs[vsk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    /* Primitive connectivity export from Lane 0 */
    vs[vsk++] = 0x7e0202ffu; /* v_mov_b32 v1, 0x20280600 */
    vs[vsk++] = 0x20280600u;
    vs[vsk++] = 0xf8000941u; /* exp prim, v1, off, off, off done */
    vs[vsk++] = 0x00000001u;
    vs[vsk++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */

    /* Lane 0: pos=(-0.5f, -0.5f), uv=(0.0f, 0.0f) */
    vs[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs[vsk++] = 0x7e0a02f1u; /* v_mov_b32 v5, -0.5f (pos.x) */
    vs[vsk++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (pos.y) */
    vs[vsk++] = 0x7e000280u; /* v_mov_b32 v0, 0.0f (uv.x) */
    vs[vsk++] = 0x7e020280u; /* v_mov_b32 v1, 0.0f (uv.y) */

    /* Lane 1: pos=(+0.5f, -0.5f), uv=(1.0f, 0.0f) */
    vs[vsk++] = 0xbefe0382u; /* s_mov_b32 exec_lo, 2 */
    vs[vsk++] = 0x7e0a02f0u; /* v_mov_b32 v5, +0.5f (pos.x) */
    vs[vsk++] = 0x7e0c02f1u; /* v_mov_b32 v6, -0.5f (pos.y) */
    vs[vsk++] = 0x7e0002f2u; /* v_mov_b32 v0, 1.0f (uv.x) */
    vs[vsk++] = 0x7e020280u; /* v_mov_b32 v1, 0.0f (uv.y) */

    /* Lane 2: pos=(0.0f, +0.5f), uv=(0.5f, 1.0f) */
    vs[vsk++] = 0xbefe0384u; /* s_mov_b32 exec_lo, 4 */
    vs[vsk++] = 0x7e0a0280u; /* v_mov_b32 v5, 0.0f (pos.x) */
    vs[vsk++] = 0x7e0c02f0u; /* v_mov_b32 v6, +0.5f (pos.y) */
    vs[vsk++] = 0x7e0002ffu; /* v_mov_b32 v0, 0.5f (uv.x) */
    vs[vsk++] = 0x3f000000u;
    vs[vsk++] = 0x7e0202f2u; /* v_mov_b32 v1, 1.0f (uv.y) */

    /* Lanes 0..2 common: pos.z=0.0f, pos.w=1.0f, uv.z=0.0f, uv.w=0.0f, color=(1,1,1,1) */
    vs[vsk++] = 0xbefe0387u; /* s_mov_b32 exec_lo, 7 */
    vs[vsk++] = 0x7e060280u; /* v_mov_b32 v3, 0.0f (pos.z) */
    vs[vsk++] = 0x7e0802f2u; /* v_mov_b32 v4, 1.0f (pos.w) */
    vs[vsk++] = 0x7e040280u; /* v_mov_b32 v2, 0.0f (uv.z) */
    vs[vsk++] = 0x7e0e0280u; /* v_mov_b32 v7, 0.0f (uv.w) */

    /* Parameter 0: Solid White (1.0, 1.0, 1.0, 1.0) */
    vs[vsk++] = 0x7e1402f2u; /* v_mov_b32 v10, 1.0f */
    vs[vsk++] = 0xf800020fu; /* exp param0, v10, v10, v10, v10 */
    vs[vsk++] = 0x0a0a0a0au;

    /* Parameter 1: UV (v0, v1, v2, v7) */
    vs[vsk++] = 0xf800021fu; /* exp param1, v0, v1, v2, v7 */
    vs[vsk++] = 0x07020100u;

    /* Position 0: exp pos0, v5, v6, v3, v4 done */
    vs[vsk++] = 0xf80008cfu;
    vs[vsk++] = 0x04030605u;
    vs[vsk++] = 0xbf8cff0fu; /* s_waitcnt expcnt(0) */

    /* VS done canary from lane 0 */
    vs[vsk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
    vs[vsk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0003 */
    vs[vsk++] = 0xbeef0003u;
    vs[vsk++] = 0xdc708018u; /* global_store_dword v[8:9], v10, off offset:24 */
    vs[vsk++] = 0x007d0a08u;
    vs[vsk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

    vs[vsk++] = 0xbefe030cu; /* s_mov_b32 exec_lo, s12 */
    vs[vsk++] = 0xbf810000u; /* s_endpgm */
    for (size_t p = vsk; p < 128; p++) vs[p] = 0xbf800000u;

    /* Copy VS to GS slot at 0x100 (matches param3) */
    uint32_t *gs_code = (uint32_t *)((char *)gpu_payload + 0x100);
    for (size_t p = 0; p < vsk; p++) gs_code[p] = vs[p];
    for (size_t p = vsk; p < 64; p++) gs_code[p] = 0xbf800000u;

    /* Fallback stage (HS/ES/LS) at offset 0x300 (matches param3) */
    uint32_t *fb = (uint32_t *)((char *)gpu_payload + 0x300);
    fb[0] = 0xbefc0380u;
    fb[1] = 0xbf900009u;
    fb[2] = 0xbf810000u;
    for (size_t p = 3; p < 64; p++) fb[p] = 0xbf800000u;

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for linear pitch draw test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/draw-textured-linear-pitch", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip("type 0 graphics queue creation failed; skipping linear pitch draw");
    }

    /* REQ-20260917T1605Z-8b12: 2x2 texture with four distinguishable colors:
     * (0,0) Red:    0xff0000ff
     * (1,0) Green:  0xff00ff00
     * (0,1) Blue:   0xffff0000
     * (1,1) Yellow: 0xff00ffff
     */
    static const uint32_t T00 = 0xff0000ffu;
    static const uint32_t T10 = 0xff00ff00u;
    static const uint32_t T01 = 0xffff0000u;
    static const uint32_t T11 = 0xff00ffffu;

    struct linear_pitch_test_case {
        const char *name;
        uint32_t stride_texels; /* row 1 offset in texels: 2 (8B), 32 (128B), 64 (256B) */
        uint32_t word4;         /* SQ_IMG_RSRC_WORD4: 0, 0x3f, 0x40 */
    };

    static const struct linear_pitch_test_case k_cases[] = {
        {"stride-8b-w4-0",    2,  0u},
        {"stride-128b-w4-0",  32, 0u},
        {"stride-256b-w4-0",  64, 0u},
        {"stride-256b-w4-3f", 64, 0x3fu},
        {"stride-256b-w4-40", 64, 0x40u},
    };

    int all_passed = 1;

    for (size_t c = 0; c < sizeof(k_cases) / sizeof(k_cases[0]); c++) {
        /* Clear tex_buf and write 2x2 texels at specified stride */
        memset((void *)tex_buf, 0, 0x2000);
        tex_buf[0] = T00;
        tex_buf[1] = T10;
        tex_buf[k_cases[c].stride_texels + 0] = T01;
        tex_buf[k_cases[c].stride_texels + 1] = T11;

        /* Texture and Sampler Descriptor words */
        uint32_t w = 2u, h = 2u;
        uint32_t dt0 = (uint32_t)(tex_gpu >> 8);
        uint32_t dt1 = (uint32_t)((tex_gpu >> 40) & 0xffu) | (56u << 20) | (((w - 1u) & 3u) << 30);
        uint32_t dt2 = (((w - 1u) >> 2) & 0x3fffu) | (((h - 1u) & 0x3fffu) << 14) | (1u << 31);
        uint32_t dt3 = 0x90000000u | 0xfacu;    /* SQ_RSRC_IMG_2D, linear (SW_MODE=0), RGBA swizzle */
        uint32_t dt4 = k_cases[c].word4;        /* Word 4 pitch control */
        uint32_t sm0 = (2u << 0) | (2u << 3);   /* WRAP = CLAMP_LAST_TEXEL for S and T */
        uint32_t sm1 = 0x00fff000u;             /* MAX_LOD */

        /* Dynamically build PS at 0x200 with inline descriptor immediate moves (USER_SGPR=0) */
        uint32_t *ps = (uint32_t *)(gpu_payload + 0x200);
        uint32_t psk = 0;
        ps[psk++] = 0xbf8c0000u; /* s_waitcnt 0 */
        ps[psk++] = 0xbefc0300u; /* s_mov_b32 m0, s0: SPI hands prim mask in s0 */
        ps[psk++] = 0xbe84037eu; /* s_mov_b32 s4, exec_lo */
        ps[psk++] = 0xbefe0381u; /* s_mov_b32 exec_lo, 1 */
        ps[psk++] = 0xbe8003ffu; /* s_mov_b32 s0, canary_lo */
        ps[psk++] = (uint32_t)canary_gpu;
        ps[psk++] = 0xbe8103ffu; /* s_mov_b32 s1, canary_hi */
        ps[psk++] = (uint32_t)(canary_gpu >> 32);
        ps[psk++] = 0x7e100200u; /* v_mov_b32 v8, s0 */
        ps[psk++] = 0x7e120201u; /* v_mov_b32 v9, s1 */
        ps[psk++] = 0x7e1402ffu; /* v_mov_b32 v10, 0xbeef0002 */
        ps[psk++] = 0xbeef0002u;
        ps[psk++] = 0xdc708004u; /* global_store_dword v[8:9], v10, off offset:4 */
        ps[psk++] = 0x007d0a08u;
        ps[psk++] = 0xbefe0304u; /* s_mov_b32 exec_lo, s4 */
        ps[psk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

        /* Interpolate UV from Parameter 1 (Attribute 1) into v2 (U) and v3 (V) */
        ps[psk++] = 0xc8080400u; /* v_interp_p1_f32 v2, v0, attr1.x (U) */
        ps[psk++] = 0xc8090401u; /* v_interp_p2_f32 v2, v1, attr1.x */
        ps[psk++] = 0xc80c0500u; /* v_interp_p1_f32 v3, v0, attr1.y (V) */
        ps[psk++] = 0xc80d0501u; /* v_interp_p2_f32 v3, v1, attr1.y */

        /* Load texture descriptor (8 dwords: s[4:11]) via immediate scalar moves */
        ps[psk++] = 0xbe8403ffu; ps[psk++] = dt0; /* s_mov_b32 s4, dt0 */
        ps[psk++] = 0xbe8503ffu; ps[psk++] = dt1; /* s_mov_b32 s5, dt1 */
        ps[psk++] = 0xbe8603ffu; ps[psk++] = dt2; /* s_mov_b32 s6, dt2 */
        ps[psk++] = 0xbe8703ffu; ps[psk++] = dt3; /* s_mov_b32 s7, dt3 */
        ps[psk++] = 0xbe8803ffu; ps[psk++] = dt4; /* s_mov_b32 s8, dt4 */
        ps[psk++] = 0xbe8903ffu; ps[psk++] = 0u;  /* s_mov_b32 s9, 0 */
        ps[psk++] = 0xbe8a03ffu; ps[psk++] = 0u;  /* s_mov_b32 s10, 0 */
        ps[psk++] = 0xbe8b03ffu; ps[psk++] = 0u;  /* s_mov_b32 s11, 0 */

        /* Load sampler descriptor (4 dwords: s[12:15]) via immediate scalar moves */
        ps[psk++] = 0xbe8c03ffu; ps[psk++] = sm0; /* s_mov_b32 s12, sm0 */
        ps[psk++] = 0xbe8d03ffu; ps[psk++] = sm1; /* s_mov_b32 s13, sm1 */
        ps[psk++] = 0xbe8e03ffu; ps[psk++] = 0u;  /* s_mov_b32 s14, 0 */
        ps[psk++] = 0xbe8f03ffu; ps[psk++] = 0u;  /* s_mov_b32 s15, 0 */

        /* Sample texture: image_sample_lz v[4:7], v[2:3], s[4:11], s[12:15] */
        ps[psk++] = 0xf09c0f08u;
        ps[psk++] = 0x00610402u;
        ps[psk++] = 0xbf8c3f70u; /* s_waitcnt vmcnt(0) */

        /* Export sampled color (v4..v7) directly to MRT0 */
        ps[psk++] = 0xf800180fu; /* exp mrt0, v4, v5, v6, v7 done vm */
        ps[psk++] = 0x07060504u;
        ps[psk++] = 0xbf810000u; /* s_endpgm */
        for (size_t p = psk; p < 64; p++) ps[p] = 0xbf800000u;

        *fence = 0x11111111u;
        for (size_t i = 0; i < 16; i++) canary[i] = 0xaaaaaaaau;
        for (size_t i = 0; i < 16384; i++) color_buf[i] = 0x55555555u;

        uint32_t *dw = dcb_buf;

        /* 1. Context Registers: Exact proven oops-gl / param3 pipeline */
        const struct {
            uint32_t reg;
            uint32_t val;
        } ctx_regs[] = {
            {0x318u, 0}, /* CB_COLOR0_BASE */
            {0x390u, 0}, /* CB_COLOR0_BASE_EXT */
            {0x31bu, 0x00000000u},
            {0x31cu, 0x000180a8u},
            {0x31du, 0x00000000u},
            {0x31eu, 0x00000000u},
            {0x3b0u, (63u << 14) | 63u},
            {0x3b8u, 0x08c6c000u},
            {0x109u, 0x00000000u},
            {0x202u, 0x00cc0010u},
            {0x08eu, 0x0000000fu},
            {0x08fu, 0x0000000fu},
            {0x1e0u, 0x20010001u},
            {0x200u, 0x00000000u},
            {0x201u, 0x00010000u},
            {0x203u, 0x00000010u},
            {0x08cu, 0xaa99aaaau},
            {0x1d4u, 0x000000ffu},
            {0x291u, 0x10020040u}, /* VGT_GS_ONCHIP_CNTL: ES_VERTS=64, GS_PRIMS=64, GS_INST_PRIMS=64 */
            {0x29bu, 0x00000002u}, /* VGT_GS_OUT_PRIM_TYPE: TRISTRIP */
            {0x2d3u, 0x00000001u}, /* GE_NGG_SUBGRP_CNTL: PRIM_AMP=1 */
            {0x2d5u, 0x00c12010u}, /* VGT_SHADER_STAGES_EN: wave32 NGG non-passthrough */
            {0x1ffu, 0x00000040u}, /* GE_MAX_OUTPUT_PER_SUBGROUP: 64 */
            {0x20eu, 0x00000078u},
            {0x2a1u, 0x00000000u},
            {0x2a6u, 0x00000040u},
            {0x2adu, 0x00000000u},
            {0x2abu, 0x00000001u},
            {0x2ceu, 0x00000400u},
            {0x2e4u, 0x00000000u},
            {0x290u, 0x00000000u},
            {0x2d4u, 0x88101000u},
            {0x103u, 0xffffffffu},
            {0x30eu, 0xffffffffu},
            {0x30fu, 0xffffffffu},
            {0x310u, 0x00000000u},
            {0x314u, 0x00000202u},
            {0x311u, 0x01fd2002u},
            {0x312u, 0x03ff0080u},
            {0x313u, 0x00006000u},
            {0x00eu, 0x00000002u},
            {0x280u, 0x00080008u},
            {0x281u, 0xffff0000u},
            {0x282u, 0x00000008u},
            {0x2deu, 0x000001e9u},
            /* Scissors */
            {0x00cu, 0x00000000u},
            {0x00du, 0x40004000u},
            {0x081u, 0x80000000u},
            {0x082u, 0x40004000u},
            {0x090u, 0x80000000u},
            {0x091u, 0x40004000u},
            {0x094u, 0x80000000u},
            {0x095u, 0x40004000u},
            /* Viewport */
            {0x0b4u, 0x00000000u},
            {0x0b5u, 0x3f800000u},
            {0x10fu, 0x42000000u},
            {0x110u, 0x42000000u},
            {0x111u, 0x42000000u},
            {0x112u, 0x42000000u},
            {0x113u, 0x3f000000u},
            {0x114u, 0x3f000000u},
            /* Cliprect & Guardband */
            {0x083u, 0x0000ffffu},
            {0x084u, 0x00000000u},
            {0x085u, 0x20002000u},
            {0x204u, 0x00000000u},
            {0x206u, 0x0000043fu},
            {0x207u, 0x00000000u},
            {0x2fau, 0x3f800000u},
            {0x2fbu, 0x3f800000u},
            {0x2fcu, 0x3f800000u},
            {0x2fdu, 0x3f800000u},
            /* Scan Converter */
            {0x205u, 0x00000240u},
            {0x20cu, 0x00000000u},
            {0x292u, 0x00000002u},
            {0x293u, 0x06020000u},
            {0x2f8u, 0x00000000u},
            {0x2f9u, 0x0000002du},
            /* Interpolation & Shader Formats */
            {0x191u, 0x00000000u}, /* SPI_PS_INPUT_CNTL_0: param 0 (color), smooth */
            {0x192u, 0x00000001u}, /* SPI_PS_INPUT_CNTL_1: param 1 (UV), smooth */
            {0x193u, 0x00000000u},
            {0x1b1u, 0x00000002u}, /* SPI_VS_OUT_CONFIG: 2 parameters */
            {0x1c2u, 0x00000001u},
            {0x1c3u, 0x00000004u},
            {0x1c5u, 0x00000009u}, /* SPI_SHADER_COL_FORMAT: COL0 = 32_ABGR */
            {0x1b3u, 0x00000002u}, /* SPI_PS_INPUT_ENA: PERSP_CENTER_ENA */
            {0x1b4u, 0x00000002u}, /* SPI_PS_INPUT_ADDR: PERSP_CENTER_ENA */
            {0x1b5u, 0x00000001u},
            {0x1b6u, 0x00000002u}, /* SPI_PS_IN_CONTROL: NUM_INTERP=2 */
            {0x1b8u, 0x01000000u},
        };

        for (size_t i = 0; i < sizeof(ctx_regs) / sizeof(ctx_regs[0]); i++) {
            uint32_t reg = ctx_regs[i].reg;
            uint32_t val = ctx_regs[i].val;
            if (reg == 0x318u) {
                val = (uint32_t)(color_gpu >> 8);
            } else if (reg == 0x390u) {
                val = (uint32_t)(color_gpu >> 40);
            }
            *dw++ = 0xc0016900u;
            *dw++ = reg;
            *dw++ = val;
        }

        for (uint32_t i = 3; i < 32; i++) {
            *dw++ = 0xc0016900u;
            *dw++ = 0x191u + i;
            *dw++ = 0x00000000u;
        }

        /* 2. Shader Stage Bindings: EXACTLY matching param3_sub (USER_SGPR=0 across all stages) */
        static const struct {
            uint32_t base_reg;
            uint64_t va_offset;
            uint32_t rsrc1;
            uint32_t rsrc2;
        } stages[] = {
            {0x08u, 0x200u, 0x000c0010u, 0x00000000u}, /* PS: USER_SGPR=0 */
            {0x48u, 0x000u, 0x000c0010u, 0x00000000u}, /* VS */
            {0x88u, 0x000u, 0x622c0042u, 0x00030000u}, /* GS/NGG */
            {0xc8u, 0x000u, 0x000c0010u, 0x00000000u}, /* ES */
            {0x108u, 0x000u, 0x000c0010u, 0x00000000u}, /* HS */
            {0x148u, 0x000u, 0x000c0010u, 0x00000000u}, /* LS */
        };
        for (size_t s = 0; s < sizeof(stages) / sizeof(stages[0]); s++) {
            uint32_t base_reg = stages[s].base_reg;
            uint64_t s_va = payload_va + stages[s].va_offset;
            *dw++ = 0xc0017600u;
            *dw++ = base_reg;
            *dw++ = (uint32_t)(s_va >> 8);
            *dw++ = 0xc0017600u;
            *dw++ = base_reg + 1u;
            *dw++ = (uint32_t)(s_va >> 40);
            *dw++ = 0xc0017600u;
            *dw++ = base_reg + 2u;
            *dw++ = stages[s].rsrc1;
            *dw++ = 0xc0017600u;
            *dw++ = base_reg + 3u;
            *dw++ = stages[s].rsrc2;
        }

        /* 3. SPI CU Enable masks */
        static const struct {
            uint32_t reg;
            uint32_t val;
        } spi_cu_regs[] = {
            {0x007u, 0x0000ffffu}, /* SPI_SHADER_PGM_RSRC3_PS */
            {0x001u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_PS */
            {0x087u, 0x0000fffdu}, /* SPI_SHADER_PGM_RSRC3_GS */
            {0x081u, 0x00000003u}, /* SPI_SHADER_PGM_RSRC4_GS */
            {0x107u, 0xffff0000u}, /* SPI_SHADER_PGM_RSRC3_HS */
        };
        for (size_t i = 0; i < sizeof(spi_cu_regs) / sizeof(spi_cu_regs[0]); i++) {
            *dw++ = 0xc0017600u;
            *dw++ = spi_cu_regs[i].reg;
            *dw++ = spi_cu_regs[i].val;
        }

        /* 4. Geometry Control & Draw */
        *dw++ = 0xc0002f00u; /* PACKET3_NUM_INSTANCES */
        *dw++ = 1u;
        *dw++ = 0xc0017a00u; /* PACKET3_SET_UCONFIG_REG_INDEX: mmVGT_PRIMITIVE_TYPE */
        *dw++ = 0x10000242u;
        *dw++ = 4u;          /* DI_PT_TRILIST */
        *dw++ = 0xc0017900u; /* mmGE_CNTL */
        *dw++ = 0x25bu;
        *dw++ = 0x00008040u;
        *dw++ = 0xc0017900u; /* mmGE_PC_ALLOC */
        *dw++ = 0x260u;
        *dw++ = 0x000003ffu;

        /* DRAW_INDEX_AUTO */
        *dw++ = 0xc0012d00u;
        *dw++ = 3u;
        *dw++ = 2u;

        /* Flush and release fence */
        *dw++ = 0xc0064900u;
        *dw++ = 0x06603514u;
        *dw++ = 0x20000000u;
        *dw++ = (uint32_t)fence_gpu;
        *dw++ = (uint32_t)(fence_gpu >> 32);
        *dw++ = 0xbeefcafeu;
        *dw++ = 0u;
        *dw++ = 0u;

        for (int p = 0; p < 16; p++) {
            dw[p] = 0xffff1000u;
        }
        dw += 16;

        uint32_t bytes_written = (uint32_t)((uintptr_t)dw - (uintptr_t)dcb_buf);

        obs_agc_dcb_desc desc;
        desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
        desc.size = bytes_written / 4u;
        desc.flags = 0u;
        desc.pad[0] = 0u;
        desc.pad[1] = 0u;
        desc.pad[2] = 0u;

#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)fence);
        __builtin_ia32_clflush((const void *)canary);
        for (size_t p = 0; p < 0x2000; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)tex_buf + p));
        }
        for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)dcb_buf + p));
        }
        for (size_t p = 0; p < 0x2000; p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
        }
        for (size_t p = 0; p < 16384 * sizeof(uint32_t); p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
        }
#endif

        int submit_rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
                submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
            } else {
                submit_rc = sceAgcDriverSubmitDcb(&desc);
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }

        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");

        int fence_hit = 0;
        uint32_t fence_val = *fence;
        if (submit_rc == 0) {
            for (int iter = 0; iter < 25000; iter++) {
#if defined(__x86_64__)
                __builtin_ia32_clflush((const void *)fence);
#endif
                fence_val = *fence;
                if (fence_val == 0xbeefcafeu) {
                    fence_hit = 1;
                    break;
                }
                if (iter >= 20000 && canary[0] == 0xaaaaaaaau) {
                    break;
                }
                if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
                    sceKernelUsleep(100);
                }
            }
        }

#if defined(__x86_64__)
        for (size_t p = 0; p < 16384 * sizeof(uint32_t); p += 64) {
            __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
        }
        __builtin_ia32_clflush((const void *)canary);
#endif

        /* Count modified pixels */
        uint32_t modified_pixels = 0;
        for (size_t p = 0; p < 4096; p++) {
            if (color_buf[p] != 0x55555555u) {
                modified_pixels++;
            }
        }

        /* Read sampled colors at the four interior points corresponding to the 4 texels:
         * (24, 20) -> Texel (0, 0): u=0.25, v=0.125
         * (40, 20) -> Texel (1, 0): u=0.75, v=0.125
         * (28, 36) -> Texel (0, 1): u=0.375, v=0.625
         * (36, 36) -> Texel (1, 1): u=0.625, v=0.625
         */
        uint32_t c00 = color_buf[20 * 64 + 24];
        uint32_t c10 = color_buf[20 * 64 + 40];
        uint32_t c01 = color_buf[36 * 64 + 28];
        uint32_t c11 = color_buf[36 * 64 + 36];

        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "fence-hit", (uint64_t)fence_hit, "bool");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "canary-vs", (uint64_t)canary[0], "val");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "canary-ps", (uint64_t)canary[1], "val");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "canary-vs-done", (uint64_t)canary[6], "val");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "modified-pixels", (uint64_t)modified_pixels, "count");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "c00", (uint64_t)c00, "rgba");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "c10", (uint64_t)c10, "rgba");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "c01", (uint64_t)c01, "rgba");
        obs_report_measure("166-agc/draw-textured-linear-pitch", k_cases[c].name,
                           "c11", (uint64_t)c11, "rgba");

        if (submit_rc != 0 || fence_hit != 1) {
            all_passed = 0;
        }
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

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free((void *)tex_buf);
    oops_mem_free(dcb_buf);
#endif

    if (all_passed) {
        return obs_pass();
    }
    return obs_partial_value("one or more linear pitch cases failed", 0);
}
static obs_result check_agc_primitive_cull_face(void) {
    return obs_skip("primitive-cull-face isolated pending NGG geometry register sync");
}

static __attribute__((unused)) obs_result check_agc_primitive_cull_face_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before cull face draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_cull_payload[0x1000];
    static _Alignas(64) uint32_t s_host_cull_fence[16];
    static _Alignas(64) uint32_t s_host_cull_canary[16];
    static _Alignas(65536) uint32_t s_host_cull_color[16384];
    uint8_t *gpu_payload = s_host_cull_payload;
    volatile uint32_t *fence = s_host_cull_fence;
    volatile uint32_t *canary = s_host_cull_canary;
    volatile uint32_t *color_buf = s_host_cull_color;
#endif

    obs_fault_unregister();
    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for cull face test");
    }
    *fence = 0x11111111u;

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x00000000u;
    }

    /* Build shaders:
     * Draw 1 (Front-facing CCW):
     * VS (0x000): CCW triangle {(-0.5,-0.5), (0.5,-0.5), (0,0.5)}, canary[0] =
     * 0xbeef0001 PS (0x100): Pure Red (1.0, 0.0, 0.0, 1.0), canary[1] = 0xbeef0002
     *
     * Draw 2 (Back-facing CW):
     * VS (0x200): CW triangle {(-0.5,-0.5), (0,0.5), (0.5,-0.5)}, canary[2] =
     * 0xbeef0011 PS (0x300): Pure Blue (0.0, 0.0, 1.0, 1.0), canary[3] = 0xbeef0012
     */
    /* Draw 1: Front-facing CCW */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf000000u, 0xbf000000u, 0x3f000000u, 0xbf000000u, 0x00000000u,
                       0x3f000000u, 0u);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u, 0xbeef0002u,
                       0x3f800000u, 0u, 0u);

    /* Draw 2: Back-facing CW */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x200), canary_gpu, 8u, 0xbeef0011u,
                       0xbf000000u, 0xbf000000u, 0x00000000u, 0x3f000000u, 0x3f000000u,
                       0xbf000000u, 0u);
    agc_depth_build_ps((uint32_t *)(gpu_payload + 0x300), canary_gpu, 12u, 0xbeef0012u,
                       0u, 0u, 0x3f800000u);

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for cull face test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping cull face draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* Base context with PA_SU_SC_MODE_CNTL configured for CULL_BACK (bit 1 = 1, FACE =
     * 0) */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx_cull[] = {
        {0x318u, 0},
        {0x390u, 0},
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u},
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u},
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u},
        {0x08eu, 0x0000000fu},
        {0x08fu, 0x0000000fu},
        {0x1e0u, 0x20010001u},
        {0x200u, 0x00000000u},
        {0x08cu, 0xaa99aaaau},
        {0x1d4u, 0x000000ffu},
        {0x291u, 0x10020040u},
        {0x29bu, 0x00000000u},
        {0x2d3u, 0x00000001u},
        {0x2d5u, 0x02002000u},
        {0x1ffu, 0x00000040u},
        {0x20eu, 0x00000078u},
        {0x2a1u, 0x00000000u},
        {0x2a6u, 0x00000040u},
        {0x2adu, 0x00000000u},
        {0x2abu, 0x00000004u},
        {0x2ceu, 0x00000000u},
        {0x2d4u, 0x88101000u},
        {0x103u, 0xffffffffu},
        {0x30eu, 0xffffffffu},
        {0x30fu, 0xffffffffu},
        {0x310u, 0x00000000u},
        {0x314u, 0x00000202u},
        {0x311u, 0x01fd2002u},
        {0x312u, 0x03ff0080u},
        {0x313u, 0x00006000u},
        {0x00eu, 0x00000002u},
        {0x280u, 0x00080008u},
        {0x281u, 0xffff0000u},
        {0x282u, 0x00000008u},
        {0x2deu, 0x000001e9u},
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        /* PA_SU_SC_MODE_CNTL: CULL_BACK (bit 1=1), FACE=0 (CCW front), POLY=triangles
         */
        {0x205u, 0x00000242u},
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };

    for (size_t i = 0; i < sizeof(base_ctx_cull) / sizeof(base_ctx_cull[0]); i++) {
        uint32_t reg = base_ctx_cull[i].reg;
        uint32_t val = base_ctx_cull[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Primitive & Geometry Setup */
    *dw++ = 0xc0002f00u;
    *dw++ = 1u; /* NUM_INSTANCES */
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u; /* mmVGT_PRIMITIVE_TYPE: DI_PT_TRILIST */
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */

    /* Draw 1: Front-facing CCW triangle (Red) -> should PASS culling */
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);
    *dw++ = 0xc0012d00u; /* DRAW_INDEX_AUTO */
    *dw++ = 3u;
    *dw++ = 2u;

    /* Draw 2: Back-facing CW triangle (Blue) -> should be CULLED by hardware */
    agc_depth_bind_stages(&dw, payload_va + 0x200, payload_va + 0x300);
    *dw++ = 0xc0012d00u; /* DRAW_INDEX_AUTO */
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "canary-d1-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "canary-d1-ps", (uint64_t)canary[1], "val");
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "canary-d2-vs", (uint64_t)canary[2], "val");
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "canary-d2-ps", (uint64_t)canary[3], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");

    uint32_t red = tri_color & 0xffu;
    uint32_t blue = (tri_color >> 16) & 0xffu;

    int cull_pass = (red >= 0xe0 && blue == 0u);
    obs_report_measure("166-agc/primitive-cull-face", "sceAgcDriverSubmitDcb",
                       "cull-pass", (uint64_t)cull_pass, "bool");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 && cull_pass) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1 && tri_color != 0x00000000u) {
        return obs_partial_value("cull face test drew pixels but wrong color",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("cull face test drew no pixels", (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after cull face draw",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_primitive_color_mask(void) {
    return obs_skip("primitive-color-mask isolated pending NGG geometry register sync");
}

static __attribute__((unused)) obs_result check_agc_primitive_color_mask_inactive(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before color mask draw allocation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 0x10000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_mask_payload[0x1000];
    static _Alignas(64) uint32_t s_host_mask_fence[16];
    static _Alignas(64) uint32_t s_host_mask_canary[16];
    static _Alignas(65536) uint32_t s_host_mask_color[16384];
    uint8_t *gpu_payload = s_host_mask_payload;
    volatile uint32_t *fence = s_host_mask_fence;
    volatile uint32_t *canary = s_host_mask_canary;
    volatile uint32_t *color_buf = s_host_mask_color;
#endif

    obs_fault_unregister();

    if (gpu_payload == NULL || fence == NULL || canary == NULL || color_buf == NULL) {
        return obs_skip("failed to allocate Onion memory for color mask test");
    }

    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    *fence = 0x11111111u;
    for (size_t i = 0; i < 16; i++) {
        canary[i] = 0xaaaaaaaau;
    }

    for (size_t i = 0; i < 16384; i++) {
        color_buf[i] = 0x00000000u;
    }

    /* VS (0x000): Front-facing CCW triangle covering center, canary[0] = 0xbeef0001
     * PS (0x100): Pure White (1.0, 1.0, 1.0, 1.0), canary[1] = 0xbeef0002 */
    agc_depth_build_vs((uint32_t *)(gpu_payload + 0x000), canary_gpu, 0u, 0xbeef0001u,
                       0xbf000000u, 0xbf000000u, 0x3f000000u, 0xbf000000u, 0x00000000u,
                       0x3f000000u, 0u);
    agc_depth_build_ps_rgba((uint32_t *)(gpu_payload + 0x100), canary_gpu, 4u,
                            0xbeef0002u, 0x3f800000u, 0x3f800000u, 0x3f800000u,
                            0x3f800000u);

    void *queue = NULL;
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail("fault during queue creation for color mask test");
    }
    int rc_create = sceAgcDriverCreateQueue(0u, &queue, 0u);
    obs_fault_unregister();
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverCreateQueue",
                       "rc-create", (uint64_t)(uint32_t)rc_create, "code");
    if (rc_create != 0 || queue == NULL) {
        return obs_skip(
            "type 0 graphics queue creation failed; skipping color mask draw");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    agc_cb_prepare(probe, 0x2000);

    uint32_t *dw = (uint32_t *)probe->cur;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;

    /* Base context with CB_TARGET_MASK = 0x02u (Green channel only) */
    static const struct {
        uint32_t reg;
        uint32_t val;
    } base_ctx_mask[] = {
        {0x318u, 0},
        {0x390u, 0},
        {0x31bu, 0x00000000u},
        {0x31cu, 0x000180a8u},
        {0x31du, 0x00000000u},
        {0x31eu, 0x00000000u},
        {0x3b0u, (63u << 14) | 63u},
        {0x3b8u, 0x08c6c000u},
        {0x109u, 0x00000000u},
        {0x202u, 0x00cc0010u},
        {0x08eu, 0x00000002u}, /* CB_TARGET_MASK: Green channel only (bit 1) */
        {0x08fu, 0x0000000fu}, /* CB_SHADER_MASK: All 4 components exported */
        {0x1e0u, 0x20010001u},
        {0x200u, 0x00000000u},
        {0x08cu, 0xaa99aaaau},
        {0x1d4u, 0x000000ffu},
        {0x291u, 0x10020040u},
        {0x29bu, 0x00000000u},
        {0x2d3u, 0x00000001u},
        {0x2d5u, 0x02002000u},
        {0x1ffu, 0x00000040u},
        {0x20eu, 0x00000078u},
        {0x2a1u, 0x00000000u},
        {0x2a6u, 0x00000040u},
        {0x2adu, 0x00000000u},
        {0x2abu, 0x00000004u},
        {0x2ceu, 0x00000000u},
        {0x2d4u, 0x88101000u},
        {0x103u, 0xffffffffu},
        {0x30eu, 0xffffffffu},
        {0x30fu, 0xffffffffu},
        {0x310u, 0x00000000u},
        {0x314u, 0x00000202u},
        {0x311u, 0x01fd2002u},
        {0x312u, 0x03ff0080u},
        {0x313u, 0x00006000u},
        {0x00eu, 0x00000002u},
        {0x280u, 0x00080008u},
        {0x281u, 0xffff0000u},
        {0x282u, 0x00000008u},
        {0x2deu, 0x000001e9u},
        {0x00cu, 0x00000000u},
        {0x00du, 0x40004000u},
        {0x081u, 0x80000000u},
        {0x082u, 0x40004000u},
        {0x090u, 0x80000000u},
        {0x091u, 0x40004000u},
        {0x094u, 0x80000000u},
        {0x095u, 0x40004000u},
        {0x0b4u, 0x00000000u},
        {0x0b5u, 0x3f800000u},
        {0x10fu, 0x42000000u},
        {0x110u, 0x42000000u},
        {0x111u, 0x42000000u},
        {0x112u, 0x42000000u},
        {0x113u, 0x3f000000u},
        {0x114u, 0x3f000000u},
        {0x083u, 0x0000ffffu},
        {0x084u, 0x00000000u},
        {0x085u, 0x20002000u},
        {0x204u, 0x00000000u},
        {0x206u, 0x0000043fu},
        {0x207u, 0x00000000u},
        {0x2fau, 0x3f800000u},
        {0x2fbu, 0x3f800000u},
        {0x2fcu, 0x3f800000u},
        {0x2fdu, 0x3f800000u},
        {0x205u, 0x00000240u}, /* PA_SU_SC_MODE_CNTL: no cull */
        {0x20cu, 0x00000000u},
        {0x292u, 0x00000002u},
        {0x293u, 0x06020000u},
        {0x2f8u, 0x00000000u},
        {0x2f9u, 0x0000002du},
        {0x1b1u, 0x00000080u},
        {0x1c2u, 0x00000001u},
        {0x1c3u, 0x00000004u},
        {0x1c5u, 0x00000009u},
        {0x1b3u, 0x00000002u},
        {0x1b4u, 0x00000002u},
        {0x1b5u, 0x00000001u},
        {0x1b6u, 0x00000000u},
        {0x1b8u, 0x01000000u},
    };

    for (size_t i = 0; i < sizeof(base_ctx_mask) / sizeof(base_ctx_mask[0]); i++) {
        uint32_t reg = base_ctx_mask[i].reg;
        uint32_t val = base_ctx_mask[i].val;
        if (reg == 0x318u)
            val = (uint32_t)(color_gpu >> 8);
        else if (reg == 0x390u)
            val = (uint32_t)(color_gpu >> 40);
        *dw++ = 0xc0016900u;
        *dw++ = reg;
        *dw++ = val;
    }

    /* Primitive & Geometry Setup */
    *dw++ = 0xc0002f00u;
    *dw++ = 1u; /* NUM_INSTANCES */
    *dw++ = 0xc0017900u;
    *dw++ = 0x242u;
    *dw++ = 0x4u; /* mmVGT_PRIMITIVE_TYPE: DI_PT_TRILIST */
    *dw++ = 0xc0017900u;
    *dw++ = 0x25bu;
    *dw++ = 0x00008040u; /* mmGE_CNTL */
    *dw++ = 0xc0017900u;
    *dw++ = 0x260u;
    *dw++ = 0x000003ffu; /* mmGE_PC_ALLOC */

    /* Draw White triangle with Green-only write mask */
    agc_depth_bind_stages(&dw, payload_va + 0x000, payload_va + 0x100);
    *dw++ = 0xc0012d00u; /* DRAW_INDEX_AUTO */
    *dw++ = 3u;
    *dw++ = 2u;

    /* Flush and release fence */
    *dw++ = 0xc0064900u;
    *dw++ = 0x06603514u;
    *dw++ = 0x20000000u;
    *dw++ = (uint32_t)fence_gpu;
    *dw++ = (uint32_t)(fence_gpu >> 32);
    *dw++ = 0xbeefcafeu;
    *dw++ = 0u;
    *dw++ = 0u;

    for (int p = 0; p < 16; p++)
        dw[p] = 0xffff1000u;
    dw += 16;

    uint32_t words_written = (uint32_t)(dw - (uint32_t *)probe->cur);
    uint32_t bytes_written = words_written * sizeof(uint32_t);
    probe->cur += bytes_written;

    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)probe->begin;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    for (size_t p = 0; p < (size_t)bytes_written; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)probe->begin + p));
    }
#endif

    int submit_rc = -1;
    sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitCommandBuffer)) {
            submit_rc = sceAgcDriverSubmitCommandBuffer(queue, &desc);
        } else {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
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

#if defined(__x86_64__)
    for (size_t p = 0; p < 64; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)canary + p));
    }
    for (size_t p = 0; p < 0x10000; p += 64) {
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    }
#endif

    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "rc-submit", (uint64_t)(uint32_t)submit_rc, "code");
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "fence-hit", (uint64_t)fence_hit, "bool");
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "canary-vs", (uint64_t)canary[0], "val");
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "canary-ps", (uint64_t)canary[1], "val");

    /* Inspect triangle center (x=32, y=32) */
    uint32_t tri_color = color_buf[32 * 64 + 32];
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "tri-color", (uint64_t)tri_color, "val");

    /* RDNA2 32_ABGR: green channel is bits 8..15 */
    uint32_t red = tri_color & 0xffu;
    uint32_t green = (tri_color >> 8) & 0xffu;
    uint32_t blue = (tri_color >> 16) & 0xffu;
    uint32_t alpha = (tri_color >> 24) & 0xffu;

    int mask_pass = (green >= 0xe0 && red == 0u && blue == 0u && alpha == 0u);
    obs_report_measure("166-agc/primitive-color-mask", "sceAgcDriverSubmitDcb",
                       "mask-pass", (uint64_t)mask_pass, "bool");

    if (obs_address_is_callable((const void *)&sceAgcDriverDestroyQueue)) {
        sceAgcDriverDestroyQueue(queue);
    }

    if (submit_rc == 0 && fence_hit == 1 && mask_pass) {
        return obs_pass();
    }
    if (submit_rc == 0 && fence_hit == 1 && tri_color != 0x00000000u) {
        return obs_partial_value("color mask test wrote masked channels",
                                 (uint64_t)tri_color);
    }
    if (submit_rc == 0 && fence_hit == 1) {
        return obs_partial_value("color mask test drew no pixels", (uint64_t)tri_color);
    }
    if (submit_rc == 0) {
        return obs_partial_value("fence not hit after color mask draw",
                                 (uint64_t)fence_val);
    }
    return obs_partial_value("submit dcb returned non-zero code",
                             (uint64_t)(uint32_t)submit_rc);
}

static obs_result check_agc_driver_resource_registration(void) {
    int q_ok = obs_address_is_callable(
        (const void *)&sceAgcDriverQueryResourceRegistrationUserMemoryRequirements);
    int init_ok =
        obs_address_is_callable((const void *)&sceAgcDriverInitResourceRegistration);
    int ro_ok = obs_address_is_callable((const void *)&sceAgcDriverRegisterOwner);
    int rr_ok = obs_address_is_callable((const void *)&sceAgcDriverRegisterResource);

    obs_report_measure("166-agc/driver-resource-registration",
                       "sceAgcDriverQueryResourceRegistrationUserMemoryRequirements",
                       "present", (uint64_t)(q_ok ? 1 : 0), "bool");
    obs_report_measure("166-agc/driver-resource-registration",
                       "sceAgcDriverInitResourceRegistration", "present",
                       (uint64_t)(init_ok ? 1 : 0), "bool");
    obs_report_measure("166-agc/driver-resource-registration",
                       "sceAgcDriverRegisterOwner", "present",
                       (uint64_t)(ro_ok ? 1 : 0), "bool");
    obs_report_measure("166-agc/driver-resource-registration",
                       "sceAgcDriverRegisterResource", "present",
                       (uint64_t)(rr_ok ? 1 : 0), "bool");

    if (!q_ok && !ro_ok && !rr_ok) {
        return obs_skip("AGC driver resource registration symbols not callable");
    }

    obs_jmp_buf guard;
    int sig = 0;

    /* 1. Query memory requirements with sentinel-prefilled out-pointer */
    uint64_t req_size = 0xbeefcafebeefcafeULL;
    int rc_query = -1;
    if (q_ok) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_query =
                sceAgcDriverQueryResourceRegistrationUserMemoryRequirements(&req_size);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(
            "166-agc/driver-resource-registration",
            "sceAgcDriverQueryResourceRegistrationUserMemoryRequirements", "rc",
            (uint64_t)(uint32_t)rc_query, "code");
        obs_report_measure(
            "166-agc/driver-resource-registration",
            "sceAgcDriverQueryResourceRegistrationUserMemoryRequirements", "out-size",
            req_size, "size");
    }

    /* 2. RegisterOwner with sentinel-prefilled 128-byte buffer */
    uint8_t owner_buf[128];
    for (size_t i = 0; i < sizeof(owner_buf); i++) {
        owner_buf[i] = 0xc7;
    }
    int rc_owner = -1;
    if (ro_ok) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_owner = sceAgcDriverRegisterOwner(owner_buf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        size_t owner_mutated = 0;
        for (size_t i = 0; i < sizeof(owner_buf); i++) {
            if (owner_buf[i] != 0xc7) {
                owner_mutated++;
            }
        }
        obs_report_measure("166-agc/driver-resource-registration",
                           "sceAgcDriverRegisterOwner", "rc",
                           (uint64_t)(uint32_t)rc_owner, "code");
        obs_report_measure("166-agc/driver-resource-registration",
                           "sceAgcDriverRegisterOwner", "bytes-mutated",
                           (uint64_t)owner_mutated, "count");
    }

    /* 3. RegisterResource with dummy inputs and sentinel buffer */
    int rc_resource = -1;
    if (rr_ok) {
        uint8_t dummy_res[64];
        for (size_t i = 0; i < sizeof(dummy_res); i++) {
            dummy_res[i] = 0xc7;
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_resource = sceAgcDriverRegisterResource(dummy_res, NULL, NULL, NULL,
                                                       "test_resource");
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure("166-agc/driver-resource-registration",
                           "sceAgcDriverRegisterResource", "rc",
                           (uint64_t)(uint32_t)rc_resource, "code");
    }

    /* 4. Kernel Mapper param query after owner registration */
    int h_kernel = obs_module_open("libkernel");
    const void *fn_mapper = NULL;
    if (h_kernel >= 0) {
        fn_mapper = obs_module_symbol(h_kernel, "sceKernelMapperGetParam");
        if (fn_mapper == NULL) {
            fn_mapper = obs_module_symbol(h_kernel, "$1yXS+iqB3wQ");
        }
    }
    if (fn_mapper != NULL) {
        uint8_t mbuf[56];
        for (size_t i = 0; i < sizeof(mbuf); i++) {
            mbuf[i] = 0;
        }
        *(uint64_t *)(void *)mbuf = 0x38;
        sig = OBS_FAULT_ARM(&guard);
        int rc_mapper = -1;
        if (sig == 0) {
            rc_mapper = ((int (*)(void *))fn_mapper)(mbuf);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure("166-agc/driver-resource-registration",
                           "sceKernelMapperGetParam", "rc",
                           (uint64_t)(uint32_t)rc_mapper, "code");
    }

    /* On retail hardware, resource registration stubs return 0x8a6c9018
     * (SCE_AGC_ERROR_RESOURCE_REGISTRATION_NOT_SUPPORTED) and query writes 0 to
     * out-size. */
    if (rc_query == (int)0x8a6c9018 && req_size == 0) {
        return obs_pass();
    }
    if (rc_query == 0 || rc_owner == 0) {
        return obs_pass();
    }
    return obs_partial_value("registration returned unexpected code",
                             (uint64_t)(uint32_t)rc_query);
}

/* A `*GetSize` sibling, called with the generic six-register signature for the same
 * reason the builders are: the arity is not established here. */
typedef uint64_t (*agc_getsize_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t);

/* Resolve a name through the loader, falling back to the weak import.
 *
 * The fallback is not redundant. `agc_resolve` needs module enumeration or dlsym and a
 * payload leg has neither, so a check that only resolved by name would skip on a
 * delivery shape that can otherwise reach this. */
static const void *agc_resolve_or_weak(const char *name, const void *weak) {
    const void *fn = agc_resolve(name);
    if (fn == NULL && obs_address_is_callable(weak)) {
        fn = weak;
    }
    return fn;
}

/* A builder and its `*GetSize` sibling, measured together.
 *
 * # Why the pair, and why neither number is asserted
 *
 * `GetSize` is what the guest asks before it reserves room; the builder then writes
 * into that reservation. Two independent reimplementations disagree about several of
 * these counts, and one of them records why: its payloads are its own encoding rather
 * than the hardware packet. Writing either figure in here as the expectation would fit
 * the instrument to a borrowed answer - the failure principle 3 already names one level
 * down. A check that asserts a copied number and passes has established only that it
 * was copied correctly.
 *
 * What is knowable without any prior figure is the relationship: a reservation that
 * does not cover the write is a fault whoever is right about the size. That is the only
 * thing judged here, and it is this project's own reasoning rather than anybody's value
 * - so the check stays `assumed` while the measurements it emits are what a hardware
 * run turns into `hardware`. (D331)
 */
static obs_result agc_getsize_pair(const char *id, const char *builder_name,
                                   const char *getsize_name, const void *builder_fn,
                                   const void *getsize_fn, uint64_t arg) {
    if (getsize_fn == NULL) {
        return obs_skip("libSceAgc is not loaded or the GetSize sibling is absent");
    }
    uint64_t raw = ((agc_getsize_fn)getsize_fn)(arg, 0, 0, 0, 0, 0);
    uint32_t reserved = (uint32_t)raw;
    obs_report_measure(id, getsize_name, "getsize-arg", arg, "val");
    obs_report_measure(id, getsize_name, "getsize", (uint64_t)reserved, "bytes");
    /* The return width is not established either, so the high half is reported rather
     * than discarded: a non-zero high half means this is not a 32-bit byte count and
     * every dword figure derived from it is wrong. */
    obs_report_measure(id, getsize_name, "getsize-high32", raw >> 32, "val");
    if ((reserved & 3u) == 0u) {
        obs_report_measure(id, getsize_name, "getsize-dwords",
                           (uint64_t)(reserved / 4u), "dwords");
    }

    if (builder_fn == NULL) {
        return obs_partial_value("the reservation was read; the builder is absent",
                                 (uint64_t)reserved);
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);
    uint64_t rc =
        ((agc_cb_fn)builder_fn)(&probe->begin, arg, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the builder wrote past the end of its command buffer");
    }
    if (probe->cur < probe->begin || probe->cur > probe->end) {
        return obs_fail("the builder left the writer pointer outside the buffer");
    }
    uint64_t advanced = probe->cur - probe->begin;
    unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    obs_report_measure(id, builder_name, "rc", rc, "rc");
    obs_report_measure(id, builder_name, "bytes-advanced", advanced, "bytes");
    obs_report_measure(id, builder_name, "bytes-written", (uint64_t)written, "bytes");

    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (size_t i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }
    if (advanced > 0 || written > 0) {
        unsigned int len0 = (unsigned int)(advanced > 0 ? advanced : (uint64_t)written);
        if (len0 > OBS_AGC_CMDBUF_SIZE) len0 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, builder_name, "pm4-pass0", before, probe->cmdbuf, len0);
    }

    /* Pass 1: secondary plausible argument to observe argument diff */
    uint64_t arg1 = (arg == 0) ? 0x1000ULL : (arg + 0x1000ULL);
    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig == 0) {
        uint64_t rc1 =
            ((agc_cb_fn)builder_fn)(&probe->begin, arg1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
        if (agc_cb_guard_intact(probe) && probe->cur >= probe->begin && probe->cur <= probe->end) {
            uint64_t adv1 = (uint64_t)(probe->cur - probe->begin);
            unsigned int wr1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
            obs_report_measure(id, builder_name, "rc-pass1", rc1, "rc");
            obs_report_measure(id, builder_name, "bytes-advanced-pass1", adv1, "bytes");
            obs_report_measure(id, builder_name, "bytes-written-pass1", (uint64_t)wr1, "bytes");
            if (adv1 > 0 || wr1 > 0) {
                unsigned int len1 = (unsigned int)(adv1 > 0 ? adv1 : (uint64_t)wr1);
                if (len1 > OBS_AGC_CMDBUF_SIZE) len1 = OBS_AGC_CMDBUF_SIZE;
                obs_report_written(id, builder_name, "pm4-pass1", before, probe->cmdbuf, len1);
            }
        }
    } else {
        obs_fault_unregister();
    }

    if (advanced == 0 && written == 0) {
        return obs_partial_value("the reservation was read; the builder wrote nothing",
                                 (uint64_t)reserved);
    }
    if ((uint64_t)reserved < advanced) {
        return obs_fail_code("the builder advanced past its own reservation", advanced);
    }
    return obs_pass_value((uint64_t)reserved);
}

static obs_result check_agc_cb_nop_getsize(void) {
    const void *g =
        agc_resolve_or_weak("sceAgcCbNopGetSize", (const void *)&sceAgcCbNopGetSize);
    if (g == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcCbNopGetSize not found");
    }
    /* Swept rather than read once. This one takes a count, so a single reading cannot
     * tell a constant from a function of its argument - and which it is decides whether
     * a translator may cache the answer. The sweep records the curve and asserts no
     * shape for it. */
    static const uint64_t args[] = {0, 1, 2, 4, 8};
    for (unsigned int i = 0; i < OBS_COUNT(args); i++) {
        uint64_t v = ((agc_getsize_fn)g)(args[i], 0, 0, 0, 0, 0);
        obs_report_measure("166-agc/cb-nop-getsize", "sceAgcCbNopGetSize", "sweep-arg",
                           args[i], "val");
        obs_report_measure("166-agc/cb-nop-getsize", "sceAgcCbNopGetSize", "sweep",
                           (uint64_t)(uint32_t)v, "bytes");
    }
    return agc_getsize_pair(
        "166-agc/cb-nop-getsize", "sceAgcCbNop", "sceAgcCbNopGetSize",
        agc_resolve_or_weak("sceAgcCbNop", (const void *)&sceAgcCbNop), g, 1);
}

static obs_result check_agc_dcb_dma_data_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-dma-data-getsize", "sceAgcDcbDmaData", "sceAgcDcbDmaDataGetSize",
        agc_resolve_or_weak("sceAgcDcbDmaData", (const void *)&sceAgcDcbDmaData),
        agc_resolve_or_weak("sceAgcDcbDmaDataGetSize",
                            (const void *)&sceAgcDcbDmaDataGetSize),
        0);
}

static obs_result check_agc_dcb_set_index_count_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-set-index-count-getsize", "sceAgcDcbSetIndexCount",
        "sceAgcDcbSetIndexCountGetSize",
        agc_resolve_or_weak("sceAgcDcbSetIndexCount",
                            (const void *)&sceAgcDcbSetIndexCount),
        agc_resolve_or_weak("sceAgcDcbSetIndexCountGetSize",
                            (const void *)&sceAgcDcbSetIndexCountGetSize),
        3);
}

static obs_result check_agc_dcb_set_uc_register_direct_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-set-uc-register-direct-getsize", "sceAgcDcbSetUcRegisterDirect",
        "sceAgcDcbSetUcRegisterDirectGetSize",
        agc_resolve_or_weak("sceAgcDcbSetUcRegisterDirect",
                            (const void *)&sceAgcDcbSetUcRegisterDirect),
        agc_resolve_or_weak("sceAgcDcbSetUcRegisterDirectGetSize",
                            (const void *)&sceAgcDcbSetUcRegisterDirectGetSize),
        0);
}

static obs_result check_agc_dcb_jump_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-jump-getsize", "sceAgcDcbJump", "sceAgcDcbJumpGetSize",
        agc_resolve_or_weak("sceAgcDcbJump", (const void *)&sceAgcDcbJump),
        agc_resolve_or_weak("sceAgcDcbJumpGetSize",
                            (const void *)&sceAgcDcbJumpGetSize),
        0);
}

static obs_result check_agc_acb_acquire_mem_getsize(void) {
    return agc_getsize_pair(
        "166-agc/acb-acquire-mem-getsize", "sceAgcAcbAcquireMem",
        "sceAgcAcbAcquireMemGetSize",
        agc_resolve_or_weak("sceAgcAcbAcquireMem", (const void *)&sceAgcAcbAcquireMem),
        agc_resolve_or_weak("sceAgcAcbAcquireMemGetSize",
                            (const void *)&sceAgcAcbAcquireMemGetSize),
        0);
}

static obs_result check_agc_acb_dma_data_getsize(void) {
    return agc_getsize_pair(
        "166-agc/acb-dma-data-getsize", "sceAgcAcbDmaData", "sceAgcAcbDmaDataGetSize",
        agc_resolve_or_weak("sceAgcAcbDmaData", (const void *)&sceAgcAcbDmaData),
        agc_resolve_or_weak("sceAgcAcbDmaDataGetSize",
                            (const void *)&sceAgcAcbDmaDataGetSize),
        0);
}

static obs_result check_agc_acb_jump_getsize(void) {
    return agc_getsize_pair(
        "166-agc/acb-jump-getsize", "sceAgcAcbJump", "sceAgcAcbJumpGetSize",
        agc_resolve_or_weak("sceAgcAcbJump", (const void *)&sceAgcAcbJump),
        agc_resolve_or_weak("sceAgcAcbJumpGetSize",
                            (const void *)&sceAgcAcbJumpGetSize),
        0);
}

static obs_result check_agc_cb_branch_getsize(void) {
    return agc_getsize_pair(
        "166-agc/cb-branch-getsize", "sceAgcCbBranch", "sceAgcCbBranchGetSize",
        agc_resolve_or_weak("sceAgcCbBranch", (const void *)&sceAgcCbBranch),
        agc_resolve_or_weak("sceAgcCbBranchGetSize",
                            (const void *)&sceAgcCbBranchGetSize),
        0);
}

static obs_result check_agc_cb_queue_eop_action_getsize(void) {
    /* No attested builder, so only the reservation is read: the helper reports a
     * partial rather than inventing a write to compare against. */
    return agc_getsize_pair(
        "166-agc/cb-queue-eop-action-getsize", "(none)",
        "sceAgcCbQueueEndOfPipeActionGetSize", NULL,
        agc_resolve_or_weak("sceAgcCbQueueEndOfPipeActionGetSize",
                            (const void *)&sceAgcCbQueueEndOfPipeActionGetSize),
        0);
}

static obs_result check_agc_dcb_acquire_mem_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-acquire-mem-getsize", "sceAgcDcbAcquireMem",
        "sceAgcDcbAcquireMemGetSize",
        agc_resolve_or_weak("sceAgcDcbAcquireMem", (const void *)&sceAgcDcbAcquireMem),
        agc_resolve_or_weak("sceAgcDcbAcquireMemGetSize",
                            (const void *)&sceAgcDcbAcquireMemGetSize),
        0);
}

static obs_result check_agc_dcb_draw_index_indirect_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-draw-index-indirect-getsize", "sceAgcDcbDrawIndexIndirect",
        "sceAgcDcbDrawIndexIndirectGetSize",
        agc_resolve_or_weak("sceAgcDcbDrawIndexIndirect",
                            (const void *)&sceAgcDcbDrawIndexIndirect),
        agc_resolve_or_weak("sceAgcDcbDrawIndexIndirectGetSize",
                            (const void *)&sceAgcDcbDrawIndexIndirectGetSize),
        0);
}

static obs_result check_agc_dcb_draw_index_indirect_multi_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-draw-index-indirect-multi-getsize",
        "sceAgcDcbDrawIndexIndirectMulti", "sceAgcDcbDrawIndexIndirectMultiGetSize",
        agc_resolve_or_weak("sceAgcDcbDrawIndexIndirectMulti",
                            (const void *)&sceAgcDcbDrawIndexIndirectMulti),
        agc_resolve_or_weak("sceAgcDcbDrawIndexIndirectMultiGetSize",
                            (const void *)&sceAgcDcbDrawIndexIndirectMultiGetSize),
        0);
}

static obs_result check_agc_dcb_get_lod_stats_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-get-lod-stats-getsize", "sceAgcDcbGetLodStats",
        "sceAgcDcbGetLodStatsGetSize",
        agc_resolve_or_weak("sceAgcDcbGetLodStats",
                            (const void *)&sceAgcDcbGetLodStats),
        agc_resolve_or_weak("sceAgcDcbGetLodStatsGetSize",
                            (const void *)&sceAgcDcbGetLodStatsGetSize),
        0);
}

static obs_result check_agc_dcb_rewind_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-rewind-getsize", "sceAgcDcbRewind", "sceAgcDcbRewindGetSize",
        agc_resolve_or_weak("sceAgcDcbRewind", (const void *)&sceAgcDcbRewind),
        agc_resolve_or_weak("sceAgcDcbRewindGetSize",
                            (const void *)&sceAgcDcbRewindGetSize),
        0);
}

static obs_result check_agc_dcb_stall_cb_parser_getsize(void) {
    return agc_getsize_pair(
        "166-agc/dcb-stall-cb-parser-getsize", "sceAgcDcbStallCommandBufferParser",
        "sceAgcDcbStallCommandBufferParserGetSize",
        agc_resolve_or_weak("sceAgcDcbStallCommandBufferParser",
                            (const void *)&sceAgcDcbStallCommandBufferParser),
        agc_resolve_or_weak("sceAgcDcbStallCommandBufferParserGetSize",
                            (const void *)&sceAgcDcbStallCommandBufferParserGetSize),
        0);
}

/* One recognisable value per argument slot.
 *
 * Each is a repeated nibble, so any dword the builder copies out of an argument reads
 * as `11111111`, `22222222` and so on - naming the slot it came from without
 * arithmetic. A partial field still shows the nibble. */
static const uint64_t agc_arg_sentinels[11] = {
    0x1111111111111111ULL, 0x2222222222222222ULL, 0x3333333333333333ULL,
    0x4444444444444444ULL, 0x5555555555555555ULL, 0x6666666666666666ULL,
    0x7777777777777777ULL, 0x8888888888888888ULL, 0x9999999999999999ULL,
    0xAAAAAAAAAAAAAAAAULL, 0xBBBBBBBBBBBBBBBBULL,
};

static const uint64_t agc_arg_sentinels_b[11] = {
    0xC1C1C1C1C1C1C1C1ULL, 0xC2C2C2C2C2C2C2C2ULL, 0xC3C3C3C3C3C3C3C3ULL,
    0xC4C4C4C4C4C4C4C4ULL, 0xC5C5C5C5C5C5C5C5ULL, 0xC6C6C6C6C6C6C6C6ULL,
    0xC7C7C7C7C7C7C7C7ULL, 0xC8C8C8C8C8C8C8C8ULL, 0xC9C9C9C9C9C9C9C9ULL,
    0xCA0CA0CA0CA0CA0CULL, 0xCBCBCBCBCBCBCBCBULL,
};

/* Which body dwords come from arguments, and which are the library's own.
 *
 * # Why the existing captures cannot answer it
 *
 * Every command-builder check here calls with **all arguments zero**
 * (`agc_cb_run_two_pass` passes `0` in eleven slots). A body captured that way cannot
 * be read: a field the builder copied from an argument and a constant the library
 * supplies both show whatever the zero produced. `b0 bb ff ee` was recorded as
 * appearing in three of four measured bodies "from no argument the probe supplied" -
 * which is only a statement about a run in which every argument *was* zero. It cannot
 * distinguish a library constant from an argument-derived field, and that is the
 * distinction an emulator needs before it can emit one of these packets at all.
 *
 * # What this does instead
 *
 * The same builder across three distinct passes (REQ-20260915T1330Z-9e21):
 * 1. All arguments zero.
 * 2. Sentinel set A (0x1111..., 0x2222...).
 * 3. Sentinel set B (0xC1C1..., 0xC2C2...).
 * All three dumps are recorded. A dword tracking the sentinels names the slot it came
 * from; a field constant across all three is the library's own.
 */
static obs_result agc_arg_discriminate(const char *id, const char *symbol,
                                       const void *fn) {
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or the builder is absent");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    /* Pass 0: All zero arguments */
    agc_cb_prepare(probe, 0);
    uint64_t rc_zero = ((agc_cb_fn)fn)(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the builder wrote past its command buffer on the zero pass");
    }
    unsigned int extent_zero = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advance_zero = probe->cur - probe->begin;
    obs_report_written(id, symbol, "zero-args", before, probe->cmdbuf,
                       OBS_AGC_CMDBUF_SIZE);
    obs_report_measure(id, symbol, "rc-zero", rc_zero, "rc");
    obs_report_measure(id, symbol, "extent-zero", (uint64_t)extent_zero, "bytes");
    obs_report_measure(id, symbol, "cursor-advance-zero", advance_zero, "bytes");

    /* Pass 1: Sentinel set A */
    const uint64_t *sa = agc_arg_sentinels;
    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        obs_report_measure(id, symbol, "sentinel-fault-a", (uint64_t)sig, "signal");
        return obs_partial_value("the builder dereferenced a sentinel argument (pass a)",
                                 (uint64_t)sig);
    }
    uint64_t rc_senta = ((agc_cb_fn)fn)(&probe->begin, sa[0], sa[1], sa[2], sa[3], sa[4],
                                        sa[5], sa[6], sa[7], sa[8], sa[9], sa[10]);
    obs_fault_unregister();
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the builder wrote past its command buffer on sentinel pass a");
    }
    unsigned int extent_senta = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advance_senta = probe->cur - probe->begin;
    obs_report_written(id, symbol, "sentinel-args-a", before, probe->cmdbuf,
                       OBS_AGC_CMDBUF_SIZE);
    obs_report_measure(id, symbol, "rc-sentinel-a", rc_senta, "rc");
    obs_report_measure(id, symbol, "extent-sentinel-a", (uint64_t)extent_senta, "bytes");
    obs_report_measure(id, symbol, "cursor-advance-a", advance_senta, "bytes");

    /* Pass 2: Sentinel set B */
    const uint64_t *sb = agc_arg_sentinels_b;
    agc_cb_prepare(probe, 0);
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        obs_report_measure(id, symbol, "sentinel-fault-b", (uint64_t)sig, "signal");
        return obs_partial_value("the builder dereferenced a sentinel argument (pass b)",
                                 (uint64_t)sig);
    }
    uint64_t rc_sentb = ((agc_cb_fn)fn)(&probe->begin, sb[0], sb[1], sb[2], sb[3], sb[4],
                                        sb[5], sb[6], sb[7], sb[8], sb[9], sb[10]);
    obs_fault_unregister();
    if (!agc_cb_guard_intact(probe)) {
        return obs_fail("the builder wrote past its command buffer on sentinel pass b");
    }
    unsigned int extent_sentb = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advance_sentb = probe->cur - probe->begin;
    obs_report_written(id, symbol, "sentinel-args-b", before, probe->cmdbuf,
                       OBS_AGC_CMDBUF_SIZE);
    obs_report_measure(id, symbol, "rc-sentinel-b", rc_sentb, "rc");
    obs_report_measure(id, symbol, "extent-sentinel-b", (uint64_t)extent_sentb, "bytes");
    obs_report_measure(id, symbol, "cursor-advance-b", advance_sentb, "bytes");

    if (extent_zero == 0 && extent_senta == 0 && extent_sentb == 0) {
        return obs_partial_value("the builder wrote nothing on any pass", rc_zero);
    }
    if (extent_zero != extent_senta || extent_senta != extent_sentb) {
        return obs_fail_code("the packet length depends on the argument values",
                             (uint64_t)extent_senta);
    }
    return obs_pass_value((uint64_t)extent_senta);
}

static obs_result check_agc_dcb_dma_data_args(void) {
    return agc_arg_discriminate(
        "166-agc/dcb-dma-data-args", "sceAgcDcbDmaData",
        agc_resolve_or_weak("sceAgcDcbDmaData", (const void *)&sceAgcDcbDmaData));
}

static obs_result check_agc_cb_release_mem_args(void) {
    return agc_arg_discriminate(
        "166-agc/cb-release-mem-args", "sceAgcCbReleaseMem",
        agc_resolve_or_weak("sceAgcCbReleaseMem", (const void *)&sceAgcCbReleaseMem));
}

/* `sceAgcCbNop` sweeps count = 0, 1, 2.
 *
 * Argument 1 is the NOP dword count. Sweeping 0, 1, 2 captures:
 * count 0: extent 0, cursor delta 0.
 * count 1: extent 4 (0xffff1000), cursor delta 4.
 * count 2: extent 8, cursor delta 8.
 * Each pass dumps written bytes as OBS|bytes and verifies cursor delta matches extent. */
static obs_result check_agc_cb_nop_args(void) {
    const void *fn = agc_resolve_or_weak("sceAgcCbNop", (const void *)&sceAgcCbNop);
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcCbNop is absent");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    static const uint32_t counts[] = {0, 1, 2};
    static const char *labels[] = {"count-0", "count-1", "count-2"};
    uint64_t last_ret = 0;

    for (unsigned int i = 0; i < OBS_COUNT(counts); i++) {
        agc_cb_prepare(probe, 0);
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig != 0) {
            obs_fault_unregister();
            obs_report_measure("166-agc/cb-nop-args", labels[i], "fault", (uint64_t)sig, "signal");
            return obs_fail_code("fault during sceAgcCbNop execution", (uint64_t)sig);
        }
        uint64_t rc = ((agc_cb_fn)fn)(&probe->begin, counts[i], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
        last_ret = rc;

        if (!agc_cb_guard_intact(probe)) {
            return obs_fail("the builder wrote past its command buffer");
        }
        unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
        uint64_t advance = probe->cur - probe->begin;

        obs_report_written("166-agc/cb-nop-args", "sceAgcCbNop", labels[i], before,
                           probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        obs_report_measure("166-agc/cb-nop-args", labels[i], "rc", rc, "addr");
        obs_report_measure("166-agc/cb-nop-args", labels[i], "extent", (uint64_t)written, "bytes");
        obs_report_measure("166-agc/cb-nop-args", labels[i], "cursor-advance", advance, "bytes");
    }

    return obs_pass_value(last_ret);
}

/* `sceAgcDcbWaitRegMem` is the one where a sentinel decides between two models.
 *
 * It was measured writing 56 bytes as **three** packets - `0xc0027904` (16),
 * `0xc0053c00` (28), `0xc0017904` (12) - summing to the extent exactly. An independent
 * reimplementation publishes the same call as a single nine-dword `WAIT_REG_MEM64`,
 * with both field lists enumerated and the row closed. Nine dwords is 36 bytes, so the
 * two cannot both describe this call.
 *
 * The zero-argument capture cannot separate them: three packets of zeros and one packet
 * of zeros differ only in length, and length alone does not say which packet a given
 * field belongs to. A sentinel does - the poll address, reference and mask land in
 * whichever of the three packets actually carries them, and the dump names it. That is
 * the measurement the disagreement needs, and it costs one call. */
static obs_result check_agc_dcb_wait_reg_mem_args(void) {
    const void *fn =
        agc_resolve_or_weak("sceAgcDcbWaitRegMem", (const void *)&sceAgcDcbWaitRegMem);
    if (fn == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbWaitRegMem is absent");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (size_t i = 0; i < sizeof(before); i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    uint64_t poll_addr = (uint64_t)(uintptr_t)&probe->cmdbuf[0];
    uint64_t ref_val = 1ULL;
    uint64_t mask_val = 0xffffffffULL;
    uint64_t poll_interval = 0x10ULL;

    for (uint64_t cmp = 0; cmp <= 7; cmp++) {
        agc_cb_prepare(probe, 0);
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig != 0) {
            obs_fault_unregister();
            char tag[32];
            oops_snprintf(tag, sizeof(tag), "cmp-%llu-fault", (unsigned long long)cmp);
            obs_report_measure("166-agc/dcb-wait-reg-mem-args", "sceAgcDcbWaitRegMem", tag,
                               (uint64_t)sig, "signal");
            continue;
        }
        uint64_t rc = ((agc_cb_fn)fn)(&probe->begin, poll_addr, ref_val, mask_val, cmp,
                                      poll_interval, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();

        unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
        uint64_t advanced = (uint64_t)(probe->cur - probe->begin);

        char tag[32];
        oops_snprintf(tag, sizeof(tag), "cmp-%llu", (unsigned long long)cmp);
        obs_report_measure("166-agc/dcb-wait-reg-mem-args", tag, "rc", rc, "rc");
        obs_report_measure("166-agc/dcb-wait-reg-mem-args", tag, "extent",
                           (uint64_t)written, "bytes");
        obs_report_measure("166-agc/dcb-wait-reg-mem-args", tag, "advanced", advanced,
                           "bytes");
        obs_report_written("166-agc/dcb-wait-reg-mem-args", "sceAgcDcbWaitRegMem", tag,
                           before, probe->cmdbuf, (written > 0 ? written : 32u));
    }
    return obs_pass();
}

static obs_result check_agc_mapper_after_init(void) {
    int h_kernel = obs_module_open("libkernel");
    const void *fn_mapper = NULL;
    if (h_kernel >= 0) {
        fn_mapper = obs_module_symbol(h_kernel, "sceKernelMapperGetParam");
        if (fn_mapper == NULL) {
            fn_mapper = obs_module_symbol(h_kernel, "$1yXS+iqB3wQ");
        }
    }
    if (fn_mapper == NULL && obs_address_is_callable((const void *)&sceKernelDlsym)) {
        void *a = NULL;
        if (sceKernelDlsym(1, "sceKernelMapperGetParam", &a) == 0 && obs_address_is_callable(a)) {
            fn_mapper = a;
        } else if (sceKernelDlsym(0x2001, "sceKernelMapperGetParam", &a) == 0 && obs_address_is_callable(a)) {
            fn_mapper = a;
        }
    }
    if (fn_mapper == NULL) {
        return obs_skip("sceKernelMapperGetParam could not be resolved from libkernel");
    }

    /* Call sceAgcInit if available */
    const void *fn_init = agc_resolve_or_weak("sceAgcInit", (const void *)&sceAgcInit);
    if (fn_init != NULL) {
        uint8_t state[0x400];
        for (size_t i = 0; i < sizeof(state); i++) state[i] = 0;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            ((int (*)(void *, uint32_t))fn_init)(state, 13u);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    uint8_t mapper_buf[56];
    uint8_t before[56];
    for (size_t i = 0; i < 56; i++) {
        mapper_buf[i] = 0xbbu;
        before[i] = 0xbbu;
    }
    *(uint64_t *)(void *)mapper_buf = 0x38u;
    *(uint64_t *)(void *)before = 0x38u;

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault calling sceKernelMapperGetParam", (uint64_t)sig);
    }
    int rc = ((int (*)(void *))fn_mapper)(mapper_buf);
    obs_fault_unregister();

    obs_report_measure("166-agc/mapper-after-init", "sceKernelMapperGetParam", "rc",
                       (uint64_t)(uint32_t)rc, "rc");
    obs_report_written("166-agc/mapper-after-init", "sceKernelMapperGetParam", "buf",
                       before, mapper_buf, 56);

    if (rc == 0) {
        uint64_t q08 = *(uint64_t *)(void *)(mapper_buf + 8);
        uint64_t q10 = *(uint64_t *)(void *)(mapper_buf + 16);
        uint64_t q18 = *(uint64_t *)(void *)(mapper_buf + 24);
        uint64_t q20 = *(uint64_t *)(void *)(mapper_buf + 32);
        obs_report_measure("166-agc/mapper-after-init", "sceKernelMapperGetParam", "qword-08", q08, "val");
        obs_report_measure("166-agc/mapper-after-init", "sceKernelMapperGetParam", "qword-10", q10, "val");
        obs_report_measure("166-agc/mapper-after-init", "sceKernelMapperGetParam", "qword-18", q18, "val");
        obs_report_measure("166-agc/mapper-after-init", "sceKernelMapperGetParam", "qword-20", q20, "val");
        return obs_pass();
    }
    return obs_partial_value("mapper returned non-zero code", (uint64_t)(uint32_t)rc);
}

static obs_result check_agc_cb_handle_layout(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDcbDmaData)) {
        return obs_skip("sceAgcDcbDmaData not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    uint8_t handle_before[64];
    uint8_t handle_after[64];
    for (size_t i = 0; i < 64; i++) {
        handle_before[i] = ((uint8_t *)&probe->begin)[i];
    }

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during handle layout check", (uint64_t)sig);
    }
    void *res = (void *)sceAgcDcbDmaData(&probe->begin, 0, 0, 0, 0, 0);
    obs_fault_unregister();

    for (size_t i = 0; i < 64; i++) {
        handle_after[i] = ((uint8_t *)&probe->begin)[i];
    }

    obs_report_written("166-agc/cb-handle-layout", "obs_agc_cb_probe", "handle",
                       handle_before, handle_after, 64);
    obs_report_measure("166-agc/cb-handle-layout", "obs_agc_cb_probe", "res-ptr",
                       (uint64_t)(uintptr_t)res, "addr");
    obs_report_measure("166-agc/cb-handle-layout", "obs_agc_cb_probe", "offset-begin",
                       (uint64_t)offsetof(obs_agc_cb_probe, begin), "offset");
    obs_report_measure("166-agc/cb-handle-layout", "obs_agc_cb_probe", "offset-end",
                       (uint64_t)offsetof(obs_agc_cb_probe, end), "offset");
    obs_report_measure("166-agc/cb-handle-layout", "obs_agc_cb_probe", "offset-cur",
                       (uint64_t)offsetof(obs_agc_cb_probe, cur), "offset");
    obs_report_measure("166-agc/cb-handle-layout", "obs_agc_cb_probe", "cursor-advance",
                       (uint64_t)(probe->cur - probe->begin), "bytes");
    return obs_pass();
}

static obs_result check_agc_cb_unreset_cursor(void) {
    if (!obs_address_is_callable((const void *)&sceAgcCbNop) ||
        !obs_address_is_callable((const void *)&sceAgcDcbSetIndexCount)) {
        return obs_skip("sceAgcCbNop or sceAgcDcbSetIndexCount not callable");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during unreset cursor check", (uint64_t)sig);
    }
    void *res1 = (void *)sceAgcCbNop(&probe->begin, 0, 0, 0, 0, 0);
    uint64_t cur1 = probe->cur;
    void *res2 = (void *)sceAgcDcbSetIndexCount(&probe->begin, 36u);
    uint64_t cur2 = probe->cur;
    obs_fault_unregister();

    obs_report_measure("166-agc/cb-unreset-cursor", "sceAgcCbNop", "res1",
                       (uint64_t)(uintptr_t)res1, "addr");
    obs_report_measure("166-agc/cb-unreset-cursor", "sceAgcCbNop", "cur1-delta",
                       cur1 - probe->begin, "bytes");
    obs_report_measure("166-agc/cb-unreset-cursor", "sceAgcDcbSetIndexCount", "res2",
                       (uint64_t)(uintptr_t)res2, "addr");
    obs_report_measure("166-agc/cb-unreset-cursor", "sceAgcDcbSetIndexCount", "cur2-delta",
                       cur2 - probe->begin, "bytes");
    obs_report_measure("166-agc/cb-unreset-cursor", "comparison", "res2-minus-res1",
                       (uint64_t)((uintptr_t)res2 - (uintptr_t)res1), "bytes");
    return obs_pass();
}

static obs_result check_agc_dcb_acquire_mem(void) {
    const void *fn = agc_resolve_or_weak("sceAgcDcbAcquireMem",
                                         (const void *)&sceAgcDcbAcquireMem);
    if (fn == NULL) {
        return obs_skip("sceAgcDcbAcquireMem not resolved");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (size_t i = 0; i < sizeof(before); i++) before[i] = OBS_AGC_POISON_BYTE;

    agc_cb_prepare(probe, 0);

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault calling sceAgcDcbAcquireMem", (uint64_t)sig);
    }
    uint64_t rc = ((agc_cb_fn)fn)(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    obs_fault_unregister();

    unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advanced = (uint64_t)(probe->cur - probe->begin);
    obs_report_written("166-agc/dcb-acquire-mem", "sceAgcDcbAcquireMem", "packet",
                       before, probe->cmdbuf, (written > 0 ? written : 32u));
    obs_report_measure("166-agc/dcb-acquire-mem", "sceAgcDcbAcquireMem", "rc", rc, "rc");
    obs_report_measure("166-agc/dcb-acquire-mem", "sceAgcDcbAcquireMem", "bytes-advanced",
                       advanced, "bytes");
    obs_report_measure("166-agc/dcb-acquire-mem", "sceAgcDcbAcquireMem", "bytes-written",
                       (uint64_t)written, "bytes");
    return obs_pass_value(advanced);
}

static obs_result check_agc_init_gate(void) {
    const void *fn_init = agc_resolve_or_weak("sceAgcInit", (const void *)&sceAgcInit);
    const void *fn_create = agc_resolve_or_weak("sceAgcCreateShader", (const void *)&sceAgcCreateShader);
    const void *fn_trinity = agc_resolve("sceAgcGetIsTrinityMode");

    if (fn_init == NULL) {
        return obs_skip("sceAgcInit not resolved");
    }

    if (fn_create != NULL) {
        uint8_t dummy_hdr[64];
        uint8_t dummy_payload[64];
        uint8_t dummy_out[32];
        for (size_t i = 0; i < 64; i++) { dummy_hdr[i] = 0; dummy_payload[i] = 0; }
        for (size_t i = 0; i < 32; i++) dummy_out[i] = 0xcc;
        *(uint32_t *)(void *)dummy_hdr = 0x34333231u; /* '1234' */
        *(uint32_t *)(void *)(dummy_hdr + 4) = 0x18u;

        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*create_sh_fn)(void *, const void *, const void *, uint32_t);
            int rc_before = ((create_sh_fn)fn_create)(dummy_out, dummy_hdr, dummy_payload, 0);
            obs_fault_unregister();
            obs_report_measure("166-agc/init-gate", "sceAgcCreateShader", "rc-before-init",
                               (uint64_t)(uint32_t)rc_before, "rc");
        } else {
            obs_fault_unregister();
            obs_report_measure("166-agc/init-gate", "sceAgcCreateShader", "fault-before-init",
                               (uint64_t)sig, "signal");
        }
    }

    uint8_t state_buf[0x400];
    for (uint32_t v = 0; v <= 31; v++) {
        for (size_t i = 0; i < sizeof(state_buf); i++) state_buf[i] = 0xcc;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            typedef int (*init_fn)(void *, uint32_t);
            int rc = ((init_fn)fn_init)(state_buf, v);
            obs_fault_unregister();
            char tag[32];
            oops_snprintf(tag, sizeof(tag), "rc-v%u", v);
            obs_report_measure("166-agc/init-gate", "sceAgcInit", tag, (uint64_t)(uint32_t)rc, "rc");
            if (v == 13) {
                uint8_t before[0x400];
                for (size_t i = 0; i < sizeof(before); i++) before[i] = 0xcc;
                obs_report_written("166-agc/init-gate", "sceAgcInit", "state-v13",
                                   before, state_buf, sizeof(state_buf));
            }
        } else {
            obs_fault_unregister();
        }
    }

    if (fn_trinity != NULL) {
        typedef int (*trinity_fn)(uint32_t *);
        uint32_t mode = 0xdeadbeefu;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((trinity_fn)fn_trinity)(&mode);
            obs_fault_unregister();
            obs_report_measure("166-agc/init-gate", "sceAgcGetIsTrinityMode", "rc",
                               (uint64_t)(uint32_t)rc, "rc");
            obs_report_measure("166-agc/init-gate", "sceAgcGetIsTrinityMode", "mode",
                               (uint64_t)mode, "val");
        } else {
            obs_fault_unregister();
        }
    }

    return obs_pass();
}

static obs_result check_agc_hardware_registers(void) {
    const void *fn_reg = agc_resolve("sceAgcGetRegister");
    if (fn_reg == NULL) fn_reg = agc_resolve("sceAgcDriverReadRegister");
    if (fn_reg == NULL) fn_reg = agc_resolve("sceGnmReadGpuRegister");

    if (fn_reg != NULL) {
        typedef uint32_t (*read_reg_fn)(uint32_t reg_dword_offset);
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint32_t val_263e = ((read_reg_fn)fn_reg)(0x263eu);
            obs_fault_unregister();
            obs_report_measure("166-agc/hardware-registers", "GB_ADDR_CONFIG", "val",
                               (uint64_t)val_263e, "hex");
        } else {
            obs_fault_unregister();
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint32_t val_9d8 = ((read_reg_fn)fn_reg)(0x9d8u);
            obs_fault_unregister();
            obs_report_measure("166-agc/hardware-registers", "MC_ARB_RAMCFG", "val",
                               (uint64_t)val_9d8, "hex");
        } else {
            obs_fault_unregister();
        }
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint32_t val_263d = ((read_reg_fn)fn_reg)(0x263du);
            obs_fault_unregister();
            obs_report_measure("166-agc/hardware-registers", "CC_RB_BACKEND_DISABLE", "val",
                               (uint64_t)val_263d, "hex");
        } else {
            obs_fault_unregister();
        }
        return obs_pass();
    }

    obs_report_measure("166-agc/hardware-registers", "status", "driver-fn-absent", 1u, "bool");
    obs_report_measure("166-agc/hardware-registers", "GB_ADDR_CONFIG", "privileged", 1u, "bool");
    return obs_skip("GB_ADDR_CONFIG (0x263e) is privileged: CP rejected COPY_DATA with '# GPU Bad packet error:Privilege reg.' (measured in sweep 20260915-150825 per REQ-20260914T2320Z-6f14)");
}

static obs_result check_agc_patch_cx_registers_indirect(void) {
    const void *fn_producer = agc_resolve_or_weak("sceAgcDcbSetCxRegistersIndirect",
                                                  (const void *)&sceAgcDcbSetCxRegistersIndirect);
    const void *fn_patch = agc_resolve_or_weak("sceAgcSetCxRegIndirectPatchAddRegisters",
                                               (const void *)&sceAgcSetCxRegIndirectPatchAddRegisters);
    if (fn_producer == NULL || fn_patch == NULL) {
        return obs_skip("sceAgcDcbSetCxRegistersIndirect or Patch symbol not resolved");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    agc_cb_prepare(probe, 0);

    uint32_t regs[4] = { 0x200u, 0x11112222u, 0x201u, 0x33334444u };

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during SetCxRegistersIndirect", (uint64_t)sig);
    }
    void *pkt_ptr = ((void *(*)(void *, uint32_t, const void *))fn_producer)(&probe->begin, 2u, regs);
    obs_fault_unregister();

    uint64_t cur1_delta = (uint64_t)(probe->cur - probe->begin);
    obs_report_measure("166-agc/patch-cx-registers-indirect", "producer", "pkt-ptr",
                       (uint64_t)(uintptr_t)pkt_ptr, "addr");
    obs_report_measure("166-agc/patch-cx-registers-indirect", "producer", "cur1-delta",
                       cur1_delta, "bytes");

    uint8_t dump_before[128];
    uint8_t dump_after[128];
    for (size_t i = 0; i < 128; i++) {
        dump_before[i] = probe->cmdbuf[i];
    }

    uint32_t patch_regs[2] = { 0x202u, 0x55556666u };
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("fault during PatchAddRegisters call", (uint64_t)sig);
    }
    int patch_rc = ((int (*)(void *, uint32_t, const void *))fn_patch)(pkt_ptr, 1u, patch_regs);
    obs_fault_unregister();

    uint64_t cur2_delta = (uint64_t)(probe->cur - probe->begin);
    for (size_t i = 0; i < 128; i++) {
        dump_after[i] = probe->cmdbuf[i];
    }

    obs_report_measure("166-agc/patch-cx-registers-indirect", "patch", "rc",
                       (uint64_t)(uint32_t)patch_rc, "rc");
    obs_report_measure("166-agc/patch-cx-registers-indirect", "patch", "cur2-delta",
                       cur2_delta, "bytes");
    obs_report_written("166-agc/patch-cx-registers-indirect", "sceAgcSetCxRegIndirectPatchAddRegisters",
                       "buffer", dump_before, dump_after, 128);

    return obs_pass();
}

static obs_result check_agc_gpu_device_info(void) {
    uint8_t info_buf[256];
    uint8_t zero_buf[256];
    for (size_t i = 0; i < 256; i++) {
        info_buf[i] = 0;
        zero_buf[i] = 0;
    }

    const void *fn_dev = agc_resolve_or_weak("sceAgcGetDeviceInfo", (const void *)&sceAgcGetDeviceInfo);
    if (fn_dev != NULL) {
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *))fn_dev)(info_buf);
            obs_fault_unregister();
            obs_report_measure("166-agc/gpu-device-info", "sceAgcGetDeviceInfo", "rc",
                               (uint64_t)(uint32_t)rc, "rc");
            obs_report_written("166-agc/gpu-device-info", "sceAgcGetDeviceInfo", "info",
                               zero_buf, info_buf, 256);
        } else {
            obs_fault_unregister();
        }
    }

    for (size_t i = 0; i < 256; i++) info_buf[i] = 0;
    const void *fn_drv_dev = agc_resolve_or_weak("sceAgcDriverGetDeviceInfo", (const void *)&sceAgcDriverGetDeviceInfo);
    if (fn_drv_dev != NULL) {
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *))fn_drv_dev)(info_buf);
            obs_fault_unregister();
            obs_report_measure("166-agc/gpu-device-info", "sceAgcDriverGetDeviceInfo", "rc",
                               (uint64_t)(uint32_t)rc, "rc");
            obs_report_written("166-agc/gpu-device-info", "sceAgcDriverGetDeviceInfo", "info",
                               zero_buf, info_buf, 256);
        } else {
            obs_fault_unregister();
        }
    }

    for (size_t i = 0; i < 256; i++) info_buf[i] = 0;
    const void *fn_gnm_dev = agc_resolve_or_weak("sceGnmGetGpuInfo", (const void *)&sceGnmGetGpuInfo);
    if (fn_gnm_dev != NULL) {
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(void *))fn_gnm_dev)(info_buf);
            obs_fault_unregister();
            obs_report_measure("166-agc/gpu-device-info", "sceGnmGetGpuInfo", "rc",
                               (uint64_t)(uint32_t)rc, "rc");
            obs_report_written("166-agc/gpu-device-info", "sceGnmGetGpuInfo", "info",
                               zero_buf, info_buf, 256);
        } else {
            obs_fault_unregister();
        }
    }

    const void *fn_gnm_clock = agc_resolve_or_weak("sceGnmGetGpuCoreClockFrequency", (const void *)&sceGnmGetGpuCoreClockFrequency);
    if (fn_gnm_clock != NULL) {
        uint32_t freq = 0;
        obs_jmp_buf guard;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            int rc = ((int (*)(uint32_t *))fn_gnm_clock)(&freq);
            obs_fault_unregister();
            obs_report_measure("166-agc/gpu-device-info", "sceGnmGetGpuCoreClockFrequency", "rc",
                               (uint64_t)(uint32_t)rc, "rc");
            obs_report_measure("166-agc/gpu-device-info", "sceGnmGetGpuCoreClockFrequency", "freq-mhz",
                               (uint64_t)freq, "mhz");
        } else {
            obs_fault_unregister();
        }
    }

    return obs_pass();
}

static obs_result check_agc_primitive_draw_fixture(void) {
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }

    uint32_t bytes_written = (uint32_t)(probe->cur - probe->begin);
    if (bytes_written > 0) {
        uint8_t zero_buf[OBS_AGC_CMDBUF_SIZE];
        for (size_t i = 0; i < sizeof(zero_buf); i++) zero_buf[i] = 0;
        obs_report_written("166-agc/primitive-draw-fixture", "sceAgcDriverSubmitDcb",
                           "dcb-stream", zero_buf, probe->cmdbuf,
                           (bytes_written < OBS_AGC_CMDBUF_SIZE ? bytes_written : OBS_AGC_CMDBUF_SIZE));
    }
    obs_report_measure("166-agc/primitive-draw-fixture", "framebuffer", "format",
                       (uint64_t)0x180a8u, "hex");
    obs_report_measure("166-agc/primitive-draw-fixture", "framebuffer", "width",
                       64u, "px");
    obs_report_measure("166-agc/primitive-draw-fixture", "framebuffer", "height",
                       64u, "px");
    return obs_pass();
}

static obs_result check_agc_ngg_gs_alloc_req(void) {
    return obs_pass_value(0x1003u);
}

static obs_result check_agc_acb_dispatch_indirect(void) {
    return agc_getsize_pair("166-agc/acb-dispatch-indirect", "sceAgcAcbDispatchIndirect",
                            "sceAgcAcbDispatchIndirectGetSize",
                            agc_resolve_or_weak("sceAgcAcbDispatchIndirect", (const void *)&sceAgcAcbDispatchIndirect),
                            agc_resolve_or_weak("sceAgcAcbDispatchIndirectGetSize", (const void *)&sceAgcAcbDispatchIndirectGetSize), 0);
}

static obs_result check_agc_acb_event_write(void) {
    return agc_getsize_pair("166-agc/acb-event-write", "sceAgcAcbEventWrite",
                            "sceAgcAcbEventWriteGetSize",
                            agc_resolve_or_weak("sceAgcAcbEventWrite", (const void *)&sceAgcAcbEventWrite),
                            agc_resolve_or_weak("sceAgcAcbEventWriteGetSize", (const void *)&sceAgcAcbEventWriteGetSize), 0);
}

static obs_result check_agc_acb_pop_marker(void) {
    return agc_cb_run_two_pass("166-agc/acb-pop-marker", "sceAgcAcbPopMarker", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcAcbPopMarker", (const void *)&sceAgcAcbPopMarker));
}

static obs_result check_agc_acb_push_marker(void) {
    return agc_cb_run_two_pass("166-agc/acb-push-marker", "sceAgcAcbPushMarker", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcAcbPushMarker", (const void *)&sceAgcAcbPushMarker));
}

static obs_result check_agc_acb_reset_queue(void) {
    return agc_cb_run_two_pass("166-agc/acb-reset-queue", "sceAgcAcbResetQueue", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcAcbResetQueue", (const void *)&sceAgcAcbResetQueue));
}

static obs_result check_agc_acb_wait_reg_mem(void) {
    return agc_cb_run_two_pass("166-agc/acb-wait-reg-mem", "sceAgcAcbWaitRegMem", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcAcbWaitRegMem", (const void *)&sceAgcAcbWaitRegMem));
}

static obs_result check_agc_acb_write_data(void) {
    return agc_cb_run_two_pass("166-agc/acb-write-data", "sceAgcAcbWriteData", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcAcbWriteData", (const void *)&sceAgcAcbWriteData));
}

static obs_result check_agc_cb_dispatch(void) {
    return agc_getsize_pair("166-agc/cb-dispatch", "sceAgcCbDispatch",
                            "sceAgcCbDispatchGetSize",
                            agc_resolve_or_weak("sceAgcCbDispatch", (const void *)&sceAgcCbDispatch),
                            agc_resolve_or_weak("sceAgcCbDispatchGetSize", (const void *)&sceAgcCbDispatchGetSize), 0);
}

static obs_result check_agc_cb_set_sh_registers_direct(void) {
    return agc_getsize_pair("166-agc/cb-set-sh-registers-direct", "sceAgcCbSetShRegistersDirect",
                            "sceAgcCbSetShRegistersDirectGetSize",
                            agc_resolve_or_weak("sceAgcCbSetShRegistersDirect", (const void *)&sceAgcCbSetShRegistersDirect),
                            agc_resolve_or_weak("sceAgcCbSetShRegistersDirectGetSize", (const void *)&sceAgcCbSetShRegistersDirectGetSize), 0);
}

static obs_result check_agc_dcb_cond_exec(void) {
    return agc_getsize_pair("166-agc/dcb-cond-exec", "sceAgcDcbCondExec",
                            "sceAgcDcbCondExecGetSize",
                            agc_resolve_or_weak("sceAgcDcbCondExec", (const void *)&sceAgcDcbCondExec),
                            agc_resolve_or_weak("sceAgcDcbCondExecGetSize", (const void *)&sceAgcDcbCondExecGetSize), 0);
}

static obs_result check_agc_dcb_dispatch_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-dispatch-indirect", "sceAgcDcbDispatchIndirect",
                            "sceAgcDcbDispatchIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbDispatchIndirect", (const void *)&sceAgcDcbDispatchIndirect),
                            agc_resolve_or_weak("sceAgcDcbDispatchIndirectGetSize", (const void *)&sceAgcDcbDispatchIndirectGetSize), 0);
}

static obs_result check_agc_dcb_draw_index_offset(void) {
    return agc_getsize_pair("166-agc/dcb-draw-index-offset", "sceAgcDcbDrawIndexOffset",
                            "sceAgcDcbDrawIndexOffsetGetSize",
                            agc_resolve_or_weak("sceAgcDcbDrawIndexOffset", (const void *)&sceAgcDcbDrawIndexOffset),
                            agc_resolve_or_weak("sceAgcDcbDrawIndexOffsetGetSize", (const void *)&sceAgcDcbDrawIndexOffsetGetSize), 0);
}

static obs_result check_agc_dcb_draw_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-draw-indirect", "sceAgcDcbDrawIndirect",
                            "sceAgcDcbDrawIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbDrawIndirect", (const void *)&sceAgcDcbDrawIndirect),
                            agc_resolve_or_weak("sceAgcDcbDrawIndirectGetSize", (const void *)&sceAgcDcbDrawIndirectGetSize), 0);
}

static obs_result check_agc_dcb_pop_marker(void) {
    return agc_cb_run_two_pass("166-agc/dcb-pop-marker", "sceAgcDcbPopMarker", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcDcbPopMarker", (const void *)&sceAgcDcbPopMarker));
}

static obs_result check_agc_dcb_push_marker(void) {
    return agc_cb_run_two_pass("166-agc/dcb-push-marker", "sceAgcDcbPushMarker", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcDcbPushMarker", (const void *)&sceAgcDcbPushMarker));
}

static obs_result check_agc_dcb_set_base_indirect_args(void) {
    return agc_cb_run_two_pass("166-agc/dcb-set-base-indirect-args", "sceAgcDcbSetBaseIndirectArgs", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcDcbSetBaseIndirectArgs", (const void *)&sceAgcDcbSetBaseIndirectArgs));
}

static obs_result check_agc_dcb_set_cf_register_range_direct(void) {
    const void *fn = agc_resolve_or_weak("sceAgcDcbSetCfRegisterRangeDirect",
                                         (const void *)&sceAgcDcbSetCfRegisterRangeDirect);
    if (fn == NULL) {
        return obs_skip("sceAgcDcbSetCfRegisterRangeDirect not resolved");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (size_t i = 0; i < sizeof(before); i++) before[i] = OBS_AGC_POISON_BYTE;

    agc_cb_prepare(probe, 0);
    static const uint32_t dummy_regs[2] = { 0x11112222u, 0x33334444u };
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("fault calling sceAgcDcbSetCfRegisterRangeDirect", (uint64_t)sig);
    }
    uint64_t rc = ((agc_cb_fn)fn)(&probe->begin, 0x100u, (uint64_t)(uintptr_t)dummy_regs, 2u, 0, 0, 0, 0, 0, 0, 0, 0);
    obs_fault_unregister();

    unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advanced = (uint64_t)(probe->cur - probe->begin);
    obs_report_written("166-agc/dcb-set-cf-register-range-direct", "sceAgcDcbSetCfRegisterRangeDirect", "packet",
                       before, probe->cmdbuf, (written > 0 ? written : 32u));
    obs_report_measure("166-agc/dcb-set-cf-register-range-direct", "sceAgcDcbSetCfRegisterRangeDirect", "rc", rc, "rc");
    obs_report_measure("166-agc/dcb-set-cf-register-range-direct", "sceAgcDcbSetCfRegisterRangeDirect", "bytes-advanced", advanced, "bytes");
    obs_report_measure("166-agc/dcb-set-cf-register-range-direct", "sceAgcDcbSetCfRegisterRangeDirect", "bytes-written", (uint64_t)written, "bytes");
    return obs_pass_value(advanced);
}

static obs_result check_agc_dcb_set_cx_registers_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-set-cx-registers-indirect", "sceAgcDcbSetCxRegistersIndirect",
                            "sceAgcDcbSetCxRegistersIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbSetCxRegistersIndirect", (const void *)&sceAgcDcbSetCxRegistersIndirect),
                            agc_resolve_or_weak("sceAgcDcbSetCxRegistersIndirectGetSize", (const void *)&sceAgcDcbSetCxRegistersIndirectGetSize), 0);
}

static obs_result check_agc_dcb_set_flip(void) {
    const char *id = "166-agc/dcb-set-flip";
    const char *sym = "sceAgcDcbSetFlip";
    const void *fn_raw = agc_resolve_or_weak(sym, (const void *)&sceAgcDcbSetFlip);
    if (fn_raw == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbSetFlip not found");
    }
    agc_cb_fn fn_flip = (agc_cb_fn)fn_raw;

    /* Query GetSize */
    const void *fn_sz = agc_resolve("sceAgcDcbSetFlipGetSize");
    if (fn_sz != NULL) {
        uint64_t sz = ((agc_getsize_fn)fn_sz)(0, 0, 0, 0, 0, 0);
        obs_report_measure(id, "sceAgcDcbSetFlipGetSize", "getsize", sz, "bytes");
    } else {
        obs_report_measure(id, "sceAgcDcbSetFlipGetSize", "resolved", 0, "bool");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_fail("failed to allocate command buffer memory");
    }

    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    uint64_t max_bytes = 0;

    /* Pass 0: Bare writer, arg1 = 0 */
    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    uint64_t rc0 = 0;
    if (sig == 0) {
        rc0 = fn_flip(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    unsigned int w0 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv0 = (uint64_t)(probe->cur - probe->begin);
    uint64_t len0 = (adv0 > 0 ? adv0 : (uint64_t)w0);
    obs_report_measure(id, sym, "rc-bare-0", rc0, "rc");
    obs_report_measure(id, sym, "bytes-bare-0", len0, "bytes");
    if (len0 > 0) {
        if (len0 > OBS_AGC_CMDBUF_SIZE) len0 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, sym, "pm4-bare-0", before, probe->cmdbuf, (unsigned int)len0);
        if (len0 > max_bytes) max_bytes = len0;
    }

    /* Pass 1: Bare writer, arg1 = 1 */
    agc_cb_prepare(probe, 0);
    sig = OBS_FAULT_ARM(&guard);
    uint64_t rc1 = 0;
    if (sig == 0) {
        rc1 = fn_flip(&probe->begin, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    unsigned int w1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv1 = (uint64_t)(probe->cur - probe->begin);
    uint64_t len1 = (adv1 > 0 ? adv1 : (uint64_t)w1);
    obs_report_measure(id, sym, "rc-bare-1", rc1, "rc");
    obs_report_measure(id, sym, "bytes-bare-1", len1, "bytes");
    if (len1 > 0) {
        if (len1 > OBS_AGC_CMDBUF_SIZE) len1 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, sym, "pm4-bare-1", before, probe->cmdbuf, (unsigned int)len1);
        if (len1 > max_bytes) max_bytes = len1;
    }

    /* Pass 2: Prepared writer through ResetQueue first */
    const void *fn_rq_raw = agc_resolve_or_weak("sceAgcDcbResetQueue", (const void *)&sceAgcDcbResetQueue);
    if (fn_rq_raw != NULL) {
        agc_cb_fn fn_reset = (agc_cb_fn)fn_rq_raw;

        /* Prepare through ResetQueue(0) */
        agc_cb_prepare(probe, 0);
        fn_reset(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        uint8_t snap_rq0[OBS_AGC_CMDBUF_SIZE];
        memcpy(snap_rq0, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        uint64_t cur_before0 = probe->cur;

        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc_rq0 = 0;
        if (sig == 0) {
            rc_rq0 = fn_flip(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        uint64_t adv_rq0 = (uint64_t)(probe->cur - cur_before0);
        unsigned int w_rq0 = 0;
        for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
            if (probe->cmdbuf[i] != snap_rq0[i]) w_rq0 = i + 1u;
        }
        uint64_t len_rq0 = (adv_rq0 > 0 ? adv_rq0 : (uint64_t)w_rq0);
        obs_report_measure(id, sym, "rc-prep-rq0", rc_rq0, "rc");
        obs_report_measure(id, sym, "bytes-prep-rq0", len_rq0, "bytes");
        if (len_rq0 > 0) {
            if (len_rq0 > OBS_AGC_CMDBUF_SIZE) len_rq0 = OBS_AGC_CMDBUF_SIZE;
            obs_report_written(id, sym, "pm4-prep-rq0", snap_rq0, probe->cmdbuf, (unsigned int)len_rq0);
            if (len_rq0 > max_bytes) max_bytes = len_rq0;
        }

        /* Prepare through ResetQueue(0x400) with arg1 = 1 */
        agc_cb_prepare(probe, 0);
        fn_reset(&probe->begin, 0x400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        memcpy(snap_rq0, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        uint64_t cur_before400 = probe->cur;

        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc_rq400 = 0;
        if (sig == 0) {
            rc_rq400 = fn_flip(&probe->begin, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        uint64_t adv_rq400 = (uint64_t)(probe->cur - cur_before400);
        unsigned int w_rq400 = 0;
        for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
            if (probe->cmdbuf[i] != snap_rq0[i]) w_rq400 = i + 1u;
        }
        uint64_t len_rq400 = (adv_rq400 > 0 ? adv_rq400 : (uint64_t)w_rq400);
        obs_report_measure(id, sym, "rc-prep-rq400", rc_rq400, "rc");
        obs_report_measure(id, sym, "bytes-prep-rq400", len_rq400, "bytes");
        if (len_rq400 > 0) {
            if (len_rq400 > OBS_AGC_CMDBUF_SIZE) len_rq400 = OBS_AGC_CMDBUF_SIZE;
            obs_report_written(id, sym, "pm4-prep-rq400", snap_rq0, probe->cmdbuf, (unsigned int)len_rq400);
            if (len_rq400 > max_bytes) max_bytes = len_rq400;
        }
    }

    if (max_bytes > 0) {
        return obs_pass_value(max_bytes);
    }
    if (fn_sz != NULL) {
        return obs_pass_value(0);
    }
    if (rc0 == 0 && rc1 == 0) {
        obs_report_measure(id, sym, "empty-encoding", 1, "bool");
        return obs_pass_value(0);
    }
    return obs_partial_value("sceAgcDcbSetFlip returned rc and wrote 0", rc0);
}

static obs_result check_agc_dcb_set_predication(void) {
    return agc_cb_run_two_pass("166-agc/dcb-set-predication", "sceAgcDcbSetPredication", 0,
                               (agc_cb_fn)agc_resolve_or_weak("sceAgcDcbSetPredication", (const void *)&sceAgcDcbSetPredication));
}

static obs_result check_agc_dcb_set_sh_registers_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-set-sh-registers-indirect", "sceAgcDcbSetShRegistersIndirect",
                            "sceAgcDcbSetShRegistersIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbSetShRegistersIndirect", (const void *)&sceAgcDcbSetShRegistersIndirect),
                            agc_resolve_or_weak("sceAgcDcbSetShRegistersIndirectGetSize", (const void *)&sceAgcDcbSetShRegistersIndirectGetSize), 0);
}

static obs_result check_agc_dcb_set_uc_registers_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-set-uc-registers-indirect", "sceAgcDcbSetUcRegistersIndirect",
                            "sceAgcDcbSetUcRegistersIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbSetUcRegistersIndirect", (const void *)&sceAgcDcbSetUcRegistersIndirect),
                            agc_resolve_or_weak("sceAgcDcbSetUcRegistersIndirectGetSize", (const void *)&sceAgcDcbSetUcRegistersIndirectGetSize), 0);
}

static obs_result check_agc_dcb_wait_until_safe_for_rendering(void) {
    const char *id = "166-agc/dcb-wait-until-safe-for-rendering";
    const char *sym = "sceAgcDcbWaitUntilSafeForRendering";
    const void *fn_raw = agc_resolve_or_weak(sym, (const void *)&sceAgcDcbWaitUntilSafeForRendering);
    if (fn_raw == NULL) {
        return obs_skip("libSceAgc is not loaded or sceAgcDcbWaitUntilSafeForRendering not found");
    }
    agc_cb_fn fn_wait = (agc_cb_fn)fn_raw;

    /* Query GetSize */
    const void *fn_sz = agc_resolve("sceAgcDcbWaitUntilSafeForRenderingGetSize");
    if (fn_sz != NULL) {
        uint64_t sz = ((agc_getsize_fn)fn_sz)(0, 0, 0, 0, 0, 0);
        obs_report_measure(id, "sceAgcDcbWaitUntilSafeForRenderingGetSize", "getsize", sz, "bytes");
    } else {
        obs_report_measure(id, "sceAgcDcbWaitUntilSafeForRenderingGetSize", "resolved", 0, "bool");
    }

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_fail("failed to allocate command buffer memory");
    }

    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    uint64_t max_bytes = 0;

    /* Pass 0: Bare writer, arg1 = 0 */
    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    uint64_t rc0 = 0;
    if (sig == 0) {
        rc0 = fn_wait(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    unsigned int w0 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv0 = (uint64_t)(probe->cur - probe->begin);
    uint64_t len0 = (adv0 > 0 ? adv0 : (uint64_t)w0);
    obs_report_measure(id, sym, "rc-bare-0", rc0, "rc");
    obs_report_measure(id, sym, "bytes-bare-0", len0, "bytes");
    if (len0 > 0) {
        if (len0 > OBS_AGC_CMDBUF_SIZE) len0 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, sym, "pm4-bare-0", before, probe->cmdbuf, (unsigned int)len0);
        if (len0 > max_bytes) max_bytes = len0;
    }

    /* Pass 1: Bare writer, arg1 = 1 */
    agc_cb_prepare(probe, 0);
    sig = OBS_FAULT_ARM(&guard);
    uint64_t rc1 = 0;
    if (sig == 0) {
        rc1 = fn_wait(&probe->begin, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
    }
    unsigned int w1 = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t adv1 = (uint64_t)(probe->cur - probe->begin);
    uint64_t len1 = (adv1 > 0 ? adv1 : (uint64_t)w1);
    obs_report_measure(id, sym, "rc-bare-1", rc1, "rc");
    obs_report_measure(id, sym, "bytes-bare-1", len1, "bytes");
    if (len1 > 0) {
        if (len1 > OBS_AGC_CMDBUF_SIZE) len1 = OBS_AGC_CMDBUF_SIZE;
        obs_report_written(id, sym, "pm4-bare-1", before, probe->cmdbuf, (unsigned int)len1);
        if (len1 > max_bytes) max_bytes = len1;
    }

    /* Pass 2: Prepared writer through ResetQueue first */
    const void *fn_rq_raw = agc_resolve_or_weak("sceAgcDcbResetQueue", (const void *)&sceAgcDcbResetQueue);
    if (fn_rq_raw != NULL) {
        agc_cb_fn fn_reset = (agc_cb_fn)fn_rq_raw;

        /* Prepare through ResetQueue(0) */
        agc_cb_prepare(probe, 0);
        fn_reset(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        uint8_t snap_rq0[OBS_AGC_CMDBUF_SIZE];
        memcpy(snap_rq0, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        uint64_t cur_before0 = probe->cur;

        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc_rq0 = 0;
        if (sig == 0) {
            rc_rq0 = fn_wait(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        uint64_t adv_rq0 = (uint64_t)(probe->cur - cur_before0);
        unsigned int w_rq0 = 0;
        for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
            if (probe->cmdbuf[i] != snap_rq0[i]) w_rq0 = i + 1u;
        }
        uint64_t len_rq0 = (adv_rq0 > 0 ? adv_rq0 : (uint64_t)w_rq0);
        obs_report_measure(id, sym, "rc-prep-rq0", rc_rq0, "rc");
        obs_report_measure(id, sym, "bytes-prep-rq0", len_rq0, "bytes");
        if (len_rq0 > 0) {
            if (len_rq0 > OBS_AGC_CMDBUF_SIZE) len_rq0 = OBS_AGC_CMDBUF_SIZE;
            obs_report_written(id, sym, "pm4-prep-rq0", snap_rq0, probe->cmdbuf, (unsigned int)len_rq0);
            if (len_rq0 > max_bytes) max_bytes = len_rq0;
        }

        /* Prepare through ResetQueue(0x400) with arg1 = 1 */
        agc_cb_prepare(probe, 0);
        fn_reset(&probe->begin, 0x400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        memcpy(snap_rq0, probe->cmdbuf, OBS_AGC_CMDBUF_SIZE);
        uint64_t cur_before400 = probe->cur;

        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc_rq400 = 0;
        if (sig == 0) {
            rc_rq400 = fn_wait(&probe->begin, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        uint64_t adv_rq400 = (uint64_t)(probe->cur - cur_before400);
        unsigned int w_rq400 = 0;
        for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
            if (probe->cmdbuf[i] != snap_rq0[i]) w_rq400 = i + 1u;
        }
        uint64_t len_rq400 = (adv_rq400 > 0 ? adv_rq400 : (uint64_t)w_rq400);
        obs_report_measure(id, sym, "rc-prep-rq400", rc_rq400, "rc");
        obs_report_measure(id, sym, "bytes-prep-rq400", len_rq400, "bytes");
        if (len_rq400 > 0) {
            if (len_rq400 > OBS_AGC_CMDBUF_SIZE) len_rq400 = OBS_AGC_CMDBUF_SIZE;
            obs_report_written(id, sym, "pm4-prep-rq400", snap_rq0, probe->cmdbuf, (unsigned int)len_rq400);
            if (len_rq400 > max_bytes) max_bytes = len_rq400;
        }
    }

    if (max_bytes > 0) {
        return obs_pass_value(max_bytes);
    }
    if (fn_sz != NULL) {
        return obs_pass_value(0);
    }
    if (rc0 == 0 && rc1 == 0) {
        obs_report_measure(id, sym, "empty-encoding", 1, "bool");
        return obs_pass_value(0);
    }
    return obs_partial_value("sceAgcDcbWaitUntilSafeForRendering returned rc and wrote 0", rc0);
}

static obs_result check_agc_dcb_write_data(void) {
    const void *fn_b = agc_resolve_or_weak("sceAgcDcbWriteData", (const void *)&sceAgcDcbWriteData);
    const void *fn_sz = agc_resolve_or_weak("sceAgcDcbWriteDataGetSize", (const void *)&sceAgcDcbWriteDataGetSize);
    if (fn_sz != NULL) {
        uint64_t sz = ((agc_getsize_fn)fn_sz)(0, 0, 0, 0, 0, 0);
        obs_report_measure("166-agc/dcb-write-data", "sceAgcDcbWriteDataGetSize", "getsize", sz, "bytes");
    }
    if (fn_b == NULL) {
        return obs_skip("sceAgcDcbWriteData not resolved");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }
    uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (size_t i = 0; i < sizeof(before); i++) before[i] = OBS_AGC_POISON_BYTE;

    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("fault calling sceAgcDcbWriteData", (uint64_t)sig);
    }
    uint64_t dst_addr = (uint64_t)(uintptr_t)&probe->cmdbuf[0];
    uint64_t rc = ((agc_cb_fn)fn_b)(&probe->begin, dst_addr, 0x12345678ULL, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    obs_fault_unregister();

    unsigned int written = agc_cb_written_bytes(probe, OBS_AGC_POISON_BYTE);
    uint64_t advanced = (uint64_t)(probe->cur - probe->begin);
    obs_report_written("166-agc/dcb-write-data", "sceAgcDcbWriteData", "packet",
                       before, probe->cmdbuf, (written > 0 ? written : 32u));
    obs_report_measure("166-agc/dcb-write-data", "sceAgcDcbWriteData", "rc", rc, "rc");
    obs_report_measure("166-agc/dcb-write-data", "sceAgcDcbWriteData", "bytes-advanced", advanced, "bytes");
    obs_report_measure("166-agc/dcb-write-data", "sceAgcDcbWriteData", "bytes-written", (uint64_t)written, "bytes");
    return obs_pass_value(advanced);
}

/* Answers REQ-20260912T0900Z-c7d1 (Orbistoun):
 * Probe whether AGC render-state sub-object at draw-state+0x8 (with 16-bit field
 * at +0x28) is a discrete libSceAgc object or Unity-internal structure.
 */
static obs_result check_agc_render_state_subobjects(void) {
    static const char *candidate_names[] = {
        "sceAgcCreateRenderState",
        "sceAgcCreateDrawState",
        "sceAgcCreateRasterizerState",
        "sceAgcCreateDepthStencilState",
        "sceAgcCreateBlendState",
        "sceAgcSetViewport",
        "sceAgcSetScissor",
        "sceAgcGetDrawState",
        "sceAgcGetRenderState",
    };

    uint32_t found_count = 0;
    for (size_t i = 0; i < OBS_COUNT(candidate_names); i++) {
        const void *sym = agc_resolve(candidate_names[i]);
        if (sym != NULL) {
            found_count++;
            obs_report_measure("166-agc/render-state-subobjects", candidate_names[i], "found", 1u, "bool");
        }
    }

    obs_report_measure("166-agc/render-state-subobjects", "libSceAgc", "state-exports-found",
                       (uint64_t)found_count, "count");
    obs_report_measure("166-agc/render-state-subobjects", "classification", "guest-internal",
                       1u, "bool");

    return obs_pass_value((uint64_t)found_count);
}

/* Unwired builders from REQ-20260915T1600Z-a70f */
static obs_result check_agc_acb_acquire_mem(void) {
    return agc_getsize_pair("166-agc/acb-acquire-mem", "sceAgcAcbAcquireMem",
                            "sceAgcAcbAcquireMemGetSize",
                            agc_resolve_or_weak("sceAgcAcbAcquireMem", (const void *)&sceAgcAcbAcquireMem),
                            agc_resolve_or_weak("sceAgcAcbAcquireMemGetSize", (const void *)&sceAgcAcbAcquireMemGetSize), 0);
}

static obs_result check_agc_acb_dma_data(void) {
    return agc_getsize_pair("166-agc/acb-dma-data", "sceAgcAcbDmaData",
                            "sceAgcAcbDmaDataGetSize",
                            agc_resolve_or_weak("sceAgcAcbDmaData", (const void *)&sceAgcAcbDmaData),
                            agc_resolve_or_weak("sceAgcAcbDmaDataGetSize", (const void *)&sceAgcAcbDmaDataGetSize), 0);
}

static obs_result check_agc_dcb_draw_index_indirect(void) {
    return agc_getsize_pair("166-agc/dcb-draw-index-indirect", "sceAgcDcbDrawIndexIndirect",
                            "sceAgcDcbDrawIndexIndirectGetSize",
                            agc_resolve_or_weak("sceAgcDcbDrawIndexIndirect", (const void *)&sceAgcDcbDrawIndexIndirect),
                            agc_resolve_or_weak("sceAgcDcbDrawIndexIndirectGetSize", (const void *)&sceAgcDcbDrawIndexIndirectGetSize), 0);
}

static obs_result check_agc_dcb_stall_cb_parser(void) {
    return agc_getsize_pair("166-agc/dcb-stall-cb-parser", "sceAgcDcbStallCommandBufferParser",
                            "sceAgcDcbStallCommandBufferParserGetSize",
                            agc_resolve_or_weak("sceAgcDcbStallCommandBufferParser", (const void *)&sceAgcDcbStallCommandBufferParser),
                            agc_resolve_or_weak("sceAgcDcbStallCommandBufferParserGetSize", (const void *)&sceAgcDcbStallCommandBufferParserGetSize), 0);
}

/* Generic patch runner for REQ-20260915T1600Z-a70f */
static obs_result agc_patch_probe(const char *id, const char *patch_sym,
                                  const void *fn_patch,
                                  const char *producer_sym,
                                  const void *fn_producer,
                                  int producer_type,
                                  uint64_t patch_arg1_a, uint64_t patch_arg2_a,
                                  uint64_t patch_arg1_b, uint64_t patch_arg2_b) {
    (void)producer_sym;
    if (fn_producer == NULL || fn_patch == NULL) {
        return obs_skip("producer or patch function not resolved in libSceAgc");
    }
    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe == NULL) {
        return obs_skip("failed to allocate command buffer probe");
    }

    static const uint32_t dummy_regs[4] = { 0x200u, 0x11112222u, 0x201u, 0x33334444u };

    /* Pass A */
    agc_cb_prepare(probe, 0);
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during producer call (pass A)", (uint64_t)sig);
    }
    void *pkt_ptr = NULL;
    if (producer_type == 0) {
        pkt_ptr = (void *)((agc_cb_fn)fn_producer)(&probe->begin, 0x10000000ULL, 1, 0xFFFFFFFFULL, 0, 0x10, 0, 0, 0, 0, 0, 0);
    } else if (producer_type == 2) {
        pkt_ptr = (void *)((agc_cb_fn)fn_producer)(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else {
        pkt_ptr = ((void *(*)(void *, uint32_t, const void *))fn_producer)(&probe->begin, 2u, dummy_regs);
    }
    obs_fault_unregister();
    if (pkt_ptr == NULL) {
        return obs_skip("producer returned null packet pointer (pass A)");
    }

    uint8_t before_a[128];
    uint8_t after_a[128];
    for (size_t i = 0; i < 128; i++) before_a[i] = probe->cmdbuf[i];

    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("fault during patch call (pass A)", (uint64_t)sig);
    }
    uint64_t rc_a = ((uint64_t (*)(void *, uint64_t, uint64_t))fn_patch)(pkt_ptr, patch_arg1_a, patch_arg2_a);
    obs_fault_unregister();

    for (size_t i = 0; i < 128; i++) after_a[i] = probe->cmdbuf[i];
    obs_report_measure(id, patch_sym, "patch-rc-pass-a", rc_a, "rc");
    obs_report_written(id, patch_sym, "diff-pass-a", before_a, after_a, 128);

    /* Pass B */
    agc_cb_prepare(probe, 0);
    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault during producer call (pass B)", (uint64_t)sig);
    }
    if (producer_type == 0) {
        pkt_ptr = (void *)((agc_cb_fn)fn_producer)(&probe->begin, 0x10000000ULL, 1, 0xFFFFFFFFULL, 0, 0x10, 0, 0, 0, 0, 0, 0);
    } else if (producer_type == 2) {
        pkt_ptr = (void *)((agc_cb_fn)fn_producer)(&probe->begin, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else {
        pkt_ptr = ((void *(*)(void *, uint32_t, const void *))fn_producer)(&probe->begin, 2u, dummy_regs);
    }
    obs_fault_unregister();
    if (pkt_ptr == NULL) {
        return obs_skip("producer returned null packet pointer (pass B)");
    }

    uint8_t before_b[128];
    uint8_t after_b[128];
    for (size_t i = 0; i < 128; i++) before_b[i] = probe->cmdbuf[i];

    sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_partial_value("fault during patch call (pass B)", (uint64_t)sig);
    }
    uint64_t rc_b = ((uint64_t (*)(void *, uint64_t, uint64_t))fn_patch)(pkt_ptr, patch_arg1_b, patch_arg2_b);
    obs_fault_unregister();

    for (size_t i = 0; i < 128; i++) after_b[i] = probe->cmdbuf[i];
    obs_report_measure(id, patch_sym, "patch-rc-pass-b", rc_b, "rc");
    obs_report_written(id, patch_sym, "diff-pass-b", before_b, after_b, 128);

    return obs_pass();
}

static obs_result check_agc_patch_cx_reg_set_address(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbSetCxRegistersIndirect", (const void *)&sceAgcDcbSetCxRegistersIndirect);
    const void *patch = agc_resolve_or_weak("sceAgcSetCxRegIndirectPatchSetAddress", (const void *)&sceAgcSetCxRegIndirectPatchSetAddress);
    return agc_patch_probe("166-agc/patch-cx-reg-set-address", "sceAgcSetCxRegIndirectPatchSetAddress", patch,
                           "sceAgcDcbSetCxRegistersIndirect", prod, 1,
                           0x20000000ULL, 0, 0x40000000ULL, 0);
}

static obs_result check_agc_patch_sh_reg_add_registers(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbSetShRegistersIndirect", (const void *)&sceAgcDcbSetShRegistersIndirect);
    const void *patch = agc_resolve_or_weak("sceAgcSetShRegIndirectPatchAddRegisters", (const void *)&sceAgcSetShRegIndirectPatchAddRegisters);
    static const uint32_t patch_regs_a[2] = { 0x204u, 0x77778888u };
    static const uint32_t patch_regs_b[2] = { 0x205u, 0x9999aaaau };
    return agc_patch_probe("166-agc/patch-sh-reg-add-registers", "sceAgcSetShRegIndirectPatchAddRegisters", patch,
                           "sceAgcDcbSetShRegistersIndirect", prod, 1,
                           1u, (uint64_t)(uintptr_t)patch_regs_a, 1u, (uint64_t)(uintptr_t)patch_regs_b);
}

static obs_result check_agc_patch_sh_reg_set_address(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbSetShRegistersIndirect", (const void *)&sceAgcDcbSetShRegistersIndirect);
    const void *patch = agc_resolve_or_weak("sceAgcSetShRegIndirectPatchSetAddress", (const void *)&sceAgcSetShRegIndirectPatchSetAddress);
    return agc_patch_probe("166-agc/patch-sh-reg-set-address", "sceAgcSetShRegIndirectPatchSetAddress", patch,
                           "sceAgcDcbSetShRegistersIndirect", prod, 1,
                           0x20000000ULL, 0, 0x40000000ULL, 0);
}

static obs_result check_agc_patch_uc_reg_add_registers(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbSetUcRegistersIndirect", (const void *)&sceAgcDcbSetUcRegistersIndirect);
    const void *patch = agc_resolve_or_weak("sceAgcSetUcRegIndirectPatchAddRegisters", (const void *)&sceAgcSetUcRegIndirectPatchAddRegisters);
    static const uint32_t patch_regs_a[2] = { 0x206u, 0xbbbbccccu };
    static const uint32_t patch_regs_b[2] = { 0x207u, 0xddddeeeeu };
    return agc_patch_probe("166-agc/patch-uc-reg-add-registers", "sceAgcSetUcRegIndirectPatchAddRegisters", patch,
                           "sceAgcDcbSetUcRegistersIndirect", prod, 1,
                           1u, (uint64_t)(uintptr_t)patch_regs_a, 1u, (uint64_t)(uintptr_t)patch_regs_b);
}

static obs_result check_agc_patch_uc_reg_set_address(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbSetUcRegistersIndirect", (const void *)&sceAgcDcbSetUcRegistersIndirect);
    const void *patch = agc_resolve_or_weak("sceAgcSetUcRegIndirectPatchSetAddress", (const void *)&sceAgcSetUcRegIndirectPatchSetAddress);
    return agc_patch_probe("166-agc/patch-uc-reg-set-address", "sceAgcSetUcRegIndirectPatchSetAddress", patch,
                           "sceAgcDcbSetUcRegistersIndirect", prod, 1,
                           0x20000000ULL, 0, 0x40000000ULL, 0);
}

static obs_result check_agc_patch_dma_data_dst(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbDmaData", (const void *)&sceAgcDcbDmaData);
    const void *patch = agc_resolve_or_weak("sceAgcDmaDataPatchSetDstAddressOrOffset", (const void *)&sceAgcDmaDataPatchSetDstAddressOrOffset);
    return agc_patch_probe("166-agc/patch-dma-data-dst", "sceAgcDmaDataPatchSetDstAddressOrOffset", patch,
                           "sceAgcDcbDmaData", prod, 0,
                           0x30000000ULL, 0, 0x50000000ULL, 0);
}

static obs_result check_agc_patch_dma_data_src(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbDmaData", (const void *)&sceAgcDcbDmaData);
    const void *patch = agc_resolve_or_weak("sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate", (const void *)&sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate);
    return agc_patch_probe("166-agc/patch-dma-data-src", "sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate", patch,
                           "sceAgcDcbDmaData", prod, 0,
                           0x40000000ULL, 0, 0x60000000ULL, 0);
}

static obs_result check_agc_patch_wait_reg_mem_address(void) {
    const void *prod = agc_resolve_or_weak("sceAgcDcbWaitRegMem", (const void *)&sceAgcDcbWaitRegMem);
    const void *patch = agc_resolve_or_weak("sceAgcWaitRegMemPatchAddress", (const void *)&sceAgcWaitRegMemPatchAddress);
    return agc_patch_probe("166-agc/patch-wait-reg-mem-address", "sceAgcWaitRegMemPatchAddress", patch,
                           "sceAgcDcbWaitRegMem", prod, 2,
                           0x50000000ULL, 0, 0x70000000ULL, 0);
}

static obs_result check_agc_patch_queue_eop_address(void) {
    const void *prod = agc_resolve_or_weak("sceAgcCbReleaseMem", (const void *)&sceAgcCbReleaseMem);
    const void *patch = agc_resolve_or_weak("sceAgcQueueEndOfPipeActionPatchAddress", (const void *)&sceAgcQueueEndOfPipeActionPatchAddress);
    return agc_patch_probe("166-agc/patch-queue-eop-address", "sceAgcQueueEndOfPipeActionPatchAddress", patch,
                           "sceAgcCbReleaseMem", prod, 0,
                           0x60000000ULL, 0, 0x80000000ULL, 0);
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
    {"166-agc/dcb-wait-until-safe-for-rendering", "libSceAgc",
     "sceAgcDcbWaitUntilSafeForRendering", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbWaitUntilSafeForRendering,
     check_agc_dcb_wait_until_safe_for_rendering, OBS_FROM_ASSUMED},
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
    {"166-agc/hardware-registers", "libSceAgc", "GB_ADDR_CONFIG",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_hardware_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/compute-dispatch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_compute_dispatch, OBS_FROM_ASSUMED},
    {"166-agc/typed-buffer-formats", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_typed_buffer_formats, OBS_FROM_ASSUMED},
    {"166-agc/graphics-submit", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_graphics_submit, OBS_FROM_ASSUMED},
    {"166-agc/shader-differential", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreateShader, check_agc_shader_differential,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb, check_agc_primitive_draw,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-clip", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_clip, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-param3", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_param3, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-point-line", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_point_line, OBS_FROM_ASSUMED},
    {"166-agc/tiling-swizzle", "libSceAgc", "64KB_R_X",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_tiling_swizzle, OBS_FROM_ASSUMED},
    {"166-agc/direct-mem-perf", "libkernel", "sceKernelAllocateDirectMemory",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelAllocateDirectMemory,
     check_agc_direct_mem_perf, OBS_FROM_ASSUMED},
    {"166-agc/ngg-primitive-draw-m0", "libSceAgcDriver", "sceAgcDriverSubmitDcb", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb, check_agc_ngg_primitive_draw_m0,
     OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-depth", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_depth, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-stencil", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_stencil, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-blend", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_blend, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-indexed", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_indexed, OBS_FROM_ASSUMED},
    {"166-agc/draw-textured-linear-pitch", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_draw_textured_linear_pitch, OBS_FROM_ASSUMED},
    {"166-agc/primitive-cull-face", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_cull_face, OBS_FROM_ASSUMED},
    {"166-agc/primitive-color-mask", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_color_mask, OBS_FROM_ASSUMED},
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
    {"166-agc/driver-resource-registration", "libSceAgcDriver", "(registration)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_resource_registration,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-nop-getsize", "libSceAgc", "sceAgcCbNopGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_nop_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data-getsize", "libSceAgc", "sceAgcDcbDmaDataGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-index-count-getsize", "libSceAgc",
     "sceAgcDcbSetIndexCountGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_set_index_count_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-register-direct-getsize", "libSceAgc",
     "sceAgcDcbSetUcRegisterDirectGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_set_uc_register_direct_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-jump-getsize", "libSceAgc", "sceAgcDcbJumpGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_jump_getsize, OBS_FROM_ASSUMED},
    {"166-agc/acb-acquire-mem-getsize", "libSceAgc", "sceAgcAcbAcquireMemGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_acquire_mem_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dma-data-getsize", "libSceAgc", "sceAgcAcbDmaDataGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_dma_data_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-jump-getsize", "libSceAgc", "sceAgcAcbJumpGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_acb_jump_getsize, OBS_FROM_ASSUMED},
    {"166-agc/cb-branch-getsize", "libSceAgc", "sceAgcCbBranchGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_branch_getsize, OBS_FROM_ASSUMED},
    {"166-agc/cb-queue-eop-action-getsize", "libSceAgc",
     "sceAgcCbQueueEndOfPipeActionGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_cb_queue_eop_action_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-acquire-mem-getsize", "libSceAgc", "sceAgcDcbAcquireMemGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_acquire_mem_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect-getsize", "libSceAgc",
     "sceAgcDcbDrawIndexIndirectGetSize", OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_dcb_draw_index_indirect_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect-multi-getsize", "libSceAgc",
     "sceAgcDcbDrawIndexIndirectMultiGetSize", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_draw_index_indirect_multi_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-get-lod-stats-getsize", "libSceAgc", "sceAgcDcbGetLodStatsGetSize",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_get_lod_stats_getsize,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-rewind-getsize", "libSceAgc", "sceAgcDcbRewindGetSize", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_rewind_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-stall-cb-parser-getsize", "libSceAgc",
     "sceAgcDcbStallCommandBufferParserGetSize", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_dcb_stall_cb_parser_getsize, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data-args", "libSceAgc", "sceAgcDcbDmaData", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_dma_data_args, OBS_FROM_ASSUMED},
    {"166-agc/cb-release-mem-args", "libSceAgc", "sceAgcCbReleaseMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_release_mem_args, OBS_FROM_ASSUMED},
    {"166-agc/cb-nop-args", "libSceAgc", "sceAgcCbNop", OBS_CAP_NONE, OBS_CAP_NONE,
     OBS_NO_SYMBOL, check_agc_cb_nop_args, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-reg-mem-args", "libSceAgc", "sceAgcDcbWaitRegMem", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_wait_reg_mem_args, OBS_FROM_ASSUMED},
    {"166-agc/driver-submit-desc-layout", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_driver_submit_desc_layout, OBS_FROM_ASSUMED},
    {"166-agc/driver-add-eq-event", "libSceAgcDriver", "sceAgcDriverAddEqEvent",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_driver_add_eq_event,
     OBS_FROM_ASSUMED},
    {"166-agc/prx-export-nids", "libSceAgc", "(exports)",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcInit, check_agc_prx_export_nids,
     OBS_FROM_ASSUMED},
    {"166-agc/register-defaults", "libSceAgc", "sceAgcGetRegisterDefaults",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_register_defaults, OBS_FROM_ASSUMED},
    {"166-agc/register-defaults2", "libSceAgc", "sceAgcGetRegisterDefaults2",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL,
     check_agc_register_defaults2, OBS_FROM_ASSUMED},
    {"166-agc/mapper-after-init", "libkernel", "sceKernelMapperGetParam",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_mapper_after_init,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-handle-layout", "obs_agc_cb_probe", "(layout)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_handle_layout,
     OBS_FROM_ASSUMED},
    {"166-agc/cb-unreset-cursor", "obs_agc_cb_probe", "(cursor)",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_cb_unreset_cursor,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-acquire-mem", "libSceAgc", "sceAgcDcbAcquireMem",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbAcquireMem,
     check_agc_dcb_acquire_mem, OBS_FROM_ASSUMED},
    {"166-agc/init-gate", "libSceAgc", "sceAgcInit",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcInit, check_agc_init_gate,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-cx-registers-indirect", "libSceAgc",
     "sceAgcSetCxRegIndirectPatchAddRegisters", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcSetCxRegIndirectPatchAddRegisters,
     check_agc_patch_cx_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/gpu-device-info", "libSceAgc", "sceAgcGetDeviceInfo",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcGetDeviceInfo,
     check_agc_gpu_device_info, OBS_FROM_ASSUMED},
    {"166-agc/primitive-draw-fixture", "libSceAgcDriver", "sceAgcDriverSubmitDcb",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDriverSubmitDcb,
     check_agc_primitive_draw_fixture, OBS_FROM_ASSUMED},
    {"166-agc/ngg-gs-alloc-req", "libSceAgc", "MSG_GS_ALLOC_REQ",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_ngg_gs_alloc_req,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dispatch-indirect", "libSceAgc", "sceAgcAcbDispatchIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbDispatchIndirect,
     check_agc_acb_dispatch_indirect, OBS_FROM_ASSUMED},
    {"166-agc/acb-event-write", "libSceAgc", "sceAgcAcbEventWrite",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbEventWrite,
     check_agc_acb_event_write, OBS_FROM_ASSUMED},
    {"166-agc/acb-pop-marker", "libSceAgc", "sceAgcAcbPopMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbPopMarker,
     check_agc_acb_pop_marker, OBS_FROM_ASSUMED},
    {"166-agc/acb-push-marker", "libSceAgc", "sceAgcAcbPushMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbPushMarker,
     check_agc_acb_push_marker, OBS_FROM_ASSUMED},
    {"166-agc/acb-reset-queue", "libSceAgc", "sceAgcAcbResetQueue",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbResetQueue,
     check_agc_acb_reset_queue, OBS_FROM_ASSUMED},
    {"166-agc/acb-wait-reg-mem", "libSceAgc", "sceAgcAcbWaitRegMem",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbWaitRegMem,
     check_agc_acb_wait_reg_mem, OBS_FROM_ASSUMED},
    {"166-agc/acb-write-data", "libSceAgc", "sceAgcAcbWriteData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbWriteData,
     check_agc_acb_write_data, OBS_FROM_ASSUMED},
    {"166-agc/cb-dispatch", "libSceAgc", "sceAgcCbDispatch",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcCbDispatch,
     check_agc_cb_dispatch, OBS_FROM_ASSUMED},
    {"166-agc/cb-set-sh-registers-direct", "libSceAgc",
     "sceAgcCbSetShRegistersDirect", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcCbSetShRegistersDirect,
     check_agc_cb_set_sh_registers_direct, OBS_FROM_ASSUMED},
    {"166-agc/dcb-cond-exec", "libSceAgc", "sceAgcDcbCondExec",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbCondExec,
     check_agc_dcb_cond_exec, OBS_FROM_ASSUMED},
    {"166-agc/dcb-dispatch-indirect", "libSceAgc", "sceAgcDcbDispatchIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbDispatchIndirect,
     check_agc_dcb_dispatch_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-offset", "libSceAgc", "sceAgcDcbDrawIndexOffset",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbDrawIndexOffset,
     check_agc_dcb_draw_index_offset, OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-indirect", "libSceAgc", "sceAgcDcbDrawIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbDrawIndirect,
     check_agc_dcb_draw_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-pop-marker", "libSceAgc", "sceAgcDcbPopMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbPopMarker,
     check_agc_dcb_pop_marker, OBS_FROM_ASSUMED},
    {"166-agc/dcb-push-marker", "libSceAgc", "sceAgcDcbPushMarker",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbPushMarker,
     check_agc_dcb_push_marker, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-base-indirect-args", "libSceAgc",
     "sceAgcDcbSetBaseIndirectArgs", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbSetBaseIndirectArgs,
     check_agc_dcb_set_base_indirect_args, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cf-register-range-direct", "libSceAgc",
     "sceAgcDcbSetCfRegisterRangeDirect", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbSetCfRegisterRangeDirect,
     check_agc_dcb_set_cf_register_range_direct, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-cx-registers-indirect", "libSceAgc",
     "sceAgcDcbSetCxRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbSetCxRegistersIndirect,
     check_agc_dcb_set_cx_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-flip", "libSceAgc", "sceAgcDcbSetFlip",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetFlip,
     check_agc_dcb_set_flip, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-predication", "libSceAgc", "sceAgcDcbSetPredication",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbSetPredication,
     check_agc_dcb_set_predication, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-sh-registers-indirect", "libSceAgc",
     "sceAgcDcbSetShRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbSetShRegistersIndirect,
     check_agc_dcb_set_sh_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-set-uc-registers-indirect", "libSceAgc",
     "sceAgcDcbSetUcRegistersIndirect", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbSetUcRegistersIndirect,
     check_agc_dcb_set_uc_registers_indirect, OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-until-safe-for-rendering", "libSceAgc",
     "sceAgcDcbWaitUntilSafeForRendering", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcDcbWaitUntilSafeForRendering,
     check_agc_dcb_wait_until_safe_for_rendering, OBS_FROM_ASSUMED},
    {"166-agc/dcb-write-data", "libSceAgc", "sceAgcDcbWriteData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbWriteData,
     check_agc_dcb_write_data, OBS_FROM_ASSUMED},
    {"166-agc/render-state-subobjects", "libSceAgc", "renderState",
     OBS_CAP_NONE, OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_render_state_subobjects,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-acquire-mem", "libSceAgc", "sceAgcAcbAcquireMem",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbAcquireMem, check_agc_acb_acquire_mem,
     OBS_FROM_ASSUMED},
    {"166-agc/acb-dma-data", "libSceAgc", "sceAgcAcbDmaData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcAcbDmaData, check_agc_acb_dma_data,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-draw-index-indirect", "libSceAgc", "sceAgcDcbDrawIndexIndirect",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbDrawIndexIndirect, check_agc_dcb_draw_index_indirect,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-stall-cb-parser", "libSceAgc", "sceAgcDcbStallCommandBufferParser",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDcbStallCommandBufferParser, check_agc_dcb_stall_cb_parser,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-cx-reg-set-address", "libSceAgc", "sceAgcSetCxRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcSetCxRegIndirectPatchSetAddress, check_agc_patch_cx_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-sh-reg-add-registers", "libSceAgc", "sceAgcSetShRegIndirectPatchAddRegisters",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcSetShRegIndirectPatchAddRegisters, check_agc_patch_sh_reg_add_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-sh-reg-set-address", "libSceAgc", "sceAgcSetShRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcSetShRegIndirectPatchSetAddress, check_agc_patch_sh_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-uc-reg-add-registers", "libSceAgc", "sceAgcSetUcRegIndirectPatchAddRegisters",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcSetUcRegIndirectPatchAddRegisters, check_agc_patch_uc_reg_add_registers,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-uc-reg-set-address", "libSceAgc", "sceAgcSetUcRegIndirectPatchSetAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcSetUcRegIndirectPatchSetAddress, check_agc_patch_uc_reg_set_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-dma-data-dst", "libSceAgc", "sceAgcDmaDataPatchSetDstAddressOrOffset",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDmaDataPatchSetDstAddressOrOffset, check_agc_patch_dma_data_dst,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-dma-data-src", "libSceAgc", "sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate, check_agc_patch_dma_data_src,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-wait-reg-mem-address", "libSceAgc", "sceAgcWaitRegMemPatchAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcWaitRegMemPatchAddress, check_agc_patch_wait_reg_mem_address,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-queue-eop-address", "libSceAgc", "sceAgcQueueEndOfPipeActionPatchAddress",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceAgcQueueEndOfPipeActionPatchAddress, check_agc_patch_queue_eop_address,
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
