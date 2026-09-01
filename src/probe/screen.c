/*
 * The report, drawn.
 *
 * Fed by the harness as it goes and redrawn after every section, so the screen shows
 * how far a run got even when it never finishes. That is the point: a run that dies
 * three sections in currently leaves a black window, and a black window cannot be told
 * from a hung one.
 *
 * # What it shows, and what it deliberately does not
 *
 * One row per section: the identifier, a proportion bar, and the four counts.
 *
 * **The row spacing is computed, not fixed.** It used to be a constant 46 pixels, chosen
 * when there were fifteen sections and correct for as long as that lasted. At twenty-one
 * the totals row lands at y=1184 on a 1080-high framebuffer, and because `obs_display_rect`
 * clips rather than faults, nothing broke - the totals and the footer were simply drawn
 * into nothing and the last section ran off the bottom.
 *
 * That is the worst way for this to fail. The screen exists for when the text stream
 * cannot be read, so a screen that silently omits the totals is showing a confident,
 * incomplete answer. The spacing now shrinks to fit whatever the registry holds.
 *
 * **And past a point the list wraps into two columns.** Shrinking the rows has a floor -
 * below OBS_ROW_H_MIN the identifiers stop being readable across a room, which is the screen's
 * only advantage over the stream - and at thirty-seven sections a single column reaches it and
 * the last rows land back on the totals. So beyond what one column holds, the sections run in
 * two instead: the rows keep their height, and the whole run still fits in one photograph.
 * (D271)
 *
 * **The summary shows no individual checks, and the detail pages do.** A hundred and thirty
 * identifiers at a readable size does not fit on one screen, and a screen that needs
 * squinting at has lost the only advantage it has over the text stream - so the summary
 * stays the shape of the run, and the presenter cycles paged lists behind it, eighteen
 * checks at a time.
 *
 * Each of those rows carries its reason beside the marker where the result had one. The
 * screen exists for the case where the stream cannot be read, and that is precisely the
 * case where `FAIL` without the reason costs the most - the stream would have said
 * `FAIL: the attribute object could not be initialised` and the screen said only the
 * colour. Nothing is invented for the rows that have none; most passes do not carry one,
 * and an empty column is the honest render of "nothing more is known".
 */

#include "obscene/screen.h"

#include "obscene/display.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "obscene/sysinfo.h"

/* `OBS_SCREEN_MAX` comes from `obscene/sections.h`, beside the list it has to be larger than,
 * and `registry.c` asserts the two agree. It was defined here and drifted. (D259) */

#define OBS_MARGIN 48
#define OBS_TOP 190

/* The comfortable row height, used whenever the sections fit at it. */
#define OBS_ROW_H_MAX 46
/* Below this the identifier and its counts stop being readable across a room, which is
 * the only reason this screen exists. If the sections will not fit even here, the answer
 * is fewer rows on screen rather than smaller ones - not a problem yet, and the
 * reservation below says what to do when it is. */
#define OBS_ROW_H_MIN 24
/* What the totals block and footer need beneath the last row. Reserved rather than
 * discovered: the previous layout discovered it by running off the bottom. */
#define OBS_FOOTER_RESERVE 150

/* Row height that fits `rows` sections above the footer, as tall as it can be.
 *
 * Returns the maximum when there is room, which is the usual case and keeps the screen
 * looking the way it was designed to. */
static int obs_row_height(unsigned int rows, int height) {
    if (rows == 0) {
        return OBS_ROW_H_MAX;
    }
    int available = height - OBS_TOP - OBS_FOOTER_RESERVE;
    if (available < 0) {
        available = 0;
    }
    int step = available / (int)rows;
    if (step > OBS_ROW_H_MAX) {
        step = OBS_ROW_H_MAX;
    }
    if (step < OBS_ROW_H_MIN) {
        step = OBS_ROW_H_MIN;
    }
    return step;
}

typedef struct row {
    const char *id;
    obs_tally tally;
} row;

/* Every check, for the detail pages. Sized past the current suite so growth does not
 * silently truncate the screen while the stream stays complete. */
#define OBS_CHECKS_MAX 256
/* Rows of detail per page. Chosen so a page fits above the footer at this text size,
 * with the identifiers still readable across a room - which is the only thing a screen
 * does better than the stream. */
#define OBS_PAGE_ROWS 18
/* Where the reason column starts, measured from the end of the marker. Wide enough for the
 * longest check identifier in the registry at this text size; a longer one pushes its own
 * reason right rather than being overdrawn. */
#define OBS_REASON_COLUMN (44 * OBS_FONT_WIDTH * 2)

/* One sleep the platform is known to survive.
 *
 * The page pacing asked for three seconds in a single call, and PS5PCEM stopped advancing:
 * the screen froze on whatever page was up, for as long as anyone watched. The evidence was
 * already in this project's own reports - `050-time/usleep` sleeps **2 ms** and passes there,
 * while `120-measure/sleep-fidelity`, which asks for longer, is excluded as *known to end the
 * process*. Three seconds is 1500 times the duration that is known to work.
 *
 * So a page waits by repeating the short sleep rather than asking for one long one. Same total
 * time on a platform where either works, and it keeps the display advancing on one where only
 * the small one does.
 *
 * This is presence-versus-behaviour inside our own display loop: the guard above tests that
 * `sceKernelUsleep` *exists*, which says nothing about whether it returns. (D174) */
#define OBS_SLEEP_STEP_MICROSECONDS 2000u

/* Long enough to read a page, short enough that waiting for a particular one is not a
 * chore. Three seconds. */
#define OBS_PAGE_MICROSECONDS 3000000u

/* How many page-lengths to hold the summary for. Thirty seconds: long enough to read,
 * long enough for an automated capture to land on it without timing anything, and short
 * enough that the detail pages still get seen. */
#define OBS_SUMMARY_HOLD 10u

typedef struct entry {
    const char *id;
    obs_status status;
    /* Static storage only, the same contract the harness holds for `obs_result.detail` -
     * nothing here copies it. */
    const char *detail;
} entry;

static entry obs_entries[OBS_CHECKS_MAX];
static unsigned int obs_entry_count = 0;

static row obs_rows[OBS_SCREEN_MAX];
static unsigned int obs_row_count = 0;
static unsigned int obs_total_sections = 0;
static unsigned int obs_total_checks = 0;
static int obs_live = 0;

void obs_screen_begin(unsigned int sections, unsigned int checks) {
    obs_total_sections = sections;
    obs_total_checks = checks;
    obs_row_count = 0;

    /* Announced before it is attempted, for the same reason every check is.
     *
     * Opening the display is the most involved platform interaction in the program -
     * video-out, then a direct-memory reserve, map and register - and it happens
     * *first*, ahead of the boot section that establishes the report can be trusted at
     * all. Until this record existed, a fault in there ended the run with `OBS|build` as
     * the last line: no try, no section, nothing naming the display.
     *
     * That is exactly the failure announce-before-attempting exists to prevent, in the
     * one place the harness's own `try` records do not reach. */
    obs_report_display("opening", "the display is being opened", 0);

    obs_display_state state = obs_display_open();
    obs_live = (state == OBS_DISPLAY_READY);

    /* And the outcome. A photograph of a screen cannot distinguish "these are the
     * results" from "the display never came up"; this record can. An "opening" with no
     * outcome after it names the display as what killed the run. */
    static const char *const names[] = {"untried", "ready", "absent", "failed"};
    obs_report_display(names[(int)state], obs_display_status_text(), obs_display_status_code());

    if (obs_live) {
        obs_screen_redraw(0);

        /* And whether that redraw was actually seen.
         *
         * `ready` above means the platform accepted an output, a framebuffer and a flip. It
         * does not mean a frame reached a screen, and on a partly implemented display library
         * those come apart completely: every call succeeds and the window stays black. The
         * report used to carry only the acceptances, so it asserted a working display on a
         * platform that cannot present - the one claim in it with no measurement behind it.
         *
         * Emitted after the first redraw because that is the first moment there is anything
         * to have presented. (D187) */
        static const char *const seen[] = {"blind", "presenting", "unestablished"};
        int p = obs_display_presented();
        obs_report_display(seen[p == 1 ? 1 : (p == 0 ? 0 : 2)],
                           p == 1    ? "a submitted frame reached the display"
                           : p == 0  ? "flips are accepted and the frame count never moves"
                                     : "the platform will not say whether a frame was shown",
                           0);
    }
}

/* The check currently being attempted, or null between checks. */
static const char *obs_in_flight;

void obs_screen_attempt(const char *id) {
    obs_in_flight = id;
    /* Recorded, **not** redrawn.
     *
     * The first version redrew here, reasoning that an in-flight name is only useful if it is
     * on screen during the call. That is true and the cost is not affordable:
     * `obs_screen_redraw` ends in `obs_display_flip`, so it added a full present per check -
     * 515 of them - and shadPS4 began dying in `120-measure`, three checks it had never
     * crashed on before.
     *
     * A flip per check is also the wrong shape for what this is for. The screen is redrawn at
     * every section boundary anyway, so the name is on screen for the section that contains
     * the hang; narrowing it to the exact check is not worth changing how often the guest
     * presents, on a probe whose whole purpose is to not perturb what it measures. (D174) */
}

void obs_screen_check(const char *id, obs_result result) {
    obs_in_flight = NULL;
    if (obs_entry_count < OBS_CHECKS_MAX) {
        obs_entries[obs_entry_count].id = id;
        obs_entries[obs_entry_count].status = result.status;
        obs_entries[obs_entry_count].detail = result.detail;
        obs_entry_count++;
    }
}

void obs_screen_section(const char *id, obs_tally tally) {
    if (obs_row_count < OBS_SCREEN_MAX) {
        obs_rows[obs_row_count].id = id;
        obs_rows[obs_row_count].tally = tally;
        obs_row_count++;
    }
    if (obs_live) {
        obs_screen_redraw(0);
    }
}

/* The wordmark, in two colours.
 *
 * The three letters in the middle belong to the platform rather than to this project, and
 * the name has always been built around that. Drawing them apart says so at a glance, and
 * says it the way principle 5 prefers - marked, not spelled out in prose.
 *
 * Three runs rather than one string, which is what `obs_display_text` returning the next x
 * is for. It also retires the `8 * 6 * 7` that two call sites carried to work out where the
 * wordmark ended: the width is now whatever was actually drawn, so changing the scale or
 * the name cannot leave the HUD overlapping it.
 */
static int obs_draw_wordmark(int x, int y, int scale) {
    int at = obs_display_text(x, y, "OB", OBS_COLOUR_INK, scale);
    at = obs_display_text(at, y, "SCE", OBS_COLOUR_MARK, scale);
    return obs_display_text(at, y, "NE", OBS_COLOUR_INK, scale);
}

static void obs_number(int right_x, int y, unsigned int value, obs_colour colour,
                       int scale);

/* Writes an unsigned value right-aligned in a field, and returns where it started.
 * Right alignment because these are columns of counts and a ragged left edge makes
 * them harder to compare than they need to be. */
static void obs_number(int right_x, int y, unsigned int value, obs_colour colour,
                       int scale) {
    char buf[12];
    int n = 0;
    if (value == 0) {
        buf[n++] = '0';
    }
    while (value > 0 && n < (int)sizeof buf - 1) {
        buf[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    /* Built backwards, so reverse it in place. */
    for (int i = 0; i < n / 2; i++) {
        char t = buf[i];
        buf[i] = buf[n - 1 - i];
        buf[n - 1 - i] = t;
    }
    buf[n] = '\0';
    (void)obs_display_text(right_x - n * OBS_FONT_WIDTH * scale, y, buf, colour, scale);
}

/* The platform HUD: every system fact obSCEne can name, each showing its value or the
 * honest `unknown`. The colour is the finding - ink for a known value, accent for "the
 * platform has this query but we have not wired its signature", dim for "not here at all".
 * That an emulator shows a wall of dim is the point, not a defect (see obscene/sysinfo.h).
 *
 * Two rows of four, starting at (x0, y0), fitted to the width to the right of the title. */
static void obs_draw_hud(int x0, int y0, int width) {
    int col_w = (width - x0 - OBS_MARGIN) / 4;
    if (col_w < 180) {
        col_w = 180;
    }
    for (unsigned int i = 0; i < (unsigned int)OBS_SYS_COUNT; i++) {
        char value[32];
        obs_sys_state state = obs_sysinfo_value((obs_sys_field)i, value, sizeof(value));
        obs_colour colour = state == OBS_SYS_KNOWN        ? OBS_COLOUR_INK
                            : state == OBS_SYS_UNCONFIRMED ? OBS_COLOUR_ACCENT
                                                           : OBS_COLOUR_DIM;
        int x = x0 + (int)(i % 4u) * col_w;
        int y = y0 + (int)(i / 4u) * 24;
        int vx = obs_display_text(x, y, obs_sysinfo_label((obs_sys_field)i), OBS_COLOUR_DIM,
                                  2);
        (void)obs_display_text(vx + 12, y, value, colour, 2);
    }
}

/* A HUD-only screen, for a serving build that never runs the suite (src/start.c).
 *
 * Opens the display if the suite did not, draws the title and the platform facts, and
 * flips once. This is what puts the listening port - and, once its signature is confirmed,
 * the machine's own IP - in front of the person who has to point a driver at it. A single
 * draw, not a redraw loop: nothing changes while it waits on `accept`. */
void obs_screen_hud(void) {
    if (!obs_live) {
        /* Announced, and its outcome recorded, exactly as obs_screen_begin does - so a
         * black window in a serving build can be told apart from a display that never
         * opened. Without this the serve path drew (or failed to) in silence. */
        obs_report_display("opening", "the display is being opened", 0);
        obs_display_state state = obs_display_open();
        obs_live = (state == OBS_DISPLAY_READY);
        static const char *const names[] = {"untried", "ready", "absent", "failed"};
        obs_report_display(names[(int)state], obs_display_status_text(), obs_display_status_code());
    }
    if (!obs_live) {
        return;
    }
    /* Drawn a few times, not once. The suite screen converges because it redraws after
     * every section; a serving build draws the HUD and then blocks on `accept`, so a
     * single flip that the presenter has not yet scanned out leaves a black window. A
     * handful of identical frames is cheap and makes the HUD actually appear. */
    for (int frame = 0; frame < 3; frame++) {
        obs_display_clear(OBS_COLOUR_GROUND);
        obs_draw_hud(obs_draw_wordmark(OBS_MARGIN, 56, 6) + 24, 50, obs_display_width());
        obs_display_flip();
    }
}

/* A proportion bar. Widths are integer-divided from the total, and the remainder goes
 * to the largest slice, so the bar always spans exactly `w` rather than ending a pixel
 * or two short in a way that looks like a rendering bug. */
static void obs_bar(int x, int y, int w, int h, obs_tally t) {
    unsigned int total = t.pass + t.partial + t.fail + t.skip;
    if (total == 0) {
        obs_display_rect(x, y, w, h, OBS_COLOUR_SKIP);
        return;
    }
    unsigned int counts[4] = {t.pass, t.partial, t.fail, t.skip};
    obs_colour colours[4] = {OBS_COLOUR_PASS, OBS_COLOUR_PARTIAL, OBS_COLOUR_FAIL, OBS_COLOUR_SKIP};
    int widths[4];
    int used = 0;
    int biggest = 0;
    for (int i = 0; i < 4; i++) {
        widths[i] = (int)((unsigned long)counts[i] * (unsigned long)w / total);
        used += widths[i];
        if (counts[i] > counts[biggest]) {
            biggest = i;
        }
    }
    widths[biggest] += w - used;
    int at = x;
    for (int i = 0; i < 4; i++) {
        if (widths[i] > 0) {
            obs_display_rect(at, y, widths[i], h, colours[i]);
            at += widths[i];
        }
    }
}

/* One section's row - identifier, proportion bar, four counts - laid within the column
 * [x0, x0 + col_w). Split out so the summary can run the sections in two columns: at
 * thirty-seven sections a single column hits the row-height floor and the last rows overrun
 * the totals (D271). The geometry is the single-column layout's, parameterised by the column,
 * so one column reproduces the original screen pixel for pixel and two columns only narrow the
 * bar - the counts, which carry the numbers, keep their size and spacing. */
static void obs_draw_summary_row(int x0, int col_w, int y, const row *r) {
    int bar_x = x0 + 380;
    int bar_w = col_w - 380 - 340;
    if (bar_w < 40) {
        bar_w = 40;
    }
    (void)obs_display_text(x0, y + 6, r->id, OBS_COLOUR_INK, 2);
    obs_bar(bar_x, y + 4, bar_w, 20, r->tally);
    int right = x0 + col_w;
    obs_number(right, y + 6, r->tally.skip, OBS_COLOUR_SKIP, 2);
    obs_number(right - 80, y + 6, r->tally.fail, OBS_COLOUR_FAIL, 2);
    obs_number(right - 160, y + 6, r->tally.partial, OBS_COLOUR_PARTIAL, 2);
    obs_number(right - 240, y + 6, r->tally.pass, OBS_COLOUR_PASS, 2);
}

void obs_screen_redraw(const char *footer) {
    if (!obs_live) {
        return;
    }
    int w = obs_display_width();
    int h = obs_display_height();
    if (w == 0 || h == 0) {
        return;
    }

    obs_display_clear(OBS_COLOUR_GROUND);

    /* Title, and the platform HUD where the tagline used to be. */
    obs_draw_hud(obs_draw_wordmark(OBS_MARGIN, 56, 6) + 24, 50, w);

    obs_tally total = {0, 0, 0, 0};
    for (unsigned int i = 0; i < obs_row_count; i++) {
        total.pass += obs_rows[i].tally.pass;
        total.partial += obs_rows[i].tally.partial;
        total.fail += obs_rows[i].tally.fail;
        total.skip += obs_rows[i].tally.skip;
    }
    unsigned int done = total.pass + total.partial + total.fail + total.skip;

    /* Progress across the whole suite, above everything, so "is it alive" is the first
     * thing readable. */
    obs_display_rect(OBS_MARGIN, 130, w - OBS_MARGIN * 2, 3, 0xFF262C36u);
    if (obs_total_checks > 0) {
        int done_w = (int)((unsigned long)done * (unsigned long)(w - OBS_MARGIN * 2) /
                           obs_total_checks);
        obs_display_rect(OBS_MARGIN, 130, done_w, 3, OBS_COLOUR_ACCENT);
    }
    int x = obs_display_text(OBS_MARGIN, 148, "SECTION ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, 148, obs_row_count, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 4, 148, "OF ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, 148, obs_total_sections, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 4, 148, "   CHECKS ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, 148, done, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 4, 148, "OF ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, 148, obs_total_checks, OBS_COLOUR_INK, 2);

    /* What is happening right now, in words.
     *
     * A run in progress names the check it is inside; a finished one says so. Between them
     * is the case this exists for: a screen that has stopped changing, where the two are
     * indistinguishable without it. */
    if (obs_in_flight != NULL) {
        int rx = obs_display_text(OBS_MARGIN, 176, "RUNNING ", OBS_COLOUR_ACCENT, 2);
        (void)obs_display_text(rx + 8 * 2, 176, obs_in_flight, OBS_COLOUR_INK, 2);
    } else if (done >= obs_total_checks && obs_total_checks > 0) {
        (void)obs_display_text(OBS_MARGIN, 176, "SUITE COMPLETE", OBS_COLOUR_PASS, 2);
    }

    /* The sections, in as many columns as it takes to keep them above the totals.
     *
     * One column while they fit in it at a readable height; two once a single column would
     * overrun the footer, which is where thirty-seven sections put it - that many rows want
     * more height than there is even at the OBS_ROW_H_MIN floor, so the last of them used to be
     * drawn over the totals. Splitting the list holds the count without shrinking the rows, and
     * keeps the property the screen exists for: one photograph is the whole answer, which a
     * paged summary would lose. Two columns hold about sixty sections; a third is the next step
     * and not needed yet. The left column fills first, top to bottom, so the order still reads
     * down and then across. (D271) */
    int available = h - OBS_TOP - OBS_FOOTER_RESERVE;
    if (available < 0) {
        available = 0;
    }
    unsigned int columns = 1;
    if (obs_row_count * (unsigned int)OBS_ROW_H_MIN > (unsigned int)available) {
        columns = 2;
    }
    unsigned int per_col = (obs_row_count + columns - 1u) / columns;
    int row_h = obs_row_height(per_col, h);
    int gap = OBS_MARGIN;
    int col_w = (columns == 1) ? (w - OBS_MARGIN * 2) : (w - OBS_MARGIN * 2 - gap) / 2;
    for (unsigned int i = 0; i < obs_row_count; i++) {
        unsigned int col = per_col == 0 ? 0u : i / per_col;
        unsigned int within = i - col * per_col;
        int x0 = OBS_MARGIN + (int)col * (col_w + gap);
        int y = OBS_TOP + (int)within * row_h;
        obs_draw_summary_row(x0, col_w, y, &obs_rows[i]);
    }

    /* Totals, and whatever the caller wants to say. Below the taller (left) column. */
    int y = OBS_TOP + (int)per_col * row_h + 28;
    obs_display_rect(OBS_MARGIN, y, w - OBS_MARGIN * 2, 2, 0xFF262C36u);
    y += 22;
    x = OBS_MARGIN;
    obs_number(x + 8 * 3 * 4, y, total.pass, OBS_COLOUR_PASS, 3);
    x = obs_display_text(x + 8 * 3 * 4 + 12, y + 6, "PASS  ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 3 * 4, y, total.partial, OBS_COLOUR_PARTIAL, 3);
    x = obs_display_text(x + 8 * 3 * 4 + 12, y + 6, "PARTIAL  ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 3 * 4, y, total.fail, OBS_COLOUR_FAIL, 3);
    x = obs_display_text(x + 8 * 3 * 4 + 12, y + 6, "FAIL  ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 3 * 4, y, total.skip, OBS_COLOUR_SKIP, 3);
    (void)obs_display_text(x + 8 * 3 * 4 + 12, y + 6, "SKIP", OBS_COLOUR_DIM, 2);

    if (footer != 0) {
        (void)obs_display_text(OBS_MARGIN, h - 56, footer, OBS_COLOUR_DIM, 2);
    }

    obs_display_flip();
}

/* ---- detail pages -----------------------------------------------------------
 *
 * The summary shows the shape of a run; these show the checks themselves. Both exist
 * because they answer different questions, and neither replaces the other: the summary
 * says "is anything working", a page says "which one".
 *
 * Cycled *and* driven by input, with the cycle as the floor.
 *
 * The auto-cycle stays because a controller is one more thing that has to work on a platform
 * being tested precisely because things do not, and a run being photographed has nobody to
 * press a button - so the pages must advance on their own or a headless capture gets one
 * frame forever. But waiting out a thirty-second cycle to reach page nine is its own problem,
 * so the D-pad is layered on top: left and right page immediately, and any press resets the
 * auto-advance timer. Nothing depends on the pad; it only makes an attended run faster. The
 * pad is read through the same weak-symbol guard as everything else, so a platform that does
 * not resolve it simply keeps cycling. (D259)
 */

static obs_colour colour_of(obs_status status) {
    switch (status) {
    case OBS_PASS:
        return OBS_COLOUR_PASS;
    case OBS_PARTIAL:
        return OBS_COLOUR_PARTIAL;
    case OBS_FAIL:
        return OBS_COLOUR_FAIL;
    case OBS_SKIP:
    default:
        return OBS_COLOUR_SKIP;
    }
}

static const char *marker_of(obs_status status) {
    switch (status) {
    case OBS_PASS:
        return "OK";
    case OBS_PARTIAL:
        return "WARN";
    case OBS_FAIL:
        return "FAIL";
    case OBS_SKIP:
    default:
        return "--";
    }
}

static unsigned int page_count(void) {
    if (obs_entry_count == 0) {
        return 1;
    }
    return 1u + (obs_entry_count + OBS_PAGE_ROWS - 1u) / OBS_PAGE_ROWS;
}

static void draw_page(unsigned int page) {
    int w = obs_display_width();
    int h = obs_display_height();
    if (w == 0 || h == 0) {
        return;
    }

    obs_display_clear(OBS_COLOUR_GROUND);
    (void)obs_draw_wordmark(OBS_MARGIN, 56, 6);

    unsigned int first = (page - 1u) * OBS_PAGE_ROWS;
    unsigned int last = first + OBS_PAGE_ROWS;
    if (last > obs_entry_count) {
        last = obs_entry_count;
    }

    int x = obs_display_text(OBS_MARGIN + 8 * 6 * 7 + 24, 74, "CHECKS ", OBS_COLOUR_ACCENT, 2);
    obs_number(x + 8 * 2 * 4, 74, first + 1u, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 5, 74, "TO ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 4, 74, last, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 5, 74, "OF ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 4, 74, obs_entry_count, OBS_COLOUR_INK, 2);

    obs_display_rect(OBS_MARGIN, 118, w - OBS_MARGIN * 2, 2, 0xFF262C36u);

    for (unsigned int i = first; i < last; i++) {
        int y = 148 + (int)(i - first) * 44;
        obs_colour colour = colour_of(obs_entries[i].status);
        /* The verdict in its own column, so a page can be read down the left edge
         * without reading any identifier at all. */
        (void)obs_display_text(OBS_MARGIN, y, marker_of(obs_entries[i].status), colour, 2);
        int after = obs_display_text(OBS_MARGIN + 110, y, obs_entries[i].id, OBS_COLOUR_INK,
                                     2);
        /* The reason, where there is one.
         *
         * A fixed column so the reasons line up and can be read down, but never closer than
         * a space after the identifier - the longest ids would otherwise be drawn straight
         * through the text, and `obs_display_rect` clips rather than faulting, so it would
         * have looked like a font bug rather than a layout one.
         *
         * Drawn dim: the identifier and the marker are what the eye should catch first, and
         * a wall of equally bright prose beside them undoes that. Nothing is invented for
         * the rows that have no detail - most passes do not carry one, and the honest render
         * of "nothing more is known" is an empty column. */
        int reason_x = OBS_MARGIN + 110 + OBS_REASON_COLUMN;
        if (after + OBS_FONT_WIDTH * 2 > reason_x) {
            reason_x = after + OBS_FONT_WIDTH * 2;
        }
        if (obs_entries[i].detail != 0) {
            (void)obs_display_text(reason_x, y, obs_entries[i].detail, OBS_COLOUR_DIM, 2);
        }
    }

    /* Which page, and how many, so a photograph says whether it is the whole story. */
    x = obs_display_text(OBS_MARGIN, h - 56, "PAGE ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, h - 56, page, OBS_COLOUR_INK, 2);
    x = obs_display_text(x + 8 * 2 * 4, h - 56, "OF ", OBS_COLOUR_DIM, 2);
    obs_number(x + 8 * 2 * 3, h - 56, page_count() - 1u, OBS_COLOUR_INK, 2);

    /* The run state, on **every** page.
     *
     * It used to appear only on page zero, beside the summary. A detail page therefore said
     * nothing about whether the suite had finished, so someone looking at page seven of
     * fifteen could not tell a completed run cycling its results from a run stopped dead in
     * the middle - and the honest reading, when a page sits there unchanged, is the second.
     *
     * These pages are only ever drawn after the suite ends, so the word is a constant here.
     * That is exactly why it is worth printing: the reader does not know what the code
     * knows. (D174) */
    x = obs_display_text(x + 8 * 2 * 8, h - 56, "SUITE COMPLETE - ", OBS_COLOUR_PASS, 2);
    (void)obs_display_text(x, h - 56, "DPAD LEFT/RIGHT TO PAGE", OBS_COLOUR_ACCENT, 2);

    obs_display_flip();
}

/* Wait roughly `microseconds`, in steps the platform is known to survive.
 *
 * Nothing here can detect a sleep that never returns - a call that blocks blocks - but a
 * platform that handles 2 ms and not 3 s is served by this and was not served by the single
 * long call it replaces. */
static void obs_screen_wait(unsigned int microseconds) {
    unsigned int steps = microseconds / OBS_SLEEP_STEP_MICROSECONDS;
    for (unsigned int i = 0; i < steps; i++) {
        (void)sceKernelUsleep(OBS_SLEEP_STEP_MICROSECONDS);
    }
}

/* D-pad and shoulder-button masks, and the button word's offset in the pad state.
 *
 * From the OpenOrbis SDK's `OrbisPadButtonDataOffset` and `OrbisPadData` - an open-source
 * toolchain, which is a permitted provenance source (D008 allows a declaration whose origin
 * can be named). Only the button word at offset 0 is read; the rest of the structure is left
 * untouched, which is why `scePadReadState` takes a `void *` and this reads four bytes. */
#define OBS_PAD_STATE_BYTES 1024u /* >> sizeof(OrbisPadData) (0x230); over-sized on purpose */
#define OBS_PAD_UP 0x00000010u
#define OBS_PAD_RIGHT 0x00000020u
#define OBS_PAD_DOWN 0x00000040u
#define OBS_PAD_LEFT 0x00000080u
#define OBS_PAD_L1 0x00000400u
#define OBS_PAD_R1 0x00000800u
#define OBS_PAD_CIRCLE 0x00002000u

/* ~90 ms a poll: responsive to a press without spinning the CPU on a screen that is idle. */
#define OBS_PAD_POLL_MICROSECONDS 90000u
/* Polls of a held direction before it repeats, and how often it repeats after that. Holding
 * right then scrolls forward rather than moving one page per press. */
#define OBS_PAD_REPEAT_DELAY 4u
#define OBS_PAD_REPEAT_EVERY 2u
/* Polls with no input before the auto-cycle advances. The summary is held far longer than a
 * detail page because it is the one screen that answers "what happened" on its own - and an
 * automated capture polling for the run to finish lands several pages late, so a short hold
 * would be gone before it caught it. */
#define OBS_PAD_SUMMARY_IDLE 333u /* ~30 s */
#define OBS_PAD_DETAIL_IDLE 44u   /* ~4 s */

/* The keyboard's keycode array lives at this byte offset in the state buffer, and each entry
 * is a USB HID usage id. The offset is the one thing depended on, and the OpenOrbis keyboard
 * sample validates it by reading `keycodes` there in working code; the fields before it that
 * the header marks uncertain are never read. Arrow ids are the standard HID usages. */
#define OBS_KB_STATE_BYTES 256u /* >> sizeof(OrbisKeyboardData) (96); over-sized on purpose */
#define OBS_KB_KEYCODES_OFFSET 32u
#define OBS_KB_KEYCODE_COUNT 32u
#define OBS_KB_KEY_RIGHT 79u
#define OBS_KB_KEY_LEFT 80u
#define OBS_KB_KEY_DOWN 81u
#define OBS_KB_KEY_UP 82u
#define OBS_KB_KEY_RETURN 40u

static int obs_pad_handle = -1;
static int obs_kb_handle = -1;

/* Open the controller for the initial user, or leave the handle closed and page on a timer.
 *
 * Every call is guarded: a platform that resolves none of these keeps the auto-cycle and
 * loses nothing. The user service is asked, not initialised - `070-user` and the display
 * path have already done that by the time this runs, and re-initialising a working service is
 * how the display path once hung (D174's neighbour). */
static void obs_pad_open(void) {
    if (!obs_address_is_callable((const void *)&scePadInit)
        || !obs_address_is_callable((const void *)&scePadOpen)
        || !obs_address_is_callable((const void *)&scePadReadState)
        || !obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        return;
    }
    (void)scePadInit();
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return;
    }
    int handle = scePadOpen(user, 0, 0, 0);
    if (handle > 0) {
        obs_pad_handle = handle;
    }
}

/* The button word, or zero if there is no pad or the read failed.
 *
 * The buffer is zeroed and over-sized every call: `scePadReadState` writes the whole pad
 * structure and this reads only its first word, so a buffer larger than any real structure
 * cannot be overrun and a failed read leaves the zeros in place - which reads as "no buttons",
 * the safe answer. Assembled a byte at a time because the buffer has no alignment guarantee. */
static uint32_t obs_pad_buttons(void) {
    if (obs_pad_handle < 0) {
        return 0;
    }
    unsigned char state[OBS_PAD_STATE_BYTES];
    for (unsigned int i = 0; i < OBS_PAD_STATE_BYTES; i++) {
        state[i] = 0;
    }
    if (scePadReadState(obs_pad_handle, state) != 0) {
        return 0;
    }
    uint32_t buttons = 0;
    for (unsigned int i = 0; i < 4u; i++) {
        buttons |= (uint32_t)state[i] << (i * 8u);
    }
    return buttons;
}

/* Open the keyboard for the initial user, or leave it closed and rely on the pad.
 *
 * Guarded exactly like the pad: a platform without these keeps working, and neither input
 * device is required. Both are opened because a KVM offers a keyboard and not a controller,
 * so this is the path that gets driven without a DualSense to hand. */
static void obs_kb_open(void) {
    if (!obs_address_is_callable((const void *)&sceKeyboardInit)
        || !obs_address_is_callable((const void *)&sceKeyboardOpen)
        || !obs_address_is_callable((const void *)&sceKeyboardReadState)
        || !obs_address_is_callable((const void *)&sceUserServiceGetInitialUser)) {
        return;
    }
    (void)sceKeyboardInit();
    int32_t user = -1;
    if (sceUserServiceGetInitialUser(&user) != 0 || user < 0) {
        return;
    }
    int handle = sceKeyboardOpen(user, 0, 0, 0);
    if (handle >= 0) {
        obs_kb_handle = handle;
    }
}

/* Arrow keys held on the keyboard, returned as the same nav bits the pad uses so the loop
 * below needs to know nothing about which device produced them.
 *
 * Every keycode slot is scanned rather than trusting a reported count: an unused slot is zero
 * and zero is no key, so scanning all of them cannot invent a press, and it avoids depending
 * on the `nkeys` field's offset - only the keycode array's offset, which the sample validates. */
static uint32_t obs_kb_nav(void) {
    if (obs_kb_handle < 0) {
        return 0;
    }
    unsigned char state[OBS_KB_STATE_BYTES];
    for (unsigned int i = 0; i < OBS_KB_STATE_BYTES; i++) {
        state[i] = 0;
    }
    if (sceKeyboardReadState(obs_kb_handle, state) != 0) {
        return 0;
    }
    uint32_t bits = 0;
    for (unsigned int k = 0; k < OBS_KB_KEYCODE_COUNT; k++) {
        unsigned int at = OBS_KB_KEYCODES_OFFSET + k * 2u;
        uint32_t code = (uint32_t)state[at] | ((uint32_t)state[at + 1u] << 8u);
        switch (code) {
        case OBS_KB_KEY_RIGHT:
            bits |= OBS_PAD_RIGHT;
            break;
        case OBS_KB_KEY_LEFT:
            bits |= OBS_PAD_LEFT;
            break;
        case OBS_KB_KEY_DOWN:
            bits |= OBS_PAD_DOWN;
            break;
        case OBS_KB_KEY_UP:
            bits |= OBS_PAD_UP;
            break;
        case OBS_KB_KEY_RETURN:
            bits |= OBS_PAD_CIRCLE;
            break;
        default:
            break;
        }
    }
    return bits;
}

/* Whatever input is present, from either device, as one nav bitfield. */
static uint32_t obs_nav_input(void) {
    return obs_pad_buttons() | obs_kb_nav();
}

void obs_screen_present(void) {
    if (!obs_live) {
        /* No display, so nothing to present and nothing to wait for. Returning lets a
         * headless run finish, which is what CI needs. */
        return;
    }
    if (&sceKernelUsleep == 0) {
        /* Without a way to pace them, cycling would flicker through every page faster
         * than anyone could read. The summary is already on screen from the run, and
         * leaving it there is better than replacing it with a blur. */
        return;
    }

    obs_pad_open();
    obs_kb_open();

    unsigned int pages = page_count();
    unsigned int page = 0;
    uint32_t previous = 0;
    unsigned int idle = 0;
    unsigned int held = 0;
    /* Latched by the first press and never cleared. The screen auto-cycles until somebody
     * touches the controller; from then on it is theirs, and it holds whatever page they left
     * it on rather than drifting off it a few seconds later. Resuming the cycle would mean
     * fighting a viewer who has just stopped pressing to read something. A relaunch is how you
     * get the cycle back, which is a clear and reversible action. (D259) */
    int driven = 0;

    obs_screen_redraw("REPORT COMPLETE");

    for (;;) {
        obs_screen_wait(OBS_PAD_POLL_MICROSECONDS);

        uint32_t now = obs_nav_input();
        uint32_t forward = now & (OBS_PAD_RIGHT | OBS_PAD_R1);
        uint32_t backward = now & (OBS_PAD_LEFT | OBS_PAD_L1);
        uint32_t pressed = now & ~previous;
        previous = now;

        /* Any button ends the auto-cycle. "Intervenes" is read broadly on purpose: someone
         * holding the controller and pressing anything is present, and a screen that kept
         * drifting under them would be the exact annoyance this removes. */
        if (pressed != 0) {
            driven = 1;
        }

        int delta = 0;
        int changed = 0;

        if (pressed & (OBS_PAD_RIGHT | OBS_PAD_R1)) {
            delta = 1;
        } else if (pressed & (OBS_PAD_LEFT | OBS_PAD_L1)) {
            delta = -1;
        } else if (pressed & OBS_PAD_CIRCLE) {
            /* Straight back to the summary, which is the one page worth a shortcut. */
            page = 0;
            changed = 1;
            held = 0;
        } else if (forward != 0 || backward != 0) {
            /* A direction is being held: repeat it after a short delay so scrolling works. */
            held++;
            if (held > OBS_PAD_REPEAT_DELAY && (held % OBS_PAD_REPEAT_EVERY) == 0u) {
                delta = (forward != 0) ? 1 : -1;
            }
        } else {
            held = 0;
            /* Hands-off only. Once `driven`, the cycle is over and the page holds until the
             * next press. */
            if (!driven) {
                idle++;
                unsigned int threshold =
                    (page == 0) ? OBS_PAD_SUMMARY_IDLE : OBS_PAD_DETAIL_IDLE;
                if (idle >= threshold) {
                    page = (page + 1u) % pages;
                    changed = 1;
                    idle = 0;
                }
            }
        }

        if (delta != 0) {
            /* `+ pages` keeps the operand positive before the modulo, so left from page zero
             * wraps to the last page rather than underflowing. */
            page = (page + (unsigned int)((int)pages + delta)) % pages;
            changed = 1;
        }

        if (changed) {
            if (page == 0) {
                obs_screen_redraw("REPORT COMPLETE");
            } else {
                draw_page(page);
            }
        }
    }
}
