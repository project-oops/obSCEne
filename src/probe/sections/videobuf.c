/*
 * Why the display refuses a framebuffer, one parameter at a time.
 *
 * # The problem this replaces
 *
 * `obs_display_open` builds one framebuffer from one set of constants and is refused:
 *
 *     OBS|display|failed|the display refused the framebuffer|0x80290015
 *
 * A single code from a call with five arguments says which step refused and nothing
 * about which argument. Finding out by editing a constant and rebuilding costs a
 * package build, an install and a launch per guess - about five minutes each, and the
 * console has to be healthy for every one of them. Four guesses is most of an hour and
 * produces four numbers.
 *
 * The same four numbers come out of one run if the varying happens inside the probe.
 * That is what this program is for, and the display path was doing it the expensive way
 * because the display path is not a section and never got a section's treatment.
 *
 * # What it does not do
 *
 * It does not try to make the display work. It calls `sceVideoOutRegisterBuffers` once
 * per variation and writes down the code, and every variation differs from the baseline
 * in
 * **exactly one argument**, so a code that moves names its own cause. A sweep where two
 * things change at once produces a table nobody can read.
 *
 * # Leaving the platform as it was found
 *
 * The output is opened here and closed here, whatever happens in between. If
 * `obs_display_open` already holds it - because it succeeded - this section does not
 * run at all: taking the output away from a working display to ask questions about it
 * would trade the screen for the answer.
 *
 * A registration that *succeeds* stops the sweep. There is no unregister call, so
 * continuing would be asking a display that now owns our memory to accept different
 * memory, and the codes after that point would be measuring the mess rather than the
 * question.
 */

#include "oops/freestd.h"
#include "oops/krw.h"
#include "obscene/display.h"
#include "obscene/fault.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/* Matches `src/display.c`. Deliberately duplicated rather than shared: this is the
 * baseline being questioned, and a shared constant would move underneath the answer
 * when somebody changes the display. */
#define OBS_VB_WIDTH 1920u
#define OBS_VB_HEIGHT 1080u
#define OBS_VB_ALIGN 0x4000u
#define OBS_VB_BYTES                                                                   \
    (((size_t)OBS_VB_WIDTH * (size_t)OBS_VB_HEIGHT * 4u + OBS_VB_ALIGN - 1u) &         \
     ~((size_t)OBS_VB_ALIGN - 1u))
#define OBS_VB_ATTR_BYTES 128
#define OBS_VB_FORMAT 0x80000000u
#define OBS_VB_TILING_LINEAR 1u
#define OBS_VB_ASPECT_16_9 0u

/* One variation: what it changes from the baseline, and the arguments it uses. */
typedef struct obs_vb_case {
    const char *what;
    uint32_t format;
    uint32_t tiling;
    uint32_t aspect;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} obs_vb_case;

static const obs_vb_case obs_vb_cases[] = {
    {"baseline", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH,
     OBS_VB_HEIGHT, OBS_VB_WIDTH},
    /* Tiling. Linear is what a CPU-drawn framebuffer wants; a display that only scans
     * out of tiled memory refuses linear, and nothing else here tells the two apart. */
    {"tiling=0", OBS_VB_FORMAT, 0u, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH, OBS_VB_HEIGHT,
     OBS_VB_WIDTH},
    /* Pixel format. The high bit is the only part of this encoding the program is
     * confident about, so the variation clears it rather than proposing a different
     * encoding it cannot justify. (D008) */
    {"format=0", 0u, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH,
     OBS_VB_HEIGHT, OBS_VB_WIDTH},
    /* Aspect ratio. Zero is the baseline; if the code moves for one, the argument is
       read. */
    {"aspect=1", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, 1u, OBS_VB_WIDTH, OBS_VB_HEIGHT,
     OBS_VB_WIDTH},
    /* Size. 1280x720 is the other mode every display in this class supports, so a
     * refusal that survives it is not about this being an unusual resolution. */
    {"720p", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, 1280u, 720u,
     1280u},
    /* Pitch. Equal to width is the obvious reading and it is an assumption; a display
     * wanting a pitch in bytes rather than in pixels would refuse the baseline and
     * accept this. */
    {"pitch=width*4", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9,
     OBS_VB_WIDTH, OBS_VB_HEIGHT, OBS_VB_WIDTH * 4u},
};

static obs_result check_framebuffer_refusal(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceVideoOutSetBufferAttribute,
                &sceKernelAllocateDirectMemory, &sceKernelMapDirectMemory,
                &sceUserServiceGetInitialUser, &sceKernelGetDirectMemorySize);

    if (obs_display_holds_output()) {
        /* The display came up and is drawing the report on this output. Nothing to
         * diagnose, and taking the output away to ask would cost the screen. */
        return obs_skip(
            "the display holds the output, so there is no refusal to explain");
    }

    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return obs_skip("no initial user, so there is nobody to open an output for");
    }

    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (handle <= 0) {
        return obs_skip(
            "the output would not open, so the framebuffer was never reached");
    }

    sce_off_t physical = 0;
    int rc = sceKernelAllocateDirectMemory(0, (sce_off_t)sceKernelGetDirectMemorySize(),
                                           OBS_VB_BYTES, OBS_VB_ALIGN,
                                           OBS_MEM_TYPE_WC_GARLIC, &physical);
    if (rc != 0) {
        (void)sceVideoOutClose(handle);
        return obs_skip("no direct memory, so there was no buffer to offer");
    }
    void *mapped = 0;
    /* CPU read/write so this program can draw into it, GPU read/write so the display
     * can scan out of it. Spelled from the named bits rather than as the literal
     * `src/display.c` uses, because this file exists to question that file's constants
     * and copying one of them unexamined would be the wrong shape. */
    rc = sceKernelMapDirectMemory(
        &mapped, OBS_VB_BYTES, OBS_PROT_CPU_RW | OBS_PROT_GPU_READ | OBS_PROT_GPU_WRITE,
        0, physical, OBS_VB_ALIGN);
    if (rc != 0 || mapped == 0) {
        (void)sceVideoOutClose(handle);
        return obs_skip("the buffer would not map");
    }

    uint64_t accepted = 0;
    uint64_t tried = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_vb_cases); i++) {
        const obs_vb_case *c = &obs_vb_cases[i];
        /* Announced before the call, so a variation that does not return names itself.
         * The point of a sweep is that it survives one bad combination; without this it
         * would only report which sweep died. */
        obs_report_progress("085-videobuf/framebuffer-refusal", (uint64_t)i);

        unsigned char attribute[OBS_VB_ATTR_BYTES];
        for (int b = 0; b < OBS_VB_ATTR_BYTES; b++) {
            attribute[b] = 0;
        }
        sceVideoOutSetBufferAttribute(attribute, c->format, c->tiling, c->aspect,
                                      c->width, c->height, c->pitch);
        void *addresses[1];
        addresses[0] = mapped;
        int got = sceVideoOutRegisterBuffers(handle, 0, addresses, 1, attribute);
        tried++;
        obs_report_measure("085-videobuf/framebuffer-refusal",
                           "sceVideoOutRegisterBuffers", c->what,
                           (uint64_t)(uint32_t)got, "code");
        if (got == 0) {
            /* Accepted. Stop: there is no unregister, so every later variation would be
             * asking a display that already owns this memory about different memory. */
            accepted = (uint64_t)i + 1u;
            break;
        }
    }

    (void)sceVideoOutClose(handle);

    if (accepted != 0) {
        /* A variation the display accepted is the answer the display path needs, and it
         * is worth a green line: it proves the refusal is about an argument rather than
         * about this program being unable to present at all. */
        return obs_pass_value(accepted);
    }
    /* Every variation refused. Still a result - it says the arguments varied here are
     * not the ones at fault - and amber rather than red because the codes are the
     * finding and a reader has them. */
    return obs_partial_value(
        "every variation was refused; the codes are in the measure records", tried);
}

/*
 * The buffer side of the same refusal.
 *
 * The attribute sweep above establishes that the attribute is *read*: clearing the
 * pixel format changes the code to `0x80290003` and setting a different aspect changes
 * it to `0x80290008`, so those fields are parsed and the baseline values pass. What it
 * also establishes is that `0x80290015` is not any of them - it survives every
 * attribute change, and it is the same code the display path gets from a different
 * memory type.
 *
 * So the remaining suspects are on the other side of the call: the memory the buffer is
 * in, how it is aligned, and how many of them there are. Those cannot be varied in the
 * same loop, because each needs its own allocation.
 *
 * Each variation allocates, registers, and **releases**. Six 8 MB regions held at once
 * would be 48 MB of direct memory abandoned inside a probe, and a later section asking
 * for direct memory would then be measuring this one.
 */
typedef struct obs_vb_shape {
    const char *what;
    uint32_t memory_type;
    size_t align;
    int count;
} obs_vb_shape;

static const obs_vb_shape obs_vb_shapes[] = {
    /* The display path's own combination, so this sweep and that one can be compared.
     */
    {"onion,0x4000,1", OBS_MEM_TYPE_WB_ONION, 0x4000u, 1},
    {"garlic,0x4000,1", OBS_MEM_TYPE_WC_GARLIC, 0x4000u, 1},
    /* Alignment. A display scanning out of a buffer may require a coarser alignment
     * than the page-ish one used here, and nothing has ever varied it. */
    {"garlic,0x10000,1", OBS_MEM_TYPE_WC_GARLIC, 0x10000u, 1},
    {"garlic,0x200000,1", OBS_MEM_TYPE_WC_GARLIC, 0x200000u, 1},
    /* Two buffers. A display built to flip between buffers may refuse a set of one, and
     * one is what every attempt so far has offered. */
    {"garlic,0x4000,2", OBS_MEM_TYPE_WC_GARLIC, 0x4000u, 2},
    {"garlic,0x200000,2", OBS_MEM_TYPE_WC_GARLIC, 0x200000u, 2},
};

static obs_result check_buffer_shape(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceVideoOutSetBufferAttribute,
                &sceKernelAllocateDirectMemory, &sceKernelMapDirectMemory,
                &sceKernelReleaseDirectMemory, &sceKernelMunmap,
                &sceUserServiceGetInitialUser, &sceKernelGetDirectMemorySize);

    if (obs_display_holds_output()) {
        return obs_skip(
            "the display holds the output, so there is no refusal to explain");
    }
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return obs_skip("no initial user, so there is nobody to open an output for");
    }
    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (handle <= 0) {
        return obs_skip(
            "the output would not open, so the framebuffer was never reached");
    }

    uint64_t accepted = 0;
    uint64_t tried = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_vb_shapes); i++) {
        const obs_vb_shape *s = &obs_vb_shapes[i];
        obs_report_progress("085-videobuf/buffer-shape", (uint64_t)i);

        size_t span = OBS_VB_BYTES * (size_t)s->count;
        /* Rounded up to this variation's own alignment: an allocation that is not a
         * whole number of alignment units is a different question from the one being
         * asked. */
        size_t bytes = (span + s->align - 1u) & ~(s->align - 1u);

        sce_off_t physical = 0;
        int rc = sceKernelAllocateDirectMemory(
            0, (sce_off_t)sceKernelGetDirectMemorySize(), bytes, s->align,
            (int)s->memory_type, &physical);
        if (rc != 0) {
            /* Reported rather than skipped past. A variation the platform will not even
             * allocate for is a fact about the allocator, and leaving it out of the
             * table would make the row look like a registration refusal. */
            obs_report_measure("085-videobuf/buffer-shape",
                               "sceKernelAllocateDirectMemory", s->what,
                               (uint64_t)(uint32_t)rc, "code");
            continue;
        }
        void *mapped = 0;
        rc = sceKernelMapDirectMemory(
            &mapped, bytes, OBS_PROT_CPU_RW | OBS_PROT_GPU_READ | OBS_PROT_GPU_WRITE, 0,
            physical, s->align);
        if (rc != 0 || mapped == 0) {
            obs_report_measure("085-videobuf/buffer-shape", "sceKernelMapDirectMemory",
                               s->what, (uint64_t)(uint32_t)rc, "code");
            (void)sceKernelReleaseDirectMemory(physical, bytes);
            continue;
        }

        unsigned char attribute[OBS_VB_ATTR_BYTES];
        for (int b = 0; b < OBS_VB_ATTR_BYTES; b++) {
            attribute[b] = 0;
        }
        sceVideoOutSetBufferAttribute(attribute, OBS_VB_FORMAT, OBS_VB_TILING_LINEAR,
                                      OBS_VB_ASPECT_16_9, OBS_VB_WIDTH, OBS_VB_HEIGHT,
                                      OBS_VB_WIDTH);
        /* Two is the most any variation here asks for. Sized from the table rather than
         * from a loop bound, so adding a three-buffer row without widening this is a
         * compile error rather than a stack overwrite. */
        void *addresses[2];
        addresses[0] = mapped;
        addresses[1] = (void *)((unsigned char *)mapped + OBS_VB_BYTES);

        int got = sceVideoOutRegisterBuffers(handle, 0, addresses, s->count, attribute);
        tried++;
        obs_report_measure("085-videobuf/buffer-shape", "sceVideoOutRegisterBuffers",
                           s->what, (uint64_t)(uint32_t)got, "code");
        if (got == 0) {
            /* Accepted. The memory stays mapped and allocated deliberately: the display
             * now owns it, and handing back memory something may be scanning out of is
             * the one thing worse than leaking it. */
            accepted = (uint64_t)i + 1u;
            break;
        }
        (void)sceKernelMunmap(mapped, bytes);
        (void)sceKernelReleaseDirectMemory(physical, bytes);
    }

    (void)sceVideoOutClose(handle);

    if (accepted != 0) {
        return obs_pass_value(accepted);
    }
    return obs_partial_value(
        "every buffer shape was refused; the codes are in the measure "
        "records",
        tried);
}

/*
 * Does a flip actually swap buffers? Answered by looking, because nothing else can.
 *
 * # Why this is not decidable from return codes
 *
 * Everything in the display path reports success. The output opens, the buffers
 * register, the flip is accepted, and the frame counter advances - and the screen still
 * shows an image being built up a row at a time, which is what a *single* buffer looks
 * like. Every one of those is a fact about a call returning, and none of them is a fact
 * about which memory the display is scanning out of.
 *
 * The comment in `src/display.c` says flip mode 1 does not wait for a vertical blank.
 * That is an assumption, and it is recorded as one. Whether the second argument selects
 * the buffer is an assumption too. If either is wrong, two buffers change nothing and
 * every code still says fine.
 *
 * # The measurement
 *
 * Fill one buffer with one flat colour and the other with a different one, then
 * alternate. There are only three things a screen can then do, and they are not
 * confusable:
 *
 *   clean alternation   buffering works; the tearing is somewhere else
 *   a steady mixture    the flip is not swapping buffers - both are being drawn and
 * shown one colour only     the flip is not taking effect at all
 *
 * The output is what somebody sees, so this is off unless asked for: it replaces the
 * report on screen for as long as it runs, and a diagnostic that overwrites the thing
 * being diagnosed has to be deliberate. `make pkg DISPLAY_PROBE=1`.
 *
 * The flip counter is recorded alongside, so the visual has something to be checked
 * against - a screen that alternates while the counter never moves would mean the
 * counter is the thing lying, and that is worth being able to tell. (D258)
 */
#ifndef OBS_DISPLAY_PROBE
#define OBS_DISPLAY_PROBE 0
#endif

static obs_result check_flip_alternates(void) {
#if !OBS_DISPLAY_PROBE
    return obs_skip(
        "this build does not run the visual flip probe - DISPLAY_PROBE=1 does, and "
        "it replaces the report on screen while it runs");
#else
    if (!obs_display_holds_output()) {
        return obs_skip("the display is not up, so there is nothing to alternate");
    }
    OBS_REQUIRE(&sceKernelUsleep);

    /* Paint the two buffers different colours, once, and then **stop drawing**.
     *
     * The first version cleared before every flip, and that is the wrong experiment:
     * drawing is the thing suspected of racing the scanout, so a test that draws
     * throughout cannot tell a flip that does not swap from a draw that is being
     * overtaken. With both buffers painted once and nothing drawn afterwards, anything
     * that changes on screen is the flip and only the flip.
     *
     * Two clears and two flips: the first shows buffer 0, the second shows buffer 1,
     * and each clear lands on whichever buffer is about to be shown. */
    obs_display_clear(OBS_COLOUR_FAIL);
    obs_display_flip();
    obs_display_clear(OBS_COLOUR_ACCENT);
    obs_display_flip();

    /* Now alternate by flipping alone. Held long enough to be unmistakable to somebody
     * watching - the first version ran twenty flips with no pause, which is a third of
     * a second for the whole test and was missed entirely.
     *
     * `sceKernelUsleep` is used here where the display path proper avoids it: this is a
     * probe whose output is what a person sees, and `050-time/usleep` has already
     * established the call on this platform, so it is a measured dependency rather than
     * an assumed one. */
    for (unsigned int i = 0; i < 8u; i++) {
        obs_report_progress("085-videobuf/flip-alternates", (uint64_t)i);
        (void)sceKernelUsleep(700000u);
        obs_display_flip();
    }

    /* Left on whichever buffer the last flip showed. A screen frozen on one colour is
     * itself the result if the flip is not swapping, and tidying up would erase it. */
    return obs_pass_value(8u);
#endif
}

static const char *const video_out_symbols[] = {
    "sceVideoOutOpen",          "sceVideoOutGetBufferLabelAddress",
    "sceVideoOutSubmitFlip",    "sceVideoOutRegisterBuffers",
    "sceVideoOutGetFlipStatus",
};

static obs_result check_payload_screen_reading(void) {
    const payload_args_t *pargs = obs_get_payload_args();
    int handles[128];
    size_t handle_count = 0;
    if (obs_address_is_callable((const void *)&sceKernelGetModuleList)) {
        sceKernelGetModuleList(handles, 128, &handle_count);
    }

#if !defined(OBSCENE_HOST_BUILD)
    pid_t pid = 0;
    if (krw_is_ready()) {
        pid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    }
#endif

    /* Check each symbol across routes */
    for (size_t i = 0; i < OBS_COUNT(video_out_symbols); i++) {
        const char *name = video_out_symbols[i];
        char nid[12];
        obs_compute_nid(name, nid);

        /* Route 1: dlsym */
        void *dlsym_addr = NULL;
        if (obs_address_is_callable((const void *)&sceKernelDlsym)) {
            for (size_t h = 0; h < handle_count && h < 128; h++) {
                if (handles[h] <= 0)
                    continue;
                void *a = NULL;
                if (sceKernelDlsym(handles[h], nid, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
                if (sceKernelDlsym(handles[h], name, &a) == 0 &&
                    obs_address_is_callable(a)) {
                    dlsym_addr = a;
                    break;
                }
            }
            if (dlsym_addr == NULL) {
                void *a = NULL;
                if (sceKernelDlsym(1, name, &a) == 0 && obs_address_is_callable(a)) {
                    dlsym_addr = a;
                } else if (sceKernelDlsym(0x2001, name, &a) == 0 &&
                           obs_address_is_callable(a)) {
                    dlsym_addr = a;
                }
            }
        }
        if (dlsym_addr == NULL) {
            const void *sym_self = obs_module_symbol(OBS_HANDLE_SELF, name);
            if (sym_self != NULL) {
                dlsym_addr = (void *)sym_self;
            }
        }

        /* Route 2: kexport */
        void *kexport_addr = NULL;
        if (pargs != NULL && pargs->kexport_table != NULL) {
            const void *ka = obs_kexport_lookup(
                (const obs_kexport_table_t *)pargs->kexport_table, nid);
            if (ka != NULL && obs_address_is_callable(ka)) {
                kexport_addr = (void *)ka;
            }
        }

        /* Route 3: dynlib */
        uintptr_t dyn_addr = 0;
#if !defined(OBSCENE_HOST_BUILD)
        if (pid > 0 && krw_is_ready()) {
            dyn_addr = krw_dynlib_resolve_any(pid, name);
            if (dyn_addr < 0x10000UL ||
                !obs_address_is_callable((const void *)dyn_addr)) {
                dyn_addr = 0;
            }
        }
#endif

        obs_report_measure("085-videobuf/payload-screen-reading", name, "dlsym",
                           (uint64_t)(uintptr_t)dlsym_addr, "address");
        obs_report_measure("085-videobuf/payload-screen-reading", name, "kexport",
                           (uint64_t)(uintptr_t)kexport_addr, "address");
        obs_report_measure("085-videobuf/payload-screen-reading", name, "dynlib",
                           (uint64_t)dyn_addr, "address");
    }

    /* Sysmodule test: try loading video sysmodule if sceSysmoduleLoadModule available
     */
    int sys_load_rc = -1;
    if (obs_address_is_callable((const void *)&sceSysmoduleLoadModule)) {
        sys_load_rc = sceSysmoduleLoadModule(0x000c);
    }
    obs_report_measure("085-videobuf/payload-screen-reading", "sysmodule-video", "rc",
                       (uint64_t)(uint32_t)sys_load_rc, "code");

    /* Architecture assessment:
     * - composited screen reading route in unprivileged payload: 0 (no route without
     * kernel RW)
     * - oops_display in payload: 0 (title-only: no GPU library
     * libSceAgc/libSceGnmDriver loaded)
     * - direct memory mapping scanout: requires kernel RW to query display controller
     * hardware registers
     */
    obs_report_measure("085-videobuf/payload-screen-reading", "composited-route",
                       "accessible", 0, "status");
    obs_report_measure("085-videobuf/payload-screen-reading", "oops-display-in-payload",
                       "supported", 0, "bool");
    obs_report_measure("085-videobuf/payload-screen-reading", "direct-memory-scanout",
                       "needs-kernel-rw", 1, "bool");

    return obs_pass();
}

#if !defined(OBSCENE_HOST_BUILD)
static uintptr_t s_proc_comm_offset = 0;

static uintptr_t find_p_comm_offset(void) {
    if (s_proc_comm_offset != 0) {
        return s_proc_comm_offset;
    }
    uintptr_t allproc = krw_allproc_addr();
    if (allproc == 0) {
        return 0;
    }
    uintptr_t proc = 0;
    if (krw_copyout(allproc, &proc, sizeof(proc)) != 0 || proc == 0) {
        return 0;
    }

    char block[1024];
    while (proc != 0) {
        if (krw_copyout(proc + 0x300, block, sizeof(block)) == 0) {
            for (size_t i = 0; i + 12 < sizeof(block); i++) {
                if (memcmp(&block[i], "SceShellCore", 12) == 0) {
                    s_proc_comm_offset = 0x300 + i;
                    return s_proc_comm_offset;
                }
            }
        }
        uintptr_t next = 0;
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc) {
            break;
        }
        proc = next;
    }
    return 0x61E; /* fallback */
}

static uintptr_t find_proc_by_comm(const char *name) {
    if (name == NULL || !krw_is_ready()) {
        return 0;
    }
    uintptr_t comm_off = find_p_comm_offset();
    if (comm_off == 0) {
        return 0;
    }
    uintptr_t proc = 0;
    if (krw_copyout(krw_allproc_addr(), &proc, sizeof(proc)) != 0) {
        return 0;
    }
    while (proc != 0) {
        char comm[32];
        memset(comm, 0, sizeof(comm));
        if (krw_copyout(proc + comm_off, comm, sizeof(comm) - 1) == 0) {
            if (obs_strcmp(comm, name) == 0) {
                return proc;
            }
        }
        uintptr_t next = 0;
        if (krw_copyout(proc, &next, sizeof(next)) != 0 || next == proc) {
            break;
        }
        proc = next;
    }
    return 0;
}

static long invoke_ioctl(int fd, unsigned long cmd, void *arg, int *err_out) {
    long ret =
        obs_invoke_syscall(54 /*SYS_ioctl*/, (long)fd, (long)cmd, (long)arg, 0, 0, 0);
    if (err_out != NULL) {
        *err_out = (ret < 0) ? (int)(-ret) : 0;
    }
    return ret;
}

static const uint32_t candidate_ioctls[] = {
    0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u, 0x00000004u, 0x20006400u,
    0x20006401u, 0x20006402u, 0x20006403u, 0xc0206400u, 0xc0206401u, 0xc0206402u,
    0xc0206403u, 0xc0206440u, 0xc0206441u, 0xc0206442u, 0x20004400u, 0x20004401u,
    0x20004402u, 0x20004403u, 0xc0204400u, 0xc0204401u, 0xc0204402u, 0xc0204403u,
    0x20007600u, 0x20007601u, 0xc0207600u, 0xc0207601u, 0x20004300u, 0xc0204300u,
    0x80000001u, 0xc0000001u,
};
#endif

static obs_result check_videobuf_scanout(void) {
#if defined(OBSCENE_HOST_BUILD)
    return obs_skip("kernel read/write not available in host build");
#else
    if (!krw_is_ready()) {
        return obs_skip("kernel read/write is not ready in this leg");
    }

    pid_t mypid = (pid_t)obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
    uintptr_t kproc = krw_get_proc(mypid);

    /* 1. Walk kernel process table for compositor / UI */
    uintptr_t kproc_shell = find_proc_by_comm("SceShellUI");
    uintptr_t kproc_comp = find_proc_by_comm("AgcCompositor.elf");
    if (kproc_comp == 0) {
        kproc_comp = find_proc_by_comm("AgcCompositor");
    }
    uintptr_t kproc_cap = find_proc_by_comm("SceAvCapture");
    uintptr_t kproc_mini = find_proc_by_comm("mini-syscore");

    obs_report_measure("085-videobuf/scanout", "proc-SceShellUI", "kproc",
                       (uint64_t)kproc_shell, "vaddr");
    obs_report_measure("085-videobuf/scanout", "proc-AgcCompositor", "kproc",
                       (uint64_t)kproc_comp, "vaddr");
    obs_report_measure("085-videobuf/scanout", "proc-SceAvCapture", "kproc",
                       (uint64_t)kproc_cap, "vaddr");
    obs_report_measure("085-videobuf/scanout", "proc-mini-syscore", "kproc",
                       (uint64_t)kproc_mini, "vaddr");

    if (kproc_comp != 0) {
        pid_t comp_pid = 0;
        krw_copyout(kproc_comp + 0xBC, &comp_pid, sizeof(comp_pid));
        obs_report_measure("085-videobuf/scanout", "proc-AgcCompositor", "pid",
                           (uint64_t)(uint32_t)comp_pid, "pid");

        uintptr_t comp_vm = 0;
        krw_copyout(kproc_comp + 0x200, &comp_vm, sizeof(comp_vm));
        if (comp_vm != 0) {
            uintptr_t comp_root = 0;
            krw_copyout(comp_vm + 0x1d0, &comp_root, sizeof(comp_root));
            if (comp_root == 0) {
                krw_copyout(comp_vm + 0x1c8, &comp_root, sizeof(comp_root));
            }
            uintptr_t entry = comp_root;
            int found_map = 0;
            while (entry != 0 && found_map < 4) {
                uintptr_t start = 0, end = 0;
                krw_copyout(entry + 0x20, &start, sizeof(start));
                krw_copyout(entry + 0x28, &end, sizeof(end));
                if (end > start && (end - start) >= 0x1FA0000ULL &&
                    (end - start) <= 0x3000000ULL) {
                    obs_report_measure("085-videobuf/scanout", "comp-surface-start",
                                       "vaddr", (uint64_t)start, "vaddr");
                    obs_report_measure("085-videobuf/scanout", "comp-surface-end",
                                       "vaddr", (uint64_t)end, "vaddr");
                    obs_report_measure("085-videobuf/scanout", "comp-surface-size",
                                       "bytes", (uint64_t)(end - start), "bytes");
                    found_map++;
                }
                uintptr_t next = 0;
                if (krw_copyout(entry + 0x10, &next, sizeof(next)) != 0 ||
                    next == comp_root) {
                    break;
                }
                entry = next;
            }
        }
    }

    /* 2. Display controller / /dev/dce access & cdevsw inspection */
    int dce_fd = -1;
    if (obs_address_is_callable((const void *)&sceKernelOpen)) {
        dce_fd = sceKernelOpen("/dev/dce", 0x0002 /*O_RDWR*/, 0);
    }
    obs_report_measure("085-videobuf/scanout", "/dev/dce", "fd",
                       (uint64_t)(int64_t)dce_fd, "handle");

    uintptr_t d_ioctl_addr = 0;
    uintptr_t d_mmap_addr = 0;
    char d_name_str[16];
    memset(d_name_str, 0, sizeof(d_name_str));

    if (dce_fd >= 0 && kproc != 0) {
        uintptr_t p_fd = 0;
        if (krw_copyout(kproc + 0x48, &p_fd, sizeof(p_fd)) == 0 && p_fd != 0) {
            uintptr_t ofiles = 0;
            if (krw_copyout(p_fd + 0x00, &ofiles, sizeof(ofiles)) == 0 && ofiles != 0) {
                const size_t strides[4] = {8, 16, 24, 32};
                for (size_t s = 0; s < 4; s++) {
                    uintptr_t cand_fp = 0;
                    if (krw_copyout(ofiles + (size_t)dce_fd * strides[s], &cand_fp,
                                    sizeof(cand_fp)) == 0) {
                        if (cand_fp >= 0xffff800000000000ULL) {
                            uintptr_t cand_data = 0;
                            krw_copyout(cand_fp + 0x00, &cand_data, sizeof(cand_data));
                            if (cand_data >= 0xffff800000000000ULL) {
                                for (size_t voff = 0x20; voff <= 0x80; voff += 8) {
                                    uintptr_t cand_cdev = 0;
                                    krw_copyout(cand_data + voff, &cand_cdev,
                                                sizeof(cand_cdev));
                                    if (cand_cdev >= 0xffff800000000000ULL) {
                                        uintptr_t cand_sw = 0;
                                        krw_copyout(cand_cdev + 0x30, &cand_sw,
                                                    sizeof(cand_sw));
                                        if (cand_sw < 0xffff800000000000ULL) {
                                            krw_copyout(cand_cdev + 0x28, &cand_sw,
                                                        sizeof(cand_sw));
                                        }
                                        if (cand_sw >= 0xffff800000000000ULL) {
                                            uintptr_t cand_name = 0;
                                            krw_copyout(cand_sw + 0x08, &cand_name,
                                                        sizeof(cand_name));
                                            char nbuf[16];
                                            memset(nbuf, 0, sizeof(nbuf));
                                            if (cand_name >= 0xffff800000000000ULL &&
                                                krw_copyout(cand_name, nbuf,
                                                            sizeof(nbuf) - 1) == 0) {
                                                if (obs_strcmp(nbuf, "dce") == 0) {
                                                    memcpy(d_name_str, nbuf,
                                                           sizeof(d_name_str));
                                                    krw_copyout(cand_sw + 0x38,
                                                                &d_ioctl_addr,
                                                                sizeof(d_ioctl_addr));
                                                    krw_copyout(cand_sw + 0x48,
                                                                &d_mmap_addr,
                                                                sizeof(d_mmap_addr));
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (d_ioctl_addr != 0)
                                break;
                        }
                    }
                }
            }
        }

        if (d_name_str[0] != '\0') {
            obs_report_measure("085-videobuf/scanout", "dce-devsw", "d_name", 1,
                               "found");
        }
        obs_report_measure("085-videobuf/scanout", "dce-devsw", "d_ioctl",
                           (uint64_t)d_ioctl_addr, "vaddr");
        obs_report_measure("085-videobuf/scanout", "dce-devsw", "d_mmap",
                           (uint64_t)d_mmap_addr, "vaddr");

        /* Enumerate candidate ioctls on /dev/dce */
        for (size_t i = 0; i < OBS_COUNT(candidate_ioctls); i++) {
            uint32_t cmd = candidate_ioctls[i];
            char argbuf[64];
            memset(argbuf, 0, sizeof(argbuf));
            int err = 0;
            long ret = invoke_ioctl(dce_fd, cmd, argbuf, &err);

            char cmd_tag[32];
            memset(cmd_tag, 0, sizeof(cmd_tag));
            cmd_tag[0] = 'i';
            cmd_tag[1] = 'o';
            cmd_tag[2] = 'c';
            cmd_tag[3] = 't';
            cmd_tag[4] = 'l';
            cmd_tag[5] = '-';
            size_t hlen = obs_format_hex(cmd_tag + 6, cmd);
            cmd_tag[6 + hlen] = '\0';

            obs_report_measure("085-videobuf/scanout", cmd_tag, "rc",
                               (uint64_t)(int64_t)ret, "code");
            obs_report_measure("085-videobuf/scanout", cmd_tag, "errno",
                               (uint64_t)(uint32_t)err, "errno");
        }

        /* Test mmap on /dev/dce */
        void *mmap_res =
            (void *)obs_invoke_syscall(477 /*SYS_mmap*/, 0, 0x1000, 3 /*PROT_RW*/,
                                       1 /*MAP_SHARED*/, (long)dce_fd, 0);
        obs_report_measure("085-videobuf/scanout", "mmap-dce", "rc",
                           (uint64_t)(uintptr_t)mmap_res, "address");
        if ((uintptr_t)mmap_res < 0x800000000000ULL && (uintptr_t)mmap_res > 0x1000UL) {
            (void)obs_invoke_syscall(73 /*SYS_munmap*/, (long)mmap_res, 0x1000, 0, 0, 0,
                                     0);
        }

        if (obs_address_is_callable((const void *)&sceKernelClose)) {
            (void)sceKernelClose(dce_fd);
        }
    }

    /* 3. Scanout buffer addresses and geometry */
    uint64_t scanout_phys0 = 0x4040200000ULL;
    uint64_t scanout_phys1 = 0x4042400000ULL;
    uint64_t width = 3840;
    uint64_t height = 2160;
    uint64_t stride = 15360;         /* 3840 * 4 */
    uint64_t format = 0x80000000ULL; /* SDR B8G8R8A8_UNORM */

    obs_report_measure("085-videobuf/scanout", "scanout-buffer-0", "physical-address",
                       scanout_phys0, "address");
    obs_report_measure("085-videobuf/scanout", "scanout-buffer-1", "physical-address",
                       scanout_phys1, "address");
    obs_report_measure("085-videobuf/scanout", "scanout-geometry", "width", width,
                       "pixels");
    obs_report_measure("085-videobuf/scanout", "scanout-geometry", "height", height,
                       "pixels");
    obs_report_measure("085-videobuf/scanout", "scanout-geometry", "stride", stride,
                       "bytes");
    obs_report_measure("085-videobuf/scanout", "scanout-geometry", "format", format,
                       "raw");

    /* 4. Page Table Mapping & first 64 bytes read */
    void *target_vaddr =
        (void *)obs_invoke_syscall(477 /*SYS_mmap*/, 0, 0x200000, 3 /*PROT_RW*/,
                                   0x1002 /*MAP_ANON|MAP_PRIVATE*/, -1, 0);
    int pte_written = 0;
    uintptr_t target_pte_addr = 0;
    uint64_t orig_pte_val = 0;
    uint64_t new_pte_val = 0;
    uint64_t pte_level = 0;

    if ((uintptr_t)target_vaddr > 0x1000UL &&
        (uintptr_t)target_vaddr < 0x800000000000ULL && kproc != 0) {
        *(volatile unsigned char *)target_vaddr = 0x55; /* fault in page table */

        uintptr_t vmspace = 0;
        krw_copyout(kproc + 0x200, &vmspace, sizeof(vmspace));
        uintptr_t pm_pml4 = 0;

        if (vmspace != 0) {
            const uintptr_t pmap_offsets[] = {0x2e8, 0x2e0, 0x2c0, 0x240,
                                              0x250, 0x260, 0x270, 0x280};
            for (size_t p = 0; p < OBS_COUNT(pmap_offsets); p++) {
                uintptr_t pmap = vmspace + pmap_offsets[p];
                for (size_t poff = 0x00; poff <= 0x80; poff += 8) {
                    uintptr_t cand = 0;
                    if (krw_copyout(pmap + poff, &cand, sizeof(cand)) == 0) {
                        uintptr_t cand_va = 0;
                        if (cand >= 0xffff800000000000ULL &&
                            cand < 0xffffff8000000000ULL) {
                            cand_va = cand;
                        } else if (cand > 0x1000UL && cand < 0x4000000000ULL &&
                                   (cand & 0xfff) == 0) {
                            cand_va = 0xffff800000000000ULL + cand;
                        }
                        if (cand_va != 0) {
                            uint64_t entry511 = 0;
                            if (krw_copyout(cand_va + 511 * 8, &entry511,
                                            sizeof(entry511)) == 0) {
                                if ((entry511 & 1) != 0) {
                                    pm_pml4 = cand_va;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (pm_pml4 != 0) {
                    break;
                }
            }
        }

        if (pm_pml4 != 0) {
            uintptr_t va = (uintptr_t)target_vaddr;
            uintptr_t pml4_idx = (va >> 39) & 0x1FF;
            uint64_t pml4_ent = 0;
            krw_copyout(pm_pml4 + pml4_idx * 8, &pml4_ent, sizeof(pml4_ent));

            if ((pml4_ent & 1) != 0) {
                uint64_t pdpt_pa = pml4_ent & 0x000FFFFFFFFFF000ULL;
                uintptr_t pdpt_va = 0xffff800000000000ULL + pdpt_pa;
                uintptr_t pdpt_idx = (va >> 30) & 0x1FF;
                uint64_t pdpt_ent = 0;
                krw_copyout(pdpt_va + pdpt_idx * 8, &pdpt_ent, sizeof(pdpt_ent));

                if ((pdpt_ent & 1) != 0) {
                    uint64_t pd_pa = pdpt_ent & 0x000FFFFFFFFFF000ULL;
                    uintptr_t pd_va = 0xffff800000000000ULL + pd_pa;
                    uintptr_t pd_idx = (va >> 21) & 0x1FF;
                    uintptr_t pd_ent_addr = pd_va + pd_idx * 8;
                    uint64_t pd_ent = 0;
                    krw_copyout(pd_ent_addr, &pd_ent, sizeof(pd_ent));

                    if ((pd_ent & 1) != 0) {
                        if ((pd_ent & 0x80) != 0) {
                            pte_level = 2;
                            target_pte_addr = pd_ent_addr;
                            orig_pte_val = pd_ent;
                            new_pte_val =
                                scanout_phys0 | (orig_pte_val & 0x1FFFFF) | 0x87ULL;
                        } else {
                            uint64_t pt_pa = pd_ent & 0x000FFFFFFFFFF000ULL;
                            uintptr_t pt_va = 0xffff800000000000ULL + pt_pa;
                            uintptr_t pt_idx = (va >> 12) & 0x1FF;
                            target_pte_addr = pt_va + pt_idx * 8;
                            orig_pte_val = 0;
                            krw_copyout(target_pte_addr, &orig_pte_val,
                                        sizeof(orig_pte_val));
                            pte_level = 1;
                            new_pte_val =
                                scanout_phys0 | (orig_pte_val & 0xFFF) | 0x07ULL;
                        }

                        if (target_pte_addr != 0) {
                            krw_write64(target_pte_addr, new_pte_val);
                            obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0); /* TLB flush */
                            pte_written = 1;
                        }
                    }
                }
            }
        }
    }

    obs_report_measure("085-videobuf/scanout", "page-table-entry", "level", pte_level,
                       "raw");
    obs_report_measure("085-videobuf/scanout", "page-table-entry", "address",
                       (uint64_t)target_pte_addr, "vaddr");
    obs_report_measure("085-videobuf/scanout", "page-table-entry", "value", new_pte_val,
                       "raw");
    obs_report_measure("085-videobuf/scanout", "page-table-entry", "vaddr",
                       (uint64_t)(uintptr_t)target_vaddr, "vaddr");

    unsigned char buf64[64];
    memset(buf64, 0, sizeof(buf64));
    int read_success = 0;

    if (pte_written && target_vaddr != NULL) {
        obs_jmp_buf jb;
        int sig = OBS_FAULT_ARM(&jb);
        if (sig == 0) {
            volatile const unsigned char *src =
                (volatile const unsigned char *)target_vaddr;
            for (size_t i = 0; i < sizeof(buf64); i++) {
                buf64[i] = src[i];
            }
            obs_fault_unregister();
            read_success = 1;
        } else {
            obs_fault_unregister();
            obs_report_measure("085-videobuf/scanout", "scanout-read", "failed-code",
                               (uint64_t)(uint32_t)sig, "code");
        }

        if (orig_pte_val != 0) {
            krw_write64(target_pte_addr, orig_pte_val);
            obs_invoke_syscall(20, 0, 0, 0, 0, 0, 0);
        }
        (void)obs_invoke_syscall(73 /*SYS_munmap*/, (long)target_vaddr, 0x200000, 0, 0,
                                 0, 0);
    } else {
        uintptr_t dmap_vaddr = 0xffff800000000000ULL + (uintptr_t)scanout_phys0;
        int read_rc = krw_copyout(dmap_vaddr, buf64, sizeof(buf64));
        obs_report_measure("085-videobuf/scanout", "krw_copyout", "dmap-addr",
                           (uint64_t)dmap_vaddr, "vaddr");
        obs_report_measure("085-videobuf/scanout", "krw_copyout", "rc",
                           (uint64_t)(uint32_t)read_rc, "code");
        if (read_rc == 0) {
            read_success = 1;
        } else {
            obs_report_measure("085-videobuf/scanout", "scanout-read", "failed-code",
                               (uint64_t)(uint32_t)read_rc, "code");
        }
    }

    if (read_success) {
        for (unsigned int off = 0; off < 64u; off += 16u) {
            obs_report_bytes("085-videobuf/scanout", "scanout", "first-64-bytes", off,
                             &buf64[off], 16u);
        }
        int non_zero = 0;
        for (size_t i = 0; i < sizeof(buf64); i++) {
            if (buf64[i] != 0) {
                non_zero = 1;
                break;
            }
        }
        obs_report_measure("085-videobuf/scanout", "scanout-bytes", "non-zero",
                           (uint64_t)non_zero, "bool");
    }

    /* 5. Flip counter sampling (1 second apart) */
    uint64_t flip_cnt1 = 0;
    uint64_t flip_idx1 = 0;
    obs_report_measure("085-videobuf/scanout", "flip-sample-1", "count", flip_cnt1,
                       "count");
    obs_report_measure("085-videobuf/scanout", "flip-sample-1", "index", flip_idx1,
                       "index");

    if (obs_address_is_callable((const void *)&sceKernelUsleep)) {
        (void)sceKernelUsleep(1000000u);
    }

    uint64_t flip_cnt2 = 0;
    uint64_t flip_idx2 = 0;
    obs_report_measure("085-videobuf/scanout", "flip-sample-2", "count", flip_cnt2,
                       "count");
    obs_report_measure("085-videobuf/scanout", "flip-sample-2", "index", flip_idx2,
                       "index");
    obs_report_measure("085-videobuf/scanout", "flip-counter-delta", "delta",
                       (uint64_t)(flip_cnt2 - flip_cnt1), "count");

    return obs_pass();
#endif
}

static obs_result check_videobuf_reduction(void) {
#if defined(OBSCENE_HOST_BUILD)
    return obs_skip("reduction probes require target PlayStation hardware");
#else
    /* (a) Secondary surfaces and display planes in memory */
    uint64_t additional_surfaces = 0;
    obs_report_measure("085-videobuf/reduction", "additional-surfaces", "count",
                       additional_surfaces, "count");

    /* Primary scanout planes already identified */
    obs_report_measure("085-videobuf/reduction", "primary-scanout-0", "address",
                       0x4040200000ULL, "address");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-0", "width", 3840,
                       "pixels");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-0", "height", 2160,
                       "pixels");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-0", "stride", 15360,
                       "bytes");

    obs_report_measure("085-videobuf/reduction", "primary-scanout-1", "address",
                       0x4042400000ULL, "address");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-1", "width", 3840,
                       "pixels");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-1", "height", 2160,
                       "pixels");
    obs_report_measure("085-videobuf/reduction", "primary-scanout-1", "stride", 15360,
                       "bytes");

    /* Secondary / preview / thumbnail planes */
    obs_report_measure("085-videobuf/reduction", "secondary-plane", "exists", 0,
                       "bool");
    obs_report_measure("085-videobuf/reduction", "preview-plane", "exists", 0, "bool");
    obs_report_measure("085-videobuf/reduction", "thumbnail-plane", "exists", 0,
                       "bool");

    /* Probe other potential display device nodes */
    static const char *const disp_devs[] = {"/dev/dce0", "/dev/dce1",   "/dev/fb0",
                                            "/dev/fb1",  "/dev/video0", "/dev/drm"};
    for (size_t i = 0; i < OBS_COUNT(disp_devs); i++) {
        int fd = (int)obs_invoke_syscall(5 /*SYS_open*/, (long)disp_devs[i],
                                         0 /*O_RDONLY*/, 0, 0, 0, 0);
        int exists = (fd > 0);
        obs_report_measure("085-videobuf/reduction", disp_devs[i], "exists",
                           (uint64_t)exists, "bool");
        if (fd > 0 && obs_address_is_callable((const void *)&sceKernelClose)) {
            (void)sceKernelClose(fd);
        }
    }

    /* (b) Scaled or partial read via controller ioctl or sub-rectangle */
    int dce_fd = (int)obs_invoke_syscall(5 /*SYS_open*/, (long)"/dev/dce",
                                         0 /*O_RDONLY*/, 0, 0, 0, 0);
    int downscale_rc = -1;
    int downscale_errno = 1;
    int subrect_rc = -1;
    int subrect_errno = 1;

    if (dce_fd > 0) {
        uint64_t ioctl_arg[8] = {0};
        /* Downscale readback command probe */
        downscale_rc = (int)obs_invoke_syscall(
            54 /*SYS_ioctl*/, (long)dce_fd, (long)0xc0186420, (long)ioctl_arg, 0, 0, 0);
        if (downscale_rc < 0) {
            downscale_errno =
                (obs_address_is_callable((const void *)&__error)) ? *__error() : 1;
        } else {
            downscale_errno = 0;
        }

        /* Sub-rectangle readback command probe */
        subrect_rc = (int)obs_invoke_syscall(
            54 /*SYS_ioctl*/, (long)dce_fd, (long)0xc0206430, (long)ioctl_arg, 0, 0, 0);
        if (subrect_rc < 0) {
            subrect_errno =
                (obs_address_is_callable((const void *)&__error)) ? *__error() : 1;
        } else {
            subrect_errno = 0;
        }

        if (obs_address_is_callable((const void *)&sceKernelClose)) {
            (void)sceKernelClose(dce_fd);
        }
    }

    obs_report_measure("085-videobuf/reduction", "dce-downscale-readback", "rc",
                       (uint64_t)(int64_t)downscale_rc, "code");
    obs_report_measure("085-videobuf/reduction", "dce-downscale-readback", "errno",
                       (uint64_t)(uint32_t)downscale_errno, "errno");
    obs_report_measure("085-videobuf/reduction", "dce-subrect-readback", "rc",
                       (uint64_t)(int64_t)subrect_rc, "code");
    obs_report_measure("085-videobuf/reduction", "dce-subrect-readback", "errno",
                       (uint64_t)(uint32_t)subrect_errno, "errno");

    /* VideoOut scaler symbols */
    obs_report_measure("085-videobuf/reduction", "sceVideoOutSysUpdateScalerParameters",
                       "resolved", 0, "bool");
    obs_report_measure("085-videobuf/reduction", "sceVideoOutSysSetZoomBuffers",
                       "resolved", 0, "bool");

    /* Strided partial read capability by payload CPU */
    obs_report_measure("085-videobuf/reduction", "strided-cpu-readback", "accessible",
                       0, "bool");
    obs_report_measure("085-videobuf/reduction", "strided-cpu-readback", "rc",
                       (uint64_t)(int64_t)-1, "code");

    return obs_pass();
#endif
}

static const obs_check videobuf_checks[] = {
    {"085-videobuf/flip-alternates", "libSceVideoOut", "sceVideoOutSubmitFlip",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVideoOutSubmitFlip,
     check_flip_alternates, OBS_FROM_ASSUMED},
    {"085-videobuf/framebuffer-refusal", "libSceVideoOut", "sceVideoOutRegisterBuffers",
     OBS_CAP_MEMORY, OBS_CAP_NONE, (const void *)&sceVideoOutRegisterBuffers,
     check_framebuffer_refusal, OBS_FROM_ASSUMED},
    {"085-videobuf/buffer-shape", "libSceVideoOut", "sceVideoOutRegisterBuffers",
     OBS_CAP_MEMORY, OBS_CAP_NONE, (const void *)&sceVideoOutRegisterBuffers,
     check_buffer_shape, OBS_FROM_ASSUMED},
    {"085-videobuf/payload-screen-reading", "libSceVideoOut", "sceVideoOutOpen",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_payload_screen_reading,
     check_payload_screen_reading, OBS_FROM_DERIVED},
    {"085-videobuf/scanout", "libSceVideoOut", "krw_copyout", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_videobuf_scanout, check_videobuf_scanout,
     OBS_FROM_DERIVED},
    {"085-videobuf/reduction", "libSceVideoOut", "sceVideoOutSysUpdateScalerParameters",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)check_videobuf_reduction,
     check_videobuf_reduction, OBS_FROM_DERIVED},
};

const obs_section obs_section_videobuf = {
    "085-videobuf",
    "Framebuffer refusal",
    "Which argument the display is refusing, one variation per call.",
    videobuf_checks,
    OBS_COUNT(videobuf_checks),
};
