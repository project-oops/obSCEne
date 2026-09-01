/*
 * Platform facts for the on-screen HUD.
 *
 * # Why "unknown" is a first-class answer here
 *
 * obSCEne exists to flex every interface a platform offers and make the gaps visible. A HUD
 * field that reads `unknown` because the platform does not implement the query is not a
 * failure of the HUD - it is exactly the finding the whole program is built to surface, the
 * census philosophy applied to a status line. Fabricating a number would be the sin; showing
 * `unknown` is the honest opposite, and it is a standing reminder - to an emulator author
 * that a query is missing, and to us that a signature is not yet wired.
 *
 * # The one rule that survives "fail open"
 *
 * A field shows a value only when it was read through a **confirmed** signature. Fail-open
 * covers the *result* (the call errored, or the symbol is absent -> `unknown`) and it covers
 * *absence*. It does not cover calling blind: a guessed arity on a query that fills a struct
 * through a pointer crashes the HUD rather than failing open (D008). So a field is one of
 * three honest states:
 *
 *   absent               the symbol does not resolve here - an emulator gap
 *   present, unconfirmed the symbol resolves but we have no confirmed signature yet - our TODO
 *   known                read through a confirmed signature - the value, or `unknown` on error
 *
 * The first two both render `unknown`; `obs_sysinfo_state` distinguishes them so the HUD can
 * mark "the platform has this, go wire it" differently from "the platform lacks it".
 *
 * # Screen only, never the graded corpus
 *
 * These are a live readout for the human in front of the machine. A probe cannot certify its
 * own machine (D108) - inside an emulator every one of these answers as the emulator - so
 * they must never flow into the corpus as `measured` machine provenance. They may be emitted
 * as `measure` records, which are observations recorded rather than judged, and diffable
 * across platforms - which is the right vehicle for "what did this platform say about
 * itself", gap and all.
 */

#ifndef OBSCENE_SYSINFO_H
#define OBSCENE_SYSINFO_H

#include <stddef.h>

/* The build generation is needed here to decide whether the PS4-compat firmware field exists.
 * It is always defined by the Makefile; guarded so a stray include without it does not silently
 * change `OBS_SYS_COUNT` and desync the enum between translation units. */
#ifndef OBSCENE_GEN
#define OBSCENE_GEN 0
#endif

typedef enum obs_sys_field {
    OBS_SYS_LISTENING, /* the command socket's bind, when serving */
    OBS_SYS_SECRET,    /* the session secret a driver must present, when serving */
    OBS_SYS_IP,        /* the machine's own address (sceNetCtlGetInfo) */
    OBS_SYS_FIRMWARE,  /* system software version (the console's own, from kern.version) */
    OBS_SYS_GENERATION,/* which console generation resolved, or the mode we run in */
    OBS_SYS_GPU,       /* the graphics driver that resolves (gnm/agc), separate from the mode */
    OBS_SYS_MEMORY,    /* flexible memory available */
    OBS_SYS_VRAM,      /* graphics / shared memory */
    OBS_SYS_TEMP,      /* SoC temperature */
    OBS_SYS_STORAGE,   /* free storage */
#if OBSCENE_GEN == 4
    /* The PS4-compatibility environment's own version, which `sceKernelGetSystemSwVersion`
     * reports (13.090.001 on the 12.40 console it was measured on). It exists only in a gen4
     * build, because only a `ps4_game` runs in that environment - a gen5 build has no such
     * layer and so no such field, which is how "do not show it outside ps4_mode" is honoured. */
    OBS_SYS_PS4_FIRMWARE,
#endif
    OBS_SYS_COUNT
} obs_sys_field;

typedef enum obs_sys_state {
    OBS_SYS_ABSENT,      /* the query does not resolve on this platform */
    OBS_SYS_UNCONFIRMED, /* it resolves, but no confirmed signature to call it yet */
    OBS_SYS_KNOWN        /* read through a confirmed signature */
} obs_sys_state;

/* The short label shown before the value, e.g. "MEM". */
const char *obs_sysinfo_label(obs_sys_field field);

/* Fills `buf` with the value, or "unknown", and returns the field's state. `buf` must hold
 * at least 32 bytes. Never calls a query whose signature is not confirmed. */
obs_sys_state obs_sysinfo_value(obs_sys_field field, char *buf, size_t n);

/* The command socket's bind, so the HUD can show where a driver connects. Set by the serve
 * path before the HUD is drawn; zero means "not serving", and OBS_SYS_LISTENING then reads
 * absent. */
void obs_sysinfo_set_listening(unsigned int port);

/* The session secret to display. A console has no other channel for it: the HUD draws the
 * port so a driver can be pointed at it, and the secret is the other half of that. */
void obs_sysinfo_set_secret(const char *secret);

/* Emits every field as a `sysinfo` record - field key, state, value - so the report carries
 * the same readout the HUD draws. An observation stream, not verdicts (see obs_report_sysinfo
 * and D108). Called with the report header, and by the serve path once it is listening. */
void obs_sysinfo_report(void);

#endif /* OBSCENE_SYSINFO_H */
