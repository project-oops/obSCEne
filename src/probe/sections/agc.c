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

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

#include <stddef.h>

#define OBS_AGC_CMDBUF_SIZE 0x400u
#define OBS_AGC_GUARD_SIZE 64u
#define OBS_AGC_POISON_BYTE 0xCCu
#define OBS_AGC_GUARD_BYTE 0xC7u

typedef uint64_t (*agc_cb_fn)(void *arg0, uint64_t arg1, uint64_t arg2,
                              uint64_t arg3, uint64_t arg4, uint64_t arg5);

/* The caller-owned writer struct on the stack.
 *
 * Stack observation from PPSA02664:
 * arg0 = 0x6000007fbe38, pointing to {begin, end, cur, end2} sitting 0x38 in front
 * of the 0x400-byte command buffer. For NID 0x7d86501b8094ef57, arg0 - 8 holds
 * a count (0x1fa observed in the guest).
 */
typedef struct {
    uint64_t count_prefix;               /* arg0 - 8: count for NID 0x7d86501b8094ef57 */
    uint64_t begin;                      /* arg0 + 0x00: pointer to cmdbuf */
    uint64_t end;                        /* arg0 + 0x08: pointer to cmdbuf + 0x400 */
    uint64_t cur;                        /* arg0 + 0x10: current writer pointer */
    uint64_t end2;                       /* arg0 + 0x18: secondary limit */
    uint8_t pad[0x18];                   /* arg0 + 0x20 .. 0x37: distance to buffer */
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

static void agc_cb_prepare(obs_agc_cb_probe *probe, uint64_t count) {
    probe->count_prefix = count;
    probe->begin = (uint64_t)(uintptr_t)probe->cmdbuf;
    probe->end = (uint64_t)(uintptr_t)(probe->cmdbuf + OBS_AGC_CMDBUF_SIZE);
    probe->cur = probe->begin;
    probe->end2 = probe->end;
    for (unsigned int i = 0; i < (unsigned int)sizeof(probe->pad); i++) {
        probe->pad[i] = 0;
    }
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

static obs_result agc_cb_run_two_pass(const char *id, const char *symbol,
                                      uint64_t count, agc_cb_fn fn) {
    static obs_agc_cb_probe probe;
    static uint8_t before[OBS_AGC_CMDBUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_CMDBUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    /* Pass 1: arg1..arg5 = 0 */
    agc_cb_prepare(&probe, count);
    void *arg0 = (void *)&probe.begin;
    uint64_t rc0 = fn(arg0, 0, 0, 0, 0, 0);

    if (!agc_cb_guard_intact(&probe)) {
        return obs_fail("the call wrote past the end of its command buffer");
    }

    unsigned int written0 = agc_cb_written_bytes(&probe, OBS_AGC_POISON_BYTE);
    if (written0 > 0) {
        obs_report_written(id, symbol, "zero-args", before, probe.cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        return obs_pass_value((uint64_t)written0);
    }

    /* Pass 2: small non-zero values (1, 2, 4, 1, 2) */
    agc_cb_prepare(&probe, count);
    uint64_t rc1 = fn(arg0, 1, 2, 4, 1, 2);

    if (!agc_cb_guard_intact(&probe)) {
        return obs_fail("the call wrote past the end of its command buffer");
    }

    unsigned int written1 = agc_cb_written_bytes(&probe, OBS_AGC_POISON_BYTE);
    if (written1 > 0) {
        obs_report_written(id, symbol, "small-args", before, probe.cmdbuf,
                           OBS_AGC_CMDBUF_SIZE);
        return obs_pass_value((uint64_t)written1);
    }

    /* Buffer remained untouched across both passes */
    obs_report_written(id, symbol, "untouched", before, probe.cmdbuf,
                       OBS_AGC_CMDBUF_SIZE);
    if (rc0 != 0 || rc1 != 0) {
        uint64_t err = (rc1 != 0) ? rc1 : rc0;
        return obs_partial_value("call returned error code and wrote nothing", err);
    }
    return obs_fail(
        "the call returned success but wrote nothing to the command buffer");
}

/* Control test: sceAgcCbNop writes a PM4 NOP packet. */
static obs_result check_agc_cb_nop(void) {
    return agc_cb_run_two_pass("166-agc/cb-nop", "sceAgcCbNop", 0, sceAgcCbNop);
}

static obs_result check_agc_cb_release_mem(void) {
    return agc_cb_run_two_pass("166-agc/cb-release-mem", "sceAgcCbReleaseMem", 0,
                               sceAgcCbReleaseMem);
}

static obs_result check_agc_dcb_dma_data(void) {
    return agc_cb_run_two_pass("166-agc/dcb-dma-data", "sceAgcDcbDmaData", 0,
                               sceAgcDcbDmaData);
}

static obs_result check_agc_dcb_wait_reg_mem(void) {
    return agc_cb_run_two_pass("166-agc/dcb-wait-reg-mem", "sceAgcDcbWaitRegMem", 0,
                               sceAgcDcbWaitRegMem);
}

/* Unnamed NID 0x7d86501b8094ef57: count 0x1fa sits at arg0 - 8. */
static obs_result check_agc_cb_unnamed_ef57(void) {
    return agc_cb_run_two_pass("166-agc/cb-unnamed-ef57", "$fYZQG4CU71c", 0x1fa,
                               sceAgc_nid_7d86501b8094ef57);
}

/* sceAgcDcbResetQueue: Class B caller-allocated initialiser (D565 / Worklog 415).
 * Tests calling on a caller-owned zeroed/poisoned buffer with 0 and 0x400 sizes. */
static obs_result check_agc_dcb_reset_queue(void) {
#define OBS_AGC_RESET_BUF_SIZE 0x400u
    static uint8_t buf[OBS_AGC_RESET_BUF_SIZE + OBS_AGC_GUARD_SIZE];
    static uint8_t before[OBS_AGC_RESET_BUF_SIZE];
    for (unsigned int i = 0; i < OBS_AGC_RESET_BUF_SIZE; i++) {
        before[i] = OBS_AGC_POISON_BYTE;
    }

    /* Pass 1: arg1..arg5 = 0 */
    for (unsigned int i = 0; i < OBS_AGC_RESET_BUF_SIZE; i++) {
        buf[i] = OBS_AGC_POISON_BYTE;
    }
    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        buf[OBS_AGC_RESET_BUF_SIZE + i] = OBS_AGC_GUARD_BYTE;
    }

    uint64_t rc0 = sceAgcDcbResetQueue(buf, 0, 0, 0, 0, 0);

    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        if (buf[OBS_AGC_RESET_BUF_SIZE + i] != OBS_AGC_GUARD_BYTE) {
            return obs_fail("sceAgcDcbResetQueue wrote past the end of its buffer");
        }
    }

    unsigned int written0 = 0;
    for (unsigned int i = 0; i < OBS_AGC_RESET_BUF_SIZE; i++) {
        if (buf[i] != OBS_AGC_POISON_BYTE) {
            written0 = i + 1u;
        }
    }

    if (written0 > 0) {
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "zero-args",
                           before, buf, OBS_AGC_RESET_BUF_SIZE);
        return obs_pass_value((uint64_t)written0);
    }

    /* Pass 2: size 0x400 in arg1 */
    for (unsigned int i = 0; i < OBS_AGC_RESET_BUF_SIZE; i++) {
        buf[i] = OBS_AGC_POISON_BYTE;
    }
    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        buf[OBS_AGC_RESET_BUF_SIZE + i] = OBS_AGC_GUARD_BYTE;
    }

    uint64_t rc1 = sceAgcDcbResetQueue(buf, 0x400, 0, 0, 0, 0);

    for (unsigned int i = 0; i < OBS_AGC_GUARD_SIZE; i++) {
        if (buf[OBS_AGC_RESET_BUF_SIZE + i] != OBS_AGC_GUARD_BYTE) {
            return obs_fail("sceAgcDcbResetQueue wrote past the end of its buffer");
        }
    }

    unsigned int written1 = 0;
    for (unsigned int i = 0; i < OBS_AGC_RESET_BUF_SIZE; i++) {
        if (buf[i] != OBS_AGC_POISON_BYTE) {
            written1 = i + 1u;
        }
    }

    if (written1 > 0) {
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "size-0x400",
                           before, buf, OBS_AGC_RESET_BUF_SIZE);
        return obs_pass_value((uint64_t)written1);
    }

    /* Pass 3: test as a command-buffer writer struct */
    static obs_agc_cb_probe probe;
    agc_cb_prepare(&probe, 0);
    uint64_t rc2 = sceAgcDcbResetQueue((void *)&probe.begin, 0, 0, 0, 0, 0);
    if (!agc_cb_guard_intact(&probe)) {
        return obs_fail("sceAgcDcbResetQueue wrote past command buffer in writer test");
    }
    unsigned int written2 = agc_cb_written_bytes(&probe, OBS_AGC_POISON_BYTE);
    if (written2 > 0) {
        obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "writer-struct",
                           before, probe.cmdbuf, OBS_AGC_CMDBUF_SIZE);
        return obs_pass_value((uint64_t)written2);
    }

    obs_report_written("166-agc/dcb-reset-queue", "sceAgcDcbResetQueue", "untouched",
                       before, buf, OBS_AGC_RESET_BUF_SIZE);
    if (rc0 != 0 || rc1 != 0 || rc2 != 0) {
        uint64_t err = (rc1 != 0) ? rc1 : ((rc0 != 0) ? rc0 : rc2);
        return obs_partial_value("call returned error code and wrote nothing", err);
    }
    return obs_fail("the call returned success but wrote nothing to the buffer");
#undef OBS_AGC_RESET_BUF_SIZE
}

/* sceAgcCreateShader: arity 4. Dumps 32-byte out-parameter and 0x200-byte shader object. */
static obs_result check_agc_create_shader(void) {
    static const uint32_t lengths[2] = {0xd8u, 0x118u};
    static const char *labels[2] = {"payload-0xd8", "payload-0x118"};
    unsigned int valid_objects = 0;
    uint64_t last_ret = 0;

    for (unsigned int p = 0; p < 2; p++) {
        uint32_t plen = lengths[p];
        const char *label = labels[p];

        /* Header: magic '1234', header size 0x18, payload length plen */
        uint8_t header[24];
        for (size_t i = 0; i < sizeof(header); i++) {
            header[i] = 0;
        }
        header[0] = 0x31; /* '1' */
        header[1] = 0x32; /* '2' */
        header[2] = 0x33; /* '3' */
        header[3] = 0x34; /* '4' */
        header[4] = 0x18; /* header size 24 bytes */
        *(uint32_t *)(header + 8) = plen;

        /* Payload buffer with deterministic non-zero pattern */
        uint8_t payload[0x200];
        for (size_t i = 0; i < sizeof(payload); i++) {
            payload[i] = (uint8_t)((i * 7u + 0x13u) & 0xFFu);
        }

        /* 32-byte zeroed destination slot */
        uint8_t out_slot[32];
        for (size_t i = 0; i < sizeof(out_slot); i++) {
            out_slot[i] = 0;
        }

        uint64_t rc = sceAgcCreateShader((void *)out_slot, (const void *)header,
                                         (const void *)payload, 0);
        last_ret = rc;

        /* Record the 32 bytes at arg0 */
        obs_report_bytes("166-agc/create-shader", "sceAgcCreateShader", label, 0,
                         out_slot, (unsigned int)sizeof(out_slot));

        /* If *arg0 is a mapped pointer, dump 0x200 bytes from it */
        const void *shader_obj = *(const void **)out_slot;
        if (obs_address_is_callable(shader_obj)) {
            valid_objects++;
            const char *obj_label = (p == 0) ? "shader-obj-d8" : "shader-obj-118";
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

    if (valid_objects == 2) {
        return obs_pass_value((uint64_t)valid_objects);
    }
    if (valid_objects == 1) {
        return obs_partial("one shader run wrote a valid object; recorded");
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

static const obs_check agc_checks[] = {
    {"166-agc/cb-nop", "libSceAgc", "sceAgcCbNop", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceAgcCbNop, check_agc_cb_nop, OBS_FROM_ASSUMED},
    {"166-agc/cb-release-mem", "libSceAgc", "sceAgcCbReleaseMem", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCbReleaseMem, check_agc_cb_release_mem,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-dma-data", "libSceAgc", "sceAgcDcbDmaData", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbDmaData, check_agc_dcb_dma_data,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-wait-reg-mem", "libSceAgc", "sceAgcDcbWaitRegMem", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbWaitRegMem,
     check_agc_dcb_wait_reg_mem, OBS_FROM_ASSUMED},
    {"166-agc/cb-unnamed-ef57", "libSceAgc", "$fYZQG4CU71c", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgc_nid_7d86501b8094ef57,
     check_agc_cb_unnamed_ef57, OBS_FROM_ASSUMED},
    {"166-agc/dcb-reset-queue", "libSceAgc", "sceAgcDcbResetQueue", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcDcbResetQueue, check_agc_dcb_reset_queue,
     OBS_FROM_ASSUMED},
    {"166-agc/create-shader", "libSceAgc", "sceAgcCreateShader", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceAgcCreateShader, check_agc_create_shader,
     OBS_FROM_ASSUMED},
    {"166-agc/dcb-constructor-audit", "libSceAgc", "(census)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_dcb_constructor_audit,
     OBS_FROM_ASSUMED},
    {"166-agc/patch-exclusion-guard", "libSceAgc", "(guard)", OBS_CAP_NONE,
     OBS_CAP_NONE, OBS_NO_SYMBOL, check_agc_patch_exclusion_guard,
     OBS_FROM_ASSUMED},
};

const obs_section obs_section_agc = {
    "166-agc",
    "AGC command building and shaders",
    "Calling confirmed libSceAgc command builders and shader creation, "
    "recording packet encodings and shader structure offsets.",
    agc_checks,
    OBS_COUNT(agc_checks),
};
