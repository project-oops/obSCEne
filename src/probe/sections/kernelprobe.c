/*
 * What elfldr handed us that a kernel escape would use - read, classified, never invoked.
 *
 * # Why this is worth a section, and why it is only a *recon* one
 *
 * A payload loaded by elfldr is given more than `getpid` in `payload_args`. The ps5-payload-sdk
 * crt0 uses the rest to escape the sandbox: a raw kernel read/write primitive and the kernel
 * base address are handed straight over, because the exploit that loaded the payload already
 * holds them. The sibling emulator needs the 12.40 kernel offsets a `ucred` patch turns on, and
 * those are *measurable* - the primitives to read kernel memory are sitting in this very struct.
 *
 * This section does the safe half of that: it reads the handoff words and **classifies** each
 * one, so a run says which of them is a kernel address and which is not. It does **not** invoke
 * any primitive. Reading and reporting a pointer cannot crash a machine; *using* an arbitrary
 * kernel read/write can, and using one that is not really there certainly can. The invocation is
 * a second section's job, gated on this one having found a real kernel context first.
 *
 * # Why it is inert everywhere except a real, jailbroken console
 *
 * The discriminator is the one number no emulator produces: a **canonical high-half address**
 * (`0xffff_8000_0000_0000` and up). That is where a real kernel lives. The sibling emulator's
 * handoff markers, its firmware skeleton, its thunks - every address it invents - sit in the low
 * half, because that is where a user process's world is. So on real hardware a word here reads as
 * `kernel` and the section reports the base it found; on any emulator no word does, and the
 * section skips with that stated. Same binary, opposite verdicts, and neither one guesses.
 *
 * # Safety, stated plainly because it is the whole point
 *
 * 1. It only ever *reads* `payload_args`, which is memory the loader mapped and handed us.
 * 2. It reads a bounded number of words, and only after `payload_args` itself passes a shape
 *    check (non-null, canonical, aligned), so a platform that never set it up is skipped rather
 *    than dereferenced.
 * 3. It calls no function pointer out of the struct and issues no primitive. Nothing here can
 *    take a machine down, which is the property that lets it ship in a suite that runs on other
 *    people's consoles.
 */

#include "common/freestd.h"
#include "common/krw.h"
#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* The handoff pointer, captured at entry where `rdi` still holds it, or zero.
 *
 * Zero means it was never captured - a build whose entry does not call the setter, which is any
 * build not loaded as an elfldr payload. That is a clean skip, not a fault. */
static unsigned long obs_payload_args = 0;

/* Called once, from the entry, with `payload_args` while `rdi` is still live. Kept trivial: the
 * entry is the only place the value is available, and the only thing it may safely do that early
 * is stash it. */
void obs_capture_payload_args(unsigned long args);
void obs_capture_payload_args(unsigned long args) {
    obs_payload_args = args;
}

const payload_args_t *obs_get_payload_args(void);
const payload_args_t *obs_get_payload_args(void) {
    if (obs_payload_args >= 0x10000UL && obs_payload_args < 0x0000800000000000UL && (obs_payload_args & 0x7UL) == 0) {
        return (const payload_args_t *)obs_payload_args;
    }
    return NULL;
}

/* How many words of the struct to read. Past the escape primitives with room to spare, and
 * bounded so a short or absent struct is never walked off the end. */
#define OBS_PARGS_WORDS 20

/* The boundary between the two canonical halves. At or above it is a kernel address on this
 * architecture; below it is where every user-space and every emulator-invented address sits. */
#define OBS_KERNEL_HALF 0xFFFF800000000000UL
/* Below this a value is a count or a flag, not a pointer. */
#define OBS_POINTER_FLOOR 0x1000UL
/* The top of the low canonical half - above this and below the kernel half is non-canonical,
 * which is what a marker range looks like. */
#define OBS_USER_CEILING 0x0000800000000000UL

/* What one word is, as far as its value alone can say. Deliberately coarse: the section reports
 * the raw value too, so a reader can look closer, and the only distinction the verdict turns on
 * is whether anything is a kernel address. */
static const char *obs_classify(unsigned long word) {
    if (word == 0) {
        return "null";
    }
    if (word < OBS_POINTER_FLOOR) {
        return "small"; /* a count, a flag, an index - not an address */
    }
    if (word >= OBS_KERNEL_HALF) {
        return "kernel"; /* canonical high half - only a real console gets here */
    }
    if (word >= OBS_USER_CEILING) {
        return "noncanonical"; /* the gap between the halves - a marker range lives here */
    }
    return "user"; /* low canonical - a userland pointer, or an emulator's invented address */
}

/* Whether `payload_args` is shaped like a pointer worth dereferencing.
 *
 * Not proof it is mapped - nothing short of reading it is - but enough to skip a platform that
 * put something that is plainly not a struct pointer in `rdi`, so the read below only ever runs
 * against a plausible one. A real loader hands a low-canonical, eight-aligned heap pointer. */
static int obs_pargs_plausible(unsigned long p) {
    if (p < OBS_POINTER_FLOOR || p >= OBS_USER_CEILING) {
        return 0;
    }
    return (p & 0x7UL) == 0;
}

static obs_result check_handoff_words(void) {
    if (obs_payload_args == 0) {
        return obs_skip("payload_args was not captured - not an elfldr-payload build");
    }
    if (!obs_pargs_plausible(obs_payload_args)) {
        return obs_skip("payload_args is not shaped like a struct pointer on this platform");
    }

    const unsigned long *args = (const unsigned long *)obs_payload_args;
    unsigned int kernel_words = 0;
    unsigned long first_kernel = 0;

    for (unsigned int i = 0; i < OBS_PARGS_WORDS; i++) {
        /* The one read, and it is only a read. On a machine that mapped the struct this is a
         * load; on one that did not, it faults, and the harness attributes the fault to this
         * announced check rather than losing the run - `try` with no `res` says exactly that. */
        unsigned long word = args[i];
        const char *kind = obs_classify(word);
        obs_report_measure("136-kernel/handoff", "payload_args", kind, (uint64_t)word, "word");
        if (word >= OBS_KERNEL_HALF) {
            kernel_words++;
            if (first_kernel == 0) {
                first_kernel = word;
            }
        }
    }

    if (kernel_words == 0) {
        /* No kernel address anywhere in the handoff. Either an emulator, which invents only
         * low-half addresses, or a console where the escape has not run yet. A clean skip: the
         * struct was read and classified, and the honest answer is that nothing here is kernel. */
        return obs_skip("no kernel address in the handoff - emulator, or pre-escape");
    }

    /* A real kernel address was handed over. Report it as the anchor a `ucred` patch measures
     * its offsets from, and pass - this is the finding the whole section exists to surface. */
    obs_report_measure("136-kernel/handoff", "payload_args", "kernel-anchor",
                       (uint64_t)first_kernel, "addr");
    return obs_pass_value((uint64_t)kernel_words);
}

static const obs_check kernelprobe_checks[] = {
    /* Gated on its own function, not a platform symbol. Every other check guards the call it
     * makes (D058); this one calls nothing - it reads payload_args, which the loader mapped -
     * so guarding an import would only make the harness skip it in the very context it exists
     * for: a raw payload, where no import is resolved. Its own address is always callable, so
     * the gate passes and the check runs, on hardware and on host alike. */
    {"136-kernel/handoff", "obscene", "read-handoff", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&check_handoff_words, check_handoff_words, OBS_FROM_ASSUMED},
};

const obs_section obs_section_kernelprobe = {
    "136-kernel",
    "What the loader handed over",
    "The handoff words read and classified, so a run says which is a kernel address and which "
    "is not - the anchor a kernel-offset measurement needs. Reads only; invokes no primitive, so "
    "it is inert on every platform that is not a jailbroken console.",
    kernelprobe_checks,
    OBS_COUNT(kernelprobe_checks),
};
