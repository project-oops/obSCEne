/*
 * The report stream.
 *
 * One record per line, pipe-separated, fixed field order. Machine-readable first and
 * human-readable second, because the intended reader is an agent diffing this run
 * against the last one to decide whether a change helped. Colour and grouping are a
 * presentation concern and live in `obscene-tool pretty`, so the guest binary stays
 * small and the format stays single.
 *
 * The full contract is docs/OUTPUT.md. Treat it as an interface: parsers exist.
 */

#ifndef OBSCENE_REPORT_H
#define OBSCENE_REPORT_H

#include "obscene/harness.h"
#include "obscene/status.h"

/* Field separator. A detail string must never contain this. */
#define OBS_SEP '|'
/* Record prefix, so report lines survive being interleaved with emulator logging. */
#define OBS_PREFIX "OBS"
/* Format version. Bumped when field order or meaning changes, never for additions
 * at the end of a line. */
#define OBS_FORMAT_VERSION 1

/* Identifies the build that produced a report.
 *
 * A diff between two reports has to answer "what changed" - and the answer is very
 * different when the probe changed than when the platform under it did. Passed in at
 * compile time because a freestanding guest has no clock it can trust: asking the
 * platform for the time would make the stamp a measurement of the thing being
 * measured. */
#ifndef OBSCENE_BUILD_ID
#define OBSCENE_BUILD_ID "unknown"
#endif

/* Which shape this binary is.
 *
 * `module` is the vendor format a system loader takes - and therefore what an emulator
 * takes. `payload` is a plain ELF, which only a homebrew loader running on the console
 * accepts. `host` is neither.
 *
 * Carried in the report because the two are not comparable: they exercise the same
 * checks through different loaders, so a diff across them measures the loader rather
 * than the change. Without this a reader cannot tell which they are holding. */
#ifndef OBSCENE_TARGET
#define OBSCENE_TARGET "host"
#endif

void obs_report_meta(unsigned int sections, unsigned int checks);
void obs_report_build(void);

/* The measured execution context of the run - "<delivery>/<generation>" plus a basis -
 * emitted alongside `build`. See obs_report_context in src/report.c and obs_run_context
 * in runtime. */
void obs_report_context(const char *name, const char *basis);

/* The GPU a set of `gpu` records came from, and how it was reached.
 *
 * Emitted once before the results. This is the provenance that keeps a software result
 * (llvmpipe, in the build VM) from ever being read as silicon (a Deck's gfx1033, a
 * console) - the device string is the whole difference between a plumbing check and an
 * oracle, and it must travel with the numbers. */
void obs_report_gpu_device(const char *backend, const char *device, const char *type);

/* One lane of a GPU kernel: what went in, what came back.
 *
 * An observation, not a verdict - the raw input and output bit patterns of one lane,
 * with nothing asserted about them. The point is to compare these against an emulator's
 * shader translation, which is a diff the recipient does, not a pass this program can
 * judge. Bits rather than decimals because a reciprocal's last ULP is exactly where
 * hardware and emulation part company, and a rounded decimal would hide it. */
void obs_report_gpu(const char *kernel, unsigned int lane, uint32_t input,
                    uint32_t output);

/* One lane of a multi-operand GPU kernel: the output, then each input.
 *
 * A separate record from `gpu` on purpose. That record's input field is a single value,
 * and a consumer already parses it; widening that field to a list would change a
 * field's meaning, which the contract forbids (docs/OUTPUT.md). This record puts the
 * single output first and the variable-length inputs after it, so a reader takes three
 * fixed fields - kernel, lane, output - and then as many inputs as the line still
 * holds. That is how fma, pow, min and max record their operands without disturbing the
 * unary format. */
void obs_report_gpu_op(const char *kernel, unsigned int lane, uint32_t output,
                       const uint32_t *inputs, unsigned int input_count);

/* The state of the command socket, and the port it is on.
 *
 * Emitted when a serving build starts listening, so a reader of the report - on stdout,
 * in the file sink, or on the drawn screen - knows the module is waiting for the driver
 * and where to reach it. The port is the same fact the driver needs; on an emulator
 * that maps guest sockets to the host (shadPS4), the host connects to it on loopback.
 */
void obs_report_net(const char *state, unsigned int port);

/* One field of the on-screen status readout, mirrored into the report.
 *
 * The HUD's facts - memory, VRAM, generation, address - are what the platform says
 * about itself, and until now they lived only on the drawn screen. A driver reading the
 * report could see the socket come up but not a byte of the rest. This record carries
 * each field with its honest tier: `known` (read through a confirmed signature),
 * `unconfirmed` (the query resolves but its signature is not, so the value is
 * `unknown`), or `absent` (no such query here). The `state` field is the point - it is
 * what keeps "unknown because missing" distinct from "unknown because not yet wired".
 *
 * An observation, never a verdict, and never corpus provenance: a probe cannot certify
 * its own machine (D108), so these are recorded and diffed across platforms, not
 * graded. See src/sysinfo.c. */
void obs_report_sysinfo(const char *field, const char *state, const char *value);

/* Where the report was also written, or that it was not.
 *
 * Emitted once, immediately after the build record, and **always** - a null path
 * produces a record saying so rather than no record at all. That asymmetry is the
 * point: a missing record is indistinguishable from an older build that never had a
 * sink, whereas `sink|none` is a fact about this run.
 *
 * See src/sink.c. */
void obs_report_sink(const char *path);

/* How many checks this run is skipping because a previous run of the same build
 * announced them and never finished, and whether the skip set filled up. */
void obs_report_resume(unsigned int skipped, int overflowed);
/* Whether one function reads its arguments at all: "responds", "silent" or "absent".
 * See src/sections/responsive.c for why this is separable from correctness. */
void obs_report_responsive(const char *library, const char *symbol, const char *verdict,
                           uint64_t observed);

/* A number the platform produced, recorded rather than judged.
 *
 * `quantity` names what was measured ("requested", "observed", "frequency"), `unit`
 * what it is in ("us", "ticks", "hz"). Several of these belong to one check.
 *
 * Deliberately not a pass or a fail: a measurement is a fact, and the meaning is in
 * comparing it across platforms rather than against an expectation this project would
 * have had to invent. See docs/OUTPUT.md. */
void obs_report_measure(const char *id, const char *symbol, const char *quantity,
                        uint64_t value, const char *unit);

/* How far a looping check has got. A crash inside a loop otherwise names only the
 * check, and "fails on the first" and "fails on the fortieth" are different bugs. */
void obs_report_progress(const char *id, uint64_t reached);

/* One loaded module, as the platform names it. An inventory record, not a verdict.
 * See src/sections/modules.c. */
void obs_report_module(const char *name, uint64_t handle);

/* One word from the front of the module info structure, emitted only when its layout
 * is not what this program assumes. A diagnostic for deriving the layout. */
void obs_report_module_word(unsigned int offset, uint64_t value);

/* Whether the drawn report came up, and what it is. See src/display.c. */
void obs_report_display(const char *state, const char *detail, uint64_t code);

void obs_report_section(const obs_section *section);
/* Emitted *before* the call. See the note in harness.h. */
void obs_report_attempt(const obs_check *check);
void obs_report_result(const obs_check *check, obs_result result);
/* One symbol in the census: library, name, and whether the loader resolved it.
 *
 * Separate from a check result because it is a different kind of claim - presence,
 * not behaviour - and because there are an order of magnitude more of them. */
void obs_report_symbol(const char *library, const char *symbol, int present,
                       obs_availability availability);
void obs_report_section_tally(const obs_section *section, obs_tally tally);
void obs_report_tally(obs_tally tally);

/* How far up the stack the platform got before the floor gave out.
 *
 * # Why a sum is the wrong headline
 *
 * "65 pass, 6 partial, 38 fail, 7 skip" has no denominator and no shape. It cannot say
 * whether that is good, and worse, it throws away the one piece of structure this suite
 * deliberately has: checks depend on capabilities other checks establish, so a failure
 * near the foundation matters more than one at the top. Summing treats them alike.
 *
 * It also rewards the wrong work. Adding three hundred census entries moves the total
 * and changes nothing about whether a bug would be found.
 *
 * # Measured over capabilities, not over section order
 *
 * Section order is a reading order. The real dependency structure is
 * `requires_caps`/`provides_caps` - which capabilities a platform actually established,
 * and which were still missing when something needed them.
 *
 * So the frontier is two numbers: how many capabilities were established, and how many
 * checks were skipped for want of one. The second is the interesting one, because it
 * counts what could not even be attempted - the checks behind the floor. */
void obs_report_frontier(unsigned int established, unsigned int blocked,
                         unsigned int deepest_section);

/* Bytes a call wrote, recorded and not interpreted.
 *
 * # The record that makes this an instrument
 *
 * Every other record carries a conclusion. This one carries evidence, and deliberately
 * refuses to draw anything from it: `what` names the buffer, `offset` says where in it,
 * and the bytes are printed in hex. No field names, no sizes, no meaning.
 *
 * # Why that is the useful thing
 *
 * `docs/HARDWARE-PROBE.md` asks for exactly this and explains why: on a console the
 * question "did this behave correctly?" is uninteresting, because the answer is yes.
 * What cannot be got any other way is *what the platform actually wrote* - and a
 * hexdump of a buffer settles a struct layout permanently, where four experiments
 * guessing at it settle nothing.
 *
 * # And it routes around D008 rather than breaking it
 *
 * D008 forbids calling a function whose struct layout is unknown, because a wrong
 * layout produces a call that succeeds and does the wrong thing. Dumping bytes needs no
 * layout: it needs the arity, and a buffer larger than anything the call could write.
 * The expectation is what was missing, and this record does not have one.
 *
 * Bytes are emitted in bounded runs so no line grows unbounded - the report is
 * line-oriented and a parser should never meet a record it cannot buffer. */
void obs_report_bytes(const char *id, const char *symbol, const char *what,
                      unsigned int offset, const unsigned char *bytes,
                      unsigned int len);

/* Dumps a whole buffer as a series of `bytes` records.
 *
 * Trailing zeroes are dropped, because a fixed oversized buffer is mostly zeroes and
 * printing them buries the part that matters. The last non-zero byte is reported as the
 * extent, which is itself the answer to "how much does this call write?" - the question
 * that costs the most to establish any other way. */
void obs_report_buffer(const char *id, const char *symbol, const char *what,
                       const unsigned char *bytes, unsigned int len);

/* Dumps a buffer against what it held before the call, so a field written as zero is
 * not mistaken for a field that was never written.
 *
 * `obs_report_buffer` above takes the last non-zero byte as the extent, which is right
 * for a buffer that started zeroed only while the platform never writes a zero. It
 * does: a reserved word, a count of none, a null pointer. Those are fields, and on
 * hardware "wrote 0 here" and "did not touch this" are different facts about a
 * structure - one of them says the field exists.
 *
 * So the buffer is poisoned before the call and compared after it. `before` is what was
 * written in, `after` what came back. The extent is the last **changed** byte.
 *
 * Call it twice with two different poison values and union the results if the answer
 * matters: a byte the platform happens to write equal to the poison reads as untouched,
 * and no single pattern avoids that. `obs_layout_patterns` is the pair this project
 * uses. */
void obs_report_written(const char *id, const char *symbol, const char *what,
                        const unsigned char *before, const unsigned char *after,
                        unsigned int len);

/* One point on a size ladder: a size argument, and whether the call took it.
 *
 * The question this answers is "how big is that structure", and it answers it without
 * anyone deciding anything. A call given a size smaller than the structure it fills
 * refuses; given enough, it works. Walk a ladder of sizes and the boundary between the
 * two **is** the size.
 *
 * That matters more than one more hexdump because it is `derived` rather than
 * `assumed`: the platform is the one drawing the line, and every call taking a size can
 * be asked. (D008) */
void obs_report_size(const char *library, const char *symbol, unsigned int argument,
                     unsigned int size, int accepted, uint64_t code);

/* One blind call: what was called, and what came back.
 *
 * Emitted twice per symbol. The first carries outcome "attempt" and no value, before
 * the call; the second carries the classification and the returned value, after it. **A
 * record with no partner is the finding** - it names the function that ended the
 * process, which is the one thing a crash otherwise destroys the evidence for.
 *
 * `index` is the position in the census order, and it is what the resume harness reads.
 * It is in the record rather than derived from line count because a run that starts
 * partway through has no line count to derive it from.
 *
 * `outcome` labels the value by an explicit rule stated in src/sections/bulk.c;
 * `returned` is the raw answer, so a reader who disagrees with the label still has the
 * fact. */
void obs_report_call(const char *library, const char *symbol, uint64_t index,
                     const char *outcome, uint64_t returned);

/* Whether the platform knows a name, and where it put it.
 *
 * # The record that turns a probe into a name oracle
 *
 * Identifiers are one-way: `SHA-1(name)` truncated, so a name cannot be recovered from
 * one. The established route is to guess - generate candidate names, hash them, and
 * hope for a match against identifiers seen in the wild. It works and it is enormously
 * expensive, and it can only ever find names that something happens to import.
 *
 * If the platform can look a symbol up **by name**, that whole problem collapses into
 * one question per candidate, with a yes or no answer - and it reaches functions no
 * title imports at all, which guessing never can.
 *
 * `address` is reported because a resolved symbol's address is worth having in its own
 * right, and zero is a legitimate answer meaning "known but not here". */
void obs_report_resolve(const char *library, const char *symbol, int present,
                        uint64_t address);

/* A call's actual return for a deliberately wrong argument.
 *
 * Every negative check in this suite asserts that a bad argument is *refused* and
 * throws away the code that came back. On an emulator that is the right shape - the
 * code is invented anyway. On hardware the code **is the finding**: an implementation
 * returning a value no caller recognises makes guests retry forever, and a table of
 * real codes turns "unimplemented" into "fails the way the caller expects".
 *
 * `argument` describes what was wrong in words, because "null path" is what a reader
 * needs and the value alone does not say. */
void obs_report_error_code(const char *library, const char *symbol,
                           const char *argument, uint64_t returned);

/* One region of the platform's memory map, as far as it can be read.
 *
 * # Two fields are a hypothesis and are labelled as one
 *
 * Walking an enumeration means knowing where in the returned structure the next offset
 * lives, and that is a layout - the thing D008 forbids assuming. So `first` and
 * `second` are the two sixty-four-bit values at the start of the structure, named for
 * their *position* and not for their meaning. A reader who later learns the layout can
 * reinterpret them; one who trusts a field called `end` cannot.
 *
 * The authoritative record is the accompanying `bytes` dump. This exists because a
 * memory map is read as a list, and a list of hexdumps is not a list.
 *
 * `progressing` says whether the walk advanced on this step, which is the walk's own
 * check on its hypothesis - see `150-memory-map`. */
void obs_report_region(unsigned int index, uint64_t first, uint64_t second,
                       int progressing);
void obs_report_end(void);

/* One of this program's own imports: whether the loader bound it, and whether the name
 * resolves at run time. The two disagree, and that disagreement is the finding. */
void obs_report_import(const char *library, const char *symbol, int linked,
                       int resolvable);

#endif /* OBSCENE_REPORT_H */
