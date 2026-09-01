//! Which library each import comes from, and the ids that encode that in a module.
//!
//! A module does not import `sceKernelWrite` from `libkernel` by name. It imports NID
//! `<hash>` from library *id* 4, module *id* 4, and separately declares that library 4
//! is called `libkernel`. The ids are the whole mechanism, and an id with nothing
//! declared against it resolves to nothing at all - which is how a module that looks
//! complete ends up resolving none of its imports.
//!
//! # The association cannot be read off the linked file
//!
//! A `.dynsym` entry records that a symbol is undefined. It does not record who is
//! expected to define it - that is what the dynamic linker is for, and on an ordinary
//! system it is answered by searching every `DT_NEEDED` library in turn. The console
//! format answers it at build time instead, so the information has to come from the
//! source.
//!
//! It comes from the host build: `obscene-host --symbols` walks the census tables and
//! the import table and prints one `library symbol` line each. See `src/imports.c`.
//!
//! # A missing entry is an error, not a default
//!
//! The first version of this defaulted to id zero, and produced a module whose every
//! symbol was `<nid>#A#A` - four hundred imports all claiming to come from a library
//! that was never declared. It loaded, and resolved nothing. Defaulting turns a missing
//! line in a manifest into a module that fails much later and says nothing about why,
//! so there is no default.

use std::collections::BTreeMap;

/// The sigil marking a symbol whose name **is already the identifier**.
///
/// Most imports are named and hashed. Some are not: firmware modules export around a million
/// identifiers whose names nobody outside the vendor holds, and an identifier is perfectly
/// importable without one - the import *is* the identifier, and a name only ever existed to
/// compute it.
///
/// Such a symbol cannot be written as a C identifier, so it reaches the linker through an
/// assembler label, and this sigil is what tells the difference here. `$` is legal in an ELF
/// symbol name and illegal in a C one, so no real name can collide with the marker.
///
/// It lives beside the manifest rather than beside the hash because it is a convention of
/// *this project's* symbol tables, not a fact about the format. `selfish` takes an identifier
/// and does not care where it came from.
pub const PREENCODED: char = '$';

/// The manifest, with ids assigned.
#[derive(Debug, Default)]
pub struct Imports {
    /// Symbol name to the library it comes from.
    symbols: BTreeMap<String, String>,
    /// The imported libraries, in declaration order.
    libraries: Vec<Library>,
}

/// One imported library, and the ids that name it.
#[derive(Debug, Clone)]
pub struct Library {
    /// The name, exactly as the platform spells it.
    pub name: String,
    /// The library id, which is what a symbol's first suffix encodes.
    pub id: u16,
    /// The module id, which is what a symbol's second suffix encodes.
    ///
    /// Libraries and modules are separate namespaces: a module is the file that gets
    /// loaded, a library is a namespace of exports inside it, and one file can hold
    /// several. We declare one module per library, which holds for every library here -
    /// `libkernel` the module exports `libkernel` the library.
    ///
    /// **The ids are still not the same number**, because the two sequences do not start
    /// in the same place: modules from one, libraries from zero. See [`Imports::parse`].
    pub module_id: u16,
}

/// What went wrong.
#[derive(Debug)]
pub enum ImportsError {
    /// A line was not `library symbol`.
    BadLine(usize, String),
    /// Undefined symbols with no library.
    ///
    /// Carries every one of them rather than the first: a manifest that is out of date
    /// is usually out of date by more than one line, and finding that out one build at
    /// a time is slow. Each carries the library the mined corpus places it in, if any, so
    /// the error can suggest the line to add rather than only naming what is missing.
    Unplaced(Vec<Missing>),
    /// More libraries than an id can name.
    TooManyLibraries(usize),
}

/// A symbol the manifest does not place, and where the mined corpus says it lives.
#[derive(Debug, Clone)]
pub struct Missing {
    /// The symbol with no declared library.
    pub symbol: String,
    /// The library the mined corpus lists for it, preferring a spelling the manifest already
    /// uses; `None` when the corpus does not know the name and it must be placed by hand.
    pub library: Option<String>,
}

/// Index the mined corpus by symbol name to the libraries it lists.
///
/// The columns are `name libraries …`; a `-` in the library column is a flat catalogue that
/// carried no library, and is skipped. Kept separate so a caller reads the file once and this
/// stays testable without one. Used only to *suggest* a library on a missing-library error - a
/// match still has to be pasted into `src/imports.c` by hand, so nothing here can place a symbol
/// wrongly, only unhelpfully.
#[must_use]
pub fn corpus_libraries(text: &str) -> BTreeMap<String, Vec<String>> {
    let mut map: BTreeMap<String, Vec<String>> = BTreeMap::new();
    for line in text.lines() {
        if line.starts_with('#') {
            continue;
        }
        let mut parts = line.split_whitespace();
        let (Some(name), Some(libraries)) = (parts.next(), parts.next()) else {
            continue;
        };
        if libraries == "-" {
            continue;
        }
        let variants = map.entry(name.to_owned()).or_default();
        for library in libraries.split(',') {
            if !library.is_empty() && !variants.iter().any(|held| held == library) {
                variants.push(library.to_owned());
            }
        }
    }
    map
}

impl Imports {
    /// Parses `library symbol` lines and assigns ids.
    ///
    /// **Module ids start at 1 and library ids at 0**, which is not symmetry missed but
    /// the two namespaces starting in different places. Module zero is the executable
    /// itself, so an import naming it is a self-reference that resolves to something
    /// rather than failing. Library zero belongs to a module's own export library, and an
    /// executable has none - so for an executable the first import library is zero, and
    /// numbering from one leaves the table a loader indexes empty at the front. (D217)
    ///
    /// `exports_a_library` is what decides which of those two it is, and the manifest cannot
    /// know it: a shared library declares an export library and it takes id zero, so its
    /// imports begin at one. Getting this wrong is not a diagnosable failure - either two
    /// libraries claim id zero, or nothing does. (D221)
    pub fn parse(text: &str, exports_a_library: bool) -> Result<Self, ImportsError> {
        let mut out = Self::default();
        let mut order: Vec<String> = Vec::new();

        for (number, line) in text.lines().enumerate() {
            let line = line.trim();
            if line.is_empty() {
                continue;
            }
            let mut parts = line.split_whitespace();
            let (Some(library), Some(symbol), None) = (parts.next(), parts.next(), parts.next())
            else {
                return Err(ImportsError::BadLine(
                    number.saturating_add(1),
                    line.to_owned(),
                ));
            };
            if !order.iter().any(|name| name == library) {
                order.push(library.to_owned());
            }
            out.symbols.insert(symbol.to_owned(), library.to_owned());
        }

        // Libraries are numbered from zero and modules from one, and the two counts are not
        // the same number. A launching executable declares its import libraries at ids 0..N
        // and its needed modules at ids 1..N - module zero is the executable itself, so
        // nothing else may claim it, while library zero is free because an executable exports
        // no library to put there.
        //
        // This numbered both from one, which left the import-library table with no entry at
        // zero. The loader indexes that table densely from zero and refused the file in
        // `allocate_per_file_info_compact` without ever naming the id. (D217)
        let first_library = u16::from(exports_a_library);
        for (index, name) in order.into_iter().enumerate() {
            let index = u16::try_from(index).map_err(|_| ImportsError::TooManyLibraries(index))?;
            out.libraries.push(Library {
                name,
                id: index.saturating_add(first_library),
                module_id: index.saturating_add(1),
            });
        }
        Ok(out)
    }

    /// The libraries, in declaration order.
    #[must_use]
    pub fn libraries(&self) -> &[Library] {
        &self.libraries
    }

    /// The ids to encode into a symbol's name, or `None` if it has no library.
    #[must_use]
    pub fn ids_for(&self, symbol: &str) -> Option<(u16, u16)> {
        let library = self.symbols.get(symbol)?;
        self.libraries
            .iter()
            .find(|entry| &entry.name == library)
            .map(|entry| (entry.id, entry.module_id))
    }

    /// Fails if any of `symbols` has no library.
    ///
    /// `corpus` is the mined name-to-libraries index ([`corpus_libraries`]); each missing symbol
    /// is looked up there so the error can suggest where it resolves. Pass an empty map to skip
    /// suggestions. The suggested spelling prefers a library the manifest already declares -
    /// the corpus lists both `libKernel` and `libkernel`, and the manifest uses the latter - and
    /// otherwise takes the first the corpus offers.
    pub fn check_covers(
        &self,
        symbols: &[String],
        corpus: &BTreeMap<String, Vec<String>>,
    ) -> Result<(), ImportsError> {
        let missing: Vec<Missing> = symbols
            .iter()
            .filter(|symbol| !self.symbols.contains_key(*symbol))
            .map(|symbol| {
                let library = corpus.get(symbol).and_then(|variants| {
                    variants
                        .iter()
                        .find(|variant| self.libraries.iter().any(|lib| &lib.name == *variant))
                        .or_else(|| variants.first())
                        .cloned()
                });
                Missing {
                    symbol: symbol.clone(),
                    library,
                }
            })
            .collect();
        if missing.is_empty() {
            Ok(())
        } else {
            Err(ImportsError::Unplaced(missing))
        }
    }
}

impl std::fmt::Display for ImportsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::BadLine(number, line) => {
                write!(
                    f,
                    "manifest line {number} is not `library symbol`: {line:?}"
                )
            }
            Self::Unplaced(missing) => {
                write!(
                    f,
                    "{} imported symbol(s) have no library, so a module cannot say where \
                     to resolve them. Add them to src/imports.c, or to a census list in \
                     include/obscene/surface.h if they belong to the known surface:",
                    missing.len()
                )?;
                for item in missing {
                    match &item.library {
                        Some(library) => write!(
                            f,
                            "\n  {}  - the mined corpus places it in {library}; add \
                             {{\"{library}\", \"{}\"}},",
                            item.symbol, item.symbol
                        )?,
                        None => write!(
                            f,
                            "\n  {}  - not in the mined corpus; find its library by hand",
                            item.symbol
                        )?,
                    }
                }
                Ok(())
            }
            Self::TooManyLibraries(count) => {
                write!(f, "{count} libraries is more than a 16-bit id can name")
            }
        }
    }
}

impl std::error::Error for ImportsError {}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    clippy::arithmetic_side_effects,
    reason = "test fixtures build known-size buffers; a panic here is the failure \
              signal, which is the opposite of what these lints guard in the tool"
)]
mod tests {
    use super::{Imports, ImportsError, corpus_libraries};
    use std::collections::BTreeMap;

    const MANIFEST: &str = "libkernel sceKernelWrite\n\
                            libkernel sceKernelRead\n\
                            libSceLibcInternal strlen\n";

    #[test]
    fn each_library_gets_one_id_however_many_symbols_it_has() {
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        assert_eq!(imports.libraries().len(), 2);
        let (write_lib, _) = imports.ids_for("sceKernelWrite").expect("write");
        let (read_lib, _) = imports.ids_for("sceKernelRead").expect("read");
        assert_eq!(write_lib, read_lib);
    }

    #[test]
    fn module_ids_start_at_one_and_library_ids_at_zero() {
        // Module zero is the executable itself, and a symbol encoded against it points at
        // the module's own exports - which resolves to something wrong rather than failing.
        //
        // Library zero is a different matter and is *required* to be used: it belongs to a
        // module's own export library, an executable has none, and leaving it empty gives the
        // loader an import table with a hole at the front. (D217)
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        assert!(
            imports
                .libraries()
                .iter()
                .all(|library| library.module_id >= 1),
            "no import may claim module zero"
        );
        assert_eq!(
            imports.libraries().first().map(|library| library.id),
            Some(0),
            "and the first import library is zero"
        );
        for (n, library) in imports.libraries().iter().enumerate() {
            assert_eq!(
                u64::from(library.id),
                n as u64,
                "library ids are dense from zero"
            );
        }
    }

    #[test]
    fn a_shared_library_starts_its_imports_at_one_because_it_exports_library_zero() {
        // The two cases differ by exactly one, and nothing downstream can tell them apart:
        // an executable that numbers from one leaves the loader's table empty at the front,
        // and a shared library that numbers from zero has two libraries claiming it. (D221)
        let executable = Imports::parse(MANIFEST, false).expect("parse");
        let shared = Imports::parse(MANIFEST, true).expect("parse");
        assert_eq!(executable.libraries().first().map(|l| l.id), Some(0));
        assert_eq!(shared.libraries().first().map(|l| l.id), Some(1));
        // The module numbering does not move. Module zero is the executable itself either way.
        for (one, two) in executable.libraries().iter().zip(shared.libraries()) {
            assert_eq!(one.module_id, two.module_id, "module ids are unaffected");
            assert_eq!(two.id, one.id + 1, "and library ids shift by exactly one");
        }
    }

    #[test]
    fn different_libraries_get_different_ids() {
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        let (kernel, _) = imports.ids_for("sceKernelWrite").expect("kernel");
        let (libc, _) = imports.ids_for("strlen").expect("libc");
        assert_ne!(kernel, libc);
    }

    #[test]
    fn a_symbol_with_no_library_is_not_quietly_given_one() {
        // The bug this module exists to prevent: every symbol defaulting to id zero
        // produced four hundred imports from a library that was never declared.
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        assert!(imports.ids_for("sceKernelOpen").is_none());
    }

    #[test]
    fn every_unplaced_symbol_is_reported_not_just_the_first() {
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        let wanted = ["strlen".to_owned(), "fopen".to_owned(), "abs".to_owned()];
        let error = imports
            .check_covers(&wanted, &BTreeMap::new())
            .expect_err("should fail");
        match error {
            ImportsError::Unplaced(missing) => assert_eq!(missing.len(), 2),
            other => panic!("wrong error: {other:?}"),
        }
    }

    #[test]
    fn a_covered_set_passes() {
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        imports
            .check_covers(
                &["strlen".to_owned(), "sceKernelRead".to_owned()],
                &BTreeMap::new(),
            )
            .expect("covered");
    }

    #[test]
    fn a_missing_symbol_carries_the_corpus_library_preferring_the_declared_spelling() {
        let imports = Imports::parse(MANIFEST, false).expect("parse");
        // The corpus lists both spellings; the manifest declares `libkernel`, so that is the
        // one suggested - a paste-ready line rather than a choice left to the reader.
        let corpus = corpus_libraries("sceKernelMkdir libKernel,libkernel 0xdead src fn\n");
        let error = imports
            .check_covers(&["sceKernelMkdir".to_owned()], &corpus)
            .expect_err("should fail");
        match error {
            ImportsError::Unplaced(missing) => {
                let first = missing.first().expect("one missing");
                assert_eq!(first.symbol, "sceKernelMkdir");
                assert_eq!(first.library.as_deref(), Some("libkernel"));
            }
            other => panic!("wrong error: {other:?}"),
        }
    }

    #[test]
    fn a_malformed_line_is_rejected_rather_than_half_read() {
        let error = Imports::parse("libkernel\n", false).expect_err("should fail");
        assert!(matches!(error, ImportsError::BadLine(1, _)));
    }

    #[test]
    fn declaration_order_fixes_the_ids() {
        // Ids must not depend on map iteration order: a module's symbol suffixes and
        // its library table have to agree, and a build that shuffles them produces a
        // different wrong answer each time.
        let first = Imports::parse(MANIFEST, false).expect("parse");
        let second = Imports::parse(MANIFEST, false).expect("parse");
        let ids = |imports: &Imports| {
            imports
                .libraries()
                .iter()
                .map(|library| (library.name.clone(), library.id))
                .collect::<Vec<_>>()
        };
        assert_eq!(ids(&first), ids(&second));
    }
}
