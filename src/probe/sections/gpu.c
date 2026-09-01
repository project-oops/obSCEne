/*
 * GPU compute: what the device actually computes.
 *
 * Every other section asks what a *function* returns. This asks what an *instruction*
 * computes - it dispatches a compute shader over known inputs and reads the result bits
 * back. That is the one question an emulator's shader translation must be checked
 * against and cannot answer about itself, and it is the whole reason the GPU work
 * exists.
 *
 * # Two checks: one verdict, one sweep
 *
 * `pipeline` runs a kernel whose answer is exact - double every lane - and asserts it.
 * Its job is to prove the instrument before the instrument is trusted: if `x*2` comes
 * back wrong, the fault is in the dispatch path, not the platform. Same role as the
 * census control and the responsiveness "must differ" pair.
 *
 * `kernels` runs every shader in `OBS_GPU_KERNELS` over one shared vector of edge-y
 * inputs and asserts nothing - it emits the input and output bits of each lane as
 * observations. Whether the hardware's reciprocal, sine or half-float rounding matches
 * an emulator's is a diff the recipient performs, not a pass this program can judge.
 * The device name rides on every record (`gpudev`), so a software result (llvmpipe) is
 * never mistaken for silicon.
 *
 * The kernel list is generated: drop a `.comp` in `src/shaders`, regenerate, and it is
 * swept automatically. No edit here scales the corpus - the specificity lives in the
 * shaders, which is where it belongs.
 *
 * # What these kernels do and do not pin down
 *
 * `1.0/x`, `inversesqrt`, `sin` and the rest are lowered by the driver's shader
 * compiler, which on RDNA2 emits the approximate instruction plus refinement. So this
 * measures the device's *operation* as the driver renders it, not the bare ISA opcode
 * in isolation - a real hardware behaviour to diff against, but not the same as `v_rcp`
 * alone. Pinning a single opcode needs the console's own shader binary path
 * (`gpu_gnm.c`), where the submitted ISA is under our control; it is deferred, not
 * forgotten.
 *
 * # Off unless asked for, and backed per target
 *
 * The two checks exist in every build; under `OBS_GPU` they dispatch, and without it
 * they report a skip - so the capability never silently vanishes and the check count
 * does not move with the flag. The backend behind `obs_gpu_*` is chosen by the build:
 * Vulkan on the host and the Deck, a refusing stub on the console until its submission
 * format is confirmed. A build whose backend is absent skips rather than faults.
 */

#include "obscene/harness.h"
#include "obscene/report.h"
#include "obscene/sections.h"

#if defined(OBS_GPU)

#include "obscene/gpu.h"
#include "obscene/gpu_shaders.gen.h"

/* Float and its bits, by union - well-defined in C and needing no libc, so the same
 * code serves the freestanding module build and the native host build. */
static uint32_t f32_bits(float f) {
    union {
        float f;
        uint32_t u;
    } x;
    x.f = f;
    return x.u;
}

static float bits_f32(uint32_t u) {
    union {
        float f;
        uint32_t u;
    } x;
    x.u = u;
    return x.f;
}

/* ---- dispatch a compiled-in kernel by name (for the socket's `gpu` verb)
 * ---------------
 *
 * The kernel table is the generated list, expanded here so a name maps to its shader
 * and arity. One home for the list still - the same macro the sweep uses. */
static int obs_gpu_streq(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return a[i] == b[i];
}

unsigned int obs_gpu_arity(const char *name) {
#define X(kname, arity)                                                                \
    if (obs_gpu_streq(name, #kname)) {                                                 \
        return (unsigned int)(arity);                                                  \
    }
    OBS_GPU_KERNELS(X)
#undef X
    return 0u;
}

int obs_gpu_dispatch_named(const char *name, uint32_t *data, unsigned int count) {
#define X(kname, arity)                                                                \
    if (obs_gpu_streq(name, #kname)) {                                                 \
        return obs_gpu_run(obs_shader_##kname, sizeof(obs_shader_##kname), data,       \
                           count);                                                     \
    }
    OBS_GPU_KERNELS(X)
#undef X
    return -1;
}

/* One vector, every kernel. Chosen to reach the edges the operations disagree on rather
 * than to be tidy: signed zero (reciprocal's infinities), negatives (the domain edge of
 * sqrt and log), a fraction that is not representable (1/3), pi (sine's range
 * reduction), and values near the extremes of the range. Finite throughout, so the
 * `double` self-check stays exactly predictable. */
/* Every value finite, so the `double` self-check stays exactly predictable (NaN would
 * break its comparison and lives only in the multi-operand edge set). Beyond the
 * everyday values, the tail reaches the places the transcendentals strain: a subnormal
 * and the smallest normal (flush-to-zero behaviour), values near the float extremes
 * (overflow of exp and sinh, and `double` itself overflowing to infinity - which C and
 * the device agree on), a large angle that forces sine's range reduction to work, and
 * values just off 1.0 where the inverse-trig domain ends. */
#define OBS_GPU_LANES 24u
static const float obs_gpu_inputs[OBS_GPU_LANES] = {
    0.0f,    -0.0f,      1.0f,      -1.0f,       2.0f,       0.5f,
    3.0f,    0.3333333f, -2.5f,     3.1415927f,  9.0f,       100.0f,
    1e-20f,  1e20f,      0.1f,      -0.1f,       1e-40f,     1.1754944e-38f,
    3.4e38f, -3.4e38f,   100000.0f, 0.99999994f, 1.0000001f, -0.99999994f};

/* The operand set for multi-operand kernels, as raw bit patterns.
 *
 * Bits rather than float literals because the interesting operands cannot be written as
 * finite decimals: the two infinities and a NaN are exactly the values that make
 * min/max propagation and division domains diverge, and signed zero is where fma and
 * pow part ways. A multi-operand kernel is swept over the full cross-product of these,
 * so binary sees every pair and ternary every triple - small (a handful each way) but
 * aimed at the edges. */
#define OBS_GPU_EDGES 7u
static const uint32_t obs_gpu_edge[OBS_GPU_EDGES] = {
    0x00000000u, /* +0 */
    0x80000000u, /* -0 */
    0x3f800000u, /* 1 */
    0x40400000u, /* 3 */
    0x7f800000u, /* +inf */
    0xff800000u, /* -inf */
    0x7fc00000u, /* NaN */
};

/* Input patterns for the bit-operation kernels, as raw 32-bit words.
 *
 * A bit operation has nothing to say about a float value; it works on the pattern.
 * Feeding it the float input vector clusters every operand's bits where floats put them
 * - a sign bit, a biased exponent, a mantissa - and never exercises an all-ones word, a
 * lone high bit, or an alternating pattern. These are chosen for the opposite: the
 * empty and full words, single bits at each end, the half-word boundaries the fields
 * sit on, and the alternating and arbitrary patterns that catch an off-by-one in a
 * shift or a mask. Must be no longer than OBS_GPU_LANES, since the unary sweep's buffer
 * is sized for that. */
#define OBS_GPU_BITS 16u
static const uint32_t obs_gpu_bits[OBS_GPU_BITS] = {
    0x00000000u, 0xffffffffu, 0x00000001u, 0x80000000u, 0x7fffffffu, 0x0000ffffu,
    0xffff0000u, 0x0000ff00u, 0xaaaaaaaau, 0x55555555u, 0x00008000u, 0x00000080u,
    0x12345678u, 0xdeadbeefu, 0x00ffff00u, 0xf0f0f0f0u,
};

/* Operand sets for the packing conversions, which need values in a range, not bit
 * edges.
 *
 * packUnorm/packSnorm clamp to [0,1] and [-1,1] and scale, so the interesting operands
 * are the ends of those ranges, a fraction or two inside, and one value past each end
 * to exercise the clamp. Written as the bit patterns of those floats - the
 * multi-operand path lays out raw words, and a decimal literal would need converting
 * per lane. */
#define OBS_GPU_UNORM 6u
static const uint32_t obs_gpu_unorm[OBS_GPU_UNORM] = {
    0xbf000000u, /* -0.5, past the floor */
    0x00000000u, /* 0.0 */
    0x3e800000u, /* 0.25 */
    0x3f000000u, /* 0.5 */
    0x3f800000u, /* 1.0 */
    0x40000000u, /* 2.0, past the ceiling */
};
#define OBS_GPU_SNORM 7u
static const uint32_t obs_gpu_snorm[OBS_GPU_SNORM] = {
    0xc0000000u, /* -2.0, past the floor */
    0xbf800000u, /* -1.0 */
    0xbf000000u, /* -0.5 */
    0x00000000u, /* 0.0 */
    0x3f000000u, /* 0.5 */
    0x3f800000u, /* 1.0 */
    0x40000000u, /* 2.0, past the ceiling */
};

/* The unary kernels whose operand is a bit pattern, not a float. Named here so the
 * sweep feeds each the inputs that actually exercise it - the bit vector for these, the
 * float edges for the rest. The unpack conversions belong here too: they take a packed
 * 32-bit word. Kept as one list so adding a bit kernel is a single edit beside the
 * vector it uses. */
static int obs_gpu_is_bitkernel(const char *name) {
    return obs_gpu_streq(name, "bitcount") || obs_gpu_streq(name, "findmsb") ||
           obs_gpu_streq(name, "findlsb") || obs_gpu_streq(name, "bitreverse") ||
           obs_gpu_streq(name, "bfe") || obs_gpu_streq(name, "unpackunorm") ||
           obs_gpu_streq(name, "unpacksnorm") || obs_gpu_streq(name, "unpackhalf");
}

/* The operand set a multi-operand kernel is swept over: the range-bound sets for the
 * packing conversions, the bit edges for everything else. The unary counterpart is
 * obs_gpu_is_bitkernel; this is the same idea for the cross-product path. */
static void obs_gpu_operands(const char *name, const uint32_t **set,
                             unsigned int *count) {
    if (obs_gpu_streq(name, "packunorm")) {
        *set = obs_gpu_unorm;
        *count = OBS_GPU_UNORM;
    } else if (obs_gpu_streq(name, "packsnorm")) {
        *set = obs_gpu_snorm;
        *count = OBS_GPU_SNORM;
    } else {
        *set = obs_gpu_edge;
        *count = OBS_GPU_EDGES;
    }
}

static unsigned int obs_ipow(unsigned int base, unsigned int exp) {
    unsigned int r = 1u;
    while (exp-- > 0u) {
        r *= base;
    }
    return r;
}

/* Runs one kernel and emits its results. Returns 1 if it dispatched, 0 otherwise.
 *
 * Arity one is the unary path: the shared input vector, in place, `gpu` records. Arity
 * two and three lay out the edge cross-product as [inputs..., out] per lane and emit
 * `gpuop` records. One function so `check_gpu_kernels` is just the generated list
 * applied to it. */
static int sweep_kernel(const char *name, const uint32_t *spirv, size_t bytes,
                        unsigned int arity) {
    if (arity == 1u) {
        /* Bit kernels get the bit-pattern vector, everything else the float edges. The
         * buffer is sized for OBS_GPU_LANES, the larger of the two, so either set fits.
         */
        int isbits = obs_gpu_is_bitkernel(name);
        unsigned int n = isbits ? OBS_GPU_BITS : OBS_GPU_LANES;
        uint32_t data[OBS_GPU_LANES];
        for (unsigned int i = 0; i < n; i++) {
            data[i] = isbits ? obs_gpu_bits[i] : f32_bits(obs_gpu_inputs[i]);
        }
        if (obs_gpu_run(spirv, bytes, data, n) != 0) {
            return 0;
        }
        for (unsigned int i = 0; i < n; i++) {
            uint32_t in = isbits ? obs_gpu_bits[i] : f32_bits(obs_gpu_inputs[i]);
            obs_report_gpu(name, i, in, data[i]);
        }
        return 1;
    }

    /* [inputs..., out] per lane, one lane per point in the operand cross-product. The
     * operand set is per-kernel (the packing conversions want a value range, not bit
     * edges). The buffer is static to keep a several-kilobyte frame off the stack, and
     * sized for the largest arity over the default set - a bigger set at high arity is
     * refused by the check, not truncated. */
    const uint32_t *ops;
    unsigned int nops;
    obs_gpu_operands(name, &ops, &nops);
    static uint32_t buf[OBS_GPU_EDGES * OBS_GPU_EDGES * OBS_GPU_EDGES * 4u];
    unsigned int lanes = obs_ipow(nops, arity);
    unsigned int stride = arity + 1u;
    if (arity > 3u || (size_t)lanes * stride > sizeof(buf) / sizeof(buf[0])) {
        return 0;
    }
    for (unsigned int l = 0; l < lanes; l++) {
        for (unsigned int d = 0; d < arity; d++) {
            unsigned int idx = (l / obs_ipow(nops, d)) % nops;
            buf[l * stride + d] = ops[idx];
        }
        buf[l * stride + arity] = 0u;
    }
    if (obs_gpu_run(spirv, bytes, buf, lanes * stride) != 0) {
        return 0;
    }
    for (unsigned int l = 0; l < lanes; l++) {
        uint32_t inputs[3];
        for (unsigned int d = 0; d < arity; d++) {
            inputs[d] = buf[l * stride + d];
        }
        obs_report_gpu_op(name, l, buf[l * stride + arity], inputs, arity);
    }
    return 1;
}

static obs_result check_gpu_pipeline(void) {
    if (!obs_gpu_backend_available()) {
        return obs_skip("no GPU backend on this build or target");
    }
    /* Announced before any results, so every gpu record can be read against the device
     * that produced it. */
    obs_report_gpu_device(obs_gpu_backend_name(), obs_gpu_device_name(),
                          obs_gpu_device_type());

    uint32_t data[OBS_GPU_LANES];
    for (unsigned int i = 0; i < OBS_GPU_LANES; i++) {
        data[i] = f32_bits(obs_gpu_inputs[i]);
    }
    if (obs_gpu_run(obs_shader_double, sizeof(obs_shader_double), data,
                    OBS_GPU_LANES) != 0) {
        return obs_fail("the double kernel did not dispatch");
    }
    for (unsigned int i = 0; i < OBS_GPU_LANES; i++) {
        /* x*2 is exact in binary for every finite input here, so an exact comparison is
         * legitimate - this is the one GPU result the program is entitled to predict.
         */
        if (bits_f32(data[i]) != obs_gpu_inputs[i] * 2.0f) {
            return obs_fail_code("the double kernel produced the wrong value",
                                 (uint64_t)data[i]);
        }
    }
    return obs_pass_value((uint64_t)OBS_GPU_LANES);
}

static obs_result check_gpu_kernels(void) {
    if (!obs_gpu_backend_available()) {
        return obs_skip("no GPU backend on this build or target");
    }

    unsigned int ran = 0;
/* The generated list applied to sweep_kernel - one line, and it never changes as
 * kernels are added or their arity varies. */
#define OBS_GPU_SWEEP(name, arity)                                                     \
    ran += (unsigned int)sweep_kernel(#name, obs_shader_##name,                        \
                                      sizeof(obs_shader_##name), (arity));
    OBS_GPU_KERNELS(OBS_GPU_SWEEP)
#undef OBS_GPU_SWEEP

    if (ran == 0) {
        return obs_fail("no kernel dispatched");
    }
    return obs_pass_value((uint64_t)ran);
}

#else /* !OBS_GPU */

/* One skip apiece, so the checks stay in the report and the count does not move with
 * the build flag. The same identifiers, so a diff against a GPU build lines them up. */
static obs_result check_gpu_pipeline(void) {
    return obs_skip("built without OBS_GPU; see the GPU=1 build option");
}

static obs_result check_gpu_kernels(void) {
    return obs_skip("built without OBS_GPU; see the GPU=1 build option");
}

#endif

/* One table, in every build. Only the bodies above differ, so the check count is the
 * same whether or not the GPU backend is compiled - which keeps the meta record and a
 * run-to-run diff stable across the flag. */
static const obs_check checks[] = {
    {"160-gpu/pipeline", "obscene", "(compute dispatch)", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpu_pipeline, check_gpu_pipeline, OBS_FROM_DERIVED},
    /* DERIVED, not HARDWARE: the check's provenance is fixed at compile time, but
     * whether a result is hardware depends on the device at run time (llvmpipe is not
     * silicon). The honest provenance for a GPU observation lives in its `gpudev`
     * record - the device name - not in this field. */
    {"160-gpu/kernels", "obscene", "(compute dispatch)", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_gpu_kernels, check_gpu_kernels, OBS_FROM_DERIVED},
};

const obs_section obs_section_gpu = {
    "160-gpu",
    "GPU compute",
    "Dispatches a compute shader on the device and reads the result bits back - the "
    "one "
    "measurement of what the GPU actually computes rather than what a function "
    "returns.",
    checks,
    OBS_COUNT(checks),
};
