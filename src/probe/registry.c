/*
 * The running order.
 *
 * This list is the most important thing in the program, so it is one explicit array
 * in one file rather than something assembled from linker sections or constructors.
 * Ordering that emerges from link order is ordering nobody can read, and the whole
 * value of the report depends on a failure at the top being read before a failure at
 * the bottom.
 *
 * Base layers first. Each section may depend on capabilities established above it
 * and must never depend on one below.
 */

#include "obscene/harness.h"
#include "obscene/sections.h"

const obs_section *const obs_sections[] = {
    /* The report itself, and the clocks. */
    &obs_section_boot,
    /* Before anything whose absences need attributing to a generation, and before
     * the clocks, since it needs nothing but symbol addresses. */
    &obs_section_generation,
    /* Before every behavioural section, because it changes how their failures read: a
     * function that answers the same thing to everything is unimplemented, not wrong. */
    &obs_section_responsive,
    &obs_section_kernel,
    /* Synchronisation, before anything that might rely on it. Chosen from what an
     * emulator said it had just fixed - see src/sections/sync.c. */
    &obs_section_sync,
    /* POSIX under its own names, after the vendor spelling of the same locks: the
     * comparison check needs both, and reading them in this order puts the vendor
     * result on the page first. */
    &obs_section_posix,
    /* Relations, after the sections whose functions they exercise: a reader wants the
     * value checks first, and a relation that fails where those passed is the more
     * interesting result for having them above it. */
    &obs_section_relational,
    /* Resources everything else is built from. */
    &obs_section_memory,
    &obs_section_thread,
    /* The C runtime, which the allocator builds on both of the above. */
    &obs_section_libc,
    &obs_section_math,
    /* Operating-system services. */
    &obs_section_file,
    /* After the file section, whose filesystem capability both need. Reach first: it reports a
     * plain jailed/escaped verdict, so the SELF audit's skip-or-confirm below reads against a
     * known filesystem context. Behaviour only, both of them. */
    &obs_section_reach,
    /* Confirms selfish's SELF format table against a real container, on the console, reporting
     * only which rows the current generation keeps. Never copies the header off the box. (selfish#D086) */
    &obs_section_selfaudit,
    &obs_section_time,
    &obs_section_module,
    &obs_section_imports,
    &obs_section_user,
    /* Presentation, reached only once all of the above holds. */
    &obs_section_video,
    &obs_section_videobuf,
    &obs_section_audio,
    &obs_section_input,
    /* Recording, last of the presentation layer: it drives the encoder behind the same
     * output the video section acquires, so a reader wants to know whether that output
     * works before reading anything about what it encodes.
     *
     * Numbered into this group rather than appended at the end, because the report
     * contract requires sections to ascend and the numbers are what carry the layering.
     * Putting it after the census read fine in the registry and failed the gate. */
    &obs_section_record,
    &obs_section_encoder,
    /* What the platform actually has, before the census that tests a list we wrote.
     * Read in that order deliberately: the inventory says what is there, and the census
     * then says how much of what we know about is among it. */
    &obs_section_modules,
    &obs_section_modlink,
    &obs_section_modvaddr,
    /* Not a layer - a census. Last because it answers a different question, and
     * because its answer is useful even when everything above it went red. */
    /* Measurements last but for the census: they need the time sources the earlier
     * sections establish, and a reader wants the verdicts before the figures. */
    &obs_section_measure,
    /* The instrument, after the measurements and before the census: it answers
     * questions rather than testing answers, so it reads last among the sections that
     * call anything. */
    &obs_section_layout,
    /* Named knobs, beside the oracle for the same reason: it asks the platform about
     * itself rather than testing it. Before the oracle because a value read by name is
     * the plainest question in the suite, and one of its answers is what another project
     * is waiting on. */
    &obs_section_sysctl,
    /* Past the names entirely, after the section that reads them by name. Late because it
     * builds a gadget out of arithmetic on a resolved address and calls through it, which is
     * the one check here most likely to end the run - everything above it should have
     * reported first. */
    /* The handoff itself, read and classified - 136, before the section that calls through
     * it. Reads only, so it is inert off a real console. */
    &obs_section_kernelprobe,
    &obs_section_kernelcall,
    /* Where things are, beside the section that reaches them by number. Late for the same
     * reason: it asks the platform about itself rather than testing it, and a reader wants the
     * verdicts above it first. */
    &obs_section_layoutmap,
    /* Export vaddrs, confirmed by behaviour. After the map that lists addresses and before the
     * oracle, because it turns candidate offsets into measured ones by calling them - the safe
     * counterpart to 136-kernel's read-only recon. */
    &obs_section_exports,
    /* The oracle, last of the sections that call anything: it asks the platform about
     * itself rather than testing it, so a reader wants every verdict above it first. */
    &obs_section_oracle,
    /* The map, last: it is the longest-running section and the one whose records a
     * reader is most likely to scroll to the end for. */
    &obs_section_memmap,
    /* GPU compute, before the census: it measures what the device computes, which is a
     * question about the platform, so it belongs with the sections that call things rather
     * than with the census that only counts them. Skips cheaply when built without the GPU
     * capability. */
    &obs_section_gpu,
    /* The console GPU API, right after the GPU execution probe: same subsystem, the other axis
     * (the sceGnm calls rather than what the device computes). Skips as "not present" on any
     * loader without libSceGnmDriver, the host build included. */
    &obs_section_gnm,
    &obs_section_surface,
    /* After the census, because it needs nothing the census establishes and because it
     * is the only section that may not return. Anything placed behind it would be lost
     * on the first function that faults. */
    &obs_section_bulk,
};

const unsigned int obs_section_count = OBS_COUNT(obs_sections);

/* The screen has to be able to draw every section.
 *
 * Checked here rather than in `screen.c` because this is the file that decides how many there
 * are. A section added below without room on screen is now a build failure naming this line,
 * where before it was a row silently dropped and a display stuck at `SECTION 32 OF 33`. (D259)
 */
_Static_assert(OBS_COUNT(obs_sections) <= OBS_SCREEN_MAX,
               "more sections than the screen can draw - raise OBS_SCREEN_MAX in sections.h");
