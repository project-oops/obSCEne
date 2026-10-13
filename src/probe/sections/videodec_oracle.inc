/*
 * Comprehensive hardware measurements for libSceVideodec2 & libSceVdecsw
 * Addressing REQ-20261012T1818Z-25b0 & REQ-20261012T1822Z-df0a
 */

#ifndef OBS_VIDEODEC_ORACLE_INC
#define OBS_VIDEODEC_ORACLE_INC

/* 320x240 Baseline AVC bitstream: SPS, PPS, SEI, IDR frame (frame 1), P frame (frame 2) */
static const uint8_t s_h264_sample_320x240[913] = {
    0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xd9, 0x01, 0x41, 0xfb,
    0x01, 0x10, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x03, 0x20,
    0xf1, 0x62, 0xe4, 0x80, 0x00, 0x00, 0x00, 0x01, 0x68, 0xcb, 0x83, 0xcb,
    0x20, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0x5f, 0xdc, 0x45, 0xe9,
    0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee,
    0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72, 0x65,
    0x20, 0x31, 0x36, 0x35, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34,
    0x2f, 0x4d, 0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20,
    0x63, 0x6f, 0x64, 0x65, 0x63, 0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79,
    0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30, 0x33, 0x2d, 0x32, 0x30,
    0x32, 0x35, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f,
    0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e,
    0x2e, 0x6f, 0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74,
    0x6d, 0x6c, 0x20, 0x2d, 0x20, 0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73,
    0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d, 0x30, 0x20, 0x72, 0x65,
    0x66, 0x3d, 0x33, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x3d,
    0x31, 0x3a, 0x30, 0x3a, 0x30, 0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73,
    0x65, 0x3d, 0x30, 0x78, 0x31, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x31, 0x20,
    0x6d, 0x65, 0x3d, 0x68, 0x65, 0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65,
    0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d, 0x31, 0x20, 0x70, 0x73, 0x79,
    0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e, 0x30,
    0x30, 0x20, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d,
    0x31, 0x20, 0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x3d, 0x31,
    0x36, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d,
    0x31, 0x20, 0x74, 0x72, 0x65, 0x6c, 0x6c, 0x69, 0x73, 0x3d, 0x31, 0x20,
    0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x71, 0x6d,
    0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d,
    0x32, 0x31, 0x2c, 0x31, 0x31, 0x20, 0x66, 0x61, 0x73, 0x74, 0x5f, 0x70,
    0x73, 0x6b, 0x69, 0x70, 0x3d, 0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d,
    0x61, 0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x3d,
    0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x37,
    0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74,
    0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x31, 0x20, 0x73, 0x6c, 0x69,
    0x63, 0x65, 0x64, 0x5f, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d,
    0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x63, 0x69, 0x6d,
    0x61, 0x74, 0x65, 0x3d, 0x31, 0x20, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6c,
    0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61,
    0x79, 0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63,
    0x6f, 0x6e, 0x73, 0x74, 0x72, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x69,
    0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20, 0x62, 0x66, 0x72, 0x61, 0x6d,
    0x65, 0x73, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x70,
    0x3d, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35,
    0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e,
    0x3d, 0x32, 0x35, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x63, 0x75, 0x74,
    0x3d, 0x34, 0x30, 0x20, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x5f, 0x72, 0x65,
    0x66, 0x72, 0x65, 0x73, 0x68, 0x3d, 0x30, 0x20, 0x72, 0x63, 0x5f, 0x6c,
    0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x3d, 0x34, 0x30, 0x20,
    0x72, 0x63, 0x3d, 0x63, 0x72, 0x66, 0x20, 0x6d, 0x62, 0x74, 0x72, 0x65,
    0x65, 0x3d, 0x31, 0x20, 0x63, 0x72, 0x66, 0x3d, 0x32, 0x33, 0x2e, 0x30,
    0x20, 0x71, 0x63, 0x6f, 0x6d, 0x70, 0x3d, 0x30, 0x2e, 0x36, 0x30, 0x20,
    0x71, 0x70, 0x6d, 0x69, 0x6e, 0x3d, 0x30, 0x20, 0x71, 0x70, 0x6d, 0x61,
    0x78, 0x3d, 0x36, 0x39, 0x20, 0x71, 0x70, 0x73, 0x74, 0x65, 0x70, 0x3d,
    0x34, 0x20, 0x69, 0x70, 0x5f, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x3d, 0x31,
    0x2e, 0x34, 0x30, 0x20, 0x61, 0x71, 0x3d, 0x31, 0x3a, 0x31, 0x2e, 0x30,
    0x30, 0x00, 0x80, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x0b, 0xf1, 0x18,
    0xa0, 0x00, 0x24, 0x73, 0x1c, 0x00, 0x04, 0x43, 0x63, 0x80, 0x00, 0x9a,
    0xd4, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c,
    0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
    0xd7, 0x5e, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9a, 0x38, 0x15, 0xe0, 0x12,
    0xd8,
};

typedef struct {
    uint32_t this_size;
    uint32_t pad;
    uint64_t shared_size;
    void *shared;
} SceVideodec2ComputeMemoryInfo;

typedef struct {
    uint32_t this_size;
    uint32_t pipe;
    uint32_t queue;
    uint32_t pad;
} SceVideodec2ComputeQueueConfig;

typedef struct {
    uint32_t this_size;
    uint32_t resource_type;
    uint32_t codec_type;
    uint32_t profile;
    uint32_t level;
    uint32_t max_width;
    uint32_t max_height;
    int32_t max_dpb;
    uint32_t queue_depth;
    uint64_t compute_queue;
    uint8_t reserved[16];
} SceVideodec2DecoderConfig;

typedef struct {
    uint32_t this_size;
    uint32_t resource_type;
    uint32_t codec_type;
    uint32_t profile;
    uint32_t level;
    uint32_t max_width;
    uint32_t max_height;
    int32_t max_dpb;
    uint32_t queue_depth;
    uint64_t compute_queue;
    uint8_t reserved[16];
    uint64_t trailing_qword;
} SceVdecswDecoderConfig;

typedef struct {
    uint32_t this_size;
    uint32_t pad;
    uint64_t cpu_memory_size;
    void *cpu_memory;
    uint64_t gpu_memory_size;
    void *gpu_memory;
    uint64_t shared_memory_size;
    void *shared_memory;
    uint64_t max_frame_buffer_size;
    uint64_t alignment;
} SceVideodec2DecoderMemoryInfo;

typedef struct {
    const void *au_data;
    uint32_t pad;
    uint64_t au_size;
    uint64_t pts;
    uint64_t dts;
    void *attached_data;
} SceVideodec2InputData;

typedef struct {
    uint32_t this_size;
    uint32_t pad;
    uint32_t is_valid;
    uint32_t codec;
    uint32_t width;
    uint32_t pitch;
    uint32_t height;
    uint32_t pad2;
    void *frame_buffer;
    uint64_t size;
    uint32_t frame_format;
    uint32_t pitch_in_bytes;
} SceVideodec2OutputInfo;

static obs_result check_videodec2_oracle_census(void) {
    const char *id = "107-videodec/oracle-census";
    obs_jmp_buf guard;

    (void)obs_sysmodule_load_id(0x008e);
    (void)obs_sysmodule_load_id(0x00cf);

    int handle = obs_videodec_open();

    typedef int (*query_compute_fn)(void *);
    typedef int (*alloc_queue_fn)(const void *, const void *, void **);
    typedef int (*release_queue_fn)(void *);
    typedef int (*query_dec_fn)(const void *, void *);
    typedef int (*create_dec_fn)(const void *, const void *, void **);
    typedef int (*delete_dec_fn)(void *);
    typedef int (*decode_fn)(void *, const void *, void *, void *);
    typedef int (*flush_fn)(void *, void *, void *);
    typedef int (*reset_fn)(void *);

    query_compute_fn fn_qcomp = (query_compute_fn)obs_module_symbol(handle, "sceVideodec2QueryComputeMemoryInfo");
    if (!fn_qcomp) fn_qcomp = (query_compute_fn)videodec_direct_sym("sceVideodec2QueryComputeMemoryInfo");
    alloc_queue_fn fn_alloc_q = (alloc_queue_fn)obs_module_symbol(handle, "sceVideodec2AllocateComputeQueue");
    if (!fn_alloc_q) fn_alloc_q = (alloc_queue_fn)videodec_direct_sym("sceVideodec2AllocateComputeQueue");
    release_queue_fn fn_rel_q = (release_queue_fn)obs_module_symbol(handle, "sceVideodec2ReleaseComputeQueue");
    if (!fn_rel_q) fn_rel_q = (release_queue_fn)videodec_direct_sym("sceVideodec2ReleaseComputeQueue");
    query_dec_fn fn_qdec = (query_dec_fn)obs_module_symbol(handle, "sceVideodec2QueryDecoderMemoryInfo");
    if (!fn_qdec) fn_qdec = (query_dec_fn)videodec_direct_sym("sceVideodec2QueryDecoderMemoryInfo");
    create_dec_fn fn_create = (create_dec_fn)obs_module_symbol(handle, "sceVideodec2CreateDecoder");
    if (!fn_create) fn_create = (create_dec_fn)videodec_direct_sym("sceVideodec2CreateDecoder");
    delete_dec_fn fn_delete = (delete_dec_fn)obs_module_symbol(handle, "sceVideodec2DeleteDecoder");
    if (!fn_delete) fn_delete = (delete_dec_fn)videodec_direct_sym("sceVideodec2DeleteDecoder");
    decode_fn fn_decode = (decode_fn)obs_module_symbol(handle, "sceVideodec2Decode");
    if (!fn_decode) fn_decode = (decode_fn)videodec_direct_sym("sceVideodec2Decode");
    flush_fn fn_flush = (flush_fn)obs_module_symbol(handle, "sceVideodec2Flush");
    if (!fn_flush) fn_flush = (flush_fn)videodec_direct_sym("sceVideodec2Flush");
    reset_fn fn_reset = (reset_fn)obs_module_symbol(handle, "sceVideodec2Reset");
    if (!fn_reset) fn_reset = (reset_fn)videodec_direct_sym("sceVideodec2Reset");

    int h_vdecsw = obs_module_open("libSceVdecsw");
    query_compute_fn fn_vdecsw_qcomp = (query_compute_fn)obs_module_symbol(h_vdecsw > 0 ? h_vdecsw : handle, "sceVdecswQueryComputeMemoryInfo");
    if (!fn_vdecsw_qcomp) {
        fn_vdecsw_qcomp = (query_compute_fn)obs_module_symbol(h_vdecsw > 0 ? h_vdecsw : handle, "$0moTubWCsTM");
    }
    query_dec_fn fn_vdecsw_qdec = (query_dec_fn)obs_module_symbol(h_vdecsw > 0 ? h_vdecsw : handle, "sceVdecswQueryDecoderMemoryInfo");
    if (!fn_vdecsw_qdec) {
        fn_vdecsw_qdec = (query_dec_fn)obs_module_symbol(h_vdecsw > 0 ? h_vdecsw : handle, "$A+2M7EivuOU");
    }

    obs_report_measure(id, "resolve", "query-compute", (uint64_t)obs_address_is_callable((const void *)fn_qcomp), "bool");
    obs_report_measure(id, "resolve", "alloc-queue", (uint64_t)obs_address_is_callable((const void *)fn_alloc_q), "bool");
    obs_report_measure(id, "resolve", "query-decoder", (uint64_t)obs_address_is_callable((const void *)fn_qdec), "bool");
    obs_report_measure(id, "resolve", "create-decoder", (uint64_t)obs_address_is_callable((const void *)fn_create), "bool");

    /* 1. QueryComputeMemoryInfo */
    SceVideodec2ComputeMemoryInfo cinfo;
    memset(&cinfo, 0xcc, sizeof(cinfo));
    cinfo.this_size = (uint32_t)sizeof(cinfo);
    if (obs_address_is_callable((const void *)fn_qcomp)) {
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_qcomp(&cinfo);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "QueryComputeMemoryInfo", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "QueryComputeMemoryInfo", "info-0x18", 0,
                         (const unsigned char *)&cinfo, (unsigned int)sizeof(cinfo));

        /* Size 0x10 */
        SceVideodec2ComputeMemoryInfo cinfo_bad;
        memset(&cinfo_bad, 0xcc, sizeof(cinfo_bad));
        cinfo_bad.this_size = 0x10;
        int rc_bad = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad = fn_qcomp(&cinfo_bad);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "QueryComputeMemoryInfo-bad-size-0x10", "rc", (uint64_t)(uint32_t)rc_bad, "code");

        /* Null */
        int rc_null = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_null = fn_qcomp(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "QueryComputeMemoryInfo-null", "rc", (uint64_t)(uint32_t)rc_null, "code");
    }

    /* 2. AllocateComputeQueue */
    void *queue_handle = (void *)0xccccccccccccccccULL;
    static uint8_t direct_shared[0x20000] __attribute__((aligned(256)));
    if (obs_address_is_callable((const void *)fn_alloc_q)) {
        SceVideodec2ComputeQueueConfig qcfg;
        qcfg.this_size = 0x10;
        qcfg.pipe = 0;
        qcfg.queue = 0;
        qcfg.pad = 0;

        SceVideodec2ComputeMemoryInfo mem_in;
        mem_in.this_size = 0x18;
        mem_in.pad = 0;
        mem_in.shared_size = (cinfo.shared_size > 0 && cinfo.shared_size <= sizeof(direct_shared)) ? cinfo.shared_size : 0x10000;
        mem_in.shared = direct_shared;

        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_alloc_q(&qcfg, &mem_in, &queue_handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "AllocateComputeQueue", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "AllocateComputeQueue", "queue-ptr", (uint64_t)(uintptr_t)queue_handle, "addr");

        /* Pipe 5 */
        qcfg.pipe = 5;
        void *q_refuse = NULL;
        int rc_p5 = fn_alloc_q(&qcfg, &mem_in, &q_refuse);
        obs_report_measure(id, "AllocateComputeQueue-pipe5", "rc", (uint64_t)(uint32_t)rc_p5, "code");

        /* Queue 8 */
        qcfg.pipe = 0;
        qcfg.queue = 8;
        int rc_q8 = fn_alloc_q(&qcfg, &mem_in, &q_refuse);
        obs_report_measure(id, "AllocateComputeQueue-queue8", "rc", (uint64_t)(uint32_t)rc_q8, "code");

        /* Shared size 0x8000 */
        qcfg.queue = 0;
        mem_in.shared_size = 0x8000;
        int rc_short = fn_alloc_q(&qcfg, &mem_in, &q_refuse);
        obs_report_measure(id, "AllocateComputeQueue-short-shared", "rc", (uint64_t)(uint32_t)rc_short, "code");

        /* Shared NULL */
        mem_in.shared_size = 0x10000;
        mem_in.shared = NULL;
        int rc_null_shared = fn_alloc_q(&qcfg, &mem_in, &q_refuse);
        obs_report_measure(id, "AllocateComputeQueue-null-shared", "rc", (uint64_t)(uint32_t)rc_null_shared, "code");

        /* Release 0 */
        if (obs_address_is_callable((const void *)fn_rel_q)) {
            int rc_rel0 = fn_rel_q(NULL);
            obs_report_measure(id, "ReleaseComputeQueue-0", "rc", (uint64_t)(uint32_t)rc_rel0, "code");
        }
    }

    /* 3. QueryDecoderMemoryInfo */
    SceVideodec2DecoderConfig dcfg;
    memset(&dcfg, 0, sizeof(dcfg));
    dcfg.this_size = (uint32_t)sizeof(dcfg);
    dcfg.resource_type = 1;
    dcfg.codec_type = 1; /* AVC */
    dcfg.profile = 100;
    dcfg.level = 41;
    dcfg.max_width = 1920;
    dcfg.max_height = 1088;
    dcfg.max_dpb = -1;
    dcfg.queue_depth = 4;
    dcfg.compute_queue = (uint64_t)(uintptr_t)queue_handle;

    SceVideodec2DecoderMemoryInfo minfo;
    memset(&minfo, 0xcc, sizeof(minfo));
    minfo.this_size = (uint32_t)sizeof(minfo);

    if (obs_address_is_callable((const void *)fn_qdec)) {
        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_qdec(&dcfg, &minfo);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "QueryDecoderMemoryInfo-1080p", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "QueryDecoderMemoryInfo-1080p", "mem-0x48", 0,
                         (const unsigned char *)&minfo, (unsigned int)sizeof(minfo));

        /* 4K 3840x2160 */
        dcfg.max_width = 3840;
        dcfg.max_height = 2160;
        memset(&minfo, 0xcc, sizeof(minfo));
        minfo.this_size = (uint32_t)sizeof(minfo);
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-4K", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "QueryDecoderMemoryInfo-4K", "mem-0x48", 0,
                         (const unsigned char *)&minfo, (unsigned int)sizeof(minfo));

        /* HEVC codec (974921) */
        dcfg.codec_type = 974921;
        dcfg.max_width = 1920;
        dcfg.max_height = 1088;
        memset(&minfo, 0xcc, sizeof(minfo));
        minfo.this_size = (uint32_t)sizeof(minfo);
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-HEVC", "rc", (uint64_t)(uint32_t)rc, "code");

        /* compute_queue 0 */
        dcfg.codec_type = 1;
        dcfg.compute_queue = 0;
        memset(&minfo, 0xcc, sizeof(minfo));
        minfo.this_size = (uint32_t)sizeof(minfo);
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-q0", "rc", (uint64_t)(uint32_t)rc, "code");

        /* resource 0 */
        dcfg.resource_type = 0;
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-resource0", "rc", (uint64_t)(uint32_t)rc, "code");

        /* input_depth 0 */
        dcfg.resource_type = 1;
        dcfg.queue_depth = 0;
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-depth0", "rc", (uint64_t)(uint32_t)rc, "code");

        /* max_dpb 0 */
        dcfg.queue_depth = 4;
        dcfg.max_dpb = 0;
        rc = fn_qdec(&dcfg, &minfo);
        obs_report_measure(id, "QueryDecoderMemoryInfo-dpb0", "rc", (uint64_t)(uint32_t)rc, "code");
    }

    /* 4. CreateDecoder & DeleteDecoder */
    void *dec_h = NULL;
    if (obs_address_is_callable((const void *)fn_create)) {
        dcfg.resource_type = 1;
        dcfg.codec_type = 1;
        dcfg.profile = 100;
        dcfg.level = 41;
        dcfg.max_width = 320;
        dcfg.max_height = 240;
        dcfg.max_dpb = -1;
        dcfg.queue_depth = 4;
        dcfg.compute_queue = 0;

        /* compute_queue 0 -> error */
        int rc_q0 = fn_create(&dcfg, &minfo, &dec_h);
        obs_report_measure(id, "CreateDecoder-queue0", "rc", (uint64_t)(uint32_t)rc_q0, "code");

        /* Real compute queue */
        dcfg.compute_queue = (uint64_t)(uintptr_t)queue_handle;
        if (obs_address_is_callable((const void *)fn_qdec)) {
            memset(&minfo, 0, sizeof(minfo));
            minfo.this_size = (uint32_t)sizeof(minfo);
            (void)fn_qdec(&dcfg, &minfo);
        }

        static uint8_t mem_cpu[0x40000] __attribute__((aligned(256)));
        static uint8_t mem_gpu[0x40000] __attribute__((aligned(256)));
        static uint8_t mem_sh[0x40000] __attribute__((aligned(256)));
        minfo.cpu_memory = mem_cpu;
        minfo.gpu_memory = mem_gpu;
        minfo.shared_memory = mem_sh;

        int rc_create = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_create = fn_create(&dcfg, &minfo, &dec_h);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "CreateDecoder-normal", "rc", (uint64_t)(uint32_t)rc_create, "code");
        obs_report_measure(id, "CreateDecoder-normal", "handle", (uint64_t)(uintptr_t)dec_h, "addr");

        /* 5. Decode AVC 320x240 frame */
        if (obs_address_is_callable((const void *)fn_decode) && dec_h != NULL) {
            static uint8_t frame_buf[320 * 256 * 2] __attribute__((aligned(256)));
            SceVideodec2InputData in_data;
            in_data.au_data = s_h264_sample_320x240;
            in_data.pad = 0;
            in_data.au_size = 900; /* Frame 1 IDR */
            in_data.pts = 0;
            in_data.dts = 0;
            in_data.attached_data = NULL;

            SceVideodec2OutputInfo out_info;
            memset(&out_info, 0xcc, sizeof(out_info));
            out_info.this_size = 56;
            out_info.frame_buffer = frame_buf;
            out_info.size = sizeof(frame_buf);

            uint8_t pic_info[64];
            memset(pic_info, 0xcc, sizeof(pic_info));

            int rc_dec = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_dec = fn_decode(dec_h, &in_data, &out_info, pic_info);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "decode-frame1-idr", "rc", (uint64_t)(uint32_t)rc_dec, "code");
            obs_report_bytes(id, "decode-frame1-idr", "output-info-56", 0,
                             (const unsigned char *)&out_info, 56);
            obs_report_bytes(id, "decode-frame1-idr", "picture-info-64", 0, pic_info, sizeof(pic_info));
            obs_report_bytes(id, "decode-frame1-idr", "frame-first-64", 0, frame_buf, 64);

            /* Flush and Reset */
            if (obs_address_is_callable((const void *)fn_flush)) {
                int rc_flush = fn_flush(dec_h, &out_info, pic_info);
                obs_report_measure(id, "flush", "rc", (uint64_t)(uint32_t)rc_flush, "code");
            }
            if (obs_address_is_callable((const void *)fn_reset)) {
                int rc_reset = fn_reset(dec_h);
                obs_report_measure(id, "reset", "rc", (uint64_t)(uint32_t)rc_reset, "code");
            }
        }

        /* DeleteDecoder */
        if (obs_address_is_callable((const void *)fn_delete) && dec_h != NULL) {
            int rc_del1 = fn_delete(dec_h);
            obs_report_measure(id, "delete-first", "rc", (uint64_t)(uint32_t)rc_del1, "code");
            int rc_del2 = fn_delete(dec_h);
            obs_report_measure(id, "delete-second", "rc", (uint64_t)(uint32_t)rc_del2, "code");
        }
    }

    /* Release compute queue */
    if (obs_address_is_callable((const void *)fn_rel_q) && queue_handle != NULL && (uintptr_t)queue_handle != 0xccccccccccccccccULL) {
        int rc_rel = fn_rel_q(queue_handle);
        obs_report_measure(id, "ReleaseComputeQueue-allocated", "rc", (uint64_t)(uint32_t)rc_rel, "code");
    }

    /* 6. Vdecsw twins */
    if (obs_address_is_callable((const void *)fn_vdecsw_qcomp)) {
        SceVideodec2ComputeMemoryInfo sw_cinfo;
        memset(&sw_cinfo, 0xcc, sizeof(sw_cinfo));
        sw_cinfo.this_size = 0x18;
        int rc_sw_c = fn_vdecsw_qcomp(&sw_cinfo);
        obs_report_measure(id, "Vdecsw-QueryComputeMemoryInfo", "rc", (uint64_t)(uint32_t)rc_sw_c, "code");
        obs_report_bytes(id, "Vdecsw-QueryComputeMemoryInfo", "bytes-0x18", 0,
                         (const unsigned char *)&sw_cinfo, (unsigned int)sizeof(sw_cinfo));
    }
    if (obs_address_is_callable((const void *)fn_vdecsw_qdec)) {
        SceVdecswDecoderConfig sw_dcfg;
        memset(&sw_dcfg, 0, sizeof(sw_dcfg));
        sw_dcfg.this_size = 0x50;
        sw_dcfg.resource_type = 1;
        sw_dcfg.codec_type = 1;
        sw_dcfg.max_width = 1920;
        sw_dcfg.max_height = 1088;
        sw_dcfg.max_dpb = -1;
        sw_dcfg.queue_depth = 4;
        sw_dcfg.trailing_qword = 0;

        SceVideodec2DecoderMemoryInfo sw_minfo;
        memset(&sw_minfo, 0xcc, sizeof(sw_minfo));
        sw_minfo.this_size = 0x48;

        int rc_sw_d = fn_vdecsw_qdec(&sw_dcfg, &sw_minfo);
        obs_report_measure(id, "Vdecsw-QueryDecoderMemoryInfo", "rc", (uint64_t)(uint32_t)rc_sw_d, "code");
        obs_report_bytes(id, "Vdecsw-QueryDecoderMemoryInfo", "bytes-0x48", 0,
                         (const unsigned char *)&sw_minfo, (unsigned int)sizeof(sw_minfo));

        /* Size != 0x50 -> STRUCT */
        sw_dcfg.this_size = 0x48;
        int rc_sw_bad = fn_vdecsw_qdec(&sw_dcfg, &sw_minfo);
        obs_report_measure(id, "Vdecsw-QueryDecoderMemoryInfo-size0x48", "rc", (uint64_t)(uint32_t)rc_sw_bad, "code");
    }

    return obs_pass();
}

#endif /* OBS_VIDEODEC_ORACLE_INC */

