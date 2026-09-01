/*
 * The process-parameter structure.
 *
 * A console loader reads this before it transfers control. Our modules have never had
 * one - a stock linker has no idea the segment type exists - and a module without one
 * loads, maps, reports its entry point, and then faults inside the loader before
 * running a single guest instruction.
 *
 * A linker script can place this in its own segment but cannot invent its contents, so
 * it lives here as ordinary data.
 *
 * # The experiment this used to be, and how a console answered it
 *
 * This was deliberately minimal: only the leading size, everything after it zero rather
 * than guessed, posing one question - does a loader want this segment to *exist*, or to
 * *contain* something?
 *
 * **A console answered `contain`, and named the field.** Launching from `/app0`, its
 * kernel reads this structure and says what it found:
 *
 *     [KERNEL] kern_get_sdk_compiled_version: m=0 sz=80
 *     [Syscore App] Error: failed to get both Prospero and PS4 SDK version of
 *                   /app0/eboot.bin, ...ignored
 *
 * `sz=80` is `OBS_PROC_PARAM_SIZE` exactly, so the structure was found and read; `m=0`
 * is the magic, which this file never wrote. So the size was right and the contents
 * were not, which is the answer the zeroed version was built to get.
 *
 * The magic and the entry count now come from the OpenOrbis PS4 ELF specification,
 * which documents the layout and records the same `0x50` size this file had already
 * chosen. The SDK version stays zero: it is a claim about which SDK built this, there
 * is no SDK here, and the console's own message says that particular failure is
 * *ignored*. A wrong value in a field a loader acts on is still worse than an obviously
 * empty one (D008) - what changed is that the magic is no longer an unknown, it is
 * documented.
 */

#include <stddef.h>
#include <stdint.h>

/* Bytes reserved for the structure.
 *
 * Chosen to be comfortably larger than the fields a loader is likely to read, so that
 * an over-read lands inside the object rather than past it. Too large is harmless; too
 * small is an out-of-bounds read in the loader, which surfaces as a crash with no
 * indication that a size was the cause. */
/* `0x60`, which is what a launching executable states and what the fields below add up
 * to.
 *
 * It was `0x50`, chosen as "comfortably larger than the fields a loader is likely to
 * read" so that an over-read lands inside the object. That reasoning holds for the
 * *upper* bound and says nothing about the lower one: the three pointers a real
 * executable carries sit at
 * `+0x38`, `+0x40` and `+0x48`, so `0x50` had room for them and declared none. The size
 * is now derived from the structure rather than picked. */
#define OBS_PROC_PARAM_SIZE 0x60
/* What a real launching title states at `+0x0c` and `+0x10`.
 *
 * Both were zero here. Both are read by the loader before a single instruction of this
 * module runs, so a run cannot report on them - the only way to see them is to read a
 * build that works and compare, which is what `selfish`'s `procparam` probe is for.
 * (D242) */
#define OBS_PROC_PARAM_ENTRIES 5
/* Overridable from the Makefile so the two behaviours can be compared with one flag
 * rather than by editing this file between runs - which is how a comparison ends up
 * being between two builds that differed in more than the thing under test. */
#ifndef OBS_PROC_PARAM_SDK
#define OBS_PROC_PARAM_SDK 0u
#endif

/*
 * Placed in its own section so the linker script can give it its own segment. `used`
 * keeps it through dead-code elimination: nothing in this program refers to it, and
 * without the attribute it is exactly the sort of thing a linker discards.
 */
/* The magic a loader looks for, `ORBI`, little-endian.
 *
 * Named in the OpenOrbis PS4 ELF specification, which gives the whole structure: a
 * size, this magic, an entry count, an SDK version, then padding and the entries
 * themselves. The size it records as usual - `0x50` - is the size this file already
 * chose independently. */
#define OBS_PROC_PARAM_MAGIC 0x4942524FU

/*
 * The three structures the process parameters point at.
 *
 * # Why they exist, which is not a guess
 *
 * The comment above used to say this "supplies no `sceLibcParam` or
 * `sceKernelMemParam`, and claiming entries that are not there would be worse than
 * zero". That reasoning is right about *inventing* an entry and wrong about omitting
 * one: the platform library reads these before a single instruction of this program
 * runs, and a null where it expects a structure is not a gap it tolerates.
 *
 * On hardware the whole process died before its entry point with:
 *
 *     # signal: 11 (SIGSEGV)
 *     # reason: page fault (user write data, page not present)
 *     # fault address: 0000000000000028
 *     # rip: 000000080003333b            <- inside libkernel, not inside us
 *
 * A write to `0x28` with a null base. A launching executable's first structure here is
 * `0xa8` bytes and holds a **zero** at exactly `+0x28` - a slot the platform library
 * fills in. Ours was a null pointer, so the same store landed on absolute `0x28`.
 * (D219)
 *
 * # What is in them, and what deliberately is not
 *
 * Each begins with its own size, which is the pattern all three share in a real
 * executable and is what makes them safe to supply: a structure that states its length
 * can be read forward by something that knows more fields than we do, and every field
 * past the length is zero rather than invented. One of the three carries real content
 * in a real executable - an entry count, a version, and two pointers to values a title
 * chose. This program chooses none of them: it allocates nothing, so the honest value
 * for all of it is zero.
 *
 * They are **not** `const`. That is the entire point: the fault above was a write.
 */
#define OBS_LIBC_PARAM_SIZE 0xA8
#define OBS_MEM_PARAM_SIZE 0x38
#define OBS_THIRD_PARAM_SIZE 0x10

/* Sized and otherwise empty. `.data` rather than `.bss`, because the size field is
 * non-zero and a loader reads these out of the file. */
__attribute__((used)) static struct {
    uint64_t size;
    uint8_t rest[OBS_LIBC_PARAM_SIZE - 8];
} obs_libc_param = {.size = OBS_LIBC_PARAM_SIZE, .rest = {0}};

__attribute__((used)) static struct {
    uint64_t size;
    uint8_t rest[OBS_MEM_PARAM_SIZE - 8];
} obs_mem_param = {.size = OBS_MEM_PARAM_SIZE, .rest = {0}};

__attribute__((used)) static struct {
    uint64_t size;
    uint8_t rest[OBS_THIRD_PARAM_SIZE - 8];
} obs_third_param = {.size = OBS_THIRD_PARAM_SIZE, .rest = {0}};

__attribute__((used, section(".sce_process_param"))) static const struct {
    /* Size of this structure. The one field a forward-reading loader must have. */
    uint64_t size;
    /* The magic. A loader that does not find it does not believe the rest. */
    uint32_t magic;
    /* How many entries follow the fixed fields. A real launching title states five. */
    uint32_t entry_count;
    /* Which SDK this was built against. Zero - but for a different reason than before.
     *
     * The comment here used to say a real launching executable declares zero too. **It
     * does not**: read straight out of one at this offset, `11 80 00 08`, `0x08008011`.
     * That claim came from a kernel log line, `SDK vesion: PS4:00000000 PPR:00000000`,
     * which was *this program's own* log. A run's own output was read as evidence about
     * somebody else's build, which is the same mistake as D232, D233 and D235 wearing
     * different clothes.
     *
     * So the value was corrected to the measured one, and then the correction was
     * measured too. Identical builds differing only in this field, on the same console:
     *
     *   import binding   no difference. 98 symbols measured in both, none changed.
     *   `sceKernelDlsym` 105 of 122 resolvable at zero, 0 of 173 at 0x08008011.
     *
     * Declaring a real SDK version stops run-time module resolution answering, and the
     * census is built on run-time resolution. Zero stays, now on evidence rather than
     * on a misread log, and `PROC_SDK` in the Makefile repeats the experiment in one
     * word. (D244) */
    uint32_t sdk_version;
    /* Stated separately in the kernel log's `SDK vesion: PS4:... PPR:...`. Zero in the
     * real title too, and this one is genuinely zero rather than assumed so. */
    uint32_t sdk_version_second;
    /* Four slots a real executable leaves null. Named `rest` rather than guessed at. */
    uint64_t unknown[4];
    /* The three it does not. See above. */
    const void *libc_param;
    const void *mem_param;
    const void *third_param;
    /* Two more it leaves null, to the stated size. */
    uint64_t trailing[2];
} obs_process_param = {
    .size = OBS_PROC_PARAM_SIZE,
    .magic = OBS_PROC_PARAM_MAGIC,
    .entry_count = OBS_PROC_PARAM_ENTRIES,
    .sdk_version = OBS_PROC_PARAM_SDK,
    .sdk_version_second = 0,
    .unknown = {0},
    /* Required, and measured rather than assumed.
     *
     * The hope was that this one could stay null - this program has no libc of its own,
     * and announcing libc parameters is what makes the system go looking for the
     * title's `libc.prx`. It cannot. Built with all three supplied the process got past
     * the fault; built with **only this one nulled** and the other two left in place,
     * it faults again at the same instruction and the same address as before:
     *
     *     # fault address: 0000000000000028
     *     # rip: 000000080003333b
     *
     * So this is the block the platform library writes into at `+0x28`, it is not
     * optional, and the bundled modules it obliges the package to carry are the price.
     * (D219) */
    .libc_param = &obs_libc_param,
    .mem_param = &obs_mem_param,
    .third_param = &obs_third_param,
    .trailing = {0},
};

/*
 * The dynamic interpreter path.
 *
 * # Why a module that links nothing needs one
 *
 * The control experiment (D097) put a standard OpenOrbis module through the same five
 * loaders as this one. It carries nine program headers where obSCEne carries six, and
 * the first of the four missing ones is `PT_INTERP`, pointing at this string.
 *
 * Two cheaper explanations for that gap were tried and both failed (D098): the vendor
 * packaging tool adds none of them, and linking `-pie` instead of `-shared` changes
 * nothing. The reason is that `link/module.ld` declares `PHDRS` explicitly, so the
 * script decides which segments exist and lld emits no others. A segment that is not
 * declared cannot appear, whatever the link mode.
 *
 * So the header has to be declared in the script, and the script cannot invent a string
 * - the same division of labour as the process-parameter structure above.
 *
 * # The value, and why it is not a guess
 *
 * `/libexec/ld-elf.so.1` is the FreeBSD run-time linker path, and the console's system
 * software derives from FreeBSD. It is not inferred from that lineage, though: it is
 * what the OpenOrbis linker script embeds at the very front of `.text`, in a module
 * that four loaders accept and fpPS4 runs to completion. Copied from a working artefact
 * rather than reasoned towards.
 *
 * # What it is expected to change, and what would disprove it
 *
 * fpPS4 maps every segment of obSCEne, reads the vendor tables correctly, prints its
 * entry point and then the process ends with no output - while running the control on
 * the same binary and printing over its own channel. If a missing interpreter is why,
 * this is the change that fixes it. If it is not, fpPS4 behaves exactly as before and
 * the next header is the next candidate, which is a cheaper way to find out than
 * porting the whole build.
 */
__attribute__((used, section(".interp"))) static const char obs_interp[] =
    "/libexec/ld-elf.so.1";

/*
 * A placeholder for the vendor dynamic segment.
 *
 * The linker script declares a program header for it, but lld drops a declared segment
 * that no section occupies - and a header that does not exist cannot be pointed at a
 * table afterwards. One byte is enough to keep it.
 *
 * The real contents are appended by `obscene-tool mkmodule --dynlib`, which then
 * rewrites this header's offset and size. Nothing reads what is here.
 */
__attribute__((
    used, section(".sce_dynlibdata"))) static const uint8_t obs_dynlibdata_placeholder =
    0;

/* ---- the module initialiser ---------------------------------------------------------
 *
 * # Why an empty function is worth a `DT_INIT`
 *
 * A console module carries `DT_INIT`, and a loader may call it before the entry point.
 * obSCEne is linked with a stock `ld` and a linker script, and neither has any reason
 * to emit one - so for a long time the module had none, in 1,426 dynamic tags.
 *
 * Three of the four loaders here do not care. shadPS4 goes straight to `RunMainEntry`;
 * fpPS4 parses the tag and its call site is commented out; PS5PCEM deliberately
 * declines, because "the executable's PS5 CRT entry calls its own `DT_INIT` routine"
 * and running it twice would construct globals twice.
 *
 * **Kyty calls it, and does not check it exists.** `StartModule` runs
 * `run_ini_fini(init_vaddr + base_vaddr, ...)`, and `run_ini_fini` casts and calls with
 * no guard at all. With no tag, `init_vaddr` is zero, so it calls `base_vaddr` - the
 * ELF header - and executes `\x7fELF` as machine code. That is the access violation at
 * `base + 2` this module produced there for months. (D173)
 *
 * # And the reason it matters beyond one emulator
 *
 * PS5PCEM's comment says the *console's* CRT calls its own `DT_INIT`. So real hardware
 * plausibly expects one, and a module without it is a fault this project would meet on
 * a machine where a single iteration costs a manual copy rather than a rebuild.
 *
 * # Empty, and not a placeholder
 *
 * Nothing belongs here. The suite runs from the entry point, which is what every loader
 * that works today calls; an initialiser that did work as well would do it twice on a
 * loader that calls both. This exists so that a loader which calls `DT_INIT` reaches a
 * `ret` instead of the ELF header.
 *
 * The signature is the one Kyty casts to - `(size_t argc, const void *argv, void
 * *func)` - and the arguments are ignored. Returning zero because every loader that
 * inspects the result reads it as success.
 */
int obs_module_init(size_t argc, const void *argv, void *func);

int obs_module_init(size_t argc, const void *argv, void *func) {
    (void)argc;
    (void)argv;
    (void)func;
    return 0;
}
