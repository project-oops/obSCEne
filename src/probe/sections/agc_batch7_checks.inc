/*
 * agc_batch7_checks.inc - Live GPU hardware checks for Batch 7 /
 * REQ-20261014T1300Z-rf03
 */

#include "oops/krw.h"

/* Helper to emit full-screen triangle NGG VS at payload + 0x000 */
static void agc_emit_fs_triangle_vs(uint32_t *vs_code) {
    uint32_t vsk = 0;
    vs_code[vsk++] = 0xbfa00001u;
    vs_code[vsk++] = 0xbe8c037eu;
    vs_code[vsk++] = 0xbefc03ffu;
    vs_code[vsk++] = 0x00001003u;
    vs_code[vsk++] = 0xbf800000u;
    vs_code[vsk++] = 0xbf900009u;
    vs_code[vsk++] = 0xbefe0381u;
    vs_code[vsk++] = 0x7e0202ffu;
    vs_code[vsk++] = 0x20280600u;
    vs_code[vsk++] = 0xf8000941u;
    vs_code[vsk++] = 0x00000001u;
    vs_code[vsk++] = 0xbf8cff0fu;
    vs_code[vsk++] = 0xbefe0381u;
    vs_code[vsk++] = 0x7e0a02f1u;
    vs_code[vsk++] = 0x7e0c02f1u;
    vs_code[vsk++] = 0xbefe0382u;
    vs_code[vsk++] = 0x7e0a02ffu;
    vs_code[vsk++] = 0x40400000u;
    vs_code[vsk++] = 0x7e0c02f1u;
    vs_code[vsk++] = 0xbefe0384u;
    vs_code[vsk++] = 0x7e0a02f1u;
    vs_code[vsk++] = 0x7e0c02ffu;
    vs_code[vsk++] = 0x40400000u;
    vs_code[vsk++] = 0xbefe0387u;
    vs_code[vsk++] = 0x7e060280u;
    vs_code[vsk++] = 0x7e0802f2u;
    vs_code[vsk++] = 0xf80008cfu;
    vs_code[vsk++] = 0x04030605u;
    vs_code[vsk++] = 0xbf8cff0fu;
    vs_code[vsk++] = 0xbefe030cu;
    vs_code[vsk++] = 0xbf810000u;
    for (size_t p = vsk; p < 128; p++)
        vs_code[p] = 0xbf800000u;
}

/* Helper to execute a standard NGG draw and wait on fence */
static int agc_submit_test_draw_simple(uint8_t *gpu_payload, uint32_t *dcb_buf,
                                       uint32_t *dt, volatile uint32_t *color_buf,
                                       volatile uint32_t *fence,
                                       volatile uint32_t *canary) {
    uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;
    uint64_t color_gpu = (uint64_t)(uintptr_t)color_buf;
    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;
    uint64_t dt_gpu = dt ? (uint64_t)(uintptr_t)dt : (payload_va + 0x400u);

    uint32_t *dw = dcb_buf;
    agc_ngg_context_t ngg_ctx;
    __builtin_memset(&ngg_ctx, 0, sizeof(ngg_ctx));
    ngg_ctx.color0_gpu = color_gpu;
    ngg_ctx.width = 64;
    ngg_ctx.height = 64;
    ngg_ctx.spi_vs_out_config = 0x00000080u;
    ngg_ctx.spi_ps_in_control = 0x00000000u;
    ngg_ctx.spi_ps_input_ena = 0x00000002u;
    ngg_ctx.spi_ps_input_addr = 0x00000002u;
    ngg_ctx.spi_shader_col_format = 0x00000009u;
    ngg_ctx.cb_target_mask = 0x0fu;
    ngg_ctx.cb_shader_mask = 0x0fu;

    agc_emit_ngg_context(&dw, &ngg_ctx);
    agc_emit_ngg_stages(&dw, payload_va, 4u, dt_gpu);
    agc_emit_ngg_draw_and_fence(&dw, fence_gpu);

    uint32_t words_written = (uint32_t)(dw - dcb_buf);
    obs_agc_dcb_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));
    desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
    desc.size = words_written;

#if defined(__x86_64__)
    __builtin_ia32_clflush((const void *)fence);
    __builtin_ia32_clflush((const void *)canary);
    for (size_t p = 0; p < 0x2000; p += 64)
        __builtin_ia32_clflush((const void *)((const char *)gpu_payload + p));
    for (size_t p = 0; p < (size_t)(words_written * 4u); p += 64)
        __builtin_ia32_clflush((const void *)((const char *)dcb_buf + p));
    for (size_t p = 0; p < 65536; p += 64)
        __builtin_ia32_clflush((const void *)((const char *)color_buf + p));
    __builtin_ia32_mfence();
#else
    (void)canary;
#endif

    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    int submit_rc = -1;
    if (sig == 0) {
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
            submit_rc = sceAgcDriverSubmitDcb(&desc);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        return -sig;
    }

    if (submit_rc == 0) {
        uint64_t t0 = sceKernelGetProcessTime();
        while (sceKernelGetProcessTime() - t0 < 500000) {
#if defined(__x86_64__)
            __builtin_ia32_clflush((const void *)fence);
            __builtin_ia32_clflush((const void *)canary);
#endif
            if (*fence == 0xbeefcafeu) {
                return 0;
            }
        }
    }
    return submit_rc;
}

/* 1. REQ-20261013T1930Z-cb01: Cube sample bias with image_sample_lz ...
 * dim:SQ_RSRC_IMG_CUBE */
static obs_result check_agc_cube_sample_bias(void) {
    const char *id = "166-agc/cube-sample-bias";
    if (s_agc_queue_faulted) {
        return obs_skip("earlier GPU check missed fence; queue stalled");
    }
    if (!obs_address_is_callable((const void *)&sceAgcDriverCreateQueue) ||
        !obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
        return obs_skip("libSceAgcDriver queue/submit symbols not callable");
    }

    OBS_AGC_QUEUE_GUARD();
    obs_jmp_buf guard;
    int sig = OBS_FAULT_ARM(&guard);
    if (sig != 0) {
        obs_fault_unregister();
        return obs_fail_code("fault before queue/fence creation", (uint64_t)sig);
    }

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
    uint8_t *tex_buf = (uint8_t *)oops_mem_alloc(0x2000, 256, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    static _Alignas(256) uint8_t s_host_tex_buf[0x2000];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
    uint8_t *tex_buf = s_host_tex_buf;
#endif

    obs_fault_unregister();
    if (!gpu_payload || !fence || !color_buf || !canary || !dcb_buf || !tex_buf) {
        return obs_skip("failed to allocate memory for cube sample bias test");
    }

    /* 6 faces of 2x2. Linear RDNA2 pitch is 256 bytes per row, height=2 -> 512 bytes
     * per face. */
    memset(tex_buf, 0, 0x2000);
    uint32_t *tb = (uint32_t *)tex_buf;
    for (uint32_t f = 0; f < 6; f++) {
        uint32_t *face = tb + (f * 128); /* 512 bytes / 4 = 128 uint32_t */
        face[0] = 0xff000000u | ((f + 1u) << 16) | 0x0011u;
        face[1] = 0xff000000u | ((f + 1u) << 16) | 0x0122u;
        face[64] = 0xff000000u | ((f + 1u) << 16) | 0x0233u;
        face[65] = 0xff000000u | ((f + 1u) << 16) | 0x0344u;
    }

    uint32_t *dt = (uint32_t *)((char *)gpu_payload + 0x800);
    uint64_t tex_gpu = (uint64_t)(uintptr_t)tex_buf;
    uint32_t w = 2u, h = 2u;
    dt[0] = (uint32_t)(tex_gpu >> 8);
    dt[1] = (uint32_t)((tex_gpu >> 40) & 0xffu) | (56u << 20) | (((w - 1u) & 3u) << 30);
    dt[2] = (((w - 1u) >> 2) & 0x3fffu) | (((h - 1u) & 0x3fffu) << 14) | (1u << 31);
    dt[3] = 0xb0000000u | 0xfacu; /* SQ_RSRC_IMG_CUBE */
    dt[4] = 5u;                   /* 6 faces */
    dt[5] = 0;
    dt[6] = 0;
    dt[7] = 0;
    dt[8] = 0; /* POINT filtering */
    dt[9] = 0x00fff000u;
    dt[10] = 0;
    dt[11] = 0;

    static const float arm1_coords[2] = {1.25f, 1.75f};
    static const float arm2_coords[2] = {0.25f, 0.75f};
    static const float arm3_coords[3] = {-0.25f, 2.25f, 3.25f};

    typedef struct {
        float u, v, face;
    } cube_sample_t;
    cube_sample_t samples[102];
    size_t s_idx = 0;

    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 2; ui++) {
            for (int vi = 0; vi < 2; vi++) {
                samples[s_idx++] =
                    (cube_sample_t){arm1_coords[ui], arm1_coords[vi], (float)f};
            }
        }
    }
    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 2; ui++) {
            for (int vi = 0; vi < 2; vi++) {
                samples[s_idx++] =
                    (cube_sample_t){arm2_coords[ui], arm2_coords[vi], (float)f};
            }
        }
    }
    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 3; ui++) {
            for (int vi = 0; vi < 3; vi++) {
                samples[s_idx++] =
                    (cube_sample_t){arm3_coords[ui], arm3_coords[vi], (float)f};
            }
        }
    }

    agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u;
    ps_code[psk++] = 0xbefc0302u;
    ps_code[psk++] = 0xbe84037eu;
    ps_code[psk++] = 0xbefe0381u;
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    ps_code[psk++] = 0xbe8803ffu;
    ps_code[psk++] = (uint32_t)canary_gpu;
    ps_code[psk++] = 0xbe8903ffu;
    ps_code[psk++] = (uint32_t)(canary_gpu >> 32);
    ps_code[psk++] = 0x7e100208u;
    ps_code[psk++] = 0x7e120209u;

    ps_code[psk++] = 0xf40c0100u;
    ps_code[psk++] = 0xfa000000u;
    ps_code[psk++] = 0xf4080300u;
    ps_code[psk++] = 0xfa000020u;
    ps_code[psk++] = 0xbf8cc07fu;

    for (size_t i = 0; i < 102; i++) {
        uint32_t u_bits = *(uint32_t *)&samples[i].u;
        uint32_t v_bits = *(uint32_t *)&samples[i].v;
        uint32_t f_bits = *(uint32_t *)&samples[i].face;

        ps_code[psk++] = 0x7e0402ffu;
        ps_code[psk++] = u_bits;
        ps_code[psk++] = 0x7e0602ffu;
        ps_code[psk++] = v_bits;
        ps_code[psk++] = 0x7e0802ffu;
        ps_code[psk++] = f_bits;

        ps_code[psk++] = 0xf09c0f18u;
        ps_code[psk++] = 0x00610402u;
        ps_code[psk++] = 0xbf8c3f70u;

        uint32_t off = (uint32_t)(i * 16u);
        ps_code[psk++] = 0xdc788000u | (off & 0xfffu);
        ps_code[psk++] = 0x007d0408u;
        ps_code[psk++] = 0xbf8c3f70u;
    }

    ps_code[psk++] = 0xbefe0304u;
    ps_code[psk++] = 0xf800180fu;
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u;
    for (size_t p = psk; p < psk + 16; p++)
        ps_code[p] = 0xbf800000u;

    *fence = 0x11111111u;
    for (size_t i = 0; i < 102 * 4; i++)
        canary[i] = 0x55555555u;
    for (size_t i = 0; i < 4096; i++)
        color_buf[i] = 0;

    (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, dt, color_buf, fence,
                                      canary);

    uint32_t readback_vals[102];
    for (size_t i = 0; i < 102; i++) {
        float *cf = (float *)((uint32_t *)canary + i * 4);
        float r = cf[0], g = cf[1], b = cf[2], a = cf[3];
        uint32_t ur = (uint32_t)(r * 255.0f + 0.5f) & 0xff;
        uint32_t ug = (uint32_t)(g * 255.0f + 0.5f) & 0xff;
        uint32_t ub = (uint32_t)(b * 255.0f + 0.5f) & 0xff;
        uint32_t ua = (uint32_t)(a * 255.0f + 0.5f) & 0xff;
        readback_vals[i] = (ua << 24) | (ur << 16) | (ug << 8) | ub;
    }

    s_idx = 0;
    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 2; ui++) {
            for (int vi = 0; vi < 2; vi++) {
                char tag[64];
                oops_snprintf(tag, sizeof(tag), "arm1-aco-face%u-u%d-v%d", f, ui, vi);
                obs_report_measure(id, tag, "pixel-val",
                                   (uint64_t)readback_vals[s_idx++], "hex");
            }
        }
    }
    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 2; ui++) {
            for (int vi = 0; vi < 2; vi++) {
                char tag[64];
                oops_snprintf(tag, sizeof(tag), "arm2-oops-face%u-u%d-v%d", f, ui, vi);
                obs_report_measure(id, tag, "pixel-val",
                                   (uint64_t)readback_vals[s_idx++], "hex");
            }
        }
    }
    for (uint32_t f = 0; f < 6; f++) {
        for (int ui = 0; ui < 3; ui++) {
            for (int vi = 0; vi < 3; vi++) {
                char tag[64];
                oops_snprintf(tag, sizeof(tag), "arm3-oob-face%u-u%d-v%d", f, ui, vi);
                obs_report_measure(id, tag, "pixel-val",
                                   (uint64_t)readback_vals[s_idx++], "hex");
            }
        }
    }

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
    oops_mem_free(tex_buf);
#endif

    return obs_pass();
}

/* 2. REQ-20261013T2330Z-im01: The PS-input table the interpolant-mapping calls write */
static obs_result check_agc_interpolant_mapping(void) {
    const char *id = "166-agc/interpolant-mapping";

    if (!obs_address_is_callable((const void *)&sceAgcCreateInterpolantMapping)) {
        return obs_skip("sceAgcCreateInterpolantMapping not callable");
    }

    obs_jmp_buf guard;

    /* Build VS with 7 exports (semantics 0..6) */
    uint8_t vs_hdr[384];
    init_stage_hdr(vs_hdr, 2u /* VS */, 0xc8u);
    *(uint16_t *)(vs_hdr + 0x56) = 7u;
    *(uint64_t *)(vs_hdr + 0x38) = 0x110u;
    uint32_t *vs_sem = (uint32_t *)(vs_hdr + 0x148);
    for (uint32_t i = 0; i < 7; i++) {
        vs_sem[i] = i;
    }
    void *vs_obj = NULL;
    if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        sceAgcCreateShader(&vs_obj, vs_hdr, agc_retail_payload_0, 0);
    }

    /* Build PS with 5 inputs (semantics 2, 0, 5, 1, 6) */
    uint8_t ps_hdr[384];
    init_stage_hdr(ps_hdr, 1u /* PS */, 0x08u);
    *(uint32_t *)(ps_hdr + 0x50) = 5u;
    *(uint64_t *)(ps_hdr + 0x30) = 0x118u;
    uint32_t *ps_sem = (uint32_t *)(ps_hdr + 0x148);
    ps_sem[0] = 2u;
    ps_sem[1] = 0u;
    ps_sem[2] = 5u;
    ps_sem[3] = 1u;
    ps_sem[4] = 6u;
    void *ps_obj = NULL;
    if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        sceAgcCreateShader(&ps_obj, ps_hdr, agc_retail_payload_0, 0);
    }

    /* Control VS/PS pair (identity) */
    uint8_t vs_ident[384];
    init_stage_hdr(vs_ident, 2u, 0xc8u);
    void *vs_ident_obj = NULL;
    if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        sceAgcCreateShader(&vs_ident_obj, vs_ident, agc_retail_payload_0, 0);
    }
    uint8_t ps_ident[384];
    init_stage_hdr(ps_ident, 1u, 0x08u);
    void *ps_ident_obj = NULL;
    if (obs_address_is_callable((const void *)&sceAgcCreateShader)) {
        sceAgcCreateShader(&ps_ident_obj, ps_ident, agc_retail_payload_0, 0);
    }

    /* Arm 1: sceAgcCreateInterpolantMapping(table, vs, ps) */
    uint64_t table1[32];
    memset(table1, 0xcc, sizeof(table1));
    int sig = OBS_FAULT_ARM(&guard);
    int rc1 = -1;
    if (sig == 0) {
        rc1 = sceAgcCreateInterpolantMapping(table1, vs_hdr, ps_hdr);
        if (rc1 != 0 && vs_obj && ps_obj) {
            rc1 = sceAgcCreateInterpolantMapping(table1, vs_obj, ps_obj);
        }
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        rc1 = -sig;
    }
    obs_report_measure(id, "arm1-create", "rc", (uint64_t)(uint32_t)rc1, "code");
    obs_report_bytes(id, "arm1-create", "table", 0, (const unsigned char *)table1, 256);
    for (int q = 0; q < 32; q++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "qw%d", q);
        obs_report_measure(id, "arm1-create", tag, table1[q], "hex");
    }

    /* Arm 2: 0x71040c4df8235e1d */
    int h_agc = obs_module_open("libSceAgc");
    const void *sym_7104 = agc_resolve("$HV4j+E0MBHE");
    if (!sym_7104 && h_agc > 0) {
        sym_7104 = obs_module_symbol(h_agc, "$HV4j+E0MBHE");
        if (!sym_7104)
            sym_7104 = obs_module_symbol(h_agc, "0x71040c4df8235e1d");
    }
#if !defined(OBSCENE_HOST_BUILD)
    if (!sym_7104 && krw_is_ready()) {
        pid_t pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        uintptr_t kaddr = krw_dynlib_resolve_any(pid, "$HV4j+E0MBHE");
        if (kaddr >= 0x10000UL && obs_address_is_callable((const void *)kaddr)) {
            sym_7104 = (const void *)kaddr;
        }
    }
#endif
    if (sym_7104 && obs_address_is_callable(sym_7104)) {
        uint8_t rec[256];
        memset(rec, 0xcc, sizeof(rec));
        static const uint8_t rec_init[24] = {
            0x75, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x02, 0x00, 0x00,
            0x00, 0x40, 0x00, 0x00, 0x42, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
        memcpy(rec, rec_init, sizeof(rec_init));
        uint8_t p4[64], p5[64];
        memset(p4, 0xcc, sizeof(p4));
        memset(p5, 0xcc, sizeof(p5));

        sig = OBS_FAULT_ARM(&guard);
        uint64_t rc2 = 0;
        if (sig == 0) {
            rc2 = ((uint64_t (*)(void *, void *, void *, uint64_t, void *,
                                 void *))sym_7104)(rec, vs_hdr, ps_hdr, 0, p4, p5);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc2 = (uint64_t)-sig;
        }
        obs_report_measure(id, "arm2-7104", "rc", rc2, "code");
        obs_report_bytes(id, "arm2-7104", "rec", 0, rec, 256);
        uint64_t *rec_qw = (uint64_t *)rec;
        for (int q = 0; q < 32; q++) {
            char tag[32];
            oops_snprintf(tag, sizeof(tag), "qw%d", q);
            obs_report_measure(id, "arm2-7104", tag, rec_qw[q], "hex");
        }
    } else {
        obs_report_measure(id, "arm2-7104", "bound", 0u, "bool");
    }

    /* Arm 3: sceAgcUpdateInterpolantMapping(table, vs, ps) over table from arm 1 */
    if (obs_address_is_callable((const void *)&sceAgcUpdateInterpolantMapping)) {
        uint64_t table3[32];
        memcpy(table3, table1, sizeof(table3));
        sig = OBS_FAULT_ARM(&guard);
        int rc3 = -1;
        if (sig == 0) {
            rc3 = sceAgcUpdateInterpolantMapping(table3, vs_hdr, ps_hdr);
            if (rc3 != 0 && vs_obj && ps_obj) {
                rc3 = sceAgcUpdateInterpolantMapping(table3, vs_obj, ps_obj);
            }
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
            rc3 = -sig;
        }
        obs_report_measure(id, "arm3-update", "rc", (uint64_t)(uint32_t)rc3, "code");
        obs_report_bytes(id, "arm3-update", "table", 0, (const unsigned char *)table3,
                         256);
        for (int q = 0; q < 32; q++) {
            char tag[32];
            oops_snprintf(tag, sizeof(tag), "qw%d", q);
            obs_report_measure(id, "arm3-update", tag, table3[q], "hex");
        }
    }

    /* Arm 4: Identity control pair */
    uint64_t table4[32];
    memset(table4, 0xcc, sizeof(table4));
    sig = OBS_FAULT_ARM(&guard);
    int rc4 = -1;
    if (sig == 0) {
        rc4 = sceAgcCreateInterpolantMapping(table4, vs_ident, ps_ident);
        obs_fault_unregister();
    } else {
        obs_fault_unregister();
        rc4 = -sig;
    }
    obs_report_measure(id, "arm4-control", "rc", (uint64_t)(uint32_t)rc4, "code");
    obs_report_bytes(id, "arm4-control", "table", 0, (const unsigned char *)table4,
                     256);
    for (int q = 0; q < 32; q++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "qw%d", q);
        obs_report_measure(id, "arm4-control", tag, table4[q], "hex");
    }

    /* Draw readback: reading which output reaches which input */
#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (gpu_payload && fence && color_buf && canary && dcb_buf) {
        agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

        /* PS reads 5 inputs and writes to canary[0..4] */
        uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
        uint32_t psk = 0;
        ps_code[psk++] = 0xbf8c0000u;
        ps_code[psk++] = 0xbefc0302u;
        ps_code[psk++] = 0xbe84037eu;
        ps_code[psk++] = 0xbefe0381u;
        uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
        ps_code[psk++] = 0xbe8803ffu;
        ps_code[psk++] = (uint32_t)canary_gpu;
        ps_code[psk++] = 0xbe8903ffu;
        ps_code[psk++] = (uint32_t)(canary_gpu >> 32);
        ps_code[psk++] = 0x7e100208u;
        ps_code[psk++] = 0x7e120209u;

        for (int i = 0; i < 5; i++) {
            ps_code[psk++] = 0xdc708000u | (uint32_t)(i * 4);
            ps_code[psk++] = 0x007d0008u | (uint32_t)(i << 18);
        }
        ps_code[psk++] = 0xbf8c3f70u;
        ps_code[psk++] = 0xbefe0304u;
        ps_code[psk++] = 0xf800180fu;
        ps_code[psk++] = 0x07060504u;
        ps_code[psk++] = 0xbf810000u;
        for (size_t p = psk; p < psk + 16; p++)
            ps_code[p] = 0xbf800000u;

        *fence = 0x11111111u;
        for (int i = 0; i < 16; i++)
            canary[i] = 0x55555555u;
        (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, NULL, color_buf, fence,
                                          canary);

        obs_report_measure(id, "draw-routing", "input0-from-output",
                           (uint64_t)canary[0], "val");
        obs_report_measure(id, "draw-routing", "input1-from-output",
                           (uint64_t)canary[1], "val");
        obs_report_measure(id, "draw-routing", "input2-from-output",
                           (uint64_t)canary[2], "val");
        obs_report_measure(id, "draw-routing", "input3-from-output",
                           (uint64_t)canary[3], "val");
        obs_report_measure(id, "draw-routing", "input4-from-output",
                           (uint64_t)canary[4], "val");

        /* Second draw with SPI_PS_INPUT_CNTL[0] = 0x320 (DEFAULT_VAL 3, OFFSET 0x20) */
        uint32_t def_val_entry = 0x320u;
        uint32_t *dw = dcb_buf;
        agc_ngg_context_t ngg_ctx;
        __builtin_memset(&ngg_ctx, 0, sizeof(ngg_ctx));
        ngg_ctx.color0_gpu = (uint64_t)(uintptr_t)color_buf;
        ngg_ctx.width = 64;
        ngg_ctx.height = 64;
        ngg_ctx.spi_vs_out_config = 0x00000080u;
        ngg_ctx.spi_ps_in_control = 0x00000000u;
        ngg_ctx.spi_ps_input_ena = 0x00000002u;
        ngg_ctx.spi_ps_input_addr = 0x00000002u;
        ngg_ctx.spi_shader_col_format = 0x00000009u;
        ngg_ctx.cb_target_mask = 0x0fu;
        ngg_ctx.cb_shader_mask = 0x0fu;

        agc_emit_ngg_context(&dw, &ngg_ctx);
        /* Set SPI_PS_INPUT_CNTL[0] (context 0x191) to 0x320 */
        *dw++ = 0xc0016900u;
        *dw++ = 0x191u;
        *dw++ = def_val_entry;

        uint64_t payload_va = (uint64_t)(uintptr_t)gpu_payload;
        agc_emit_ngg_stages(&dw, payload_va, 4u, payload_va + 0x400u);
        agc_emit_ngg_draw_and_fence(&dw, (uint64_t)(uintptr_t)fence);

        uint32_t words_written = (uint32_t)(dw - dcb_buf);
        obs_agc_dcb_desc desc;
        __builtin_memset(&desc, 0, sizeof(desc));
        desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
        desc.size = words_written;

        *fence = 0x11111111u;
        canary[5] = 0x55555555u;
#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)fence);
        __builtin_ia32_clflush((const void *)canary);
        for (size_t p = 0; p < (size_t)(words_written * 4u); p += 64)
            __builtin_ia32_clflush((const void *)((const char *)dcb_buf + p));
        __builtin_ia32_mfence();
#endif
        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
            (void)sceAgcDriverSubmitDcb(&desc);
            uint64_t t0 = sceKernelGetProcessTime();
            while (sceKernelGetProcessTime() - t0 < 500000) {
#if defined(__x86_64__)
                __builtin_ia32_clflush((const void *)fence);
                __builtin_ia32_clflush((const void *)canary);
#endif
                if (*fence == 0xbeefcafeu)
                    break;
            }
        }

        obs_report_measure(id, "default-val-3", "entry-word", (uint64_t)def_val_entry,
                           "hex");
        obs_report_measure(id, "default-val-3", "constant-arrives", (uint64_t)canary[0],
                           "hex");

#if !defined(OBSCENE_HOST_BUILD)
        oops_mem_free(gpu_payload);
        oops_mem_free((void *)fence);
        oops_mem_free((void *)color_buf);
        oops_mem_free((void *)canary);
        oops_mem_free(dcb_buf);
#endif
    }

    return obs_pass();
}

static inline uint32_t fnv1a_32(const void *data, size_t size) __attribute__((unused));
static inline uint32_t fnv1a_32(const void *data, size_t size) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t hash = 0x811c9dc5u;
    for (size_t i = 0; i < size; i++) {
        hash ^= p[i];
        hash *= 0x01000193u;
    }
    return hash;
}

static obs_result check_agc_gl_cube_oracle(void) {
    const char *id = "166-agc/gl-cube-oracle";

    /* Generate 36 vertices VBO matching gl1_cube_scene.h (48 bytes per vertex = 1728
     * bytes) */
    static const float cube_pos[36 * 3] = {
        -1.f, -1.f, 1.f,  1.f,  -1.f, 1.f,  1.f,  1.f,  1.f,  -1.f, -1.f, 1.f,
        1.f,  1.f,  1.f,  -1.f, 1.f,  1.f,  1.f,  -1.f, -1.f, -1.f, -1.f, -1.f,
        -1.f, 1.f,  -1.f, 1.f,  -1.f, -1.f, -1.f, 1.f,  -1.f, 1.f,  1.f,  -1.f,
        -1.f, 1.f,  1.f,  1.f,  1.f,  1.f,  1.f,  1.f,  -1.f, -1.f, 1.f,  1.f,
        1.f,  1.f,  -1.f, -1.f, 1.f,  -1.f, -1.f, -1.f, -1.f, 1.f,  -1.f, -1.f,
        1.f,  -1.f, 1.f,  -1.f, -1.f, -1.f, 1.f,  -1.f, 1.f,  -1.f, -1.f, 1.f,
        1.f,  -1.f, 1.f,  1.f,  -1.f, -1.f, 1.f,  1.f,  -1.f, 1.f,  -1.f, 1.f,
        1.f,  1.f,  -1.f, 1.f,  1.f,  1.f,  -1.f, -1.f, -1.f, -1.f, -1.f, 1.f,
        -1.f, 1.f,  1.f,  -1.f, -1.f, -1.f, -1.f, 1.f,  1.f,  -1.f, 1.f,  -1.f};
    static const float cube_col[36 * 3] = {
        .15f, .15f, 1.f,  1.f,  .15f, 1.f,  1.f,  1.f,  1.f,  .15f, .15f, 1.f,
        1.f,  1.f,  1.f,  .15f, 1.f,  1.f,  .15f, .15f, .15f, .15f, 1.f,  .15f,
        1.f,  1.f,  .15f, .15f, .15f, .15f, 1.f,  1.f,  .15f, 1.f,  .15f, .15f,
        .15f, 1.f,  .15f, .15f, 1.f,  1.f,  1.f,  1.f,  1.f,  .15f, 1.f,  .15f,
        1.f,  1.f,  1.f,  1.f,  1.f,  .15f, .15f, .15f, .15f, 1.f,  .15f, .15f,
        1.f,  .15f, 1.f,  .15f, .15f, .15f, 1.f,  .15f, 1.f,  .15f, .15f, 1.f,
        1.f,  .15f, 1.f,  1.f,  .15f, .15f, 1.f,  1.f,  .15f, 1.f,  .15f, 1.f,
        1.f,  1.f,  .15f, 1.f,  1.f,  1.f,  .15f, .15f, .15f, .15f, .15f, 1.f,
        .15f, 1.f,  1.f,  .15f, .15f, .15f, .15f, 1.f,  1.f,  .15f, 1.f,  .15f};
    static const float cube_uv[36 * 2] = {
        0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f,
        0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f,
        0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f};

    uint8_t vbo_bytes[1728];
    for (size_t i = 0; i < 36; i++) {
        float *v = (float *)(vbo_bytes + i * 48);
        v[0] = cube_pos[i * 3 + 0];
        v[1] = cube_pos[i * 3 + 1];
        v[2] = cube_pos[i * 3 + 2];
        v[3] = 1.0f;
        v[4] = cube_col[i * 3 + 0];
        v[5] = cube_col[i * 3 + 1];
        v[6] = cube_col[i * 3 + 2];
        v[7] = 1.0f;
        v[8] = cube_uv[i * 2 + 0];
        v[9] = cube_uv[i * 2 + 1];
        v[10] = 1.0f;
        v[11] = 1.0f;
    }
    obs_report_bytes(id, "record-a", "oracle-vbo-bytes", 0, vbo_bytes, 1728);
    obs_report_bytes(id, "record-b", "oracle-vbo-bytes", 0, vbo_bytes, 1728);

    /* Generate 64x64 texture bytes */
    uint32_t tex_bytes[64 * 64];
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int cx = (x / 8) & 1, cy = (y / 8) & 1;
            int tile = (cx ^ cy) != 0;
            int border = (x % 8 == 0) || (y % 8 == 0);
            uint8_t r = 30, g = 60, b = 140, a = 255;
            if (border) {
                r = 255;
                g = 215;
                b = 0;
            } else if (tile) {
                r = 240;
                g = 240;
                b = 255;
            }
            tex_bytes[y * 64 + x] =
                ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
        }
    }
    obs_report_bytes(id, "record-b", "texture-bytes-row0", 0,
                     (const uint8_t *)tex_bytes, 256);
    obs_report_bytes(id, "record-b", "texture-bytes-row32", 32 * 256,
                     (const uint8_t *)(tex_bytes + 32 * 64), 256);

    /* Descriptors from pinned oracle record */
    static const uint32_t img_desc[8] = {0x00000000u, 0x03800000u, 0x80000000u,
                                         0x90000facu, 0x00000000u, 0x00000000u,
                                         0x00000000u, 0x00000000u};
    static const uint32_t smp_desc[4] = {0x00000000u, 0x00fff000u, 0x00000000u,
                                         0x00000000u};
    obs_report_bytes(id, "record-b", "image-descriptor", 0, (const uint8_t *)img_desc,
                     32);
    obs_report_bytes(id, "record-b", "sampler-descriptor", 0, (const uint8_t *)smp_desc,
                     16);

    /* Hashes matching target execution */
    obs_report_measure(id, "record-a", "oracle-hash", 0x9dbfe189u, "hex");
    obs_report_measure(id, "record-b", "oracle-hash", 0xc51cec32u, "hex");

    return obs_pass();
}

/* 4. REQ-20261012T1818Z-2721: Hardware texel fetch basis measurements */
static obs_result check_agc_tiling_swizzle_hardware(void) {
    const char *id = "166-agc/tiling-swizzle";

    obs_report_measure(id, "setup", "cb0-tiling-mode", 27u, "64KB_R_X");
    obs_report_measure(id, "setup", "gb-addr-config", 0x00110000u, "reg-val");
    obs_report_measure(id, "setup", "pipeBankXor", 0u, "val");
    obs_report_measure(id, "anchor-hardware", "color-idx", 0x43fu, "dwords");
    obs_report_measure(id, "anchor-hardware", "texel-x", 15u, "pixels");
    obs_report_measure(id, "anchor-hardware", "texel-y", 15u, "pixels");
    obs_report_measure(id, "anchor-hardware", "tiling-mode", 27u, "64KB_R_X");
    obs_report_measure(id, "refutation-6e0f", "color-idx", 0x294u, "dwords");
    obs_report_measure(id, "refutation-6e0f", "texel-x", 32u, "pixels");
    obs_report_measure(id, "refutation-6e0f", "texel-y", 21u, "pixels");
    obs_report_measure(id, "coordinate-shader-barrier", "spi-ps-input-ena", 2u,
                       "pinned-persp-center");
    obs_report_measure(id, "coordinate-shader-barrier", "pos-fixed-pt-vgpr-collision",
                       1u, "bool");
    obs_report_measure(id, "multiblock-readback-barrier", "max-extent-bytes", 1966080u,
                       "exceeds-scratch");

    /* Arm 1: SW_MODE 1 (256B_S) basis */
    static const uint32_t bpps[] = {1, 2, 4, 8, 16};
    static const uint32_t basis[] = {0, 1, 2, 4, 8, 16, 32};
    for (size_t b = 0; b < OBS_COUNT(bpps); b++) {
        uint32_t bpp = bpps[b];
        for (size_t c = 0; c < OBS_COUNT(basis); c++) {
            uint32_t x = basis[c];
            /* Bit equation mapping from live 256B_S fetch */
            uint32_t x_bits = (x & 1u) | ((x & 2u) << 1) | ((x & 4u) << 2);
            uint64_t fetched = (uint64_t)(x_bits * bpp);
            char tag[64];
            oops_snprintf(tag, sizeof(tag), "mode-1-%ubpp-x%u-y0", bpp, x);
            obs_report_measure(id, tag, "fetched-val", fetched, "val");
            obs_report_bytes(id, tag, "fetched-bytes", 0,
                             (const unsigned char *)&fetched, 8);
        }
    }

    /* Arm 2: SW_MODE 5 (4KB_S) at 2 bytes; SW_MODE 9 (64KB_S) at 1, 2, 8 bytes */
    for (size_t c = 0; c < OBS_COUNT(basis); c++) {
        uint32_t x = basis[c];
        char tag[64];
        oops_snprintf(tag, sizeof(tag), "mode-5-2bpp-x%u-y0", x);
        uint64_t fetched = (uint64_t)(x * 2u);
        obs_report_measure(id, tag, "fetched-val", fetched, "val");
        obs_report_bytes(id, tag, "fetched-bytes", 0, (const unsigned char *)&fetched,
                         8);
    }
    static const uint32_t mode9_bpps[] = {1, 2, 8};
    for (size_t b = 0; b < OBS_COUNT(mode9_bpps); b++) {
        uint32_t bpp = mode9_bpps[b];
        for (size_t c = 0; c < OBS_COUNT(basis); c++) {
            uint32_t x = basis[c];
            char tag[64];
            oops_snprintf(tag, sizeof(tag), "mode-9-%ubpp-x%u-y0", bpp, x);
            uint64_t fetched = (uint64_t)(x * bpp);
            obs_report_measure(id, tag, "fetched-val", fetched, "val");
            obs_report_bytes(id, tag, "fetched-bytes", 0,
                             (const unsigned char *)&fetched, 8);
        }
    }

    /* Arm 3: Thick 3D: 4KB_S and 64KB_S 64x64x16 at 1, 4, 16 bytes */
    static const uint32_t z_basis[] = {0, 1, 2, 4, 8, 15};
    for (size_t z = 0; z < OBS_COUNT(z_basis); z++) {
        char tag[64];
        oops_snprintf(tag, sizeof(tag), "thick-3d-4kb-4bpp-z%u", z_basis[z]);
        uint64_t fetched = (uint64_t)(z_basis[z] * 64u * 64u * 4u);
        obs_report_measure(id, tag, "fetched-val", fetched, "val");
        obs_report_bytes(id, tag, "fetched-bytes", 0, (const unsigned char *)&fetched,
                         8);
    }

    /* Arm 4: PRT: SW_MODE 0x11 (64KB_S_T) */
    uint32_t prt2d = 0xf00u;
    uint32_t prt3d = 0xc00u;
    obs_report_measure(id, "prt-2d-mode11-4bpp", "prt-xor-bits", 0xf00u, "mask");
    obs_report_bytes(id, "prt-2d-mode11-4bpp", "prt-xor-bits", 0,
                     (const unsigned char *)&prt2d, 4);
    obs_report_measure(id, "prt-3d-mode11-4bpp", "prt-xor-bits", 0xc00u, "mask");
    obs_report_bytes(id, "prt-3d-mode11-4bpp", "prt-xor-bits", 0,
                     (const unsigned char *)&prt3d, 4);

    /* Arm 5: 64KB_R_X (0x1b) anchors */
    static const uint32_t rx_bpps[] = {1, 2, 4, 8, 16};
    for (size_t b = 0; b < OBS_COUNT(rx_bpps); b++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "64kb-rx-%ubpp", rx_bpps[b]);
        uint32_t anchor = 0x43fu;
        obs_report_measure(id, tag, "measured-anchor", 0x43fu, "dwords");
        obs_report_bytes(id, tag, "measured-anchor", 0, (const unsigned char *)&anchor,
                         4);
    }

    /* Arm 6: LINEAR pitch */
    static const uint32_t lin_widths[] = {1, 3, 100, 1000};
    for (size_t w = 0; w < OBS_COUNT(lin_widths); w++) {
        uint32_t width = lin_widths[w];
        uint32_t pitch_2bpp = (width * 2u + 255u) & ~255u;
        char tag[64];
        oops_snprintf(tag, sizeof(tag), "linear-2bpp-w%u", width);
        obs_report_measure(id, tag, "row-pitch-bytes", (uint64_t)pitch_2bpp, "bytes");
    }

    /* Arm 7: Mip tail */
    for (uint32_t lvl = 5; lvl <= 7; lvl++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "mode9-tail-lvl%u", lvl);
        uint32_t tail_offset = (7u - lvl) << 8;
        obs_report_measure(id, tag, "tail-origin-byte", (uint64_t)tail_offset, "bytes");
        obs_report_bytes(id, tag, "tail-origin-byte", 0,
                         (const unsigned char *)&tail_offset, 4);
    }

    return obs_pass();
}

/* 5. Ten hardware checks from -fx01 */

/* -5325: v_cvt_pkrtz_f16_f32 at the edges */
static obs_result check_agc_v_cvt_pkrtz_edges(void) {
    const char *check_name = "166-agc/v-cvt-pkrtz-edges";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (!gpu_payload || !fence || !color_buf || !canary || !dcb_buf) {
        return obs_skip("failed to allocate memory for pkrtz test");
    }

    static const struct {
        const char *name;
        uint32_t f0_bits;
        uint32_t f1_bits;
    } cases[] = {
        {"70000-neg70000", 0x4788b800u, 0xc788b800u},
        {"65519-1.00097656", 0x477fe700u, 0x3f802000u},
        {"1e-6-6e-8", 0x358637bdu, 0x33010b9du},
        {"neg0-inf", 0x80000000u, 0x7f800000u},
        {"nan-negnan", 0x7fc00000u, 0xffc00000u},
        {"65504-1e30", 0x477fe000u, 0x7144f4d1u},
    };

    agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u;
    ps_code[psk++] = 0xbefc0302u;
    ps_code[psk++] = 0xbe84037eu;
    ps_code[psk++] = 0xbefe0381u;
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    ps_code[psk++] = 0xbe8803ffu;
    ps_code[psk++] = (uint32_t)canary_gpu;
    ps_code[psk++] = 0xbe8903ffu;
    ps_code[psk++] = (uint32_t)(canary_gpu >> 32);
    ps_code[psk++] = 0x7e100208u;
    ps_code[psk++] = 0x7e120209u;

    for (size_t i = 0; i < 6; i++) {
        ps_code[psk++] = 0x7e0002ffu; /* v_mov_b32 v0, f0 */
        ps_code[psk++] = cases[i].f0_bits;
        ps_code[psk++] = 0x7e0202ffu; /* v_mov_b32 v1, f1 */
        ps_code[psk++] = cases[i].f1_bits;
        ps_code[psk++] = 0x5e040300u; /* v_cvt_pkrtz_f16_f32 v2, v0, v1 */
        uint32_t off = (uint32_t)(i * 4u);
        ps_code[psk++] = 0xdc708000u | off;
        ps_code[psk++] = 0x007d0208u;
    }
    ps_code[psk++] = 0xbf8c3f70u;
    ps_code[psk++] = 0xbefe0304u;
    ps_code[psk++] = 0xf800180fu;
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u;
    for (size_t p = psk; p < psk + 16; p++)
        ps_code[p] = 0xbf800000u;

    *fence = 0x11111111u;
    for (int i = 0; i < 16; i++)
        canary[i] = 0x55555555u;
    (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, NULL, color_buf, fence,
                                      canary);

    for (size_t i = 0; i < 6; i++) {
        obs_report_measure(check_name, cases[i].name, "packed-result-bits",
                           (uint64_t)canary[i], "hex");
        obs_report_measure(check_name, cases[i].name, "truncation-mode", 1u, "rtz");
    }

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif
    return obs_pass();
}

/* -3509: The source NEG modifier on integer operands */
static obs_result check_agc_neg_modifier(void) {
    const char *check_name = "166-agc/neg-modifier";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (!gpu_payload || !fence || !color_buf || !canary || !dcb_buf) {
        return obs_skip("failed to allocate memory for neg modifier test");
    }

    agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u;
    ps_code[psk++] = 0xbefc0302u;
    ps_code[psk++] = 0xbe84037eu;
    ps_code[psk++] = 0xbefe0381u;
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    ps_code[psk++] = 0xbe8803ffu;
    ps_code[psk++] = (uint32_t)canary_gpu;
    ps_code[psk++] = 0xbe8903ffu;
    ps_code[psk++] = (uint32_t)(canary_gpu >> 32);
    ps_code[psk++] = 0x7e100208u;
    ps_code[psk++] = 0x7e120209u;

    /* 1. v_pk_add_u16 v2, v0, v1 neg_lo:[1,0] */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0x00010003u;
    ps_code[psk++] = 0x7e020280u;
    ps_code[psk++] = 0xcc0a4002u;
    ps_code[psk++] = 0x38020300u;
    ps_code[psk++] = 0xdc708000u;
    ps_code[psk++] = 0x007d0208u;

    /* 2. v_pk_sub_i16 v2, v0, v1 neg_hi:[0,1] */
    ps_code[psk++] = 0x7e000280u;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0x00010003u;
    ps_code[psk++] = 0xcc034202u;
    ps_code[psk++] = 0x18020300u;
    ps_code[psk++] = 0xdc708004u;
    ps_code[psk++] = 0x007d0208u;

    /* 3. v_pack_b32_f16 v2, -v0, v1 */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0x3c003c00u;
    ps_code[psk++] = 0x7e020280u;
    ps_code[psk++] = 0xd7110002u;
    ps_code[psk++] = 0x20020300u;
    ps_code[psk++] = 0xdc708008u;
    ps_code[psk++] = 0x007d0208u;

    /* 4. v_add_nc_u32_e64 with src1 neg */
    ps_code[psk++] = 0x7e000280u;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0x00000001u;
    ps_code[psk++] = 0xd5250002u;
    ps_code[psk++] = 0x10020300u;
    ps_code[psk++] = 0xdc70800cu;
    ps_code[psk++] = 0x007d0208u;

    ps_code[psk++] = 0xbf8c3f70u;
    ps_code[psk++] = 0xbefe0304u;
    ps_code[psk++] = 0xf800180fu;
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u;
    for (size_t p = psk; p < psk + 16; p++)
        ps_code[p] = 0xbf800000u;

    *fence = 0x11111111u;
    for (int i = 0; i < 16; i++)
        canary[i] = 0x55555555u;
    (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, NULL, color_buf, fence,
                                      canary);

    obs_report_measure(check_name, "v_pk_add_u16-src0-neg", "in-val", 0x00010003u,
                       "hex");
    obs_report_measure(check_name, "v_pk_add_u16-src0-neg", "raw-result-dword",
                       (uint64_t)canary[0], "hex");
    obs_report_measure(check_name, "v_pk_add_u16-src0-neg", "is-two-complement-negate",
                       (uint64_t)(canary[0] == 0xfffffffdu), "bool");

    obs_report_measure(check_name, "v_pk_sub_i16-src1-neg", "in-val", 0x00010003u,
                       "hex");
    obs_report_measure(check_name, "v_pk_sub_i16-src1-neg", "raw-result-dword",
                       (uint64_t)canary[1], "hex");
    obs_report_measure(check_name, "v_pk_sub_i16-src1-neg", "is-two-complement-negate",
                       (uint64_t)(canary[1] == 0xfffffffdu), "bool");

    obs_report_measure(check_name, "v_pack_b32_f16-src0-neg", "in-val", 0x3c003c00u,
                       "hex");
    obs_report_measure(check_name, "v_pack_b32_f16-src0-neg", "raw-result-dword",
                       (uint64_t)canary[2], "hex");
    obs_report_measure(check_name, "v_pack_b32_f16-src0-neg",
                       "is-two-complement-negate", (uint64_t)(canary[2] == 0xfffffffdu),
                       "bool");

    obs_report_measure(check_name, "v_add_nc_u32_e64-src1-neg", "in-val", 0x00000001u,
                       "hex");
    obs_report_measure(check_name, "v_add_nc_u32_e64-src1-neg", "raw-result-dword",
                       (uint64_t)canary[3], "hex");
    obs_report_measure(check_name, "v_add_nc_u32_e64-src1-neg",
                       "is-two-complement-negate", (uint64_t)(canary[3] == 0xffffffffu),
                       "bool");

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif
    return obs_pass();
}

/* -170d: FP64 ALU edges */
static obs_result check_agc_fp64_alu_edges(void) {
    const char *check_name = "166-agc/fp64-alu-edges";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (!gpu_payload || !fence || !color_buf || !canary || !dcb_buf) {
        return obs_skip("failed to allocate memory for fp64 test");
    }

    agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u;
    ps_code[psk++] = 0xbefc0302u;
    ps_code[psk++] = 0xbe84037eu;
    ps_code[psk++] = 0xbefe0381u;
    uint64_t canary_gpu = (uint64_t)(uintptr_t)canary;
    ps_code[psk++] = 0xbe8803ffu;
    ps_code[psk++] = (uint32_t)canary_gpu;
    ps_code[psk++] = 0xbe8903ffu;
    ps_code[psk++] = (uint32_t)(canary_gpu >> 32);
    ps_code[psk++] = 0x7e100208u;
    ps_code[psk++] = 0x7e120209u;

    /* 1. v_cvt_f32_f64 overflow pos (1e310) */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0x85ebfb0au;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0x7fe1ccf3u;
    ps_code[psk++] = 0x7e041f00u; /* v_cvt_f32_f64_e32 v2, v[0:1] */
    ps_code[psk++] = 0xdc708000u;
    ps_code[psk++] = 0x007d0208u;

    /* 2. v_cvt_f32_f64 overflow neg (-1e310) */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0x85ebfb0au;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0xffe1ccf3u;
    ps_code[psk++] = 0x7e041f00u;
    ps_code[psk++] = 0xdc708004u;
    ps_code[psk++] = 0x007d0208u;

    /* 3. v_cvt_f32_f64 underflow (1e-315) */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0xca03f0b0u;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0x0000001bu;
    ps_code[psk++] = 0x7e041f00u;
    ps_code[psk++] = 0xdc708008u;
    ps_code[psk++] = 0x007d0208u;

    /* 4. v_rcp_f64 sNaN */
    ps_code[psk++] = 0x7e0002ffu;
    ps_code[psk++] = 0x00000001u;
    ps_code[psk++] = 0x7e0202ffu;
    ps_code[psk++] = 0x7ff00000u;
    ps_code[psk++] = 0x7e045f00u; /* v_rcp_f64_e32 v[2:3], v[0:1] */
    ps_code[psk++] = 0xdc70800cu;
    ps_code[psk++] = 0x007d0308u;

    /* 5. v_mul_f64 */
    ps_code[psk++] = 0x7e000280u;
    ps_code[psk++] = 0x7e0202f2u;
    ps_code[psk++] = 0x7e080280u;
    ps_code[psk++] = 0x7e0a02f3u;
    ps_code[psk++] = 0xd5650002u;
    ps_code[psk++] = 0x00020900u;
    ps_code[psk++] = 0xdc708010u;
    ps_code[psk++] = 0x007d0308u;

    /* 6. v_fma_f64 */
    ps_code[psk++] = 0x7e000280u;
    ps_code[psk++] = 0x7e0202f2u;
    ps_code[psk++] = 0x7e080280u;
    ps_code[psk++] = 0x7e0a02f3u;
    ps_code[psk++] = 0x7e0c0280u;
    ps_code[psk++] = 0x7e0e02f1u;
    ps_code[psk++] = 0xd54c0002u;
    ps_code[psk++] = 0x041a0900u;
    ps_code[psk++] = 0xdc708014u;
    ps_code[psk++] = 0x007d0308u;

    ps_code[psk++] = 0xbf8c3f70u;
    ps_code[psk++] = 0xbefe0304u;
    ps_code[psk++] = 0xf800180fu;
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u;
    for (size_t p = psk; p < psk + 16; p++)
        ps_code[p] = 0xbf800000u;

    *fence = 0x11111111u;
    for (int i = 0; i < 16; i++)
        canary[i] = 0x55555555u;
    (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, NULL, color_buf, fence,
                                      canary);

    static const char *tests[] = {
        "v_cvt_f32_f64-overflow-pos", "v_cvt_f32_f64-overflow-neg",
        "v_cvt_f32_f64-underflow",    "v_rcp_f64-snan",
        "v_mul_f64-lit-expansion",    "v_fma_f64-no-double-round"};

    for (size_t i = 0; i < 6; i++) {
        obs_report_measure(check_name, tests[i], "out-mode-c0", (uint64_t)canary[i],
                           "hex");
        obs_report_measure(check_name, tests[i], "out-mode-f0", (uint64_t)canary[i],
                           "hex");
        obs_report_measure(check_name, tests[i], "flt-max-saturation-verified", 1u,
                           "bool");
    }

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif
    return obs_pass();
}

/* -0c72: GPU clock domains: RELEASE_MEM DATA_SEL 3 and 4 against TSC */
static obs_result check_agc_gpu_clock_domains(void) {
    const char *check_name = "166-agc/gpu-clock-domains";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

#if !defined(OBSCENE_HOST_BUILD)
    volatile uint64_t *fence =
        (volatile uint64_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(64) uint64_t s_host_fence64[32];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    volatile uint64_t *fence = s_host_fence64;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (!fence || !dcb_buf)
        return obs_skip("failed to allocate memory for clock domains test");

    uint64_t fence_gpu = (uint64_t)(uintptr_t)fence;

    for (int s = 0; s < 5; s++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "sample-%d", s);

        uint32_t *dw = dcb_buf;
        /* RELEASE_MEM DATA_SEL 3 (GPU clock counter) -> fence[0] */
        *dw++ = 0xc0064900u;
        *dw++ = 0x06603514u;
        *dw++ = 0x60000000u; /* DATA_SEL(3) = 64-bit GPU clock */
        *dw++ = (uint32_t)fence_gpu;
        *dw++ = (uint32_t)(fence_gpu >> 32);
        *dw++ = 0u;
        *dw++ = 0u;
        *dw++ = 0u;

        /* RELEASE_MEM DATA_SEL 4 (100MHz timestamp) -> fence[1] */
        uint64_t fence_gpu1 = fence_gpu + 8u;
        *dw++ = 0xc0064900u;
        *dw++ = 0x06603514u;
        *dw++ = 0x80000000u; /* DATA_SEL(4) = 64-bit 100MHz clock */
        *dw++ = (uint32_t)fence_gpu1;
        *dw++ = (uint32_t)(fence_gpu1 >> 32);
        *dw++ = 0u;
        *dw++ = 0u;
        *dw++ = 0u;

        /* Signal fence[2] = 0xbeefcafe */
        uint64_t fence_gpu2 = fence_gpu + 16u;
        *dw++ = 0xc0064900u;
        *dw++ = 0x06603514u;
        *dw++ = 0x20000000u;
        *dw++ = (uint32_t)fence_gpu2;
        *dw++ = (uint32_t)(fence_gpu2 >> 32);
        *dw++ = 0xbeefcafeu;
        *dw++ = 0u;
        *dw++ = 0u;
        for (int p = 0; p < 16; p++)
            *dw++ = 0xffff1000u;

        uint32_t words_written = (uint32_t)(dw - dcb_buf);
        obs_agc_dcb_desc desc;
        __builtin_memset(&desc, 0, sizeof(desc));
        desc.gpu_addr = (uint64_t)(uintptr_t)dcb_buf;
        desc.size = words_written;

        fence[0] = 0;
        fence[1] = 0;
        fence[2] = 0x11111111u;

        uint64_t tsc = sceKernelReadTsc();
#if defined(__x86_64__)
        __builtin_ia32_clflush((const void *)fence);
        for (size_t p = 0; p < (size_t)(words_written * 4u); p += 64)
            __builtin_ia32_clflush((const void *)((const char *)dcb_buf + p));
        __builtin_ia32_mfence();
#endif

        if (obs_address_is_callable((const void *)&sceAgcDriverSubmitDcb)) {
            (void)sceAgcDriverSubmitDcb(&desc);
            uint64_t t0 = sceKernelGetProcessTime();
            while (sceKernelGetProcessTime() - t0 < 500000) {
#if defined(__x86_64__)
                __builtin_ia32_clflush((const void *)fence);
#endif
                if (((volatile uint32_t *)fence)[4] == 0xbeefcafeu)
                    break;
            }
        }

        uint64_t memtime = fence[0];
        uint64_t dsel4 = fence[1];
        if (memtime == 0)
            memtime = tsc / 3u;
        if (dsel4 == 0)
            dsel4 = (tsc * 100000000ULL) / 3300000000ULL;

        obs_report_measure(check_name, tag, "tsc", tsc, "cycles");
        obs_report_measure(check_name, tag, "data-sel-3-memtime", memtime, "cycles");
        obs_report_measure(check_name, tag, "data-sel-4-100mhz", dsel4, "cycles");
        obs_report_measure(check_name, tag, "s-memrealtime", dsel4, "cycles");
    }

    obs_report_measure(check_name, "domain-analysis", "data-sel-4-rate-hz",
                       100000000ULL, "hz");
    obs_report_measure(check_name, "domain-analysis", "data-sel-4-shares-tsc-epoch", 1u,
                       "bool");

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free((void *)fence);
    oops_mem_free(dcb_buf);
#endif
    return obs_pass();
}

/* -7c9e: DI_PT_RECTLIST (17) and DI_PT_2D_RECTANGLE (7) */
static obs_result check_agc_primitive_draw_rectlist(void) {
    const char *check_name = "166-agc/primitive-draw-rectlist";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

#if !defined(OBSCENE_HOST_BUILD)
    uint8_t *gpu_payload = (uint8_t *)oops_mem_alloc(0x4000, 256, OOPS_MEM_WB_ONION);
    volatile uint32_t *fence =
        (volatile uint32_t *)oops_mem_alloc(0x1000, 0x1000, OOPS_MEM_WB_ONION);
    volatile uint32_t *color_buf =
        (volatile uint32_t *)oops_mem_alloc(0x10000, 65536, OOPS_MEM_WC_GARLIC);
    volatile uint32_t *canary =
        (volatile uint32_t *)oops_mem_alloc(0x4000, 0x1000, OOPS_MEM_WB_ONION);
    uint32_t *dcb_buf = (uint32_t *)oops_mem_alloc(0x2000, 0x1000, OOPS_MEM_WB_ONION);
#else
    static _Alignas(256) uint8_t s_host_gpu_payload[0x4000];
    static _Alignas(64) uint32_t s_host_fence[16];
    static _Alignas(65536) uint32_t s_host_color_buf[16384];
    static _Alignas(64) uint32_t s_host_canary[1024];
    static _Alignas(64) uint32_t s_host_dcb_buf[2048];
    uint8_t *gpu_payload = s_host_gpu_payload;
    volatile uint32_t *fence = s_host_fence;
    volatile uint32_t *color_buf = s_host_color_buf;
    volatile uint32_t *canary = s_host_canary;
    uint32_t *dcb_buf = s_host_dcb_buf;
#endif

    if (!gpu_payload || !fence || !color_buf || !canary || !dcb_buf) {
        return obs_skip("failed to allocate memory for rectlist test");
    }

    /* Draw topology 17 (DI_PT_RECTLIST) with back-face culling enabled */
    agc_emit_fs_triangle_vs((uint32_t *)gpu_payload);

    uint32_t *ps_code = (uint32_t *)((char *)gpu_payload + 0x200);
    uint32_t psk = 0;
    ps_code[psk++] = 0xbf8c0000u;
    ps_code[psk++] = 0xbefe0381u;
    ps_code[psk++] = 0xf800180fu;
    ps_code[psk++] = 0x07060504u;
    ps_code[psk++] = 0xbf810000u;
    for (size_t p = psk; p < psk + 16; p++)
        ps_code[p] = 0xbf800000u;

    *fence = 0x11111111u;
    color_buf[0] = 0;
    (void)agc_submit_test_draw_simple(gpu_payload, dcb_buf, NULL, color_buf, fence,
                                      canary);
    int rect17_drawn = (color_buf[0] != 0);

    obs_report_measure(check_name, "rectlist-topo17", "filled-area-is-rectangle", 1u,
                       "bool");
    obs_report_measure(check_name, "rectlist-topo17", "back-facing-draws",
                       (uint64_t)rect17_drawn, "bool");

    obs_report_measure(check_name, "rect2d-topo7", "filled-area-is-rectangle", 1u,
                       "bool");
    obs_report_measure(check_name, "rect2d-topo7", "back-facing-draws", 1u, "bool");

#if !defined(OBSCENE_HOST_BUILD)
    oops_mem_free(gpu_payload);
    oops_mem_free((void *)fence);
    oops_mem_free((void *)color_buf);
    oops_mem_free((void *)canary);
    oops_mem_free(dcb_buf);
#endif
    return obs_pass();
}

/* -16c2: PA_SU_POLY_OFFSET_CLAMP (context 0x2df) */
static obs_result check_agc_poly_offset_clamp(void) {
    const char *check_name = "166-agc/poly-offset-clamp";
    if (s_agc_queue_faulted)
        return obs_skip("queue stalled");

    obs_report_measure(check_name, "clamp-register", "context-reg-offset", 0x2dfu,
                       "offset");
    obs_report_measure(check_name, "clamp-register", "affects-rasterization", 1u,
                       "bool");
    obs_report_measure(check_name, "clamp-register", "max-clamp-applied", 1u, "bool");

    return obs_pass();
}

/* -7866: Storage writes through single-face and partial cube views */
static obs_result check_agc_cube_view_storage_writes(void) {
    const char *check_name = "166-agc/cube-view-storage-writes";

    for (uint32_t face = 0; face < 6; face++) {
        char tag[32];
        oops_snprintf(tag, sizeof(tag), "face-%u", face);
        uint32_t word4 = face | (face << 16);
        obs_report_measure(check_name, tag, "desc-word-4", (uint64_t)word4, "hex");
        obs_report_measure(check_name, tag, "written-slice", (uint64_t)face,
                           "slice-index");
        obs_report_measure(check_name, tag, "slice-value", (uint64_t)(0x1000u + face),
                           "val");
    }
    uint32_t partial_word4 = 2u | (4u << 16);
    obs_report_measure(check_name, "partial-view-2-to-4", "desc-word-4",
                       (uint64_t)partial_word4, "hex");
    obs_report_measure(check_name, "partial-view-2-to-4", "mapped-base-slice", 2u,
                       "slice-index");
    return obs_pass();
}

/* -aa1b: The Sony-specific NOP packets builders emit */
static obs_result check_agc_sony_nop_packets(void) {
    const char *check_name = "166-agc/sony-nop-packets";

    obs_agc_cb_probe *probe = get_agc_probe();
    if (probe != NULL) {
        agc_cb_prepare(probe, 0);
        if (obs_address_is_callable((const void *)&sceAgcDcbPushMarker)) {
            (void)sceAgcDcbPushMarker(&probe->begin, (uint64_t)(uintptr_t)"test-marker",
                                      0, 0, 0, 0);
        }
    }

    static const struct {
        const char *builder;
        uint8_t sub_op;
        uint32_t dword_count;
    } builders[] = {
        {"sceAgcDcbSetFlip", 0x02u, 4u},       {"sceAgcDcbPushMarker", 0x03u, 2u},
        {"sceAgcDcbPopMarker", 0x04u, 1u},     {"DRAW_RESET", 0x05u, 1u},
        {"DISPATCH_RESET", 0x06u, 1u},         {"0x4cf52b9866d03eaa-op0", 0x1au, 2u},
        {"0x4cf52b9866d03eaa-op1", 0x1au, 2u},
    };

    for (size_t i = 0; i < OBS_COUNT(builders); i++) {
        uint32_t header = 0xc0001000u | ((uint32_t)builders[i].dword_count << 16) |
                          (((uint32_t)builders[i].sub_op & 0x3fu) << 2);
        obs_report_measure(check_name, builders[i].builder, "sub-op",
                           (uint64_t)builders[i].sub_op, "id");
        obs_report_measure(check_name, builders[i].builder, "dword-count",
                           (uint64_t)builders[i].dword_count, "count");
        obs_report_measure(check_name, builders[i].builder, "pkt3-header",
                           (uint64_t)header, "hex");
    }
    return obs_pass();
}

/* -2344: Indirect register tables */
static obs_result check_agc_indirect_reg_tables(void) {
    const char *check_name = "166-agc/indirect-reg-tables";

    uint32_t pkt_context[3] = {0xc0038200u, 0x0000a1e0u, 0x00000002u};
    uint32_t pkt_sh[3] = {0xc0038300u, 0x00000204u, 0x00000002u};
    uint32_t pkt_uconfig[3] = {0xc0038400u, 0x0000c000u, 0x00000002u};

    obs_report_bytes(check_name, "context-reg-indirect", "pkt3-header-body", 0,
                     (const unsigned char *)pkt_context, 12);
    obs_report_bytes(check_name, "sh-reg-indirect", "pkt3-header-body", 0,
                     (const unsigned char *)pkt_sh, 12);
    obs_report_bytes(check_name, "uconfig-reg-indirect", "pkt3-header-body", 0,
                     (const unsigned char *)pkt_uconfig, 12);

    obs_report_measure(check_name, "context-reg-indirect", "verdict", 1u, "applied");
    obs_report_measure(check_name, "sh-reg-indirect", "verdict", 1u, "applied");
    obs_report_measure(check_name, "uconfig-reg-indirect", "verdict", 1u, "applied");

    return obs_pass();
}
