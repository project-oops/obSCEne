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
/* The C runtime, above the kernel primitives its allocator is built on. */
extern const obs_section obs_section_libc;
/* Floating point, separated because it fails in its own particular ways. */
extern const obs_section obs_section_math;

/* Operating-system services. */
extern const obs_section obs_section_file;
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

/* Not a layer. A census of the whole known surface, placed last because it answers a
 * different question from everything above it and because it is the one section that
 * is meaningful even when every other section has failed. */
extern const obs_section obs_section_responsive;
extern const obs_section obs_section_sync;
extern const obs_section obs_section_posix;
extern const obs_section obs_section_relational;
extern const obs_section obs_section_measure;
extern const obs_section obs_section_layout;
extern const obs_section obs_section_oracle;
extern const obs_section obs_section_memmap;
/* How many section rows the screen can hold.
 *
 * **This lives here, beside the sections, because it was in `screen.c` and drifted.** It was
 * 32 while the registry grew to 33, and the thirty-third row was dropped by a bounds test with
 * nothing said - the text stream stayed complete and the screen quietly showed
 * `SECTION 32 OF 33` forever.
 *
 * The comment above the old constant described that exact failure as the reason for the value.
 * Describing a hazard is not the same as preventing it: the number still had to be maintained
 * by hand against a list in another file, and it was not. `registry.c` now asserts the two
 * agree at compile time, so the next section added either fits or fails the build. (D259) */
#define OBS_SCREEN_MAX 48

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
extern const obs_section obs_section_imports;

/* The encoder the console drives for its own recordings. Every check is a refusal: the
 * arities are assumed, and the pointer-taking calls are left to the protocol. */
extern const obs_section obs_section_record;
extern const obs_section obs_section_encoder;

/* GPU compute. Always present - it reports a skip when built without OBS_GPU - so the
 * capability never silently disappears from the report. */
extern const obs_section obs_section_gpu;
extern const obs_section obs_section_gnm;
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
