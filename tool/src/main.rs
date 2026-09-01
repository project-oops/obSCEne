//! Build and analysis tooling for the obSCEne conformance probe.
//!
//! One binary, several subcommands. It replaces a set of Python scripts that grew from
//! throwaway helpers into load-bearing parts of the build - at which point the reasons
//! for a script stopped applying and the reasons against one started to.
//!
//! # Why this is compiled
//!
//! Most of this tool manipulates binary formats: offsets, sizes, packed structures,
//! byte order. That is precisely the work where a stringly-typed `struct.pack` format
//! and an untyped integer are liabilities, and where a mistake produces a file that is
//! accepted and subtly wrong rather than one that fails.
//!
//! The build already required Python, so the "one dependency, clang" property this
//! replaced was not real - and unlike a script, this ships as a binary that needs no
//! runtime installed at all.

mod caps;
mod census;
mod claims;
mod compat;
mod consensus;
mod corpus;
mod counts;
mod crack;
mod decisions;
mod derive;
mod diff;
mod doccheck;
mod drive;
mod elf;
mod font;
mod gap;
mod gpudiff;
mod gpuref;
mod gpustats;
mod gpusurface;
mod guards;
mod hardware;
mod imports;
mod mining;
mod pretty;
mod probe;
mod protocol;
mod report;
mod rows;
mod sections;
mod selfheader;
mod shaders;
mod suffix;
mod surface;
mod unresolved;
mod vaddrs;
mod verify;

use std::path::PathBuf;
use std::process::ExitCode;
use std::time::Duration;

use clap::{Parser, Subcommand};

/// Tooling for building and analysing the conformance probe.
#[derive(Parser)]
#[command(name = "obscene-tool", version = oops_build::line!(), about)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

/// Arguments for `mkmodule`.
///
/// A struct rather than inline fields: the command has enough of them that the match
/// arm was the longest in the dispatcher, and clippy was right about it.
#[derive(clap::Args)]
pub struct MkmoduleArgs {
    /// The linked ELF, modified in place.
    file: PathBuf,
    /// Read the hash suffix from here instead of the committed data file.
    #[arg(long)]
    suffix_file: Option<PathBuf>,
    /// The `library symbol` manifest, from `obscene-host --symbols`.
    ///
    /// Every undefined symbol must appear in it. A module encodes each import as a
    /// library id, and an id with no library declared against it resolves to
    /// nothing - so a missing line here is a silent half-linked module.
    #[arg(long)]
    symbols: PathBuf,
    /// The module's own name, as it will identify itself to a loader.
    #[arg(long, default_value = "obscene")]
    module_name: String,
    /// Which console generation this module declares itself for: 5 or 4.
    ///
    /// Written into `e_ident[EI_ABIVERSION]` as 2 or 0. The loaders disagree and all
    /// of them are right to: Kyty and craziiEmu read 2 as the current generation, and
    /// shadPS4 is a previous-generation emulator that refuses anything but 0.
    ///
    /// Defaults to 5, because that is what this probe is for. Build with `GEN=4` to
    /// run it under a previous-generation emulator - which is worth doing, since the
    /// most complete one available is one.
    #[arg(long, default_value_t = 5, value_parser = clap::value_parser!(u8).range(4..=5))]
    generation: u8,

    /// Which dynamic-table convention to write: `legacy` or `current`.
    ///
    /// `legacy` spends a vendor tag on every table. It is what this module has always
    /// written and what every loader in the toolkit accepts - three of them being
    /// previous-generation emulators where it *is* the convention.
    ///
    /// `current` uses the standard ELF tags for the standard tables and the high vendor
    /// range for the rest, which is what all six retail current-generation dumps use. Under
    /// `legacy`, prosper's reader finds **none** of this module's imports; the newer form is
    /// the reason to have the switch at all. (D193)
    ///
    /// It changes the **layout** as well as the numbers, because the two go together: under
    /// `current` the tables are placed in a mapped `PT_LOAD` past the last one and the tags
    /// hold virtual addresses, where under `legacy` they sit in `PT_SCE_DYNLIBDATA` at
    /// vaddr 0 and the tags hold offsets into it. prosper reads 35,518 imports from the
    /// former and none from the latter. See `docs/MODULE-FORMAT.md`.
    ///
    /// **Pick it by generation, not by preference** - which is why the Makefile derives it
    /// from `GEN` and you should not normally pass this by hand. The previous-generation
    /// emulators reject the standard tags outright (`unsupported dynamic tag 0x02`), so
    /// `current` is not a strictly better module, it is a different platform's module.
    #[arg(long, default_value = "legacy")]
    table: String,

    /// Which kind of object this is: `executable`, `fixed` or `shared`.
    ///
    /// Three `e_type` values, and a console's `rtld` refuses each of the wrong two **by name**,
    /// which is the only reason this is knowable:
    ///
    /// ```text
    /// executable  0xFE10  ERROR _exec_self_imgact:1375: Unsupported ELF e_type. eboot.bin fe10
    /// fixed       0xFE00  what the eboot inside a real package carries, measured
    /// shared      0xFE18  ERROR self_load_shared_object:2816: Unsupported ELF e_type. x.prx fe10
    /// ```
    ///
    /// `executable` is the default and is what every emulator and `elfldr` accept, so the module
    /// targets are unchanged. The `eboot` targets pass `fixed`, because an eboot in a package is
    /// executed rather than mapped. A `.prx` a title bundles passes `shared`, because it is
    /// loaded *by* the executable rather than being one.
    ///
    /// This was a `--fixed` flag, which is two states for three cases. The third was not
    /// foreseen and a bool could not express it. (D221)
    #[arg(long, default_value = "executable")]
    kind: String,
}

/// Arguments for `drive`.
///
/// A struct rather than inline fields, matching `MkmoduleArgs`: enough options that the
/// match arm would be the longest in the dispatcher, and clippy is right that a dispatcher
/// arm which is a whole procedure stops the dispatcher reading as a table.
#[derive(clap::Args)]
pub struct DriveArgs {
    /// Where the probe is listening, as `host:port`. The probe listens and this connects:
    /// a console has no DNS and no configuration file, but it has an address somebody can
    /// read off a screen.
    #[arg(long, conflicts_with = "replay")]
    address: Option<String>,
    /// Read a captured transcript instead of connecting to anything.
    #[arg(long)]
    replay: Option<PathBuf>,
    /// A command to send, repeatable, in order: the verb and its arguments only.
    ///
    /// The `CMD|` prefix and the sequence number are supplied by the driver, so a caller
    /// writes `--command "resolve|libkernel|sceKernelWrite"`. Sequences must strictly
    /// increase, and a caller that never states one cannot get that wrong - which also
    /// keeps the number this end matches a timeout against the same number that went over
    /// the wire.
    #[arg(long = "command")]
    commands: Vec<String>,
    /// How long to wait for one command before recording a timeout, in seconds.
    ///
    /// Running out produces `timeout`, never `died`. The connection is still open, so this
    /// end cannot tell a blocked call from a dead process, and the record says which was
    /// observed rather than which was guessed.
    #[arg(long, default_value = "30")]
    budget: u64,
    /// Where to write the corpus records. Standard output when absent.
    #[arg(long)]
    out: Option<PathBuf>,
    /// Operator-asserted machine identity, `key=value`, repeatable.
    ///
    /// This is where the corpus gets a *trustworthy* origin. The probe reports what it can
    /// observe about itself, but it cannot certify its own machine - inside an emulator its
    /// system calls answer as the emulator, so a self-reported firmware is not a
    /// measurement of any hardware. The operator knows what the run was pointed at and says
    /// so here: `--part target=prospero --part firmware=13.520.001`. These win over anything
    /// the probe reported under the same key, and land on every corpus line so a consumer
    /// can grade by machine. See docs/OUTPUT.md.
    #[arg(long = "part")]
    part: Vec<String>,
}

#[derive(Subcommand)]
enum Command {
    /// Show the NID and encoded symbol name for a function.
    Nid {
        /// The symbol, exactly as the platform spells it.
        name: String,
        /// Read the hash suffix from here instead of the committed data file.
        #[arg(long)]
        suffix_file: Option<PathBuf>,
        /// Library identifier to embed in the symbol name.
        ///
        /// Sixteen bits, because that is the width the packed table entries carry it in.
        #[arg(long, default_value_t = 0)]
        library: u16,
        /// Module identifier to embed in the symbol name.
        #[arg(long, default_value_t = 0)]
        module: u16,
    },
    /// Recover the value behind an encoded NID.
    ///
    /// The inverse of the encoding, not of the hash - a NID cannot be turned back into
    /// a name. Useful for reading a real module's symbol table, where the names are
    /// already encoded.
    Decode {
        /// The eleven-character encoded form, with or without the `#lib#mod` suffix.
        encoded: String,
    },
    /// Turn a linked ELF into a module a console loader will run.
    ///
    /// Fixes the header and builds the vendor dynamic segment. Both, always: a loader
    /// ignores every standard dynamic tag, so a module without the vendor segment loads
    /// and resolves nothing. That used to be behind a flag, from when it was an
    /// experiment; keeping the flag would only preserve the ability to build something
    /// broken.
    ///
    /// Idempotent, so it is safe in a build rule that runs repeatedly.
    Mkmodule(MkmoduleArgs),
    /// Wrap a module in the container the console's own loader accepts.
    ///
    /// `mkmodule` builds the payload; this builds what goes around it, and the difference
    /// decides which loader runs the program. A bare ELF reaches a homebrew loader that maps
    /// the segments itself and can answer nothing about the platform; only this shape puts
    /// the platform's own loader in the path. (D180)
    ///
    /// Fake, and it says so in a field rather than pretending: `ex_info.ptype` carries the
    /// value meaning fake, and every digest and the signature area are zero.
    Mkself {
        /// The module to wrap, as produced by `mkmodule`.
        file: PathBuf,
        /// Console generation: 5 for the current one, 4 for the previous. Decides the magic.
        #[arg(long, default_value_t = 5)]
        generation: u8,
        /// Where to write the container. Defaults to `eboot.bin` beside the input.
        #[arg(long)]
        out: Option<PathBuf>,
    },
    /// List what a module imports.
    ///
    /// Every undefined dynamic symbol is a function the loader must resolve. An empty
    /// list means the platform calls were resolved at link time or optimised away, and
    /// the module would test nothing - so that is an error rather than a result.
    Imports {
        /// The module to read.
        file: PathBuf,
    },
    /// Show a module's structure: segments, and its dynamic table.
    ///
    /// On a vendor-format module this is how the unassigned dynamic tags get assigned:
    /// each value's shape gives away its role.
    Inspect {
        /// The module to read.
        file: PathBuf,
    },
    /// Re-derive the vendor dynamic tag assignment from a module, and check it.
    ///
    /// The fourteen tag values are not documented anywhere this project may read; they
    /// were worked out by arithmetic on a reference module's layout. That reference is
    /// gone. This re-runs the reasoning against any module laid out the same way -
    /// including the ones we build, which is what keeps the derivation exercised rather
    /// than merely written down.
    ///
    /// Exits non-zero if a relation does not hold.
    Derive {
        /// The module to derive from.
        file: PathBuf,
    },
    /// Resolve a module's exports to a `name vaddr` table, names from the mined corpus.
    ///
    /// The vaddr is measured from the file; the name is looked up by the NID the export table
    /// stores, because the table holds the hash, not the name. An export whose NID the corpus
    /// does not know keeps its encoded NID. The output is the address table a loader places its
    /// reimplementations by - a payload reaching `base + vaddr` lands on the named function.
    Vaddrs {
        /// The module to read (a signed container or a bare ELF).
        module: PathBuf,
        /// The corpus to resolve names against.
        #[arg(long, default_value = "data/mined-names.txt")]
        corpus: PathBuf,
        /// The provenance label for the header, e.g. "12.40 `libkernel_sys.sprx`".
        #[arg(long, default_value = "unknown module")]
        source: String,
    },
    /// Recover names from NIDs by hashing candidates and matching.
    ///
    /// A NID cannot be turned back into a name - hashing is one way - so the only route
    /// is to guess. This does the guessing part fast and exactly; supplying good
    /// candidates is the actual problem, and belongs outside the tool.
    ///
    /// A match is proof. A miss says only that the candidate list did not contain the
    /// name, never that no such name exists, and the summary reports how many were tried
    /// so a reader can weigh that.
    Crack(CrackArgs),
    /// Compare reports from several implementations and print only the disagreements.
    ///
    /// A substitute oracle for a project with no hardware: several independent
    /// implementations agreeing is evidence, and "you are the only one of four that fails
    /// this" is actionable in a way "you failed check X" is not.
    ///
    /// Names every implementation rather than counting them. These projects read each
    /// other's source, so agreement between them is not four witnesses, and a bare count
    /// would hide that.
    Consensus {
        /// Reports to compare, as `name=path`.
        ///
        /// The name appears verbatim in the output. `shadps4=reports/shadps4.txt`.
        #[arg(required = true)]
        reports: Vec<String>,
    },
    /// Check a report against the format contract.
    ///
    /// Says nothing about whether the checks passed - an all-red report is perfectly
    /// well-formed, and on a host build that is the expected outcome.
    Verify {
        /// The report to check.
        file: PathBuf,
    },
    /// Compare two reports and say what changed.
    ///
    /// Exits 1 on regression, where a regression means a check that got *worse* - not
    /// one that is failing.
    Diff {
        /// The earlier report.
        before: PathBuf,
        /// The later report.
        after: PathBuf,
    },
    /// Compare two GPU observation corpora lane by lane, and print where they diverge.
    ///
    /// The point of the GPU probe: run the same kernels on real RDNA2 and on an emulator (or
    /// an exact reference), and the lanes whose output bits differ are the shader gaps, named
    /// by the operands that produced them. Both sides' device provenance is printed first, so
    /// a cross-device diff (the gap) is never confused with a same-device one (nondeterminism).
    ///
    /// Exits 1 when anything diverges, so it works as a gate: "does this device still match
    /// the golden".
    Gpudiff {
        /// The reference corpus - a committed golden, or the first of two runs.
        golden: PathBuf,
        /// The corpus to check against it.
        fresh: PathBuf,
    },
    /// Compute a near-exact reference corpus from the inputs in a GPU corpus.
    ///
    /// Reads the operands out of a corpus and prints what each kernel *should* compute -
    /// host libm through f64, rounded to f32 - as a corpus in the same format. Diff a device
    /// corpus against it (`gpudiff device reference`) to see, per operand, where the device
    /// is approximate. The exact operations must match bit for bit; the transcendentals are
    /// measured against a strong baseline, not a claim of correct rounding. Prints to stdout.
    Gpuref {
        /// A corpus whose inputs to compute references for (a device run, or a golden).
        corpus: PathBuf,
    },
    /// Report per-kernel ULP distance between a device corpus and a reference.
    ///
    /// Where gpudiff answers "does it match", this answers "by how far" - the number that
    /// matters for the transcendentals, which are allowed to be approximate. Prints a table,
    /// worst kernel first: lanes compared, exact, diverged, and the max and mean ULP error,
    /// with the worst lane as an example. Pair the device corpus with a `gpuref` reference to
    /// read the approximation map; the exact operations should show zero.
    Gpustats {
        /// The device corpus (a run, or a golden).
        device: PathBuf,
        /// The reference to measure against (usually a `gpuref` output).
        reference: PathBuf,
    },
    /// Render a report in colour, grouped by section.
    Pretty {
        /// The report to render, or stdin when omitted.
        file: Option<PathBuf>,
        /// Force colour on or off instead of detecting a terminal.
        #[arg(long)]
        colour: Option<bool>,
    },
    /// Capture obSCEne's report from the console system log.
    ///
    /// The probe writes every record to the system log unconditionally, because that channel
    /// leaves the title's sandbox as it goes (D233) - the file sink it also writes lands locked
    /// *inside* the sandbox, where ftpsrv and shsrv cannot read it. So the system log is the
    /// readable path for a packaged run: this listens to it, keeps every `OBS|` record, writes
    /// them to a file for other tools, and prints the run's outcome. The records only flow while
    /// the title runs - launch it inside the window, or use `./bin/obscene deploy`, which
    /// launches and captures in one. This is what `hw logs` is for, minus the system's own noise.
    Report {
        /// How long to listen.
        #[arg(long, default_value_t = 120)]
        seconds: u64,
        /// Where to write the captured records.
        #[arg(long, default_value = "reports/hardware/console-klog.txt")]
        into: PathBuf,
        /// Which console, when several are registered.
        #[arg(long)]
        name: Option<String>,
    },
    /// Build a minimal module that asks a loader about one dynamic tag.
    ///
    /// A loader reports the tags it does not recognise. Run one of these and read its
    /// output: silence about the candidate, with the controls still reported, means it
    /// recognised the tag. Supply a value to chase a named failure instead.
    Probe {
        /// The candidate tag, in hex.
        tag: String,
        /// Where to write the probe module.
        out: PathBuf,
        /// Value to give the tag. Zero keeps a recognised tag harmless.
        #[arg(long, default_value = "0")]
        value: String,
    },
    /// Build a module that imports nothing and returns immediately.
    ///
    /// The control for every other loading question. If this runs, everything except
    /// import resolution is known good; if it does not, the problem is more basic than
    /// whatever was being investigated.
    Minimal {
        /// Where to write it.
        out: PathBuf,
        /// Spin forever instead of returning.
        ///
        /// A hang proves the guest's instructions are executing. A crash after `ret`
        /// does not: returning with nothing on the stack faults, which looks the same
        /// as never having run.
        #[arg(long)]
        spin: bool,
        /// Include a vendor process-parameter segment.
        ///
        /// Real modules carry one and ours never have. Compare a run with and without
        /// to find out whether a loader needs it.
        #[arg(long)]
        proc_param: bool,
    },
    /// Drive a probe over the command protocol, and write what it said as corpus records.
    ///
    /// The other end of `docs/PROTOCOL.md`. Half the protocol's meaning is established
    /// here rather than by the probe: a command that faults leaves an acknowledgement with
    /// nothing after it, and turning that into `died` - never into `returned 0` - is this
    /// side's job, because the probe is gone and cannot do it.
    ///
    /// With `--replay` it reads a captured transcript instead of connecting, which is how
    /// it is exercised with no hardware attached. That is the reason the specification
    /// shipped with transcripts rather than only prose.
    Drive(DriveArgs),
    /// Check the NID chain against its published pair.
    ///
    /// Verifies the suffix, byte order, alphabet and bit packing together. Each is
    /// individually plausible when wrong, so they are checked as one.
    /// Update or check the generated counts in the documentation
    Counts {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Rewrite the marked regions in place
        #[arg(long)]
        write: bool,
        /// Exit non-zero if any region is out of date
        #[arg(long)]
        check: bool,
    },
    /// Register a console and ask what it can currently answer
    Hw {
        /// register | list | check | logs | send | sh | ls | pull | push | install | launch
        action: String,
        /// Address for `register`; an ELF for `send`; a command for `sh`; a console path for
        /// `ls`/`pull`; a local file for `push` (`--into` is the remote path); a `.pkg` for
        /// `install`; a title id for `launch`
        address: Option<String>,
        /// Which console, when several are registered
        #[arg(long)]
        name: Option<String>,
        /// How long to listen, for `logs`
        #[arg(long, default_value_t = 10)]
        seconds: u64,
        /// Where `pull` writes what it fetched
        #[arg(long, default_value = "console-report.txt")]
        into: PathBuf,
    },
    /// Build the per-loader results table from reports
    Compat {
        /// Reports to tabulate, as `name=path`
        reports: Vec<String>,
        /// The document holding the marked region
        #[arg(long, default_value = "docs/COMPATIBILITY.md")]
        into: PathBuf,
        /// Rewrite the region in place
        #[arg(long)]
        write: bool,
        /// Exit non-zero if the region has drifted
        #[arg(long)]
        check: bool,
    },
    /// Regenerate or check a generated census header
    Census {
        /// Which header: corpus or nids
        which: String,
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Include libSce*/libkernel*/libc* as well as libraries already used
        #[arg(long)]
        platform: bool,
        /// Include every library with an attribution
        #[arg(long)]
        all_libraries: bool,
        /// Exit non-zero if the header has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Translate a loader's unresolved-import log into names
    Unresolved {
        /// Log files to read
        logs: Vec<PathBuf>,
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Where the emulator checkouts live
        #[arg(long, env = "OBSCENE_EMULATORS", default_value = "../../emulators/src")]
        emulators: PathBuf,
    },
    /// What the emulators implement that obSCEne does not reach
    Gap {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Where the emulator checkouts live
        #[arg(long, env = "OBSCENE_EMULATORS", default_value = "../../emulators/src")]
        emulators: PathBuf,
        /// Count only the current-generation emulators
        #[arg(long)]
        current: bool,
    },
    /// Mine every emulator table into data/mined-names.txt and data/unnamed-nids.txt
    Mine {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Where the emulator checkouts live
        #[arg(long, env = "OBSCENE_EMULATORS", default_value = "../../emulators/src")]
        emulators: PathBuf,
        /// Where the extracted firmware trees live
        #[arg(long, default_value = "fw")]
        firmware: PathBuf,
    },
    /// Regenerate or check the GPU ISA surface census
    Gpusurface {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// LLVM's IntrinsicsAMDGPU.td, when installed
        #[arg(
            long,
            default_value = "/usr/lib/llvm-18/include/llvm/IR/IntrinsicsAMDGPU.td"
        )]
        td: PathBuf,
        /// Write `docs/GPU_SURFACE.md`
        #[arg(long)]
        write: bool,
        /// Exit non-zero if the document has drifted
        #[arg(long)]
        check: bool,
    },
    /// Regenerate or check the bitmap font from its art
    Font {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Exit non-zero if the table has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Regenerate or check the curated census header
    Surface {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Exit non-zero if the header has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Compile the compute shaders and embed them as SPIR-V
    Shaders {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Exit non-zero if the header has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Project selfish's SELF header table into `include/obscene/self_header.gen.h`
    Selfheader {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Exit non-zero if the header has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Check the mined corpus against the checkouts it was mined from
    Corpus {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Where the emulator checkouts live
        #[arg(long, env = "OBSCENE_EMULATORS", default_value = "../../emulators/src")]
        emulators: PathBuf,
        /// Where the extracted firmware trees live
        #[arg(long, default_value = "fw")]
        firmware: PathBuf,
    },
    /// Check that prose anchored to the source still describes what the source does
    Claims {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
    },
    /// Check that every prerequisite capability is granted before it is needed
    Caps {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
    },
    /// Check that the table parser sees every check a report says ran
    Rows {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// A report from a run of the suite.
        #[arg(long)]
        report: PathBuf,
    },
    /// Regenerate or check the index at the top of the decision log
    Decisions {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Exit non-zero if the index has drifted instead of rewriting it
        #[arg(long)]
        check: bool,
    },
    /// Check the captured protocol transcripts against the specification
    Protocol {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
        /// Directory of transcripts. Defaults to docs/examples/protocol.
        #[arg(long)]
        examples: Option<PathBuf>,
        /// Corrupt each transcript in turn and prove the checker rejects it
        #[arg(long)]
        selftest: bool,
    },
    /// Check that the documentation names nothing which does not exist
    Doccheck {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
    },
    /// Check that every check guards the platform symbols it calls
    Guards {
        /// Repository root. Defaults to the current directory.
        #[arg(long)]
        root: Option<PathBuf>,
    },
    Selftest {
        /// Read the hash suffix from here instead of the committed data file.
        #[arg(long)]
        suffix_file: Option<PathBuf>,
    },
}

/// Where `crack` reads from. Its own struct so the command dispatch stays short enough
/// to read in one go.
#[derive(clap::Args)]
struct CrackArgs {
    /// Encoded NIDs to recover, one per line. `<nid>#lib#mod` is accepted, so a symbol
    /// table can be used directly.
    #[arg(long)]
    nids: PathBuf,
    /// Candidate names, one per line.
    #[arg(long)]
    words: PathBuf,
    /// Known `nid name` pairs, used to measure the candidate list rather than the
    /// platform. A list that cannot reproduce names already known is not ready to be
    /// believed about unknown ones.
    #[arg(long)]
    known: Option<PathBuf>,
    /// Read the hash suffix from here instead of the committed data file.
    #[arg(long)]
    suffix_file: Option<PathBuf>,
}

/// A published (symbol, encoded NID) pair.
///
/// The whole chain rests on this. See `nid.rs` and `ACKNOWLEDGEMENTS.md`.
const PUBLISHED_PAIR: (&str, &str) = ("sceKernelLoadStartModule", "wzvqT4UqKX8");

/// The NID self-test: prove the hash chain against its published pair.
fn run_selftest(
    suffix_file: Option<&std::path::Path>,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let bytes = suffix::read(suffix_file)?;
    let (name, expected) = PUBLISHED_PAIR;
    let got = selfish_nid::Nid::with_suffix(name, &bytes).encode();
    if got == expected {
        println!("  ok    {name} -> {expected}");
        println!("\nthe suffix, byte order, alphabet and bit packing all agree");
        Ok(ExitCode::SUCCESS)
    } else {
        println!("  FAIL  {name} -> {got}, published value is {expected}");
        println!(
            "\nOne of the suffix, byte order, alphabet or bit packing is wrong.\n\
                     They cannot be told apart from this failure alone - it constrains\n\
                     all four together, which is what makes it worth having."
        );
        Ok(ExitCode::FAILURE)
    }
}

/// Where the probe leaves its report on the hardware, and the directory it sits in.
///
/// Defaults rather than constants in the crate: `pros_link` speaks to any target and has
/// no business knowing what this project writes. (D190)
const REPORT_PATH: &str = "/data/obscene-report.txt";
/// The directory `ls` looks in when nothing else is named.
const REPORT_DIRECTORY: &str = "/data";

/// Registered consoles: what is known, and what each can answer right now.
///
/// `check` is the interesting one and it deliberately measures rather than remembers. A
/// console's capabilities are whatever payloads happen to be loaded, that set does not
/// survive a reboot, and a cached answer would be wrong in exactly the situation somebody
/// consults it. (D184)
fn run_hw(
    action: &str,
    address: Option<&str>,
    name: Option<&str>,
    seconds: u64,
    into: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    match action {
        "register" => {
            let Some(address) = address else {
                return Err("register needs an address: obscene-tool hw register <address>".into());
            };
            hardware::register(name.unwrap_or("ps5"), address)?;
            Ok(ExitCode::SUCCESS)
        }
        "list" => {
            let consoles = hardware::load()?;
            if consoles.is_empty() {
                println!("no consoles registered");
                return Ok(ExitCode::SUCCESS);
            }
            for console in &consoles {
                println!("{:<16} {}", console.name, console.address);
            }
            Ok(ExitCode::SUCCESS)
        }
        "check" => run_hw_check(name),
        "logs" => {
            use std::io::Write as _;
            let console = hardware::resolve(hardware::load()?, name)?;
            eprintln!("listening to {} for {seconds}s", console.address);
            let (_stopper, lines) = pros_link::log::follow(&pros_link::Link::to(&console.address))?;
            let deadline = deadline_in(seconds);
            for line in lines {
                match line {
                    Ok(l) => {
                        println!("{l}");
                        let _ = std::io::stdout().flush();
                    }
                    Err(_) => break,
                }
                if std::time::Instant::now() >= deadline {
                    break;
                }
            }
            Ok(ExitCode::SUCCESS)
        }
        "send" => run_hw_send(name, address, seconds),
        "sh" => {
            let Some(command) = address else {
                return Err("sh needs a command: obscene-tool hw sh \"ps\"".into());
            };
            let console = hardware::resolve(hardware::load()?, name)?;
            let out = pros_link::shell::run(
                &pros_link::Link::to(&console.address),
                command,
                Duration::from_millis(1200),
            )?;
            if out.trim().is_empty() {
                println!("no output - is shsrv loaded? `obscene-tool hw check` will say");
            } else {
                print!("{out}");
            }
            Ok(ExitCode::SUCCESS)
        }
        "ls" => run_hw_ls(name, address.unwrap_or(REPORT_DIRECTORY)),
        "pull" => run_hw_pull(name, address.unwrap_or(REPORT_PATH), into),
        // The deploy path: put a file on the console, install a package, launch an app. Every
        // step is an existing prosperous call - `files::store` (FTP STOR), `shell::run` (shsrv).
        // This is the "real way" to a foreground context, and it runs the moment `make pkg`
        // produces an installable package (which is selfish's format work, assumed here).
        "push" => run_hw_push(name, address, into),
        // Install a package, over HTTP, because that is what the installer takes.
        //
        // `pkg_install` wants a **url**: a bare path and `file://` both return an empty content
        // id and install nothing, which reads like a rejected package and is not one. The target
        // fetches with `Range` requests and grows the window, so the server has to honour them -
        // serving the whole file with `200` makes it retry the first chunk and give up part way.
        //
        // None of that is this project's to implement: `pros_core::handover` does it, binds to
        // the interface the target can actually reach, and stops when the file has been taken.
        // (prosperous D021)
        "install" => run_hw_install(name, address, seconds),
        // The native counterpart to `install`: a native title is a *directory* (eboot.bin plus
        // sce_sys/{param.json,icon0.png}), so deploying it is a directory upload, not a package
        // fetch. It lands in a scan root (default /user/data) where an auto-mounter registers it -
        // not /user/app, which is never scanned. Every byte goes through prosperous's
        // transfer::upload, the same call `pros-cli restore` makes. (D291)
        "install-native" => run_hw_install_native(name, address, into),
        // Launch an installed app by its title id, into the foreground - which owns the display
        // and a user session, the two things an elfldr-injected background payload never gets.
        "launch" => run_hw_launch(name, address, seconds),
        // Install and launch holding one server open across both, which is the only way the
        // launch can work.
        //
        // An app installed from a package is registered as a *download stub*: its `app.json`
        // records the URL it was fetched from, and the launcher re-fetches the image from that
        // URL when it mounts `/app0`. `handover` stops serving the instant the install has taken
        // the file (its `Drop`), so `install` then a separate `launch` finds a dead URL and the
        // mount fails - `mountApp0Dir 0x80020002`, `launchApp 0x80020002`, observed on hardware.
        // One `Handover`, alive across the install, the launch, and a settle window after it,
        // lets the mount's range fetches land on a server that is still there.
        "deploy" => run_hw_deploy(name, address, into, seconds),
        other => Err(format!(
            "unknown action `{other}`: try register, list, check, logs, send, sh, ls, pull, \
             push, install, install-native, launch or deploy"
        )
        .into()),
    }
}

/// List a directory on the console.
fn run_hw_ls(name: Option<&str>, path: &str) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let console = hardware::resolve(hardware::load()?, name)?;
    for entry in pros_link::files::list(&pros_link::Link::to(&console.address), path)? {
        if entry.is_usable() {
            let size = entry.size.map_or_else(|| "-".to_owned(), |n| n.to_string());
            println!("  {size:>10}  {}", entry.name);
        } else {
            // Shown, not dropped. A listing that hides the lines it could not read says a
            // directory is emptier than it is. (D189)
            println!("  {:>10}  ? {}", "", entry.raw);
        }
    }
    Ok(ExitCode::SUCCESS)
}

/// Fetch a file off the console and write it here.
///
/// Written by this program rather than redirected by the shell, deliberately: a redirect
/// on Windows decides the encoding itself and can leave a byte-order mark at the front of
/// a report every parser then has to cope with. (D190)
fn run_hw_pull(
    name: Option<&str>,
    path: &str,
    into: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let console = hardware::resolve(hardware::load()?, name)?;
    let bytes = pros_link::files::retrieve(&pros_link::Link::to(&console.address), path)?;
    std::fs::write(into, &bytes)?;
    println!("{} bytes from {path} -> {}", bytes.len(), into.display());
    println!("  obscene-tool verify {}", into.display());
    Ok(ExitCode::SUCCESS)
}

/// Probe every known service and say what the console can do about it.
fn run_hw_check(name: Option<&str>) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let console = hardware::resolve(hardware::load()?, name)?;
    println!("{} ({})", console.name, console.address);
    let timeout = Duration::from_millis(1500);
    let mut missing_required = 0_u32;
    let mut missing_optional = 0_u32;
    for service in pros_link::SERVICES {
        let seen = pros_link::probe(&console.address, service.port, timeout);
        // A slow answer is worth saying out loud: it separates "the console is busy or the
        // network is poor" from "that payload is not loaded", which look identical in a
        // column of up and down.
        let slow = if seen.took > Duration::from_millis(400) {
            format!("  ({}ms)", seen.took.as_millis())
        } else {
            String::new()
        };
        let mark = if seen.open {
            "up  "
        } else if service.required {
            missing_required = missing_required.saturating_add(1);
            "DOWN"
        } else {
            missing_optional = missing_optional.saturating_add(1);
            "--  "
        };
        println!(
            "  {mark} {:<9} :{:<5} {}{}",
            service.name, service.port, service.unlocks, slow
        );
    }
    println!();
    if missing_required > 0 {
        println!("a required service is down; the console cannot be given a payload");
        // Not an error exit: "the console is off" is an answer, and a caller scripting
        // around this wants to branch on it rather than trap it.
    } else if missing_optional > 0 {
        println!("usable, but something will be invisible if a run goes wrong");
    } else {
        println!("everything this project knows how to use is answering");
    }
    Ok(ExitCode::SUCCESS)
}

/// The compatibility-table gate. Reports are named `label=path`.
fn run_compat(
    reports: &[String],
    into: &std::path::Path,
    write: bool,
    check: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let mut named = Vec::new();
    for spec in reports {
        let Some((label, path)) = spec.split_once('=') else {
            return Err(format!("expected name=path, got `{spec}`").into());
        };
        named.push((label.to_owned(), compat::parse(std::path::Path::new(path))?));
    }
    if compat::run(into, &named, write, check)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// A generated census header, regenerated or gated against drift.
fn run_census(
    which: &str,
    root: Option<&std::path::Path>,
    platform: bool,
    all_libraries: bool,
    check: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    let target = match which {
        "corpus" => census::Which::Corpus,
        "nids" => census::Which::Nids,
        other => return Err(format!("unknown census `{other}`; expected corpus or nids").into()),
    };
    let scope = if all_libraries {
        census::Scope::All
    } else if platform {
        census::Scope::Platform
    } else {
        census::Scope::Used
    };
    if census::run(root, target, scope, check)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// Mine every emulator table and write the corpus files.
fn run_mine(
    root: Option<&std::path::Path>,
    emulators: &std::path::Path,
    firmware: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    mining::write(root, emulators, firmware)?;
    Ok(ExitCode::SUCCESS)
}

/// The corpus-provenance gate: has the mined data seen the current checkouts?
fn run_corpus(
    root: Option<&std::path::Path>,
    emulators: &std::path::Path,
    firmware: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    if corpus::check(root, emulators, firmware)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The parser-against-reality gate.
fn run_rows(
    root: Option<&std::path::Path>,
    report: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    if rows::run(root, report)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The anchored-prose gate.
fn run_claims(root: Option<&std::path::Path>) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    if claims::run(root)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The capability-ordering gate.
fn run_caps(root: Option<&std::path::Path>) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    if caps::run(root)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The decision-log index: generated from the entries, gated against drift.
fn run_decisions(
    root: Option<&std::path::Path>,
    check: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    let path = root.join("docs").join("DECISIONS.md");
    if decisions::run(&path, check)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The protocol gate: every transcript against the written specification.
fn run_protocol(
    root: Option<&std::path::Path>,
    examples: Option<&std::path::Path>,
    selftest: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    let default = root.join("docs").join("examples").join("protocol");
    let examples = examples.unwrap_or(&default);
    let spec = root.join("docs").join("PROTOCOL.md");
    if selftest {
        let (caught, missed) = protocol::selftest(examples, &spec)?;
        if missed.is_empty() {
            println!("the checker rejected all {caught} corruptions");
            return Ok(ExitCode::SUCCESS);
        }
        println!("the checker accepted transcripts it should have rejected:");
        for line in &missed {
            println!("  {line}");
        }
        return Ok(ExitCode::FAILURE);
    }
    let (problems, count) = protocol::run(examples, &spec)?;
    if problems.is_empty() {
        println!("{count} protocol transcripts match the specification");
        return Ok(ExitCode::SUCCESS);
    }
    println!("the captured exchanges disagree with the specification:");
    for line in &problems {
        println!("  {line}");
    }
    Ok(ExitCode::FAILURE)
}

/// The documentation-reference gate. Exits non-zero when prose names something missing.
fn run_doccheck(root: Option<&std::path::Path>) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    let problems = doccheck::run(root)?;
    if problems.is_empty() {
        println!("documentation references resolve");
        return Ok(ExitCode::SUCCESS);
    }
    println!("documentation names things that do not exist:");
    for line in &problems {
        println!("  {line}");
    }
    Ok(ExitCode::FAILURE)
}

/// The generated-count gate. Exits non-zero when a marked region has drifted.
fn run_counts(
    root: Option<&std::path::Path>,
    write: bool,
    check: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    if counts::run(root, write, check)? {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// The cross-symbol guard gate. Exits non-zero when any check calls an unguarded symbol.
fn run_guards(root: Option<&std::path::Path>) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let root = root.unwrap_or_else(|| std::path::Path::new("."));
    let imports = std::fs::read_to_string(sections::find_file(root, "imports.c"))?;
    let (problems, total) = guards::scan(&sections::find_dir(root, "sections"), &imports)?;
    if problems.is_empty() {
        println!("all {total} checks guard every platform symbol they call");
        return Ok(ExitCode::SUCCESS);
    }
    println!(
        "{} of {total} checks call an unguarded platform symbol
",
        problems.len()
    );
    for problem in &problems {
        println!("  {}", problem.check_id);
        println!("    announces {}", problem.declared);
        println!("    also calls {}", problem.also_calls.join(", "));
    }
    println!(
        "
See CLAUDE.md and D058. The fix is to test the address and obs_skip."
    );
    Ok(ExitCode::FAILURE)
}

/// A deadline `secs` from now.
///
/// `Instant + Duration` panics on overflow, which `arithmetic_side_effects` reports and this
/// crate treats as a defect rather than a nuisance: the lint is set to warn on purpose because
/// "arithmetic on offsets and sizes is the whole job here". Three callers wrote the same
/// addition, so the check lives in one place instead of three.
fn deadline_in(secs: u64) -> std::time::Instant {
    std::time::Instant::now()
        .checked_add(Duration::from_secs(secs))
        .expect("a deadline this close to now fits in an Instant")
}
fn main() -> ExitCode {
    // Held for the whole of `main`: the guard is what keeps the writers alive, and `let _`
    // would drop it here.
    let _logging = oops_log::Logging::new("obscene-tool")
        .build(oops_build::line!())
        .init();
    let cli = Cli::parse();
    match run(cli) {
        Ok(code) => code,
        Err(error) => {
            eprintln!("error: {error}");
            ExitCode::FAILURE
        }
    }
}

/// The generators: everything that writes a committed artefact from a data file.
///
/// Split from [`run_gate`] for the same reason that was split from [`run`] - the match
/// grew past the length lint with each port - and because "regenerates something" and
/// "checks something" are different jobs even where the same subcommand does both.
fn run_generator(command: Command) -> Result<ExitCode, Box<dyn std::error::Error>> {
    match command {
        Command::Font { root, check } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if font::run(&root, check)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Surface { root, check } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if surface::run(&root, check)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Shaders { root, check } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if shaders::run(&root, check)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Census {
            which,
            root,
            platform,
            all_libraries,
            check,
        } => run_census(&which, root.as_deref(), platform, all_libraries, check),
        Command::Gpusurface {
            root,
            td,
            write,
            check,
        } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if gpusurface::run(&root, &td, write, check)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Selfheader { root, check } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if selfheader::run(&root, check)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        _ => unreachable!("not a generator"),
    }
}

/// The gates: the checks `verify.sh` and CI run.
///
/// Split from [`run`] because the match grew past the function-length lint with every
/// checker ported out of Python, and shaving one line each time is not a design. These are
/// also the arms a reader looks for together - they are what the build has to pass.
fn run_gate(command: Command) -> Result<ExitCode, Box<dyn std::error::Error>> {
    match command {
        Command::Counts { root, write, check } => run_counts(root.as_deref(), write, check),
        Command::Compat {
            reports,
            into,
            write,
            check,
        } => run_compat(&reports, &into, write, check),
        Command::Hw {
            action,
            address,
            name,
            seconds,
            into,
        } => run_hw(&action, address.as_deref(), name.as_deref(), seconds, &into),
        Command::Unresolved {
            logs,
            root,
            emulators,
        } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            if unresolved::run(&root, &emulators, &logs)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Gap {
            root,
            emulators,
            current,
        } => {
            let root = root.unwrap_or_else(|| PathBuf::from("."));
            let scope = if current {
                gap::Scope::CurrentGeneration
            } else {
                gap::Scope::All
            };
            if gap::run(&root, &emulators, scope)? {
                Ok(ExitCode::SUCCESS)
            } else {
                Ok(ExitCode::FAILURE)
            }
        }
        Command::Mine {
            root,
            emulators,
            firmware,
        } => run_mine(root.as_deref(), &emulators, &firmware),
        Command::Corpus {
            root,
            emulators,
            firmware,
        } => run_corpus(root.as_deref(), &emulators, &firmware),
        Command::Caps { root } => run_caps(root.as_deref()),
        Command::Claims { root } => run_claims(root.as_deref()),
        Command::Rows { root, report } => run_rows(root.as_deref(), &report),
        Command::Decisions { root, check } => run_decisions(root.as_deref(), check),
        Command::Protocol {
            root,
            examples,
            selftest,
        } => run_protocol(root.as_deref(), examples.as_deref(), selftest),
        Command::Doccheck { root } => run_doccheck(root.as_deref()),
        Command::Guards { root } => run_guards(root.as_deref()),
        Command::Selftest { suffix_file } => run_selftest(suffix_file.as_deref()),
        // Everything else is handled by `run`, which dispatches here only for the gates.
        other @ (Command::Font { .. }
        | Command::Surface { .. }
        | Command::Shaders { .. }
        | Command::Census { .. }
        | Command::Gpusurface { .. }
        | Command::Selfheader { .. }) => run_generator(other),
        _ => unreachable!("run dispatches only the gate arms here"),
    }
}

fn run(cli: Cli) -> Result<ExitCode, Box<dyn std::error::Error>> {
    match cli.command {
        Command::Nid {
            name,
            suffix_file,
            library,
            module,
        } => {
            let bytes = suffix::read(suffix_file.as_deref())?;
            let value = selfish_nid::Nid::with_suffix(&name, &bytes);
            println!("{name}");
            println!("  nid     {:#018x}", value.value());
            println!("  encoded {}", value.encode());
            println!(
                "  symbol  {}",
                selfish_nid::symbol_name(value, library, module)
            );
            Ok(ExitCode::SUCCESS)
        }
        Command::Decode { encoded } => {
            // A real symbol name carries the library and module after the NID; accept
            // either form so this works on text copied straight out of a module.
            let head = encoded.split('#').next().unwrap_or(&encoded);
            let value = selfish_nid::Nid::decode(head)?;
            println!("{head}");
            println!("  nid  {:#018x}", value.value());
            Ok(ExitCode::SUCCESS)
        }
        Command::Vaddrs {
            module,
            corpus,
            source,
        } => {
            let (total, resolved) = vaddrs::run(&module, &corpus, &source)?;
            eprintln!("{resolved}/{total} exports resolved to names");
            Ok(ExitCode::SUCCESS)
        }
        Command::Mkmodule(args) => run_mkmodule(&args),
        Command::Mkself {
            file,
            out,
            generation,
        } => run_mkself(&file, out.as_deref(), generation),
        Command::Imports { file } => run_imports(&file),
        Command::Crack(args) => run_crack(&args),
        Command::Consensus { reports } => run_consensus(&reports),
        Command::Derive { file } => {
            let bytes = std::fs::read(&file)?;
            let parsed = elf::Elf::parse(&bytes)?;
            Ok(print_derivation(&derive::run(&parsed)))
        }
        Command::Inspect { file } => {
            let bytes = std::fs::read(&file)?;
            let parsed = elf::Elf::parse(&bytes)?;
            inspect(&parsed);
            Ok(ExitCode::SUCCESS)
        }
        Command::Probe { tag, out, value } => {
            let tag = u64::from_str_radix(tag.trim_start_matches("0x"), 16)?;
            let value = u64::from_str_radix(value.trim_start_matches("0x"), 16)?;
            std::fs::write(&out, probe::build(tag, value))?;
            println!("{}: probing {tag:#x} = {value:#x}", out.display());
            Ok(ExitCode::SUCCESS)
        }
        Command::Minimal {
            out,
            spin,
            proc_param,
        } => run_minimal(&out, spin, proc_param),
        Command::Verify { file } => run_verify(&file),
        Command::Diff { before, after } => {
            let old = report::Report::parse(&std::fs::read_to_string(&before)?);
            let new = report::Report::parse(&std::fs::read_to_string(&after)?);
            let comparison = diff::compare(&old, &new);
            print_diff(&comparison);
            if comparison.has_regression() {
                Ok(ExitCode::FAILURE)
            } else {
                Ok(ExitCode::SUCCESS)
            }
        }
        Command::Gpudiff { golden, fresh } => run_gpudiff(&golden, &fresh),
        Command::Gpuref { corpus } => run_gpuref(&corpus),
        Command::Gpustats { device, reference } => run_gpustats(&device, &reference),
        Command::Pretty { file, colour } => {
            let text = match file {
                Some(path) => std::fs::read_to_string(path)?,
                None => std::io::read_to_string(std::io::stdin())?,
            };
            // Colour only when a terminal is attached, unless told otherwise. Piping
            // into a file or an agent should produce text, not escapes to strip.
            let enabled =
                colour.unwrap_or_else(|| std::io::IsTerminal::is_terminal(&std::io::stdout()));
            let parsed = report::Report::parse(&text);
            print!("{}", pretty::render(&parsed, enabled));
            Ok(ExitCode::SUCCESS)
        }
        Command::Report {
            seconds,
            into,
            name,
        } => run_report(seconds, &into, name.as_deref()),
        Command::Drive(args) => run_drive(&args),
        other => run_gate(other),
    }
}

/// Capture obSCEne's records from the console system log into a file.
///
/// `hw logs` reads the same channel; this keeps only the `OBS|` records, so what lands on disk
/// is ready for a parser (`pretty`, `diff`, `verify`) rather than interleaved with the system's
/// own logging. It is the readable path for a packaged run: the file the probe also writes lands
/// locked inside the title's sandbox (D233), and the system log is the one channel that leaves
/// it. Extracted from the dispatcher like the other `run_*` helpers so the match stays a table.
fn run_report(
    seconds: u64,
    into: &std::path::Path,
    name: Option<&str>,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let console = hardware::resolve(hardware::load()?, name)?;
    eprintln!(
        "listening to {} for {seconds}s -> {}",
        console.address,
        into.display()
    );
    let text = pros_link::log::read(
        &pros_link::Link::to(&console.address),
        Duration::from_secs(seconds),
    )?;
    // Every line the probe emits begins `OBS|`; the system's own noise carries a `<NNN>`
    // priority prefix, so a start-anchored match on the record prefix separates them cleanly.
    let prefix = format!("{}{}", report::PREFIX, report::SEPARATOR);
    let records: Vec<&str> = text
        .lines()
        .filter(|line| line.starts_with(&prefix))
        .collect();
    if let Some(dir) = into.parent().filter(|d| !d.as_os_str().is_empty()) {
        std::fs::create_dir_all(dir)?;
    }
    let mut body = records.join("\n");
    if !body.is_empty() {
        body.push('\n');
    }
    std::fs::write(into, &body)?;
    println!(
        "captured {} OBS record(s) -> {}",
        records.len(),
        into.display()
    );
    // The lines that name the run's shape - which build wrote them, the final tally, the end
    // marker - printed so a capture says whether it is complete without opening the file.
    for line in &records {
        let kind = line.split(report::SEPARATOR).nth(1).unwrap_or_default();
        if matches!(kind, "meta" | "build" | "tally" | "end") {
            println!("  {line}");
        }
    }
    if records.is_empty() {
        println!(
            "no OBS records - was the title running during the window? \
             (`./bin/obscene deploy` launches and captures together)"
        );
    }
    Ok(ExitCode::SUCCESS)
}

/// Compares two GPU corpora and prints the divergences. Non-zero exit when any differ.
///
/// Split out of the dispatcher for the same reason as `run_drive`: an arm that is a whole
/// procedure stops the dispatcher reading as a table of what-maps-to-what.
fn run_gpudiff(
    golden: &std::path::Path,
    fresh: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let g = gpudiff::GpuCorpus::parse(&std::fs::read_to_string(golden)?);
    let f = gpudiff::GpuCorpus::parse(&std::fs::read_to_string(fresh)?);
    let cmp = gpudiff::compare(&g, &f);
    print!("{}", gpudiff::render(&cmp));
    if cmp.divergences.is_empty() {
        Ok(ExitCode::SUCCESS)
    } else {
        Ok(ExitCode::FAILURE)
    }
}

/// Prints per-kernel ULP statistics of a device corpus against a reference. Extracted from the
/// dispatcher for the same reason as the other `run_*` helpers.
fn run_gpustats(
    device: &std::path::Path,
    reference: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let d = gpudiff::GpuCorpus::parse(&std::fs::read_to_string(device)?);
    let r = gpudiff::GpuCorpus::parse(&std::fs::read_to_string(reference)?);
    let stats = gpustats::analyze(&d, &r);
    print!("{}", gpustats::render(&stats, &d.device, &r.device));
    Ok(ExitCode::SUCCESS)
}

/// Computes a reference corpus from a corpus's inputs and prints it. Extracted from the
/// dispatcher for the same reason as the other `run_*` helpers.
fn run_gpuref(corpus: &std::path::Path) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let c = gpudiff::GpuCorpus::parse(&std::fs::read_to_string(corpus)?);
    let (text, skipped) = gpuref::render_reference(&c);
    print!("{text}");
    if skipped > 0 {
        eprintln!("{skipped} lane(s) had no reference and were skipped");
    }
    Ok(ExitCode::SUCCESS)
}

/// Runs a probe session (live or replayed) and writes the corpus.
///
/// Split out of the dispatcher because it is the longest arm by some way, and a match arm
/// that is a whole procedure is where the dispatcher stops being readable as a table of
/// what-maps-to-what. Same reason `MkmoduleArgs` is a struct rather than inline fields.
fn run_drive(args: &DriveArgs) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let mut session = if let Some(path) = &args.replay {
        // A transcript that ends is a transcript, not a dropped connection. Passing `false`
        // is what keeps an unterminated command in a captured file recorded as `lost`
        // rather than as a death this end never witnessed.
        let text = std::fs::read_to_string(path)?;
        let lines: Vec<String> = text.lines().map(str::to_string).collect();
        drive::parse_session(&lines, false)
    } else {
        let Some(address) = &args.address else {
            eprintln!("drive needs either --address or --replay");
            return Ok(ExitCode::FAILURE);
        };
        drive::run_session(address, &args.commands, Duration::from_secs(args.budget))?
    };

    // Operator-asserted machine identity, merged in so it lands on every corpus line and
    // wins over anything the probe self-reported under the same key. This is the origin a
    // consumer grades by, and it is the operator's to assert because the probe cannot
    // certify its own machine (docs/OUTPUT.md).
    for entry in &args.part {
        let Some((key, value)) = entry.split_once('=') else {
            eprintln!("--part expects key=value, got {entry:?}");
            return Ok(ExitCode::FAILURE);
        };
        session.part.insert(key.to_string(), value.to_string());
    }

    let corpus = drive::to_corpus(&session);
    match &args.out {
        Some(path) => std::fs::write(path, corpus)?,
        None => print!("{corpus}"),
    }

    // Non-zero when something did not answer, so this is usable as a gate. A refusal is not
    // a failure - it is the protocol working - and neither is a command declined for want
    // of a capability, which is the driver correctly not asking a question this target
    // cannot be asked. What counts is a command that went out and never came back.
    let unanswered = session
        .exchanges
        .iter()
        .filter(|e| {
            matches!(
                e.outcome,
                drive::Outcome::Died | drive::Outcome::TimedOut | drive::Outcome::Lost
            )
        })
        .count();
    if unanswered > 0 {
        eprintln!("{unanswered} command(s) did not answer");
        return Ok(ExitCode::FAILURE);
    }
    Ok(ExitCode::SUCCESS)
}

/// Prints a module's structure, and what can be said about each dynamic tag.
fn inspect(parsed: &elf::Elf<'_>) {
    println!("e_type       {:#06x}", parsed.e_type);
    println!("entry        {:#x}", parsed.entry);
    println!("segments     {}", parsed.program_headers.len());
    for header in &parsed.program_headers {
        let label = match header.p_type {
            elf::PT_LOAD => "LOAD",
            elf::PT_DYNAMIC => "DYNAMIC",
            elf::PT_SCE_DYNLIBDATA => "SCE_DYNLIBDATA",
            _ => "",
        };
        println!(
            "  {:#010x} {:<15} offset {:#010x}  vaddr {:#012x}  filesz {:#010x}",
            header.p_type, label, header.offset, header.vaddr, header.filesz
        );
    }

    let Ok(entries) = parsed.dynamic() else {
        println!(
            "
dynamic table unreadable"
        );
        return;
    };
    if entries.is_empty() {
        return;
    }

    // Only meaningful against a vendor segment: the shapes below are all relative to
    // it, so without one there is nothing to classify a value against.
    let dynlib = parsed.segment(elf::PT_SCE_DYNLIBDATA);
    println!(
        "
dynamic table: {} entries",
        entries.len()
    );
    for entry in entries {
        let name = selfish_elf::dynamic::tag_name(entry.tag).unwrap_or("");
        let shape =
            dynlib.map_or_else(String::new, |segment| shape_of(entry.value, segment.filesz));
        let marker = if name.is_empty() && entry.tag >= u64::from(elf::PT_SCE_DYNLIBDATA) {
            "  <-- unassigned"
        } else {
            ""
        };
        println!(
            "  {:#012x} {:#018x}  {name:<22}{shape}{marker}",
            entry.tag, entry.value
        );
    }
}

/// Recovers names from NIDs, and says what the result is worth.
fn run_crack(args: &CrackArgs) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let suffix = suffix::read(args.suffix_file.as_deref())?;
    let nid_text = std::fs::read_to_string(&args.nids)?;
    let word_text = std::fs::read_to_string(&args.words)?;
    let known_pairs = match args.known.as_deref() {
        Some(path) => crack::parse_pairs(&std::fs::read_to_string(path)?),
        None => Vec::new(),
    };

    let out = crack::crack(nid_text.lines(), word_text.lines(), &known_pairs, &suffix);

    // A header, so a table pasted somewhere still says where it came from. A generated
    // table outlives the run that made it and gets trusted long after it stops being
    // right; this is the same argument as provenance on a check.
    println!("# obscene-tool crack");
    println!("# suffix     {}", selfish_nid::suffix_fingerprint(&suffix));
    println!("# candidates {}", out.tried);
    if out.known_total > 0 {
        println!(
            "# generator  reproduced {} of {} known pairs",
            out.known_hit, out.known_total
        );
    } else {
        println!("# generator  not measured - no known pairs supplied");
    }
    println!(
        "# recovered  {} of {}",
        out.found.len(),
        out.found.len().saturating_add(out.unmatched.len())
    );
    println!("#");
    println!("# A match is proof. A miss means this candidate list did not contain the");
    println!("# name - never that no such name exists.");

    for (encoded, name) in &out.found {
        println!("{encoded} {name}");
    }

    if !out.generator_is_credible() && out.known_total > 0 {
        eprintln!(
            "warning: the candidate list reproduced only {} of {} names already known,              so a miss above is at least as likely to be a gap in the candidates as a              name that does not exist",
            out.known_hit, out.known_total
        );
    }
    Ok(ExitCode::SUCCESS)
}

/// Prints a derivation and returns the exit status it implies.
fn print_derivation(derivation: &derive::Derivation) -> ExitCode {
    if derivation.not_a_module {
        println!("not a vendor-format module: no PT_SCE_DYNLIBDATA segment, or no readable");
        println!("dynamic table. There is nothing here to derive from.");
        return ExitCode::FAILURE;
    }

    println!("layout relations");
    for relation in &derivation.relations {
        let mark = if relation.skipped {
            "--  "
        } else if relation.holds {
            "ok  "
        } else {
            "FAIL"
        };
        println!(
            "  {mark} {:<34} {:#x} vs {:#x}   -> {}",
            relation.claim, relation.left, relation.right, relation.identifies
        );
    }

    for note in &derivation.ambiguous {
        println!(
            "
not established by arithmetic: {note}"
        );
    }

    if !derivation.unassigned.is_empty() {
        println!(
            "
vendor tags present with no assignment:"
        );
        for tag in &derivation.unassigned {
            println!("  {tag:#012x}");
        }
    }

    if derivation.is_consistent() {
        println!(
            "
the assignment in selfish's tag tables reproduces this module's layout."
        );
        ExitCode::SUCCESS
    } else {
        println!();
        println!("a relation failed. Either the constants are wrong or this module is not");
        println!("laid out the way we think - this cannot tell those apart, and both are");
        println!("worth stopping for.");
        ExitCode::FAILURE
    }
}

/// What a value's magnitude says about its role.
///
/// Evidence for an assignment, not the assignment itself - a tag whose role cannot be
/// pinned is left unassigned rather than guessed, because a wrong one produces a module
/// that loads and resolves nothing.
fn shape_of(value: u64, dynlib_size: u64) -> String {
    match value {
        0 => "zero".to_owned(),
        0x18 => "entry size (0x18)".to_owned(),
        7 => "relocation type 7 = RELA, so this is DT_SCE_PLTREL".to_owned(),
        v if v < dynlib_size => "offset into dynlibdata".to_owned(),
        _ => "outside dynlibdata: a size, or not an offset".to_owned(),
    }
}

/// Prints a comparison in the same shape the report itself uses.
fn print_diff(comparison: &diff::Comparison) {
    if let Some((before, after)) = &comparison.target_mismatch {
        println!("REFUSED: these are different targets ({before} vs {after}).");
        println!();
        println!("Both run the same checks, through different loaders - so a comparison");
        println!("across them measures the loader rather than any change, and every line");
        println!("of it would be noise presented as signal. Compare like with like.");
        return;
    }
    if let Some((before, after)) = &comparison.build_changed {
        println!("NOTE: different probe builds ({before} -> {after}).");
        println!("Changes below may be the probe rather than the platform.");
    }
    for change in &comparison.changed_values {
        let before = if change.before.is_empty() {
            "-"
        } else {
            &change.before
        };
        let after = if change.after.is_empty() {
            "-"
        } else {
            &change.after
        };
        println!("  value      {}: {before} -> {after}", change.id);
    }
    for transition in &comparison.improved {
        println!(
            "  improved   {}: {} -> {}",
            transition.id, transition.before, transition.after
        );
    }
    for transition in &comparison.regressed {
        println!(
            "  REGRESSED  {}: {} -> {}",
            transition.id, transition.before, transition.after
        );
    }
    for id in &comparison.vanished {
        println!("  REGRESSED  {id}: present -> missing from the report entirely");
    }
    for (library, symbol) in &comparison.now_present {
        println!("  improved   {library}:{symbol}: absent -> present");
    }
    for (library, symbol) in &comparison.now_absent {
        println!("  REGRESSED  {library}:{symbol}: present -> absent");
    }

    println!();
    println!(
        "{} improved, {} regressed, {} unchanged",
        comparison.improved.len(),
        comparison.regression_count(),
        comparison.unchanged
    );
    if !comparison.now_present.is_empty() || !comparison.now_absent.is_empty() {
        println!(
            "symbols: {} newly present, {} newly absent",
            comparison.now_present.len(),
            comparison.now_absent.len()
        );
    }
    if let Some((before, after)) = comparison.tally_delta {
        // Saturating, because these figures come from a file this tool does not
        // control and a wrapped delta prints a number that is confidently wrong.
        let delta = |a: u32, b: u32| i64::from(a).saturating_sub(i64::from(b));
        let deltas = [
            ("pass", delta(after.pass, before.pass)),
            ("partial", delta(after.partial, before.partial)),
            ("fail", delta(after.fail, before.fail)),
            ("skip", delta(after.skip, before.skip)),
        ];
        let moved: Vec<String> = deltas
            .iter()
            .filter(|(_, delta)| *delta != 0)
            .map(|(name, delta)| format!("{name} {delta:+}"))
            .collect();
        if moved.is_empty() {
            println!("tally: no change");
        } else {
            println!("tally: {}", moved.join(", "));
        }
    }
}

/// Checks a report against the format contract.
fn run_verify(file: &std::path::Path) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let text = std::fs::read_to_string(file)?;
    let problems = verify::check(&report::Report::parse(&text));
    if problems.is_empty() {
        println!("report is well-formed");
        return Ok(ExitCode::SUCCESS);
    }
    println!("report is malformed ({} problems):", problems.len());
    for problem in &problems {
        println!("  - {problem}");
    }
    Ok(ExitCode::FAILURE)
}

/// Compares reports from several implementations.
fn run_consensus(reports: &[String]) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let mut named = Vec::new();
    for spec in reports {
        // `name=path`, because which implementation said what is the entire output and
        // deriving a name from a filename would make it accidental.
        let (name, path) = spec
            .split_once('=')
            .ok_or_else(|| format!("expected name=path, got {spec}"))?;
        let text = std::fs::read_to_string(path)?;
        named.push((name.to_owned(), report::Report::parse(&text)));
    }
    if named.len() < 2 {
        return Err("consensus needs at least two reports".into());
    }

    let out = consensus::compare(&named);
    println!("# obscene-tool consensus");
    println!("# implementations {}", out.implementations.join(", "));
    println!("# agreed          {}", out.unanimous);
    println!("# disagreed       {}", out.disagreements.len());
    if out.not_comparable > 0 {
        println!(
            "# no opinion      {} (too few implementations attempted them)",
            out.not_comparable
        );
    }
    if !out.partial_coverage.is_empty() {
        println!(
            "# not in every report {} (a run that died early, not a disagreement)",
            out.partial_coverage.len()
        );
    }
    println!("#");
    println!("# These implementations are not independent of each other - they read each");
    println!("# other's source. Agreement is evidence, not four witnesses.");

    if out.disagreements.is_empty() {
        println!("\nno disagreements");
        return Ok(ExitCode::SUCCESS);
    }

    for item in &out.disagreements {
        if let Some((who, status)) = item.outlier() {
            println!("\nOUTLIER  {}", item.check);
            println!("  {who} alone says {status}");
        } else {
            println!("\nSPLIT    {}", item.check);
        }
        for (status, who) in &item.by_status {
            println!("    {status:<8} {}", who.join(", "));
        }
    }
    // Success: disagreements are the output, not an error. A non-zero exit would make
    // this unusable in a pipeline that expects to find some.
    Ok(ExitCode::SUCCESS)
}

/// Applies the header fixups a console loader requires.
/// Wrap a module in a fake container.
///
/// Reports what it built rather than only that it built: the entry count is twice the
/// number of qualifying segments, and that doubling is the single thing most likely to be
/// wrong, so it is printed where a reader will see it.
fn run_mkself(
    file: &std::path::Path,
    out: Option<&std::path::Path>,
    generation: u8,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let payload = std::fs::read(file)?;
    let generation = if generation == 5 {
        selfish_abi::Generation::Current
    } else {
        selfish_abi::Generation::Previous
    };
    let container = selfish_container::build(&payload, generation)?;
    let target = match out {
        Some(p) => p.to_path_buf(),
        None => file.with_file_name("eboot.bin"),
    };
    std::fs::write(&target, &container)?;
    println!(
        "{}: {} bytes from a {} byte payload, {generation}",
        target.display(),
        container.len(),
        payload.len()
    );
    Ok(ExitCode::SUCCESS)
}

/// The encoded import names a finished module carries, or `None` if it carries no vendor
/// tables at all.
///
/// Falls back to the linker's undefined symbols at the call site, which is what a module
/// looks like *before* `mkmodule` has run over it.
fn vendor_import_names(bytes: &[u8]) -> Option<Vec<String>> {
    let elf = selfish_elf::Elf::parse(bytes).ok()?;
    let (segment, info) = elf.tables().ok()??;
    let strings = selfish_elf::dynamic::strings(segment, &info);
    let symbols = selfish_elf::dynamic::symbols(segment, &info).ok()?;
    Some(
        symbols
            .iter()
            .filter(|symbol| symbol.is_import() && symbol.name_offset != 0)
            .filter_map(|symbol| selfish_elf::dynamic::string_at(strings, symbol.name_offset).ok())
            .map(str::to_owned)
            .collect(),
    )
}

/// The linked tables the vendor segment is rebuilt from, out of the linker's sections.
///
/// Returns `.dynsym`, its string table, the two relocation tables, the linkage table's
/// address, and the initialiser's address if this module defines one.
///
/// **The initialiser is looked up rather than assumed, and absent rather than zeroed.** A
/// loader that calls it without checking the tag was present would otherwise execute the ELF
/// header as instructions.
#[allow(clippy::type_complexity, reason = "six tables, named at the call site")]
fn read_linked_tables(
    bytes: &[u8],
) -> Result<(Vec<u8>, Vec<u8>, Vec<u8>, Vec<u8>, u64, Option<u64>), Box<dyn std::error::Error>> {
    let parsed = selfish_elf::Elf::parse(bytes)?;
    let sections = parsed
        .sections()?
        .ok_or("the linked module has no sections to rebuild from")?;

    let dynsym = sections.find(".dynsym").ok_or("no .dynsym")?;
    let names = sections
        .headers()
        .get(dynsym.link.get() as usize)
        .and_then(|header| sections.contents(header))
        .ok_or("no string table for .dynsym")?;

    let init = sections.symbols().and_then(|(symbols, strings)| {
        symbols
            .iter()
            .find(|symbol| {
                !symbol.is_undefined()
                    && selfish_elf::section::string_at(strings, symbol.name_offset)
                        == Some(MODULE_INIT)
            })
            .map(|symbol| symbol.value)
    });

    let contents = |name: &str| {
        sections
            .find(name)
            .and_then(|header| sections.contents(header))
            .unwrap_or_default()
            .to_vec()
    };
    // `.got.plt` when the linker split them, `.got` when it did not, and zero if neither
    // exists - a module that calls nothing indirectly, which is possible.
    let pltgot = sections
        .find(".got.plt")
        .or_else(|| sections.find(".got"))
        .map_or(0, |header| header.addr.get());

    Ok((
        sections
            .contents(dynsym)
            .ok_or("no .dynsym bytes")?
            .to_vec(),
        names.to_vec(),
        contents(".rela.plt"),
        contents(".rela.dyn"),
        pltgot,
        init,
    ))
}

/// The initialiser this project's modules define, if they define one.
const MODULE_INIT: &str = "obs_module_init";

fn run_mkmodule(args: &MkmoduleArgs) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let file = args.file.as_path();
    // 5 and 4 on the command line, 2 and 0 in the header. The generation number is what
    // anyone building this thinks in; the byte is what the format holds.
    let generation = if args.generation == 5 {
        selfish_abi::Generation::Current
    } else {
        selfish_abi::Generation::Previous
    };
    let mut bytes = std::fs::read(file)?;

    // An executable, not a shared library. A loader that respects the distinction runs a
    // library's initialisers and then looks elsewhere for a process to start, so a module
    // marked the other way loads, relocates, and is never entered.
    //
    // Which kind of object this is, and it decides which loader will take the file. See the
    // `--kind` documentation: each wrong value is refused by name, which is the only reason
    // the three are distinguishable at all.
    let object_type = match args.kind.as_str() {
        "executable" => selfish_elf::ObjectType::Executable,
        "fixed" => selfish_elf::ObjectType::FixedExecutable,
        "shared" => selfish_elf::ObjectType::SharedLibrary,
        other => {
            return Err(
                format!("unknown --kind {other:?}: expected executable, fixed or shared").into(),
            );
        }
    };
    let changes = selfish_elf::identity::stamp(&mut bytes, object_type, generation)?;
    let name = file.file_name().unwrap_or_default().to_string_lossy();
    for change in &changes {
        println!(
            "{name}: {} {:#06x} -> {:#06x}",
            change.field, change.from, change.to
        );
    }

    let suffix = suffix::read(args.suffix_file.as_deref())?;
    // A shared library declares an export library, which takes library id zero and pushes the
    // imports up by one. An executable declares none, so its first import library *is* zero.
    // See `Imports::parse`. (D221)
    let exports_a_library = !object_type.is_executable();
    let manifest =
        imports::Imports::parse(&std::fs::read_to_string(&args.symbols)?, exports_a_library)?;
    let table = match args.table.as_str() {
        "current" => selfish_elf::dynamic::Table::Current,
        _ => selfish_elf::dynamic::Table::Legacy,
    };

    // Before anything is built. The builder would catch this too, but only after laying out
    // four tables - and the point of the check is to say which symbols are unplaced, which is
    // just as true before the work as after it.
    //
    // The mined corpus is read only to *suggest* where a missing symbol resolves, so a
    // failure names the line to add rather than only what is missing. `mkmodule` runs from the
    // repository root during the build, so the path is relative; if it is not found the check
    // still runs, just without suggestions.
    let corpus = std::fs::read_to_string("data/mined-names.txt")
        .map(|text| imports::corpus_libraries(&text))
        .unwrap_or_default();
    manifest.check_covers(&elf::Elf::parse(&bytes)?.undefined_symbols()?, &corpus)?;

    // The linked tables, out of the sections the linker left. They are the input the vendor
    // segment is rebuilt from, and they are the last thing this file reads through sections:
    // the built module has none, deliberately.
    let (symbols, names, jmprel, rela, pltgot, init) = read_linked_tables(&bytes)?;

    let libraries: Vec<selfish_elf::dynlib::Library> = manifest
        .libraries()
        .iter()
        .map(|library| selfish_elf::dynlib::Library {
            name: library.name.clone(),
            id: library.id,
            module_id: library.module_id,
        })
        .collect();

    let segment = selfish_elf::dynlib::build(
        selfish_elf::dynlib::Linked {
            symbols: &symbols,
            names: &names,
            jmprel: &jmprel,
            rela: &rela,
            pltgot,
        },
        &args.module_name,
        &libraries,
        &|plain| {
            let (library, module) = manifest.ids_for(plain)?;
            // A name carrying the sigil **is already the identifier** and must not be hashed:
            // hashing it would produce the identifier of the *string*, which names nothing.
            // Decoded rather than spliced, so a malformed one is caught here.
            let nid = match plain.strip_prefix(imports::PREENCODED) {
                Some(already) => selfish_nid::Nid::decode(already).ok()?,
                None => selfish_nid::Nid::with_suffix(plain, &suffix),
            };
            Some(selfish_elf::dynlib::Resolution {
                nid,
                library,
                module,
            })
        },
    )?;

    let installed = selfish_elf::dynlib::install(&mut bytes, &segment, table, generation, init)?;
    println!(
        "{name}: vendor segment at {:#x}, {} bytes, {} tags, {} symbols from {} libraries",
        installed.segment_offset,
        installed.segment_size,
        installed.tags,
        installed.encoded,
        installed.libraries
    );
    std::fs::write(file, &bytes)?;

    // Re-derive the tag assignment from what we just wrote, rather than trusting that
    // the constants used to build it were right. It costs a millisecond and it is the
    // difference between a derivation that is documented and one that is checked.
    let written = elf::Elf::parse(&bytes)?;
    let derivation = derive::run_with(
        &written,
        match args.table.as_str() {
            "current" => selfish_elf::dynamic::Table::Current,
            _ => selfish_elf::dynamic::Table::Legacy,
        },
    );
    if !derivation.is_consistent() {
        eprintln!("{name}: the module does not reproduce the tag derivation");
        return Ok(print_derivation(&derivation));
    }
    println!(
        "{name}: layout reproduces the tag derivation ({} relations)",
        derivation.relations.len()
    );
    Ok(ExitCode::SUCCESS)
}

/// Lists what a module imports.
fn run_imports(file: &std::path::Path) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let bytes = std::fs::read(file)?;
    let parsed = elf::Elf::parse(&bytes)?;
    // A finished module has no section headers, so `.dynsym` is not reachable. Read
    // its vendor tables the way a loader would; fall back to sections for a payload,
    // which keeps both shapes inspectable with one command.
    let names = match vendor_import_names(&bytes) {
        Some(names) => names,
        None => parsed.undefined_symbols()?,
    };
    println!("entry point  {:#x}", parsed.entry);
    println!("imports      {}", names.len());
    for name in &names {
        println!("  {name}");
    }
    if names.is_empty() {
        // An empty list means the platform calls were resolved at link time or
        // optimised away, and the module would test nothing at all.
        eprintln!("no imports: this module asks the platform for nothing");
        return Ok(ExitCode::FAILURE);
    }
    Ok(ExitCode::SUCCESS)
}

/// Writes a control module that imports nothing.
fn run_minimal(
    out: &std::path::Path,
    spin: bool,
    proc_param: bool,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    std::fs::write(out, probe::minimal(spin, proc_param))?;
    let body = if spin {
        "spins forever"
    } else {
        "returns immediately"
    };
    let param = if proc_param { ", with proc_param" } else { "" };
    println!("{}: imports nothing, {body}{param}", out.display());
    Ok(ExitCode::SUCCESS)
}

/// `hw send` - put a payload on the console and print whatever comes back.
///
/// Split out of `run_hw`, which is a dispatch table: the arms that are more than a few lines
/// each become their own function, the way `run_hw_check`, `run_hw_ls` and `run_hw_pull`
/// already had.
fn run_hw_send(
    name: Option<&str>,
    address: Option<&str>,
    seconds: u64,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(path) = address else {
        return Err("send needs a path: obscene-tool hw send <file.elf>".into());
    };
    let console = hardware::resolve(hardware::load()?, name)?;
    let elf = std::fs::read(path)?;
    // Refuse the wrong shape before it reaches a loader that will die on it.
    //
    // A vendor module and a plain ELF begin with the same four bytes, so `elfldr`'s
    // own sanity check passes either - and then it maps a module whose entry expects
    // thirty-five thousand resolved imports and goes down with it. That happened
    // here and cost a reboot.
    //
    // **Which byte says so is `pros_link`'s knowledge now, and the send guards on
    // it too.** Asking here as well is not a duplicated check: it is where the
    // advice specific to *this* project's two build outputs can be given, which a
    // shared library has no business knowing. (D189)
    let found = pros_link::identify(&elf);
    if !found.is_payload() {
        let advice = "this project builds both, so send obscene.elf, not obscene.eboot.bin";
        return Err(format!(
            "{path} is {} - {}. {advice}",
            found.describe(),
            found.remedy()
        )
        .into());
    }
    println!("sending {} bytes to {}", elf.len(), console.address);
    let out = pros_link::loader::send(
        &pros_link::Link::to(&console.address),
        &elf,
        Duration::from_secs(seconds),
    )?;
    if out.trim().is_empty() {
        println!("nothing arrived on the socket within {seconds}s");
        println!("not necessarily failure: only a payload launched this way reports");
        println!("here at all. The report is on the console:");
        println!("  obscene-tool hw pull");
    } else {
        print!("{out}");
    }
    Ok(ExitCode::SUCCESS)
}

/// `hw install` - serve the package and have the console fetch it.
///
/// Split out of `run_hw` for the same reason as `run_hw_send`: that function is a dispatch
/// table and its arms are functions.
fn run_hw_install(
    name: Option<&str>,
    address: Option<&str>,
    seconds: u64,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(pkg) = address else {
        return Err("install needs a package: obscene-tool hw install <file.pkg>".into());
    };
    let console = hardware::resolve(hardware::load()?, name)?;
    let path = std::path::Path::new(pkg);
    let offered = pros_core::handover::offer_to(path, &console.address)
        .map_err(|why| format!("could not offer the package: {why}"))?;
    println!("offering {} at {}", path.display(), offered.url);

    let out = pros_link::shell::run(
        &pros_link::Link::to(&console.address),
        &format!("pkg_install {}", offered.url),
        Duration::from_secs(seconds),
    )?;
    print!("{out}");

    // What the target actually did, which is the only honest evidence that it came for
    // the file at all. Zero fetches means it never asked, whatever the shell printed.
    println!("fetched {} time(s)", offered.taken());
    for line in offered.asked() {
        println!("  asked: {line}");
    }
    Ok(ExitCode::SUCCESS)
}

/// `hw push` - put a local file on the console's filesystem.
///
/// Split out of `run_hw` for the same reason as `run_hw_send` and `run_hw_install`.
fn run_hw_push(
    name: Option<&str>,
    address: Option<&str>,
    into: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(local) = address else {
        return Err("push needs a local file: obscene-tool hw push <file> --into <remote>".into());
    };
    let console = hardware::resolve(hardware::load()?, name)?;
    let remote = into.to_str().ok_or("the remote path is not valid UTF-8")?;
    let bytes = std::fs::read(local)?;
    pros_link::files::store(&pros_link::Link::to(&console.address), remote, &bytes)?;
    println!("pushed {} bytes -> {remote}", bytes.len());
    Ok(ExitCode::SUCCESS)
}

/// `hw install-native` - upload a native title directory to a scan root on the target.
///
/// `make native` writes `<BUILD>/native/<TITLE_ID>/`, so the id is the directory's own name and the
/// remote is `<base>/<TITLE_ID>`. The upload is prosperous's `transfer::upload` - an FTP STOR per
/// file, a directory made as needed - the same call `pros-cli restore` uses.
///
/// The base defaults to `/user/data`, which is one of the directories an auto-mounter
/// (`ShadowMountPlus`) *scans* - it registers what it finds there into `/user/app` for you. **Not**
/// `/user/app` itself, which is where titles are registered *to* and is never scanned, so a copy
/// there is inert. Pass `--into <path>` (an absolute path such as `/mnt/usb0`) to target a
/// different scan root - a USB drive, or a titles subfolder.
///
/// It copies the bytes; registration into the app database is the auto-mounter's job (or
/// `sceAppInstUtilAppInstallTitleDir` from a privileged payload). (D291)
fn run_hw_install_native(
    name: Option<&str>,
    address: Option<&str>,
    into: &std::path::Path,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(local) = address else {
        return Err(
            "install-native needs a title directory: obscene-tool hw install-native <dir>".into(),
        );
    };
    let dir = std::path::Path::new(local);
    let appid = dir
        .file_name()
        .and_then(|n| n.to_str())
        .ok_or("the title directory has no name to install under")?;
    // A scan root, so the auto-mounter finds and registers it. `--into` overrides with any absolute
    // path; the report-file default that `into` otherwise carries is not one, so it falls through.
    let base = match into.to_str() {
        Some(p) if p.starts_with('/') => p.trim_end_matches('/'),
        _ => "/user/data/homebrew",
    };
    let remote = format!("{base}/{appid}");
    let console = hardware::resolve(hardware::load()?, name)?;
    let mut session = pros_link::files::Session::open(&pros_link::Link::to(&console.address))?;
    let _ = session.make_directory(base);
    let _ = session.make_directory(&remote);
    let summary = pros_core::transfer::upload(
        &mut session,
        dir,
        &remote,
        &mut |progress| println!("  {}", progress.current),
        &|| false,
    );
    session.close();
    let sum = summary?;
    for skipped in &sum.skipped {
        eprintln!("  skipped {}: {}", skipped.path, skipped.why);
    }
    if !sum.skipped.is_empty() {
        return Err(format!("upload completed with {} skipped entries", sum.skipped.len()).into());
    }
    println!("installed {appid} -> {remote} ({} files, {} bytes)", sum.files, sum.bytes);
    Ok(ExitCode::SUCCESS)
}

/// `hw launch` - start an installed title by its id, in the foreground.
///
/// Split out of `run_hw` for the same reason as the three beside it.
fn run_hw_launch(
    name: Option<&str>,
    address: Option<&str>,
    seconds: u64,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(title) = address else {
        return Err("launch needs a title id: obscene-tool hw launch <TITLEID>".into());
    };
    let console = hardware::resolve(hardware::load()?, name)?;
    let out = pros_link::shell::run(
        &pros_link::Link::to(&console.address),
        &format!("launch {title}"),
        Duration::from_secs(seconds),
    )?;
    print!("{out}");
    Ok(ExitCode::SUCCESS)
}

/// `hw deploy` - install and launch holding one server open across both.
///
/// Split out of `run_hw` for the same reason as the four beside it. The reason it is one
/// action rather than two is in the comment above its arm: the launcher re-fetches the image
/// from the URL the install recorded, so a server that stops between them leaves a dead URL.
fn run_hw_deploy(
    name: Option<&str>,
    address: Option<&str>,
    into: &std::path::Path,
    seconds: u64,
) -> Result<ExitCode, Box<dyn std::error::Error>> {
    let Some(pkg) = address else {
        return Err(
            "deploy needs a package: obscene-tool hw deploy <file.pkg> [--into TITLEID]".into(),
        );
    };
    // The title id to launch. `--into` carries it (its report-file default is not one),
    // and obSCEne's own title is the fallback so the common case needs no flag.
    let title = into
        .to_str()
        .filter(|value| *value != "console-report.txt")
        .unwrap_or("OBSC00001");
    let console = hardware::resolve(hardware::load()?, name)?;
    let path = std::path::Path::new(pkg);
    let offered = pros_core::handover::offer_to(path, &console.address)
        .map_err(|why| format!("could not offer the package: {why}"))?;
    println!("offering {} at {}", path.display(), offered.url);

    // Install: the console fetches the whole file and registers the app against this url.
    let out = pros_link::shell::run(
        &pros_link::Link::to(&console.address),
        &format!("pkg_install {}", offered.url),
        Duration::from_secs(seconds),
    )?;
    print!("{out}");
    println!("fetched {} time(s) for the install", offered.taken());

    // Promote, then launch - by polling, not on a blind clock. `pkg_install` returns once it has
    // taken the file, but the console then re-installs and *promotes* this app in the background
    // (DbgInstall -> prepromote -> a BGFT copy of app.pkg), and a launch into that race is refused
    // with `launchApp 0x80a40086` (not yet launchable). Rather than sleep a fixed 30s and hope,
    // retry the launch and read the result: while it says `0x80a40086` the promote is still
    // running, so wait briefly and try again; the first launch that does not is the one that took.
    // This launches the instant the console is ready - usually well under 30s - and gives up with a
    // clear message rather than launching blind if the promote never completes. Each probe returns
    // as soon as the launcher answers, so a ready console is not made to wait out a timeout.
    println!("waiting for the install to promote, then launching {title}...");
    let promote_deadline = deadline_in(seconds.max(90));
    let out = loop {
        let out = pros_link::shell::run(
            &pros_link::Link::to(&console.address),
            &format!("launch {title}"),
            Duration::from_secs(20),
        )?;
        if !out.contains("0x80a40086") {
            break out;
        }
        if std::time::Instant::now() >= promote_deadline {
            print!("{out}");
            return Err(
                "the install never became launchable (launchApp kept returning 0x80a40086)".into(),
            );
        }
        std::thread::sleep(Duration::from_secs(2));
    };
    print!("{out}");

    // Hold the server for the app0 mount, but only while it is still reading. The mount fetches the
    // image lazily over range requests; once `taken()` stops climbing the image is fully read and
    // the handover can drop. This replaces the blind hold: a run and a crash both stop the moment
    // the fetching does, instead of sitting idle for the whole window - which is what made a
    // crashed launch cost minutes with nothing on record. The full window is only ever the ceiling.
    println!(
        "holding {} open while the mount reads the image...",
        offered.url
    );
    let hold_deadline = deadline_in(seconds.max(30));
    let mut last = offered.taken();
    let mut quiet = 0u32;
    loop {
        std::thread::sleep(Duration::from_secs(3));
        let now = offered.taken();
        if now == last {
            quiet = quiet.saturating_add(1);
            // ~9s with no new range request: the mount has read what it needs.
            if quiet >= 3 {
                break;
            }
        } else {
            quiet = 0;
            last = now;
        }
        if std::time::Instant::now() >= hold_deadline {
            break;
        }
    }
    println!("fetched {} time(s) in total", offered.taken());
    Ok(ExitCode::SUCCESS)
}
