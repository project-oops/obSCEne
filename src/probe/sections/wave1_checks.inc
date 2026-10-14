/*
 * Wave 1 Hardware Probe Checks:
 *   - libScePngDec & libScePngEnc (REQ-20261012T1818Z-2bfa & REQ-20261012T1822Z-8912)
 *   - libSceSaveDataDialog.native (REQ-20261012T1818Z-570e & REQ-20261012T1822Z-cb67)
 *   - System Dialogs: CommonDialog, ImeDialog, MsgDialog, ErrorDialog, WebBrowser, PlayerInvitation
 *     (REQ-20261012T1818Z-6f24, REQ-20261012T1816Z-b874, REQ-20261012T1822Z-bc38)
 *   - libSceUlt (REQ-20261012T2016Z-5df8)
 */

#ifndef OBS_WAVE1_CHECKS_INC
#define OBS_WAVE1_CHECKS_INC

/* Minimal Valid Sample PNGs */
static const uint8_t s_png_sample_rgba[75] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00,
    0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
    0x1f, 0x0c, 0x81, 0x34, 0x18, 0x00, 0x00, 0x49, 0xc8, 0x09, 0xf7, 0xf9,
    0xab, 0xb6, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82,
};

static const uint8_t s_png_sample_rgb[75] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x02, 0x00, 0x00, 0x00, 0xfd, 0xd4, 0x9a, 0x73, 0x00, 0x00, 0x00,
    0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xc0,
    0x00, 0xc2, 0x0c, 0xff, 0x81, 0x00, 0x00, 0x1f, 0xee, 0x05, 0xfb, 0x0b,
    0xd9, 0x68, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82,
};

static const uint8_t s_png_sample_interlaced[76] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x06, 0x00, 0x00, 0x01, 0x05, 0xb1, 0x3d, 0xb2, 0x00, 0x00, 0x00,
    0x13, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0x00,
    0x46, 0x40, 0x82, 0xe1, 0x3f, 0x18, 0x00, 0x00, 0x4b, 0xc7, 0x09, 0xf7,
    0x41, 0xa9, 0xbf, 0x18, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
    0xae, 0x42, 0x60, 0x82,
};

static const uint8_t s_png_sample_clut_trns[111] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x03, 0x00, 0x00, 0x00, 0x45, 0x68, 0xfd, 0x16, 0x00, 0x00, 0x00,
    0x0c, 0x50, 0x4c, 0x54, 0x45, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
    0x00, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x60, 0xf6, 0x00, 0x00, 0x00,
    0x04, 0x74, 0x52, 0x4e, 0x53, 0xff, 0x80, 0x40, 0x00, 0x7c, 0xda, 0x34,
    0xee, 0x00, 0x00, 0x00, 0x0e, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63,
    0x60, 0x60, 0x64, 0x60, 0x62, 0x06, 0x00, 0x00, 0x11, 0x00, 0x07, 0x9e,
    0xa2, 0x2a, 0x12, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82,
};

static const uint8_t s_png_sample_16bit[73] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x10, 0x06, 0x00, 0x00, 0x00, 0x4f, 0x85, 0x18, 0xca, 0x00, 0x00, 0x00,
    0x10, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xff, 0x9f, 0x01,
    0x08, 0xfe, 0xff, 0x07, 0x00, 0x11, 0xf7, 0x03, 0xfd, 0xa4, 0x44, 0xa9,
    0xc4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60,
    0x82,
};

/* ---- 1. PngDec & PngEnc --------------------------------------------------- */
typedef struct {
    uint32_t this_size;
    uint32_t attribute;
    uint32_t max_image_width;
} ScePngDecCreateParam;

typedef struct {
    const void *png_addr;
    uint32_t png_size;
    uint32_t reserved;
} ScePngDecParseParam;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t color_space;
    uint16_t bit_depth;
    uint32_t image_flag;
} ScePngDecImageInfo;

typedef struct {
    const void *png_addr;
    void *image_addr;
    uint32_t png_size;
    uint32_t image_size;
    uint16_t pixel_format;
    uint16_t alpha_value;
    uint32_t image_pitch;
} ScePngDecDecodeParam;

typedef struct {
    uint32_t this_size;
    uint32_t attribute;
    uint32_t max_image_width;
    uint32_t max_filter;
} ScePngEncCreateParam;

typedef struct {
    const void *image_addr;
    void *png_addr;
    uint32_t image_size;
    uint32_t png_size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint16_t pixel_format;
    uint16_t color_space;
    uint16_t bit_depth;
    uint16_t clut_number;
    uint16_t filter_type;
    uint16_t compression_level;
} ScePngEncEncodeParam;

typedef struct {
    uint32_t png_size;
    uint32_t reserved;
} ScePngEncOutputInfo;

static obs_result check_png_dec_enc(void) {
    const char *id = "000-boot/png-dec-enc";
    obs_jmp_buf guard;

    (void)obs_sysmodule_load_id(0x008c);
    (void)obs_sysmodule_load_id(0x008d);

    int h_dec = obs_module_open("libScePngDec");
    int h_enc = obs_module_open("libScePngEnc");

    typedef int (*pngdec_create_fn)(const ScePngDecCreateParam *, void *, size_t, uint32_t *);
    typedef int (*pngdec_decode_fn)(uint32_t, const ScePngDecDecodeParam *, ScePngDecImageInfo *);
    typedef int (*pngdec_delete_fn)(uint32_t);
    typedef int (*pngdec_parse_fn)(const ScePngDecParseParam *, ScePngDecImageInfo *);
    typedef int (*pngdec_query_fn)(const ScePngDecCreateParam *, size_t *);

    typedef int (*pngenc_create_fn)(const ScePngEncCreateParam *, void *, size_t, uint32_t *);
    typedef int (*pngenc_delete_fn)(uint32_t);
    typedef int (*pngenc_encode_fn)(uint32_t, const ScePngEncEncodeParam *, ScePngEncOutputInfo *);
    typedef int (*pngenc_query_fn)(const ScePngEncCreateParam *, size_t *);

    pngdec_create_fn fn_dec_create = (pngdec_create_fn)obs_module_symbol(h_dec, "scePngDecCreate");
    if (!fn_dec_create && obs_address_is_callable((const void *)&scePngDecCreate_static)) fn_dec_create = (pngdec_create_fn)&scePngDecCreate_static;
    pngdec_decode_fn fn_dec_decode = (pngdec_decode_fn)obs_module_symbol(h_dec, "scePngDecDecode");
    if (!fn_dec_decode && obs_address_is_callable((const void *)&scePngDecDecode_static)) fn_dec_decode = (pngdec_decode_fn)&scePngDecDecode_static;
    pngdec_delete_fn fn_dec_delete = (pngdec_delete_fn)obs_module_symbol(h_dec, "scePngDecDelete");
    if (!fn_dec_delete && obs_address_is_callable((const void *)&scePngDecDelete_static)) fn_dec_delete = (pngdec_delete_fn)&scePngDecDelete_static;
    pngdec_parse_fn fn_dec_parse = (pngdec_parse_fn)obs_module_symbol(h_dec, "scePngDecParseHeader");
    if (!fn_dec_parse && obs_address_is_callable((const void *)&scePngDecParseHeader_static)) fn_dec_parse = (pngdec_parse_fn)&scePngDecParseHeader_static;
    pngdec_query_fn fn_dec_query = (pngdec_query_fn)obs_module_symbol(h_dec, "scePngDecQueryMemorySize");
    if (!fn_dec_query && obs_address_is_callable((const void *)&scePngDecQueryMemorySize_static)) fn_dec_query = (pngdec_query_fn)&scePngDecQueryMemorySize_static;

    pngenc_create_fn fn_enc_create = (pngenc_create_fn)obs_module_symbol(h_enc, "scePngEncCreate");
    if (!fn_enc_create && obs_address_is_callable((const void *)&scePngEncCreate_static)) fn_enc_create = (pngenc_create_fn)&scePngEncCreate_static;
    pngenc_delete_fn fn_enc_delete = (pngenc_delete_fn)obs_module_symbol(h_enc, "scePngEncDelete");
    if (!fn_enc_delete && obs_address_is_callable((const void *)&scePngEncDelete_static)) fn_enc_delete = (pngenc_delete_fn)&scePngEncDelete_static;
    pngenc_encode_fn fn_enc_encode = (pngenc_encode_fn)obs_module_symbol(h_enc, "scePngEncEncode");
    if (!fn_enc_encode && obs_address_is_callable((const void *)&scePngEncEncode_static)) fn_enc_encode = (pngenc_encode_fn)&scePngEncEncode_static;
    pngenc_query_fn fn_enc_query = (pngenc_query_fn)obs_module_symbol(h_enc, "scePngEncQueryMemorySize");
    if (!fn_enc_query && obs_address_is_callable((const void *)&scePngEncQueryMemorySize_static)) fn_enc_query = (pngenc_query_fn)&scePngEncQueryMemorySize_static;

    obs_report_measure(id, "PngDec", "resolved-create", (uint64_t)obs_address_is_callable((const void *)fn_dec_create), "bool");
    obs_report_measure(id, "PngDec", "resolved-decode", (uint64_t)obs_address_is_callable((const void *)fn_dec_decode), "bool");
    obs_report_measure(id, "PngDec", "resolved-delete", (uint64_t)obs_address_is_callable((const void *)fn_dec_delete), "bool");
    obs_report_measure(id, "PngDec", "resolved-parse", (uint64_t)obs_address_is_callable((const void *)fn_dec_parse), "bool");
    obs_report_measure(id, "PngDec", "resolved-query", (uint64_t)obs_address_is_callable((const void *)fn_dec_query), "bool");

    obs_report_measure(id, "PngEnc", "resolved-create", (uint64_t)obs_address_is_callable((const void *)fn_enc_create), "bool");
    obs_report_measure(id, "PngEnc", "resolved-delete", (uint64_t)obs_address_is_callable((const void *)fn_enc_delete), "bool");
    obs_report_measure(id, "PngEnc", "resolved-encode", (uint64_t)obs_address_is_callable((const void *)fn_enc_encode), "bool");
    obs_report_measure(id, "PngEnc", "resolved-query", (uint64_t)obs_address_is_callable((const void *)fn_enc_query), "bool");

    if (obs_address_is_callable((const void *)fn_dec_query)) {
        static const uint32_t widths[] = {0, 1, 8, 4096, 1000000, 1000001};
        static const uint32_t attrs[] = {0, 1, 2};
        for (size_t a = 0; a < OBS_COUNT(attrs); a++) {
            for (size_t w = 0; w < OBS_COUNT(widths); w++) {
                ScePngDecCreateParam p;
                p.this_size = (uint32_t)sizeof(p);
                p.attribute = attrs[a];
                p.max_image_width = widths[w];
                size_t mem_sz = 0xcccccccc;
                int rc = -1;
                int sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc = fn_dec_query(&p, &mem_sz);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                char tag[64];
                snprintf(tag, sizeof(tag), "query-attr%u-w%u", attrs[a], widths[w]);
                obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
                obs_report_measure(id, tag, "mem-size", (uint64_t)mem_sz, "bytes");
            }
        }
        /* Bad this_size = 16 */
        ScePngDecCreateParam p_bad;
        p_bad.this_size = 16;
        p_bad.attribute = 0;
        p_bad.max_image_width = 64;
        size_t mem_bad = 0;
        int rc_bad = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad = fn_dec_query(&p_bad, &mem_bad);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "query-bad-this-size-16", "rc", (uint64_t)(uint32_t)rc_bad, "code");
    }

    /* PngDec Create & Delete */
    static uint8_t dec_mem[128 * 1024] __attribute__((aligned(64)));
    uint32_t dec_handle = 0xffffffff;
    size_t normal_mem_sz = 0;
    if (obs_address_is_callable((const void *)fn_dec_query)) {
        ScePngDecCreateParam p;
        p.this_size = 12;
        p.attribute = 0;
        p.max_image_width = 64;
        (void)fn_dec_query(&p, &normal_mem_sz);
    }
    if (normal_mem_sz == 0 || normal_mem_sz > sizeof(dec_mem)) {
        normal_mem_sz = 0xD490;
    }

    if (obs_address_is_callable((const void *)fn_dec_create)) {
        ScePngDecCreateParam p;
        p.this_size = 12;
        p.attribute = 0;
        p.max_image_width = 64;

        /* Create with 1 byte less */
        uint32_t h_short = 0xffffffff;
        int rc_short = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_short = fn_dec_create(&p, dec_mem, normal_mem_sz - 1, &h_short);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "create-mem-short", "rc", (uint64_t)(uint32_t)rc_short, "code");

        /* Create normal */
        memset(dec_mem, 0xcc, sizeof(dec_mem));
        int rc_create = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_create = fn_dec_create(&p, dec_mem, normal_mem_sz, &dec_handle);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "create-normal", "rc", (uint64_t)(uint32_t)rc_create, "code");
        obs_report_measure(id, "create-normal", "handle", (uint64_t)dec_handle, "handle");
        obs_report_measure(id, "create-normal", "handle-rel-mem", (uint64_t)((uintptr_t)dec_handle - (uintptr_t)dec_mem), "offset");
        obs_report_bytes(id, "create-normal", "mem-first-18h", 0, dec_mem, 0x18);
    }

    /* PngDec ParseHeader */
    if (obs_address_is_callable((const void *)fn_dec_parse)) {
        /* reserved != 0 */
        ScePngDecParseParam pp;
        ScePngDecImageInfo info;
        pp.png_addr = s_png_sample_rgba;
        pp.png_size = sizeof(s_png_sample_rgba);
        pp.reserved = 1;
        memset(&info, 0xcc, sizeof(info));
        int rc = fn_dec_parse(&pp, &info);
        obs_report_measure(id, "parse-reserved-nonzero", "rc", (uint64_t)(uint32_t)rc, "code");

        /* size < 8 */
        pp.png_size = 7;
        pp.reserved = 0;
        rc = fn_dec_parse(&pp, &info);
        obs_report_measure(id, "parse-size-short-7", "rc", (uint64_t)(uint32_t)rc, "code");

        /* Corrupt CRC */
        uint8_t corrupt_png[sizeof(s_png_sample_rgba)];
        memcpy(corrupt_png, s_png_sample_rgba, sizeof(corrupt_png));
        corrupt_png[29] ^= 0xff;
        pp.png_addr = corrupt_png;
        pp.png_size = sizeof(corrupt_png);
        rc = fn_dec_parse(&pp, &info);
        obs_report_measure(id, "parse-corrupt-crc", "rc", (uint64_t)(uint32_t)rc, "code");

        /* Valid samples */
        static const struct {
            const char *name;
            const uint8_t *data;
            size_t size;
        } samples[] = {
            {"rgba", s_png_sample_rgba, sizeof(s_png_sample_rgba)},
            {"rgb", s_png_sample_rgb, sizeof(s_png_sample_rgb)},
            {"interlaced", s_png_sample_interlaced, sizeof(s_png_sample_interlaced)},
            {"clut-trns", s_png_sample_clut_trns, sizeof(s_png_sample_clut_trns)},
            {"16bit", s_png_sample_16bit, sizeof(s_png_sample_16bit)},
        };

        for (size_t s = 0; s < OBS_COUNT(samples); s++) {
            pp.png_addr = samples[s].data;
            pp.png_size = (uint32_t)samples[s].size;
            pp.reserved = 0;
            memset(&info, 0xcc, sizeof(info));
            rc = -1;
            int sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc = fn_dec_parse(&pp, &info);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            char tag[64];
            snprintf(tag, sizeof(tag), "parse-%s", samples[s].name);
            obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
            obs_report_measure(id, tag, "width", (uint64_t)info.width, "px");
            obs_report_measure(id, tag, "height", (uint64_t)info.height, "px");
            obs_report_measure(id, tag, "color-space", (uint64_t)info.color_space, "enum");
            obs_report_measure(id, tag, "bit-depth", (uint64_t)info.bit_depth, "bits");
            obs_report_measure(id, tag, "image-flag", (uint64_t)info.image_flag, "flags");
        }
    }

    /* PngDec Decode */
    if (obs_address_is_callable((const void *)fn_dec_decode) && dec_handle != 0xffffffff) {
        static uint8_t out_pixels[64 * 64 * 4];
        ScePngDecDecodeParam dp;
        ScePngDecImageInfo info;

        /* Decode RGBA to R8G8B8A8 (format 0) */
        memset(out_pixels, 0xcc, sizeof(out_pixels));
        memset(&info, 0xcc, sizeof(info));
        dp.png_addr = s_png_sample_rgba;
        dp.image_addr = out_pixels;
        dp.png_size = sizeof(s_png_sample_rgba);
        dp.image_size = sizeof(out_pixels);
        dp.pixel_format = 0;
        dp.alpha_value = 0x80;
        dp.image_pitch = 64 * 4;

        int rc = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-rgba-format0", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_measure(id, "decode-rgba-format0", "width", (uint64_t)info.width, "px");
        obs_report_measure(id, "decode-rgba-format0", "height", (uint64_t)info.height, "px");
        obs_report_bytes(id, "decode-rgba-format0", "first-8-pixels", 0, out_pixels, 32);

        /* Decode RGBA to B8G8R8A8 (format 1) */
        memset(out_pixels, 0xcc, sizeof(out_pixels));
        dp.pixel_format = 1;
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-rgba-format1", "rc", (uint64_t)(uint32_t)rc, "code");
        obs_report_bytes(id, "decode-rgba-format1", "first-8-pixels", 0, out_pixels, 32);

        /* Decode with too small output buffer */
        dp.pixel_format = 0;
        dp.image_size = 1;
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-small-buffer", "rc", (uint64_t)(uint32_t)rc, "code");

        /* Decode with too small pitch */
        dp.image_size = sizeof(out_pixels);
        dp.image_pitch = 1;
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-small-pitch", "rc", (uint64_t)(uint32_t)rc, "code");

        /* 16-bit PNG on attribute-0 decoder */
        dp.png_addr = s_png_sample_16bit;
        dp.png_size = sizeof(s_png_sample_16bit);
        dp.image_pitch = 64 * 4;
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-16bit-on-attr0", "rc", (uint64_t)(uint32_t)rc, "code");

        /* Format 0x84 (CLUT) */
        dp.png_addr = s_png_sample_rgba;
        dp.png_size = sizeof(s_png_sample_rgba);
        dp.pixel_format = 0x84;
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-format-0x84", "rc", (uint64_t)(uint32_t)rc, "code");

        /* Handle off by 8 */
        rc = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc = fn_dec_decode(dec_handle + 8, &dp, &info);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "decode-handle-off-by-8", "rc", (uint64_t)(uint32_t)rc, "code");
    }

    /* PngDec Delete tests */
    if (obs_address_is_callable((const void *)fn_dec_delete) && dec_handle != 0xffffffff) {
        int rc = fn_dec_delete(dec_handle);
        obs_report_measure(id, "delete-first", "rc", (uint64_t)(uint32_t)rc, "code");

        int rc2 = fn_dec_delete(dec_handle);
        obs_report_measure(id, "delete-second", "rc", (uint64_t)(uint32_t)rc2, "code");

        int rc_bad = fn_dec_delete(0x12345678);
        obs_report_measure(id, "delete-bad-handle", "rc", (uint64_t)(uint32_t)rc_bad, "code");
    }

    /* PngEnc Tests */
    if (obs_address_is_callable((const void *)fn_enc_query)) {
        /* Arm 1: QueryMemorySize for max widths 1, 4, 64, 4096 and filters 0..4 */
        static const uint32_t q_widths[] = {1, 4, 64, 4096};
        static const uint32_t q_filters[] = {0, 1, 2, 3, 4};
        for (size_t w = 0; w < OBS_COUNT(q_widths); w++) {
            for (size_t f = 0; f < OBS_COUNT(q_filters); f++) {
                ScePngEncCreateParam qp;
                memset(&qp, 0, sizeof(qp));
                qp.this_size = 16;
                qp.attribute = 0;
                qp.max_image_width = q_widths[w];
                qp.max_filter = q_filters[f];
                size_t q_mem = 0;
                int q_rc = -1;
                int sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    q_rc = fn_enc_query(&qp, &q_mem);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                char qtag[64];
                snprintf(qtag, sizeof(qtag), "query-w%u-f%u", q_widths[w], q_filters[f]);
                obs_report_measure(id, qtag, "rc", (uint64_t)(uint32_t)q_rc, "code");
                obs_report_measure(id, qtag, "mem-size", (uint64_t)q_mem, "bytes");
            }
        }

        /* Arm 2: Create attempts varying this_size, attribute, max width */
        static _Alignas(64) uint8_t enc_mem[512 * 1024];
        static const uint32_t try_sizes[] = {12, 16, 24, 32};
        static const uint32_t try_attrs[] = {0, 1};
        uint32_t valid_enc_h = 0xffffffff;

        if (obs_address_is_callable((const void *)fn_enc_create)) {
            for (size_t s = 0; s < OBS_COUNT(try_sizes); s++) {
                for (size_t a = 0; a < OBS_COUNT(try_attrs); a++) {
                    ScePngEncCreateParam cp;
                    memset(&cp, 0, sizeof(cp));
                    cp.this_size = try_sizes[s];
                    cp.attribute = try_attrs[a];
                    cp.max_image_width = 64;
                    cp.max_filter = 4;
                    uint32_t h = 0xffffffff;
                    memset(enc_mem, 0xcc, sizeof(enc_mem));
                    int rc = -1;
                    int sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc = fn_enc_create(&cp, enc_mem, sizeof(enc_mem), &h);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    char ctag[64];
                    snprintf(ctag, sizeof(ctag), "create-sz%u-attr%u", try_sizes[s], try_attrs[a]);
                    obs_report_measure(id, ctag, "rc", (uint64_t)(uint32_t)rc, "code");
                    obs_report_measure(id, ctag, "handle", (uint64_t)h, "handle");
                    if (rc == 0) {
                        obs_report_bytes(id, ctag, "context-first-64", 0, enc_mem, 64);
                        if (obs_address_is_callable((const void *)fn_enc_delete)) {
                            fn_enc_delete(h);
                        }
                    }
                }
            }

            /* Dedicated create for Arm 3 & Arm 4 with clean buffer */
            memset(enc_mem, 0, sizeof(enc_mem));
            ScePngEncCreateParam cp_act;
            memset(&cp_act, 0, sizeof(cp_act));
            cp_act.this_size = 16;
            cp_act.attribute = 0;
            cp_act.max_image_width = 64;
            cp_act.max_filter = 4;
            (void)fn_enc_create(&cp_act, enc_mem, sizeof(enc_mem), &valid_enc_h);

            /* Arm 3: Encode on 4x4 RGBA image with valid encoder */
            if (obs_address_is_callable((const void *)fn_enc_encode) && valid_enc_h != 0xffffffff) {
                static uint8_t raw_4x4[4 * 4 * 4];
                static uint8_t enc_out[4096];
                for (size_t i = 0; i < sizeof(raw_4x4); i++) {
                    raw_4x4[i] = (uint8_t)(i * 7);
                }

                static const uint16_t compressions[] = {0, 9};
                static const uint16_t filters[] = {1, 2, 4, 8, 15};

                for (size_t c = 0; c < OBS_COUNT(compressions); c++) {
                    for (size_t f = 0; f < OBS_COUNT(filters); f++) {
                        memset(enc_out, 0xcc, sizeof(enc_out));
                        ScePngEncEncodeParam p;
                        memset(&p, 0, sizeof(p));
                        p.image_addr = raw_4x4;
                        p.png_addr = enc_out;
                        p.image_size = sizeof(raw_4x4);
                        p.png_size = sizeof(enc_out);
                        p.width = 4;
                        p.height = 4;
                        p.pitch = 4 * 4;
                        p.pixel_format = 0;
                        p.color_space = 19; /* RGBA */
                        p.bit_depth = 8;
                        p.clut_number = 0;
                        p.filter_type = filters[f];
                        p.compression_level = compressions[c];

                        ScePngEncOutputInfo out_info;
                        memset(&out_info, 0xcc, sizeof(out_info));
                        int rc = -1;
                        int sig = OBS_FAULT_ARM(&guard);
                        if (sig == 0) {
                            rc = fn_enc_encode(valid_enc_h, &p, &out_info);
                            obs_fault_unregister();
                        } else {
                            obs_fault_unregister();
                        }
                        char tag[64];
                        snprintf(tag, sizeof(tag), "encode-c%u-f%u", compressions[c], filters[f]);
                        obs_report_measure(id, tag, "rc", (uint64_t)(uint32_t)rc, "code");
                        obs_report_measure(id, tag, "out-size", (uint64_t)out_info.png_size, "bytes");
                        obs_report_bytes(id, tag, "out-info", 0, (const unsigned char *)&out_info, sizeof(out_info));
                        if (out_info.png_size > 0 && out_info.png_size <= sizeof(enc_out)) {
                            obs_report_bytes(id, tag, "png-first-64", 0, enc_out,
                                             out_info.png_size > 64 ? 64 : out_info.png_size);
                        }
                    }
                }

                /* Pitch below width x 4 */
                ScePngEncEncodeParam p_pitch;
                memset(&p_pitch, 0, sizeof(p_pitch));
                p_pitch.image_addr = raw_4x4;
                p_pitch.png_addr = enc_out;
                p_pitch.image_size = sizeof(raw_4x4);
                p_pitch.png_size = sizeof(enc_out);
                p_pitch.width = 4;
                p_pitch.height = 4;
                p_pitch.pitch = 8; /* below 4*4 */
                p_pitch.pixel_format = 0;
                p_pitch.color_space = 19;
                p_pitch.bit_depth = 8;
                p_pitch.filter_type = 1;
                p_pitch.compression_level = 0;
                ScePngEncOutputInfo info_pitch;
                memset(&info_pitch, 0xcc, sizeof(info_pitch));
                int rc_pitch = -1;
                int sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_pitch = fn_enc_encode(valid_enc_h, &p_pitch, &info_pitch);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "encode-small-pitch", "rc", (uint64_t)(uint32_t)rc_pitch, "code");

                /* Unaligned image address */
                ScePngEncEncodeParam p_unaligned;
                memset(&p_unaligned, 0, sizeof(p_unaligned));
                p_unaligned.image_addr = raw_4x4 + 1;
                p_unaligned.png_addr = enc_out;
                p_unaligned.image_size = sizeof(raw_4x4) - 1;
                p_unaligned.png_size = sizeof(enc_out);
                p_unaligned.width = 4;
                p_unaligned.height = 3;
                p_unaligned.pitch = 4 * 4;
                p_unaligned.pixel_format = 0;
                p_unaligned.color_space = 19;
                p_unaligned.bit_depth = 8;
                p_unaligned.filter_type = 1;
                p_unaligned.compression_level = 0;

                ScePngEncOutputInfo info_unalign;
                memset(&info_unalign, 0xcc, sizeof(info_unalign));
                int rc_unalign = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_unalign = fn_enc_encode(valid_enc_h, &p_unaligned, &info_unalign);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "encode-unaligned-addr", "rc", (uint64_t)(uint32_t)rc_unalign, "code");

                /* Output buffer too small */
                ScePngEncEncodeParam p_small;
                memset(&p_small, 0, sizeof(p_small));
                p_small.image_addr = raw_4x4;
                p_small.png_addr = enc_out;
                p_small.image_size = sizeof(raw_4x4);
                p_small.png_size = 1; /* too small */
                p_small.width = 4;
                p_small.height = 4;
                p_small.pitch = 4 * 4;
                p_small.pixel_format = 0;
                p_small.color_space = 19;
                p_small.bit_depth = 8;
                p_small.filter_type = 1;
                p_small.compression_level = 0;

                ScePngEncOutputInfo info_small;
                memset(&info_small, 0xcc, sizeof(info_small));
                int rc_small = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_small = fn_enc_encode(valid_enc_h, &p_small, &info_small);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "encode-small-outbuf", "rc", (uint64_t)(uint32_t)rc_small, "code");
            }

            /* Arm 4: Delete once and twice */
            if (obs_address_is_callable((const void *)fn_enc_delete) && valid_enc_h != 0xffffffff) {
                int rc_del1 = fn_enc_delete(valid_enc_h);
                obs_report_measure(id, "enc-delete-first", "rc", (uint64_t)(uint32_t)rc_del1, "code");
                int rc_del2 = fn_enc_delete(valid_enc_h);
                obs_report_measure(id, "enc-delete-second", "rc", (uint64_t)(uint32_t)rc_del2, "code");
            }
        }
    }

    return obs_pass();
}

/* ---- 2. SaveDataDialog ---------------------------------------------------- */
static obs_result check_savedata_dialog(void) {
    const char *id = "000-boot/savedata-dialog";
    obs_jmp_buf guard;

    int h_fs = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_fs = sceKernelLoadStartModule("/system/priv/lib/libSceFsInternalForVsh.sprx", 0, NULL, 0, NULL, NULL);
        if (h_fs <= 1) {
            h_fs = sceKernelLoadStartModule("libSceFsInternalForVsh.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    obs_report_measure(id, "load-fs-internal", "rc", (uint64_t)(uint32_t)h_fs, "code");

    int h_sd_base = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_sd_base = sceKernelLoadStartModule("/system/common/lib/libSceSaveData.native.sprx", 0, NULL, 0, NULL, NULL);
        if (h_sd_base <= 1) {
            h_sd_base = sceKernelLoadStartModule("libSceSaveData.native.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    obs_report_measure(id, "load-sd-native-sprx", "rc", (uint64_t)(uint32_t)h_sd_base, "code");

    int h_sd1 = -1, h_sd2 = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_sd2 = sceKernelLoadStartModule("/system/common/lib/libSceSaveDataDialog.native.sprx", 0, NULL, 0, NULL, NULL);
        if (h_sd2 <= 1) {
            h_sd2 = sceKernelLoadStartModule("libSceSaveDataDialog.native.sprx", 0, NULL, 0, NULL, NULL);
        }

        h_sd1 = sceKernelLoadStartModule("/system/common/lib/libSceSaveDataDialog.sprx", 0, NULL, 0, NULL, NULL);
        if (h_sd1 <= 1) {
            h_sd1 = sceKernelLoadStartModule("libSceSaveDataDialog.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    obs_report_measure(id, "load-sdd-native-sprx", "rc", (uint64_t)(uint32_t)h_sd2, "code");
    obs_report_measure(id, "load-sdd-sprx", "rc", (uint64_t)(uint32_t)h_sd1, "code");

    int h_sd = (h_sd2 > 1) ? h_sd2 : ((h_sd1 > 1) ? h_sd1 : -1);
    if (h_sd <= 1) {
        h_sd = obs_module_open("libSceSaveDataDialog.native");
    }
    if (h_sd <= 1) {
        h_sd = obs_module_open("libSceSaveDataDialog");
    }

    typedef int (*sdd_close_fn)(void);
    typedef int (*sdd_get_result_fn)(void *);
    typedef int (*sdd_get_status_fn)(void);
    typedef int (*sdd_init_fn)(void *);
    typedef int (*sdd_ready_fn)(void);
    typedef int (*sdd_open_fn)(const void *);
    typedef int (*sdd_inc_fn)(uint32_t);
    typedef int (*sdd_set_val_fn)(uint32_t, uint32_t);
    typedef int (*sdd_term_fn)(void);
    typedef int (*sdd_update_status_fn)(void);

    sdd_close_fn fn_close = (sdd_close_fn)obs_module_symbol(h_sd, "sceSaveDataDialogClose");
    if (!fn_close && obs_address_is_callable((const void *)&sceSaveDataDialogClose_static)) fn_close = (sdd_close_fn)&sceSaveDataDialogClose_static;
    sdd_get_result_fn fn_get_result = (sdd_get_result_fn)obs_module_symbol(h_sd, "sceSaveDataDialogGetResult");
    if (!fn_get_result && obs_address_is_callable((const void *)&sceSaveDataDialogGetResult_static)) fn_get_result = (sdd_get_result_fn)&sceSaveDataDialogGetResult_static;
    sdd_get_status_fn fn_get_status = (sdd_get_status_fn)obs_module_symbol(h_sd, "sceSaveDataDialogGetStatus");
    if (!fn_get_status && obs_address_is_callable((const void *)&sceSaveDataDialogGetStatus_static)) fn_get_status = (sdd_get_status_fn)&sceSaveDataDialogGetStatus_static;
    sdd_init_fn fn_init = (sdd_init_fn)obs_module_symbol(h_sd, "sceSaveDataDialogInitialize");
    if (!fn_init && obs_address_is_callable((const void *)&sceSaveDataDialogInitialize_static)) fn_init = (sdd_init_fn)&sceSaveDataDialogInitialize_static;
    sdd_ready_fn fn_ready = (sdd_ready_fn)obs_module_symbol(h_sd, "sceSaveDataDialogIsReadyToDisplay");
    if (!fn_ready && obs_address_is_callable((const void *)&sceSaveDataDialogIsReadyToDisplay_static)) fn_ready = (sdd_ready_fn)&sceSaveDataDialogIsReadyToDisplay_static;
    sdd_open_fn fn_open = (sdd_open_fn)obs_module_symbol(h_sd, "sceSaveDataDialogOpen");
    if (!fn_open && obs_address_is_callable((const void *)&sceSaveDataDialogOpen_static)) fn_open = (sdd_open_fn)&sceSaveDataDialogOpen_static;
    sdd_inc_fn fn_inc = (sdd_inc_fn)obs_module_symbol(h_sd, "sceSaveDataDialogProgressBarInc");
    if (!fn_inc && obs_address_is_callable((const void *)&sceSaveDataDialogProgressBarInc_static)) fn_inc = (sdd_inc_fn)&sceSaveDataDialogProgressBarInc_static;
    sdd_set_val_fn fn_set_val = (sdd_set_val_fn)obs_module_symbol(h_sd, "sceSaveDataDialogProgressBarSetValue");
    if (!fn_set_val && obs_address_is_callable((const void *)&sceSaveDataDialogProgressBarSetValue_static)) fn_set_val = (sdd_set_val_fn)&sceSaveDataDialogProgressBarSetValue_static;
    sdd_term_fn fn_term = (sdd_term_fn)obs_module_symbol(h_sd, "sceSaveDataDialogTerminate");
    if (!fn_term && obs_address_is_callable((const void *)&sceSaveDataDialogTerminate_static)) fn_term = (sdd_term_fn)&sceSaveDataDialogTerminate_static;
    sdd_update_status_fn fn_update = (sdd_update_status_fn)obs_module_symbol(h_sd, "sceSaveDataDialogUpdateStatus");
    if (!fn_update && obs_address_is_callable((const void *)&sceSaveDataDialogUpdateStatus_static)) fn_update = (sdd_update_status_fn)&sceSaveDataDialogUpdateStatus_static;

    uintptr_t base_sdd = obs_find_loaded_module_base("libSceSaveDataDialog.native");
    int sdd_is_native = 1;
    if (base_sdd == 0) {
        base_sdd = obs_find_loaded_module_base("libSceSaveDataDialog");
        sdd_is_native = 0;
    }
    if (base_sdd != 0) {
        if (sdd_is_native) {
            if (!fn_close) fn_close = (sdd_close_fn)(base_sdd + 0xeb00);
            if (!fn_get_result) fn_get_result = (sdd_get_result_fn)(base_sdd + 0xe900);
            if (!fn_get_status) fn_get_status = (sdd_get_status_fn)(base_sdd + 0xe830);
            if (!fn_init) fn_init = (sdd_init_fn)(base_sdd + 0xe3a0);
            if (!fn_ready) fn_ready = (sdd_ready_fn)(base_sdd + 0xefe0);
            if (!fn_open) fn_open = (sdd_open_fn)(base_sdd + 0xe5d0);
            if (!fn_inc) fn_inc = (sdd_inc_fn)(base_sdd + 0xed40);
            if (!fn_set_val) fn_set_val = (sdd_set_val_fn)(base_sdd + 0xee90);
            if (!fn_term) fn_term = (sdd_term_fn)(base_sdd + 0xec60);
            if (!fn_update) fn_update = (sdd_update_status_fn)(base_sdd + 0xe730);
        } else {
            if (!fn_close) fn_close = (sdd_close_fn)(base_sdd + 0xe510);
            if (!fn_get_result) fn_get_result = (sdd_get_result_fn)(base_sdd + 0xe430);
            if (!fn_get_status) fn_get_status = (sdd_get_status_fn)(base_sdd + 0xe360);
            if (!fn_init) fn_init = (sdd_init_fn)(base_sdd + 0xdfb0);
            if (!fn_ready) fn_ready = (sdd_ready_fn)(base_sdd + 0xe890);
            if (!fn_open) fn_open = (sdd_open_fn)(base_sdd + 0xe160);
            if (!fn_inc) fn_inc = (sdd_inc_fn)(base_sdd + 0xe6b0);
            if (!fn_set_val) fn_set_val = (sdd_set_val_fn)(base_sdd + 0xe7a0);
            if (!fn_term) fn_term = (sdd_term_fn)(base_sdd + 0xe600);
            if (!fn_update) fn_update = (sdd_update_status_fn)(base_sdd + 0xe260);
        }
    }

    obs_report_measure(id, "SaveDataDialog", "module-handle", (uint64_t)(uint32_t)h_sd, "handle");
    obs_report_measure(id, "SaveDataDialog", "module-base", (uint64_t)base_sdd, "vaddr");
    obs_report_measure(id, "SaveDataDialog", "resolved-init", (uint64_t)obs_address_is_callable((const void *)fn_init), "bool");
    obs_report_measure(id, "SaveDataDialog", "resolved-open", (uint64_t)obs_address_is_callable((const void *)fn_open), "bool");
    obs_report_measure(id, "SaveDataDialog", "resolved-close", (uint64_t)obs_address_is_callable((const void *)fn_close), "bool");
    obs_report_measure(id, "SaveDataDialog", "resolved-getresult", (uint64_t)obs_address_is_callable((const void *)fn_get_result), "bool");

    /* Calls before Initialize */
    if (obs_address_is_callable((const void *)fn_get_status)) {
        int st_before = fn_get_status();
        obs_report_measure(id, "before-init", "get-status", (uint64_t)(uint32_t)st_before, "status");
    }
    if (obs_address_is_callable((const void *)fn_ready)) {
        int rdy_before = fn_ready();
        obs_report_measure(id, "before-init", "is-ready", (uint64_t)(uint32_t)rdy_before, "code");
    }
    if (obs_address_is_callable((const void *)fn_close)) {
        int rc_close_before = fn_close();
        obs_report_measure(id, "before-init", "close-rc", (uint64_t)(uint32_t)rc_close_before, "code");
    }
    if (obs_address_is_callable((const void *)fn_open)) {
        uint8_t pre_param[0x98];
        memset(pre_param, 0, sizeof(pre_param));
        *(uint32_t *)(pre_param + 0x30) = 0x98;
        *(uint32_t *)(pre_param + 0x34) = 1;
        int sig = OBS_FAULT_ARM(&guard);
        int rc_open_before = -1;
        if (sig == 0) {
            rc_open_before = fn_open(pre_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "before-init", "open-rc", (uint64_t)(uint32_t)rc_open_before, "code");
    }
    if (obs_address_is_callable((const void *)fn_inc)) {
        int rc_inc_before = fn_inc(0);
        obs_report_measure(id, "before-init", "progress-inc-rc", (uint64_t)(uint32_t)rc_inc_before, "code");
    }
    if (obs_address_is_callable((const void *)fn_set_val)) {
        int rc_val_before = fn_set_val(0, 50);
        obs_report_measure(id, "before-init", "progress-val-rc", (uint64_t)(uint32_t)rc_val_before, "code");
    }

    /* Initialize twice */
    if (obs_address_is_callable((const void *)fn_init)) {
        int rc_init1 = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init1 = fn_init(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "init-first", "rc", (uint64_t)(uint32_t)rc_init1, "code");

        int rc_init2 = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init2 = fn_init(NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "init-second", "rc", (uint64_t)(uint32_t)rc_init2, "code");
    }

    if (obs_address_is_callable((const void *)fn_get_status)) {
        int st_after_init = fn_get_status();
        obs_report_measure(id, "after-init", "get-status", (uint64_t)(uint32_t)st_after_init, "status");
    }
    if (obs_address_is_callable((const void *)fn_ready)) {
        int rdy_after_init = fn_ready();
        obs_report_measure(id, "after-init", "is-ready", (uint64_t)(uint32_t)rdy_after_init, "code");
    }

    /* Open with bad param size */
    if (obs_address_is_callable((const void *)fn_open)) {
        uint8_t bad_param[0x80];
        memset(bad_param, 0, sizeof(bad_param));
        *(uint32_t *)(bad_param + 0x30) = 0x80;
        int rc_bad = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_bad = fn_open(bad_param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "open-bad-size-0x80", "rc", (uint64_t)(uint32_t)rc_bad, "code");

        /* Open normal (0x98 bytes) */
        uint8_t param[0x98];
        memset(param, 0, sizeof(param));
        *(uint32_t *)(param + 0x30) = 0x98;
        *(uint32_t *)(param + 0x34) = 1; /* mode LIST */
        *(uint64_t *)(param + 0x70) = 0x12345678ULL; /* userData */

        struct {
            int32_t user_id;
            uint32_t pad;
            const char *title_id;
            const void *dir_names;
            uint32_t dir_names_num;
            uint32_t pad2;
        } items;
        memset(&items, 0, sizeof(items));
        items.user_id = 0x1ea2f4d9;
        items.title_id = "PPSA90000";
        *(uint64_t *)(param + 0x48) = (uintptr_t)&items;

        int rc_open = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_open = fn_open(param);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "open-normal-0x98", "rc", (uint64_t)(uint32_t)rc_open, "code");

        /* Poll status for up to 300 iterations */
        int prev_st = -999;
        for (int frame = 0; frame < 300; frame++) {
            int st_get = -1;
            int st_update = -1;
            if (obs_address_is_callable((const void *)fn_get_status)) {
                st_get = fn_get_status();
            }
            if (obs_address_is_callable((const void *)fn_update)) {
                st_update = fn_update();
            }
            if (frame < 5 || st_get != prev_st) {
                char tag[64];
                snprintf(tag, sizeof(tag), "frame-%d", frame);
                obs_report_measure(id, tag, "get-status", (uint64_t)(uint32_t)st_get, "status");
                obs_report_measure(id, tag, "update-status", (uint64_t)(uint32_t)st_update, "status");
                prev_st = st_get;
            }
            if (st_get == 3 || st_update == 3) {
                break;
            }
        }

        /* Progress calls */
        if (obs_address_is_callable((const void *)fn_inc)) {
            int rc_inc = fn_inc(0);
            obs_report_measure(id, "during-dialog", "progress-inc-rc", (uint64_t)(uint32_t)rc_inc, "code");
        }
        if (obs_address_is_callable((const void *)fn_set_val)) {
            int rc_val = fn_set_val(0, 50);
            obs_report_measure(id, "during-dialog", "progress-val-rc", (uint64_t)(uint32_t)rc_val, "code");
        }

        /* GetResult (0x48 bytes filled 0xcc, with pointers at +0x10 and +0x18) */
        if (obs_address_is_callable((const void *)fn_get_result)) {
            uint8_t res[0x48];
            memset(res, 0xcc, sizeof(res));
            *(uint64_t *)(res + 0x10) = 0x1111111122222222ULL;
            *(uint64_t *)(res + 0x18) = 0x3333333344444444ULL;

            int rc_res = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_res = fn_get_result(res);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "get-result", "rc", (uint64_t)(uint32_t)rc_res, "code");
            obs_report_measure(id, "get-result", "mode-0x00", (uint64_t)*(uint32_t *)(res + 0x00), "val");
            obs_report_measure(id, "get-result", "result-0x04", (uint64_t)*(uint32_t *)(res + 0x04), "val");
            obs_report_measure(id, "get-result", "button-0x08", (uint64_t)*(uint32_t *)(res + 0x08), "val");
            obs_report_measure(id, "get-result", "ptr-0x10-survived", (uint64_t)(*(uint64_t *)(res + 0x10) == 0x1111111122222222ULL), "bool");
            obs_report_measure(id, "get-result", "ptr-0x18-survived", (uint64_t)(*(uint64_t *)(res + 0x18) == 0x3333333344444444ULL), "bool");
            obs_report_measure(id, "get-result", "userdata-0x20", (uint64_t)*(uint64_t *)(res + 0x20), "val");
            obs_report_bytes(id, "get-result", "result-0x48", 0, res, 0x48);
        }

        /* Close */
        if (obs_address_is_callable((const void *)fn_close)) {
            int rc_close = fn_close();
            obs_report_measure(id, "close", "rc", (uint64_t)(uint32_t)rc_close, "code");
        }
        if (obs_address_is_callable((const void *)fn_get_status)) {
            int st_after_close = fn_get_status();
            obs_report_measure(id, "after-close", "get-status", (uint64_t)(uint32_t)st_after_close, "status");
        }

        /* Terminate */
        if (obs_address_is_callable((const void *)fn_term)) {
            int rc_term = fn_term();
            obs_report_measure(id, "term-first", "rc", (uint64_t)(uint32_t)rc_term, "code");

            if (obs_address_is_callable((const void *)fn_get_status)) {
                int st_after_term = fn_get_status();
                obs_report_measure(id, "after-term", "get-status", (uint64_t)(uint32_t)st_after_term, "status");
            }

            int rc_term2 = fn_term();
            obs_report_measure(id, "term-second", "rc", (uint64_t)(uint32_t)rc_term2, "code");
        }
    }

    return obs_pass();
}

/* ---- 3. System Dialogs (Common, IME, Msg, Error, WebBrowser) --------------- */
static obs_result check_system_dialogs(void) {
    const char *id = "000-boot/system-dialogs";
    obs_jmp_buf guard;

    /* (1) VideoOut set-up per REQ-20261014T0110Z-md02:
     * Open a VideoOut port, register and flip one buffer as a title does before its first dialog. */
    (void)obs_display_open();
    obs_display_clear(OBS_COLOUR_GROUND);
    obs_display_flip();
    obs_report_measure(id, "video-out", "display-ready", (uint64_t)obs_display_is_ready(), "bool");
    obs_report_measure(id, "video-out", "status-code", (uint64_t)obs_display_status_code(), "code");
    int vhandle = obs_display_get_video_handle();
    if (vhandle <= 0) {
        vhandle = 0x4e100400; /* borrowed compositor handle if open refused (e.g. 0x80290009) */
    }
    obs_report_measure(id, "video-out", "handle", (uint64_t)(uint32_t)vhandle, "handle");

    /* (2) Sysmodule loads for dialogs */
    int rc_mod_ime = obs_sysmodule_load_id(0x0096);
    int rc_mod_msg = obs_sysmodule_load_id(0x00a4);
    int rc_mod_err = obs_sysmodule_load_id(0x00ac);
    obs_report_measure(id, "sysmodule-load", "ime-0x96-rc", (uint64_t)(uint32_t)rc_mod_ime, "code");
    obs_report_measure(id, "sysmodule-load", "msg-0xa4-rc", (uint64_t)(uint32_t)rc_mod_msg, "code");
    obs_report_measure(id, "sysmodule-load", "err-0xac-rc", (uint64_t)(uint32_t)rc_mod_err, "code");

    /* (3) CommonDialog Initialize */
    int h_cd = obs_module_open("libSceCommonDialog");
    typedef int (*cd_init_fn)(void);
    typedef int (*cd_is_used_fn)(void);
    cd_init_fn fn_cd_init = (cd_init_fn)obs_module_symbol(h_cd, "sceCommonDialogInitialize");
    if (!fn_cd_init && obs_address_is_callable((const void *)&sceCommonDialogInitialize_static)) fn_cd_init = (cd_init_fn)&sceCommonDialogInitialize_static;
    cd_is_used_fn fn_cd_is_used = (cd_is_used_fn)obs_module_symbol(h_cd, "sceCommonDialogIsUsed");
    if (!fn_cd_is_used && obs_address_is_callable((const void *)&sceCommonDialogIsUsed_static)) fn_cd_is_used = (cd_is_used_fn)&sceCommonDialogIsUsed_static;

    if (obs_address_is_callable((const void *)fn_cd_is_used)) {
        int u1 = fn_cd_is_used();
        obs_report_measure(id, "common-dialog", "is-used-before", (uint64_t)(uint32_t)u1, "code");
    }
    if (obs_address_is_callable((const void *)fn_cd_init)) {
        int rc1 = fn_cd_init();
        int rc2 = fn_cd_init();
        obs_report_measure(id, "common-dialog", "init-first", (uint64_t)(uint32_t)rc1, "code");
        obs_report_measure(id, "common-dialog", "init-second", (uint64_t)(uint32_t)rc2, "code");
    }
    if (obs_address_is_callable((const void *)fn_cd_is_used)) {
        int u2 = fn_cd_is_used();
        obs_report_measure(id, "common-dialog", "is-used-after", (uint64_t)(uint32_t)u2, "code");
    }

    uint64_t tsc_freq = sceKernelGetTscFrequency ? sceKernelGetTscFrequency() : 0;

    /* (4) ImeDialog with oops-sdk's layout */
    int h_ime = obs_module_open("libSceImeDialog");
    typedef int (*ime_status_fn)(void);
    typedef int (*ime_result_fn)(void *);
    typedef int (*ime_init_fn)(const void *, const void *);
    typedef int (*ime_abort_fn)(void);
    typedef int (*ime_term_fn)(void);
    typedef int (*ime_panel_size_fn)(uint32_t, void *);

    ime_status_fn fn_ime_status = (ime_status_fn)obs_module_symbol(h_ime, "sceImeDialogGetStatus");
    if (!fn_ime_status && obs_address_is_callable((const void *)&sceImeDialogGetStatus_static)) fn_ime_status = (ime_status_fn)&sceImeDialogGetStatus_static;
    ime_result_fn fn_ime_result = (ime_result_fn)obs_module_symbol(h_ime, "sceImeDialogGetResult");
    if (!fn_ime_result && obs_address_is_callable((const void *)&sceImeDialogGetResult_static)) fn_ime_result = (ime_result_fn)&sceImeDialogGetResult_static;
    ime_init_fn fn_ime_init = (ime_init_fn)obs_module_symbol(h_ime, "sceImeDialogInit");
    if (!fn_ime_init && obs_address_is_callable((const void *)&sceImeDialogInit_static)) fn_ime_init = (ime_init_fn)&sceImeDialogInit_static;
    ime_abort_fn fn_ime_abort = (ime_abort_fn)obs_module_symbol(h_ime, "sceImeDialogAbort");
    if (!fn_ime_abort && obs_address_is_callable((const void *)&sceImeDialogAbort_static)) fn_ime_abort = (ime_abort_fn)&sceImeDialogAbort_static;
    ime_term_fn fn_ime_term = (ime_term_fn)obs_module_symbol(h_ime, "sceImeDialogTerm");
    if (!fn_ime_term && obs_address_is_callable((const void *)&sceImeDialogTerm_static)) fn_ime_term = (ime_term_fn)&sceImeDialogTerm_static;
    ime_panel_size_fn fn_ime_panel = (ime_panel_size_fn)obs_module_symbol(h_ime, "sceImeDialogGetPanelSizeExtended");
    if (!fn_ime_panel && obs_address_is_callable((const void *)&sceImeDialogGetPanelSizeExtended_static)) fn_ime_panel = (ime_panel_size_fn)&sceImeDialogGetPanelSizeExtended_static;

    int32_t user = 0;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        if (sceUserServiceGetInitialUser(&user) != 0) user = 0x1ea2f4d9;
    } else {
        user = 0x1ea2f4d9;
    }

    if (obs_address_is_callable((const void *)fn_ime_init)) {
        /* SceImeDialogParam layout (96 bytes per oops-sdk dialog.c):
         * +0x00: int32_t userId
         * +0x04: int32_t type = 0
         * +0x08: uint64_t supportedLanguages = 0
         * +0x10: int32_t enterLabel = 0
         * +0x14: int32_t inputMethod = 0
         * +0x18: void *filter = NULL
         * +0x20: uint32_t option = 0
         * +0x24: uint32_t maxTextLength = 16
         * +0x28: uint16_t *inputTextBuffer
         * +0x30..0x60: zeros */
        uint8_t ime_param[96];
        memset(ime_param, 0, sizeof(ime_param));
        *(int32_t *)(ime_param + 0x00) = user;
        *(int32_t *)(ime_param + 0x04) = 0;
        *(uint32_t *)(ime_param + 0x24) = 16;
        uint16_t input_buf[17];
        memset(input_buf, 0, sizeof(input_buf));
        *(uint64_t *)(ime_param + 0x28) = (uintptr_t)input_buf;

        int rc_init = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_init = fn_ime_init(ime_param, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ime-dialog", "init-rc", (uint64_t)(uint32_t)rc_init, "code");

        /* Poll status for 1 second */
        int st_ime = -1;
        if (obs_address_is_callable((const void *)fn_ime_status)) {
            st_ime = fn_ime_status();
            obs_report_measure(id, "ime-dialog", "status-initial", (uint64_t)(uint32_t)st_ime, "status");
            if (tsc_freq > 0 && sceKernelReadTsc) {
                uint64_t t_end = sceKernelReadTsc() + tsc_freq;
                while (sceKernelReadTsc() < t_end) {
                    st_ime = fn_ime_status();
                    if (st_ime != 1 && st_ime != 0) break;
                    if (obs_address_is_callable((const void *)&sceKernelUsleep)) sceKernelUsleep(16000);
                }
            }
            obs_report_measure(id, "ime-dialog", "status-1s", (uint64_t)(uint32_t)st_ime, "status");
        }

        /* Abort */
        if (obs_address_is_callable((const void *)fn_ime_abort)) {
            int rc_abort = fn_ime_abort();
            obs_report_measure(id, "ime-dialog", "abort-rc", (uint64_t)(uint32_t)rc_abort, "code");
        }

        /* Poll status after abort */
        if (obs_address_is_callable((const void *)fn_ime_status)) {
            int st_post_abort = fn_ime_status();
            obs_report_measure(id, "ime-dialog", "status-after-abort", (uint64_t)(uint32_t)st_post_abort, "status");
        }

        /* GetResult into 0xcc block */
        if (obs_address_is_callable((const void *)fn_ime_result)) {
            uint8_t res[0x40];
            memset(res, 0xcc, sizeof(res));
            int rc_res = fn_ime_result(res);
            obs_report_measure(id, "ime-dialog", "result-post-abort-rc", (uint64_t)(uint32_t)rc_res, "code");
            obs_report_bytes(id, "ime-dialog", "result-post-abort", 0, res, sizeof(res));
        }

        /* Term */
        if (obs_address_is_callable((const void *)fn_ime_term)) {
            int rc_term = fn_ime_term();
            obs_report_measure(id, "ime-dialog", "term-rc", (uint64_t)(uint32_t)rc_term, "code");
        }
    }

    if (obs_address_is_callable((const void *)fn_ime_panel)) {
        uint32_t sz_out[4] = {0};
        int rc_p1 = fn_ime_panel(0, sz_out);
        obs_report_measure(id, "ime-dialog", "panel-size-text-rc", (uint64_t)(uint32_t)rc_p1, "code");
        obs_report_measure(id, "ime-dialog", "panel-w", (uint64_t)sz_out[0], "px");
        obs_report_measure(id, "ime-dialog", "panel-h", (uint64_t)sz_out[1], "px");
    }

    /* (5) ErrorDialog */
    int h_err = obs_module_open("libSceErrorDialog");
    typedef int (*err_init_fn)(void);
    typedef int (*err_open_fn)(const void *);
    typedef int (*err_status_fn)(void);
    typedef int (*err_update_fn)(void);
    typedef int (*err_close_fn)(void);
    typedef int (*err_term_fn)(void);

    err_init_fn fn_e_init = (err_init_fn)obs_module_symbol(h_err, "sceErrorDialogInitialize");
    if (!fn_e_init && obs_address_is_callable((const void *)&sceErrorDialogInitialize_static)) fn_e_init = (err_init_fn)&sceErrorDialogInitialize_static;
    err_open_fn fn_e_open = (err_open_fn)obs_module_symbol(h_err, "sceErrorDialogOpen");
    if (!fn_e_open && obs_address_is_callable((const void *)&sceErrorDialogOpen_static)) fn_e_open = (err_open_fn)&sceErrorDialogOpen_static;
    err_status_fn fn_e_status = (err_status_fn)obs_module_symbol(h_err, "sceErrorDialogGetStatus");
    if (!fn_e_status && obs_address_is_callable((const void *)&sceErrorDialogGetStatus_static)) fn_e_status = (err_status_fn)&sceErrorDialogGetStatus_static;
    err_update_fn fn_e_update = (err_update_fn)obs_module_symbol(h_err, "sceErrorDialogUpdateStatus");
    if (!fn_e_update && obs_address_is_callable((const void *)&sceErrorDialogUpdateStatus_static)) fn_e_update = (err_update_fn)&sceErrorDialogUpdateStatus_static;
    err_close_fn fn_e_close = (err_close_fn)obs_module_symbol(h_err, "sceErrorDialogClose");
    if (!fn_e_close && obs_address_is_callable((const void *)&sceErrorDialogClose_static)) fn_e_close = (err_close_fn)&sceErrorDialogClose_static;
    err_term_fn fn_e_term = (err_term_fn)obs_module_symbol(h_err, "sceErrorDialogTerminate");
    if (!fn_e_term && obs_address_is_callable((const void *)&sceErrorDialogTerminate_static)) fn_e_term = (err_term_fn)&sceErrorDialogTerminate_static;

    if (obs_address_is_callable((const void *)fn_e_init)) {
        int rc1 = fn_e_init();
        obs_report_measure(id, "error-dialog", "init-first", (uint64_t)(uint32_t)rc1, "code");

        if (obs_address_is_callable((const void *)fn_e_open)) {
            /* Open with {16, 0x80020001, 0, 0} */
            uint8_t ep[16];
            memset(ep, 0, sizeof(ep));
            *(uint32_t *)(ep + 0) = 16;
            *(int32_t *)(ep + 4) = (int32_t)0x80020001;
            int rc_open = fn_e_open(ep);
            obs_report_measure(id, "error-dialog", "open-normal-rc", (uint64_t)(uint32_t)rc_open, "code");

            /* GetStatus / UpdateStatus each frame for 1s */
            int last_err_st = -1;
            if (tsc_freq > 0 && sceKernelReadTsc) {
                uint64_t t_end = sceKernelReadTsc() + tsc_freq;
                while (sceKernelReadTsc() < t_end) {
                    if (fn_e_update) fn_e_update();
                    if (fn_e_status) last_err_st = fn_e_status();
                    if (obs_address_is_callable((const void *)&sceKernelUsleep)) sceKernelUsleep(16000);
                }
            }
            obs_report_measure(id, "error-dialog", "status-1s", (uint64_t)(uint32_t)last_err_st, "status");

            if (obs_address_is_callable((const void *)fn_e_close)) {
                int rc_close = fn_e_close();
                obs_report_measure(id, "error-dialog", "close-rc", (uint64_t)(uint32_t)rc_close, "code");
            }
            if (obs_address_is_callable((const void *)fn_e_status)) {
                int st_close = fn_e_status();
                obs_report_measure(id, "error-dialog", "status-post-close", (uint64_t)(uint32_t)st_close, "status");
            }
            if (obs_address_is_callable((const void *)fn_e_term)) {
                int rc_term = fn_e_term();
                obs_report_measure(id, "error-dialog", "term-rc", (uint64_t)(uint32_t)rc_term, "code");
            }
        }
    }

    /* (6) MsgDialog */
    int h_msg = obs_module_open("libSceMsgDialog");
    if (h_msg <= 1) h_msg = obs_module_open("libSceMsgDialog.native");

    typedef int (*msg_close_fn)(void);
    typedef int (*msg_get_result_fn)(void *);
    typedef int (*msg_get_status_fn)(void);
    typedef int (*msg_init_fn)(void);
    typedef int (*msg_open_fn)(const void *);
    typedef int (*msg_inc_fn)(uint32_t);
    typedef int (*msg_set_val_fn)(uint32_t, uint32_t);
    typedef int (*msg_set_msg_fn)(uint32_t, const char *);
    typedef int (*msg_term_fn)(void);
    typedef int (*msg_update_fn)(void);

    msg_init_fn fn_m_init = (msg_init_fn)obs_module_symbol(h_msg, "sceMsgDialogInitialize");
    if (!fn_m_init) fn_m_init = (msg_init_fn)obs_module_symbol(h_msg, "lDqxaY1UbEo");
    if (!fn_m_init) fn_m_init = (msg_init_fn)obs_module_symbol(h_msg, "$lDqxaY1UbEo");
    if (!fn_m_init && obs_address_is_callable((const void *)&sceMsgDialogInitialize_static)) fn_m_init = (msg_init_fn)&sceMsgDialogInitialize_static;
    if (!fn_m_init && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "lDqxaY1UbEo", (void **)&fn_m_init);

    msg_open_fn fn_m_open = (msg_open_fn)obs_module_symbol(h_msg, "sceMsgDialogOpen");
    if (!fn_m_open) fn_m_open = (msg_open_fn)obs_module_symbol(h_msg, "b06Hh0DPEaE");
    if (!fn_m_open) fn_m_open = (msg_open_fn)obs_module_symbol(h_msg, "$b06Hh0DPEaE");
    if (!fn_m_open && obs_address_is_callable((const void *)&sceMsgDialogOpen_static)) fn_m_open = (msg_open_fn)&sceMsgDialogOpen_static;
    if (!fn_m_open && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "b06Hh0DPEaE", (void **)&fn_m_open);

    msg_get_status_fn fn_m_status = (msg_get_status_fn)obs_module_symbol(h_msg, "sceMsgDialogGetStatus");
    if (!fn_m_status) fn_m_status = (msg_get_status_fn)obs_module_symbol(h_msg, "CWVW78Qc3fI");
    if (!fn_m_status) fn_m_status = (msg_get_status_fn)obs_module_symbol(h_msg, "$CWVW78Qc3fI");
    if (!fn_m_status && obs_address_is_callable((const void *)&sceMsgDialogGetStatus_static)) fn_m_status = (msg_get_status_fn)&sceMsgDialogGetStatus_static;
    if (!fn_m_status && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "CWVW78Qc3fI", (void **)&fn_m_status);

    msg_update_fn fn_m_update = (msg_update_fn)obs_module_symbol(h_msg, "sceMsgDialogUpdateStatus");
    if (!fn_m_update) fn_m_update = (msg_update_fn)obs_module_symbol(h_msg, "6fIC3XKt2k0");
    if (!fn_m_update) fn_m_update = (msg_update_fn)obs_module_symbol(h_msg, "$6fIC3XKt2k0");
    if (!fn_m_update && obs_address_is_callable((const void *)&sceMsgDialogUpdateStatus_static)) fn_m_update = (msg_update_fn)&sceMsgDialogUpdateStatus_static;
    if (!fn_m_update && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "6fIC3XKt2k0", (void **)&fn_m_update);

    msg_close_fn fn_m_close = (msg_close_fn)obs_module_symbol(h_msg, "sceMsgDialogClose");
    if (!fn_m_close) fn_m_close = (msg_close_fn)obs_module_symbol(h_msg, "HTrcDKlFKuM");
    if (!fn_m_close) fn_m_close = (msg_close_fn)obs_module_symbol(h_msg, "$HTrcDKlFKuM");
    if (!fn_m_close && obs_address_is_callable((const void *)&sceMsgDialogClose_static)) fn_m_close = (msg_close_fn)&sceMsgDialogClose_static;
    if (!fn_m_close && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "HTrcDKlFKuM", (void **)&fn_m_close);

    msg_get_result_fn fn_m_result = (msg_get_result_fn)obs_module_symbol(h_msg, "sceMsgDialogGetResult");
    if (!fn_m_result) fn_m_result = (msg_get_result_fn)obs_module_symbol(h_msg, "Lr8ovHH9l6A");
    if (!fn_m_result) fn_m_result = (msg_get_result_fn)obs_module_symbol(h_msg, "$Lr8ovHH9l6A");
    if (!fn_m_result && obs_address_is_callable((const void *)&sceMsgDialogGetResult_static)) fn_m_result = (msg_get_result_fn)&sceMsgDialogGetResult_static;
    if (!fn_m_result && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "Lr8ovHH9l6A", (void **)&fn_m_result);

    msg_term_fn fn_m_term = (msg_term_fn)obs_module_symbol(h_msg, "sceMsgDialogTerminate");
    if (!fn_m_term) fn_m_term = (msg_term_fn)obs_module_symbol(h_msg, "ePw-kqZmelo");
    if (!fn_m_term) fn_m_term = (msg_term_fn)obs_module_symbol(h_msg, "$ePw-kqZmelo");
    if (!fn_m_term && obs_address_is_callable((const void *)&sceMsgDialogTerminate_static)) fn_m_term = (msg_term_fn)&sceMsgDialogTerminate_static;
    if (!fn_m_term && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "ePw-kqZmelo", (void **)&fn_m_term);

    msg_inc_fn fn_m_inc = (msg_inc_fn)obs_module_symbol(h_msg, "sceMsgDialogProgressBarInc");
    if (!fn_m_inc) fn_m_inc = (msg_inc_fn)obs_module_symbol(h_msg, "Gc5k1qcK4fs");
    if (!fn_m_inc) fn_m_inc = (msg_inc_fn)obs_module_symbol(h_msg, "$Gc5k1qcK4fs");
    if (!fn_m_inc && obs_address_is_callable((const void *)&sceMsgDialogProgressBarInc_static)) fn_m_inc = (msg_inc_fn)&sceMsgDialogProgressBarInc_static;
    if (!fn_m_inc && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "Gc5k1qcK4fs", (void **)&fn_m_inc);

    msg_set_val_fn fn_m_val = (msg_set_val_fn)obs_module_symbol(h_msg, "sceMsgDialogProgressBarSetValue");
    if (!fn_m_val) fn_m_val = (msg_set_val_fn)obs_module_symbol(h_msg, "wTpfglkmv34");
    if (!fn_m_val) fn_m_val = (msg_set_val_fn)obs_module_symbol(h_msg, "$wTpfglkmv34");
    if (!fn_m_val && obs_address_is_callable((const void *)&sceMsgDialogProgressBarSetValue_static)) fn_m_val = (msg_set_val_fn)&sceMsgDialogProgressBarSetValue_static;
    if (!fn_m_val && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "wTpfglkmv34", (void **)&fn_m_val);

    msg_set_msg_fn fn_m_set_msg = (msg_set_msg_fn)obs_module_symbol(h_msg, "sceMsgDialogProgressBarSetMsg");
    if (!fn_m_set_msg) fn_m_set_msg = (msg_set_msg_fn)obs_module_symbol(h_msg, "6H-71OdrpXM");
    if (!fn_m_set_msg) fn_m_set_msg = (msg_set_msg_fn)obs_module_symbol(h_msg, "$6H-71OdrpXM");
    if (!fn_m_set_msg && obs_address_is_callable((const void *)&sceMsgDialogProgressBarSetMsg_static)) fn_m_set_msg = (msg_set_msg_fn)&sceMsgDialogProgressBarSetMsg_static;
    if (!fn_m_set_msg && obs_address_is_callable((const void *)&sceKernelDlsym)) (void)sceKernelDlsym(h_msg, "6H-71OdrpXM", (void **)&fn_m_set_msg);

    obs_report_measure(id, "msg-dialog", "module-handle", (uint64_t)(uint32_t)h_msg, "handle");
    obs_report_measure(id, "msg-dialog", "fn-init-ptr", (uint64_t)(uintptr_t)fn_m_init, "vaddr");

    if (obs_address_is_callable((const void *)fn_m_init)) {
        int rc_init = fn_m_init();
        obs_report_measure(id, "msg-dialog", "init-rc", (uint64_t)(uint32_t)rc_init, "code");

        /* Progress calls with no progress dialog */
        if (obs_address_is_callable((const void *)fn_m_inc)) {
            int rc_inc = fn_m_inc(0);
            obs_report_measure(id, "msg-dialog", "progress-inc-no-dialog-rc", (uint64_t)(uint32_t)rc_inc, "code");
        }
        if (obs_address_is_callable((const void *)fn_m_set_msg)) {
            int rc_sm = fn_m_set_msg(0, "probe_msg");
            obs_report_measure(id, "msg-dialog", "progress-set-msg-no-dialog-rc", (uint64_t)(uint32_t)rc_sm, "code");
        }
        if (obs_address_is_callable((const void *)fn_m_val)) {
            int rc_val = fn_m_val(0, 50);
            obs_report_measure(id, "msg-dialog", "progress-val-no-dialog-rc", (uint64_t)(uint32_t)rc_val, "code");
        }

        /* Open user-message dialog */
        if (obs_address_is_callable((const void *)fn_m_open)) {
            uint8_t m_param[0x80];
            memset(m_param, 0, sizeof(m_param));
            *(uint32_t *)(m_param + 0x30) = 0x80;
            *(uint32_t *)(m_param + 0x34) = 1; /* USER_MSG */
            int rc_open = fn_m_open(m_param);
            obs_report_measure(id, "msg-dialog", "open-rc", (uint64_t)(uint32_t)rc_open, "code");

            /* Poll status for 1s */
            int last_m_st = -1;
            if (tsc_freq > 0 && sceKernelReadTsc) {
                uint64_t t_end = sceKernelReadTsc() + tsc_freq;
                while (sceKernelReadTsc() < t_end) {
                    if (fn_m_status) last_m_st = fn_m_status();
                    if (obs_address_is_callable((const void *)&sceKernelUsleep)) sceKernelUsleep(16000);
                }
            }
            obs_report_measure(id, "msg-dialog", "status-1s", (uint64_t)(uint32_t)last_m_st, "status");

            /* Close */
            if (obs_address_is_callable((const void *)fn_m_close)) {
                int rc_close = fn_m_close();
                obs_report_measure(id, "msg-dialog", "close-rc", (uint64_t)(uint32_t)rc_close, "code");
            }
            if (obs_address_is_callable((const void *)fn_m_status)) {
                int st_after_close = fn_m_status();
                obs_report_measure(id, "msg-dialog", "status-after-close", (uint64_t)(uint32_t)st_after_close, "status");
            }

            /* GetResult */
            if (obs_address_is_callable((const void *)fn_m_result)) {
                uint8_t mres[0x30];
                memset(mres, 0xcc, sizeof(mres));
                int rc_res = fn_m_result(mres);
                obs_report_measure(id, "msg-dialog", "result-rc", (uint64_t)(uint32_t)rc_res, "code");
                obs_report_bytes(id, "msg-dialog", "result-0x30", 0, mres, sizeof(mres));
            }

            if (obs_address_is_callable((const void *)fn_m_term)) {
                int rc_term = fn_m_term();
                obs_report_measure(id, "msg-dialog", "term-rc", (uint64_t)(uint32_t)rc_term, "code");
            }
        }
    }

    /* (5) WebBrowserDialog (REQ-20261012T1816Z-dbfe & REQ-20261012T1818Z-6f24) */
    int h_web_sprx = -1;
    if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
        h_web_sprx = sceKernelLoadStartModule("/system/common/lib/libSceWebBrowserDialog.sprx", 0, NULL, 0, NULL, NULL);
        if (h_web_sprx <= 1) {
            h_web_sprx = sceKernelLoadStartModule("libSceWebBrowserDialog.sprx", 0, NULL, 0, NULL, NULL);
        }
    }
    obs_report_measure(id, "web-browser", "load-web-sprx", (uint64_t)(uint32_t)h_web_sprx, "code");

    int h_web = (h_web_sprx > 1) ? h_web_sprx : -1;
    if (h_web <= 1) {
        h_web = obs_module_open("libSceWebBrowserDialog");
    }
    obs_report_measure(id, "web-browser", "module-handle", (uint64_t)(uint32_t)h_web, "handle");
    typedef int (*web_init_fn)(void);
    typedef int (*web_open_fn)(const void *);
    typedef int (*web_status_fn)(void);
    typedef int (*web_update_fn)(void);
    typedef int (*web_close_fn)(void);
    typedef int (*web_term_fn)(void);

    web_init_fn fn_w_init = (web_init_fn)obs_module_symbol(h_web, "sceWebBrowserDialogInitialize");
    if (!fn_w_init && obs_address_is_callable((const void *)&sceWebBrowserDialogInitialize_static)) fn_w_init = (web_init_fn)&sceWebBrowserDialogInitialize_static;
    web_open_fn fn_w_open = (web_open_fn)obs_module_symbol(h_web, "sceWebBrowserDialogOpen");
    if (!fn_w_open && obs_address_is_callable((const void *)&sceWebBrowserDialogOpen_static)) fn_w_open = (web_open_fn)&sceWebBrowserDialogOpen_static;
    web_status_fn fn_w_status = (web_status_fn)obs_module_symbol(h_web, "sceWebBrowserDialogGetStatus");
    if (!fn_w_status && obs_address_is_callable((const void *)&sceWebBrowserDialogGetStatus_static)) fn_w_status = (web_status_fn)&sceWebBrowserDialogGetStatus_static;
    web_update_fn fn_w_update = (web_update_fn)obs_module_symbol(h_web, "sceWebBrowserDialogUpdateStatus");
    if (!fn_w_update && obs_address_is_callable((const void *)&sceWebBrowserDialogUpdateStatus_static)) fn_w_update = (web_update_fn)&sceWebBrowserDialogUpdateStatus_static;
    web_close_fn fn_w_close = (web_close_fn)obs_module_symbol(h_web, "sceWebBrowserDialogClose");
    if (!fn_w_close && obs_address_is_callable((const void *)&sceWebBrowserDialogClose_static)) fn_w_close = (web_close_fn)&sceWebBrowserDialogClose_static;
    web_term_fn fn_w_term = (web_term_fn)obs_module_symbol(h_web, "sceWebBrowserDialogTerminate");
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        if (!fn_w_init) (void)sceKernelDlsym(h_web, "jqb7HntFQFc#G#A", (void **)&fn_w_init);
        if (!fn_w_open) (void)sceKernelDlsym(h_web, "FraP7debcdg#G#A", (void **)&fn_w_open);
        if (!fn_w_status) (void)sceKernelDlsym(h_web, "CFTG6a8TjOU#G#A", (void **)&fn_w_status);
        if (!fn_w_update) (void)sceKernelDlsym(h_web, "Wit4LjeoeX4#G#A", (void **)&fn_w_update);
        if (!fn_w_close) (void)sceKernelDlsym(h_web, "PSK+Eik919Q#G#A", (void **)&fn_w_close);
        if (!fn_w_term) (void)sceKernelDlsym(h_web, "ocHtyBwHfys#G#A", (void **)&fn_w_term);
    }

    uintptr_t base_web = obs_find_loaded_module_base("libSceWebBrowserDialog");
    if (base_web != 0) {
        if (!fn_w_init) fn_w_init = (web_init_fn)(base_web + 0x1a30);
        if (!fn_w_open) fn_w_open = (web_open_fn)(base_web + 0x1cf0);
        if (!fn_w_update) fn_w_update = (web_update_fn)(base_web + 0x25c0);
        if (!fn_w_status) fn_w_status = (web_status_fn)(base_web + 0x2780);
        if (!fn_w_close) fn_w_close = (web_close_fn)(base_web + 0x2a10);
        if (!fn_w_term) fn_w_term = (web_term_fn)(base_web + 0x2ad0);
    }
    obs_report_measure(id, "web-browser", "module-base", (uint64_t)base_web, "vaddr");
    obs_report_measure(id, "web-browser", "fn-init-ptr", (uint64_t)(uintptr_t)fn_w_init, "vaddr");

    /* 1. UpdateStatus before Initialize */
    if (obs_address_is_callable((const void *)fn_w_update)) {
        int ust_before = fn_w_update();
        obs_report_measure(id, "web-browser", "update-status-before-init", (uint64_t)(uint32_t)ust_before, "status");
    }

    /* 2. Initialize */
    if (obs_address_is_callable((const void *)fn_w_init)) {
        int rc1 = fn_w_init();
        obs_report_measure(id, "web-browser", "init-first-rc", (uint64_t)(uint32_t)rc1, "code");
        if (obs_address_is_callable((const void *)fn_w_update)) {
            int ust_post_init = fn_w_update();
            obs_report_measure(id, "web-browser", "update-status-post-init", (uint64_t)(uint32_t)ust_post_init, "status");
        }
        int rc2 = fn_w_init();
        obs_report_measure(id, "web-browser", "init-second-rc", (uint64_t)(uint32_t)rc2, "code");

        /* 3. Open refusals: NULL, zeroed 0x100, 0x100 with size word 0x100 */
        if (obs_address_is_callable((const void *)fn_w_open)) {
            int sig = OBS_FAULT_ARM(&guard);
            int rc_null = -1;
            if (sig == 0) {
                rc_null = fn_w_open(NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "web-browser", "open-null-rc", (uint64_t)(uint32_t)rc_null, "code");

            uint8_t zero_block[0x100];
            memset(zero_block, 0, sizeof(zero_block));
            sig = OBS_FAULT_ARM(&guard);
            int rc_zero = -1;
            if (sig == 0) {
                rc_zero = fn_w_open(zero_block);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "web-browser", "open-zero-rc", (uint64_t)(uint32_t)rc_zero, "code");

            uint8_t sized_block[0x100];
            memset(sized_block, 0, sizeof(sized_block));
            *(uint32_t *)sized_block = 0x100;
            sig = OBS_FAULT_ARM(&guard);
            int rc_sized = -1;
            if (sig == 0) {
                rc_sized = fn_w_open(sized_block);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "web-browser", "open-sized-rc", (uint64_t)(uint32_t)rc_sized, "code");
        }

        /* 4. Terminate */
        if (obs_address_is_callable((const void *)fn_w_term)) {
            int rc_term1 = fn_w_term();
            obs_report_measure(id, "web-browser", "term-first-rc", (uint64_t)(uint32_t)rc_term1, "code");
            if (obs_address_is_callable((const void *)fn_w_update)) {
                int ust_post_term = fn_w_update();
                obs_report_measure(id, "web-browser", "update-status-post-term", (uint64_t)(uint32_t)ust_post_term, "status");
            }
            int rc_term2 = fn_w_term();
            obs_report_measure(id, "web-browser", "term-second-rc", (uint64_t)(uint32_t)rc_term2, "code");
        }
    }

    return obs_pass();
}

/* ---- 4. libSceUlt --------------------------------------------------------- */
static volatile int s_ult_thread_ran = 0;
static volatile uint64_t s_ult_thread_id = 0;
static volatile uintptr_t s_ult_thread_stack = 0;
static void (*s_fn_ult_exit)(int) = NULL;

/* 5c6c entry registers buffer and stub */
static uint64_t s_5c6c_regs[8];
void ult_entry_regs_stub(uint64_t arg);
__asm__(
    ".global ult_entry_regs_stub\n"
    "ult_entry_regs_stub:\n"
    "movq %rdi, s_5c6c_regs+0(%rip)\n"
    "movq %rsi, s_5c6c_regs+8(%rip)\n"
    "movq %rdx, s_5c6c_regs+16(%rip)\n"
    "movq %rcx, s_5c6c_regs+24(%rip)\n"
    "movq %r8,  s_5c6c_regs+32(%rip)\n"
    "movq %r9,  s_5c6c_regs+40(%rip)\n"
    "movq %rsp, s_5c6c_regs+48(%rip)\n"
    "movq %rbp, s_5c6c_regs+56(%rip)\n"
    "movq s_fn_ult_exit(%rip), %rax\n"
    "testq %rax, %rax\n"
    "jz 1f\n"
    "xorl %edi, %edi\n"
    "callq *%rax\n"
    "1:\n"
    "retq\n"
);

typedef struct {
    uint64_t start_tsc;
    uint64_t end_tsc;
    uint64_t tid;
} UltWorkerRecord;
static volatile UltWorkerRecord s_5c6c_records[5];

static void ult_spin_worker(uint64_t idx) {
    if (idx < 5) {
        s_5c6c_records[idx].start_tsc = sceKernelReadTsc();
        if (obs_address_is_callable((const void *)&scePthreadSelf)) {
            s_5c6c_records[idx].tid = (uint64_t)(uintptr_t)scePthreadSelf();
        }
        uint64_t t0 = s_5c6c_records[idx].start_tsc;
        while ((sceKernelReadTsc() - t0) < 80000000ULL) {
#if defined(__x86_64__)
            __asm__ __volatile__("pause");
#endif
        }
        s_5c6c_records[idx].end_tsc = sceKernelReadTsc();
    }
    if (s_fn_ult_exit) s_fn_ult_exit(0);
}

static volatile int s_5c6c_order[2];
static volatile int s_5c6c_order_idx = 0;
static void *s_5c6c_mutex_ptr = NULL;
typedef int (*fn_mtx_lock_t)(void *);
typedef int (*fn_mtx_unlock_t)(void *);
static fn_mtx_lock_t s_5c6c_mtx_lock = NULL;
static fn_mtx_unlock_t s_5c6c_mtx_unlock = NULL;

static void ult_blocking_worker_a(uint64_t arg) {
    (void)arg;
    if (s_5c6c_mtx_lock && s_5c6c_mutex_ptr) {
        s_5c6c_mtx_lock(s_5c6c_mutex_ptr);
        s_5c6c_order[__atomic_fetch_add(&s_5c6c_order_idx, 1, __ATOMIC_SEQ_CST)] = 1;
        if (s_5c6c_mtx_unlock) s_5c6c_mtx_unlock(s_5c6c_mutex_ptr);
    }
    if (s_fn_ult_exit) s_fn_ult_exit(0);
}

static void ult_blocking_worker_b(uint64_t arg) {
    (void)arg;
    s_5c6c_order[__atomic_fetch_add(&s_5c6c_order_idx, 1, __ATOMIC_SEQ_CST)] = 2;
    if (s_fn_ult_exit) s_fn_ult_exit(0);
}

static void ult_return_0x55_worker(uint64_t arg) {
    (void)arg;
#if defined(__x86_64__)
    __asm__ __volatile__("movl $0x55, %eax");
#endif
}

static volatile uintptr_t s_5c6c_frame_addr = 0;
static void ult_stack_measure_worker(uint64_t arg) {
    (void)arg;
    volatile uint8_t local_buf[64];
    s_5c6c_frame_addr = (uintptr_t)local_buf;
    if (s_fn_ult_exit) s_fn_ult_exit(0);
}

static void ult_worker_func(uint64_t arg) {
    (void)arg;
    s_ult_thread_ran = 1;
    if (obs_address_is_callable((const void *)&scePthreadSelf)) {
        s_ult_thread_id = (uint64_t)(uintptr_t)scePthreadSelf();
    }
    int stack_var = 42;
    s_ult_thread_stack = (uintptr_t)&stack_var;
    if (s_fn_ult_exit != NULL && (uintptr_t)s_fn_ult_exit > 0x10000) {
        s_fn_ult_exit(0x1234);
    }
}

static obs_result check_sce_ult(void) {
    const char *id = "000-boot/sce-ult";
    obs_jmp_buf guard;

    /* 1. Binding attempts across candidate sysmodules and module paths */
    static const char *const mod_paths[] = {
        "/system/common/lib/libSceUlt.sprx",
        "libSceUlt.sprx",
    };
    int last_valid_h = -1;
    for (size_t p = 0; p < OBS_COUNT(mod_paths); p++) {
        if (obs_address_is_callable((const void *)&sceKernelLoadStartModule)) {
            int sig = OBS_FAULT_ARM(&guard);
            int m_h = -1;
            if (sig == 0) {
                m_h = sceKernelLoadStartModule(mod_paths[p], 0, NULL, 0, NULL, NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, mod_paths[p], "load-rc", (uint64_t)(uint32_t)m_h, "code");
            if (m_h > 1 && last_valid_h <= 1) {
                last_valid_h = m_h;
            }
        }
    }

    int h_ult = obs_module_open("libSceUlt");
    if (h_ult <= 1 && last_valid_h > 1) {
        h_ult = last_valid_h;
    }
    obs_report_measure(id, "libSceUlt", "module-handle", (uint64_t)(uint32_t)h_ult, "handle");

    typedef size_t (*ult_rt_sz_fn)(uint32_t, uint32_t);
    typedef size_t (*ult_pool_sz_fn)(uint32_t, const void *);
    typedef int (*ult_rt_opt_init_fn)(void *, uint32_t);
    typedef int (*ult_rt_init_fn)(void *, const char *, uint32_t, uint32_t, void *, const void *, size_t);
    typedef int (*ult_rt_term_fn)(void *);
    typedef int (*ult_pool_init_fn)(void *, const char *, uint32_t, uint32_t, void *, const void *);
    typedef int (*ult_pool_term_fn)(void *);
    typedef int (*ult_create_fn)(void *, const char *, void (*)(uint64_t), uint64_t, void *, size_t, void *, const void *);
    typedef void (*ult_exit_fn)(int);
    typedef int (*ult_join_fn)(void *, int *);
    typedef int (*ult_yield_fn)(void);
    typedef int (*ult_mutex_init_fn)(void *, const char *, void *, const void *);
    typedef int (*ult_mutex_lock_fn)(void *);
    typedef int (*ult_mutex_trylock_fn)(void *);
    typedef int (*ult_mutex_unlock_fn)(void *);
    typedef int (*ult_mutex_term_fn)(void *);

    ult_rt_sz_fn fn_rt_sz = (ult_rt_sz_fn)obs_module_symbol(h_ult, "sceUltUlthreadRuntimeGetWorkAreaSize");
    if (!fn_rt_sz && obs_address_is_callable((const void *)&sceUltUlthreadRuntimeGetWorkAreaSize_static)) fn_rt_sz = (ult_rt_sz_fn)&sceUltUlthreadRuntimeGetWorkAreaSize_static;
    ult_pool_sz_fn fn_pool_sz = (ult_pool_sz_fn)obs_module_symbol(h_ult, "sceUltWaitingQueueResourcePoolGetWorkAreaSize");
    if (!fn_pool_sz && obs_address_is_callable((const void *)&sceUltWaitingQueueResourcePoolGetWorkAreaSize_static)) fn_pool_sz = (ult_pool_sz_fn)&sceUltWaitingQueueResourcePoolGetWorkAreaSize_static;
    ult_rt_opt_init_fn fn_rt_opt_init = (ult_rt_opt_init_fn)obs_module_symbol(h_ult, "_sceUltUlthreadRuntimeOptParamInitialize");
    if (!fn_rt_opt_init && obs_address_is_callable((const void *)&_sceUltUlthreadRuntimeOptParamInitialize_static)) fn_rt_opt_init = (ult_rt_opt_init_fn)&_sceUltUlthreadRuntimeOptParamInitialize_static;
    ult_rt_init_fn fn_rt_init = (ult_rt_init_fn)obs_module_symbol(h_ult, "_sceUltUlthreadRuntimeCreate");
    if (!fn_rt_init && obs_address_is_callable((const void *)&_sceUltUlthreadRuntimeCreate_static)) fn_rt_init = (ult_rt_init_fn)&_sceUltUlthreadRuntimeCreate_static;
    ult_rt_term_fn fn_rt_term = (ult_rt_term_fn)obs_module_symbol(h_ult, "sceUltUlthreadRuntimeDestroy");
    if (!fn_rt_term && obs_address_is_callable((const void *)&sceUltUlthreadRuntimeDestroy_static)) fn_rt_term = (ult_rt_term_fn)&sceUltUlthreadRuntimeDestroy_static;
    ult_pool_init_fn fn_pool_init = (ult_pool_init_fn)obs_module_symbol(h_ult, "_sceUltWaitingQueueResourcePoolCreate");
    if (!fn_pool_init && obs_address_is_callable((const void *)&_sceUltWaitingQueueResourcePoolCreate_static)) fn_pool_init = (ult_pool_init_fn)&_sceUltWaitingQueueResourcePoolCreate_static;
    ult_pool_term_fn fn_pool_term = (ult_pool_term_fn)obs_module_symbol(h_ult, "sceUltWaitingQueueResourcePoolDestroy");
    if (!fn_pool_term && obs_address_is_callable((const void *)&sceUltWaitingQueueResourcePoolDestroy_static)) fn_pool_term = (ult_pool_term_fn)&sceUltWaitingQueueResourcePoolDestroy_static;
    ult_create_fn fn_create = (ult_create_fn)obs_module_symbol(h_ult, "_sceUltUlthreadCreate");
    if (!fn_create && obs_address_is_callable((const void *)&_sceUltUlthreadCreate_static)) fn_create = (ult_create_fn)&_sceUltUlthreadCreate_static;
    ult_exit_fn fn_exit = (ult_exit_fn)obs_module_symbol(h_ult, "sceUltUlthreadExit");
    if (!fn_exit && obs_address_is_callable((const void *)&sceUltUlthreadExit_static)) fn_exit = (ult_exit_fn)&sceUltUlthreadExit_static;
    s_fn_ult_exit = fn_exit;
    ult_join_fn fn_join = (ult_join_fn)obs_module_symbol(h_ult, "sceUltUlthreadJoin");
    if (!fn_join && obs_address_is_callable((const void *)&sceUltUlthreadJoin_static)) fn_join = (ult_join_fn)&sceUltUlthreadJoin_static;
    ult_yield_fn fn_yield = (ult_yield_fn)obs_module_symbol(h_ult, "sceUltUlthreadYield");
    if (!fn_yield && obs_address_is_callable((const void *)&sceUltUlthreadYield_static)) fn_yield = (ult_yield_fn)&sceUltUlthreadYield_static;
    ult_mutex_init_fn fn_mutex_init = (ult_mutex_init_fn)obs_module_symbol(h_ult, "_sceUltMutexCreate");
    if (!fn_mutex_init && obs_address_is_callable((const void *)&_sceUltMutexCreate_static)) fn_mutex_init = (ult_mutex_init_fn)&_sceUltMutexCreate_static;
    ult_mutex_lock_fn fn_mutex_lock = (ult_mutex_lock_fn)obs_module_symbol(h_ult, "sceUltMutexLock");
    if (!fn_mutex_lock && obs_address_is_callable((const void *)&sceUltMutexLock_static)) fn_mutex_lock = (ult_mutex_lock_fn)&sceUltMutexLock_static;
    ult_mutex_trylock_fn fn_mutex_trylock = (ult_mutex_trylock_fn)obs_module_symbol(h_ult, "sceUltMutexTryLock");
    if (!fn_mutex_trylock && obs_address_is_callable((const void *)&sceUltMutexTryLock_static)) fn_mutex_trylock = (ult_mutex_trylock_fn)&sceUltMutexTryLock_static;
    ult_mutex_unlock_fn fn_mutex_unlock = (ult_mutex_unlock_fn)obs_module_symbol(h_ult, "sceUltMutexUnlock");
    if (!fn_mutex_unlock && obs_address_is_callable((const void *)&sceUltMutexUnlock_static)) fn_mutex_unlock = (ult_mutex_unlock_fn)&sceUltMutexUnlock_static;
    ult_mutex_term_fn fn_mutex_term = (ult_mutex_term_fn)obs_module_symbol(h_ult, "sceUltMutexDestroy");
    if (!fn_mutex_term && obs_address_is_callable((const void *)&sceUltMutexDestroy_static)) fn_mutex_term = (ult_mutex_term_fn)&sceUltMutexDestroy_static;

    uintptr_t base_ult = obs_find_loaded_module_base("libSceUlt");

    obs_report_measure(id, "libSceUlt", "module-base", (uint64_t)base_ult, "vaddr");
    obs_report_measure(id, "libSceUlt", "resolved-rt-sz", (uint64_t)obs_address_is_callable((const void *)fn_rt_sz), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-pool-sz", (uint64_t)obs_address_is_callable((const void *)fn_pool_sz), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-opt-init", (uint64_t)obs_address_is_callable((const void *)fn_rt_opt_init), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-create", (uint64_t)obs_address_is_callable((const void *)fn_create), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-join", (uint64_t)obs_address_is_callable((const void *)fn_join), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-mutex", (uint64_t)obs_address_is_callable((const void *)fn_mutex_lock), "bool");
    obs_report_measure(id, "libSceUlt", "resolved-trylock", (uint64_t)obs_address_is_callable((const void *)fn_mutex_trylock), "bool");

    /* Arm 1: Sizers */
    size_t sz_rt = 0;
    if (obs_address_is_callable((const void *)fn_rt_sz)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sz_rt = fn_rt_sz(16, 3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "runtime-workarea-16-3", "size", (uint64_t)sz_rt, "bytes");
    }
    size_t sz_pool_16 = 0;
    size_t sz_pool_null = 0;
    if (obs_address_is_callable((const void *)fn_pool_sz)) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sz_pool_null = fn_pool_sz(16, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "pool-workarea-16-null", "size", (uint64_t)sz_pool_null, "bytes");

        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            sz_pool_16 = fn_pool_sz(16, (const void *)16);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "pool-workarea-16-16", "size", (uint64_t)sz_pool_16, "bytes");
    }

    /* Arm 1: OptParam Initialize */
    uint8_t opt_block[0x80] __attribute__((aligned(8)));
    memset(opt_block, 0xcc, sizeof(opt_block));
    int rc_opt_init = -1;
    if (obs_address_is_callable((const void *)fn_rt_opt_init) && (uintptr_t)fn_rt_opt_init > 0x10000) {
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_opt_init = fn_rt_opt_init(opt_block, 0x12400000);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ult-runtime", "opt-init-rc", (uint64_t)(uint32_t)rc_opt_init, "code");
        obs_report_bytes(id, "ult-runtime", "opt-param-0x80", 0, opt_block, sizeof(opt_block));
    }

    if (obs_address_is_callable((const void *)fn_rt_init) &&
        (uintptr_t)fn_rt_init > 0x10000) {
        static uint8_t s_ult_rt_mem[0x20000] __attribute__((aligned(64)));
        static uint8_t s_ult_pool_mem[0x20000] __attribute__((aligned(64)));
        static uint8_t s_ult_stack[0x8000] __attribute__((aligned(64)));
        static uint8_t s_ult_rt_obj[0x1000] __attribute__((aligned(64)));
        static uint8_t s_ult_pool_obj[512] __attribute__((aligned(64)));
        static uint8_t s_ult_thread_obj[512] __attribute__((aligned(64)));
        static uint8_t s_ult_mutex_obj[512] __attribute__((aligned(64)));

        /* Try 1: with optParam = NULL */
        int rc_rinit_null = -1;
        int sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rinit_null = fn_rt_init(s_ult_rt_obj, "probe_rt", 16, 3, s_ult_rt_mem, NULL, sizeof(s_ult_rt_mem));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ult-runtime", "init-null-opt-rc", (uint64_t)(uint32_t)rc_rinit_null, "code");

        /* If NULL opt succeeded, destroy to try initialized opt block */
        if (rc_rinit_null == 0 && obs_address_is_callable((const void *)fn_rt_term) && (uintptr_t)fn_rt_term > 0x10000) {
            fn_rt_term(s_ult_rt_obj);
        }

        /* Try 2: with 0x80-byte block of zeros */
        uint8_t zero_opt[0x80] __attribute__((aligned(8)));
        memset(zero_opt, 0, sizeof(zero_opt));
        int rc_rinit_zeros = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_rinit_zeros = fn_rt_init(s_ult_rt_obj, "probe_rt", 16, 3, s_ult_rt_mem, zero_opt, sizeof(s_ult_rt_mem));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ult-runtime", "init-zeros-opt-rc", (uint64_t)(uint32_t)rc_rinit_zeros, "code");

        if (rc_rinit_zeros == 0 && obs_address_is_callable((const void *)fn_rt_term) && (uintptr_t)fn_rt_term > 0x10000) {
            fn_rt_term(s_ult_rt_obj);
        }

        /* Try 3: with initialized optParam (or NULL fallback) */
        int rc_rinit = -1;
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            const void *use_opt = (rc_opt_init == 0) ? (const void *)opt_block : NULL;
            rc_rinit = fn_rt_init(s_ult_rt_obj, "probe_rt", 16, 3, s_ult_rt_mem, use_opt, sizeof(s_ult_rt_mem));
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
        obs_report_measure(id, "ult-runtime", "init-rc", (uint64_t)(uint32_t)rc_rinit, "code");

        if (rc_rinit == 0 && obs_address_is_callable((const void *)fn_pool_init) &&
            (uintptr_t)fn_pool_init > 0x10000) {
            int rc_pinit = -1;
            sig = OBS_FAULT_ARM(&guard);
            if (sig == 0) {
                rc_pinit = fn_pool_init(s_ult_pool_obj, "probe_pool", 16, 16, s_ult_pool_mem, NULL);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "ult-pool", "init-rc", (uint64_t)(uint32_t)rc_pinit, "code");

            if (rc_pinit == 0 && obs_address_is_callable((const void *)fn_create) &&
                (uintptr_t)fn_create > 0x10000) {
                s_ult_thread_ran = 0;
                s_ult_thread_id = 0;
                s_ult_thread_stack = 0;

                uint64_t creator_tid = 0;
                if (obs_address_is_callable((const void *)&scePthreadSelf)) {
                    creator_tid = (uint64_t)(uintptr_t)scePthreadSelf();
                }
                obs_report_measure(id, "ult-thread", "creator-tid", creator_tid, "id");
                obs_report_measure(id, "ult-thread", "stack-base", (uint64_t)(uintptr_t)s_ult_stack, "vaddr");
                obs_report_measure(id, "ult-thread", "stack-limit", (uint64_t)(uintptr_t)(s_ult_stack + sizeof(s_ult_stack)), "vaddr");

                /* Arm 2: Create thread with runtime passed as 7th argument */
                int rc_create = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_create = fn_create(s_ult_thread_obj, "probe_ult", ult_worker_func, 0,
                                          s_ult_stack, sizeof(s_ult_stack), s_ult_rt_obj, NULL);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "ult-thread", "create-rc", (uint64_t)(uint32_t)rc_create, "code");
                /* Arm 2: Read ran flag immediately before yield */
                obs_report_measure(id, "ult-thread", "ran-before-yield", (uint64_t)s_ult_thread_ran, "bool");

                if (obs_address_is_callable((const void *)fn_yield) && (uintptr_t)fn_yield > 0x10000) {
                    (void)fn_yield();
                }
                obs_report_measure(id, "ult-thread", "ran-after-yield", (uint64_t)s_ult_thread_ran, "bool");

                /* Arm 5: Where the entry runs */
                obs_report_measure(id, "ult-thread", "entry-tid", s_ult_thread_id, "id");
                obs_report_measure(id, "ult-thread", "ran-on-creator", (uint64_t)(s_ult_thread_id == creator_tid), "bool");
                obs_report_measure(id, "ult-thread", "entry-stack-addr", (uint64_t)s_ult_thread_stack, "vaddr");
                obs_report_measure(id, "ult-thread", "stack-inside-context",
                                   (uint64_t)(s_ult_thread_stack >= (uintptr_t)s_ult_stack &&
                                              s_ult_thread_stack < (uintptr_t)(s_ult_stack + sizeof(s_ult_stack))),
                                   "bool");

                /* Arm 3: Join with 0xcc status buffer */
                if (obs_address_is_callable((const void *)fn_join) && (uintptr_t)fn_join > 0x10000) {
                    uint8_t join_buf[16] __attribute__((aligned(8)));
                    memset(join_buf, 0xcc, sizeof(join_buf));
                    int *join_st_ptr = (int *)(join_buf + 4);
                    int rc_join = -1;
                    sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc_join = fn_join(s_ult_thread_obj, join_st_ptr);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    obs_report_measure(id, "ult-thread", "join-rc", (uint64_t)(uint32_t)rc_join, "code");
                    obs_report_measure(id, "ult-thread", "join-status", (uint64_t)(uint32_t)*join_st_ptr, "code");
                    obs_report_bytes(id, "ult-thread", "join-status-bytes", 0, join_buf, sizeof(join_buf));
                }

                /* ---- REQ-20261013T1440Z-5c6c: Ulthreads after u1t2 ---- */
                /* Arm 1: Entry registers with arg=0 and arg=0x1111 */
                memset(s_5c6c_regs, 0xcc, sizeof(s_5c6c_regs));
                static uint8_t s_5c6c_thread_obj1[512] __attribute__((aligned(64)));
                static uint8_t s_5c6c_stack1[0x2000] __attribute__((aligned(64)));
                int rc_reg0 = fn_create(s_5c6c_thread_obj1, "ult_reg0", ult_entry_regs_stub, 0,
                                        s_5c6c_stack1, sizeof(s_5c6c_stack1), s_ult_rt_obj, NULL);
                obs_report_measure(id, "5c6c-entry-regs-arg0", "create-rc", (uint64_t)(uint32_t)rc_reg0, "code");
                if (rc_reg0 == 0) {
                    if (fn_yield) fn_yield();
                    int st = 0;
                    if (fn_join) fn_join(s_5c6c_thread_obj1, &st);
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rdi", s_5c6c_regs[0], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rsi", s_5c6c_regs[1], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rdx", s_5c6c_regs[2], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rcx", s_5c6c_regs[3], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "r8",  s_5c6c_regs[4], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "r9",  s_5c6c_regs[5], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rsp", s_5c6c_regs[6], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rbp", s_5c6c_regs[7], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rsi-points-to-thread",
                                       (uint64_t)(s_5c6c_regs[1] == (uintptr_t)s_5c6c_thread_obj1), "bool");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rsi-points-to-rt",
                                       (uint64_t)(s_5c6c_regs[1] == (uintptr_t)s_ult_rt_obj), "bool");
                    obs_report_measure(id, "5c6c-entry-regs-arg0", "rsp-inside-stack",
                                       (uint64_t)(s_5c6c_regs[6] >= (uintptr_t)s_5c6c_stack1 &&
                                                  s_5c6c_regs[6] < (uintptr_t)(s_5c6c_stack1 + sizeof(s_5c6c_stack1))), "bool");
                }

                memset(s_5c6c_regs, 0xcc, sizeof(s_5c6c_regs));
                int rc_reg1 = fn_create(s_5c6c_thread_obj1, "ult_reg1", ult_entry_regs_stub, 0x1111,
                                        s_5c6c_stack1, sizeof(s_5c6c_stack1), s_ult_rt_obj, NULL);
                obs_report_measure(id, "5c6c-entry-regs-arg1111", "create-rc", (uint64_t)(uint32_t)rc_reg1, "code");
                if (rc_reg1 == 0) {
                    if (fn_yield) fn_yield();
                    int st = 0;
                    if (fn_join) fn_join(s_5c6c_thread_obj1, &st);
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rdi", s_5c6c_regs[0], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rsi", s_5c6c_regs[1], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rdx", s_5c6c_regs[2], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rcx", s_5c6c_regs[3], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "r8",  s_5c6c_regs[4], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "r9",  s_5c6c_regs[5], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rsp", s_5c6c_regs[6], "reg");
                    obs_report_measure(id, "5c6c-entry-regs-arg1111", "rbp", s_5c6c_regs[7], "reg");
                }

                /* Arm 2: Concurrency on 3 workers (5 threads spinning 50ms) */
                static uint8_t s_5c6c_spin_objs[5][512] __attribute__((aligned(64)));
                static uint8_t s_5c6c_spin_stacks[5][0x2000] __attribute__((aligned(64)));
                memset((void *)s_5c6c_records, 0, sizeof(s_5c6c_records));
                int rc_spins = 0;
                for (uint64_t i = 0; i < 5; i++) {
                    char tname[32];
                    snprintf(tname, sizeof(tname), "spin_%u", (uint32_t)i);
                    int rc_c = fn_create(s_5c6c_spin_objs[i], tname, ult_spin_worker, i,
                                         s_5c6c_spin_stacks[i], sizeof(s_5c6c_spin_stacks[i]), s_ult_rt_obj, NULL);
                    if (rc_c != 0) rc_spins++;
                }
                obs_report_measure(id, "5c6c-concurrency", "create-all-rc", (uint64_t)(uint32_t)rc_spins, "code");
                for (int i = 0; i < 5; i++) {
                    int st = 0;
                    if (fn_join) fn_join(s_5c6c_spin_objs[i], &st);
                    char ttag[32];
                    snprintf(ttag, sizeof(ttag), "thread-%d", i);
                    obs_report_measure(id, "5c6c-concurrency", ttag, s_5c6c_records[i].tid, "tid");
                    snprintf(ttag, sizeof(ttag), "start-%d", i);
                    obs_report_measure(id, "5c6c-concurrency", ttag, s_5c6c_records[i].start_tsc, "ticks");
                    snprintf(ttag, sizeof(ttag), "end-%d", i);
                    obs_report_measure(id, "5c6c-concurrency", ttag, s_5c6c_records[i].end_tsc, "ticks");
                }

                /* Arm 4: Return 0x55 versus exit */
                static uint8_t s_5c6c_ret_obj[512] __attribute__((aligned(64)));
                static uint8_t s_5c6c_ret_stack[0x2000] __attribute__((aligned(64)));
                int rc_ret_c = fn_create(s_5c6c_ret_obj, "ret_55", ult_return_0x55_worker, 0,
                                         s_5c6c_ret_stack, sizeof(s_5c6c_ret_stack), s_ult_rt_obj, NULL);
                obs_report_measure(id, "5c6c-return-vs-exit", "create-rc", (uint64_t)(uint32_t)rc_ret_c, "code");
                if (rc_ret_c == 0) {
                    int ret_st = (int)0xcccccccc;
                    int rc_j = fn_join ? fn_join(s_5c6c_ret_obj, &ret_st) : -1;
                    obs_report_measure(id, "5c6c-return-vs-exit", "join-rc", (uint64_t)(uint32_t)rc_j, "code");
                    obs_report_measure(id, "5c6c-return-vs-exit", "status", (uint64_t)(uint32_t)ret_st, "code");
                }

                /* Arm 5: Stack use on 0x2000-byte context */
                static uint8_t s_5c6c_stk_obj[512] __attribute__((aligned(64)));
                static uint8_t s_5c6c_stk_ctx[0x2000] __attribute__((aligned(64)));
                memset(s_5c6c_stk_ctx, 0xcc, sizeof(s_5c6c_stk_ctx));
                s_5c6c_frame_addr = 0;
                int rc_stk_c = fn_create(s_5c6c_stk_obj, "stk_meas", ult_stack_measure_worker, 0,
                                         s_5c6c_stk_ctx, sizeof(s_5c6c_stk_ctx), s_ult_rt_obj, NULL);
                obs_report_measure(id, "5c6c-stack-use", "create-rc", (uint64_t)(uint32_t)rc_stk_c, "code");
                if (rc_stk_c == 0) {
                    int st = 0;
                    if (fn_join) fn_join(s_5c6c_stk_obj, &st);
                    obs_report_measure(id, "5c6c-stack-use", "ctx-base", (uint64_t)(uintptr_t)s_5c6c_stk_ctx, "vaddr");
                    obs_report_measure(id, "5c6c-stack-use", "ctx-limit", (uint64_t)(uintptr_t)(s_5c6c_stk_ctx + sizeof(s_5c6c_stk_ctx)), "vaddr");
                    obs_report_measure(id, "5c6c-stack-use", "lowest-frame-addr", (uint64_t)s_5c6c_frame_addr, "vaddr");
                    size_t modified_bytes = 0;
                    for (size_t b = 0; b < sizeof(s_5c6c_stk_ctx); b++) {
                        if (s_5c6c_stk_ctx[b] != 0xcc) modified_bytes++;
                    }
                    obs_report_measure(id, "5c6c-stack-use", "modified-bytes", (uint64_t)modified_bytes, "bytes");
                }
            }

            /* Arm 4: Mutex semantics with valid resource pool */
            if (obs_address_is_callable((const void *)fn_mutex_init) &&
                (uintptr_t)fn_mutex_init > 0x10000 &&
                obs_address_is_callable((const void *)fn_mutex_lock) &&
                (uintptr_t)fn_mutex_lock > 0x10000 &&
                obs_address_is_callable((const void *)fn_mutex_unlock) &&
                (uintptr_t)fn_mutex_unlock > 0x10000) {

                int rc_minit = -1;
                sig = OBS_FAULT_ARM(&guard);
                if (sig == 0) {
                    rc_minit = fn_mutex_init(s_ult_mutex_obj, "probe_mtx", s_ult_pool_obj, NULL);
                    obs_fault_unregister();
                } else {
                    obs_fault_unregister();
                }
                obs_report_measure(id, "ult-mutex", "init-rc", (uint64_t)(uint32_t)rc_minit, "code");

                if (rc_minit == 0) {
                    /* First lock */
                    int rc_lock1 = -1;
                    sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc_lock1 = fn_mutex_lock(s_ult_mutex_obj);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    obs_report_measure(id, "ult-mutex", "lock1-rc", (uint64_t)(uint32_t)rc_lock1, "code");

                    /* Second lock by owner (self-relock) */
                    int rc_relock = -1;
                    sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc_relock = fn_mutex_lock(s_ult_mutex_obj);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    obs_report_measure(id, "ult-mutex", "self-relock-rc", (uint64_t)(uint32_t)rc_relock, "code");

                    /* Non-blocking TryLock while held */
                    if (obs_address_is_callable((const void *)fn_mutex_trylock) && (uintptr_t)fn_mutex_trylock > 0x10000) {
                        int rc_try = -1;
                        sig = OBS_FAULT_ARM(&guard);
                        if (sig == 0) {
                            rc_try = fn_mutex_trylock(s_ult_mutex_obj);
                            obs_fault_unregister();
                        } else {
                            obs_fault_unregister();
                        }
                        obs_report_measure(id, "ult-mutex", "trylock-held-rc", (uint64_t)(uint32_t)rc_try, "code");
                    }

                    /* First unlock */
                    int rc_unlock1 = -1;
                    sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc_unlock1 = fn_mutex_unlock(s_ult_mutex_obj);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    obs_report_measure(id, "ult-mutex", "unlock1-rc", (uint64_t)(uint32_t)rc_unlock1, "code");

                    /* Unlock when not held */
                    int rc_unlock_unheld = -1;
                    sig = OBS_FAULT_ARM(&guard);
                    if (sig == 0) {
                        rc_unlock_unheld = fn_mutex_unlock(s_ult_mutex_obj);
                        obs_fault_unregister();
                    } else {
                        obs_fault_unregister();
                    }
                    obs_report_measure(id, "ult-mutex", "unlock-unheld-rc", (uint64_t)(uint32_t)rc_unlock_unheld, "code");

                    if (obs_address_is_callable((const void *)fn_mutex_term) && (uintptr_t)fn_mutex_term > 0x10000) {
                        fn_mutex_term(s_ult_mutex_obj);
                    }
                }
            }

            if (obs_address_is_callable((const void *)fn_pool_term) && (uintptr_t)fn_pool_term > 0x10000) {
                (void)fn_pool_term(s_ult_pool_obj);
            }
        }

        if (rc_rinit == 0 && obs_address_is_callable((const void *)fn_rt_term) && (uintptr_t)fn_rt_term > 0x10000) {
            (void)fn_rt_term(s_ult_rt_obj);
        }

        /* Arm 3: 1 worker blocking test */
        if (obs_address_is_callable((const void *)fn_rt_init) &&
            obs_address_is_callable((const void *)fn_pool_init) &&
            obs_address_is_callable((const void *)fn_mutex_init) &&
            obs_address_is_callable((const void *)fn_mutex_lock) &&
            obs_address_is_callable((const void *)fn_mutex_unlock)) {

            static uint8_t s_1w_rt_mem[0x20000] __attribute__((aligned(64)));
            static uint8_t s_1w_pool_mem[0x20000] __attribute__((aligned(64)));
            static uint8_t s_1w_rt_obj[0x1000] __attribute__((aligned(64)));
            static uint8_t s_1w_pool_obj[512] __attribute__((aligned(64)));
            static uint8_t s_1w_mtx_obj[512] __attribute__((aligned(64)));
            static uint8_t s_1w_th_a_obj[512] __attribute__((aligned(64)));
            static uint8_t s_1w_th_b_obj[512] __attribute__((aligned(64)));
            static uint8_t s_1w_stk_a[0x2000] __attribute__((aligned(64)));
            static uint8_t s_1w_stk_b[0x2000] __attribute__((aligned(64)));

            int rc_1w_rt = fn_rt_init(s_1w_rt_obj, "1w_rt", 16, 1, s_1w_rt_mem, NULL, sizeof(s_1w_rt_mem));
            obs_report_measure(id, "5c6c-1worker-blocking", "rt-init-rc", (uint64_t)(uint32_t)rc_1w_rt, "code");
            if (rc_1w_rt == 0) {
                int rc_p = fn_pool_init(s_1w_pool_obj, "1w_pool", 16, 16, s_1w_pool_mem, NULL);
                int rc_m = fn_mutex_init(s_1w_mtx_obj, "1w_mtx", s_1w_pool_obj, NULL);
                if (rc_p == 0 && rc_m == 0) {
                    s_5c6c_mutex_ptr = s_1w_mtx_obj;
                    s_5c6c_mtx_lock = fn_mutex_lock;
                    s_5c6c_mtx_unlock = fn_mutex_unlock;
                    s_5c6c_order[0] = 0; s_5c6c_order[1] = 0;
                    s_5c6c_order_idx = 0;

                    /* Creator locks mutex */
                    int rc_l = fn_mutex_lock(s_1w_mtx_obj);
                    obs_report_measure(id, "5c6c-1worker-blocking", "creator-lock-rc", (uint64_t)(uint32_t)rc_l, "code");

                    /* Create thread A which will block on mutex */
                    fn_create(s_1w_th_a_obj, "th_a", ult_blocking_worker_a, 0, s_1w_stk_a, sizeof(s_1w_stk_a), s_1w_rt_obj, NULL);
                    /* Create thread B */
                    fn_create(s_1w_th_b_obj, "th_b", ult_blocking_worker_b, 0, s_1w_stk_b, sizeof(s_1w_stk_b), s_1w_rt_obj, NULL);

                    /* Yield to allow worker to run */
                    if (fn_yield) fn_yield();
                    obs_report_measure(id, "5c6c-1worker-blocking", "b-ran-while-a-blocked", (uint64_t)(s_5c6c_order_idx > 0 && s_5c6c_order[0] == 2), "bool");

                    /* Creator unlocks mutex */
                    fn_mutex_unlock(s_1w_mtx_obj);

                    int st_a = 0, st_b = 0;
                    if (fn_join) {
                        fn_join(s_1w_th_a_obj, &st_a);
                        fn_join(s_1w_th_b_obj, &st_b);
                    }
                    obs_report_measure(id, "5c6c-1worker-blocking", "finish-order-0", (uint64_t)s_5c6c_order[0], "id");
                    obs_report_measure(id, "5c6c-1worker-blocking", "finish-order-1", (uint64_t)s_5c6c_order[1], "id");
                }
                if (fn_rt_term) fn_rt_term(s_1w_rt_obj);
            }
        }
    }

    return obs_pass();
}

/* ---- REQ-20261013T1015Z-ag02: VideoOut, SaveDataMemory2, Accessibility ---- */
static obs_result check_ag02_shapes(void) {
    const char *id = "000-boot/ag02-shapes";

    int32_t user = 0x1ea2f4d9;
    if (obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        int32_t init_user = -1;
        if (sceUserServiceGetInitialUser(&init_user) == 0 && init_user > 0) {
            user = init_user;
        }
    }
    obs_report_measure(id, "setup", "user-id", (uint64_t)(uint32_t)user, "id");

    /* 1. VideoOut: open main port, borrow handle if needed */
    int rc_open_user = -1;
    int rc_open_ff = -1;
    int vo_handle = -1;
    if (obs_address_is_callable((const void *)&sceVideoOutOpen)) {
        rc_open_user = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, NULL);
        if (rc_open_user > 0) {
            vo_handle = rc_open_user;
        } else {
            rc_open_ff = sceVideoOutOpen(0xff, OBS_VIDEO_BUS_MAIN, 0, NULL);
            if (rc_open_ff > 0) {
                vo_handle = rc_open_ff;
            }
        }
    }
    if (vo_handle <= 0) {
        vo_handle = obs_display_get_video_handle();
    }
    obs_report_measure(id, "video-out", "open-user-rc", (uint64_t)(int64_t)rc_open_user, "handle");
    obs_report_measure(id, "video-out", "open-0xff-rc", (uint64_t)(int64_t)rc_open_ff, "handle");
    obs_report_measure(id, "video-out", "open-rc", (uint64_t)(int64_t)vo_handle, "handle");

    /* eb-gvTYQcoY (0x86721036bde0bf79) */
    int h_vo = obs_module_open("libSceVideoOut");
    if (h_vo < 0) h_vo = obs_module_open("libSceVideoOut.sprx");
    typedef int (*fn_eb_gv_t)(int, void *, int);
    fn_eb_gv_t fn_eb_gv = NULL;
    if (h_vo > 0) {
        fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo, "$eb-gvTYQcoY");
        if (!fn_eb_gv) fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo, "$eb+gvTYQcoY");
        if (!fn_eb_gv) fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(h_vo, "0x86721036bde0bf79");
    }
    if (!fn_eb_gv) {
        fn_eb_gv = (fn_eb_gv_t)obs_module_symbol(OBS_HANDLE_SELF, "$eb-gvTYQcoY");
    }
    if (!fn_eb_gv && obs_address_is_callable((const void *)&sceVideoOutUnknown_86721036bde0bf79_static)) {
        fn_eb_gv = (fn_eb_gv_t)(void *)&sceVideoOutUnknown_86721036bde0bf79_static;
    }

    if (vo_handle > 0) {
        if (fn_eb_gv && obs_address_is_callable((const void *)fn_eb_gv)) {
            obs_jmp_buf guard;
            uint8_t buf0[0x20];
            memset(buf0, 0, sizeof(buf0));
            int sig = OBS_FAULT_ARM(&guard);
            int rc0 = -1;
            if (sig == 0) {
                rc0 = fn_eb_gv(vo_handle, buf0, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "eb-gvTYQcoY-zeroed", "rc", (uint64_t)(uint32_t)rc0, "code");
            obs_report_bytes(id, "eb-gvTYQcoY-zeroed", "buf-20h", 0, buf0, 0x20);

            uint8_t bufcc[0x20];
            memset(bufcc, 0xcc, sizeof(bufcc));
            sig = OBS_FAULT_ARM(&guard);
            int rccc = -1;
            if (sig == 0) {
                rccc = fn_eb_gv(vo_handle, bufcc, 0);
                obs_fault_unregister();
            } else {
                obs_fault_unregister();
            }
            obs_report_measure(id, "eb-gvTYQcoY-0xcc", "rc", (uint64_t)(uint32_t)rccc, "code");
            obs_report_bytes(id, "eb-gvTYQcoY-0xcc", "buf-20h", 0, bufcc, 0x20);
        }

        if (obs_address_is_callable((const void *)&sceVideoOutIsOutputSupported)) {
            int rc_d0a = sceVideoOutIsOutputSupported(vo_handle, (int)0xd000000a);
            obs_report_measure(id, "video-out", "supported-0xd000000a-rc", (uint64_t)(uint32_t)rc_d0a, "code");

            int rc_0 = sceVideoOutIsOutputSupported(vo_handle, 0);
            obs_report_measure(id, "video-out", "supported-0-rc", (uint64_t)(uint32_t)rc_0, "code");

            int rc_d00 = sceVideoOutIsOutputSupported(vo_handle, (int)0xd0000000);
            obs_report_measure(id, "video-out", "supported-0xd0000000-rc", (uint64_t)(uint32_t)rc_d00, "code");
        }
    }

    /* 2 & 3. SaveDataMemory2, after library is initialised */
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        (void)sceSysmoduleLoadModule(0x00B6);
    }
    int h_sd = obs_module_open("libSceSaveData_native");
    if (h_sd <= 1) {
        h_sd = obs_module_open("libSceSaveData");
    }
    typedef int (*fn_sd_init3_t)(void *);
    typedef int (*fn_sd_setup_t)(void *, void *);
    typedef int (*fn_sd_set_t)(void *);
    typedef int (*fn_sd_get_t)(void *);
    typedef int (*fn_sd_sync_t)(void *);

    fn_sd_init3_t fn_sd_init3 = NULL;
    fn_sd_setup_t fn_sd_setup = NULL;
    fn_sd_set_t fn_sd_set = NULL;
    fn_sd_get_t fn_sd_get = NULL;
    fn_sd_sync_t fn_sd_sync = NULL;

    if (h_sd > 1) {
        fn_sd_init3 = (fn_sd_init3_t)obs_module_symbol(h_sd, "TywrFKCoLGY#j#U");
        if (!fn_sd_init3) fn_sd_init3 = (fn_sd_init3_t)obs_module_symbol(h_sd, "$TywrFKCoLGY");
        if (!fn_sd_init3) fn_sd_init3 = (fn_sd_init3_t)obs_module_symbol(h_sd, "sceSaveDataInitialize3");
        fn_sd_setup = (fn_sd_setup_t)obs_module_symbol(h_sd, "oQySEUfgXRA#T#U");
        if (!fn_sd_setup) fn_sd_setup = (fn_sd_setup_t)obs_module_symbol(h_sd, "$oQySEUfgXRA");
        if (!fn_sd_setup) fn_sd_setup = (fn_sd_setup_t)obs_module_symbol(h_sd, "sceSaveDataSetupSaveDataMemory2");
        fn_sd_set = (fn_sd_set_t)obs_module_symbol(h_sd, "cduy9v4YmT4#T#U");
        if (!fn_sd_set) fn_sd_set = (fn_sd_set_t)obs_module_symbol(h_sd, "$cduy9v4YmT4");
        if (!fn_sd_set) fn_sd_set = (fn_sd_set_t)obs_module_symbol(h_sd, "sceSaveDataSetSaveDataMemory2");
        fn_sd_get = (fn_sd_get_t)obs_module_symbol(h_sd, "QwOO7vegnV8#T#U");
        if (!fn_sd_get) fn_sd_get = (fn_sd_get_t)obs_module_symbol(h_sd, "$QwOO7vegnV8");
        if (!fn_sd_get) fn_sd_get = (fn_sd_get_t)obs_module_symbol(h_sd, "sceSaveDataGetSaveDataMemory2");
        fn_sd_sync = (fn_sd_sync_t)obs_module_symbol(h_sd, "wiT9jeC7xPw#T#U");
        if (!fn_sd_sync) fn_sd_sync = (fn_sd_sync_t)obs_module_symbol(h_sd, "$wiT9jeC7xPw");
        if (!fn_sd_sync) fn_sd_sync = (fn_sd_sync_t)obs_module_symbol(h_sd, "sceSaveDataSyncSaveDataMemory");
    }
    if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
        if (!fn_sd_init3) {
            for (int h = 1; h <= 256; h++) {
                void *p = NULL;
                if (sceKernelDlsym(h, "TywrFKCoLGY#j#U", &p) == 0 && obs_address_is_callable(p)) {
                    fn_sd_init3 = (fn_sd_init3_t)p;
                    break;
                }
            }
        }
        if (!fn_sd_setup) {
            for (int h = 1; h <= 256; h++) {
                void *p = NULL;
                if (sceKernelDlsym(h, "oQySEUfgXRA#T#U", &p) == 0 && obs_address_is_callable(p)) {
                    fn_sd_setup = (fn_sd_setup_t)p;
                    break;
                }
            }
        }
        if (!fn_sd_set) {
            for (int h = 1; h <= 256; h++) {
                void *p = NULL;
                if (sceKernelDlsym(h, "cduy9v4YmT4#T#U", &p) == 0 && obs_address_is_callable(p)) {
                    fn_sd_set = (fn_sd_set_t)p;
                    break;
                }
            }
        }
        if (!fn_sd_get) {
            for (int h = 1; h <= 256; h++) {
                void *p = NULL;
                if (sceKernelDlsym(h, "QwOO7vegnV8#T#U", &p) == 0 && obs_address_is_callable(p)) {
                    fn_sd_get = (fn_sd_get_t)p;
                    break;
                }
            }
        }
        if (!fn_sd_sync) {
            for (int h = 1; h <= 256; h++) {
                void *p = NULL;
                if (sceKernelDlsym(h, "wiT9jeC7xPw#T#U", &p) == 0 && obs_address_is_callable(p)) {
                    fn_sd_sync = (fn_sd_sync_t)p;
                    break;
                }
            }
        }
    }
    if (!fn_sd_init3 && obs_address_is_callable((const void *)&sceSaveDataInitialize3_static)) {
        fn_sd_init3 = (fn_sd_init3_t)&sceSaveDataInitialize3_static;
    } else if (!fn_sd_init3 && obs_address_is_callable((const void *)&sceSaveDataInitialize3_named)) {
        fn_sd_init3 = (fn_sd_init3_t)&sceSaveDataInitialize3_named;
    }
    if (!fn_sd_setup && obs_address_is_callable((const void *)&sceSaveDataSetupSaveDataMemory2_static)) {
        fn_sd_setup = (fn_sd_setup_t)&sceSaveDataSetupSaveDataMemory2_static;
    } else if (!fn_sd_setup && obs_address_is_callable((const void *)&sceSaveDataSetupSaveDataMemory2_named)) {
        fn_sd_setup = (fn_sd_setup_t)&sceSaveDataSetupSaveDataMemory2_named;
    }
    if (!fn_sd_set && obs_address_is_callable((const void *)&sceSaveDataSetSaveDataMemory2_static)) {
        fn_sd_set = (fn_sd_set_t)&sceSaveDataSetSaveDataMemory2_static;
    } else if (!fn_sd_set && obs_address_is_callable((const void *)&sceSaveDataSetSaveDataMemory2_named)) {
        fn_sd_set = (fn_sd_set_t)&sceSaveDataSetSaveDataMemory2_named;
    }
    if (!fn_sd_get && obs_address_is_callable((const void *)&sceSaveDataGetSaveDataMemory2_static)) {
        fn_sd_get = (fn_sd_get_t)&sceSaveDataGetSaveDataMemory2_static;
    } else if (!fn_sd_get && obs_address_is_callable((const void *)&sceSaveDataGetSaveDataMemory2_named)) {
        fn_sd_get = (fn_sd_get_t)&sceSaveDataGetSaveDataMemory2_named;
    }
    if (!fn_sd_sync && obs_address_is_callable((const void *)&sceSaveDataSyncSaveDataMemory_static)) {
        fn_sd_sync = (fn_sd_sync_t)&sceSaveDataSyncSaveDataMemory_static;
    } else if (!fn_sd_sync && obs_address_is_callable((const void *)&sceSaveDataSyncSaveDataMemory_named)) {
        fn_sd_sync = (fn_sd_sync_t)&sceSaveDataSyncSaveDataMemory_named;
    }

    int rc_init_null = -1;
    if (fn_sd_init3) {
        rc_init_null = fn_sd_init3(NULL);
    }
    obs_report_measure(id, "savedata-memory2", "init3-null-rc", (uint64_t)(uint32_t)rc_init_null, "code");

    uint8_t init_param[64];
    memset(init_param, 0, sizeof(init_param));
    int rc_init_valid = -1;
    if (fn_sd_init3) {
        rc_init_valid = fn_sd_init3(init_param);
    }
    obs_report_measure(id, "savedata-memory2", "init3-param-rc", (uint64_t)(uint32_t)rc_init_valid, "code");

    /* Sync before any Setup */
    uint8_t sync_param[32];
    memset(sync_param, 0, sizeof(sync_param));
    *(uint32_t *)sync_param = (uint32_t)user;
    int rc_sync_before = -1;
    if (fn_sd_sync) {
        rc_sync_before = fn_sd_sync(sync_param);
    }
    obs_report_measure(id, "savedata-memory2", "sync-before-setup-rc", (uint64_t)(uint32_t)rc_sync_before, "code");

    /* SetupSaveDataMemory2 with 0x10000 memory size */
    uint8_t setup_param[0x40];
    memset(setup_param, 0, sizeof(setup_param));
    *(uint32_t *)(setup_param + 0) = (uint32_t)user;
    *(uint64_t *)(setup_param + 0x08) = 0x10000; /* 64KB */
    uint8_t setup_result[0x40];
    memset(setup_result, 0, sizeof(setup_result));
    int rc_setup = -1;
    if (fn_sd_setup) {
        rc_setup = fn_sd_setup(setup_param, setup_result);
    }
    obs_report_measure(id, "savedata-memory2", "setup-rc", (uint64_t)(uint32_t)rc_setup, "code");

    /* SetSaveDataMemory2: user, 16-byte buffer of 0x11..0x20 at offset 0 */
    uint8_t sample_data[16];
    for (int i = 0; i < 16; i++) sample_data[i] = (uint8_t)(0x11 + i);

    uint8_t set_param[0x40];
    memset(set_param, 0, sizeof(set_param));
    *(uint32_t *)(set_param + 0) = (uint32_t)user;
    *(uint64_t *)(set_param + 0x08) = 0; /* offset */
    *(uint64_t *)(set_param + 0x10) = 16; /* size */
    *(uint64_t *)(set_param + 0x18) = (uintptr_t)sample_data;
    int rc_set = -1;
    if (fn_sd_set) {
        rc_set = fn_sd_set(set_param);
    }
    obs_report_measure(id, "savedata-memory2", "set-rc", (uint64_t)(uint32_t)rc_set, "code");

    /* GetSaveDataMemory2: reading back */
    uint8_t readback_buf[16];
    memset(readback_buf, 0xcc, sizeof(readback_buf));
    uint8_t get_param[0x40];
    memset(get_param, 0, sizeof(get_param));
    *(uint32_t *)(get_param + 0) = (uint32_t)user;
    *(uint64_t *)(get_param + 0x08) = 0; /* offset */
    *(uint64_t *)(get_param + 0x10) = 16; /* size */
    *(uint64_t *)(get_param + 0x18) = (uintptr_t)readback_buf;
    int rc_get = -1;
    if (fn_sd_get) {
        rc_get = fn_sd_get(get_param);
    }
    obs_report_measure(id, "savedata-memory2", "get-rc", (uint64_t)(uint32_t)rc_get, "code");
    obs_report_bytes(id, "savedata-memory2", "get-16b", 0, readback_buf, 16);

    /* Sync after set */
    int rc_sync_after = -1;
    if (fn_sd_sync) {
        rc_sync_after = fn_sd_sync(sync_param);
    }
    obs_report_measure(id, "savedata-memory2", "sync-after-set-rc", (uint64_t)(uint32_t)rc_sync_after, "code");

    /* 4. Accessibility zoom follow focus */
    uint8_t zoom_out[8];
    memset(zoom_out, 0xcc, sizeof(zoom_out));
    int rc_zoom = -1;
    if (obs_address_is_callable((const void *)&sceUserServiceGetAccessibilityZoomFollowFocus)) {
        rc_zoom = sceUserServiceGetAccessibilityZoomFollowFocus(user, zoom_out);
    }
    obs_report_measure(id, "accessibility", "zoom-follow-focus-rc", (uint64_t)(uint32_t)rc_zoom, "code");
    obs_report_bytes(id, "accessibility", "zoom-follow-focus-8b", 0, zoom_out, 8);

    return obs_pass();
}

/* ---- REQ-20261012T2205Z-ap03: AudioOut2 PortSetAttributes and pacing ---- */
static obs_result check_ap03_audioout2_attributes(void) {
    const char *id = "000-boot/audioout2-port-set-attributes";
    obs_jmp_buf guard;
    int sig;

    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        obs_jmp_buf g;
        if (OBS_FAULT_ARM(&g) == 0) {
            sceSysmoduleLoadModule(0x0096);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    int h_ao = obs_module_open("libSceAudioOut2");

    typedef int (*fn_ao_init_t)(void);
    typedef int (*fn_ao_reset_param_t)(void *);
    typedef int (*fn_ao_query_mem_t)(void *, uint64_t *);
    typedef int (*fn_ao_ctx_create_t)(void *, void *, uint64_t, void **);
    typedef int (*fn_ao_user_create_t)(int32_t, void **);
    typedef int (*fn_ao_port_create_t)(void *, const void *, void **);
    typedef int (*fn_ao_port_set_attr_t)(void *, const void *, uint32_t);
    typedef int (*fn_ao_port_get_state_t)(void *, void *);
    typedef int (*fn_ao_get_queue_t)(void *, uint32_t *, uint32_t *);
    typedef int (*fn_ao_push_t)(void *, uint32_t);
    typedef int (*fn_ao_advance_t)(void *);
    typedef int (*fn_ao_port_destroy_t)(void *);
    typedef int (*fn_ao_ctx_destroy_t)(void *);
    typedef int (*fn_ao_user_destroy_t)(void *);

    fn_ao_init_t fn_init =
        obs_address_is_callable((const void *)&sceAudioOut2Initialize_static)
            ? (fn_ao_init_t)&sceAudioOut2Initialize_static
            : NULL;
    if (!fn_init && h_ao > 0)
        fn_init = (fn_ao_init_t)obs_module_symbol(h_ao, "sceAudioOut2Initialize");

    fn_ao_reset_param_t fn_reset_param =
        obs_address_is_callable((const void *)&sceAudioOut2ContextResetParam_static)
            ? (fn_ao_reset_param_t)&sceAudioOut2ContextResetParam_static
            : NULL;
    if (!fn_reset_param && h_ao > 0)
        fn_reset_param =
            (fn_ao_reset_param_t)obs_module_symbol(h_ao, "$t5YrizufpQc");

    fn_ao_query_mem_t fn_query_mem =
        obs_address_is_callable((const void *)&sceAudioOut2ContextQueryMemory_static)
            ? (fn_ao_query_mem_t)&sceAudioOut2ContextQueryMemory_static
            : NULL;
    if (!fn_query_mem && h_ao > 0)
        fn_query_mem =
            (fn_ao_query_mem_t)obs_module_symbol(h_ao, "$pDmme7Bgm6E");

    fn_ao_ctx_create_t fn_ctx_create =
        obs_address_is_callable((const void *)&sceAudioOut2ContextCreate_static)
            ? (fn_ao_ctx_create_t)&sceAudioOut2ContextCreate_static
            : NULL;
    if (!fn_ctx_create && h_ao > 0)
        fn_ctx_create =
            (fn_ao_ctx_create_t)obs_module_symbol(h_ao, "$0x6o1VVAYSY");

    fn_ao_user_create_t fn_user_create =
        obs_address_is_callable((const void *)&sceAudioOut2UserCreate_static)
            ? (fn_ao_user_create_t)&sceAudioOut2UserCreate_static
            : NULL;
    if (!fn_user_create && h_ao > 0)
        fn_user_create =
            (fn_ao_user_create_t)obs_module_symbol(h_ao, "$xywYcRB7nbQ");

    fn_ao_port_create_t fn_port_create =
        obs_address_is_callable((const void *)&sceAudioOut2PortCreate_static)
            ? (fn_ao_port_create_t)&sceAudioOut2PortCreate_static
            : NULL;
    if (!fn_port_create && h_ao > 0)
        fn_port_create =
            (fn_ao_port_create_t)obs_module_symbol(h_ao, "$A89PZmqwrSQ");

    fn_ao_port_set_attr_t fn_port_set_attr =
        obs_address_is_callable((const void *)&sceAudioOut2PortSetAttributes_static)
            ? (fn_ao_port_set_attr_t)&sceAudioOut2PortSetAttributes_static
            : NULL;
    if (!fn_port_set_attr && h_ao > 0)
        fn_port_set_attr =
            (fn_ao_port_set_attr_t)obs_module_symbol(h_ao, "$8XTArSPyWHk");

    fn_ao_port_get_state_t fn_port_get_state =
        obs_address_is_callable((const void *)&sceAudioOut2PortGetState_static)
            ? (fn_ao_port_get_state_t)&sceAudioOut2PortGetState_static
            : NULL;
    if (!fn_port_get_state && h_ao > 0)
        fn_port_get_state =
            (fn_ao_port_get_state_t)obs_module_symbol(h_ao, "$gatEUKG+Ea4");

    fn_ao_get_queue_t fn_get_queue =
        obs_address_is_callable(
            (const void *)&sceAudioOut2ContextGetQueueLevel_static)
            ? (fn_ao_get_queue_t)(void *)&sceAudioOut2ContextGetQueueLevel_static
            : NULL;
    if (!fn_get_queue && h_ao > 0)
        fn_get_queue =
            (fn_ao_get_queue_t)obs_module_symbol(h_ao, "$R7d0F1g2qsU");

    fn_ao_push_t fn_push =
        obs_address_is_callable((const void *)&sceAudioOut2ContextPush_static)
            ? (fn_ao_push_t)(void *)&sceAudioOut2ContextPush_static
            : NULL;
    if (!fn_push && h_ao > 0)
        fn_push = (fn_ao_push_t)obs_module_symbol(h_ao, "$aII9h5nli9U");

    fn_ao_advance_t fn_advance =
        obs_address_is_callable((const void *)&sceAudioOut2ContextAdvance_static)
            ? (fn_ao_advance_t)&sceAudioOut2ContextAdvance_static
            : NULL;
    if (!fn_advance && h_ao > 0)
        fn_advance = (fn_ao_advance_t)obs_module_symbol(h_ao, "$PE2zHMqLSHs");

    fn_ao_port_destroy_t fn_port_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2PortDestroy_static)
            ? (fn_ao_port_destroy_t)&sceAudioOut2PortDestroy_static
            : NULL;
    if (!fn_port_destroy && h_ao > 0)
        fn_port_destroy =
            (fn_ao_port_destroy_t)obs_module_symbol(h_ao, "$cd+Rtw+D1x8");

    fn_ao_ctx_destroy_t fn_ctx_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2ContextDestroy_static)
            ? (fn_ao_ctx_destroy_t)&sceAudioOut2ContextDestroy_static
            : NULL;
    if (!fn_ctx_destroy && h_ao > 0)
        fn_ctx_destroy =
            (fn_ao_ctx_destroy_t)obs_module_symbol(h_ao, "$on6ZH7Abo10");

    fn_ao_user_destroy_t fn_user_destroy =
        obs_address_is_callable((const void *)&sceAudioOut2UserDestroy_static)
            ? (fn_ao_user_destroy_t)&sceAudioOut2UserDestroy_static
            : NULL;
    if (!fn_user_destroy && h_ao > 0)
        fn_user_destroy =
            (fn_ao_user_destroy_t)obs_module_symbol(h_ao, "$6vY70ydXpiE");

    if (fn_init)
        fn_init();

    /* Setup context with PPSA21564's parameters: {0x14, 0x200, 0, 1, 0x200, 1}
     */
    uint8_t ctx_param[64];
    memset(ctx_param, 0, sizeof(ctx_param));
    if (fn_reset_param)
        fn_reset_param(ctx_param);
    *(uint32_t *)(void *)(ctx_param + 0x00) = 0x14u;
    *(uint32_t *)(void *)(ctx_param + 0x04) = 0x200u;
    *(uint32_t *)(void *)(ctx_param + 0x08) = 0u;
    *(uint32_t *)(void *)(ctx_param + 0x0c) = 1u;
    *(uint32_t *)(void *)(ctx_param + 0x10) = 0x200u;
    *(uint32_t *)(void *)(ctx_param + 0x14) = 1u;

    uint64_t mem_size = 0;
    if (fn_query_mem)
        fn_query_mem(ctx_param, &mem_size);

    static _Alignas(4096) uint8_t s_ap03_ctx_mem[2 * 1024 * 1024];
    uint64_t alloc_size =
        (mem_size > 0 && mem_size <= sizeof(s_ap03_ctx_mem)) ? mem_size : 0x10000u;
    void *context = NULL;
    if (fn_ctx_create)
        fn_ctx_create(ctx_param, s_ap03_ctx_mem, alloc_size, &context);

    void *user = NULL;
    if (fn_user_create)
        fn_user_create(0xff, &user);

    /* 8-channel type-0 port with first word 0 */
    uint8_t pparam[64];
    memset(pparam, 0, sizeof(pparam));
    *(uint32_t *)(void *)(pparam + 0x00) = 0;
    *(uint32_t *)(void *)(pparam + 0x04) = (uint32_t)(8 << 8); /* 8 ch */
    *(uint32_t *)(void *)(pparam + 0x08) = 48000;
    *(uint32_t *)(void *)(pparam + 0x0c) = 0; /* type 0 */
    *(uint64_t *)(void *)(pparam + 0x10) = (uint64_t)(uintptr_t)user;

    void *port = NULL;
    int rc_port = -1;
    if (fn_port_create)
        rc_port = fn_port_create(context, pparam, &port);
    obs_report_measure(id, "PortCreate", "rc", (uint64_t)(uint32_t)rc_port,
                       "code");

    /* PCM buffer: 512 frames x 8 channels of float 0.25 */
    static float s_ap03_pcm[512 * 8];
    for (size_t i = 0; i < 512 * 8; i++)
        s_ap03_pcm[i] = 0.25f;
    static uint64_t s_ap03_pcm_ptr = 0;
    s_ap03_pcm_ptr = (uint64_t)(uintptr_t)s_ap03_pcm;

    static float s_ap03_gains[8] = {1.0f, 1.0f, 1.0f, 1.0f,
                                    1.0f, 1.0f, 1.0f, 1.0f};
    uint32_t mode1 = 1;
    uint32_t mode2 = 2;

    struct obs_ao2_attr_rec {
        uint32_t id;
        uint32_t pad;
        uint64_t value_ptr;
        uint64_t size;
    };

    /* 1. count 3: {0, &pcm_ptr, 8}, {1, &gains, 32}, {5, &mode, 4} with mode 1
     */
    struct obs_ao2_attr_rec recs3_mode1[3] = {
        {0, 0, (uint64_t)(uintptr_t)&s_ap03_pcm_ptr, 8},
        {1, 0, (uint64_t)(uintptr_t)s_ap03_gains, 32},
        {5, 0, (uint64_t)(uintptr_t)&mode1, 4}};
    int rc_set1 = -1;
    if (fn_port_set_attr && port) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set1 = fn_port_set_attr(port, recs3_mode1, 3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "PortSetAttributes-count3-mode1", "rc",
                       (uint64_t)(uint32_t)rc_set1, "code");

    /* 2. count 3 with mode 2 */
    struct obs_ao2_attr_rec recs3_mode2[3] = {
        {0, 0, (uint64_t)(uintptr_t)&s_ap03_pcm_ptr, 8},
        {1, 0, (uint64_t)(uintptr_t)s_ap03_gains, 32},
        {5, 0, (uint64_t)(uintptr_t)&mode2, 4}};
    int rc_set2 = -1;
    if (fn_port_set_attr && port) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set2 = fn_port_set_attr(port, recs3_mode2, 3);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "PortSetAttributes-count3-mode2", "rc",
                       (uint64_t)(uint32_t)rc_set2, "code");

    /* 3. count 2: {0, &pcm_ptr, 8}, {1, &gains, 32} */
    struct obs_ao2_attr_rec recs2[2] = {
        {0, 0, (uint64_t)(uintptr_t)&s_ap03_pcm_ptr, 8},
        {1, 0, (uint64_t)(uintptr_t)s_ap03_gains, 32}};
    int rc_set3 = -1;
    if (fn_port_set_attr && port) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_set3 = fn_port_set_attr(port, recs2, 2);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "PortSetAttributes-count2", "rc",
                       (uint64_t)(uint32_t)rc_set3, "code");

    /* 4. After (1), the blocking pump for one second:
     * PortGetState, PortSetAttributes as in (1), ContextAdvance(0),
     * ContextPush(0, 1): iterations, the first 10 pushes' TSC ticks, and
     * ContextGetQueueLevel after each. */
    uint64_t tsc_freq =
        sceKernelGetTscFrequency ? sceKernelGetTscFrequency() : 0;
    uint8_t state_buf[0x100];
    uint64_t blk_ticks[10] = {0};
    uint32_t blk_q[10] = {0};
    uint32_t blk_a[10] = {0};
    uint64_t iterations = 0;

    if (tsc_freq > 0 && fn_push && context && port) {
        uint64_t t_start = sceKernelReadTsc ? sceKernelReadTsc() : 0;
        uint64_t t_end = t_start + tsc_freq;
        while ((sceKernelReadTsc ? sceKernelReadTsc() : 0) < t_end) {
            if (fn_port_get_state)
                fn_port_get_state(port, state_buf);
            if (fn_port_set_attr)
                fn_port_set_attr(port, recs3_mode1, 3);
            if (fn_advance)
                fn_advance(context);

            if (iterations < 10) {
                uint64_t tb = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                fn_push(context, 1);
                uint64_t ta = sceKernelReadTsc ? sceKernelReadTsc() : 0;
                blk_ticks[iterations] = ta - tb;
                uint32_t q = 0, a = 0;
                if (fn_get_queue)
                    fn_get_queue(context, &q, &a);
                blk_q[iterations] = q;
                blk_a[iterations] = a;
            } else {
                fn_push(context, 1);
            }
            iterations++;
        }
    }
    obs_report_measure(id, "blocking-pump", "iterations", iterations, "count");
    for (int p = 0; p < 10 && (uint64_t)p < iterations; p++) {
        char tag[64];
        snprintf(tag, sizeof(tag), "push-%d", p);
        obs_report_measure(id, tag, "ticks", blk_ticks[p], "ticks");
        obs_report_measure(id, tag, "queued", (uint64_t)blk_q[p], "count");
        obs_report_measure(id, tag, "available", (uint64_t)blk_a[p], "count");
    }

    /* Cleanup */
    if (fn_port_destroy && port)
        fn_port_destroy(port);
    if (fn_ctx_destroy && context)
        fn_ctx_destroy(context);
    if (fn_user_destroy && user)
        fn_user_destroy(user);

    return obs_pass();
}

/* ---- REQ-20261012T2235Z-aw03: AMPR command buffer queries and submission ----
 */
static obs_result check_aw03_ampr_command_buffer(void) {
    const char *id = "000-boot/ampr-command-buffer-queries";
    obs_jmp_buf guard;
    int sig;

    int h_ampr = obs_module_open("libSceAmpr");
    if (h_ampr < 0)
        h_ampr = obs_module_open("libSceAmpr.sprx");
    int h_kernel = obs_module_open("libkernel");

    const void *fn_ctor = (const void *)&sceAmprCommandBufferConstructor_static;
    if (!obs_address_is_callable(fn_ctor) && h_ampr > 0) {
        fn_ctor = obs_module_symbol(h_ampr, "sceAmprCommandBufferConstructor");
        if (!fn_ctor)
            fn_ctor = obs_module_symbol(h_ampr, "$8aI7R7WaOlc");
    }

    const void *fn_setbuf = (const void *)&sceAmprCommandBufferSetBuffer_static;
    if (!obs_address_is_callable(fn_setbuf))
        fn_setbuf = (const void *)&sceAmprCommandBufferSetBuffer;
    if (!obs_address_is_callable(fn_setbuf) && h_ampr > 0) {
        fn_setbuf = obs_module_symbol(h_ampr, "sceAmprCommandBufferSetBuffer");
        if (!fn_setbuf)
            fn_setbuf = obs_module_symbol(h_ampr, "$N-FSPA4S3nI");
    }

    typedef int (*fn_write_addr_t)(void *, void *, uint32_t, uint32_t);
    fn_write_addr_t fn_write_addr =
        obs_address_is_callable(
            (const void *)&sceAmprCommandBufferWriteAddress_04_00_static)
            ? (fn_write_addr_t)&sceAmprCommandBufferWriteAddress_04_00_static
            : NULL;
    if (!fn_write_addr && h_ampr > 0) {
        fn_write_addr =
            (fn_write_addr_t)obs_module_symbol(h_ampr, "$j0+3uJMxYJY");
        if (!fn_write_addr)
            fn_write_addr = (fn_write_addr_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferWriteAddress_04_00");
    }

    typedef uint32_t (*fn_get_off_t)(void *);
    fn_get_off_t fn_get_offset =
        obs_address_is_callable(
            (const void *)&sceAmprCommandBufferGetCurrentOffset_static)
            ? (fn_get_off_t)(void *)&sceAmprCommandBufferGetCurrentOffset_static
            : NULL;
    if (!fn_get_offset && h_ampr > 0) {
        fn_get_offset =
            (fn_get_off_t)obs_module_symbol(h_ampr, "$GnxKOHEawhk");
        if (!fn_get_offset)
            fn_get_offset = (fn_get_off_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferGetCurrentOffset");
    }

    typedef uint32_t (*fn_get_num_t)(void *);
    fn_get_num_t fn_get_num_cmds =
        obs_address_is_callable(
            (const void *)&sceAmprCommandBufferGetNumCommands_static)
            ? (fn_get_num_t)(void *)&sceAmprCommandBufferGetNumCommands_static
            : NULL;
    if (!fn_get_num_cmds && h_ampr > 0) {
        fn_get_num_cmds =
            (fn_get_num_t)obs_module_symbol(h_ampr, "$gzndltBEzWc");
        if (!fn_get_num_cmds)
            fn_get_num_cmds = (fn_get_num_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferGetNumCommands");
    }

    typedef uint32_t (*fn_get_sz_t)(void *);
    fn_get_sz_t fn_get_size =
        obs_address_is_callable(
            (const void *)&sceAmprCommandBufferGetSize_static)
            ? (fn_get_sz_t)(void *)&sceAmprCommandBufferGetSize_static
            : NULL;
    if (!fn_get_size && h_ampr > 0) {
        fn_get_size = (fn_get_sz_t)obs_module_symbol(h_ampr, "$tZDDEo2tE5k");
        if (!fn_get_size)
            fn_get_size = (fn_get_sz_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferGetSize");
    }

    typedef int (*fn_clear_buf_t)(void *);
    fn_clear_buf_t fn_clear_buf =
        obs_address_is_callable(
            (const void *)&sceAmprCommandBufferClearBuffer_static)
            ? (fn_clear_buf_t)(void *)&sceAmprCommandBufferClearBuffer_static
            : NULL;
    if (!fn_clear_buf && h_ampr > 0) {
        fn_clear_buf =
            (fn_clear_buf_t)obs_module_symbol(h_ampr, "$ULvXMDz56po");
        if (!fn_clear_buf)
            fn_clear_buf = (fn_clear_buf_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferClearBuffer");
    }

    typedef int (*fn_reset_t)(void *);
    fn_reset_t fn_reset =
        obs_address_is_callable((const void *)&sceAmprCommandBufferReset_alt)
            ? (fn_reset_t)(void *)&sceAmprCommandBufferReset_alt
            : (obs_address_is_callable(
                   (const void *)&sceAmprCommandBufferReset_static)
                   ? (fn_reset_t)(void *)&sceAmprCommandBufferReset_static
                   : NULL);
    if (!fn_reset && h_ampr > 0) {
        fn_reset = (fn_reset_t)obs_module_symbol(h_ampr, "$voD27PUOpG0");
        if (!fn_reset)
            fn_reset = (fn_reset_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferReset");
    }

    typedef int (*fn_write_eq_t)(void *, uint64_t, uint64_t, uint64_t);
    fn_write_eq_t fn_write_eq =
        obs_address_is_callable(
            (const void *)
                &sceAmprCommandBufferWriteKernelEventQueue_04_00_static)
            ? (fn_write_eq_t)
                  &sceAmprCommandBufferWriteKernelEventQueue_04_00_static
            : NULL;
    if (!fn_write_eq && h_ampr > 0) {
        fn_write_eq =
            (fn_write_eq_t)obs_module_symbol(h_ampr, "$H896Pt-yB4I");
        if (!fn_write_eq)
            fn_write_eq = (fn_write_eq_t)obs_module_symbol(
                h_ampr, "sceAmprCommandBufferWriteKernelEventQueue_04_00");
    }

    const void *fn_apr_submit =
        obs_address_is_callable(
            (const void *)&sceKernelAprSubmitCommandBufferAndGetResult_static)
            ? (const void *)&sceKernelAprSubmitCommandBufferAndGetResult_static
            : NULL;
    if (!fn_apr_submit && h_kernel > 0) {
        fn_apr_submit = obs_module_symbol(
            h_kernel, "sceKernelAprSubmitCommandBufferAndGetResult");
        if (!fn_apr_submit)
            fn_apr_submit = obs_module_symbol(h_kernel, "$ASoW5WE-UPo");
    }

    const void *fn_apr_wait =
        obs_address_is_callable(
            (const void *)&sceKernelAprWaitCommandBuffer_static)
            ? (const void *)&sceKernelAprWaitCommandBuffer_static
            : NULL;
    if (!fn_apr_wait && h_kernel > 0) {
        fn_apr_wait =
            obs_module_symbol(h_kernel, "sceKernelAprWaitCommandBuffer");
        if (!fn_apr_wait)
            fn_apr_wait = obs_module_symbol(h_kernel, "$rqwFKI4PAiM");
    }

    const void *fn_read_file = (const void *)&sceAmprAprCommandBufferReadFile_static;
    if (!obs_address_is_callable(fn_read_file) && h_ampr > 0)
        fn_read_file = obs_module_symbol(h_ampr, "sceAmprAprCommandBufferReadFile");
    if (!obs_address_is_callable(fn_read_file) && h_ampr > 0)
        fn_read_file = obs_module_symbol(h_ampr, "$mQ16-QdKv7k");

    /* REQ-aw02 setup: constructed object, 0x1000 buffer bound, filled 0xcc */
    static _Alignas(64) uint8_t obj[0x200];
    static _Alignas(64) uint8_t cbuf[0x200];
    static _Alignas(64) uint8_t fresh_buf[0x1000];
    memset(obj, 0xcc, sizeof(obj));
    memset(cbuf, 0xcc, sizeof(cbuf));
    memset(fresh_buf, 0xcc, sizeof(fresh_buf));

    if (fn_ctor) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            ((void (*)(void *, void *, void *, uint64_t, void *, uint64_t))
                 fn_ctor)(obj, obj, cbuf, 0, cbuf, 0x47d);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_setbuf) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            ((int (*)(void *, void *, uint32_t, uint64_t, uint64_t, uint64_t))
                 fn_setbuf)(obj, fresh_buf, 0x1000, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }

    /* 1. Before any command: queries */
    uint64_t off0 = fn_get_offset ? fn_get_offset(obj) : 0;
    uint64_t num0 = fn_get_num_cmds ? fn_get_num_cmds(obj) : 0;
    uint64_t sz0 = fn_get_size ? fn_get_size(obj) : 0;
    obs_report_measure(id, "before-cmd", "offset", off0, "bytes");
    obs_report_measure(id, "before-cmd", "num-commands", num0, "count");
    obs_report_measure(id, "before-cmd", "size", sz0, "bytes");

    /* 2. WriteAddress_04_00(obj, slot, 0x12345678, 0) with slot 8-aligned 8
     * bytes filled 0xcc */
    static _Alignas(8) uint8_t slot[8];
    memset(slot, 0xcc, sizeof(slot));
    int rc_wa = -1;
    if (fn_write_addr) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wa = fn_write_addr(obj, slot, 0x12345678, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "write-address", "rc", (uint64_t)(uint32_t)rc_wa,
                       "code");

    uint64_t off1 = fn_get_offset ? fn_get_offset(obj) : 0;
    uint64_t num1 = fn_get_num_cmds ? fn_get_num_cmds(obj) : 0;
    uint64_t sz1 = fn_get_size ? fn_get_size(obj) : 0;
    obs_report_measure(id, "after-write-address", "offset", off1, "bytes");
    obs_report_measure(id, "after-write-address", "num-commands", num1, "count");
    obs_report_measure(id, "after-write-address", "size", sz1, "bytes");

    /* 3. Submit buffer with lone WriteAddress */
    uint64_t rc_submit = 0xffffffffffffffffULL;
    if (fn_apr_submit) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_submit = ((uint64_t (*)(void *, void *))fn_apr_submit)(obj, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "submit-nowrite", "rc", rc_submit, "code");

    uint64_t rc_wait = 0xffffffffffffffffULL;
    if (fn_apr_wait && (int64_t)rc_submit >= 0) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wait = ((uint64_t (*)(void *, uint64_t))fn_apr_wait)(obj,
                                                                    1000000ULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "wait-nowrite", "rc", rc_wait, "code");

    /* REQ-20261013T1730Z-rf01 item 1:
     * ReadFile of a small file, then WriteAddress_04_00, submit and wait */
    if (fn_reset) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            fn_reset(obj);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    if (fn_setbuf) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            ((int (*)(void *, void *, uint32_t, uint64_t, uint64_t, uint64_t))
                 fn_setbuf)(obj, fresh_buf, 0x1000, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    /* 1. Resolve /app0/sce_sys/param.json */
    const char *resolve_paths[1] = {"/app0/sce_sys/param.json"};
    uint32_t file_id = 0;
    uint64_t file_size = 0;
    int32_t file_status = -1;
    int rc_resolve = -1;
    if (obs_address_is_callable(
            (const void *)&sceKernelAprResolveFilepathsToIdsAndFileSizes_static)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_resolve =
                sceKernelAprResolveFilepathsToIdsAndFileSizes_static(
                    resolve_paths, 1, &file_id, &file_size, &file_status, NULL);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "apr-resolve", "rc", (uint64_t)(int64_t)rc_resolve,
                       "code");
    obs_report_measure(id, "apr-resolve", "file-id", (uint64_t)file_id, "id");
    obs_report_measure(id, "apr-resolve", "file-size", file_size, "bytes");
    obs_report_measure(id, "apr-resolve", "file-status",
                       (uint64_t)(int64_t)file_status, "status");

    /* 2. ReadFile with resolved file_id (7 arguments: obj, obj+0x18, obj+0x20, id, dst, 16, 0) */
    uint8_t dst[16];
    memset(dst, 0xcc, sizeof(dst));
    uint64_t rc_read = 0xffffffffffffffffULL;
    typedef int (*fn_read_file_7_t)(void *, void *, void *, uint32_t, void *,
                                    uint64_t, uint64_t);
    fn_read_file_7_t fn_rf7 = (fn_read_file_7_t)fn_read_file;
    if (fn_rf7 && obs_address_is_callable((const void *)fn_rf7)) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_read = (uint64_t)(int64_t)fn_rf7(
                obj, (uint8_t *)obj + 0x18, (uint8_t *)obj + 0x20, file_id, dst, 16, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "apr-read-file", "rc", rc_read, "code");
    obs_report_bytes(id, "apr-read-file", "dst-16b", 0, dst, 16);

    /* 3. WriteAddress_04_00 */
    memset(slot, 0xcc, sizeof(slot));
    int rc_wa2 = -1;
    if (fn_write_addr) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_wa2 = fn_write_addr(obj, slot, 0x12345678, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "write-address-after-read", "rc",
                       (uint64_t)(uint32_t)rc_wa2, "code");

    /* 4. Submit buffer and wait */
    uint64_t apr_out_result = 0;
    uint32_t apr_wait_id = 0;
    uint64_t rc_submit_read = 0xffffffffffffffffULL;
    if (fn_apr_submit) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_submit_read =
                ((uint64_t (*)(void *, uint32_t, void *, void *))fn_apr_submit)(
                    obj, 0, &apr_out_result, &apr_wait_id);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "submit-after-read", "rc", rc_submit_read, "code");
    obs_report_measure(id, "submit-after-read", "wait-id",
                       (uint64_t)apr_wait_id, "id");
    obs_report_measure(id, "submit-after-read", "out-result", apr_out_result,
                       "code");

    uint64_t rc_wait_read = 0xffffffffffffffffULL;
    if (fn_apr_wait && (int64_t)rc_submit_read >= 0) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            uint32_t wid = apr_wait_id ? apr_wait_id : (uint32_t)(uintptr_t)obj;
            rc_wait_read = ((uint64_t (*)(uint32_t))fn_apr_wait)(wid);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "wait-after-read", "rc", rc_wait_read, "code");
    obs_report_bytes(id, "slot-after-wait", "slot-8b", 0, slot, 8);

    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        sceKernelUsleep(10000); /* 10ms */
    }
    obs_report_bytes(id, "slot-after-sleep10ms", "slot-8b", 0, slot, 8);

    /* 4. ClearBuffer and Reset */
    int rc_clear = -1;
    if (fn_clear_buf) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_clear = fn_clear_buf(obj);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "clear-buffer", "rc", (uint64_t)(uint32_t)rc_clear,
                       "code");

    int rc_reset = -1;
    if (fn_reset) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_reset = fn_reset(obj);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "reset", "rc", (uint64_t)(uint32_t)rc_reset, "code");

    uint64_t off2 = fn_get_offset ? fn_get_offset(obj) : 0;
    uint64_t num2 = fn_get_num_cmds ? fn_get_num_cmds(obj) : 0;
    uint64_t sz2 = fn_get_size ? fn_get_size(obj) : 0;
    obs_report_measure(id, "after-reset", "offset", off2, "bytes");
    obs_report_measure(id, "after-reset", "num-commands", num2, "count");
    obs_report_measure(id, "after-reset", "size", sz2, "bytes");

    /* 5. WriteKernelEventQueue_04_00 */
    int bound_weq = (fn_write_eq != NULL &&
                     obs_address_is_callable((const void *)fn_write_eq));
    obs_report_measure(id, "sceAmprCommandBufferWriteKernelEventQueue_04_00",
                       "bound", (uint64_t)bound_weq, "bool");
    int rc_weq = -1;
    if (bound_weq) {
        sig = OBS_FAULT_ARM(&guard);
        if (sig == 0) {
            rc_weq = fn_write_eq(obj, 0, 0, 0);
            obs_fault_unregister();
        } else {
            obs_fault_unregister();
        }
    }
    obs_report_measure(id, "WriteKernelEventQueue", "rc",
                       (uint64_t)(uint32_t)rc_weq, "code");

    return obs_pass();
}

#endif /* OBS_WAVE1_CHECKS_INC */

