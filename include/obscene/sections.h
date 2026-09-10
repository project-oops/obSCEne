/*
 * The section declarations.
 *
 * Each section is defined in its layer's file and listed, in order, in registry.c.
 * The list is explicit rather than assembled by a linker section or a constructor
 * trick: the running order is the single most important thing about this program,
 * and it should be readable in one file without knowing how the linker feels.
 */

#ifndef OBSCENE_SECTIONS_H
#define OBSCENE_SECTIONS_H

#include "obscene/harness.h"

/* Base layer: the report itself, then the process it runs in. */
extern const obs_section obs_section_boot;
extern const obs_section obs_section_kernel;
/* Which console this is. Runs early: it decides how every later absence reads. */
extern const obs_section obs_section_generation;

/* Resources the rest of the platform is built on. */
extern const obs_section obs_section_memory;
extern const obs_section obs_section_thread;
extern const obs_section obs_section_fiber;
/* What a thread attribute set says about the stack a running thread is on, and which
 * end of that stack the address it reports is. After 030-thread: it describes a thread,
 * so a platform that cannot make one has nothing to describe. */
extern const obs_section obs_section_stackattr;
/* The platform's futex - censused for presence and never called until now. Every wait
 * runs on a worker nobody joins, so a platform that never returns from one loses that
 * thread rather than the run. */
extern const obs_section obs_section_syncaddr;
/* The C runtime, above the kernel primitives its allocator is built on. */
extern const obs_section obs_section_libc;
/* Floating point, separated because it fails in its own particular ways. */
extern const obs_section obs_section_math;

/* Operating-system services. */
extern const obs_section obs_section_file;
extern const obs_section obs_section_disc;
extern const obs_section obs_section_selfaudit;
extern const obs_section obs_section_reach;
extern const obs_section obs_section_time;
extern const obs_section obs_section_module;
extern const obs_section obs_section_user;

/* Presentation layers, reached only once everything above holds. */
extern const obs_section obs_section_video;
extern const obs_section obs_section_videobuf;
extern const obs_section obs_section_audio;
extern const obs_section obs_section_input;
extern const obs_section obs_section_input_ext;
extern const obs_section obs_section_net;
extern const obs_section obs_section_shellui;

/* Not a layer. A census of the whole known surface, placed last because it answers a
 * different question from everything above it and because it is the one section that
 * is meaningful even when every other section has failed. */
extern const obs_section obs_section_responsive;
extern const obs_section obs_section_sync;
/* The bounds of the primitives 015-sync proves work: what a poll's count argument
 * means, what a bad handle returns, which wait-mode bits are understood. Never waits.
 */
extern const obs_section obs_section_syncbounds;
extern const obs_section obs_section_posix;
/* The failure convention of the POSIX-named exports, held against the vendor encoding
 * their own twins use. Records the encoding rather than asserting one. */
extern const obs_section obs_section_posixerr;
extern const obs_section obs_section_relational;
extern const obs_section obs_section_measure;
extern const obs_section obs_section_layout;
extern const obs_section obs_section_oracle;
extern const obs_section obs_section_memmap;
extern const obs_section obs_section_jit;
/* How many section rows the screen can hold.
 *
 * **This lives here, beside the sections, because it was in `screen.c` and drifted.**
 * It was 32 while the registry grew to 33, and the thirty-third row was dropped by a
 * bounds test with nothing said - the text stream stayed complete and the screen
 * quietly showed `SECTION 32 OF 33` forever.
 *
 * The comment above the old constant described that exact failure as the reason for the
 * value. Describing a hazard is not the same as preventing it: the number still had to
 * be maintained by hand against a list in another file, and it was not. `registry.c`
 * now asserts the two agree at compile time, so the next section added either fits or
 * fails the build. (D259)
 *
 * **48 to 56 on 2026-09-07**, for the two sections the stack-attribute and futex probes
 * added. The ceiling is what two columns hold, not an arbitrary headroom:
 * `screen.c` splits the list at two columns and its own note puts that at about sixty
 * sections, so 56 stays inside the layout that exists. Going past it is the point at
 * which the third column that note describes has to be written, and the assertion in
 * `registry.c` is what will say so. */
#define OBS_SCREEN_MAX 56

extern const obs_section obs_section_modules;
extern const obs_section obs_section_modlink;
extern const obs_section obs_section_modvaddr;

extern const obs_section obs_section_sysctl;
extern const obs_section obs_section_kernelcall;
extern const obs_section obs_section_layoutmap;
extern const obs_section obs_section_exports;
extern const obs_section obs_section_kernelprobe;

/* Stash payload_args at entry, where rdi still holds it, for the kernel-probe section.
 * A no-op's worth of work, but it must run first: see src/sections/kernelprobe.c. */
void obs_capture_payload_args(unsigned long args);
void obs_set_payload_kexport_table(void *table);
extern const obs_section obs_section_imports;

/* The encoder the console drives for its own recordings. Every check is a refusal: the
 * arities are assumed, and the pointer-taking calls are left to the protocol. */
extern const obs_section obs_section_record;
extern const obs_section obs_section_encoder;

/* The decode counterparts to the encoder, resolved the same way: whether the
 * media-decode libraries load and export the entry points a stream client or player
 * needs. Resolution only - the decode calls' structure layouts are unconfirmed, so none
 * is called. */
extern const obs_section obs_section_videodec;
extern const obs_section obs_section_audiodec;

/* GPU compute. Always present - it reports a skip when built without OBS_GPU - so the
 * capability never silently disappears from the report. */
extern const obs_section obs_section_gpu;
extern const obs_section obs_section_gnm;
extern const obs_section obs_section_agc;
extern const obs_section obs_section_gpucap;
extern const obs_section obs_section_surface;

/* The blind prober. Compiled in only under OBS_BULK - it is the one section expected to
 * end the process, and the default suite has to run to completion. */
extern const obs_section obs_section_bulk;

/* Calls `fn` once per censused symbol with the library it belongs to.
 *
 * Used by the host build to emit the manifest the module build links against. The
 * census tables are the only complete record of which library a name comes from, and
 * a second copy would go stale without anything noticing. */
void obs_surface_each_symbol(void (*fn)(const char *library, const char *symbol));

#endif /* OBSCENE_SECTIONS_H */
