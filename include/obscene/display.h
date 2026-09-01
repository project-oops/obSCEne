/*
 * Drawing the report to the screen.
 *
 * # Why a probe draws anything at all
 *
 * The report is text and text is what can be diffed, so the stream stays the contract.
 * But a run on an emulator with no readable output channel produces nothing at all, and
 * a run watched by a person produces a black window - neither says whether the thing is
 * alive. A framebuffer says so at a glance, and it says it without any output function
 * working.
 *
 * That makes this a second, independent report: text for machines, pixels for people,
 * and either one surviving on its own is better than the current all-or-nothing.
 *
 * # It must never take the run down
 *
 * Every function here is safe to call when the display was never opened, or failed to
 * open, or the platform has none of the symbols. `obs_display_open` reports what
 * happened and everything else quietly does nothing. A probe that crashed while drawing
 * its own results would be worse than one that drew nothing.
 */

#ifndef OBSCENE_DISPLAY_H
#define OBSCENE_DISPLAY_H

#include <stdint.h>

/* The font table in font.c. Generated from glyph art by scripts/gen-font.py. */
#define OBS_FONT_FIRST 0x20
#define OBS_FONT_LAST 0x5F
#define OBS_FONT_COUNT (OBS_FONT_LAST - OBS_FONT_FIRST + 1)
#define OBS_FONT_WIDTH 8
#define OBS_FONT_HEIGHT 8

extern const unsigned char obs_font[OBS_FONT_COUNT][OBS_FONT_HEIGHT];

/* Packed 0xAARRGGBB, which is what the framebuffer format expects.
 *
 * Prefixed `OBS_COLOUR_` rather than named after the verdicts they draw. The short
 * names collide with the `obs_status` values, and a colour macro quietly shadowing an
 * enum constant turns a `switch` over statuses into a switch over integers - which the
 * compiler then, correctly, refuses. */
typedef uint32_t obs_colour;

#define OBS_COLOUR_INK 0xFFE6EAF0u
#define OBS_COLOUR_GROUND 0xFF0D1116u
#define OBS_COLOUR_DIM 0xFF78828Fu
#define OBS_COLOUR_PASS 0xFF57BF93u
#define OBS_COLOUR_PARTIAL 0xFFD3A545u
#define OBS_COLOUR_FAIL 0xFFE0776Fu
#define OBS_COLOUR_SKIP 0xFF6F7986u
#define OBS_COLOUR_ACCENT 0xFF74A8D6u
/* The three letters in the middle of the name, which are the platform's and not this
 * project's. Drawn apart from the rest of the wordmark rather than mentioned in prose,
 * which is the same reasoning as principle 5: the vendor's name is load-bearing in the
 * ABI and nowhere else, so it is marked rather than spelled out.
 *
 * Chosen to sit with the verdict colours rather than shout over them, so the banner
 * reads as one palette.
 *
 * The value is the project's own green, taken from `assets/logo.svg` rather than picked
 * to match it - a colour chosen by eye drifts from the artwork the first time either
 * changes, and the two are meant to be the same green. It was a purple until the
 * branding moved.
 *
 * **It is now in the same family as `OBS_COLOUR_PASS`**, and the older wording here
 * claimed the mark "deliberately matches no status". That was true of the purple and is
 * not true of this. The two are distinguishable - this one is brighter and more
 * saturated, and the mark only ever appears inside the wordmark where no verdict is
 * drawn - but the claim was worth correcting rather than leaving to read as though it
 * still held. */
#define OBS_COLOUR_MARK 0xFF3DDC84u

/* How the display came up, reported into the stream so a run says whether what is on
 * the screen can be believed. */
typedef enum obs_display_state {
    OBS_DISPLAY_UNTRIED,
    /* Open, mapped and registered. Anything drawn will be seen. */
    OBS_DISPLAY_READY,
    /* The platform does not have the symbols. Not a failure, just no screen. */
    OBS_DISPLAY_ABSENT,
    /* The symbols are there and a step refused. The reason is in the report. */
    OBS_DISPLAY_FAILED
} obs_display_state;

/* Brings up a framebuffer, or explains why not. Safe to call once; later calls return
 * the state already reached. */
obs_display_state obs_display_open(void);

/* What `obs_display_open` concluded, for the report to state. */
obs_display_state obs_display_status(void);
const char *obs_display_status_text(void);

/* The code the platform returned when it refused, or zero if no call reported one.
 *
 * Separate from the text because the text is this program's sentence about the step and
 * the code is the platform's own answer. A reader can look the second one up; the first
 * one only says which step. (D249) */
uint64_t obs_display_status_code(void);

/* All no-ops unless the state is READY. */
void obs_display_clear(obs_colour colour);
void obs_display_rect(int x, int y, int w, int h, obs_colour colour);
/* Draws at `scale` times the 8x8 cell. Folds case; see font.c. Returns the x the caller
 * would continue at, so a caller can chain runs of different colours. */
int obs_display_text(int x, int y, const char *text, obs_colour colour, int scale);
/* Puts what has been drawn on the screen. */
void obs_display_flip(void);

/* Whether a frame has demonstrably reached the screen: 1 yes, 0 no, -1 not established.
 *
 * Distinct from `obs_display_status`, which says whether the display *accepted* what it
 * was given. A platform can accept an output, a framebuffer and a flip, report success
 * to all three, and show nothing - so a report that only carries the acceptances
 * asserts more than it measured. (D187) */
int obs_display_presented(void);

/* Whether the probe currently holds the main video output.
 *
 * The video checks open that same output and hand it straight back, which on at least
 * one platform tears down the registration underneath the display. They ask this first
 * and skip rather than fight over it - a drawn report is worth more than a check that
 * duplicates what opening the display already proved. */
int obs_display_holds_output(void);

/* Width and height of the framebuffer, or zero when there is none. */
int obs_display_width(void);
int obs_display_height(void);

#endif /* OBSCENE_DISPLAY_H */
