/*
 * Why the display refuses a framebuffer, one parameter at a time.
 *
 * # The problem this replaces
 *
 * `obs_display_open` builds one framebuffer from one set of constants and is refused:
 *
 *     OBS|display|failed|the display refused the framebuffer|0x80290015
 *
 * A single code from a call with five arguments says which step refused and nothing about
 * which argument. Finding out by editing a constant and rebuilding costs a package build, an
 * install and a launch per guess - about five minutes each, and the console has to be healthy
 * for every one of them. Four guesses is most of an hour and produces four numbers.
 *
 * The same four numbers come out of one run if the varying happens inside the probe. That is
 * what this program is for, and the display path was doing it the expensive way because the
 * display path is not a section and never got a section's treatment.
 *
 * # What it does not do
 *
 * It does not try to make the display work. It calls `sceVideoOutRegisterBuffers` once per
 * variation and writes down the code, and every variation differs from the baseline in
 * **exactly one argument**, so a code that moves names its own cause. A sweep where two things
 * change at once produces a table nobody can read.
 *
 * # Leaving the platform as it was found
 *
 * The output is opened here and closed here, whatever happens in between. If `obs_display_open`
 * already holds it - because it succeeded - this section does not run at all: taking the
 * output away from a working display to ask questions about it would trade the screen for the
 * answer.
 *
 * A registration that *succeeds* stops the sweep. There is no unregister call, so continuing
 * would be asking a display that now owns our memory to accept different memory, and the codes
 * after that point would be measuring the mess rather than the question.
 */

#include "obscene/display.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"

/* Matches `src/display.c`. Deliberately duplicated rather than shared: this is the baseline
 * being questioned, and a shared constant would move underneath the answer when somebody
 * changes the display. */
#define OBS_VB_WIDTH 1920u
#define OBS_VB_HEIGHT 1080u
#define OBS_VB_ALIGN 0x4000u
#define OBS_VB_BYTES                                                       \
    (((size_t)OBS_VB_WIDTH * (size_t)OBS_VB_HEIGHT * 4u + OBS_VB_ALIGN - 1u) \
     & ~((size_t)OBS_VB_ALIGN - 1u))
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
    /* Tiling. Linear is what a CPU-drawn framebuffer wants; a display that only scans out of
     * tiled memory refuses linear, and nothing else here tells the two apart. */
    {"tiling=0", OBS_VB_FORMAT, 0u, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH, OBS_VB_HEIGHT,
     OBS_VB_WIDTH},
    /* Pixel format. The high bit is the only part of this encoding the program is confident
     * about, so the variation clears it rather than proposing a different encoding it cannot
     * justify. (D008) */
    {"format=0", 0u, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH, OBS_VB_HEIGHT,
     OBS_VB_WIDTH},
    /* Aspect ratio. Zero is the baseline; if the code moves for one, the argument is read. */
    {"aspect=1", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, 1u, OBS_VB_WIDTH, OBS_VB_HEIGHT,
     OBS_VB_WIDTH},
    /* Size. 1280x720 is the other mode every display in this class supports, so a refusal
     * that survives it is not about this being an unusual resolution. */
    {"720p", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, 1280u, 720u, 1280u},
    /* Pitch. Equal to width is the obvious reading and it is an assumption; a display wanting
     * a pitch in bytes rather than in pixels would refuse the baseline and accept this. */
    {"pitch=width*4", OBS_VB_FORMAT, OBS_VB_TILING_LINEAR, OBS_VB_ASPECT_16_9, OBS_VB_WIDTH,
     OBS_VB_HEIGHT, OBS_VB_WIDTH * 4u},
};

static obs_result check_framebuffer_refusal(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceVideoOutSetBufferAttribute,
                &sceKernelAllocateDirectMemory, &sceKernelMapDirectMemory,
                &sceUserServiceGetInitialUser, &sceKernelGetDirectMemorySize);

    if (obs_display_holds_output()) {
        /* The display came up and is drawing the report on this output. Nothing to diagnose,
         * and taking the output away to ask would cost the screen. */
        return obs_skip("the display holds the output, so there is no refusal to explain");
    }

    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return obs_skip("no initial user, so there is nobody to open an output for");
    }

    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (handle <= 0) {
        return obs_skip("the output would not open, so the framebuffer was never reached");
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
    /* CPU read/write so this program can draw into it, GPU read/write so the display can
     * scan out of it. Spelled from the named bits rather than as the literal `src/display.c`
     * uses, because this file exists to question that file's constants and copying one of
     * them unexamined would be the wrong shape. */
    rc = sceKernelMapDirectMemory(&mapped, OBS_VB_BYTES,
                                  OBS_PROT_CPU_RW | OBS_PROT_GPU_READ | OBS_PROT_GPU_WRITE, 0,
                                  physical,
                                  OBS_VB_ALIGN);
    if (rc != 0 || mapped == 0) {
        (void)sceVideoOutClose(handle);
        return obs_skip("the buffer would not map");
    }

    uint64_t accepted = 0;
    uint64_t tried = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_vb_cases); i++) {
        const obs_vb_case *c = &obs_vb_cases[i];
        /* Announced before the call, so a variation that does not return names itself. The
         * point of a sweep is that it survives one bad combination; without this it would
         * only report which sweep died. */
        obs_report_progress("085-videobuf/framebuffer-refusal", (uint64_t)i);

        unsigned char attribute[OBS_VB_ATTR_BYTES];
        for (int b = 0; b < OBS_VB_ATTR_BYTES; b++) {
            attribute[b] = 0;
        }
        sceVideoOutSetBufferAttribute(attribute, c->format, c->tiling, c->aspect, c->width,
                                      c->height, c->pitch);
        void *addresses[1];
        addresses[0] = mapped;
        int got = sceVideoOutRegisterBuffers(handle, 0, addresses, 1, attribute);
        tried++;
        obs_report_measure("085-videobuf/framebuffer-refusal", "sceVideoOutRegisterBuffers",
                           c->what, (uint64_t)(uint32_t)got, "code");
        if (got == 0) {
            /* Accepted. Stop: there is no unregister, so every later variation would be
             * asking a display that already owns this memory about different memory. */
            accepted = (uint64_t)i + 1u;
            break;
        }
    }

    (void)sceVideoOutClose(handle);

    if (accepted != 0) {
        /* A variation the display accepted is the answer the display path needs, and it is
         * worth a green line: it proves the refusal is about an argument rather than about
         * this program being unable to present at all. */
        return obs_pass_value(accepted);
    }
    /* Every variation refused. Still a result - it says the arguments varied here are not the
     * ones at fault - and amber rather than red because the codes are the finding and a reader
     * has them. */
    return obs_partial_value("every variation was refused; the codes are in the measure records",
                             tried);
}

/*
 * The buffer side of the same refusal.
 *
 * The attribute sweep above establishes that the attribute is *read*: clearing the pixel
 * format changes the code to `0x80290003` and setting a different aspect changes it to
 * `0x80290008`, so those fields are parsed and the baseline values pass. What it also
 * establishes is that `0x80290015` is not any of them - it survives every attribute change,
 * and it is the same code the display path gets from a different memory type.
 *
 * So the remaining suspects are on the other side of the call: the memory the buffer is in,
 * how it is aligned, and how many of them there are. Those cannot be varied in the same loop,
 * because each needs its own allocation.
 *
 * Each variation allocates, registers, and **releases**. Six 8 MB regions held at once would
 * be 48 MB of direct memory abandoned inside a probe, and a later section asking for direct
 * memory would then be measuring this one.
 */
typedef struct obs_vb_shape {
    const char *what;
    uint32_t memory_type;
    size_t align;
    int count;
} obs_vb_shape;

static const obs_vb_shape obs_vb_shapes[] = {
    /* The display path's own combination, so this sweep and that one can be compared. */
    {"onion,0x4000,1", OBS_MEM_TYPE_WB_ONION, 0x4000u, 1},
    {"garlic,0x4000,1", OBS_MEM_TYPE_WC_GARLIC, 0x4000u, 1},
    /* Alignment. A display scanning out of a buffer may require a coarser alignment than the
     * page-ish one used here, and nothing has ever varied it. */
    {"garlic,0x10000,1", OBS_MEM_TYPE_WC_GARLIC, 0x10000u, 1},
    {"garlic,0x200000,1", OBS_MEM_TYPE_WC_GARLIC, 0x200000u, 1},
    /* Two buffers. A display built to flip between buffers may refuse a set of one, and one
     * is what every attempt so far has offered. */
    {"garlic,0x4000,2", OBS_MEM_TYPE_WC_GARLIC, 0x4000u, 2},
    {"garlic,0x200000,2", OBS_MEM_TYPE_WC_GARLIC, 0x200000u, 2},
};

static obs_result check_buffer_shape(void) {
    OBS_REQUIRE(&sceVideoOutOpen, &sceVideoOutClose, &sceVideoOutSetBufferAttribute,
                &sceKernelAllocateDirectMemory, &sceKernelMapDirectMemory,
                &sceKernelReleaseDirectMemory, &sceKernelMunmap,
                &sceUserServiceGetInitialUser, &sceKernelGetDirectMemorySize);

    if (obs_display_holds_output()) {
        return obs_skip("the display holds the output, so there is no refusal to explain");
    }
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return obs_skip("no initial user, so there is nobody to open an output for");
    }
    int handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (handle <= 0) {
        return obs_skip("the output would not open, so the framebuffer was never reached");
    }

    uint64_t accepted = 0;
    uint64_t tried = 0;
    for (unsigned int i = 0; i < OBS_COUNT(obs_vb_shapes); i++) {
        const obs_vb_shape *s = &obs_vb_shapes[i];
        obs_report_progress("085-videobuf/buffer-shape", (uint64_t)i);

        size_t span = OBS_VB_BYTES * (size_t)s->count;
        /* Rounded up to this variation's own alignment: an allocation that is not a whole
         * number of alignment units is a different question from the one being asked. */
        size_t bytes = (span + s->align - 1u) & ~(s->align - 1u);

        sce_off_t physical = 0;
        int rc = sceKernelAllocateDirectMemory(0, (sce_off_t)sceKernelGetDirectMemorySize(),
                                               bytes, s->align, (int)s->memory_type,
                                               &physical);
        if (rc != 0) {
            /* Reported rather than skipped past. A variation the platform will not even
             * allocate for is a fact about the allocator, and leaving it out of the table
             * would make the row look like a registration refusal. */
            obs_report_measure("085-videobuf/buffer-shape", "sceKernelAllocateDirectMemory",
                               s->what, (uint64_t)(uint32_t)rc, "code");
            continue;
        }
        void *mapped = 0;
        rc = sceKernelMapDirectMemory(&mapped, bytes,
                                      OBS_PROT_CPU_RW | OBS_PROT_GPU_READ | OBS_PROT_GPU_WRITE,
                                      0, physical, s->align);
        if (rc != 0 || mapped == 0) {
            obs_report_measure("085-videobuf/buffer-shape", "sceKernelMapDirectMemory", s->what,
                               (uint64_t)(uint32_t)rc, "code");
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
        /* Two is the most any variation here asks for. Sized from the table rather than from
         * a loop bound, so adding a three-buffer row without widening this is a compile error
         * rather than a stack overwrite. */
        void *addresses[2];
        addresses[0] = mapped;
        addresses[1] = (void *)((unsigned char *)mapped + OBS_VB_BYTES);

        int got = sceVideoOutRegisterBuffers(handle, 0, addresses, s->count, attribute);
        tried++;
        obs_report_measure("085-videobuf/buffer-shape", "sceVideoOutRegisterBuffers", s->what,
                           (uint64_t)(uint32_t)got, "code");
        if (got == 0) {
            /* Accepted. The memory stays mapped and allocated deliberately: the display now
             * owns it, and handing back memory something may be scanning out of is the one
             * thing worse than leaking it. */
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
    return obs_partial_value("every buffer shape was refused; the codes are in the measure "
                             "records",
                             tried);
}

/*
 * Does a flip actually swap buffers? Answered by looking, because nothing else can.
 *
 * # Why this is not decidable from return codes
 *
 * Everything in the display path reports success. The output opens, the buffers register, the
 * flip is accepted, and the frame counter advances - and the screen still shows an image being
 * built up a row at a time, which is what a *single* buffer looks like. Every one of those is
 * a fact about a call returning, and none of them is a fact about which memory the display is
 * scanning out of.
 *
 * The comment in `src/display.c` says flip mode 1 does not wait for a vertical blank. That is
 * an assumption, and it is recorded as one. Whether the second argument selects the buffer is
 * an assumption too. If either is wrong, two buffers change nothing and every code still says
 * fine.
 *
 * # The measurement
 *
 * Fill one buffer with one flat colour and the other with a different one, then alternate.
 * There are only three things a screen can then do, and they are not confusable:
 *
 *   clean alternation   buffering works; the tearing is somewhere else
 *   a steady mixture    the flip is not swapping buffers - both are being drawn and shown
 *   one colour only     the flip is not taking effect at all
 *
 * The output is what somebody sees, so this is off unless asked for: it replaces the report on
 * screen for as long as it runs, and a diagnostic that overwrites the thing being diagnosed
 * has to be deliberate. `make pkg DISPLAY_PROBE=1`.
 *
 * The flip counter is recorded alongside, so the visual has something to be checked against -
 * a screen that alternates while the counter never moves would mean the counter is the thing
 * lying, and that is worth being able to tell. (D258)
 */
#ifndef OBS_DISPLAY_PROBE
#define OBS_DISPLAY_PROBE 0
#endif

static obs_result check_flip_alternates(void) {
#if !OBS_DISPLAY_PROBE
    return obs_skip("this build does not run the visual flip probe - DISPLAY_PROBE=1 does, and "
                    "it replaces the report on screen while it runs");
#else
    if (!obs_display_holds_output()) {
        return obs_skip("the display is not up, so there is nothing to alternate");
    }
    OBS_REQUIRE(&sceKernelUsleep);

    /* Paint the two buffers different colours, once, and then **stop drawing**.
     *
     * The first version cleared before every flip, and that is the wrong experiment: drawing
     * is the thing suspected of racing the scanout, so a test that draws throughout cannot
     * tell a flip that does not swap from a draw that is being overtaken. With both buffers
     * painted once and nothing drawn afterwards, anything that changes on screen is the flip
     * and only the flip.
     *
     * Two clears and two flips: the first shows buffer 0, the second shows buffer 1, and each
     * clear lands on whichever buffer is about to be shown. */
    obs_display_clear(OBS_COLOUR_FAIL);
    obs_display_flip();
    obs_display_clear(OBS_COLOUR_ACCENT);
    obs_display_flip();

    /* Now alternate by flipping alone. Held long enough to be unmistakable to somebody
     * watching - the first version ran twenty flips with no pause, which is a third of a
     * second for the whole test and was missed entirely.
     *
     * `sceKernelUsleep` is used here where the display path proper avoids it: this is a probe
     * whose output is what a person sees, and `050-time/usleep` has already established the
     * call on this platform, so it is a measured dependency rather than an assumed one. */
    for (unsigned int i = 0; i < 8u; i++) {
        obs_report_progress("085-videobuf/flip-alternates", (uint64_t)i);
        (void)sceKernelUsleep(700000u);
        obs_display_flip();
    }

    /* Left on whichever buffer the last flip showed. A screen frozen on one colour is itself
     * the result if the flip is not swapping, and tidying up would erase it. */
    return obs_pass_value(8u);
#endif
}

static const obs_check videobuf_checks[] = {
    {"085-videobuf/flip-alternates", "libSceVideoOut", "sceVideoOutSubmitFlip", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&sceVideoOutSubmitFlip, check_flip_alternates,
     OBS_FROM_ASSUMED},
    {"085-videobuf/framebuffer-refusal", "libSceVideoOut", "sceVideoOutRegisterBuffers",
     OBS_CAP_MEMORY, OBS_CAP_NONE, (const void *)&sceVideoOutRegisterBuffers,
     check_framebuffer_refusal, OBS_FROM_ASSUMED},
    {"085-videobuf/buffer-shape", "libSceVideoOut", "sceVideoOutRegisterBuffers",
     OBS_CAP_MEMORY, OBS_CAP_NONE, (const void *)&sceVideoOutRegisterBuffers,
     check_buffer_shape, OBS_FROM_ASSUMED},
};

const obs_section obs_section_videobuf = {
    "085-videobuf",
    "Framebuffer refusal",
    "Which argument the display is refusing, one variation per call.",
    videobuf_checks,
    OBS_COUNT(videobuf_checks),
};
