//! Every symbol name any emulator in the toolkit knows, with the identifier it claims.
//!
//! # Why this exists
//!
//! obSCEne cannot discover a symbol. It resolves by an identifier computed from a name this
//! project supplies, so a name absent from `surface.h` and `platform.h` is not merely
//! untested - it is unreachable, and no amount of running the suite will ever surface it
//! (D103). The surface grows by reading somebody else's table and diffing, or it does not
//! grow.
//!
//! # Six formats, because nobody agreed on one
//!
//! Each emulator publishes its table differently, and every one carries the name beside the
//! identifier - so nothing here is inferred from a naming convention:
//!
//! ```text
//! shadPS4     LIB_FUNCTION("NVDXiUesSbA", "libSceAjm", 1, "libSceAjm", sceAjmBatchCancel)
//! fpPS4       (i:$E304B37BDD8184B2;n:'sceKernelWrite'),
//! PS5PCEM     .{ .name = "sceVideoOutSubmitFlip", ..., .expect_id = "U46NwOiJpys" },
//! SharpEMU    Nid = "...", ExportName = "...", LibraryName = "..."
//! GPCS4       { 0x7660F26CDFFF167F, "sceAjmBatchJobControlBufferRa", (void*)... },
//! Chonky4     module.addSymbolExport("eDFeTyi+G3Y", "sceAjmDecMp3ParseFrame", "libSceAjm", ...)
//! ```
//!
//! Two give the identifier in hex and four in the encoded form, and both are **normalised to
//! hex on record** so that agreement between sources can be seen at all - comparing the two
//! notations as written made 5,094 symbols look disputed, and not one of them was.
//!
//! That is a change of notation, not a hash. A hash here would be a second implementation of
//! the thing the corpus exists to check, and would agree with itself for free. (The Python
//! this replaced said in its own docstring that nothing was converted, while calling the
//! converter one screen further down.)
//!
//! # The identifier is evidence, not payload
//!
//! obSCEne imports by **name**: `mkmodule` hashes it. So the name is what this corpus is for,
//! and the claimed identifier is what makes the corpus checkable.
//!
//! # Parsed by hand, except the firmware
//!
//! These are line-oriented tables, and splitting on their markers is both simpler to read
//! and easier to test than a pattern - the ported checkers found real bugs in their Python
//! regexes doing exactly this. The firmware descriptions are JSON, where escapes, nesting
//! and encodings are precisely where a hand-rolled reader is quietly wrong, so those go
//! through `serde_json`.

use std::collections::{BTreeMap, BTreeSet};
use std::fmt::Write as _;
use std::path::{Path, PathBuf};

/// What one source said about one name.
#[derive(Debug, Default, Clone)]
pub struct Entry {
    /// Libraries the name was attributed to.
    pub libraries: BTreeSet<String>,
    /// Identifiers claimed for it, as published.
    pub identifiers: BTreeSet<String>,
    /// Which sources named it.
    pub sources: BTreeSet<String>,
    /// `fn` or `data`. Only the firmware records this.
    pub kinds: BTreeSet<String>,
}

/// Not a symbol name.
///
/// `^.L` is an assembler-local label, which is never an export. The whitespace clause
/// catches something different: one exported table holds rows whose name column is a
/// fragment of a log format string, because whatever produced it walked a binary and
/// captured the neighbouring literal. A C identifier cannot contain whitespace.
///
/// Placeholders like `Func_00F4D778F1C88CB3` are deliberately **not** filtered. They are a
/// source's way of writing "the function whose identifier is this", so the hex in the name
/// is real data.
#[must_use]
pub fn not_a_symbol(name: &str) -> bool {
    name.starts_with(".L") || name.chars().any(char::is_whitespace) || name.is_empty()
}

/// The encoded alphabet: six bits per character, eleven characters.
const ALPHABET: &str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

/// Both published notations reduced to one, so agreement can be seen.
///
/// Two sources give the identifier in hex and four in the encoded form; comparing them as
/// strings made every symbol both knew look disputed - 5,094 of them did, and not one was a
/// real disagreement.
///
/// **This converts a notation; it does not compute a hash.** A hash here would be a second
/// implementation of the thing the corpus exists to check, and would agree with itself for
/// free.
#[must_use]
pub fn normalise(ident: &str) -> String {
    if ident.starts_with("0x") {
        return ident.to_lowercase();
    }
    if ident.len() != 11 || !ident.chars().all(|c| ALPHABET.contains(c)) {
        return ident.to_owned();
    }
    let mut bits: u128 = 0;
    for c in ident.chars() {
        let Some(index) = ALPHABET.find(c) else {
            return ident.to_owned();
        };
        bits = (bits << 6) | index as u128;
    }
    // Eleven characters hold 66 bits; the identifier is the top 64.
    format!("0x{:016x}", bits >> 2)
}

/// A source file's text, with invalid bytes replaced rather than the file skipped.
///
/// `read_to_string` fails outright on invalid UTF-8, and two of shadPS4's `.cpp` files
/// carry it - `np_manager.cpp` among them. Skipping a file that fails to decode silently
/// dropped fifty-eight real rows, and the only symptom was a source missing from a column
/// nobody was diffing. Lossy is right here: a stray byte in a comment must not cost the
/// whole table.
fn text_of(path: &Path) -> Option<String> {
    let raw = std::fs::read(path).ok()?;
    let text = String::from_utf8_lossy(&raw).into_owned();
    Some(
        text.strip_prefix('\u{feff}')
            .map_or(text.clone(), str::to_owned),
    )
}

/// Every file under `root` with one of these extensions.
fn walk(root: &Path, extensions: &[&str]) -> Vec<PathBuf> {
    let mut out = Vec::new();
    let mut stack = vec![root.to_path_buf()];
    while let Some(dir) = stack.pop() {
        let Ok(entries) = std::fs::read_dir(&dir) else {
            continue;
        };
        for entry in entries.filter_map(Result::ok) {
            let path = entry.path();
            let name = entry.file_name().to_string_lossy().into_owned();
            if path.is_dir() {
                if !matches!(name.as_str(), ".git" | "target" | "build") {
                    stack.push(path);
                }
            } else if extensions
                .iter()
                .any(|e| path.extension().is_some_and(|x| x.eq_ignore_ascii_case(e)))
            {
                out.push(path);
            }
        }
    }
    out.sort();
    out
}

/// The text between the next pair of quotes after `marker`, and what follows it.
fn quoted_after<'a>(text: &'a str, marker: &str) -> Option<(&'a str, &'a str)> {
    let (_, rest) = text.split_once(marker)?;
    let rest = rest.trim_start();
    let rest = rest.strip_prefix('"')?;
    rest.split_once('"')
}

/// One record mined from a source: name, identifier as published, and library if known.
type Row = (String, String, Option<String>);

/// The libraries an unnamed identifier was exported from, and the firmware versions it was
/// seen in.
pub type Sighting = (BTreeSet<String>, BTreeSet<String>);

/// Unnamed identifiers, keyed by the identifier itself.
pub type Unnamed = BTreeMap<String, Sighting>;

/// The named corpus and the unnamed identifiers, which is what one mining pass produces.
pub type Mined = (BTreeMap<String, Entry>, Unnamed);

/// `LIB_FUNCTION("<nid>", "<library>", 1, "<library>", <name>)`
fn parse_shadps4(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split("LIB_FUNCTION(").skip(1) {
        let Some((head, _)) = chunk.split_once(')') else {
            continue;
        };
        let mut fields = head.split(',');
        let (Some(nid), Some(library)) = (fields.next(), fields.next()) else {
            continue;
        };
        let nid = nid.trim().trim_matches('"');
        let library = library.trim().trim_matches('"');
        let Some(name) = head.rsplit(',').next() else {
            continue;
        };
        let name = name.trim();
        // The fifth argument names the *implementation*, and only when it is a plain
        // identifier is it also the exported name.
        //
        // `LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", Libraries::Net::sys_connect)`
        // binds a networking function to a libkernel export, and `sys_connect` hashes to
        // `L3GvCsVw1ZQ` - not the identifier the row claims. Recording the name would put a
        // symbol in the corpus that resolves to nothing.
        //
        // `ORBIS(posix_pthread_attr_destroy)` is the same trap in a different shape:
        // unwrapping it yields a name hashing to `7w+8HkdPgUQ` against a claimed
        // `62KCwEMmzcM`. So a wrapper or a qualification means the export name is simply not
        // in the row, and recording anything would put a symbol in the corpus that resolves
        // to nothing.
        //
        // The Python dropped both by accident - its name pattern was `\w+`, which happens to
        // match neither. Stating the rule means a new shape gets recognised rather than
        // silently captured.
        let plain = !name.is_empty()
            && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
            && name
                .chars()
                .next()
                .is_some_and(|c| c.is_ascii_alphabetic() || c == '_');
        if nid.len() == 11 && plain {
            out.push((name.to_owned(), nid.to_owned(), Some(library.to_owned())));
        }
    }
    out
}

/// `(i:$HEX;n:'name')`
fn parse_fpps4(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split("(i:$").skip(1) {
        let Some((hex, rest)) = chunk.split_once(';') else {
            continue;
        };
        if hex.len() != 16 || !hex.chars().all(|c| c.is_ascii_hexdigit()) {
            continue;
        }
        let Some(rest) = rest.trim_start().strip_prefix("n:'") else {
            continue;
        };
        let Some((name, _)) = rest.split_once('\'') else {
            continue;
        };
        out.push((name.to_owned(), format!("0x{}", hex.to_uppercase()), None));
    }
    out
}

/// `.name = "<name>" ... .expect_id = "<id>"`
fn parse_ps5pcem(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split(".name = \"").skip(1) {
        let Some((name, rest)) = chunk.split_once('"') else {
            continue;
        };
        // The identifier must belong to this entry, not a later one.
        let Some(entry_end) = rest.find('}') else {
            continue;
        };
        let Some(entry) = rest.get(..entry_end) else {
            continue;
        };
        let Some((ident, _)) = quoted_after(entry, ".expect_id =") else {
            continue;
        };
        out.push((name.to_owned(), ident.to_owned(), None));
    }
    out
}

/// `Nid = "..." ... ExportName = "..." ... LibraryName = "..."`
fn parse_sharpemu(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split("Nid = \"").skip(1) {
        let Some((nid, rest)) = chunk.split_once('"') else {
            continue;
        };
        // Bounded to this initialiser, so a missing field does not borrow the next entry's.
        let end = rest.find(')').unwrap_or(rest.len());
        let Some(entry) = rest.get(..end) else {
            continue;
        };
        let (Some((name, _)), Some((library, _))) = (
            quoted_after(entry, "ExportName ="),
            quoted_after(entry, "LibraryName ="),
        ) else {
            continue;
        };
        out.push((name.to_owned(), nid.to_owned(), Some(library.to_owned())));
    }
    out
}

/// `{ 0xHEX, "name", ... }`
fn parse_gpcs4(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split("{ 0x").skip(1) {
        let Some((hex, rest)) = chunk.split_once(',') else {
            continue;
        };
        let hex = hex.trim();
        if hex.len() != 16 || !hex.chars().all(|c| c.is_ascii_hexdigit()) {
            continue;
        }
        let Some(rest) = rest.trim_start().strip_prefix('"') else {
            continue;
        };
        let Some((name, _)) = rest.split_once('"') else {
            continue;
        };
        out.push((name.to_owned(), format!("0x{}", hex.to_uppercase()), None));
    }
    out
}

/// `addSymbolExport("<nid>", "<name>", "<library>", ...)`
fn parse_chonky(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split("addSymbolExport(").skip(1) {
        let Some((head, _)) = chunk.split_once(')') else {
            continue;
        };
        let fields: Vec<&str> = head
            .split(',')
            .map(|f| f.trim().trim_matches('"'))
            .collect();
        let (Some(nid), Some(name), Some(library)) = (fields.first(), fields.get(1), fields.get(2))
        else {
            continue;
        };
        if nid.len() == 11 {
            out.push((
                (*name).to_owned(),
                (*nid).to_owned(),
                Some((*library).to_owned()),
            ));
        }
    }
    out
}

/// One in-source table: which checkout directory, which file extension, which parser.
type Source = (&'static str, &'static str, fn(&str) -> Vec<Row>);

/// The in-source tables.
const SOURCES: [Source; 7] = [
    ("shadPS4", "cpp", parse_shadps4),
    ("fpPS4", "pas", parse_fpps4),
    ("PS5PCEM", "zig", parse_ps5pcem),
    ("SharpEMU", "cs", parse_sharpemu),
    ("craziiEmu", "cs", parse_sharpemu),
    ("GPCS4", "cpp", parse_gpcs4),
    ("ChonkyStation4", "cpp", parse_chonky),
];

/// Flat catalogues: one identifier and one name per line, no library.
///
/// Far larger than anything mined from source, because they were built by walking firmware
/// rather than by somebody implementing functions one at a time.
const FLAT: [(&str, &str, char); 2] = [
    ("aerolib.csv", "ps4_module_loader/aerolib.csv", ' '),
    ("SharpEMU-aerolib", "SharpEMU/artifacts/aerolib.txt", '\t'),
];

fn record(
    found: &mut BTreeMap<String, Entry>,
    name: &str,
    ident: &str,
    library: Option<&str>,
    source: &str,
    kind: Option<&str>,
) {
    if not_a_symbol(name) {
        return;
    }
    let entry = found.entry(name.to_owned()).or_default();
    if !ident.is_empty() {
        // Normalised here rather than at write time, so two sources publishing the same
        // identifier in different notations collapse to one entry instead of looking like a
        // disagreement.
        entry.identifiers.insert(normalise(ident));
    }
    if let Some(library) = library {
        entry.libraries.insert(library.to_owned());
    }
    entry.sources.insert(source.to_owned());
    if let Some(kind) = kind {
        entry.kinds.insert(kind.to_owned());
    }
}

/// Mine every source. Returns the named corpus and the unnamed identifiers.
pub fn mine(emulators: &Path, firmware: &Path) -> std::io::Result<Mined> {
    let mut found: BTreeMap<String, Entry> = BTreeMap::new();

    for (label, extension, parse) in SOURCES {
        let root = emulators.join(label);
        if !root.is_dir() {
            eprintln!("  (absent: {label})");
            continue;
        }
        let mut count = 0usize;
        for path in walk(&root, &[extension]) {
            let Some(text) = text_of(&path) else {
                continue;
            };
            for (name, ident, library) in parse(&text) {
                record(&mut found, &name, &ident, library.as_deref(), label, None);
                count = count.saturating_add(1);
            }
        }
        eprintln!("{label:<18}: {count} rows");
    }

    for (label, relative, separator) in FLAT {
        let path = emulators.join(relative);
        let Some(text) = text_of(&path) else {
            eprintln!("  (absent: {label})");
            continue;
        };
        let mut count = 0usize;
        for line in text.lines() {
            if line.starts_with('#') {
                continue;
            }
            let fields: Vec<&str> = if separator == ' ' {
                line.split_whitespace().collect()
            } else {
                line.trim_end().split(separator).collect()
            };
            if fields.len() != 2 {
                continue;
            }
            let (Some(ident), Some(name)) = (fields.first(), fields.get(1)) else {
                continue;
            };
            record(&mut found, name, ident, None, label, None);
            count = count.saturating_add(1);
        }
        eprintln!("{label:<18}: {count} rows");
    }

    // Names with no identifier beside them.
    //
    // A published list of names is still worth having - obSCEne hashes the name itself, so a
    // name is sufficient to reach a symbol - it just cannot corroborate anything, which is
    // why these carry no identifier and never enter the disagreement check.
    let names_only = emulators.join("ps4libdoc").join("known_names.txt");
    match text_of(&names_only) {
        Some(text) => {
            let mut count = 0usize;
            for line in text.lines() {
                let name = line.trim().trim_start_matches('\u{feff}');
                if name.is_empty() || not_a_symbol(name) {
                    continue;
                }
                record(&mut found, name, "", None, "ps4libdoc", None);
                count = count.saturating_add(1);
            }
            eprintln!("ps4libdoc         : {count} names (no identifiers)");
        }
        None => eprintln!("  (absent: ps4libdoc)"),
    }

    let unnamed = mine_firmware(firmware, &mut found)?;
    Ok((found, unnamed))
}

/// Every firmware version's module descriptions.
///
/// The one source that carries the **library** for each symbol, which is what the others
/// cannot supply and what obSCEne needs in order to import anything: `mkmodule` refuses to
/// guess which library a symbol comes from, and a guess produces a symbol that never
/// resolves.
///
/// Returns the unnamed identifiers separately. They are the larger half by far, and unlike a
/// flat list of hashes these carry a library too - so an unnamed identifier can be imported
/// from the right place and actually resolve.
fn mine_firmware(firmware: &Path, found: &mut BTreeMap<String, Entry>) -> std::io::Result<Unnamed> {
    let mut unnamed: Unnamed = BTreeMap::new();
    if !firmware.is_dir() {
        eprintln!("  (absent: firmware trees at {})", firmware.display());
        return Ok(unnamed);
    }
    let mut versions: Vec<String> = std::fs::read_dir(firmware)?
        .filter_map(Result::ok)
        .filter(|e| e.path().is_dir())
        .map(|e| e.file_name().to_string_lossy().into_owned())
        .collect();
    versions.sort();

    let mut named_rows = 0usize;
    for version in &versions {
        for path in walk(&firmware.join(version), &["json"]) {
            let Ok(raw) = std::fs::read(&path) else {
                continue;
            };
            // A byte-order mark at the front of every file, which a JSON reader will not
            // accept - stripped rather than letting the whole version fail.
            let text = String::from_utf8_lossy(&raw);
            let text = text.strip_prefix('\u{feff}').unwrap_or(&text);
            let Ok(doc) = serde_json::from_str::<serde_json::Value>(text) else {
                continue;
            };
            let modules = doc.get("modules").and_then(|m| m.as_array());
            for module in modules.into_iter().flatten() {
                let libraries = module.get("libraries").and_then(|l| l.as_array());
                for library in libraries.into_iter().flatten() {
                    let lib = library.get("name").and_then(|n| n.as_str());
                    let symbols = library.get("symbols").and_then(|s| s.as_array());
                    for symbol in symbols.into_iter().flatten() {
                        let name = symbol.get("name").and_then(|n| n.as_str());
                        let ident = symbol.get("encoded_id").and_then(|n| n.as_str());
                        // `type` absent means Function, per the format's own note. Object
                        // and TLS are data: calling one jumps into a variable, which is how
                        // the blind prober died at `libSceImageUtil|__dso_handle`.
                        let kind = match symbol.get("type").and_then(|t| t.as_str()) {
                            Some("Object" | "TLS" | "Unknown11") => "data",
                            _ => "fn",
                        };
                        match (name, ident) {
                            (Some(name), ident) => {
                                record(
                                    found,
                                    name,
                                    ident.unwrap_or_default(),
                                    lib,
                                    "firmware",
                                    Some(kind),
                                );
                                named_rows = named_rows.saturating_add(1);
                            }
                            (None, Some(ident)) => {
                                let slot = unnamed.entry(ident.to_owned()).or_default();
                                if let Some(lib) = lib {
                                    slot.0.insert(lib.to_owned());
                                }
                                slot.1.insert(version.clone());
                            }
                            (None, None) => {}
                        }
                    }
                }
            }
        }
    }
    eprintln!(
        "firmware          : {named_rows} named rows across {} versions, {} unnamed",
        versions.len(),
        unnamed.len()
    );
    Ok(unnamed)
}

/// The nameless half, written separately because it is a different kind of thing: an
/// identifier with no recoverable name, and an order of magnitude larger than the corpus
/// beside it.
fn write_unnamed(data: &Path, unnamed: &Unnamed, provenance: &str) -> std::io::Result<()> {
    if unnamed.is_empty() {
        return Ok(());
    }
    let mut out = [
        "# Identifiers observed in firmware modules with no known name.",
        "# Columns: identifier, library, firmware versions seen in.",
        "#",
        "# These are not junk. Every one names a real function - the vendor's",
        "# toolchain hashed a real symbol to produce it and kept only the hash, which",
        "# is what the scheme is for. Nobody outside the vendor has the name, and that",
        "# is the only reason these are here rather than in the corpus beside them.",
        "#",
        "# The library matters more here than it does for a named symbol. A name can",
        "# be hashed and asked for anywhere; an identifier can only be imported from",
        "# the library that exports it, so this column is what makes these usable at",
        "# all rather than a list of numbers.",
        "#",
    ]
    .join(
        "
",
    );
    out.push('\n');
    out.push_str(provenance);
    for (ident, (libraries, versions)) in unnamed {
        let libs = if libraries.is_empty() {
            "-".to_owned()
        } else {
            libraries.iter().cloned().collect::<Vec<_>>().join(",")
        };
        let seen = versions.iter().cloned().collect::<Vec<_>>().join(",");
        let _ = writeln!(out, "{ident} {libs} {seen}");
    }
    let path = data.join("unnamed-nids.txt");
    std::fs::write(&path, out)?;
    println!(
        "written: {} ({} unnamed identifiers)",
        path.display(),
        unnamed.len()
    );
    Ok(())
}

/// Mine, then write both corpus files with the provenance header `obscene-tool corpus`
/// checks against.
///
/// Headers are built as a list of lines rather than one continued string literal. The first
/// version used backslash-newline continuations and left nine spaces of source
/// indentation on every line after the first - which a leading-hash filter then failed
/// to recognise as comments, so twelve header lines counted as symbol names.
pub fn write(root: &Path, emulators: &Path, firmware: &Path) -> std::io::Result<()> {
    let (found, unnamed) = mine(emulators, firmware)?;
    let provenance = crate::corpus::provenance_lines(emulators, firmware);
    let data = root.join("data");
    std::fs::create_dir_all(&data)?;

    write_unnamed(&data, &unnamed, &provenance)?;

    let mut out = [
        "# Every symbol name the emulator toolkit knows, mined by obscene-tool mine.",
        "# Columns: name, library, identifiers, sources, kind.",
        "#",
        "# `kind` is fn or data. Only the firmware descriptions record it - an",
        "# absent type means Function, per their own format note - and the",
        "# emulator tables are export lists of functions by construction.",
        "# It matters because `910-bulk` calls what it is given, and calling",
        "# an Object jumps into a variable: the prober died at",
        "# `libSceImageUtil|__dso_handle` doing exactly that (D114).",
        "# Identifiers are as published - hex from two sources, encoded from four - and",
        "# are evidence rather than payload: obSCEne hashes the name. See D103.",
        "#",
    ]
    .join(
        "
",
    );
    out.push('\n');
    out.push_str(&provenance);
    out.push_str(
        "#
",
    );
    for (name, entry) in &found {
        let libs = if entry.libraries.is_empty() {
            "-".to_owned()
        } else {
            entry
                .libraries
                .iter()
                .cloned()
                .collect::<Vec<_>>()
                .join(",")
        };
        let ids = if entry.identifiers.is_empty() {
            "-".to_owned()
        } else {
            entry
                .identifiers
                .iter()
                .cloned()
                .collect::<Vec<_>>()
                .join(",")
        };
        let sources = entry.sources.iter().cloned().collect::<Vec<_>>().join(",");
        // `data` wins a disagreement, because the consequence is asymmetric: calling a
        // function recorded as data costs a skipped probe, and calling data recorded as a
        // function jumps into a variable.
        let kind = if entry.kinds.contains("data") {
            "data"
        } else {
            "fn"
        };
        let _ = writeln!(out, "{name} {libs} {ids} {sources} {kind}");
    }
    let path = data.join("mined-names.txt");
    std::fs::write(&path, out)?;

    report_disagreements(&found);
    println!("written: {} ({} names)", path.display(), found.len());
    Ok(())
}

/// Names two sources give different identifiers for.
///
/// Reported rather than resolved: a disagreement is either a mis-parse here or an error
/// there, and both are worth knowing before several thousand names are taken on trust.
///
/// **Compared after normalising the notation.** Two sources publish hex and four publish the
/// encoded form, and comparing them as written made 5,094 symbols look disputed - not one of
/// which was a real disagreement. What is left is the real thing: one source binding a single
/// function to several identifiers.
fn report_disagreements(found: &BTreeMap<String, Entry>) {
    let mut disputed = Vec::new();
    for (name, entry) in found {
        let distinct: BTreeSet<String> = entry.identifiers.iter().map(|i| normalise(i)).collect();
        if distinct.len() > 1 {
            disputed.push((name.clone(), distinct));
        }
    }
    if disputed.is_empty() {
        return;
    }
    eprintln!(
        "
{} names carry more than one identifier (first 5):",
        disputed.len()
    );
    for (name, idents) in disputed.iter().take(5) {
        for ident in idents {
            eprintln!("  {name:<44} {ident}");
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_name_with_whitespace_is_not_a_symbol() {
        assert!(not_a_symbol("sceBgftNotifyExtUsbEjected 0x%08x"));
        assert!(not_a_symbol(".Lstr.1"));
        assert!(!not_a_symbol("sceKernelOpen"));
        // A placeholder is real data: the source's way of writing "the function whose
        // identifier is this".
        assert!(!not_a_symbol("Func_00F4D778F1C88CB3"));
    }

    /// Comparing notations as strings made 5,094 symbols look disputed, and not one was.
    #[test]
    fn both_notations_normalise_to_one() {
        assert_eq!(normalise("0xE304B37BDD8184B2"), "0xe304b37bdd8184b2");
        // Eleven characters hold 66 bits; the identifier is the top 64.
        assert_eq!(normalise("AAAAAAAAAAA"), "0x0000000000000000");
        // Anything that is not eleven characters of the alphabet passes through untouched.
        // The first fixture here was `"not-encoded"`, which is exactly eleven characters and
        // every one of them is in the alphabet - so it decoded, correctly, and the test was
        // wrong rather than the function.
        assert_eq!(normalise("short"), "short");
        assert_eq!(normalise("has_underscore"), "has_underscore");
        assert_eq!(normalise("elevenchar!"), "elevenchar!");
    }

    #[test]
    fn shadps4_rows_carry_the_library() {
        let rows = parse_shadps4(
            "LIB_FUNCTION(\"NVDXiUesSbA\", \"libSceAjm\", 1, \"libSceAjm\", sceAjmBatchCancel)",
        );
        assert_eq!(rows.len(), 1);
        assert_eq!(
            rows.first()
                .map(|r| (r.0.as_str(), r.1.as_str(), r.2.as_deref())),
            Some(("sceAjmBatchCancel", "NVDXiUesSbA", Some("libSceAjm")))
        );
    }

    #[test]
    fn fpps4_rows_are_hex() {
        let rows = parse_fpps4("(i:$E304B37BDD8184B2;n:'sceKernelWrite'),");
        assert_eq!(
            rows.first().map(|r| (r.0.as_str(), r.1.as_str())),
            Some(("sceKernelWrite", "0xE304B37BDD8184B2"))
        );
    }

    #[test]
    fn gpcs4_rows_are_hex() {
        let rows =
            parse_gpcs4("{ 0x7660F26CDFFF167F, \"sceAjmBatchJobControlBufferRa\", (void*)x },");
        assert_eq!(
            rows.first().map(|r| (r.0.as_str(), r.1.as_str())),
            Some(("sceAjmBatchJobControlBufferRa", "0x7660F26CDFFF167F"))
        );
    }

    #[test]
    fn chonky_rows_carry_the_library() {
        let rows = parse_chonky(
            "module.addSymbolExport(\"eDFeTyi+G3Y\", \"sceAjmDecMp3ParseFrame\", \"libSceAjm\", 1)",
        );
        assert_eq!(
            rows.first().map(|r| (r.0.as_str(), r.2.as_deref())),
            Some(("sceAjmDecMp3ParseFrame", Some("libSceAjm")))
        );
    }

    #[test]
    fn ps5pcem_pairs_a_name_with_its_own_identifier() {
        let rows = parse_ps5pcem(
            ".{ .name = \"sceVideoOutSubmitFlip\", .expect_id = \"U46NwOiJpys\" },\n\
             .{ .name = \"sceVideoOutOpen\", .expect_id = \"Up36PTk687E\" },",
        );
        assert_eq!(rows.len(), 2);
        assert_eq!(
            rows.first().map(|r| (r.0.as_str(), r.1.as_str())),
            Some(("sceVideoOutSubmitFlip", "U46NwOiJpys"))
        );
        assert_eq!(
            rows.get(1).map(|r| (r.0.as_str(), r.1.as_str())),
            Some(("sceVideoOutOpen", "Up36PTk687E"))
        );
    }

    /// An entry missing a field must not borrow the next entry's, which is the failure a
    /// non-greedy pattern is written to avoid and a naive split walks straight into.
    #[test]
    fn a_sharpemu_entry_does_not_borrow_the_next_ones_fields() {
        let rows = parse_sharpemu(
            "new(){ Nid = \"aaa\" }\nnew(){ Nid = \"bbb\", ExportName = \"sceReal\", LibraryName = \"libSceX\" }",
        );
        assert_eq!(rows.len(), 1, "got {rows:?}");
        assert_eq!(
            rows.first().map(|r| (r.0.as_str(), r.1.as_str())),
            Some(("sceReal", "bbb"))
        );
    }
}
