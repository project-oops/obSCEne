/*
 * The report, drawn.
 *
 * The text stream is the contract; this is a second, independent report for people. A
 * run that dies partway through leaves the screen showing how far it got, which is the
 * one thing a black window cannot say.
 *
 * Every call is safe when there is no display. See src/display.c.
 */

#ifndef OBSCENE_SCREEN_H
#define OBSCENE_SCREEN_H

#include "obscene/harness.h"

/* Opens the display and draws the empty frame. Called once, before any check runs, so
 * something is on screen from the start rather than after the first section. */
void obs_screen_begin(unsigned int sections, unsigned int checks);

/* Records one finished check. `id` is not copied - the registry's strings outlive the
 * run - and the status is what the detail pages show. */
/* Takes the whole result rather than the status alone.
 *
 * The status is a colour and the `detail` beside it is the reason - and the reason was
 * being dropped here while the text report carried it, so the screen said `FAIL` where the
 * stream said `FAIL: the attribute object could not be initialised`. The screen exists for
 * the case where the stream cannot be read, which is exactly the case where losing the
 * reason costs the most. */
/* The check about to be attempted, drawn immediately so a hang names what it is inside. */
void obs_screen_attempt(const char *id);

void obs_screen_check(const char *id, obs_result result);

/* Records a finished section and redraws. `id` is not copied - the registry's strings
 * outlive the run. */
void obs_screen_section(const char *id, obs_tally tally);

/* Redraws from what has been recorded, with an optional line along the bottom. */
void obs_screen_redraw(const char *footer);

/* Cycles the summary and detail pages on screen, for as long as there is a display.
 *
 * Called once the run is over, so the whole report is on record before anything starts
 * paging. Returns immediately when there is no display, which is what keeps a headless
 * run finishing rather than hanging. */
void obs_screen_present(void);

/* Draw a HUD-only screen (title + platform facts) for a serving build that does not run
 * the suite. See src/start.c and src/screen.c. */
void obs_screen_hud(void);

#endif /* OBSCENE_SCREEN_H */
