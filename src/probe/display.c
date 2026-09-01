/*
 * Drawing the report to the screen.
 *
 * # Why a probe draws anything
 *
 * The text stream stays the contract - it is what `verify` and `diff` read. But it has
 * two failure modes that leave a run saying nothing at all: an emulator with no working
 * write path discards every byte, and a person watching a run sees a black window and
 * cannot tell a working probe from a hung one.
 *
 * A framebuffer fixes both, and it does so without any output function working. Text
 * for machines, pixels for people; either surviving alone beats the current
 * all-or-nothing.
 *
 * # The layout is not declared, on purpose
 *
 * `sceVideoOutSetBufferAttribute` fills the attribute structure, so this program never
 * needs to know what is in it. That is the difference between a confident signature and
 * an invented struct (D008): the buffer here is opaque and generously sized, and if the
 * real structure is larger than expected the call writes inside it rather than past it.
 *
 * # Nothing here may take the run down
 *
 * Every entry point is safe when the display was never opened, failed to open, or the
 * platform has none of the symbols. A probe that crashed while drawing its own results
 * would be worse than one that drew nothing, so each step checks its import, checks its
 * result, and gives up quietly with a reason the report can state.
 */

#include "obscene/harness.h"
#include "obscene/display.h"
#include "obscene/report.h"

#include "obscene/platform.h"
#include "obscene/runtime.h"

#if defined(OBSCENE_HOST_BUILD)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

/* 1080p, which every output supports. Asking the platform for its actual resolution
 * means reading a structure whose layout is not known, and being wrong about that is
 * worse than being conservative about this. */
#define OBS_FB_WIDTH 1920
#define OBS_FB_HEIGHT 1080
#define OBS_FB_PIXELS ((size_t)OBS_FB_WIDTH * (size_t)OBS_FB_HEIGHT)

/* The framebuffer's alignment, and this number is the whole reason the display never came up.
 *
 * `0x4000` was refused with `0x80290015` on every attempt this project ever made - through two
 * memory types, both generations' entry points, two resolutions, four pixel formats and two
 * pitches. Measured by `085-videobuf/buffer-shape`, which varies one thing per call:
 *
 *     onion,0x4000,1     0x80290015
 *     garlic,0x4000,1    0x80290015
 *     garlic,0x10000,1   0x0          accepted
 *
 * So `0x80290015` is the display saying the buffer is not aligned coarsely enough, and it says
 * it identically whatever else is wrong or right - which is why five sessions of varying the
 * *attribute* never moved it. (D253) */
#define OBS_FB_ALIGN 0x10000u

/* Rounded up to the granularity, because a direct-memory length that is not a multiple
 * of it is refused outright.
 *
 * 1920x1080x4 is 0x7E9000, which is 505.5 pages - and the refusal says only "no direct
 * memory", which reads as "not enough" rather than "not a whole number of pages". The
 * few kilobytes of slack are never drawn into. */
#define OBS_FB_RAW (OBS_FB_PIXELS * 4u)
#define OBS_FB_BYTES (((OBS_FB_RAW) + OBS_FB_ALIGN - 1u) & ~((size_t)OBS_FB_ALIGN - 1u))

/* Comfortably larger than the attribute structure is believed to be. Too large costs
 * stack; too small is a write past the end of it, inside a platform function, which
 * would be untraceable. */
#define OBS_ATTR_BYTES 256

/* Which generation's register/attribute pair the display uses. See the tie-break below. */
#ifndef OBS_DISPLAY_PAIR
#define OBS_DISPLAY_PAIR 0
#endif

/* Which direct-memory type the framebuffer is allocated from.
 *
 * `WB_ONION` is write-back and CPU-cached; `WC_GARLIC` is write-combined and the one a GPU
 * scans out of. This allocated onion memory and handed it to the display, which is a plausible
 * reason for a refusal that has nothing to do with the entry point or the attributes - and it
 * was never varied, so it was never eliminated. A flag, so it can be. (D252) */
#ifndef OBS_DISPLAY_MEM
#define OBS_DISPLAY_MEM OBS_MEM_TYPE_WC_GARLIC
#endif

/* Asserted, from public interface documentation:
 *   pixel format  32-bit BGRA, sRGB
 *   tiling        linear, which is what a buffer written by the CPU must be
 *   aspect ratio  16:9
 * A wrong value here shows up as wrong colours or a scrambled image rather than as a
 * crash, so this is a safe place to be asserting rather than deriving. */
#define OBS_PIXEL_FORMAT 0x80000000u
#define OBS_TILING_LINEAR 1u
#define OBS_ASPECT_16_9 0u

/* The same identifier again, for the current generation's call, which takes it as
 * **sixty-four bits** rather than thirty-two.
 *
 * Not a widening of the value above: it sits in the *upper* half. Passing the
 * previous generation's `0x80000000` into `sceVideoOutSetBufferAttribute2` therefore
 * describes a format nothing recognises, and this program did exactly that for as long as
 * the current-generation path has existed - undetected, because the only loader that
 * reaches it and checks is one obSCEne could not previously get this far on.
 *
 * `IMPLEMENTATIONS`: Kyty accepts `0x8000000000000000` or `0x8000000022000000` here and
 * refuses everything else. The plain one is chosen because it is the same identifier the
 * previous generation uses, in the wider field; the second differs in bits this program has
 * no way to interpret and no reason to set. */
#define OBS_PIXEL_FORMAT_2 0x8000000000000000ULL

/* The protection to map display memory with. The memory type comes from platform.h,
 * where the memory section already uses it. */
#define OBS_PROT_RW 0x33

static obs_display_state obs_state = OBS_DISPLAY_UNTRIED;
static const char *obs_state_text = "not attempted";
static uint32_t *obs_fb = 0;

#if !defined(OBSCENE_HOST_BUILD)
static int obs_video_handle = -1;

/* How many framebuffers are registered.
 *
 * **One is why the screen tore.** With a single buffer this program draws into the very memory
 * the display is scanning out of, continuously and with no synchronisation - so a frame shows
 * whatever had been written by the time the scanout beam reached each row. The symptom is
 * exact: the bottom of the screen is clean, because by the time the beam gets there the CPU
 * has finished, and the top is shredded.
 *
 * Two buffers removes it without introducing a wait. Draw into the one the display is *not*
 * showing, submit it, then draw into the other. Nothing blocks, so the rule that no probe may
 * hang (CLAUDE.md) is untouched - the flip stays mode 1, which does not wait for a vertical
 * blank.
 *
 * This is not a consequence of building without an SDK. It is an ordinary choice about how
 * many buffers to allocate, and every real implementation makes it the other way. (D256) */
#define OBS_FB_COUNT 2

/* Where the whole registered region begins, and which buffer is being drawn into.
 *
 * `obs_fb` points at the back buffer and moves on every flip; this is what it moves within.
 * Kept separately so the drawing code needs no changes at all - it writes to `obs_fb` and does
 * not know there are two. */
static uint32_t *obs_fb_base = 0;
static unsigned int obs_fb_index = 0;
/* Held so the allocation can be identified in a debugger. Nothing releases it: the
 * framebuffer lives as long as the process, and handing it back while the display is
 * still scanning out of it would be worse than leaking it at exit. */
static sce_off_t obs_fb_physical = 0;
#endif

int obs_display_holds_output(void) {
#if defined(OBSCENE_HOST_BUILD)
    return obs_state == OBS_DISPLAY_READY;
#else
    /* Whether the output handle is open, not whether the display came up.
     *
     * These are different, and reading the second for the first produced a finding this
     * program invented about the platform. The display opened the output, failed three steps
     * later at the framebuffer, and **never closed the handle** - so it was holding the main
     * output while answering "no" here. `080-video/open` then tried to open the same output,
     * got `0x80290009`, and reported "the main video output would not open" as a fact about
     * the console.
     *
     * The output is released on every give-up below now, so this is usually moot. It is still
     * written against the handle rather than the state, because the two questions are not the
     * same one and the caller is asking this one. (D250) */
    return obs_video_handle > 0;
#endif
}

int obs_display_width(void) {
    return obs_fb != 0 ? OBS_FB_WIDTH : 0;
}

int obs_display_height(void) {
    return obs_fb != 0 ? OBS_FB_HEIGHT : 0;
}

obs_display_state obs_display_status(void) {
    return obs_state;
}

const char *obs_display_status_text(void) {
    return obs_state_text;
}

/* The code the platform returned when it refused, or zero where there was no call.
 *
 * Seven places gave up here and not one of them kept the number. "The display refused the
 * framebuffer" is a sentence about a step; `0x80290009` is the platform's own account of why,
 * and it is the only part a reader can look up or compare between two consoles. Throwing it
 * away is the same fault as a check reporting `fail` with no code, which this program has
 * never done - the display path simply grew its own reporting and did not inherit the rule.
 * (D249) */
static uint64_t obs_state_code;

/* Hand the main output back if this program is holding it.
 *
 * Called from every give-up, because a give-up is a decision to stop and stopping while
 * holding the output is never right - it leaves the console's main display owned by a process
 * that has decided it cannot use it, for the life of that process. The video checks then
 * measure this program rather than the platform. (D250)
 *
 * The framebuffer itself is deliberately *not* released; see `obs_fb_physical`. Handing back
 * memory the display may still be scanning out of is worse than leaking it at exit. The output
 * handle is different: nothing is scanning out of an output whose buffers were refused. */
static void obs_release_output(void) {
#if !defined(OBSCENE_HOST_BUILD)
    if (obs_video_handle > 0 && obs_address_is_callable((const void *)&sceVideoOutClose)) {
        (void)sceVideoOutClose(obs_video_handle);
    }
    obs_video_handle = -1;
#endif
}

static obs_display_state obs_give_up(obs_display_state state, const char *why) {
    obs_release_output();
    obs_state = state;
    obs_state_text = why;
    obs_state_code = 0;
    return state;
}

/* Give up on a call that returned a code. Separate from `obs_give_up` rather than an extra
 * argument on it, so a site with nothing to report cannot pass a stale value by accident.
 *
 * Target-only: the host build draws into ordinary memory and has no platform call to get a
 * code from. Guarded rather than left for the linker to drop, because `-Werror` on an unused
 * static is what the build is for. */
#if !defined(OBSCENE_HOST_BUILD)
static obs_display_state obs_give_up_code(obs_display_state state, const char *why, int rc) {
    obs_release_output();
    obs_state = state;
    obs_state_text = why;
    obs_state_code = (uint64_t)(uint32_t)rc;
    return state;
}
#endif

#if defined(OBSCENE_HOST_BUILD)

/* The host build draws into ordinary memory and writes a PNG at the end.
 *
 * This is what makes the renderer checkable. Every pixel this program will ever put on
 * a console goes through the same clear, rect and text code below; here that code can
 * be run and *looked at* without an emulator, a module format, or a loader. A glyph
 * that is upside down is obvious in a PNG and invisible in a black window. (D001) */
obs_display_state obs_display_open(void) {
    if (obs_state != OBS_DISPLAY_UNTRIED) {
        return obs_state;
    }
    obs_fb = (uint32_t *)malloc(OBS_FB_BYTES);
    if (obs_fb == NULL) {
        return obs_give_up(OBS_DISPLAY_FAILED, "the host could not allocate a framebuffer");
    }
    obs_state = OBS_DISPLAY_READY;
    obs_state_text = "host framebuffer";
    return obs_state;
}

/* Minimal PNG: one IDAT of stored-mode deflate. Written by hand rather than pulled in
 * from a library, because the host build has no dependencies and this is not worth
 * acquiring one for. */
static unsigned long obs_crc_table[256];
static int obs_crc_ready = 0;

static unsigned long obs_crc(const unsigned char *buf, size_t len, unsigned long crc) {
    if (!obs_crc_ready) {
        for (unsigned long n = 0; n < 256; n++) {
            unsigned long c = n;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320uL ^ (c >> 1)) : (c >> 1);
            }
            obs_crc_table[n] = c;
        }
        obs_crc_ready = 1;
    }
    for (size_t i = 0; i < len; i++) {
        crc = obs_crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

static void obs_put_be32(unsigned char *p, unsigned long v) {
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static void obs_png_chunk(FILE *f, const char *tag, const unsigned char *data, size_t len) {
    unsigned char head[8];
    obs_put_be32(head, (unsigned long)len);
    memcpy(head + 4, tag, 4);
    fwrite(head, 1, 8, f);
    if (len != 0) {
        fwrite(data, 1, len, f);
    }
    unsigned long crc = obs_crc((const unsigned char *)tag, 4, 0xFFFFFFFFuL);
    crc = obs_crc(data, len, crc) ^ 0xFFFFFFFFuL;
    unsigned char tail[4];
    obs_put_be32(tail, crc);
    fwrite(tail, 1, 4, f);
}

/* The host build always presents: it writes the frame to a PNG, so "did anything reach a
 * screen" has a definite answer and it is yes. Stated here rather than left to the target
 * definition so the harness sees the same interface on both builds. */
int obs_display_presented(void) {
    return 1;
}

void obs_display_flip(void) {
    if (obs_state != OBS_DISPLAY_READY) {
        return;
    }
    const char *path = getenv("OBSCENE_DISPLAY_PNG");
    if (path == NULL) {
        path = "display.png";
    }
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return;
    }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);

    unsigned char ihdr[13];
    obs_put_be32(ihdr, OBS_FB_WIDTH);
    obs_put_be32(ihdr + 4, OBS_FB_HEIGHT);
    ihdr[8] = 8;  /* bit depth */
    ihdr[9] = 2;  /* truecolour */
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    obs_png_chunk(f, "IHDR", ihdr, sizeof ihdr);

    /* Raw scanlines: a filter byte then RGB triples. */
    size_t raw_len = (size_t)OBS_FB_HEIGHT * (1u + (size_t)OBS_FB_WIDTH * 3u);
    unsigned char *raw = (unsigned char *)malloc(raw_len);
    if (raw == NULL) {
        fclose(f);
        return;
    }
    size_t at = 0;
    for (int y = 0; y < OBS_FB_HEIGHT; y++) {
        raw[at++] = 0;
        for (int x = 0; x < OBS_FB_WIDTH; x++) {
            uint32_t p = obs_fb[(size_t)y * OBS_FB_WIDTH + (size_t)x];
            raw[at++] = (unsigned char)((p >> 16) & 0xFF);
            raw[at++] = (unsigned char)((p >> 8) & 0xFF);
            raw[at++] = (unsigned char)(p & 0xFF);
        }
    }

    /* zlib stream, stored blocks. No compression, which keeps this short and honest;
     * the file is a diagnostic, not an asset. */
    size_t z_cap = raw_len + (raw_len / 65535u + 1u) * 5u + 6u;
    unsigned char *z = (unsigned char *)malloc(z_cap);
    if (z == NULL) {
        free(raw);
        fclose(f);
        return;
    }
    size_t zi = 0;
    z[zi++] = 0x78;
    z[zi++] = 0x01;
    size_t left = raw_len, off = 0;
    while (left > 0) {
        size_t block = left > 65535u ? 65535u : left;
        z[zi++] = (unsigned char)(block == left ? 1 : 0);
        z[zi++] = (unsigned char)(block & 0xFF);
        z[zi++] = (unsigned char)(block >> 8);
        z[zi++] = (unsigned char)(~block & 0xFF);
        z[zi++] = (unsigned char)((~block >> 8) & 0xFF);
        memcpy(z + zi, raw + off, block);
        zi += block;
        off += block;
        left -= block;
    }
    unsigned long a = 1, b = 0;
    for (size_t i = 0; i < raw_len; i++) {
        a = (a + raw[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    obs_put_be32(z + zi, (b << 16) | a);
    zi += 4;

    obs_png_chunk(f, "IDAT", z, zi);
    obs_png_chunk(f, "IEND", (const unsigned char *)"", 0);
    fclose(f);
    free(z);
    free(raw);
}

#else /* target build */

obs_display_state obs_display_open(void) {
    if (obs_state != OBS_DISPLAY_UNTRIED) {
        return obs_state;
    }

    /* Every symbol first. An absent display is not a failure - it is a platform
     * without one - and saying so costs nothing. */
    /* Either generation's pair will do, so the register and attribute calls are checked
     * as *pairs* rather than individually. Requiring the previous generation's by name -
     * which this did - meant a current-generation platform reported "the display symbols
     * are not present" while carrying a complete set of them under different names. */
    int previous_pair = &sceVideoOutRegisterBuffers != 0 && &sceVideoOutSetBufferAttribute != 0;
    int current_pair = &sceVideoOutRegisterBuffers2 != 0 && &sceVideoOutSetBufferAttribute2 != 0;
    if (&sceVideoOutOpen == 0 || (!previous_pair && !current_pair) ||
        &sceVideoOutSubmitFlip == 0 ||
        &sceKernelAllocateDirectMemory == 0 || &sceKernelMapDirectMemory == 0 ||
        /* Missed by the original list, and the one that mattered most: it is called
         * inline as an argument to the allocation below, so it runs before anything
         * this function has said. This code path is the *first* platform interaction in
         * the whole program - ahead of the boot section - so a null here ended the run
         * with `OBS|build` as the last record: no try, no section, nothing naming the
         * display at all. */
        &sceKernelGetDirectMemorySize == 0) {
        return obs_give_up(OBS_DISPLAY_ABSENT, "the display symbols are not present");
    }

    /* The display is opened against a user, exactly as 080-video does. Without one
     * there is nobody to open an output for.
     *
     * Initialised first, which this did not do.
     *
     * `070-user/initialise` calls `sceUserServiceInitialize` and then
     * `070-user/initial-user` succeeds - but the display runs *before* any section, as the
     * program's first platform interaction, so it was asking an uninitialised service. One
     * loader answered anyway and another refused, and the refusal read as "this platform
     * has no users" when the report two hundred lines later said `initial-user pass
     * 0x10000000`.
     *
     * Initialising before use is right on every platform, so this is not a workaround for
     * the strict one - it is the order the interface documents, which the lenient one let
     * this program get away with ignoring. */
    int32_t user = 0;
    if (&sceUserServiceGetInitialUser == 0) {
        return obs_give_up(OBS_DISPLAY_ABSENT, "no initial user to open an output for");
    }
    if (sceUserServiceGetInitialUser(&user) != 0) {
        /* Only now, and this order is the whole point.
         *
         * Initialising first broke a loader that was working: it hangs inside
         * `sceUserServiceInitialize`, and the run stopped at `display|opening` with four
         * records where it had been drawing the full report. Asking first and initialising
         * only on refusal leaves every platform that already answers completely untouched,
         * and is a smaller claim besides - "this needed initialising" rather than "this
         * always needs initialising".
         *
         * The result of the initialise is not checked: already-initialised is a legitimate
         * answer that some platforms report as an error. What matters is whether the retry
         * below can name a user. */
        if (!obs_address_is_callable((const void *)&sceUserServiceInitialize)) {
            return obs_give_up(OBS_DISPLAY_ABSENT, "no initial user to open an output for");
        }
        (void)sceUserServiceInitialize(0);
        if (sceUserServiceGetInitialUser(&user) != 0) {
            return obs_give_up(OBS_DISPLAY_ABSENT,
                               "no initial user, even after initialising the service");
        }
    }
    obs_video_handle = sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0);
    if (obs_video_handle <= 0) {
        return obs_give_up_code(OBS_DISPLAY_FAILED, "the video output would not open",
                                obs_video_handle);
    }

    sce_off_t physical = 0;
    int rc = sceKernelAllocateDirectMemory(0, (sce_off_t)sceKernelGetDirectMemorySize(),
                                           OBS_FB_BYTES * OBS_FB_COUNT, OBS_FB_ALIGN,
                                           OBS_DISPLAY_MEM, &physical);
    if (rc != 0) {
        return obs_give_up_code(OBS_DISPLAY_FAILED, "no direct memory for a framebuffer", rc);
    }
    obs_fb_physical = physical;

    void *mapped = 0;
    rc = sceKernelMapDirectMemory(&mapped, OBS_FB_BYTES * OBS_FB_COUNT, OBS_PROT_RW, 0,
                                  physical,
                                  OBS_FB_ALIGN);
    if (rc != 0 || mapped == 0) {
        return obs_give_up_code(OBS_DISPLAY_FAILED, "the framebuffer would not map", rc);
    }
    obs_fb_base = (uint32_t *)mapped;
    obs_fb_index = 0;
    obs_fb = obs_fb_base;

    /* Filled by the platform, never by us. */
    unsigned char attribute[OBS_ATTR_BYTES];
    for (int i = 0; i < OBS_ATTR_BYTES; i++) {
        attribute[i] = 0;
    }
    /* Whichever generation's pair this platform actually has; the build generation breaks
     * a tie.
     *
     * "Did this symbol resolve" is still the primary signal, because the two generations
     * expose different entry points and that answers the question directly where
     * `005-generation` would only infer it (D110).
     *
     * # Why a tie exists at all, and why the older one used to win it
     *
     * Preferring the newer form once broke a loader that had been working: **fpPS4 installs
     * a logging stub for every import it cannot resolve**, so
     * `sceVideoOutSetBufferAttribute2` has a perfectly good non-null address there and does
     * nothing:
     *
     *     nop nid:libSceVideoOut:3E34B9B804B0715F:sceVideoOutSetBufferAttribute2
     *
     * An address proves a symbol resolved. It does not prove anything is behind it - the
     * thesis of `007-responsive` and `910-bulk`, arriving here uninvited.
     *
     * # The premise that justified always preferring the older one is false
     *
     * It was this: *"a current-generation platform does not offer the older form at all, so
     * there is no tie to lose."* Kyty offers both. It registers VideoOut twice - a
     * twelve-function set under module version `0.0` and a three-function set under `1.1` -
     * and patches every unresolved import to a trampoline, so **both pairs have non-null
     * addresses and only one of them is real**. A current-generation module then took the
     * previous-generation path into functions that do nothing, reported the display ready,
     * and drew a black window for the rest of the run.
     *
     * So the tie is broken by what this module actually is. `OBSCENE_GEN` is the generation
     * written into `EI_ABIVERSION`, which a loader reads before the first guest instruction -
     * it is a build fact, not a runtime one, and it is the only thing here that cannot be
     * faked by a stub. A platform offering exactly one pair is unaffected either way. */
    /* The tie-break, overridable, because the rule below was written against emulators and
     * hardware is a case it did not have.
     *
     * `OBS_DISPLAY_PAIR` is 0 to keep the rule, 1 to force the previous generation's pair, 2
     * to force the current one. On a real console both pairs resolve and the rule picks the
     * previous one from `EI_ABIVERSION 0`, which is a fact about what this module *is* rather
     * than about which entry point the console's `libSceVideoOut` actually implements. Those
     * were assumed to be the same question. Whether they are is a measurement, so it is a
     * flag and the experiment is one word. (D251) */
    int use_current = current_pair && (!previous_pair || OBSCENE_GEN >= 5);
#if OBS_DISPLAY_PAIR == 1
    use_current = 0;
#elif OBS_DISPLAY_PAIR == 2
    use_current = 1;
#endif

    /* Which pair was found and which was taken, before either is called.
     *
     * The display's own imports are named by no check, so nothing in the report said whether
     * they had bound - and when registration refused with `0x80290015` there was no way to
     * tell a bad parameter from a call into a function that was never there. The two need
     * completely different work.
     *
     * Announced before the call for the same reason every check is (CLAUDE.md, first
     * principle): if the registration does not return, this line is what says which of the
     * two entry points it did not return from. (D251) */
    obs_report_display(use_current ? "path-current" : "path-previous",
#if OBS_DISPLAY_PAIR != 0
                       "the build forced this pair, so no tie was broken",
#else
                       previous_pair && current_pair
                           ? "both generations' entry points resolved; the build generation "
                             "broke the tie"
                       : previous_pair ? "only the previous generation's entry points resolved"
                       : current_pair  ? "only the current generation's entry points resolved"
                                       : "neither generation's entry points resolved",
#endif
                       0);
    if (use_current) {
        /* The current generation's form: no aspect ratio and no pitch, and three extra
         * arguments carrying compression settings this program has no opinion about.
         * Zero for all of them is "none", which is what a linear untiled buffer wants. */
        sceVideoOutSetBufferAttribute2(attribute, OBS_PIXEL_FORMAT_2, OBS_TILING_LINEAR,
                                       OBS_FB_WIDTH, OBS_FB_HEIGHT, 0, 0, 0);

        /* And it takes buffer *descriptors* rather than bare addresses - thirty-two
         * bytes each, the address first and a metadata pointer second. That difference
         * is the reason this cannot be a one-line swap of the function called.
         *
         * Declared here rather than in platform.h because it is a structure this program
         * fills, not one the platform fills: the layout has to be right or the display
         * reads our stack as a pointer. Two independent implementations agree on the
         * stride and on the first field, which is what makes writing it defensible at
         * all (D111). */
        struct obs_video_buffer {
            void *data;
            void *metadata;
            void *reserved[2];
        } buffers[1];
        for (unsigned int i = 0; i < sizeof(buffers); i++) {
            ((unsigned char *)buffers)[i] = 0;
        }
        buffers[0].data = mapped;

        rc = sceVideoOutRegisterBuffers2(obs_video_handle, 0, 0, buffers, 1, attribute, 0,
                                         0);
    } else {
        sceVideoOutSetBufferAttribute(attribute, OBS_PIXEL_FORMAT, OBS_TILING_LINEAR,
                                      OBS_ASPECT_16_9, OBS_FB_WIDTH, OBS_FB_HEIGHT,
                                      OBS_FB_WIDTH);

        void *addresses[OBS_FB_COUNT];
        for (unsigned int i = 0; i < OBS_FB_COUNT; i++) {
            addresses[i] = (void *)((unsigned char *)mapped + (size_t)i * OBS_FB_BYTES);
        }
        rc = sceVideoOutRegisterBuffers(obs_video_handle, 0, addresses, OBS_FB_COUNT,
                                        attribute);
    }
    if (rc != 0) {
        obs_fb = 0;
        return obs_give_up_code(OBS_DISPLAY_FAILED, "the display refused the framebuffer", rc);
    }

    obs_state = OBS_DISPLAY_READY;
    obs_state_text = "1920x1080 framebuffer";
    return obs_state;
}

/* Room for the whole status structure and then some.
 *
 * The two implementations here write 96 and 128 bytes; only the first eight are read. Sized
 * far past both because the risk is entirely one-sided - a platform writing more than
 * expected corrupts whatever follows a short buffer, and a few unused bytes cost nothing. */
#define OBS_FLIP_STATUS_BYTES 256

/* Frames the display says it has presented, or zero when it will not say.
 *
 * Zero is indistinguishable from "none yet", which is correct: before the first flip there
 * genuinely have been none, and a platform that refuses the call is telling us nothing rather
 * than telling us none. Only a *change* in this number is evidence, which is why the caller
 * compares two readings rather than testing one. */
static uint64_t obs_flip_count(void) {
    if (!obs_address_is_callable((const void *)&sceVideoOutGetFlipStatus)) {
        return 0;
    }
    unsigned char status[OBS_FLIP_STATUS_BYTES];
    for (unsigned int i = 0; i < sizeof(status); i++) {
        status[i] = 0;
    }
    if (sceVideoOutGetFlipStatus(obs_video_handle, status) != 0) {
        return 0;
    }
    /* The first field, assembled a byte at a time rather than read through a cast: the
     * buffer is a plain array with no alignment guarantee, and a misaligned 64-bit load is
     * undefined even where it happens to work. */
    uint64_t count = 0;
    for (unsigned int i = 0; i < 8; i++) {
        count |= (uint64_t)status[i] << (i * 8);
    }
    return count;
}

/* Whether a frame has ever demonstrably reached the screen.
 *
 * Three states, because two would lie. `-1` is "not established" - nothing has been shown
 * *and* nothing says it has not, which is where a platform without a working status call
 * stays forever. */
static int obs_presented = -1;

/* Whether the question has been asked yet, kept separate from the answer: `obs_presented`
 * has three values and none of them means "not tried". */
static int obs_present_tested;

int obs_display_presented(void) {
    return obs_presented;
}

/* Wait, briefly and with a hard bound, for a submitted flip to take effect.
 *
 * Two buffers alone do not stop tearing. Submitting a flip does not mean the display has
 * switched - it means the request was accepted - so with no wait at all this program starts
 * drawing into the other buffer immediately, and if the switch has not happened yet that other
 * buffer is still the one on screen. The result is exactly what a single buffer does, arriving
 * one frame later.
 *
 * **Bounded, never blocking.** `CLAUDE.md` is explicit that anything which can block is
 * written as a try or not at all, and a probe that waits forever for a vertical blank on a
 * platform whose vblank never arrives loses the entire report it is holding. So this gives up
 * after a fixed number of polls and returns; the worst case is the tearing that was there
 * before, on a platform that cannot present anyway.
 *
 * `sceKernelUsleep` is not used to pace the polls: it is a function this program *measures*
 * (`050-time`), and a display path that depended on it would be reporting on itself. The poll
 * is a plain read of the counter the platform already exposes. (D257)
 */
static void obs_wait_for_flip(uint64_t before) {
    if (!obs_address_is_callable((const void *)&sceVideoOutGetFlipStatus)) {
        return;
    }
    /* Sized to be comfortably longer than one frame at any rate a display in this class runs
     * at, and short enough that a platform which never advances the counter costs a bounded
     * pause per flip rather than a run. */
    for (unsigned int spin = 0; spin < 200000u; spin++) {
        if (obs_flip_count() != before) {
            return;
        }
    }
}

void obs_display_flip(void) {
    if (obs_state != OBS_DISPLAY_READY) {
        return;
    }
    /* Flip mode 1 is the one that does not wait for a vertical blank. A probe that
     * blocked here on a platform whose vblank never arrives would hang holding a
     * complete report, which is the worst outcome available. */
    /* Submit the buffer that was just drawn, then move to the other one.
     *
     * The index is advanced *before* the outcome is known deliberately: if the flip is
     * refused the display is being given up on anyway, and if it succeeds the buffer just
     * submitted is the one being scanned out - so nothing may draw into it again. Advancing
     * only on success would leave the failure path drawing into the live buffer, which is the
     * exact fault two buffers exist to remove. */
    unsigned int shown = obs_fb_index;
    obs_fb_index = (obs_fb_index + 1u) % OBS_FB_COUNT;
    obs_fb = (uint32_t *)((unsigned char *)obs_fb_base + (size_t)obs_fb_index * OBS_FB_BYTES);

    uint64_t before = obs_flip_count();
    int rc = sceVideoOutSubmitFlip(obs_video_handle, (int)shown, 1, 0);
    obs_wait_for_flip(before);
    if (rc != 0) {
        /* Stop, and stay stopped.
         *
         * A flip that is refused usually means the registration is gone, and repeating
         * it is not harmless: one emulator answered the second refused flip by faulting
         * inside its own presenter thread. Refusing to keep asking turns a lost screen
         * into a lost screen, rather than into a lost run.
         *
         * What is already drawn stays on the display. */
        (void)obs_give_up(OBS_DISPLAY_FAILED, "a flip was refused; the screen is frozen");
        return;
    }

    /* Accepted. Now find out whether anything was actually shown.
     *
     * These are different questions and the platform can answer yes to the first while the
     * answer to the second is no - which is exactly what a partly implemented display library
     * does. Every call succeeds, nothing appears, and a report built from return codes says
     * the display is fine.
     *
     * Recorded once, on the first flip that moves the counter, and never revisited: what is
     * being established is *whether this platform can present at all*, and one frame settles
     * that. Checking every flip would turn a report into a frame counter. */
    if (!obs_present_tested) {
        obs_present_tested = 1;
        if (!obs_address_is_callable((const void *)&sceVideoOutGetFlipStatus)) {
            /* Not a failure - a silence. A platform with no way to report frame counts has
             * not told us it cannot present, it has told us nothing, and a report that
             * confused those would be making the same overclaim in the other direction. */
            obs_presented = -1;
        } else {
            /* Polled, not read once, and this was the first version's mistake.
             *
             * A flip is *queued*. The counter moves when a presenter picks it up, which has
             * not happened yet at the instant the call returns - so reading immediately
             * reported "blind" for shadPS4, which draws the report perfectly well and whose
             * implementation of this call is entirely correct. A test that says a working
             * display is broken is worse than no test.
             *
             * Bounded, and short: twenty tries at five milliseconds is a tenth of a second,
             * which is several frames at any refresh rate a display has. A platform slower
             * than that is not one this can measure, and waiting longer to find out would
             * cost every run. */
            for (int i = 0; i < 20; i++) {
                if (obs_flip_count() != before) {
                    obs_presented = 1;
                    break;
                }
                if (!obs_address_is_callable((const void *)&sceKernelUsleep)) {
                    break;
                }
                (void)sceKernelUsleep(5000u);
            }
            if (obs_presented != 1) {
                obs_presented = 0;
            }
        }
    }
}

#endif

void obs_display_clear(obs_colour colour) {
    if (obs_state != OBS_DISPLAY_READY || obs_fb == 0) {
        return;
    }
    for (size_t i = 0; i < OBS_FB_PIXELS; i++) {
        obs_fb[i] = colour;
    }
}

void obs_display_rect(int x, int y, int w, int h, obs_colour colour) {
    if (obs_state != OBS_DISPLAY_READY || obs_fb == 0) {
        return;
    }
    if (w <= 0 || h <= 0) {
        return;
    }
    /* Clipped rather than trusted. A caller computing a position from a count can walk
     * off the edge, and writing past a framebuffer is a fault inside somebody else's
     * memory. */
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > OBS_FB_WIDTH ? OBS_FB_WIDTH : x + w;
    int y1 = y + h > OBS_FB_HEIGHT ? OBS_FB_HEIGHT : y + h;
    for (int py = y0; py < y1; py++) {
        uint32_t *row = obs_fb + (size_t)py * OBS_FB_WIDTH;
        for (int px = x0; px < x1; px++) {
            row[px] = colour;
        }
    }
}

/* Folds case and maps anything outside the table to the hollow box, so a missing glyph
 * is visible rather than a gap. */
static const unsigned char *obs_glyph(char c) {
    int code = (unsigned char)c;
    if (code >= 'a' && code <= 'z') {
        code -= 32;
    }
    if (code < OBS_FONT_FIRST || code > OBS_FONT_LAST) {
        code = '?';
    }
    return obs_font[code - OBS_FONT_FIRST];
}

int obs_display_text(int x, int y, const char *text, obs_colour colour, int scale) {
    if (text == 0) {
        return x;
    }
    if (scale < 1) {
        scale = 1;
    }
    int at = x;
    for (const char *p = text; *p != '\0'; p++) {
        if (*p == '\n') {
            continue;
        }
        const unsigned char *glyph = obs_glyph(*p);
        for (int row = 0; row < OBS_FONT_HEIGHT; row++) {
            unsigned char bits = glyph[row];
            for (int col = 0; col < OBS_FONT_WIDTH; col++) {
                if ((bits >> (7 - col)) & 1) {
                    obs_display_rect(at + col * scale, y + row * scale, scale, scale,
                                     colour);
                }
            }
        }
        at += OBS_FONT_WIDTH * scale;
    }
    return at;
}

uint64_t obs_display_status_code(void) {
    return obs_state_code;
}
